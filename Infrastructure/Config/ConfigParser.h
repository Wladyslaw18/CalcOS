#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "ConfigDefaults.h"

// Parses a simple TOML-like string buffer and updates config.
// Returns 0 on success, or non-zero error code.
// No dynamic memory allocations are performed.
int parse_config(CpuConfig* config, const char* toml_data, uint32_t length);

#endif // CONFIG_PARSER_H
