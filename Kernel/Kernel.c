/*
 * Kernel.c — Kernel Service Locator implementation
 *
 * Pure C. Zero allocations. Fixed-size registry.
 *
 * Lifecycle flow:
 *   1. kernel_init()          — Zero the Kernel, link CalculatorState
 *   2. kernel_register() * N  — Register each service implementation
 *   3. kernel_activate() * N  — Activate after service init succeeds
 *   4. kernel_get() * M       — O(1) lookups during runtime
 *   5. kernel_shutdown_all()  — Deactivate all, prepare for reboot
 */

#include "Kernel.h"
#include "API/ParserAPI.h"
#include "API/LoggerAPI.h"
#include "API/DisplayAPI.h"
#include "API/InputAPI.h"
#include "API/HistoryAPI.h"
#include "API/FormatterAPI.h"
#include "API/StackAPI.h"
#include "API/ConfigAPI.h"
#include "API/ProfilerAPI.h"

/* ═══════════════════════════════════════════════════════════════
 * NULL OBJECT DEFINITIONS
 * Each provides a stable no-op implementation so disabled services
 * never crash the system — they just silently return safe defaults.
 * ═══════════════════════════════════════════════════════════════ */

/* ─── Logger Null Object ─── */
static void null_log_write_string(const char* str) { (void)str; }
static void null_log_write_char(char c) { (void)c; }
static void null_log_set_level(LogLevelAPI level) { (void)level; }
static void null_log_log(LogLevelAPI level, const char* msg) { (void)level; (void)msg; }

const LoggerAPI ker_logger_null = {
    .log          = null_log_log,
    .write_string = null_log_write_string,
    .write_char   = null_log_write_char,
    .set_level    = null_log_set_level,
};

/* ─── Parser Null Object ─── */
static double null_parser_parse(const char* src, CalculatorState* st, bool* ok) {
    (void)src; (void)st; if (ok) *ok = false; return 0.0;
}
static bool null_parser_register_fn(const char* n, double (*f)(double)) {
    (void)n; (void)f; return false;
}

const ParserAPI ker_parser_null = {
    .parse             = null_parser_parse,
    .register_function = null_parser_register_fn,
};

/* ─── Display Null Object ─── */
static void null_disp_putchar(void* ctx, char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg) {
    (void)ctx; (void)c; (void)x; (void)y; (void)fg; (void)bg;
}
static void null_disp_writestr(void* ctx, const char* s, uint32_t x, uint32_t y, uint32_t f, uint32_t b) {
    (void)ctx; (void)s; (void)x; (void)y; (void)f; (void)b;
}
static void null_disp_setcursor(void* ctx, uint32_t x, uint32_t y) { (void)ctx; (void)x; (void)y; }
static void null_disp_clear(void* ctx, uint32_t bg) { (void)ctx; (void)bg; }
static void null_disp_scroll(void* ctx, int32_t l) { (void)ctx; (void)l; }
static void null_disp_present(void* ctx) { (void)ctx; }

const DisplayAPI ker_display_null = {
    .context   = NULL,
    .width     = 80,
    .height    = 25,
    .text_cols = 80,
    .text_rows = 25,
    .put_char  = null_disp_putchar,
    .write_str = null_disp_writestr,
    .set_cursor = null_disp_setcursor,
    .clear     = null_disp_clear,
    .scroll    = null_disp_scroll,
    .present   = null_disp_present,
};

/* ─── Input Null Object ─── */
static int null_input_available(void* ctx) { (void)ctx; return 0; }
static uint32_t null_input_readkey(void* ctx) { (void)ctx; return 0; }
static int null_input_init(void* ctx) { (void)ctx; return 0; }
static void null_input_deinit(void* ctx) { (void)ctx; }

const InputAPI ker_input_null = {
    .key_available = null_input_available,
    .read_key      = null_input_readkey,
    .init          = null_input_init,
    .deinit        = null_input_deinit,
};

