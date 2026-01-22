#ifndef _LOCK_H
#define _LOCK_H

typedef struct {
	char lock;
} spinlock_t;

void acquire(spinlock_t *lock);
void release(spinlock_t *lock);

#endif // _LOCK_H
