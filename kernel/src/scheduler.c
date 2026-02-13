#include <scheduler.h>
#include <interrupts.h>
#include <types/types.h>
#include <alloc.h>
#include <string.h>
#include <lock.h>
#include <stdio.h>
#include <memory.h>
#include <fail.h>

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

static spinlock_t proc_lock = {0};

void add_process(void (*f)(void)) {
	acquire(&proc_lock);
	process_t *p = alloc(sizeof(process_t));
	p->next = NULL;
	p->status = READY;
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

	release(&proc_lock);
}

/*
 * this function is called from a function in kernel space
 * or from a syscall (e.g. the current_process is the running process)
 * so we mark this process as dead, so it gets deleted next time
 * and return control to the kernel (e.g) the frist entry in the process list
 * */
void exit(void) {
	current_process->status = DEAD;

	schedule(current_process->context);

	asm volatile("push $_after_timer_handler");
	asm volatile("ret");
}


void add_user_process(uintptr_t f) {
	acquire(&proc_lock);
	process_t *p = alloc(sizeof(process_t));
	p->next = NULL;
	p->status = READY;
	map_to(allocate_frame(), 0x200000, FLAG_PRESENT | FLAG_WRITEABLE | FLAG_USER);
	cpu_status_t *ctx = (void*)0x200000;
	memset(ctx, 0, sizeof(*ctx));
	ctx->iret_rip = f;
	ctx->iret_cs = 0x18 | 3;
	ctx->iret_flags = get_rflags();
	ctx->iret_rsp = (uintptr_t)((uint8_t*)ctx + 0x1000);
	ctx->iret_ss = 0x20 | 3;
	p->context = ctx;
	process_t *last = process_list;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = p;

	release(&proc_lock);
}

void delete_process(process_t *prev, process_t *p) {
	if (p->context > HIGHER_HALF) {
		// TODO: free stack of process
		// free(p->context);
	} else {
		/*
		 *	TODO: free the resources of the user process
		 * */
	}
	prev->next = p->next;
	free(p);
}

cpu_status_t *schedule(cpu_status_t *context) {
	current_process->context = context;
	if (current_process->status != DEAD)
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
			current_process = prev_process;
		} else {
			current_process->status = RUNNING;
			break;
		}
	}

	return current_process->context;
}
