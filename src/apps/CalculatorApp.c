// src/apps/CalculatorApp.c
#include "../../include/kernel/app.h"
#include "../../include/kernel/write_ahead_log.h"
#include "../../Kernel/State/CalculatorState.h"
#include "../../Application/UI/ui_core.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"
#include "../../Application/Output/Formatter.h"
#include "../../include/calc/ui.h"
#include "../../Application/Input/Parser.h"

typedef struct {
    CalculatorState  state;   // 64 bytes (aligned)
    UIContext        ui;      // Local UI contexts & history buffers
} CalculatorAppContext;

// Static allocation: completely bypasses the heap
static CalculatorAppContext g_calc_app_state;

static void calc_state_init_local(CalculatorState* state) {
    if (!state) return;
    fast_memset(state, 0, sizeof(CalculatorState));
}

static void calc_app_init(void* ctx) {
    CalculatorAppContext* app = (CalculatorAppContext*)ctx;
    fast_memset(app, 0, sizeof(CalculatorAppContext));
    app->ui.history_pos = -1;
    calc_state_init_local(&app->state);
    
    // Check if a Write-Ahead Log exists for crash recovery
    StateWAL* s_wal = get_state_wal();
    if (s_wal->magic == WAL_MAGIC && s_wal->cursor > 0) {
        // Replay the log through the parser to rebuild the stack
        bool success;
        parse_expression(s_wal->history_log, &app->state, &success);
        (void)success;
    }
}

static void calc_app_update(void* ctx, uint32_t key) {
    CalculatorAppContext* app = (CalculatorAppContext*)ctx;
    
    // If a printable char is pressed, log it to the WAL first
    if (key >= 32 && key <= 126) {
        wal_log_char((char)key);
    } else if (key == UI_KEY_ENTER) {
        wal_log_char('\n');
    }
    
    // Pass control straight to your optimized input parser
    ui_process_input_default_with_ctx(&app->ui, &app->state, key, (UIDriver*)0);
    
    // If the calculation succeeded, clear the recovery log
    if (key == UI_KEY_ENTER && app->ui.last_event == UI_EVENT_RESULT) {
        wal_clear_log();
    }
}

static void calc_app_draw(void* ctx, const DisplayDriver* disp, ClipRect clip) {
    CalculatorAppContext* app = (CalculatorAppContext*)ctx;
    
    // Screen rendering using offset parameters relative to the ClipRect
    char line[80];
    format_string(line, sizeof(line), "CalcOS> %s", app->ui.buffer);
    
    // Write directly to coordinates within the ClipRect limits
    if (disp->write_str) {
        disp->write_str(disp, line, clip.x, clip.y, UI_COLOR_YELLOW, UI_COLOR_BLACK);
    }
}

// Bind the Application Struct
const Application g_calculator_app = {
    .name = "Calculator",
    .context = &g_calc_app_state,
    .init = calc_app_init,
    .update = calc_app_update,
    .draw = calc_app_draw
};

REGISTER_APPLICATION(g_calculator_app)
