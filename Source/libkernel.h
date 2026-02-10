#ifndef LIBKERNEL_H
#define LIBKERNEL_H

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

#define KERNEL_CODE_SEGMENT 8
#define KERNEL_DATA_SEGMENT 16

#define LOW_8BITS(value) (uint8_t)((value) & 0xff)
#define HIGH_8BITS(value) (uint8_t)(((value) >> 8) & 0xff)

#define LOW_16BITS(value) (uint16_t)((value) & 0xffff)
#define HIGH_16BITS(value) (uint16_t)(((value) >> 16) & 0xffff)

// Helper macros
#define K_STRINGIFY(x) K_STRINGIFY2(x)
#define K_STRINGIFY2(x) #x

#define k_array_count(arr) (sizeof(arr) / sizeof(*(arr)))

#define k_pseudo_breakpoint() asm volatile("1: jmp 1b")

// Assertions and debugging
#define k_assert(expr, msg) do { if (!(expr)) { k_assertion_failure(K_STRINGIFY(expr), msg, __func__, __FILE__, __LINE__, false); } } while(0)
#define k_panic(msg) do { k_assertion_failure("", msg, __func__, __FILE__, __LINE__, true); } while(0)

void k_assertion_failure(
    const char *expr,
    const char *msg,
    const char *func,
    const char *filename,
    int line,
    bool panic
);

extern void stack_top;
extern void stack_bottom;

void *k_memset(void *dst, k_byte_t value, k_size_t length);
void *k_memcpy(void *dst, const void *src, k_size_t length);
int k_memcmp(const void *a, const void *b, k_size_t length);

k_size_t k_strlen(const char *str);
char *k_strcpy(char *dst, const char *src);
int k_strcmp(const char *a, const char *b);
int k_strncmp(const char *a, const char *b, k_size_t n);
bool k_streq(const char *a, const char *b);

bool k_is_print(char c);
bool k_is_alpha(char c);
bool k_is_digit(char c);
bool k_is_space(char c);

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

uint32_t k_get_esp(void);

k_size_t k_print_all_stack(int lines);
k_size_t k_print_stack(void);

#endif // LIBKERNEL_H
