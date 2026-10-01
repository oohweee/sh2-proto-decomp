/*
 * sh_vu0.c: hand-scheduled FPU/VU0 math (angles, sin/cos, atan, rotation
 * matrices, random numbers) and a few helpers. Most functions are whole-function
 * asm with filled delay slots; the DWARF shows them as (void), so their signatures
 * (read from the registers they use) come from config/prototype_overrides.txt.
 *
 * The note above each asm function gives the evidence that the original wrote it as
 * asm: in the original's line table a C function's return is on its closing brace,
 * lines after its last statement, and MWCC -O2 never fills a return's delay slot
 * (docs/toolchain.md).
 *
 * sincosdata/atandata are small constant tables addressed gp-relative.
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"

extern unsigned int atandata[12] __attribute__((section(".sdata")));
extern unsigned int sincosdata[8] __attribute__((section(".sdata")));

static void sh_mulmatrix(void);

static int rand_seed = 1;
static int rand_seed_stack;

/** angle (f12) wrapped into [-pi, pi] */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 84). */
asm float shAngleRegulate(float angle) {
    .set noreorder
    la           t7, sincosdata
    abs.s        $f8, $f12
    lwc1         $f9, 0x14(t7)
    mov.s        $f0, $f12
    c.lt.s       $f8, $f9
    bc1t         @2
    sub.s        $f11, $f11, $f11
    add.s        $f8, $f8, $f9
    lwc1         $f10, 0x1C(t7)
    mul.s        $f8, $f8, $f10
    c.lt.s       $f12, $f11
    bc1tl        @1
    neg.s        $f8, $f8
@1:
    cvt.w.s      $f8, $f8
    add.s        $f9, $f9, $f9
    cvt.s.w      $f8, $f8
    mul.s        $f8, $f8, $f9
    sub.s        $f0, $f0, $f8
@2:
    jr           ra
    nop
}

/** atan2 of v[0] (y) and v[2] (x) */
/* Original asm: the return (jr t6) is on its own line right after the code in the line table (line
 * 113); it returns through t6, keeping ra there across its call; the return's delay slot is
 * filled. */
asm float shAtanV(float *v) {
    .set noreorder
    por          t6, zero, ra
    jal          shAtan_asm
    lqc2         vf4, 0x0(a0)
    qmfc2.ni     t7, vf4
    jr           t6
    mtc1         t7, $f0
}

/** Returns atan2(`y`, `x`) in radians (hand-written asm). */
/* Original asm: the return (jr t5) is on its own line right after the code in the line table (line
 * 135); it returns through t5, keeping ra there across its call; the return's delay slot is
 * filled. */
asm float shAtan2(float y, float x) {
    .set noreorder
    mfc1         t6, $f12
    mfc1         t7, $f13
    por          t5, zero, ra
    pcpyld       t7, t6, t7
    jal          shAtan_asm
    qmtc2.ni     t7, vf4
    qmfc2.ni     t7, vf4
    jr           t5
    mtc1         t7, $f0
}

