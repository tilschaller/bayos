#include <memory.h>
#include <types/types.h>
#include <stdio.h>
#include <fail.h>
#include <string.h>
#include <lock.h>
#include <limine.h>

void *phys_to_virt(uint64_t phys) {
	/*
	        we have a offset page table from bootloader
	        -> for every virt address below 100GiB
	        applies: phys + offset = virt
	        offset = 0xffff800000000000
	*/
	if (phys > 107374182400) return 0;
	uint64_t virt = phys + 0xffff800000000000;
	return (void*)virt;
}

void *virt_to_phys(uint64_t virt) {
	if (virt > 107374182400 + 0xffff800000000000 || virt < 0xffff800000000000) return 0;
	uint64_t phys = virt - 0xffff800000000000;
	return (void*)phys;
}

typedef struct {
	struct limine_memmap_response *memory_map;
	uint64_t region;
	uint64_t offset;
} frame_allocator;

frame_allocator f_allocator;

static uint64_t next_region(void) {
	for (uint32_t i = f_allocator.region + 1; i < f_allocator.memory_map->entry_count; i++) {
		if (f_allocator.memory_map->entries[i]->type == LIMINE_MEMMAP_USABLE &&
		                f_allocator.memory_map->entries[i]->length >= 0x1000) {
			return i;
		}
	}
	return 0;
}

spinlock_t map_to_lock = {0};
spinlock_t allocate_frame_lock = {0};

/*
returns the physical address of an unused 4KB page
*/
uintptr_t allocate_frame(void) {
	acquire(&allocate_frame_lock);
start:
	struct limine_memmap_entry *r = f_allocator.memory_map->entries[f_allocator.region];
	f_allocator.offset += 0x1000;
	if (r->length >= f_allocator.offset) {
		uint64_t return_addr = r->base + f_allocator.offset;
		release(&allocate_frame_lock);
		return return_addr;
	} else {
		uint64_t region = next_region();
		if (region == 0) {
			release(&allocate_frame_lock);
			return 0;
		} else {
			f_allocator.region = region;
			f_allocator.offset = 0;
			goto start;
		}
	}
	release(&allocate_frame_lock);
	return 0;
}


typedef struct {
	uint64_t *pml4;
} memory_mapper;

memory_mapper mapper;

/*
maps a 4KB phyiscal page to virt address
*/
void map_to(uintptr_t phys, uintptr_t virt, int flags) {
	acquire(&map_to_lock);
	(void)flags;

	size_t level_4_entry = (virt >> 39) & 0x1ff;
	size_t level_3_entry = (virt >> 30) & 0x1ff;
	size_t level_2_entry = (virt >> 21) & 0x1ff;
	size_t level_1_entry = (virt >> 12) & 0x1ff;

	/*
	now check if the pages already exists
	*/
	if ((mapper.pml4[level_4_entry] & 1) == 0) {
		uintptr_t frame = allocate_frame();
		memset(phys_to_virt(frame), 0, 0x1000);
		mapper.pml4[level_4_entry] = frame | flags;
	}
	uint64_t *level_3 = phys_to_virt(mapper.pml4[level_4_entry] & ~(0xfff));
	if ((level_3[level_3_entry] & 1) == 0) {
		uintptr_t frame = allocate_frame();
		memset(phys_to_virt(frame), 0, 0x1000);
		level_3[level_3_entry] = frame | flags;
	}
	uint64_t *level_2 = phys_to_virt(level_3[level_3_entry] & ~(0xfff));
	if ((level_2[level_2_entry] & 1) == 0) {
		uintptr_t frame = allocate_frame();
		memset(phys_to_virt(frame), 0, 0x1000);
		level_2[level_2_entry] = frame | flags;
	}
	uint64_t *level_1 = phys_to_virt(level_2[level_2_entry] & ~(0xfff));
	level_1[level_1_entry] = phys | flags;

	asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
	release(&map_to_lock);
}

void mem_init(struct limine_memmap_response *memory_map) {
	/*
	first we retrieve the pml4 from cr3 register
	*/
	uint64_t pml4;
	asm volatile("mov %%cr3, %0" : "=r"(pml4) : : );
	/*
	then we find the address of entry 0
	in pml4
	*/
	mapper.pml4 = phys_to_virt(pml4 & ~0xfff);
	/*
	*/
	/*
	unmap low memory
	*/
	mapper.pml4[0] = 0;

	/*
	 * init the frame allocator
	*/

	release(&map_to_lock);
	release(&allocate_frame_lock);

	f_allocator.memory_map = memory_map;
	f_allocator.offset = 0xfffffffffff;
	for (uint32_t i = 0; i < memory_map->entry_count; i++) {
		if (memory_map->entries[i]->type == LIMINE_MEMMAP_USABLE &&
		                memory_map->entries[i]->length >= 0x1000) {
			if (f_allocator.offset > 0) {
				f_allocator.region = i;
				f_allocator.offset = 0;
			}
			uint64_t new_start = (memory_map->entries[i]->base + 0x1000) & ~(0x1000);
			memory_map->entries[i]->length -= new_start - memory_map->entries[i]->base;
			memory_map->entries[i]->base = new_start;
		}
	}

	if (f_allocator.offset != 0) hcf();

	/*
	create a address space for the heap
	*/
	for (int i = 0; i < HEAP_LENGTH / 0x1000; i++) {
		uintptr_t frame = allocate_frame();
		uintptr_t addr = HEAP_ADDRESS + i * 0x1000;
		map_to(frame, addr, FLAG_PRESENT | FLAG_WRITEABLE);
	}
}
