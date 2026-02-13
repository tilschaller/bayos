#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <interrupts.h>
#include <types/types.h>

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

void sched_init(void);

void add_process(void (*f)(void));
void add_user_process(uintptr_t f);
void delete_process(process_t * prev, process_t *p);

cpu_status_t *schedule(cpu_status_t *context);

#endif // _SCHEDULER_H
