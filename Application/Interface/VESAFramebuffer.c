#include "VESAFramebuffer.h"
#include "MathSymbols.h" // Provides glyph arrays for text rendering
#include <stdbool.h>
#include <stdint.h>

// FIX: Track remaining bytes for scroll alignment (#14)
// Internal VESA framebuffer parameters cached upon initialization
static uint32_t g_fb_phys = 0;
static uint16_t g_width = 0;
static uint16_t g_height = 0;
static uint16_t g_pitch = 0;
static uint8_t g_bpp = 0;
// FIX: add sanity flag for initialization check
static bool g_fb_initialized = false;

void vesa_init(const VesaModeInfo* mode_info) {
    if (!mode_info) return;
    g_fb_phys = mode_info->framebuffer;
    g_width = mode_info->width;
    g_height = mode_info->height;
    g_pitch = mode_info->pitch;
    g_bpp = mode_info->bpp;
    g_fb_initialized = true;
}

// Highly optimized pixel plot
void vesa_put_pixel(uint32_t x, uint32_t y, VesaColorRGB color) {
    if (g_fb_phys == 0) return;  // Don't write to address 0!
    if (x >= g_width || y >= g_height) return;
    
    // Calculate memory location using pitch to handle alignment correctly
    if (g_bpp == 32) {
        volatile uint32_t* pixel_ptr = (volatile uint32_t*)(uintptr_t)(g_fb_phys + y * g_pitch + x * 4);
        // Direct color representation (typically XRGB, 0x00RRGGBB)
        *pixel_ptr = ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | color.b;
    } else if (g_bpp == 24) {
        volatile uint8_t* pixel_ptr = (volatile uint8_t*)(uintptr_t)(g_fb_phys + y * g_pitch + x * 3);
        pixel_ptr[0] = color.b;
        pixel_ptr[1] = color.g;
        pixel_ptr[2] = color.r;
    }
}

void vesa_clear(VesaColorRGB color) {
    if (g_fb_phys == 0) return;
    
    if (g_bpp == 32) {
        uint32_t raw_color = ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | color.b;
        uint64_t raw_color_x2 = ((uint64_t)raw_color << 32) | raw_color;
        
        for (uint32_t y = 0; y < g_height; y++) {
            volatile uint64_t* row_64 = (volatile uint64_t*)(uintptr_t)(g_fb_phys + y * g_pitch);
            uint32_t row_qwords = g_width / 2;
            for (uint32_t i = 0; i < row_qwords; i++) {
                row_64[i] = raw_color_x2;
            }
            if (g_width & 1) {
                volatile uint32_t* row_32 = (volatile uint32_t*)row_64;
                row_32[g_width - 1] = raw_color;
            }
        }
    } else {
        // Fallback pixel-by-pixel for other formats
        for (uint32_t y = 0; y < g_height; y++) {
            for (uint32_t x = 0; x < g_width; x++) {
                vesa_put_pixel(x, y, color);
            }
        }
    }
}

