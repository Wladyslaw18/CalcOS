#ifndef CALC_H
#define CALC_H

#include "Kernel/State/CalculatorState.h"
#include "Kernel/State/NumericValue.h"
#include "Kernel/Core/CPU/CPUID.h"
#include "Kernel/Operations/Arithmetic/Addition.h"
#include "Kernel/Operations/Complex/ComplexOps.h"
#include "Kernel/Operations/Rational/RationalOps.h"
#include "Kernel/Operations/LinearAlgebra/LinearAlgebra.h"
#include "Kernel/Operations/Calculus/Calculus.h"
#include "Kernel/Operations/NumberTheory/NumberTheory.h"

// Initialize the static/global state and detect CPU features
void calc_init(void);

// Get a pointer to the global static calculator state (aligned to 64 bytes)
CalculatorState* calc_get_state(void);

// Addition
void calc_add(const double* a, const double* b, double* result, uint32_t count);

// Complex Operations
ComplexValue calc_complex_add(ComplexValue a, ComplexValue b);
ComplexValue calc_complex_sub(ComplexValue a, ComplexValue b);
ComplexValue calc_complex_mul(ComplexValue a, ComplexValue b);
ComplexValue calc_complex_div(ComplexValue a, ComplexValue b);
ComplexValue calc_complex_conjugate(ComplexValue a);
double calc_complex_modulus(ComplexValue a);
void calc_complex_polar(ComplexValue a, double* r, double* theta);

// Rational Operations
RationalValue calc_rational_add(RationalValue a, RationalValue b);
RationalValue calc_rational_sub(RationalValue a, RationalValue b);
RationalValue calc_rational_mul(RationalValue a, RationalValue b);
RationalValue calc_rational_div(RationalValue a, RationalValue b);
void calc_rational_simplify(RationalValue* r);

// Linear Algebra
bool calc_matrix_add(const double* A, uint32_t a_rows, uint32_t a_cols,
                     const double* B, uint32_t b_rows, uint32_t b_cols,
                     double* C);
bool calc_matrix_mul(const double* A, uint32_t a_rows, uint32_t a_cols,
                     const double* B, uint32_t b_rows, uint32_t b_cols,
                     double* C);
bool calc_matrix_transpose(const double* A, uint32_t rows, uint32_t cols, double* C);
bool calc_matrix_determinant(const double* A, uint32_t dim, double* result);
bool calc_matrix_inverse(const double* A, uint32_t dim, double* C);
bool calc_matrix_rref(const double* A, uint32_t rows, uint32_t cols, double* C);

// Calculus
double calc_derivative(double (*f)(double), double x, double h);
double calc_integral(double (*f)(double), double a, double b, uint32_t steps);
double calc_newton_raphson(double (*f)(double), double (*df)(double), double x0, uint32_t max_iter, double tol);
double calc_bisection(double (*f)(double), double a, double b, uint32_t max_iter, double tol);

// Number Theory
uint64_t calc_gcd(uint64_t a, uint64_t b);
uint64_t calc_lcm(uint64_t a, uint64_t b);
uint64_t calc_mod_exp(uint64_t base, uint64_t exp, uint64_t mod);
bool calc_is_prime(uint64_t n);

#endif // CALC_H
