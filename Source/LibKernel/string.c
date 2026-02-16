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

int k_strncmp(const char *a, const char *b, k_size_t n) {
	k_size_t i = 0;
	while (i < n && a[i] && b[i] && a[i] == b[i]) {
		i += 1;
	}

	if (i == n) {
		return 0;
	}

	return (int)a[i] - (int)b[i];
}

bool k_streq(const char *a, const char *b) {
	return k_strcmp(a, b) == 0;
}

bool k_is_print(char c) {
	return c >= ' ' && c < 127;
}

bool k_is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool k_is_digit(char c) {
	return c >= '0' && c <= '9';
}

bool k_is_space(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

uint32_t k_str_to_uint32(const char *str, k_size_t len) {
	int base = 10;
	if (len >= 2 && k_strncmp(str, "0x", 2) == 0) {
		base = 16;
		str += 2;
		len -= 2;
	}

	uint32_t v = 0;
	k_size_t i = 0;
	while (i < len) {
		int digit = 0;
		if (base == 16 && str[i] >= 'a' && str[i] <= 'f') {
			digit = 10 + str[i] - 'a';
		} else if (base == 16 && str[i] >= 'A' && str[i] <= 'F') {
			digit = 10 + str[i] - 'A';
		} else if (str[i] >= '0' && str[i] <= '9') {
			digit = str[i] - '0';
		} else {
			break;
		}

		v *= base;
		v += digit;

		i += 1;
	}

	return v;
}
