#include "ComplexOps.h"

// Newton-Raphson sqrt. 4 iterations covers normal double range.
// for extreme exponents (near 1e-300 or 1e300), 4 iterations won't reach 1 ULP.
// fix: seed via bit-manipulation of the exponent field so the initial guess
// is already in the right ballpark, then iterate. much faster convergence.
static double custom_sqrt(double x) {
    if (x <= 0.0) return 0.0;
    union { double d; uint64_t i; } u;
    u.d = x;
    int exp = (int)((u.i >> 52) & 0x7FF) - 1023;
    int new_exp = (exp / 2) + 1023; // halve the biased exponent = sqrt of 2^exp
    u.i = (u.i & 0x800FFFFFFFFFFFFFULL) | ((uint64_t)new_exp << 52);
    double val = u.d;
    // 4 Heron iterations. converges quadratically from a good seed.
    val = 0.5 * (val + x / val);
    val = 0.5 * (val + x / val);
    val = 0.5 * (val + x / val);
    val = 0.5 * (val + x / val);
    return val;
}

// atan2 approximation -- no libm, no math.h.
// uses Hastings polynomial: error < 0.005 radians. good enough for polar display.
// adjusts quadrant by checking x sign after computing atan(y/x).
static double custom_atan2(double y, double x) {
    const double pi = 3.14159265358979323846;
    if (x == 0.0) {
        if (y > 0.0) return pi / 2.0;
        if (y < 0.0) return -pi / 2.0;
        return 0.0;
    }

    double z = y / x;
    double abs_z = z < 0 ? -z : z;
    double atan_val = 0.0;

    // |z| <= 1: direct polynomial. |z| > 1: use pi/2 - atan(1/z) identity.
    if (abs_z <= 1.0) {
        atan_val = z / (1.0 + 0.28 * z * z);
    } else {
        double inv_z = 1.0 / z;
        double sign = z > 0 ? 1.0 : -1.0;
        atan_val = sign * (pi / 2.0) - inv_z / (1.0 + 0.28 * inv_z * inv_z);
    }

    // quadrant correction: if x < 0, result is in Q2 or Q3
    if (x < 0.0) {
        if (y >= 0.0) return atan_val + pi;
        else return atan_val - pi;
    }
    return atan_val;
}

ComplexValue complex_add(ComplexValue a, ComplexValue b) {
    ComplexValue res;
    res.real = a.real + b.real;
    res.imag = a.imag + b.imag;
    return res;
}

ComplexValue complex_sub(ComplexValue a, ComplexValue b) {
    ComplexValue res;
    res.real = a.real - b.real;
    res.imag = a.imag - b.imag;
    return res;
}

// (a+bi)(c+di) = ac-bd + (ad+bc)i
ComplexValue complex_mul(ComplexValue a, ComplexValue b) {
    ComplexValue res;
    res.real = a.real * b.real - a.imag * b.imag;
    res.imag = a.real * b.imag + a.imag * b.real;
    return res;
}

// (a+bi)/(c+di) = (ac+bd)/(c²+d²) + (bc-ad)/(c²+d²)i
// denom==0: return NaN not (0,0). silent zero hides errors.
// NaN propagates through the expression tree so the user sees the failure.
ComplexValue complex_div(ComplexValue a, ComplexValue b) {
    ComplexValue res;
    double denom = b.real * b.real + b.imag * b.imag;
    if (denom == 0.0) {
        // quiet NaN: IEEE 754 bit pattern 0x7FF8000000000000
        union { uint64_t i; double d; } nan_val;
        nan_val.i = 0x7FF8000000000000ULL;
        res.real = nan_val.d;
        res.imag = nan_val.d;
        return res;
    }
    res.real = (a.real * b.real + a.imag * b.imag) / denom;
    res.imag = (a.imag * b.real - a.real * b.imag) / denom;
    return res;
}

// conjugate: flip imaginary sign. |a|² = a * conj(a).
ComplexValue complex_conjugate(ComplexValue a) {
    ComplexValue res;
    res.real = a.real;
    res.imag = -a.imag;
    return res;
}

// modulus = sqrt(re² + im²). uses our hand-rolled sqrt, no libm.
double complex_modulus(ComplexValue a) {
    return custom_sqrt(a.real * a.real + a.imag * a.imag);
}

// polar form: r = |z|, theta = arg(z). null-checked -- bare-metal callers
// might pass NULL for either output if they only need one component.
void complex_polar(ComplexValue a, double* r, double* theta) {
    if (r) *r = complex_modulus(a);
    if (theta) *theta = custom_atan2(a.imag, a.real);
}
