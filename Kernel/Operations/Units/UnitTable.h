#ifndef UNIT_TABLE_H
#define UNIT_TABLE_H

typedef enum {
    UNIT_TYPE_LENGTH,
    UNIT_TYPE_MASS,
    UNIT_TYPE_TEMP
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
    UNIT_FAHRENHEIT
} UnitId;

extern const UnitEntry LENGTH_TABLE[4];
extern const UnitEntry MASS_TABLE[3];
extern const UnitEntry TEMP_TABLE[3];

#endif // UNIT_TABLE_H
