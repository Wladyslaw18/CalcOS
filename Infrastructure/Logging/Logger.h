#ifndef LOGGER_H
#define LOGGER_H

#include "LogLevel.h"

void logger_init(void);
void logger_log(LogLevel level, const char* message);
void logger_write_string(const char* str);
void logger_write_char(char c);
void logger_write_hex(unsigned long long val);
void logger_write_dec(long long val);

#endif // LOGGER_H
