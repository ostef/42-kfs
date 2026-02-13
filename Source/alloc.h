#ifndef ALLOC_H
#define ALLOC_H

#include "memory.h"

#define KMALLOC_TOTAL_CAPACITY (4 * 1024 * 1024)

void allocators_init(void);

void *kmalloc(k_size_t size);
void kfree(void *ptr);
k_size_t ksize(void *ptr);


typedef struct vmalloc_header_s {
	struct vmalloc_header_s *prev;
	struct vmalloc_header_s *next;
	k_size_t size;
	uint32_t unused; // Padding to make the header size a multiple of 16 bytes, which is important for alignment
} vmalloc_header_t;

void *vmalloc(k_size_t size);
void vfree(void *ptr);
k_size_t vsize(void *ptr);
void *vbrk(k_size_t increment);

void allocators_print_info(void);

#endif // ALLOC_H
