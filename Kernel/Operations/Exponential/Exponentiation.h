#ifndef EXPONENTIATION_H
#define EXPONENTIATION_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar exponentiation fallback (result = a^b)
void exp_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// SSE 128-bit vectorized exponentiation
void exp_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// AVX2 256-bit vectorized exponentiation
void exp_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// ARM Neon vectorized exponentiation
void exp_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// Dynamic dispatch wrapper
void execute_exponentiation(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features);

#endif // EXPONENTIATION_H
