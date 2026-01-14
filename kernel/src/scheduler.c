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
