#include <memory.h>
#include <types/types.h>

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
        uint64_t *pml4;
} memory_mapper;

memory_mapper mapper;

void mem_init(void *memory_map) {
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
        */
        /*
        unmap low memory
        */
        mapper.pml4[0] = 0;
}