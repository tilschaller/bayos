#ifndef  _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <types/types.h>
#include <limine.h>

typedef struct {
	struct limine_framebuffer *info;
	uint64_t x_pos;
	uint64_t y_pos;
	void *fb;
} framebuffer;

void fb_init(struct limine_framebuffer *info);
int putchar(int c);
void fb_clear(void);

#endif /* _FRAMEBUFFER_H */
