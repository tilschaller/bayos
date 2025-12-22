#include <stdio.h>
#include <framebuffer.h>

int32_t printf(const char *restrict format, ...) {
        int32_t char_cnt = 0;
        for(int i = 0; format[i] != '\0'; i++, char_cnt++) {
                if(format[i] == '\\') {
                        i++;
                        if(format[i] == '\0') break;

                        switch(format[i]) {
                        case 'n':
                                putchar('\n');
                                char_cnt++;
                                break;
                        case 'r':
                                putchar('\r');
                                char_cnt++;
                                break;
                        case 'b':
                                putchar('\b');
                                char_cnt++;
                                break;
                        case 't':
                                putchar('\t');
                                char_cnt++;
                                break;
                        default:
                                putchar('\\');
                                putchar(format[i]);
                                char_cnt += 2;
                                break;
                        }
                } else {
                        putchar((uint8_t)format[i]);
                        char_cnt++;
                }
        }
        return char_cnt;
}
