#ifndef RATIONAL_OPS_H
#define RATIONAL_OPS_H

#include "../../State/NumericValue.h"

// Rational operations declarations
RationalValue rational_add(RationalValue a, RationalValue b);
RationalValue rational_sub(RationalValue a, RationalValue b);
RationalValue rational_mul(RationalValue a, RationalValue b);
RationalValue rational_div(RationalValue a, RationalValue b);
void rational_simplify(RationalValue* r);

#endif // RATIONAL_OPS_H
