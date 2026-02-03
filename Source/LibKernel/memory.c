#include "LibKernel/libkernel.h"

void *k_memset(void *dst, uint8_t value, int64_t length) {
    for (int64_t i = 0; i < length; i += 1) {
        ((uint8_t *)dst)[i] = value;
    }

    return dst;
}

void *k_memcpy(void *dst, const void *src, int64_t length) {
    for (int64_t i = 0; i < length; i += 1) {
        ((uint8_t *)dst)[i] = ((const uint8_t *)src)[i];
    }

    return dst;
}

int k_memcmp(const void *a, const void *b, int64_t length) {
    int64_t i = 0;
    while (i < length && ((const uint8_t *)a)[i] == ((const uint8_t *)b)[i]) {
        i += 1;
    }

    if (i == length) {
        return 0;
    }

    return (int)((const uint8_t *)a)[i] - (int)((const uint8_t *)b)[i];
}
