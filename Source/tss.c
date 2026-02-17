# include "tss.h"
# include "gdt.h"

struct tss_entry_s g_tss_entry = {0};

void init_tss(void)
{

	g_tss_entry.ss = KERNEL_STACK_SEGMENT;
	g_tss_entry.esp = stack_top;
	g_tss_entry.iomap_base = sizeof(struct tss_entry_s); // No I/O bitmap
	k_printf("aaa\n");


	// update gdt entry
	k_printf("Setting TSS descriptor with base %p and limit %p\n", (uint32_t)&g_tss_entry, sizeof(g_tss_entry));
	set_tss_descriptor((uint32_t)&g_tss_entry, sizeof(g_tss_entry));

	k_printf("TSS entry at %p: ss0=%x, esp0=%x\n", &g_tss_entry, g_tss_entry.ss0, g_tss_entry.esp0);
	// k_printf("TSS descriptor at %p: %p %p\n", g_gdt_entries + 7 * 8, *(uint32_t *)(g_gdt_entries + 7 * 8), *(uint32_t *)(g_gdt_entries + 7 * 8 + 4));
	k_printf("TSS segment selector: %x\n", TSS_SEGMENT);
	k_printf("STACK segment selector: %x\n", KERNEL_STACK_SEGMENT);
	// load tss
    asm volatile ("ltr %0" : : "r" (TSS_SEGMENT));
}