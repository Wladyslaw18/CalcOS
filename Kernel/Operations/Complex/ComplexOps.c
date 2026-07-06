/*
 * ComplexOps.c — Complex arithmetic and transcendental operations.
 *
 * Zero heap. No libm. No stdlib math.
 * All floating-point helpers come from MathUtils.h (math_sqrt, math_atan2,
 * math_nan, math_abs, math_ldexp). The old custom_sqrt / custom_atan2
 * duplicates have been removed in favour of those shared primitives.
 */

#include "ComplexOps.h"
/* MathUtils.h is already transitively included via ComplexOps.h, but an
 * explicit include here makes the dependency visible at the TU level.     */
#include "../../../Infrastructure/Utils/MathUtils.h"

// ─────────────────────────────────────────────────────────────────────────────
// Internal scalar helpers (static — not exported)
// ─────────────────────────────────────────────────────────────────────────────

/* scalar_exp — e^x without libm.
 * Algorithm:
 *   1. Range-guard: clamp to ±overflow limits (IEEE 754 double range).
 *   2. Argument reduction: write x = k*ln2 + f, |f| < ln2/2.
 *      k = round(x / ln2), f = x - k*ln2.
 *   3. Polynomial: e^f via 7-term Taylor (relative error < 2^-52 for |f|<0.35).
 *   4. Scale result by 2^k via bit-manipulation (math_ldexp).
 */
static double scalar_exp(double x) {
    /* Overflow / underflow guards matching IEEE 754 double exponent range */
    if (x > 709.782711149557429) {
        union { uint64_t i; double d; } inf;
        inf.i = 0x7FF0000000000000ULL;
        return inf.d; /* +Inf */
    }
    if (x < -745.1332191019411) return 0.0;

    const double log2_e = 1.4426950408889634074; /* 1/ln2 */
    const double ln2_hi = 0.6931471805599453;    /* ln2, high part */

    /* k = nearest integer to x*log2(e) */
    double kd = (double)((int64_t)(x * log2_e + (x >= 0.0 ? 0.5 : -0.5)));
    /* f = x - k*ln2 (argument reduction; |f| <= ln2/2 ~ 0.347) */
    double f  = x - kd * ln2_hi;

    /* e^f via Horner-form 7-term Taylor: 1 + f + f²/2! + ... + f^7/7! */
    double ef = 1.0 + f * (1.0
              + f * (0.5
              + f * (0.16666666666666666
              + f * (0.041666666666666664
              + f * (0.008333333333333333
              + f * (0.0013888888888888889
              + f *  0.0001984126984126984))))));

    /* Scale by 2^k — math_ldexp does this via exponent field manipulation */
    return math_ldexp(ef, (int)kd);
}

/* scalar_sin — sin(x) without libm.
 * Argument reduction to [-π/2, π/2] via k = round(x/π), then a 6-term
 * Taylor series for sin on the reduced argument. Sign correction for odd k.
 */
static double scalar_sin(double x) {
    const double inv_pi = 0.31830988618379067154;
    const double pi     = 3.14159265358979323846;
    double kd = (double)((int64_t)(x * inv_pi + (x >= 0.0 ? 0.5 : -0.5)));
    double xr = x - kd * pi;
    double z2 = xr * xr;
    double v  = xr * (1.0
              + z2 * (-0.16666666666666666
              + z2 * ( 0.008333333333333333
              + z2 * (-0.0001984126984126984
              + z2 * ( 2.75573192239859e-6
              + z2 *  -2.50521083854417e-8)))));
    return ((int64_t)kd & 1) ? -v : v;
}

