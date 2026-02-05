#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#if __SIZEOF_SIZE_T__ == 4
typedef int32_t k_size_t;
typedef uint32_t k_usize_t;
#elif __SIZEOF_SIZE_T == 8
typedef int64_t k_size_t;
typedef uint64_t k_usize_t;
#else
#error "Expected 32 or 64 bits for size_t type"
#endif

typedef int8_t k_sbyte_t;
typedef uint8_t k_byte_t;

void *k_memset(void *dst, k_byte_t value, k_size_t length);
void *k_memcpy(void *dst, const void *src, k_size_t length);
int k_memcmp(const void *a, const void *b, k_size_t length);

k_size_t k_strlen(const char *str);
char *k_strcpy(char *dst, const char *src);
int k_strcmp(const char *a, const char *b);
bool k_streq(const char *a, const char *b);

typedef struct k_format_int_t {
    int min_digits;
    char pad_char;
    unsigned int base_n;
    const char *base;
} k_format_int_t;

k_size_t k_print_char(char c);
k_size_t k_print_str(const char *str);
k_size_t k_print_uint_formatted(uint64_t x, k_format_int_t fmt);
k_size_t k_print_int_formatted(int64_t x, k_format_int_t fmt);
k_size_t k_print_int(int x);
k_size_t k_print_uint(unsigned int x);
k_size_t k_print_hex(unsigned int x);
k_size_t k_print_bin(unsigned int x);
k_size_t k_print_ptr(const void *ptr);

k_size_t k_vprintf(const char *fmt, va_list va);
k_size_t k_printf(const char *fmt, ...);

k_size_t k_printf(const char *fmt, ...);
