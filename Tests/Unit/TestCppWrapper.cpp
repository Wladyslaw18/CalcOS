#include "Tests/Unit/TestAssert.h"
#include "include/calc/calc.hpp"
#include <iostream>

int main() {
    printf("=== RUNNING TestCppWrapper ===\n");

    // 1. Test basic operations and RAII
    {
        calc::Calculator calculator;
        
        // Test basic math
        bool success = false;
        double result = calculator.evaluate("2 + 3", success);
        assert_true(success);
        assert_double_eq(result, 5.0);

        // Test float division
        result = calculator.evaluate("10 / 4", success);
        assert_true(success);
        assert_double_eq(result, 2.5);

        // Test error flags (division by zero)
        result = calculator.evaluate("1 / 0", success);
        assert_true(!success); // Should fail
        assert_true(calculator.get_flags() != 0); // Status flag set

        // Test clear flags
        calculator.clear_flags();
        assert_true(calculator.get_flags() == 0);
    }

    // 2. Test modes get/set
    {
        calc::Calculator calculator;
        calculator.set_mode(2); // Hex mode
        assert_true(calculator.get_mode() == 2);
        
        calculator.set_mode(0); // Float mode
        assert_true(calculator.get_mode() == 0);
    }

    // 3. Test custom Move constructor
    {
        calc::Calculator calc1;
        calc1.set_mode(1);
        
        // Move construct
        calc::Calculator calc2 = std::move(calc1);
        
        // Verify state is preserved in calc2
        assert_true(calc2.get_mode() == 1);
        
        // Verify we can still evaluate in calc2
        bool success = false;
        double result = calc2.evaluate("3 * 4", success);
        assert_true(success);
        assert_double_eq(result, 12.0);
    }

    // 4. Test custom Move assignment
    {
        calc::Calculator calc1;
        calc1.set_mode(3);
        
        calc::Calculator calc2;
        calc2 = std::move(calc1);
        
        // Verify state
        assert_true(calc2.get_mode() == 3);
        
        bool success = false;
        double result = calc2.evaluate("15 - 5", success);
        assert_true(success);
        assert_double_eq(result, 10.0);
    }

    // Report results
    printf("TestCppWrapper summary: %d/%d assertions passed.\n", 
           total_assertions - failed_assertions, total_assertions);

    return failed_assertions > 0 ? 1 : 0;
}
