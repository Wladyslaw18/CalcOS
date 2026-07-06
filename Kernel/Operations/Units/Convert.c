#include "Convert.h"
#include "UnitTable.h"

// ── Existing tables ────────────────────────────────────────────────────────────

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
    { "K", 1.0,                 0.0 },           // UNIT_KELVIN   (idx 0 in table)
    { "C", 1.0,                 273.15 },         // UNIT_CELSIUS
    { "F", 0.5555555555555556,  255.37222222222222 } // UNIT_FAHRENHEIT
};

// ── New tables ─────────────────────────────────────────────────────────────────

// TIME — base unit: second
// factor = number of seconds in one unit of that row
const UnitEntry TIME_TABLE[8] = {
    { "s",   1.0,          0.0 },  // UNIT_SECOND
    { "ms",  1e-3,         0.0 },  // UNIT_MILLISECOND  (1 ms = 0.001 s)
    { "us",  1e-6,         0.0 },  // UNIT_MICROSECOND  (1 µs = 1e-6 s)
    { "ns",  1e-9,         0.0 },  // UNIT_NANOSECOND   (1 ns = 1e-9 s)
    { "min", 60.0,         0.0 },  // UNIT_MINUTE
    { "h",   3600.0,       0.0 },  // UNIT_HOUR
    { "d",   86400.0,      0.0 },  // UNIT_DAY
    { "wk",  604800.0,     0.0 }   // UNIT_WEEK         (7 × 86400)
};

// SPEED — base unit: m/s
// factor = number of m/s in one unit of that row
const UnitEntry SPEED_TABLE[5] = {
    { "m/s",   1.0,                0.0 },  // UNIT_METERS_PER_SECOND
    { "km/h",  0.27777777777778,   0.0 },  // UNIT_KM_PER_HOUR    (1/3.6)
    { "mph",   0.44704,            0.0 },  // UNIT_MILES_PER_HOUR
    { "kn",    0.51444444444444,   0.0 },  // UNIT_KNOTS          (1852/3600)
    { "ft/s",  0.3048,            0.0 }   // UNIT_FEET_PER_SECOND
};

// ENERGY — base unit: joule
// factor = number of joules in one unit of that row
const UnitEntry ENERGY_TABLE[7] = {
    { "J",    1.0,                   0.0 },  // UNIT_JOULE
    { "kJ",   1000.0,                0.0 },  // UNIT_KILOJOULE
    { "cal",  4.184,                 0.0 },  // UNIT_CALORIE       (thermochemical)
    { "kcal", 4184.0,                0.0 },  // UNIT_KILOCALORIE
    { "Wh",   3600.0,                0.0 },  // UNIT_WATT_HOUR
    { "kWh",  3600000.0,             0.0 },  // UNIT_KILOWATT_HOUR
    { "eV",   1.602176634e-19,       0.0 }   // UNIT_ELECTRONVOLT  (exact, 2019 SI)
};

// FORCE — base unit: newton
// factor = number of newtons in one unit of that row
const UnitEntry FORCE_TABLE[4] = {
    { "N",   1.0,       0.0 },  // UNIT_NEWTON
    { "kN",  1000.0,    0.0 },  // UNIT_KILONEWTON
    { "lbf", 4.4482216, 0.0 },  // UNIT_POUND_FORCE
    { "dyn", 1e-5,      0.0 }   // UNIT_DYNE
};

// PRESSURE — base unit: pascal
// factor = number of pascals in one unit of that row
const UnitEntry PRESSURE_TABLE[6] = {
    { "Pa",   1.0,        0.0 },  // UNIT_PASCAL
    { "kPa",  1000.0,     0.0 },  // UNIT_KILOPASCAL
    { "bar",  100000.0,   0.0 },  // UNIT_BAR
    { "atm",  101325.0,   0.0 },  // UNIT_ATM
    { "psi",  6894.757,   0.0 },  // UNIT_PSI
    { "mmHg", 133.32239,  0.0 }   // UNIT_MMHG  (torr; 13595.1 kg/m³ × 9.80665 m/s² × 0.001 m)
};

// DATA — base unit: bit
// factor = number of bits in one unit of that row
// Using SI binary: 1 KB = 1024 bytes = 8192 bits (IEC 80000-13 kibibyte convention
// common in OS reporting; adjust to 1000-byte SI if your domain requires it).
const UnitEntry DATA_TABLE[6] = {
    { "bit", 1.0,                  0.0 },  // UNIT_BIT
    { "B",   8.0,                  0.0 },  // UNIT_BYTE        (8 bits)
    { "KB",  8192.0,               0.0 },  // UNIT_KILOBYTE    (1024 × 8)
    { "MB",  8388608.0,            0.0 },  // UNIT_MEGABYTE    (1024² × 8)
    { "GB",  8589934592.0,         0.0 },  // UNIT_GIGABYTE    (1024³ × 8)
    { "TB",  8796093022208.0,      0.0 }   // UNIT_TERABYTE    (1024⁴ × 8)
};

