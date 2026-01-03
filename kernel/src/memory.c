#include <memory.h>
#include <types/types.h>
#include <stdio.h>
#include <fail.h>

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
        uint64_t *pml4;
} memory_mapper;

memory_mapper mapper;
	
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

void *allocate_frame(void) {
        start:
	memory_map_entry *r = &f_allocator.memory_map->memory_map[f_allocator.region];
        f_allocator.offset += 0x1000;
        if (r->length >= f_allocator.offset) {
                uint64_t return_addr = r->addr + f_allocator.offset;
                if (return_addr < 0x400000) {
                        goto start;
                } else {
                        return (void*)return_addr; 

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
}
