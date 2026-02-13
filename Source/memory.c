#include "memory.h"

#define NUM_BLOCKS_PER_ENTRY (sizeof(uint32_t) * 8)

// Set by the linker.ld script
extern uint8_t kernel_start;
extern uint8_t kernel_end;

uintptr_t get_kernel_start_phys_addr(void) {
	return (uintptr_t)&kernel_start;
}

uintptr_t get_kernel_end_phys_addr(void) {
	return (uintptr_t)&kernel_end;
}

static uint32_t g_system_memory;
static uint32_t g_num_used_physical_blocks;
static uint32_t g_num_physical_blocks;
static uint32_t *g_physical_memory_map; // Bit array, maps 4 GiB of memory
static uint32_t g_physical_memory_map_num_elements;
static void *g_kernel_brk;

static bool g_paging_enabled;
static mem_page_dir_table_t *g_current_page_dir_table;

uint32_t mem_get_used_physical_blocks() {
    return g_num_used_physical_blocks;
}

uint32_t mem_get_total_physical_blocks() {
    return g_num_physical_blocks;
}

uint32_t mem_get_remaining_physical_memory() {
    return g_system_memory - g_num_used_physical_blocks * MEM_PAGE_SIZE;
}

uint32_t mem_get_total_physical_memory() {
    return g_system_memory;
}

static
uint32_t get_physical_block_index_of_addr(uint32_t addr) {
    return addr / MEM_PAGE_SIZE;
}

static
uint32_t get_physical_block_addr(uint32_t block_index) {
    return block_index * MEM_PAGE_SIZE;
}

static
bool is_physical_block_free(uint32_t block_index) {
    k_assert(block_index < g_num_physical_blocks, "Invalid block index");

    uint32_t entry_index = block_index / NUM_BLOCKS_PER_ENTRY;
    uint32_t bit_index = block_index % NUM_BLOCKS_PER_ENTRY;

    return (g_physical_memory_map[entry_index] & (1 << bit_index)) == 0;
}

static
void mark_physical_block_as_used(uint32_t block_index) {
    k_assert(block_index < g_num_physical_blocks, "Invalid block index");

    if (is_physical_block_free(block_index)) {
        g_num_used_physical_blocks += 1;
    }

    uint32_t entry_index = block_index / NUM_BLOCKS_PER_ENTRY;
    uint32_t bit_index = block_index % NUM_BLOCKS_PER_ENTRY;

    g_physical_memory_map[entry_index] |= (1 << bit_index);
}

static
void mark_physical_block_as_free(uint32_t block_index) {
    k_assert(block_index != 0, "Cannot mark physical block 0 as free");
    k_assert(block_index < g_num_physical_blocks, "Invalid block index");

    if (!is_physical_block_free(block_index)) {
        g_num_used_physical_blocks -= 1;
    }

    uint32_t entry_index = block_index / NUM_BLOCKS_PER_ENTRY;
    uint32_t bit_index = block_index % NUM_BLOCKS_PER_ENTRY;

    g_physical_memory_map[entry_index] &= ~(1 << bit_index);
}

static
void mark_physical_region_as_used(uint32_t start_addr, uint32_t end_addr) {
    k_assert(end_addr > start_addr, "Invalid parameters");

    uint32_t start_block_index = get_physical_block_index_of_addr(start_addr);
    uint32_t end_block_index = get_physical_block_index_of_addr(end_addr);

    k_assert(end_block_index >= start_block_index, "Error when calculating end block index (probable integer overflow)");

    for (uint32_t i = start_block_index; i <= end_block_index; i += 1) {
        mark_physical_block_as_used(i);
    }
}

static
void mark_physical_region_as_free(uint32_t start_addr, uint32_t end_addr) {
    k_assert(end_addr > start_addr, "Invalid parameters");

    uint32_t start_block_index = get_physical_block_index_of_addr(start_addr);
    uint32_t end_block_index = get_physical_block_index_of_addr(end_addr);

    k_assert(end_block_index >= start_block_index, "Error when calculating end block index (probable integer overflow)");

    for (uint32_t i = start_block_index; i <= end_block_index; i += 1) {
        mark_physical_block_as_free(i);
    }
}

uint32_t get_first_free_physical_block_from(uint32_t start_index)
{
	for (uint32_t i = start_index; i < g_num_physical_blocks; i += 1) {
		if (is_physical_block_free(i)) {
			return i;
		}
	}

	return 0; // Block 0 is reserved, so it's fine that we return it when no block is free
}

