#include "alloc.h"

// All allocations are 16-byte aligned
//
// Kmalloc manages bins. Each bin manages a list of allocation
// There are two different types of allocations in kmalloc:
//
// 1- small allocations (< page size): we allocate a page worth of memory, and fragment that
// page with as many fixed size blocks as possible. The bin is put at the
// top of the page, and added to the list of bins. The size of the allocations
// is one of a fixed set of size class, each allocation request is rounded up
// to the next size class.
//
// 2- big allocations (>= page size): for big allocations we have only one bin per size class.
// All bins are stored statically inside the kmalloc_heap_t structure, not
// allocated on the go like for small allocations. Slots for big allocations
// are not preallocated, though once a big allocation is made, the slot is reused
// for the next allocations if it has been freed.
//
// For both small and big allocations, a header is put at the start of the block
// of memory to store necessary metadata (the bin and size, prev and next ptrs).

typedef struct kmalloc_header_t {
	struct kmalloc_header_t *prev;
	struct kmalloc_header_t *next;
	k_size_t size;
	void *bin;
} kmalloc_header_t;

k_static_assert(sizeof(kmalloc_header_t) % 16 == 0);

static const k_size_t g_kmalloc_size_classes[] = {
	64, 128, 256, 512, 1024,
};

static const k_size_t g_kmalloc_big_size_classes[] = {
	     4096 - sizeof(kmalloc_header_t), //   4k
	 2 * 4096 - sizeof(kmalloc_header_t), //   8k
	 4 * 4096 - sizeof(kmalloc_header_t), //  16k
	 8 * 4096 - sizeof(kmalloc_header_t), //  32k
	16 * 4096 - sizeof(kmalloc_header_t), //  64k
	32 * 4096 - sizeof(kmalloc_header_t), // 128k
};

#define NUM_KMALLOC_SIZE_CLASSES k_array_count(g_kmalloc_size_classes)
#define NUM_KMALLOC_BIG_SIZE_CLASSES k_array_count(g_kmalloc_big_size_classes)
#define NUM_KMALLOC_DEFAULT_BINS 3

typedef struct kmalloc_bin_t {
	struct kmalloc_bin_t *next;
	k_size_t alloc_size;
	kmalloc_header_t *free_list;
	kmalloc_header_t *occupied_list;
} kmalloc_bin_t;

k_static_assert(sizeof(kmalloc_bin_t) % 16 == 0);

typedef struct kmalloc_big_bin_t {
	struct kmalloc_header_t *free_list;
	struct kmalloc_header_t *occupied_list;
	k_size_t alloc_size;
} kmalloc_big_bin_t;

typedef struct kmalloc_heap_t {
	uint8_t *end_addr;
	uint8_t *curr_addr;

	kmalloc_bin_t *bin_list[NUM_KMALLOC_SIZE_CLASSES];
	kmalloc_big_bin_t big_bin_list[NUM_KMALLOC_BIG_SIZE_CLASSES];
} kmalloc_heap_t;

static kmalloc_heap_t g_kmalloc_heap;

static
bool extend_heap(kmalloc_heap_t *heap, k_size_t increment) {
	uint8_t *current_brk = kbrk(0);

	if (!kbrk(increment)) {
		return false;
	}

	// In case other subsystems used kbrk, we make sure to discard our previous blocks of memory
	if (current_brk != heap->end_addr) {
		heap->curr_addr = current_brk;
		heap->end_addr = kbrk(0);
	}

	return true;
}

static
void alloc_push_front(kmalloc_header_t **first, kmalloc_header_t *header) {
	if (!first) {
		return;
	}

	header->next = *first;
	if (header->next) {
		header->next->prev = header;
	}

	header->prev = NULL;
	*first = header;
}

static
void alloc_pop(kmalloc_header_t **first, kmalloc_header_t *header) {
	if (!first || !header) {
		return;
	}

	if (*first == header) {
		*first = header->next;
	}

	if (header->prev) {
		header->prev->next = header->next;
		header->prev = NULL;
	}

	if (header->next) {
		header->next->prev = header->prev;
		header->next = NULL;
	}
}

static
kmalloc_header_t *alloc_pop_front(kmalloc_header_t **first) {
	if (!first) {
		return NULL;
	}

	kmalloc_header_t *header = *first;
	if (!header) {
		return NULL;
	}

	alloc_pop(first, header);

	return header;
}