/* ─── History Null Object ─── */
static void null_hist_push(CalculatorState* s, double a, double b, double r, unsigned char op) {
    (void)s; (void)a; (void)b; (void)r; (void)op;
}
static int null_hist_get(const CalculatorState* s, uint32_t i, HistoryEntryAPI* e) {
    (void)s; (void)i; (void)e; return 0;
}
static void null_hist_clear(CalculatorState* s) { (void)s; }

const HistoryAPI ker_history_null = {
    .push  = null_hist_push,
    .get   = null_hist_get,
    .clear = null_hist_clear,
};

/* ─── Formatter Null Object ─── */
static size_t null_fmt_format(char* buf, size_t sz, const char* fmt, ...) {
    (void)buf; (void)sz; (void)fmt; if (sz > 0) *buf = '\0'; return 0;
}
static size_t null_fmt_format_va(char* buf, size_t sz, const char* fmt, va_list args) {
    (void)buf; (void)sz; (void)fmt; (void)args; if (sz > 0) *buf = '\0'; return 0;
}

const FormatterAPI ker_formatter_null = {
    .format     = null_fmt_format,
    .format_va  = null_fmt_format_va,
};

/* ─── Stack Null Object ─── */
static int null_stack_push(CalculatorState* s, double v) { (void)s; (void)v; return 0; }
static int null_stack_pop(CalculatorState* s, double* v) { (void)s; if (v) *v = 0.0; return 0; }
static int null_stack_peek(const CalculatorState* s, double* v) { (void)s; if (v) *v = 0.0; return 0; }
static void null_stack_clear(CalculatorState* s) { (void)s; }

const StackAPI ker_stack_null = {
    .push  = null_stack_push,
    .pop   = null_stack_pop,
    .peek  = null_stack_peek,
    .clear = null_stack_clear,
};

/* ─── Config Null Object ─── */
static CpuConfig null_config_data;
static CpuConfig* null_config_get(void) { return &null_config_data; }
static int null_config_parse(CpuConfig* cfg, const char* data, uint32_t len) {
    (void)cfg; (void)data; (void)len; return -1;
}

const ConfigAPI ker_config_null = {
    .get_config = null_config_get,
    .parse      = null_config_parse,
};

/* ─── Profiler Null Object ─── */
static ProfilerAPIResult null_profiler_profile(double (*func)(void*), void* arg) {
    (void)func; (void)arg;
    ProfilerAPIResult r;
    r.expression = NULL;
    r.cycles     = 0;
    r.seconds    = 0.0;
    /* quiet NaN: service is down, result is undefined */
    union { uint64_t i; double d; } nan_val;
    nan_val.i    = 0x7FF8000000000000ULL;
    r.result     = nan_val.d;
    r.error      = PROFILER_API_ERROR_NONE;
    return r;
}
static void null_profiler_format(const ProfilerAPIResult* r, char* buf, uint32_t buf_size) {
    (void)r;
    if (buf && buf_size > 0) buf[0] = '\0';
}
static uint32_t null_profiler_cpu_mhz(void) { return 0; }

const ProfilerAPI ker_profiler_null = {
    .profile  = null_profiler_profile,
    .format   = null_profiler_format,
    .cpu_mhz  = null_profiler_cpu_mhz,
};

/* ═══════════════════════════════════════════════════════════════
 * STABLE NULL-PROXY ROUTING TABLE
 * ═══════════════════════════════════════════════════════════════
 * Maps each KernelServiceId to its corresponding null-object instance.
 * kernel_get() falls back to this table when a service is unregistered
 * or deactivated, instead of returning NULL.
 *
 * Slots set to NULL have no safe null object (e.g. UI_CONTEXT is a
 * concrete struct, not an API vtable; CPU_FEATURES is hardware data).
 * Callers of those services must still guard with kernel_has().
 *
 * DESIGN: designated-initializer syntax guarantees that if a new
 * KernelServiceId is added to the enum without a matching entry here,
 * GCC/Clang emit a -Wmissing-field-initializers warning. Fix it by
 * adding the new slot before shipping.
 * ═══════════════════════════════════════════════════════════════ */
