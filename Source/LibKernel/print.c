#include "LibKernel/libkernel.h"
#include "terminal.h"

k_size k_print_char(char c) {
    terminal_putchar(c);
    return 1;
}

k_size k_print_str(const char *str) {
    k_size i = 0;
    while (str[i]) {
        k_print_char(str[i]);
        i += 1;
    }

    return i;
}

k_size k_print_uint_formatted(uint64_t x, K_FormatInt fmt) {
    if (fmt.base_n < 2 || !fmt.base) {
        return 0;
    }

    char buff[64]; // Max base 2 number is assumed to be 64 digits (64-bit ints)

    k_size length = 0;
    uint64_t x2 = x;
    while (length == 0 || x2 > 0) {
        length += 1;
        x2 /= fmt.base_n;
    }

    // Should not happen but as a safety measure
    if (length < 0) {
        length = 0;
    }
    if (length > (k_size)sizeof(buff)) {
        length = sizeof(buff);
    }

    for (k_size i = 0; i < length; i += 1) {
        uint64_t digit = x % fmt.base_n;
        x /= fmt.base_n;

        buff[length - i - 1] = fmt.base[digit];
    }

    k_size result = 0;
    for (k_size i = 0; i < fmt.min_digits - length; i += 1) {
        result += k_print_char(fmt.pad_char);
    }

    for (k_size i = 0; i < length; i += 1) {
        result += k_print_char(buff[i]);
    }

    return result;
}

k_size k_print_int_formatted(int64_t x, K_FormatInt fmt) {
    k_size result = 0;

    if (x < 0) {
        result += k_print_char('-');
        result += k_print_uint_formatted((uint64_t)-x, fmt);
    } else {
        result += k_print_uint_formatted((uint64_t)x, fmt);
    }

    return result;
}

k_size k_print_int(int x) {
    return k_print_int_formatted(x, (K_FormatInt){
            .min_digits=0,
            .pad_char=' ',
            .base_n=10,
            .base="0123456789",
        });
}

k_size k_print_uint(unsigned int x) {
    return k_print_uint_formatted(x, (K_FormatInt){
            .min_digits=0,
            .pad_char=' ',
            .base_n=10,
            .base="0123456789"
        });
}

k_size k_print_hex(unsigned int x) {
    return k_print_uint_formatted(x, (K_FormatInt){
            .min_digits=0,
            .pad_char=' ',
            .base_n=16,
            .base="0123456789abcdef"
        });
}

k_size k_print_ptr(const void *ptr) {
    k_size result= k_print_str("0x");
    result += k_print_uint_formatted((uintptr_t)ptr, (K_FormatInt){
            .min_digits=sizeof(uintptr_t),
            .pad_char='0',
            .base_n=16,
            .base="0123456789abcdef"
        });

    return result;
}

k_size k_vprintf(const char *fmt, va_list va) {
    k_size result = 0;
    k_size i = 0;
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

            case 'd':
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

            case 'p': {
                const void *p = va_arg(va, const void *);
                result += k_print_ptr(p);
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

k_size k_printf(const char *fmt, ...) {
    va_list va;

    va_start(va, fmt);
    k_size result = k_vprintf(fmt, va);
    va_end(va);

    return result;
}
