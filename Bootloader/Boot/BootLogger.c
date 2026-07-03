#include "BootConfig.h"

/* VGA state variables */
static volatile char* const vga_buffer = (volatile char*)VGA_MEM_TEXT;
static int cursor_x = 0;
static int cursor_y = 0;

void boot_log_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i * 2] = ' ';
        vga_buffer[i * 2 + 1] = VGA_COLOR_WHITE_ON_BLACK;
    }
    cursor_x = 0;
    cursor_y = 0;
}

static void boot_log_scroll(void) {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[((y - 1) * VGA_WIDTH + x) * 2] = vga_buffer[(y * VGA_WIDTH + x) * 2];
            vga_buffer[((y - 1) * VGA_WIDTH + x) * 2 + 1] = vga_buffer[(y * VGA_WIDTH + x) * 2 + 1];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[((VGA_HEIGHT - 1) * VGA_WIDTH + x) * 2] = ' ';
        vga_buffer[((VGA_HEIGHT - 1) * VGA_WIDTH + x) * 2 + 1] = VGA_COLOR_WHITE_ON_BLACK;
    }
    cursor_y = VGA_HEIGHT - 1;
}

void boot_log_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        int index = (cursor_y * VGA_WIDTH + cursor_x) * 2;
        vga_buffer[index] = c;
        vga_buffer[index + 1] = VGA_COLOR_WHITE_ON_BLACK;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        boot_log_scroll();
    }
}

void boot_log_string(const char* str) {
    while (*str) {
        boot_log_char(*str++);
    }
}

void boot_log_hex(unsigned long long val) {
    char hex_chars[] = "0123456789ABCDEF";
    boot_log_string("0x");
    /* Print 16 hex digits (64-bit value) */
    for (int i = 60; i >= 0; i -= 4) {
        boot_log_char(hex_chars[(val >> i) & 0xF]);
    }
}
