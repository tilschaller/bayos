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
#include <elf.h>

// test process that prints second process and deletes itself
void wait(void) {
	printf("Second process\n");

	// end process
	asm volatile("mov $2, %rax");
	asm volatile("int $0x80");

	__builtin_unreachable();
}

typedef struct {
	uint8_t length;
	uint8_t ext_attr;
	uint32_t lba_le;
	uint32_t lba_be;
	uint32_t length_le;
	uint32_t length_be;
	char date_time[7];
	uint8_t file_flags;
	uint8_t dk[2];
	uint16_t vsn_le;
	uint16_t vsn_be;
	uint8_t name_length;
	char name[];
} __attribute__((packed)) iso_dir;

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

	uint32_t *root_dir_lba = (uint32_t*)(read_buf + 158);
	read_cdrom(CDROM_BASE_PORT, false, *root_dir_lba, 1, (uint16_t*)read_buf);
	// read_buf now contains the entries of the root directory
	// find the shell file in the iso
	iso_dir *file = (iso_dir*)read_buf;
	bool found_file = false;
	for (;;) {
		if (!file->length)
			break;
		if (!memcmp(file->name, "SHELL.;", 7)) {
			found_file = true;
			break;
		}
		file = (iso_dir*)((uint8_t*)file + file->length);
	}
	if (!found_file) {
		printf("Could not find test file in iso\n");
		hcf();
	}
	elf_header *shell = alloc(file->length * 2048);
	if (!shell) {
		printf("Could not allocate space for shell file\n");
		hcf();
	}
	if (read_cdrom(CDROM_BASE_PORT, false, file->lba_le, file->length, (uint16_t*)shell)) {
		printf("Could not read shell file from cdrom\n");
		hcf();
	}
	free(read_buf);
	// create the shell process we just use the kernel address space, since we dont really need a seperate one
	// the kernel will be in all address spaces anyway and the lower part of this one is completely free
	elf_program *programs = (elf_program*)((uint8_t*)shell + shell->e_phoff);
	for (int i = 0; i < shell->e_phnum; i++) {
		if (programs[i].p_type == 1 /* PT_LOAD type */) {
			int pages = ALIGN(programs[i].p_memsz, 0x1000) / 0x1000;
			for (int j = 0; j < pages; j++) {
				map_to(allocate_frame(), programs[i].p_vaddr + j * 0x1000, FLAG_PRESENT | FLAG_WRITEABLE | FLAG_USER);
			}
			memcpy((void*)programs[i].p_vaddr, (uint8_t*)shell + programs[i].p_offset, programs[i].p_filesz);
		}
	}

	add_user_process(shell->e_entry);
	free(shell); 

	while (1) {
		asm volatile("hlt");
	}

        __builtin_unreachable();
}
