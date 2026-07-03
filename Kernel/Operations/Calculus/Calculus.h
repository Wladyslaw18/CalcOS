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

#endif // CALCULUS_H
