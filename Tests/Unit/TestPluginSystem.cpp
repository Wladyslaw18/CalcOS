#include "include/calc/plugin/PluginManager.hpp"
#include "include/calc/plugin/PluginLoader.hpp"
#include "include/calc/plugin/IPlugin.hpp"
#include "Kernel/State/CalculatorState.h"
#include "Application/Input/Experimental/ShuntingYard.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <filesystem>

extern "C" double parse_expression(const char* source, CalculatorState* state, bool* success);

static int g_event_log_count = 0;
static const char* g_event_log[64];

static void log_event(const char* event) {
    if (g_event_log_count < 64) {
        g_event_log[g_event_log_count++] = event;
    }
}

// A simple API struct that CorePlugin will export
struct CoreAPI {
    int (*add_numbers)(int a, int b);
};

static int core_add_numbers(int a, int b) {
    return a + b;
}

static CoreAPI g_core_api = { core_add_numbers };

// Custom mathematical function registered by a plugin (doubles the input value)
static ComplexValue plugin_custom_double(ComplexValue z) {
    ComplexValue res;
    res.real = z.real * 2.0;
    res.imag = z.imag * 2.0;
    return res;
}

struct TestPlugin : calc::IPlugin {
    TestPlugin(const char* name, calc::LoadPhase phase) 
        : m_name(name), m_phase(phase) {}

    const char* GetName() const override { return m_name; }
    calc::LoadPhase GetPhase() const override { return m_phase; }

    size_t GetDependencyCount() const override {
        if (strcmp(m_name, "CoreB") == 0) return 1;
        if (strcmp(m_name, "ServiceA") == 0) return 1;
        if (strcmp(m_name, "FeatureA") == 0) return 1;
        if (strcmp(m_name, "PluginX") == 0) return 1;
        if (strcmp(m_name, "PluginY") == 0) return 1;
        return 0;
    }

    const char* GetDependency(size_t index) const override {
        if (strcmp(m_name, "CoreB") == 0 && index == 0) return "CoreA";
        if (strcmp(m_name, "ServiceA") == 0 && index == 0) return "CoreB";
        if (strcmp(m_name, "FeatureA") == 0 && index == 0) return "ServiceA";
        if (strcmp(m_name, "PluginX") == 0 && index == 0) return "PluginY";
        if (strcmp(m_name, "PluginY") == 0 && index == 0) return "PluginX";
        return nullptr;
    }

    void OnInit(calc::PluginContext* context) override {
        char buf[64];
        sprintf(buf, "%s_Init", m_name);
        log_event(strdup(buf));
        
        // Test Namespaced DB Set
        context->SetDBValue("init_flag", 42.0);
        
        // Export API if CoreA
        if (strcmp(m_name, "CoreA") == 0) {
            context->ExportAPI(&g_core_api);
        }
    }

    void OnLoad(calc::PluginContext* context) override {
        char buf[64];
        sprintf(buf, "%s_Load", m_name);
        log_event(strdup(buf));

        // Test Namespaced DB value retrieval
        double flag = context->GetDBValue("init_flag");
        assert(flag == 42.0);

        // Test API Consumption if CoreB (depends on CoreA)
        if (strcmp(m_name, "CoreB") == 0) {
            CoreAPI* api = static_cast<CoreAPI*>(context->RequireAPI("CoreA"));
            assert(api != nullptr);
            int res = api->add_numbers(10, 20);
            assert(res == 30);
            log_event("CoreB_Consumed_CoreA_API");
        }

        // Test Dynamic Function Registration if FeatureA
        if (strcmp(m_name, "FeatureA") == 0) {
            bool reg_ok = context->RegisterFunction("dbl", plugin_custom_double);
            assert(reg_ok);
        }
    }

    void OnShutdown(calc::PluginContext* context) override {
        char buf[64];
        sprintf(buf, "%s_Shutdown", m_name);
        log_event(strdup(buf));
    }

private:
    const char* m_name;
    calc::LoadPhase m_phase;
};

