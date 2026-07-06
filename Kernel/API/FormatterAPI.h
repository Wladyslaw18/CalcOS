/*
 * FormatterAPI.h — Stable proxy interface for string formatting
 *
 * Null Object: ker_formatter_null — writes nothing, returns 0
 */

#ifndef FORMATTER_API_H
#define FORMATTER_API_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Formatter Service Interface ─── */
typedef struct {
    /* Format a string (printf-like, no libc dependency).
     * buf: output buffer
     * buf_size: buffer capacity
     * fmt: format string
     * Returns: number of chars written (excluding null) */
    size_t (*format)(char* buf, size_t buf_size, const char* fmt, ...);

    /* Format with va_list (for wrapping) */
    size_t (*format_va)(char* buf, size_t buf_size,
                        const char* fmt, va_list args);

} FormatterAPI;

/* ─── Null Object ─── */
extern const FormatterAPI ker_formatter_null;

#ifdef __cplusplus
}
#endif

#endif /* FORMATTER_API_H */
