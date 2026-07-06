/*
 * ParserAdapter.c — Bridges the existing Application/Input/Parser.c
 * implementation to the Kernel ParserAPI stable interface.
 *
 * This lets the Pratt parser be registered as a Kernel service.
 * To swap to the Shunting Yard parser, just register a different
 * ParserAPI struct — no consumer code changes needed.
 */

#include "../../Kernel/API/ParserAPI.h"
#include "../../Application/Input/Parser.h"

/* ─── Parse wrapper ─── */
static double adapter_parse(const char* source, CalculatorState* state, bool* success) {
    return parse_expression(source, state, success);
}

/* ─── Register custom function (TODO: wire through to parser's symbol table) ─── */
static bool adapter_register_fn(const char* name, double (*fn)(double)) {
    (void)name; (void)fn;
    /* Placeholder — the current Pratt parser doesn't support dynamic
     * function extension. The ShuntingYard experimental parser does.
     * When that's wired up, this will route to it. */
    return false;
}

/* ─── Public API struct for registration ─── */
const ParserAPI g_parser_service = {
    .parse             = adapter_parse,
    .register_function = adapter_register_fn,
};
