#ifndef CALCULUS_H
#define CALCULUS_H

#include "../../State/CalculatorState.h"

// Numerical derivative using central difference method
// f: Function pointer taking a double and returning a double
// x: Point at which derivative is evaluated
// h: Step size (if 0.0, defaults to optimal step)
double derivative(CalculatorState* state, double (*f)(double), double x, double h);

// Numerical integration using Adaptive Simpson's Rule or composite Simpson's rule
// f: Function pointer
// a: lower bound
// b: upper bound
// steps: number of intervals (must be even, if 0 defaults to 1000)
double integral(CalculatorState* state, double (*f)(double), double a, double b, uint32_t steps);

// Newton-Raphson method for root finding
// f: Function pointer
// df: Derivative function pointer (if NULL, numerical derivative will be used)
// x0: Initial guess
// max_iter: Maximum iterations (if 0, defaults to 100)
// tol: Tolerance (if 0.0, defaults to 1e-9)
double newton_raphson(CalculatorState* state, double (*f)(double), double (*df)(double), double x0, uint32_t max_iter, double tol);

// Bisection method for root finding
// f: Function pointer
// a: left bound
// b: right bound
// max_iter: Maximum iterations (if 0, defaults to 100)
// tol: Tolerance (if 0.0, defaults to 1e-9)
double bisection(CalculatorState* state, double (*f)(double), double a, double b, uint32_t max_iter, double tol);

// Numerical gradient of f: Rⁿ → R using central differences.
// out[i] = (f(x + h·eᵢ) - f(x - h·eᵢ)) / (2h)
// Internally stack-allocates a single x_perturb[8] scratch buffer.
// Returns false (no output written) if:
//   - any pointer is NULL
//   - n == 0 or n > 8  (stack buffer limit)
// h defaults to 1e-5 when passed as 0.0 (near-optimal for double precision).
bool calc_gradient(CalculatorState* state, double (*f)(const double*), const double* x, uint32_t n, double h, double* out);

// Numerical Jacobian of f: Rⁿ → Rᵐ using central differences.
// J[i*n + j] = ∂fᵢ/∂xⱼ  (row-major layout, m rows × n columns)
// Stack-allocates two scratch buffers: x_perturb[8] and f_plus[8] / f_minus[8].
// Total stack: 3 × 8 × 8 = 192 bytes  (well within typical stack limits).
// Returns false (no output written) if:
//   - any pointer is NULL
//   - n == 0 || n > 8 || m == 0 || m > 8  (stack buffer limits)
// h defaults to 1e-5 when passed as 0.0.
bool calc_jacobian(CalculatorState* state, void (*f)(const double*, double*), const double* x, uint32_t n, double* out_f, uint32_t m, double h, double* J);

#endif // CALCULUS_H
