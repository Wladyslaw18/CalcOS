#include "Tangent.h"
#include "Sine.h"
#include "Cosine.h"
#include "../Arithmetic/Division.h"

#define CHUNK_SIZE 64

void tan_scalar(CalculatorState* state, const double* a, double* result, uint32_t count) {
    double sin_buf[CHUNK_SIZE];
    double cos_buf[CHUNK_SIZE];
    
    for (uint32_t offset = 0; offset < count; offset += CHUNK_SIZE) {
        uint32_t current_chunk = (count - offset < CHUNK_SIZE) ? (count - offset) : CHUNK_SIZE;
        sin_scalar(state, &a[offset], sin_buf, current_chunk);
        cos_scalar(state, &a[offset], cos_buf, current_chunk);
        div_scalar(state, sin_buf, cos_buf, &result[offset], current_chunk);
    }
}

void tan_sse(CalculatorState* state, const double* a, double* result, uint32_t count) {
    double sin_buf[CHUNK_SIZE];
    double cos_buf[CHUNK_SIZE];
    
    for (uint32_t offset = 0; offset < count; offset += CHUNK_SIZE) {
        uint32_t current_chunk = (count - offset < CHUNK_SIZE) ? (count - offset) : CHUNK_SIZE;
        sin_sse(state, &a[offset], sin_buf, current_chunk);
        cos_sse(state, &a[offset], cos_buf, current_chunk);
        div_sse(state, sin_buf, cos_buf, &result[offset], current_chunk);
    }
}

void tan_avx2(CalculatorState* state, const double* a, double* result, uint32_t count) {
    double sin_buf[CHUNK_SIZE];
    double cos_buf[CHUNK_SIZE];
    
    for (uint32_t offset = 0; offset < count; offset += CHUNK_SIZE) {
        uint32_t current_chunk = (count - offset < CHUNK_SIZE) ? (count - offset) : CHUNK_SIZE;
        sin_avx2(state, &a[offset], sin_buf, current_chunk);
        cos_avx2(state, &a[offset], cos_buf, current_chunk);
        div_avx2(state, sin_buf, cos_buf, &result[offset], current_chunk);
    }
}

void tan_neon(CalculatorState* state, const double* a, double* result, uint32_t count) {
    double sin_buf[CHUNK_SIZE];
    double cos_buf[CHUNK_SIZE];
    
    for (uint32_t offset = 0; offset < count; offset += CHUNK_SIZE) {
        uint32_t current_chunk = (count - offset < CHUNK_SIZE) ? (count - offset) : CHUNK_SIZE;
        sin_neon(state, &a[offset], sin_buf, current_chunk);
        cos_neon(state, &a[offset], cos_buf, current_chunk);
        div_neon(state, sin_buf, cos_buf, &result[offset], current_chunk);
    }
}

void execute_tangent(CalculatorState* state, const double* a, double* result, uint32_t count, const CPUFeatures* features) {
    if (features->has_neon) {
        tan_neon(state, a, result, count);
    } else if (features->has_avx2) {
        tan_avx2(state, a, result, count);
    } else if (features->has_sse2) {
        tan_sse(state, a, result, count);
    } else {
        tan_scalar(state, a, result, count);
    }
}
