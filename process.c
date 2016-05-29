#include "process.h"
#include <stddef.h>
#include "list.h"
#include "paging.h"
#include "threads.h"
#include "elf.h"
#include "kmem_cache.h"
#include "memory.h"

#define USERSPACE_STACK_DEFAULT_SIZE PAGE_SIZE
#define USERSPACE_HIGH 0x100000000

p_mm_entry * create_p_mm_entry(virt_t user_virt, phys_t paddr, size_t size, mm_entry_type type) {
	p_mm_entry * new_mm_entry = kmem_alloc(sizeof(p_mm_entry));
	list_init(&new_mm_entry->list);
	new_mm_entry->region.len = size;
	new_mm_entry->region.phys_start = paddr;
	new_mm_entry->region.virt_start = user_virt;
	new_mm_entry->type = type;
	return new_mm_entry;
}

void destroy_p_mm_entry(p_mm_entry * entry) {
	list_del(&entry->list);
	kmem_free(entry);
}
