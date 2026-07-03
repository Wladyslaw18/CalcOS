#ifndef CALC_PLUGIN_CONTEXT_HPP
#define CALC_PLUGIN_CONTEXT_HPP

#include "../../../Kernel/State/NumericValue.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

namespace calc {

// Typedef for custom plugin function
typedef ComplexValue (*PluginFuncPtr)(ComplexValue);

struct RegisteredFunction {
    char name[16];
    PluginFuncPtr fn;
};

struct RegisteredVariable {
    char key[64];
    double value;
};

class PluginContext {
public:
    PluginContext() 
        : m_plugin_id(nullptr), m_func_count(0), m_var_count(0), m_exported_api(nullptr), m_api_lookup(nullptr), m_lookup_ctx(nullptr) {}

    void Initialize(const char* plugin_id) { m_plugin_id = plugin_id; }

    ~PluginContext() = default;

    // Secure database prefix wrapper (mimicking the namespaced js DB key storage)
    void SetDBValue(const char* key, double val) {
        char full_key[64];
        // Form: plugin:plugin_id:key
        snprintf(full_key, sizeof(full_key), "plugin:%s:%s", m_plugin_id, key);
        
        for (size_t i = 0; i < m_var_count; ++i) {
            if (strcmp(m_variables[i].key, full_key) == 0) {
                m_variables[i].value = val;
                return;
            }
        }
        if (m_var_count < 16) {
            strncpy(m_variables[m_var_count].key, full_key, sizeof(m_variables[m_var_count].key));
            m_variables[m_var_count].value = val;
            m_var_count++;
        }
    }

    double GetDBValue(const char* key, double default_val = 0.0) const {
        char full_key[64];
        snprintf(full_key, sizeof(full_key), "plugin:%s:%s", m_plugin_id, key);
        
        for (size_t i = 0; i < m_var_count; ++i) {
            if (strcmp(m_variables[i].key, full_key) == 0) {
                return m_variables[i].value;
            }
        }
        return default_val;
    }

    // Dynamic Function registration (mimicking the command registry hooks)
    bool RegisterFunction(const char* name, PluginFuncPtr fn) {
        if (m_func_count >= 8) return false;
        
        strncpy(m_functions[m_func_count].name, name, sizeof(m_functions[m_func_count].name));
        m_functions[m_func_count].fn = fn;
        m_func_count++;
        return true;
    }

    // Service mesh / API sharing
    void ExportAPI(void* api_ptr) {
        m_exported_api = api_ptr;
    }

    void* RequireAPI(const char* plugin_id) {
        return m_api_lookup ? m_api_lookup(m_lookup_ctx, plugin_id) : nullptr;
    }

    // Resource tracking cleanup (fired on reload/shutdown to prevent leaks)
    void Reset() {
        m_func_count = 0;
        m_var_count = 0;
        m_exported_api = nullptr;
    }

    // Lookup functions
    PluginFuncPtr LookupFunction(const char* name) const {
        for (size_t i = 0; i < m_func_count; ++i) {
            if (strcmp(m_functions[i].name, name) == 0) {
                return m_functions[i].fn;
            }
        }
        return nullptr;
    }

    void* GetExportedAPI() const { return m_exported_api; }
    const char* GetPluginId() const { return m_plugin_id; }

    // Callbacks for API mesh
    typedef void* (*APILookupCallback)(void* ctx, const char* id);
    void SetAPILookup(APILookupCallback cb, void* ctx) {
        m_api_lookup = cb;
        m_lookup_ctx = ctx;
    }

private:
    const char* m_plugin_id;
    RegisteredFunction m_functions[8];
    size_t m_func_count;
    
    RegisteredVariable m_variables[16];
    size_t m_var_count;

    void* m_exported_api;
    APILookupCallback m_api_lookup;
    void* m_lookup_ctx;
};

} // namespace calc

#endif // CALC_PLUGIN_CONTEXT_HPP
