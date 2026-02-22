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
#include <tty.h>
#include <tar.h>

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memory_map_request = {
	.id = LIMINE_MEMMAP_REQUEST_ID,
	.revision = 0,
};

__attribute__((used, section(".limine_request")))
static volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST_ID,
	.revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
	.id = LIMINE_MODULE_REQUEST_ID,
	.revision = 0,
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

tar_archive *initrd;

__attribute__((noreturn))
void _start(void) {
	if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) hcf();
	if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) hcf();

	fb_init(framebuffer_request.response->framebuffers[0]);

	printf("#########################\n");
	printf("#    Welcome to Bayos   #\n");
	printf("#########################\n");

	if (memory_map_request.response == NULL) hcf();
	if (rsdp_request.response == NULL) hcf();

	int_init();
	gdt_init();

	mem_init(
	        memory_map_request.response
	);

	heap_init();

	acpi_init(rsdp_request.response->address);

	sched_init();

	add_process(terminal);

	if (module_request.response == NULL || module_request.response->module_count == 0) {
		printf("ERROR: no modules loaded, will not be able to lauch processes\n");
		hcf();
	}

	initrd = tar_create((uint64_t)module_request.response->modules[0]->address);
	if (!initrd) {
		printf("ERROR: failed to create initramfs\n");
		hcf();
	}
	printf("LOG: %llx file(s) in initrd\n", initrd->entry_count);

	enable_interrupts();

	while (1) {
		asm volatile("hlt");
	}

	__builtin_unreachable();
}
