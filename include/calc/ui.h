/*
 * File: ui.h
 * Author: W. Kowal
 * Description: UI abstraction layer for the Bare Metal Calculator.
 * 
 * This is the TOP-LEVEL interface for building any UI on top
 * of the calculator engine. It connects:
 * - DisplayDriver (rendering)
 * - InputDriver (keyboard/touch/serial input)
 * - CalculatorState (the actual math engine)
 * - Event loop (render input compute render)
 * 
 * == ARCHITECTURE ==
 * 
 * 
 * Your UI Application
 * (Terminal / SDL / Web / OLED / ..)
 * 
 * UIDriver interface ui.h
 * 
 * DisplayDriver InputDriver display.h, input.h
 * 
 * Calculator Engine (Core)
 * (calc.h, Parser, Kernel operations)
 * 
 * 
 * == HOW TO USE ==
 * 1. Implement a DisplayDriver (or use built-in VGA/VESA)
 * 2. Implement an InputDriver (or use built-in PS2/Serial)
 * 3. Initialize with ui_init()
 * 4. Call ui_run() for the default event loop
 * 5. Or drive it yourself with ui_process_input()/ui_render()
 * 
 * == BUILT-IN IMPLEMENTATIONS ==
 * - UIDriver_VGA (x86_64 VGA text mode + PS2 keyboard)
 * - UIDriver_VESA (x86_64 VESA framebuffer + PS2 keyboard)
 * - UIDriver_Serial (Serial terminal + serial input)
 * 
 * THE FLEX: Most calculators couple UI to engine.
 * THIS you swap the UI without touching a single
 * math function. You're welcome.
 */

#ifndef CALC_UI_H
#define CALC_UI_H

#include "display.h"
#include "input.h"
#include "../../Kernel/State/CalculatorState.h"

// Reason codes for the state observer callback
typedef enum {
    UI_EVENT_NONE = 0,
    UI_EVENT_RESULT,           // New computation result available
    UI_EVENT_ERROR,            // Error occurred (div by zero, syntax, etc.)
    UI_EVENT_INPUT_CHANGE,     // Input buffer changed
    UI_EVENT_MODE_CHANGE,      // Display mode changed (float/int/hex)
    UI_EVENT_HISTORY_CHANGE,   // History was modified
    UI_EVENT_STACK_CHANGE,     // Operand stack changed
    UI_EVENT_CLEAR,            // Display/state was cleared
    UI_EVENT_INIT,             // Calculator was initialized
    UI_EVENT_SHUTDOWN          // Calculator is shutting down
} UIEvent;

// Forward declaration
typedef struct UIDriver UIDriver;

/*
 * UIDriver top-level abstraction that connects the calculator engine
 * to a specific display + input implementation.
 * 
 * This is the MINIMAL interface you need to implement for a custom UI.
 * Everything else (event loop, rendering logic) builds on these primitives.
 */
struct UIDriver {
    // Driver metadata
    const char* name;              // Human-readable name (e.g., "VGA Text Mode")
    void* context;                 // Driver-specific data (implementation-defined)
    
    // Sub-drivers
    const DisplayDriver* display;  // Display rendering interface
    const InputDriver* input;      // Input interface
    
    // Calculator state
    CalculatorState* calc_state;   // Pointer to the active calculator state
    
    // Lifecycle
    // Initialize the UI driver (sets up display, input, and calls calc_init()).
    // Returns 0 on success, non-zero on failure.
    int (*init)(UIDriver* self);
    
    // Shutdown the UI driver (cleanup, restore hardware state).
    void (*deinit)(UIDriver* self);
    
    // Event loop
    // Run the default event loop (render input compute render).
    // This blocks until the user exits.
    int (*run_loop)(UIDriver* self);
    
    // Event handling
    // Process a single input event and update calculator state accordingly.
    // Returns the event kind that was triggered.
    // Call this from your own event loop if you want custom control flow.
    UIEvent (*process_input)(UIDriver* self, uint32_t key);
    
    // Rendering
    // Full re-render of the current calculator state to the display.
    void (*render)(const UIDriver* self);
    
    // Partial update only redraw the regions that changed.
    // More efficient than full render for simple UI changes.
    void (*render_partial)(const UIDriver* self, UIEvent reason);
    
    // State observer
    // Called whenever the calculator state changes.
    // Set this to NULL if you handle state observation yourself.
    void (*on_state_change)(UIDriver* self, UIEvent reason);
};

// ============================================================
// HELPER FUNCTIONS (implemented in Application/UI/ui_core.c)
// ============================================================

// Initialize a UIDriver with default bindings.
// This sets up the calculator state, display, and input drivers.
// The driver struct must have 'display' and 'input' already assigned.
int ui_init(UIDriver* self);

// Process a single key and update the calculator state.
// This is the default implementation that:
// - Appends printable characters to the input buffer
// - Processes backspace/delete/enter
// - Sends the expression to the parser on enter
// - Updates the operand stack and history
UIEvent ui_process_input_default(UIDriver* self, uint32_t key);

// Full re-render of the calculator UI.
// Draws the prompt, input buffer, operand stack, and history.
void ui_render_default(const UIDriver* self);

// Partial update redraw only the affected section.
void ui_render_partial_default(const UIDriver* self, UIEvent reason);

// Default event loop polls input, processes, renders.
int ui_run_loop_default(UIDriver* self);

// ============================================================
// BUILT-IN DRIVER REGISTRATION
// ============================================================

// Register the VGA text-mode UIDriver (x86_64).
// Uses VGA text buffer at 0xB8000 + PS/2 keyboard.
void ui_register_vga_driver(void);

// Register the VESA framebuffer UIDriver (x86_64 with VESA support).
// Requires a VESA mode to be set up first.
void ui_register_vesa_driver(void);

// Register the Serial terminal UIDriver (COM1).
// Uses serial port for both display and input.
void ui_register_serial_driver(void);

// Register the Null driver (no-op, for testing).
void ui_register_null_driver(void);

#endif // CALC_UI_H
