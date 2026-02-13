#ifndef _STDIO_H
#define _STDIO_H

#include <types/types.h>
#include <num_sys.h>


/*print a string of characters to screen. char-combinations with %* and \* will
 * be replaced by the equvalent characters returns number of printed characters
 * and a negative value if error occur*/
int32_t printf(const char *restrict format, ...);

/*few functions to reduce printf-complexity*/
int32_t printi(INT_TYPES t, va_list ap); 	/* print integer from x with type t.
				       	 * see types/types.h -> INT_TYPES */

/*print the string s to screen, ending with a newline-character \n for puts, no
\n for putstr. return number of printed characters*/
int32_t putstr(const char *s);
int32_t puts(const char *s);

#endif /* _STDIO_H */
