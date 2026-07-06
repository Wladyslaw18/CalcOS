/*
 * File: ui_core.c
 * Author: W. Kowal
 * Description: Default UI implementations for the calculator engine.
 * 
 * This provides the reference implementations of:
 * - ui_init() Initialize the UI driver
 * - ui_process_input_default() Parse keys into calculator operations
 * - ui_render_default() Full screen render
 * - ui_render_partial_default() Efficient partial redraw
 * - ui_run_loop_default() The main event loop
 * 
 * These are NOT tied to any specific display hardware they call
 * the DisplayDriver and InputDriver function pointers, which you
 * provide. Swap VGA for VESA for SDL just by changing the driver.
 * 
 * ZERO HEAP ALLOCATIONS. Everything runs on the stack.
 */

#include "../../include/calc/ui.h"
/* Service locator replaces direct module includes — stable proxy pattern */
#include "../../Kernel/Kernel.h"
#include "../../Kernel/API/ParserAPI.h"
#include "../../Kernel/API/FormatterAPI.h"
#include "../../Kernel/API/StackAPI.h"
/* Formatter is a pure utility (no state). Direct call is fine;
 * the Kernel service locator pattern is for stateful/pluggable services. */
#include "../Output/Formatter.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

// Line-editing state (per-instance, stored in driver context)
#define UI_INPUT_BUF_SIZE 256
#define UI_HISTORY_LINES  5

typedef struct {
    char buffer[UI_INPUT_BUF_SIZE];   // Current input buffer
    uint32_t length;                   // Current input length
    uint32_t cursor;                   // Cursor position within buffer
    char history[UI_HISTORY_LINES][UI_INPUT_BUF_SIZE]; // Command history
    uint32_t history_count;            // Number of history entries
    int32_t history_pos;               // Current history navigation position (-1 = fresh)
    char last_result[64];              // Formatted last result
    char last_error[64];               // Formatted last error
    UIEvent last_event;                // Last event that occurred
} UIContext;

// Initialize the UI driver
int ui_init(UIDriver* self) {
    if (!self || !self->display || !self->input) return -1;

    // Allocate context in the driver's context pointer
    // (Stack-allocated in real usage, no heap!)
    if (!self->context) {
        // Context must be set by the specific driver implementation
        return -2;
    }

    UIContext* ctx = (UIContext*)self->context;
    fast_memset(ctx, 0, sizeof(UIContext));
    ctx->history_pos = -1;
    ctx->last_event = UI_EVENT_INIT;

    // Register the state observer
    if (self->on_state_change) {
        self->on_state_change(self, UI_EVENT_INIT);
    }

    return 0;
}

