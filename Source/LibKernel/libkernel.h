#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

void *k_memset(void *dst, uint8_t value, int64_t length);
void *k_memcpy(void *dst, const void *src, int64_t length);
int k_memcmp(const void *a, const void *b, int64_t length);

int64_t k_strlen(const char *str);
char *k_strcpy(char *dst, const char *src);
int k_strcmp(const char *a, const char *b);
bool k_streq(const char *a, const char *b);

int k_print_char(char c);
int k_print_str(const char *str);
int k_print_uint_base(unsigned int x, unsigned int base_n, const char *base);
int k_print_int(int x);
int k_print_uint(unsigned int x);
int k_print_hex(unsigned int x);

int k_vprintf(const char *fmt, va_list va);
int k_printf(const char *fmt, ...);
