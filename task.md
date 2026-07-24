# CalcOS Expansion Task List

## Infrastructure (ChaoticJS — Direct)
- [x] CMakeLists.txt if(MSVC) bug fix
- [x] Kernel.h stable null-proxy upgrade
- [x] Kernel.c g_kernel_null_proxies table
- [x] Infrastructure/Utils/MathUtils.h — shared custom_sqrt, custom_atan2, calc_nan
- [x] Kernel.h — uint32_t → uint64_t bitmask upgrade
- [x] Kernel.h — Add KRN_PROFILER enum slot
- [x] Kernel/API/ProfilerAPI.h — new Profiler service API
- [x] Kernel.c — ker_profiler_null + g_kernel_null_proxies update
- [x] Kernel.c — kernel_log() fix (route through KRN_LOGGER when live)
- [x] ODE.c — silent truncation → set error flag on state

## Agent 1 — NumberTheory expansion
- [x] euler_totient(n)
- [x] extended_gcd(a, b, x, y)
- [x] mod_inverse(a, m)
- [x] chinese_remainder(r[], m[], count, result)
- [x] next_prime(n)

## Agent 2 — RNG expansion
- [x] calc_random_poisson(lambda)
- [x] calc_random_gamma(shape, scale)
- [x] calc_random_bernoulli(p)
- [x] calc_random_get_state() / calc_random_set_state()
- [x] Box-Muller with cached 2nd sample & xoshiro256++ core

## Agent 3 — ComplexOps expansion
- [x] complex_exp(z)
- [x] complex_log(z)
- [x] complex_pow(base, exp)
- [x] complex_sqrt(z)

## Agent 4 — Statistics expansion
- [x] calc_percentile(arr, len, p)
- [x] calc_skewness(arr, len)
- [x] calc_kurtosis(arr, len)
- [x] calc_linear_regression(x, y, n, slope, intercept)

## Agent 5 — Units expansion
- [x] Time (s, ms, μs, min, hr, day)
- [x] Speed (m/s, km/h, mph, knots)
- [x] Energy (J, kJ, cal, kcal, eV)
- [x] Force (N, kN, lbf)
- [x] Pressure (Pa, kPa, bar, atm, psi)
- [x] Data (bit, byte, KB, MB, GB, TB)
- [x] Angle (rad, deg, grad)
- [x] Power (W, kW, hp)

## Agent 6 — Calculus/ODE expansion
- [x] gradient(f, x[], n, h, out[])
- [x] jacobian(f[], m, x[], n, h, out[m*n])
- [x] ODE_RK45 adaptive step (Dormand-Prince)

## Agent 7 — LinearAlgebra eigenvalues
- [x] calc_matrix_eigenvalues(A, dim, eigenvals[])
- [x] calc_matrix_power_iteration(A, dim, out_vec[], out_val)
