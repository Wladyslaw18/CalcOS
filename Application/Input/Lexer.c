/*
 * Author: W. Kowal
 * FIX: null-termination guard + dangling decimal cursor #1 #2
 */

#include "Lexer.h"
#include <stddef.h>

void lexer_init(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->cursor = 0;
    if (source) {
        const char* p = source;
        while (*p) p++;
        lexer->source_len = (uint32_t)(p - source);
    } else {
        lexer->source_len = 0;
    }
}

static double parse_double(const char* start, uint32_t length) {
    if (!start || length == 0) return 0.0;
    uint32_t i = 0;
    
    double sign = 1.0;
    if (start[i] == '+') {
        i++;
    } else if (start[i] == '-') {
        sign = -1.0;
        i++;
    }

    // Optimize: Parse integer part using fast integer math
    uint64_t int_val = 0;
    while (i < length && start[i] >= '0' && start[i] <= '9') {
        int_val = int_val * 10 + (start[i] - '0');
        i++;
    }
    double value = (double)int_val;
    
    // Optimize: Parse fractional part using fast integer math and single division
    if (i < length && start[i] == '.') {
        i++;
        uint64_t frac_val = 0;
        double divisor = 1.0;
        while (i < length && start[i] >= '0' && start[i] <= '9') {
            frac_val = frac_val * 10 + (start[i] - '0');
            divisor *= 10.0;
            i++;
        }
        value += (double)frac_val / divisor;
    }
    
    value *= sign;

    if (i < length && (start[i] == 'e' || start[i] == 'E')) {
        i++;
        double exp_sign = 1.0;
        if (i < length && start[i] == '+') {
            i++;
        } else if (i < length && start[i] == '-') {
            exp_sign = -1.0;
            i++;
        }
        double exp_val = 0.0;
        while (i < length && start[i] >= '0' && start[i] <= '9') {
            exp_val = exp_val * 10.0 + (double)(start[i] - '0');
            i++;
        }
        double factor = 1.0;
        double base_10 = 10.0;
        int64_t exponent = (int64_t)exp_val;
        while (exponent > 0) {
            if (exponent & 1) factor *= base_10;
            base_10 *= base_10;
            exponent >>= 1;
        }
        if (exp_sign < 0.0) {
            value /= factor;
        } else {
            value *= factor;
        }
    }
    
    return value;
}

