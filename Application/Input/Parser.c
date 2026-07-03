/*
 * Author: WladyslawKW
 * Pratt parser for CalcOS. no stdlib, no libm, no heap.
 */

#include "Parser.h"
#include <stddef.h>
#include "../../Kernel/State/NumericValue.h"
#include "../../Kernel/Operations/Statistics/Statistics.h"

#define MAX_SYMBOLS 128

// FIX: Recursion depth limit for parse_precedence (#29)
// Prevents stack overflow from deeply nested expressions
#define MAX_PARSE_DEPTH 256

/* CALC_TINY_MEM: define this flag when targeting <64KB systems (Cortex-M, DOS, retro) */#ifdef CALC_TINY_MEM
#define PARSER_EXPR_STR_SIZE 64
#else
#define PARSER_EXPR_STR_SIZE 512
#endif

// FNV-1a hash - O(L) cost, O(1) compare. Unrolling not needed: branch predictor handles this well. comparison instead of char-by-char needles.
// O(L) hash, O(1) compare. Your branch predictor just sent a thank-you note.
static inline uint64_t fnv1a_hash(const char* key, uint32_t len) {
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    for (uint32_t i = 0; i < len; i++) {
        hash ^= (uint64_t)(unsigned char)key[i];
        hash *= 1099511628211ULL;            // FNV prime
    }
    return hash;
}

typedef struct {
    uint64_t hash;   // FNV-1a hash of variable name
    double value;
} HashSymbol;

#if defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define THREAD_LOCAL __thread
#else
#define THREAD_LOCAL _Thread_local
#endif

static THREAD_LOCAL HashSymbol symbol_table[MAX_SYMBOLS];
static THREAD_LOCAL int symbol_count = 0;

// Parse depth counter: reset to 0 at parse_expression entry, incremented per recursive call. (#29)
// Single-core safe: each parse_expression call resets it.
// Not thread-safe, but this is a single-core bare-metal kernel.
static int g_parse_depth = 0;

// xoshiro256++ state for expression-level rand()/randn()
// 256-bit state = 2^256 period, passes TestU01 BigCrush
static uint64_t expr_rng[4] = {123456789ULL, 362436069ULL, 521288629ULL, 88675123ULL};

