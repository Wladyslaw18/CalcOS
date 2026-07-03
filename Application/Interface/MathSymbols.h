#ifndef MATH_SYMBOLS_H
#define MATH_SYMBOLS_H

#include <stdint.h>

// Retrieves the 8x16 bitmap representation for the specified ASCII character or math symbol.
// If the glyph is not defined, returns a default fallback character box.
const uint8_t* math_symbols_get_glyph(char c);

#endif // MATH_SYMBOLS_H
