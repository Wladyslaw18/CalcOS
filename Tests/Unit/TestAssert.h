#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include <stdio.h>
#include <stdbool.h>

static int total_assertions = 0;
static int failed_assertions = 0;

#define assert_true(expr) \
    do { \
        total_assertions++; \
        if (!(expr)) { \
            failed_assertions++; \
            printf("  [FAIL] %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #expr); \
        } \
    } while (0)

#define assert_double_eq(val1, val2) \
    do { \
        total_assertions++; \
        double diff = (val1) - (val2); \
        if (diff < 0.0) diff = -diff; \
        if (diff > 1e-9) { \
            failed_assertions++; \
            printf("  [FAIL] %s:%d: Assertion failed: %s == %s (expected %f, got %f)\n", \
                   __FILE__, __LINE__, #val1, #val2, (double)(val2), (double)(val1)); \
        } \
    } while (0)

#endif // TEST_ASSERT_H
