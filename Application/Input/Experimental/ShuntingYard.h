/*
 * ShuntingYard.h ALTERNATIVE EXPERIMENTAL PARSER BACKEND (NOT CURRENTLY WIRED)
 * 
 * High-performance, zero-allocation Expression compiler using the Shunting-Yard
 * algorithm. Supports functions, nested expressions, and unary minus context.
 */

#ifndef SHUNTING_YARD_H
#define SHUNTING_YARD_H

#include "../../../Kernel/State/CalculatorState.h"
#include "../../../Kernel/State/NumericValue.h"
#include <stdbool.h>

#define MAX_RPN_TOKENS 128

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RPN_TOKEN_NUMBER,
    RPN_TOKEN_OPERATOR,
    RPN_TOKEN_FUNCTION,
    RPN_TOKEN_CUSTOM_FUNCTION
} RPNTokenType;

typedef enum {
    FN_NONE = 0,
    FN_SIN,
    FN_COS,
    FN_TAN,
    FN_SQRT,
    FN_FACT,
    FN_LN,
    FN_LOG10
} RPNFunctionType;

typedef struct {
    RPNTokenType type;
    union {
        ComplexValue value;
        char op;
        RPNFunctionType fn;
        ComplexValue (*custom_fn)(ComplexValue);
    } data;
} RPNToken;

// Zero-allocation RPN compilation output
typedef struct {
    RPNToken tokens[MAX_RPN_TOKENS];
    uint32_t count;
} RPNQueue;

// Compiles infix string directly to RPN queue on the stack
bool infix_to_rpn(CalculatorState* state, const char* infix, RPNQueue* rpn_queue);

// Parses a raw RPN string (e.g. "2 3 + 4 *") directly into the RPN queue
bool parse_rpn(CalculatorState* state, const char* rpn_str, RPNQueue* rpn_queue);

// Evaluates the RPN queue on the cache-aligned CalculatorState
ComplexValue evaluate_rpn(CalculatorState* state, const RPNQueue* rpn_queue);

#ifdef __cplusplus
}
#endif

#endif // SHUNTING_YARD_H
