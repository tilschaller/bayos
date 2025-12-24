#ifndef _STRING_H
#define _STRING_H

#include <types/types.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

int32_t toupper(int32_t c);
int32_t tolower(int32_t c);
int32_t isalpha(int32_t c);

#endif /* _STRING_H */