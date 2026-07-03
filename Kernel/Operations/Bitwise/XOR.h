#ifndef XOR_H
#define XOR_H

#include "../../State/CalculatorState.h"
#include "../../Core/CPU/CPUID.h"

// Scalar XOR fallback (result = (int64_t)a ^ (int64_t)b)
void xor_scalar(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// SSE 128-bit vectorized XOR
void xor_sse(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// AVX2 256-bit vectorized XOR
void xor_avx2(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// ARM Neon vectorized XOR
void xor_neon(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count);

// Dynamic dispatch wrapper
void execute_xor(CalculatorState* state, const double* a, const double* b, double* result, uint32_t count, const CPUFeatures* features);

#endif // XOR_H
