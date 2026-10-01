/*
 * vu0_func.c: shOuterXZ, a side-of-line test in the XZ plane (the unit's only function).
 */
#include "sh2.h"

/**
 * Returns the 2D cross product (z component) of (x - x0, z - z0) and (x1 - x0, z1 - z0) in the
 * XZ plane: > 0 when (x, z) is on one side of the line through (x0, z0)-(x1, z1), < 0 on the
 * other.
 *
 * Written as FPU asm: the temporaries sit in $f1-$f4 in order and nothing is fused into
 * madd/msub, which MWCC does for the same C.
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
float shOuterXZ(float x, float z, float x0, float z0, float x1, float z1) {
    float ret;

    asm {
        sub.s   $f1, x, x0
        sub.s   $f2, z1, z0
        sub.s   $f3, z, z0
        sub.s   $f4, x1, x0
        mul.s   $f1, $f1, $f2
        mul.s   $f3, $f3, $f4
        sub.s   ret, $f1, $f3
    }
    return ret;
}
