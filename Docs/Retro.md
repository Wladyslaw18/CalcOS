# Retro CPU Compatibility

CalcOS runs on hardware going back to 1989. This document explains how.

## The x87 FPU Path

Pre-SSE CPUs (486, Pentium) have no XMM registers. The SIMD detection in `CPUID.c`
returns `has_sse2 = false`, and all math falls back to scalar C code.

The scalar path uses standard IEEE 754 double-precision arithmetic, which maps
directly to x87 FPU instructions when compiled for i386/486 targets.

The `Infrastructure/Platform/Retro1990/x87FPU.c` module provides helper wrappers
for x87 FPU control word manipulation (rounding mode, precision control).

## The 1990 Boot Path

`Infrastructure/Platform/Retro1990/BootEntry1990.asm` is an alternative boot sector
for true 32-bit flat-model boots without Long Mode. It:

1. Switches to 32-bit Protected Mode (PE bit in CR0)
2. Sets up a flat GDT (base=0, limit=4GB)
3. Jumps to the C kernel at `0x8000`

No Long Mode. No PAE. No 64-bit anything. Pure 32-bit flat model.

## Memory Footprint on a 486

- Boot sector: 512 bytes
- Kernel code: ~80 KB
- Stack: 64 KB (`0x90000` base)
- Parser state: ~6 KB (symbol table + stack frames)
- VGA buffer: 4 KB

**Total active RAM: ~150 KB out of 640 KB conventional memory.**

The machine spends 490 KB just... waiting. Available for future features.
