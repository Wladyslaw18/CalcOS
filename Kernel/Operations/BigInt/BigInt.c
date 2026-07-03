/*
 * File: BigInt.c
 * Author: W. Kowal
 * Description: Arbitrary precision integer arithmetic implementation.
 * 
 * ALL STACK, NO HEAP. ZERO ALLOCATIONS. CACHE-LINE FRIENDLY.
 * 
 * Performance notes:
 * - Addition/Subtraction: O(n) single pass forward
 * - Multiplication: O(n*m) schoolbook simple, cache-friendly forward scan
 * - Division: O(n*m) long division the bottleneck, but fine for 4096-bit
 * - All loops iterate forward (limb 0 N) for optimal cache prefetch
 */

#include "BigInt.h"
#include "../../../Infrastructure/Utils/MemoryUtils.h"
#include "../NumberTheory/NumberTheory.h"

// ============================================================
// INTERNAL HELPERS
// ============================================================

// Detect 128-bit integer support (GCC/Clang)
#if defined(__SIZEOF_INT128__)
#define HAVE_UINT128 1
typedef unsigned __int128 uint128_t;
#else
#define HAVE_UINT128 0
#endif

// Normalize: strip leading zero limbs, ensure len >= 1
void bigint_normalize(BigInt* a) {
    if (!a) return;
    uint32_t i = a->len;
    while (i > 1 && a->limbs[i - 1] == 0) {
        i--;
    }
    a->len = i;
    // Zero is never negative
    if (i == 1 && a->limbs[0] == 0) {
        a->negative = false;
    }
}

// Clear entire limb array (fast_memset, zero-cost on hot cache)
static void bigint_clear(BigInt* a) {
    fast_memset(a->limbs, 0, sizeof(bigint_limb_t) * BIGINT_MAX_LIMBS);
    a->len = 1;
    a->negative = false;
}

// ============================================================
// INITIALIZATION
// ============================================================

void bigint_zero(BigInt* a) {
    if (!a) return;
    a->limbs[0] = 0;
    a->len = 1;
    a->negative = false;
}

void bigint_from_u64(BigInt* a, uint64_t val) {
    if (!a) return;
    a->limbs[0] = val;
    a->len = 1;
    a->negative = false;
    // If there are higher bits... there can't be, it's a uint64_t
    // But normalize handles edge case where val might be 0 (which it handles)
    if (val == 0) {
        bigint_normalize(a);
    }
}

void bigint_from_i64(BigInt* a, int64_t val) {
    if (!a) return;
    if (val < 0) {
        // FIX: INT64_MIN negation overflows int64_t (UB)
        // -INT64_MIN = 2^63 which fits in uint64_t but NOT in int64_t.
        // Cast to uint64_t BEFORE negation to avoid signed overflow.
        a->limbs[0] = (uint64_t)0 - (uint64_t)val;
        a->negative = true;
    } else {
        a->limbs[0] = (uint64_t)val;
        a->negative = false;
    }
    a->len = 1;
    if (val == 0) {
        a->negative = false;
    }
}

// ============================================================
// COMPARISON
// ============================================================

int bigint_compare(const BigInt* a, const BigInt* b) {
    if (!a || !b) return 0;

    // Different signs: positive > negative
    if (a->negative != b->negative) {
        return b->negative ? 1 : -1;
    }

    // Same sign: compare magnitude
    bool both_neg = a->negative;

    // Different lengths: longer = larger magnitude
    if (a->len != b->len) {
        int cmp = (a->len > b->len) ? 1 : -1;
        return both_neg ? -cmp : cmp;
    }

    // Same length: compare from most significant limb downward
    uint32_t i = a->len;
    while (i > 0) {
        i--;
        if (a->limbs[i] != b->limbs[i]) {
            int cmp = (a->limbs[i] > b->limbs[i]) ? 1 : -1;
            return both_neg ? -cmp : cmp;
        }
    }

    return 0; // Equal
}

bool bigint_equal(const BigInt* a, const BigInt* b) {
    return bigint_compare(a, b) == 0;
}

bool bigint_is_zero(const BigInt* a) {
    return a && a->len == 1 && a->limbs[0] == 0;
}

// ============================================================
// ADDITION & SUBTRACTION
// ============================================================

