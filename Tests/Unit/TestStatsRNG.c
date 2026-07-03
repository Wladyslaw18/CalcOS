/*
 * File: TestStatsRNG.c
 * Author: W. Kowal
 * Description: Unit tests for statistics and RNG functions.
 */

#include "TestAssert.h"
#include "../../Kernel/Operations/Statistics/Statistics.h"
#include "../../Kernel/Operations/RNG/RNG.h"

int main() {
    printf("=== RUNNING TestStatsRNG ===\n");

    // 1. Test mean calculation
    double data_mean[5] = {10.0, 20.0, 30.0, 40.0, 50.0};
    double mean = calc_mean(data_mean, 5);
    assert_double_eq(mean, 30.0);

    double data_mean2[4] = {1.5, 2.5, 3.5, 4.5};
    double mean2 = calc_mean(data_mean2, 4);
    assert_double_eq(mean2, 3.0);

    // 2. Test median calculation
    double data_median_odd[5] = {3.0, 1.0, 5.0, 4.0, 2.0};
    double median_odd = calc_median(data_median_odd, 5);
    assert_double_eq(median_odd, 3.0);
    // Ensure the original array was NOT modified (due to stack copy)
    assert_double_eq(data_median_odd[0], 3.0);

    double data_median_even[6] = {3.0, 1.0, 5.0, 4.0, 2.0, 6.0};
    double median_even = calc_median(data_median_even, 6);
    assert_double_eq(median_even, 3.5);

    // 3. Test variance calculation
    double data_var[5] = {2.0, 4.0, 4.0, 4.0, 5.5};
    double var = calc_variance(data_var, 5);
    assert_double_eq(var, 1.55);

    // 4. Test covariance calculation
    double x[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y[5] = {2.0, 4.0, 6.0, 8.0, 10.0};
    double cov = calc_covariance(x, y, 5);
    assert_double_eq(cov, 5.0);

    // 5. Test correlation calculation
    double corr = calc_correlation(x, y, 5);
    assert_double_eq(corr, 1.0);

    // 6. Test RNG functions
    calc_random_seed(42ULL);
    
    // Check uniform distribution range
    for (int i = 0; i < 100; ++i) {
        double u = calc_random_uniform(-5.0, 5.0);
        assert_true(u >= -5.0 && u < 5.0);
    }

    // Check normal distribution generation (Box-Muller)
    double norm_sum = 0.0;
    for (int i = 0; i < 1000; ++i) {
        double n = calc_random_normal(0.0, 1.0);
        norm_sum += n;
    }
    double norm_mean = norm_sum / 1000.0;
    assert_true(norm_mean >= -0.5 && norm_mean <= 0.5);

    // Check exponential distribution generation
    double exp_sum = 0.0;
    double lambda = 2.0;
    for (int i = 0; i < 1000; ++i) {
        double e = calc_random_exponential(lambda);
        assert_true(e >= 0.0);
        exp_sum += e;
    }
    double exp_mean = exp_sum / 1000.0;
    assert_true(exp_mean >= 0.3 && exp_mean <= 0.7);

    // Print summary
    printf("TestStatsRNG summary: %d/%d assertions passed.\n", 
           total_assertions - failed_assertions, total_assertions);
    return failed_assertions > 0 ? 1 : 0;
}
