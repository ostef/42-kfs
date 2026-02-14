#include "alloc.h"

typedef struct vmalloc_addr_space_t {
	struct vmalloc_addr_space_t *prev;
	struct vmalloc_addr_space_t *next;
	uint32_t min;
	uint32_t max;
} vmalloc_addr_space_t;

typedef struct vmalloc_header_t {
	vmalloc_addr_space_t *addr_space;
	k_size_t size;
	uint32_t padding16[2];
} vmalloc_header_t;

k_static_assert(sizeof(vmalloc_header_t) % 16 == 0);

typedef struct vmalloc_heap_t {
	vmalloc_addr_space_t *free_addr_space_list;
	vmalloc_addr_space_t *occupied_addr_space_list;
} vmalloc_heap_t;

static
vmalloc_addr_space_t *split_addr_space(vmalloc_addr_space_t **first, vmalloc_addr_space_t *space, uint32_t size) {
	k_assert(size <= space->max - space->min, "Address space range is to small");

	if (space->max - space->min == size) {
		return space;
	}

	vmalloc_addr_space_t *new_space = kmalloc(sizeof(vmalloc_addr_space_t));
	if (!new_space) {
		return NULL;
	}

	k_memset(new_space, 0, sizeof(*new_space));

	new_space->prev = space->prev;
	if (new_space->prev) {
		new_space->prev->next = new_space;
	}

	new_space->next = space;
	space->prev = new_space;

	new_space->min = space->min;
	new_space->max = new_space->min + size - 1;
	space->min = new_space->max + 1;

	if (*first == space) {
		*first = new_space;
	}

	return new_space;
}

static vmalloc_heap_t g_vmalloc_heap;

void vmalloc_init(void) {
	vmalloc_addr_space_t *base_addr_space = kmalloc(sizeof(vmalloc_addr_space_t));
	k_memset(base_addr_space, 0, sizeof(*base_addr_space));
	base_addr_space->min = VMALLOC_VIRT_START;
	base_addr_space->max = VMALLOC_VIRT_END;

	g_vmalloc_heap.free_addr_space_list = base_addr_space;
}

static
void addr_space_push_front(vmalloc_addr_space_t **first, vmalloc_addr_space_t *space) {
	if (!first) {
		return;
	}

	space->next = *first;
	if (space->next) {
		space->next->prev = space;
	}

	space->prev = NULL;
	*first = space;
}

static
void addr_space_insert_after(vmalloc_addr_space_t **first, vmalloc_addr_space_t *after, vmalloc_addr_space_t *space) {
	if (!first) {
		return;
	}

	if (after) {
		if (after->next) {
			after->next->prev = space;
		}
		space->next = after->next;
		after->next = space;
		space->prev = after;
	} else {
		space->next = *first;
		if (space->next) {
			space->next->prev = space;
		}

		*first = space;
	}
}

static
void addr_space_pop(vmalloc_addr_space_t **first, vmalloc_addr_space_t *space) {
	if (!first || !space) {
		return;
	}

	if (*first == space) {
		*first = space->next;
	}

	if (space->prev) {
		space->prev->next = space->next;
	}

	if (space->next) {
		space->next->prev = space->prev;
	}

	space->prev = NULL;
	space->next = NULL;
}

static
vmalloc_addr_space_t *addr_space_pop_front(vmalloc_addr_space_t **first) {
	if (!first) {
		return NULL;
	}

	vmalloc_addr_space_t *space = *first;
	if (!space) {
		return NULL;
	}

	addr_space_pop(first, space);

	return space;
}

static
virt_addr_t alloc_virt_addr_space(vmalloc_heap_t *heap, k_size_t size) {
	vmalloc_addr_space_t *best_fit = NULL;
	k_size_t best_fit_size = 0;
	for (vmalloc_addr_space_t *space = heap->free_addr_space_list; space; space = space->next) {
		k_size_t space_size = space->max - space->min;
		if (space_size > size) {
			if (best_fit) {
				if (space_size < best_fit_size) {
					best_fit = space;
					best_fit_size = space_size;
				}
			} else {
				best_fit = space;
				best_fit_size = space_size;
			}
		} else if (space_size == size) { // Can't get any better than that!
			best_fit = space;
			best_fit_size = space_size;
			break;
		}
	}

	if (!best_fit) {
		return make_virt_addr(0);
	}

	vmalloc_addr_space_t *new_space = split_addr_space(&heap->free_addr_space_list, best_fit, size);
	addr_space_pop(&heap->free_addr_space_list, new_space);
	addr_space_push_front(&heap->occupied_addr_space_list, new_space);

	return make_virt_addr(new_space->min);
}

static
void free_virt_addr_space(vmalloc_heap_t *heap, vmalloc_addr_space_t *space) {
	addr_space_pop(&heap->occupied_addr_space_list, space);

	vmalloc_addr_space_t *insert_after = NULL;
	for (vmalloc_addr_space_t *s = heap->free_addr_space_list; s; s = s->next) {
		if (s->min > space->max) {
			insert_after = s->prev;
			break;
		} else if (!s->next) {
			insert_after = s;
		}
	}

	// Coalesce
	if (insert_after && insert_after->max == space->min - 1) {
		insert_after->max = space->max;
		kfree(space);

		while (insert_after->next && insert_after->max == insert_after->next->min - 1) {
			vmalloc_addr_space_t *next = insert_after->next;
			insert_after->max = next->max;
			if (next->next) {
				next->next->prev = insert_after;
			}
			insert_after->next = next->next;

			kfree(next);
		}
	} else {
		addr_space_insert_after(&heap->free_addr_space_list, insert_after, space);
	}
}

