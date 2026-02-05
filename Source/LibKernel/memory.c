#include "libkernel.h"

void *k_memset(void *dst, k_byte_t value, k_size_t length) {
	for (k_size_t i = 0; i < length; i += 1) {
		((k_byte_t *)dst)[i] = value;
	}

	return dst;
}

void *k_memcpy(void *dst, const void *src, k_size_t length) {
	if (dst < src) {
		for (k_size_t i = 0; i < length; i += 1) {
			((k_byte_t *)dst)[i] = ((const k_byte_t *)src)[i];
		}
	} else if (dst > src) {
		for (k_size_t i = length - 1; i >= 0; i -= 1) {
			((k_byte_t *)dst)[i] = ((const k_byte_t *)src)[i];
		}
	}

	return dst;
}

int k_memcmp(const void *a, const void *b, k_size_t length) {
	k_size_t i = 0;
	while (i < length && ((const k_byte_t *)a)[i] == ((const k_byte_t *)b)[i]) {
		i += 1;
	}

	if (i == length) {
		return 0;
	}

	return (int)((const k_byte_t *)a)[i] - (int)((const k_byte_t *)b)[i];
}
