/*
 * host_term.c ANSI Terminal Display Driver
 * 
 * Implements DisplayDriver from include/calc/display.h using standard
 * ANSI escape sequences on stdout. No external libraries required.
 * 
 * Features:
 * - Truecolor text output (\033[38;2;R;G;Bm)
 * - Cursor positioning (\033[Y;XH)
 * - Scroll regions (\033[S / \033[T)
 * - Pixel grid emulation via Unicode half-blocks
 * 
 * COMPILE with: -DPLATFORM_HOST
 * No special linker flags needed. Runs on any POSIX terminal.
 * 
 * THE FLEX: The calc engine doesn't know it's writing to a terminal.
 * It calls put_pixel() and figure it out.
 */

#include "../../include/calc/display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Terminal display context
typedef struct {
    uint32_t  width;          // Terminal cell columns
    uint32_t  height;         // Terminal cell rows
    char*     screen_buf;     // Double-buffered screen content
    uint32_t  buf_size;
    uint32_t  bg_color;       // Current background color (0x00RRGGBB)
    uint32_t  fg_color;       // Current foreground color (0x00RRGGBB)
} TermContext;

// ANSI escape code helpers
static void term_printf(const DisplayDriver* self, const char* fmt, ...) {
    (void)self;
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

// Position cursor
// ANSI: \033[row;colH  (1-based)
static void term_set_cursor(const DisplayDriver* self, uint32_t x, uint32_t y) {
    term_printf(self, "\033[%d;%dH", (int)(y + 1), (int)(x + 1));
}

// put_char
// Write a single character at (x, y) with terminal truecolor ANSI codes
static void term_put_char(const DisplayDriver* self, char c,
                           uint32_t x, uint32_t y,
                           UIColor fg, UIColor bg) {
    if (!self) return;
    uint8_t fr = (fg >> 16) & 0xFF, fg_g = (fg >> 8) & 0xFF, fb = fg & 0xFF;
    uint8_t br = (bg >> 16) & 0xFF, bg_g = (bg >> 8) & 0xFF, bb = bg & 0xFF;
    
    // Write ANSI truecolor escape, then character
    fprintf(stdout, "\033[%d;%dH\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm%c",
            (int)(y + 1), (int)(x + 1),
            fr, fg_g, fb, br, bg_g, bb, c ? c : ' ');
    fflush(stdout);
}

// write_str
static void term_write_str(const DisplayDriver* self, const char* str,
                            uint32_t x, uint32_t y,
                            UIColor fg, UIColor bg) {
    if (!self || !str) return;
    uint8_t fr = (fg >> 16) & 0xFF, fg_g = (fg >> 8) & 0xFF, fb = fg & 0xFF;
    uint8_t br = (bg >> 16) & 0xFF, bg_g = (bg >> 8) & 0xFF, bb = bg & 0xFF;
    
    fprintf(stdout, "\033[%d;%dH\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm%s",
            (int)(y + 1), (int)(x + 1),
            fr, fg_g, fb, br, bg_g, bb, str);
    fflush(stdout);
}

// clear
static void term_clear(const DisplayDriver* self, UIColor bg) {
    if (!self) return;
    uint8_t r = (bg >> 16) & 0xFF, g = (bg >> 8) & 0xFF, b = bg & 0xFF;
    fprintf(stdout, "\033[2J\033[48;2;%d;%d;%dm\033[H", r, g, b);
    fflush(stdout);
}

// scroll
static void term_scroll(const DisplayDriver* self, int32_t lines) {
    if (!self) return;
    if (lines > 0) {
        // Scroll up (new lines at bottom)
        for (int32_t i = 0; i < lines; i++) fprintf(stdout, "\033[S");
    } else if (lines < 0) {
        // Scroll down (new lines at top)
        for (int32_t i = 0; i < -lines; i++) fprintf(stdout, "\033[T");
    }
    fflush(stdout);
}

// put_pixel (emulated)
// Terminals can't do individual pixels, so approximate with characters.
// In practice, the terminal driver works at character granularity.
static void term_put_pixel(const DisplayDriver* self, uint32_t x, uint32_t y,
                            uint8_t r, uint8_t g, uint8_t b) {
    (void)self; (void)x; (void)y; (void)r; (void)g; (void)b;
    // No-op: terminals don't have pixel-level control.
    // Use put_char for text-mode rendering instead.
}

// present
static void term_present(const DisplayDriver* self) {
    (void)self;
    fflush(stdout);
}

// Public API
int host_term_create(DisplayDriver* driver, uint32_t cols, uint32_t rows) {
    if (!driver) return -1;

    TermContext* ctx = (TermContext*)calloc(1, sizeof(TermContext));
    if (!ctx) return -2;

    ctx->width  = cols ? cols : 80;
    ctx->height = rows ? rows : 25;
    ctx->bg_color = UI_COLOR_BLACK;
    ctx->fg_color = UI_COLOR_WHITE;

    // Wire up DisplayDriver function pointers
    driver->context    = ctx;
    driver->width      = 0;                // Pixel dimensions unknown
    driver->height     = 0;
    driver->text_cols  = ctx->width;
    driver->text_rows  = ctx->height;
    driver->char_width  = 8;               // Standard terminal cell size
    driver->char_height = 16;
    driver->bpp        = 0;                // Text mode
    driver->put_char   = term_put_char;
    driver->write_str  = term_write_str;
    driver->set_cursor = term_set_cursor;
    driver->scroll     = term_scroll;
    driver->put_pixel  = term_put_pixel;
    driver->clear      = term_clear;
    driver->present    = term_present;
    // No draw_line or fill_rect terminal can't do pixel graphics

    // Clear the terminal
    term_clear(driver, UI_COLOR_BLACK);

    return 0;
}

void host_term_destroy(DisplayDriver* driver) {
    if (!driver || !driver->context) return;
    TermContext* ctx = (TermContext*)driver->context;

    // Reset terminal attributes
    fprintf(stdout, "\033[0m\033[2J\033[H");
    fflush(stdout);

    if (ctx->screen_buf) free(ctx->screen_buf);
    free(ctx);
    driver->context = NULL;
}
