#ifndef SH_VU0_H
#define SH_VU0_H

/*
 * The game's VU0/FPU vector helpers (original: src\SH2_common\sh_vu0.h).
 *
 * The header's name and path come from the DWARF: out-of-line copies of _shAddVector,
 * _shScaleVector, _shNormalize, _shOuterProduct, _shInnerProduct (sh2_JmsSpot_Man) and _shLength
 * (sh_character_status) sit in compile units named after it, and their code gives the exact bodies.
 * Those six are defined first. The first five are in the order of their copies in
 * sh2_JmsSpot_Man, which declares them before their caller and includes this header after it:
 * MWCC then emits the copies in definition order (tested; the order of the declarations and of
 * the first calls doesn't matter). If the original used the same construct, this was the
 * header's order too. Where _shLength stood among them is unknown.
 *
 * The other _sh* helpers below are inline-only and left no DWARF. They are here because they
 * share the prefix and the style, not because the DWARF places them in this header; their
 * bodies are the ones the matched files use.
 *
 * All are `static inline` with GCC-style extended asm, which MWCC schedules and optimizes
 * around. A native `asm { }` block doesn't get that treatment, and it switches off optimization
 * for the whole calling function (docs/decomp-workflow.md), so a file whose code needs the
 * native form must keep that helper local under another name.
 *
 * Vectors are float[4] and must be 16-byte aligned (lqc2/sqc2).
 */

/** v0 = v1 + v2 (xyzw). */
static inline void _shAddVector(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vadd.xyzw vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** v0 = v1 * s (xyzw). */
static inline void _shScaleVector(float *v0, float *v1, float s) {
    __asm__ __volatile__("
    mfc1       t7, %2
    lqc2       vf4, 0x0(%1)
    qmtc2.ni   t7, vf5
    vmulx.xyzw vf4, vf4, vf5x
    sqc2       vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "f"(s));
}

/** v0.xyz = v1.xyz / |v1.xyz|; v0.w = v1.w. */
static inline void _shNormalize(float *v0, float *v1) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    vmul.xyz  vf5, vf4, vf4
    vaddy.x   vf5, vf5, vf5y
    vaddz.x   vf5, vf5, vf5z
    vrsqrt    Q, vf0w, vf5x
    vwaitq
    vmulq.xyz vf4, vf4, Q
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}

