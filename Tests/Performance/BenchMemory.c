#include <stdio.h>
#include <x86intrin.h> // For __rdtsc()
#include "../../Infrastructure/Utils/MemoryUtils.h"

#define BUFFER_SIZE 65536
#define ITERATIONS 2000

uint8_t src_buf[BUFFER_SIZE] __attribute__((aligned(64)));
uint8_t dest_buf[BUFFER_SIZE] __attribute__((aligned(64)));

int main() {
    printf("=== RUNNING BenchMemory ===\n");

    // Initialize source buffer
    for (int i = 0; i < BUFFER_SIZE; i++) {
        src_buf[i] = (uint8_t)(i & 0xFF);
    }

    // Warm-up cache
    fast_memcpy(dest_buf, src_buf, BUFFER_SIZE);

    // Bench fast_memcpy
    uint64_t t0 = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        fast_memcpy(dest_buf, src_buf, BUFFER_SIZE);
    }
    uint64_t t1 = __rdtsc();
    uint64_t cycles_memcpy = (t1 - t0) / ITERATIONS;

    // Bench fast_memset
    t0 = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        fast_memset(dest_buf, 0xAA, BUFFER_SIZE);
    }
    t1 = __rdtsc();
    uint64_t cycles_memset = (t1 - t0) / ITERATIONS;

    printf("Results (Average CPU cycles for buffer size %d):\n", BUFFER_SIZE);
    printf("  fast_memcpy: %llu cycles\n", cycles_memcpy);
    printf("  fast_memset: %llu cycles\n", cycles_memset);

    return 0;
}