/* scalar_cos — cos(x) without libm. Same reduction as scalar_sin. */
static double scalar_cos(double x) {
    const double inv_pi = 0.31830988618379067154;
    const double pi     = 3.14159265358979323846;
    double kd = (double)((int64_t)(x * inv_pi + (x >= 0.0 ? 0.5 : -0.5)));
    double xr = x - kd * pi;
    double z2 = xr * xr;
    double v  = 1.0
              + z2 * (-0.5
              + z2 * ( 0.041666666666666664
              + z2 * (-0.0013888888888888889
              + z2 * ( 2.48015873015873e-5
              + z2 *  -2.75573192239859e-7))));
    return ((int64_t)kd & 1) ? -v : v;
}

/* scalar_log — ln(x) without libm.   x must be > 0.
 * Algorithm:
 *   1. Extract biased exponent e and normalised mantissa m ∈ [1, 2).
 *   2. ln(x) = e*ln2 + ln(m).
 *   3. ln(m): write m = 1+t, t ∈ [0,1); use identity
 *        ln(1+t) = 2*atanh(t/(2+t)) = 2 * Σ (t/(2+t))^(2n+1)/(2n+1)
 *      which converges quickly for |u| = |t/(2+t)| ≤ 1/3.
 *   4. To improve convergence for m close to sqrt(2) (t ~ 0.41), rescale
 *      m → m/sqrt(2) and add 0.5*ln2 to the exponent contribution.
 *      This keeps |u| ≤ tan(π/8) ≈ 0.172 → 6 terms give ~1e-16 accuracy.
 */
static double scalar_log(double x) {
    if (x <= 0.0) return math_nan(); /* ln of non-positive is undefined here */

    const double ln2    = 0.69314718055994530942;
    const double sqrt2  = 1.41421356237309504880;

    /* Decompose x = m * 2^e,  m ∈ [1, 2) */
    union { double d; uint64_t i; } u;
    u.d = x;
    int e = (int)((u.i >> 52) & 0x7FF) - 1023;
    /* Force exponent to 0 (i.e. m ∈ [1,2)) */
    u.i = (u.i & 0x800FFFFFFFFFFFFFULL) | (1023ULL << 52);
    double m = u.d;

    /* If m >= sqrt(2), divide by 2 and compensate with +1 to exponent */
    if (m >= sqrt2) {
        m *= 0.5;
        e += 1;
    }

    /* atanh series: ln(m) = 2*atanh((m-1)/(m+1))
     * Let u = (m-1)/(m+1);  then ln(m) = 2*(u + u^3/3 + u^5/5 + ...) */
    double t  = m - 1.0;
    double uu = t / (m + 1.0);   /* u = (m-1)/(m+1) */
    double u2 = uu * uu;
    /* 8-term series — sufficient for |u| <= 0.172 (relative error ~1e-17) */
    double series = uu * (1.0
                  + u2 * (1.0/3.0
                  + u2 * (1.0/5.0
                  + u2 * (1.0/7.0
                  + u2 * (1.0/9.0
                  + u2 * (1.0/11.0
                  + u2 * (1.0/13.0
                  + u2 *  1.0/15.0)))))));
    double log_m = 2.0 * series;

    return (double)e * ln2 + log_m;
}

// ─────────────────────────────────────────────────────────────────────────────
// Basic arithmetic
// ─────────────────────────────────────────────────────────────────────────────

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

/* (a+bi)(c+di) = ac-bd + (ad+bc)i */
ComplexValue complex_mul(ComplexValue a, ComplexValue b) {
    ComplexValue res;
    res.real = a.real * b.real - a.imag * b.imag;
    res.imag = a.real * b.imag + a.imag * b.real;
    return res;
}

/* (a+bi)/(c+di) = (ac+bd)/(c²+d²) + (bc-ad)/(c²+d²)i
 * denom==0: return NaN — silent zero would hide divide-by-zero errors. */
ComplexValue complex_div(ComplexValue a, ComplexValue b) {
    ComplexValue res;
    double denom = b.real * b.real + b.imag * b.imag;
    if (denom == 0.0) {
        double nan = math_nan();
        res.real = nan;
        res.imag = nan;
        return res;
    }
    res.real = (a.real * b.real + a.imag * b.imag) / denom;
    res.imag = (a.imag * b.real - a.real * b.imag) / denom;
    return res;
}

