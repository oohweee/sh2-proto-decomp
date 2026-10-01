#ifndef LIBC_STDLIB_H
#define LIBC_STDLIB_H

/*
 * <stdlib.h>: the functions the game calls from its C library (linked as assembly, no DWARF).
 * ISO C signatures, except qsort's comparison function: the game's comparators take `void *`
 * (their DWARF, light_n.c), so the parameter does too.
 *
 * Provenance: the function names are the binary's symbols, the prototypes are the C standard's
 * (but for qsort's comparator, above). No SDK or C-library header was used.
 */

#include "libc/stddef.h"

int abs(int x);
void qsort(void *base, size_t n, size_t size, int (*cmp)(void *, void *));
int rand(void);
void srand(unsigned int seed);

#endif
