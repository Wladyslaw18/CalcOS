/*
 * ProfilerAPI.h — Stable proxy interface for the RDTSC expression profiler
 *
 * Wraps CalcProfiler.h behind the Kernel service locator so any module can
 * profile an expression via kernel_get(K, KRN_PROFILER) without a direct
 * include of CalcProfiler.h.
 *
 * Null Object: ker_profiler_null — returns a zero-cycle ProfileResult with
 * CALC_ERROR_NONE so callers can safely invoke profile() unconditionally.
 */

#ifndef PROFILER_API_H
#define PROFILER_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mirrors CalcProfiler.h CalcError without pulling in the full header */
typedef enum {
    PROFILER_API_ERROR_NONE = 0,
    PROFILER_API_ERROR_CRASH,
    PROFILER_API_ERROR_UNKNOWN
} ProfilerAPIError;

/* Compact profile result — 40 bytes, L1 friendly */
typedef struct {
    const char*      expression; /* The expression that was profiled */
    uint64_t         cycles;     /* CPU cycles elapsed (RDTSC delta) */
    double           seconds;    /* Approximate wall time */
    double           result;     /* Computed value (NaN on error) */
    ProfilerAPIError error;      /* Error code */
} ProfilerAPIResult;

/* ─── Profiler Service Interface ─── */
typedef struct {
    /*
     * profile: Time and evaluate a function call.
     * func(arg) is called once inside an error-recovery boundary.
     * Returns a ProfilerAPIResult with cycle count and result.
     */
    ProfilerAPIResult (*profile)(double (*func)(void*), void* arg);

    /*
     * format: Write a human-readable result string into buffer.
     * Example: "expr = 1.4142 [2048 cycles | 0.00 μs]"
     */
    void (*format)(const ProfilerAPIResult* r, char* buf, uint32_t buf_size);

    /*
     * cpu_mhz: Estimate or return cached CPU frequency in MHz.
     * Used to convert cycles → seconds.
     */
    uint32_t (*cpu_mhz)(void);

} ProfilerAPI;

/* ─── Null Object ─── */
extern const ProfilerAPI ker_profiler_null;

#ifdef __cplusplus
}
#endif

#endif /* PROFILER_API_H */
