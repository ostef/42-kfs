#ifndef ALLOC_H
#define ALLOC_H

#include "memory.h"

#define KMALLOC_TOTAL_CAPACITY (4 * 1024 * 1024)

#define VMALLOC_VIRT_START 0xe0000000
#define VMALLOC_VIRT_END   0xffffffff

void kmalloc_init(void);
void kmalloc_print_info(void);

void *kmalloc(k_size_t size);
void kfree(void *ptr);
k_size_t ksize(void *ptr);

void vmalloc_init(void);
void vmalloc_print_info(void);

void *vmalloc(k_size_t size);
void vfree(void *ptr);
k_size_t vsize(void *ptr);

#endif // ALLOC_H
