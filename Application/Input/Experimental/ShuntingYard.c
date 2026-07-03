/*
 * ShuntingYard.c ALTERNATIVE EXPERIMENTAL PARSER BACKEND (NOT CURRENTLY WIRED)
 * 
 * Implements a highly-optimized Shunting-Yard algorithm supporting:
 * - Basic math operations (+, -, *, /)
 * - Unary minus context (internal token '_')
 * - Freestanding trig/log functions (sin, cos, tan, ln, log10, sqrt, fact)
 * - Custom zero-dependency Taylor approximations and Newton square root
 * - Zero dynamic allocations, stack-only execution.
 */

#include "ShuntingYard.h"
#include "../../../Infrastructure/Utils/MemoryUtils.h"
#include "Kernel/Operations/Complex/ComplexOps.h"
#include "Kernel/Operations/Complex/ComplexTrig.h"
#include <stddef.h>

#define PI 3.14159265358979323846
#define INV_PI 0.31830988618379067154

// Freestanding local mathematical functions to avoid any external runtime dependency
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

static inline double local_sqrt(double val) {
    if (val < 0.0) return calc_nan();
    double res = val;
    if (val > 0.0) {
        for (int i = 0; i < 8; i++) res = 0.5 * (res + val / res);
    }
    return res;
}

static inline double local_factorial(double val) {
    if (val < 0.0 || val > 170.0) return calc_nan();
    int64_t n = (int64_t)val;
    if ((double)n != val) return calc_nan();
    double res = 1.0;
    for (int64_t i = 2; i <= n; i++) {
        res *= (double)i;
    }
    return res;
}

static inline ComplexValue local_complex_sqrt(ComplexValue z) {
    ComplexValue res;
    double x = z.real;
    double y = z.imag;
    if (y == 0.0) {
        if (x >= 0.0) {
            res.real = local_sqrt(x);
            res.imag = 0.0;
        } else {
            res.real = 0.0;
            res.imag = local_sqrt(-x);
        }
    } else {
        double r = local_sqrt(x * x + y * y);
        res.real = local_sqrt(0.5 * (r + x));
        double imag_val = local_sqrt(0.5 * (r - x));
        res.imag = (y < 0.0) ? -imag_val : imag_val;
    }
    return res;
}

static inline ComplexValue local_complex_ln(ComplexValue z) {
    ComplexValue res;
    double x = z.real;
    double y = z.imag;
    if (y == 0.0) {
        if (x > 0.0) {
            res.real = simple_ln(x);
            res.imag = 0.0;
        } else if (x < 0.0) {
            res.real = simple_ln(-x);
            res.imag = PI;
        } else {
            res.real = calc_nan();
            res.imag = 0.0;
        }
    } else {
        double r, theta;
        complex_polar(z, &r, &theta);
        res.real = simple_ln(r);
        res.imag = theta;
    }
    return res;
}

static int local_strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static bool is_digit(char c) {
    return (c >= '0' && c <= '9') || c == '.';
}

static double parse_double(const char* str, const char** endptr) {
    double val = 0.0;
    double divisor = 1.0;
    bool decimal = false;
    while (*str == ' ') str++;
    const char* p = str;
    while (is_digit(*p)) {
        if (*p == '.') {
            if (decimal) break;
            decimal = true;
        } else {
            if (!decimal) {
                val = val * 10.0 + (*p - '0');
            } else {
                divisor *= 10.0;
                val += (*p - '0') / divisor;
            }
        }
        p++;
    }
    *endptr = p;
    return val;
}

static int get_stack_precedence(char token) {
    if ((unsigned char)token >= 128) {
        return 4; // Functions bind tighter than operators
    }
    switch (token) {
        case '_': return 3; // Unary minus
        case '*':
        case '/': return 2;
        case '+':
        case '-': return 1;
        default:  return 0;
    }
}

