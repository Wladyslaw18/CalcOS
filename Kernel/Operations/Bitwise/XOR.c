/*
 * File: XOR.c (refactored no more GPRXMM roundtrip crimes)
 * 
 * OLD CODE: _mm_set_epi64x + _mm_xor_si128 + _mm_storeu_si128
 * = 4 pinsrq (stall!) + 1 pxor + 1 store (stall!) + 2 GPR casts
 * 
 * NEW CODE: _mm_xor_pd on the double bits directly
 * = 2 vmovupd + 1 vxorpd + 1 vmovupd
 * = 4 uops, zero stalls, zero crossings.
 */

#include "XOR.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

/* Safe bitwise XOR on double bit patterns via uint64 memcpy alias - no UB, no FP cast */
void xor_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
    for (uint32_t i = 0; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] ^ *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
}

void xor_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;
    for (; i + 1 < count; i += 2) {
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d vr = _mm_xor_pd(va, vb);        // Bitwise XOR on double bits
        _mm_storeu_pd(&result[i], vr);
    }
    for (; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] ^ *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
#else
    xor_scalar(state, a, b, result, count);
#endif
}

void xor_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;
    for (; i + 3 < count; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d vr = _mm256_xor_pd(va, vb);      // 256-bit bitwise XOR
        _mm256_storeu_pd(&result[i], vr);
    }
    for (; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] ^ *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
#else
    xor_scalar(state, a, b, result, count);
#endif
}

void xor_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#if defined(COMPILER_ARM) && (defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON))
    uint32_t i = 0;
    for (; i + 1 < count; i += 2) {
        uint64x2_t va = vld1q_u64((const uint64_t*)&a[i]);
        uint64x2_t vb = vld1q_u64((const uint64_t*)&b[i]);
        uint64x2_t vr = veorq_u64(va, vb);
        vst1q_u64((uint64_t*)&result[i], vr);
    }
    for (; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] ^ *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
#else
    xor_scalar(state, a, b, result, count);
#endif
}

void execute_xor(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_neon) {
        xor_neon(state, a, b, result, count);
    } else if (features->has_avx2) {
        xor_avx2(state, a, b, result, count);
    } else if (features->has_sse2) {
        xor_sse(state, a, b, result, count);
    } else {
        xor_scalar(state, a, b, result, count);
    }
}

