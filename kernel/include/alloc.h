#ifndef _ALLOC_H
#define _ALLOC_H

#include <types/types.h>

void heap_init();
void *alloc(size_t size);
void free(void *ptr);

#endif /* _ALLOC_H */