/** atan2 for asm callers: vf4.x = atan2(vf4.x, vf4.z) (hand-written asm). */
/* in/out: vf4 (x = y, z = x) -> vf4x = atan2 */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 213); the return's delay slot is filled. */
asm void shAtan_asm(void) {
    .set noreorder
    addiu        sp, sp, -0x30
    sq           t0, 0x0(sp)
    qmfc2.ni     t0, vf4
    vabs.xz      vf4, vf4
    sq           t1, 0x10(sp)
    vsubz.x      vf5, vf4, vf4z
    sq           t2, 0x20(sp)
    vsub.y       vf4, vf4, vf4
    qmfc2.ni     t1, vf5
    sll          t1, t1, 0
    bgez         t1, @1
    xor          t2, t2, t2
    vmove.x      vf5, vf4
    vaddz.x      vf4, vf0, vf4z
    ori          t2, t2, 0x1
    vaddx.z      vf4, vf0, vf5x
@1:
    qmfc2.ni     t1, vf4
    beql         t1, zero, @4
    vsub.x       vf4, vf4, vf4
    la           t1, atandata
    vdiv         Q, vf4z, vf4x
    lqc2         vf5, 0x0(t1)
    lqc2         vf6, 0x20(t1)
    vwaitq
    vaddq.x      vf8, vf0, Q
    vmul.x       vf9, vf8, vf8
    vmulx.xyzw   vf10, vf5, vf9x
    vmulax.x     ACC, vf8, vf6x
    vmulx.yzw    vf11, vf10, vf9x
    vmaddax.x    ACC, vf8, vf10x
    vmulx.zw     vf10, vf11, vf9x
    vmadday.x    ACC, vf8, vf11y
    vmulx.w      vf11, vf10, vf9x
    vmaddaz.x    ACC, vf8, vf10z
    vmaddw.x     vf8, vf8, vf11w
    lqc2         vf5, 0x10(t1)
    vaddax.x     ACC, vf0, vf6x
    vmulx.yzw    vf11, vf5, vf9x
    vmaddax.x    ACC, vf9, vf5x
    vmulx.zw     vf10, vf11, vf9x
    vmadday.x    ACC, vf9, vf11y
    vmulx.w      vf11, vf10, vf9x
    vmaddaz.x    ACC, vf9, vf10z
    vmaddw.x     vf9, vf9, vf11w
    vdiv         Q, vf8x, vf9x
    pexcw        t1, t0
    vaddy.x      vf5, vf0, vf6y
    vaddz.x      vf6, vf0, vf6z
    vwaitq
    vaddq.x      vf4, vf0, Q
    beql         t2, zero, @2
    vsub.x       vf4, vf5, vf4
@2:
    sll          t2, t0, 0
    bltzl        t1, @3
    vsub.x       vf4, vf6, vf4
@3:
    bltzl        t2, @4
    vsub.x       vf4, vf0, vf4
@4:
    lq           t0, 0x0(sp)
    lq           t1, 0x10(sp)
    lq           t2, 0x20(sp)
    jr           ra
    addiu        sp, sp, 0x30
}

/* f12 = angle -> vf4 = (cos, sin, 0, 0) */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 285); the return's delay slot is filled. */
static asm void sh_ecossin(void) {
    .set noreorder
    la           t7, sincosdata
    lwc1         $f10, 0x10(t7)
    abs.s        $f9, $f12
    neg.s        $f11, $f10
    sub.s        $f8, $f10, $f9
    c.lt.s       $f8, $f11
    bc1f         @1
    lwc1         $f10, 0x1C(t7)
    mul.s        $f9, $f9, $f10
    lwc1         $f10, 0x14(t7)
    cvt.w.s      $f9, $f9
    add.s        $f13, $f10, $f10
    cvt.s.w      $f9, $f9
    mul.s        $f9, $f9, $f13
    neg.s        $f10, $f10
    add.s        $f8, $f8, $f9
    c.lt.s       $f8, $f11
    bc1tl        @1
    sub.s        $f8, $f10, $f8
@1:
    mul.s        $f9, $f8, $f8
    sub.s        $f11, $f11, $f11
    lwc1         $f13, 0xC(t7)
    mul.s        $f10, $f8, $f9
    adda.s       $f11, $f8
    mul.s        $f11, $f10, $f9
    madda.s      $f13, $f10
    lwc1         $f14, 0x8(t7)
    mul.s        $f10, $f11, $f9
    madda.s      $f14, $f11
    lwc1         $f13, 0x4(t7)
    lwc1         $f14, 0x0(t7)
    mul.s        $f11, $f10, $f9
    madda.s      $f13, $f10
    madd.s       $f8, $f14, $f11
    lui          t6, (0x3F800000 >> 16)
    mul.s        $f9, $f8, $f8
    mtc1         t6, $f10
    mfc1         t6, $f8
    sub.s        $f9, $f10, $f9
    qmtc2.ni     t6, vf4
    sqrt.s       $f9, $f9
    lwc1         $f10, 0x18(t7)
    mfc1         t6, $f9
    mul.s        $f9, $f12, $f10
    qmtc2.ni     t6, vf5
    cvt.w.s      $f9, $f9
    sub.s        $f10, $f10, $f10
    mfc1         t6, $f9
    c.lt.s       $f12, $f10
    andi         t6, t6, 0x1
    bc1tl        @2
    xori         t6, t6, 0x1
@2:
    beql         t6, zero, @3
    vaddx.y      vf4, vf0, vf5x
    vsubx.y      vf4, vf0, vf5x
@3:
    jr           ra
    vsub.zw      vf4, vf4, vf4
}

