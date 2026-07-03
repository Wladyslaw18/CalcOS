#include "ConfigParser.h"

static int is_whitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

static int is_digit(char c) {
    return (c >= '0' && c <= '9');
}

static int string_compare(const char* s1, const char* s2, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (s1[i] != s2[i]) return 0;
        if (s2[i] == '\0') return 1;
    }
    return 1;
}

static uint32_t parse_unsigned(const char* start, const char* end) {
    uint32_t val = 0;
    while (start < end) {
        if (is_digit(*start)) {
            val = val * 10 + (*start - '0');
        }
        start++;
    }
    return val;
}

int parse_config(CpuConfig* config, const char* toml_data, uint32_t length) {
    if (!config || !toml_data) return -1;

    uint32_t i = 0;
    while (i < length) {
        while (i < length && is_whitespace(toml_data[i])) {
            i++;
        }
        if (i >= length) break;

        if (toml_data[i] == '#') {
            while (i < length && toml_data[i] != '\n' && toml_data[i] != '\r') {
                i++;
            }
            continue;
        }

        if (toml_data[i] == '[') {
            while (i < length && toml_data[i] != '\n' && toml_data[i] != '\r') {
                i++;
            }
            continue;
        }

        uint32_t key_start = i;
        while (i < length && toml_data[i] != '=' && toml_data[i] != '\n' && toml_data[i] != '\r' && toml_data[i] != '#') {
            i++;
        }

        if (i >= length || toml_data[i] != '=') {
            while (i < length && toml_data[i] != '\n' && toml_data[i] != '\r') {
                i++;
            }
            continue;
        }

        uint32_t key_end = i;
        i++; // skip '='

        while (key_end > key_start && is_whitespace(toml_data[key_end - 1])) {
            key_end--;
        }
        while (key_start < key_end && is_whitespace(toml_data[key_start])) {
            key_start++;
        }

        while (i < length && is_whitespace(toml_data[i]) && toml_data[i] != '\n' && toml_data[i] != '\r') {
            i++;
        }

        uint32_t val_start = i;
        while (i < length && toml_data[i] != '\n' && toml_data[i] != '\r' && toml_data[i] != '#') {
            i++;
        }

        uint32_t val_end = i;
        while (val_end > val_start && is_whitespace(toml_data[val_end - 1])) {
            val_end--;
        }

        uint32_t key_len = key_end - key_start;
        uint32_t val_len = val_end - val_start;

        if (key_len == 0 || val_len == 0) {
            continue;
        }

        if (key_len == 17 && string_compare(toml_data + key_start, "cpu_frequency_mhz", 17)) {
            config->cpu_frequency_mhz = parse_unsigned(toml_data + val_start, toml_data + val_end);
        } else if (key_len == 21 && string_compare(toml_data + key_start, "enable_hyperthreading", 21)) {
            config->enable_hyperthreading = (uint8_t)parse_unsigned(toml_data + val_start, toml_data + val_end);
        } else if (key_len == 10 && string_compare(toml_data + key_start, "enable_sse", 10)) {
            config->enable_sse = (uint8_t)parse_unsigned(toml_data + val_start, toml_data + val_end);
        } else if (key_len == 10 && string_compare(toml_data + key_start, "enable_avx", 10)) {
            config->enable_avx = (uint8_t)parse_unsigned(toml_data + val_start, toml_data + val_end);
        } else if (key_len == 15 && string_compare(toml_data + key_start, "cache_line_size", 15)) {
            config->cache_line_size = parse_unsigned(toml_data + val_start, toml_data + val_end);
        } else if (key_len == 9 && string_compare(toml_data + key_start, "log_level", 9)) {
            config->log_level = parse_unsigned(toml_data + val_start, toml_data + val_end);
        }
    }

    return 0;
}
