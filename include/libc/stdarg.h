#ifndef LIBC_STDARG_H
#define LIBC_STDARG_H

/*
 * <stdarg.h> for MWCC on the EE. The game's variadic functions (dbfntprint.c, verbose.c,
 * result.c) keep their argument pointer in a `char *` (the DWARF's type), so va_list is char *.
 *
 * va_start is reconstructed from their code (MWCC's own header isn't available): under the EE
 * EABI, the named integer arguments use __builtin_args_info(2) of the 8 argument registers.
 * va_list and va_start are the C standard's names. No SDK or C-library header was used.
 */

typedef char *va_list;

#define va_start(ap, parm) \
    (ap = (char *)__builtin_next_arg(parm) - ((__builtin_args_info(2) >= 8) ? 0 : (8 - __builtin_args_info(2)) * 8))

#endif
