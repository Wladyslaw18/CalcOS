#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "Lexer.h"

typedef struct {
    Lexer lexer;
    Token current;
    Token next;
} Tokenizer;

void tokenizer_init(Tokenizer* tok, const char* source);
Token tokenizer_peek(Tokenizer* tok);
Token tokenizer_consume(Tokenizer* tok);

#endif // TOKENIZER_H
