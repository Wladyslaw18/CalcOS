#ifndef SINE_H
#define SINE_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar sine fallback
void sin_scalar(CalculatorState* state, const double* a, double* result, uint32_t count);

// SSE 128-bit vectorized sine
void sin_sse(CalculatorState* state, const double* a, double* result, uint32_t count);

// AVX2 256-bit vectorized sine
void sin_avx2(CalculatorState* state, const double* a, double* result, uint32_t count);

// ARM Neon vectorized sine
void sin_neon(CalculatorState* state, const double* a, double* result, uint32_t count);

// Dynamic dispatch wrapper
void execute_sine(CalculatorState* state, const double* a, double* result, uint32_t count, const CPUFeatures* features);

#endif // SINE_H