/** Returns sin(`angle`) (hand-written asm). */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 340). */
asm float shSinF(float angle) {
    .set noreorder
    la           t7, sincosdata
    abs.s        $f0, $f12
    lwc1         $f10, 0x10(t7)
    c.lt.s       $f0, $f10
    bc1t         @1
    lwc1         $f9, 0x14(t7)
    add.s        $f8, $f0, $f9
    lwc1         $f13, 0x1C(t7)
    mul.s        $f8, $f8, $f13
    add.s        $f11, $f9, $f9
    cvt.w.s      $f8, $f8
    cvt.s.w      $f8, $f8
    mul.s        $f8, $f8, $f11
    sub.s        $f0, $f0, $f8
    c.lt.s       $f0, $f10
    bc1fl        @1
    sub.s        $f0, $f9, $f0
    neg.s        $f10, $f10
    neg.s        $f9, $f9
    c.lt.s       $f0, $f10
    bc1tl        @1
    sub.s        $f0, $f9, $f0
@1:
    mul.s        $f8, $f0, $f0
    sub.s        $f10, $f10, $f10
    lwc1         $f11, 0xC(t7)
    mul.s        $f9, $f0, $f8
    adda.s       $f10, $f0
    mul.s        $f10, $f9, $f8
    madda.s      $f11, $f9
    lwc1         $f13, 0x8(t7)
    mul.s        $f9, $f10, $f8
    madda.s      $f13, $f10
    lwc1         $f11, 0x4(t7)
    lwc1         $f13, 0x0(t7)
    mul.s        $f10, $f9, $f8
    madda.s      $f11, $f9
    sub.s        $f8, $f8, $f8
    madd.s       $f0, $f13, $f10
    c.lt.s       $f12, $f8
    bc1tl        @2
    neg.s        $f0, $f0
@2:
    jr           ra
    nop
}

/** Returns cos(`angle`) (hand-written asm). */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 386); the return's delay slot is filled. */
asm float shCosF(float angle) {
    .set noreorder
    la           t7, sincosdata
    lwc1         $f10, 0x10(t7)
    abs.s        $f8, $f12
    neg.s        $f9, $f10
    sub.s        $f0, $f10, $f8
    c.lt.s       $f0, $f9
    bc1f         @1
    lwc1         $f13, 0x1C(t7)
    mul.s        $f8, $f8, $f13
    lwc1         $f10, 0x14(t7)
    cvt.w.s      $f8, $f8
    add.s        $f11, $f10, $f10
    cvt.s.w      $f8, $f8
    mul.s        $f8, $f8, $f11
    neg.s        $f10, $f10
    add.s        $f0, $f0, $f8
    c.lt.s       $f0, $f9
    bc1tl        @1
    sub.s        $f0, $f10, $f0
@1:
    mul.s        $f8, $f0, $f0
    sub.s        $f10, $f10, $f10
    lwc1         $f11, 0xC(t7)
    mul.s        $f9, $f0, $f8
    adda.s       $f10, $f0
    mul.s        $f10, $f9, $f8
    madda.s      $f11, $f9
    lwc1         $f13, 0x8(t7)
    mul.s        $f9, $f10, $f8
    madda.s      $f13, $f10
    lwc1         $f11, 0x4(t7)
    lwc1         $f13, 0x0(t7)
    mul.s        $f10, $f9, $f8
    madda.s      $f11, $f9
    jr           ra
    madd.s       $f0, $f13, $f10
}

/** v (a0), angle (f12): v = (sin, 0, cos, 0) */
/* Original asm: the return (jr t5) is on its own line right after the code in the line table (line
 * 421); it returns through t5, keeping ra there across its call; the return's delay slot is
 * filled. */
asm void shSinCosV(float *v, float angle) {
    .set noreorder
    por          t5, zero, ra
    jal          sh_ecossin
    nop
    vaddx.z      vf4, vf0, vf4x
    vaddy.x      vf4, vf0, vf4y
    vsub.yw      vf4, vf0, vf0
    jr           t5
    sqc2         vf4, 0x0(a0)
}

