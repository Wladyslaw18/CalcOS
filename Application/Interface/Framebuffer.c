#include "Framebuffer.h"

static inline void outb(uint16_t port, uint8_t val) {
#ifndef _MSC_VER
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
#else
    (void)port; (void)val;
#endif
}

void framebuffer_clear(void) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEM;
    uint16_t clear_val = (uint16_t)(' ' | (vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK) << 8));
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = clear_val;
    }
    framebuffer_set_cursor(0, 0);
}

void framebuffer_putc(char c, uint8_t color, int x, int y) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEM;
    vga[y * VGA_WIDTH + x] = (uint16_t)(c | (color << 8));
}

void framebuffer_scroll(void) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEM;
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga[(y - 1) * VGA_WIDTH + x] = vga[y * VGA_WIDTH + x];
        }
    }
    uint16_t clear_val = (uint16_t)(' ' | (vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK) << 8));
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = clear_val;
    }
}

void framebuffer_write(const char* str, uint8_t color, int x, int y) {
    int cur_x = x;
    int cur_y = y;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cur_x = x;
            cur_y++;
            if (cur_y >= VGA_HEIGHT) {
                framebuffer_scroll();
                cur_y = VGA_HEIGHT - 1;
            }
        } else {
            framebuffer_putc(str[i], color, cur_x, cur_y);
            cur_x++;
            if (cur_x >= VGA_WIDTH) {
                cur_x = 0;
                cur_y++;
                if (cur_y >= VGA_HEIGHT) {
                    framebuffer_scroll();
                    cur_y = VGA_HEIGHT - 1;
                }
            }
        }
    }
}

void framebuffer_set_cursor(int x, int y) {
    uint16_t pos = (uint16_t)(y * VGA_WIDTH + x);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}
