/*
 * File: Statistics.c
 * Author: W. Kowal
 * Description: Optimized statistics functions.
 */

#include "Statistics.h"
#include <stdbool.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

// sqrt via Newton, seeded from exponent halving
static inline double custom_sqrt(double x) {
    if (x <= 0.0) return 0.0;
    union { double d; uint64_t i; } u;
    u.d = x;
    int exp = (int)((u.i >> 52) & 0x7FF) - 1023;
    int new_exp = (exp / 2) + 1023;
    u.i = (u.i & 0x800FFFFFFFFFFFFFULL) | ((uint64_t)new_exp << 52);
    double val = u.d;
    val = 0.5 * (val + x / val);
    val = 0.5 * (val + x / val);
    val = 0.5 * (val + x / val);
    val = 0.5 * (val + x / val);
    return val;
}

double calc_mean(const double* arr, uint32_t len) {
    if (len == 0 || !arr) return 0.0;
    double sum = 0.0;
    uint32_t i = 0;

#if defined(COMPILER_X86)
#if defined(USE_AVX2) && USE_AVX2
    // Process 4 doubles at a time
    __m256d vsum = _mm256_setzero_pd();
    for (; i + 3 < len; i += 4) {
        __m256d val = _mm256_loadu_pd(&arr[i]);
        vsum = _mm256_add_pd(vsum, val);
    }
    double temp[4];
    _mm256_storeu_pd(temp, vsum);
    sum += temp[0] + temp[1] + temp[2] + temp[3];
#elif defined(USE_SSE2) && USE_SSE2
    // Process 2 doubles at a time
    __m128d vsum = _mm_setzero_pd();
    for (; i + 1 < len; i += 2) {
        __m128d val = _mm_loadu_pd(&arr[i]);
        vsum = _mm_add_pd(vsum, val);
    }
    double temp[2];
    _mm_storeu_pd(temp, vsum);
    sum += temp[0] + temp[1];
#endif
#endif

    for (; i < len; ++i) {
        sum += arr[i];
    }
    return sum / len;
}

