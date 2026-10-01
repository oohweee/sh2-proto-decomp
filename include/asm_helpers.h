#ifndef ASM_HELPERS_H
#define ASM_HELPERS_H

/*
 * Small inline-asm helpers that many of the game's files had.
 *
 * Their original header (or headers) and names are unknown: inline-only functions leave no
 * DWARF, and no out-of-line copy of any of these survives. The names are ours. What is known is
 * the code: the instructions below appear inline in the original functions, in exactly this
 * form, and each body here is the one the most matched files use.
 *
 * Most helpers are MWCC native `asm { }` blocks. A native block is never optimized, and it
 * switches off optimization (scheduling, CSE) for the whole function it is inlined into, so the
 * helper's style matters as much as its instructions (docs/decomp-workflow.md). Where the
 * original used another form for the same operation, that form has its own name here. Native
 * inline-call arguments are evaluated last to first, so parameter order matters too.
 *
 * The helpers named after libm functions (fabsf, fmaxf, fminf) are in asm_libm.h, so that files
 * calling the library functions can still use these.
 */

/** Returns f truncated to int, on the FPU (cvt.w.s; mfc1). A C `(int)f` calls fptosi
 *  instead. */
inline int ftoi(float f) {
    int r;

    asm {
        cvt.w.s f, f
        mfc1    r, f
    }
    return r;
}

/** Returns i converted to float, in place (mtc1; cvt.s.w). */
inline float itof(int i) {
    float f;

    asm {
        mtc1    i, f
        cvt.s.w f, f
    }
    return f;
}

/** Returns the bits of f as an int (mfc1). With a constant argument MWCC folds the mfc1 into an
 *  integer constant: `lui r, 0x3F80` for 1.0f, `move r, zero` and a nop for 0.0f. */
inline int fbits(float f) {
    int r;

    asm {
        mfc1 r, f
    }
    return r;
}

/** Returns v * v (mul.s). */
inline float sqr(float v) {
    asm {
        mul.s v, v, v
    }
    return v;
}

/** Returns sqrt(x) (sqrt.s). sh_vu0.h's _shSqrt is the GCC-style form, which MWCC schedules
 *  differently. */
inline float fsqrt(float x) {
    asm {
        sqrt.s x, x
    }
    return x;
}

