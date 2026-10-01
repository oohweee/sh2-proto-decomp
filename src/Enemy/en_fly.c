/*
 * en_fly.c: a single fly that drifts up and down around its start height and wanders,
 * steered with the enemy path code (enSetPath). Its state reuses an enemy work (fly.dp).
 * A buzzing fly: a sound source, not an enemy, used only by the TGS-trial stage (verified; docs/characters.md).
 */
#include "sh2.h"

/* Matching: local copies of asm_helpers.h/sh_vu0.h/fi_libvu0_inline.h helpers, renamed `*_local`;
 * with those headers included instead, flyMove's float constants load in another order. */
/* 16-byte vector copy through t7. */
inline void vcopy_local(void *s, void *d) {
    asm {
        lq t7, 0(s)
        sq t7, 0(d)
    }
}

/* sh_vu0.h */
static inline void _shAddVector_local(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vadd.xyzw vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

static inline void _shAddVectorXYZ_local(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vadd.xyz  vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/* libvu0: v0.xyz = v1.xyz, v0.w = 1 */
static inline void _sceVu0CopyVectorXYZ_local(float *v0, float *v1) {
    __asm__ __volatile__("
    lqc2    vf4, 0x0(%1)
    vmove.w vf4, vf0
    sqc2    vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}

#define FLY_POS ((float *)&fly.scp.pos)
#define FLY_WORK ((float *)&fly.dp.scu) /* the fly's own state, kept in the per-kind union */

struct EN_FLY_DATA fly;

/** Places the fly at pos with a random heading and sets its height limits (pos.y +/- 500).
 * @param pos start position */
void flyInit(float *pos) {
    shQzero(&fly, sizeof(fly));
    fly.dp.scp = &fly.scp;
    _sceVu0CopyVectorXYZ_local(FLY_POS, pos);
    fly.scp.rot.y = 2.0f * (3.1415927f * (shRandF() - 0.5f));
    fly.dp.flag = 0x100;
    enInitPath(&fly.dp.path, fly.scp.rot.y);
    FLY_WORK[0] = pos[1];
    FLY_WORK[1] = 0.0f;
    FLY_WORK[2] = pos[1] - 500.0f;
    FLY_WORK[3] = 500.0f + pos[1];
}

/* FAKEMATCH: the (float) cast of a float sum is there only for codegen: it loads the angle before 500.0f for
 * shSinCosV_Scale. Matching: the stripped stand-in sets the order of the first shSway1f call's
 * constants (0.1f before -25.0f). */
static float __stripped_float_code_100(float x0, float x1, float x2, float x3) { x3 += 697.0f * x3; return x0; } /* fitted, not recovered: 1 constant */
/** Moves the fly for one frame: random vertical drift within its limits, a swaying heading,
 * path steering around obstacles, then forward movement. */
void flyMove(void) {
    float vec[4];

    FLY_WORK[1] = 0.95f * (FLY_WORK[1] + shSway1f(-25.0f, 0.1f) - 0.01f * (FLY_POS[1] - FLY_WORK[0]));
    FLY_POS[1] += FLY_WORK[1];
    if (FLY_POS[1] < FLY_WORK[2]) {
        FLY_POS[1] = FLY_WORK[2];
    }
    if (FLY_POS[1] > FLY_WORK[3]) {
        FLY_POS[1] = FLY_WORK[3];
    }
    FLY_WORK[5] = 0.95f * (FLY_WORK[5] + shSway1f(-0.034906585f, 0.1f) - 0.01f * FLY_WORK[4]);
    FLY_WORK[4] = 0.95f * (FLY_WORK[4] + FLY_WORK[5]);
    shSinCosV_Scale(vec, (float)(fly.scp.rot.y + FLY_WORK[4]), 500.0f);
    _shAddVector_local(vec, FLY_POS, vec);
    if (enSetPath(&fly.dp, vec, FLY_POS)) {
        fly.dp.path.angle = fly.dp.path.markangle;
        FLY_WORK[4] = FLY_WORK[5] = 0.0f;
    }
    enMoveAngle(&fly.dp.path, 0.2617994f);
    shSinCosV_Scale(vec, fly.scp.rot.y = fly.dp.path.angle, 1500.0f * shGetDT());
    _shAddVectorXYZ_local(FLY_POS, FLY_POS, vec);
}

/** Copies the fly's position to pos.
 * @param pos result */
void flyGetPos(float *pos) {
    vcopy_local(FLY_POS, pos);
}