static inline bool is_nan_check(double x) {
    union { double d; uint64_t i; } u;
    u.d = x;
    return ((u.i & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) &&
           ((u.i & 0x000FFFFFFFFFFFFFULL) != 0ULL);
}

static inline bool nan_safe_less(double a, double b) {
    bool a_nan = is_nan_check(a);
    bool b_nan = is_nan_check(b);
    if (a_nan && b_nan) return false;
    if (a_nan) return false; // Push NaNs to the right (greater)
    if (b_nan) return true;  // Real numbers are smaller than NaN
    return a < b;
}

static void quicksort(double* a, int32_t left, int32_t right) {
    while (left < right) {
        int32_t mid = left + (right - left) / 2;
        
        // Median-of-three pivot selection with NaN safety
        if (nan_safe_less(a[mid], a[left])) {
            double tmp = a[left]; a[left] = a[mid]; a[mid] = tmp;
        }
        if (nan_safe_less(a[right], a[left])) {
            double tmp = a[left]; a[left] = a[right]; a[right] = tmp;
        }
        if (nan_safe_less(a[mid], a[right])) {
            double tmp = a[right]; a[right] = a[mid]; a[mid] = tmp;
        }
        
        double pivot = a[right];
        int32_t i = left - 1;
        for (int32_t j = left; j < right; ++j) {
            if (nan_safe_less(a[j], pivot)) {
                i++;
                double tmp = a[i];
                a[i] = a[j];
                a[j] = tmp;
            }
        }
        double tmp = a[i + 1];
        a[i + 1] = a[right];
        a[right] = tmp;
        int32_t pivot_idx = i + 1;
        
        // Tail-call optimization: recurse on smaller partition
        if (pivot_idx - left < right - pivot_idx) {
            quicksort(a, left, pivot_idx - 1);
            left = pivot_idx + 1;
        } else {
            quicksort(a, pivot_idx + 1, right);
            right = pivot_idx - 1;
        }
    }
}

double calc_median(double* arr, uint32_t len) {
    if (len == 0 || !arr) return 0.0;
    // Small arrays: copy to 2KB stack buf, preserve input array
    // Large arrays: sort in-place (caller must be OK with mutation)
    double stack_buf[256];
    double* target;
    if (len <= 256) {
        for (uint32_t i = 0; i < len; i++) stack_buf[i] = arr[i];
        target = stack_buf;
    } else {
        target = arr;
    }
    quicksort(target, 0, (int32_t)len - 1);
    if (len % 2 == 1) {
        return target[len / 2];
    } else {
        return (target[len / 2 - 1] + target[len / 2]) * 0.5;
    }
}

double calc_variance(const double* arr, uint32_t len) {
    if (len <= 1 || !arr) return 0.0;
    double mean = calc_mean(arr, len);
    double sum_sq_diff = 0.0;
    uint32_t i = 0;

#if defined(COMPILER_X86)
#if defined(USE_AVX2) && USE_AVX2
    __m256d vmean = _mm256_set1_pd(mean);
    __m256d vsum_sq = _mm256_setzero_pd();
    for (; i + 3 < len; i += 4) {
        __m256d val = _mm256_loadu_pd(&arr[i]);
        __m256d diff = _mm256_sub_pd(val, vmean);
        vsum_sq = _mm256_add_pd(vsum_sq, _mm256_mul_pd(diff, diff));
    }
    double temp[4];
    _mm256_storeu_pd(temp, vsum_sq);
    sum_sq_diff += temp[0] + temp[1] + temp[2] + temp[3];
#elif defined(USE_SSE2) && USE_SSE2
    __m128d vmean = _mm_set1_pd(mean);
    __m128d vsum_sq = _mm_setzero_pd();
    for (; i + 1 < len; i += 2) {
        __m128d val = _mm_loadu_pd(&arr[i]);
        __m128d diff = _mm_sub_pd(val, vmean);
        vsum_sq = _mm_add_pd(vsum_sq, _mm_mul_pd(diff, diff));
    }
    double temp[2];
    _mm_storeu_pd(temp, vsum_sq);
    sum_sq_diff += temp[0] + temp[1];
#endif
#endif

    for (; i < len; ++i) {
        double diff = arr[i] - mean;
        sum_sq_diff += diff * diff;
    }
    return sum_sq_diff / (len - 1);
}

double calc_covariance(const double* x_arr, const double* y_arr, uint32_t len) {
    if (len <= 1 || !x_arr || !y_arr) return 0.0;
    double mean_x = calc_mean(x_arr, len);
    double mean_y = calc_mean(y_arr, len);
    double sum_coproduct = 0.0;
    uint32_t i = 0;

#if defined(COMPILER_X86)
#if defined(USE_AVX2) && USE_AVX2
    __m256d vmean_x = _mm256_set1_pd(mean_x);
    __m256d vmean_y = _mm256_set1_pd(mean_y);
    __m256d vsum_coprod = _mm256_setzero_pd();
    for (; i + 3 < len; i += 4) {
        __m256d vx = _mm256_loadu_pd(&x_arr[i]);
        __m256d vy = _mm256_loadu_pd(&y_arr[i]);
        __m256d diff_x = _mm256_sub_pd(vx, vmean_x);
        __m256d diff_y = _mm256_sub_pd(vy, vmean_y);
        vsum_coprod = _mm256_add_pd(vsum_coprod, _mm256_mul_pd(diff_x, diff_y));
    }
    double temp[4];
    _mm256_storeu_pd(temp, vsum_coprod);
    sum_coproduct += temp[0] + temp[1] + temp[2] + temp[3];
#elif defined(USE_SSE2) && USE_SSE2
    __m128d vmean_x = _mm_set1_pd(mean_x);
    __m128d vmean_y = _mm_set1_pd(mean_y);
    __m128d vsum_coprod = _mm_setzero_pd();
    for (; i + 1 < len; i += 2) {
        __m128d vx = _mm_loadu_pd(&x_arr[i]);
        __m128d vy = _mm_loadu_pd(&y_arr[i]);
        __m128d diff_x = _mm_sub_pd(vx, vmean_x);
        __m128d diff_y = _mm_sub_pd(vy, vmean_y);
        vsum_coprod = _mm_add_pd(vsum_coprod, _mm_mul_pd(diff_x, diff_y));
    }
    double temp[2];
    _mm_storeu_pd(temp, vsum_coprod);
    sum_coproduct += temp[0] + temp[1];
#endif
#endif

    for (; i < len; ++i) {
        sum_coproduct += (x_arr[i] - mean_x) * (y_arr[i] - mean_y);
    }
    return sum_coproduct / (len - 1);
}

double calc_correlation(const double* x_arr, const double* y_arr, uint32_t len) {
    if (len <= 1 || !x_arr || !y_arr) return 0.0;
    double var_x = calc_variance(x_arr, len);
    double var_y = calc_variance(y_arr, len);
    if (var_x <= 0.0 || var_y <= 0.0) return 0.0;
    
    double cov = calc_covariance(x_arr, y_arr, len);
    double std_dev_x = custom_sqrt(var_x);
    double std_dev_y = custom_sqrt(var_y);
    
    return cov / (std_dev_x * std_dev_y);
}
