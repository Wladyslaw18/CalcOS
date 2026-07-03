#ifndef CALC_IPLUGIN_HPP
#define CALC_IPLUGIN_HPP

#include <stdint.h>

#include "PluginContext.hpp"

namespace calc {

// Staggered loading phases. Core loads first, then Services, then Features.
enum class LoadPhase : uint8_t {
    Core     = 0,
    Services = 1,
    Features = 2,
    COUNT
};

struct IPlugin {
    virtual ~IPlugin() = default;
    
    // Returns the unique name of the plugin (used for dependency lookup)
    virtual const char* GetName() const = 0;
    
    // Returns the phase in which this plugin should be initialized/loaded
    virtual LoadPhase GetPhase() const = 0;
    
    // Self-describing dependency list
    virtual size_t GetDependencyCount() const = 0;
    virtual const char* GetDependency(size_t index) const = 0;
    
    // Lifecycle Hooks
    virtual void OnInit(PluginContext* context) = 0;
    virtual void OnLoad(PluginContext* context) = 0;
    virtual void OnShutdown(PluginContext* context) = 0;
};

} // namespace calc

#endif // CALC_IPLUGIN_HPP
