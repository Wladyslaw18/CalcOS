#include "Division.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

// scalar fallback: b==0 produces NaN, not a crash.
void div_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    int has_zero = 0;
    for (uint32_t i = 0; i < count; ++i) {
        int is_zero = (b[i] == 0.0);
        has_zero |= is_zero;
        result[i] = is_zero ? calc_nan() : (a[i] / b[i]);
    }
    if (state && has_zero) {
        state->flags |= 2; // DivZero flag
    }
}

// SSE2 division with branchless zero-lane replacement.
// mask where vb==0, substitute 1.0 in those lanes so the division doesn't trap,
// then blend NaN over the result in those same lanes.
void div_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
#ifdef COMPILER_X86
    uint32_t i = 0;
    __m128d v_zero = _mm_setzero_pd();
    __m128d v_nan = _mm_set1_pd(calc_nan());
    __m128d v_one = _mm_set1_pd(1.0);
    int mask_accum = 0;
    for (; i + 1 < count; i += 2) {
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d v_is_zero = _mm_cmpeq_pd(vb, v_zero);
        mask_accum |= _mm_movemask_pd(v_is_zero);
        __m128d vb_safe = _mm_or_pd(vb, _mm_and_pd(v_is_zero, v_one)); // replace 0 with 1
        __m128d vr = _mm_div_pd(va, vb_safe);
        vr = _mm_or_pd(_mm_andnot_pd(v_is_zero, vr), _mm_and_pd(v_is_zero, v_nan)); // NaN where zero
        _mm_storeu_pd(&result[i], vr);
    }
    int scalar_has_zero = 0;
    for (; i < count; ++i) {
        int is_zero = (b[i] == 0.0);
        scalar_has_zero |= is_zero;
        result[i] = is_zero ? calc_nan() : (a[i] / b[i]);
    }
    if (state) {
        state->flags |= ((mask_accum | scalar_has_zero) != 0) << 1;
    }
#else
    div_scalar(state, a, b, result, count);
#endif
}

// AVX2 division: 4 doubles per cycle, same zero-lane trick as SSE2.
// tail: 2-element SSE2 spillover, then single scalar for the last odd element.
void div_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
#ifdef COMPILER_X86
    uint32_t i = 0;
    __m256d v_zero = _mm256_setzero_pd();
    __m256d v_nan = _mm256_set1_pd(calc_nan());
    __m256d v_one = _mm256_set1_pd(1.0);
    int mask_accum = 0;
    for (; i + 3 < count; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d v_is_zero = _mm256_cmp_pd(vb, v_zero, _CMP_EQ_OQ);
        mask_accum |= _mm256_movemask_pd(v_is_zero);
        __m256d vb_safe = _mm256_or_pd(vb, _mm256_and_pd(v_is_zero, v_one));
        __m256d vr = _mm256_div_pd(va, vb_safe);
        vr = _mm256_or_pd(_mm256_andnot_pd(v_is_zero, vr), _mm256_and_pd(v_is_zero, v_nan));
        _mm256_storeu_pd(&result[i], vr);
    }
    // 2-element SSE2 tail: don't waste the vector unit going scalar here
    if (i + 1 < count) {
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d v_is_zero_128 = _mm_cmpeq_pd(vb, _mm_setzero_pd());
        mask_accum |= _mm_movemask_pd(v_is_zero_128);
        __m128d vb_safe_128 = _mm_or_pd(vb, _mm_and_pd(v_is_zero_128, _mm_set1_pd(1.0)));
        __m128d vr = _mm_div_pd(va, vb_safe_128);
        vr = _mm_or_pd(_mm_andnot_pd(v_is_zero_128, vr), _mm_and_pd(v_is_zero_128, _mm_set1_pd(calc_nan())));
        _mm_storeu_pd(&result[i], vr);
        i += 2;
    }
    int scalar_has_zero = 0;
    for (; i < count; ++i) {
        int is_zero = (b[i] == 0.0);
        scalar_has_zero |= is_zero;
        result[i] = is_zero ? calc_nan() : (a[i] / b[i]);
    }
    if (state) {
        state->flags |= ((mask_accum | scalar_has_zero) != 0) << 1;
    }
#else
    div_scalar(state, a, b, result, count);
#endif
}

// NEON division: ARM AArch64 only. vbslq_f64 does the lane-select without separate and/or.
void div_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
#if defined(COMPILER_ARM) && (defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON))
    uint32_t i = 0;
    float64x2_t v_zero = vdupq_n_f64(0.0);
    float64x2_t v_nan = vdupq_n_f64(calc_nan());
    float64x2_t v_one = vdupq_n_f64(1.0);
    uint64x2_t v_accum = vdupq_n_u64(0);
    for (; i + 1 < count; i += 2) {
        float64x2_t va = vld1q_f64(&a[i]);
        float64x2_t vb = vld1q_f64(&b[i]);
        uint64x2_t v_is_zero = vceqq_f64(vb, v_zero);
        v_accum = vorrq_u64(v_accum, v_is_zero);
        float64x2_t vb_safe = vbslq_f64(v_is_zero, v_one, vb); // select: zero? use 1.0 else vb
        float64x2_t vr = vdivq_f64(va, vb_safe);
        vr = vbslq_f64(v_is_zero, v_nan, vr); // select: zero? return NaN else result
        vst1q_f64(&result[i], vr);
    }
    int scalar_has_zero = 0;
    for (; i < count; ++i) {
        int is_zero = (b[i] == 0.0);
        scalar_has_zero |= is_zero;
        result[i] = is_zero ? calc_nan() : (a[i] / b[i]);
    }
    if (state) {
        uint64_t accum_scalar = vgetq_lane_u64(v_accum, 0) | vgetq_lane_u64(v_accum, 1);
        state->flags |= ((accum_scalar != 0 || scalar_has_zero) != 0) << 1;
    }
#else
    div_scalar(state, a, b, result, count);
#endif
}

// NEON checked first -- ARM builds should never hit x86 paths.
void execute_division(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_neon) {
        div_neon(state, a, b, result, count);
    } else if (features->has_avx2) {
        div_avx2(state, a, b, result, count);
    } else if (features->has_sse2) {
        div_sse(state, a, b, result, count);
    } else {
        div_scalar(state, a, b, result, count);
    }
}
