#ifndef _NUM_SYS_H
#define _NUM_SYS_H

#include <types/types.h>

char *dec_to_hex(uint64_t in);
char *dec_to_oct(uint64_t in);

void lhex(char hex[19]);
void uhex(char hex[19]);

#endif /* _NUM_SYS_H */