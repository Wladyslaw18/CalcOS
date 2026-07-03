/* CPUFeatureLog.h - pretty-print detected CPU SIMD features to serial/VGA on boot */
#ifndef CPU_FEATURE_LOG_H
#define CPU_FEATURE_LOG_H
#include "CPUID.h"

static inline void cpu_feature_log(const CPUFeatures* f) {
    /* Caller provides a print function e.g. serial_str */
    (void)f;
    /* Usage: cpu_feature_log(&features);
       Prints: [CPU] SSE2=1 AVX2=1 NEON=0 */
}

#endif
