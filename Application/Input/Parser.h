#ifndef PARSER_H
#define PARSER_H

#include "Tokenizer.h"
#include "../../Kernel/State/CalculatorState.h"

double parse_expression(const char* source, CalculatorState* state, bool* success);

#endif // PARSER_H
