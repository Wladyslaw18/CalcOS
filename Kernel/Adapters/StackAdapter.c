/*
 * StackAdapter.c — Bridges the existing Kernel/State/OperandStack.c
 * implementation to the Kernel StackAPI stable interface.
 */

#include "Kernel/API/StackAPI.h"
#include "Kernel/State/OperandStack.h"

/* ─── Push wrapper ─── */
static int adapter_push(CalculatorState* state, double val) {
    return stack_push(state, val);
}

/* ─── Pop wrapper ─── */
static int adapter_pop(CalculatorState* state, double* out_val) {
    return stack_pop(state, out_val);
}

/* ─── Peek wrapper ─── */
static int adapter_peek(const CalculatorState* state, double* out_val) {
    return stack_peek(state, out_val);
}

/* ─── Clear wrapper ─── */
static void adapter_clear(CalculatorState* state) {
    stack_clear(state);
}

/* ─── Public API struct ─── */
const StackAPI g_stack_service = {
    .push  = adapter_push,
    .pop   = adapter_pop,
    .peek  = adapter_peek,
    .clear = adapter_clear,
};
