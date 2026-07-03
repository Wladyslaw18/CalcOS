/*
 * File: RNG.h
 * Author: W. Kowal
 * Description: High-performance seeded random number generation.
 */

#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void calc_random_seed(uint64_t seed);
double calc_random_uniform(double min, double max);
double calc_random_normal(double mu, double sigma);
double calc_random_exponential(double lambda);

#endif // RNG_H
