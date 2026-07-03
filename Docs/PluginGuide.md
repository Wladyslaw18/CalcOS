# CalcOS Plugin Development Guide

A plugin is a dynamically loaded `.dll` (Windows) or `.so` (Linux) that extends the
calculator's expression language at runtime. Plugins register custom functions that
are callable directly from the parser without recompiling the engine.

---

## The Isolation Protocol

Plugins talk to the engine through a strict three-layer boundary:

```
YOUR PLUGIN DLL  ->  PluginContext API  ->  CalcOS Engine
```

You never touch the engine internals. The `PluginContext` is your entire surface area.

---

## Minimal Plugin Example

```cpp
#include "calc/plugin/IPlugin.hpp"
#include "calc/plugin/PluginContext.hpp"

static double plugin_double(const double* args, int argc, void*) {
    if (argc < 1) return 0.0;
    return args[0] * 2.0;
}

class MyPlugin : public IPlugin {
public:
    const char* GetId()      const override { return "my_plugin"; }
    const char* GetVersion() const override { return "1.0.0"; }
    const char* GetAuthor()  const override { return "you"; }

    std::vector<std::string> GetDependencies() const override { return {}; }

    void OnLoad(PluginContext* ctx) override {
        ctx->RegisterFunction("dbl", plugin_double);
    }
    void OnShutdown(PluginContext* ctx) override {
        ctx->UnregisterFunction("dbl");
    }
};

extern "C" IPlugin* CreatePlugin() { return new MyPlugin(); }
extern "C" void DestroyPlugin(IPlugin* p) { delete p; }
```

Drop the compiled `.dll` into `./plugins/` and it will be auto-discovered on startup.

---

## PluginContext API Reference

| Method | Description |
|---|---|
| `RegisterFunction(name, fn)` | Registers a custom callable in the expression parser |
| `UnregisterFunction(name)` | Removes a custom callable on shutdown |
| `DBSet(key, value)` | Writes to the plugin's namespaced database (`plugin:id:key`) |
| `DBGet(key)` | Reads from the plugin's namespaced database |
| `ExportAPI(name, ptr)` | Publishes a struct pointer to the service mesh |
| `RequireAPI(name)` | Fetches another plugin's exported struct pointer |

---

## Dependency Declaration

If your plugin depends on another, declare it in `GetDependencies()`. The topological
loader resolves the correct boot order automatically:

```cpp
std::vector<std::string> GetDependencies() const override {
    return { "core_plugin" };  // core_plugin loads before this one
}
```

Circular dependencies are detected and blocked at load time. The console will print
the offending cycle and abort the load sequence.

---

## Rules

1. Never store raw pointers across `OnShutdown`. The engine unmaps your DLL after
   calling it - any dangling pointer is an instant `0xC0000005`.
2. Your `RegisterFunction` callback receives stack-allocated `double` args. Do not
   store the pointer. Copy the values you need.
3. DB keys are namespaced automatically. `DBSet("x", 1.0)` writes to
   `plugin:my_plugin:x`. You cannot access another plugin's keys.
