// include/kernel/service_locator.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    KRN_DISPLAY = 0,    // DisplayAPI wrapper (VGA, VESA, or SDL)
    KRN_INPUT,          // InputAPI wrapper (PS/2, Serial, or Keyboard)
    KRN_LOGGER,         // LoggerAPI (Serial UART)
    KRN_FORMATTER,      // FormatterAPI (snprintf, zero-libc)
    KRN_PARSER,         // Pratt Parser API
    KRN_APP_MANAGER,    // Window & App manager (WAL)
    KRN_COUNT = 16      // Strictly capped to fit in uint16_t masks
} ServiceId;

typedef struct {
    const char* name;
    void*       instance;
    uint8_t     active;
} RegistryEntry;

typedef struct {
    RegistryEntry services[KRN_COUNT];
    uint16_t      active_mask; // Bitmask of live services
} KernelRegistry;

extern KernelRegistry g_registry;

static inline void* kernel_get_service(ServiceId id) {
    if (id >= KRN_COUNT) return (void*)0;
    if (!(g_registry.active_mask & (1U << id))) {
        // Fall back to stable null-proxies to prevent page faults on missing services
        extern const void* const g_null_proxies[KRN_COUNT];
        return (void*)g_null_proxies[id];
    }
    return g_registry.services[id].instance;
}
