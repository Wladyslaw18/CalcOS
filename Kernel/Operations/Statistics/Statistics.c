/*
 * File: Statistics.c
 * Author: W. Kowal
 * Description: Optimized statistics functions.
 */

#include "Statistics.h"
#include "../../../Infrastructure/Utils/MathUtils.h"
#include "../../Core/CPU/CPUID.h"
#include <stdbool.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define COMPILER_X86
#endif

#if defined(__GNUC__) || defined(__clang__)
#define TARGET_AVX2 __attribute__((target("avx2")))
#define TARGET_SSE2 __attribute__((target("sse2")))
#else
#define TARGET_AVX2
#define TARGET_SSE2
#endif

static inline const CPUFeatures* get_cpu_features(void) {
    static CPUFeatures features;
    static bool detected = false;
    if (!detected) {
        cpu_detect_features(&features);
        detected = true;
    }
    return &features;
}

double calc_mean_scalar(const double* arr, uint32_t len) {
    double sum = 0.0;
    for (uint32_t i = 0; i < len; ++i) {
        sum += arr[i];
    }
    return sum / len;
}

TARGET_SSE2 double calc_mean_sse2(const double* arr, uint32_t len) {
    double sum = 0.0;
    uint32_t i = 0;
#ifdef COMPILER_X86
    __m128d vsum = _mm_setzero_pd();
    for (; i + 1 < len; i += 2) {
        __m128d val = _mm_loadu_pd(&arr[i]);
        vsum = _mm_add_pd(vsum, val);
    }
    double temp[2];
    _mm_storeu_pd(temp, vsum);
    sum += temp[0] + temp[1];
#endif
    for (; i < len; ++i) {
        sum += arr[i];
    }
    return sum / len;
}

TARGET_AVX2 double calc_mean_avx2(const double* arr, uint32_t len) {
    double sum = 0.0;
    uint32_t i = 0;
#ifdef COMPILER_X86
    __m256d vsum = _mm256_setzero_pd();
    for (; i + 3 < len; i += 4) {
        __m256d val = _mm256_loadu_pd(&arr[i]);
        vsum = _mm256_add_pd(vsum, val);
    }
    double temp[4];
    _mm256_storeu_pd(temp, vsum);
    sum += temp[0] + temp[1] + temp[2] + temp[3];
#endif
    for (; i < len; ++i) {
        sum += arr[i];
    }
    return sum / len;
}

