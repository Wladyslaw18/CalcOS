#ifndef SUBTRACTION_H
#define SUBTRACTION_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar subtraction fallback
void sub_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// SSE 128-bit vectorized subtraction (processes 2 doubles at a time)
void sub_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// AVX2 256-bit vectorized subtraction (processes 4 doubles at a time)
void sub_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// ARM Neon vectorized subtraction (processes 2 doubles at a time)
void sub_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// Dynamic dispatch wrapper
void execute_subtraction(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features);

#endif // SUBTRACTION_H
