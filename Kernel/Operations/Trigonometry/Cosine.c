#include "Cosine.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

#define PI 3.14159265358979323846
#define INV_PI 0.31830988618379067154

#if defined(__GNUC__) || defined(__clang__)
#define TARGET_AVX2 __attribute__((target("avx2")))
#define TARGET_SSE2 __attribute__((target("sse2")))
#else
#define TARGET_AVX2
#define TARGET_SSE2
#endif

static inline double approx_cos_scalar(double x) {
    double k_d = (double)((int64_t)(x * INV_PI + (x >= 0.0 ? 0.5 : -0.5)));
    double xr = x - k_d * PI;
    double z2 = xr * xr;
    double val = 1.0 + z2 * (-0.5 + z2 * (0.041666666666666664 + z2 * (-0.0013888888888888889 + z2 * (0.0000248015873015873 + z2 * (-2.75573192239859e-7)))));
    int64_t k_i = (int64_t)k_d;
    if (k_i & 1) {
        val = -val;
    }
    return val;
}

void cos_scalar(CalculatorState* state, const double* a, double* result, uint32_t count) {
    (void)state;
    for (uint32_t i = 0; i < count; ++i) {
        result[i] = approx_cos_scalar(a[i]);
    }
}

TARGET_SSE2 void cos_sse(CalculatorState* state, const double* a, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;
    __m128d v_inv_pi = _mm_set1_pd(INV_PI);
    __m128d v_pi = _mm_set1_pd(PI);
    __m128d v_half = _mm_set1_pd(0.5);
    __m128d v_neg_half = _mm_set1_pd(-0.5);
    __m128d v_zero = _mm_setzero_pd();
    __m128d d1 = _mm_set1_pd(-0.5);
    __m128d d2 = _mm_set1_pd(0.041666666666666664);
    __m128d d3 = _mm_set1_pd(-0.0013888888888888889);
    __m128d d4 = _mm_set1_pd(0.0000248015873015873);
    __m128d d5 = _mm_set1_pd(-2.75573192239859e-7);
    __m128d v_one = _mm_set1_pd(1.0);

    for (; i + 1 < count; i += 2) {
        __m128d vx = _mm_loadu_pd(&a[i]);
        __m128d v_prod = _mm_mul_pd(vx, v_inv_pi);
        __m128d v_offset = _mm_blendv_pd(v_neg_half, v_half, _mm_cmpge_pd(vx, v_zero));
        __m128d v_k = _mm_round_pd(_mm_add_pd(v_prod, v_offset), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);

        __m128d vxr = _mm_sub_pd(vx, _mm_mul_pd(v_k, v_pi));
        __m128d vz2 = _mm_mul_pd(vxr, vxr);

        __m128d vy = _mm_mul_pd(vz2, d5);
        vy = _mm_mul_pd(vz2, _mm_add_pd(vy, d4));
        vy = _mm_mul_pd(vz2, _mm_add_pd(vy, d3));
        vy = _mm_mul_pd(vz2, _mm_add_pd(vy, d2));
        vy = _mm_mul_pd(vz2, _mm_add_pd(vy, d1));
        vy = _mm_add_pd(v_one, vy);

        // Sign correction:
        __m128d v_half_k = _mm_mul_pd(v_k, _mm_set1_pd(0.5));
        __m128d v_floor_half_k = _mm_round_pd(v_half_k, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
        __m128d v_odd_d = _mm_sub_pd(v_k, _mm_mul_pd(v_floor_half_k, _mm_set1_pd(2.0)));
        __m128d v_is_odd = _mm_cmpgt_pd(v_odd_d, _mm_set1_pd(0.5));
        __m128d v_sign_mask = _mm_and_pd(v_is_odd, _mm_set1_pd(-0.0));
        vy = _mm_xor_pd(vy, v_sign_mask);

        _mm_storeu_pd(&result[i], vy);
    }
    for (; i < count; ++i) {
        result[i] = approx_cos_scalar(a[i]);
    }
#else
    cos_scalar(state, a, result, count);
#endif
}

TARGET_AVX2 void cos_avx2(CalculatorState* state, const double* a, double* result, uint32_t count) {
    (void)state;
#ifdef COMPILER_X86
    uint32_t i = 0;
    __m256d v_inv_pi = _mm256_set1_pd(INV_PI);
    __m256d v_pi = _mm256_set1_pd(PI);
    __m256d v_half = _mm256_set1_pd(0.5);
    __m256d v_neg_half = _mm256_set1_pd(-0.5);
    __m256d v_zero = _mm256_setzero_pd();
    __m256d d1 = _mm256_set1_pd(-0.5);
    __m256d d2 = _mm256_set1_pd(0.041666666666666664);
    __m256d d3 = _mm256_set1_pd(-0.0013888888888888889);
    __m256d d4 = _mm256_set1_pd(0.0000248015873015873);
    __m256d d5 = _mm256_set1_pd(-2.75573192239859e-7);
    __m256d v_one = _mm256_set1_pd(1.0);

    for (; i + 3 < count; i += 4) {
        __m256d vx = _mm256_loadu_pd(&a[i]);
        __m256d v_prod = _mm256_mul_pd(vx, v_inv_pi);
        __m256d v_offset = _mm256_blendv_pd(v_neg_half, v_half, _mm256_cmp_pd(vx, v_zero, _CMP_GE_OQ));
        __m256d v_k = _mm256_round_pd(_mm256_add_pd(v_prod, v_offset), _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);

        __m256d vxr = _mm256_sub_pd(vx, _mm256_mul_pd(v_k, v_pi));
        __m256d vz2 = _mm256_mul_pd(vxr, vxr);

        __m256d vy = _mm256_mul_pd(vz2, d5);
        vy = _mm256_mul_pd(vz2, _mm256_add_pd(vy, d4));
        vy = _mm256_mul_pd(vz2, _mm256_add_pd(vy, d3));
        vy = _mm256_mul_pd(vz2, _mm256_add_pd(vy, d2));
        vy = _mm256_mul_pd(vz2, _mm256_add_pd(vy, d1));
        vy = _mm256_add_pd(v_one, vy);

        // Sign correction:
        __m256d v_half_k = _mm256_mul_pd(v_k, _mm256_set1_pd(0.5));
        __m256d v_floor_half_k = _mm256_round_pd(v_half_k, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
        __m256d v_odd_d = _mm256_sub_pd(v_k, _mm256_mul_pd(v_floor_half_k, _mm256_set1_pd(2.0)));
        __m256d v_is_odd = _mm256_cmp_pd(v_odd_d, _mm256_set1_pd(0.5), _CMP_GT_OQ);
        __m256d v_sign_mask = _mm256_and_pd(v_is_odd, _mm256_set1_pd(-0.0));
        vy = _mm256_xor_pd(vy, v_sign_mask);

        _mm256_storeu_pd(&result[i], vy);
    }
    if (i + 1 < count) {
        cos_sse(state, &a[i], &result[i], 2);
        i += 2;
    }
    for (; i < count; ++i) {
        result[i] = approx_cos_scalar(a[i]);
    }
#else
    cos_scalar(state, a, result, count);
#endif
}

void cos_neon(CalculatorState* state, const double* a, double* result, uint32_t count) {
#if defined(COMPILER_ARM) && (defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON))
    (void)state;
    uint32_t i = 0;
    for (; i + 1 < count; i += 2) {
        float64x2_t vx = vld1q_f64(&a[i]);
        float64x2_t v_prod = vmulq_n_f64(vx, INV_PI);
        float64x2_t v_k = vrndaq_f64(v_prod);
        float64x2_t vxr = vsubq_f64(vx, vmulq_n_f64(v_k, PI));
        float64x2_t vz2 = vmulq_f64(vxr, vxr);

        float64x2_t vy = vmulq_n_f64(vz2, -2.75573192239859e-7);
        vy = vmulq_f64(vz2, vaddq_n_f64(vy, 0.0000248015873015873));
        vy = vmulq_f64(vz2, vaddq_n_f64(vy, -0.0013888888888888889));
        vy = vmulq_f64(vz2, vaddq_n_f64(vy, 0.041666666666666664));
        vy = vmulq_f64(vz2, vaddq_n_f64(vy, -0.5));
        vy = vaddq_n_f64(vy, 1.0);

        int64x2_t vi_k = vcvtq_s64_f64(v_k);
        int64x2_t v_odd = vandq_s64(vi_k, vdupq_n_s64(1));
        float64x2_t v_multiplier = vbslq_f64(vreinterpretq_u64_s64(v_odd), vdupq_n_f64(-1.0), vdupq_n_f64(1.0));
        vy = vmulq_f64(vy, v_multiplier);

        vst1q_f64(&result[i], vy);
    }
    for (; i < count; ++i) {
        result[i] = approx_cos_scalar(a[i]);
    }
#else
    cos_scalar(state, a, result, count);
#endif
}

void execute_cosine(CalculatorState* state, const double* a, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_neon) {
        cos_neon(state, a, result, count);
    } else if (features->has_avx2) {
        cos_avx2(state, a, result, count);
    } else if (features->has_sse2) {
        cos_sse(state, a, result, count);
    } else {
        cos_scalar(state, a, result, count);
    }
}