static
bool is_big_size_class(k_size_t size) {
	return size > g_kmalloc_size_classes[NUM_KMALLOC_SIZE_CLASSES - 1];
}

static
int get_size_class(k_size_t alloc_size) {
	for (int i = 0; i < (int)NUM_KMALLOC_SIZE_CLASSES; i += 1) {
		if (alloc_size <= g_kmalloc_size_classes[i]) {
			return i;
		}
	}

	return -1;
}

static
int get_big_size_class(k_size_t alloc_size) {
	for (int i = 0; i < (int)NUM_KMALLOC_BIG_SIZE_CLASSES; i += 1) {
		if (alloc_size <= g_kmalloc_big_size_classes[i]) {
			return i;
		}
	}

	return -1;
}

static
kmalloc_bin_t *create_bin(kmalloc_heap_t *heap, k_size_t alloc_size) {
	int size_class = get_size_class(alloc_size);
	k_assert(size_class >= 0, "Invalid size class");

	alloc_size = g_kmalloc_size_classes[size_class];

	if (heap->curr_addr + MEM_PAGE_SIZE > heap->end_addr) {
		if (!extend_heap(heap, MEM_PAGE_SIZE)) {
			return NULL;
		}
	}

	kmalloc_bin_t *bin = (kmalloc_bin_t *)heap->curr_addr;
	if (!bin) {
		return NULL;
	}

	heap->curr_addr += MEM_PAGE_SIZE;
	k_memset(bin, 0, sizeof(*bin));

	bin->next = heap->bin_list[size_class];
	heap->bin_list[size_class] = bin;

	bin->alloc_size = alloc_size;
	bin->free_list = (kmalloc_header_t *)(bin + 1);

	kmalloc_header_t *prev = bin->free_list;
	k_memset(prev, 0, sizeof(*prev));
	prev->bin = bin;
	prev->size = alloc_size;

	k_size_t size_with_header = alloc_size + sizeof(kmalloc_header_t);
	k_size_t num_slots = (MEM_PAGE_SIZE - sizeof(*bin)) / size_with_header;
	for (k_size_t i = 1; i < num_slots; i += 1) {
		kmalloc_header_t *header = (kmalloc_header_t *)((uint8_t *)bin->free_list + i * size_with_header);
		prev->next = header;
		header->prev = prev;
		header->next = NULL;
		header->bin = bin;
		header->size = alloc_size;

		prev = header;
	}

	k_printf("Created kmalloc bin for size %d (class %d)\n", alloc_size, size_class);

	return bin;
}

static
kmalloc_bin_t *get_bin(kmalloc_heap_t *heap, k_size_t alloc_size) {
	int class = get_size_class(alloc_size);
	if (class < 0) {
		return NULL;
	}

	for (kmalloc_bin_t *bin = heap->bin_list[class]; bin; bin = bin->next) {
		if (bin->free_list) {
			return bin;
		}
	}

	return NULL;
}

static
void *bin_alloc(kmalloc_bin_t *bin) {
	kmalloc_header_t *alloc = alloc_pop_front(&bin->free_list);
	if (!alloc) {
		return NULL;
	}

	alloc_push_front(&bin->occupied_list, alloc);

	return (void *)(alloc + 1);
}

static
void bin_free(kmalloc_bin_t *bin, kmalloc_header_t *alloc) {
	alloc_pop(&bin->occupied_list, alloc);
	alloc_push_front(&bin->free_list, alloc);
}

static
void *big_alloc(kmalloc_heap_t *heap, kmalloc_big_bin_t *bin) {
	kmalloc_header_t *alloc = alloc_pop_front(&bin->free_list);
	if (alloc) {
		alloc_push_front(&bin->occupied_list, alloc);

		return (void *)(alloc + 1);
	}

	k_size_t size_with_header = bin->alloc_size + sizeof(kmalloc_header_t);

	if (heap->curr_addr + size_with_header > heap->end_addr) {
		if (!extend_heap(heap, size_with_header)) {
			return NULL;
		}
	}

	void *ptr = heap->curr_addr;
	if (!ptr) {
		return NULL;
	}

	heap->curr_addr += size_with_header;

	alloc = (kmalloc_header_t *)ptr;
	alloc->size = bin->alloc_size;
	alloc->bin = bin;
	alloc_push_front(&bin->occupied_list, alloc);

	return (void *)(alloc + 1);
}

static
void big_free(kmalloc_big_bin_t *bin, kmalloc_header_t *alloc) {
	alloc_pop(&bin->occupied_list, alloc);
	alloc_push_front(&bin->free_list, alloc);
}

