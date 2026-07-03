#include "TestAssert.h"
#include "../../Kernel/State/CalculatorState.h"
#include "../../Kernel/Operations/Arithmetic/Addition.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

int main() {
    printf("=== RUNNING TestAddition ===\n");

    CalculatorState state;
    fast_memset(&state, 0, sizeof(CalculatorState));

    double a[4] = {1.0, 2.5, -3.0, 0.0};
    double b[4] = {2.0, -1.5, 4.0, 5.5};
    double res[4] = {0.0};

    // 1. Test scalar addition
    add_scalar(&state, a, b, res, 4);
    assert_double_eq(res[0], 3.0);
    assert_double_eq(res[1], 1.0);
    assert_double_eq(res[2], 1.0);
    assert_double_eq(res[3], 5.5);

    // 2. Test SSE addition
    fast_memset(res, 0, sizeof(res));
    add_sse(&state, a, b, res, 4);
    assert_double_eq(res[0], 3.0);
    assert_double_eq(res[1], 1.0);
    assert_double_eq(res[2], 1.0);
    assert_double_eq(res[3], 5.5);

    // 3. Test AVX2 addition
    fast_memset(res, 0, sizeof(res));
    add_avx2(&state, a, b, res, 4);
    assert_double_eq(res[0], 3.0);
    assert_double_eq(res[1], 1.0);
    assert_double_eq(res[2], 1.0);
    assert_double_eq(res[3], 5.5);

    // Print summary
    printf("TestAddition summary: %d/%d assertions passed.\n", 
           total_assertions - failed_assertions, total_assertions);
    return failed_assertions > 0 ? 1 : 0;
}
