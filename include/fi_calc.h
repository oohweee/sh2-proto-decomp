#ifndef FI_CALC_H
#define FI_CALC_H

/*
 * Vector helpers (original: src\Chacter\fi_calc.h).
 *
 * Known from the DWARF: the out-of-line copy of ktVectorNormal (lens_flare.c) sits in a compile
 * unit named after this header, lines 197-199, with local (static) binding. Its body is the
 * copy's: a call of fi_libvu0_inline.h's _sceVu0Normalize, so that header came first. The rest of
 * the original header's content is unknown.
 */

#include "sh2.h"
#include "fi_libvu0_inline.h"

/** dest.xyz = src.xyz / |src.xyz|; dest.w = 0. */
/* Matching: #line puts the definition on its original line (the line table of the copy). */
#line 197
static inline void ktVectorNormal(struct FVEC *dest, struct FVEC *src) {
    _sceVu0Normalize((float *)dest, (float *)src);
}

#endif
