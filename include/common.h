#ifndef COMMON_H
#define COMMON_H

/*
 * Shared declarations for the Metrowerks MIPS C 2.4 build (EE, R5900).
 * Note: `long` is 64-bit on this target (8 bytes, like `long long`).
 */

typedef signed char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
typedef long s64;
typedef unsigned long u64;
typedef unsigned int u_long128 __attribute__((mode(TI)));
#include "eeregs.h"
typedef int long128 __attribute__((mode(TI)));

/*
 * 16-byte-aligned typedefs (Sony libvu0.h and the game's quadword union). The DWARF has no
 * typedefs, but these matter: MWCC gives an address-taken or spilled pointer to an aligned
 * typedef a 16-byte-aligned stack slot (a plain `union Q_WORDDATA *` gets a packed 4-byte
 * one). Use them where the original's stack layout shows it (docs/decomp-workflow.md).
 */
typedef float sceVu0FVECTOR[4] __attribute__((aligned(16)));
typedef float sceVu0FMATRIX[4][4] __attribute__((aligned(16)));
typedef int sceVu0IVECTOR[4] __attribute__((aligned(16)));
typedef union Q_WORDDATA Q_WORDDATA __attribute__((aligned(16)));

#ifndef NULL
#define NULL ((void *)0)
#endif

/* printf, for the asserts below. */
#include "libc/stdio.h"

#define SH_STRINGIFY2(x) #x
#define SH_STRINGIFY(x) SH_STRINGIFY2(x)

/* Number of elements of an array (a size_t, like the sizeof expression it stands for). */
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

/*
 * The game's asserts: print "<file>:<line>> assert:(<expr>)" and hang.
 * __FILE__ is the bare file name under MWCC, and the line number is baked into the
 * format string, so a matching assert has to sit on its original source line.
 *
 * Two variants exist in the binary and compile differently:
 * - assert():    a bare `if` (e.g. title.c); the hang loop is `L: nop; b L`.
 * - assert_dw(): wrapped in do { } while (0) (e.g. dbalocate.c); one extra nop
 *                before the hang loop.
 */
#define assert(x)                                                                       \
    if (!(x)) {                                                                         \
        printf(__FILE__ ":" SH_STRINGIFY(__LINE__) "> assert:(%s)\n", #x);              \
        while (1) {}                                                                    \
    }

#define assert_dw(x)                                                                    \
    do {                                                                                \
        if (!(x)) {                                                                     \
            printf(__FILE__ ":" SH_STRINGIFY(__LINE__) "> assert:(%s)\n", #x);          \
            while (1) {}                                                                \
        }                                                                               \
    } while (0)

/*
 * The other assert (Font, Fog, MC and the enemy files): an expression that calls fjAssert_
 * (Font/fj_man.c: print, then break) with the file and line when x is false. The macro's name
 * is inferred from the function's.
 */
#define fjAssert(x) ((x) ? 0 : fjAssert_(__FILE__, __LINE__, #x))

/*
 * Stand-in for a dead-stripped function that did software-double arithmetic. Once MWCC has
 * compiled double code, the rest of the file uses a2 rather than a0/a1 for temporaries (see
 * docs/decomp-workflow.md, "Software-double mode"). Some original files evidently had such a
 * function, which the linker stripped; nothing else of it survives. Put this where the
 * original's register use changes. tools/mwcc_fixup.py strips it
 * (config/stripped_functions.txt).
 */
#define STRIPPED_DOUBLE_CODE() \
    static double __stripped_double_code(double x) { return x * 2.5 + 1.0; }

#endif