static
uint32_t get_first_free_physical_blocks(int32_t num_blocks) {
	if (num_blocks <= 0) {
		return 0;
	}

	if (g_num_used_physical_blocks >= g_num_physical_blocks) {
		return 0;
	}

	for (uint32_t i = 0; i < g_num_physical_blocks; i += 1) {
		bool all_free = true;
		for (uint32_t j = 0; j < (uint32_t)num_blocks; j += 1) {
			if (!is_physical_block_free(i + j)) {
				all_free = false;
				break;
			}
		}

		if (all_free) {
			return i;
		}
	}

	return 0; // Block 0 is reserved, so it's fine that we return it when no block is free
}

static void init_virtual_memory();

void mem_init_with_multiboot_info(const multiboot_info_t *info) {
	k_assert((info->flags & MULTIBOOT_INFO_MEM_MAP) != 0, "Memory map is not present in multiboot info");

	g_system_memory = 0;

	// Iterate over the multiboot memory map to calculate the available system memory
	uint32_t offset = 0;
	while (offset < info->mmap_length) {
		const multiboot_memory_map_t *map = (const multiboot_memory_map_t *)(info->mmap_addr + offset);
		offset += map->size + sizeof(map->size);

		k_assert(map->addr_high == 0, "Non 32-bit block address");
		k_assert(map->len_high == 0, "Non 32-bit block length");

		if (map->type == MULTIBOOT_MEMORY_AVAILABLE && map->addr_low + map->len_low > g_system_memory) {
			g_system_memory = map->addr_low + map->len_low;
		}
	}

	g_num_physical_blocks = g_system_memory / MEM_PAGE_SIZE;
	g_physical_memory_map_num_elements = g_num_physical_blocks / NUM_BLOCKS_PER_ENTRY + ((g_num_physical_blocks % NUM_BLOCKS_PER_ENTRY) != 0);
	uint32_t memory_map_size = g_physical_memory_map_num_elements * sizeof(uint32_t);

	g_physical_memory_map = (uint32_t *)k_align_forward(get_kernel_end_phys_addr(), 16);

	k_printf("Kernel loaded at %p - %p\n", get_kernel_start_phys_addr(), get_kernel_end_phys_addr());
	k_printf("System memory: %n, %u blocks\n", g_system_memory, g_num_physical_blocks);
	k_printf("Memory map addr: %p, %u entries\n", g_physical_memory_map, g_physical_memory_map_num_elements);

	// Iterate over the multiboot memory map to make sure we put the physical memory map in a valid place
	offset = 0;
	while (offset < info->mmap_length) {
		const multiboot_memory_map_t *map = (const multiboot_memory_map_t *)(info->mmap_addr + offset);
		offset += map->size + sizeof(map->size);

		k_assert(map->addr_high == 0, "Non 32-bit block address");
		k_assert(map->len_high == 0, "Non 32-bit block length");

		if (map->addr_low <= (uint32_t)g_physical_memory_map && map->addr_low + map->len_low > (uint32_t)g_physical_memory_map) {
			k_assert(map->type == MULTIBOOT_MEMORY_AVAILABLE, "Placed memory map in an unavailable memory region");
			k_assert((uint32_t)g_physical_memory_map + memory_map_size <= map->addr_low + map->len_low, "Placed memory map in a memory region that is not big enough");
			break;
		}
	}

	// Mark all blocks as free
	g_num_used_physical_blocks = 0;
	k_memset(g_physical_memory_map, 0, memory_map_size);

	// Iterate over the multiboot memory map to mark blocks as used in our memory map array based on their type
	offset = 0;
	while (offset < info->mmap_length) {
		const multiboot_memory_map_t *map = (const multiboot_memory_map_t *)(info->mmap_addr + offset);
		offset += map->size + sizeof(map->size);

		k_assert(map->addr_high == 0, "Non 32-bit block address");
		k_assert(map->len_high == 0, "Non 32-bit block length");
		k_assert(map->len_low > 0, "Invalid memory map length");

		if (map->addr_low >= g_system_memory) {
			break;
		}

		if (map->type != MULTIBOOT_MEMORY_AVAILABLE) {
			mark_physical_region_as_used(map->addr_low, map->addr_low + (map->len_low - 1));
		}
	}

	// Mark memory for our kernel and our memory system as used
	uint32_t kernel_reserved_end_addr = (uint32_t)g_physical_memory_map + memory_map_size;
	mark_physical_region_as_used(0, kernel_reserved_end_addr - 1);

	mem_print_physical_memory_map();

	init_virtual_memory();
	k_printf("Initialized memory\n");
}

