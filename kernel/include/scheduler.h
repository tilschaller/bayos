#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <interrupts.h>

void sched_init(void);

void add_process(void (*f)(void));
void delete_process(void);

cpu_status_t *schedule(cpu_status_t *context);

#endif // _SCHEDULER_H
