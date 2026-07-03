#include "NumericValue.h"

#if defined(__GNUC__) || defined(__clang__)
#define ctzll(x) __builtin_ctzll(x)
#elif defined(_MSC_VER)
#include <intrin.h>
static inline int msvc_ctzll(uint64_t val) {
    if (val == 0) return 64;
    unsigned long index;
#if defined(_M_AMD64) || defined(_M_ARM64)
    _BitScanForward64(&index, val);
    return (int)index;
#else
    if (_BitScanForward(&index, (unsigned long)(val & 0xFFFFFFFF))) {
        return (int)index;
    }
    _BitScanForward(&index, (unsigned long)(val >> 32));
    return (int)(index + 32);
#endif
}
#define ctzll(x) msvc_ctzll(x)
#else
static inline int custom_ctzll(uint64_t val) {
    if (val == 0) return 64;
    int count = 0;
    if ((val & 0xFFFFFFFFULL) == 0) { count += 32; val >>= 32; }
    if ((val & 0xFFFFULL) == 0) { count += 16; val >>= 16; }
    if ((val & 0xFFULL) == 0) { count += 8; val >>= 8; }
    if ((val & 0xFULL) == 0) { count += 4; val >>= 4; }
    if ((val & 0x3ULL) == 0) { count += 2; val >>= 2; }
    if ((val & 0x1ULL) == 0) { count += 1; }
    return count;
}
#define ctzll(x) custom_ctzll(x)
#endif

// Greatest Common Divisor using Stein's binary algorithm (branchless/efficient, no recursion)
static int64_t gcd_binary(int64_t a, int64_t b) {
    if (a == 0) return b < 0 ? -b : b;
    if (b == 0) return a < 0 ? -a : a;
    
    // Make absolute
    uint64_t ua = a < 0 ? -a : a;
    uint64_t ub = b < 0 ? -b : b;
    
    int shift = ctzll(ua | ub);
    ua >>= ctzll(ua);
    
    do {
        ub >>= ctzll(ub);
        if (ua > ub) {
            uint64_t temp = ua;
            ua = ub;
            ub = temp;
        }
        ub -= ua;
    } while (ub != 0);
    
    return (int64_t)(ua << shift);
}

void numeric_simplify_rational(RationalValue* r) {
    if (r->den == 0) {
        // Leave division-by-zero indicators as is or set canonical
        return;
    }
    if (r->num == 0) {
        r->den = 1;
        return;
    }
    
    int64_t g = gcd_binary(r->num, r->den);
    if (g > 1) {
        r->num /= g;
        r->den /= g;
    }
    
    // Ensure denominator is positive
    if (r->den < 0) {
        r->num = -r->num;
        r->den = -r->den;
    }
}

NumericValue numeric_from_real(double val) {
    NumericValue nv;
    nv.type = NUMERIC_REAL;
    nv.value.real = val;
    return nv;
}

NumericValue numeric_from_complex(double real, double imag) {
    NumericValue nv;
    nv.type = NUMERIC_COMPLEX;
    nv.value.complex.real = real;
    nv.value.complex.imag = imag;
    return nv;
}

NumericValue numeric_from_rational(int64_t num, int64_t den) {
    NumericValue nv;
    nv.type = NUMERIC_RATIONAL;
    nv.value.rational.num = num;
    nv.value.rational.den = den;
    numeric_simplify_rational(&nv.value.rational);
    return nv;
}
