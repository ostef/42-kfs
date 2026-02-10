#include "libkernel.h"

uint32_t k_get_esp(void)
{
    uint32_t esp;
	
    asm volatile ("movl %%esp, %0" : "=r"(esp));
    return esp;
}