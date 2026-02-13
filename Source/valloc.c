#include "alloc.h"

static vmalloc_header_t *g_vmalloc_heap;
static vmalloc_header_t *g_vmalloc_last_alloc = NULL;

void *vmalloc(k_size_t size)
{
	uint32_t				nb_pages;
	uint32_t				virt_addr_first_page;

	size += sizeof(vmalloc_header_t);
	nb_pages = size / MEM_PAGE_SIZE + ((size % MEM_PAGE_SIZE) != 0);

	uint32_t i = 0;
	for (; i < nb_pages; i++) {
		uint32_t block_index = get_first_free_physical_block_from(i);
		if (block_index == 0) {
			// Handle allocation failure
			return NULL;
		}
		// allocate
		uint32_t virtual_addr;
		if (!g_vmalloc_last_alloc)
			 virtual_addr = 0xc0000400; // Start after 3g + 4mb for kernel
		else
			virtual_addr = (uint32_t)g_vmalloc_last_alloc + g_vmalloc_last_alloc->size + MEM_PAGE_SIZE * i;
		if (mem_map_page(block_index * MEM_PAGE_SIZE, make_virt_addr(virtual_addr)) == false) {
			// Handle allocation failure
			return NULL;
		}
		if (i == 0) {
			virt_addr_first_page = virtual_addr;
		}
	}

	if (nb_pages != i) {
		// Handle allocation failure
		return NULL;
	}
	vmalloc_header_t *header = (vmalloc_header_t *)virt_addr_first_page;
	header->size = size;
	header->next = NULL;

	if (!g_vmalloc_last_alloc) {
		g_vmalloc_heap = header;
		g_vmalloc_last_alloc = header;
	}
	else {
		g_vmalloc_last_alloc->next = header;
	}

	return (void *)(header + 1);
}


void vfree(void *ptr);

k_size_t vsize(void *ptr)
{
	vmalloc_header_t *header = (vmalloc_header_t *)ptr - 1;
	return header->size;
}

void *vbrk(k_size_t increment);