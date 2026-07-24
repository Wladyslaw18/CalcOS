#include "PrintInt.h"

static const char digits_lower[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char digits_upper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

size_t print_int(char* buf, int64_t val, int base, bool uppercase) {
    if (base < 2 || base > 36) {
        if (buf) buf[0] = '\0';
        return 0;
    }
    
    char temp[66];
    size_t temp_idx = 0;
    bool is_neg = false;
    uint64_t uval;
    
    if (val < 0 && base == 10) {
        is_neg = true;
        uval = (uint64_t)0 - (uint64_t)val;
    } else {
        uval = (uint64_t)val;
    }
    
    const char* digits = uppercase ? digits_upper : digits_lower;
    
    do {
        temp[temp_idx++] = digits[uval % (uint64_t)base];
        uval /= (uint64_t)base;
    } while (uval > 0);
    
    if (is_neg) {
        temp[temp_idx++] = '-';
    }
    
    for (size_t i = 0; i < temp_idx; i++) {
        buf[i] = temp[temp_idx - 1 - i];
    }
    buf[temp_idx] = '\0';
    
    return temp_idx;
}
