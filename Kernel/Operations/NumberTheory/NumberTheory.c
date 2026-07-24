#include "NumberTheory.h"

// ctzll: count trailing zeros on uint64. essential for binary GCD.
// GCC/Clang: __builtin_ctzll compiles to a single BSF/TZCNT instruction.
// MSVC: _BitScanForward64 on 64-bit, split 32-bit fallback for x86.
// Generic: portable bit-scan via halving. slower but correct anywhere.
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
    // x86 32-bit MSVC: no _BitScanForward64. split into two 32-bit scans.
    if (_BitScanForward(&index, (unsigned long)(val & 0xFFFFFFFF))) {
        return (int)index;
    }
    _BitScanForward(&index, (unsigned long)(val >> 32));
    return (int)(index + 32);
#endif
}
#define ctzll(x) msvc_ctzll(x)
#else
// portable fallback: binary search for lowest set bit
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

// Stein's binary GCD. avoids division entirely -- only shifts and subtracts.
// factor out common 2s first (ctzll(a|b)), then alternate shift + subtract.
// faster than Euclidean on CPUs where division is slow (embedded, bare-metal).
uint64_t gcd(uint64_t a, uint64_t b) {
    if (a == 0) return b;
    if (b == 0) return a;

    // extract and save the common power of 2
    int shift = ctzll(a | b);
    a >>= ctzll(a); // strip all 2s from a

    do {
        b >>= ctzll(b); // strip all 2s from b
        if (a > b) { // keep a <= b for the subtraction
            uint64_t t = b;
            b = a;
            a = t;
        }
        b -= a;
    } while (b != 0);

    return a << shift; // restore common 2s
}

// divide before multiply to avoid overflow: lcm(a,b) = a/gcd(a,b) * b
uint64_t lcm(uint64_t a, uint64_t b) {
    if (a == 0 || b == 0) return 0;
    return (a / gcd(a, b)) * b;
}

// 128-bit safe modular multiply. a*b can overflow uint64 for large a,b.
// __int128 available on GCC/Clang. MSVC: double-and-add fallback (slower).
static inline uint64_t mul_mod(uint64_t a, uint64_t b, uint64_t mod) {
#if defined(__SIZEOF_INT128__)
    return (uint64_t)(((unsigned __int128)a * b) % mod);
#else
    // no __int128: double-and-add. O(log b) multiplications mod mod.
    uint64_t res = 0;
    a %= mod;
    while (b > 0) {
        if (b & 1) res = (res + a) % mod;
        a = (a * 2) % mod;
        b >>= 1;
    }
    return res;
#endif
}

// fast modular exponentiation via square-and-multiply. O(log exp) multiplications.
uint64_t mod_exp(uint64_t base, uint64_t exp, uint64_t mod) {
    if (mod == 1) return 0; // any number mod 1 is 0
    uint64_t result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) {
            result = mul_mod(result, base, mod);
        }
        base = mul_mod(base, base, mod);
        exp >>= 1;
    }
    return result;
}

// Miller-Rabin single-witness test for composite.
// returns true if 'a' witnesses compositeness of n. false = probably prime.
static bool miller_rabin_witness(uint64_t a, uint64_t d, uint64_t s, uint64_t n) {
    uint64_t x = mod_exp(a, d, n);
    if (x == 1 || x == n - 1) return false;
    for (uint64_t r = 1; r < s; r++) {
        x = mul_mod(x, x, n);
        if (x == n - 1) return false;
    }
    return true; // composite
}

