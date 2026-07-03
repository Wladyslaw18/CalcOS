# Bare Metal Calculator: SIMD Strategy

This document details the usage of Single Instruction Multiple Data (SIMD) instructions within the Bare Metal Calculator.

## Supported SIMD Features

The kernel targets the x86_64 architecture and takes advantage of:
1. **SSE/SSE2 (128-bit registers)**: Guaranteed to be present on all x86_64 CPUs. Used for parallel memory moves and character conversions.
2. **AVX/AVX2 (256-bit registers)**: Enabled dynamically or compile-time when supported. Used for fast rendering of screen arrays and line clearing.

## Key Applications

### 1. Screen Buffer Clearance
Clearing the 80x25 VGA text screen (`0xB8000`) involves writing `80 * 25 = 2000` character-attribute cells (4000 bytes). Rather than a byte-by-byte loop, we write 32 bytes (16 characters) at a time using AVX2 registers:

```c
#include <immintrin.h>

void vga_clear_simd(uint16_t attribute_entry) {
    // Fill a 256-bit AVX2 register with the entry (16 copies of the 16-bit cell)
    __m256i vec = _mm256_set1_epi16(attribute_entry);
    
    uint8_t* vga_mem = (uint8_t*)0xB8000;
    for (int i = 0; i < 4000; i += 32) {
        _mm256_store_si256((__m256i*)(vga_mem + i), vec);
    }
}
```

### 2. Multi-word Fixed-Point Math
For calculations requiring higher precision than 64-bit integer limits, SIMD parallel addition and multiplication are used to execute multi-word arithmetic without sequence-dependent carries slowing down the pipeline.

## Boot-time FPU/SSE/AVX Enabling

Before using any SIMD instruction, the kernel must enable the FPU and SIMD extensions in the CPU control registers.

```nasm
; Assembly code to enable SSE/AVX
mov rax, cr0
and ax, 0xFFFB      ; Clear TS (Task Switched)
or ax, 0x0002       ; Set MP (Monitor Coprocessor)
mov cr0, rax

mov rax, cr4
or ax, 3 << 9       ; Set OSFXSR and OSXMMEXCPT (SSE support)
mov cr4, rax
```