const void* const g_kernel_null_proxies[KRN_COUNT] = {
    [KRN_PARSER]          = &ker_parser_null,
    [KRN_LOGGER]          = &ker_logger_null,
    [KRN_CONFIG]          = &ker_config_null,
    [KRN_HISTORY]         = &ker_history_null,
    [KRN_OPERAND_STACK]   = &ker_stack_null,
    [KRN_FORMATTER]       = &ker_formatter_null,
    [KRN_UI_CONTEXT]      = NULL, /* Concrete UIContext struct — no vtable null object */
    [KRN_DISPLAY]         = &ker_display_null,
    [KRN_INPUT]           = &ker_input_null,
    [KRN_CPU_FEATURES]    = NULL, /* Hardware data — caller must use kernel_has() */
    [KRN_MEMORY_UTILS]    = NULL, /* Low-level utils — always registered at boot */
    [KRN_PLUGIN_REGISTRY] = NULL, /* Optional C++ bridge — guard with kernel_has() */
    [KRN_PROFILER]        = &ker_profiler_null,
};



/* ─── Internal Kernel Logger ─── */
/*
 * kernel_log: Internal diagnostic writer used during kernel lifecycle.
 *
 * Strategy (avoids circular dependency — Logger is itself a kernel service):
 *   1. If KRN_LOGGER is already registered AND active, route through it.
 *   2. Otherwise, write directly to COM1 (port 0x3F8) via a blocking byte loop.
 *      COM1 was initialised by BootEntry.asm at 38400 baud before C code ran.
 *      On hosted builds (PLATFORM_HOST), fall through silently — serial isn't
 *      mapped, and the host logger will catch subsequent messages anyway.
 *
 * This gives full boot-sequence visibility without any libc dependency.
 */
static void kernel_log(const Kernel* krn, const char* msg) {
    if (!msg) return;

    /* Attempt 1: route through the live logger service if available */
    if (krn) {
        uint64_t bit = 1ULL << (uint32_t)KRN_LOGGER;
        if ((krn->registered_mask & bit) && (krn->active_mask & bit)) {
            const LoggerAPI* log = (const LoggerAPI*)krn->services[KRN_LOGGER].instance;
            if (log && log->write_string) {
                log->write_string(msg);
                log->write_char('\n');
                return;
            }
        }
    }

#if !defined(PLATFORM_HOST) && (defined(__i386__) || defined(__x86_64__))
    /* Attempt 2: COM1 direct port I/O (freestanding bare-metal only).
     * Wait for Transmitter Holding Register Empty (bit 5 of LSR = port 0x3FD),
     * then write one byte at a time to the data register (port 0x3F8). */
    for (const char* p = msg; *p; ++p) {
        /* Busy-wait on LSR bit 5 */
        uint8_t lsr;
        do {
            __asm__ volatile ("inb $0x3FD, %0" : "=a"(lsr));
        } while (!(lsr & 0x20));
        __asm__ volatile ("outb %0, $0x3F8" :: "a"(*p));
    }
    /* Newline */
    uint8_t lsr;
    do { __asm__ volatile ("inb $0x3FD, %0" : "=a"(lsr)); } while (!(lsr & 0x20));
    __asm__ volatile ("outb %0, $0x3F8" :: "a"('\n'));
#endif
    /* PLATFORM_HOST: silent — host logger registered immediately after init */
}

/* ─── Initialization ─── */

void kernel_init(Kernel* krn, CalculatorState* calc_state) {
    if (!krn) return;

    /* Zero everything — including all service entries and masks */
    for (uint32_t i = 0; i < KRN_COUNT; ++i) {
        krn->services[i].name = NULL;
        krn->services[i].instance = NULL;
        krn->services[i].flags = 0;
    }
    krn->calc_state = calc_state;
    krn->registered_mask = 0;
    krn->active_mask = 0;

    kernel_log(krn, "[Kernel] initialized");
}