void kmalloc_init(void) {
	if (!extend_heap(&g_kmalloc_heap, KMALLOC_TOTAL_CAPACITY)) {
		k_panic("Could not initialize kmalloc");
	}

	for (int i = 0; i < (int)NUM_KMALLOC_BIG_SIZE_CLASSES; i += 1) {
		kmalloc_big_bin_t *bin = &g_kmalloc_heap.big_bin_list[i];
		bin->alloc_size = g_kmalloc_big_size_classes[i];
	}

	for (int j = 0; j < NUM_KMALLOC_DEFAULT_BINS; j += 1) {
		for (int i = 0; i < (int)NUM_KMALLOC_SIZE_CLASSES; i += 1) {
			if (!create_bin(&g_kmalloc_heap, g_kmalloc_size_classes[i])) {
				k_printf("Warning: could not allocate kmalloc bin for size %d\n", g_kmalloc_size_classes[i]);
			}
		}
	}

	k_printf("Initialized kernel allocators\n");
}

void *kmalloc(k_size_t size) {
	if (size <= 0 || size > g_kmalloc_big_size_classes[NUM_KMALLOC_BIG_SIZE_CLASSES - 1]) {
		return NULL;
	}

	if (is_big_size_class(size)) {
		int size_class = get_big_size_class(size);
		k_assert(size_class >= 0, "");

		kmalloc_big_bin_t *bin = &g_kmalloc_heap.big_bin_list[size_class];

		return big_alloc(&g_kmalloc_heap, bin);
	} else {
		kmalloc_bin_t *bin = get_bin(&g_kmalloc_heap, size);
		if (!bin) {
			bin = create_bin(&g_kmalloc_heap, size);

			if (!bin) {
				return NULL;
			}
		}

		return bin_alloc(bin);
	}
}

void kfree(void *ptr) {
	kmalloc_header_t *header = (kmalloc_header_t *)ptr - 1;

	k_assert(header->bin != NULL, "Invalid ptr");

	if (is_big_size_class(header->size)) {
		big_free((kmalloc_big_bin_t *)header->bin, header);
	} else {
		bin_free((kmalloc_bin_t *)header->bin, header);
	}
}

k_size_t ksize(void *ptr) {
	kmalloc_header_t *header = (kmalloc_header_t *)ptr - 1;

	return header->size;
}

void kmalloc_print_info(void) {
	k_printf("Kmalloc heap info:\n");

	for (int i = 0; i < (int)NUM_KMALLOC_SIZE_CLASSES; i += 1) {
		k_printf("Bins %d, size %n:\n", i, g_kmalloc_size_classes[i]);
		int bin_idx = 0;
		int total_num_free_slots = 0;
		int total_num_occupied_slots = 0;
		for (kmalloc_bin_t *bin = g_kmalloc_heap.bin_list[i]; bin; bin = bin->next) {
			int num_free_slots = 0;
			for (kmalloc_header_t *alloc = bin->free_list; alloc; alloc = alloc->next) {
				num_free_slots += 1;
			}

			int num_occupied_slots = 0;
			for (kmalloc_header_t *alloc = bin->occupied_list; alloc; alloc = alloc->next) {
				num_occupied_slots += 1;
			}

			k_printf("  [%d]: %d free slot(s), %d occupied slot(s)\n", bin_idx, num_free_slots, num_occupied_slots);

			total_num_free_slots += num_free_slots;
			total_num_occupied_slots += num_occupied_slots;

			bin_idx += 1;
		}

		k_printf("  %d total free slot(s), %d occupied\n", total_num_free_slots, total_num_occupied_slots);
	}

	for (int i = 0; i < (int)NUM_KMALLOC_BIG_SIZE_CLASSES; i += 1) {
		kmalloc_big_bin_t *bin = &g_kmalloc_heap.big_bin_list[i];

		int num_free_slots = 0;
		for (kmalloc_header_t *alloc = bin->free_list; alloc; alloc = alloc->next) {
			num_free_slots += 1;
		}

		int num_occupied_slots = 0;
		for (kmalloc_header_t *alloc = bin->occupied_list; alloc; alloc = alloc->next) {
			num_occupied_slots += 1;
		}

		k_printf("Big bin %d, size %n: %d free slot(s), %d occupied slot(s)\n", i, g_kmalloc_big_size_classes[i], num_free_slots, num_occupied_slots);
	}

}