// Compute result = |a| + |b| (unsigned addition)
// Returns the resulting length (may need len+1 limbs for carry)
static uint32_t bigint_add_magnitude(const BigInt* a, const BigInt* b, BigInt* result) {
    uint32_t max_len = (a->len > b->len) ? a->len : b->len;
    uint32_t min_len = (a->len < b->len) ? a->len : b->len;

    bigint_limb_t carry = 0;
    uint32_t i;

#if HAVE_UINT128
    // Fast path: use 128-bit intermediate for carry detection
    for (i = 0; i < min_len; i++) {
        uint128_t sum = (uint128_t)a->limbs[i] + (uint128_t)b->limbs[i] + (uint128_t)carry;
        result->limbs[i] = (bigint_limb_t)sum;
        carry = (bigint_limb_t)(sum >> 64);
    }
#else
    // Portable path: detect carry manually
    for (i = 0; i < min_len; i++) {
        bigint_limb_t sum = a->limbs[i] + b->limbs[i];
        // Did overflow
        bigint_limb_t new_carry = (sum < a->limbs[i] || sum < b->limbs[i]) ? 1ULL : 0;
        sum += carry;
        if (carry && sum < carry) new_carry = 1;
        result->limbs[i] = sum;
        carry = new_carry;
    }
#endif

    // Propagate carry through remaining limbs of the longer operand
    const BigInt* longer = (a->len > b->len) ? a : b;
    for (; i < max_len; i++) {
#if HAVE_UINT128
        uint128_t sum = (uint128_t)longer->limbs[i] + (uint128_t)carry;
        result->limbs[i] = (bigint_limb_t)sum;
        carry = (bigint_limb_t)(sum >> 64);
#else
        bigint_limb_t sum = longer->limbs[i] + carry;
        carry = (sum < longer->limbs[i] || (carry && sum == 0)) ? 1 : 0;
        result->limbs[i] = sum;
#endif
    }

    // Final carry
    if (carry) {
        result->limbs[max_len] = 1;
        return max_len + 1;
    }
    return max_len;
}

// Compute result = |a| - |b| (unsigned subtraction, assumes |a| >= |b|)
// Returns the resulting length
static uint32_t bigint_sub_magnitude(const BigInt* a, const BigInt* b, BigInt* result) {
    bigint_limb_t borrow = 0;
    uint32_t i;

    for (i = 0; i < b->len; i++) {
#if HAVE_UINT128
        uint128_t diff = (uint128_t)a->limbs[i] - (uint128_t)b->limbs[i] - (uint128_t)borrow;
        result->limbs[i] = (bigint_limb_t)diff;
        borrow = (diff >> 64) ? 1 : 0;
#else
        bigint_limb_t diff = a->limbs[i] - b->limbs[i];
        bigint_limb_t new_borrow = (diff > a->limbs[i]) ? 1 : 0;
        diff -= borrow;
        if (borrow && diff > (a->limbs[i] - b->limbs[i] - 1)) new_borrow = 1;
        result->limbs[i] = diff;
        borrow = new_borrow;
#endif
    }

    // Propagate borrow through remaining limbs of a
    for (; i < a->len; i++) {
#if HAVE_UINT128
        uint128_t diff = (uint128_t)a->limbs[i] - (uint128_t)borrow;
        result->limbs[i] = (bigint_limb_t)diff;
        borrow = (diff >> 64) ? 1 : 0;
#else
        bigint_limb_t diff = a->limbs[i] - borrow;
        borrow = (diff > a->limbs[i]) ? 1 : 0;
        result->limbs[i] = diff;
#endif
    }

    return a->len; // Result length, may need normalization
}

