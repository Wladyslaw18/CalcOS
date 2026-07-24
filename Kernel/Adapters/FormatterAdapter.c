/*
 * FormatterAdapter.c — Bridges the existing Application/Output/Formatter.c
 * implementation to the Kernel FormatterAPI stable interface.
 */

#include <stdarg.h>
#include "Kernel/API/FormatterAPI.h"
#include "Application/Output/Formatter.h"

/* ─── Format wrapper ─── */
static size_t adapter_format(char* buf, size_t buf_size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t result = format_string_va(buf, buf_size, fmt, args);
    va_end(args);
    return result;
}

/* ─── Format va_list wrapper ─── */
static size_t adapter_format_va(char* buf, size_t buf_size,
                                const char* fmt, va_list args) {
    return format_string_va(buf, buf_size, fmt, args);
}

/* ─── Public API struct ─── */
const FormatterAPI g_formatter_service = {
    .format     = adapter_format,
    .format_va  = adapter_format_va,
};
