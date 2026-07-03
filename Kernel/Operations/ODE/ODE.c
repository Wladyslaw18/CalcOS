/*
 * File: ODE.c
 * Author: W. Kowal
 * Description: Runge-Kutta 4 (RK4) ODE solver implementation.
 */

#include "ODE.h"

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

void calc_ode_rk4_system(void (*f)(double, const double*, double*), double t0, const double* y0, double dt, uint32_t steps, uint32_t num_eqs, double* out) {
    if (!out || num_eqs == 0 || !y0) {
        return;
    }

    // Support stack-allocated up to 8 equations, zero-heap
    double y[8];
    double k1[8];
    double k2[8];
    double k3[8];
    double k4[8];
    double temp[8];

    uint32_t n = num_eqs;
    if (n > 8) {
        n = 8;
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
