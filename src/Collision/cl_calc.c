/*
 * Plane equation of a collision polygon.
 */

#include "sh2.h"

/**
 * Computes the plane equation (normal xyz, distance w) of @p pl's first three points into
 * @p pparam: v.xyz = (p1 - p0) x (p2 - p1), v.w = -dot(v.xyz, p0).
 * Original asm: a GCC-style statement in the body. The original's line table has the load of
 * pl->p on the statement's first line, then an entry per instruction, and the function's return
 * on a later closing brace.
 */
void clCalcPlaneEquation(struct _CL_HITPOLY_PLANE *pl, float *pparam) {
    __asm__ __volatile__("
    lqc2        vf4, 0x0(%0)
    lqc2        vf5, 0x10(%0)
    lqc2        vf6, 0x20(%0)
    vsub.xyzw   vf7, vf5, vf4
    vsub.xyzw   vf5, vf6, vf5
    vsub.w      vf6, vf6, vf6
    vopmula.xyz ACC, vf5, vf7
    vopmsub.xyz vf6, vf7, vf5
    vmul.xyz    vf4, vf6, vf4
    vaddy.x     vf4, vf4, vf4y
    vaddz.x     vf4, vf4, vf4z
    vsub.w      vf6, vf6, vf6
    vsubx.w     vf6, vf6, vf4x
    sqc2        vf6, 0x0(%1)
    " : : "r"(pl->p), "r"(pparam));
}
