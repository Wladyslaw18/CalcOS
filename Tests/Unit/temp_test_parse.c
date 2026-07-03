/*
 * Quick test to verify the parser handles basic expressions.
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "Application/Input/Parser.h"
#include "Application/Input/Experimental/ShuntingYard.h"
#include "Kernel/State/CalculatorState.h"

int main(void) {
    CalculatorState state;
    memset(&state, 0, sizeof(state));
    bool success = false;

    // Test operations and constants
    const char* tests[] = {
        "2+3",
        "pi",
        "e",
        "sin(0)",
        "cos(0)",
        "tan(0.7853981633974483)", // tan(pi/4) = 1.0
        "asin(1.0)",               // asin(1.0) = pi/2 = 1.570796
        "acos(1.0)",               // acos(1.0) = 0.0
        "atan(1.0)",               // atan(1.0) = pi/4 = 0.785398
        "ln(2.718281828459045)",   // ln(e) = 1.0
        "log10(100)",              // log10(100) = 2.0
        "sqrt(16)",
        "√16",
        "fact(5)",                 // fact(5) = 120
        "5!",                      // 5! = 120
        "2^0.5",                   // 2^0.5 = 1.4142...
        "5e3",                     // 5e3 = 5000.0
        "5E-3",                    // 5E-3 = 0.005
        "5 EXP 3",                 // 5 * 10^3 = 5000.0
        "1/0",                     // Div by zero error
        NULL
    };

    printf("=== RUNNING PARSER UNIT TESTS ===\n");
    for (int i = 0; tests[i]; i++) {
        success = false;
        state.flags = 0;
        double result = parse_expression(tests[i], &state, &success);
        printf("expr='%s' -> success=%d result=%f error_flags=%d\n", tests[i], success, result, state.flags);
        if (success) {
            // Push result onto stack to populate operands[0] for Ans
            for (int j = 3; j > 0; j--) state.operands[j] = state.operands[j - 1];
            state.operands[0] = result;
            state.op_count = state.op_count < 4 ? state.op_count + 1 : 4;
        }
    }

    // Test Ans variable
    success = false;
    double result = parse_expression("Ans + 10", &state, &success);
    printf("expr='Ans + 10' -> success=%d result=%f\n", success, result);

    // Test direct sub-expression parsing
    success = false;
    parse_expression("x = 0", &state, &success);
    double sub_res = parse_expression("x^2 - 4", &state, &success);
    printf("expr='x^2 - 4' (x=0) -> success=%d result=%f\n", success, sub_res);

    // Test solver argument parsing
    success = false;
    double solve_res = parse_expression("solve(x^2 - 4, x, 0, 5)", &state, &success);
    printf("expr='solve(x^2 - 4, x, 0, 5)' -> success=%d result=%f\n", success, solve_res);

    // Chaos stress tests
    printf("\n=== RUNNING CHAOS STRESS TESTS ===\n");
    
    // 1. Solve sin(x) - cos(x)
    success = false;
    double solve_trig = parse_expression("solve(sin(x) - cos(x), x, 0, pi)", &state, &success);
    printf("Chaos #1: expr='solve(sin(x) - cos(x), x, 0, pi)' -> success=%d result=%f (expected: 0.785398)\n", success, solve_trig);

    // 2. 250-level 1+1 test
    char deep_1plus1[1024];
    int depth_1plus1 = 250;
    int idx_1plus1 = 0;
    for (int i = 0; i < depth_1plus1; i++) deep_1plus1[idx_1plus1++] = '(';
    deep_1plus1[idx_1plus1++] = '1';
    deep_1plus1[idx_1plus1++] = '+';
    deep_1plus1[idx_1plus1++] = '1';
    for (int i = 0; i < depth_1plus1; i++) deep_1plus1[idx_1plus1++] = ')';
    deep_1plus1[idx_1plus1] = '\0';
    success = false;
    double deep_1plus1_res = parse_expression(deep_1plus1, &state, &success);
    printf("Chaos #2: 250-nested '1+1' -> success=%d result=%f (expected: 2.000000)\n", success, deep_1plus1_res);

    // 3. Golden Ratio and Ans loop
    success = false;
    double phi_res = parse_expression("phi = (1 + sqrt(5)) / 2", &state, &success);
    if (success) {
        for (int j = 3; j > 0; j--) state.operands[j] = state.operands[j - 1];
        state.operands[0] = phi_res;
        state.op_count = state.op_count < 4 ? state.op_count + 1 : 4;
    }
    printf("Chaos #3.1: expr='phi = (1 + sqrt(5)) / 2' -> success=%d result=%f (expected: 1.618034)\n", success, phi_res);

    success = false;
    double phi_eq = parse_expression("phi^2 - phi - 1", &state, &success);
    if (success) {
        for (int j = 3; j > 0; j--) state.operands[j] = state.operands[j - 1];
        state.operands[0] = phi_eq;
        state.op_count = state.op_count < 4 ? state.op_count + 1 : 4;
    }
    printf("Chaos #3.2: expr='phi^2 - phi - 1' -> success=%d result=%e (expected: ~0.000000)\n", success, phi_eq);

    success = false;
    double ans_stat = parse_expression("Ans * 10^5 + mean(1, 2, 3, 4, 5) * median(10, 20, 30)", &state, &success);
    printf("Chaos #3.3: expr='Ans * 10^5 + mean(1, 2, 3, 4, 5) * median(10, 20, 30)' -> success=%d result=%f (expected: 60.000000)\n", success, ans_stat);

    // Test depth limits
    printf("\n=== TESTING DEPTH LIMITS ===\n");
    char deep_expr[1024];
    // 250 nested parens (should pass, limit is 256)
    int depth = 250;
    int idx = 0;
    for (int i = 0; i < depth; i++) deep_expr[idx++] = '(';
    deep_expr[idx++] = '2';
    for (int i = 0; i < depth; i++) deep_expr[idx++] = ')';
    deep_expr[idx] = '\0';
    success = false;
    parse_expression(deep_expr, &state, &success);
    printf("depth=250 -> success=%d\n", success);

    // 260 nested parens (should fail, exceeds 256 limit)
    depth = 260;
    idx = 0;
    for (int i = 0; i < depth; i++) deep_expr[idx++] = '(';
    deep_expr[idx++] = '2';
    for (int i = 0; i < depth; i++) deep_expr[idx++] = ')';
    deep_expr[idx] = '\0';
    success = false;
    parse_expression(deep_expr, &state, &success);
    printf("depth=260 -> success=%d (expected fail)\n", success);

    // Test Experimental Shunting-Yard parser
    printf("\n=== RUNNING EXPERIMENTAL SHUNTING-YARD TESTS ===\n");
    const char* sy_tests[] = {
        "2+3",
        "10 / 4",
        "sin(0)",
        "cos(0)",
        "sqrt(16)",
        "fact(5)",
        "ln(2.718281828459045)",
        "2*3+4",
        "2*(3+4)",
        "-5+3",
        "2+3i",
        "cos(i)",
        "sqrt(-4)",
        NULL
    };
    for (int i = 0; sy_tests[i]; i++) {
        RPNQueue queue;
        success = infix_to_rpn(&state, sy_tests[i], &queue);
        if (success) {
            state.flags = 0;
            ComplexValue res = evaluate_rpn(&state, &queue);
            if (res.imag == 0.0) {
                printf("SY expr='%s' -> success=1 result=%g flags=%d\n", sy_tests[i], res.real, state.flags);
            } else if (res.real == 0.0) {
                printf("SY expr='%s' -> success=1 result=%gi flags=%d\n", sy_tests[i], res.imag, state.flags);
            } else {
                printf("SY expr='%s' -> success=1 result=%g%+gi flags=%d\n", sy_tests[i], res.real, res.imag, state.flags);
            }
        } else {
            printf("SY expr='%s' -> compile failed\n", sy_tests[i]);
        }
    }

    // Test Direct RPN parser
    printf("\n=== RUNNING DIRECT RPN PARSER TESTS ===\n");
    const char* raw_rpn_tests[] = {
        "2 3 +",
        "10 4 /",
        "0 sin",
        "0 cos",
        "16 sqrt",
        "5 fact",
        "2.718281828459045 ln",
        "2 3 * 4 +",
        "2 3 4 + *",
        "-5 3 +",
        "2 3 i * +",
        "2 3i +",
        "-4 sqrt",
        "1 1 i * + ln",
        NULL
    };
    for (int i = 0; raw_rpn_tests[i]; i++) {
        RPNQueue queue;
        success = parse_rpn(&state, raw_rpn_tests[i], &queue);
        if (success) {
            state.flags = 0;
            ComplexValue res = evaluate_rpn(&state, &queue);
            if (res.imag == 0.0) {
                printf("Direct RPN expr='%s' -> success=1 result=%g flags=%d\n", raw_rpn_tests[i], res.real, state.flags);
            } else if (res.real == 0.0) {
                printf("Direct RPN expr='%s' -> success=1 result=%gi flags=%d\n", raw_rpn_tests[i], res.imag, state.flags);
            } else {
                printf("Direct RPN expr='%s' -> success=1 result=%g%+gi flags=%d\n", raw_rpn_tests[i], res.real, res.imag, state.flags);
            }
        } else {
            printf("Direct RPN expr='%s' -> compile failed\n", raw_rpn_tests[i]);
        }
    }

    return 0;
}
