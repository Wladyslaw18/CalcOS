/*
 * Speed benchmark - how fast is this overengineered beast?
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "Application/Input/Parser.h"
#include "Kernel/State/CalculatorState.h"
#include "Kernel/Operations/Arithmetic/Addition.h"
#include "Kernel/Operations/Arithmetic/Multiplication.h"
#include "Kernel/Operations/Arithmetic/Division.h"
#include "Kernel/Core/CPU/CPUID.h"

#define BENCH_MS(label, iters, code) do { \
    clock_t _t0 = clock(); \
    for (int _i = 0; _i < (iters); _i++) { code; } \
    clock_t _t1 = clock(); \
    double _ms = (double)(_t1 - _t0) * 1000.0 / CLOCKS_PER_SEC; \
    double _ops = (double)(iters) / (_ms / 1000.0); \
    printf("  %-42s %8.2f ms  %12.0f ops/sec\n", label, _ms, _ops); \
} while(0)

int main(void) {
    CalculatorState state;
    memset(&state, 0, sizeof(state));
    CPUFeatures feats;
    cpu_detect_features(&feats);

    printf("\n==============================================\n");
    printf("  SPEED BENCHMARK - OVERENGINEERED CALC\n");
    printf("==============================================\n");
    printf("  CPU feats: SSE=%d SSE2=%d AVX2=%d NEON=%d\n\n",
           feats.has_sse, feats.has_sse2, feats.has_avx2, feats.has_neon);

    // ---- [1] PARSER THROUGHPUT ----
    printf("[1] PARSER THROUGHPUT (parse_expression calls/sec)\n");
    bool ok = false;
    volatile double sink = 0.0;

    BENCH_MS("'2+3' simple add",           500000,
        sink += parse_expression("2+3", &state, &ok));

    BENCH_MS("'sin(pi/4)' trig call",      200000,
        sink += parse_expression("sin(pi/4)", &state, &ok));

    BENCH_MS("'(1+sqrt(5))/2' nested",     200000,
        sink += parse_expression("(1+sqrt(5))/2", &state, &ok));

    BENCH_MS("'2^32' pow",                 200000,
        sink += parse_expression("2^32", &state, &ok));

    BENCH_MS("'fact(10)' factorial",        200000,
        sink += parse_expression("fact(10)", &state, &ok));

    BENCH_MS("'solve(x^2-4,x,0,5)'",        20000,
        sink += parse_expression("solve(x^2-4, x, 0, 5)", &state, &ok));

    // ---- [2] RAW SIMD VECTOR MATH ----
    printf("\n[2] RAW SIMD VECTOR THROUGHPUT (4096 doubles/call)\n");
    #define N 4096
    static double va[N], vb[N], vr[N];
    for (int i = 0; i < N; i++) { va[i] = (double)i + 1.0; vb[i] = (double)i + 2.0; }

    BENCH_MS("add_scalar  (4096 doubles)",  10000,
        add_scalar(&state, va, vb, vr, N));

    if (feats.has_sse2) {
    BENCH_MS("add_sse     (4096 doubles)",  10000,
        add_sse(&state, va, vb, vr, N));
    }
    if (feats.has_avx2) {
    BENCH_MS("add_avx2    (4096 doubles)",  10000,
        add_avx2(&state, va, vb, vr, N));
    }

    BENCH_MS("mul_scalar  (4096 doubles)",  10000,
        mul_scalar(&state, va, vb, vr, N));

    if (feats.has_sse2) {
    BENCH_MS("mul_sse     (4096 doubles)",  10000,
        mul_sse(&state, va, vb, vr, N));
    }
    if (feats.has_avx2) {
    BENCH_MS("mul_avx2    (4096 doubles)",  10000,
        mul_avx2(&state, va, vb, vr, N));
    }

    BENCH_MS("div_scalar  (4096 doubles)",  10000,
        div_scalar(&state, va, vb, vr, N));

    if (feats.has_sse2) {
    BENCH_MS("div_sse     (4096 doubles)",  10000,
        div_sse(&state, va, vb, vr, N));
    }
    if (feats.has_avx2) {
    BENCH_MS("div_avx2    (4096 doubles)",  10000,
        div_avx2(&state, va, vb, vr, N));
    }

    // ---- [3] THROUGHPUT IN DOUBLES/SEC ----
    printf("\n[3] DOUBLES PROCESSED PER SECOND\n");
    {
        clock_t t0 = clock();
        int reps = 10000;
        for (int i = 0; i < reps; i++) add_avx2(&state, va, vb, vr, N);
        clock_t t1 = clock();
        double ms = (double)(t1 - t0) * 1000.0 / CLOCKS_PER_SEC;
        double doubles_per_sec = (double)reps * N / (ms / 1000.0);
        double gflops = doubles_per_sec / 1e9;
        printf("  add_avx2: %.2f BILLION doubles/sec (%.3f GFlop/s)\n", gflops, gflops);
    }
    {
        clock_t t0 = clock();
        int reps = 10000;
        for (int i = 0; i < reps; i++) mul_avx2(&state, va, vb, vr, N);
        clock_t t1 = clock();
        double ms = (double)(t1 - t0) * 1000.0 / CLOCKS_PER_SEC;
        double doubles_per_sec = (double)reps * N / (ms / 1000.0);
        double gflops = doubles_per_sec / 1e9;
        printf("  mul_avx2: %.2f BILLION doubles/sec (%.3f GFlop/s)\n", gflops, gflops);
    }

    (void)sink; // prevent dead-code elim
    printf("\n==============================================\n");
    printf("  DONE\n");
    printf("==============================================\n\n");
    return 0;
}
