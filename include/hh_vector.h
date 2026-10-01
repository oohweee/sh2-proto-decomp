#ifndef HH_VECTOR_H
#define HH_VECTOR_H

/*
 * Vector_Zero, shared by the HH effect classes that clear vectors (src/Effect2/hh_class_water_*.c,
 * hh_class_glass_piece_00.c).
 *
 * The header's name and the helper's name are ours (inline-only, no DWARF; the original header
 * is unknown). It is kept out of hh_math.h because of its initialized local array: MWCC emits
 * the array's zero-initializer template into .bss when it compiles the inline definition, even
 * in a file that never calls it, which would add 0x10 bytes of .bss to every file that includes
 * the header. Only the files that call Vector_Zero include this one.
 */

#include "sdk/libvu0.h"

/** v = (0, 0, 0, 0), copied from a local array with sceVu0CopyVector. */
static inline void Vector_Zero(float *v) {
    float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    sceVu0CopyVector(v, zero);
}

#endif
