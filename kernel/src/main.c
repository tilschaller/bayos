/*
        TODO: add implementation of
                memset
                memcmp
                memcpy
                memmove
                in string.h and string.c
                and then remove -fno-builtin from Makefile
        TODO: add header for standart types like
                uint32_t -> unsigned int
                uint16_t -> unsigned short
                uint8_t -> unsigned long long
                size_t .... 
                etc.
                and then replace types in main.h with new types

*/

__attribute__((noreturn))
void hcf() {
        for (;;) {
                asm volatile("hlt");
        }
}

void *phys_to_virt(unsigned long long phys) {
        /*
                we have a offset page table from bootloader 
                -> for every virt address below 100GiB
                applies: phys + offset = virt
                offset = 0xffff800000000000
        */
        if (phys > 107374182400) return 0;
        unsigned long long virt = phys + 0xffff800000000000;
        return (void*)virt;
} 

__attribute__((noreturn))
void _start(unsigned long long memory_map, unsigned long long video_mode) {

        (void)memory_map;
        (void)video_mode;

        /*
                NOTE: the struct for video mode
                can be found in bootloader source
                -> copy to kernel header -> makes
                working with this easier
        */
        volatile unsigned int *framebuffer = phys_to_virt(*(unsigned long long*)(video_mode + 0x28));
        if (framebuffer == 0) {
                hcf();
        }

        for (int i = 0; i < 100; i++) {
                // Note: we assume the framebuffer model is rgb with 32bit pixels
                framebuffer[i * 500 + i] = 0xffffff;  
        }

        hcf();
}
