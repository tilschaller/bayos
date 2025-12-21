#include <types/types.h>

__attribute__((noreturn))
void hcf() {
        for (;;) {
                asm volatile("hlt");
        }
}

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

__attribute__((noreturn))
void _start(uint64_t memory_map, uint64_t video_mode) {

        (void)memory_map;
        (void)video_mode;

        /*
                NOTE: the struct for video mode
                can be found in bootloader source
                -> copy to kernel header -> makes
                working with this easier
        */
        volatile uint32_t *framebuffer = phys_to_virt(*(uint64_t*)(video_mode + 0x28));
        if (framebuffer == 0) {
                hcf();
        }

        for (int i = 0; i < 100; i++) {
                // Note: we assume the framebuffer model is rgb with 32bit pixels
                framebuffer[i * 500 + i] = 0xffffff;  
        }

        hcf();
}
