/*
 * File: RNG.h
 * Author: W. Kowal
 * Description: High-performance seeded random number generation.
 */

#ifndef RNG_H
#define RNG_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Core / seeding                                                      */
/* ------------------------------------------------------------------ */

/* Seed the xoshiro256++ generator via SplitMix64 mixer.              */
void calc_random_seed(uint64_t seed);

/* ------------------------------------------------------------------ */
/*  State save / restore                                                */
/* ------------------------------------------------------------------ */

/* Copy the internal 256-bit xoshiro256++ state into caller-supplied   *
 * array.  out_state must point to at least 4 uint64_t values.         *
 * Allows saving the RNG position for reproducible replay.             */
void calc_random_get_state(uint64_t *out_state);

/* Restore the generator from a previously saved state array.          *
 * state must point to 4 uint64_t values.                              *
 * Rejects an all-zero state (invalid for xoshiro256++).               */
void calc_random_set_state(const uint64_t *state);

/* ------------------------------------------------------------------ */
/*  Distributions                                                       */
/* ------------------------------------------------------------------ */

/* Uniform real in [min, max).                                         */
double calc_random_uniform(double min, double max);

/* Normal (Gaussian) variate with given mean and standard deviation.   *
 * Uses polar Box-Muller with second-sample caching (no trig waste).   */
double calc_random_normal(double mu, double sigma);

/* Exponential variate with rate lambda.                               */
double calc_random_exponential(double lambda);

/* Bernoulli trial: returns 1.0 with probability p, else 0.0.         *
 * p is clamped to [0, 1].                                             */
double calc_random_bernoulli(double p);

/* Poisson variate with mean lambda (returned as double).              *
 * Uses Knuth's algorithm for lambda < 30, normal approximation        *
 * (clamped to >= 0) for lambda >= 30.                                 */
double calc_random_poisson(double lambda);

/* Gamma variate with given shape (alpha) and scale (beta).            *
 * Uses Marsaglia-Tsang's method for alpha >= 1; for alpha < 1 uses   *
 * the identity Gamma(alpha) = Gamma(alpha+1) * U^(1/alpha).          */
double calc_random_gamma(double shape, double scale);

#endif /* RNG_H */