// Fast vertical screen scrolling utilizing optimized copying
void vesa_scroll(uint32_t scanlines, VesaColorRGB fill_color) {
    if (scanlines == 0) return;
    if (g_fb_phys == 0 || scanlines >= g_height) {
        vesa_clear(fill_color);
        return;
    }
    
    uint32_t bytes_to_copy = (g_height - scanlines) * g_pitch;
    uint32_t src_offset = scanlines * g_pitch;
    
    // Copy the screen content up (optimized 64-bit movement)
    volatile uint64_t* dest_ptr = (volatile uint64_t*)(uintptr_t)g_fb_phys;
    volatile uint64_t* src_ptr = (volatile uint64_t*)(uintptr_t)(g_fb_phys + src_offset);
    uint32_t qwords_to_copy = bytes_to_copy / 8;
    uint32_t remaining_bytes = bytes_to_copy % 8;
    
    for (uint32_t i = 0; i < qwords_to_copy; i++) {
        dest_ptr[i] = src_ptr[i];
    }
    // FIX: Handle remaining bytes (#14)
    if (remaining_bytes > 0) {
        volatile uint8_t* dest_byte = (volatile uint8_t*)(uintptr_t)g_fb_phys + qwords_to_copy * 8;
        volatile uint8_t* src_byte = (volatile uint8_t*)(uintptr_t)(g_fb_phys + src_offset) + qwords_to_copy * 8;
        for (uint32_t i = 0; i < remaining_bytes; i++) {
            dest_byte[i] = src_byte[i];
        }
    }
    
    // Clear the newly exposed region at the bottom
    uint32_t fill_start_y = g_height - scanlines;
    if (g_bpp == 32) {
        uint32_t raw_color = ((uint32_t)fill_color.r << 16) | ((uint32_t)fill_color.g << 8) | fill_color.b;
        uint64_t raw_color_x2 = ((uint64_t)raw_color << 32) | raw_color;
        
        volatile uint64_t* fill_ptr = (volatile uint64_t*)(uintptr_t)(g_fb_phys + fill_start_y * g_pitch);
        uint32_t fill_bytes = scanlines * g_pitch;
        uint32_t fill_qwords = fill_bytes / 8;
        
        for (uint32_t i = 0; i < fill_qwords; i++) {
            fill_ptr[i] = raw_color_x2;
        }
        uint32_t remaining = fill_bytes % 8;
        if (remaining > 0) {
            volatile uint8_t* remainder_ptr = (volatile uint8_t*)fill_ptr + fill_qwords * 8;
            for (uint32_t i = 0; i < remaining; i++) {
                remainder_ptr[i] = ((volatile uint8_t*)&raw_color_x2)[i];
            }
        }
    } else {
        for (uint32_t y = fill_start_y; y < g_height; y++) {
            for (uint32_t x = 0; x < g_width; x++) {
                vesa_put_pixel(x, y, fill_color);
            }
        }
    }
}

// Bresenham's line drawing algorithm (branch-free/optimized where possible)
void vesa_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, VesaColorRGB color) {
    // FIX: Null framebuffer guard
    if (g_fb_phys == 0 || !g_fb_initialized) return;
    if (g_fb_phys == 0) return;
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    
    // Absolute values
    int32_t abs_dx = (dx < 0) ? -dx : dx;
    int32_t abs_dy = (dy < 0) ? -dy : dy;
    
    int32_t sx = (dx < 0) ? -1 : 1;
    int32_t sy = (dy < 0) ? -1 : 1;
    
    int32_t err = abs_dx - abs_dy;
    
    while (1) {
        vesa_put_pixel((uint32_t)x0, (uint32_t)y0, color);
        if (x0 == x1 && y0 == y1) break;
        
        int32_t e2 = 2 * err;
        // Branchless modification of coords and err accumulator
        if (e2 > -abs_dy) {
            err -= abs_dy;
            x0 += sx;
        }
        if (e2 < abs_dx) {
            err += abs_dx;
            y0 += sy;
        }
    }
}

// Draw a single character using the 8x16 font glyph data
void vesa_draw_char(char c, uint32_t x, uint32_t y, VesaColorRGB fg, VesaColorRGB bg) {
    // FIX: Null framebuffer guard (#16)
    if (g_fb_phys == 0 || !g_fb_initialized) return;
    
    const uint8_t* glyph = math_symbols_get_glyph(c);
    if (!glyph) return;
    
    for (uint32_t row = 0; row < 16; row++) {
        uint8_t glyph_row = glyph[row];
        for (uint32_t col = 0; col < 8; col++) {
            // Check MSB first to draw standard left-to-right character representation
            uint8_t active = (glyph_row >> (7 - col)) & 1;
            VesaColorRGB pixel_color = active ? fg : bg;
            vesa_put_pixel(x + col, y + row, pixel_color);
        }
    }
}

// Draw a string using standard 8-pixel horizontal and 16-pixel vertical layout
void vesa_draw_string(const char* str, uint32_t x, uint32_t y, VesaColorRGB fg, VesaColorRGB bg) {
    // FIX: Null framebuffer guard (#16)
    if (g_fb_phys == 0 || !g_fb_initialized) return;
    
    if (!str) return;
    uint32_t cur_x = x;
    uint32_t cur_y = y;
    
    for (uint32_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cur_x = x;
            cur_y += 16;
        } else {
            vesa_draw_char(str[i], cur_x, cur_y, fg, bg);
            cur_x += 8;
        }
    }
}