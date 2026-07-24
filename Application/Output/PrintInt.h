#ifndef PRINT_INT_H
#define PRINT_INT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

size_t print_int(char* buf, int64_t val, int base, bool uppercase);

#endif // PRINT_INT_H
