#include <string.h>
#include <types/types.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
        uint8_t *restrict pdest = (uint8_t *restrict)dest;
        const uint8_t *restrict psrc = (const uint8_t *restrict)src;

        for (size_t i = 0; i < n; i++) {
                pdest[i] = psrc[i];
        }

        return dest;
}

void *memset(void *s, int c, size_t n) {
        uint8_t *p = (uint8_t *)s;

        for (size_t i = 0; i < n; i++) {
                p[i] = (uint8_t)c;
        }

        return s;
}

void *memmove(void *dest, const void *src, size_t n) {
        uint8_t *pdest = (uint8_t *)dest;
        const uint8_t *psrc = (const uint8_t *)src;

        if (src > dest) {
                for (size_t i = 0; i < n; i++) {
                        pdest[i] = psrc[i];
                }
        } else if (src < dest) {
                for (size_t i = n; i > 0; i--) {
                        pdest[i-1] = psrc[i-1];
                }
        }

        return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
        const uint8_t *p1 = (const uint8_t *)s1;
        const uint8_t *p2 = (const uint8_t *)s2;

        for (size_t i = 0; i < n; i++) {
                if (p1[i] != p2[i]) {
                        return p1[i] < p2[i] ? -1 : 1;
                }
        }

        return 0;
}

int32_t tolower(int32_t c) {
        if((c >= 'A') && (c <= 'Z')) {
                return (c - 'A' + 'a');
        }

        return c;
}

int32_t toupper(int32_t c) {
        if((c >= 'a') && (c <= 'z')) {
                return (c - 'a' + 'A');
        }

        return c;
}

int32_t isalpha(int32_t c) {
        if(     ((c >= 'a') && (c <= 'z')) ||
                ((c >= 'A') && (c <= 'Z'))      ) {
                return 1;
        } else {
                return 0;
        }
}