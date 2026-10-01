#ifndef HH_MATH_H
#define HH_MATH_H

/*
 * Plain-C math inlines shared by the HH effect classes (src/Effect2/hh_class_*.c).
 *
 * The header's name is ours. These helpers are inline-only, so they left no DWARF, and the
 * original header is unknown; hh_math_wrapper.c (the HH code's out-of-line math functions) only
 * hints that the HH code had a math header. The helper names are ours too. The bodies are the
 * ones the matched files use, identical in every file that has them.
 *
 * Float_Bits is also used outside the HH code, by model3_vu0_n.c and kari_lf_draw.c, with the
 * same body; they include this header for it. Vector_Zero is in hh_vector.h (see there why).
 */

#include "libc/math.h"

/** Returns radian wrapped into [-PI, PI]. */
static inline float Radian_Normalize(float radian) {
    float result;

    if (radian > 0.0f) {
        result = fmodf(radian, 6.2831855f);
        if (result > 3.1415927f) {
            result -= 6.2831855f;
        }
    } else {
        result = fmodf(radian, -6.2831855f);
        if (result < -3.1415927f) {
            result += 6.2831855f;
        }
    }
    return result;
}

/** Returns radian converted to degrees. */
static inline float Radian_To_Degree(float radian) {
    return radian * (180.0f / 3.1415927f);
}

/** Returns f's bit pattern (for the GS's floating-point register fields). */
static inline unsigned int Float_Bits(float f) {
    return *(unsigned int *)&f;
}

#endif