double calc_mean(const double* arr, uint32_t len) {
    if (len == 0 || !arr) return 0.0;
    const CPUFeatures* features = get_cpu_features();
    if (features->has_avx2) {
        return calc_mean_avx2(arr, len);
    } else if (features->has_sse2) {
        return calc_mean_sse2(arr, len);
    } else {
        return calc_mean_scalar(arr, len);
    }
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

TARGET_AVX2 double calc_variance_with_mean(const double* arr, uint32_t len, double mean) {
    double sum_sq_diff = 0.0;
    uint32_t i = 0;
    const CPUFeatures* features = get_cpu_features();

#ifdef COMPILER_X86
    (void)features;
    if (features->has_avx2) {
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
    } else if (features->has_sse2) {
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
    }
#endif

    for (; i < len; ++i) {
        double diff = arr[i] - mean;
        sum_sq_diff += diff * diff;
    }
    return sum_sq_diff / (double)(len - 1);
}

double calc_variance(const double* arr, uint32_t len) {
    if (len <= 1 || !arr) return 0.0;
    double mean = calc_mean(arr, len);
    return calc_variance_with_mean(arr, len, mean);
}

TARGET_AVX2 double calc_covariance_with_means(const double* x_arr, const double* y_arr, uint32_t len, double mean_x, double mean_y) {
    double sum_coproduct = 0.0;
    uint32_t i = 0;
    const CPUFeatures* features = get_cpu_features();

#ifdef COMPILER_X86
    (void)features;
    if (features->has_avx2) {
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
    } else if (features->has_sse2) {
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
    }
#endif

    for (; i < len; ++i) {
        sum_coproduct += (x_arr[i] - mean_x) * (y_arr[i] - mean_y);
    }
    return sum_coproduct / (double)(len - 1);
}

double calc_covariance(const double* x_arr, const double* y_arr, uint32_t len) {
    if (len <= 1 || !x_arr || !y_arr) return 0.0;
    double mean_x = calc_mean(x_arr, len);
    double mean_y = calc_mean(y_arr, len);
    return calc_covariance_with_means(x_arr, y_arr, len, mean_x, mean_y);
}

TARGET_AVX2 void calc_var_covar_combined(const double* x_arr, const double* y_arr, uint32_t len,
                             double mean_x, double mean_y,
                             double* out_var_x, double* out_var_y, double* out_cov) {
    double sum_sq_x = 0.0;
    double sum_sq_y = 0.0;
    double sum_coprod = 0.0;
    uint32_t i = 0;
    const CPUFeatures* features = get_cpu_features();

#ifdef COMPILER_X86
    (void)features;
    if (features->has_avx2) {
        __m256d vmean_x = _mm256_set1_pd(mean_x);
        __m256d vmean_y = _mm256_set1_pd(mean_y);
        __m256d vsum_x = _mm256_setzero_pd();
        __m256d vsum_y = _mm256_setzero_pd();
        __m256d vsum_cp = _mm256_setzero_pd();
        for (; i + 3 < len; i += 4) {
            __m256d vx = _mm256_loadu_pd(&x_arr[i]);
            __m256d vy = _mm256_loadu_pd(&y_arr[i]);
            __m256d dx = _mm256_sub_pd(vx, vmean_x);
            __m256d dy = _mm256_sub_pd(vy, vmean_y);
            vsum_x = _mm256_add_pd(vsum_x, _mm256_mul_pd(dx, dx));
            vsum_y = _mm256_add_pd(vsum_y, _mm256_mul_pd(dy, dy));
            vsum_cp = _mm256_add_pd(vsum_cp, _mm256_mul_pd(dx, dy));
        }
        double tx[4], ty[4], tcp[4];
        _mm256_storeu_pd(tx, vsum_x);
        _mm256_storeu_pd(ty, vsum_y);
        _mm256_storeu_pd(tcp, vsum_cp);
        sum_sq_x += tx[0] + tx[1] + tx[2] + tx[3];
        sum_sq_y += ty[0] + ty[1] + ty[2] + ty[3];
        sum_coprod += tcp[0] + tcp[1] + tcp[2] + tcp[3];
    } else if (features->has_sse2) {
        __m128d vmean_x = _mm_set1_pd(mean_x);
        __m128d vmean_y = _mm_set1_pd(mean_y);
        __m128d vsum_x = _mm_setzero_pd();
        __m128d vsum_y = _mm_setzero_pd();
        __m128d vsum_cp = _mm_setzero_pd();
        for (; i + 1 < len; i += 2) {
            __m128d vx = _mm_loadu_pd(&x_arr[i]);
            __m128d vy = _mm_loadu_pd(&y_arr[i]);
            __m128d dx = _mm_sub_pd(vx, vmean_x);
            __m128d dy = _mm_sub_pd(vy, vmean_y);
            vsum_x = _mm_add_pd(vsum_x, _mm_mul_pd(dx, dx));
            vsum_y = _mm_add_pd(vsum_y, _mm_mul_pd(dy, dy));
            vsum_cp = _mm_add_pd(vsum_cp, _mm_mul_pd(dx, dy));
        }
        double tx[2], ty[2], tcp[2];
        _mm_storeu_pd(tx, vsum_x);
        _mm_storeu_pd(ty, vsum_y);
        _mm_storeu_pd(tcp, vsum_cp);
        sum_sq_x += tx[0] + tx[1];
        sum_sq_y += ty[0] + ty[1];
        sum_coprod += tcp[0] + tcp[1];
    }
#endif

    for (; i < len; ++i) {
        double dx = x_arr[i] - mean_x;
        double dy = y_arr[i] - mean_y;
        sum_sq_x += dx * dx;
        sum_sq_y += dy * dy;
        sum_coprod += dx * dy;
    }

    *out_var_x = sum_sq_x / (double)(len - 1);
    *out_var_y = sum_sq_y / (double)(len - 1);
    *out_cov   = sum_coprod / (double)(len - 1);
}

double calc_correlation(const double* x_arr, const double* y_arr, uint32_t len) {
    if (len <= 1 || !x_arr || !y_arr) return 0.0;
    double mean_x = calc_mean(x_arr, len);
    double mean_y = calc_mean(y_arr, len);
    double var_x = 0.0, var_y = 0.0, cov = 0.0;
    calc_var_covar_combined(x_arr, y_arr, len, mean_x, mean_y, &var_x, &var_y, &cov);
    if (var_x <= 0.0 || var_y <= 0.0) return 0.0;
    
    double std_dev_x = math_sqrt(var_x);
    double std_dev_y = math_sqrt(var_y);
    
    return cov / (std_dev_x * std_dev_y);
}

double calc_percentile(double* arr, uint32_t len, double p) {
    if (!arr || len == 0) return 0.0;

    // Clamp p to [0, 100]
    if (p < 0.0)   p = 0.0;
    if (p > 100.0) p = 100.0;

    double stack_buf[256];
    double* target;
    if (len <= 256) {
        for (uint32_t i = 0; i < len; i++) stack_buf[i] = arr[i];
        target = stack_buf;
    } else {
        target = arr;
    }
    quicksort(target, 0, (int32_t)len - 1);

    // Map p into a 0-based floating index
    double index = (p / 100.0) * (double)(len - 1);
    uint32_t lo  = (uint32_t)index;
    uint32_t hi  = lo + 1;
    double   frac = index - (double)lo;

    if (hi >= len) return target[len - 1]; // p == 100 or single element
    return target[lo] + frac * (target[hi] - target[lo]);
}

TARGET_AVX2 double calc_skewness(const double* arr, uint32_t len) {
    if (!arr || len < 3) return math_nan();

    double mean = calc_mean(arr, len);
    double var  = calc_variance_with_mean(arr, len, mean);   // sample variance (÷ n-1)
    if (var <= 0.0) return 0.0;              // constant array → skewness = 0

    double stddev     = math_sqrt(var);
    double stddev3    = stddev * stddev * stddev;
    double sum_cube   = 0.0;
    uint32_t i        = 0;
    const CPUFeatures* features = get_cpu_features();

#ifdef COMPILER_X86
    (void)features;
    if (features->has_avx2) {
        __m256d vmean      = _mm256_set1_pd(mean);
        __m256d vsum_cube  = _mm256_setzero_pd();
        for (; i + 3 < len; i += 4) {
            __m256d val  = _mm256_loadu_pd(&arr[i]);
            __m256d diff = _mm256_sub_pd(val, vmean);
            __m256d d2   = _mm256_mul_pd(diff, diff);
            __m256d d3   = _mm256_mul_pd(d2,   diff);
            vsum_cube    = _mm256_add_pd(vsum_cube, d3);
        }
        double temp[4];
        _mm256_storeu_pd(temp, vsum_cube);
        sum_cube += temp[0] + temp[1] + temp[2] + temp[3];
    } else if (features->has_sse2) {
        __m128d vmean      = _mm_set1_pd(mean);
        __m128d vsum_cube  = _mm_setzero_pd();
        for (; i + 1 < len; i += 2) {
            __m128d val  = _mm_loadu_pd(&arr[i]);
            __m128d diff = _mm_sub_pd(val, vmean);
            __m128d d2   = _mm_mul_pd(diff, diff);
            __m128d d3   = _mm_mul_pd(d2,   diff);
            vsum_cube    = _mm_add_pd(vsum_cube, d3);
        }
        double temp[2];
        _mm_storeu_pd(temp, vsum_cube);
        sum_cube += temp[0] + temp[1];
    }
#endif

    for (; i < len; ++i) {
        double diff = arr[i] - mean;
        sum_cube += diff * diff * diff;
    }

    double moment3 = sum_cube / (double)len;
    return moment3 / stddev3;
}

TARGET_AVX2 double calc_kurtosis(const double* arr, uint32_t len) {
    if (!arr || len < 4) return math_nan();

    double mean = calc_mean(arr, len);
    double var  = calc_variance_with_mean(arr, len, mean);   // sample variance (÷ n-1)
    if (var <= 0.0) return 0.0;              // constant array → kurtosis = 0 (excess)

    double var2      = var * var;
    double sum_quart = 0.0;
    uint32_t i       = 0;
    const CPUFeatures* features = get_cpu_features();

#ifdef COMPILER_X86
    (void)features;
    if (features->has_avx2) {
        __m256d vmean       = _mm256_set1_pd(mean);
        __m256d vsum_quart  = _mm256_setzero_pd();
        for (; i + 3 < len; i += 4) {
            __m256d val  = _mm256_loadu_pd(&arr[i]);
            __m256d diff = _mm256_sub_pd(val, vmean);
            __m256d d2   = _mm256_mul_pd(diff, diff);
            __m256d d4   = _mm256_mul_pd(d2,   d2);
            vsum_quart   = _mm256_add_pd(vsum_quart, d4);
        }
        double temp[4];
        _mm256_storeu_pd(temp, vsum_quart);
        sum_quart += temp[0] + temp[1] + temp[2] + temp[3];
    } else if (features->has_sse2) {
        __m128d vmean       = _mm_set1_pd(mean);
        __m128d vsum_quart  = _mm_setzero_pd();
        for (; i + 1 < len; i += 2) {
            __m128d val  = _mm_loadu_pd(&arr[i]);
            __m128d diff = _mm_sub_pd(val, vmean);
            __m128d d2   = _mm_mul_pd(diff, diff);
            __m128d d4   = _mm_mul_pd(d2,   d2);
            vsum_quart   = _mm_add_pd(vsum_quart, d4);
        }
        double temp[2];
        _mm_storeu_pd(temp, vsum_quart);
        sum_quart += temp[0] + temp[1];
    }
#endif

    for (; i < len; ++i) {
        double diff = arr[i] - mean;
        double d2   = diff * diff;
        sum_quart  += d2 * d2;
    }

    double moment4 = sum_quart / (double)len;
    return (moment4 / var2) - 3.0;
}

void calc_linear_regression(const double* x, const double* y, uint32_t n,
                             double* slope, double* intercept) {
    if (!x || !y || n == 0) {
        if (slope)     *slope     = 0.0;
        if (intercept) *intercept = 0.0;
        return;
    }

    double mean_x = calc_mean(x, n);
    double mean_y = calc_mean(y, n);
    double var_x  = calc_variance_with_mean(x, n, mean_x);

    if (var_x == 0.0) {
        // Degenerate: all x values identical → vertical cloud, no defined slope
        if (slope)     *slope     = 0.0;
        if (intercept) *intercept = mean_y;
        return;
    }

    double cov_xy = calc_covariance_with_means(x, y, n, mean_x, mean_y);
    double s      = cov_xy / var_x;

    if (slope)     *slope     = s;
    if (intercept) *intercept = mean_y - s * mean_x;
}