// Default input processing
// Maps key presses to calculator operations, calling the parser
// when Enter is pressed and updating the operand stack/history.
UIEvent ui_process_input_default(UIDriver* self, uint32_t key) {
    if (!self || !self->calc_state) return UI_EVENT_NONE;

    UIContext* ctx = (UIContext*)self->context;
    if (!ctx) return UI_EVENT_NONE;

    CalculatorState* calc = self->calc_state;

    switch (key) {
        case UI_KEY_ENTER: {
            if (ctx->length == 0) return UI_EVENT_NONE;

            bool success = false;
            calc->flags = 0;
            /* Kernel service locator: parser via KRN_PARSER */
            double result = 0.0;
            ParserAPI* parser = (ParserAPI*)kernel_get(K, KRN_PARSER);
            if (parser) {
                result = parser->parse(ctx->buffer, calc, &success);
            } else {
                success = false;
            }

            // Add to history
            if (ctx->history_count < UI_HISTORY_LINES) {
                ctx->history_count++;
            }
            // Shift history
            for (uint32_t i = ctx->history_count - 1; i > 0; i--) {
                fast_memcpy(ctx->history[i], ctx->history[i - 1], UI_INPUT_BUF_SIZE);
            }
            fast_memcpy(ctx->history[0], ctx->buffer, UI_INPUT_BUF_SIZE);

            ctx->history_pos = -1;

            if (success) {
                // Push result onto operand stack
                for (int i = 3; i > 0; i--) {
                    calc->operands[i] = calc->operands[i - 1];
                }
                calc->operands[0] = result;
                if (calc->op_count < 4) calc->op_count++;

                format_string(ctx->last_result, sizeof(ctx->last_result), "%f", result);
                // FIX: Strip trailing zeros "4.000000" "4"
                // Keeps significant decimals: "3.500000" "3.5", "4.000000" "4"
                for (char* p = ctx->last_result; *p; p++) {
                    if (*p == '.') {
                        char* end = p;
                        while (*end) end++;           // Go to end
                        while (end > p && *--end == '0');  // Backtrack past trailing zeros
                        if (end == p) *end = '\0';    // Nothing after dot kill dot only
                        else *(end + 1) = '\0';        // Keep non-zero digit, null after it
                        break;
                    }
                }
                ctx->last_result[sizeof(ctx->last_result) - 1] = '\0';
                ctx->last_error[0] = '\0';
                ctx->last_event = UI_EVENT_RESULT;
            } else {
                if (calc->flags & 2) {
                    format_string(ctx->last_error, sizeof(ctx->last_error),
                                  "Division by zero", 0);
                } else {
                    format_string(ctx->last_error, sizeof(ctx->last_error),
                                  "Invalid expression", 0);
                }
                ctx->last_result[0] = '\0';
                ctx->last_event = UI_EVENT_ERROR;
            }

            // Clear input buffer
            ctx->length = 0;
            ctx->cursor = 0;
            fast_memset(ctx->buffer, 0, UI_INPUT_BUF_SIZE);

            if (self->on_state_change) {
                self->on_state_change(self, ctx->last_event);
            }
            return ctx->last_event;
        }

        case UI_KEY_BACKSPACE: {
            if (ctx->cursor > 0) {
                for (uint32_t i = ctx->cursor - 1; i < ctx->length - 1; i++) {
                    ctx->buffer[i] = ctx->buffer[i + 1];
                }
                ctx->length--;
                ctx->cursor--;
                ctx->buffer[ctx->length] = '\0';
                ctx->last_event = UI_EVENT_INPUT_CHANGE;
                if (self->on_state_change) {
                    self->on_state_change(self, UI_EVENT_INPUT_CHANGE);
                }
            }
            return ctx->last_event;
        }

        case UI_KEY_DELETE: {
            if (ctx->cursor < ctx->length) {
                for (uint32_t i = ctx->cursor; i < ctx->length - 1; i++) {
                    ctx->buffer[i] = ctx->buffer[i + 1];
                }
                ctx->length--;
                ctx->buffer[ctx->length] = '\0';
                ctx->last_event = UI_EVENT_INPUT_CHANGE;
                if (self->on_state_change) {
                    self->on_state_change(self, UI_EVENT_INPUT_CHANGE);
                }
            }
            return ctx->last_event;
        }

        case UI_KEY_LEFT: {
            if (ctx->cursor > 0) {
                ctx->cursor--;
                ctx->last_event = UI_EVENT_INPUT_CHANGE;
            }
            return ctx->last_event;
        }

        case UI_KEY_RIGHT: {
            if (ctx->cursor < ctx->length) {
                ctx->cursor++;
                ctx->last_event = UI_EVENT_INPUT_CHANGE;
            }
            return ctx->last_event;
        }

        case UI_KEY_UP: {
            if (ctx->history_count > 0) {
                if (ctx->history_pos < (int32_t)ctx->history_count - 1) {
                    ctx->history_pos++;
                    // Copy history entry to buffer
                    uint32_t h = (uint32_t)ctx->history_pos;
                    fast_memset(ctx->buffer, 0, UI_INPUT_BUF_SIZE);
                    fast_memcpy(ctx->buffer, ctx->history[h], UI_INPUT_BUF_SIZE);
                    ctx->length = 0;
                    while (ctx->length < UI_INPUT_BUF_SIZE && ctx->buffer[ctx->length] != '\0')
                        ctx->length++;
                    ctx->cursor = ctx->length;
                    ctx->last_event = UI_EVENT_INPUT_CHANGE;
                    if (self->on_state_change) {
                        self->on_state_change(self, UI_EVENT_INPUT_CHANGE);
                    }
                }
            }
            return ctx->last_event;
        }

        case UI_KEY_DOWN: {
            if (ctx->history_pos > 0) {
                ctx->history_pos--;
                uint32_t h = (uint32_t)ctx->history_pos;
                fast_memset(ctx->buffer, 0, UI_INPUT_BUF_SIZE);
                fast_memcpy(ctx->buffer, ctx->history[h], UI_INPUT_BUF_SIZE);
                ctx->length = 0;
                while (ctx->length < UI_INPUT_BUF_SIZE && ctx->buffer[ctx->length] != '\0')
                    ctx->length++;
                ctx->cursor = ctx->length;
                ctx->last_event = UI_EVENT_INPUT_CHANGE;
            } else if (ctx->history_pos == 0) {
                ctx->history_pos = -1;
                fast_memset(ctx->buffer, 0, UI_INPUT_BUF_SIZE);
                ctx->length = 0;
                ctx->cursor = 0;
                ctx->last_event = UI_EVENT_INPUT_CHANGE;
            }
            if (self->on_state_change) {
                self->on_state_change(self, UI_EVENT_INPUT_CHANGE);
            }
            return ctx->last_event;
        }

        case UI_KEY_ESCAPE: {
            ctx->length = 0;
            ctx->cursor = 0;
            ctx->history_pos = -1;
            fast_memset(ctx->buffer, 0, UI_INPUT_BUF_SIZE);
            ctx->last_event = UI_EVENT_CLEAR;
            if (self->on_state_change) {
                self->on_state_change(self, UI_EVENT_CLEAR);
            }
            return ctx->last_event;
        }

        default: {
            // Printable character insert at cursor
            if (key >= 32 && key <= 126) {
                char c = (char)key;
                if (ctx->length < UI_INPUT_BUF_SIZE - 1) {
                    // Shift characters right to make room
                    for (uint32_t i = ctx->length; i > ctx->cursor; i--) {
                        ctx->buffer[i] = ctx->buffer[i - 1];
                    }
                    ctx->buffer[ctx->cursor] = c;
                    ctx->length++;
                    ctx->cursor++;
                    ctx->buffer[ctx->length] = '\0';
                    ctx->last_event = UI_EVENT_INPUT_CHANGE;
                    if (self->on_state_change) {
                        self->on_state_change(self, UI_EVENT_INPUT_CHANGE);
                    }
                }
            }
            return ctx->last_event;
        }
    }

    return UI_EVENT_NONE;
}

