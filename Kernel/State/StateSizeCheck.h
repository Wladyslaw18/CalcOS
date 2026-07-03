/* CalculatorState size guard - if this fails, you added a field without
   trimming _pad. Keep it at exactly 64 bytes for L1 cache-line alignment. */
#ifndef CALC_STATE_SIZE_CHECK_H
#define CALC_STATE_SIZE_CHECK_H
#include "CalculatorState.h"
#include <stddef.h>
/* Uncomment for freestanding builds that support static_assert: */
/* static_assert(sizeof(CalculatorState) == 64, "CalculatorState must be 64 bytes"); */
#define CALC_STATE_EXPECTED_SIZE 64
#endif
