#include "Calculus.h"
#include <stdbool.h>

// bit-punch absolute value. no branch, no libm fabs().
// clears IEEE 754 sign bit. works for all finite values including -0.0.
static inline double absolute_val(double v) {
    union {
        double d;
        uint64_t i;
    } u;
    u.d = v;
    u.i &= 0x7FFFFFFFFFFFFFFFULL;
    return u.d;
}

// central difference derivative: (f(x+h) - f(x-h)) / 2h
// h defaults to 1e-5 if caller passes 0. that's near-optimal for doubles:
// too small and cancellation error dominates, too large and truncation error dominates.
// if 2h flushes to 0 (denormal underflow), flag it and return NaN.
double derivative(CalculatorState* state, double (*f)(double), double x, double h) {
    if (h == 0.0) {
        h = 1e-5;
    }
    if (2.0 * h == 0.0) {
        // h is so small that 2h underflows to zero. can't divide.
        if (state) state->flags |= 2;
        return calc_nan();
    }
    double f_plus = f(x + h);
    double f_minus = f(x - h);
    return (f_plus - f_minus) / (2.0 * h);
}

// Simpson's rule integration. O(h^4) error vs O(h^2) for trapezoidal.
// steps must be even -- Simpson requires pairs. force-even via bitmask.
// branchless even/odd factor: 2 + 2*(i&1) gives 2 for even i, 4 for odd i.
double integral(CalculatorState* state, double (*f)(double), double a, double b, uint32_t steps) {
    (void)state;
    if (steps == 0) {
        steps = 1000;
    }
    // force even step count. Simpson's rule requires it.
    steps = (steps + 1) & ~1U;

    double h = (b - a) / (double)steps;
    double sum = f(a) + f(b); // endpoints get weight 1

    for (uint32_t i = 1; i < steps; i++) {
        double x = a + (double)i * h;
        // even index: weight 2. odd index: weight 4. branchless.
        double factor = 2.0 + 2.0 * (double)(i & 1);
        sum += factor * f(x);
    }

    return sum * (h / 3.0);
}

// Newton-Raphson root finder. converges quadratically near the root.
// if df is NULL, uses numerical derivative (central difference, h=1e-5).
// singularity guard: dy near zero means the step (y/dy) would be huge.
//   dy == 0.0 exactly: return 0.0 (can't step at all)
//   dy tiny but nonzero: return current x as best guess (step would overshoot)
double newton_raphson(CalculatorState* state, double (*f)(double), double (*df)(double), double x0, uint32_t max_iter, double tol) {
    if (max_iter == 0) {
        max_iter = 100;
    }
    if (tol == 0.0) {
        tol = 1e-9;
    }

    double x = x0;
    for (uint32_t iter = 0; iter < max_iter; iter++) {
        double y = f(x);
        if (absolute_val(y) < tol) {
            return x; // converged
        }

        double dy;
        if (df) {
            dy = df(x);
        } else {
            dy = derivative(state, f, x, 0.0); // numerical fallback
        }

        if (absolute_val(dy) < 1e-15) {
            if (state) state->flags |= 2; // singularity / near-zero derivative
            return (dy == 0.0) ? 0.0 : x;
        }

        x = x - y / dy;
    }
    return x; // max iterations hit, return best estimate
}

// bisection root finder. slower than Newton but guaranteed to converge if
// f(a) and f(b) have opposite signs (intermediate value theorem).
// requires sign change in [a,b]. if both sides same sign: NaN, set domain flag.
double bisection(CalculatorState* state, double (*f)(double), double a, double b, uint32_t max_iter, double tol) {
    if (max_iter == 0) {
        max_iter = 100;
    }
    if (tol == 0.0) {
        tol = 1e-9;
    }

    double ya = f(a);
    double yb = f(b);

    // same sign on both sides: no guaranteed root in this interval
    if ((ya > 0.0 && yb > 0.0) || (ya < 0.0 && yb < 0.0)) {
        if (state) state->flags |= 4; // domain error
        union { uint64_t i; double d; } u;
        u.i = 0x7FF8000000000000ULL; // quiet NaN
        return u.d;
    }

    double mid = a;
    for (uint32_t iter = 0; iter < max_iter; iter++) {
        mid = a + (b - a) * 0.5; // avoids (a+b)/2 overflow for large values
        double ymid = f(mid);

        if (absolute_val(ymid) < tol || (b - a) * 0.5 < tol) {
            return mid; // converged
        }

        // narrow the bracket: if ymid same sign as ya, root is in [mid, b]
        bool same_sign = (ymid > 0.0 && ya > 0.0) || (ymid < 0.0 && ya < 0.0);
        if (same_sign) {
            a = mid;
            ya = ymid;
        } else {
            b = mid;
        }
    }
    return mid;
}