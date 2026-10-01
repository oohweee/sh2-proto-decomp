#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

/*
 * <stdio.h>: the formatted-output functions the game calls from its C library (linked as
 * assembly, no DWARF). ISO C signatures; only what the game uses. common.h includes this (for
 * printf in its assert macros), so every file has it.
 *
 * Provenance: the function names are the binary's symbols, the prototypes are the C standard's.
 * No SDK or C-library header was used.
 */

#include "libc/stdarg.h"

int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int sscanf(const char *s, const char *format, ...);
int vsprintf(char *buf, const char *fmt, va_list ap);

#endif
