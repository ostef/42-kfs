#include "libkernel.h"
#include "tty.h"
#include "interrupts.h"

void kernel_main(void) {
	tty_initialize();
	interrupts_initialize();

	k_printf("Hello Kernel!\nkernel_main=%p\n", kernel_main);
	tty_putstr("This is \x1b[31mred\x1b[0m text.\n");
	tty_putstr("This is \x1b[41mred\x1b[0m text.\n");
}
