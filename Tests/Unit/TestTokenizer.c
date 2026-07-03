#include "TestAssert.h"
#include "../../Application/Input/Tokenizer.h"
#include "../../Application/Input/Lexer.h"

int main() {
    printf("=== RUNNING TestTokenizer ===\n");

    const char* expr = "42.5 + (10 - 3.14)";
    Tokenizer tok;
    tokenizer_init(&tok, expr);

    // 1. Consume 42.5
    Token t = tokenizer_consume(&tok);
    assert_true(t.type == TOKEN_NUMBER);
    assert_double_eq(t.value, 42.5);

    // 2. Consume +
    t = tokenizer_consume(&tok);
    assert_true(t.type == TOKEN_PLUS);

    // 3. Peek '('
    t = tokenizer_peek(&tok);
    assert_true(t.type == TOKEN_LPAREN);

    // 4. Consume '('
    t = tokenizer_consume(&tok);
    assert_true(t.type == TOKEN_LPAREN);

    // 5. Consume 10
    t = tokenizer_consume(&tok);
    assert_true(t.type == TOKEN_NUMBER);
    assert_double_eq(t.value, 10.0);

    // 6. Consume -
    t = tokenizer_consume(&tok);
    assert_true(t.type == TOKEN_MINUS);

    // 7. Consume 3.14
    t = tokenizer_consume(&tok);
    assert_true(t.type == TOKEN_NUMBER);
    assert_double_eq(t.value, 3.14);

    // 8. Consume ')'
    t = tokenizer_consume(&tok);
    assert_true(t.type == TOKEN_RPAREN);

    // 9. Consume EOF
    t = tokenizer_consume(&tok);
    assert_true(t.type == TOKEN_EOF);

    printf("TestTokenizer summary: %d/%d assertions passed.\n", 
           total_assertions - failed_assertions, total_assertions);
    return failed_assertions > 0 ? 1 : 0;
}
