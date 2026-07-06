#ifndef COMPLEX_OPS_H
#define COMPLEX_OPS_H

#include "../../State/NumericValue.h"
#include "../../../Infrastructure/Utils/MathUtils.h"

// ─────────────────────────────────────────────────────────────────────────────
// Basic arithmetic
// ─────────────────────────────────────────────────────────────────────────────
ComplexValue complex_add(ComplexValue a, ComplexValue b);
ComplexValue complex_sub(ComplexValue a, ComplexValue b);
ComplexValue complex_mul(ComplexValue a, ComplexValue b);
ComplexValue complex_div(ComplexValue a, ComplexValue b);
ComplexValue complex_conjugate(ComplexValue a);
double       complex_modulus(ComplexValue a);
void         complex_polar(ComplexValue a, double* r, double* theta);

// ─────────────────────────────────────────────────────────────────────────────
// Transcendental operations
// ─────────────────────────────────────────────────────────────────────────────

/* complex_exp — e^z = e^(re) * (cos(im) + i*sin(im))
 * Returns complex NaN when |re| exceeds the double exponent range (~709). */
ComplexValue complex_exp(ComplexValue z);

/* complex_log — Principal value: ln(z) = ln(|z|) + i*arg(z).
 * Returns complex NaN for z = 0+0i (log of zero is undefined). */
ComplexValue complex_log(ComplexValue z);

/* complex_pow — base^exp = exp(exp * log(base)).
 * Special cases: 0^p (p.real>0) → 0; integer exponents use repeated mul. */
ComplexValue complex_pow(ComplexValue base, ComplexValue exponent);

/* complex_sqrt — Principal square root of z.
 * Uses: sqrt(z) = sqrt((|z|+re)/2) + i*sign(im)*sqrt((|z|-re)/2).
 * Returns 0+0i for z = 0, and 0+i*sqrt(-re) for negative-real pure inputs. */
ComplexValue complex_sqrt(ComplexValue z);

#endif // COMPLEX_OPS_H