/** v = (sin, 0, cos, 0) of `angle`, times `scale` (hand-written asm). */
/* v (a0), angle (f12), scale (f13) */
/* Original asm: the return (jr t5) is on its own line right after the code in the line table (line
 * 465); it returns through t5, keeping ra there across its call; the return's delay slot is
 * filled. */
asm void shSinCosV_Scale(float *v, float angle, float scale) {
    .set noreorder
    addiu        sp, sp, -0x10
    por          t5, zero, ra
    jal          sh_ecossin
    swc1         $f13, 0x0(sp)
    vaddx.z      vf4, vf0, vf4x
    lqc2         vf5, 0x0(sp)
    vaddy.x      vf4, vf0, vf4y
    addiu        sp, sp, 0x10
    vmulx.xz     vf4, vf4, vf5x
    vsub.yw      vf4, vf0, vf0
    jr           t5
    sqc2         vf4, 0x0(a0)
}

/** dst = src rotated by `angle` about the Y axis (hand-written asm). */
/* dst (a0), src (a1), angle (f12) */
/* Original asm: the return (jr t5) is on its own line right after the code in the line table (line
 * 488); it returns through t5, keeping ra there across its call; the return's delay slot is
 * filled. */
asm void shRotVectorY(float *dst, float *src, float angle) {
    .set noreorder
    por          t5, zero, ra
    jal          sh_ecossin
    lqc2         vf7, 0x0(a1)
    vadd.x       vf5, vf0, vf4
    vsuby.z      vf5, vf0, vf4y
    vaddy.x      vf6, vf0, vf4y
    vaddx.z      vf6, vf0, vf4x
    vmulax.xz    ACC, vf5, vf7x
    vmaddz.xz    vf7, vf6, vf7z
    jr           t5
    sqc2         vf7, 0x0(a0)
}

/** dst = src rotated by `angle` about the X axis (hand-written asm). */
/* dst (a0), src (a1), angle (f12) */
/* Original asm: the return (jr t5) is on its own line right after the code in the line table (line
 * 516); it returns through t5, keeping ra there across its call. */
asm void shRotMatrixX(float (*dst)[4], float (*src)[4], float angle) {
    .set noreorder
    por          t5, zero, ra
    jal          sh_ecossin
    vsub.xyzw    vf6, vf6, vf6
    vmove.xyzw   vf7, vf6
    vmove.xyzw   vf8, vf6
    vaddw.x      vf6, vf0, vf0w
    vmove.xyzw   vf9, vf0
    vaddy.z      vf7, vf0, vf4y
    vaddx.y      vf7, vf0, vf4x
    vsuby.y      vf8, vf0, vf4y
    jal          sh_mulmatrix
    vaddx.z      vf8, vf0, vf4x
    jr           t5
    nop
}

/** dst = src rotated by `angle` about the Y axis (hand-written asm). */
/* dst (a0), src (a1), angle (f12) */
/* Original asm: the return (jr t5) is on its own line right after the code in the line table (line
 * 544); it returns through t5, keeping ra there across its call. */
asm void shRotMatrixY(float (*dst)[4], float (*src)[4], float angle) {
    .set noreorder
    por          t5, zero, ra
    jal          sh_ecossin
    vsub.xyzw    vf6, vf6, vf6
    vmove.xyzw   vf7, vf6
    vmove.xyzw   vf8, vf6
    vaddw.y      vf7, vf0, vf0w
    vmove.xyzw   vf9, vf0
    vsuby.z      vf6, vf0, vf4y
    vaddx.x      vf6, vf0, vf4x
    vaddy.x      vf8, vf0, vf4y
    jal          sh_mulmatrix
    vaddx.z      vf8, vf0, vf4x
    jr           t5
    nop
}

/** dst = src rotated by `angle` about the Z axis (hand-written asm). */
/* dst (a0), src (a1), angle (f12) */
/* Original asm: the return (jr t5) is on its own line right after the code in the line table (line
 * 570); it returns through t5, keeping ra there across its call. */
asm void shRotMatrixZ(float (*dst)[4], float (*src)[4], float angle) {
    .set noreorder
    por          t5, zero, ra
    jal          sh_ecossin
    vsub.xyzw    vf6, vf6, vf6
    vmove.xyzw   vf7, vf6
    vmr32.xyzw   vf8, vf0
    vmove.xyzw   vf9, vf0
    vadd.xy      vf6, vf0, vf4
    vsuby.x      vf7, vf0, vf4y
    jal          sh_mulmatrix
    vaddx.y      vf7, vf0, vf4x
    jr           t5
    nop
}

