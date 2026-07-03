/*
 * paravirt_serial.c Unikernel (KVM/Firecracker) Serial Display Driver
 * 
 * Implements the DisplayDriver interface (display.h) for KVM-based
 * unikernels using paravirtualized serial I/O. Designed for Solo5
 * tender interfaces and Firecracker micro-VMs.
 * 
 * In a unikernel, there's no graphics hardware output goes to
 * the VM console via COM1 serial port (0x3F8). This driver strips
 * ALL graphical operations and maps write_str directly to serial.
 * 
 * COMPILE with: -DPLATFORM_UNIKERNEL
 * Toolchain: x86_64-solo5-none-static or custom freestanding
 * 
 * THE FLEX: 3MB micro-VM on Firecracker running a calculator.
 * No kernel. No init. Just serial out + CPU cycles.
 * 
 * Firecracker setup:
 * # ./calc_unikernel --config <(echo '{}')
 * Output appears in the VM serial console
 * Logged to host via `screen /dev/pts/X`
 */

#include "../../include/calc/display.h"
#include <stdint.h>
#include <string.h>

#ifdef PLATFORM_UNIKERNEL

// x86_64 Serial Port I/O (no OS, direct port access)
#define COM1_PORT 0x3F8

// FIX: Namespaced to avoid collision with PortIO.c globals (#7)
static inline void unik_outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t unik_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Wait for serial transmitter to be ready
static void serial_wait_tx(void) {
    for (int i = 0; i < 100000; i++) {
        if (unik_inb(COM1_PORT + 5) & 0x20) break;
    }
}

// Write a single character to serial
static void serial_putc(char c) {
    serial_wait_tx();
    unik_outb(COM1_PORT, (uint8_t)c);
    // Handle LF CRLF for proper terminal output
    if (c == '\n') {
        serial_wait_tx();
        unik_outb(COM1_PORT, '\r');
    }
}

// DisplayDriver implementation

// put_char: Write character to serial console
static void unik_put_char(const DisplayDriver* self, char c,
                           uint32_t x, uint32_t y,
                           UIColor fg, UIColor bg) {
    (void)self; (void)x; (void)y; (void)fg; (void)bg;
    serial_putc(c);
}

// write_str: Write null-terminated string to serial
static void unik_write_str(const DisplayDriver* self, const char* str,
                            uint32_t x, uint32_t y,
                            UIColor fg, UIColor bg) {
    (void)self; (void)x; (void)y; (void)fg; (void)bg;
    if (!str) return;
    while (*str) serial_putc(*str++);
}

// clear: Send form-feed equivalent
static void unik_clear(const DisplayDriver* self, UIColor bg) {
    (void)self; (void)bg;
    // ANSI escape: clear screen, home cursor
    serial_putc('\033');
    serial_putc('[');
    serial_putc('2');
    serial_putc('J');
    serial_putc('\033');
    serial_putc('[');
    serial_putc('H');
}

// All pixel operations are NO-OPs in serial/console mode
static void unik_put_pixel(const DisplayDriver* self, uint32_t x, uint32_t y,
                            uint8_t r, uint8_t g, uint8_t b) {
    (void)self; (void)x; (void)y; (void)r; (void)g; (void)b;
}

static void unik_draw_line(const DisplayDriver* self,
                            int32_t x0, int32_t y0,
                            int32_t x1, int32_t y1,
                            UIColor color) {
    (void)self; (void)x0; (void)y0; (void)x1; (void)y1; (void)color;
}

static void unik_present(const DisplayDriver* self) {
    (void)self;
    // Serial is unbuffered no flush needed
}

// Public API
int unik_display_init(DisplayDriver* driver) {
    if (!driver) return -1;

    // Initialize serial port (9600 baud, 8N1)
    unik_outb(COM1_PORT + 1, 0x00);    // Disable interrupts
    unik_outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    unik_outb(COM1_PORT + 0, 0x03);    // Divisor low = 3 (38400 baud)
    unik_outb(COM1_PORT + 1, 0x00);    // Divisor high = 0
    unik_outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, 1 stop bit
    unik_outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
    unik_outb(COM1_PORT + 4, 0x0B);    // IRQ enabled, RTS/DSR set

    // Wire up DisplayDriver interface
    driver->context    = NULL;      // No dynamic context needed
    driver->width      = 0;
    driver->height     = 0;
    driver->text_cols  = 80;       // Standard serial terminal
    driver->text_rows  = 25;
    driver->bpp        = 0;        // Text mode
    driver->put_char   = unik_put_char;
    driver->write_str  = unik_write_str;
    driver->clear      = unik_clear;
    driver->put_pixel  = unik_put_pixel;
    driver->draw_line  = unik_draw_line;
    driver->present    = unik_present;

    unik_write_str(driver, "\r\n=== Calc Engine — Unikernel Mode ===\r\n", 0, 0, 0, 0);
    return 0;
}

void unik_display_deinit(DisplayDriver* driver) {
    (void)driver;
    unik_write_str(driver, "\r\n=== Shutdown ===\r\n", 0, 0, 0, 0);
}

#else // Not unikernel compilation stub
int unik_display_init(DisplayDriver* driver) { (void)driver; return -99; }
void unik_display_deinit(DisplayDriver* driver) { (void)driver; }
#endif // PLATFORM_UNIKERNEL