/** Returns x clamped to [lo, hi] (max.s; min.s). GCC-style asm: MWCC schedules around it. */
inline float fclamp(float x, float lo, float hi) {
    __asm__ __volatile__("
    max.s %0, %1, %2
    min.s %0, %0, %3
    " : "=f"(x) : "f"(x), "f"(lo), "f"(hi));
    return x;
}

/** Returns x, or 0 when |x| < 2^-16 (the exponent field is compared with that of 2^-16). */
inline float fzero(float x) {
    asm {
        .set noreorder
        lui     t5, 0x7F80
        mfc1    t7, x
        lui     t6, 0x3780
        and     t7, t7, t5
        slt     t7, t7, t6
        bnel    t7, zero, fzero_end
        mtc1    zero, x
    fzero_end:
        .set reorder
    }
    return x;
}

/** Returns |x| for an int (slt; neg; movn, through t6/t7). */
inline int iabs(int x) {
    asm {
        slt  t7, x, zero
        neg  t6, x
        movn x, t6, t7
    }
    return x;
}

/** iabs written as GCC-style asm, which MWCC optimizes around instead of switching optimization
 *  off for the calling function. Not interchangeable with iabs. */
static inline int iabs_gcc(int x) {
    __asm__ __volatile__("
    slt  t7, %0, zero
    neg  t6, %0
    movn %0, t6, t7
    " : "+r"(x));
    return x;
}

/** Returns the int x clamped to [lo, hi] (slt; movn, through t6/t7). GCC-style asm. */
inline int iclamp(int x, int lo, int hi) {
    __asm__ __volatile__("
    slt  t6, %0, %1
    slt  t7, %2, %0
    movn %0, %1, t6
    movn %0, %2, t7
    " : "+r"(x) : "r"(lo), "r"(hi));
    return x;
}

/** Returns |v.xz|, sqrt(v.x * v.x + v.z * v.z), on the FPU. sh_vu0.h's _shLengthXZ is the
 *  GCC-style form. */
inline float lengthXZ(float *v) {
    float d;

    asm {
        lwc1   d, 0(v)
        lwc1   $f8, 8(v)
        mula.s d, d
        madd.s d, $f8, $f8
        sqrt.s d, d
    }
    return d;
}

/** Returns |v.xyz| on the FPU. sh_vu0.h's _shVectorLength is the GCC-style form. */
inline float lengthXYZ(float *v) {
    float d;

    asm {
        lwc1    d, 0(v)
        lwc1    $f8, 4(v)
        lwc1    $f9, 8(v)
        mula.s  d, d
        madda.s $f8, $f8
        madd.s  d, $f9, $f9
        sqrt.s  d, d
    }
    return d;
}

/** Returns the XZ distance between points a and b, on the FPU. The loads start with a, the
 *  second parameter, so callers pass b first. enemy.h's enDistXZ(a, b) is the same computation
 *  taking a first. Matching: both are needed: native inline-call arguments are evaluated last to
 *  first, so the parameter order changes the call's code (the enemy files' 59 enDistXZ calls
 *  written as distXZ(b, a) break 17 functions in 6 files). */
inline float distXZ(float *b, float *a) {
    float d;

    asm {
        lwc1   d, 0(a)
        lwc1   $f8, 0(b)
        lwc1   $f9, 8(a)
        lwc1   $f10, 8(b)
        sub.s  d, d, $f8
        sub.s  $f9, $f9, $f10
        mula.s d, d
        madd.s d, $f9, $f9
        sqrt.s d, d
    }
    return d;
}

/** Returns the distance between points a and b (xyz), on the FPU, with b passed first as in
 *  distXZ. enemy.h's enDist takes a first; both are needed, as for distXZ (en_insect.c's enDist
 *  calls written as distXYZ(b, a) break 2 functions). */
inline float distXYZ(float *b, float *a) {
    float d;

    asm {
        lwc1    d, 0(a)
        lwc1    $f8, 0(b)
        lwc1    $f9, 4(a)
        sub.s   d, d, $f8
        lwc1    $f10, 4(b)
        mula.s  d, d
        lwc1    d, 8(a)
        lwc1    $f8, 8(b)
        sub.s   $f9, $f9, $f10
        sub.s   d, d, $f8
        madda.s $f9, $f9
        madd.s  d, d, d
        sqrt.s  d, d
    }
    return d;
}

/** Returns f converted to 12.4 fixed point, on VU0 (vftoi4). */
inline int ftoi4(float f) {
    int r;

    asm {
        mfc1     r, f
        qmtc2.ni r, vf4
        vftoi4.x vf4, vf4
        qmfc2.ni r, vf4
    }
    return r;
}

/** Returns the 12.4 fixed-point value i converted to float, on VU0 (vitof4). */
inline float itof4(int i) {
    float r;

    asm {
        qmtc2.ni i, vf4
        vitof4.x vf4, vf4
        qmfc2.ni t7, vf4
        mtc1     t7, r
    }
    return r;
}

/** m = the 4x4 identity, built from vf0 = (0, 0, 0, 1). The same instructions as
 *  fi_libvu0_inline.h's _sceVu0UnitMatrix, as a native asm block. */
inline void unitmatrix(float (*m)[4]) {
    asm {
        vsub.xyzw  vf5, vf0, vf0
        vsub.xyzw  vf6, vf0, vf0
        vmr32.xyzw vf4, vf0
        vaddw.y    vf5, vf0, vf0w
        vaddw.x    vf6, vf0, vf0w
        sqc2       vf0, 0x30(m)
        sqc2       vf4, 0x20(m)
        sqc2       vf5, 0x10(m)
        sqc2       vf6, 0x0(m)
    }
}

/** Copies 16 bytes from s to d through t7 (lq; sq). Both must be 16-byte aligned. */
inline void vcopy(void *s, void *d) {
    asm {
        lq t7, 0(s)
        sq t7, 0(d)
    }
}

/** vcopy with the destination first. The instructions are the same, but the arguments are
 *  evaluated in the other order, which changes register allocation in the caller. */
inline void vcopy_dst_first(void *d, void *s) {
    asm {
        lq t7, 0(s)
        sq t7, 0(d)
    }
}

/** vcopy_dst_first written as GCC-style asm, which MWCC optimizes around instead of switching
 *  optimization off for the calling function. Not interchangeable with the native forms. */
static inline void vcopy_gcc(void *d, void *s) {
    __asm__ __volatile__("
    lq t7, 0x0(%1)
    sq t7, 0x0(%0)
    " : : "r"(d), "r"(s));
}

/** Copies the xyz of s to d through t7; d's w is kept. */
inline void vcopy3(void *s, void *d) {
    asm {
        lq     t7, 0(s)
        sd     t7, 0(d)
        pcpyud t7, t7, zero
        sw     t7, 8(d)
    }
}

/** Clears 16 bytes at d (sq zero). */
inline void vzero(void *d) {
    asm {
        sq zero, 0(d)
    }
}

/** Copies a 4x4 matrix (4 qwords) from s to d through t6/t7. Both must be 16-byte aligned. */
inline void mcopy(void *s, void *d) {
    asm {
        lq t6, 0x0(s)
        lq t7, 0x10(s)
        sq t6, 0x0(d)
        sq t7, 0x10(d)
        lq t6, 0x20(s)
        lq t7, 0x30(s)
        sq t6, 0x20(d)
        sq t7, 0x30(d)
    }
}

#endif
