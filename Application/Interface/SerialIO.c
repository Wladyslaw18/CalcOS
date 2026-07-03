#include "SerialIO.h"

static inline void outb(uint16_t port, uint8_t val) {
#ifndef _MSC_VER
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
#else
    (void)port; (void)val;
#endif
}

static inline uint8_t inb(uint16_t port) {
#ifndef _MSC_VER
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
#else
    (void)port;
    return 0;
#endif
}

void serial_init(void) {
    outb(PORT_COM1 + 1, 0x00);    // Disable all interrupts
    outb(PORT_COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(PORT_COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT_COM1 + 1, 0x00);    //                  (hi byte)
    outb(PORT_COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(PORT_COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT_COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int serial_received(void) {
    return inb(PORT_COM1 + 5) & 1;
}

char serial_read(void) {
    while (serial_received() == 0);
    return (char)inb(PORT_COM1);
}

int is_transmit_empty(void) {
    return inb(PORT_COM1 + 5) & 0x20;
}

void serial_write_char(char a) {
    while (is_transmit_empty() == 0);
    outb(PORT_COM1, (uint8_t)a);
}

void serial_write_str(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_write_char(str[i]);
    }
}
