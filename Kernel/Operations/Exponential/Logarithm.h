#ifndef LOGARITHM_H
#define LOGARITHM_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar logarithm fallback
void log_scalar(CalculatorState* state, const double* a, double* result, uint32_t count);

// SSE 128-bit vectorized logarithm
void log_sse(CalculatorState* state, const double* a, double* result, uint32_t count);

// AVX2 256-bit vectorized logarithm
void log_avx2(CalculatorState* state, const double* a, double* result, uint32_t count);

// ARM Neon vectorized logarithm
void log_neon(CalculatorState* state, const double* a, double* result, uint32_t count);

// Dynamic dispatch wrapper
void execute_logarithm(CalculatorState* state, const double* a, double* result, uint32_t count, const CPUFeatures* features);

#endif // LOGARITHM_H
