#include "History.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

#if defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define THREAD_LOCAL __thread
#else
#define THREAD_LOCAL _Thread_local
#endif

// Static circular buffer allocation (No malloc allowed!)
static THREAD_LOCAL HistoryEntry history_buffer[HISTORY_MAX_ENTRIES];
static THREAD_LOCAL uint32_t total_history_count = 0;

/* FIX: Null-guard added - bare-metal callers may pass NULL on OOM paths */
void history_push(CalculatorState* state, double a, double b, double result, OpType op) {
    if (!state) return;
    
    uint32_t idx = state->history_idx;
    history_buffer[idx].operand_a = a;
    history_buffer[idx].operand_b = b;
    history_buffer[idx].result = result;
    history_buffer[idx].op = op;
    
    state->history_idx = (state->history_idx + 1) % HISTORY_MAX_ENTRIES;
    if (total_history_count < HISTORY_MAX_ENTRIES) {
        total_history_count++;
    }
}

int history_get(const CalculatorState* state, uint32_t index, HistoryEntry* out_entry) {
    if (!state || !out_entry || index >= total_history_count) {
        return 0;
    }
    
    // Retrieve latest starting at history_idx - 1
    int32_t target_idx = (int32_t)state->history_idx - 1 - (int32_t)index;
    while (target_idx < 0) {
        target_idx += HISTORY_MAX_ENTRIES;
    }
    
    *out_entry = history_buffer[target_idx % HISTORY_MAX_ENTRIES];
    return 1;
}

void history_clear(CalculatorState* state) {
    if (state) {
        state->history_idx = 0;
    }
    total_history_count = 0;
    fast_memset(history_buffer, 0, sizeof(history_buffer));
}

