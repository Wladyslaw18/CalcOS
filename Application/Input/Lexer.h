/*
 * Author: W. Kowal
 */

#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TOKEN_EOF = 0,      // 0
    TOKEN_NUMBER,
    TOKEN_PLUS,          // 2
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,         // 5
    TOKEN_PERCENT,       // 6 modulo
    TOKEN_POWER,         // 7 exponentiation (^)
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_IDENTIFIER,
    TOKEN_COMMA,         // 11
    TOKEN_EQUALS,
    TOKEN_FACTORIAL,
    TOKEN_SQRT,
    TOKEN_EXP_OP,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    uint32_t length;
    double value;
} Token;

typedef struct {
    const char* source;
    uint32_t cursor;
    // FIX: Track source length to prevent over-reads on non-null-terminated input (#1)
    uint32_t source_len;
} Lexer;

void lexer_init(Lexer* lexer, const char* source);
Token lexer_next_token(Lexer* lexer);

#endif // LEXER_H