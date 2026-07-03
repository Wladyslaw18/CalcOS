#include "OperandStack.h"

int stack_push(CalculatorState* state, double val) {
    if (!state) return 0;
    if (state->op_count >= 4) {
        state->flags |= 1; // Overflow flag
        return 0;
    }
    state->operands[state->op_count++] = val;
    return 1;
}

int stack_pop(CalculatorState* state, double* out_val) {
    if (!state || !out_val || state->op_count == 0) {
        return 0;
    }
    *out_val = state->operands[--state->op_count];
    return 1;
}

int stack_peek(const CalculatorState* state, double* out_val) {
    if (!state || !out_val || state->op_count == 0) {
        return 0;
    }
    *out_val = state->operands[state->op_count - 1];
    return 1;
}

void stack_clear(CalculatorState* state) {
    if (!state) return;
    state->op_count = 0;
}
