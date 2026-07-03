#ifndef OPERAND_STACK_H
#define OPERAND_STACK_H

#include "CalculatorState.h"

// Push a value to the operand stack
int stack_push(CalculatorState* state, double val);

// Pop a value from the operand stack
int stack_pop(CalculatorState* state, double* out_val);

// Peek the top value of the operand stack
int stack_peek(const CalculatorState* state, double* out_val);

// Clear the operand stack
void stack_clear(CalculatorState* state);

#endif // OPERAND_STACK_H
