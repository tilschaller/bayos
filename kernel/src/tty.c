#include <tty.h>
#include <queue.h>
#include <framebuffer.h>
#include <fail.h>

queue_t *q = NULL;

void terminal(void) {
	q = queue_create(0x200, 1);

	if (!q) hcf();

	for (;;) {
		if (!queue_is_empty(q)) {
			char *c = queue_peek(q);
			dequeue(q);
			putchar(*c);
		}
	}

	__builtin_unreachable();
}
