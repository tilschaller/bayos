#ifndef _MEMORY_H
#define _MEMORY_H

#include <types/types.h>

void *phys_to_virt(uint64_t phys);
void *virt_to_phys(uint64_t virt);
void mem_init(void *memory_map);

#endif // _MEMORY_H