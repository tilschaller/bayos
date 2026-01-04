#include <memory.h>
#include <types/types.h>
#include <stdio.h>
#include <fail.h>
#include <string.h>

/*
the heap is placed directly beneath the kernel
*/
#define HEAP_LENGTH 0x200000
#define HEAP_ADDRESS 0xffffffff80000000 - HEAP_LENGTH

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
	uint64_t addr;
	uint64_t length;
	uint32_t type;
	uint32_t acpi;
} __attribute__((packed)) memory_map_entry;

typedef struct {
	uint32_t num_of_entries;
	memory_map_entry memory_map[];
} __attribute__((packed)) memory_map;
	
typedef struct {
	memory_map *memory_map;
	uint64_t region;
	uint64_t offset;
} frame_allocator;

frame_allocator f_allocator;

static uint64_t next_region(void) {
        for (uint32_t i = f_allocator.region + 1; i < f_allocator.memory_map->num_of_entries; i++) {
                if (f_allocator.memory_map->memory_map[i].type == 1 &&
                    f_allocator.memory_map->memory_map[i].length >= 0x1000) {
                        return i;
                }
        }
        return 0;
}

/*
returns the physical address of an unused 4KB page
*/
uintptr_t allocate_frame(void) {
        start:
	memory_map_entry *r = &f_allocator.memory_map->memory_map[f_allocator.region];
        f_allocator.offset += 0x1000;
        if (r->length >= f_allocator.offset) {
                uint64_t return_addr = r->addr + f_allocator.offset;
                if (return_addr < 0x400000) {
                        goto start;
                } else {
                        return return_addr; 

                }
        } else {
                uint64_t region = next_region();
                if (region == 0) {
                        return 0;
                } else {
                        f_allocator.region = region;
                        f_allocator.offset = 0;
                        goto start;
                }
        }
}


typedef struct {
        uint64_t *pml4;
} memory_mapper;

memory_mapper mapper;

/*
maps a 4KB phyiscal page to virt address
flags is unused
*/
void map_to(uintptr_t phys, uintptr_t virt, int flags) {
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
                mapper.pml4[level_4_entry] = frame | 3;
        }
        uint64_t *level_3 = phys_to_virt(mapper.pml4[level_4_entry] & ~(0xfff));
        if ((level_3[level_3_entry] & 1) == 0) {
                uintptr_t frame = allocate_frame();
                memset(phys_to_virt(frame), 0, 0x1000);
                level_3[level_3_entry] = frame | 3;
        }
        uint64_t *level_2 = phys_to_virt(level_3[level_3_entry] & ~(0xfff));
        if ((level_2[level_2_entry] & 1) == 0) {
                uintptr_t frame = allocate_frame();
                memset(phys_to_virt(frame), 0, 0x1000);
                level_2[level_2_entry] = frame | 3;
        }
        uint64_t *level_1 = phys_to_virt(level_2[level_2_entry] & ~(0xfff));
        level_1[level_1_entry] = phys | 3;

        asm volatile("invlpg (%0)" :: "r"(virt) : "memory");

}

void mem_init(void *mm) {
        memory_map* memory_map = mm;
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
        uint64_t *pdpt_low = phys_to_virt(mapper.pml4[0] & 0xfff);
        /*
        pdpt_low is 0x1000 bytes long and can be used for other things
	for example to map the kernel heap
        */
        /*
        unmap low memory
        */
        mapper.pml4[0] = 0;
	/*
	 * init the frame allocator
	*/

	f_allocator.memory_map = memory_map;
        f_allocator.offset = 0xfffffffffff;
	for (uint32_t i = 0; i < memory_map->num_of_entries; i++) {
                if (memory_map->memory_map[i].type == 1 && 
                    memory_map->memory_map[i].length >= 0x1000) {
                        if (f_allocator.offset > 0) {
                                f_allocator.region = i;
                                f_allocator.offset = 0;
                        }
                        uint64_t new_start = (memory_map->memory_map[i].addr + 0x1000) & ~(0x1000);
                        memory_map->memory_map[i].length -= new_start - memory_map->memory_map[i].addr;
                        memory_map->memory_map[i].addr = new_start;
                }
        }


        if (f_allocator.offset != 0) hcf();

        /*
        create a address space for the heap
        */
        for (int i = 0; i < HEAP_LENGTH / 0x1000; i++) {
                uintptr_t frame = allocate_frame();
                uintptr_t addr = HEAP_ADDRESS + i * 0x1000;
                map_to(frame, addr, 0);
        }
}
