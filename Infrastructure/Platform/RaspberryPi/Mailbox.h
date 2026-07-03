#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>
#include <stdbool.h>

// Raspberry Pi physical registers base address
// Pi 3 uses 0x3F000000, Pi 4 uses 0xFE000000. target Pi 3/4 baseline.
#define MMIO_BASE       0x3F000000

#define MBOX_REQUEST    0
#define MBOX_CH_PROP    8

// Mailbox buffer alignment constraint (MUST be 16-byte aligned for the GPU!)
#define MBOX_ALIGN __attribute__((aligned(16)))

// Communicate with VideoCore GPU
bool mbox_call(uint32_t channel, volatile uint32_t* mbox);

#endif // MAILBOX_H
