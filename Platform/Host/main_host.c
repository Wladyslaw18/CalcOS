/*
 * main_host.c Desktop Host Application Entry Point
 * 
 * The unified entry point for the Target A (Host) build of the calculator.
 * Connect to any DisplayDriver + InputDriver combo via the UIDriver
 * interface and run the calculator event loop.
 * 
 * Build targets:
 * app: SDL2 graphical (requires libsdl2-dev)
 * app_terminal: ANSI terminal (no deps, works everywhere)
 * 
 * COMPILE with: -DPLATFORM_HOST
 * 
 * THE FLEX: Same calc engine, completely different frontends.
 * No recompilation of the engine needed. Just swap the driver.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define TokenType WindowsTokenType
#include <windows.h>
#undef TokenType
#endif

#include "../../include/calc/display.h"
#include "../../include/calc/input.h"
#include "../../include/calc/ui.h"
#include "../../Application/UI/ui_core.h"
/* ═══ KERNEL SERVICE LOCATOR ═══
 * Instead of directly including every module header, modules
 * access each other through the Kernel by service ID lookup.
 * This breaks the compile-time coupling spaghetti. */
#include "../../Kernel/Kernel.h"
#include "../../Kernel/API/ParserAPI.h"
#include "../../Kernel/API/FormatterAPI.h"
#include "../../Kernel/API/HistoryAPI.h"
#include "../../Kernel/API/StackAPI.h"
#include "../../Kernel/API/LoggerAPI.h"
#include "../../Kernel/State/CalculatorState.h"
#include "../../Kernel/Core/CPU/CPUID.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

// External driver factory declarations
extern int host_sdl_create(DisplayDriver* driver, uint32_t width, uint32_t height, const char* title);
extern void host_sdl_destroy(DisplayDriver* driver);

extern int host_term_create(DisplayDriver* driver, uint32_t cols, uint32_t rows);
extern void host_term_destroy(DisplayDriver* driver);

extern int host_input_create_sdl(InputDriver* driver);
extern int host_input_create_terminal(InputDriver* driver);
extern void host_input_destroy(InputDriver* driver);

// Application state (inline, NO heap, NO offsetof tricks)
typedef struct {
    UIContext ui_ctx;          // UI state
    CalculatorState calc;      // Calculator state
    int using_sdl;             // Which driver is active
    int using_terminal;
    DisplayDriver display;     // Display backend
    InputDriver input;         // Input backend
} HostAppState;

// FIX: Proper UI init use self->context and self->calc_state directly
// The OLD code used a broken offsetof trick that assumed the UIDriver was
// embedded inside HostAppState. It wasn't driver is a separate stack var.
// Now main() sets the pointers directly BEFORE calling init().
static int host_ui_init(UIDriver* self) {
    if (!self || !self->display || !self->input) return -1;
    if (!self->context || !self->calc_state) return -2;

    UIContext* ctx = (UIContext*)self->context;
    CalculatorState* calc = (CalculatorState*)self->calc_state;

    fast_memset(calc, 0, sizeof(CalculatorState));
    fast_memset(ctx, 0, sizeof(UIContext));
    ctx->history_pos = -1;
    ctx->last_event = UI_EVENT_INIT;

    // Switch to terminal alternate screen buffer (creates a clean "window")
    if (self->display->text_cols > 0) {
        fprintf(stdout, "\033[?1049h");  // Save screen + switch to alt buffer
        fflush(stdout);
    }

    if (self->display->clear) {
        self->display->clear(self->display, UI_COLOR_BLACK);
    }
    if (self->input->init) {
        self->input->init(self->input);
    }

    if (self->on_state_change) {
        self->on_state_change(self, UI_EVENT_INIT);
    }
    return 0;
}

static void host_ui_deinit(UIDriver* self) {
    if (!self) return;
    // Restore terminal main screen buffer (restores previous terminal content)
    if (self->display && self->display->text_cols > 0) {
        fprintf(stdout, "\033[?1049l");  // Restore main screen buffer
        fflush(stdout);
    }
    self->context = NULL;
    self->calc_state = NULL;
}

