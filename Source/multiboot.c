#include "libkernel.h"
#include "multiboot.h"

static
void helper_print_field_u8(const char *field_name, uint8_t value) {
    k_printf("  %s: 0x%.2x\n", field_name, value);
}

static
void helper_print_field_u16(const char *field_name, uint16_t value) {
    k_printf("  %s: 0x%.4x\n", field_name, value);
}

static
void helper_print_field_u32(const char *field_name, uint32_t value) {
    k_printf("  %s: 0x%.8x\n", field_name, value);
}

static
void helper_print_field_u64(const char *field_name, uint32_t low, uint32_t high) {
    k_printf("  %s: 0x%x%.8x\n", field_name, high, low);
}

static
void helper_print_field_string(const char *field_name, const char *str) {
    k_printf("  %s: '%s'\n", field_name, str);
}

#define print_field_u8(field) helper_print_field_u8(#field, info->field)
#define print_field_u16(field) helper_print_field_u16(#field, info->field)
#define print_field_u32(field) helper_print_field_u32(#field, info->field)
#define print_field_u64(field) helper_print_field_u64(#field, info->field ##_low, info->field ##_high)
#define print_field_string(field) helper_print_field_string(#field, (const char *)info->field)

void print_multiboot_info(const multiboot_info_t *info) {
	k_printf("Multiboot info:\n");
	k_printf("  flags: 0b%b", info->flags);
	if (info->flags & MULTIBOOT_INFO_MEMORY) {
        k_printf(" MEMORY");
    }
    if (info->flags & MULTIBOOT_INFO_BOOTDEV) {
        k_printf(" BOOTDEV");
    }
    if (info->flags & MULTIBOOT_INFO_CMDLINE) {
        k_printf(" CMDLINE");
    }
    if (info->flags & MULTIBOOT_INFO_MODS) {
        k_printf(" MODS");
    }
    if (info->flags & MULTIBOOT_INFO_AOUT_SYMS) {
        k_printf(" AOUT_SYMS");
    }
    if (info->flags & MULTIBOOT_INFO_ELF_SHDR) {
        k_printf(" ELF_SHDR");
    }
    if (info->flags & MULTIBOOT_INFO_MEM_MAP) {
        k_printf(" MEM_MAP");
    }
    if (info->flags & MULTIBOOT_INFO_DRIVE_INFO) {
        k_printf(" DRIVE_INFO");
    }
    if (info->flags & MULTIBOOT_INFO_CONFIG_TABLE) {
        k_printf(" CONFIG_TABLE");
    }
    if (info->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
        k_printf(" BOOT_LOADER_NAME");
    }
    if (info->flags & MULTIBOOT_INFO_APM_TABLE) {
        k_printf(" APM_TABLE");
    }
    if (info->flags & MULTIBOOT_INFO_VBE_INFO) {
        k_printf(" VBE_INFO");
    }
    if (info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        k_printf(" FRAMEBUFFER_INFO");
    }
    k_printf("\n");

	if (info->flags & MULTIBOOT_INFO_MEMORY) {
        print_field_u32(mem_lower);
        print_field_u32(mem_upper);
    }

    if (info->flags & MULTIBOOT_INFO_BOOTDEV) {
        print_field_u32(boot_device);
    }

    if (info->flags & MULTIBOOT_INFO_CMDLINE) {
        print_field_string(cmdline);
    }

    if (info->flags & MULTIBOOT_INFO_MODS) {
        print_field_u32(mods_count);
        print_field_u32(mods_addr);
    }

    if (info->flags & MULTIBOOT_INFO_AOUT_SYMS) {
        k_printf("  MULTIBOOT_INFO_AOUT_SYMS:\n");
        print_field_u32(u.aout_sym.tabsize);
        print_field_u32(u.aout_sym.strsize);
        print_field_u32(u.aout_sym.addr);
        print_field_u32(u.aout_sym.reserved);
    }

    if (info->flags & MULTIBOOT_INFO_ELF_SHDR) {
        k_printf("  MULTIBOOT_INFO_ELF_SHDR:\n");
        print_field_u32(u.elf_sec.num);
        print_field_u32(u.elf_sec.size);
        print_field_u32(u.elf_sec.addr);
        print_field_u32(u.elf_sec.shndx);
    }

    if (info->flags & MULTIBOOT_INFO_MEM_MAP) {
        k_printf("  mmap:\n");
        uint32_t offset = 0;
        uint32_t i = 0;
        while (offset < info->mmap_length) {
            multiboot_memory_map_t map = *(multiboot_memory_map_t *)(info->mmap_addr + offset);
            offset += map.size + sizeof(map.size);

            k_printf("   [%u], size: %u, addr: 0x%x%.8x, len: 0x%x%.8x, type: %u", i, map.size, map.addr_high, map.addr_low, map.len_high, map.len_low, map.type);

            switch (map.type) {
            case MULTIBOOT_MEMORY_AVAILABLE: {
                k_printf(" AVAILABLE");
            } break;
            case MULTIBOOT_MEMORY_RESERVED: {
                k_printf(" RESERVED");
            } break;
            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE: {
                k_printf(" ACPI_RECLAIMABLE");
            } break;
            case MULTIBOOT_MEMORY_NVS: {
                k_printf(" NVS");
            } break;
            case MULTIBOOT_MEMORY_BADRAM: {
                k_printf(" BADRAM");
            } break;
            }

            k_printf("\n");

            i += 1;
        }
    }

    if (info->flags & MULTIBOOT_INFO_DRIVE_INFO) {
        print_field_u32(drives_length);
        print_field_u32(drives_addr);
    }
    if (info->flags & MULTIBOOT_INFO_CONFIG_TABLE) {
        print_field_u32(config_table);
    }
    if (info->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
        print_field_string(boot_loader_name);
    }
    if (info->flags & MULTIBOOT_INFO_APM_TABLE) {
        print_field_u32(apm_table);
    }

    if (info->flags & MULTIBOOT_INFO_VBE_INFO) {
        print_field_u32(vbe_control_info);
        print_field_u32(vbe_mode_info);
        print_field_u16(vbe_mode);
        print_field_u16(vbe_interface_seg);
        print_field_u16(vbe_interface_off);
        print_field_u16(vbe_interface_len);
    }

    if (info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        print_field_u32(framebuffer_addr_low);
        print_field_u32(framebuffer_addr_high);
        print_field_u32(framebuffer_pitch);
        print_field_u32(framebuffer_width);
        print_field_u32(framebuffer_height);
        print_field_u8(framebuffer_bpp);

        k_printf("  framebuffer_type: %i", info->framebuffer_type);

        switch (info->framebuffer_type) {
        case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED: {
            k_printf(" INDEXED\n");
            print_field_u32(framebuffer_palette_addr);
            print_field_u32(framebuffer_palette_num_colors);
        } break;
        case MULTIBOOT_FRAMEBUFFER_TYPE_RGB: {
            k_printf(" RGB\n");
            print_field_u32(framebuffer_red_field_position);
            print_field_u32(framebuffer_red_mask_size);
            print_field_u32(framebuffer_green_field_position);
            print_field_u32(framebuffer_green_mask_size);
            print_field_u32(framebuffer_blue_field_position);
            print_field_u32(framebuffer_blue_mask_size);
        } break;
        case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT: {
            k_printf(" EGA_TEXT\n");
        } break;
        }
    }
}