void run_basic_plugin_tests() {
    printf("--- Running Basic Plugin Manager Tests ---\n");
    g_event_log_count = 0;

    calc::PluginManager<16> pm;

    TestPlugin coreA("CoreA", calc::LoadPhase::Core);
    TestPlugin coreB("CoreB", calc::LoadPhase::Core);
    TestPlugin serviceA("ServiceA", calc::LoadPhase::Services);
    TestPlugin featureA("FeatureA", calc::LoadPhase::Features);

    // Register out of order to verify sorting is doing real work
    assert(pm.RegisterPlugin(&featureA));
    assert(pm.RegisterPlugin(&serviceA));
    assert(pm.RegisterPlugin(&coreB));
    assert(pm.RegisterPlugin(&coreA));

    // Resolve dependencies and sort topologically across phases
    bool sort_success = pm.ResolveAndSort();
    assert(sort_success);
    printf("Topological sort resolved successfully!\n");

    // Check topological sorted indices
    // Order must be: CoreA (3) -> CoreB (2) -> ServiceA (1) -> FeatureA (0)
    assert(strcmp(pm.GetMetadata(pm.GetSortedIndex(0)).name, "CoreA") == 0);
    assert(strcmp(pm.GetMetadata(pm.GetSortedIndex(1)).name, "CoreB") == 0);
    assert(strcmp(pm.GetMetadata(pm.GetSortedIndex(2)).name, "ServiceA") == 0);
    assert(strcmp(pm.GetMetadata(pm.GetSortedIndex(3)).name, "FeatureA") == 0);
    printf("Topological order verified: CoreA -> CoreB -> ServiceA -> FeatureA\n");

    // Fire lifecycle hooks (Init and Load)
    pm.InitializePlugins();
    pm.LoadPlugins();

    // Verify Namespaced DB prefix separation
    // FeatureA (idx 0 in registration) should NOT see CoreA's init_flag (it uses prefix "plugin:FeatureA:init_flag" vs "plugin:CoreA:init_flag")
    assert(pm.GetContext(0).GetDBValue("init_flag") == 0.0);
    
    // CoreA (idx 3 in registration) should have flag 42
    assert(pm.GetContext(3).GetDBValue("init_flag") == 42.0);
    printf("Namespaced database prefix separation verified!\n");

    // Verify event logs & API consumption
    assert(strcmp(g_event_log[0], "CoreA_Init") == 0);
    assert(strcmp(g_event_log[1], "CoreB_Init") == 0);
    assert(strcmp(g_event_log[2], "ServiceA_Init") == 0);
    assert(strcmp(g_event_log[3], "FeatureA_Init") == 0);

    assert(strcmp(g_event_log[4], "CoreA_Load") == 0);
    assert(strcmp(g_event_log[5], "CoreB_Load") == 0);
    assert(strcmp(g_event_log[6], "CoreB_Consumed_CoreA_API") == 0);
    printf("API service mesh resolved and consumed successfully!\n");

    // Test Dynamic Function execution in calculator state via RPN parser
    printf("\n--- Running Dynamic Plugin Function Engine Tests ---\n");
    CalculatorState state;
    memset(&state, 0, sizeof(state));
    state.custom_lookup_ctx = &pm;
    state.custom_lookup = calc::PluginManager<16>::CustomFunctionLookupCallback;

    // Verify custom function lookup resolves correctly
    void* lookup_res = state.custom_lookup(state.custom_lookup_ctx, "dbl");
    assert(lookup_res != nullptr);
    assert(lookup_res == (void*)plugin_custom_double);
    printf("Dynamic custom function lookup resolved to plugin function pointer successfully!\n");

    // Compile and evaluate postfix RPN expression "5 dbl"
    RPNQueue rpn_queue;
    bool compile_ok = parse_rpn(&state, "5 dbl", &rpn_queue);
    assert(compile_ok);
    printf("RPN expression compiled with custom function dbl!\n");

    ComplexValue rpn_res = evaluate_rpn(&state, &rpn_queue);
    assert(rpn_res.real == 10.0);
    assert(rpn_res.imag == 0.0);
    printf("RPN expression evaluation succeeded: 5 dbl = 10!\n");

    // Compile and evaluate infix expression "dbl(3+4i)"
    RPNQueue infix_queue;
    bool compile_infix_ok = infix_to_rpn(&state, "dbl(3+4i)", &infix_queue);
    assert(compile_infix_ok);
    printf("Infix expression compiled with custom function dbl!\n");

    ComplexValue infix_res = evaluate_rpn(&state, &infix_queue);
    assert(infix_res.real == 6.0);
    assert(infix_res.imag == 8.0);
    printf("Infix expression evaluation succeeded: dbl(3+4i) = 6+8i!\n");

    // Compile and evaluate via main Pratt parser (parse_expression)
    bool pratt_success = false;
    double pratt_res = parse_expression("dbl(10)", &state, &pratt_success);
    assert(pratt_success);
    assert(pratt_res == 20.0);
    printf("Main Pratt parser evaluation succeeded: dbl(10) = 20!\n");

    // Shutdown and verify reverse topological ordering (teardown starts with Features)
    pm.ShutdownPlugins();
    assert(strcmp(g_event_log[8], "FeatureA_Shutdown") == 0);
    assert(strcmp(g_event_log[9], "ServiceA_Shutdown") == 0);
    assert(strcmp(g_event_log[10], "CoreB_Shutdown") == 0);
    assert(strcmp(g_event_log[11], "CoreA_Shutdown") == 0);

    // Verify resource tracking cleanup: Context function list must be reset to 0!
    assert(pm.GetContext(0).LookupFunction("dbl") == nullptr);
    printf("Reverse teardown cleanup and dynamic function deregistration verified successfully!\n");

    // Clean up allocated logs
    for (int i = 0; i < g_event_log_count; ++i) {
        free((void*)g_event_log[i]);
    }
}

