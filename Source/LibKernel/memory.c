#include "libkernel.h"

void *k_memset(void *dst, k_byte value, k_size length) {
    for (k_size i = 0; i < length; i += 1) {
        ((k_byte *)dst)[i] = value;
    }

    return dst;
}

void *k_memcpy(void *dst, const void *src, k_size length) {
    for (k_size i = 0; i < length; i += 1) {
        ((k_byte *)dst)[i] = ((const k_byte *)src)[i];
    }

    return dst;
}

int k_memcmp(const void *a, const void *b, k_size length) {
    k_size i = 0;
    while (i < length && ((const k_byte *)a)[i] == ((const k_byte *)b)[i]) {
        i += 1;
    }

    if (i == length) {
        return 0;
    }

    return (int)((const k_byte *)a)[i] - (int)((const k_byte *)b)[i];
}