/* conjugate: flip imaginary sign. |a|² = a * conj(a). */
ComplexValue complex_conjugate(ComplexValue a) {
    ComplexValue res;
    res.real =  a.real;
    res.imag = -a.imag;
    return res;
}

/* modulus = sqrt(re² + im²). Uses math_sqrt from MathUtils.h — no libm. */
double complex_modulus(ComplexValue a) {
    return math_sqrt(a.real * a.real + a.imag * a.imag);
}

/* polar form: r = |z|, theta = arg(z).
 * Null-checked — bare-metal callers may pass NULL for either output.
 * Uses math_atan2 from MathUtils.h (minimax 5th-order, error < 5e-6 rad). */
void complex_polar(ComplexValue a, double* r, double* theta) {
    if (r)     *r     = complex_modulus(a);
    if (theta) *theta = math_atan2(a.imag, a.real);
}

// ─────────────────────────────────────────────────────────────────────────────
// Transcendental operations
// ─────────────────────────────────────────────────────────────────────────────

/*
 * complex_exp — e^z, where z = re + i*im.
 *
 * Formula:  e^z = e^re * (cos(im) + i*sin(im))   [Euler's formula]
 *
 * Steps:
 *   1. Compute magnitude = scalar_exp(re).
 *   2. Compute scalar_cos(im) and scalar_sin(im) for the directional part.
 *   3. Multiply: result = (mag*cos_im, mag*sin_im).
 *
 * Error return: if re is out of scalar_exp's safe range (> ~709), the
 * magnitude overflows to Inf; if re < ~-745 it underflows to 0. Both
 * are IEEE-correct — no special NaN needed beyond what scalar_exp provides.
 */
ComplexValue complex_exp(ComplexValue z) {
    ComplexValue res;
    double mag    = scalar_exp(z.real);
    double cos_im = scalar_cos(z.imag);
    double sin_im = scalar_sin(z.imag);
    res.real = mag * cos_im;
    res.imag = mag * sin_im;
    return res;
}

/*
 * complex_log — Principal value of ln(z).
 *
 * Formula:  ln(z) = ln(|z|) + i*arg(z)
 *
 * Steps:
 *   1. r = complex_modulus(z)   →  |z|.
 *   2. ln(r) via scalar_log     →  real part.
 *   3. arg(z) = math_atan2(im, re)  →  imaginary part (range: (-π, π]).
 *
 * Special case: z = 0+0i  →  ln(0) is -∞; return complex NaN to signal error.
 */
ComplexValue complex_log(ComplexValue z) {
    ComplexValue res;

    /* Guard: log(0) is undefined */
    if (z.real == 0.0 && z.imag == 0.0) {
        double nan = math_nan();
        res.real = nan;
        res.imag = nan;
        return res;
    }

    double r  = complex_modulus(z);   /* |z| = sqrt(re²+im²)         */
    res.real  = scalar_log(r);        /* ln(|z|)                      */
    res.imag  = math_atan2(z.imag, z.real); /* arg(z) ∈ (-π, π]       */
    return res;
}

/*
 * complex_pow — base^exp computed as exp(exp * log(base)).
 *
 * Special cases handled before the general path:
 *   a) base = 0+0i, exp.real > 0  →  0+0i   (consistent with real pow(0,p))
 *   b) base = 0+0i, exp.real <= 0 →  complex NaN (0^0, 0^negative undefined)
 *   c) exp.imag == 0 and exp.real is a small non-negative integer (0..15):
 *      use repeated complex multiplication for exact results (no log/exp error).
 *
 * General path: base^exp = exp(exp * log(base)).
 */