void mem_print_physical_memory_map(void) {
	if (g_num_physical_blocks == 0) {
		return;
	}

	k_printf("Memory map (%d blocks, %d used blocks):\n", g_num_physical_blocks, g_num_used_physical_blocks);

	uint32_t prev_block = 0;
	bool prev_block_was_free = is_physical_block_free(prev_block);
	for (uint32_t i = 1; i < g_num_physical_blocks; i += 1) {
		bool is_free = is_physical_block_free(i);
		if (is_free != prev_block_was_free || i == g_num_physical_blocks - 1) {
			uint32_t addr1 = get_physical_block_addr(prev_block);
			uint32_t addr2 = get_physical_block_addr(i) - 1;

			uint32_t block = i == g_num_physical_blocks - 1 ? i : i  - 1;
			k_printf("  %s: %p-%p (%n) blocks %u-%u\n", prev_block_was_free ? "Free" : "Used", addr1, addr2, addr2 - addr1 + 1, prev_block, block);
			prev_block = i;
			prev_block_was_free = is_free;
		}
	}

	k_printf("End of memory map\n");
}

mem_page_table_entry_t *mem_get_page_table_entry(mem_page_table_t *table, virt_addr_t addr) {
	if (table) {
		return &table->entries[addr.page_index];
	}
	return NULL;
}

mem_page_dir_entry_t *mem_get_page_dir_entry(mem_page_dir_table_t *table, virt_addr_t addr) {
	if (table) {
		return &table->entries[addr.directory_index];
	}
	return NULL;
}

uint32_t mem_alloc_physical_blocks(int32_t num_blocks) {
	uint32_t block_index = get_first_free_physical_blocks(num_blocks);
	if (!block_index) {
		return 0;
	}

	for (int32_t i = 0; i < num_blocks; i += 1) {
		mark_physical_block_as_used(block_index + i);
	}

	uint32_t ptr = get_physical_block_addr(block_index);
	k_printf("Allocated %d physical block(s): %p\n", num_blocks, ptr);

	return ptr;
}

void mem_free_physical_blocks(uint32_t block, int32_t num_blocks) {
	if (!block || num_blocks <= 0) {
		return;
	}

	uint32_t block_index = get_physical_block_index_of_addr(block);
	k_printf("Freeing %d block(s) at %p (index %u)\n", num_blocks, block, block_index);
	k_assert(get_physical_block_addr(block_index) == block, "Block does not point to the start of a physical block");

	// @Todo: ensure we cannot mark reserved blocks as free
	for (int32_t i = 0; i < num_blocks; i += 1) {
		k_assert(!is_physical_block_free(block_index), "Double free");
		mark_physical_block_as_free(block_index + i);
	}
}

uint32_t mem_alloc_physical_memory(int32_t size) {
	int32_t num_blocks = size / MEM_PAGE_SIZE + ((size % MEM_PAGE_SIZE) != 0);

	return mem_alloc_physical_blocks(num_blocks);
}

void mem_free_physical_memory(uint32_t ptr, int32_t size) {
	int32_t num_blocks = size / MEM_PAGE_SIZE + ((size % MEM_PAGE_SIZE) != 0);

	mem_free_physical_blocks(ptr, num_blocks);
}

