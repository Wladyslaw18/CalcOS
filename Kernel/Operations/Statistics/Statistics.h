/*
 * File: Statistics.h
 * Author: W. Kowal
 * Description: Optimized statistics functions.
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdint.h>

double calc_mean(const double* arr, uint32_t len);
double calc_median(double* arr, uint32_t len);
double calc_variance(const double* arr, uint32_t len);
double calc_covariance(const double* x_arr, const double* y_arr, uint32_t len);
double calc_correlation(const double* x_arr, const double* y_arr, uint32_t len);

/* p-th percentile (p in [0, 100]).
 * Stack-copies up to 256 elements; sorts in-place for larger arrays.
 * Uses linear interpolation between adjacent sorted values.
 * Returns 0.0 for NULL / len == 0. Clamps p to [0, 100]. */
double calc_percentile(double* arr, uint32_t len, double p);

/* Pearson's sample skewness: (1/n * Σ(xi-x̄)³) / stddev³.
 * Returns math_nan() for len < 3.
 * Vectorised: AVX2 → SSE2 → scalar. */
double calc_skewness(const double* arr, uint32_t len);

/* Excess kurtosis (Fisher): (1/n * Σ(xi-x̄)⁴) / variance² - 3.
 * Returns math_nan() for len < 4.
 * Vectorised: AVX2 → SSE2 → scalar. */
double calc_kurtosis(const double* arr, uint32_t len);

/* Ordinary Least Squares linear regression.
 * slope     = cov(x,y) / var(x)
 * intercept = mean(y) - slope * mean(x)
 * Degenerate (var(x)==0): slope=0, intercept=mean(y).
 * Either output pointer may be NULL to skip that result. */
void calc_linear_regression(const double* x, const double* y, uint32_t n,
                             double* slope, double* intercept);

#endif // STATISTICS_H
