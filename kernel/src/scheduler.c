#include <scheduler.h>
#include <interrupts.h>
#include <types/types.h>
#include <alloc.h>
#include <string.h>

typedef enum {
	READY,
	RUNNING,
	DEAD,
} status_t;

typedef struct process_t {
	status_t status;
	cpu_status_t *context;
	struct process_t *next;
} process_t;

process_t *process_list = NULL;
process_t *current_process = NULL;

static inline uint64_t get_rflags(void) {
	uint64_t rflags;

	asm volatile("pushfq; popq %0" : "=r"(rflags) : : "memory");

	return rflags;
}

void sched_init(void) {
	process_t *p = alloc(sizeof(process_t));
	p->next = NULL;
	p->status = RUNNING;
	process_list = p;
	current_process = p;
}

void add_process(void (*f)(void)) {
	process_t *p = alloc(sizeof(process_t));
	p->next = NULL;
	p->status = RUNNING;
	cpu_status_t *ctx = alloc(0x1000);
	memset(ctx, 0, sizeof(*ctx));
	ctx->iret_rip = (uintptr_t)f;
	ctx->iret_cs = 0x8;
	ctx->iret_flags = get_rflags();
	ctx->iret_rsp = (uintptr_t)((uint8_t*)ctx + 0x1000);
	ctx->iret_ss = 0x10;
	p->context = ctx;
	process_t *last = process_list;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = p;
}
void delete_process(void) {}

cpu_status_t *schedule(cpu_status_t *context) {
	current_process->context = context;

	if (current_process->next !=  NULL) {
		current_process = current_process->next;
	} else {
		current_process = process_list;
	}

	return current_process->context;
}
