#include <tty.h>
#include <queue.h>
#include <framebuffer.h>
#include <fail.h>
#include <stdio.h>
#include <alloc.h>
#include <string.h>

queue_t *q = NULL;
static char *buf = NULL;
int pos;

static void process_keypress(char c) {
	/*
	 *	start a new line
	 * */
	if (c == '\n') {
		putchar(c);
		if (pos == 5 && !memcmp(buf, "clear", 5)) {
			fb_clear();
		}
		printf("> ");
		pos = 0;
	} else {
		putchar(c);
		// Write the char into the buffer
		buf[pos++] = c;
	}
}

void terminal(void) {
	q = queue_create(0x200, 1);
	buf = alloc(0x200);
	pos = 0;

	if (!q || !buf) {
		printf("Could not create terminal queue or buffer\n");
		hcf();
	}

	printf("> ");

	for (;;) {
		if (!queue_is_empty(q)) {
			char *c = queue_peek(q);
			dequeue(q);
			process_keypress(*c);
		}
	}

	__builtin_unreachable();
}
