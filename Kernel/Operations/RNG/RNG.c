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
 */

#include "RNG.h"

#define PI 3.14159265358979323846

// xoshiro256++ state: 4 uint64_t = 256 bits
static uint64_t rng_state[4];

// ============================================================
// ROTATE LEFT (helper)
// ============================================================

static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

// ============================================================
// xoshiro256++ CORE The main generator
// ============================================================

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

// ============================================================
// SEEDING (SplitMix64 mixer)
// ============================================================

void calc_random_seed(uint64_t seed) {
    if (seed == 0) {
        seed = 0xDEADC0DE9E651635ULL;
    }

    // SplitMix64 to spread a single 64-bit seed into 4 64-bit state
    uint64_t z = seed + 0x9e3779b97f4a7c15ULL;
    for (int i = 0; i < 4; i++) {
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        z = z ^ (z >> 31);
        rng_state[i] = z;
    }

    // Guarantee at least one non-zero state (xoshiro can't have all-zero state)
    if (rng_state[0] == 0 && rng_state[1] == 0 && rng_state[2] == 0 && rng_state[3] == 0) {
        rng_state[0] = 123456789ULL;
        rng_state[1] = 362436069ULL;
        rng_state[2] = 521288629ULL;
        rng_state[3] = 88675123ULL;
    }
}

// ============================================================
// DOUBLE GENERATION
// ============================================================

// Generate double in [0, 1) with 53-bit precision
// Uses only 53 bits from the 64-bit output (mask + convert)
static inline double next_double(void) {
    return (xoshiro256pp() >> 11) * (1.0 / 9007199254740992.0); // 2^53
}

// Generate double in (0, 1) reject zero
static inline double next_double_open(void) {
    double u;
    do {
        u = next_double();
    } while (u == 0.0);
    return u;
}

// ============================================================
// MATH HELPERS (no libm dependency)
// ============================================================

static inline double custom_log(double x) {
    if (x <= 0.0) {
        union { uint64_t i; double d; } u;
        if (x == 0.0) {
            u.i = 0xFFF0000000000000ULL; // -Infinity
            return u.d;
        }
        u.i = 0x7FF8000000000000ULL; // NaN
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
    double val = x;
    for (int i = 0; i < 8; ++i) {
        val = 0.5 * (val + x / val);
    }
    return val;
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

// ============================================================
// PUBLIC API
// ============================================================

double calc_random_uniform(double min, double max) {
    return min + (max - min) * next_double();
}

double calc_random_normal(double mu, double sigma) {
    // Box-Muller transform
    double u1 = next_double_open();
    double u2 = next_double();

    double ln_u1 = custom_log(u1);
    double r = custom_sqrt(-2.0 * ln_u1);
    double theta = 2.0 * PI * u2;

    return mu + sigma * r * approx_cos(theta);
}

double calc_random_exponential(double lambda) {
    if (lambda <= 0.0) return 0.0;
    double u = next_double_open();
    return -custom_log(u) / lambda;
}

