#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include "../../Kernel/State/CalculatorState.h"

#define CMD_BUFFER_SIZE 256

typedef struct {
    char buffer[CMD_BUFFER_SIZE];
    uint32_t length;
    uint32_t cursor;
} CommandLineState;

void cli_init(CommandLineState* cli);
void cli_process_char(CommandLineState* cli, CalculatorState* calc, char c);

#endif // COMMAND_LINE_H
