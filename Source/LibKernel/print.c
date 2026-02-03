#include "LibKernel/libkernel.h"

int k_print_char(char c) {
    // @Todo
    (void)c;

    return 1;
}

int k_print_str(const char *str) {
    int64_t i = 0;
    while (str[i]) {
        k_print_char(str[i]);
        i += 1;
    }

    return i;
}

int k_print_int(int x) {
    int64_t result = 0;

    if (x < 0) {
        result += k_print_char('-');
        result += k_print_uint((unsigned int)-x);
    } else {
        result += k_print_uint((unsigned int)x);
    }

    return result;
}

int k_print_uint_base(unsigned int x, unsigned int base_n, const char *base) {
    if (base_n < 2) {
        return 0;
    }

    char buff[64];

    int64_t length = 0;
    unsigned int x2 = x;
    while (length == 0 || x2 > 0) {
        length += 1;
        x2 /= 10;
    }

    // Should not happen but as a safety measure
    if (length < 0) {
        length = 0;
    }
    if (length >= sizeof(buff)) {
        length = sizeof(buff);
    }

    int64_t i = 0;
    while (i < length) {
        unsigned int digit = x % base_n;
        x /= base_n;

        buff[length - i - 1] = base[digit];

        i += 1;
    }

    int64_t result = 0;
    i = 0;
    while (i < length) {
        result += k_print_char(buff[i]);
        i += 1;
    }

    return result;
}

int k_print_uint(unsigned int x) {
    return k_print_uint_base(x, 10, "0123456789");
}

int k_print_hex(unsigned int x) {
    return k_print_uint_base(x, 16, "0123456789abcdef");
}

int k_vprintf(const char *fmt, va_list va) {
    int64_t result = 0;
    int64_t i = 0;
    while (fmt[i]) {
        if (fmt[i] == '%') {
            i += 1;
            if (!fmt[i]) {
                return result;
            }

            switch(fmt[i]) {
            case 's': {
                const char *str = va_arg(va, const char *);
                result += k_print_str(str);
            } break;

            case 'n':
            case 'i': {
                int i = va_arg(va, int);
                result += k_print_int(i);
            } break;

            case 'u': {
                unsigned int u = va_arg(va, unsigned int);
                result += k_print_uint(u);
            } break;

            case 'x': {
                unsigned int u = va_arg(va, unsigned int);
                result += k_print_hex(u);
            } break;

            case 'c': {
                char c = (char)va_arg(va, int);
                result += k_print_char(c);
            } break;
            }
        } else {
            result += k_print_char(fmt[i]);
        }

        i += 1;
    }

    return result;
}

int k_printf(const char *fmt, ...) {
    va_list va;

    va_start(va, fmt);
    int result = k_vprintf(fmt, va);
    va_end(va);

    return result;
}
