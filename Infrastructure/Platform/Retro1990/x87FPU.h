#ifndef X87_FPU_H
#define X87_FPU_H

#include "../../Kernel/State/CalculatorState.h"

// x87 Coprocessor Float Math Implementations (NO SSE/AVX EMOJIS OR LOGIC!)
void x87_add(const double* a, const double* b, double* result, uint32_t count);
void x87_sub(const double* a, const double* b, double* result, uint32_t count);
void x87_mul(const double* a, const double* b, double* result, uint32_t count);
void x87_div(const double* a, const double* b, double* result, uint32_t count);

#endif // X87_FPU_H
