#include "include/calc/calc.h"
#include "Kernel/State/CalculatorState.h"
#include "Application/Input/Parser.h"
#include "Infrastructure/Utils/MemoryUtils.h"

// Define the opaque struct calc_state to map directly to the cache-aligned CalculatorState
struct calc_state {
    CalculatorState state;
};

size_t calc_state_size(void) {
    return sizeof(struct calc_state);
}

void calc_state_init(calc_state_t* state) {
    if (!state) return;
    fast_memset(state, 0, sizeof(struct calc_state));
}

double calc_evaluate(calc_state_t* state, const char* expression, bool* success) {
    if (!state || !expression) {
        if (success) *success = false;
        return calc_nan();
    }
    return parse_expression(expression, &state->state, success);
}

uint8_t calc_get_flags(const calc_state_t* state) {
    if (!state) return 0;
    return state->state.flags;
}

void calc_clear_flags(calc_state_t* state) {
    if (!state) return;
    state->state.flags = 0;
}

uint8_t calc_get_mode(const calc_state_t* state) {
    if (!state) return 0;
    return state->state.mode;
}

void calc_set_mode(calc_state_t* state, uint8_t mode) {
    if (!state) return;
    state->state.mode = mode;
}