// CR0 register:
// 0 	PE 	Protected Mode Enable 	If 1, system is in protected mode, else, system is in real mode
// 1 	MP 	Monitor co-processor 	Controls interaction of WAIT/FWAIT instructions with TS flag in CR0
// 2 	EM 	Emulation				If set, no x87 floating-point unit present, if clear, x87 FPU present
// 3 	TS 	Task switched 			Allows saving x87 task context upon a task switch only after x87 instruction used
// 4 	ET 	Extension type 			On the 386, it allowed to specify whether the external math coprocessor was an 80287 or 80387
// 5 	NE 	Numeric error 			On the 486 and later, enable internal x87 floating point error reporting when set, else enable PC-style error reporting from the internal floating-point unit using external logic[13]
// 16 	WP 	Write protect 			When set, the CPU cannot write to read-only pages when privilege level is 0
// 18 	AM 	Alignment mask 			Alignment check enabled if AM set, AC flag (in EFLAGS register) set, and privilege level is 3
// 29 	NW 	Not-write through 		Globally enables/disable write-through caching
// 30 	CD 	Cache disable 			Globally enables/disable the memory cache
// 31 	PG 	Paging 					If 1, enable paging and use the § CR3 register, else disable paging.
void mem_set_paging_enabled(bool enabled) {
	if (g_paging_enabled == enabled) {
		return;
	}

	if (enabled) {
		k_printf("Enabling paging...\n");
	} else {
		k_printf("Disabling paging...\n");
	}

	uint32_t cr0;
	asm volatile("mov %%cr0, %0" : "=r"(cr0));

	if (enabled) {
		cr0 |= (uint32_t)(1 << 31);
	} else {
		cr0 &= ~(uint32_t)(1 << 31);
	}

	asm volatile("mov %0, %%cr0" :: "r"(cr0));

	if (enabled) {
		k_printf("Enabled paging\n");
	} else {
		k_printf("Disabled paging\n");
	}

	g_paging_enabled = enabled;
}

void mem_flush_tlb() {
	k_printf("TLB flush\n");

	asm volatile(
		"mov %%cr0, %%eax\n"
		"mov %%eax, %%cr0\n"
		: : : "eax", "memory"
	);
}

void mem_flush_page(virt_addr_t addr) {
	k_printf("Page flush: %p\n", addr);

	asm volatile(
		"cli\n"
		"invlpg (%0)\n"
		"sti\n" : : "r"(addr)
	);
}

bool mem_change_page_dir_table(mem_page_dir_table_t *table) {
	if (!table) {
		return false;
	}

	k_assert((((uint32_t)table) % MEM_PAGE_SIZE) == 0, "Table pointer is not page aligned");

	k_printf("Changing page directory table to %p\n", table);
	g_current_page_dir_table = table;
	asm volatile("mov %0, %%cr3" :: "r"(table));

	return true;
}

mem_page_dir_table_t *mem_get_current_page_dir_table() {
	return g_current_page_dir_table;
}

bool mem_map_page(uint32_t physical_addr, virt_addr_t virt_addr) {
	k_printf("Mapping %p to %p\n", physical_addr, virt_addr);

	bool paging_was_enabled = g_paging_enabled;
	bool should_reset_paging = false;

	mem_page_dir_table_t *dir_table = mem_get_current_page_dir_table();
	mem_page_dir_entry_t *dir = mem_get_page_dir_entry(dir_table, virt_addr);
	if (!dir->is_present_in_physical_memory) {
		// Need to allocate a new page, so make sure paging is disabled and we can
		// access physical addresses as is
		mem_set_paging_enabled(false);
		should_reset_paging = true;

		mem_page_table_t *table = (mem_page_table_t *)mem_alloc_physical_blocks(1);
		if (!table) {
			mem_set_paging_enabled(paging_was_enabled);
			return false;
		}

		k_memset(table, 0, sizeof(mem_page_table_t));

		dir->page_table_physical_addr_4KiB = (uint32_t)table / MEM_PAGE_SIZE;
		dir->is_writable = 1;
		dir->is_present_in_physical_memory = 1;
	}

	mem_page_table_t *table = (mem_page_table_t *)(dir->page_table_physical_addr_4KiB * MEM_PAGE_SIZE);
	mem_page_table_entry_t *entry = mem_get_page_table_entry(table, virt_addr);

	entry->is_present_in_physical_memory = 1;
	entry->physical_addr_4KiB = physical_addr / MEM_PAGE_SIZE;

	if (should_reset_paging) {
		mem_set_paging_enabled(paging_was_enabled);
	}

	return true;
}

