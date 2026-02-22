#include <tty.h>
#include <queue.h>
#include <framebuffer.h>
#include <fail.h>
#include <stdio.h>
#include <alloc.h>
#include <string.h>
#include <tar.h>
#include <elf.h>
#include <memory.h>
#include <scheduler.h>

queue_t *q = NULL;
static char *buf = NULL;
static int pos;

// defined in main.c
extern tar_archive *initrd;

static void process_keypress(char c) {
	/*
	 *	start a new line
	 * */
	if (c == '\n') {
		putchar(c);
		if (pos == 5 && !memcmp(buf, "clear", 5)) {
			fb_clear();
			goto end;
		}
		// search for a process in initrd
		for (int i = 0; i < initrd->entry_count; i++) {
			if (!memcmp(buf, initrd->headers[i]->filename, pos - 1) && initrd->headers[i]->filename[pos] == '\0') {
				// write the program into the current address space
				elf_header *proc = (elf_header*)(initrd->headers[i]->file);
				elf_program *programs = (elf_program*)((uint8_t*)proc + proc->e_phoff);
				for (int i = 0; i < proc->e_phnum; i++) {
					if (programs[i].p_type == 1) {
						int pages = ALIGN(programs[i].p_memsz, 0x1000) / 0x1000;
						for (int j = 0; j < pages; j++) {
							map_to(allocate_frame(), programs[i].p_vaddr + j * 0x1000, FLAG_PRESENT | FLAG_WRITEABLE | FLAG_USER);
						}
						memcpy((void*)programs[i].p_vaddr, (uint8_t*)proc + programs[i].p_offset, programs[i].p_filesz);
					}
				}
				add_user_process(proc->e_entry);

				goto end;
			}
		}
		printf("Wrong command\n");
end:
		printf("> ");
		pos = 0;
	} else {
		putchar(c);
		// Write the char into the buffer
		buf[pos++] = c;
	}
}

void terminal(void) {
	q = queue_create(0x200, 1);
	buf = alloc(0x200);
	pos = 0;

	if (!q || !buf) {
		printf("Could not create terminal queue or buffer\n");
		hcf();
	}

	printf("> ");

	for (;;) {
		if (!queue_is_empty(q)) {
			char *c = queue_peek(q);
			dequeue(q);
			process_keypress(*c);
		}
	}

	__builtin_unreachable();
}
