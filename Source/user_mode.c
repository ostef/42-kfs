#include "libkernel.h"

void enter_user_mode(void)
{
	k_printf("Entered user mode successfully!\n");
	while (true) {
		asm volatile("hlt");
	}
}