static
void init_virtual_memory() {
	mem_page_table_t *identity_table = (mem_page_table_t *)mem_alloc_physical_memory(sizeof(mem_page_table_t));
	k_assert(identity_table != NULL, "Physical memory allocation failure");

	mem_page_table_t *kernel_table = (mem_page_table_t *)mem_alloc_physical_memory(sizeof(mem_page_table_t));
	k_assert(kernel_table != NULL, "Physical memory allocation failure");

	k_memset(identity_table, 0, sizeof(*identity_table));
	k_memset(kernel_table, 0, sizeof(*kernel_table));

	// Identity map the first 4 MiB of virtual address space (virt addr == phys addr)
	uint32_t addr = 0;
	for (int32_t i = 0; i < MEM_NUM_PAGE_TABLE_ENTRIES; i += 1) {
		mem_page_table_entry_t *entry = mem_get_page_table_entry(identity_table, make_virt_addr(addr));
		k_assert(entry != NULL, "");

		entry->is_present_in_physical_memory = 1;
		entry->physical_addr_4KiB = addr / MEM_PAGE_SIZE;

		addr += MEM_PAGE_SIZE;
	}

	// Map the kernel (first 4 MiB) to virtual address space 3 GiB
	uint32_t phys_addr = 0;
	uint32_t virt_addr = KERNEL_VIRT_START;
	for (int32_t i = 0; i < MEM_NUM_PAGE_TABLE_ENTRIES; i += 1) {
		mem_page_table_entry_t *entry = mem_get_page_table_entry(kernel_table, make_virt_addr(virt_addr));
		k_assert(entry != NULL, "");

		entry->is_present_in_physical_memory = 1;
		entry->physical_addr_4KiB = phys_addr / MEM_PAGE_SIZE;

		virt_addr += MEM_PAGE_SIZE;
		phys_addr += MEM_PAGE_SIZE;
	}

	// Create a directory table
	mem_page_dir_table_t *dir_table = (mem_page_dir_table_t *)mem_alloc_physical_memory(sizeof(mem_page_dir_table_t));
	k_assert(dir_table != NULL, "Memory allocation failure");

	k_memset(dir_table, 0, sizeof(*dir_table));

	mem_page_dir_entry_t *identity_dir = mem_get_page_dir_entry(dir_table, make_virt_addr(0));
	identity_dir->is_present_in_physical_memory = 1;
	identity_dir->is_writable = 1;
	identity_dir->page_table_physical_addr_4KiB = (uint32_t)identity_table / MEM_PAGE_SIZE;

	mem_page_dir_entry_t *kernel_dir = mem_get_page_dir_entry(dir_table, make_virt_addr(KERNEL_VIRT_START));
	kernel_dir->is_present_in_physical_memory = 1;
	kernel_dir->is_writable = 1;
	kernel_dir->page_table_physical_addr_4KiB = (uint32_t)kernel_table / MEM_PAGE_SIZE;

	k_assert(identity_dir != kernel_dir, "");

	mem_change_page_dir_table(dir_table);
	mem_set_paging_enabled(true);

	g_kernel_brk = (void *)(KERNEL_VIRT_START + 0x400000);

	{
		uint32_t value = 0xbadcafe;
		uint32_t *addr1 = &value;
		uint32_t *addr2 = (uint32_t *)((uint32_t)addr1 + KERNEL_VIRT_START);

		k_assert(*addr1 == 0xbadcafe, "Memory map test failed");
		k_assert(*addr2 == 0xbadcafe, "Memory map test failed");
	}
}

void *kbrk(k_size_t increment) {
	k_assert(g_kernel_brk != NULL, "Kernel brk not initialized");

	if (increment <= 0) {
		return g_kernel_brk;
	}

	k_printf("Incrementing kernel brk by %d bytes\n", increment);

	uint32_t num_pages_increment = (uint32_t)increment / MEM_PAGE_SIZE + (((uint32_t)increment % MEM_PAGE_SIZE) != 0);
	uint32_t phys_brk = (uint32_t)g_kernel_brk - KERNEL_VIRT_START;
	uint32_t phys_brk_page = phys_brk / MEM_PAGE_SIZE;

	if (phys_brk_page + num_pages_increment > g_num_physical_blocks) {
		k_printf("kbrk: exceeding total amount of physical memory (requested %d bytes)\n", increment);
		return NULL;
	}

	for (uint32_t i = phys_brk_page; i < phys_brk_page + num_pages_increment; i += 1) {
		if (!is_physical_block_free(i)) {
			k_printf("kbrk: could not find contiguous blocks of memory to satisfy request (requested %d bytes)\n", increment);
			return NULL;
		}
	}

	for (uint32_t i = phys_brk_page; i < phys_brk_page + num_pages_increment; i += 1) {
		mark_physical_block_as_used(i);
		// Identity map
		mem_map_page(phys_brk, make_virt_addr(phys_brk));
		// Kernel map
		mem_map_page(phys_brk, make_virt_addr((uint32_t)g_kernel_brk));

		phys_brk += MEM_PAGE_SIZE;
		g_kernel_brk += MEM_PAGE_SIZE;
	}

	return g_kernel_brk;
}
