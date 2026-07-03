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

#endif // NUMBER_THEORY_H
