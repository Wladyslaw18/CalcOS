#include "Logger.h"
#include "../../Platform/x86_64/PortIO.h"

#define COM1_PORT 0x3F8

void logger_init(void) {
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set divisor)
    outb(COM1_PORT + 0, 0x03);    // Divisor low byte (38400 baud)
    outb(COM1_PORT + 1, 0x00);    // Divisor high byte
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, 1 stop bit
    outb(COM1_PORT + 2, 0xC7);    // FIFO enable and clear
    outb(COM1_PORT + 4, 0x0B);    // Enable IRQs, set RTS and DTR
}

static int is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void logger_write_char(char c) {
    while (is_transmit_empty() == 0);
    outb(COM1_PORT, c);
}

void logger_write_string(const char* str) {
    if (!str) return;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            logger_write_char('\r');
        }
        logger_write_char(str[i]);
    }
}

void logger_write_hex(unsigned long long val) {
    char buf[20];
    int idx = 0;
    if (val == 0) {
        logger_write_char('0');
        return;
    }
    while (val > 0) {
        int digit = val & 0xF;
        buf[idx++] = (digit < 10) ? ('0' + digit) : ('A' + (digit - 10));
        val >>= 4;
    }
    for (int i = idx - 1; i >= 0; i--) {
        logger_write_char(buf[i]);
    }
}

void logger_write_dec(long long val) {
    char buf[24];
    int idx = 0;
    int negative = 0;
    unsigned long long uval;
    if (val < 0) {
        negative = 1;
        uval = -(unsigned long long)val;
    } else {
        uval = (unsigned long long)val;
    }
    if (uval == 0) {
        logger_write_char('0');
        return;
    }
    while (uval > 0) {
        buf[idx++] = '0' + (uval % 10);
        uval /= 10;
    }
    if (negative) {
        logger_write_char('-');
    }
    for (int i = idx - 1; i >= 0; i--) {
        logger_write_char(buf[i]);
    }
}

void logger_log(LogLevel level, const char* message) {
    switch (level) {
        case LOG_LEVEL_DEBUG:
            logger_write_string("[DEBUG] ");
            break;
        case LOG_LEVEL_INFO:
            logger_write_string("[INFO]  ");
            break;
        case LOG_LEVEL_WARN:
            logger_write_string("[WARN]  ");
            break;
        case LOG_LEVEL_ERROR:
            logger_write_string("[ERROR] ");
            break;
        case LOG_LEVEL_FATAL:
            logger_write_string("[FATAL] ");
            break;
    }
    logger_write_string(message);
    logger_write_string("\n");
}