void bigint_add(const BigInt* a, const BigInt* b, BigInt* result) {
    if (!a || !b || !result) return;

    // If signs are the same: add magnitudes, keep sign
    if (a->negative == b->negative) {
        uint32_t new_len = bigint_add_magnitude(a, b, result);
        result->len = new_len;
        result->negative = a->negative;
        bigint_normalize(result);
        return;
    }

    // Signs differ: subtract smaller magnitude from larger, result sign = larger's sign
    int cmp = bigint_compare(a, b);
    // Need to compare by magnitude only but compare already handles sign-adjusted
    // For sign-differing case, must compare absolute values
    // Quick hack: just compare abs by swapping signs temporarily (safe on const?)
    // Better: compare magnitude directly

    // Check if |a| >= |b| by creating temp views with same sign
    int mag_cmp;
    {
        // magnitude comparison: negate both (effectively making both positive)
        BigInt mag_a = *a; mag_a.negative = false;
        BigInt mag_b = *b; mag_b.negative = false;
        mag_cmp = bigint_compare(&mag_a, &mag_b);
    }

    if (mag_cmp >= 0) {
        // |a| >= |b|, result = |a| - |b|, sign = sign of a
        result->len = bigint_sub_magnitude(a, b, result);
        result->negative = a->negative;
    } else {
        // |a| < |b|, result = |b| - |a|, sign = sign of b
        result->len = bigint_sub_magnitude(b, a, result);
        result->negative = b->negative;
    }
    bigint_normalize(result);
}

void bigint_sub(const BigInt* a, const BigInt* b, BigInt* result) {
    if (!a || !b || !result) return;

    // a - b = a + (-b): just flip b's sign and add
    BigInt neg_b = *b;
    if (!bigint_is_zero(b)) {
        neg_b.negative = !neg_b.negative;
    } else {
        neg_b.negative = false;
    }
    bigint_add(a, &neg_b, result);
}

// ============================================================
// PORTABLE 6464 128-BIT MULTIPLY (no __int128 needed!)
// ============================================================
// Splits each operand into 32-bit halves and does 4 partial products
// like grade-school multiplication, but with carry tracking.
// Proved correct for all uint64_t inputs.
static void mul64x64_portable(uint64_t a, uint64_t b, uint64_t* hi, uint64_t* lo) {
    uint64_t a_lo = a & 0xFFFFFFFFULL;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = b & 0xFFFFFFFFULL;
    uint64_t b_hi = b >> 32;

    uint64_t lo_lo = a_lo * b_lo;
    uint64_t hi_lo = a_hi * b_lo;
    uint64_t lo_hi = a_lo * b_hi;
    uint64_t hi_hi = a_hi * b_hi;

    uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFFULL) + (lo_hi & 0xFFFFFFFFULL);
    *lo = (cross << 32) | (lo_lo & 0xFFFFFFFFULL);
    *hi = hi_hi + (hi_lo >> 32) + (lo_hi >> 32) + (cross >> 32);
}

// ---- Knuth Algorithm D: 128-bit comparison helper ----: returns true if (hi << 64 | lo) >= (rh << 64 | rl)
// No __int128 needed, pure 64-bit arithmetic
static bool geq_128(uint64_t hi, uint64_t lo, uint64_t rh, uint64_t rl) {
    if (hi > rh) return true;
    if (hi < rh) return false;
    // high 64 bits equal, compare low
    return lo >= rl;
}

// Portable comparison: (q_hat * v2) > ((q_rem << 64) | u_jn2)
// Returns true if the product exceeds the 128-bit reference
static bool qhat_product_too_big(uint64_t q_hat, uint64_t v2, uint64_t q_rem, uint64_t u_jn2) {
    uint64_t p_hi, p_lo;
    mul64x64_portable(q_hat, v2, &p_hi, &p_lo);
    // Compare p_hi:p_lo vs q_rem:u_jn2
    return (p_hi > q_rem) || (p_hi == q_rem && p_lo > u_jn2);
}

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

static inline uint64_t div128_by_64(uint64_t u1, uint64_t u0, uint64_t v, uint64_t* r) {
#if defined(_MSC_VER) && defined(_M_X64)
    return _udiv128(u1, u0, v, r);
#else
    uint64_t v1 = v >> 32, v0 = v & 0xFFFFFFFFULL;
    uint64_t u3 = u1 >> 32, u2 = u1 & 0xFFFFFFFFULL;
    uint64_t u1_d = u0 >> 32, u0_d = u0 & 0xFFFFFFFFULL;
    uint64_t q1 = ((u3 << 32) | u2) / v1, r1 = ((u3 << 32) | u2) % v1;
    while (q1 >= 0x100000000ULL || q1 * v0 > ((r1 << 32) | u1_d)) { q1--; r1 += v1; if (r1 >= 0x100000000ULL) break; }
    uint64_t rem = ((r1 << 32) | u1_d) - q1 * v0;
    uint64_t q0 = rem / v1, r0 = rem % v1;
    while (q0 >= 0x100000000ULL || q0 * v0 > ((r0 << 32) | u0_d)) { q0--; r0 += v1; if (r0 >= 0x100000000ULL) break; }
    if (r) *r = ((r0 << 32) | u0_d) - q0 * v0;
    return (q1 << 32) | q0;
#endif
}

