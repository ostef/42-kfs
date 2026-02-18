#ifndef GDT_H
#define GDT_H

#include "libkernel.h"

extern const uint16_t KERNEL_STACK_SEGMENT;
extern const uint16_t TSS_SEGMENT;

struct gdt_entry_s
{
	uint32_t limit_low : 16;
	uint32_t base_low : 24;

	uint32_t present : 1;
	uint32_t DPL : 2;					 // privilege level
	uint32_t code_data_segment : 1;		 // should be 1 for everything but TSS and LDT
	uint32_t code : 1;					 // 1 for code, 0 for data
	uint32_t conforming_expand_down : 1; // conforming for code, expand down for data
	uint32_t read_write : 1;			 // readable for code, writable for data
	uint32_t accessed : 1;

	uint32_t gran : 1; // 1 to use 4k page addressing, 0 for byte addressing
	uint32_t big : 1;  // 32-bit opcodes for code, uint32_t stack for data
	uint32_t long_mode : 1;
	uint32_t available : 1; // only used in software; has no effect on hardware
	uint32_t limit_high : 4;

	uint32_t base_high : 8;
} __attribute__((packed));

struct GDTR
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

void set_tss_descriptor(uint32_t tss_base, uint32_t tss_size);

#endif // GDT_H