bool infix_to_rpn(CalculatorState* state, const char* infix, RPNQueue* rpn_queue) {
    if (!infix || !rpn_queue) return false;
    
    rpn_queue->count = 0;
    unsigned char op_stack[64];
    int op_top = -1;
    bool expect_operand = true;
    
    void* custom_fns[16];
    int custom_fn_count = 0;

    const char* p = infix;
    while (*p != '\0') {
        if (*p == ' ') {
            p++;
            continue;
        }
        
        // Parse raw 'i' / 'I' imaginary constant
        if ((*p == 'i' || *p == 'I') && !is_digit(*(p + 1)) && !(*(p + 1) >= 'a' && *(p + 1) <= 'z')) {
            if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
            rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_NUMBER;
            rpn_queue->tokens[rpn_queue->count].data.value.real = 0.0;
            rpn_queue->tokens[rpn_queue->count].data.value.imag = 1.0;
            rpn_queue->count++;
            p++;
            expect_operand = false;
            continue;
        }

        // Parse numbers
        if (is_digit(*p)) {
            const char* endptr;
            double val = parse_double(p, &endptr);
            bool is_imag = false;
            if (*endptr == 'i' || *endptr == 'I') {
                is_imag = true;
                endptr++;
            }
            if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
            
            rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_NUMBER;
            if (is_imag) {
                rpn_queue->tokens[rpn_queue->count].data.value.real = 0.0;
                rpn_queue->tokens[rpn_queue->count].data.value.imag = val;
            } else {
                rpn_queue->tokens[rpn_queue->count].data.value.real = val;
                rpn_queue->tokens[rpn_queue->count].data.value.imag = 0.0;
            }
            rpn_queue->count++;
            p = endptr;
            expect_operand = false;
            continue;
        }
        
        // Parse functions
        if (*p >= 'a' && *p <= 'z') {
            const char* start = p;
            while (*p >= 'a' && *p <= 'z') p++;
            int len = (int)(p - start);
            
            RPNFunctionType fn = FN_NONE;
            if (len == 3 && local_strncmp(start, "sin", 3) == 0) fn = FN_SIN;
            else if (len == 3 && local_strncmp(start, "cos", 3) == 0) fn = FN_COS;
            else if (len == 3 && local_strncmp(start, "tan", 3) == 0) fn = FN_TAN;
            else if (len == 4 && local_strncmp(start, "sqrt", 4) == 0) fn = FN_SQRT;
            else if (len == 4 && local_strncmp(start, "fact", 4) == 0) fn = FN_FACT;
            else if (len == 2 && local_strncmp(start, "ln", 2) == 0) fn = FN_LN;
            else if (len == 5 && local_strncmp(start, "log10", 5) == 0) fn = FN_LOG10;
            else if (len == 3 && local_strncmp(start, "log", 3) == 0) fn = FN_LOG10;
            
            if (fn != FN_NONE) {
                if (op_top >= 63) return false;
                op_stack[++op_top] = (unsigned char)(128 + fn);
                expect_operand = true;
                continue;
            } else if (state && state->custom_lookup) {
                char fn_name[16];
                if (len < 16) {
                    fast_memcpy(fn_name, start, len);
                    fn_name[len] = '\0';
                    void* custom_fn = state->custom_lookup(state->custom_lookup_ctx, fn_name);
                    if (custom_fn && custom_fn_count < 16) {
                        if (op_top >= 63) return false;
                        custom_fns[custom_fn_count] = custom_fn;
                        op_stack[++op_top] = (unsigned char)(128 + FN_LOG10 + 1 + custom_fn_count);
                        custom_fn_count++;
                        expect_operand = true;
                        continue;
                    }
                }
                return false;
            } else {
                return false;
            }
        }
        
        // Left parenthesis
        if (*p == '(') {
            if (op_top >= 63) return false;
            op_stack[++op_top] = '(';
            expect_operand = true;
            p++;
            continue;
        }
        
        // Right parenthesis
        if (*p == ')') {
            while (op_top >= 0 && op_stack[op_top] != '(') {
                if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
                rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_OPERATOR;
                rpn_queue->tokens[rpn_queue->count].data.op = op_stack[op_top--];
                rpn_queue->count++;
            }
            if (op_top < 0) return false;
            op_top--; // Pop '('
            
            // If function is on the stack immediately below, pop it to output
            if (op_top >= 0 && op_stack[op_top] >= 128) {
                if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
                unsigned char val = op_stack[op_top--];
                if (val >= 128 + FN_LOG10 + 1) {
                    int custom_idx = val - 128 - FN_LOG10 - 1;
                    rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_CUSTOM_FUNCTION;
                    rpn_queue->tokens[rpn_queue->count].data.custom_fn = (ComplexValue (*)(ComplexValue))custom_fns[custom_idx];
                } else {
                    rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_FUNCTION;
                    rpn_queue->tokens[rpn_queue->count].data.fn = (RPNFunctionType)(val - 128);
                }
                rpn_queue->count++;
            }
            
            expect_operand = false;
            p++;
            continue;
        }
        
        // Operators
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
            char current_op = *p;
            if (current_op == '-' && expect_operand) {
                current_op = '_'; // Unary minus
            }
            
            while (op_top >= 0 && op_stack[op_top] != '(' &&
                   get_stack_precedence(op_stack[op_top]) >= get_stack_precedence(current_op)) {
                if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
                rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_OPERATOR;
                rpn_queue->tokens[rpn_queue->count].data.op = op_stack[op_top--];
                rpn_queue->count++;
            }
            
            if (op_top >= 63) return false;
            op_stack[++op_top] = current_op;
            expect_operand = true;
            p++;
            continue;
        }
        
        return false;
    }
    
    while (op_top >= 0) {
        if (op_stack[op_top] == '(' || op_stack[op_top] == ')') return false;
        if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
        
        if (op_stack[op_top] >= 128) {
            unsigned char val = op_stack[op_top--];
            if (val >= 128 + FN_LOG10 + 1) {
                int custom_idx = val - 128 - FN_LOG10 - 1;
                rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_CUSTOM_FUNCTION;
                rpn_queue->tokens[rpn_queue->count].data.custom_fn = (ComplexValue (*)(ComplexValue))custom_fns[custom_idx];
            } else {
                rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_FUNCTION;
                rpn_queue->tokens[rpn_queue->count].data.fn = (RPNFunctionType)(val - 128);
            }
        } else {
            rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_OPERATOR;
            rpn_queue->tokens[rpn_queue->count].data.op = op_stack[op_top--];
        }
        rpn_queue->count++;
    }
    
    return true;
}

