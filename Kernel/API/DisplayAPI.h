/*
 * DisplayAPI.h — Stable proxy interface for the display driver service
 *
 * This wraps the existing DisplayDriver from include/calc/display.h
 * in a Kernel-compatible stable interface. Functions take a void* context
 * parameter (the underlying DisplayDriver pointer).
 *
 * Null Object: ker_display_null — discards all rendering, returns 0s
 */

#ifndef DISPLAY_API_H
#define DISPLAY_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Color in 0x00RRGGBB format (matches include/calc/display.h UIColor) */
typedef uint32_t APIColor;

/* ─── Display Service Interface ─── */
typedef struct {
    /* Driver context (the underlying DisplayDriver*) */
    void* context;

    /* Properties */
    uint32_t width;
    uint32_t height;
    uint32_t text_cols;
    uint32_t text_rows;

    /* Put a single character at (x, y) with fg/bg color */
    void (*put_char)(void* ctx, char c, uint32_t x, uint32_t y, APIColor fg, APIColor bg);

    /* Write a string at (x, y) */
    void (*write_str)(void* ctx, const char* str, uint32_t x, uint32_t y,
                      APIColor fg, APIColor bg);

    /* Set cursor position */
    void (*set_cursor)(void* ctx, uint32_t x, uint32_t y);

    /* Clear the display with a background color */
    void (*clear)(void* ctx, APIColor bg);

    /* Scroll by the given number of lines (positive = up) */
    void (*scroll)(void* ctx, int32_t lines);

    /* Flush/present pending rendering (for double-buffered displays) */
    void (*present)(void* ctx);

} DisplayAPI;

/* ─── Null Object (no-op display) ─── */
extern const DisplayAPI ker_display_null;

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_API_H */
