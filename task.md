# CalcOS Expansion Task List

## Infrastructure (ChaoticJS — Direct)
- [x] CMakeLists.txt if(MSVC) bug fix
- [x] Kernel.h stable null-proxy upgrade
- [x] Kernel.c g_kernel_null_proxies table
- [/] Infrastructure/Utils/MathUtils.h — shared custom_sqrt, custom_atan2, calc_nan
- [/] Kernel.h — uint32_t → uint64_t bitmask upgrade
- [/] Kernel.h — Add KRN_PROFILER enum slot
- [/] Kernel/API/ProfilerAPI.h — new Profiler service API
- [/] Kernel.c — ker_profiler_null + g_kernel_null_proxies update
- [/] Kernel.c — kernel_log() fix (route through KRN_LOGGER when live)
- [/] ODE.c — silent truncation → set error flag on state

## Agent 1 — NumberTheory expansion
- [ ] euler_totient(n)
- [ ] extended_gcd(a, b, x, y)
- [ ] mod_inverse(a, m)
- [ ] chinese_remainder(r[], m[], count, result)
- [ ] next_prime(n)

## Agent 2 — RNG expansion
- [ ] calc_random_poisson(lambda)
- [ ] calc_random_gamma(shape, scale)
- [ ] calc_random_bernoulli(p)
- [ ] calc_random_get_state() / calc_random_set_state()
- [ ] Ziggurat normal (replace Box-Muller)

## Agent 3 — ComplexOps expansion
- [ ] complex_exp(z)
- [ ] complex_log(z)
- [ ] complex_pow(base, exp)
- [ ] complex_sqrt(z)

## Agent 4 — Statistics expansion
- [ ] calc_percentile(arr, len, p)
- [ ] calc_skewness(arr, len)
- [ ] calc_kurtosis(arr, len)
- [ ] calc_linear_regression(x, y, n, slope, intercept)

## Agent 5 — Units expansion
- [ ] Time (s, ms, μs, min, hr, day)
- [ ] Speed (m/s, km/h, mph, knots)
- [ ] Energy (J, kJ, cal, kcal, eV)
- [ ] Force (N, kN, lbf)
- [ ] Pressure (Pa, kPa, bar, atm, psi)
- [ ] Data (bit, byte, KB, MB, GB, TB)
- [ ] Angle (rad, deg, grad)
- [ ] Power (W, kW, hp)

## Agent 6 — Calculus/ODE expansion
- [ ] gradient(f, x[], n, h, out[])
- [ ] jacobian(f[], m, x[], n, h, out[m*n])
- [ ] ODE_RK45 adaptive step (Dormand-Prince)

## Agent 7 — LinearAlgebra eigenvalues
- [ ] calc_matrix_eigenvalues(A, dim, eigenvals[])
- [ ] calc_matrix_power_iteration(A, dim, out_vec[], out_val)