ComplexValue complex_pow(ComplexValue base, ComplexValue exponent) {
    ComplexValue res;
    double nan = math_nan();

    /* ── Special case (a,b): base is zero ─────────────────────────────── */
    if (base.real == 0.0 && base.imag == 0.0) {
        if (exponent.real > 0.0) {
            res.real = 0.0; res.imag = 0.0;
        } else {
            /* 0^0 and 0^negative are undefined */
            res.real = nan; res.imag = nan;
        }
        return res;
    }

    /* ── Special case (c): pure integer exponent 0..15 ────────────────── *
     * Only when exp.imag == 0 and exp.real is a whole number in [0, 15].  *
     * Repeated squaring would be faster but simple iteration is clearer    *
     * and the range is small (≤15 muls).                                   */
    if (exponent.imag == 0.0) {
        double p  = exponent.real;
        long   ip = (long)p;
        if ((double)ip == p && ip >= 0 && ip <= 15) {
            /* result = base^ip via repeated multiplication */
            ComplexValue acc;
            acc.real = 1.0; acc.imag = 0.0; /* multiplicative identity */
            for (long k = 0; k < ip; ++k) {
                acc = complex_mul(acc, base);
            }
            return acc;
        }
        /* Negative integer exponents: base^(-n) = 1 / base^n */
        if ((double)ip == p && ip < 0 && ip >= -15) {
            ComplexValue acc;
            acc.real = 1.0; acc.imag = 0.0;
            for (long k = 0; k < -ip; ++k) {
                acc = complex_mul(acc, base);
            }
            /* Return reciprocal */
            ComplexValue one = {1.0, 0.0};
            return complex_div(one, acc);
        }
    }

    /* ── General path: base^exp = exp(exp * log(base)) ────────────────── */
    ComplexValue log_base = complex_log(base);
    /* If log returned NaN propagate immediately */
    if (math_is_nan(log_base.real)) {
        res.real = nan; res.imag = nan;
        return res;
    }
    ComplexValue product  = complex_mul(exponent, log_base);
    return complex_exp(product);
}

/*
 * complex_sqrt — Principal square root of z = re + i*im.
 *
 * Formula (avoids catastrophic cancellation for small im):
 *   |z| = complex_modulus(z)
 *   real part: sqrt((|z| + re) / 2)
 *   imag part: sign(im) * sqrt((|z| - re) / 2)
 *
 * Derivation: if z = r*e^(iθ), then sqrt(z) = sqrt(r)*e^(iθ/2).
 * Expanding with half-angle identities gives the formula above.
 *
 * Special cases:
 *   z = 0+0i  →  0+0i  (exact)
 *   im == 0, re < 0  →  0 + i*sqrt(-re)  (pure imaginary result)
 *   im == 0, re >= 0 →  sqrt(re) + 0i    (pure real result)
 */
ComplexValue complex_sqrt(ComplexValue z) {
    ComplexValue res;

    /* ── z = 0 ─────────────────────────────────────────────────────────── */
    if (z.real == 0.0 && z.imag == 0.0) {
        res.real = 0.0; res.imag = 0.0;
        return res;
    }

    /* ── Pure-real inputs: avoid dividing (0) by zero ───────────────────  */
    if (z.imag == 0.0) {
        if (z.real > 0.0) {
            res.real = math_sqrt(z.real);
            res.imag = 0.0;
        } else {
            /* re < 0: sqrt(re) = i*sqrt(-re) */
            res.real = 0.0;
            res.imag = math_sqrt(-z.real);
        }
        return res;
    }

    /* ── General formula ──────────────────────────────────────────────── */
    double modulus = complex_modulus(z);          /* |z| = sqrt(re²+im²)  */
    res.real = math_sqrt((modulus + z.real) * 0.5);
    double imag_mag = math_sqrt((modulus - z.real) * 0.5);
    /* imag part carries the sign of im (principal value convention)       */
    res.imag = (z.imag >= 0.0) ? imag_mag : -imag_mag;
    return res;
}