/* a0 = (vf6..vf9) * a1 */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 608); the return's delay slot is filled. */
static asm void sh_mulmatrix(void) {
    .set noreorder
    lqc2         vf4, 0x0(a1)
    vmulax.xyzw  ACC, vf6, vf4x
    vmadday.xyzw ACC, vf7, vf4y
    vmaddaz.xyzw ACC, vf8, vf4z
    vmaddw.xyzw  vf4, vf9, vf4w
    lqc2         vf5, 0x10(a1)
    vmulax.xyzw  ACC, vf6, vf5x
    vmadday.xyzw ACC, vf7, vf5y
    vmaddaz.xyzw ACC, vf8, vf5z
    vmaddw.xyzw  vf5, vf9, vf5w
    sqc2         vf4, 0x0(a0)
    lqc2         vf4, 0x20(a1)
    vmulax.xyzw  ACC, vf6, vf4x
    vmadday.xyzw ACC, vf7, vf4y
    vmaddaz.xyzw ACC, vf8, vf4z
    vmaddw.xyzw  vf4, vf9, vf4w
    sqc2         vf5, 0x10(a0)
    lqc2         vf5, 0x30(a1)
    vmulax.xyzw  ACC, vf6, vf5x
    vmadday.xyzw ACC, vf7, vf5y
    vmaddaz.xyzw ACC, vf8, vf5z
    vmaddw.xyzw  vf5, vf9, vf5w
    sqc2         vf4, 0x20(a0)
    jr           ra
    sqc2         vf5, 0x30(a0)
}

/** Seeds the random number generator with `seed`. */
void shSrand(int seed) {
    rand_seed = seed;
}

/** Saves the current random seed (one level) and seeds the generator with `seed`. */
void shPushRandSeed(int seed) {
    rand_seed_stack = rand_seed;
    rand_seed = seed;
}

/** Restores the seed saved by shPushRandSeed(). Returns the seed it replaces. */
int shPopRandSeed(void) {
    int seed;

    seed = rand_seed;
    rand_seed = rand_seed_stack;
    return seed;
}

/** Returns the next random integer (linear congruential, hand-written asm). */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 674); the return's delay slot is filled. */
asm int shRandI(void) {
    .set noreorder
    la           t5, rand_seed
    lui          t6, (0x41C64E6D >> 16)
    lw           t7, 0x0(t5)
    ori          t6, t6, (0x41C64E6D & 0xFFFF)
    mult         t7, t7, t6
    addiu        t7, t7, 0x3039
    dsll32       t7, t7, 1
    dsrl32       t7, t7, 1
    sw           t7, 0x0(t5)
    jr           ra
    addu         v0, t7, zero
}

/** Returns a random float in [0, 1) (hand-written asm, through shRandF_asm). */
/* Original asm: the return (jr t4) is on its own line right after the code in the line table (line
 * 700); it returns through t4, keeping ra there across its call; the return's delay slot is
 * filled. */
asm float shRandF(void) {
    .set noreorder
    por          t4, zero, ra
    jal          shRandF_asm
    nop
    jr           t4
    mov.s        $f0, $f8
}

/** shRandF() for asm callers: the result in f8 (hand-written asm). */
/* -> f8 = [0, 1) */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 717); the return's delay slot is filled. */
asm void shRandF_asm(void) {
    .set noreorder
    la           t5, rand_seed
    lui          t6, (0x41C64E6D >> 16)
    lw           t7, 0x0(t5)
    ori          t6, t6, (0x41C64E6D & 0xFFFF)
    mult         t7, t7, t6
    addiu        t7, t7, 0x3039
    dsll32       t7, t7, 1
    dsrl32       t7, t7, 1
    sw           t7, 0x0(t5)
    lui          t6, (0x3F800000 >> 16)
    srl          t7, t7, 8
    or           t7, t7, t6
    mtc1         t6, $f9
    mtc1         t7, $f8
    jr           ra
    sub.s        $f8, $f8, $f9
}

