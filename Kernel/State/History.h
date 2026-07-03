#ifndef HISTORY_H
#define HISTORY_H

#include "CalculatorState.h"

#define HISTORY_MAX_ENTRIES 32

typedef struct {
    double operand_a;
    double operand_b;
    double result;
    OpType op;
} HistoryEntry;

// Push a new entry to the static circular history buffer
void history_push(CalculatorState* state, double a, double b, double result, OpType op);

// Retrieve an entry from history by relative index (0 = latest)
// Returns 1 if successful, 0 if out of range/empty
int history_get(const CalculatorState* state, uint32_t index, HistoryEntry* out_entry);

// Clear history
void history_clear(CalculatorState* state);

#endif // HISTORY_H
