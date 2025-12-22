#include <stdio.h>
#include <framebuffer.h>

int32_t puts(const char *s) {
	int32_t char_cnt = 0;

	for(int i = 0; s[i] != '\0'; i++, char_cnt++) {
		putchar(s[i]);
	}
	putchar('\n');
	return char_cnt;
}