/*
 * File: RNG.c
 * Author: W. Kowal
 * Description: High-quality seeded random number generation using xoshiro256++.
 *
 * WHY xoshiro256++
 * - Passes TestU01 BigCrush (xorshift128 fails)
 * - 256-bit state = 2^256 period
 * - No multiplication in state update (faster on bare metal)
 * - ~10 lines of C = minimal code bloat
 * - Same author as xorshift (Blackman & Vigna) just better
 *
 * WHY NOT the old xorshift128
 * - Fails linear complexity tests
 * - Only 2^128 period (fine but weaker for Monte Carlo)
 * - Same code size, worse quality no reason to keep it
 *
 * calc_random_normal — polar Box-Muller with second-sample cache
 * - Classic Box-Muller produces two independent N(0,1) values per call.
 *   The old version threw the second one away (wasted a log + 2 trig ops).
 *   We cache the second value in a static variable and return it on the
 *   next call, halving the average per-call cost.
 * - Uses the POLAR form (Marsaglia) so cos/sin can be replaced by a
 *   simple (x,y) circle rejection — but here we keep approx_cos because
 *   the polar rejection loop introduces unpredictable branches.  The
 *   improvement from caching already eliminates the 50 % waste.
 *
 * calc_random_poisson — Knuth / normal approximation
 * - Knuth's algorithm (lambda < 30): multiply uniforms until their
 *   product falls below e^(-lambda).  Expected iterations = lambda + 1.
 * - Normal approximation (lambda >= 30): CLT guarantees Poisson(lambda)
 *   ≈ Normal(lambda, sqrt(lambda)) when lambda is large.  Rounded and
 *   clamped to >= 0.
 *
 * calc_random_gamma — Marsaglia-Tsang (2000)
 * - Shape alpha >= 1: accept/reject with a squeeze for speed.
 * - Shape alpha < 1:  use Gamma(alpha) = Gamma(alpha+1) * U^(1/alpha).
 */

#include "RNG.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#define COMPILER_ARM
#endif

#define PI 3.14159265358979323846

/* ============================================================
 * xoshiro256++ STATE  4 × uint64_t = 256 bits
 * ============================================================ */

static uint64_t rng_state[4];

/* ============================================================
 * ROTATE LEFT (helper)
 * ============================================================ */

static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

/* ============================================================
 * xoshiro256++ CORE  The main generator
 * ============================================================ */

static inline uint64_t xoshiro256pp(void) {
    const uint64_t result = rotl(rng_state[1] * 5, 7) * 9;
    const uint64_t t = rng_state[1] << 17;

    rng_state[2] ^= rng_state[0];
    rng_state[3] ^= rng_state[1];
    rng_state[1] ^= rng_state[2];
    rng_state[0] ^= rng_state[3];

    rng_state[2] ^= t;
    rng_state[3] = rotl(rng_state[3], 45);

    return result;
}

/* ============================================================
 * SEEDING (SplitMix64 mixer)
 * ============================================================ */

void calc_random_seed(uint64_t seed) {
    if (seed == 0) {
        seed = 0xDEADC0DE9E651635ULL;
    }

    /* SplitMix64 to spread a single 64-bit seed into 4 64-bit state */
    uint64_t z = seed + 0x9e3779b97f4a7c15ULL;
    for (int i = 0; i < 4; i++) {
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        z = z ^ (z >> 31);
        rng_state[i] = z;
    }

    /* Guarantee at least one non-zero state (xoshiro can't have all-zero state) */
    if (rng_state[0] == 0 && rng_state[1] == 0 && rng_state[2] == 0 && rng_state[3] == 0) {
        rng_state[0] = 123456789ULL;
        rng_state[1] = 362436069ULL;
        rng_state[2] = 521288629ULL;
        rng_state[3] = 88675123ULL;
    }
}

/* ============================================================
 * STATE SAVE / RESTORE
 * ============================================================ */

/*
 * calc_random_get_state
 * ---------------------
 * Copies the 256-bit xoshiro256++ state into the caller-provided array.
 * out_state must point to at least 4 uint64_t values.
 * No-op if out_state is NULL.
 */
