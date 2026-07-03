#include "Tokenizer.h"

void tokenizer_init(Tokenizer* tok, const char* source) {
    lexer_init(&tok->lexer, source);
    tok->current = lexer_next_token(&tok->lexer);
    tok->next = lexer_next_token(&tok->lexer);
}

Token tokenizer_peek(Tokenizer* tok) {
    return tok->current;
}

Token tokenizer_consume(Tokenizer* tok) {
    Token prev = tok->current;
    tok->current = tok->next;
    // Only fetch next token if are not at EOF already
    if (tok->current.type != TOKEN_EOF) {
        tok->next = lexer_next_token(&tok->lexer);
    } else {
        tok->next = tok->current;
    }
    return prev;
}
