# include "libkernel.h"
# include "gdt.h"

uint8_t *g_gdt_entries = (uint8_t *)0x800;
struct GDTR g_gdtr;

void encode_gdt_entry(uint8_t *target, struct gdt_entry_s source)
{
	target[0] = source.limit_low & 0xFF;
	target[1] = (source.limit_low >> 8) & 0xFF;
	target[2] = source.base_low & 0xFF;
	target[3] = (source.base_low >> 8) & 0xFF;
	target[4] = (source.base_low >> 16) & 0xFF;
	target[5] = (source.accessed) |
				(source.read_write << 1) |
				(source.conforming_expand_down << 2) |
				(source.code << 3) |
				(source.code_data_segment << 4) |
				(source.DPL << 5) |
				(source.present << 7);
	target[6] = (source.limit_high & 0x0F) |
				(source.available << 4) |
				(source.long_mode << 5) |
				(source.big << 6) |
				(source.gran << 7);
	target[7] = source.base_high & 0xFF;
}

// void initialize_gdt() {
// 	struct gdt_entry_s null_descriptor = {0};
// 	struct gdt_entry_s code_descriptor = {
// 	.limit_low = 0xFFFF,
// 	.base_low = 0x0000,

// 	.present = 1,
// 	.DPL = 0,
// 	.code_data_segment = 1,
// 	.code = 1,
// 	.conforming_expand_down = 0,
// 	.read_write = 1,
// 	.accessed = 0,

// 	.gran = 1,
// 	.big = 1,
// 	.long_mode = 0,
// 	.available = 0,
// 	.limit_high = 0xF,

// 	.base_high = 0x00,
// 		// access_byte = 0b10011010,
// 		// flags = 0b1100,
// 	};
// 	struct gdt_entry_s data_descriptor = {
// 		.limit_low = 0xFFFF,
// 		.base_low = 0x0000,

// 		.present = 1,
// 		.DPL = 0,
// 		.code_data_segment = 1,
// 		.code = 0,
// 		.conforming_expand_down = 0,
// 		.read_write = 1,
// 		.accessed = 0,

// 		.gran = 1,
// 		.big = 1,
// 		.long_mode = 0,
// 		.available = 0,
// 		.limit_high = 0xF,

// 		.base_high = 0x00,
// 		// access_byte = 0b10010010,
// 		// flags = 0b1100,
// 	};
// 	struct gdt_entry_s kernel_stack_descriptor = {
// 		.limit_low = 0xFFFF,
// 		.base_low = 0x0000,

// 		.present = 1,
// 		.DPL = 0,
// 		.code_data_segment = 1,
// 		.code = 0,
// 		.conforming_expand_down = 1,
// 		.read_write = 1,
// 		.accessed = 0,

// 		.gran = 1,
// 		.big = 1,
// 		.long_mode = 0,
// 		.available = 0,
// 		.limit_high = 0xF,

// 		.base_high = 0x00,
// 		// access_byte = 0b10010110,
// 		// flags = 0b1100,
// 	};
// 	struct gdt_entry_s user_code_descriptor = {
// 		.limit_low = 0xFFFF,
// 		.base_low = 0x0000,

// 		.present = 1,
// 		.DPL = 0b11,
// 		.code_data_segment = 1,
// 		.code = 1,
// 		.conforming_expand_down = 0,
// 		.read_write = 1,
// 		.accessed = 0,

// 		.gran = 1,
// 		.big = 1,
// 		.long_mode = 0,
// 		.available = 0,
// 		.limit_high = 0xF,

// 		.base_high = 0x00,
// 		// access_byte = 0b11111010,
// 		// flags = 0b1100,
// 	};
// 	struct gdt_entry_s user_data_descriptor = {
// 		.limit_low = 0xFFFF,
// 		.base_low = 0x0000,

// 		.present = 1,
// 		.DPL = 0b11,
// 		.code_data_segment = 1,
// 		.code = 0,
// 		.conforming_expand_down = 0,
// 		.read_write = 1,
// 		.accessed = 0,

// 		.gran = 1,
// 		.big = 1,
// 		.long_mode = 0,
// 		.available = 0,
// 		.limit_high = 0xF,

// 		.base_high = 0x00,
// 		// access_byte = 0b11110010,
// 		// flags = 0b1100,
// 	};
// 	struct gdt_entry_s user_stack_descriptor = {
// 		.limit_low = 0xFFFF,
// 		.base_low = 0x0000,

// 		.present = 1,
// 		.DPL = 0b11,
// 		.code_data_segment = 1,
// 		.code = 0,
// 		.conforming_expand_down = 1,
// 		.read_write = 1,
// 		.accessed = 0,

// 		.gran = 1,
// 		.big = 1,
// 		.long_mode = 0,
// 		.available = 0,
// 		.limit_high = 0xF,

// 		.base_high = 0x00,
// 		// access_byte = 0b11110110,
// 		// flags = 0b1100,
// 	};
// 	struct gdt_entry_s tss_descriptor = {};

// 	encode_gdt_entry(&g_gdt_entries[0], null_descriptor);
// 	encode_gdt_entry(&g_gdt_entries[1], code_descriptor);
// 	encode_gdt_entry(&g_gdt_entries[2], data_descriptor);
// 	encode_gdt_entry(&g_gdt_entries[3], kernel_stack_descriptor);
// 	encode_gdt_entry(&g_gdt_entries[4], user_code_descriptor);
// 	encode_gdt_entry(&g_gdt_entries[5], user_data_descriptor);
// 	encode_gdt_entry(&g_gdt_entries[6], user_stack_descriptor);
// 	encode_gdt_entry(&g_gdt_entries[7], tss_descriptor);

// 	g_gdtr.limit = (sizeof(struct gdt_entry_s) * 8)- 1;
// 	g_gdtr.base = (uint32_t)gdt_entries;

// 	// load_gdtr(&gdtr);
// }


void set_tss_descriptor(uint32_t tss_base, uint32_t tss_size)
{
	uint32_t base = tss_base;
	uint32_t limit = tss_size - 1;
	struct gdt_entry_s tss_descriptor1 = {
		// Add a TSS descriptor to the GDT.
		.limit_low = limit,
		.base_low = base & 0xFFFFFF,

		.accessed = 1,					// With a system entry (`code_data_segment` = 0), 1 indicates TSS and 0 indicates LDT
		.read_write = 0,				// For a TSS, indicates busy (1) or not busy (0).
		.conforming_expand_down = 0,	// always 0 for TSS
		.code = 1,						// For a TSS, 1 indicates 32-bit (1) or 16-bit (0).
		.code_data_segment = 0,			// indicates TSS/LDT (see also `accessed`)
		.DPL = 0,						// ring 0, see the comments below
		.present = 1,

		.limit_high = (limit & (0xf << 16)) >> 16, // isolate top nibble
		.available = 0,					// 0 for a TSS
		.long_mode = 0,
		.big = 0,						// should leave zero according to manuals.
		.gran = 0,						// limit is in bytes, not pages

		.base_high = (base & (0xff << 24)) >> 24, //isolate top byte
	};

	encode_gdt_entry(g_gdt_entries + 7 * 8, tss_descriptor1);
}