void calc_random_get_state(uint64_t *out_state) {
    if (out_state == (void *)0) return;
    out_state[0] = rng_state[0];
    out_state[1] = rng_state[1];
    out_state[2] = rng_state[2];
    out_state[3] = rng_state[3];
}

/*
 * calc_random_set_state
 * ---------------------
 * Restores the generator from a previously saved state array.
 * state must point to 4 uint64_t values.
 * Rejects an all-zero state (invalid for xoshiro256++) and NULL pointer.
 */
void calc_random_set_state(const uint64_t *state) {
    if (state == (void *)0) return;

    /* All-zero state is illegal for xoshiro256++ — reject silently */
    if (state[0] == 0 && state[1] == 0 && state[2] == 0 && state[3] == 0) return;

    rng_state[0] = state[0];
    rng_state[1] = state[1];
    rng_state[2] = state[2];
    rng_state[3] = state[3];
}

/* ============================================================
 * DOUBLE GENERATION
 * ============================================================ */

/* Generate double in [0, 1) with 53-bit precision
 * Uses only 53 bits from the 64-bit output (mask + convert) */
static inline double next_double(void) {
    return (xoshiro256pp() >> 11) * (1.0 / 9007199254740992.0); /* 2^53 */
}

/* Generate double in (0, 1) reject zero */
static inline double next_double_open(void) {
    double u;
    do {
        u = next_double();
    } while (u == 0.0);
    return u;
}

/* ============================================================
 * MATH HELPERS (no libm dependency)
 * ============================================================ */

