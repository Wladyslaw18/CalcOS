// include/kernel/app.h
#pragma once
#include <stdint.h>
#include "../../include/calc/display.h"

typedef struct {
    uint16_t x, y, w, h; // Physical screen coordinates
} ClipRect;

typedef struct {
    const char* name;
    void*       context; // Pointer to the app's contiguous private state
    
    // Lifecycle Hooks
    void (*init)(void* ctx);
    void (*update)(void* ctx, uint32_t key_event);
    void (*draw)(void* ctx, const DisplayDriver* disp, ClipRect clip);
} Application;

// --- LINK-TIME APPLICATION REGISTRY ---
// We pack Application pointer structures into a custom linker section.
// This allows registering an arbitrary number of apps with zero runtime allocations.

#if defined(_MSC_VER)
    // MSVC custom section naming rules (sorted alphabetically to guarantee start/end boundaries)
    #pragma section("appreg$a", read) // Start marker
    #pragma section("appreg$b", read) // App registry data
    #pragma section("appreg$z", read) // End marker

    #define DECLARE_APP_SECTION __declspec(allocate("appreg$b"))
    #define DECLARE_APP_START   __declspec(allocate("appreg$a"))
    #define DECLARE_APP_END     __declspec(allocate("appreg$z"))
#else
    // GCC / Clang attribute section mapping
    #define DECLARE_APP_SECTION __attribute__((section(".appreg")))
    #define DECLARE_APP_START
    #define DECLARE_APP_END
#endif

// Register macro to link-time compile app definitions
#define REGISTER_APPLICATION(app_var_name) \
    extern const Application app_var_name; \
    DECLARE_APP_SECTION const Application* const p_##app_var_name = &app_var_name;
