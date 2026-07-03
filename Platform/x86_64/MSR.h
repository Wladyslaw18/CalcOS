#ifndef MSR_H
#define MSR_H

#include <stdint.h>

// Standard MSR constants
#define IA32_APIC_BASE      0x0000001B
#define IA32_EFER           0xC0000080
#define IA32_STAR           0xC0000081
#define IA32_LSTAR          0xC0000082
#define IA32_CSTAR          0xC0000083
#define IA32_FMASK          0xC0000084
#define IA32_FS_BASE        0xC0000100
#define IA32_GS_BASE        0xC0000101
#define IA32_KERNEL_GS_BASE 0xC0000102

uint64_t read_msr(uint32_t msr);
void write_msr(uint32_t msr, uint64_t value);

#endif // MSR_H
