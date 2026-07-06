#ifndef UNIT_TABLE_H
#define UNIT_TABLE_H

typedef enum {
    UNIT_TYPE_LENGTH,
    UNIT_TYPE_MASS,
    UNIT_TYPE_TEMP,
    UNIT_TYPE_TIME,
    UNIT_TYPE_SPEED,
    UNIT_TYPE_ENERGY,
    UNIT_TYPE_FORCE,
    UNIT_TYPE_PRESSURE,
    UNIT_TYPE_DATA,
    UNIT_TYPE_ANGLE,
    UNIT_TYPE_POWER
} UnitType;

typedef struct {
    const char* name;
    double factor;     // Multiplication factor relative to a base unit (e.g., Meter for length, Gram for mass)
    double offset;     // Offset adjustment (primarily for Temperature, e.g. Kelvin/Celsius/Fahrenheit)
} UnitEntry;

// Unit index definitions
typedef enum {
    // Length (Base: Meter)
    UNIT_METER = 0,
    UNIT_KILOMETER,
    UNIT_MILE,
    UNIT_FOOT,

    // Mass (Base: Gram)
    UNIT_GRAM,
    UNIT_KILOGRAM,
    UNIT_POUND,

    // Temperature (Base: Kelvin)
    UNIT_KELVIN,
    UNIT_CELSIUS,
    UNIT_FAHRENHEIT,

    // Time (Base: Second)
    UNIT_SECOND,
    UNIT_MILLISECOND,
    UNIT_MICROSECOND,
    UNIT_NANOSECOND,
    UNIT_MINUTE,
    UNIT_HOUR,
    UNIT_DAY,
    UNIT_WEEK,

    // Speed (Base: m/s)
    UNIT_METERS_PER_SECOND,
    UNIT_KM_PER_HOUR,
    UNIT_MILES_PER_HOUR,
    UNIT_KNOTS,
    UNIT_FEET_PER_SECOND,

    // Energy (Base: Joule)
    UNIT_JOULE,
    UNIT_KILOJOULE,
    UNIT_CALORIE,
    UNIT_KILOCALORIE,
    UNIT_WATT_HOUR,
    UNIT_KILOWATT_HOUR,
    UNIT_ELECTRONVOLT,

    // Force (Base: Newton)
    UNIT_NEWTON,
    UNIT_KILONEWTON,
    UNIT_POUND_FORCE,
    UNIT_DYNE,

    // Pressure (Base: Pascal)
    UNIT_PASCAL,
    UNIT_KILOPASCAL,
    UNIT_BAR,
    UNIT_ATM,
    UNIT_PSI,
    UNIT_MMHG,

    // Data (Base: Bit)
    UNIT_BIT,
    UNIT_BYTE,
    UNIT_KILOBYTE,
    UNIT_MEGABYTE,
    UNIT_GIGABYTE,
    UNIT_TERABYTE,

    // Angle (Base: Radian)
    UNIT_RADIAN,
    UNIT_DEGREE,
    UNIT_GRADIAN,

    // Power (Base: Watt)
    UNIT_WATT,
    UNIT_KILOWATT,
    UNIT_MEGAWATT,
    UNIT_HORSEPOWER,
    UNIT_BTU_PER_HOUR
} UnitId;

extern const UnitEntry LENGTH_TABLE[4];
extern const UnitEntry MASS_TABLE[3];
extern const UnitEntry TEMP_TABLE[3];
extern const UnitEntry TIME_TABLE[8];
extern const UnitEntry SPEED_TABLE[5];
extern const UnitEntry ENERGY_TABLE[7];
extern const UnitEntry FORCE_TABLE[4];
extern const UnitEntry PRESSURE_TABLE[6];
extern const UnitEntry DATA_TABLE[6];
extern const UnitEntry ANGLE_TABLE[3];
extern const UnitEntry POWER_TABLE[5];

#endif // UNIT_TABLE_H
