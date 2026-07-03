#include "../Boot/BootConfig.h"

// boot_error is defined in BootError.c declared here since there's no
// shared header between the bootloader stages for this fatal handler.
extern void boot_error(const char* message);

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %b0, %w1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %w1, %b0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Verifies A20 line mapping using memory wrapping check */
int check_a20(void) {
    volatile unsigned int* low_mem = (volatile unsigned int*)0x07DFE;
    volatile unsigned int* high_mem = (volatile unsigned int*)0x107DFE;

    unsigned int saved_low = *low_mem;
    unsigned int saved_high = *high_mem;

    *low_mem = 0xDEADBEEF;
    *high_mem = 0xCAFEBABE;

    int enabled = (*low_mem != *high_mem);

    *low_mem = saved_low;
    *high_mem = saved_high;

    return enabled;
}

/* Enables A20 line using Fast A20 or Keyboard Controller */
void enable_a20(void) {
    if (check_a20()) return;

    /* Fast A20 Port 92h */
    unsigned char val = inb(0x92);
    if (!(val & 2)) {
        outb(0x92, val | 2);
    }

    if (check_a20()) return;

    /* Keyboard Controller fallback */
    /*
 FIX: Don't write to keyboard controller while busy (#21)
 * The old code had a 10K-iteration timeout but still wrote to 0x64
 * even if the controller was still busy (Input Buffer flag = bit 1).
 * Writing to 0x64 while the controller is busy can flood the command
 * queue and lock the keyboard interface, ignoring all future keystrokes.
 * Now checks the Output Buffer (bit 0) too, and only proceed when
 * both the controller is ready AND drain any pending output.
 */
    
    /* Wait for keyboard controller to be ready (input buffer empty) */
    int timeout = 100000;
    while (timeout > 0) {
        uint8_t status = inb(0x64);
        if ((status & 0x02) == 0) break; /* Input buffer empty = ready */
        /* Drain output buffer if there's data waiting */
        if (status & 0x01) {
            inb(0x60); /* Read and discard */
        }
        timeout--;
    }
    if (timeout <= 0) return; /* Controller hung, skip keyboard method */
    
    outb(0x64, 0xD1);

    timeout = 100000;
    while (timeout > 0) {
        if ((inb(0x64) & 0x02) == 0) break;
        timeout--;
    }
    if (timeout <= 0) return;
    
    outb(0x60, 0xDF);

    /* Give A20 gate time to settle */
    for (volatile int i = 0; i < 1000; i++) { __asm__ volatile ("nop"); }
    
    if (!check_a20()) { boot_error("Failed to enable A20 Gate via KBC"); }
}