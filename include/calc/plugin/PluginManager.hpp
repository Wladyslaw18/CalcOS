#ifndef CALC_PLUGIN_MANAGER_HPP
#define CALC_PLUGIN_MANAGER_HPP

#include "IPlugin.hpp"
#include "PluginContext.hpp"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace calc {

// Struct to store a small list of raw dependency names during registration
struct RawDependencyList {
    const char* names[8];
    size_t count;
};

// Metadata for a single plugin. Aligned to 64 bytes to prevent cache line splitting.
struct alignas(64) PluginMetadata {
    IPlugin* instance;
    uint64_t dependency_mask; // Bitmask of dependencies (index mapping)
    const char* name;
    LoadPhase phase;
};

template <size_t MaxPlugins = 32>
class PluginManager {
public:
    static_assert(MaxPlugins <= 64, "Bitmask optimization currently supports up to 64 plugins.");

    PluginManager() : m_plugin_count(0), m_resolved(false) {
        memset(m_metadata, 0, sizeof(m_metadata));
        memset(m_raw_deps, 0, sizeof(m_raw_deps));
    }

    ~PluginManager() = default;

    // Registers a plugin (dependencies are read directly from the plugin instance)
    bool RegisterPlugin(IPlugin* plugin) {
        if (!plugin || m_plugin_count >= MaxPlugins) {
            return false;
        }

        size_t idx = m_plugin_count++;
        m_metadata[idx].instance = plugin;
        m_metadata[idx].name = plugin->GetName();
        m_metadata[idx].phase = plugin->GetPhase();
        m_metadata[idx].dependency_mask = 0;

        m_contexts[idx].Initialize(plugin->GetName());
        m_contexts[idx].SetAPILookup([](void* ctx, const char* id) -> void* {
            return static_cast<PluginManager*>(ctx)->GetPluginAPI(id);
        }, this);

        size_t dep_count = plugin->GetDependencyCount();
        m_raw_deps[idx].count = dep_count < 8 ? dep_count : 8;
        for (size_t i = 0; i < m_raw_deps[idx].count; ++i) {
            m_raw_deps[idx].names[i] = plugin->GetDependency(i);
        }

        m_resolved = false;
        return true;
    }

    // Resolves dependencies and performs topological sorting across all staggered phases
    bool ResolveAndSort() {
        // 1. Resolve names to bitmask indexes
        for (size_t i = 0; i < m_plugin_count; ++i) {
            m_metadata[i].dependency_mask = 0;
            for (size_t d = 0; d < m_raw_deps[i].count; ++d) {
                const char* dep_name = m_raw_deps[i].names[d];
                int dep_idx = FindPluginIndex(dep_name);
                if (dep_idx < 0) {
                    return false; // Dependency name not registered!
                }
                m_metadata[i].dependency_mask |= (1ULL << dep_idx);
            }
        }

        // 2. Perform Staggered Topological Sort (Phase by Phase)
        size_t sorted_count = 0;
        uint64_t loaded_mask = 0; // Bitmask of loaded plugins

        for (uint8_t p = 0; p < (uint8_t)LoadPhase::COUNT; ++p) {
            LoadPhase current_phase = (LoadPhase)p;

            // Collect all plugins belonging to this phase
            uint64_t phase_mask = 0;
            for (size_t i = 0; i < m_plugin_count; ++i) {
                if (m_metadata[i].phase == current_phase) {
                    phase_mask |= (1ULL << i);
                }
            }

            uint64_t unresolved_in_phase = phase_mask;
            while (unresolved_in_phase != 0) {
                uint64_t progress_mask = 0;
                for (size_t i = 0; i < m_plugin_count; ++i) {
                    if (unresolved_in_phase & (1ULL << i)) {
                        // Check if dependencies are satisfied.
                        // Since dependencies must be loaded before, their bitmasks must be a subset of loaded_mask.
                        if ((m_metadata[i].dependency_mask & loaded_mask) == m_metadata[i].dependency_mask) {
                            m_sorted_order[sorted_count++] = i;
                            progress_mask |= (1ULL << i);
                        }
                    }
                }
                if (progress_mask == 0) {
                    return false; // Circular dependency detected!
                }
                loaded_mask |= progress_mask;
                unresolved_in_phase &= ~progress_mask;
            }
        }

        m_resolved = (sorted_count == m_plugin_count);
        return m_resolved;
    }

    // Executes the staggered initialization phase
    void InitializePlugins() {
        if (!m_resolved && !ResolveAndSort()) {
            return;
        }

        // Initialize in topological order
        for (size_t i = 0; i < m_plugin_count; ++i) {
            size_t idx = m_sorted_order[i];
            m_metadata[idx].instance->OnInit(&m_contexts[idx]);
        }
    }

    // Executes the loading phase
    void LoadPlugins() {
        if (!m_resolved && !ResolveAndSort()) {
            return;
        }

        // Load in topological order
        for (size_t i = 0; i < m_plugin_count; ++i) {
            size_t idx = m_sorted_order[i];
            m_metadata[idx].instance->OnLoad(&m_contexts[idx]);
        }
    }

    // Executes the shutdown phase (in reverse topological order)
    void ShutdownPlugins() {
        if (!m_resolved) {
            return;
        }

        // Shutdown in reverse topological order (clean teardown)
        for (size_t i = m_plugin_count; i > 0; --i) {
            size_t idx = m_sorted_order[i - 1];
            m_metadata[idx].instance->OnShutdown(&m_contexts[idx]);
            m_contexts[idx].Reset(); // Resource cleanup (prevent leaks!)
        }
    }

    // C-compatible lookup callback to feed CalculatorState
    static void* CustomFunctionLookupCallback(void* ctx, const char* name) {
        return static_cast<PluginManager*>(ctx)->LookupCustomFunction(name);
    }

    // Looks up a custom registered function by name in all registered plugin contexts
    void* LookupCustomFunction(const char* name) {
        for (size_t i = 0; i < m_plugin_count; ++i) {
            PluginFuncPtr fn = m_contexts[i].LookupFunction(name);
            if (fn) {
                return (void*)fn;
            }
        }
        return nullptr;
    }

    // API Service Mesh Lookup
    void* GetPluginAPI(const char* id) {
        int idx = FindPluginIndex(id);
        return idx >= 0 ? m_contexts[idx].GetExportedAPI() : nullptr;
    }

    // Helper to fetch sorted index
    size_t GetSortedIndex(size_t i) const {
        return m_sorted_order[i];
    }

    // Helper to get raw plugins list size
    size_t GetPluginCount() const {
        return m_plugin_count;
    }

    // Get metadata by index
    const PluginMetadata& GetMetadata(size_t idx) const {
        return m_metadata[idx];
    }

    // Get plugin context by index
    PluginContext& GetContext(size_t idx) {
        return m_contexts[idx];
    }

private:
    int FindPluginIndex(const char* name) const {
        for (size_t i = 0; i < m_plugin_count; ++i) {
            if (strcmp(m_metadata[i].name, name) == 0) {
                return (int)i;
            }
        }
        return -1;
    }

    PluginMetadata m_metadata[MaxPlugins];
    PluginContext m_contexts[MaxPlugins];
    RawDependencyList m_raw_deps[MaxPlugins];
    size_t m_sorted_order[MaxPlugins];
    size_t m_plugin_count;
    bool m_resolved;
};

} // namespace calc

#endif // CALC_PLUGIN_MANAGER_HPP
