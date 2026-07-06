#ifndef NUMBER_THEORY_H
#define NUMBER_THEORY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../State/CalculatorState.h"

// Greatest Common Divisor (GCD) using Euclidean Algorithm
uint64_t gcd(uint64_t a, uint64_t b);

// Least Common Multiple (LCM)
uint64_t lcm(uint64_t a, uint64_t b);

// Modular Exponentiation: (base^exp) % mod
uint64_t mod_exp(uint64_t base, uint64_t exp, uint64_t mod);

// Primality test using trial division (up to 65536) and Miller-Rabin above that
bool is_prime(uint64_t n);

// Euler's totient φ(n): count of integers in [1,n] coprime to n.
// Returns 1 for n==1, 0 for n==0.
uint64_t euler_totient(uint64_t n);

// Extended Euclidean algorithm: finds x, y such that a*x + b*y = gcd(a,b).
// Writes gcd into *gcd_out. x and y may be NULL to skip coefficient output.
// Returns false if both a and b are 0.
bool extended_gcd(uint64_t a, uint64_t b, int64_t *x, int64_t *y, uint64_t *gcd_out);

// Modular multiplicative inverse of a mod m (a^-1 mod m).
// Returns false if gcd(a,m) != 1 or m <= 1 (inverse does not exist).
bool mod_inverse(uint64_t a, uint64_t m, uint64_t *result);

// Chinese Remainder Theorem: find x such that x ≡ remainders[i] (mod moduli[i]).
// Moduli must be pairwise coprime. count must be in [1, 8].
// Returns false on coprimality violation, count==0, or count>8.
bool chinese_remainder(const uint64_t *remainders, const uint64_t *moduli,
                       uint32_t count, uint64_t *result);

// Returns the smallest prime strictly greater than n.
// Returns 0 if the search overflows uint64 without finding a prime.
uint64_t next_prime(uint64_t n);

#endif // NUMBER_THEORY_H
