#include "Exponentiation.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

#define LN2 0.69314718055994530942

static inline double local_log_scalar(CalculatorState* state, double x) {
    if (x <= 0.0) {
        if (state) state->flags |= 2;
        union { uint64_t i; double d; } u;
        if (x == 0.0) {
            u.i = 0xFFF0000000000000ULL; // -Infinity
            return u.d;
        }
        u.i = 0x7FF8000000000000ULL; // NaN
        return u.d;
    }
    union {
        double d;
        uint64_t i;
    } u;
    u.d = x;
    int64_t k = ((u.i >> 52) & 0x7FF) - 1023;
    u.i = (u.i & 0x000FFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
    double m = u.d;
    double num = m - 1.0;
    double den = m + 1.0;
    double z = num / den;
    double z2 = z * z;
    double poly = z * (2.0 + z2 * (0.6666666666666666 + z2 * (0.4 + z2 * (0.2857142857142857 + z2 * (0.2222222222222222 + z2 * 0.1818181818181818)))));
    return poly + (double)k * LN2;
}

static inline double approx_exp(double x) {
    if (x > 709.78) {
        union { uint64_t i; double d; } u;
        u.i = 0x7FF0000000000000ULL; // Infinity
        return u.d;
    }
    if (x < -745.13) return 0.0;
    
    double log2_e = 1.4426950408889634074;
    double ln2 = 0.69314718055994530942;
    double k_d = (double)((int64_t)(x * log2_e + (x >= 0.0 ? 0.5 : -0.5)));
    double f = x * log2_e - k_d;
    double z = f * ln2;
    double ez = 1.0 + z * (1.0 + z * (0.5 + z * (0.16666666666666666 + z * (0.041666666666666664 + z * (0.008333333333333333 + z * (0.0013888888888888889 + z * 0.0001984126984126984))))));
    
    int64_t k = (int64_t)k_d;
    double val = ez;
    if (k < -1022) {
        union { double d; uint64_t i; } scale;
        scale.i = (uint64_t)(1) << 52; // 2^-1022
        val *= scale.d;
        k += 1022;
    } else if (k > 1023) {
        union { double d; uint64_t i; } scale;
        scale.i = (uint64_t)(2046) << 52; // 2^1023
        val *= scale.d;
        k -= 1023;
    }
    union { double d; uint64_t i; } scale;
    scale.i = (uint64_t)(k + 1023) << 52;
    return val * scale.d;
}

static inline double approx_pow(CalculatorState* state, double a, double b) {
    if (a == 0.0) {
        if (b == 0.0) return 1.0;
        return 0.0;
    }
    if (a < 0.0) {
        if ((double)(int64_t)b == b) {
            double abs_pow = approx_exp(b * local_log_scalar(state, -a));
            if ((int64_t)b & 1) {
                return -abs_pow;
            }
            return abs_pow;
        }
        if (state) state->flags |= 1;
        union { uint64_t i; double d; } u;
        u.i = 0x7FF8000000000000ULL; // NaN
        return u.d;
    }
    return approx_exp(b * local_log_scalar(state, a));
}

void exp_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        result[i] = approx_pow(state, a[i], b[i]);
    }
}

void exp_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    exp_scalar(state, a, b, result, count);
}

void exp_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    exp_scalar(state, a, b, result, count);
}

void exp_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count) {
    exp_scalar(state, a, b, result, count);
}

void execute_exponentiation(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_neon) {
        exp_neon(state, a, b, result, count);
    } else if (features->has_avx2) {
        exp_avx2(state, a, b, result, count);
    } else if (features->has_sse2) {
        exp_sse(state, a, b, result, count);
    } else {
        exp_scalar(state, a, b, result, count);
    }
}
