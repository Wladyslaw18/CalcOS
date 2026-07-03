#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

#include <stdint.h>

typedef struct {
    uint32_t cpu_frequency_mhz;
    uint8_t enable_hyperthreading;
    uint8_t enable_sse;
    uint8_t enable_avx;
    uint32_t cache_line_size;
    uint32_t log_level;
} CpuConfig;

// Default CPU configuration presets
#define DEFAULT_CPU_FREQ_MHZ       3000
#define DEFAULT_ENABLE_HT          1
#define DEFAULT_ENABLE_SSE         1
#define DEFAULT_ENABLE_AVX         1
#define DEFAULT_CACHE_LINE_SIZE    64
#define DEFAULT_LOG_LEVEL          1 // INFO

static inline void load_default_config(CpuConfig* config) {
    if (!config) return;
    config->cpu_frequency_mhz = DEFAULT_CPU_FREQ_MHZ;
    config->enable_hyperthreading = DEFAULT_ENABLE_HT;
    config->enable_sse = DEFAULT_ENABLE_SSE;
    config->enable_avx = DEFAULT_ENABLE_AVX;
    config->cache_line_size = DEFAULT_CACHE_LINE_SIZE;
    config->log_level = DEFAULT_LOG_LEVEL;
}

#endif // CONFIG_DEFAULTS_H
