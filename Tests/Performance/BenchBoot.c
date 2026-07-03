#include <stdio.h>
#include <x86intrin.h> // For __rdtsc()
#include "../../Kernel/State/CalculatorState.h"
#include "../../Kernel/Core/CPU/CPUID.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

int main() {
    printf("=== RUNNING BenchBoot ===\n");

    // Bench CPUID detection boot latency
    CPUFeatures features;
    uint64_t t0 = __rdtsc();
    cpu_detect_features(&features);
    uint64_t t1 = __rdtsc();
    uint64_t cpu_detect_cycles = t1 - t0;

    // Bench CalculatorState initialization boot latency
    CalculatorState state;
    t0 = __rdtsc();
    fast_memset(&state, 0, sizeof(CalculatorState));
    t1 = __rdtsc();
    uint64_t state_init_cycles = t1 - t0;

    printf("Boot Startup Profile:\n");
    printf("  CPU feature detection latency:   %llu cycles\n", cpu_detect_cycles);
    printf("  CalculatorState zero-init latency: %llu cycles\n", state_init_cycles);

    return 0;
}
