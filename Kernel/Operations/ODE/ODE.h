/*
 * File: ODE.h
 * Author: W. Kowal
 * Description: Runge-Kutta 4 (RK4) ODE solver declarations.
 */

#ifndef ODE_H
#define ODE_H

#include <stdint.h>

/*
 * Solve a first-order scalar ODE dy/dt = f(t, y) using RK4.
 * Writes the trajectory (steps + 1 states, starting with y0 at index 0) into out.
 */
void calc_ode_rk4(double (*f)(double, double), double t0, double y0, double dt, uint32_t steps, double* out);

/*
 * Solve a system of first-order ODEs dy/dt = f(t, y) using RK4.
 * num_eqs is the number of equations (up to 8, zero-heap allocation).
 * Writes the trajectory ((steps + 1) * num_eqs states, starting with y0) into out.
 */
void calc_ode_rk4_system(void (*f)(double, const double*, double*), double t0, const double* y0, double dt, uint32_t steps, uint32_t num_eqs, double* out);

#endif // ODE_H
