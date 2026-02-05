#include "libkernel.h"

k_size_t k_strlen(const char *str) {
    k_size_t i = 0;
    while (str[i]) {
        i += 1;
    }

    return i;
}

char *k_strcpy(char *dst, const char *src) {
    k_size_t i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i += 1;
    }

    return dst;
}

int k_strcmp(const char *a, const char *b) {
    k_size_t i = 0;
    while (a[i] && b[i] && a[i] == b[i]) {
        i += 1;
    }

    return (int)a[i] - (int)b[i];
}

bool k_streq(const char *a, const char *b) {
    return k_strcmp(a, b) == 0;
}
