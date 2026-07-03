/*
 * Complex calculation stress test - lets rip it
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "Application/Input/Parser.h"
#include "Kernel/State/CalculatorState.h"

static void run(const char* label, const char* expr, CalculatorState* state) {
    bool success = false;
    state->flags = 0;
    double result = parse_expression(expr, state, &success);
    if (success) {
        for (int j = 3; j > 0; j--) state->operands[j] = state->operands[j - 1];
        state->operands[0] = result;
        state->op_count = state->op_count < 4 ? state->op_count + 1 : 4;
        printf("  %-45s = %g\n", label, result);
    } else {
        printf("  %-45s = [FAIL flags=%d]\n", label, state->flags);
    }
}

int main(void) {
    CalculatorState state;
    memset(&state, 0, sizeof(state));

    printf("\n======================================\n");
    printf("  COMPLEX CALCULATIONS STRESS TEST\n");
    printf("======================================\n\n");

    printf("[1] TRIG IDENTITIES (should all be ~1.0 or ~0.0)\n");
    run("sin^2(0.7) + cos^2(0.7)",          "sin(0.7)^2 + cos(0.7)^2",            &state);
    run("sin(pi/6) [= 0.5]",                "sin(pi/6)",                           &state);
    run("cos(pi/3) [= 0.5]",                "cos(pi/3)",                           &state);
    run("tan(pi/4) [= 1.0]",                "tan(pi/4)",                           &state);
    run("asin(sin(1.2)) [= 1.2]",           "asin(sin(1.2))",                      &state);
    run("acos(cos(0.9)) [= 0.9]",           "acos(cos(0.9))",                      &state);
    run("sin(2*pi) [~= 0.0]",               "sin(2*pi)",                           &state);
    run("cos(2*pi) [= 1.0]",                "cos(2*pi)",                           &state);

    printf("\n[2] LOGARITHMS & EXPONENTIALS\n");
    run("e^1 [= 2.71828...]",               "e^1",                                 &state);
    run("ln(e^5) [= 5.0]",                  "ln(e^5)",                             &state);
    run("log10(10^7) [= 7.0]",              "log10(10^7)",                         &state);
    run("e^(ln(42)) [= 42.0]",              "e^(ln(42))",                          &state);
    run("ln(1) [= 0.0]",                    "ln(1)",                               &state);
    run("2^10 [= 1024.0]",                  "2^10",                                &state);
    run("2^32 [= 4294967296]",              "2^32",                                &state);
    run("2^53 [= 9007199254740992]",        "2^53",                                &state);

    printf("\n[3] NESTED / COMPOUND EXPRESSIONS\n");
    run("sqrt(2^2 + 3^2) [= sqrt(13)]",     "sqrt(2^2 + 3^2)",                     &state);
    run("(1+sqrt(5))/2 [golden ratio]",     "(1+sqrt(5))/2",                       &state);
    run("sin(cos(tan(0.1)))",               "sin(cos(tan(0.1)))",                  &state);
    run("ln(sqrt(e)) [= 0.5]",             "ln(sqrt(e))",                         &state);
    run("fact(10) [= 3628800]",             "fact(10)",                            &state);
    run("fact(12) [= 479001600]",           "fact(12)",                            &state);
    run("sqrt(fact(4)) [= sqrt(24)]",       "sqrt(fact(4))",                       &state);

    printf("\n[4] ADVANCED POWER & PRECISION TESTS\n");
    run("pi^e [= 22.459...]",               "pi^e",                                &state);
    run("e^pi [= 23.140...]",               "e^pi",                                &state);
    run("pi^pi [= 36.462...]",              "pi^pi",                               &state);
    run("sqrt(pi) [= 1.7724...]",           "sqrt(pi)",                            &state);
    run("ln(pi) [= 1.1447...]",             "ln(pi)",                              &state);
    run("sin(e) [= 0.4107...]",             "sin(e)",                              &state);
    run("cos(pi^2) [should work]",          "cos(pi^2)",                           &state);
    run("(e^pi) - pi [= 19.999...~20]",     "(e^pi) - pi",                         &state);
    run("2^(1/12) [semitone = 1.0594...]",  "2^(1/12)",                            &state);
    run("(1+1/1000000)^1000000 [~= e]",     "(1+1/1000000)^1000000",               &state);

    printf("\n[5] ROOT FINDING (solver)\n");
    run("x^2 - 9 = 0 in [0,5] [= 3.0]",   "solve(x^2 - 9, x, 0, 5)",            &state);
    run("x^3 - 27 = 0 [= 3.0]",            "solve(x^3 - 27, x, 0, 5)",           &state);
    run("x^2 - 2 = 0 [= sqrt(2)]",         "solve(x^2 - 2, x, 0, 2)",            &state);
    run("sin(x)-0.5=0 [= pi/6 = 0.5236]",  "solve(sin(x) - 0.5, x, 0, 2)",       &state);
    run("e^x - 10 = 0 [= ln(10)]",         "solve(e^x - 10, x, 0, 5)",           &state);

    printf("\n[6] EDGE CASES & BOUNDARY CONDITIONS\n");
    run("0^0 [= 1.0]",                      "0^0",                                 &state);
    run("1/0 [div-by-zero]",                "1/0",                                 &state);
    run("sqrt(-1) [= NaN]",                 "sqrt(-1)",                            &state);
    run("ln(-1) [= NaN]",                   "ln(-1)",                              &state);
    run("1000! [huge, overflow to Inf]",    "fact(1000)",                          &state);
    run("sin(1e6) [large angle reduction]", "sin(1000000)",                        &state);
    run("cos(1e6) [large angle reduction]", "cos(1000000)",                        &state);

    printf("\n[7] CHAINED ANS OPERATIONS\n");
    run("100 (seed Ans)",                   "100",                                 &state);
    run("Ans * 2 [= 200]",                  "Ans * 2",                             &state);
    run("Ans + Ans [= 400]",                "Ans + Ans",                           &state);
    run("sqrt(Ans) [= 20]",                 "sqrt(Ans)",                           &state);
    run("Ans^2 [= 400]",                    "Ans^2",                               &state);

    printf("\n======================================\n");
    printf("  DONE\n");
    printf("======================================\n\n");

    return 0;
}