Token lexer_next_token(Lexer* lexer) {
    if (!lexer || !lexer->source) {
        Token err = {TOKEN_ERROR, NULL, 0, 0.0};
        return err;
    }
    
    // Skip whitespace
    // Optimize: check ws directly in register to save memory loads
    while (lexer->cursor < lexer->source_len) {
        char ws = lexer->source[lexer->cursor];
        if (ws != ' ' && ws != '\t' && ws != '\r' && ws != '\n') break;
        lexer->cursor++;
    }

    // FIX: check bounds BEFORE reading the character!
    // The old code loaded c FIRST and checked cursor >= source_len AFTER,
    // meaning if cursor == source_len, c was an out-of-bounds read.
    if (lexer->cursor >= lexer->source_len) {
        Token token = {TOKEN_EOF, &lexer->source[lexer->cursor], 0, 0.0};
        return token;
    }

    // Check for multi-byte UTF-8 symbols: pi (π) and sqrt (√)
    if (lexer->cursor + 1 < lexer->source_len &&
        (unsigned char)lexer->source[lexer->cursor] == 0xCF &&
        (unsigned char)lexer->source[lexer->cursor + 1] == 0x80) {
        Token token = {TOKEN_NUMBER, &lexer->source[lexer->cursor], 2, 3.14159265358979323846};
        lexer->cursor += 2;
        return token;
    }
    if (lexer->cursor + 2 < lexer->source_len &&
        (unsigned char)lexer->source[lexer->cursor] == 0xE2 &&
        (unsigned char)lexer->source[lexer->cursor + 1] == 0x88 &&
        (unsigned char)lexer->source[lexer->cursor + 2] == 0x9A) {
        Token token = {TOKEN_SQRT, &lexer->source[lexer->cursor], 3, 0.0};
        lexer->cursor += 3;
        return token;
    }

    // Now it's safe to read cursor is guaranteed < source_len
    char c = lexer->source[lexer->cursor];

    // Fast dispatch based on character
    switch (c) {
        case '+': {
            Token token = {TOKEN_PLUS, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case '-': {
            Token token = {TOKEN_MINUS, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case '*': {
            Token token = {TOKEN_STAR, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case '/': {
            Token token = {TOKEN_SLASH, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case '%': {
            Token token = {TOKEN_PERCENT, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case '^': {
            Token token = {TOKEN_POWER, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case '!': {
            Token token = {TOKEN_FACTORIAL, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case '(': {
            Token token = {TOKEN_LPAREN, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case ')': {
            Token token = {TOKEN_RPAREN, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case ',': {
            Token token = {TOKEN_COMMA, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
        case '=': {
            Token token = {TOKEN_EQUALS, &lexer->source[lexer->cursor], 1, 0.0};
            lexer->cursor++;
            return token;
        }
    }

    // Identifier token scanning
    // FIX: add bound checks on every character access
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        uint32_t start_pos = lexer->cursor;
        while (lexer->cursor < lexer->source_len) {
            char next_c = lexer->source[lexer->cursor];
            if ((next_c >= 'a' && next_c <= 'z') || (next_c >= 'A' && next_c <= 'Z') ||
                (next_c >= '0' && next_c <= '9') || next_c == '_') {
                lexer->cursor++;
            } else {
                break;
            }
            // If hit the end, stop scanning
            if (lexer->cursor >= lexer->source_len) break;
        }
        uint32_t length = lexer->cursor - start_pos;
        if (length == 0) length = 1; // Safety: at least the starting char

        // Check for EXP operator
        if (length == 3 && 
            (lexer->source[start_pos] == 'E' || lexer->source[start_pos] == 'e') &&
            (lexer->source[start_pos+1] == 'X' || lexer->source[start_pos+1] == 'x') &&
            (lexer->source[start_pos+2] == 'P' || lexer->source[start_pos+2] == 'p')) {
            uint32_t temp_cursor = lexer->cursor;
            while (temp_cursor < lexer->source_len && 
                   (lexer->source[temp_cursor] == ' ' || lexer->source[temp_cursor] == '\t')) {
                temp_cursor++;
            }
            if (temp_cursor >= lexer->source_len || lexer->source[temp_cursor] != '(') {
                Token token = {TOKEN_EXP_OP, &lexer->source[start_pos], length, 0.0};
                return token;
            }
        }

        Token token = {TOKEN_IDENTIFIER, &lexer->source[start_pos], length, 0.0};
        return token;
    }

    // Number token scanning
    // FIX: don't advance cursor past '.' on error
    if ((c >= '0' && c <= '9') || c == '.') {
        uint32_t start_pos = lexer->cursor;
        bool has_dot = false;

        if (c == '.') {
            has_dot = true;
            // Check if next char is a digit if not, emit TOKEN_ERROR
            // WITHOUT advancing the cursor past the dot
            uint32_t next_pos = lexer->cursor + 1;
            if (next_pos >= lexer->source_len) {
                // Trailing dot at end of input return error at this position
                Token token = {TOKEN_ERROR, &lexer->source[start_pos], 1, 0.0};
                return token;
            }
            char next_c = lexer->source[next_pos];  // Read the CHAR AFTER the dot, not the dot itself! #2
            if (next_c < '0' || next_c > '9') {
                // Dangling decimal: emit error WITHOUT consuming the dot
                Token token = {TOKEN_ERROR, &lexer->source[start_pos], 1, 0.0};
                return token;
            }
            lexer->cursor++;  // Now advance past the dot
        }

        while (1) {
            if (lexer->cursor >= lexer->source_len) break;
            char next_c = lexer->source[lexer->cursor];
            if (next_c >= '0' && next_c <= '9') {
                lexer->cursor++;
            } else if (next_c == '.' && !has_dot) {
                // Check that there's a digit after the dot
                if (lexer->cursor + 1 >= lexer->source_len || 
                    lexer->source[lexer->cursor + 1] < '0' || 
                    lexer->source[lexer->cursor + 1] > '9') {
                    break; // Trailing dot stop the number scan here
                }
                has_dot = true;
                lexer->cursor++;
            } else if ((next_c == 'e' || next_c == 'E') && (lexer->cursor + 1 < lexer->source_len)) {
                char c2 = lexer->source[lexer->cursor + 1];
                if (c2 >= '0' && c2 <= '9') {
                    lexer->cursor += 2;
                } else if ((c2 == '+' || c2 == '-') && (lexer->cursor + 2 < lexer->source_len)) {
                    char c3 = lexer->source[lexer->cursor + 2];
                    if (c3 >= '0' && c3 <= '9') {
                        lexer->cursor += 3;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        uint32_t length = lexer->cursor - start_pos;
        double val = parse_double(&lexer->source[start_pos], length);
        Token token = {TOKEN_NUMBER, &lexer->source[start_pos], length, val};
        // FIX: if the number starts with '.' and has no digits, it's an error
        if (length == 1 && c == '.') {
            Token err = {TOKEN_ERROR, &lexer->source[start_pos], 1, 0.0};
            return err;
        }
        return token;
    }

    Token token = {TOKEN_ERROR, &lexer->source[lexer->cursor], 1, 0.0};
    lexer->cursor++;
    return token;
}