// Event loop
static int host_run_loop(UIDriver* self) {
    if (!self || !self->display || !self->input) return -1;

    int running = 1;
    clock_t last_esc_time = 0;
    int redraw = 1;
    while (running) {
        if (redraw) {
            if (self->render) self->render(self);
            if (self->display->present) self->display->present(self->display);
            redraw = 0;
        }

        if (self->input->key_available && self->input->key_available(self->input)) {
            uint32_t key = self->input->read_key(self->input);
            if (key == UI_KEY_ESCAPE) {
                clock_t current_time = clock();
                double elapsed = (double)(current_time - last_esc_time) / CLOCKS_PER_SEC;
                if (elapsed <= 0.5 && last_esc_time != 0) {
                    running = 0;
                    continue;
                }
                last_esc_time = current_time;
                continue;
            }
            if (self->process_input) {
                self->process_input(self, key);
            }
            redraw = 1;
        }

#ifdef _WIN32
        Sleep(10);
#else
        struct timespec ts = {0, 10000000}; // 10ms
        nanosleep(&ts, NULL);
#endif
    }
    return 0;
}

// Default render: terminal calculator UI
static void host_render(const UIDriver* self) {
    if (!self || !self->display || !self->context) return;
    UIContext* ctx = (UIContext*)self->context;
    DisplayDriver* disp = (DisplayDriver*)self->display;

    if (disp->text_cols > 0) {
        char line_buf[512];

        disp->write_str(disp, "╔══════════════════════════════════╗", 0, 0, UI_COLOR_CYAN, UI_COLOR_BLACK);
        disp->write_str(disp, "║  BARE METAL CALCULATOR (HOST)    ║", 0, 1, UI_COLOR_CYAN, UI_COLOR_BLACK);
        disp->write_str(disp, "╚══════════════════════════════════╝", 0, 2, UI_COLOR_CYAN, UI_COLOR_BLACK);
        disp->write_str(disp, "Type expressions and press Enter. ESC twice to exit.", 2, 3, UI_COLOR_GRAY, UI_COLOR_BLACK);

        int row = 5;
        for (uint32_t i = 0; i < ctx->history_count; i++) {
            if (ctx->history[i][0] != '\0') {
                snprintf(line_buf, sizeof(line_buf), "  > %s\033[K", ctx->history[i]);
                disp->write_str(disp, line_buf, 0, (uint32_t)row, UI_COLOR_GRAY, UI_COLOR_BLACK);
                row++;
            }
        }

        if (ctx->last_result[0] != '\0') {
            snprintf(line_buf, sizeof(line_buf), "  = %s\033[K", ctx->last_result);
            disp->write_str(disp, line_buf, 0, (uint32_t)(row + 1), UI_COLOR_GREEN, UI_COLOR_BLACK);
        }
        if (ctx->last_error[0] != '\0') {
            snprintf(line_buf, sizeof(line_buf), "  ! %s\033[K", ctx->last_error);
            disp->write_str(disp, line_buf, 0, (uint32_t)(row + 1), UI_COLOR_RED, UI_COLOR_BLACK);
        }

        // Clear line then write prompt + input (prevents ghost chars after backspace)
        disp->write_str(disp, "────────────────────────────────────\033[K", 0, (uint32_t)(row + 3), UI_COLOR_GRAY, UI_COLOR_BLACK);
        snprintf(line_buf, sizeof(line_buf), ">> %s\033[K", ctx->buffer);
        disp->write_str(disp, line_buf, 0, (uint32_t)(row + 4), UI_COLOR_YELLOW, UI_COLOR_BLACK);
    }
}

// Main
int main(int argc, char* argv[]) {
#ifdef _WIN32
    UINT old_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
#endif
    int use_terminal = 0;
    int use_sdl = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--term") == 0 || strcmp(argv[i], "--terminal") == 0) {
            use_terminal = 1;
        } else if (strcmp(argv[i], "--sdl") == 0) {
            use_sdl = 1;
        } else if (strcmp(argv[i], "--cli") == 0) {
            use_terminal = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Calc Engine — Host Build\n");
            printf("Usage: %s [--sdl | --term]\n", argv[0]);
            printf("  --sdl         SDL2 graphical window (default if available)\n");
            printf("  --term, --cli ANSI terminal mode (fallback)\n");
#ifdef _WIN32
            SetConsoleOutputCP(old_cp);
#endif
            return 0;
        }
    }

    HostAppState state;
    UIDriver driver;
    memset(&state, 0, sizeof(state));
    memset(&driver, 0, sizeof(driver));

    int created = -1;

    // FIX: Wire up context and calc_state BEFORE calling init
    // The old code tried to compute these via offsetof (which crashed because
    // driver is NOT embedded in HostAppState). Now set them directly.
    driver.context = &state.ui_ctx;
    driver.calc_state = &state.calc;