// Default full render
// Renders the entire calculator UI using the DisplayDriver interface.
// This is hardware-agnostic works on VGA, VESA, Serial, SDL, whatever.
void ui_render_default(const UIDriver* self) {
    if (!self || !self->display) return;

    UIContext* ctx = (UIContext*)self->context;
    CalculatorState* calc = self->calc_state;
    const DisplayDriver* disp = self->display;

    // Clear screen
    if (disp->clear) {
        disp->clear(disp, UI_COLOR_BLACK);
    }

    // Header
    const char* header = "=== Bare Metal Calculator ===";
    if (disp->write_str) {
        disp->write_str(disp, header, 0, 0, UI_COLOR_CYAN, UI_COLOR_BLACK);
    }

    // Operand stack (bottom up: operands[0] is top)
    char stack_line[64];
    if (disp->write_str) {
        disp->write_str(disp, "Stack:", 0, 2, UI_COLOR_GREEN, UI_COLOR_BLACK);
    }
    for (uint32_t i = 0; i < calc->op_count && i < 4; i++) {
        format_string(stack_line, sizeof(stack_line), "  [%u]: %f",
                      i, calc->operands[i]);
        if (disp->write_str) {
            disp->write_str(disp, stack_line, 0, 3 + i,
                            UI_COLOR_WHITE, UI_COLOR_BLACK);
        }
    }

    // Input buffer with prompt
    char prompt_buf[UI_INPUT_BUF_SIZE + 16];
    format_string(prompt_buf, sizeof(prompt_buf), "Calc> %s",
                  ctx ? ctx->buffer : "");
    uint32_t prompt_y = 3 + 4 + 1; // After stack
    if (disp->write_str) {
        disp->write_str(disp, prompt_buf, 0, prompt_y,
                        UI_COLOR_YELLOW, UI_COLOR_BLACK);
    }

    // Result / Error area
    uint32_t result_y = prompt_y + 2;
    if (ctx) {
        if (ctx->last_result[0] != '\0' && disp->write_str) {
            char result_line[80];
            format_string(result_line, sizeof(result_line), "Result: %s",
                          ctx->last_result);
            disp->write_str(disp, result_line, 0, result_y,
                            UI_COLOR_WHITE, UI_COLOR_BLACK);
        }
        if (ctx->last_error[0] != '\0' && disp->write_str) {
            char error_line[80];
            format_string(error_line, sizeof(error_line), "Error: %s",
                          ctx->last_error);
            disp->write_str(disp, error_line, 0, result_y,
                            UI_COLOR_RED, UI_COLOR_BLACK);
        }
    }

    // History
    if (ctx && ctx->history_count > 0 && disp->write_str) {
        disp->write_str(disp, "History:", 0, result_y + 2,
                        UI_COLOR_DARK_GREEN, UI_COLOR_BLACK);
        uint32_t max_hist = ctx->history_count < 3 ? ctx->history_count : 3;
        for (uint32_t i = 0; i < max_hist; i++) {
            char hist_line[UI_INPUT_BUF_SIZE + 8];
            format_string(hist_line, sizeof(hist_line), "  [%u]: %s",
                          max_hist - 1 - i, 
                          ctx->history[max_hist - 1 - i]);
            if (disp->write_str) {
                disp->write_str(disp, hist_line, 0, result_y + 3 + i,
                                UI_COLOR_GRAY, UI_COLOR_BLACK);
            }
        }
    }

    // Status bar (last line)
    char status[80];
    uint32_t status_y = (disp->text_rows > 0) ? disp->text_rows - 1 : 24;
    format_string(status, sizeof(status), " Mode: %s | Flags: 0x%02x",
                  calc->mode == 0 ? "Float" : calc->mode == 1 ? "Int" : "Hex",
                  calc->flags);
    if (disp->write_str) {
        disp->write_str(disp, status, 0, status_y,
                        UI_COLOR_LIGHT_GRAY, UI_COLOR_BLACK);
    }

    // Set cursor position
    if (disp->set_cursor && ctx) {
        disp->set_cursor(disp, 6 + ctx->cursor, prompt_y);
    }

    // Present (flush if double-buffered)
    if (disp->present) {
        disp->present(disp);
    }
}

