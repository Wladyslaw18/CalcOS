#include "BootConfig.h"

/* Assembly functions for port I/O */
static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %b0, %w1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %w1, %b0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Play sound using the speaker */
void play_sound(unsigned int frequency) {
    unsigned int div;
    unsigned char tmp;

    /* Set PIT channel 2 frequency */
    div = 1193182 / frequency;
    outb(PORT_PIT_COMMAND, 0xB6);
    outb(PORT_PIT_DATA2, (unsigned char)(div & 0xFF));
    outb(PORT_PIT_DATA2, (unsigned char)((div >> 8) & 0xFF));

    /* Turn speaker on */
    tmp = inb(PORT_SPEAKER);
    if (tmp != (tmp | 3)) {
        outb(PORT_SPEAKER, tmp | 3);
    }
}

/* Stop speaker sound */
void stop_sound(void) {
    unsigned char tmp = inb(PORT_SPEAKER) & 0xFC;
    outb(PORT_SPEAKER, tmp);
}

/* Simple delay loop */
void boot_delay(unsigned int count) {
    for (volatile unsigned int i = 0; i < count * 200000; i++) {
        __asm__ volatile ("nop");
    }
}

/* Beep to signal error */
void boot_beep(unsigned int freq, unsigned int duration) {
    play_sound(freq);
    boot_delay(duration);
    stop_sound();
    boot_delay(duration / 2);
}

/* Error handler: prints message, plays sound, and halts */
void boot_error(const char* message) {
    extern void boot_log_string(const char* str);
    boot_log_string("\n*** BOOTLOADER FATAL ERROR: ");
    boot_log_string(message);
    boot_log_string(" ***\n");

    /* Distress Morse-like tone (S.O.S.) */
    for (int i = 0; i < 3; i++) boot_beep(880, 5);  /* S */
    boot_delay(10);
    for (int i = 0; i < 3; i++) boot_beep(660, 15); /* O */
    boot_delay(10);
    for (int i = 0; i < 3; i++) boot_beep(880, 5);  /* S */

    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}
