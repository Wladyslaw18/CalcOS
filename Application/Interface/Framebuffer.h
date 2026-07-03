#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEM 0xB8000

typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_YELLOW = 14,
    VGA_COLOR_WHITE = 15
} VgaColor;

static inline uint8_t vga_entry_color(VgaColor fg, VgaColor bg) {
    return (uint8_t)(fg | (bg << 4));
}

void framebuffer_clear(void);
void framebuffer_putc(char c, uint8_t color, int x, int y);
void framebuffer_write(const char* str, uint8_t color, int x, int y);
void framebuffer_scroll(void);
void framebuffer_set_cursor(int x, int y);

#endif // FRAMEBUFFER_H
