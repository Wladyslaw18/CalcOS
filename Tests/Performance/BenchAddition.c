#include <stdio.h>
#include <x86intrin.h> // For __rdtsc() and intrinsics
#include "../../Kernel/State/CalculatorState.h"
#include "../../Kernel/Core/CPU/CPUID.h"
#include "../../Kernel/Operations/Arithmetic/Addition.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

#define ARRAY_SIZE 4096
#define ITERATIONS 10000

// Cache line aligned buffers to avoid cache penalty and misaligned access misses
double input_a[ARRAY_SIZE] __attribute__((aligned(64)));
double input_b[ARRAY_SIZE] __attribute__((aligned(64)));
double output_res[ARRAY_SIZE] __attribute__((aligned(64)));

int main() {
    CPUFeatures features;
    cpu_detect_features(&features);

    printf("=== HARDWARE ACCELERATION PROFILE ===\n");
    printf("AVX2 Support: %s\n", features.has_avx2 ? "DETECTED 🔥" : "MISSING 😭");
    printf("SSE2 Support: %s\n", features.has_sse2 ? "DETECTED 🔥" : "MISSING 😭");

    // Initialize arrays
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        input_a[i] = (double)i * 0.1;
        input_b[i] = (double)i * 0.2;
    }

    CalculatorState state;
    fast_memset(&state, 0, sizeof(CalculatorState));

    // Warm-up cache lines to avoid cold-start penalties
    add_scalar(&state, input_a, input_b, output_res, ARRAY_SIZE);

    // Benchmark Scalar Fallback
    uint64_t start_scalar = __rdtsc();
    for (int i = 0; i < ITERATIONS; ++i) {
        add_scalar(&state, input_a, input_b, output_res, ARRAY_SIZE);
    }
    uint64_t end_scalar = __rdtsc();

    // Benchmark SSE
    uint64_t start_sse = __rdtsc();
    for (int i = 0; i < ITERATIONS; ++i) {
        add_sse(&state, input_a, input_b, output_res, ARRAY_SIZE);
    }
    uint64_t end_sse = __rdtsc();

    // Benchmark AVX2
    uint64_t start_avx2 = __rdtsc();
    for (int i = 0; i < ITERATIONS; ++i) {
        add_avx2(&state, input_a, input_b, output_res, ARRAY_SIZE);
    }
    uint64_t end_avx2 = __rdtsc();

    uint64_t scalar_cycles = (end_scalar - start_scalar) / ITERATIONS;
    uint64_t sse_cycles = (end_sse - start_sse) / ITERATIONS;
    uint64_t avx2_cycles = (end_avx2 - start_avx2) / ITERATIONS;

    printf("\n=== PERFORMANCE RESULTS (AVG CYCLES PER ARRAY OF SIZE %d) ===\n", ARRAY_SIZE);
    printf("Scalar Fallback: %llu cycles\n", scalar_cycles);
    printf("SSE Vectorized : %llu cycles (Speedup: %.2fx)\n", sse_cycles, (double)scalar_cycles / sse_cycles);
    printf("AVX2 Vectorized: %llu cycles (Speedup: %.2fx) 🔥\n", avx2_cycles, (double)scalar_cycles / avx2_cycles);

    return 0;
}
