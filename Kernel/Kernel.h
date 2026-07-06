/*
 * Kernel.h — Central Service Locator / Stable Proxy Pattern for CalcOS
 *
 * Every stateful, pluggable, or cross-cutting subsystem registers here.
 * All inter-module calls go THROUGH kernel_get() — NO direct includes
 * between modules. This is NOT a god object: the Kernel is just a
 * fixed-size registry. Services keep their own state in their own structs.
 *
 * ═══ ARCHITECTURE ═══
 *
 *   ┌──────────────┐   kernel_register()   ┌──────────────┐
 *   │ ParserImpl   │──────────────────────►│              │
 *   │ LoggerImpl   │──────────────────────►│   KERNEL     │
 *   │ DisplayDriver│──────────────────────►│  (registry)  │
 *   │ InputDriver  │──────────────────────►│   O(1) enum  │
 *   │ ...          │──────────────────────►│              │
 *   └──────────────┘                       └──────┬───────┘
 *                                                 │
 *   ┌──────────────┐   kernel_get(KRN_PARSER)     │
 *   │ ui_core.c    │◄─────────────────────────────┘
 *   │ (no Parser.h │   Returns ParserAPI* — cast and call
 *   │  include!)   │   Returns NULL if disabled (null object pattern)
 *   └──────────────┘
 *
 * ═══ ZERO ALLOCATION POLICY ═══
 * Fixed-size array of KRN_COUNT slots. No malloc. No dynamic maps.
 * Every service is registered once at init and stays. Static forever.
 *
 * ═══ NULL OBJECT PATTERN ═══
 * kernel_get() returns NULL for unregistered OR disabled services.
 * The caller pattern is:
 *   ParserAPI* p = (ParserAPI*)kernel_get(krn, KRN_PARSER);
 *   if (p) p->parse(krn, expr, &success);
 * Each API also provides a static Null{Name} instance with no-op functions
 * so you can always call safely: &null_parser always returns 0.0.
 */

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../Kernel/State/CalculatorState.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════
 * Service Identifiers
 * ═══════════════════════════════════════════════════════════════
 * Every stateful or pluggable subsystem gets a unique enum slot.
 * These are indices into a fixed-size array — O(1) access.
 * Add new services BEFORE KRN_COUNT (which is the array size).
 */
typedef enum {
    /* ─── Infrastructure Services ─── */
    KRN_PARSER = 0,          // Expression parser (Pratt or ShuntingYard)
    KRN_LOGGER,              // Logging/serial output
    KRN_CONFIG,              // CPU/System configuration
    KRN_HISTORY,             // Circular expression history
    KRN_OPERAND_STACK,       // Operand stack operations
    KRN_FORMATTER,           // String formatting (printf-like, no libc)
    KRN_UI_CONTEXT,          // UI context (input buffer, history, result)

    /* ─── Pluggable Drivers ─── */
    KRN_DISPLAY,             // DisplayDriver (VGA / VESA / SDL / Terminal / Null)
    KRN_INPUT,               // InputDriver (PS2 / Serial / SDL / Null)

    /* ─── Platform / Hardware ─── */
    KRN_CPU_FEATURES,        // CPU SIMD capabilities (CPUID result)

    /* ─── Cross-cutting Utilities ─── */
    KRN_MEMORY_UTILS,        // Memory operations (fast_memcpy, fast_memset)

    /* ─── Plugin System Bridge (C++ → C) ─── */
    KRN_PLUGIN_REGISTRY,     // Plugin function lookup (bridges PluginManager)

    /* ─── Diagnostics ─── */
    KRN_PROFILER,            // RDTSC-based expression profiler (CalcProfiler)

    /* ─── MUST BE LAST: Number of service slots ─── */
    KRN_COUNT                // Current count: 13. uint64_t masks support up to 64.
} KernelServiceId;

/* ═══════════════════════════════════════════════════════════════
 * Kernel Entry — one per service slot
 * ═══════════════════════════════════════════════════════════════ */
typedef struct {
    const char* name;        // Human-readable identifier (e.g. "parser")
    void* instance;          // Opaque pointer to the service instance
    uint32_t flags;          // Bit 0: slot_occupied, Bit 1: active
} KernelEntry;

/* ═══════════════════════════════════════════════════════════════
 * Kernel Instance
 * ═══════════════════════════════════════════════════════════════
 * Fixed-size. No heap. Cache-line sympathetic layout.
 * NOT exactly 64 bytes — that's CalculatorState's constraint.
 */
typedef struct Kernel {
    KernelEntry services[KRN_COUNT];  // Service registry (fixed size)
    CalculatorState* calc_state;      // Direct alias for hot-path access
    uint64_t registered_mask;         // Bitmask: which slots are occupied (u64 → 64 slots max)
    uint64_t active_mask;             // Bitmask: which slots are enabled
} Kernel;

/* ═══════════════════════════════════════════════════════════════
 * Service API — Lifecycle
 * ═══════════════════════════════════════════════════════════════ */

/*
 * kernel_init: Initialize the Kernel and bind it to a CalculatorState.
 * Called ONCE at boot, before any service registration.
 * All fields zeroed; masks start empty.
 */
void kernel_init(Kernel* krn, CalculatorState* calc_state);

/*
 * kernel_register: Register a service implementation.
 * id       — enum ID from KernelServiceId
 * name     — human-readable string (e.g. "parser", "display")
 * instance — opaque pointer to the service impl struct
 *
 * The service starts INACTIVE. Call kernel_activate() after init succeeds.
 * Overwrites any previous registration at the same ID (logs warning).
 */
