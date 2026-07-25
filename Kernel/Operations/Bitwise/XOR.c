#include "XOR.h"
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

static inline double bitwise_xor_scalar_single(double a, double b) {
    uint64_t ua, ub, ur;
    memcpy(&ua, &a, sizeof(double));
    memcpy(&ub, &b, sizeof(double));
    ur = ua ^ ub;
    double r;
    memcpy(&r, &ur, sizeof(double));
    return r;
}

void xor_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
    for (uint32_t i = 0; i < count; ++i) {
        result[i] = bitwise_xor_scalar_single(a[i], b[i]);
    }
}

void xor_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;
    for (; i + 1 < count; i += 2) {
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d vr = _mm_xor_pd(va, vb);
        _mm_storeu_pd(&result[i], vr);
    }
    for (; i < count; ++i) {
        result[i] = bitwise_xor_scalar_single(a[i], b[i]);
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
        __m256d vr = _mm256_xor_pd(va, vb);
        _mm256_storeu_pd(&result[i], vr);
    }
    for (; i < count; ++i) {
        result[i] = bitwise_xor_scalar_single(a[i], b[i]);
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
        float64x2_t va = vld1q_f64(&a[i]);
        float64x2_t vb = vld1q_f64(&b[i]);
        uint64x2_t vr = veorq_u64(vreinterpretq_u64_f64(va), vreinterpretq_u64_f64(vb));
        vst1q_f64(&result[i], vreinterpretq_f64_u64(vr));
    }
    for (; i < count; ++i) {
        result[i] = bitwise_xor_scalar_single(a[i], b[i]);
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
