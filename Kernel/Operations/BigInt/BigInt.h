/*
 * File: BigInt.h
 * Author: W. Kowal (with spirit of K&R)
 * Description: Arbitrary precision integer arithmetic no heap, all stack.
 * MAX_BIGINT_BITS supports up to ~1232 decimal digits (4096 bits).
 * Why 4096 bits Because RSA-4096 is the standard, that's why.
 * 
 * THE FLEX: Python's big ints are 1MB of C. Ours is 200 lines of bare metal.
 */

#ifndef BIGINT_H
#define BIGINT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 64 limbs 64 bits = 4096 bits = ~1232 decimal digits
#ifdef CALC_TINY_MEM
#define BIGINT_MAX_LIMBS 4   // 256-bit max for 16KB RAM target!
#else
#define BIGINT_MAX_LIMBS 64  // 4096-bit standard
#endif
#define BIGINT_BITS (BIGINT_MAX_LIMBS * 64)

// A single 'digit' is a 64-bit unsigned integer (limb)
typedef uint64_t bigint_limb_t;

/*
 * Representation: little-endian limb array
 * value = sum(limbs[i] * 2^(64*i)) for i in [0, len)
 * sign is stored separately as a boolean.
 * 'len' indicates the number of limbs actually used (most significant limb != 0).
 * If value is zero, len == 1 and limbs[0] == 0.
 * 
 * WHY LITTLE-ENDIAN Because addition/subtraction propagate carry forward (index 0N).
 * That means process forward in memory. Cache prefetch loves this.
 */
typedef struct {
    bigint_limb_t limbs[BIGINT_MAX_LIMBS];  // 512 bytes one cache line is 64B, this is 8 cache lines
    uint32_t len;                            // Number of limbs in use (at least 1)
    bool negative;                           // Sign flag
} BigInt;

// ============================================================
// INITIALIZATION
// ============================================================

// Set BigInt to zero (fast just clears the first limb and sets len=1)
void bigint_zero(BigInt* a);

// Create a BigInt from a uint64_t value
void bigint_from_u64(BigInt* a, uint64_t val);

// Create a BigInt from an int64_t value
void bigint_from_i64(BigInt* a, int64_t val);

// ============================================================
// COMPARISON
// ============================================================

// Returns -1 if a < b, 0 if equal, 1 if a > b
int bigint_compare(const BigInt* a, const BigInt* b);

// Returns true if a and b have the same magnitude and sign
bool bigint_equal(const BigInt* a, const BigInt* b);

// Returns true if |a| == 0
bool bigint_is_zero(const BigInt* a);

// ============================================================
// ARITHMETIC all operations store result in 'result' parameter
//            (may alias input parameters, e.g. bigint_add(a, b, a))
// ============================================================

// result = a + b
void bigint_add(const BigInt* a, const BigInt* b, BigInt* result);

// result = a - b  (handles sign automatically)
void bigint_sub(const BigInt* a, const BigInt* b, BigInt* result);

// result = a * b  (schoolbook multiplication O(n*m))
void bigint_mul(const BigInt* a, const BigInt* b, BigInt* result);

// result = a / b, remainder = a % b
// Returns false if division by zero (b == 0)
bool bigint_div_mod(const BigInt* a, const BigInt* b, BigInt* quotient, BigInt* remainder);

// Convenience: result = a / b
static inline bool bigint_div(const BigInt* a, const BigInt* b, BigInt* result) {
    BigInt rem;
    return bigint_div_mod(a, b, result, &rem);
}

// Convenience: result = a % b
static inline bool bigint_mod(const BigInt* a, const BigInt* b, BigInt* result) {
    BigInt quot;
    return bigint_div_mod(a, b, &quot, result);
}

// result = a^exp  (exponentiation by squaring)
void bigint_pow(const BigInt* a, uint64_t exp, BigInt* result);

// result = a!  (factorial)
void bigint_factorial(uint64_t n, BigInt* result);

// ============================================================
// STRING CONVERSION
// ============================================================

// Convert BigInt to decimal string. Returns the number of characters written (excluding null).
// If buffer is too small, returns the required size (like snprintf).
// Maximum decimal digits for 4096 bits: ceil(4096 * log10(2)) 1234, plus sign + null = 1236
#define BIGINT_STR_MAX 1236
uint32_t bigint_to_string(const BigInt* a, char* buffer, uint32_t buffer_size);

// Parse decimal string into BigInt.
// Returns false if the string contains invalid characters.
bool bigint_from_string(BigInt* result, const char* str, uint32_t len);

// ============================================================
// UTILITY
// ============================================================

// Normalize: trim leading zero limbs and ensure len >= 1
void bigint_normalize(BigInt* a);

// Absolute value: result = |a|
void bigint_abs(const BigInt* a, BigInt* result);

// Negate: result = -a
void bigint_negate(const BigInt* a, BigInt* result);

// GCD: result = gcd(a, b) using binary GCD (Stein's algorithm) on big ints
void bigint_gcd(const BigInt* a, const BigInt* b, BigInt* result);

// Check if BigInt is probably prime (Miller-Rabin). Returns false for composite, true for probably prime.
// Only meaningful for values that fit in uint64_t; larger values use trial division.
bool bigint_is_prime(const BigInt* a);

#endif // BIGINT_H