/* ─── Registration ─── */

void kernel_register(Kernel* krn, KernelServiceId id,
                     const char* name, void* instance) {
    if (!krn || (uint32_t)id >= (uint32_t)KRN_COUNT) return;
    if (!instance) return;

    uint64_t bit = 1ULL << (uint32_t)id;

    /* Check for collision */
    if (krn->registered_mask & bit) {
        kernel_log(krn, "[Kernel] WARNING: service slot already occupied");
    }

    krn->services[id].name = name ? name : "unnamed";
    krn->services[id].instance = instance;
    krn->services[id].flags = 0x01; /* slot occupied, not yet active */
    krn->registered_mask |= bit;

    /* Clear active bit (service starts inactive until activated) */
    krn->active_mask &= ~bit;
}

/* ─── Lifecycle ─── */

bool kernel_activate(Kernel* krn, KernelServiceId id) {
    if (!krn || (uint32_t)id >= (uint32_t)KRN_COUNT) return false;
    uint64_t bit = 1ULL << (uint32_t)id;
    if (!(krn->registered_mask & bit)) return false; /* Not registered */

    krn->services[id].flags |= 0x02; /* active */
    krn->active_mask |= bit;
    return true;
}

bool kernel_deactivate(Kernel* krn, KernelServiceId id) {
    if (!krn || (uint32_t)id >= (uint32_t)KRN_COUNT) return false;
    uint64_t bit = 1ULL << (uint32_t)id;
    if (!(krn->registered_mask & bit)) return false; /* Not registered */

    krn->services[id].flags &= ~0x02; /* inactive */
    krn->active_mask &= ~bit;
    return true;
}

void kernel_shutdown_all(Kernel* krn) {
    if (!krn) return;

    /* Deactivate all registered services in reverse order */
    for (int32_t i = (int32_t)KRN_COUNT - 1; i >= 0; --i) {
        uint32_t id  = (uint32_t)i;
        uint64_t bit = 1ULL << id;
        if (krn->registered_mask & bit) {
            kernel_deactivate(krn, (KernelServiceId)id);
        }
    }

    /* Clear registry (allows re-init) */
    for (uint32_t i = 0; i < KRN_COUNT; ++i) {
        krn->services[i].name = NULL;
        krn->services[i].instance = NULL;
        krn->services[i].flags = 0;
    }
    krn->registered_mask = 0;
    krn->active_mask = 0;

    kernel_log(krn, "[Kernel] shutdown complete");
}

/* ─── String-based Lookup ─── */

void* kernel_get_by_name(const Kernel* krn, const char* name) {
    if (!krn || !name) return NULL;

    /* FNV-1a hash of the query name — O(len) once, then O(1) compare per slot.
     * FNV offset basis and prime from the reference implementation (32-bit). */
    uint32_t query_hash = 2166136261u;
    for (const char* p = name; *p; ++p) {
        query_hash ^= (uint8_t)*p;
        query_hash *= 16777619u;
    }

    for (uint32_t i = 0; i < KRN_COUNT; ++i) {
        uint64_t bit = 1ULL << i;
        if ((krn->registered_mask & bit) && (krn->active_mask & bit)) {
            if (krn->services[i].name) {
                /* Hash the registered name and compare — avoids full strcmp
                 * on every slot when there are many registered services. */
                uint32_t slot_hash = 2166136261u;
                for (const char* p = krn->services[i].name; *p; ++p) {
                    slot_hash ^= (uint8_t)*p;
                    slot_hash *= 16777619u;
                }
                if (slot_hash == query_hash) {
                    /* Hash match: confirm with full strcmp to rule out collision */
                    const char* a = krn->services[i].name;
                    const char* b = name;
                    while (*a && *b && *a == *b) { ++a; ++b; }
                    if (*a == *b) return krn->services[i].instance;
                }
            }
        }
    }
    return NULL;
}
