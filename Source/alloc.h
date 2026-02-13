#ifndef ALLOC_H
#define ALLOC_H

#include "memory.h"

void init_allocators();

void *kmalloc(k_size_t size);
void kfree(void *ptr);
k_size_t ksize(void *ptr);
void *kbrk(k_size_t increment);


typedef struct vmalloc_header_s {
	// struct vmalloc_header_s *prev;
	struct vmalloc_header_s *next;
	// uint32_t free;
	k_size_t size;
} vmalloc_header_t;

void *vmalloc(k_size_t size);
void vfree(void *ptr);
k_size_t vsize(void *ptr);
void *vbrk(k_size_t increment);

#endif // ALLOC_H