/** v.xyz = a random vector in [-1, 1) per component, times `scale`; v.w = 0 (hand-written asm). */
/* v (a0), scale (f12) */
/* Original asm: the return (jr t2) is on its own line right after the code in the line table (line
 * 774); it returns through t2, keeping ra there across its call; the return's delay slot is
 * filled. */
asm void shRandV_Scale(float *v, float scale) {
    .set noreorder
    por          t2, zero, ra
    jal          shRandV_asm
    vsub.w       vf6, vf6, vf6
    vadd.xyz     vf4, vf4, vf4
    mfc1         t7, $f12
    vsubw.xyz    vf4, vf4, vf0w
    qmtc2.ni     t7, vf5
    vmulx.xyz    vf6, vf4, vf5x
    jr           t2
    sqc2         vf6, 0x0(a0)
}

/** A random vector for asm callers: vf4.xyz in [0, 1) (hand-written asm). */
/* -> vf4.xyz = [0, 1) */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 815); the return's delay slot is filled. */
asm void shRandV_asm(void) {
    .set noreorder
    la           t3, rand_seed
    lui          t4, (0x41C64E6D >> 16)
    lw           t5, 0x0(t3)
    ori          t4, t4, (0x41C64E6D & 0xFFFF)
    mult         t5, t5, t4
    addiu        t5, t5, 0x3039
    dsll32       t5, t5, 1
    dsrl32       t5, t5, 1
    mult         t6, t5, t4
    addiu        t6, t6, 0x3039
    dsll32       t6, t6, 1
    dsrl32       t6, t6, 1
    mult         t7, t6, t4
    lui          t4, (0x3F800000 >> 16)
    addiu        t7, t7, 0x3039
    srl          t5, t5, 8
    dsll32       t7, t7, 1
    srl          t6, t6, 8
    dsrl32       t7, t7, 1
    or           t5, t5, t4
    sw           t7, 0x0(t3)
    or           t6, t6, t4
    srl          t7, t7, 8
    qmtc2.ni     t5, vf4
    qmtc2.ni     t6, vf5
    or           t7, t7, t4
    vaddx.y      vf4, vf0, vf5x
    qmtc2.ni     t7, vf5
    vaddx.z      vf4, vf0, vf5x
    jr           ra
    vsubw.xyz    vf4, vf4, vf0w
}

/** Returns a random sway value from `min` and `max` (hand-written asm; the formula is in shSway1f_asm). */
/* Original asm: the return (jr t4) is on its own line right after the code in the line table (line
 * 837); it returns through t4, keeping ra there across its call; the return's delay slot is
 * filled. */
asm float shSway1f(float min, float max) {
    .set noreorder
    por          t4, zero, ra
    jal          shSway1f_asm
    nop
    jr           t4
    mov.s        $f0, $f4
}

/** shSway1f() for asm callers (hand-written asm). */
/* f12, f13 -> f4 */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 873); the return's delay slot is filled. */
asm void shSway1f_asm(void) {
    .set noreorder
    mtc1         zero, $f8
    mov.s        $f4, $f12
    c.le.s       $f12, $f8
    bc1f         @1
    nop
    la           t5, rand_seed
    lui          t6, (0x41C64E6D >> 16)
    lw           t7, 0x0(t5)
    ori          t6, t6, (0x41C64E6D & 0xFFFF)
    mult         t7, t7, t6
    addiu        t7, t7, 0x3039
    dsll32       t7, t7, 1
    dsrl32       t7, t7, 1
    sw           t7, 0x0(t5)
    andi         t7, t7, 0x1000
    bnel         t7, zero, @1
    neg.s        $f4, $f4
@1:
    la           t5, rand_seed
    lui          t6, (0x41C64E6D >> 16)
    lw           t7, 0x0(t5)
    ori          t6, t6, (0x41C64E6D & 0xFFFF)
    mult         t7, t7, t6
    addiu        t7, t7, 0x3039
    dsll32       t7, t7, 1
    dsrl32       t7, t7, 1
    sw           t7, 0x0(t5)
    lui          t6, (0x3F800000 >> 16)
    srl          t7, t7, 8
    or           t7, t7, t6
    mtc1         t6, $f9
    mtc1         t7, $f8
    sub.s        $f8, $f8, $f9
    add.s        $f9, $f9, $f13
    mul.s        $f9, $f9, $f13
    add.s        $f8, $f8, $f13
    div.s        $f8, $f9, $f8
    sub.s        $f8, $f8, $f13
    jr           ra
    mul.s        $f4, $f4, $f8
}

