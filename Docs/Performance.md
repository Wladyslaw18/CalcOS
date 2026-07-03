# Bare Metal Calculator: Performance Profile

This document details optimization techniques, profiling methods, and micro-architectural considerations.

## Branchless Programming

To prevent pipeline stalls and branch mispredictions (which incur a 15–20 cycle penalty on modern x86_64 cores), we employ branchless programming patterns for conditional logic.

### 1. Integer-to-ASCII (`itoa`)
Instead of conditional checks for formatting signs or selecting digits, lookup tables (LUTs) and bitmasks are used.

```c
// Example: Converting sign to character branchlessly
char sign_char = '-' * (value < 0) + ' ' * (value >= 0);
```

### 2. Lookup Tables (LUT)
Scancode translation and operator execution are routed through static lookup tables instead of `switch` or `if/else` statements.

## SIMD Vectorization

Where vector operations are applicable (e.g. updating the VGA video memory buffer, computing multi-word arithmetic, or performing batch conversions), SSE/AVX registers are utilized. See [SIMD.md](SIMD.md) for detailed vector strategies.

## Interrupt and Context-Switch Latency

- **No OS overhead**: The CPU doesn't cross user-to-kernel space or perform thread scheduling.
- **Fast Interrupt Service Routines (ISRs)**: Written in highly optimized assembly / naked C, pushing only minimal registers before handling and returning (`iretq`).
- **HLT State**: When idle, the CPU remains in a low-power `hlt` state, waking up instantly upon receiving keyboard hardware interrupts.
