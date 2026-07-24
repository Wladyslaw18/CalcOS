/**
 * LoggerAdapter.c  –  Adapter that wraps the concrete Logger implementation
 *                      (Infrastructure/Logging/Logger.h) behind the abstract
 *                      LoggerAPI interface (Kernel/API/LoggerAPI.h).
 *
 * The global constant g_logger_service is linked into the Kernel so that
 * every Kernel-aware component can log through kernel_get(K, KRN_LOGGER)
 * without depending on the Logger .c / .h directly.
 */
#include "Kernel/API/LoggerAPI.h"
#include "Infrastructure/Logging/Logger.h"
#include "Infrastructure/Logging/LogLevel.h"

/* Bridge LogLevelAPI (Kernel) → LogLevel (Infrastructure).
 * Both enums have the same values (DEBUG=0..FATAL=4) so direct cast is safe. */
#define KERN_LOG_TO_INFRA(lvl) ((LogLevel)(lvl))

// ---------------------------------------------------------------------------
// Wrappers  –  one per LoggerAPI function pointer
// ---------------------------------------------------------------------------

static void adapter_log(LogLevelAPI level, const char* message) {
    logger_log(KERN_LOG_TO_INFRA(level), message);
}

static void adapter_write_string(const char* str) {
    logger_write_string(str);
}

static void adapter_write_char(char c) {
    logger_write_char(c);
}

static void adapter_set_level(LogLevelAPI level) {
    (void)level;
    /* The underlying Logger does not expose a runtime level-setter yet.
     * This no-op satisfies the interface contract; once Logger grows a
     * set_level function the body can be wired through here. */
}

// ---------------------------------------------------------------------------
// Public adapter singleton  –  const so it lives in .rodata
// ---------------------------------------------------------------------------
const LoggerAPI g_logger_service = {
    .log          = adapter_log,
    .write_string = adapter_write_string,
    .write_char   = adapter_write_char,
    .set_level    = adapter_set_level
};
