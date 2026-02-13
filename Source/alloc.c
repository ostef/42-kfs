#include "alloc.h"

static const k_size_t g_kmalloc_size_classes[] = {
	64, 128, 256, 512, 1024,
};

#define NUM_KMALLOC_SIZE_CLASSES k_array_count(g_kmalloc_size_classes)

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
	kmalloc_bin_t *first_bin[NUM_KMALLOC_SIZE_CLASSES];
	kmalloc_bin_t *last_bin[NUM_KMALLOC_SIZE_CLASSES];
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
void alloc_pop_single_ended(kmalloc_header_t **first, kmalloc_header_t *header) {

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
void alloc_pop_double_ended(kmalloc_header_t **first, kmalloc_header_t **last, kmalloc_header_t *header) {
	if (!first || !last || !header) {
		return;
	}

	if (*first == header) {
		*first = header->next;
	}
	if (*last == header) {
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
	for (int i = 0; i < NUM_KMALLOC_SIZE_CLASSES; i += 1) {
		if (alloc_size < g_kmalloc_size_classes[0]) {
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
	kmalloc_bin_t *bin = mem_alloc_physical_blocks(1);
	k_memset(bin, 0, sizeof(*bin));

	return bin;
}

static
kmalloc_bin_t *get_bin(kmalloc_heap_t *heap, k_size_t alloc_size) {
	int class = get_size_class(alloc_size);
	if (class < 0) {
		return NULL;
	}

	for (kmalloc_bin_t *bin = heap->first_bin[class]; bin; bin = bin->next) {
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
}

static
void *big_alloc(kmalloc_heap_t *heap, k_size_t size) {
	k_size_t size_with_header = size + sizeof(kmalloc_header_t);
	void *ptr = mem_alloc_physical_memory(size_with_header);

	kmalloc_header_t *header = (kmalloc_header_t *)ptr;
	header->size = size;
	header->bin = NULL;
	alloc_push_back(&heap->first_big_alloc, &heap->last_big_alloc, header);

	return (void *)(header + 1);
}

static
void *big_free(kmalloc_heap_t *heap, kmalloc_header_t *alloc) {
	alloc_pop_double_ended(&heap->first_big_alloc, &heap->last_big_alloc, alloc);
	k_memset(alloc, 0, sizeof(kmalloc_header_t));
	mem_free_physical_memory(alloc, alloc->size + sizeof(kmalloc_header_t));
}

void init_allocators() {

}

void *kmalloc(k_size_t size) {
	if (size <= 0) {
		return NULL;
	}

	if (is_big_alloc(size)) {
		return big_alloc(&g_kmalloc_heap, size);
	}
}

void kfree(void *ptr) {
	kmalloc_header_t *header = (kmalloc_header_t *)ptr - 1;

	if (!header->bin) {
		big_free(&g_kmalloc_heap, header);
	} else {
	}
}

k_size_t ksize(void *ptr) {
	kmalloc_header_t *header = (kmalloc_header_t *)ptr - 1;

	return header->size;
}

void *kbrk(k_size_t increment) {
}
