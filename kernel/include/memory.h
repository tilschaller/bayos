#ifndef _MEMORY_H
#define _MEMORY_H

#include <types/types.h>

/*
the heap is placed directly beneath the kernel
*/
#define HEAP_LENGTH 0x200000
#define HEAP_ADDRESS (0xffffffff80000000 - HEAP_LENGTH)
#define HIGHER_HALF 0xffff800000000000

// align to power of two
#define ALIGN(num, power) \
	(((num) + (power) - 1) & ~((power) - 1))

#define FLAG_PRESENT 1
#define FLAG_WRITEABLE (1 << 1)
#define FLAG_USER (1 << 2)

void *phys_to_virt(uint64_t phys);
void *virt_to_phys(uint64_t virt);
void mem_init(void *memory_map);
uintptr_t allocate_frame(void);
void map_to(uintptr_t phys, uintptr_t virt, int flags);

#endif /* _MEMORY_H */
