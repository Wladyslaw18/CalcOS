#include "TestAssert.h"
#include "../../Kernel/Core/CPU/CPUID.h"
#include "../../Kernel/State/CalculatorState.h"
#include "../../Kernel/Operations/Arithmetic/Addition.h"
#include "../../Kernel/Operations/Arithmetic/Subtraction.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

int main() {
    printf("=== RUNNING TestSIMD ===\n");

    CPUFeatures features;
    cpu_detect_features(&features);

    printf("Detected CPU Features:\n");
    printf("  SSE2: %s\n", features.has_sse2 ? "YES" : "NO");
    printf("  AVX2: %s\n", features.has_avx2 ? "YES" : "NO");

    CalculatorState state;
    fast_memset(&state, 0, sizeof(CalculatorState));

    double a[8] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    double b[8] = {8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0};
    double res[8] = {0.0};

    // Test execute_addition dynamically routing the call
    execute_addition(&state, a, b, res, 8, &features);
    for (int i = 0; i < 8; ++i) {
        assert_double_eq(res[i], 9.0);
    }

    // Test execute_subtraction dynamically routing the call
    fast_memset(res, 0, sizeof(res));
    execute_subtraction(&state, a, b, res, 8, &features);
    for (int i = 0; i < 8; ++i) {
        assert_double_eq(res[i], (double)(i * 2 - 7));
    }

    printf("TestSIMD summary: %d/%d assertions passed.\n", 
           total_assertions - failed_assertions, total_assertions);
    return failed_assertions > 0 ? 1 : 0;
}
