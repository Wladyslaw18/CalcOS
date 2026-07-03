#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <stdint.h>
#include <stddef.h>

// Zero-allocation, high-performance block memory copying. NO LIBC INCLUDED!
void fast_memcpy(void* dest, const void* src, size_t n);

// Zero-allocation memory filler
void fast_memset(void* s, int c, size_t n);

#endif // MEMORY_UTILS_H
