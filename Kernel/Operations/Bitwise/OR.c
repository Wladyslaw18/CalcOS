/*
 * File: OR.c (refactored no more GPRXMM roundtrip crimes)
 * 
 * OLD CODE CRIME:
 * _mm_set_epi64x + _mm_storeu_si128 was doing:
 * GPR -> pinsrq -> XMM -> pand -> store -> GPR
 * Every pinsrq is ~5 cycles of store-forwarding stall.
 * For 2 doubles, that's 4 pinsrq + 1 por + 1 store + 2 casts = ~35 uops.
 * 
 * NEW CODE:
 * _mm_or_pd operates directly on double bits in XMM registers.
 * 1 load + 1 load + 1 or + 1 store = 4 uops for 2 doubles.
 * Zero register class crossings. Zero stalls.
 */

#include "OR.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

void or_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
    for (uint32_t i = 0; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] | *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
}

void or_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;
    for (; i + 1 < count; i += 2) {
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d vr = _mm_or_pd(va, vb);        // Bitwise OR on double bits
        _mm_storeu_pd(&result[i], vr);
    }
    for (; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] | *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
#else
    or_scalar(state, a, b, result, count);
#endif
}

void or_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;
    for (; i + 3 < count; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d vr = _mm256_or_pd(va, vb);      // 256-bit bitwise OR
        _mm256_storeu_pd(&result[i], vr);
    }
    for (; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] | *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
#else
    or_scalar(state, a, b, result, count);
#endif
}

/* ARM NEON: vandq_u64 / vorrq_u64 operate on raw 64-bit lanes - no FP register crossings */
void or_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#if defined(COMPILER_ARM) && (defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON))
    uint32_t i = 0;
    for (; i + 1 < count; i += 2) {
        uint64x2_t va = vld1q_u64((const uint64_t*)&a[i]);
        uint64x2_t vb = vld1q_u64((const uint64_t*)&b[i]);
        uint64x2_t vr = vorrq_u64(va, vb);
        vst1q_u64((uint64_t*)&result[i], vr);
    }
    for (; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] | *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
#else
    or_scalar(state, a, b, result, count);
#endif
}

void execute_or(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_neon) {
        or_neon(state, a, b, result, count);
    } else if (features->has_avx2) {
        or_avx2(state, a, b, result, count);
    } else if (features->has_sse2) {
        or_sse(state, a, b, result, count);
    } else {
        or_scalar(state, a, b, result, count);
    }
}
