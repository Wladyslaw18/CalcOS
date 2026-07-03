/*
 * ui_core.h Public header for the default UI implementations in ui_core.c
 * 
 * Exposes the default UIDriver callback implementations so that custom
 * drivers (VGA, VESA, SDL, Terminal, etc.) can use them without
 * duplicating the input processing and rendering logic.
 */

#ifndef UI_CORE_H
#define UI_CORE_H

#include "../../include/calc/ui.h"

// Per-instance UI context (used by all default implementations)
typedef struct {
    char buffer[256];                    // Current input buffer
    uint32_t length;                     // Current input length
    uint32_t cursor;                     // Cursor position within buffer
    char history[5][256];                // Command history (ring buffer)
    uint32_t history_count;              // Number of history entries
    int32_t history_pos;                 // Current history navigation position
    char last_result[64];                // Formatted last result
    char last_error[64];                 // Formatted last error
    UIEvent last_event;                  // Last event that occurred
} UIContext;

// Default implementations (implemented in ui_core.c)

// Initialize the UI driver with default bindings
int ui_init(UIDriver* self);

// Default input processor maps keycodes to calculator operations
UIEvent ui_process_input_default(UIDriver* self, uint32_t key);

// Full re-render of all calculator UI elements
void ui_render_default(const UIDriver* self);

// Efficient partial redraw (only redraws affected region)
void ui_render_partial_default(const UIDriver* self, UIEvent reason);

// Default event loop polls input processes renders repeats
int ui_run_loop_default(UIDriver* self);

#endif // UI_CORE_H
