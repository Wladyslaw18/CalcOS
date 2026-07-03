/*
 * File: CalcProfiler.c
 * Author: W. Kowal
 * Description: Error recovery boundary and RDTSC profiler implementation.
 * 
 * ZERO HEAP ALLOCATIONS. Everything is stack-based.
 * The global jmp_buf is part of the static calculator state no dynamic allocation.
 */

#include "CalcProfiler.h"
#include "Infrastructure/Utils/MemoryUtils.h"
#include "Application/Output/Formatter.h"
#include "Kernel/State/CalculatorState.h"

// ============================================================
// GLOBALS
// ============================================================

// Error recovery jump buffer
jmp_buf g_calc_err_jmp;

// Last error code
static CalcError g_last_error = CALC_ERROR_NONE;

// Cached CPU frequency (MHz)
static uint32_t g_cpu_mhz_cached = 0;

// Error message table ZERO ALLOCATION, all string literals
const char* const CALC_ERROR_MESSAGES[CALC_ERROR_COUNT] = {
    [CALC_ERROR_NONE]           = "No error",
    [CALC_ERROR_DIVISION_BY_ZERO] = "Division by zero",
    [CALC_ERROR_OVERFLOW]       = "Numeric overflow",
    [CALC_ERROR_SYNTAX]         = "Syntax error in expression",
    [CALC_ERROR_DOMAIN]         = "Domain error (e.g., sqrt of negative)",
    [CALC_ERROR_SINGULARITY]    = "Mathematical singularity",
    [CALC_ERROR_OUT_OF_MEMORY]  = "Out of memory (stack limit hit)",
    [CALC_ERROR_INVALID_INPUT]  = "Invalid input",
    [CALC_ERROR_UNKNOWN]        = "Unknown error"
};

// ============================================================
// ERROR API
// ============================================================

CalcError calc_get_last_error(void) {
    return g_last_error;
}

void calc_set_error(CalcError err) {
    g_last_error = err;
}

// ============================================================
// CPU FREQUENCY ESTIMATION
// ============================================================

uint32_t profile_estimate_cpu_mhz(void) {
    if (g_cpu_mhz_cached != 0) return g_cpu_mhz_cached;

    // Default to 2.4 GHz. Proper calibration would need HPET/ACPI platform access
    // overkill when the profiler overhead is only ~24 cycles anyway.
    g_cpu_mhz_cached = 2400;
    return g_cpu_mhz_cached;
}

// ============================================================
// EXPRESSION PROFILING
// ============================================================

ProfileResult profile_expression(double (*func)(void*), void* arg) {
    ProfileResult result;
    fast_memset(&result, 0, sizeof(result));
    result.error = CALC_ERROR_NONE;

    if (!func) {
        result.error = CALC_ERROR_INVALID_INPUT;
        result.result = calc_nan();
        return result;
    }

    // Set up error recovery boundary
    CalcError saved_error = g_last_error;

    // Read TSC before evaluation
    ProfileTimestamp t0 = profile_read_tsc();

    // Setjmp: establish recovery point
    int jmp_val = setjmp(g_calc_err_jmp);

    if (jmp_val == 0) {
        // Normal execution path
        result.result = func(arg);

        // Read TSC after evaluation
        ProfileTimestamp t1 = profile_read_tsc();

        result.cycles = t1 - t0;

        // Get the error code (function may have set it)
        result.error = g_last_error;

        // If no cycles recorded (fallback platform), set to 0
        if (t0 == 0 && t1 == 0) {
            result.cycles = 0;
        }
    } else {
        // Longjmp recovery path: an error occurred during evaluation
        ProfileTimestamp t1 = profile_read_tsc();
        result.cycles = t1 - t0;
        result.result = calc_nan();
        result.error = (CalcError)jmp_val;
    }

    // Calculate approximate seconds
    uint32_t mhz = profile_estimate_cpu_mhz();
    if (mhz > 0) {
        // cycles / (MHz * 10^6) = seconds
        result.seconds = (double)result.cycles / ((double)mhz * 1000000.0);
    } else {
        result.seconds = 0.0;
    }

    return result;
}

// ============================================================
// FORMATTING
// ============================================================

void profile_format_result(const ProfileResult* pr, char* buffer, uint32_t buffer_size) {
    if (!pr || !buffer || buffer_size == 0) return;

    if (pr->error != CALC_ERROR_NONE) {
        format_string(buffer, buffer_size, "[ERROR] %s after %llu cycles",
                 CALC_ERROR_MESSAGES[pr->error],
                 (unsigned long long)pr->cycles);
        return;
    }

    // Choose the best time unit
    double seconds = pr->seconds;
    const char* unit = "s";
    double display_time = seconds;

    if (seconds < 1e-9) {
        display_time = seconds * 1e12;
        unit = "ps";
    } else if (seconds < 1e-6) {
        display_time = seconds * 1e9;
        unit = "ns";
    } else if (seconds < 1e-3) {
        display_time = seconds * 1e6;
        unit = "μs";
    } else if (seconds < 1.0) {
        display_time = seconds * 1e3;
        unit = "ms";
    }

    format_string(buffer, buffer_size, "%.15g [%llu cycles | %.3f %s]",
             pr->result,
             (unsigned long long)pr->cycles,
             display_time,
             unit);
}
