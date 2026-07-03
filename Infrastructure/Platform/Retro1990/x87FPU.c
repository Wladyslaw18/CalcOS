#include "x87FPU.h"

// x87 coprocessor addition: fld loads to ST(0), fadd adds to ST(0), fstp stores ST(0) back to memory
void x87_add(const double* a, const double* b, double* result, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        double val_a = a[i];
        double val_b = b[i];
        double res;
        __asm__ volatile (
            "fldl %1;"     // Load val_a into ST(0)
            "faddl %2;"    // Add val_b to ST(0)
            "fstpl %0;"    // Store ST(0) to res and pop
            : "=m"(res)
            : "m"(val_a), "m"(val_b)
        );
        result[i] = res;
    }
}

void x87_sub(const double* a, const double* b, double* result, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        double val_a = a[i];
        double val_b = b[i];
        double res;
        __asm__ volatile (
            "fldl %1;"     // Load val_a into ST(0)
            "fsubl %2;"    // Subtract val_b from ST(0)
            "fstpl %0;"    // Store ST(0) to res and pop
            : "=m"(res)
            : "m"(val_a), "m"(val_b)
        );
        result[i] = res;
    }
}

void x87_mul(const double* a, const double* b, double* result, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        double val_a = a[i];
        double val_b = b[i];
        double res;
        __asm__ volatile (
            "fldl %1;"     // Load val_a into ST(0)
            "fmull %2;"    // Multiply by val_b
            "fstpl %0;"    // Store ST(0) to res and pop
            : "=m"(res)
            : "m"(val_a), "m"(val_b)
        );
        result[i] = res;
    }
}

void x87_div(const double* a, const double* b, double* result, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        double val_a = a[i];
        double val_b = b[i];
        double res;
        __asm__ volatile (
            "fldl %1;"     // Load val_a into ST(0)
            "fdivl %2;"    // Divide by val_b
            "fstpl %0;"    // Store ST(0) to res and pop
            : "=m"(res)
            : "m"(val_a), "m"(val_b)
        );
        result[i] = res;
    }
}