void kernel_register(Kernel* krn, KernelServiceId id,
                     const char* name, void* instance);

/*
 * kernel_activate / kernel_deactivate: Enable/disable a registered service.
 * Returns false if the ID is not registered.
 * Activate sets the "active" bit; deactivate clears it.
 * kernel_get() only returns instances for active services.
 */
bool kernel_activate(Kernel* krn, KernelServiceId id);
bool kernel_deactivate(Kernel* krn, KernelServiceId id);

/*
 * kernel_shutdown_all: Deactivate all services and clear registry.
 * Called at shutdown/reboot. Does NOT free anything (zero alloc policy).
 */
void kernel_shutdown_all(Kernel* krn);

/* ═══════════════════════════════════════════════════════════════
 * Service API — Lookup (HOT PATH)
 * ═══════════════════════════════════════════════════════════════ */

/*
 * g_kernel_null_proxies: Per-slot stable null-object fallback table.
 * Defined in Kernel.c alongside the null object instances.
 * Each slot holds a pointer to the matching ker_*_null struct, or NULL
 * for services that have no meaningful null object (UI, CPU features, etc.).
 *
 * WHY THIS EXISTS:
 *   Old behaviour — kernel_get returns NULL when unregistered/disabled.
 *   Every call site needed: if (p) p->method();
 *   If a plugin DLL is unloaded mid-execution and a caller forgets the guard,
 *   it jumps into unmapped memory → instant segfault / 0xC0000005.
 *
 *   New behaviour — kernel_get returns a stable no-op proxy instead of NULL.
 *   The proxy's function pointers all point to static no-op stubs.
 *   Call sites can call p->method() unconditionally and get a safe no-op.
 *   kernel_has() still exists for callers that genuinely need to branch on
 *   whether the service is live.
 */
extern const void* const g_kernel_null_proxies[KRN_COUNT];

/*
 * kernel_get_null_proxy: Return the stable null-proxy for a service slot.
 * Returns NULL only for slots that have no defined null object
 * (KRN_UI_CONTEXT, KRN_CPU_FEATURES, KRN_MEMORY_UTILS, KRN_PLUGIN_REGISTRY).
 * INLINE — compiles to a single array load after bounds check.
 */
static inline void* kernel_get_null_proxy(KernelServiceId id) {
    if ((uint32_t)id >= (uint32_t)KRN_COUNT) return NULL;
    return (void*)g_kernel_null_proxies[id];
}

/*
 * kernel_get: O(1) service lookup by enum ID.
 *
 * HOT PATH — static inline, zero function-call overhead at every call site.
 *
 * Returns the live instance pointer if the service is registered AND active.
 * Falls back to the stable null-proxy (from g_kernel_null_proxies) if the
 * service is unregistered or deactivated, so callers are safe to invoke
 * function pointers without an explicit NULL guard.
 *
 * Returns raw NULL only when:
 *   - krn is NULL
 *   - id is out of range
 *   - the slot has no defined null proxy (UI_CONTEXT, CPU_FEATURES, etc.)
 *
 * INLINE — compiles to two bitmask ANDs + one array load (live path)
 *       or one array load from the proxy table (null path).
 *       No branch misprediction penalty on the fast path after warm-up.
 */
static inline void* kernel_get(const Kernel* krn, KernelServiceId id) {
    if (!krn || (uint32_t)id >= (uint32_t)KRN_COUNT) return NULL;
    uint64_t bit = 1ULL << (uint32_t)id;

    /* Fast path: service is registered AND active — return live instance */
    if ((krn->registered_mask & bit) && (krn->active_mask & bit)) {
        return krn->services[id].instance;
    }

    /* Fallback: return stable null-proxy instead of NULL.
     * Prevents dangling-call crashes when a plugin DLL is unmapped or a
     * service has not yet been registered during early boot sequencing. */
    return kernel_get_null_proxy(id);
}

/*
 * kernel_has: Check if a service is registered AND active.
 * Returns true only if both conditions are met.
 */
static inline bool kernel_has(const Kernel* krn, KernelServiceId id) {
    if (!krn || (uint32_t)id >= (uint32_t)KRN_COUNT) return false;
    uint64_t bit = 1ULL << (uint32_t)id;
    return (krn->registered_mask & bit) && (krn->active_mask & bit);
}

/*
 * kernel_get_by_name: String-based lookup (for plugin bridge).
 * O(n) scan over registered services. Use the enum ID when possible.
 * Returns NULL if no registered service matches the name.
 */
void* kernel_get_by_name(const Kernel* krn, const char* name);

/* ═══════════════════════════════════════════════════════════════
 * Convenience Accessors — avoid casts at every call site
 * ═══════════════════════════════════════════════════════════════
 * These are thin inline wrappers around kernel_get() with the cast
 * built in. Each API header (Kernel/API/ headers) also provides convenience accessors.
 */

/* Quick-access alias to the CalculatorState (most frequently accessed) */
static inline CalculatorState* kernel_calc(const Kernel* krn) {
    return krn ? krn->calc_state : NULL;
}

/* ═══════════════════════════════════════════════════════════════
 * Global Kernel Instance
 * ═══════════════════════════════════════════════════════════════
 * For freestanding / single-core builds, a global Kernel is the
 * simplest path. Declared extern here, defined in Kernel.c.
 *
 * Use the K shorthand for convenience:
 *   ParserAPI* p = (ParserAPI*)kernel_get(K, KRN_PARSER);
 *
 * Multi-instance host apps can create their own Kernel on the stack
 * and pass it explicitly. The global is just a convenience.
 */
extern Kernel g_kernel;
#define K (&g_kernel)

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_H */
