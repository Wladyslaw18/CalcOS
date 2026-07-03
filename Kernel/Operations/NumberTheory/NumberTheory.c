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

    for (uint32_t i = 0; i < sizeof(bases)/sizeof(bases[0]); i++) {
        uint64_t a = bases[i];
        if (n <= a) break; // base >= n is invalid for Miller-Rabin
        if (miller_rabin_witness(a, d, s, n)) return false;
    }

    return true;
}
