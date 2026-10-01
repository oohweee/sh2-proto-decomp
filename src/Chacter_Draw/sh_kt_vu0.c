/*
 * Small VU0 macro-mode matrix/vector routines used by the character renderer (Chacter_Draw).
 * Each body is a GCC-style extended asm block on its arguments.
 */
#include "sh2.h"

/**
 * Transforms a point: v0 = m0 * (v1.xyz, 1).
 * @param v0 result (16-byte aligned)
 * @param m0 4x4 matrix
 * @param v1 source vector; its w is ignored
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void ktVu0ApplyMatrixXYZ1(float *v0, float (*m0)[4], float *v1) {
    __asm__ __volatile__("
    lqc2         vf4, 0x0(%1)
    lqc2         vf5, 0x10(%1)
    lqc2         vf6, 0x20(%1)
    lqc2         vf7, 0x30(%1)
    lqc2         vf8, 0x0(%2)
    vmulax.xyzw  ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw  vf12, vf7, vf0w
    sqc2         vf12, 0x0(%0)
    " : : "r"(v0), "r"(m0), "r"(v1));
}

/**
 * Transforms a direction (no translation): v0 = m0 * (v1.xyz, 0).
 * @param v0 result (16-byte aligned)
 * @param m0 4x4 matrix
 * @param v1 source vector; its w is ignored
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void ktVu0ApplyMatrixXYZ0(float *v0, float (*m0)[4], float *v1) {
    __asm__ __volatile__("
    lqc2         vf4, 0x0(%1)
    lqc2         vf5, 0x10(%1)
    lqc2         vf6, 0x20(%1)
    lqc2         vf8, 0x0(%2)
    vmulax.xyzw  ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddz.xyzw  vf12, vf6, vf8z
    sqc2         vf12, 0x0(%0)
    " : : "r"(v0), "r"(m0), "r"(v1));
}

/**
 * Converts the xyz of a float vector to 12.4 fixed point (vftoi4); v0.w gets vf5's stale w.
 * @param v0 result (16-byte aligned)
 * @param v1 source vector
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void ktVu0FTOI4VectorXYZ(int *v0, float *v1) {
    __asm__ __volatile__("
    lqc2         vf4, 0x0(%1)
    vftoi4.xyz   vf5, vf4
    sqc2         vf5, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}
