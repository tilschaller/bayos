#include <scheduler.h>
#include <interrupts.h>
#include <types/types.h>
#include <alloc.h>
#include <string.h>
#include <lock.h>

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

spinlock_t add_lock = {0};
spinlock_t del_lock = {0};

void add_process(void (*f)(void)) {
	acquire(&add_lock);

	process_t *p = alloc(sizeof(process_t));
	p->next = NULL;
	p->status = READY;
	cpu_status_t *ctx = alloc(0x1000);
	memset(ctx, 0, sizeof(*ctx));
	ctx->iret_rip = (uintptr_t)f;
	ctx->iret_cs = 0x8;
	ctx->iret_flags = get_rflags();
	ctx->iret_rsp = (uintptr_t)((uint8_t*)ctx + 0x4000);
	ctx->iret_ss = 0x10;
	p->context = ctx;
	process_t *last = process_list;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = p;

	release(&add_lock);
}
void delete_process(process_t *prev, process_t *p) {
	acquire(&del_lock);
	free(p->context);
	free(p);
	prev->next = p->next;
	free(&del_lock);
}

cpu_status_t *schedule(cpu_status_t *context) {
	current_process->context = context;
	current_process->status = READY;

	while (1) {
		process_t *prev_process = current_process;
		if (current_process->next != NULL) {
			current_process = current_process->next;
		} else {
			current_process = process_list;
		}

		if (current_process != NULL && current_process->status == DEAD) {
			delete_process(prev_process, current_process);
		} else {
			current_process->status = RUNNING;
			break;
		}
	}

	return current_process->context;
}
