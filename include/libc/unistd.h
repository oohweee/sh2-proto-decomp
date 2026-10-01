#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

/*
 * <unistd.h>: getopt, which the game's boot options (Multi_thr/boot/bootopt.c) use, from its C
 * library (linked as assembly, lib/getopt; no DWARF). POSIX signature without the consts,
 * as bootopt.c passes it.
 *
 * Provenance: the name is the binary's symbol, the prototype POSIX's (less the consts). No SDK or
 * C-library header was used.
 */

int getopt(int argc, char **argv, char *optstring);

#endif