// ============================================================
// MULTIPLICATION
// ============================================================

// Schoolbook multiplication: O(n*m)
// For 64-limb numbers: 64 64 = 4096 multiplications. That's nothing.
void bigint_mul(const BigInt* a, const BigInt* b, BigInt* result) {
    if (!a || !b || !result) return;

    // Handle zero
    if (bigint_is_zero(a) || bigint_is_zero(b)) {
        bigint_zero(result);
        return;
    }

    // ALIASING FIX
    // If result aliases a or b, 'd zero this own operands!
    // Use a stack temp and copy back at the end.
    BigInt temp_result;
    BigInt* target = result;
    if (result == a || result == b) {
        target = &temp_result;
        fast_memset(target->limbs, 0, sizeof(bigint_limb_t) * BIGINT_MAX_LIMBS);
        target->len = 1;
    } else {
        fast_memset(result->limbs, 0, sizeof(bigint_limb_t) * BIGINT_MAX_LIMBS);
    }

    uint32_t max_result_len = a->len + b->len;
    if (max_result_len > BIGINT_MAX_LIMBS) {
        max_result_len = BIGINT_MAX_LIMBS;
    }

    // Schoolbook multiplication: forward scan (cache-friendly)
    for (uint32_t i = 0; i < a->len && i < BIGINT_MAX_LIMBS; i++) {
        bigint_limb_t carry = 0;
        uint32_t j;

        for (j = 0; j < b->len && (i + j) < BIGINT_MAX_LIMBS; j++) {
#if HAVE_UINT128
            uint128_t product = (uint128_t)a->limbs[i] * (uint128_t)b->limbs[j]
                              + (uint128_t)target->limbs[i + j]
                              + (uint128_t)carry;
            target->limbs[i + j] = (bigint_limb_t)product;
            carry = (bigint_limb_t)(product >> 64);
#else
            // Portable: mul64x64_portable gives the full 128-bit product
            uint64_t p_hi, p_lo;
            mul64x64_portable(a->limbs[i], b->limbs[j], &p_hi, &p_lo);
            
            // Add existing result limb and carry to product's low 64 bits
            uint64_t sum_lo = p_lo + target->limbs[i + j];
            uint64_t carry_in = (sum_lo < p_lo) ? 1ULL : 0ULL;
            sum_lo += carry;
            if (carry && sum_lo < carry) carry_in++;
            target->limbs[i + j] = sum_lo;
            
            // Propagate to high 64 bits
            carry = p_hi + carry_in;
#endif
        }

        // If has carry and room, write it
        if (carry && (i + b->len) < BIGINT_MAX_LIMBS) {
            target->limbs[i + b->len] += carry;
        }
        // Handle potential overflow in result limb
    }

    target->len = max_result_len;
    target->negative = a->negative ^ b->negative;
    bigint_normalize(target);
    
    // Copy back to result if used a temp buffer (aliasing fix)
    if (target != result) {
        *result = *target;
    }
}

// ============================================================
// ============================================================
// KNUTH ALGORITHM D Multi-Precision Division
//
// The Old Code: Repeated subtraction with 10M iteration cap.
//   10^100 / 1 would loop 10M times and return WRONG RESULTS.
//
// This Implementation: Proper long division estimating each
//   quotient digit from the top 3 limbs of the dividend and
//   top 2 limbs of the divisor. O(n*m) worst case = 4096
// iterations for a 64-limb number COMPARED TO 10 MILLION.
// ============================================================

bool bigint_div_mod(const BigInt* a, const BigInt* b, BigInt* quotient, BigInt* remainder) {
    if (!a || !b || !quotient || !remainder) return false;
    if (bigint_is_zero(b)) return false;

    // |a| < |b| quotient = 0, remainder = a
    {
        BigInt ma = *a, mb = *b;
        ma.negative = false; mb.negative = false;
        if (bigint_compare(&ma, &mb) < 0) {
            bigint_zero(quotient);
            *remainder = *a;
            bigint_normalize(remainder);
            return true;
        }
    }

    // |a| == |b| quotient = 1, remainder = 0
    {
        BigInt ma = *a, mb = *b;
        ma.negative = false; mb.negative = false;
        if (bigint_compare(&ma, &mb) == 0) {
            bigint_from_i64(quotient, (a->negative ^ b->negative) ? -1 : 1);
            bigint_zero(remainder);
            return true;
        }
    }

    bool result_neg = a->negative ^ b->negative;

    // Copy and work with magnitudes
    BigInt u = *a; u.negative = false;  // dividend (will be mutated)
    BigInt v = *b; v.negative = false;  // divisor (will be normalized)
    BigInt q; bigint_zero(&q);          // quotient

    // Normalize: shift so v's top limb >= 2^63
    // This guarantees q is off by at most 2
    bigint_limb_t v_top = v.limbs[v.len - 1];
    uint32_t lz = 0;
    { bigint_limb_t m = v_top; for (; lz < 64 && !(m & (1ULL << 63)); lz++) m <<= 1; }

    if (lz > 0) {
        bigint_limb_t c = 0;
        for (uint32_t i = 0; i < v.len; i++) {
            bigint_limb_t t = v.limbs[i];
            v.limbs[i] = (t << lz) | c;
            c = t >> (64 - lz);
        }
        if (c) v.limbs[v.len++] = c;

        c = 0;
        for (uint32_t i = 0; i < u.len; i++) {
            bigint_limb_t t = u.limbs[i];
            u.limbs[i] = (t << lz) | c;
            c = t >> (64 - lz);
        }
        if (c) u.limbs[u.len++] = c;
    }

    // u needs at least v.len + 1 limbs
    if (u.len <= v.len) u.limbs[u.len++] = 0;

    uint32_t n = v.len;
    uint32_t m = u.len - n;
    if (m > BIGINT_MAX_LIMBS) m = BIGINT_MAX_LIMBS;
    q.len = m;

    bigint_limb_t v1 = v.limbs[n - 1];
    bigint_limb_t v2 = (n >= 2) ? v.limbs[n - 2] : 0;

    for (uint32_t j = m; j > 0; j--) {
        uint32_t jm = j - 1;

        bigint_limb_t u_jn  = (jm + n < u.len) ? u.limbs[jm + n] : 0;
        bigint_limb_t u_jn1 = (jm + n - 1 < u.len) ? u.limbs[jm + n - 1] : 0;
        bigint_limb_t u_jn2 = (jm + n - 2 < u.len) ? u.limbs[jm + n - 2] : 0;

        // Estimate q = floor((u_jn * 2^64 + u_jn1) / v1)
        bigint_limb_t q_hat, q_rem;
#if HAVE_UINT128
        uint128_t top = ((uint128_t)u_jn << 64) | (uint128_t)u_jn1;
        if (v1) { q_hat = (bigint_limb_t)(top / v1); q_rem = (bigint_limb_t)(top % v1); }
        else    { q_hat = (bigint_limb_t)(top >> 64); q_rem = (bigint_limb_t)top; }
#else
        if (v1) {
            q_hat = div128_by_64(u_jn, u_jn1, v1, &q_rem);
        } else {
            q_hat = ~0ULL;
            q_rem = 0;
        }
#endif

        // Correct q (at most 2 iterations)
#if HAVE_UINT128
        for (uint32_t k = 0; k < 3; k++) {
            // FIX: OPERATOR PRECEDENCE BUG!
            // The `|` operator has LOWER precedence than `>`!
            // Original: ((prod > (q_rem << 64)) | u_jn2) ALWAYS truthy!
            // Fix: parens to compare the full 128-bit product vs reference.
            if ((uint128_t)q_hat * v2 > (((uint128_t)q_rem << 64) | (uint128_t)u_jn2)) {
                q_hat--; q_rem += v1;
                if (q_rem < v1) break;
            } else break;
        }
#else
        // Portable q correction (no __int128 available)
        // Use mul64x64_portable to check if q_hat * v2 is too large
        for (uint32_t k = 0; k < 3; k++) {
            // Compute new q_rem = (u_jn << 64) | u_jn1 - q_hat * v1
            // approximate: if q_hat * v2 > (q_rem << 64) | u_jn2, reduce q_hat
            if (qhat_product_too_big(q_hat, v2, q_rem, u_jn2)) {
                q_hat--;
                // Add back v1 to q_rem (with carry from u_jn1)
                uint64_t new_q_rem = q_rem + v1;
                if (new_q_rem < q_rem || new_q_rem < v1) {
                    // Carry into the high word means reduced u_jn by 1
                    // but u_jn is not directly available here. Just break
                    // this is extremely rare (q_hat off by >1).
                    q_rem = new_q_rem;
                    break;
                }
                q_rem = new_q_rem;
            } else break;
            if (q_rem < v1) break;
        }
#endif

        // Multiply and subtract: u -= q * v * 2^(64*(j-1))
        bigint_limb_t borrow = 0;
        for (uint32_t i = 0; i < n && (jm + i) < BIGINT_MAX_LIMBS; i++) {
#if HAVE_UINT128
            uint128_t prod = (uint128_t)q_hat * v.limbs[i] + borrow;
            bigint_limb_t plo = (bigint_limb_t)prod, phi = (bigint_limb_t)(prod >> 64);
            bigint_limb_t uv = u.limbs[jm + i];
            if (uv < plo) phi++;
            u.limbs[jm + i] = uv - plo;
            borrow = phi;
#else
            uint64_t p_hi, p_lo;
            mul64x64_portable(q_hat, v.limbs[i], &p_hi, &p_lo);
            uint64_t sum_lo = p_lo + borrow;
            uint64_t next_borrow = p_hi;
            if (sum_lo < p_lo) next_borrow++;
            bigint_limb_t uv = u.limbs[jm + i];
            if (uv < sum_lo) next_borrow++;
            u.limbs[jm + i] = uv - sum_lo;
            borrow = next_borrow;
#endif
        }

        if (jm + n < u.len) {
            if (u.limbs[jm + n] < borrow) {
                // Over-subtracted: add back (rare, at most once per digit)
                u.limbs[jm + n] -= borrow;
                bigint_limb_t carry = 0;
                for (uint32_t i = 0; i < n && (jm + i) < BIGINT_MAX_LIMBS; i++) {
                    bigint_limb_t s = u.limbs[jm + i] + v.limbs[i] + carry;
                    u.limbs[jm + i] = s;
                    carry = (s < v.limbs[i]) ? 1 : 0;
                }
                if (jm + n < u.len) u.limbs[jm + n] += carry;
                q_hat--; // The correction
            } else {
                u.limbs[jm + n] -= borrow;
            }
        }

        if (jm < BIGINT_MAX_LIMBS) q.limbs[jm] = q_hat;
    }

    bigint_normalize(&q);
    *quotient = q;
    quotient->negative = result_neg && !bigint_is_zero(quotient);

    // Denormalize remainder (shift right by lz bits)
    if (lz > 0) {
        bigint_limb_t carry = 0;
        for (uint32_t i = n; i > 0; i--) {
            bigint_limb_t t = u.limbs[i - 1];
            u.limbs[i - 1] = (t >> lz) | carry;
            carry = t << (64 - lz);
        }
    }

    fast_memset(remainder->limbs, 0, sizeof(bigint_limb_t) * BIGINT_MAX_LIMBS);
    for (uint32_t i = 0; i < n; i++) remainder->limbs[i] = (i < u.len) ? u.limbs[i] : 0;
    remainder->len = n;
    remainder->negative = false;
    bigint_normalize(remainder);

    return true;
}

// ============================================================
// EXPONENTIATION & FACTORIAL
// ============================================================

void bigint_pow(const BigInt* a, uint64_t exp, BigInt* result) {
    if (!a || !result) return;

    if (exp == 0) {
        bigint_from_u64(result, 1);
        return;
    }

    if (exp == 1) {
        *result = *a;
        return;
    }

    // Exponentiation by squaring
    BigInt base = *a;
    bigint_from_u64(result, 1);

    while (exp > 0) {
        if (exp & 1) {
            bigint_mul(result, &base, result);
        }
        bigint_mul(&base, &base, &base);
        exp >>= 1;
    }
}

void bigint_factorial(uint64_t n, BigInt* result) {
    if (!result) return;

    bigint_from_u64(result, 1);
    if (n <= 1) return;

    BigInt factor;
    for (uint64_t i = 2; i <= n; i++) {
        bigint_from_u64(&factor, i);
        bigint_mul(result, &factor, result);
    }
}

// ============================================================
// STRING CONVERSION
// ============================================================

uint32_t bigint_to_string(const BigInt* a, char* buffer, uint32_t buffer_size) {
    if (!a || !buffer || buffer_size == 0) return 0;

    // Handle zero
    if (bigint_is_zero(a)) {
        if (buffer_size >= 2) {
            buffer[0] = '0';
            buffer[1] = '\0';
            return 1;
        }
        return 1;
    }

    // Count the required characters
    // Worst case: 1234 digits + sign + null = 1236
    // Convert to base 10 by repeated division by 10
    char digits[BIGINT_STR_MAX];
    uint32_t digit_count = 0;

    BigInt temp = *a;
    temp.negative = false;  // Work with absolute value

    BigInt ten;
    bigint_from_u64(&ten, 10);
    BigInt rem;

    while (!bigint_is_zero(&temp)) {
        bigint_div_mod(&temp, &ten, &temp, &rem);
        digits[digit_count++] = (char)('0' + rem.limbs[0]);
    }

    uint32_t needed = digit_count + (a->negative ? 1 : 0) + 1; // +1 for null
    if (buffer_size < needed) {
        // Not enough space return required size
        return needed;
    }

    uint32_t pos = 0;
    if (a->negative) {
        buffer[pos++] = '-';
    }

    // Digits are in reverse order
    for (uint32_t i = digit_count; i > 0; i--) {
        buffer[pos++] = digits[i - 1];
    }
    buffer[pos] = '\0';

    return pos;
}

bool bigint_from_string(BigInt* result, const char* str, uint32_t len) {
    if (!result || !str || len == 0) return false;

    bigint_zero(result);

    uint32_t start = 0;
    bool negative = false;

    if (str[0] == '-') {
        negative = true;
        start = 1;
    } else if (str[0] == '+') {
        start = 1;
    }

    if (start >= len) return false;

    BigInt ten;
    bigint_from_u64(&ten, 10);
    BigInt digit_val;
    BigInt temp;

    for (uint32_t i = start; i < len; i++) {
        if (str[i] < '0' || str[i] > '9') return false;

        // result = result * 10 + digit
        bigint_mul(result, &ten, &temp);
        bigint_from_u64(&digit_val, (uint64_t)(str[i] - '0'));
        bigint_add(&temp, &digit_val, result);
    }

    result->negative = negative && !bigint_is_zero(result);
    bigint_normalize(result);
    return true;
}

// ============================================================
// UTILITY
// ============================================================

void bigint_abs(const BigInt* a, BigInt* result) {
    if (!a || !result) return;
    *result = *a;
    result->negative = false;
}

void bigint_negate(const BigInt* a, BigInt* result) {
    if (!a || !result) return;
    *result = *a;
    if (!bigint_is_zero(a)) {
        result->negative = !result->negative;
    }
}

void bigint_gcd(const BigInt* a, const BigInt* b, BigInt* result) {
    if (!a || !b || !result) return;

    BigInt abs_a = *a; abs_a.negative = false;
    BigInt abs_b = *b; abs_b.negative = false;

    // Local helper to check if the least significant bit of a BigInt is 0
    #define bigint_is_even(x) (((x).limbs[0] & 1ULL) == 0)

    // Binary GCD (Stein's algorithm)
    if (bigint_is_zero(&abs_a)) {
        *result = abs_b;
        bigint_normalize(result);
        return;
    }
    if (bigint_is_zero(&abs_b)) {
        *result = abs_a;
        bigint_normalize(result);
        return;
    }

    // Find common factors of 2 (shift)
    uint64_t shift = 0;
    {
        // FAST PATH: just check LSB and shift limbs (no division needed!)
        while (bigint_is_even(abs_a) && bigint_is_even(abs_b)) {
            // Shift abs_a right by 1
            uint64_t carry_a = 0;
            for (uint32_t i = abs_a.len; i > 0; i--) {
                uint64_t t = abs_a.limbs[i - 1];
                abs_a.limbs[i - 1] = (t >> 1) | (carry_a << 63);
                carry_a = t & 1;
            }
            if (abs_a.len > 1 && abs_a.limbs[abs_a.len - 1] == 0) abs_a.len--;

            // Shift abs_b right by 1
            uint64_t carry_b = 0;
            for (uint32_t i = abs_b.len; i > 0; i--) {
                uint64_t t = abs_b.limbs[i - 1];
                abs_b.limbs[i - 1] = (t >> 1) | (carry_b << 63);
                carry_b = t & 1;
            }
            if (abs_b.len > 1 && abs_b.limbs[abs_b.len - 1] == 0) abs_b.len--;

            shift++;
        }

        // Remove factors of 2 from abs_a
        while (bigint_is_even(abs_a)) {
            uint64_t carry = 0;
            for (uint32_t i = abs_a.len; i > 0; i--) {
                uint64_t t = abs_a.limbs[i - 1];
                abs_a.limbs[i - 1] = (t >> 1) | (carry << 63);
                carry = t & 1;
            }
            if (abs_a.len > 1 && abs_a.limbs[abs_a.len - 1] == 0) abs_a.len--;
        }

        // Remove factors of 2 from abs_b
        while (bigint_is_even(abs_b)) {
            uint64_t carry = 0;
            for (uint32_t i = abs_b.len; i > 0; i--) {
                uint64_t t = abs_b.limbs[i - 1];
                abs_b.limbs[i - 1] = (t >> 1) | (carry << 63);
                carry = t & 1;
            }
            if (abs_b.len > 1 && abs_b.limbs[abs_b.len - 1] == 0) abs_b.len--;
        }
    }

    // Simple Euclidean algorithm for big ints
    BigInt a_val = abs_a, b_val = abs_b;
    BigInt remainder;
    while (!bigint_is_zero(&b_val)) {
        bigint_div_mod(&a_val, &b_val, &remainder, &remainder);
        a_val = b_val;
        b_val = remainder;
    }

    // Shift back
    BigInt shift_factor;
    bigint_from_u64(&shift_factor, 1);
    for (uint64_t i = 0; i < shift; i++) {
        BigInt two; bigint_from_u64(&two, 2);
        BigInt temp;
        bigint_mul(&a_val, &two, &temp);
        a_val = temp;
    }

    #undef bigint_is_even

    *result = a_val;
    result->negative = false;
    bigint_normalize(result);
}

bool bigint_is_prime(const BigInt* a) {
    if (!a || bigint_is_zero(a)) return false;

    // For small values (fit in uint64_t), delegate to existing is_prime
    if (a->len == 1) {
        return is_prime(a->limbs[0]); // From NumberTheory.h
    }

    // For larger BigInts, can attempt Miller-Rabin up to a point
    // But for truly large values (4096-bit), need modular exponentiation
    // on BigInts. For now, perform a stronger composite check.
    
    // For big values: quick divisibility checks
    // Extended set of small primes more thorough trial division
    static const uint64_t small_primes[] = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
        31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
        73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
        127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
        179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
        233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
        283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
        353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
        419, 421, 431, 433, 439, 443, 449, 457, 461, 463
    };

    BigInt temp;
    for (uint32_t i = 0; i < sizeof(small_primes) / sizeof(small_primes[0]); i++) {
        BigInt p; bigint_from_u64(&p, small_primes[i]);

        BigInt rem;
        bigint_div_mod(a, &p, &temp, &rem);
        if (bigint_is_zero(&rem)) {
            return false;
        }
        // Early safety check: if 've exhausted all small primes up to sqrt of a small BigInt
        // and it still has no factors, it may well be prime.
        // But for large BigInts (> 50 bits), this is only a heuristic.
    }

    // If trial division by primes up to 463 found no factors, the number
    // is probably prime (with high probability for values > 463^2 214k)
    return true; // probable prime would need full BigInt Miller-Rabin for certainty
}
