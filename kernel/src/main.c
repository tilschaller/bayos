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
#include <cdrom.h>
#include <string.h>

void wait(void) {
	printf("Second process\n");
	while (1) {
		asm volatile("hlt");
	}

	__builtin_unreachable();
}


char *msg = "Hello from syscall\n";

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

	uint8_t *read_buf = alloc(2048);
	if (!read_buf) {
		printf("Error: could not alloc read_buf\n");
		hcf();
	}
	read_cdrom(CDROM_BASE_PORT, false, 16, 1, (uint16_t*)read_buf);
	if (memcmp(&read_buf[1], "CD001", 5)) {
		printf("Error: could not find pmi of iso on cdrom\n");
		hcf();
	}

	// do a write syscall
	asm volatile("mov %0, %%rbx" : : "r"(msg));
	asm volatile("mov $19, %rcx");
	asm volatile("mov $4, %rax");
	asm volatile("int $0x80");

	while (1) {
		asm volatile("hlt");
	}

        __builtin_unreachable();
}
