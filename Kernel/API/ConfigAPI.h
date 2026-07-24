/*
 * ConfigAPI.h — Stable proxy interface for the configuration service
 *
 * Uses the existing CpuConfig from Infrastructure/Config/ConfigDefaults.h.
 * The Kernel service locator provides access to system configuration.
 *
 * Null Object: ker_config_null — returns default config
 */

#ifndef CONFIG_API_H
#define CONFIG_API_H

#include <stdint.h>
#include "../../Infrastructure/Config/ConfigDefaults.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Config Service Interface ─── */
typedef struct {
    /* Get the current system configuration (defaults if unparsed) */
    CpuConfig* (*get_config)(void);

    /* Parse TOML-style config data into CpuConfig. Returns 0 on success. */
    int (*parse)(CpuConfig* config, const char* toml_data, uint32_t length);

} ConfigAPI;

/* ─── Null Object — returns default config, parse is no-op ─── */
extern const ConfigAPI ker_config_null;

/* ─── Adapter Instance ─── */
extern const ConfigAPI g_config_service;

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_API_H */
