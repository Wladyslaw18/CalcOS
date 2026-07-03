#ifndef VESA_FRAMEBUFFER_H
#define VESA_FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

// VESA Mode Information block structure (subset of fields necessary for framebuffer access)
#pragma pack(push, 1)
typedef struct {
    uint16_t attributes;
    uint8_t win_a, win_b;
    uint16_t granularity;
    uint16_t winsize;
    uint16_t segment_a, segment_b;
    uint32_t real_mode_pointer;
    uint16_t pitch;              // Bytes per scanline
    uint16_t width;              // Horizontal resolution in pixels
    uint16_t height;             // Vertical resolution in pixels
    uint8_t w_char, y_char, planes, bpp, banks;
    uint8_t memory_model, bank_size, image_pages;
    uint8_t reserved0;
    
    // Direct color fields
    uint8_t red_mask, red_position;
    uint8_t green_mask, green_position;
    uint8_t blue_mask, blue_position;
    uint8_t rsv_mask, rsv_position;
    uint8_t direct_color_attributes;
    
    uint32_t framebuffer;        // Physical address of the linear framebuffer
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t reserved1[206];
} VesaModeInfo;
#pragma pack(pop)

// RGB Color helpers
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} VesaColorRGB;

// Initialize VESA framebuffer parameters
void vesa_init(const VesaModeInfo* mode_info);

// Primitive operations
void vesa_put_pixel(uint32_t x, uint32_t y, VesaColorRGB color);
void vesa_clear(VesaColorRGB color);
void vesa_scroll(uint32_t scanlines, VesaColorRGB fill_color);

// Bresenham's line drawing algorithm (highly optimized/branchless where possible)
void vesa_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, VesaColorRGB color);

// Character / text rendering
void vesa_draw_char(char c, uint32_t x, uint32_t y, VesaColorRGB fg, VesaColorRGB bg);
void vesa_draw_string(const char* str, uint32_t x, uint32_t y, VesaColorRGB fg, VesaColorRGB bg);

#endif // VESA_FRAMEBUFFER_H
