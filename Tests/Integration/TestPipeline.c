#include "../Unit/TestAssert.h"
#include "../../Kernel/State/CalculatorState.h"
#include "../../Application/Input/Parser.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

int main() {
    printf("=== RUNNING TestPipeline ===\n");

    CalculatorState state;
    fast_memset(&state, 0, sizeof(CalculatorState));

    bool success = false;
    double result = 0.0;

    // Test simple addition
    result = parse_expression("10 + 20.5", &state, &success);
    assert_true(success);
    assert_double_eq(result, 30.5);

    // Test operator precedence and parentheses
    result = parse_expression("2 * (3 + 4)", &state, &success);
    assert_true(success);
    assert_double_eq(result, 14.0);

    // Test floating point subtraction and multiplication
    result = parse_expression("50 - 5 * 2.5", &state, &success);
    assert_true(success);
    assert_double_eq(result, 37.5);

    // Test division by zero sets division by zero flag (value 2) in CalculatorState
    result = parse_expression("10 / 0", &state, &success);
    assert_true(!success);
    assert_true((state.flags & 2) != 0);

    printf("TestPipeline summary: %d/%d assertions passed.\n", 
           total_assertions - failed_assertions, total_assertions);
    return failed_assertions > 0 ? 1 : 0;
}
