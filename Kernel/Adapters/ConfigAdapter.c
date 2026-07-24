/*
 * File: ConfigAdapter.c
 * Author: W. Kowal
 * Description: Kernel-compatible Config service adapter.
 * Wraps Infrastructure/Config/ConfigParser.h into the Kernel service locator pattern.
 *
 * ZERO HEAP ALLOCATIONS. Everything is stack/static based.
 */

#include "Kernel/API/ConfigAPI.h"
#include "Infrastructure/Config/ConfigParser.h"
#include "Infrastructure/Config/ConfigDefaults.h"

// Static config instance -- no heap allocation
static CpuConfig g_config;

static CpuConfig* get_config_impl(void) {
    return &g_config;
}

static int parse_impl(CpuConfig* config, const char* toml_data, uint32_t length) {
    return parse_config(config, toml_data, length);
}

const ConfigAPI g_config_service = {
    .get_config = get_config_impl,
    .parse = parse_impl
};
