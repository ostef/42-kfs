#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#if __SIZEOF_SIZE_T__ == 4
typedef int32_t k_size;
typedef uint32_t k_usize;
#elif __SIZEOF_SIZE_T == 8
typedef int64_t k_size;
typedef uint64_t k_usize;
#else
#error "Expected 32 or 64 bits for size_t type"
#endif

typedef int8_t k_sbyte;
typedef uint8_t k_byte;

void *k_memset(void *dst, k_byte value, k_size length);
void *k_memcpy(void *dst, const void *src, k_size length);
int k_memcmp(const void *a, const void *b, k_size length);

k_size k_strlen(const char *str);
char *k_strcpy(char *dst, const char *src);
int k_strcmp(const char *a, const char *b);
bool k_streq(const char *a, const char *b);

typedef struct K_FormatInt {
    int min_digits;
    char pad_char;
    unsigned int base_n;
    const char *base;
} K_FormatInt;

k_size k_print_char(char c);
k_size k_print_str(const char *str);
k_size k_print_uint_formatted(uint64_t x, K_FormatInt fmt);
k_size k_print_int_formatted(int64_t x, K_FormatInt fmt);
k_size k_print_int(int x);
k_size k_print_uint(unsigned int x);
k_size k_print_hex(unsigned int x);
k_size k_print_bin(unsigned int x);
k_size k_print_ptr(const void *ptr);

k_size k_vprintf(const char *fmt, va_list va);
k_size k_printf(const char *fmt, ...);
