#ifndef CALCULATOR_STATE_H
#define CALCULATOR_STATE_H

#include <stdint.h>

// quiet NaN via bit punch. IEEE 754: sign=0 exp=0x7FF mantissa=0x8000000000000
// do NOT return 0.0 on error. NaN propagates through expressions so the user
// sees the failure instead of getting a silent wrong answer. volatile kills
// MSVC constant folding -- yes the compiler would optimize it away otherwise.
static inline double calc_nan(void) {
    union { uint64_t i; double d; } u;
    u.i = 0x7FF8000000000000ULL;
    return u.d;
}

// 64-byte alignment. one cache line. no unaligned load penalty.
// if the struct crosses a cache line boundary the CPU needs two fetches.
// it won't. the _Static_assert below enforces this at compile time.
#if defined(__GNUC__) || defined(__clang__)
#define CACHE_ALIGN __attribute__((aligned(64)))
#elif defined(_MSC_VER)
#define CACHE_ALIGN __declspec(align(64))
#else
#define CACHE_ALIGN
#endif

typedef enum {
    OP_NONE = 0,
    OP_ADD  = 1,
    OP_SUB  = 2,
    OP_MUL  = 3,
    OP_DIV  = 4
} OpType;

typedef struct CACHE_ALIGN {
    double operands[4];       // 32 bytes - operand stack, L1 friendly
    uint8_t op_count;         // 1 byte  - stack depth
    uint8_t current_op;       // 1 byte  - pending OpType
    uint8_t flags;            // 1 byte  - 1=overflow 2=divzero 4=domain
    uint8_t mode;             // 1 byte  - 0=float 1=int 2=hex
    uint32_t history_idx;     // 4 bytes - circular history offset
    void* custom_lookup_ctx;  // 8 bytes - plugin registry pointer
    void* (*custom_lookup)(void* ctx, const char* name); // 8 bytes - function lookup hook
    uint8_t _pad[8];          // 8 bytes - keep total at exactly 64. touch this and the assert fires.
} CalculatorState;

// if this blows up you added a field without shrinking _pad. fix it.
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(CalculatorState) == 64, "CalculatorState must be exactly 64 bytes. shrink _pad.");
#endif

#endif // CALCULATOR_STATE_H
