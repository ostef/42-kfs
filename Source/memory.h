#ifndef MEMORY_H
#define MEMORY_H

#include "libkernel.h"
#include "multiboot.h"

void mem_init_with_multiboot_info(const multiboot_info_t *info);

uint32_t mem_get_used_physical_blocks(void);
uint32_t mem_get_total_physical_blocks(void);
uint32_t mem_get_remaining_physical_memory(void);
uint32_t mem_get_total_physical_memory(void);

void mem_print_physical_memory_map(void);

// http://wiki.osdev.org/Paging
// http://www.brokenthorn.com/Resources/OSDev18.html

typedef struct virt_addr_t {
    uint32_t page_offset : 12;
    uint32_t page_index : 10;
    uint32_t directory_index : 10;
} virt_addr_t;

static inline
virt_addr_t make_virt_addr(uint32_t addr) {
    return *((virt_addr_t *)&addr);
}

typedef struct mem_page_table_entry_t {
    uint32_t is_present_in_physical_memory : 1;
    uint32_t is_writable : 1;
    uint32_t is_user_mode : 1;
    uint32_t pat_enable_writethrough : 1; // Can be set if enable_pat is set
    uint32_t pat_disable_caching : 1; // Can be set if enable_pat is set
    uint32_t has_been_accessed : 1; // Set by the MMU, can be reset by the kernel
    uint32_t has_been_written_to : 1; // Set by the MMU, can be reset by the kernel
    uint32_t enable_pat : 1; // Only supported since Pentium3
    uint32_t is_cpu_global : 1;
    uint32_t unused : 3;
    uint32_t physical_addr_4KiB : 20; // Physical address divided by 4096 (this is why we can have only 20 bits)
} mem_page_table_entry_t;

typedef struct mem_page_dir_entry_t {
    uint32_t is_present_in_physical_memory : 1;
    uint32_t is_writable : 1;
    uint32_t is_user_mode : 1;
    uint32_t pat_enable_writethrough : 1;
    uint32_t pat_disable_caching : 1;
    uint32_t has_been_accessed : 1;
    uint32_t reserved : 1;
    uint32_t pages_size_is_4_mib : 1;
    uint32_t is_cpu_global : 1;
    uint32_t unused : 3;
    uint32_t page_table_physical_addr_4KiB : 19; // Physical address divided by 4096 (this is why we can have only 19 bits)
} mem_page_dir_entry_t;

#define MEM_NUM_PAGE_TABLE_ENTRIES 1024
#define MEM_NUM_PAGE_DIR_TABLE_ENTRIES 1024
#define MEM_PAGE_SIZE 4096
#define MEM_PAGE_TABLE_ADDR_RANGE (MEM_PAGE_SIZE * MEM_NUM_PAGE_TABLE_ENTRIES)
#define MEM_PAGE_DIR_TABLE_ADDR_RANGE (MEM_PAGE_TABLE_ADDR_RANGE * Mem_NumPageDirectoryTableEntries)

typedef struct mem_page_table_t {
    mem_page_table_entry_t entries[MEM_NUM_PAGE_TABLE_ENTRIES];
} mem_page_table_t;

typedef struct mem_page_dir_table_t {
    mem_page_dir_entry_t entries[MEM_NUM_PAGE_DIR_TABLE_ENTRIES];
} mem_page_dir_table_t;

bool mem_alloc_page_physical(mem_page_table_entry_t *entry);
void mem_free_page_physical(mem_page_table_entry_t *entry);
mem_page_table_entry_t *mem_get_page_table_entry(mem_page_table_t *table, virt_addr_t addr);

void mem_set_paging_enabled(bool enabled);
void mem_flush_tlb(void);
void mem_flush_page(virt_addr_t addr);
bool mem_change_page_dir_table(mem_page_dir_table_t *table);
mem_page_dir_table_t *mem_get_current_page_dir_table(void);

bool mem_map_page(uint32_t physical_addr, virt_addr_t virt_addr);

#endif // MEMORY_H
