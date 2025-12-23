#include <stdio.h>
#include <framebuffer.h>

int32_t printf(const char *restrict format, ...) {
        int32_t char_cnt = 0;

        va_list ap;
        va_start(ap, format);

        for(; *format != '\0'; format++, char_cnt++) {
                
                /* format specifier with reference to:
                 * https://en.cppreference.com/w/c/io/fprintf */
                if(*format == '%') {
                        format++;
                        switch(*format) {
                        case 'h':
                                format++;
                                switch(*format) {
                                case 'd': case 'i':
                                        printi(S_SHORT, ap);
                                        break;
                                case 'o': /*implement dec to octal conv unsigned short*/
                                        break;
                                case 'x': case 'X': /*implement dec to hex conv unsigned short*/
                                        break;
                                case 'u':
                                        printi(U_SHORT, ap);
                                        break;
                                case 'n': /*print how much chars printed yet short*/
                                /*all cases for %h* until here, not %hh* */
                                case 'h':
                                        format++;
                                        switch(*format) {
                                        case 'd': case 'i':
                                                printi(S_CHAR, ap);
                                                break;
                                        case 'o': /*see above*/
                                                break;
                                        case 'x': case 'X': /*see above*/
                                                break;
                                        case 'u':
                                                printi(U_CHAR, ap);
                                                break;
                                        case 'n': /*see above*/
                                                break;
                                        default:
                                                break;
                                        }
                                default:
                                        break;
                                }

                                break;
                        case 'c':
                                putchar(va_arg(ap, int32_t));
                                break;
                        case 's':
                                putstr(va_arg(ap, const char *));
                                break;
                        case 'd': case 'i':
                                printi(S_INT, ap);
                                break;
                        case 'o': /*see above*/
                                break;
                        case 'x': case 'X': /*see above*/
                                break;
                        case 'u':
                                printi(U_INT, ap);
                                break;
                        case 'f': case 'F': /*no float provided yet*/
                                break;
                        case 'e': case 'E': /*no float provided yet*/
                                break;
                        case 'a': case 'A': /*no float provided yet*/
                                break;
                        case 'g': case 'G': /*no float provided yet*/
                                break;
                        case 'n': /*see above*/
                                break;
                        case 'p': /*print pointer, next TODO on my list*/
                                break;
                        case 'l':
                                format++;
                                switch(*format) {
                                case 'c': /*implementation not needed yet*/
                                        break;
                                case 's': /*implementation not needed yet*/
                                        break;
                                case 'd': case 'i':
                                        printi(S_LONG, ap);
                                        break;
                                case 'o': /*see above*/
                                        break;
                                case 'x': case 'X': /*see above*/
                                        break;
                                case 'u':
                                        printi(U_LONG, ap);
                                        break;
                                case 'f': case 'F': /*no float provided yet*/
                                        break;
                                case 'e': case 'E': /*no float provided yet*/
                                        break;
                                case 'a': case 'A': /*no float provided yet*/
                                        break;
                                case 'g': case 'G': /*no float provided yet*/
                                        break;
                                case 'n': /*see above*/
                                        break;
                                case 'l':
                                        format++;
                                        switch(*format) {
                                        case 'd': case 'i':
                                                printi(S_LLONG, ap);
                                                break;
                                        case 'o': /*see above*/
                                                break;
                                        case 'x': case 'X': /*see above*/
                                                break;
                                        case 'u':
                                                printi(U_LLONG, ap);
                                                break;
                                        case 'n': /*see above*/
                                                break;
                                        default:
                                                break;
                                        }
                                default:
                                        break;
                                }
                        case 'L':
                                format++;
                                switch(*format) {
                                case 'f': case 'F': /*no float provided yet*/
                                        break;
                                case 'e': case 'E': /*no float provided yet*/
                                        break;
                                case 'a': case 'A': /*no float provided yet*/
                                        break;
                                case 'g': case 'G': /*no float provided yet*/
                                        break;
                                }
                        }
                } else {
                        putchar((uint8_t)*format);
                        char_cnt++;
                }
        }
        return char_cnt;
}


/*not nice but rare...*/
int32_t printi(INT_TYPES t, va_list ap) {
        int8_t leading_z = 1, isNegative = 0, char_cnt = 0;
        int64_t val = 0;
        char digits[20] = {0}; /* Note: uint64_t cannot contain more than 20 
                                * digits*/

        switch(t) {     /*currently no testing if input is correct. rn hu and hd
                         * are the same*/
        case S_CHAR: case U_CHAR:
                val = (int8_t) va_arg(ap, int);
                break;
        case S_SHORT: case U_SHORT:
                val = (int16_t) va_arg(ap, int);
                break;
        case S_INT: case U_INT:
                val = (int32_t) va_arg(ap, int);
                break;
        case S_LONG: case U_LONG: case S_LLONG: case U_LLONG:
                val = (int64_t) va_arg(ap, int);
                break;
        }

        if(val < 0) {
                isNegative = 1;
                val = -val;
        }

        if(val == 0) {
                putchar('0');
                return 1;
        }

        for(int i = 0; i < 20; i++) {
                digits[i] = (val % 10) + '0';
                val /= 10;
        }

        if(isNegative) {
                putchar('-');
                ++char_cnt;
        }

        for(int i = 19; i >= 0; --i) {
                putchar(digits[i]);
                char_cnt++;
        }
        return char_cnt;
}