/** v0.xyz = v1 x v2; v0.w = 0. */
static inline void _shOuterProduct(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2        vf5, 0x0(%1)
    lqc2        vf6, 0x0(%2)
    vsub.w      vf4, vf0, vf0
    vopmula.xyz ACC, vf5, vf6
    vopmsub.xyz vf4, vf6, vf5
    sqc2        vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** Returns v0.xyz . v1.xyz, on the FPU. */
static inline float _shInnerProduct(float *v0, float *v1) {
    float r;

    __asm__ __volatile__("
    lwc1    %0, 0x0(%1)
    lwc1    $f8, 0x0(%2)
    lwc1    $f9, 0x4(%1)
    lwc1    $f10, 0x4(%2)
    mula.s  %0, $f8
    lwc1    %0, 0x8(%1)
    lwc1    $f8, 0x8(%2)
    madda.s $f9, $f10
    madd.s  %0, %0, $f8
    " : "=f"(r) : "r"(v0), "r"(v1));
    return r;
}

/** Returns |v0.xyz - v1.xyz| (the distance between two points), on the FPU. */
static inline float _shLength(float *v0, float *v1) {
    float r;

    __asm__ __volatile__("
    lwc1    %0, 0x0(%1)
    lwc1    $f8, 0x0(%2)
    lwc1    $f9, 0x4(%1)
    sub.s   %0, %0, $f8
    lwc1    $f10, 0x4(%2)
    mula.s  %0, %0
    lwc1    %0, 0x8(%1)
    lwc1    $f8, 0x8(%2)
    sub.s   $f9, $f9, $f10
    sub.s   %0, %0, $f8
    madda.s $f9, $f9
    madd.s  %0, %0, %0
    sqrt.s  %0, %0
    " : "=f"(r) : "r"(v0), "r"(v1));
    return r;
}

/* ---- inline-only helpers (names and header placement not from the DWARF) ---- */

/** v0 = v1 - v2 (xyzw). */
static inline void _shSubVector(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vsub.xyzw vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** v0.xyz = v1.xyz + v2.xyz; v0.w = v1.w. */
static inline void _shAddVectorXYZ(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vadd.xyz  vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** v0.xyz = v1.xyz - v2.xyz; v0.w = v1.w. */
static inline void _shSubVectorXYZ(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vsub.xyz  vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** v0.xyz = v1.xyz * s; v0.w = v1.w. */
static inline void _shScaleVectorXYZ(float *v0, float *v1, float s) {
    __asm__ __volatile__("
    mfc1       t7, %2
    lqc2       vf4, 0x0(%1)
    qmtc2.ni   t7, vf5
    vmulx.xyz  vf4, vf4, vf5x
    sqc2       vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "f"(s));
}

/** v0.xyz = v1.xyz / s (one FPU divide, then a VU0 multiply); v0.w = v1.w. */
static inline void _shDivVectorXYZ(float *v0, float *v1, float s) {
    __asm__ __volatile__("
    lui        t7, 0x3F80
    mtc1       t7, $f8
    div.s      $f8, $f8, %2
    lqc2       vf4, 0x0(%1)
    mfc1       t7, $f8
    qmtc2.ni   t7, vf5
    vmulx.xyz  vf4, vf4, vf5x
    sqc2       vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "f"(s));
}

/** v0 = v1, a 16-byte copy through t7 (GCC-style; for the native-asm copy see vcopy). */
static inline void _shCopyVector(float *v0, float *v1) {
    __asm__ __volatile__("
    lq t7, 0x0(%1)
    sq t7, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}

/** v = (0, 0, 0, 1). */
static inline void _shUnitVector(float *v) {
    __asm__ __volatile__("
    sqc2 vf0, 0x0(%0)
    " : : "r"(v));
}

/** Returns |v.xyz| (the length of one vector), on the FPU. Several files called their copy
 *  _shLength, which the DWARF shows is the two-point distance above; the name here is ours. */
static inline float _shVectorLength(float *v) {
    float r;

    __asm__ __volatile__("
    lwc1    %0, 0x0(%1)
    lwc1    $f8, 0x4(%1)
    lwc1    $f9, 0x8(%1)
    mula.s  %0, %0
    madda.s $f8, $f8
    madd.s  %0, $f9, $f9
    sqrt.s  %0, %0
    " : "=f"(r) : "r"(v));
    return r;
}

/** Returns |v.xz|, sqrt(v.x * v.x + v.z * v.z), on the FPU (asm_helpers.h's lengthXZ is the
 *  native-asm form). */
static inline float _shLengthXZ(float *v) {
    float r;

    __asm__ __volatile__("
    lwc1   %0, 0x0(%1)
    lwc1   $f8, 0x8(%1)
    mula.s %0, %0
    madd.s %0, $f8, $f8
    sqrt.s %0, %0
    " : "=f"(r) : "r"(v));
    return r;
}

/** Returns sqrt(x) with sqrt.s (a plain sqrtf() is a library call). */
static inline float _shSqrt(float x) {
    float r;

    __asm__ __volatile__("
    sqrt.s %0, %1
    " : "=f"(r) : "f"(x));
    return r;
}

/** Returns -1, 0 or 1 by the sign of x. MWCC drops the `mov.s f8, f8` when r lands in f8 (its
 *  own scratch). */
static inline float _shSign(float x) {
    float r;

    __asm__ __volatile__("
    .set noreorder
    sub.s  $f8, $f8, $f8
    lui    t7, 0x3f80
    c.eq.s $f8, %1
    bc1tl  sign_end
    mov.s  %0, $f8
    c.lt.s %0, $f8
    mtc1   t7, %0
    bc1tl  sign_end
    neg.s  %0, %0
sign_end:
    .set reorder
    " : "=f"(r) : "f"(x));
    return r;
}

/** _shSign computed in place ("+f": x's register is the result). Allocates differently from
 *  _shSign; en_common and en_tyu use both, at different call sites. */
static inline float _shSignIP(float x) {
    __asm__ __volatile__("
    .set noreorder
    sub.s  $f8, $f8, $f8
    lui    t7, 0x3f80
    c.eq.s $f8, %0
    bc1tl  signip_end
    mov.s  %0, $f8
    c.lt.s %0, $f8
    mtc1   t7, %0
    bc1tl  signip_end
    neg.s  %0, %0
signip_end:
    .set reorder
    " : "+f"(x));
    return x;
}

/** v0 = m0 * v1 without m0's translation row: v1.x * m0[0] + v1.y * m0[1] + v1.z * m0[2], plus
 *  v1.w in w. */
static inline void _shApplyRotMatrix(float *v0, float (*m0)[4], float *v1) {
    __asm__ __volatile__("
    lqc2         vf4, 0x0(%2)
    lqc2         vf5, 0x0(%1)
    lqc2         vf6, 0x10(%1)
    vmulax.xyzw  ACC, vf5, vf4x
    lqc2         vf5, 0x20(%1)
    vmadday.xyzw ACC, vf6, vf4y
    vmaddaz.xyzw ACC, vf5, vf4z
    vmaddw.xyzw  vf4, vf0, vf4w
    sqc2         vf4, 0x0(%0)
    " : : "r"(v0), "r"(m0), "r"(v1));
}

/** Perspective transform of v1 by m0 into v0 (xyz divided by w); returns Q (1/w). */
static inline float _shRotTransPersQ(float *v0, float (*m0)[4], float *v1) {
    float q;

    __asm__ __volatile__("
    lqc2         vf4, 0x0(%3)
    lqc2         vf5, 0x0(%2)
    lqc2         vf6, 0x10(%2)
    vmulax.xyzw  ACC, vf5, vf4x
    vmadday.xyzw ACC, vf6, vf4y
    lqc2         vf5, 0x20(%2)
    lqc2         vf6, 0x30(%2)
    vmaddaz.xyzw ACC, vf5, vf4z
    vmaddw.xyzw  vf4, vf6, vf4w
    vdiv         Q, vf0w, vf4w
    vwaitq
    vmulq.xyz    vf4, vf4, Q
    cfc2.ni      t7, vi22
    sqc2         vf4, 0x0(%1)
    mtc1         t7, %0
    " : "=f"(q) : "r"(v0), "r"(m0), "r"(v1));
    return q;
}

/** Screen transform of v by m into iv (12.4 fixed-point x/y, integer z); returns the packed
 *  GS XYZ. */
static inline unsigned long _shRotTransPersXYZ(int *iv, float (*m)[4], float *v) {
    unsigned long r;

    __asm__ __volatile__("
    lqc2         vf4, 0x0(%3)
    lqc2         vf5, 0x0(%2)
    lqc2         vf6, 0x10(%2)
    vmulax.xyzw  ACC, vf5, vf4x
    vmadday.xyzw ACC, vf6, vf4y
    lqc2         vf5, 0x20(%2)
    lqc2         vf6, 0x30(%2)
    vmaddaz.xyzw ACC, vf5, vf4z
    vmaddw.xyzw  vf4, vf6, vf4w
    vdiv         Q, vf0w, vf4w
    vwaitq
    vmulq.xyz    vf4, vf4, Q
    vftoi4.xyw   vf4, vf4
    vftoi0.z     vf4, vf4
    sqc2         vf4, 0x0(%1)
    qmfc2.ni     %0, vf4
    pexch        t7, %0
    pextuw       %0, zero, %0
    pextlw       %0, %0, t7
    " : "=r"(r) : "r"(iv), "r"(m), "r"(v));
    return r;
}

#endif
