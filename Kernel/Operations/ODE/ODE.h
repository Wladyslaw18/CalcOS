/*
 * File: ODE.h
 * Author: W. Kowal
 * Description: Runge-Kutta 4 (RK4) ODE solver declarations.
 */

#ifndef ODE_H
#define ODE_H

#include <stdint.h>
#include "../../State/CalculatorState.h"

/*
 * Solve a first-order scalar ODE dy/dt = f(t, y) using RK4.
 * Writes the trajectory (steps + 1 states, starting with y0 at index 0) into out.
 */
void calc_ode_rk4(double (*f)(double, double), double t0, double y0, double dt, uint32_t steps, double* out);

/*
 * Solve a system of first-order ODEs dy/dt = f(t, y) using RK4.
 * num_eqs is the number of equations (max 8, zero-heap stack allocation).
 * If num_eqs > 8: state->flags |= 1 (overflow) is set and the system is truncated.
 *   The caller MUST check state->flags & 1 after the call to detect truncation.
 * Writes the trajectory ((steps + 1) * min(num_eqs,8) states) into out.
 * state may be NULL (truncation detection will be silently skipped).
 */
void calc_ode_rk4_system(void (*f)(double, const double*, double*), double t0, const double* y0, double dt, uint32_t steps, uint32_t num_eqs, double* out, CalculatorState* state);

/*
 * Solve a scalar ODE dy/dt = f(t,y) using adaptive RK45 (Dormand-Prince).
 *
 * Unlike fixed-step RK4, this solver automatically adjusts the step size
 * to keep the local error below `tol`. It will not diverge on stiff systems.
 *
 * Parameters:
 *   f          : dy/dt = f(t, y)
 *   t0, y0    : initial conditions
 *   t_end     : end time (must be > t0)
 *   dt_init   : initial step size (0 = auto: (t_end-t0)/100)
 *   tol       : error tolerance per step (0 = default 1e-6)
 *   out_t     : output array for accepted time points [max_steps]
 *   out_y     : output array for accepted y values    [max_steps]
 *   max_steps : maximum accepted steps (0 = 4096)
 *   state     : CalculatorState for error flags (NULL = ignore)
 *               flags |= 4 is set if step size hits the dt_min floor
 *
 * Returns: number of accepted steps written to out_t/out_y.
 */
uint32_t calc_ode_rk45(
    double (*f)(double, double),
    double t0, double y0,
    double t_end, double dt_init,
    double tol,
    double* out_t, double* out_y,
    uint32_t max_steps,
    CalculatorState* state);

#endif // ODE_H
