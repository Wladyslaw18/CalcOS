#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool has_sse;
    bool has_sse2;
    bool has_sse3;
    bool has_avx;
    bool has_avx2;
    bool has_fma;
    bool has_neon;
} CPUFeatures;

// Query CPUID for SIMD capabilities
void cpu_detect_features(CPUFeatures* features);

#endif // CPUID_H
