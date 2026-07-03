/*
 * File: AND.c
 * Author: W. Kowal (refactored by performance elves)
 * 
 * THE PROBLEM WITH THE OLD CODE:
 * _mm_set_epi64x() + _mm_storeu_si128() was doing GPRXMMstackGPR roundtrips
 * for every pair of doubles. Each pinsrq costs ~5 cycles, each store-forwarding stall
 * costs ~10 cycles. The "vectorized" version was SLOWER than scalar.
 * 
 * THE FIX: Use _mm_and_pd directly on double bits zero register class crossing.
 * Or just use scalar which is actually faster for small arrays (which bitwise
 * ops on doubles usually are nobody does 10,000 bitwise-ANDs on floats).
 * 
 * For AVX2: same story but 4-wide. _mm256_and_pd, no roundtrips.
 */

#include "AND.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

/*
 * SCALAR FALLBACK: Simple GPR bitwise AND on the double's bit pattern.
 * Turns out this is actually FAST because:
 *   - No XMM register pressure
 *   - No store-forwarding stalls
 *   - Compiler can fuse the load+cast+and+cast+store into 3-4 uops
 *   - The GPR integer ALU is already 6-wide on modern x86
 *
 * For small arrays (< 32 elements), this is the fastest option.
 */
void and_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
    // FIX: Don't cast through int64! That truncates the value.
    // Use raw IEEE 754 bitwise AND on double bits, matching _mm_and_pd exactly.
    for (uint32_t i = 0; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] & *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
}

/*
 * SSE VECTORIZED: Use _mm_and_pd to operate directly on double bit patterns
 * inside XMM registers. NO CAST, NO PINSRQ, NO STORE-FORWARDING STALL.
 * 
 * _mm_and_pd performs bitwise AND on the 128 bits of two XMM registers.
 * Since IEEE 754 doubles store their sign, exponent, and mantissa as contiguous bits,
 * _mm_and_pd operates on those bits directly. The bits ARE the double.
 * 
 * No need to cast to int64 and back the hardware does the right thing.
 * This eliminates ALL GPRXMM boundary crossings.
 */
void and_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;

    // Process 2 doubles at a time ZERO GPR CROSSINGS
    for (; i + 1 < count; i += 2) {
        __m128d va = _mm_loadu_pd(&a[i]);      // Load double bits directly into XMM
        __m128d vb = _mm_loadu_pd(&b[i]);      // (no int64 conversion needed!)
        __m128d vr = _mm_and_pd(va, vb);         // Bitwise AND on the double bits
        _mm_storeu_pd(&result[i], vr);           // Store back as doubles
    }

    // Tail handling
    for (; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] & *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
#else
    and_scalar(state, a, b, result, count);
#endif
}

/*
 * AVX2 VECTORIZED: 4-wide version of the same. _mm256_and_pd on 256-bit registers.
 * Still zero GPR crossings. Each iteration processes 4 doubles with exactly:
 * 2 vmovupd loads + 1 vandpd + 1 vmovupd store = 4 uops
 * Compare that to the old version which had ~20 uops from pinsrq spills.
 */
void and_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;

    for (; i + 3 < count; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);     // Load 4 doubles
        __m256d vb = _mm256_loadu_pd(&b[i]);     // Load 4 doubles
        __m256d vr = _mm256_and_pd(va, vb);       // 256-bit bitwise AND
        _mm256_storeu_pd(&result[i], vr);          // Store 4 results
    }

    for (; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] & *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
#else
    and_scalar(state, a, b, result, count);
#endif
}

/*
 * ARM NEON: Same principle using vandq_u64 on uint64x2_t registers.
 * NEON doesn't distinguish float/int at the bitwise level vandq_u64 is
 * register-format agnostic. No lane insertions, no roundtrips.
 */
void and_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
#if defined(COMPILER_ARM) && (defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON))
    uint32_t i = 0;

    for (; i + 1 < count; i += 2) {
        uint64x2_t va = vld1q_u64((const uint64_t*)&a[i]);  // Load raw 64-bit lanes
        uint64x2_t vb = vld1q_u64((const uint64_t*)&b[i]);  // Zero conversion cost
        uint64x2_t vr = vandq_u64(va, vb);                    // Bitwise AND
        vst1q_u64((uint64_t*)&result[i], vr);                  // Store as-is
    }

    for (; i < count; ++i) {
        uint64_t tmp = *(const uint64_t*)&a[i] & *(const uint64_t*)&b[i];
        result[i] = *(double*)&tmp;
    }
#else
    and_scalar(state, a, b, result, count);
#endif
}

void execute_and(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_neon) {
        and_neon(state, a, b, result, count);
    } else if (features->has_avx2) {
        and_avx2(state, a, b, result, count);
    } else if (features->has_sse2) {
        and_sse(state, a, b, result, count);
    } else {
        and_scalar(state, a, b, result, count);
    }
}
