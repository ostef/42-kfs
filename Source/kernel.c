#include "libkernel.h"
#include "tty.h"
#include "com.h"
#include "interrupts.h"
#include "keyboard.h"

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

	k_printf("Welcome to Pantheon OS\n");
	k_printf("42\n");
	k_printf("kernel_main=%p\n", kernel_main);
	k_printf("This is some \x1b[31mred\x1b[0m text.\n");
	k_printf("This is some \x1b[41mred\x1b[0m text.\n");

	while (true) {
		kb_event_t kb;
		if (kb_poll_event(&kb)) {
			switch (kb.type) {
			case KB_EVENT_TEXT_INPUT: {
				k_printf("%c", kb.ascii_value);
			} break;

			case KB_EVENT_PRESS:
			case KB_EVENT_REPEAT: {
				if (kb.scancode == KB_SCANCODE_RETURN) {
					k_printf("\n");
				}

				if (kb_get_mod_state() == (KB_MOD_CTRL | KB_MOD_SHIFT)) {
					if (kb.scancode >= KB_SCANCODE_1 && kb.scancode <= KB_SCANCODE_0) {
						tty_set_active(kb.scancode - KB_SCANCODE_1);
					} else if (kb.scancode == KB_SCANCODE_PAGE_UP) {
						tty_id_t id = tty_get_active();
						if (id == MAX_TTYS - 1) {
							tty_set_active(0);
						} else {
							tty_set_active(id + 1);
						}
					} else if (kb.scancode == KB_SCANCODE_PAGE_DOWN) {
						tty_id_t id = tty_get_active();
						if (id == 0) {
							tty_set_active(MAX_TTYS - 1);
						} else {
							tty_set_active(id - 1);
						}
					}
				}
			} break;
			}
		}
	}
}
