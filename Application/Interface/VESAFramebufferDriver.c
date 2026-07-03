/*
 * VESAFramebufferDriver.c VESA DisplayDriver Wrapper
 * 
 * Wraps the raw VESA framebuffer API (VESAFramebuffer.h) into the
 * abstract DisplayDriver interface (display.h). This the Plotter
 * and UI modules work with VESA without any VESA-specific code.
 * 
 * Use this on the bare-metal target to bridge the old VESA API to the
 * new DisplayDriver virtual interface. No changes needed to VESA code.
 */

#include "../../include/calc/display.h"
#include "VESAFramebuffer.h"

// put_pixel
// Delegates to vesa_put_pixel with color component extraction
static void vesa_dd_put_pixel(const DisplayDriver* self, uint32_t x, uint32_t y,
                               uint8_t r, uint8_t g, uint8_t b) {
    (void)self;
    VesaColorRGB color = { r, g, b };
    vesa_put_pixel(x, y, color);
}

// clear
static void vesa_dd_clear(const DisplayDriver* self, UIColor bg) {
    (void)self;
    VesaColorRGB color = { (uint8_t)((bg >> 16) & 0xFF),
                           (uint8_t)((bg >> 8) & 0xFF),
                           (uint8_t)(bg & 0xFF) };
    vesa_clear(color);
}

// draw_line
static void vesa_dd_draw_line(const DisplayDriver* self,
                               int32_t x0, int32_t y0,
                               int32_t x1, int32_t y1,
                               UIColor color) {
    (void)self;
    VesaColorRGB vcolor = { (uint8_t)((color >> 16) & 0xFF),
                            (uint8_t)((color >> 8) & 0xFF),
                            (uint8_t)(color & 0xFF) };
    vesa_draw_line(x0, y0, x1, y1, vcolor);
}

// draw_char
static void vesa_dd_draw_char(const DisplayDriver* self, char c,
                               uint32_t x, uint32_t y,
                               UIColor fg, UIColor bg) {
    (void)self;
    VesaColorRGB vfg = { (uint8_t)((fg >> 16) & 0xFF),
                         (uint8_t)((fg >> 8) & 0xFF),
                         (uint8_t)(fg & 0xFF) };
    VesaColorRGB vbg = { (uint8_t)((bg >> 16) & 0xFF),
                         (uint8_t)((bg >> 8) & 0xFF),
                         (uint8_t)(bg & 0xFF) };
    vesa_draw_char(c, x, y, vfg, vbg);
}

// write_str
static void vesa_dd_write_str(const DisplayDriver* self, const char* str,
                               uint32_t x, uint32_t y,
                               UIColor fg, UIColor bg) {
    (void)self;
    VesaColorRGB vfg = { (uint8_t)((fg >> 16) & 0xFF),
                         (uint8_t)((fg >> 8) & 0xFF),
                         (uint8_t)(fg & 0xFF) };
    VesaColorRGB vbg = { (uint8_t)((bg >> 16) & 0xFF),
                         (uint8_t)((bg >> 8) & 0xFF),
                         (uint8_t)(bg & 0xFF) };
    vesa_draw_string(str, x, y, vfg, vbg);
}

// scroll
static void vesa_dd_scroll(const DisplayDriver* self, int32_t lines) {
    (void)self;
    VesaColorRGB black = {0, 0, 0};
    if (lines > 0) {
        vesa_scroll((uint32_t)lines, black);
    }
}

// Public API
// Populate a DisplayDriver struct with VESA function pointers.
void vesa_display_driver_init(DisplayDriver* driver) {
    if (!driver) return;
    driver->put_pixel  = vesa_dd_put_pixel;
    driver->clear      = vesa_dd_clear;
    driver->draw_line  = vesa_dd_draw_line;
    driver->put_char   = vesa_dd_draw_char;
    driver->write_str  = vesa_dd_write_str;
    driver->scroll     = vesa_dd_scroll;
    driver->present    = NULL; // VESA is single-buffered, no present needed
    driver->set_cursor = NULL; // Not applicable for VESA
    driver->fill_rect  = NULL; // VESA has no native fill_rect
    driver->plot_function = NULL; // Not implemented
    driver->put_char   = vesa_dd_draw_char; // Already set above, keep for clarity
    driver->width      = 800;  // Default VESA width (override after init)
    driver->height     = 600;  // Default VESA height
    driver->bpp        = 32;
    driver->char_width  = 8;  // 816 font cell
    driver->char_height = 16;
    driver->text_cols   = 0;
    driver->text_rows   = 0;
    driver->context     = NULL;
}