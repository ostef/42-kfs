#include "memory.h"

#define NUM_BLOCKS_PER_ENTRY 1024

static uint32_t g_kernel_size_in_memory;
static uint32_t g_system_memory;
static uint32_t g_num_used_physical_blocks;
static uint32_t g_num_physical_blocks;
static uint32_t *g_physical_memory_map; // Bit array, maps 4 GiB of memory
static uint32_t g_physical_memory_map_num_elements;

static mem_page_dir_table_t *g_current_page_dir_table;

// static void initialize_virtual_memory();

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

	g_kernel_size_in_memory = 1 * 1024 * 1024; // Assume at most 1 MiB, but we should get this value at boot time somehow
	g_physical_memory_map = (uint32_t *)(KERNEL_LOAD_ADDRESS + g_kernel_size_in_memory);

	k_printf("Kernel loaded at %p\n", (uintptr_t)KERNEL_LOAD_ADDRESS);
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

	// initialize_virtual_memory();
	k_printf("Initialized memory\n");
}

void mem_print_physical_memory_map(void) {
	if (g_num_physical_blocks == 0) {
		return;
	}

	k_printf("Memory map:\n");

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

// bool mem_alloc_page_physical(mem_page_table_entry_t *entry);
// void mem_free_page_physical(mem_page_table_entry_t *entry);
// mem_page_table_entry_t *mem_get_page_table_entry(mem_page_table_t *table, virt_addr_t addr);

// void mem_set_paging_enabled(bool enabled);
// void mem_flush_tlb();
// void mem_flush_page(virt_addr_t addr);
// bool mem_change_page_dir_table(mem_page_dir_table_t *table);
// mem_page_dir_table_t *mem_get_current_page_dir_table();

// bool mem_map_page(uint32_t physical_addr, virt_addr_t virt_addr);
