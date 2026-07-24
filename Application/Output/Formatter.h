#ifndef FORMATTER_H
#define FORMATTER_H

#include <stddef.h>
#include <stdarg.h>

size_t format_string(char* buf, size_t buf_size, const char* fmt, ...);
size_t format_string_va(char* buf, size_t buf_size, const char* fmt, va_list args);

#endif // FORMATTER_H
