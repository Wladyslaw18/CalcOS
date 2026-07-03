/*
 * Author: W. Kowal
 * Complex trigonometric and hyperbolic operations implementation.
 */

#include "ComplexTrig.h"
#include "ComplexOps.h"

#define PI 3.14159265358979323846
#define INV_PI 0.31830988618379067154

static inline double local_sin(double x) {
    double k_d = (double)((int64_t)(x * INV_PI + (x >= 0.0 ? 0.5 : -0.5)));
    double xr = x - k_d * PI;
    double z2 = xr * xr;
    double val = xr * (1.0 + z2 * (-0.16666666666666666 + z2 * (0.008333333333333333 + z2 * (-0.0001984126984126984 + z2 * (0.00000275573192239859 + z2 * (-2.50521083854417e-8))))));
    int64_t k_i = (int64_t)k_d;
    if (k_i & 1) {
        val = -val;
    }
    return val;
}

static inline double local_cos(double x) {
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

static inline double local_exp(double x) {
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

static inline double local_sinh(double x) {
    return 0.5 * (local_exp(x) - local_exp(-x));
}

static inline double local_cosh(double x) {
    return 0.5 * (local_exp(x) + local_exp(-x));
}

ComplexValue complex_sin(ComplexValue z) {
    ComplexValue res;
    res.real = local_sin(z.real) * local_cosh(z.imag);
    res.imag = local_cos(z.real) * local_sinh(z.imag);
    return res;
}

ComplexValue complex_cos(ComplexValue z) {
    ComplexValue res;
    res.real = local_cos(z.real) * local_cosh(z.imag);
    res.imag = -local_sin(z.real) * local_sinh(z.imag);
    return res;
}

ComplexValue complex_tan(ComplexValue z) {
    return complex_div(complex_sin(z), complex_cos(z));
}

ComplexValue complex_sinh(ComplexValue z) {
    ComplexValue res;
    res.real = local_sinh(z.real) * local_cos(z.imag);
    res.imag = local_cosh(z.real) * local_sin(z.imag);
    return res;
}

ComplexValue complex_cosh(ComplexValue z) {
    ComplexValue res;
    res.real = local_cosh(z.real) * local_cos(z.imag);
    res.imag = local_sinh(z.real) * local_sin(z.imag);
    return res;
}

ComplexValue complex_tanh(ComplexValue z) {
    return complex_div(complex_sinh(z), complex_cosh(z));
}
