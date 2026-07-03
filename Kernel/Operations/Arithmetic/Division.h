#ifndef DIVISION_H
#define DIVISION_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar division fallback
void div_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// SSE 128-bit vectorized division (processes 2 doubles at a time)
void div_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// AVX2 256-bit vectorized division (processes 4 doubles at a time)
void div_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// ARM Neon vectorized division (processes 2 doubles at a time)
void div_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// Dynamic dispatch wrapper
void execute_division(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features);

#endif // DIVISION_H
