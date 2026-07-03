#include "Convert.h"
#include "UnitTable.h"

const UnitEntry LENGTH_TABLE[4] = {
    { "m",  1.0,      0.0 },  // UNIT_METER
    { "km", 1000.0,   0.0 },  // UNIT_KILOMETER
    { "mi", 1609.344, 0.0 },  // UNIT_MILE
    { "ft", 0.3048,   0.0 }   // UNIT_FOOT
};

const UnitEntry MASS_TABLE[3] = {
    { "g",  1.0,       0.0 },  // UNIT_GRAM   (idx 0 in table)
    { "kg", 1000.0,    0.0 },  // UNIT_KILOGRAM
    { "lb", 453.59237, 0.0 }   // UNIT_POUND
};

const UnitEntry TEMP_TABLE[3] = {
    { "K", 1.0,                 0.0 },          // UNIT_KELVIN   (idx 0 in table)
    { "C", 1.0,                 273.15 },        // UNIT_CELSIUS
    { "F", 0.5555555555555556,  255.37222222222222 } // UNIT_FAHRENHEIT
};

bool convert_units(double input_value, UnitType type, int from_idx, int to_idx, double* output_value) {
    if (!output_value) return false;

    double value_in_base = 0.0;

    switch (type) {
        case UNIT_TYPE_LENGTH: {
            if (from_idx < 0 || from_idx >= 4 || to_idx < 0 || to_idx >= 4) return false;
            const UnitEntry* from = &LENGTH_TABLE[from_idx];
            const UnitEntry* to = &LENGTH_TABLE[to_idx];
            // Convert to base (Meter)
            value_in_base = input_value * from->factor;
            // Convert from base to target
            *output_value = value_in_base / to->factor;
            return true;
        }
        case UNIT_TYPE_MASS: {
            // MASS_TABLE is 0-indexed: 0=gram, 1=kg, 2=lb
            if (from_idx < 0 || from_idx >= 3 || to_idx < 0 || to_idx >= 3) return false;
            const UnitEntry* from = &MASS_TABLE[from_idx];
            const UnitEntry* to = &MASS_TABLE[to_idx];
            value_in_base = input_value * from->factor;
            *output_value = value_in_base / to->factor;
            return true;
        }
        case UNIT_TYPE_TEMP: {
            // TEMP_TABLE is 0-indexed: 0=kelvin, 1=celsius, 2=fahrenheit
            if (from_idx < 0 || from_idx >= 3 || to_idx < 0 || to_idx >= 3) return false;
            const UnitEntry* from = &TEMP_TABLE[from_idx];
            const UnitEntry* to = &TEMP_TABLE[to_idx];
            value_in_base = input_value * from->factor + from->offset;
            *output_value = (value_in_base - to->offset) / to->factor;
            return true;
        }
        default:
            return false;
    }
}
