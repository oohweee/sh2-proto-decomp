#ifndef FI_LIBVU0_INLINE_H
#define FI_LIBVU0_INLINE_H

/*
 * Inline versions of libvu0 routines (original: src\Chacter\fi_libvu0_inline.h).
 *
 * Known from the DWARF: the out-of-line copies of three of its functions sit in compile units
 * named after this header, with local (static) binding and these lines: _sceVu0Normalize 109-122,
 * _sceVu0ApplyMatrix_1 209-223, _sceVu0RotTransPers 227-250 (in anime.c and lens_flare.c). They
 * are defined here, `static inline`, on those lines; the bodies are the copies'.
 *
 * MWCC emits a `static inline` function out of line, right after the function being compiled,
 * when a call to it is not inlined (docs/headers.md, section 4): with inlining off, as for
 * lens_flare.c (config/file_flags.txt), and for a call made from inside another inline function
 * (anime.c, through fi_calc.h's ktVectorNormal and a wrapper). Only the first caller gets a copy.
 *
 * The rest of the original header is unknown: the lines before and between the three (#line puts
 * them on their original lines), and whether the inline-only helpers after them were here. Those
 * left no DWARF; they are here because they share the prefix and the style, and their bodies are
 * the ones the matched files use. All are `static inline` with GCC-style extended asm (an MWCC
 * asm block for _sceVu0RotTransPers). Vectors are float[4] and matrices float[4][4], 16-byte
 * aligned (lqc2/sqc2).
 */

/** v0.xyz = v1.xyz / |v1.xyz|; v0.w = 0. */
/* Matching: each #line puts a definition on its original line (the line tables of the copies). */
#line 109
static inline void _sceVu0Normalize(float *v0, float *v1) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    vmul.xyz  vf5, vf4, vf4
    vaddy.x   vf5, vf5, vf5y
    vaddz.x   vf5, vf5, vf5z
    vrsqrt    Q, vf0w, vf5x
    vsub.w    vf4, vf0, vf0
    vwaitq
    vmulq.xyz vf4, vf4, Q
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}

