#include "libkernel.h"
#include "tty.h"
#include "com.h"

k_size_t k_print_char(char c) {
	tty_putchar(tty_get_active(), c);
	com1_write_byte(c);
	return 1;
}

k_size_t k_print_str(const char *str) {
    k_size_t i;

    i = 0;
    while (str[i]) {
        k_print_char(str[i++]);
    }

    return i;
}

k_size_t k_print_uint_formatted(uint64_t x, k_format_int_t fmt) {
	if (fmt.base_n < 2 || !fmt.base) {
		return 0;
	}

	char buff[64]; // Max base 2 number is assumed to be 64 digits (64-bit ints)

	k_size_t length = 0;
	uint64_t x2 = x;
	while (length == 0 || x2 > 0) {
		length += 1;
		x2 /= fmt.base_n;
	}

	// Should not happen but as a safety measure
	if (length < 0) {
		length = 0;
	}
	if (length > (k_size_t)sizeof(buff)) {
		length = sizeof(buff);
	}

	for (k_size_t i = 0; i < length; i += 1) {
		uint64_t digit = x % fmt.base_n;
		x /= fmt.base_n;

		buff[length - i - 1] = fmt.base[digit];
	}

	k_size_t result = 0;
	for (k_size_t i = 0; i < fmt.min_digits - length; i += 1) {
		result += k_print_char(fmt.pad_char);
	}

	for (k_size_t i = 0; i < length; i += 1) {
		result += k_print_char(buff[i]);
	}

	return result;
}

k_size_t k_print_int_formatted(int64_t x, k_format_int_t fmt) {
	k_size_t result = 0;

	if (x < 0) {
		result += k_print_char('-');
		result += k_print_uint_formatted((uint64_t)-x, fmt);
	} else {
		result += k_print_uint_formatted((uint64_t)x, fmt);
	}

	return result;
}

k_size_t k_print_int(int x) {
	return k_print_int_formatted(x, (k_format_int_t){
			.min_digits=0,
			.pad_char=' ',
			.base_n=10,
			.base="0123456789",
		});
}

k_size_t k_print_uint(unsigned int x) {
	return k_print_uint_formatted(x, (k_format_int_t){
			.min_digits=0,
			.pad_char=' ',
			.base_n=10,
			.base="0123456789"
		});
}

k_size_t k_print_hex(unsigned int x) {
	return k_print_uint_formatted(x, (k_format_int_t){
			.min_digits=0,
			.pad_char=' ',
			.base_n=16,
			.base="0123456789abcdef"
		});
}

k_size_t k_print_bin(unsigned int x) {
	return k_print_uint_formatted(x, (k_format_int_t){
			.min_digits=0,
			.pad_char=' ',
			.base_n=2,
			.base="01"
		});
}

k_size_t k_print_pretty_size(unsigned int x) {
	if (x < 1024) {
        k_print_uint(x);
        k_print_str(" B");
    } else if (x < 1024 * 1024) {
        k_print_uint(x / 1024);
        k_print_str(" KiB");
    } else if (x < 1024 * 1024 * 1024) {
        k_print_uint(x / (1024 * 1024));
        k_print_str(" MiB");
    } else {
        k_print_uint(x / (1024 * 1024 * 1024));
        k_print_str(" GiB");
    }
}

k_size_t k_print_ptr(const void *ptr) {
	k_size_t result= k_print_str("0x");
	result += k_print_uint_formatted((uintptr_t)ptr, (k_format_int_t){
			.min_digits=sizeof(uintptr_t) * 2,
			.pad_char='0',
			.base_n=16,
			.base="0123456789abcdef"
		});

	return result;
}

