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

#endif // STATISTICS_H
