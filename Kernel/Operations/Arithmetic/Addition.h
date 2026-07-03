#ifndef ADDITION_H
#define ADDITION_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar addition fallback
void add_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// SSE 128-bit vectorized addition (processes 2 doubles at a time)
void add_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// AVX2 256-bit vectorized addition (processes 4 doubles at a time)
void add_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// Dynamic dispatch wrapper - selects the absolute fastest pipeline at runtime!
void execute_addition(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features);

#endif // ADDITION_H
