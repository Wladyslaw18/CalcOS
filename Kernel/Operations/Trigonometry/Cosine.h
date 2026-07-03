#ifndef COSINE_H
#define COSINE_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar cosine fallback
void cos_scalar(CalculatorState* state, const double* a, double* result, uint32_t count);

// SSE 128-bit vectorized cosine
void cos_sse(CalculatorState* state, const double* a, double* result, uint32_t count);

// AVX2 256-bit vectorized cosine
void cos_avx2(CalculatorState* state, const double* a, double* result, uint32_t count);

// ARM Neon vectorized cosine
void cos_neon(CalculatorState* state, const double* a, double* result, uint32_t count);

// Dynamic dispatch wrapper
void execute_cosine(CalculatorState* state, const double* a, double* result, uint32_t count, const CPUFeatures* features);

#endif // COSINE_H