void run_circular_dependency_tests() {
    printf("\n--- Running Circular Dependency Failure Tests ---\n");
    calc::PluginManager<16> pm;

    TestPlugin pluginX("PluginX", calc::LoadPhase::Core);
    TestPlugin pluginY("PluginY", calc::LoadPhase::Core);

    assert(pm.RegisterPlugin(&pluginX));
    assert(pm.RegisterPlugin(&pluginY));

    bool sort_success = pm.ResolveAndSort();
    assert(!sort_success);
    printf("Circular dependency successfully blocked load sequence!\n");
}

void run_dynamic_file_loader_tests() {
    printf("\n--- Running Dynamic File Loader Tests ---\n");
    
    calc::PluginManager<16> pm;
    calc::PluginLoader<16> loader(pm);

    // Find directory where the compiled shared plugin was staged
    std::filesystem::path plugins_dir = "./plugins";
    if (!std::filesystem::exists(plugins_dir)) {
        plugins_dir = "./build/bin/Release/plugins";
    }
    if (!std::filesystem::exists(plugins_dir)) {
        plugins_dir = "./build/bin/Debug/plugins";
    }
    if (!std::filesystem::exists(plugins_dir)) {
        plugins_dir = "./build/bin/plugins";
    }

    printf("Scanning plugins directory: %s\n", plugins_dir.string().c_str());

    size_t loaded = loader.LoadDirectory(plugins_dir);
    assert(loaded > 0);
    printf("Successfully loaded %zu dynamic plugin file(s)!\n", loaded);

    // Verify metadata
    assert(strcmp(pm.GetMetadata(0).name, "SampleDynamic") == 0);
    printf("Dynamic plugin metadata assertions passed!\n");

    // Run dynamic plugin lifecycle
    pm.InitializePlugins();
    pm.LoadPlugins();

    // Verify Namespaced DB status key was written by OnInit in SampleDynamic
    assert(pm.GetContext(0).GetDBValue("loaded_status") == 100.0);
    printf("Dynamic plugin Namespaced DB initialization check passed!\n");

    // Verify dynamic custom function works globally in the main Pratt parser
    CalculatorState state;
    memset(&state, 0, sizeof(state));
    state.custom_lookup_ctx = &pm;
    state.custom_lookup = calc::PluginManager<16>::CustomFunctionLookupCallback;

    bool success = false;
    double pratt_res = parse_expression("triple(7)", &state, &success);
    assert(success);
    assert(pratt_res == 21.0);
    printf("Dynamic custom function Pratt evaluation succeeded: triple(7) = 21!\n");

    // Cleanup handles
    loader.UnloadAll();
    printf("Dynamic plugin unloaded and resources cleared successfully!\n");
}

#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
#if defined(_MSC_VER)
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_error_mode(_OUT_TO_STDERR);
#endif
    printf("DEBUG: main starting...\n");
    printf("==========================================\n");
    printf("    PLUGIN LOADER SYSTEM VERIFICATION     \n");
    printf("==========================================\n\n");

    printf("DEBUG: running basic plugin tests...\n");
    run_basic_plugin_tests();
    printf("DEBUG: running circular dependency tests...\n");
    run_circular_dependency_tests();
    printf("DEBUG: running dynamic file loader tests...\n");
    run_dynamic_file_loader_tests();

    printf("\n==========================================\n");
    printf("   SUCCESS: 100%% OF PLUGIN TESTS PASSED   \n");
    printf("==========================================\n");

    return 0;
}
