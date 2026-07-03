#ifndef NUMERIC_VALUE_H
#define NUMERIC_VALUE_H

#include <stdint.h>
#include <stdbool.h>

// Numeric type enumeration
typedef enum {
    NUMERIC_REAL     = 0,
    NUMERIC_COMPLEX  = 1,
    NUMERIC_RATIONAL = 2
} NumericType;

// Rational representation: numerator / denominator
typedef struct {
    int64_t num;
    int64_t den;
} RationalValue;

// Complex representation: real + i*imag
typedef struct {
    double real;
    double imag;
} ComplexValue;

// Discriminated union of numeric types
typedef struct {
    NumericType type;
    union {
        double real;             // NUMERIC_REAL
        ComplexValue complex;     // NUMERIC_COMPLEX
        RationalValue rational;   // NUMERIC_RATIONAL
    } value;
} NumericValue;

// Conversion and utility functions (implemented without stdlib)
void numeric_simplify_rational(RationalValue* r);
NumericValue numeric_from_real(double val);
NumericValue numeric_from_complex(double real, double imag);
NumericValue numeric_from_rational(int64_t num, int64_t den);

#endif // NUMERIC_VALUE_H