#ifdef COMPILE_SDL
    if (!use_terminal || use_sdl) {
        created = host_sdl_create(&state.display, 800, 600, "Calc Engine");
        if (created == 0) {
            created = host_input_create_sdl(&state.input);
            if (created == 0) {
                state.using_sdl = 1;
                driver.display = &state.display;
                driver.input = &state.input;
                printf("SDL2 display initialized (800x600)\n");
            }
        }
    }
#endif

    if (created != 0) {
        created = host_term_create(&state.display, 80, 25);
        if (created == 0) {
            created = host_input_create_terminal(&state.input);
            state.using_terminal = 1;
            if (created == 0) {
                driver.display = &state.display;
                driver.input = &state.input;
                printf("Terminal display initialized (80x25)\n");
            }
        }
    }

    if (created != 0) {
        fprintf(stderr, "Failed to initialize display/input driver (err=%d)\n", created);
        fprintf(stderr, "Try --term for terminal mode.\n");
        return 1;
    }

    /* ══════════════════════════════════════════════════════
     * KERNEL BOOT — Initialize service locator & register all services
     * ══════════════════════════════════════════════════════ */
    extern const ParserAPI g_parser_service;
    extern const FormatterAPI g_formatter_service;
    extern const HistoryAPI g_history_service;
    extern const StackAPI g_stack_service;

    /* The global Kernel `g_kernel` (declared in Kernel.h as `K`) is a
     * convenience for freestanding bare-metal builds. In this host app,
     * we initialize it and register every stateful service. */
    kernel_init(K, &state.calc);

    kernel_register(K, KRN_PARSER, "parser", (void*)&g_parser_service);
    kernel_register(K, KRN_FORMATTER, "formatter", (void*)&g_formatter_service);
    kernel_register(K, KRN_HISTORY, "history", (void*)&g_history_service);
    kernel_register(K, KRN_OPERAND_STACK, "stack", (void*)&g_stack_service);
    kernel_register(K, KRN_DISPLAY, "display", (void*)&state.display);
    kernel_register(K, KRN_INPUT, "input", (void*)&state.input);

    /* Detect and register CPU features */
    CPUFeatures cpu_features;
    fast_memset(&cpu_features, 0, sizeof(cpu_features));
    cpu_detect_features(&cpu_features);
    kernel_register(K, KRN_CPU_FEATURES, "cpufeatures", (void*)&cpu_features);

    /* Activate all registered services */
    for (uint32_t i = 0; i < KRN_COUNT; ++i) {
        kernel_activate(K, (KernelServiceId)i);
    }

    driver.name = "Host UI";
    driver.init = host_ui_init;
    driver.deinit = host_ui_deinit;
    driver.run_loop = host_run_loop;
    driver.render = host_render;
    driver.process_input = ui_process_input_default;
    driver.on_state_change = NULL;

    int exit_code = 0;
    int ret = driver.init(&driver);
    if (ret != 0) {
        fprintf(stderr, "UI init failed (err=%d)\n", ret);
        if (state.using_sdl) host_sdl_destroy(&state.display);
        if (state.using_terminal) host_term_destroy(&state.display);
        host_input_destroy(&state.input);
        exit_code = 1;
        goto cleanup;
    }

    driver.run_loop(&driver);

    // FIX: Only destroy the driver that was actually initialized!
    driver.deinit(&driver);
    host_input_destroy(&state.input);
    if (state.using_sdl) host_sdl_destroy(&state.display);
    if (state.using_terminal) host_term_destroy(&state.display);

cleanup:
#ifdef _WIN32
    SetConsoleOutputCP(old_cp);
#endif
    return exit_code;
}
