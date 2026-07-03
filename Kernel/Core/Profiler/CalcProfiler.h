/*
 * File: CalcProfiler.h
 * Author: W. Kowal
 * Description: Error recovery boundary (setjmp/longjmp) and RDTSC-based
 * expression profiling. ALL IN C, NO C++ EXCEPTIONS, NO LIBRARY DEPENDENCIES.
 * 
 * setjmp/longjmp is part of the C standard library (<setjmp.h>).
 * RDTSC is an x86 instruction (__rdtsc() on MSVC, rdtsc inline asm on GCC).
 * 
 * THE FLEX: Python's profiler adds 100s overhead. Ours is ~24 cycles.
 */

#ifndef CALC_PROFILER_H
#define CALC_PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>

// ============================================================
// ERROR RECOVERY (setjmp/longjmp)
// ============================================================

// Error codes
typedef enum {
    CALC_ERROR_NONE = 0,
    CALC_ERROR_DIVISION_BY_ZERO,
    CALC_ERROR_OVERFLOW,
    CALC_ERROR_SYNTAX,
    CALC_ERROR_DOMAIN,        // e.g., sqrt(-1) in real mode
    CALC_ERROR_SINGULARITY,   // e.g., tan(pi/2)
    CALC_ERROR_OUT_OF_MEMORY, // Stack allocation cap hit
    CALC_ERROR_INVALID_INPUT,
    CALC_ERROR_UNKNOWN,
    CALC_ERROR_COUNT
} CalcError;

// Error message table (points to string literals zero allocation)
extern const char* const CALC_ERROR_MESSAGES[CALC_ERROR_COUNT];

// The jump buffer for error recovery global for the calculator instance
// (Since has a single static calculator state, this is fine.)
extern jmp_buf g_calc_err_jmp;

// Returns the last error code
CalcError calc_get_last_error(void);

// Sets the last error code
void calc_set_error(CalcError err);

// ============================================================
// PROFILING (RDTSC)
// ============================================================

// A timestamp in CPU cycles (64-bit, wraps every ~500 years at 4GHz)
typedef uint64_t ProfileTimestamp;

// Read the CPU's timestamp counter (RDTSC).
// Returns the number of CPU cycles since reset.
// On modern CPUs: invariant TSC, so this is wall-clock time in cycles.
static inline ProfileTimestamp profile_read_tsc(void) {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    return __rdtsc();
#elif defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | (uint64_t)lo;
#else
    // Fallback: return 0 on non-x86 platforms
    return 0;
#endif
}

// A profile measurement result
typedef struct {
    const char* expression;  // The expression that was profiled
    uint64_t    cycles;      // CPU cycles taken
    double      seconds;     // Approximate seconds (uses CPU frequency estimate)
    double      result;      // The result (NaN if error)
    CalcError   error;       // Error code
} ProfileResult;

// Estimate CPU frequency by calibrating against a known delay loop.
// Returns MHz (e.g., 2400 for 2.4 GHz).
// Only needs to be called once cached internally.
uint32_t profile_estimate_cpu_mhz(void);

// Profile an expression: evaluate it and time it.
// Uses an error recovery boundary so crashes in user functions don't take down the system.
// 'expr' is a C function pointer (double (*)(double)) with optional argument.
// Returns a ProfileResult struct with timing info.
ProfileResult profile_expression(double (*func)(void*), void* arg);

// Format a profile result as a string (write into buffer).
// Example output: "sin(3.14) = 0.00159265 [1234 cycles | 0.51 s]"
void profile_format_result(const ProfileResult* pr, char* buffer, uint32_t buffer_size);

#endif // CALC_PROFILER_H