/** dest = mat * (src.xyz, 1). */
#line 209
static inline void _sceVu0ApplyMatrix_1(float *dest, float (*mat)[4], float *src) {
    __asm__ __volatile__("
    lqc2         vf4, 0x0(%1)
    lqc2         vf5, 0x10(%1)
    lqc2         vf6, 0x20(%1)
    lqc2         vf7, 0x30(%1)
    lqc2         vf8, 0x0(%2)
    vmulax.xyzw  ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw  vf8, vf7, vf0w
    sqc2         vf8, 0x0(%0)
    " : : "r"(dest), "r"(mat), "r"(src));
}

/** dest = mat * src with xyz divided by w, as 12.4 fixed point; when mode is non-zero, z and w
 *  are integers instead. An asm block naming its parameters. */
#line 227
static inline void _sceVu0RotTransPers(int *dest, float (*mat)[4], float *src, int mode) {
    asm {
        .set noreorder
        lqc2         vf8, 0x0(src)
        lqc2         vf4, 0x0(mat)
        lqc2         vf5, 0x10(mat)
        lqc2         vf6, 0x20(mat)
        lqc2         vf7, 0x30(mat)
        vmulax.xyzw  ACC, vf4, vf8x
        vmadday.xyzw ACC, vf5, vf8y
        vmaddaz.xyzw ACC, vf6, vf8z
        vmaddw.xyzw  vf8, vf7, vf8w
        vdiv         Q, vf0w, vf8w
        vwaitq
        vmulq.xyz    vf8, vf8, Q
        beqz         mode, @skip
        vftoi4.xyzw  vf9, vf8
        vftoi0.zw    vf9, vf8
    @skip:
        sqc2         vf9, 0x0(dest)
        .set reorder
    }
}

/* ---- inline-only helpers (names and header placement not from the DWARF) ---- */

/** m = the 4x4 identity. */
static inline void _sceVu0UnitMatrix(float (*m)[4]) {
    __asm__ __volatile__("
    vsub.xyzw  vf5, vf0, vf0
    vsub.xyzw  vf6, vf0, vf0
    vmr32.xyzw vf4, vf0
    vaddw.y    vf5, vf0, vf0w
    vaddw.x    vf6, vf0, vf0w
    sqc2       vf0, 0x30(%0)
    sqc2       vf4, 0x20(%0)
    sqc2       vf5, 0x10(%0)
    sqc2       vf6, 0x0(%0)
    " : : "r"(m));
}

/** v0 = m0 * v1 (xyzw). */
static inline void _sceVu0ApplyMatrix(float *v0, float (*m0)[4], float *v1) {
    __asm__ __volatile__("
    lqc2         vf4, 0x0(%2)
    lqc2         vf5, 0x0(%1)
    lqc2         vf6, 0x10(%1)
    vmulax.xyzw  ACC, vf5, vf4x
    lqc2         vf5, 0x20(%1)
    vmadday.xyzw ACC, vf6, vf4y
    lqc2         vf6, 0x30(%1)
    vmaddaz.xyzw ACC, vf5, vf4z
    vmaddw.xyzw  vf4, vf6, vf4w
    sqc2         vf4, 0x0(%0)
    " : : "r"(v0), "r"(m0), "r"(v1));
}

/** m0 = m1 with tv.xyz added to its translation row. */
static inline void _sceVu0TransMatrix(float (*m0)[4], float (*m1)[4], float *tv) {
    __asm__ __volatile__("
    lqc2     vf5, 0x0(%2)
    lqc2     vf4, 0x30(%1)
    lq       t7, 0x0(%1)
    lq       t6, 0x10(%1)
    sq       t7, 0x0(%0)
    vadd.xyz vf4, vf4, vf5
    sq       t6, 0x10(%0)
    lq       t7, 0x20(%1)
    sq       t7, 0x20(%0)
    sqc2     vf4, 0x30(%0)
    " : : "r"(m0), "r"(m1), "r"(tv));
}

/** m0 = transpose of m1 (EE MMI, clobbers t0-t7). */
static inline void _sceVu0TransposeMatrix(float (*m0)[4], float (*m1)[4]) {
    __asm__ __volatile__("
    lq     t0, 0x0(%1)
    lq     t1, 0x10(%1)
    lq     t2, 0x20(%1)
    lq     t3, 0x30(%1)
    pextlw t4, t1, t0
    pextuw t5, t1, t0
    pextlw t6, t3, t2
    pextuw t7, t3, t2
    pcpyld t0, t6, t4
    pcpyud t1, t4, t6
    pcpyld t2, t7, t5
    pcpyud t3, t5, t7
    sq     t0, 0x0(%0)
    sq     t1, 0x10(%0)
    sq     t2, 0x20(%0)
    sq     t3, 0x30(%0)
    " : : "r"(m0), "r"(m1));
}

/** v0.xyz = v1.xyz; v0.w = 1. */
static inline void _sceVu0CopyVectorXYZ(float *v0, float *v1) {
    __asm__ __volatile__("
    lqc2     vf4, 0x0(%1)
    vmove.w  vf4, vf0
    sqc2     vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}

/** v = (0, 0, 0, 1). */
static inline void _sceVu0UnitVector(float *v) {
    __asm__ __volatile__("
    sqc2 vf0, 0x0(%0)
    " : : "r"(v));
}

/** v = (0, 0, 0, 0). */
static inline void _sceVu0ZeroVector(float *v) {
    __asm__ __volatile__("
    sq zero, 0x0(%0)
    " : : "r"(v));
}

/** v0 = v1 + v2 (xyzw). */
static inline void _sceVu0AddVector(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vadd.xyzw vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** v0 = v1 - v2 (xyzw). */
static inline void _sceVu0SubVector(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vsub.xyzw vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** v0.xyz = v1.xyz + v2.xyz; v0.w = v1.w. */
static inline void _sceVu0AddVectorXYZ(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vadd.xyz  vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** v0.xyz = v1.xyz - v2.xyz; v0.w = v1.w. */
static inline void _sceVu0SubVectorXYZ(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vsub.xyz  vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** v0 = v1 * s (xyzw). */
static inline void _sceVu0ScaleVector(float *v0, float *v1, float s) {
    __asm__ __volatile__("
    mfc1       t7, %2
    lqc2       vf4, 0x0(%1)
    qmtc2.ni   t7, vf5
    vmulx.xyzw vf4, vf4, vf5x
    sqc2       vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "f"(s));
}

/** v0 = v1 with every component clamped to [min, max]. */
static inline void _sceVu0ClampVector(float *v0, float *v1, float min, float max) {
    __asm__ __volatile__("
    mfc1        t6, %2
    mfc1        t7, %3
    lqc2        vf4, 0x0(%1)
    qmtc2.ni    t6, vf5
    vmaxx.xyzw  vf4, vf4, vf5x
    qmtc2.ni    t7, vf6
    vminix.xyzw vf4, vf4, vf6x
    sqc2        vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "f"(min), "f"(max));
}

/** v0 = v1 * t + v2 * (1 - t). */
static inline void _sceVu0InterVector(float *v0, float *v1, float *v2, float t) {
    __asm__ __volatile__("
    mfc1        t7, %3
    qmtc2.ni    t7, vf6
    vsubx.w     vf6, vf0, vf6x
    lqc2        vf4, 0x0(%1)
    lqc2        vf5, 0x0(%2)
    vmulax.xyzw ACC, vf4, vf6x
    vmaddw.xyzw vf4, vf5, vf6w
    sqc2        vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2), "f"(t));
}

/** v0.xyz = v1 x v2; v0.w = 0 (same body as _shOuterProduct). */
static inline void _sceVu0OuterProduct(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2        vf5, 0x0(%1)
    lqc2        vf6, 0x0(%2)
    vsub.w      vf4, vf0, vf0
    vopmula.xyz ACC, vf5, vf6
    vopmsub.xyz vf4, vf6, vf5
    sqc2        vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

/** v = the int vector at v converted from 20.12 fixed point to float, in place. */
static inline void _sceVu0ItoF12Vector(float *v) {
    __asm__ __volatile__("
    lqc2         vf4, 0x0(%0)
    vitof12.xyzw vf5, vf4
    sqc2         vf5, 0x0(%0)
    " : : "r"(v));
}

/** v = the int vector at v converted from 17.15 fixed point to float, in place. */
static inline void _sceVu0ItoF15Vector(float *v) {
    __asm__ __volatile__("
    lqc2         vf4, 0x0(%0)
    vitof15.xyzw vf5, vf4
    sqc2         vf5, 0x0(%0)
    " : : "r"(v));
}

#endif