void *vmalloc(k_size_t size) {
	if (size <= 0) {
		return NULL;
	}

	k_size_t size_with_header = size + sizeof(vmalloc_header_t);
	size_with_header = k_align_forward(size_with_header, MEM_PAGE_SIZE);

	virt_addr_t virt_start = alloc_virt_addr_space(&g_vmalloc_heap, size_with_header);
	if (!*(uint32_t *)&virt_start) {
		return NULL;
	}

	virt_addr_t virt_addr = virt_start;

	vmalloc_addr_space_t *space = g_vmalloc_heap.occupied_addr_space_list;

	// Allocate blocks one by one (we don't need them to be contiguous)
	// If we cannot allocate physical blocks, try to find blocks by searching in
	// reverse and eating from memory usable by kbrk
	bool reverse_search = false;
	uint32_t num_pages = size_with_header / MEM_PAGE_SIZE;
	uint32_t start_search_index = get_physical_block_index_of_addr(KERNEL_VIRT_LINEAR_MAPPING_END - KERNEL_VIRT_START);
	uint32_t block_index = start_search_index;
	for (uint32_t i = 0; i < num_pages; i += 1) {
		uint32_t addr = mem_alloc_physical_blocks(1, block_index, reverse_search);
		if (!addr && block_index >= start_search_index) {
			reverse_search = true;
			block_index = start_search_index - 1;
			addr = mem_alloc_physical_blocks(1, block_index, reverse_search);
		}

		if (!addr) {
			return NULL;
		}

		if (reverse_search) {
			block_index = get_physical_block_index_of_addr(addr) - 1;
		} else {
			block_index = get_physical_block_index_of_addr(addr) + 1;
		}

		if (!mem_map_page(addr, virt_addr, default_page_table_alloc)) {
			return NULL;
		}

		virt_addr = make_virt_addr(*(uint32_t *)&virt_addr + MEM_PAGE_SIZE);
	}

	vmalloc_header_t *header = *(vmalloc_header_t **)&virt_start;
	k_memset(header, 0, sizeof(*header));

	header->addr_space = space;
	header->size = size_with_header - sizeof(vmalloc_header_t);

	return (void *)(header + 1);
}

void vfree(void *ptr) {
	if (!ptr) {
		return;
	}

	vmalloc_header_t *header = (vmalloc_header_t *)ptr - 1;
	k_assert(header->size > 0, "Invalid ptr");
	k_assert((header->size + sizeof(vmalloc_header_t)) % MEM_PAGE_SIZE == 0, "Invalid ptr");

	mem_page_dir_table_t *dir = mem_get_current_page_dir_table();
	uint32_t addr = (uint32_t)header;
	for (uint32_t i = 0; i < (uint32_t)header->size; i += MEM_PAGE_SIZE) {
		virt_addr_t virt_addr = make_virt_addr(addr + i);

		mem_page_dir_entry_t *dir_entry = mem_get_page_dir_entry(dir, virt_addr);
		k_assert(dir_entry->is_present_in_physical_memory, "");

		uint32_t page_table_phys_addr = dir_entry->page_table_physical_addr_4KiB * MEM_PAGE_SIZE;
		mem_page_table_t *page_table = (mem_page_table_t *)page_table_phys_addr;
		mem_page_table_entry_t *table_entry = mem_get_page_table_entry(page_table, virt_addr);
		k_assert(table_entry->is_present_in_physical_memory, "");

		uint32_t phys_addr = table_entry->physical_addr_4KiB * MEM_PAGE_SIZE;
		mem_free_physical_blocks(phys_addr, 1);

		mem_unmap_page(virt_addr);
	}

	free_virt_addr_space(&g_vmalloc_heap, header->addr_space);
}

k_size_t vsize(void *ptr) {
	if (!ptr) {
		return 0;
	}

	vmalloc_header_t *header = (vmalloc_header_t *)ptr - 1;

	return header->size;
}

void vmalloc_print_info(void) {
	k_printf("Vmalloc heap info:\n");

	k_printf("Free address space:\n");
	for (vmalloc_addr_space_t *free = g_vmalloc_heap.free_addr_space_list; free; free = free->next) {
		k_printf("  %p - %p\n", free->min, free->max);
	}

	uint32_t total_allocated_bytes = 0;
	k_printf("\nOccupied address space:\n");
	for (vmalloc_addr_space_t *occupied = g_vmalloc_heap.occupied_addr_space_list; occupied; occupied = occupied->next) {
		k_printf("  %p - %p\n", occupied->min, occupied->max);
		total_allocated_bytes += occupied->max - occupied->min;
	}

	k_printf("\nTotal allocated: %n\n", total_allocated_bytes);
}
