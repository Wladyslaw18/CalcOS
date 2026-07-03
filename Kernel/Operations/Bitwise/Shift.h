#ifndef SHIFT_H
#define SHIFT_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar Shift fallback (result = (int64_t)a << (int64_t)b)
void shift_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// SSE 128-bit vectorized Shift
void shift_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// AVX2 256-bit vectorized Shift
void shift_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// ARM Neon vectorized Shift
void shift_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// Dynamic dispatch wrapper
void execute_shift(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features);

#endif // SHIFT_H
