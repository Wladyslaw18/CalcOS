#ifndef COMPLEX_OPS_H
#define COMPLEX_OPS_H

#include "../../State/NumericValue.h"

// Complex operations declarations
ComplexValue complex_add(ComplexValue a, ComplexValue b);
ComplexValue complex_sub(ComplexValue a, ComplexValue b);
ComplexValue complex_mul(ComplexValue a, ComplexValue b);
ComplexValue complex_div(ComplexValue a, ComplexValue b);
ComplexValue complex_conjugate(ComplexValue a);
double complex_modulus(ComplexValue a);
void complex_polar(ComplexValue a, double* r, double* theta);

#endif // COMPLEX_OPS_H
