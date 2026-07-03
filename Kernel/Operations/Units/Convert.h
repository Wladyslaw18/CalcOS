#ifndef CONVERT_H
#define CONVERT_H

#include "UnitTable.h"
#include <stdbool.h>

// Convert a numeric value between units of the same type. Returns true on success.
bool convert_units(double input_value, UnitType type, int from_idx, int to_idx, double* output_value);

#endif // CONVERT_H
