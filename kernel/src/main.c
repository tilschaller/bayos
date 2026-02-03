#include <types/types.h>
#include <framebuffer.h>
#include <fail.h>
#include <memory.h>
#include <stdio.h>
#include <gdt.h>
#include <interrupts.h>
#include <alloc.h>
#include <acpi.h>
#include <scheduler.h>

void wait(void) {
	printf("Second process\n");
	while (1) {
		asm volatile("hlt");
	}

	__builtin_unreachable();
}

__attribute__((noreturn))
void _start(uint64_t memory_map, uint64_t video_info) {
        fb_init(
		phys_to_virt(video_info)
	);

        printf("#########################\n");
        printf("#    Welcome to Bayos   #\n");
        printf("#########################\n");

        gdt_init();
        int_init();

        mem_init(
                phys_to_virt(memory_map)
        );

        heap_init();

	acpi_init();
	
	sched_init();

	enable_interrupts();

	add_process(wait);

	asm volatile("int $0x80");

	while (1) {
		asm volatile("hlt");
	}

        __builtin_unreachable();
}
