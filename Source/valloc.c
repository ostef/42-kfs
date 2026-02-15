#include "alloc.h"

typedef struct vmalloc_header_s {
	struct vmalloc_header_s *prev;
	struct vmalloc_header_s *next;
	k_size_t size;
	uint32_t unused; // Padding to make the header size a multiple of 16 bytes, which is important for alignment
} vmalloc_header_t;

static vmalloc_header_t *g_vmalloc_heap;
static vmalloc_header_t *g_vmalloc_last_alloc = NULL;

static void free_size(uint32_t addr, uint32_t size) {
	uint32_t	nb_pages;
	uint32_t	physical_addr;

	nb_pages = size / MEM_PAGE_SIZE + ((size % MEM_PAGE_SIZE) != 0);
	for (uint32_t i = 0; i < nb_pages; i++) {
		physical_addr = get_physical_address(make_virt_addr(addr + i * MEM_PAGE_SIZE));
		mem_free_physical_blocks(physical_addr, 1);
		mem_unmap_page(make_virt_addr(addr + i * MEM_PAGE_SIZE));
	}

}


void vfree(void *ptr)
{
	vmalloc_header_t *prev = NULL;
	vmalloc_header_t *next = NULL;
	if (!ptr) {
		return;
	}

	vmalloc_header_t *header = ((vmalloc_header_t *)ptr) - 1;
	prev = header->prev;
	next = header->next;

	if (g_vmalloc_last_alloc == header) {
		g_vmalloc_last_alloc = NULL;
	}
	if (prev) {
		prev->next = next;
	}
	if (next) {
		next->prev = prev;
	}
	free_size((uint32_t)header, header->size + sizeof(vmalloc_header_t));
	return;
}

void *vmalloc(k_size_t size)
{
	uint32_t	nb_pages;
	uint32_t	virt_addr_first_page;
	uint32_t	alloc_size;

	alloc_size = size + sizeof(vmalloc_header_t);
	nb_pages = alloc_size / MEM_PAGE_SIZE + ((alloc_size % MEM_PAGE_SIZE) != 0);

	uint32_t i = 0;
	virt_addr_first_page = virt_addr_to_uint32(find_first_unmapped_virtual_address_pages_from(make_virt_addr(0), nb_pages));
	k_printf("vmalloc: Allocating %d bytes (%d pages) at virtual address %p\n", size, nb_pages, virt_addr_first_page);
	for (; i < nb_pages; i++) {
		uint32_t block_index = get_first_free_physical_block_from(i);
		if (block_index == 0) {
			if (i != 0)
				free_size(virt_addr_first_page, (i - 1) * MEM_PAGE_SIZE);
			return NULL;
		}
		uint32_t virtual_addr = virt_addr_first_page + i * MEM_PAGE_SIZE;
		mark_physical_block_as_used(block_index);
		if (mem_map_page(block_index * MEM_PAGE_SIZE, make_virt_addr(virtual_addr)) == false) {
			if( i != 0)
				free_size(virt_addr_first_page, (i - 1) * MEM_PAGE_SIZE);
			return NULL;
		}
		if (i == 0) {
			virt_addr_first_page = virtual_addr;
		}
	}
	vmalloc_header_t *header = (vmalloc_header_t *)virt_addr_first_page;
	header->size = size;
	header->next = NULL;
	header->prev = NULL;

	if (g_vmalloc_last_alloc) {
		header->prev = g_vmalloc_last_alloc;
		g_vmalloc_last_alloc->next = header;
	}
	else {
		g_vmalloc_heap = header;
	}
	g_vmalloc_last_alloc = header;
	return (void *)(header + 1);
}

k_size_t vsize(void *ptr)
{
	vmalloc_header_t *header = (vmalloc_header_t *)ptr - 1;
	return header->size;
}

void *vbrk(k_size_t increment);