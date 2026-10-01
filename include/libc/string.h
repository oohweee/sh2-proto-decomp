#ifndef LIBC_STRING_H
#define LIBC_STRING_H

/*
 * <string.h>: the string and memory functions the game calls from its C library (linked as
 * assembly, lib/memset, lib/strcmp... in config/main.yaml; no DWARF). ISO C signatures, and
 * bzero's BSD one; only what the game uses.
 *
 * Some files call memset or memcpy without a prototype, as the original did (their arguments are
 * passed unconverted); they declare it `void *memset();` themselves and must not include this.
 *
 * Provenance: the function names are the binary's symbols, the prototypes are the C standard's
 * (BSD's for bzero). No SDK or C-library header was used.
 */

#include "libc/stddef.h"

void bzero(void *s, int n);
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
char *strcat(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dst, const char *src);
size_t strlen(const char *str);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dst, const char *src, size_t n);

#endif
