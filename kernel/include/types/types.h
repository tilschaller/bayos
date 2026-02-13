/* kernel/core/types/types.h contains typedefs for datatype in thr form of
 * [signed/unsigned][type][bits]_t
 *
 */

#include <stdarg.h>

#ifndef _TYPES_H
#define _TYPES_H

typedef signed char             int8_t;
typedef unsigned char           uint8_t;

typedef signed short int        int16_t;
typedef unsigned short int      uint16_t;

typedef signed int              int32_t;
typedef unsigned int            uint32_t;

typedef signed long long int    int64_t;
typedef unsigned long long int  uint64_t;

/*Note: when subtracting two pointers on 64bit, the difference should not be
 * in range of int64_t*/
typedef signed long long int    ptrdiff_t;
typedef unsigned long long int  uintptr_t;

/*I would say nothing should be bigger then 2^64 bytes ;)*/
typedef unsigned long int  size_t;

typedef enum {
	S_CHAR,
	U_CHAR,
	S_SHORT,
	U_SHORT,
	S_INT,
	U_INT,
	S_LONG,
	U_LONG,
	S_LLONG,
	U_LLONG
} INT_TYPES;

#define NULL (void*)0

#endif /* _TYPES_H */