ComplexValue evaluate_rpn(CalculatorState* state, const RPNQueue* rpn_queue) {
    ComplexValue error_val = {0.0, 0.0};
    if (!state || !rpn_queue || rpn_queue->count == 0) return error_val;
    
    ComplexValue eval_stack[128];
    int eval_top = -1;
    
    for (uint32_t i = 0; i < rpn_queue->count; ++i) {
        const RPNToken* token = &rpn_queue->tokens[i];
        
        if (token->type == RPN_TOKEN_NUMBER) {
            if (eval_top >= 127) return error_val;
            eval_stack[++eval_top] = token->data.value;
        } else if (token->type == RPN_TOKEN_OPERATOR) {
            if (token->data.op == '_') {
                if (eval_top < 0) return error_val;
                eval_stack[eval_top].real = -eval_stack[eval_top].real;
                eval_stack[eval_top].imag = -eval_stack[eval_top].imag;
                continue;
            }
            
            if (eval_top < 1) return error_val;
            ComplexValue val2 = eval_stack[eval_top--];
            ComplexValue val1 = eval_stack[eval_top--];
            ComplexValue result = {0.0, 0.0};
            
            switch (token->data.op) {
                case '+': result = complex_add(val1, val2); break;
                case '-': result = complex_sub(val1, val2); break;
                case '*': result = complex_mul(val1, val2); break;
                case '/': {
                    double denom = val2.real * val2.real + val2.imag * val2.imag;
                    if (denom == 0.0) {
                        state->flags |= 2;
                        ComplexValue nan_val = {calc_nan(), calc_nan()};
                        return nan_val;
                    }
                    result = complex_div(val1, val2);
                    break;
                }
            }
            eval_stack[++eval_top] = result;
        } else if (token->type == RPN_TOKEN_FUNCTION) {
            if (eval_top < 0) return error_val;
            ComplexValue val = eval_stack[eval_top--];
            ComplexValue result = {0.0, 0.0};
            
            switch (token->data.fn) {
                case FN_SIN:   result = complex_sin(val); break;
                case FN_COS:   result = complex_cos(val); break;
                case FN_TAN:   result = complex_tan(val); break;
                case FN_SQRT:  result = local_complex_sqrt(val); break;
                case FN_FACT: {
                    result.real = local_factorial(val.real);
                    result.imag = 0.0;
                    break;
                }
                case FN_LN:    result = local_complex_ln(val); break;
                case FN_LOG10: {
                    ComplexValue ln_val = local_complex_ln(val);
                    result.real = ln_val.real * 0.43429448190325182765;
                    result.imag = ln_val.imag * 0.43429448190325182765;
                    break;
                }
                default: return error_val;
            }
            eval_stack[++eval_top] = result;
        } else if (token->type == RPN_TOKEN_CUSTOM_FUNCTION) {
            if (eval_top < 0) return error_val;
            ComplexValue val = eval_stack[eval_top--];
            ComplexValue result = token->data.custom_fn(val);
            eval_stack[++eval_top] = result;
        }
    }
    
    if (eval_top != 0) return error_val;
    return eval_stack[0];
}

