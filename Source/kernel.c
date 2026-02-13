#include "libkernel.h"
#include "tty.h"
#include "com.h"
#include "interrupts.h"
#include "keyboard.h"
#include "shell.h"
#include "memory.h"
#include "alloc.h"

void k_assertion_failure(const char *expr, const char *msg, const char *func, const char *filename, int line, bool panic) {
	k_print_stack();
	k_print_registers();

	k_printf("\x1b[31m");

	if (panic) {
		k_printf("Panic in ");
	} else {
		k_printf("Assertion failed in ");
	}

	k_printf("%s, %s:%d", func, filename, line);
	if (expr && expr[0]) {
		k_printf(" (%s)", expr);
	}
	k_printf(":\x1b[0m\n");
	k_printf("    %s\n", msg);

	k_pseudo_breakpoint();
}

void print_multiboot_info(const multiboot_info_t *info);

void kernel_main(uint32_t magic_number, const multiboot_info_t *multiboot_info) {
	com1_initialize();
	tty_initialize();

	if (magic_number != MULTIBOOT_BOOTLOADER_MAGIC) {
		k_printf("Error: bootloader is not multiboot compliant (magic number is %x)\n", magic_number);
		k_panic("Boot error");
		return;
	}

	print_multiboot_info(multiboot_info);

	interrupts_initialize();
	mem_init_with_multiboot_info(multiboot_info);
	kb_initialize();

	tty_clear(0);

	k_printf("Welcome to \x1b[32mPantheon OS\x1b[0m!\n\n");
	shell_print_help();
	shell_loop();
}
