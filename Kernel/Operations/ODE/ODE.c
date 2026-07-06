/*
 * File: ODE.c
 * Author: W. Kowal
 * Description: Runge-Kutta 4 (RK4) and RK45 (Dormand-Prince) ODE solvers.
 */

#include "ODE.h"
#include "../../../Infrastructure/Utils/MathUtils.h"

void calc_ode_rk4(double (*f)(double, double), double t0, double y0, double dt, uint32_t steps, double* out) {
    if (!out) {
        return;
    }

    double t = t0;
    double y = y0;
    out[0] = y;

    for (uint32_t i = 1; i <= steps; ++i) {
        double k1 = f(t, y);
        double k2 = f(t + 0.5 * dt, y + 0.5 * dt * k1);
        double k3 = f(t + 0.5 * dt, y + 0.5 * dt * k2);
        double k4 = f(t + dt, y + dt * k3);

        y += (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
        t += dt;
        out[i] = y;
    }
}

void calc_ode_rk4_system(void (*f)(double, const double*, double*), double t0, const double* y0, double dt, uint32_t steps, uint32_t num_eqs, double* out, CalculatorState* state) {
    if (!out || num_eqs == 0 || !y0) {
        return;
    }

    /* Support stack-allocated up to 8 equations, zero-heap.
     * If the caller passes more than 8, set the overflow flag and cap.
     * The caller MUST check state->flags & 1 to detect truncation. */
    double y[8];
    double k1[8];
    double k2[8];
    double k3[8];
    double k4[8];
    double temp[8];

    uint32_t n = num_eqs;
    if (n > 8) {
        if (state) state->flags |= 1; /* overflow: system truncated to 8 equations */
        n = 8; /* hard cap: still run with truncated system rather than crashing */
    }

    // Initialize state and write to output at step 0
    for (uint32_t j = 0; j < n; ++j) {
        y[j] = y0[j];
        out[j] = y[j];
    }

    double t = t0;

    for (uint32_t i = 1; i <= steps; ++i) {
        // k1 = f(t, y)
        f(t, y, k1);

        // k2 = f(t + 0.5 * dt, y + 0.5 * dt * k1)
        for (uint32_t j = 0; j < n; ++j) {
            temp[j] = y[j] + 0.5 * dt * k1[j];
        }
        f(t + 0.5 * dt, temp, k2);

        // k3 = f(t + 0.5 * dt, y + 0.5 * dt * k2)
        for (uint32_t j = 0; j < n; ++j) {
            temp[j] = y[j] + 0.5 * dt * k2[j];
        }
        f(t + 0.5 * dt, temp, k3);

        // k4 = f(t + dt, y + dt * k3)
        for (uint32_t j = 0; j < n; ++j) {
            temp[j] = y[j] + dt * k3[j];
        }
        f(t + dt, temp, k4);

        // y_new = y + (dt / 6.0) * (k1 + 2*k2 + 2*k3 + k4)
        for (uint32_t j = 0; j < n; ++j) {
            y[j] += (dt / 6.0) * (k1[j] + 2.0 * k2[j] + 2.0 * k3[j] + k4[j]);
            out[i * n + j] = y[j];
        }
        t += dt;
    }
}

/*
 * calc_ode_rk45: Adaptive step-size RK45 (Dormand-Prince) scalar ODE solver.
 *
 * Solves dy/dt = f(t, y) with automatic step-size control.
 * Uses the Dormand-Prince Butcher tableau to compute a 4th-order solution
 * (y4) and a 5th-order solution (y5) from 6 function evaluations.
 * The error |y5 - y4| drives step-size adaptation:
 *   error > tol  → reject step, shrink dt by factor 0.9*(tol/err)^0.2
 *   error < tol  → accept step, grow dt by factor 0.9*(tol/err)^0.2 (capped at 5x)
 *
 * Output: writes accepted steps into out[] until t reaches t_end or max_steps used.
 * Returns the number of accepted steps written.
 *
 * Convergence guarantee: unlike fixed-step RK4, RK45 will not diverge on
 * stiff systems because it will shrink dt until the error is acceptable.
 * The minimum dt guard (dt_min = 1e-10) prevents infinite loops on
 * extremely stiff systems.
 */
uint32_t calc_ode_rk45(
    double (*f)(double, double),
    double t0, double y0,
    double t_end, double dt_init,
    double tol,
    double* out_t, double* out_y,
    uint32_t max_steps,
    CalculatorState* state)
{
    if (!out_t || !out_y || !f) return 0;
    if (max_steps == 0) max_steps = 4096;
    if (tol      == 0.0) tol      = 1e-6;
    if (dt_init  == 0.0) dt_init  = (t_end - t0) / 100.0;

    /* Dormand-Prince Butcher tableau coefficients */
    const double c2  = 1.0/5.0,   a21 = 1.0/5.0;
    const double c3  = 3.0/10.0,  a31 = 3.0/40.0,       a32 = 9.0/40.0;
    const double c4  = 4.0/5.0,   a41 = 44.0/45.0,      a42 = -56.0/15.0,    a43 = 32.0/9.0;
    const double c5  = 8.0/9.0,   a51 = 19372.0/6561.0, a52 = -25360.0/2187.0,
                                   a53 = 64448.0/6561.0, a54 = -212.0/729.0;
    const double     /* c6=1 */    a61 = 9017.0/3168.0,  a62 = -355.0/33.0,
                                   a63 = 46732.0/5247.0, a64 = 49.0/176.0,    a65 = -5103.0/18656.0;
    /* 4th-order weights (for error estimation) */
    const double e1  =  71.0/57600.0,  e3 = -71.0/16695.0,   e4 =  71.0/1920.0;
    const double e5  = -17253.0/339200.0, e6 = 22.0/525.0;
    /* 5th-order weights (for the propagated solution) */
    const double b1  =  35.0/384.0, b3 = 500.0/1113.0, b4 = 125.0/192.0;
    const double b5  = -2187.0/6784.0, b6 = 11.0/84.0;

    const double dt_min = 1e-10;
    const double dt_max = math_abs(t_end - t0); /* never step past the interval */

    double t  = t0;
    double y  = y0;
    double dt = dt_init;

    uint32_t step = 0;
    out_t[step] = t;
    out_y[step] = y;
    step++;

    while (t < t_end && step < max_steps) {
        /* Clamp dt so we don't overshoot t_end */
        if (t + dt > t_end) dt = t_end - t;
        if (dt < dt_min) {
            /* dt hit the floor — stiff system or singularity. Set domain flag. */
            if (state) state->flags |= 4;
            break;
        }

        /* 6 stage evaluations (Dormand-Prince) */
        double k1 = f(t,                   y);
        double k2 = f(t + c2*dt,            y + dt*(a21*k1));
        double k3 = f(t + c3*dt,            y + dt*(a31*k1 + a32*k2));
        double k4 = f(t + c4*dt,            y + dt*(a41*k1 + a42*k2 + a43*k3));
        double k5 = f(t + c5*dt,            y + dt*(a51*k1 + a52*k2 + a53*k3 + a54*k4));
        double k6 = f(t +    dt,            y + dt*(a61*k1 + a62*k2 + a63*k3 + a64*k4 + a65*k5));

        /* 5th-order solution (used for propagation) */
        double y5 = y + dt*(b1*k1 + b3*k3 + b4*k4 + b5*k5 + b6*k6);

        /* Error estimate (difference between 4th and 5th order) */
        double err = math_abs(dt*(e1*k1 + e3*k3 + e4*k4 + e5*k5 + e6*k6));

        if (err <= tol) {
            /* Accept step */
            t += dt;
            y  = y5;
            out_t[step] = t;
            out_y[step] = y;
            step++;
        }
        /* Adapt dt: factor = 0.9 * (tol/err)^0.2
         * Approximation of x^0.2: use three Newton steps on x^5 = (tol/err). */
        if (err > 0.0) {
            double ratio = tol / err;
            /* Fast x^0.2 via x^(1/5): five-root via Newton on v^5 = ratio */
            double v = 1.0;
            v = (4.0*v + ratio/(v*v*v*v)) * 0.2;
            v = (4.0*v + ratio/(v*v*v*v)) * 0.2;
            v = (4.0*v + ratio/(v*v*v*v)) * 0.2;
            double factor = 0.9 * v;
            if (factor < 0.1) factor = 0.1;   /* shrink limit */
            if (factor > 5.0) factor = 5.0;   /* growth limit */
            dt *= factor;
        }
        if (dt > dt_max) dt = dt_max;
    }

    return step;
}