bool parse_rpn(CalculatorState* state, const char* rpn_str, RPNQueue* rpn_queue) {
    if (!rpn_str || !rpn_queue) return false;
    
    rpn_queue->count = 0;
    const char* p = rpn_str;
    
    while (*p != '\0') {
        if (*p == ' ') {
            p++;
            continue;
        }
        
        // Check if it's an operator
        if ((*p == '+' || *p == '-' || *p == '*' || *p == '/') && (*(p + 1) == ' ' || *(p + 1) == '\0')) {
            if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
            rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_OPERATOR;
            rpn_queue->tokens[rpn_queue->count].data.op = *p;
            rpn_queue->count++;
            p++;
            continue;
        }
        
        // Parse raw 'i' / 'I' imaginary constant
        if ((*p == 'i' || *p == 'I') && !is_digit(*(p + 1)) && !(*(p + 1) >= 'a' && *(p + 1) <= 'z')) {
            if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
            rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_NUMBER;
            rpn_queue->tokens[rpn_queue->count].data.value.real = 0.0;
            rpn_queue->tokens[rpn_queue->count].data.value.imag = 1.0;
            rpn_queue->count++;
            p++;
            continue;
        }
        
        // Check if it's a number (can have leading '+' or '-')
        bool is_num = false;
        if (is_digit(*p)) {
            is_num = true;
        } else if ((*p == '-' || *p == '+') && is_digit(*(p + 1))) {
            is_num = true;
        }
        
        if (is_num) {
            const char* endptr;
            double sign = 1.0;
            if (*p == '-') {
                sign = -1.0;
                p++;
            } else if (*p == '+') {
                p++;
            }
            double val = parse_double(p, &endptr);
            
            bool is_imag = false;
            if (*endptr == 'i' || *endptr == 'I') {
                is_imag = true;
                endptr++;
            }
            
            if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
            
            rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_NUMBER;
            if (is_imag) {
                rpn_queue->tokens[rpn_queue->count].data.value.real = 0.0;
                rpn_queue->tokens[rpn_queue->count].data.value.imag = sign * val;
            } else {
                rpn_queue->tokens[rpn_queue->count].data.value.real = sign * val;
                rpn_queue->tokens[rpn_queue->count].data.value.imag = 0.0;
            }
            rpn_queue->count++;
            p = endptr;
            continue;
        }
        
        // Check if it's a function (letters)
        if (*p >= 'a' && *p <= 'z') {
            const char* start = p;
            while (*p >= 'a' && *p <= 'z') p++;
            int len = (int)(p - start);
            
            RPNFunctionType fn = FN_NONE;
            if (len == 3 && local_strncmp(start, "sin", 3) == 0) fn = FN_SIN;
            else if (len == 3 && local_strncmp(start, "cos", 3) == 0) fn = FN_COS;
            else if (len == 3 && local_strncmp(start, "tan", 3) == 0) fn = FN_TAN;
            else if (len == 4 && local_strncmp(start, "sqrt", 4) == 0) fn = FN_SQRT;
            else if (len == 4 && local_strncmp(start, "fact", 4) == 0) fn = FN_FACT;
            else if (len == 2 && local_strncmp(start, "ln", 2) == 0) fn = FN_LN;
            else if (len == 5 && local_strncmp(start, "log10", 5) == 0) fn = FN_LOG10;
            else if (len == 3 && local_strncmp(start, "log", 3) == 0) fn = FN_LOG10;
            
            if (fn != FN_NONE) {
                if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
                rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_FUNCTION;
                rpn_queue->tokens[rpn_queue->count].data.fn = fn;
                rpn_queue->count++;
                continue;
            } else if (state && state->custom_lookup) {
                char fn_name[16];
                if (len < 16) {
                    fast_memcpy(fn_name, start, len);
                    fn_name[len] = '\0';
                    void* custom_fn = state->custom_lookup(state->custom_lookup_ctx, fn_name);
                    if (custom_fn) {
                        if (rpn_queue->count >= MAX_RPN_TOKENS) return false;
                        rpn_queue->tokens[rpn_queue->count].type = RPN_TOKEN_CUSTOM_FUNCTION;
                        rpn_queue->tokens[rpn_queue->count].data.custom_fn = (ComplexValue (*)(ComplexValue))custom_fn;
                        rpn_queue->count++;
                        continue;
                    }
                }
                return false;
            } else {
                return false;
            }
        }
        
        return false; // Unknown token
    }
    
    return true;
}
