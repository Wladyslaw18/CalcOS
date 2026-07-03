#include "include/calc/plugin/IPlugin.hpp"
#include <string.h>

#if defined(_WIN32)
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// Custom function: triples the value
static ComplexValue plugin_custom_triple(ComplexValue z) {
    ComplexValue res;
    res.real = z.real * 3.0;
    res.imag = z.imag * 3.0;
    return res;
}

struct SamplePlugin : calc::IPlugin {
    const char* GetName() const override { return "SampleDynamic"; }
    calc::LoadPhase GetPhase() const override { return calc::LoadPhase::Features; }
    
    size_t GetDependencyCount() const override { return 0; }
    const char* GetDependency(size_t index) const override { return nullptr; }

    void OnInit(calc::PluginContext* context) override {
        context->SetDBValue("loaded_status", 100.0);
    }

    void OnLoad(calc::PluginContext* context) override {
        context->RegisterFunction("triple", plugin_custom_triple);
    }

    void OnShutdown(calc::PluginContext* context) override {
        // Nothing to do
    }
};

static SamplePlugin g_plugin_instance;

PLUGIN_EXPORT calc::IPlugin* calc_get_plugin() {
    return &g_plugin_instance;
}
