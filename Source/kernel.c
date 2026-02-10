#include "libkernel.h"
#include "tty.h"
#include "com.h"
#include "interrupts.h"
#include "keyboard.h"
#include "shell.h"

void k_assertion_failure(const char *expr, const char *msg, const char *func, const char *filename, int line, bool panic) {
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

void kernel_main(void) {
	com1_initialize();
	tty_initialize();
	interrupts_initialize();
	kb_initialize();

	tty_clear(0);

	k_printf("Welcome to \x1b[32mPantheon OS\x1b[0m!\n\n");
	k_print_stack();
	shell_print_help();
	shell_loop();
}