/** n = the unit normal of the triangle `v0`, `v1`, `v2` (hand-written asm). */
/* n (a0), v0 (a1), v1 (a2), v2 (a3) */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 911); the return's delay slot is filled. */
asm void shCreateNormal(float *n, float *v0, float *v1, float *v2) {
    .set noreorder
    lqc2         vf5, 0x0(a1)
    lqc2         vf6, 0x0(a3)
    lqc2         vf7, 0x0(a2)
    vsub.xyzw    vf6, vf6, vf5
    vsub.xyzw    vf7, vf7, vf5
    vopmula.xyz  ACC, vf6, vf7
    vopmsub.xyz  vf4, vf7, vf6
    vmul.xyz     vf5, vf4, vf4
    vaddy.x      vf5, vf5, vf5y
    vaddz.x      vf5, vf5, vf5z
    vrsqrt       Q, vf0w, vf5x
    vsub.w       vf4, vf4, vf4
    vwaitq
    vmulq.xyz    vf4, vf4, Q
    jr           ra
    sqc2         vf4, 0x0(a0)
}

/** Sets `min` and `max` to the bounds of the `n` points `v` (hand-written asm). */
/* min (a0), max (a1), v (a2), n (a3) */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 941); the return's delay slot is filled. */
asm void shSetMiniMaxN(float *min, float *max, float (*v)[4], int n) {
    .set noreorder
    addi         t7, a3, -0x1
    lqc2         vf5, 0x0(a2)
    blez         t7, @1
    vmove.xyzw   vf6, vf5
@loop:
    lqc2         vf4, 0x0(a2)
    vmini.xyzw   vf5, vf5, vf4
    vmax.xyzw    vf6, vf6, vf4
    addi         t7, t7, -0x1
    addiu        a2, a2, 0x10
    bnez         t7, @loop
    nop
@1:
    sqc2         vf5, 0x0(a0)
    jr           ra
    sqc2         vf6, 0x0(a1)
}

/**
 * Returns 1 when the screen vertex `v0` (12.4 fixed point x/y, and w) is outside the clip area, 0
 * otherwise.
 */
char shScreenClipI(int *v0) {
    int x;

    x = (v0[0] >> 4) - 0x800;
    if (iabs(x) > 0x100) {
        return 1;
    }
    x = (v0[1] >> 4) - 0x800;
    if (iabs(x) > 0x100) {
        return 1;
    }
    x = v0[3] >> 4;
    if (x < 0x40 || x > 0x7FFF) {
        return 1;
    }
    return 0;
}

/**
 * Returns 1 when the screen vertex `v0` (float x/y, and w) is outside the clip area, 0 otherwise.
 */
char shScreenClipF(float *v0) {
    float x;

    x = v0[0] - 2048.0f;
    if (fabsf(x) > 256.0f) {
        return 1;
    }
    x = v0[1] - 2048.0f;
    if (fabsf(x) > 256.0f) {
        return 1;
    }
    x = v0[3];
    if (x < 64.0f || x > 32767.0f) {
        return 1;
    }
    return 0;
}

/** Clears `size` bytes at `adr`, by quadwords where aligned (hand-written asm). */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 1103). */
asm void shQzero(void *adr, int size) {
    .set noreorder
    beqz         a1, @4
    andi         t6, a0, 0xF
    beqz         t6, @1
    addi         t7, zero, 0x10
    sub          t6, t7, t6
    slt          t7, a1, t6
    bnel         t7, zero, @loop4
    add          t6, a1, zero
    sub          a1, a1, t6
@loop1:
    sb           zero, 0x0(a0)
    addi         t6, t6, -0x1
    addiu        a0, a0, 0x1
    nop
    nop
    bnez         t6, @loop1
    nop
@1:
    srl          t6, a1, 6
    beqz         t6, @2
    nop
@loop2:
    addiu        t6, t6, -0x1
    sq           zero, 0x0(a0)
    sq           zero, 0x10(a0)
    sq           zero, 0x20(a0)
    sq           zero, 0x30(a0)
    bnez         t6, @loop2
    addiu        a0, a0, 0x40
@2:
    andi         t6, a1, 0x3F
    srl          t6, t6, 4
    beqz         t6, @3
    nop
@loop3:
    sq           zero, 0x0(a0)
    addiu        t6, t6, -0x1
    addiu        a0, a0, 0x10
    nop
    nop
    bnez         t6, @loop3
    nop
@3:
    andi         t6, a1, 0xF
    beqz         t6, @4
    nop
@loop4:
    sb           zero, 0x0(a0)
    addiu        t6, t6, -0x1
    addiu        a0, a0, 0x1
    nop
    nop
    bnez         t6, @loop4
    nop
@4:
    jr           ra
    nop
}