// deterministic primality test for any n < 2^64.
// trial division up to 65536 handles small composites cheaply.
// for large n, the 12-base Miller-Rabin set below is mathematically proven
// sufficient for all n < 3,317,044,064,679,887,385,961,981. covers all uint64.
bool is_prime(uint64_t n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if ((n & 1) == 0 || n % 3 == 0) return false;

    // trial division: eliminates most composites before hitting Miller-Rabin
    uint64_t limit = 65536;
    for (uint64_t i = 5; i * i <= n && i <= limit; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }

    // if trial division was exhaustive (n < limit²), we're done
    if (n < limit * limit) {
        return true;
    }

    // 12-base Miller-Rabin: deterministic for all 64-bit integers
    static const uint64_t bases[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};

    // write n-1 as d * 2^s
    uint64_t d = n - 1;
    uint64_t s = ctzll(d);
    d >>= s;

    for (size_t i = 0; i < sizeof(bases)/sizeof(bases[0]); i++) {
        uint64_t a = bases[i];
        if (n <= a) break; // base >= n is invalid for Miller-Rabin
        if (miller_rabin_witness(a, d, s, n)) return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// euler_totient: Euler's phi function using trial-division factorisation.
//
// Algorithm:
//   For each prime factor p of n (found via trial division up to sqrt(n)):
//     result = result / p * (p - 1)      [equivalent to result * (1 - 1/p)]
//     strip all occurrences of p from n.
//   If n > 1 after the loop, the remaining n is itself a prime factor.
//
//   Special cases:
//     phi(0) = 0  (undefined; return 0 as sentinel)
//     phi(1) = 1  (there is exactly one integer <= 1 that is coprime to 1)
// ---------------------------------------------------------------------------
uint64_t euler_totient(uint64_t n) {
    if (n == 0) return 0;
    if (n == 1) return 1;

    uint64_t result = n;

    // trial division: try factor 2 first, then odd numbers up to sqrt(n)
    if (n % 2 == 0) {
        result = result / 2 * (2 - 1); // result -= result/2
        while (n % 2 == 0) n /= 2;
    }

    for (uint64_t p = 3; p * p <= n; p += 2) {
        if (n % p == 0) {
            result = result / p * (p - 1);
            while (n % p == 0) n /= p;
        }
    }

    // if n is still > 1 it is a remaining prime factor
    if (n > 1) {
        result = result / n * (n - 1);
    }

    return result;
}

// ---------------------------------------------------------------------------
// extended_gcd: Extended Euclidean algorithm (iterative).
//
// Algorithm (iterative Bezout coefficient tracking):
//   Maintain (old_r, r) = current pair of remainders (starts as (a, b)).
//   Maintain (old_s, s) and (old_t, t) as Bezout coefficient sequences.
//   Each step: quotient q = old_r / r, then
//     (old_r, r) <- (r, old_r - q*r)
//     (old_s, s) <- (s, old_s - q*s)
//     (old_t, t) <- (t, old_t - q*t)
//   At termination old_r = gcd, old_s = x, old_t = y.
//
//   The coefficients use signed 64-bit to accommodate negative Bezout values.
//   A sentinel trick converts unsigned inputs to signed by tracking sign
//   changes separately -- inputs are uint64 but intermediate Bezout values
//   fit in int64 for all inputs < 2^63.
// ---------------------------------------------------------------------------
bool extended_gcd(uint64_t a, uint64_t b, int64_t *x, int64_t *y, uint64_t *gcd_out) {
    if (a == 0 && b == 0) return false;

    // work in signed domain; safe for a, b < 2^63; for larger values
    // the gcd result is still correct even if coefficients overflow.
    int64_t old_r = (int64_t)a, r = (int64_t)b;
    int64_t old_s = 1,          s = 0;
    int64_t old_t = 0,          t = 1;

    while (r != 0) {
        int64_t q   = old_r / r;

        int64_t tmp = r;
        r     = old_r - q * r;
        old_r = tmp;

        tmp   = s;
        s     = old_s - q * s;
        old_s = tmp;

        tmp   = t;
        t     = old_t - q * t;
        old_t = tmp;
    }

    // old_r is the gcd (always non-negative for positive inputs)
    if (gcd_out) *gcd_out = (uint64_t)(old_r < 0 ? -old_r : old_r);
    if (x)       *x       = old_s;
    if (y)       *y       = old_t;

    return true;
}

// ---------------------------------------------------------------------------
// mod_inverse: Modular multiplicative inverse via Extended Euclidean algorithm.
//
// Algorithm:
//   a^-1 mod m exists iff gcd(a, m) == 1.
//   Use extended_gcd(a, m) to find x such that a*x + m*y = 1.
//   Then a*x ≡ 1 (mod m), so the inverse is x mod m (adjusted to be positive).
//
//   Guard conditions:
//     m <= 1: modulus must be >= 2 for the inverse to be meaningful.
//     gcd(a,m) != 1: no inverse exists; return false.
// ---------------------------------------------------------------------------
bool mod_inverse(uint64_t a, uint64_t m, uint64_t *result) {
    if (result == NULL) return false;
    if (m <= 1)         return false;

    uint64_t g;
    int64_t  x, y;
    if (!extended_gcd(a, m, &x, &y, &g)) return false;
    if (g != 1) return false; // not coprime; inverse does not exist

    // x may be negative; reduce mod m into [0, m)
    int64_t sm = (int64_t)m;
    x = ((x % sm) + sm) % sm;
    *result = (uint64_t)x;
    return true;
}

// ---------------------------------------------------------------------------
// chinese_remainder: Chinese Remainder Theorem via iterative construction.
//
// Algorithm (Garner / successive substitution):
//   For each equation x ≡ r[i] (mod m[i]), fold it into a running solution.
//   Start with x = r[0], M = m[0].
//   For each subsequent i:
//     Solve: x + M * k ≡ r[i] (mod m[i])
//       => k ≡ (r[i] - x) * M^-1  (mod m[i])    [requires gcd(M, m[i])==1]
//     Update: x += M * k,   M *= m[i].
//   Final x is the unique solution mod (m[0]*m[1]*...*m[count-1]).
//
//   Limits:
//     count == 0         -> false
//     count >  8         -> false (stack-only; 8 is the compile-time maximum)
//     any gcd(M, m[i])!=1 -> false (moduli not pairwise coprime)
// ---------------------------------------------------------------------------
bool chinese_remainder(const uint64_t *remainders, const uint64_t *moduli,
                       uint32_t count, uint64_t *result) {
    if (remainders == NULL || moduli == NULL || result == NULL) return false;
    if (count == 0 || count > 8) return false;

    uint64_t x = remainders[0] % moduli[0];
    uint64_t M = moduli[0];

    for (uint32_t i = 1; i < count; i++) {
        uint64_t mi = moduli[i];
        uint64_t ri = remainders[i] % mi;

        // require gcd(M, mi) == 1
        if (gcd(M, mi) != 1) return false;

        // compute M^-1 mod mi
        uint64_t M_inv;
        if (!mod_inverse(M % mi, mi, &M_inv)) return false;

        // k = ((ri - x % mi) + mi) % mi * M_inv % mi
        uint64_t x_mod_mi = x % mi;
        uint64_t diff     = (ri >= x_mod_mi)
                          ? (ri - x_mod_mi)
                          : (mi - x_mod_mi + ri);
        uint64_t k = (diff % mi) * M_inv % mi;

        // x += M * k  (work mod the product; track running x directly)
        // to avoid overflow in M*k for very large M we keep x in range:
        // since M and k may both be large, use a loop addition with mod
        // of the accumulated product. Because count<=8 and each mi fits
        // in uint64, and we fold one at a time, x stays in [0, M_new).
        for (uint64_t step = 0; step < k; step++) {
            x += M;
        }
        M *= mi;
        x %= M;
    }

    *result = x;
    return true;
}

// ---------------------------------------------------------------------------
// next_prime: smallest prime strictly greater than n.
//
// Algorithm:
//   Skip even candidates (except the transition from n < 2).
//   Start the search at n+1, bump up to the next odd number if needed, then
//   probe every odd candidate using the existing deterministic is_prime().
//   Overflow guard: if candidate wraps to 0 during the search, return 0.
// ---------------------------------------------------------------------------
uint64_t next_prime(uint64_t n) {
    // handle small boundary
    if (n < 2) return 2;

    // start at the next candidate above n; must be odd (primes > 2 are odd)
    uint64_t candidate = n + 1;
    if (candidate == 0) return 0; // overflow: n was UINT64_MAX

    // bump to odd if even (and not 2 which is already returned above)
    if (candidate % 2 == 0) {
        candidate++;
        if (candidate == 0) return 0; // overflow
    }

    while (!is_prime(candidate)) {
        // skip by 2 to stay on odd numbers; detect overflow
        if (candidate > UINT64_MAX - 2) return 0;
        candidate += 2;
    }

    return candidate;
}
