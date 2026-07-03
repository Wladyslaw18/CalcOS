#ifndef TANGENT_H
#define TANGENT_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar tangent fallback
void tan_scalar(CalculatorState* state, const double* a, double* result, uint32_t count);

// SSE 128-bit vectorized tangent
void tan_sse(CalculatorState* state, const double* a, double* result, uint32_t count);

// AVX2 256-bit vectorized tangent
void tan_avx2(CalculatorState* state, const double* a, double* result, uint32_t count);

// ARM Neon vectorized tangent
void tan_neon(CalculatorState* state, const double* a, double* result, uint32_t count);

// Dynamic dispatch wrapper
void execute_tangent(CalculatorState* state, const double* a, double* result, uint32_t count, const CPUFeatures* features);

#endif // TANGENT_H
