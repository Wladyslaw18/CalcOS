/*
 * HistoryAdapter.c — Bridges the existing Kernel/State/History.c
 * implementation to the Kernel HistoryAPI stable interface.
 */

#include "../../Kernel/API/HistoryAPI.h"
#include "../../Kernel/State/History.h"

/* ─── Push wrapper ─── */
static void adapter_push(CalculatorState* state, double a, double b,
                          double result, unsigned char op) {
    /* Map unsigned char to uint8_t / OpType */
    history_push(state, a, b, result, (OpType)(op & 0xFF));
}

/* ─── Get wrapper ─── */
static int adapter_get(const CalculatorState* state, uint32_t index,
                        HistoryEntryAPI* out_entry) {
    if (!out_entry) return 0;
    /* Map HistoryEntryAPI back to HistoryEntry */
    HistoryEntry entry;
    int ret = history_get(state, index, &entry);
    if (ret) {
        out_entry->operand_a = entry.operand_a;
        out_entry->operand_b = entry.operand_b;
        out_entry->result    = entry.result;
        out_entry->op        = (uint8_t)entry.op;
    }
    return ret;
}

/* ─── Clear wrapper ─── */
static void adapter_clear(CalculatorState* state) {
    history_clear(state);
}

/* ─── Public API struct ─── */
const HistoryAPI g_history_service = {
    .push  = adapter_push,
    .get   = adapter_get,
    .clear = adapter_clear,
};