/** Fills `num` words at `adr` with `data` (hand-written asm). */
/* adr (a0), data (a1), num (a2, in words) */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 1171). */
asm void shFill(void *adr, int data, int num) {
    .set noreorder
    beqz         a2, @4
    pextlw       a1, a1, a1
    andi         t6, a0, 0x3
    pextlw       a1, a1, a1
    beqz         t6, @1
    addi         t7, zero, 0x4
    sub          t6, t7, t6
    slt          t7, a2, t6
    bnel         t7, zero, @loop3
    add          t6, a2, zero
    sub          a2, a2, t6
@loop1:
    sw           a1, 0x0(a0)
    addi         t6, t6, -0x1
    addiu        a0, a0, 0x4
    nop
    nop
    bnez         t6, @loop1
    nop
@1:
    srl          t7, a0, 24
    addi         t7, t7, -0x7
    beqz         t7, @2
    slti         t6, a2, 0x9B0
    lui          t7, (0x20000000 >> 16)
    movn         t7, zero, t6
@2:
    srl          t6, a2, 4
    beqz         t6, @3
    addu         a0, a0, t7
@loop2:
    addiu        t6, t6, -0x1
    sq           a1, 0x0(a0)
    sq           a1, 0x10(a0)
    sq           a1, 0x20(a0)
    sq           a1, 0x30(a0)
    bnez         t6, @loop2
    addiu        a0, a0, 0x40
@3:
    andi         t6, a2, 0xF
    beqz         t6, @4
    nop
@loop3:
    sw           a1, 0x0(a0)
    addiu        t6, t6, -0x1
    addiu        a0, a0, 0x4
    nop
    nop
    bnez         t6, @loop3
    nop
@4:
    jr           ra
    nop
}

/** m0 = m1 * m2 (4x4 matrices, on VU0). */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void shMulMatrix(float (*m0)[4], float (*m1)[4], float (*m2)[4]) {
    asm {
        lqc2         vf1, 0x0(m1)
        lqc2         vf2, 0x10(m1)
        lqc2         vf3, 0x20(m1)
        lqc2         vf4, 0x30(m1)
        lqc2         vf5, 0x0(m2)
        vmulax.xyzw  ACC, vf1, vf5x
        vmadday.xyzw ACC, vf2, vf5y
        vmaddaz.xyzw ACC, vf3, vf5z
        vmaddw.xyzw  vf5, vf4, vf5w
        lqc2         vf6, 0x10(m2)
        vmulax.xyzw  ACC, vf1, vf6x
        vmadday.xyzw ACC, vf2, vf6y
        vmaddaz.xyzw ACC, vf3, vf6z
        vmaddw.xyzw  vf6, vf4, vf6w
        lqc2         vf7, 0x20(m2)
        vmulax.xyzw  ACC, vf1, vf7x
        vmadday.xyzw ACC, vf2, vf7y
        vmaddaz.xyzw ACC, vf3, vf7z
        vmaddw.xyzw  vf7, vf4, vf7w
        lqc2         vf8, 0x30(m2)
        vmulax.xyzw  ACC, vf1, vf8x
        vmadday.xyzw ACC, vf2, vf8y
        vmaddaz.xyzw ACC, vf3, vf8z
        vmaddw.xyzw  vf8, vf4, vf8w
        sqc2         vf5, 0x0(m0)
        sqc2         vf6, 0x10(m0)
        sqc2         vf7, 0x20(m0)
        sqc2         vf8, 0x30(m0)
    }
}
