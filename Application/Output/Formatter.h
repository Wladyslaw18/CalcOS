#ifndef FORMATTER_H
#define FORMATTER_H

#include <stddef.h>
#include <stdarg.h>

#if defined(__GNUC__) || defined(__clang__)
  #define FORMAT_PRINTF(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
  #define FORMAT_PRINTF(fmt_idx, arg_idx)
#endif

FORMAT_PRINTF(3, 4)
size_t format_string(char* buf, size_t buf_size, const char* fmt, ...);

FORMAT_PRINTF(3, 0)
size_t format_string_va(char* buf, size_t buf_size, const char* fmt, va_list args);

#endif // FORMATTER_H
