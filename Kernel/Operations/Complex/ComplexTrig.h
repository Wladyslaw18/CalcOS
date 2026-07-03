/*
 * Author: W. Kowal
 * Complex trigonometric and hyperbolic operations declarations.
 */

#ifndef COMPLEX_TRIG_H
#define COMPLEX_TRIG_H

#include "../../State/NumericValue.h"

ComplexValue complex_sin(ComplexValue z);
ComplexValue complex_cos(ComplexValue z);
ComplexValue complex_tan(ComplexValue z);
ComplexValue complex_sinh(ComplexValue z);
ComplexValue complex_cosh(ComplexValue z);
ComplexValue complex_tanh(ComplexValue z);

#endif // COMPLEX_TRIG_H