// Default partial render
// Only redraws the section affected by the event.
void ui_render_partial_default(const UIDriver* self, UIEvent reason) {
    if (!self || !self->display) return;
    UIContext* ctx = (UIContext*)self->context;
    const DisplayDriver* disp = self->display;

    if (!ctx) { ui_render_default(self); return; }

    // For text-mode drivers, just redraw the relevant line
    switch (reason) {
        case UI_EVENT_INPUT_CHANGE: {
            // Redraw just the input line
            char prompt_buf[UI_INPUT_BUF_SIZE + 16];
            format_string(prompt_buf, sizeof(prompt_buf), "Calc> %s", ctx->buffer);
            uint32_t prompt_y = 8;
            if (disp->write_str) {
                disp->write_str(disp, prompt_buf, 0, prompt_y,
                                UI_COLOR_YELLOW, UI_COLOR_BLACK);
            }
            if (disp->set_cursor) {
                disp->set_cursor(disp, 6 + ctx->cursor, prompt_y);
            }
            break;
        }
        case UI_EVENT_RESULT:
        case UI_EVENT_ERROR: {
            // Redraw just the result area
            uint32_t result_y = 10;
            if (ctx->last_result[0] != '\0' && disp->write_str) {
                char result_line[80];
                format_string(result_line, sizeof(result_line), "Result: %s",
                              ctx->last_result);
                disp->write_str(disp, result_line, 0, result_y,
                                UI_COLOR_WHITE, UI_COLOR_BLACK);
            }
            if (ctx->last_error[0] != '\0' && disp->write_str) {
                char error_line[80];
                format_string(error_line, sizeof(error_line), "Error: %s",
                              ctx->last_error);
                disp->write_str(disp, error_line, 0, result_y,
                                UI_COLOR_RED, UI_COLOR_BLACK);
            }
            break;
        }
        default:
            ui_render_default(self);
            break;
    }

    if (disp->present) disp->present(disp);
}

// Default event loop
// Simple polling loop: check for input process render repeat.
int ui_run_loop_default(UIDriver* self) {
    if (!self || !self->display || !self->input) return -1;

    // Initial render
    if (self->render) {
        self->render(self);
    }

    // Event loop
    while (1) {
        // Poll for input
        if (self->input->key_available && self->input->key_available(self->input)) {
            uint32_t key = self->input->read_key(self->input);

            // Process input
            UIEvent event = UI_EVENT_NONE;
            if (self->process_input) {
                event = self->process_input(self, key);
            }

            // Render based on event
            if (event != UI_EVENT_NONE) {
                if (self->render_partial) {
                    self->render_partial(self, event);
                } else if (self->render) {
                    self->render(self);
                }
            }
        }

        // Yield CPU (halt on bare metal, yield on OS)
#ifndef _MSC_VER
        __asm__ volatile ("pause" : : : "memory");
#endif
    }

    return 0;
}