static inline uint64_t rotl64(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

#define PI 3.14159265358979323846
#define INV_PI 0.31830988618379067154

static inline double local_sin(double x) {
    double k_d = (double)((int64_t)(x * INV_PI + (x >= 0.0 ? 0.5 : -0.5)));
    double xr = x - k_d * PI;
    double z2 = xr * xr;
    double val = xr * (1.0 + z2 * (-0.16666666666666666 + z2 * (0.008333333333333333 + z2 * (-0.0001984126984126984 + z2 * (0.00000275573192239859 + z2 * (-2.50521083854417e-8))))));
    int64_t k_i = (int64_t)k_d;
    if (k_i & 1) {
        val = -val;
    }
    return val;
}

static inline double local_cos(double x) {
    double k_d = (double)((int64_t)(x * INV_PI + (x >= 0.0 ? 0.5 : -0.5)));
    double xr = x - k_d * PI;
    double z2 = xr * xr;
    double val = 1.0 + z2 * (-0.5 + z2 * (0.041666666666666664 + z2 * (-0.0013888888888888889 + z2 * (0.0000248015873015873 + z2 * (-2.75573192239859e-7)))));
    int64_t k_i = (int64_t)k_d;
    if (k_i & 1) {
        val = -val;
    }
    return val;
}

static inline double local_exp(double x) {
    if (x > 709.78) {
        union { uint64_t i; double d; } u;
        u.i = 0x7FF0000000000000ULL;
        return u.d;
    }
    if (x < -745.13) return 0.0;
    
    double log2_e = 1.4426950408889634074;
    double ln2 = 0.69314718055994530942;
    double k_d = (double)((int64_t)(x * log2_e + (x >= 0.0 ? 0.5 : -0.5)));
    double f = x * log2_e - k_d;
    double z = f * ln2;
    double ez = 1.0 + z * (1.0 + z * (0.5 + z * (0.16666666666666666 + z * (0.041666666666666664 + z * (0.008333333333333333 + z * (0.0013888888888888889 + z * 0.0001984126984126984))))));
    
    int64_t k = (int64_t)k_d;
    union {
        double d;
        uint64_t i;
    } u;
    u.d = ez;
    if (k < -1000) {
        u.i += (uint64_t)(k + 500) << 52;
        union { double d; uint64_t i; } scale;
        scale.i = (uint64_t)(1023 - 500) << 52; // 2^-500
        u.d *= scale.d;
    } else {
        u.i += (uint64_t)(k) << 52;
    }
    return u.d;
}

static inline double simple_ln(double x) {
    if (x <= 0.0) return 0.0;
    union { double d; uint64_t i; } u;
    u.d = x;
    int k = ((u.i >> 52) & 0x7FF) - 1023;
    u.i = (u.i & 0x000FFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
    double m = u.d;
    double num = m - 1.0;
    double den = m + 1.0;
    double z = num / den;
    double z2 = z * z;
    double poly = z * (2.0 + z2 * (0.6666666666666666 + z2 * (0.4 + z2 * (0.2857142857142857 + z2 * 0.2222222222222222))));
    return poly + (double)k * 0.6931471805599453;
}

static double local_rand(void) {
    const uint64_t result = rotl64(expr_rng[1] * 5, 7) * 9;
    const uint64_t t = expr_rng[1] << 17;
    expr_rng[2] ^= expr_rng[0];
    expr_rng[3] ^= expr_rng[1];
    expr_rng[1] ^= expr_rng[2];
    expr_rng[0] ^= expr_rng[3];
    expr_rng[2] ^= t;
    expr_rng[3] = rotl64(expr_rng[3], 45);
    return (result >> 11) * (1.0 / 9007199254740992.0);
}

static double local_randn(void) {
    double u1 = local_rand();
    double u2 = local_rand();
    if (u1 < 1e-15) u1 = 1e-15;
    double ln_u1 = simple_ln(u1);
    double val = -2.0 * ln_u1;
    double r = 0.0;
    if (val > 0.0) {
        double sq = val;
        for (int i = 0; i < 8; ++i) {
            sq = 0.5 * (sq + val / sq);
        }
        r = sq;
    }
    double theta = 2.0 * PI * u2;
    return r * local_cos(theta);
}

static int mystrncmp(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

static double get_variable(const char* name, int length, bool* found) {
    uint64_t target_hash = fnv1a_hash(name, (uint32_t)length);
    for (int i = 0; i < symbol_count; i++) {
        // Single uint64_t comparison -- NO char loops, NO unaligned reads, ALL speed
        if (symbol_table[i].hash == target_hash) {
            *found = true;
            return symbol_table[i].value;
        }
    }
    *found = false;
    return 0.0;
}

static void set_variable(const char* name, int length, double value) {
    uint64_t target_hash = fnv1a_hash(name, (uint32_t)length);
    for (int i = 0; i < symbol_count; i++) {
        if (symbol_table[i].hash == target_hash) {
            symbol_table[i].value = value;
            return;
        }
    }
    if (symbol_count < MAX_SYMBOLS) {
        symbol_table[symbol_count].hash = target_hash;
        symbol_table[symbol_count].value = value;
        symbol_count++;
    }
}

static void remove_variable(const char* name, int length) {
    uint64_t target_hash = fnv1a_hash(name, (uint32_t)length);
    for (int i = 0; i < symbol_count; i++) {
        if (symbol_table[i].hash == target_hash) {
            for (int k = i; k < symbol_count - 1; k++) {
                symbol_table[k] = symbol_table[k + 1];
            }
            symbol_count--;
            return;
        }
    }
}

static double evaluate_expr_1var(const char* expr, const char* var_name, int var_len, double var_val, CalculatorState* state, bool* success) {
    bool var_existed;
    double old_var = get_variable(var_name, var_len, &var_existed);

    set_variable(var_name, var_len, var_val);
    double res = parse_expression(expr, state, success);

    if (var_existed) {
        set_variable(var_name, var_len, old_var);
    } else {
        remove_variable(var_name, var_len);
    }
    return res;
}

static double evaluate_expr_2vars(const char* expr, const char* x_name, int x_len, double x_val, const char* y_name, int y_len, double y_val, CalculatorState* state, bool* success) {
    bool x_existed, y_existed;
    double old_x = get_variable(x_name, x_len, &x_existed);
    double old_y = get_variable(y_name, y_len, &y_existed);

    set_variable(x_name, x_len, x_val);
    set_variable(y_name, y_len, y_val);

    double res = parse_expression(expr, state, success);

    if (x_existed) {
        set_variable(x_name, x_len, old_x);
    } else {
        remove_variable(x_name, x_len);
    }
    if (y_existed) {
        set_variable(y_name, y_len, old_y);
    } else {
        remove_variable(y_name, y_len);
    }
    return res;
}

static double solve_bisection(const char* expr, const char* var_name, int var_len, double low, double high, CalculatorState* state, bool* success) {
    double f_low = evaluate_expr_1var(expr, var_name, var_len, low, state, success);
    if (!*success) return 0.0;
    double f_high = evaluate_expr_1var(expr, var_name, var_len, high, state, success);
    if (!*success) return 0.0;

    if (f_low * f_high > 0.0) {
        *success = false;
        return 0.0;
    }

    double mid = low;
    for (int i = 0; i < 100; i++) {
        mid = 0.5 * (low + high);
        if ((high - low) < 1e-9) {
            break;
        }
        double f_mid = evaluate_expr_1var(expr, var_name, var_len, mid, state, success);
        if (!*success) return 0.0;

        if (f_mid == 0.0) {
            break;
        }
        if (f_low * f_mid < 0.0) {
            high = mid;
            f_high = f_mid;
        } else {
            low = mid;
            f_low = f_mid;
        }
    }
    return mid;
}

static double solve_newton(const char* expr, const char* var_name, int var_len, double guess, CalculatorState* state, bool* success) {
    double x = guess;
    double h = 1e-5;
    for (int i = 0; i < 100; i++) {
        double y = evaluate_expr_1var(expr, var_name, var_len, x, state, success);
        if (!*success) return 0.0;
        if (y < 1e-9 && y > -1e-9) {
            return x;
        }
        double y_h = evaluate_expr_1var(expr, var_name, var_len, x + h, state, success);
        if (!*success) return 0.0;
        double dy = (y_h - y) / h;
        if (dy == 0.0) {
            break;
        }
        x = x - y / dy;
    }
    return x;
}

static double rk4_solve(const char* expr, const char* x_name, int x_len, const char* y_name, int y_len, double x0, double y0, double x1, uint32_t steps, CalculatorState* state, bool* success) {
    double h = (x1 - x0) / (double)steps;
    double x = x0;
    double y = y0;

    for (uint32_t i = 0; i < steps; i++) {
        double k1 = evaluate_expr_2vars(expr, x_name, x_len, x, y_name, y_len, y, state, success);
        if (!*success) return 0.0;
        
        double k2 = evaluate_expr_2vars(expr, x_name, x_len, x + 0.5 * h, y_name, y_len, y + 0.5 * h * k1, state, success);
        if (!*success) return 0.0;

        double k3 = evaluate_expr_2vars(expr, x_name, x_len, x + 0.5 * h, y_name, y_len, y + 0.5 * h * k2, state, success);
        if (!*success) return 0.0;

        double k4 = evaluate_expr_2vars(expr, x_name, x_len, x + h, y_name, y_len, y + h * k3, state, success);
        if (!*success) return 0.0;

        y += (h / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
        x += h;
    }
    return y;
}

typedef enum {
    PREC_NONE = 0,
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // -
    PREC_PRIMARY
} Precedence;

typedef double (*PrefixFn)(Tokenizer* tok, CalculatorState* state, bool* success);
typedef double (*InfixFn)(Tokenizer* tok, CalculatorState* state, double left, bool* success);

typedef struct {
    PrefixFn prefix;
    InfixFn infix;
    Precedence precedence;
} ParseRule;

static double parse_precedence(Tokenizer* tok, CalculatorState* state, Precedence precedence, bool* success);
static double parse_number(Tokenizer* tok, CalculatorState* state, bool* success);
static double parse_unary(Tokenizer* tok, CalculatorState* state, bool* success);
static double parse_grouping(Tokenizer* tok, CalculatorState* state, bool* success);
static double parse_binary(Tokenizer* tok, CalculatorState* state, double left, bool* success);
static double parse_identifier(Tokenizer* tok, CalculatorState* state, bool* success);
static double parse_factorial(Tokenizer* tok, CalculatorState* state, double left, bool* success);
static double parse_unary_sqrt(Tokenizer* tok, CalculatorState* state, bool* success);

static void tokenizer_reset_to(Tokenizer* tok, const char* pos) {
    tok->lexer.source = pos;
    tok->lexer.cursor = 0;
    // FIX: Recompute source_len after reset (#3)
    // After resetting to a new position in the source, the old source_len
    // is too large it covers bytes past the new substring. Recompute it
    // so the Lexer's bounds guard works correctly.
    tok->lexer.source_len = 0;
    while (pos[tok->lexer.source_len] != '\0') tok->lexer.source_len++;
    
    tok->current = lexer_next_token(&tok->lexer);
    tok->next = lexer_next_token(&tok->lexer);
}

static const ParseRule rules[] = {
    // TOKEN_EOF
    { NULL,             NULL,         PREC_NONE },
    // TOKEN_NUMBER
    { parse_number,     NULL,         PREC_NONE },
    // TOKEN_PLUS
    { NULL,             parse_binary, PREC_TERM },
    // TOKEN_MINUS
    { parse_unary,      parse_binary, PREC_TERM },
    // TOKEN_STAR
    { NULL,             parse_binary, PREC_FACTOR },
    // TOKEN_SLASH
    { NULL,             parse_binary, PREC_FACTOR },
    // TOKEN_PERCENT (modulo same precedence as multiply/divide)
    { NULL,             parse_binary, PREC_FACTOR },
    // TOKEN_POWER (exponentiation right-associative, highest precedence)
    { NULL,             parse_binary, PREC_UNARY },
    // TOKEN_LPAREN
    { parse_grouping,   NULL,         PREC_NONE },
    // TOKEN_RPAREN
    { NULL,             NULL,         PREC_NONE },
    // TOKEN_IDENTIFIER
    { parse_identifier, NULL,         PREC_NONE },
    // TOKEN_COMMA
    { NULL,             NULL,         PREC_NONE },
    // TOKEN_EQUALS
    { NULL,             NULL,         PREC_NONE },
    // TOKEN_FACTORIAL
    { NULL,             parse_factorial, PREC_UNARY },
    // TOKEN_SQRT
    { parse_unary_sqrt, NULL,         PREC_UNARY },
    // TOKEN_EXP_OP
    { NULL,             parse_binary, PREC_UNARY },
    // TOKEN_ERROR
    { NULL,             NULL,         PREC_NONE }
};

static ParseRule get_rule(TokenType type) {
    return rules[type];
}

static double parse_number(Tokenizer* tok, CalculatorState* state, bool* success) {
    (void)state;
    (void)success;
    Token token = tokenizer_consume(tok);
    return token.value;
}

static double parse_unary(Tokenizer* tok, CalculatorState* state, bool* success) {
    Token op = tokenizer_consume(tok);
    double val = parse_precedence(tok, state, PREC_UNARY, success);
    if (op.type == TOKEN_MINUS) {
        return -val;
    }
    return val;
}

static double parse_grouping(Tokenizer* tok, CalculatorState* state, bool* success) {
    tokenizer_consume(tok); // consume '('
    double val = parse_precedence(tok, state, PREC_NONE, success);
    Token next = tokenizer_consume(tok);
    if (next.type != TOKEN_RPAREN) {
        *success = false;
    }
    return val;
}

static double local_asin(double x) {
    if (x < -1.0 || x > 1.0) return calc_nan();
    double abs_x = x < 0.0 ? -x : x;
    if (abs_x < 1e-8) return x;
    
    double res;
    if (abs_x <= 0.5) {
        double x2 = abs_x * abs_x;
        res = abs_x * (1.0 + x2 * (0.16666666666666666 + x2 * (0.075 + x2 * (0.04464285714285714 + x2 * 0.030381944444444444))));
    } else {
        double rem = 0.5 * (1.0 - abs_x);
        double u = rem;
        if (rem > 0.0) {
            for (int i = 0; i < 8; i++) u = 0.5 * (u + rem / u);
        } else {
            u = 0.0;
        }
        double u2 = u * u;
        double asin_u = u * (1.0 + u2 * (0.16666666666666666 + u2 * (0.075 + u2 * (0.04464285714285714 + u2 * 0.030381944444444444))));
        res = PI / 2.0 - 2.0 * asin_u;
    }
    return x < 0.0 ? -res : res;
}

static double local_acos(double x) {
    if (x < -1.0 || x > 1.0) return calc_nan();
    return PI / 2.0 - local_asin(x);
}

static double local_atan(double x) {
    double abs_x = x < 0.0 ? -x : x;
    double res;
    if (abs_x <= 1.0) {
        double x2 = abs_x * abs_x;
        res = abs_x * (1.0 + x2 * (-0.3333333333333333 + x2 * (0.2 + x2 * (-0.14285714285714285 + x2 * (0.1111111111111111 + x2 * -0.09090909090909091)))));
    } else {
        double inv_x = 1.0 / abs_x;
        double inv_x2 = inv_x * inv_x;
        double atan_inv = inv_x * (1.0 + inv_x2 * (-0.3333333333333333 + inv_x2 * (0.2 + inv_x2 * (-0.14285714285714285 + inv_x2 * (0.1111111111111111 + inv_x2 * -0.09090909090909091)))));
        res = PI / 2.0 - atan_inv;
    }
    return x < 0.0 ? -res : res;
}

static double local_pow(double base, double exp) {
    if (exp == 0.0) return 1.0;
    if (base == 0.0) return 0.0;
    if (base < 0.0) {
        int64_t n = (int64_t)exp;
        if ((double)n == exp) {
            double res = local_pow(-base, exp);
            if (n & 1) return -res;
            return res;
        }
        return calc_nan();
    }
    return local_exp(exp * simple_ln(base));
}

static double parse_factorial(Tokenizer* tok, CalculatorState* state, double left, bool* success) {
    tokenizer_consume(tok);
    (void)state;
    if (left < 0.0 || left > 170.0) {
        *success = false;
        return 0.0;
    }
    int64_t n = (int64_t)left;
    if ((double)n != left) {
        *success = false;
        return 0.0;
    }
    double res = 1.0;
    for (int64_t i = 2; i <= n; i++) {
        res *= (double)i;
    }
    return res;
}

static double parse_unary_sqrt(Tokenizer* tok, CalculatorState* state, bool* success) {
    tokenizer_consume(tok);
    double val = parse_precedence(tok, state, PREC_UNARY, success);
    if (val < 0.0) {
        *success = false;
        return 0.0;
    }
    double res = val;
    if (val > 0.0) {
        for (int i = 0; i < 8; i++) res = 0.5 * (res + val / res);
    }
    return res;
}

static double parse_binary(Tokenizer* tok, CalculatorState* state, double left, bool* success) {
    Token op = tokenizer_consume(tok);
    ParseRule rule = get_rule(op.type);
    double right = parse_precedence(tok, state, (Precedence)(rule.precedence + 1), success);
    if (!*success) return 0.0;

    switch (op.type) {
        case TOKEN_PLUS:  return left + right;
        case TOKEN_MINUS: return left - right;
        case TOKEN_STAR:  return left * right;
        case TOKEN_SLASH: {
            if (right == 0.0) {
                if (state) {
                    state->flags |= 2; // Division by zero flag
                }
                *success = false;
                return 0.0;
            }
            return left / right;
        }
        case TOKEN_PERCENT: {
            if (right == 0.0) {
                if (state) state->flags |= 2;
                *success = false;
                return 0.0;
            }
            // FIX: Integer modulo (no libm dependency)
            // Cast to int64_t for modulo operation. Non-integer operands
            // are truncated toward zero (standard C behavior).
            return (double)((int64_t)left % (int64_t)right);
        }
        case TOKEN_POWER: {
            return local_pow(left, right);
        }
        case TOKEN_EXP_OP: {
            return left * local_pow(10.0, right);
        }
        default:
            *success = false;
            return 0.0;
    }
}

static double parse_identifier(Tokenizer* tok, CalculatorState* state, bool* success) {
    Token id_tok = tokenizer_consume(tok);

    // 1. Assignment: var = expr
    if (tokenizer_peek(tok).type == TOKEN_EQUALS) {
        tokenizer_consume(tok); // consume '='
        double val = parse_precedence(tok, state, PREC_NONE, success);
        if (*success) {
            set_variable(id_tok.start, id_tok.length, val);
        }
        return val;
    }

    // 2. Function Call: func(args)
    if (tokenizer_peek(tok).type == TOKEN_LPAREN) {
        tokenizer_consume(tok); // consume '('

        // solve(expr, var, low, high) or solve(expr, var, guess)
        if (id_tok.length == 5 && mystrncmp(id_tok.start, "solve", 5) == 0) {
            char expr_str[PARSER_EXPR_STR_SIZE];
            int expr_len = 0;
            const char* start = tok->current.start;
            const char* p = start;
            int paren_depth = 1;  // '(' already consumed by tokenizer
            int max_scan = 4096;  // Safety bound (#3)
            while (max_scan > 0 && *p != '\0') {
                if (*p == '(') {
                    paren_depth++;
                } else if (*p == ')') {
                    paren_depth--;
                    if (paren_depth == 0) break;
                } else if (*p == ',' && paren_depth == 1) {
                    break;
                }
                p++;
                max_scan--;
            }
            if (max_scan <= 0 || paren_depth != 1 || *p != ',') {
                *success = false;
                return 0.0;
            }
            expr_len = (int)(p - start);
            if (expr_len >= PARSER_EXPR_STR_SIZE) {
                *success = false;
                return 0.0;
            }
            int copy_len = expr_len >= PARSER_EXPR_STR_SIZE ? PARSER_EXPR_STR_SIZE - 1 : expr_len;
            for (int i = 0; i < copy_len; i++) expr_str[i] = start[i];
            expr_str[copy_len] = '\0';

            if (*p != ',') {
                *success = false;
                return 0.0;
            }
            tokenizer_reset_to(tok, p + 1);

            Token var_tok = tokenizer_consume(tok);
            if (var_tok.type != TOKEN_IDENTIFIER) {
                *success = false;
                return 0.0;
            }

            if (tokenizer_consume(tok).type != TOKEN_COMMA) {
                *success = false;
                return 0.0;
            }

            double val3 = parse_precedence(tok, state, PREC_NONE, success);
            if (!*success) return 0.0;

            if (tokenizer_peek(tok).type == TOKEN_COMMA) {
                tokenizer_consume(tok); // consume ','
                double val4 = parse_precedence(tok, state, PREC_NONE, success);
                if (!*success) return 0.0;

                if (tokenizer_consume(tok).type != TOKEN_RPAREN) {
                    *success = false;
                    return 0.0;
                }

                char var_name[32];
                int var_len = var_tok.length > 31 ? 31 : var_tok.length;
                for (int i = 0; i < var_len; i++) var_name[i] = var_tok.start[i];
                var_name[var_len] = '\0';

                return solve_bisection(expr_str, var_name, var_len, val3, val4, state, success);
            } else {
                if (tokenizer_consume(tok).type != TOKEN_RPAREN) {
                    *success = false;
                    return 0.0;
                }

                char var_name[32];
                int var_len = var_tok.length > 31 ? 31 : var_tok.length;
                for (int i = 0; i < var_len; i++) var_name[i] = var_tok.start[i];
                var_name[var_len] = '\0';

                return solve_newton(expr_str, var_name, var_len, val3, state, success);
            }
        }

        // rk4(expr, x_var, y_var, x0, y0, x1, steps)
        if (id_tok.length == 3 && mystrncmp(id_tok.start, "rk4", 3) == 0) {
            char expr_str[PARSER_EXPR_STR_SIZE];
            int expr_len = 0;
            const char* start = tok->current.start;
            const char* p = start;
            int paren_depth = 1;  // '(' already consumed
            int max_scan = 4096;  // Safety bound (#3)
            while (max_scan > 0 && *p != '\0') {
                if (*p == '(') {
                    paren_depth++;
                } else if (*p == ')') {
                    paren_depth--;
                    if (paren_depth == 0) break;
                } else if (*p == ',' && paren_depth == 1) {
                    break;
                }
                p++;
                max_scan--;
            }
            if (max_scan <= 0 || paren_depth != 1 || *p != ',') {
                *success = false;
                return 0.0;
            }
            expr_len = (int)(p - start);
            if (expr_len >= PARSER_EXPR_STR_SIZE) {
                *success = false;
                return 0.0;
            }
            int copy_len = expr_len >= PARSER_EXPR_STR_SIZE ? PARSER_EXPR_STR_SIZE - 1 : expr_len;
            for (int i = 0; i < copy_len; i++) expr_str[i] = start[i];
            expr_str[copy_len] = '\0';

            if (*p != ',') {
                *success = false;
                return 0.0;
            }
            tokenizer_reset_to(tok, p + 1);

            Token x_var_tok = tokenizer_consume(tok);
            if (x_var_tok.type != TOKEN_IDENTIFIER) {
                *success = false;
                return 0.0;
            }

            if (tokenizer_consume(tok).type != TOKEN_COMMA) {
                *success = false;
                return 0.0;
            }

            Token y_var_tok = tokenizer_consume(tok);
            if (y_var_tok.type != TOKEN_IDENTIFIER) {
                *success = false;
                return 0.0;
            }

            if (tokenizer_consume(tok).type != TOKEN_COMMA) {
                *success = false;
                return 0.0;
            }

            double x0 = parse_precedence(tok, state, PREC_NONE, success);
            if (!*success) return 0.0;

            if (tokenizer_consume(tok).type != TOKEN_COMMA) {
                *success = false;
                return 0.0;
            }

            double y0 = parse_precedence(tok, state, PREC_NONE, success);
            if (!*success) return 0.0;

            if (tokenizer_consume(tok).type != TOKEN_COMMA) {
                *success = false;
                return 0.0;
            }

            double x1 = parse_precedence(tok, state, PREC_NONE, success);
            if (!*success) return 0.0;

            if (tokenizer_consume(tok).type != TOKEN_COMMA) {
                *success = false;
                return 0.0;
            }

            double steps_val = parse_precedence(tok, state, PREC_NONE, success);
            if (!*success) return 0.0;

            if (tokenizer_consume(tok).type != TOKEN_RPAREN) {
                *success = false;
                return 0.0;
            }

            char x_var_name[32];
            int x_var_len = x_var_tok.length > 31 ? 31 : x_var_tok.length;
            for (int i = 0; i < x_var_len; i++) x_var_name[i] = x_var_tok.start[i];
            x_var_name[x_var_len] = '\0';

            char y_var_name[32];
            int y_var_len = y_var_tok.length > 31 ? 31 : y_var_tok.length;
            for (int i = 0; i < y_var_len; i++) y_var_name[i] = y_var_tok.start[i];
            y_var_name[y_var_len] = '\0';

            uint32_t steps = (uint32_t)steps_val;
            if (steps == 0) steps = 1;

            return rk4_solve(expr_str, x_var_name, x_var_len, y_var_name, y_var_len, x0, y0, x1, steps, state, success);
        }

        // rand()
        if (id_tok.length == 4 && mystrncmp(id_tok.start, "rand", 4) == 0) {
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) {
                *success = false;
                return 0.0;
            }
            return local_rand();
        }

        // randn()
        if (id_tok.length == 5 && mystrncmp(id_tok.start, "randn", 5) == 0) {
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) {
                *success = false;
                return 0.0;
            }
            return local_randn();
        }

        // Statistical functions: mean, median, var, cov, corr
        if ((id_tok.length == 4 && mystrncmp(id_tok.start, "mean", 4) == 0) ||
            (id_tok.length == 6 && mystrncmp(id_tok.start, "median", 6) == 0) ||
            (id_tok.length == 3 && mystrncmp(id_tok.start, "var", 3) == 0) ||
            (id_tok.length == 3 && mystrncmp(id_tok.start, "cov", 3) == 0) ||
            (id_tok.length == 4 && mystrncmp(id_tok.start, "corr", 4) == 0)) {
            
            double args[256];
            int arg_count = 0;
            // FIX: Reject if too many arguments (#5)
            // Previously the loop silently discarded args beyond 256, returning
            // a result based on garbage/incomplete data. Now reject explicitly.
            while (tokenizer_peek(tok).type != TOKEN_RPAREN) {
                double val = parse_precedence(tok, state, PREC_NONE, success);
                if (!*success) return 0.0;
                if (arg_count >= 256) {
                    *success = false;
                    return 0.0;
                }
                args[arg_count++] = val;
                if (tokenizer_peek(tok).type == TOKEN_COMMA) {
                    tokenizer_consume(tok);
                } else {
                    break;
                }
            }
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) {
                *success = false;
                return 0.0;
            }

            if (id_tok.length == 4 && mystrncmp(id_tok.start, "mean", 4) == 0) {
                if (arg_count == 0) return 0.0;
                double sum = 0.0;
                for (int i = 0; i < arg_count; i++) sum += args[i];
                return sum / arg_count;
            } else if (id_tok.length == 6 && mystrncmp(id_tok.start, "median", 6) == 0) {
                if (arg_count == 0) return 0.0;
                // Delegate to the optimized median-of-three quicksort in Statistics.c
                // instead of the O(n) bubble sort that was here before
                return calc_median(args, (uint32_t)arg_count);
            } else if (id_tok.length == 3 && mystrncmp(id_tok.start, "var", 3) == 0) {
                if (arg_count <= 1) return 0.0;
                double sum = 0.0;
                for (int i = 0; i < arg_count; i++) sum += args[i];
                double mean_val = sum / arg_count;
                double sq_sum = 0.0;
                for (int i = 0; i < arg_count; i++) {
                    double diff = args[i] - mean_val;
                    sq_sum += diff * diff;
                }
                return sq_sum / (arg_count - 1);
            } else if (id_tok.length == 3 && mystrncmp(id_tok.start, "cov", 3) == 0) {
                if (arg_count < 4 || (arg_count % 2 != 0)) {
                    *success = false;
                    return 0.0;
                }
                int n = arg_count / 2;
                double sum_x = 0.0, sum_y = 0.0;
                for (int i = 0; i < n; i++) {
                    sum_x += args[2 * i];
                    sum_y += args[2 * i + 1];
                }
                double mean_x = sum_x / n;
                double mean_y = sum_y / n;
                double cov_sum = 0.0;
                for (int i = 0; i < n; i++) {
                    cov_sum += (args[2 * i] - mean_x) * (args[2 * i + 1] - mean_y);
                }
                return cov_sum / (n - 1);
            } else if (id_tok.length == 4 && mystrncmp(id_tok.start, "corr", 4) == 0) {
                if (arg_count < 4 || (arg_count % 2 != 0)) {
                    *success = false;
                    return 0.0;
                }
                int n = arg_count / 2;
                double sum_x = 0.0, sum_y = 0.0;
                for (int i = 0; i < n; i++) {
                    sum_x += args[2 * i];
                    sum_y += args[2 * i + 1];
                }
                double mean_x = sum_x / n;
                double mean_y = sum_y / n;
                double cov_sum = 0.0;
                double var_x_sum = 0.0;
                double var_y_sum = 0.0;
                for (int i = 0; i < n; i++) {
                    double dx = args[2 * i] - mean_x;
                    double dy = args[2 * i + 1] - mean_y;
                    cov_sum += dx * dy;
                    var_x_sum += dx * dx;
                    var_y_sum += dy * dy;
                }
                if (var_x_sum == 0.0 || var_y_sum == 0.0) return 0.0;
                double denom_val = var_x_sum * var_y_sum;
                double denom_sqrt = denom_val;
                if (denom_val > 0.0) {
                    for (int i = 0; i < 8; i++) {
                        denom_sqrt = 0.5 * (denom_sqrt + denom_val / denom_sqrt);
                    }
                }
                return cov_sum / denom_sqrt;
            }
        }

        // sin(x)
        if (id_tok.length == 3 && mystrncmp(id_tok.start, "sin", 3) == 0) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            return local_sin(arg);
        }
        // cos(x)
        if (id_tok.length == 3 && mystrncmp(id_tok.start, "cos", 3) == 0) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            return local_cos(arg);
        }
        // tan(x)
        if (id_tok.length == 3 && mystrncmp(id_tok.start, "tan", 3) == 0) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            double cos_val = local_cos(arg);
            if (cos_val == 0.0) {
                if (state) state->flags |= 2;
                *success = false;
                return 0.0;
            }
            return local_sin(arg) / cos_val;
        }
        // asin(x) or arcsin(x)
        if ((id_tok.length == 4 && mystrncmp(id_tok.start, "asin", 4) == 0) ||
            (id_tok.length == 6 && mystrncmp(id_tok.start, "arcsin", 6) == 0)) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            return local_asin(arg);
        }
        // acos(x) or arccos(x)
        if ((id_tok.length == 4 && mystrncmp(id_tok.start, "acos", 4) == 0) ||
            (id_tok.length == 6 && mystrncmp(id_tok.start, "arccos", 6) == 0)) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            return local_acos(arg);
        }
        // atan(x) or arctan(x)
        if ((id_tok.length == 4 && mystrncmp(id_tok.start, "atan", 4) == 0) ||
            (id_tok.length == 6 && mystrncmp(id_tok.start, "arctan", 6) == 0)) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            return local_atan(arg);
        }
        // ln(x)
        if (id_tok.length == 2 && mystrncmp(id_tok.start, "ln", 2) == 0) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            if (arg <= 0.0) {
                if (state) state->flags |= 2;
                *success = false;
                return 0.0;
            }
            return simple_ln(arg);
        }
        // log(x) or log10(x)
        if ((id_tok.length == 3 && mystrncmp(id_tok.start, "log", 3) == 0) ||
            (id_tok.length == 5 && mystrncmp(id_tok.start, "log10", 5) == 0)) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            if (arg <= 0.0) {
                if (state) state->flags |= 2;
                *success = false;
                return 0.0;
            }
            return simple_ln(arg) * 0.43429448190325182765;
        }
        // sqrt(x)
        if (id_tok.length == 4 && mystrncmp(id_tok.start, "sqrt", 4) == 0) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            if (arg < 0.0) { *success = false; return 0.0; }
            double res = arg;
            if (arg > 0.0) {
                for (int i = 0; i < 8; i++) res = 0.5 * (res + arg / res);
            }
            return res;
        }
        // fact(x) or factorial(x)
        if ((id_tok.length == 4 && mystrncmp(id_tok.start, "fact", 4) == 0) ||
            (id_tok.length == 9 && mystrncmp(id_tok.start, "factorial", 9) == 0)) {
            double arg = parse_precedence(tok, state, PREC_NONE, success);
            if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
            if (arg < 0.0 || arg > 170.0) { *success = false; return 0.0; }
            int64_t n = (int64_t)arg;
            if ((double)n != arg) { *success = false; return 0.0; }
            double res = 1.0;
            for (int64_t i = 2; i <= n; i++) res *= (double)i;
            return res;
        }

        // 2.x Fallback to plugin custom functions
        if (state && state->custom_lookup) {
            char fn_name[16];
            if (id_tok.length < 16U) {
                for (uint32_t i = 0; i < id_tok.length; i++) fn_name[i] = id_tok.start[i];
                fn_name[id_tok.length] = '\0';
                void* custom_fn_ptr = state->custom_lookup(state->custom_lookup_ctx, fn_name);
                if (custom_fn_ptr) {
                    double arg = parse_precedence(tok, state, PREC_NONE, success);
                    if (tokenizer_consume(tok).type != TOKEN_RPAREN) { *success = false; return 0.0; }
                    ComplexValue (*custom_fn)(ComplexValue) = (ComplexValue (*)(ComplexValue))custom_fn_ptr;
                    ComplexValue arg_c = {arg, 0.0};
                    ComplexValue res_c = custom_fn(arg_c);
                    return res_c.real;
                }
            }
        }

        *success = false;
        return 0.0;
    }

    // 3. Simple variable reference
    bool found = false;
    double val = get_variable(id_tok.start, id_tok.length, &found);
    if (!found) {
        if (id_tok.length == 2 && id_tok.start[0] == 'p' && id_tok.start[1] == 'i') {
            return 3.141592653589793;
        } else if (id_tok.length == 1 && id_tok.start[0] == 'e') {
            return 2.718281828459045;
        } else if (id_tok.length == 3 &&
                   (id_tok.start[0] == 'A' || id_tok.start[0] == 'a') &&
                   (id_tok.start[1] == 'N' || id_tok.start[1] == 'n') &&
                   (id_tok.start[2] == 'S' || id_tok.start[2] == 's')) {
            if (state && state->op_count > 0) {
                return state->operands[0];
            }
            return 0.0;
        }
        *success = false;
        return 0.0;
    }
    return val;
}

