#include "Mailbox.h"

// Mailbox MMIO register offsets
#define MBOX_READ      ((volatile uint32_t*)(MMIO_BASE + 0x00B880))
#define MBOX_STATUS    ((volatile uint32_t*)(MMIO_BASE + 0x00B898))
#define MBOX_WRITE     ((volatile uint32_t*)(MMIO_BASE + 0x00B8A0))

#define MBOX_FULL      0x80000000
#define MBOX_EMPTY     0x40000000

bool mbox_call(uint32_t channel, volatile uint32_t* mbox) {
    // 1. Calculate the physical address representing the message payload
    // The lower 4 bits of the address are used for channel selection, so the address must be 16-byte aligned!
    uint32_t addr = (uint32_t)((uint64_t)mbox & ~0xF) | (channel & 0xF);

    // 2. Wait until the GPU is ready to receive messages (Status write queue is not full)
    while (*MBOX_STATUS & MBOX_FULL) {
        __asm__ volatile("nop");
    }

    // 3. Write this message address to the GPU write register
    *MBOX_WRITE = addr;

    // 4. Wait for the reply from GPU
    while (1) {
        while (*MBOX_STATUS & MBOX_EMPTY) {
            __asm__ volatile("nop");
        }
        
        // Check if the reply is for this channel
        if (*MBOX_READ == addr) {
            // Check if the request succeeded (GPU status bit 31 = Success)
            return mbox[1] == 0x80000000;
        }
    }
    return false;
}
