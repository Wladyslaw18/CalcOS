#ifndef CALC_PLUGIN_LOADER_HPP
#define CALC_PLUGIN_LOADER_HPP

#include "PluginManager.hpp"
#include <filesystem>
#include <string>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef HMODULE LibHandle;
#define GET_SYM(handle, name) GetProcAddress(handle, name)
#define FREE_LIB(handle) FreeLibrary(handle)
#define LIB_SUFFIX ".dll"
#else
#include <dlfcn.h>
typedef void* LibHandle;
#define GET_SYM(handle, name) dlsym(handle, name)
#define FREE_LIB(handle) dlclose(handle)
#define LIB_SUFFIX ".so"
#endif

namespace calc {

template <size_t MaxPlugins = 32>
class PluginLoader {
public:
    PluginLoader(PluginManager<MaxPlugins>& manager) 
        : m_manager(manager), m_loaded_count(0) {
        memset(m_handles, 0, sizeof(m_handles));
    }

    ~PluginLoader() {
        UnloadAll();
    }

    // Scans a directory and dynamically loads all plugins inside it
    size_t LoadDirectory(const std::filesystem::path& dir_path) {
        if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
            return 0;
        }

        size_t loaded = 0;
        for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == LIB_SUFFIX) {
                if (LoadPluginFile(entry.path())) {
                    loaded++;
                }
            }
        }
        return loaded;
    }

    bool LoadPluginFile(const std::filesystem::path& file_path) {
        if (m_loaded_count >= MaxPlugins) {
            return false;
        }

        LibHandle handle;
#if defined(_WIN32)
        std::wstring wpath = file_path.wstring();
        handle = LoadLibraryW(wpath.c_str());
#else
        handle = dlopen(file_path.string().c_str(), RTLD_NOW);
#endif

        if (!handle) {
            return false;
        }

        // Lookup entry point function "calc_get_plugin"
        typedef IPlugin* (*GetPluginFunc)();
        GetPluginFunc get_plugin = (GetPluginFunc)GET_SYM(handle, "calc_get_plugin");

        if (!get_plugin) {
            FREE_LIB(handle);
            return false;
        }

        IPlugin* plugin = get_plugin();
        if (!plugin) {
            FREE_LIB(handle);
            return false;
        }

        // Register to manager
        if (m_manager.RegisterPlugin(plugin)) {
            m_handles[m_loaded_count++] = handle;
            return true;
        }

        FREE_LIB(handle);
        return false;
    }

    void UnloadAll() {
        if (m_loaded_count == 0) {
            return; // Prevent double-unload access violation!
        }
        m_manager.ShutdownPlugins(); // Ensure hooks execute and contexts reset before library unload!
        for (size_t i = 0; i < m_loaded_count; ++i) {
            if (m_handles[i]) {
                FREE_LIB(m_handles[i]);
                m_handles[i] = nullptr;
            }
        }
        m_loaded_count = 0;
    }

private:
    PluginManager<MaxPlugins>& m_manager;
    LibHandle m_handles[MaxPlugins];
    size_t m_loaded_count;
};

} // namespace calc

#endif // CALC_PLUGIN_LOADER_HPP
