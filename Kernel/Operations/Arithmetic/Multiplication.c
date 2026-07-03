#include "Multiplication.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

// scalar fallback. no overflow guards needed -- IEEE 754 handles inf.
void mul_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
    for (uint32_t i = 0; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
}

// SSE2: 2 doubles per cycle. 128-bit multiply.
void mul_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;
    for (; i + 1 < count; i += 2) {
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d vr = _mm_mul_pd(va, vb);
        _mm_storeu_pd(&result[i], vr);
    }
    for (; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
#else
    mul_scalar(state, a, b, result, count);
#endif
}

// AVX2: 4 doubles per cycle. tail: SSE2 for 2-element remainder, scalar for last 1.
void mul_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;
    for (; i + 3 < count; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d vr = _mm256_mul_pd(va, vb);
        _mm256_storeu_pd(&result[i], vr);
    }
    // 2-element SSE2 tail: half a 256-bit lane, don't waste it
    if (i + 1 < count) {
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d vr = _mm_mul_pd(va, vb);
        _mm_storeu_pd(&result[i], vr);
        i += 2;
    }
    for (; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
#else
    mul_scalar(state, a, b, result, count);
#endif
}

// NEON: AArch64 float64x2 multiply. vmulq_f64, 2 doubles per instruction.
void mul_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#if defined(COMPILER_ARM) && (defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON))
    uint32_t i = 0;
    for (; i + 1 < count; i += 2) {
        float64x2_t va = vld1q_f64(&a[i]);
        float64x2_t vb = vld1q_f64(&b[i]);
        float64x2_t vr = vmulq_f64(va, vb);
        vst1q_f64(&result[i], vr);
    }
    for (; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
#else
    mul_scalar(state, a, b, result, count);
#endif
}

// runtime dispatch. NEON first -- ARM builds don't have AVX2.
void execute_multiplication(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_neon) {
        mul_neon(state, a, b, result, count);
    } else if (features->has_avx2) {
        mul_avx2(state, a, b, result, count);
    } else if (features->has_sse2) {
        mul_sse(state, a, b, result, count);
    } else {
        mul_scalar(state, a, b, result, count);
    }
}
