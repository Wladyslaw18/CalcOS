/*
 * LoggerAPI.h — Stable proxy interface for the logging service
 *
 * LOG_LEVEL is configurable. At DEBUG level, every kernel_get() can log.
 * At FATAL level, only panics and errors go through.
 *
 * Null Object: ker_logger_null — silently discards all messages
 */

#ifndef LOGGER_API_H
#define LOGGER_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Log Levels ─── */
typedef enum {
    KERN_LOG_DEBUG = 0,
    KERN_LOG_INFO,
    KERN_LOG_WARN,
    KERN_LOG_ERROR,
    KERN_LOG_FATAL
} LogLevelAPI;

/* ─── Logger Service Interface ─── */
typedef struct {
    /* Write a formatted log message at the given level */
    void (*log)(LogLevelAPI level, const char* message);

    /* Write a raw string directly (bypasses formatting) */
    void (*write_string)(const char* str);

    /* Write a single character */
    void (*write_char)(char c);

    /* Configure the minimum log level (lower = more verbose) */
    void (*set_level)(LogLevelAPI level);

} LoggerAPI;

/* ─── Null Object (silent fallback) ─── */
extern const LoggerAPI ker_logger_null;

/* ─── Adapter Instance ─── */
extern const LoggerAPI g_logger_service;

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_API_H */
