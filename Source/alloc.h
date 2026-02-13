#ifndef ALLOC_H
#define ALLOC_H

#include "memory.h"

void allocators_init();

void *kmalloc(k_size_t size);
void kfree(void *ptr);
k_size_t ksize(void *ptr);

void *vmalloc(k_size_t size);
void vfree(void *ptr);
k_size_t vsize(void *ptr);
void *vbrk(k_size_t increment);

#endif // ALLOC_H
