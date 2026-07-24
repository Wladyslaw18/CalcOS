/*
 * StackAPI.h — Stable proxy interface for the operand stack
 *
 * Null Object: ker_stack_null — always returns 0 / failure
 */

#ifndef STACK_API_H
#define STACK_API_H

#include "../../Kernel/State/CalculatorState.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Operand Stack Service Interface ─── */
typedef struct {
    /* Push a value onto the stack. Returns 1 if successful, 0 if full. */
    int (*push)(CalculatorState* state, double val);

    /* Pop a value from the stack. Returns 1 if successful, 0 if empty. */
    int (*pop)(CalculatorState* state, double* out_val);

    /* Peek at the top value. Returns 1 if successful, 0 if empty. */
    int (*peek)(const CalculatorState* state, double* out_val);

    /* Clear all values from the stack */
    void (*clear)(CalculatorState* state);

} StackAPI;

/* ─── Null Object ─── */
extern const StackAPI ker_stack_null;

/* ─── Adapter Instance ─── */
extern const StackAPI g_stack_service;

#ifdef __cplusplus
}
#endif

#endif /* STACK_API_H */
