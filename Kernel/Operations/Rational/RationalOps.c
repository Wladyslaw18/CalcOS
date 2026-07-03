#include "RationalOps.h"
#include "../NumberTheory/NumberTheory.h"

void rational_simplify(RationalValue* r) {
    numeric_simplify_rational(r);
}

// Helper: signed GCD wrapper for cross-term reduction
static int64_t signed_gcd(int64_t a, int64_t b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    return (int64_t)gcd((uint64_t)a, (uint64_t)b);
}

RationalValue rational_add(RationalValue a, RationalValue b) {
    RationalValue res;
    // Cross-reduce BEFORE multiplying to prevent overflow
    // (a.num/a.den) + (b.num/b.den) = (a.num*(b.den/g) + b.num*(a.den/g)) / (a.den/g*b.den)
    int64_t g = signed_gcd(a.den, b.den);
    int64_t b_den_r = b.den / g; // reduced b.den
    int64_t a_den_r = a.den / g; // reduced a.den
    res.num = a.num * b_den_r + b.num * a_den_r;
    res.den = a.den * b_den_r;
    numeric_simplify_rational(&res);
    return res;
}

RationalValue rational_sub(RationalValue a, RationalValue b) {
    RationalValue res;
    // Cross-reduce to prevent overflow
    int64_t g = signed_gcd(a.den, b.den);
    int64_t b_den_r = b.den / g;
    int64_t a_den_r = a.den / g;
    res.num = a.num * b_den_r - b.num * a_den_r;
    res.den = a.den * b_den_r;
    numeric_simplify_rational(&res);
    return res;
}

RationalValue rational_mul(RationalValue a, RationalValue b) {
    RationalValue res;
    // Cross-reduce before multiply: a.num/b.den and b.num/a.den
    int64_t g1 = signed_gcd(a.num < 0 ? -a.num : a.num, b.den < 0 ? -b.den : b.den);
    int64_t g2 = signed_gcd(b.num < 0 ? -b.num : b.num, a.den < 0 ? -a.den : a.den);
    res.num = (a.num / g1) * (b.num / g2);
    res.den = (a.den / g2) * (b.den / g1);
    numeric_simplify_rational(&res);
    return res;
}

RationalValue rational_div(RationalValue a, RationalValue b) {
    RationalValue res;
    // (a/b) / (c/d) = (a*d) / (b*c) - cross-reduce before multiplying
    int64_t g1 = signed_gcd(a.num < 0 ? -a.num : a.num, b.num < 0 ? -b.num : b.num);
    int64_t g2 = signed_gcd(a.den < 0 ? -a.den : a.den, b.den < 0 ? -b.den : b.den);
    res.num = (a.num / g1) * (b.den / g2);
    res.den = (a.den / g2) * (b.num / g1);
    numeric_simplify_rational(&res);
    return res;
}
