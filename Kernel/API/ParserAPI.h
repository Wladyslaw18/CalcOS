/*
 * ParserAPI.h — Stable proxy interface for the expression parser service
 *
 * The parser is the bridge between user input text and computed results.
 * By making it a Kernel service, we can:
 *   1. Swap implementations (Pratt parser ←→ Shunting Yard parser)
 *   2. Disable it without crashing (null object pattern)
 *   3. Let plugins register custom parser extensions
 *
 * Null Object: ker_parser_null — always returns 0.0, success=false
 */

#ifndef PARSER_API_H
#define PARSER_API_H

#include "../../Kernel/State/CalculatorState.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Parser Service Interface ─── */
typedef struct {
    /* Parse a mathematical expression string.
     * source: null-terminated expression string
     * state:  CalculatorState for symbol table access
     * success: set to true on success, false on failure
     * returns: computed result (NaN on failure) */
    double (*parse)(const char* source, CalculatorState* state, bool* success);

    /* Register a custom function by name (for plugin extensions).
     * Returns true if successfully registered. */
    bool (*register_function)(const char* name, double (*fn)(double));

} ParserAPI;

/* ─── Null Object (always returns 0.0, failure) ─── */
extern const ParserAPI ker_parser_null;

#ifdef __cplusplus
}
#endif

#endif /* PARSER_API_H */