k_size_t k_vprintf(const char *fmt, va_list va) {
	k_size_t result = 0;
	k_size_t i = 0;
	while (fmt[i]) {
		if (fmt[i] == '%') {
			i += 1;

            int min_digits = 1;

            if (fmt[i] == '.') {
                i += 1;
                min_digits = 0;

                while (fmt[i] >= '0' && fmt[i] <= '9') {
                    min_digits *= 10;
                    min_digits += fmt[i] - '0';
                    i += 1;
                }
            }

			switch(fmt[i]) {
			case 'S': {
				k_size_t len = va_arg(va, k_size_t);
				const char *str = va_arg(va, const char *);
				for (k_size_t i = 0; i < len; i += 1) {
					result += k_print_char(str[i]);
				}
			} break;

			case 's': {
				const char *str = va_arg(va, const char *);
				result += k_print_str(str);
			} break;

			case 'd':
			case 'i': {
				int i = va_arg(va, int);
				result += k_print_int_formatted(i, (k_format_int_t){
					.min_digits=min_digits,
					.pad_char='0',
					.base_n=10,
					.base="0123456789",
				});
			} break;

			case 'u': {
				unsigned int u = va_arg(va, unsigned int);
				result += k_print_uint_formatted(u, (k_format_int_t){
					.min_digits=min_digits,
					.pad_char='0',
					.base_n=10,
					.base="0123456789",
				});
			} break;

			case 'x': {
				unsigned int u = va_arg(va, unsigned int);
				result += k_print_uint_formatted(u, (k_format_int_t){
					.min_digits=min_digits,
					.pad_char='0',
					.base_n=16,
					.base="0123456789ABCDEF",
				});
			} break;

			case 'n': {
				unsigned int u = va_arg(va, unsigned int);
				result += k_print_pretty_size(u);
			} break;

			case 'b': {
				unsigned int u = va_arg(va, unsigned int);
				result += k_print_uint_formatted(u, (k_format_int_t){
					.min_digits=min_digits,
					.pad_char='0',
					.base_n=2,
					.base="01",
				});
			} break;

			case 'p': {
				const void *p = va_arg(va, const void *);
				result += k_print_ptr(p);
			} break;

			case 'c': {
				char c = (char)va_arg(va, int);
				result += k_print_char(c);
			} break;

			default : {
				result += k_print_char(fmt[i]);
			} break;
			}
		} else {
			result += k_print_char(fmt[i]);
		}

		i += 1;
	}

	return result;
}

k_size_t k_printf(const char *fmt, ...) {
	va_list va;

	va_start(va, fmt);
	k_size_t result = k_vprintf(fmt, va);
	va_end(va);

	return result;
}

k_size_t k_print_all_stack(int lines)
{
	k_size_t top;
	k_size_t i = 0;
	k_size_t result = 0;

	top = (k_size_t)&stack_top - sizeof(uint32_t) * 4;
	result += k_print_str("Stack top:\n");
	while (top >= (k_size_t)&stack_bottom) {
		if (lines > 0 && i >= (k_size_t)lines) {
			break;
		}
		uint32_t *ptr = (uint32_t *)top;
		result += k_printf("%p %p %p %p\n",
         (void *)ptr[0], (void *)ptr[1], (void *)ptr[2], (void *)ptr[3]);
		top -= sizeof(uint32_t) * 4;
		i++;
	}
	result += k_print_str("Stack bottom\n");
	return result;
}

k_size_t k_print_stack(void)
{
	uint32_t	esp;
	k_size_t	result = 0;

	esp = k_get_esp();
	result += k_print_str("Stack contents (ESP):\n");
	while (esp < (uint32_t)&stack_top) {
		char *str = (char *)esp;
		uint32_t *ptr = (uint32_t *)esp;

		result += k_printf(
			"%p %p %p %p  ",
        	(void *)ptr[0], (void *)ptr[1], (void *)ptr[2], (void *)ptr[3]
		);

		for (int i = 0; i < 4 * 4; i += 1) {
			if (!k_is_print(str[i]) || str[i] == '\n' || str[i] == '\t') {
				result += k_print_char('.');
			} else {
				result += k_print_char(str[i]);
			}
		}

		result += k_print_char('\n');

		esp += sizeof(uint32_t) * 4;
	}

	result += k_print_str("Stack top\n\n");

	return result;
}

k_size_t k_print_registers(void) {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp;

    asm volatile (
        "mov %%eax, %0\n"
        "mov %%ebx, %1\n"
        "mov %%ecx, %2\n"
        "mov %%edx, %3\n"
        "mov %%esi, %4\n"
        "mov %%edi, %5\n"
        "mov %%ebp, %6\n"
        "mov %%esp, %7\n"
        : "=m"(eax), "=m"(ebx), "=m"(ecx), "=m"(edx),
          "=m"(esi), "=m"(edi), "=m"(ebp), "=m"(esp)
    );

	k_size_t result = 0;
	result += k_printf("Registers:\n");
    result += k_printf("AX=%x ", eax);
    result += k_printf("BX=%x ", ebx);
    result += k_printf("CX=%x ", ecx);
    result += k_printf("DX=%x ", edx);
    result += k_printf("SI=%x ", esi);
    result += k_printf("DI=%x ", edi);
    result += k_printf("BP=%x ", ebp);
    result += k_printf("SP=%x ", esp);
	result += k_printf("\n");

	return result;
}
