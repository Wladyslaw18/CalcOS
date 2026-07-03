#include "Addition.h"
#include <immintrin.h>

// scalar fallback. no SIMD. works on anything. grandma's Pentium included.
void add_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
    for (uint32_t i = 0; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
}

// SSE2: 128-bit registers, 2 doubles per instruction. minimum x86_64 baseline.
void add_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
    uint32_t i = 0;
    for (; i + 1 < count; i += 2) {
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d vr = _mm_add_pd(va, vb);
        _mm_storeu_pd(&result[i], vr);
    }
    // tail: leftover single element, can't fit in a 128-bit lane
    for (; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
}

// AVX2: 256-bit registers, 4 doubles per instruction. twice the throughput of SSE2.
// tail falls back to SSE2 for 2-element remainder, then scalar for the last 1.
// don't go straight to scalar for the 2-element tail -- wastes half a vector unit.
void add_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    (void)state;
    uint32_t i = 0;
    for (; i + 3 < count; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d vr = _mm256_add_pd(va, vb);
        _mm256_storeu_pd(&result[i], vr);
    }
    // 2-element spillover: use SSE2 instead of burning 2 scalar ops
    if (i + 1 < count) {
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d vr = _mm_add_pd(va, vb);
        _mm_storeu_pd(&result[i], vr);
        i += 2;
    }
    // final single-element cleanup
    for (; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
}

// runtime dispatch. checked once at init via CPUID, zero branch overhead after that.
void execute_addition(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_avx2) {
        add_avx2(state, a, b, result, count);
    } else if (features->has_sse2) {
        add_sse(state, a, b, result, count);
    } else {
        add_scalar(state, a, b, result, count);
    }
}
