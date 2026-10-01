#ifndef LIBC_STDDEF_H
#define LIBC_STDDEF_H

/*
 * <stddef.h> for the game's C library (see libc/string.h). size_t is unsigned int: MWCC's type
 * for `sizeof` on the EE (32-bit pointers). size_t and NULL are the C standard's. No SDK or
 * C-library header was used.
 */

typedef unsigned int size_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif
