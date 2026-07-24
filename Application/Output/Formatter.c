#include "Formatter.h"
#include "PrintInt.h"
#include "PrintFloat.h"

static void add_thousands_commas(char* out, size_t out_size, const char* in) {
    if (!out || out_size == 0 || !in) return;
    if (in[0] == 'N' || in[0] == 'I' || (in[0] == '-' && in[1] == 'I')) {
        size_t idx = 0;
        while (in[idx] != '\0' && idx < out_size - 1) {
            out[idx] = in[idx];
            idx++;
        }
        out[idx] = '\0';
        return;
    }
    const char* p = in;
    char* q = out;
    char* out_end = out + out_size - 1;
    if (*p == '-' || *p == '+') {
        if (q < out_end) *q++ = *p++;
    }
    const char* int_start = p;
    while (*p >= '0' && *p <= '9') {
        p++;
    }
    const char* int_end = p;
    size_t int_len = int_end - int_start;
    if (int_len == 0) {
        while (*in && q < out_end) {
            *q++ = *in++;
        }
        *q = '\0';
        return;
    }
    for (size_t i = 0; i < int_len; i++) {
        if (q >= out_end) break;
        *q++ = int_start[i];
        size_t remaining = int_len - 1 - i;
        if (remaining > 0 && remaining % 3 == 0) {
            if (q < out_end) *q++ = ',';
        }
    }
    while (*p && q < out_end) {
        *q++ = *p++;
    }
    *q = '\0';
}

size_t format_string(char* buf, size_t buf_size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t len = format_string_va(buf, buf_size, fmt, args);
    va_end(args);
    return len;
}

size_t format_string_va(char* buf, size_t buf_size, const char* fmt, va_list args) {
    if (buf_size == 0) return 0;

    size_t write_idx = 0;
    const char* p = fmt;

    while (*p != '\0' && write_idx < buf_size - 1) {
        if (*p == '%') {
            p++;
            if (*p == '\0') break;

            // Handle 'll' (long long) prefix for 64-bit integers
            int long_long = 0;
            if (*p == 'l' && *(p + 1) == 'l') {
                long_long = 1;
                p += 2;
                if (*p == '\0') break;
            }
            
            switch (*p) {
                case '%': {
                    buf[write_idx++] = '%';
                    p++;
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    buf[write_idx++] = c;
                    p++;
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) s = "(null)";
                    while (*s != '\0' && write_idx < buf_size - 1) {
                        buf[write_idx++] = *s++;
                    }
                    p++;
                    break;
                }
                case 'd':
                case 'i': {
                    int64_t val = long_long ? va_arg(args, long long) : (int64_t)va_arg(args, int);
                    char tmp[32];
                    size_t len = print_int(tmp, val, 10, false);
                    char tmp_commas[64];
                    add_thousands_commas(tmp_commas, sizeof(tmp_commas), tmp);
                    for (size_t i = 0; tmp_commas[i] != '\0' && write_idx < buf_size - 1; i++) {
                        buf[write_idx++] = tmp_commas[i];
                    }
                    p++;
                    break;
                }
                case 'u': {
                    uint64_t val = long_long ? va_arg(args, unsigned long long) : (uint64_t)va_arg(args, unsigned int);
                    char tmp[32];
                    size_t len = print_int(tmp, (int64_t)val, 10, false);
                    char tmp_commas[64];
                    add_thousands_commas(tmp_commas, sizeof(tmp_commas), tmp);
                    for (size_t i = 0; tmp_commas[i] != '\0' && write_idx < buf_size - 1; i++) {
                        buf[write_idx++] = tmp_commas[i];
                    }
                    p++;
                    break;
                }
                case 'x':
                case 'X': {
                    uint64_t val = long_long ? va_arg(args, unsigned long long) : (uint64_t)va_arg(args, unsigned int);
                    char tmp[32];
                    size_t len = print_int(tmp, (int64_t)val, 16, (*p == 'X'));
                    for (size_t i = 0; i < len && write_idx < buf_size - 1; i++) {
                        buf[write_idx++] = tmp[i];
                    }
                    p++;
                    break;
                }
                case 'f': {
                    double val = va_arg(args, double);
                    char tmp[64];
                    size_t len = print_float(tmp, val, 6);
                    char tmp_commas[128];
                    add_thousands_commas(tmp_commas, sizeof(tmp_commas), tmp);
                    for (size_t i = 0; tmp_commas[i] != '\0' && write_idx < buf_size - 1; i++) {
                        buf[write_idx++] = tmp_commas[i];
                    }
                    p++;
                    break;
                }
                default: {
                    buf[write_idx++] = '%';
                    if (!long_long && write_idx < buf_size - 1) {
                        buf[write_idx++] = *p;
                    }
                    p++;
                    break;
                }
            }
        } else {
            buf[write_idx++] = *p++;
        }
    }

    buf[write_idx] = '\0';
    return write_idx;
}