static double parse_precedence(Tokenizer* tok, CalculatorState* state, Precedence precedence, bool* success) {
    // FIX: Depth-limit guard (#29)
    // Deeply nested parentheses like ((((...))) ) 500 would exhaust the 64KB
    // kernel stack and cause a physical system lockup. Cap it at 256 levels.
    if (++g_parse_depth > MAX_PARSE_DEPTH) {
        g_parse_depth--;
        *success = false;
        return 0.0;
    }

    Token token = tokenizer_peek(tok);
    PrefixFn prefix = get_rule(token.type).prefix;
    if (prefix == NULL) {
        g_parse_depth--;
        *success = false;
        return 0.0;
    }

    double result = prefix(tok, state, success);
    g_parse_depth--;
    if (!*success) return 0.0;

    while (precedence <= get_rule(tokenizer_peek(tok).type).precedence) {
        Token next_token = tokenizer_peek(tok);
        InfixFn infix = get_rule(next_token.type).infix;
        if (infix == NULL) break;
        result = infix(tok, state, result, success);
        if (!*success) return 0.0;
    }

    return result;
}

double parse_expression(const char* source, CalculatorState* state, bool* success) {
    Tokenizer tok;
    tokenizer_init(&tok, source);
    int old_depth = g_parse_depth;
    g_parse_depth = 0; // Reset depth for each top-level expression parse
    *success = true;
    double val = parse_precedence(&tok, state, PREC_NONE, success);
    if (tokenizer_peek(&tok).type != TOKEN_EOF) {
        *success = false;
    }
    g_parse_depth = old_depth;
    return val;
}

