#ifndef _STDIO_H
#define _STDIO_H

#include <types/types.h>

/*print a string of characters to screen. char-combinations with %* and \* will
 * be replaced by the equvalent characters returns number of printed characters
 * and a negative value if error occur*/
int32_t printf(const char *restrict format, ...);

/*print the string s to screen, ending with a newline-character \n. return 
number of printed characters*/
int32_t puts(const char *s);

#endif /*_STDIO_H*/
