# Bare Metal Calculator: Data-Oriented Design (DOD)

This document details the data layouts and memory layout paradigms used in the Bare Metal Calculator.

## Structure Layout: `CalculatorState`

To maximize cache efficiency and prevent CPU cache line splits, the entire runtime state of the calculator is designed to fit into exactly 64 bytes (the size of a standard CPU cache line) and is aligned to 64 bytes.

```c
// Alignment specifier ensures the compiler places the struct on a cache line boundary
struct __attribute__((aligned(64))) CalculatorState {
    int64_t accumulator;      // 8 bytes: Current accumulated value (fixed-point)
    int64_t current_input;    // 8 bytes: Active input value being typed (fixed-point)
    uint32_t flags;           // 4 bytes: State flags (is_negative, input_active, etc.)
    uint8_t active_op;        // 1 byte: Current active math operation (+, -, *, /)
    uint8_t cursor_pos;       // 1 byte: Position of the text editing cursor
    uint8_t decimal_scale;    // 1 byte: Decimal scale factor for fixed-point math
    uint8_t pad[41];          // 41 bytes: Pad to exactly 64 bytes
};
```

## Core Design Principles

### 1. Spatial Locality
- All frequently modified values reside together in `CalculatorState`.
- When the processor accesses any field, the entire state is loaded into L1 cache in a single transaction.

### 2. Cache Alignment
- By aligning the struct to 64 bytes (`alignas(64)` or `__attribute__((aligned(64)))`), we prevent the struct from crossing a cache line boundary. This avoids the latency penalty of loading two cache lines.

### 3. Flat Memory Arrays
- Input buffers and history logs are laid out in contiguous flat arrays rather than linked lists or node trees. This guarantees perfect linear memory access, enabling CPU prefetchers to work at peak efficiency.

### 4. Zero Indirection
- No pointers are stored inside the core processing state. All indexing uses flat offset values or direct lookups, avoiding pointer chasing overhead.
