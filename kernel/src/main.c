#include <types/types.h>
#include <framebuffer.h>
#include <fail.h>
#include <memory.h>
#include <stdio.h>
#include <gdt.h>
#include <interrupts.h>

__attribute__((noreturn))
void _start(uint64_t memory_map, uint64_t video_info) {
        (void)memory_map;

        video_mode_info *vinfo = phys_to_virt(video_info);
        fb_init(vinfo);

        printf("#########################\n");
        printf("#    Welcome to Bayos   #\n");
        printf("#########################\n");

        /* printf("%ho \n %llx", 5687, 8); */

        gdt_init();
        int_init();
        mem_init(
                phys_to_virt(memory_map)
        );
        
        hcf();
}
