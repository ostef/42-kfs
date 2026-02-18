# include "tss.h"
# include "gdt.h"

struct tss_entry_s g_tss_entry = {0};

void init_tss(void)
{

	g_tss_entry.ss = KERNEL_STACK_SEGMENT;
	g_tss_entry.esp = (uint32_t)&stack_top;
	g_tss_entry.iomap_base = sizeof(struct tss_entry_s); // No I/O bitmap


	// update gdt entry
	set_tss_descriptor((uint32_t)&g_tss_entry, sizeof(g_tss_entry));

	// load tss
    asm volatile ("ltr %0" : : "r" (TSS_SEGMENT));
}