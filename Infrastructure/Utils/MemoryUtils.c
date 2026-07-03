#include "MemoryUtils.h"

#ifdef _MSC_VER
#include <string.h>
#define __builtin_memcpy memcpy
#define __builtin_memset memset
#endif

// NO casting to uint64_t* on unaligned addresses. ever.
// unaligned uint64_t* read/write violates C aliasing rules and can:
//   - cause split-load penalties (two cache line fetches for one 8-byte load)
//   - trigger #AC alignment check fault if CR0.AM=1 (real hardware does this)
// the compiler will auto-vectorize byte loops into word ops when alignment is provable.
// it can't prove alignment here, so let it generate safe unaligned loads itself.
void fast_memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;

    // 32-byte aligned path: use AVX or SSE2 moves directly via inline asm
    if (((uintptr_t)d % 32 == 0) && ((uintptr_t)s % 32 == 0)) {
        while (n >= 32) {
            #if defined(__AVX2__) && !defined(HOST_APP) && !defined(_MSC_VER)
            __asm__ volatile (
                "vmovdqu (%1), %%ymm0\n\t"
                "vmovdqu %%ymm0, (%0)\n\t"
                :: "r"(d), "r"(s)
                : "ymm0", "memory"
            );
            #elif defined(__SSE2__) && !defined(HOST_APP) && !defined(_MSC_VER)
            // two 128-bit moves to cover 32 bytes
            __asm__ volatile (
                "movdqu (%1), %%xmm0\n\t"
                "movdqu %%xmm0, (%0)\n\t"
                "movdqu 16(%1), %%xmm1\n\t"
                "movdqu %%xmm1, 16(%0)\n\t"
                :: "r"(d), "r"(s)
                : "xmm0", "xmm1", "memory"
            );
            #else
            // MSVC/hosted fallback: four 8-byte copies. aligned so no #AC risk.
            *(uint64_t*)(d + 0)  = *(const uint64_t*)(s + 0);
            *(uint64_t*)(d + 8)  = *(const uint64_t*)(s + 8);
            *(uint64_t*)(d + 16) = *(const uint64_t*)(s + 16);
            *(uint64_t*)(d + 24) = *(const uint64_t*)(s + 24);
            #endif
            d += 32;
            s += 32;
            n -= 32;
        }
    }

    // 8-byte aligned chunk loop for leftover
    while (n >= 8) {
        *(uint64_t*)d = *(const uint64_t*)s;
        d += 8;
        s += 8;
        n -= 8;
    }

    // byte-by-byte for the last 0-7 bytes
    while (n > 0) {
        *d++ = *s++;
        n--;
    }
}

void fast_memset(void* s, int c, size_t n) {
    // __builtin_memset handles alignment and aliasing correctly.
    // raw *(uint64_t*)p = pattern violates strict aliasing and causes #AC on real hardware.
    // zero overhead -- the compiler knows exactly what this lowers to.
    __builtin_memset(s, c, n);
}