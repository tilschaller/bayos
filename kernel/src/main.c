#include <types/types.h>
#include <framebuffer.h>
#include <fail.h>
#include <memory.h>
#include <stdio.h>

__attribute__((noreturn))
void _start(uint64_t memory_map, uint64_t video_info) {
        (void)memory_map;

        video_mode_info *vinfo = phys_to_virt(video_info);
        fb_init(vinfo);

<<<<<<< HEAD
        printf("#########################\n");
        printf("#    Welcome to Bayos   #\n");
        printf("#########################\n");

=======
        printf("bayos!\n");
        
>>>>>>> c5cd6ec (kernel: add basic \ -operators in printf())
        hcf();
}
