#include <stdio.h>
#include <framebuffer.h>

int32_t printf(const char *restrict format, ...) {
        int32_t char_cnt = 0;
        for(int i = 0; format[i] != '\0'; i++, char_cnt++) {
                putchar((uint8_t)format[i]);
        }
        return char_cnt;
}
