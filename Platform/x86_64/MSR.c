#include "MSR.h"

uint64_t read_msr(uint32_t msr) {
#if defined(__GNUC__) || defined(__clang__)
    uint32_t low, high;
    __asm__ __volatile__ ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr) : "memory");
    return ((uint64_t)high << 32) | low;
#else
    (void)msr;
    return 0;
#endif
}

void write_msr(uint32_t msr, uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32);
    __asm__ __volatile__ ("wrmsr" : : "a"(low), "d"(high), "c"(msr) : "memory");
#else
    (void)msr;
    (void)value;
#endif
}