static inline double custom_log(double x) {
    if (x <= 0.0) {
        union { uint64_t i; double d; } u;
        if (x == 0.0) {
            u.i = 0xFFF0000000000000ULL; /* -Infinity */
            return u.d;
        }
        u.i = 0x7FF8000000000000ULL; /* NaN */
        return u.d;
    }
    union { double d; uint64_t i; } u;
    u.d = x;
    int64_t k = ((u.i >> 52) & 0x7FF) - 1023;
    u.i = (u.i & 0x000FFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
    double m = u.d;
    double num = m - 1.0;
    double den = m + 1.0;
    double z = num / den;
    double z2 = z * z;
    double poly = z * (2.0 + z2 * (0.6666666666666666 + z2 * (0.4 + z2 * (0.2857142857142857 + z2 * (0.2222222222222222 + z2 * 0.1818181818181818)))));
    return poly + (double)k * 0.69314718055994530942;
}

static inline double custom_sqrt(double x) {
    if (x <= 0.0) return 0.0;
#if defined(COMPILER_X86)
    return _mm_cvtsd_f64(_mm_sqrt_sd(_mm_setzero_pd(), _mm_set_sd(x)));
#elif defined(COMPILER_ARM) && (defined(__aarch64__) || defined(_M_ARM64))
    // Use AArch64 hardware vector/scalar square root
    float64x2_t v_x = vdupq_n_f64(x);
    return vgetq_lane_f64(vsqrtq_f64(v_x), 0);
#else
    // Fallback: fast inverse square root with magic constant 0x5fe6eb50c7b537a9
    double xhalf = 0.5 * x;
    union {
        double d;
        uint64_t i;
    } u;
    u.d = x;
    u.i = 0x5fe6eb50c7b537a9ULL - (u.i >> 1);
    double y = u.d;
    y = y * (1.5 - xhalf * y * y);
    y = y * (1.5 - xhalf * y * y);
    y = y * (1.5 - xhalf * y * y);
    y = y * (1.5 - xhalf * y * y); // 4 iterations of Newton-Raphson for double-precision accuracy
    return y * x;
#endif
}

static inline double approx_sin(double x) {
    double k_d = (double)((int64_t)(x * 0.31830988618379067154 + (x >= 0.0 ? 0.5 : -0.5)));
    double xr = x - k_d * PI;
    double z2 = xr * xr;
    double val = xr * (1.0 + z2 * (-0.16666666666666666 + z2 * (0.008333333333333333 + z2 * (-0.0001984126984126984 + z2 * (0.00000275573192239859 + z2 * (-2.50521083854417e-8))))));
    if ((int64_t)k_d & 1) val = -val;
    return val;
}

static inline double approx_cos(double x) {
    double k_d = (double)((int64_t)(x * 0.31830988618379067154 + (x >= 0.0 ? 0.5 : -0.5)));
    double xr = x - k_d * PI;
    double z2 = xr * xr;
    double val = 1.0 + z2 * (-0.5 + z2 * (0.041666666666666664 + z2 * (-0.0013888888888888889 + z2 * (0.0000248015873015873 + z2 * (-2.75573192239859e-7)))));
    if ((int64_t)k_d & 1) val = -val;
    return val;
}

/* floor substitute — returns the greatest integer <= x */
static inline double custom_floor(double x) {
    int64_t xi = (int64_t)x;
    return (double)(xi - (x < (double)xi ? 1 : 0));
}

/* exp substitute via identity exp(x) = 2^(x/ln2)
 * Range: reliable for |x| < ~700 (adequate for Poisson lambda < 30). */
static inline double custom_exp(double x) {
    /* exp(x) = e^x.  Decompose: x = n*ln2 + r, |r| <= ln2/2.
     * e^x = 2^n * e^r.  Use a Horner polynomial for e^r (|r|<=0.347). */
    const double LN2     = 0.69314718055994530942;
    const double INV_LN2 = 1.44269504088896340736;

    double nd = x * INV_LN2;
    int64_t n = (int64_t)(nd + (nd >= 0.0 ? 0.5 : -0.5)); /* round */
    double r = x - (double)n * LN2;

    /* Degree-6 Horner for e^r */
    double er = 1.0 + r * (1.0 + r * (0.5 + r * (0.16666666666666666 +
                r * (0.041666666666666664 + r * (0.008333333333333333 +
                r * 0.001388888888888889)))));

    /* Scale by 2^n using IEEE 754 bit manipulation */
    union { double d; uint64_t i; } u;
    /* Clamp n to avoid undefined behaviour on extreme inputs */
    if (n >  1023) { u.i = 0x7FF0000000000000ULL; return u.d; } /* +Inf */
    if (n < -1022) return 0.0;
    u.i = (uint64_t)(n + 1023) << 52;
    return er * u.d;
}

/* ============================================================
 * PUBLIC API
 * ============================================================ */

/* ----------------------------------------
 * Uniform in [min, max)
 * ---------------------------------------- */
double calc_random_uniform(double min, double max) {
    return min + (max - min) * next_double();
}

/* ----------------------------------------
 * Normal (Gaussian) — polar Box-Muller
 *   with second-sample caching
 *
 * Box-Muller generates TWO independent
 * N(0,1) values from one pair of uniforms.
 * We cache the second value in a static
 * variable and return it on the next call,
 * halving the average log+trig cost.
 * ---------------------------------------- */
double calc_random_normal(double mu, double sigma) {
    static double cached      = 0.0;
    static int    cache_valid = 0;

    double z;

    if (cache_valid) {
        /* Return the spare sample we computed last time */
        cache_valid = 0;
        z = cached;
    } else {
        /* Compute a fresh pair of N(0,1) variates */
        double u1 = next_double_open();
        double u2 = next_double();

        double ln_u1 = custom_log(u1);
        double r     = custom_sqrt(-2.0 * ln_u1);
        double theta = 2.0 * PI * u2;

        /* Store the second variate for the next call */
        cached      = r * approx_sin(theta);
        cache_valid = 1;

        z = r * approx_cos(theta);
    }

    return mu + sigma * z;
}

/* ----------------------------------------
 * Exponential with rate lambda
 * ---------------------------------------- */
double calc_random_exponential(double lambda) {
    if (lambda <= 0.0) return 0.0;
    double u = next_double_open();
    return -custom_log(u) / lambda;
}

/* ----------------------------------------
 * Bernoulli trial
 *   Returns 1.0 with probability p, else 0.0.
 *   p clamped to [0, 1].
 * ---------------------------------------- */
double calc_random_bernoulli(double p) {
    if (p <= 0.0) return 0.0;
    if (p >= 1.0) return 1.0;
    return (next_double() < p) ? 1.0 : 0.0;
}

/* ----------------------------------------
 * Poisson variate with mean lambda
 *
 * Small lambda (< 30):
 *   Knuth's algorithm.  Draw U ~ Uniform(0,1)
 *   repeatedly; multiply into a running product p.
 *   Stop when p < e^(-lambda); the count k-1 is
 *   Poisson(lambda).  Expected loop count = lambda+1.
 *
 * Large lambda (>= 30):
 *   CLT: Poisson(lambda) ≈ Normal(lambda, sqrt(lambda)).
 *   Round to nearest integer and clamp to 0.
 * ---------------------------------------- */
double calc_random_poisson(double lambda) {
    if (lambda <= 0.0) return 0.0;

    if (lambda < 30.0) {
        /* Knuth's algorithm */
        double L = custom_exp(-lambda); /* threshold e^(-lambda) */
        double p = 1.0;
        double k = 0.0;

        do {
            k += 1.0;
            p *= next_double_open(); /* p = product of U_i in (0,1) */
        } while (p > L);

        return k - 1.0;
    } else {
        /* Normal approximation: Poisson(lambda) ≈ N(lambda, sqrt(lambda)) */
        double sample = calc_random_normal(lambda, custom_sqrt(lambda));
        if (sample < 0.0) sample = 0.0;
        /* Round to nearest integer (no libm round needed) */
        double floored = custom_floor(sample);
        return (sample - floored >= 0.5) ? floored + 1.0 : floored;
    }
}

/* ----------------------------------------
 * Gamma variate: Gamma(shape, scale)
 *
 * Marsaglia-Tsang (2000) for shape >= 1:
 *   Let d = shape - 1/3, c = 1/sqrt(9d).
 *   Loop:
 *     x ~ N(0,1), v = (1 + c*x)^3
 *     reject if v <= 0
 *     u ~ Uniform(0,1)
 *     accept if u < 1 - 0.0331*(x^2)^2  (squeeze)
 *     or if log(u) < 0.5*x^2 + d*(1 - v + log(v))
 *   Return d * v * scale.
 *
 * For shape < 1:
 *   Gamma(alpha) = Gamma(alpha+1) * U^(1/alpha)
 *   where U ~ Uniform(0,1).  Then recurse with
 *   shape+1 and apply the correction factor.
 * ---------------------------------------- */
double calc_random_gamma(double shape, double scale) {
    if (shape <= 0.0 || scale <= 0.0) return 0.0;

    /* Handle shape < 1 via the boost identity */
    if (shape < 1.0) {
        double u = next_double_open();
        /* U^(1/shape) correction factor */
        double boosted = calc_random_gamma(shape + 1.0, scale);
        /* custom_exp((1/shape)*custom_log(u)) = u^(1/shape) */
        double correction = custom_exp(custom_log(u) / shape);
        return boosted * correction;
    }

    /* Marsaglia-Tsang for shape >= 1 */
    double d = shape - (1.0 / 3.0);
    double c = 1.0 / custom_sqrt(9.0 * d);

    for (;;) {
        double x, v;

        /* Draw x ~ N(0,1) and compute v = (1 + c*x)^3 */
        do {
            x = calc_random_normal(0.0, 1.0);
            v = 1.0 + c * x;
        } while (v <= 0.0);

        v = v * v * v; /* cube */

        double u = next_double_open();
        double x2 = x * x;

        /* Quick squeeze: accept without log if this holds */
        if (u < 1.0 - 0.0331 * (x2 * x2)) {
            return d * v * scale;
        }

        /* Full log-space accept/reject */
        if (custom_log(u) < 0.5 * x2 + d * (1.0 - v + custom_log(v))) {
            return d * v * scale;
        }
        /* Otherwise reject and loop */
    }
}
