#include "alloc.h"

static const k_size_t g_kmalloc_size_classes[] = {
	64, 128, 256, 512, 1024,
};

#define NUM_KMALLOC_SIZE_CLASSES k_array_count(g_kmalloc_size_classes)
#define NUM_KMALLOC_DEFAULT_BINS 3

struct kmalloc_bin_t;

typedef struct kmalloc_header_t {
	struct kmalloc_header_t *prev;
	struct kmalloc_header_t *next;
	k_size_t size;
	struct kmalloc_bin_t *bin;
} kmalloc_header_t;

typedef struct kmalloc_bin_t {
	struct kmalloc_bin_t *next;
	k_size_t alloc_size;
	kmalloc_header_t *free_list;
	kmalloc_header_t *occupied_list;
} kmalloc_bin_t;

typedef struct kmalloc_heap_t {
	void *start_addr;
	k_size_t total_size;
	kmalloc_bin_t *bin_list[NUM_KMALLOC_SIZE_CLASSES];
	kmalloc_header_t *first_big_alloc;
	kmalloc_header_t *last_big_alloc;
} kmalloc_heap_t;

static kmalloc_heap_t g_kmalloc_heap;

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
kmalloc_header_t *alloc_pop_front(kmalloc_header_t **first) {
	if (!first) {
		return NULL;
	}

	kmalloc_header_t *header = *first;
	if (!header) {
		return NULL;
	}

	*first = header->next;

	if (header->prev) {
		header->prev->next = header->next;
		header->prev = NULL;
	}

	if (header->next) {
		header->next->prev = header->prev;
		header->next = NULL;
	}

	return header;
}

static
void alloc_push_back(kmalloc_header_t **first, kmalloc_header_t **last, kmalloc_header_t *header) {
	if (!first || !last || !header) {
		return;
	}

	if (*last) {
		(*last)->next = header;
		header->prev = *last;
		header->next = NULL;
		*last = header;
	} else {
		*first = header;
		*last = header;
		header->prev = NULL;
		header->next = NULL;
	}
}

static
void alloc_pop(kmalloc_header_t **first, kmalloc_header_t **last, kmalloc_header_t *header) {
	if (!first || !header) {
		return;
	}

	if (*first == header) {
		*first = header->next;
	}
	if (last && *last == header) {
		*last = header->prev;
	}

	if (header->prev) {
		header->prev->next = header->next;
	}
	if (header->next) {
		header->next->prev = header->prev;
	}

	header->prev = NULL;
	header->next = NULL;
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
bool is_big_alloc(k_size_t size) {
	return get_size_class(size) < 0;
}

static
kmalloc_bin_t *create_bin(kmalloc_heap_t *heap, k_size_t alloc_size) {
	int size_class = get_size_class(alloc_size);
	k_assert(size_class >= 0, "Invalid size class");

	alloc_size = g_kmalloc_size_classes[size_class];

	kmalloc_bin_t *bin = kbrk(0);
	if (!bin) {
		return NULL;
	}

	if (!kbrk(MEM_PAGE_SIZE)) {
		return NULL;
	}

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
	alloc_pop(&bin->occupied_list, NULL, alloc);
	alloc_push_front(&bin->free_list, alloc);
}

static
void *big_alloc(kmalloc_heap_t *heap, k_size_t size) {
	k_size_t size_with_header = size + sizeof(kmalloc_header_t);
	void *ptr = kbrk(0);
	if (!kbrk(size_with_header)) {
		return NULL;
	}

	kmalloc_header_t *header = (kmalloc_header_t *)ptr;
	header->size = size;
	header->bin = NULL;
	alloc_push_back(&heap->first_big_alloc, &heap->last_big_alloc, header);

	return (void *)(header + 1);
}

static
void big_free(kmalloc_heap_t *heap, kmalloc_header_t *alloc) {
	alloc_pop(&heap->first_big_alloc, &heap->last_big_alloc, alloc);
	k_memset(alloc, 0, sizeof(kmalloc_header_t));
	// mem_free_physical_memory(alloc, alloc->size + sizeof(kmalloc_header_t));
}

void allocators_init() {
	for (int j = 0; j < NUM_KMALLOC_DEFAULT_BINS; j += 1) {
		for (int i = 0; i < (int)NUM_KMALLOC_SIZE_CLASSES; i += 1) {
			if (!create_bin(&g_kmalloc_heap, g_kmalloc_size_classes[i])) {
				k_printf("Could not allocate kmalloc bin for size %d\n", g_kmalloc_size_classes[i]);
				return;
			}
		}
	}
}

void *kmalloc(k_size_t size) {
	if (size <= 0) {
		return NULL;
	}

	if (is_big_alloc(size)) {
		return big_alloc(&g_kmalloc_heap, size);
	}

	kmalloc_bin_t *bin = get_bin(&g_kmalloc_heap, size);
	if (!bin) {
		bin = create_bin(&g_kmalloc_heap, size);

		if (!bin) {
			return NULL;
		}
	}

	return bin_alloc(bin);
}

void kfree(void *ptr) {
	kmalloc_header_t *header = (kmalloc_header_t *)ptr - 1;

	if (!header->bin) {
		big_free(&g_kmalloc_heap, header);
	} else {
		bin_free(header->bin, header);
	}
}

k_size_t ksize(void *ptr) {
	kmalloc_header_t *header = (kmalloc_header_t *)ptr - 1;

	return header->size;
}
