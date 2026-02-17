#include "libkernel.h"
#include "tty.h"
#include "com.h"
#include "interrupts.h"
#include "keyboard.h"
#include "shell.h"
#include "memory.h"
#include "alloc.h"
#include "tss.h"

void k_assertion_failure(const char *expr, const char *msg, const char *func, const char *filename, int line, bool panic) {
	k_print_stack();

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

typedef struct page_fault_t {
	uint32_t protection_violation : 1;
	uint32_t write_access : 1;
	uint32_t user : 1;
	uint32_t reserved_write : 1;
	uint32_t instruction_fetch : 1;
	uint32_t protection_key_violation : 1;
	uint32_t shadow_stack_access : 1;
	uint32_t sgx_violation : 1;
} page_fault_t;

void handle_page_fault(interrupt_registers_t registers) {
	uint32_t virt_addr;
	asm volatile("mov %%cr2, %0" : "=r"(virt_addr));

	page_fault_t fault = *(page_fault_t *)&registers.error_code;

	const char *access = fault.write_access ? "writing" : "reading";
	k_printf("Page fault when accessing address %p for %s (error code is %p)\n", virt_addr, access, registers.error_code);

	if (fault.protection_violation) {
		k_printf("Accessed a protected page\n");
	} else {
		k_printf("Accessed a page that is not present\n");
	}

	k_pseudo_breakpoint();
}

void print_multiboot_info(const multiboot_info_t *info);

void kernel_main(uint32_t magic_number, const multiboot_info_t *multiboot_info) {
	tty_initialize();
	com1_initialize();
	init_tss();

	if (magic_number != MULTIBOOT_BOOTLOADER_MAGIC) {
		k_printf("Error: bootloader is not multiboot compliant (magic number is %x)\n", magic_number);
		k_panic("Boot error");
		return;
	}

	print_multiboot_info(multiboot_info);

	interrupts_initialize();
	interrupt_register_handler(INT_PAGE_FAULT, handle_page_fault);
	mem_init_with_multiboot_info(multiboot_info);
	kmalloc_init();
	vmalloc_init();
	kb_initialize();

	tty_clear(0);

	k_printf("Welcome to \x1b[32mPantheon OS\x1b[0m!\n\n");
	// jump_usermode();
	shell_print_help();
	shell_loop();
}
