#include "calc.h"
#include "Infrastructure/Utils/MemoryUtils.h"

// 64-byte aligned. one cache line. fits the whole OS state. not negotiable.
static CACHE_ALIGN CalculatorState g_calc_state;
static CACHE_ALIGN CPUFeatures g_cpu_features;
static bool g_initialized = false;

void calc_init(void) {
    if (!g_initialized) {
        fast_memset(&g_calc_state, 0, sizeof(CalculatorState));
        cpu_detect_features(&g_cpu_features);
        g_initialized = true;
    }
}

CalculatorState* calc_get_state(void) {
    calc_init();
    return &g_calc_state;
}

// dispatches to AVX2/SSE2/scalar based on what the CPU actually supports
void calc_add(const double* a, const double* b, double* result, uint32_t count) {
    calc_init();
    execute_addition(&g_calc_state, a, b, result, count, &g_cpu_features);
}

// complex ops — thin wrappers, keep the kernel isolated from callers
ComplexValue calc_complex_add(ComplexValue a, ComplexValue b) {
    return complex_add(a, b);
}

ComplexValue calc_complex_sub(ComplexValue a, ComplexValue b) {
    return complex_sub(a, b);
}

ComplexValue calc_complex_mul(ComplexValue a, ComplexValue b) {
    return complex_mul(a, b);
}

ComplexValue calc_complex_div(ComplexValue a, ComplexValue b) {
    return complex_div(a, b);
}

ComplexValue calc_complex_conjugate(ComplexValue a) {
    return complex_conjugate(a);
}

double calc_complex_modulus(ComplexValue a) {
    return complex_modulus(a);
}

void calc_complex_polar(ComplexValue a, double* r, double* theta) {
    complex_polar(a, r, theta);
}

// rational ops — same deal, just forwarding to the kernel
RationalValue calc_rational_add(RationalValue a, RationalValue b) {
    return rational_add(a, b);
}

RationalValue calc_rational_sub(RationalValue a, RationalValue b) {
    return rational_sub(a, b);
}

RationalValue calc_rational_mul(RationalValue a, RationalValue b) {
    return rational_mul(a, b);
}

RationalValue calc_rational_div(RationalValue a, RationalValue b) {
    return rational_div(a, b);
}

void calc_rational_simplify(RationalValue* r) {
    rational_simplify(r);
}

// linear algebra — state pointer required, matrix ops need scratch space in it
bool calc_matrix_add(const double* A, uint32_t a_rows, uint32_t a_cols,
                     const double* B, uint32_t b_rows, uint32_t b_cols,
                     double* C) {
    calc_init();
    return matrix_add(&g_calc_state, A, a_rows, a_cols, B, b_rows, b_cols, C);
}

bool calc_matrix_mul(const double* A, uint32_t a_rows, uint32_t a_cols,
                     const double* B, uint32_t b_rows, uint32_t b_cols,
                     double* C) {
    calc_init();
    return matrix_mul(&g_calc_state, A, a_rows, a_cols, B, b_rows, b_cols, C);
}

bool calc_matrix_transpose(const double* A, uint32_t rows, uint32_t cols, double* C) {
    calc_init();
    return matrix_transpose(&g_calc_state, A, rows, cols, C);
}

bool calc_matrix_determinant(const double* A, uint32_t dim, double* result) {
    calc_init();
    return matrix_determinant(&g_calc_state, A, dim, result);
}

bool calc_matrix_inverse(const double* A, uint32_t dim, double* C) {
    calc_init();
    return matrix_inverse(&g_calc_state, A, dim, C);
}

bool calc_matrix_rref(const double* A, uint32_t rows, uint32_t cols, double* C) {
    calc_init();
    return matrix_rref(&g_calc_state, A, rows, cols, C);
}

// calculus — numerical methods, no libm, no mercy
double calc_derivative(double (*f)(double), double x, double h) {
    calc_init();
    return derivative(&g_calc_state, f, x, h);
}

double calc_integral(double (*f)(double), double a, double b, uint32_t steps) {
    calc_init();
    return integral(&g_calc_state, f, a, b, steps);
}

double calc_newton_raphson(double (*f)(double), double (*df)(double), double x0, uint32_t max_iter, double tol) {
    calc_init();
    return newton_raphson(&g_calc_state, f, df, x0, max_iter, tol);
}

double calc_bisection(double (*f)(double), double a, double b, uint32_t max_iter, double tol) {
    calc_init();
    return bisection(&g_calc_state, f, a, b, max_iter, tol);
}

// number theory — GCD, LCM, modular exp, Miller-Rabin. stateless, no init needed
uint64_t calc_gcd(uint64_t a, uint64_t b) {
    return gcd(a, b);
}

uint64_t calc_lcm(uint64_t a, uint64_t b) {
    return lcm(a, b);
}

uint64_t calc_mod_exp(uint64_t base, uint64_t exp, uint64_t mod) {
    return mod_exp(base, exp, mod);
}

bool calc_is_prime(uint64_t n) {
    return is_prime(n);
}
