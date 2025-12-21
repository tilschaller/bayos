#include <memory.h>

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