// ANGLE — base unit: radian
// factor = number of radians in one unit of that row
// pi/180 = 0.017453292519943 rad/°; pi/200 = 0.015707963267949 rad/grad
const UnitEntry ANGLE_TABLE[3] = {
    { "rad",  1.0,                  0.0 },  // UNIT_RADIAN
    { "deg",  0.017453292519943,    0.0 },  // UNIT_DEGREE   (π/180)
    { "grad", 0.015707963267949,    0.0 }   // UNIT_GRADIAN  (π/200)
};

// POWER — base unit: watt
// factor = number of watts in one unit of that row
// Mechanical horsepower = 550 ft·lbf/s = 745.69987 W (exact per NIST)
// BTU/hr: 1 BTU = 1055.05585 J, so 1 BTU/hr = 1055.05585/3600 ≈ 0.29307108 W
const UnitEntry POWER_TABLE[5] = {
    { "W",      1.0,         0.0 },  // UNIT_WATT
    { "kW",     1000.0,      0.0 },  // UNIT_KILOWATT
    { "MW",     1000000.0,   0.0 },  // UNIT_MEGAWATT
    { "hp",     745.69987,   0.0 },  // UNIT_HORSEPOWER  (mechanical)
    { "BTU/hr", 0.29307108,  0.0 }   // UNIT_BTU_PER_HOUR
};

// ── Generic conversion helper ──────────────────────────────────────────────────
// All simple (non-temperature) tables use factor-only conversion:
//   base = input * from->factor
//   output = base / to->factor
// Temperature uses: base = input * from->factor + from->offset
//                   output = (base - to->offset) / to->factor

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
        case UNIT_TYPE_TIME: {
            if (from_idx < 0 || from_idx >= 8 || to_idx < 0 || to_idx >= 8) return false;
            const UnitEntry* from = &TIME_TABLE[from_idx];
            const UnitEntry* to = &TIME_TABLE[to_idx];
            value_in_base = input_value * from->factor;
            *output_value = value_in_base / to->factor;
            return true;
        }
        case UNIT_TYPE_SPEED: {
            if (from_idx < 0 || from_idx >= 5 || to_idx < 0 || to_idx >= 5) return false;
            const UnitEntry* from = &SPEED_TABLE[from_idx];
            const UnitEntry* to = &SPEED_TABLE[to_idx];
            value_in_base = input_value * from->factor;
            *output_value = value_in_base / to->factor;
            return true;
        }
        case UNIT_TYPE_ENERGY: {
            if (from_idx < 0 || from_idx >= 7 || to_idx < 0 || to_idx >= 7) return false;
            const UnitEntry* from = &ENERGY_TABLE[from_idx];
            const UnitEntry* to = &ENERGY_TABLE[to_idx];
            value_in_base = input_value * from->factor;
            *output_value = value_in_base / to->factor;
            return true;
        }
        case UNIT_TYPE_FORCE: {
            if (from_idx < 0 || from_idx >= 4 || to_idx < 0 || to_idx >= 4) return false;
            const UnitEntry* from = &FORCE_TABLE[from_idx];
            const UnitEntry* to = &FORCE_TABLE[to_idx];
            value_in_base = input_value * from->factor;
            *output_value = value_in_base / to->factor;
            return true;
        }
        case UNIT_TYPE_PRESSURE: {
            if (from_idx < 0 || from_idx >= 6 || to_idx < 0 || to_idx >= 6) return false;
            const UnitEntry* from = &PRESSURE_TABLE[from_idx];
            const UnitEntry* to = &PRESSURE_TABLE[to_idx];
            value_in_base = input_value * from->factor;
            *output_value = value_in_base / to->factor;
            return true;
        }
        case UNIT_TYPE_DATA: {
            if (from_idx < 0 || from_idx >= 6 || to_idx < 0 || to_idx >= 6) return false;
            const UnitEntry* from = &DATA_TABLE[from_idx];
            const UnitEntry* to = &DATA_TABLE[to_idx];
            value_in_base = input_value * from->factor;
            *output_value = value_in_base / to->factor;
            return true;
        }
        case UNIT_TYPE_ANGLE: {
            if (from_idx < 0 || from_idx >= 3 || to_idx < 0 || to_idx >= 3) return false;
            const UnitEntry* from = &ANGLE_TABLE[from_idx];
            const UnitEntry* to = &ANGLE_TABLE[to_idx];
            value_in_base = input_value * from->factor;
            *output_value = value_in_base / to->factor;
            return true;
        }
        case UNIT_TYPE_POWER: {
            if (from_idx < 0 || from_idx >= 5 || to_idx < 0 || to_idx >= 5) return false;
            const UnitEntry* from = &POWER_TABLE[from_idx];
            const UnitEntry* to = &POWER_TABLE[to_idx];
            value_in_base = input_value * from->factor;
            *output_value = value_in_base / to->factor;
            return true;
        }
        default:
            return false;
    }
}
