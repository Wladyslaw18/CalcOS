#include <stdio.h>
#include <x86intrin.h> // For __rdtsc()
#include "../../Kernel/State/CalculatorState.h"
#include "../../Kernel/Core/CPU/CPUID.h"
#include "../../Kernel/Operations/Arithmetic/Addition.h"
#include "../../Kernel/Operations/Arithmetic/Subtraction.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

#define ARRAY_SIZE 4096
#define ITERATIONS 10000

double a[ARRAY_SIZE] __attribute__((aligned(64)));
double b[ARRAY_SIZE] __attribute__((aligned(64)));
double r[ARRAY_SIZE] __attribute__((aligned(64)));

int main() {
    printf("=== RUNNING BenchSIMD ===\n");

    CPUFeatures features;
    cpu_detect_features(&features);

    for (int i = 0; i < ARRAY_SIZE; i++) {
        a[i] = (double)i * 1.5;
        b[i] = (double)i * 0.5;
    }

    CalculatorState state;
    fast_memset(&state, 0, sizeof(state));

    // Warm-up
    add_scalar(&state, a, b, r, ARRAY_SIZE);

    // Bench addition scalar
    uint64_t t0 = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        add_scalar(&state, a, b, r, ARRAY_SIZE);
    }
    uint64_t t1 = __rdtsc();
    uint64_t cycles_add_scalar = (t1 - t0) / ITERATIONS;

    // Bench addition SSE
    t0 = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        add_sse(&state, a, b, r, ARRAY_SIZE);
    }
    t1 = __rdtsc();
    uint64_t cycles_add_sse = (t1 - t0) / ITERATIONS;

    // Bench addition AVX2
    t0 = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        add_avx2(&state, a, b, r, ARRAY_SIZE);
    }
    t1 = __rdtsc();
    uint64_t cycles_add_avx2 = (t1 - t0) / ITERATIONS;

    // Bench subtraction scalar
    t0 = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        sub_scalar(&state, a, b, r, ARRAY_SIZE);
    }
    t1 = __rdtsc();
    uint64_t cycles_sub_scalar = (t1 - t0) / ITERATIONS;

    // Bench subtraction SSE
    t0 = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        sub_sse(&state, a, b, r, ARRAY_SIZE);
    }
    t1 = __rdtsc();
    uint64_t cycles_sub_sse = (t1 - t0) / ITERATIONS;

    // Bench subtraction AVX2
    t0 = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        sub_avx2(&state, a, b, r, ARRAY_SIZE);
    }
    t1 = __rdtsc();
    uint64_t cycles_sub_avx2 = (t1 - t0) / ITERATIONS;

    printf("Results (Average CPU cycles for size %d):\n", ARRAY_SIZE);
    printf("  Addition Scalar:    %llu cycles\n", cycles_add_scalar);
    printf("  Addition SSE:       %llu cycles\n", cycles_add_sse);
    printf("  Addition AVX2:      %llu cycles\n", cycles_add_avx2);
    printf("  Subtraction Scalar: %llu cycles\n", cycles_sub_scalar);
    printf("  Subtraction SSE:    %llu cycles\n", cycles_sub_sse);
    printf("  Subtraction AVX2:   %llu cycles\n", cycles_sub_avx2);

    return 0;
}
