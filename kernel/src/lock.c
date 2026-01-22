#include <lock.h>

// for now these functions shouldnt work with interrupts

void acquire(spinlock_t *lock) {
	asm volatile("cli");
	while (__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE));
}

extern int int_enabled;
void release(spinlock_t *lock) {
	if (int_enabled == 1)
		asm volatile("sti");
	__atomic_clear(&lock->lock, __ATOMIC_RELEASE);
}
