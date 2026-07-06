/*
 * HistoryAPI.h — Stable proxy interface for expression history
 *
 * Null Object: ker_history_null — empty history, no-ops
 */

#ifndef HISTORY_API_H
#define HISTORY_API_H

#include "../../Kernel/State/CalculatorState.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── History Entry (matches Kernel/State/History.h) ─── */
typedef struct {
    double operand_a;
    double operand_b;
    double result;
    uint8_t op;
} HistoryEntryAPI;

/* ─── History Service Interface ─── */
typedef struct {
    /* Push a new operation to history */
    void (*push)(CalculatorState* state, double a, double b,
                 double result, uint8_t op);

    /* Get an entry by index (0 = most recent).
     * Returns 1 if found, 0 if out of range. */
    int (*get)(const CalculatorState* state, uint32_t index,
               HistoryEntryAPI* out_entry);

    /* Clear all history */
    void (*clear)(CalculatorState* state);

} HistoryAPI;

/* ─── Null Object ─── */
extern const HistoryAPI ker_history_null;

#ifdef __cplusplus
}
#endif

#endif /* HISTORY_API_H */
