#include <stdint.h>
#include "Shift.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

static inline int64_t perform_shift(int64_t val, int64_t count) {
    if (count >= 0) {
        if (count >= 64) return 0;
        return val << count;
    } else {
        // FIX: INT64_MIN overflow
        // -INT64_MIN = -(-2^63) = 2^63 which overflows int64_t (UB in C!)
        // Use uint64_t for safe negation of the full signed range
        uint64_t abs_count = (count == INT64_MIN) 
            ? (uint64_t)INT64_MAX + 1  // = 2^63, which is > 63 treated as >=64
            : (uint64_t)(-count);
        if (abs_count >= 64) return (val < 0) ? -1 : 0;
        return val >> (int)abs_count;
    }
}

void shift_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
    for (uint32_t i = 0; i < count; ++i) {
        result[i] = (double)perform_shift((int64_t)a[i], (int64_t)b[i]);
    }
}

void shift_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    // Shifts with variable, per-element counts are dynamic and best optimized through scalar helpers
    // unless AVX-512 variable shifts are available.
    shift_scalar(state, a, b, result, count);
}

void shift_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    shift_scalar(state, a, b, result, count);
}

void shift_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    shift_scalar(state, a, b, result, count);
}

void execute_shift(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_neon) {
        shift_neon(state, a, b, result, count);
    } else if (features->has_avx2) {
        shift_avx2(state, a, b, result, count);
    } else if (features->has_sse2) {
        shift_sse(state, a, b, result, count);
    } else {
        shift_scalar(state, a, b, result, count);
    }
}