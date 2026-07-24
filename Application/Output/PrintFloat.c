#include "PrintFloat.h"
#include "PrintInt.h"

size_t print_float(char* buf, double val, int precision) {
    if (val != val) {
        buf[0] = 'N'; buf[1] = 'a'; buf[2] = 'N'; buf[3] = '\0';
        return 3;
    }
    if (val >= 1.8e19) {
        buf[0] = 'I'; buf[1] = 'n'; buf[2] = 'f'; buf[3] = '\0';
        return 3;
    }
    if (val <= -1.8e19) {
        buf[0] = '-'; buf[1] = 'I'; buf[2] = 'n'; buf[3] = 'f'; buf[4] = '\0';
        return 4;
    }

    size_t idx = 0;
    if (val < 0.0) {
        buf[idx++] = '-';
        val = -val;
    }

    double round = 0.5;
    for (int i = 0; i < precision; i++) {
        round /= 10.0;
    }
    val += round;

    uint64_t int_part = (uint64_t)val;
    double frac_part = val - (double)int_part;

    char int_buf[32];
    size_t int_len = print_int(int_buf, int_part, 10, false);
    for (size_t i = 0; i < int_len; i++) {
        buf[idx++] = int_buf[i];
    }

    if (precision > 0) {
        buf[idx++] = '.';
        for (int i = 0; i < precision; i++) {
            frac_part *= 10.0;
            uint32_t digit = (uint32_t)frac_part;
            buf[idx++] = (char)('0' + (digit % 10));
            frac_part -= digit;
        }
    }
    buf[idx] = '\0';
    return idx;
}
