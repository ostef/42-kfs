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

	// Clear registers
	asm volatile(
		"xor %%eax, %%eax\n"
		"xor %%ebx, %%ebx\n"
		"xor %%ecx, %%ecx\n"
		"xor %%edx, %%edx\n"
		"xor %%esi, %%esi\n"
		"xor %%edi, %%edi\n"
		:
		:
		: "eax", "ebx", "ecx", "edx", "esi", "edi"
	);

	// Block interrupts
	asm volatile("cli");

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
	allocators_init();
	kb_initialize();

	tty_clear(0);

	k_printf("Welcome to \x1b[32mPantheon OS\x1b[0m!\n\n");
	uint32_t *ptr1 = vmalloc(10);
	k_printf("Allocated virtual memory at %p\n", ptr1);
	k_printf("Size %d\n", vsize(ptr1));
	vfree(ptr1);

	shell_print_help();

	// uint8_t *ptr = kmalloc(10);
	// k_size_t size = ksize(ptr);
	// *ptr = 'a';
	// k_printf("%p : %d, %c\n", ptr, size, *ptr);

	// ptr = kmalloc(10);
	// size = ksize(ptr);
	// *ptr = 'a';
	// k_printf("%p : %d, %c\n", ptr, size, *ptr);

	// kfree(ptr);
	// ptr = kmalloc(10);
	// size = ksize(ptr);
	// *ptr = 'b';
	// k_printf("%p : %d, %c\n", ptr, size, *ptr);

	// ptr = kmalloc(100);
	// size = ksize(ptr);
	// *ptr = 'a';
	// k_printf("%p : %d, %c\n", ptr, size, *ptr);

	// ptr = kmalloc(5000);
	// size = ksize(ptr);
	// *ptr = 'Z';
	// k_printf("%p : %d, %c\n", ptr, size, *ptr);

	// kfree(ptr);

	// ptr = kmalloc(6000);
	// size = ksize(ptr);
	// *ptr = 'E';
	// k_printf("%p : %d, %c\n", ptr, size, *ptr);

	// ptr = kmalloc(1024 * 1024);
	// k_printf("%p\n", ptr);

	shell_loop();
}
