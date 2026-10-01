/*
 * cl_calc2.c: hand-scheduled VU0 macro-mode hit tests (line/plane, line/column,
 * wall/column, column/column). Every function is one asm block with filled branch
 * delay slots (`.set noreorder`); the result flag is written to result->chk and
 * returned. The line/plane arguments are VU0 quadwords (xyzw), loaded by parameter
 * name (the original's DWARF has the parameters, so its asm referred to them).
 */

#include "sh2.h"

/** Tests the segment @p line0-@p line1 against the quad @p plane0..@p plane3; on a hit, stores the
 * point in @p result. @return result->chk. */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
int clCheckSubLineToPlane(struct _CL_HITRESULT *result, float (*line0)[4], float (*line1)[4], float (*plane0)[4], float (*plane1)[4], float (*plane2)[4], float (*plane3)[4]) {
    asm {
        .set noreorder
        addiu        v0, a0, 0x10
        sw           zero, 0x0(a0)
        lqc2         vf1, 0x0(line0)
        lqc2         vf2, 0x0(line1)
        lqc2         vf3, 0x0(plane0)
        lqc2         vf4, 0x0(plane1)
        lqc2         vf5, 0x0(plane2)
        lqc2         vf6, 0x0(plane3)
        vmax.xyz     vf7, vf3, vf4
        vmax.xyz     vf8, vf5, vf6
        vmini.xyz    vf9, vf3, vf4
        vmini.xyz    vf10, vf5, vf6
        vmax.xyz     vf7, vf7, vf8
        vmini.xyz    vf8, vf1, vf2
        vmini.xyz    vf9, vf9, vf10
        vmax.xyz     vf10, vf1, vf2
        vsub.w       vf1, vf1, vf1
        vnop
        vnop
        ctc2.ni      zero, vi16
        vsub.xyz     vf7, vf7, vf8
        vsub.xyz     vf8, vf10, vf9
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni      t0, vi16
        andi         t0, t0, 0x80
        bnez         t0, @2
        vsub.xyz     vf13, vf4, vf3
        vsub.xyz     vf14, vf5, vf3
        vsub.xyz     vf17, vf1, vf3
        vsub.xyz     vf12, vf2, vf1
        sub.s        $f0, $f0, $f0
        vopmula.xyz  ACC, vf14, vf13
        vopmsub.xyz  vf7, vf13, vf14
        vsub.xyz     vf14, vf5, vf4
        vsub.xyz     vf15, vf6, vf5
        vsub.xyz     vf16, vf3, vf6
        vaddy.x      vf8, vf0, vf7y
        vaddz.x      vf9, vf0, vf7z
        vadda.x      ACC, vf0, vf0
        vmsuba.x     ACC, vf7, vf3
        vmsubay.x    ACC, vf8, vf3y
        vmsubz.x     vf10, vf9, vf3z
        vmula.x      ACC, vf7, vf1
        vmadday.x    ACC, vf8, vf1y
        vmaddz.x     vf11, vf9, vf1z
        vadda.x      ACC, vf0, vf10
        vmadda.x     ACC, vf7, vf2
        vmadday.x    ACC, vf8, vf2y
        vnop
        vnop
        vnop
        ctc2.ni      zero, vi16
        vmaddz.x     vf9, vf9, vf2z
        vadd.x       vf8, vf11, vf10
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni      t0, vi16
        andi         t0, t0, 0x80
        beqz         t0, @2
        vopmula.xyz  ACC, vf13, vf12
        vopmsub.xyz  vf21, vf12, vf13
        vaddy.x      vf9, vf0, vf17y
        vaddz.x      vf10, vf0, vf17z
        vsub.xyz     vf18, vf1, vf4
        vmula.x      ACC, vf17, vf21
        vmadday.x    ACC, vf9, vf21y
        vmaddz.x     vf17, vf10, vf21z
        vopmula.xyz  ACC, vf14, vf12
        vopmsub.xyz  vf22, vf12, vf14
        vaddy.x      vf9, vf0, vf18y
        qmfc2.ni     t0, vf17
        mtc1         t0, $f1
        c.lt.s       $f0, $f1
        bc1t         @2
        vaddz.x      vf10, vf0, vf18z
        vmula.x      ACC, vf18, vf22
        vmadday.x    ACC, vf9, vf22y
        vsub.xyz     vf19, vf1, vf5
        vmaddz.x     vf18, vf10, vf22z
        vopmula.xyz  ACC, vf15, vf12
        vopmsub.xyz  vf23, vf12, vf15
        vaddy.x      vf9, vf0, vf19y
        qmfc2.ni     t0, vf18
        mtc1         t0, $f1
        c.lt.s       $f0, $f1
        bc1t         @2
        vaddz.x      vf10, vf0, vf19z
        vmula.x      ACC, vf19, vf23
        vmadday.x    ACC, vf9, vf23y
        vsub.xyz     vf20, vf1, vf6
        vmaddz.x     vf19, vf10, vf23z
        vopmula.xyz  ACC, vf16, vf12
        vopmsub.xyz  vf24, vf12, vf16
        vaddy.x      vf9, vf0, vf20y
        qmfc2.ni     t0, vf19
        mtc1         t0, $f1
        c.lt.s       $f0, $f1
        bc1t         @2
        vaddz.x      vf10, vf0, vf20z
        vmula.x      ACC, vf20, vf24
        vmadday.x    ACC, vf9, vf24y
        vnop
        vmaddz.x     vf20, vf10, vf24z
        vaddy.x      vf10, vf0, vf7y
        vaddz.x      vf11, vf0, vf7z
        vnop
        qmfc2.ni     t0, vf20
        mtc1         t0, $f1
        c.lt.s       $f0, $f1
        bc1t         @2
        vmula.x      ACC, vf7, vf12
        vmadday.x    ACC, vf10, vf12y
        vmaddz.x     vf10, vf11, vf12z
        addi         t1, zero, 0x1
        vnop
        vnop
        qmfc2.ni     t0, vf10
        mtc1         t0, $f1
        c.eq.s       $f0, $f1
        bc1t         @1
        vnop
        vdiv         Q, vf8x, vf10x
        vadda.xyz    ACC, vf0, vf1
        vnop
        vnop
        vnop
        vnop
        vnop
        vwaitq
        vmsubq.xyz   vf1, vf12, Q
        vnop
        vnop
        vnop
    @1:
        sqc2         vf1, 0x0(v0)
        sw           t1, 0x0(a0)
    @2:
    }
    return result->chk;
}

/** Tests the segment @p line0-@p line1 against the triangle @p plane0..@p plane2; on a hit, stores
 * the point in @p result. @return result->chk. */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
int clCheckSubLineToPlane3(struct _CL_HITRESULT *result, float (*line0)[4], float (*line1)[4], float (*plane0)[4], float (*plane1)[4], float (*plane2)[4]) {
    asm {
        .set noreorder
        addiu        v0, a0, 0x10
        sw           zero, 0x0(a0)
        lqc2         vf1, 0x0(line0)
        lqc2         vf2, 0x0(line1)
        lqc2         vf3, 0x0(plane0)
        lqc2         vf4, 0x0(plane1)
        lqc2         vf5, 0x0(plane2)
        vmax.xyz     vf6, vf3, vf4
        vmini.xyz    vf7, vf3, vf4
        vmax.xyz     vf8, vf1, vf2
        vmini.xyz    vf9, vf1, vf2
        vmax.xyz     vf6, vf6, vf5
        vmini.xyz    vf7, vf7, vf5
        vnop
        vnop
        ctc2.ni      zero, vi16
        vsub.xyz     vf6, vf6, vf9
        vsub.xyz     vf7, vf8, vf7
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni      t0, vi16
        andi         t0, t0, 0x80
        bnez         t0, @2
        vsub.xyz     vf6, vf4, vf3
        vsub.xyz     vf7, vf5, vf3
        vsub.xyz     vf14, vf2, vf1
        vsub.xyz     vf15, vf5, vf4
        vsub.xyz     vf16, vf3, vf5
        vopmula.xyz  ACC, vf7, vf6
        vopmsub.xyz  vf8, vf6, vf7
        vsub.xyz     vf17, vf1, vf3
        vsub.xyz     vf18, vf1, vf4
        vsub.xyz     vf19, vf1, vf5
        vaddy.x      vf9, vf0, vf8y
        vaddz.x      vf10, vf0, vf8z
        vadda.x      ACC, vf0, vf0
        vmsuba.x     ACC, vf8, vf3
        vmsubay.x    ACC, vf9, vf3y
        vmsubz.x     vf11, vf10, vf3z
        vmula.x      ACC, vf8, vf1
        vmadday.x    ACC, vf9, vf1y
        vmaddz.x     vf12, vf10, vf1z
        vadda.x      ACC, vf0, vf11
        vmadda.x     ACC, vf8, vf2
        vmadday.x    ACC, vf9, vf2y
        vnop
        vnop
        ctc2.ni      zero, vi16
        vmaddz.x     vf13, vf10, vf2z
        vadd.x       vf12, vf12, vf11
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni      t0, vi16
        andi         t0, t0, 0x80
        beqz         t0, @2
        vopmula.xyz  ACC, vf6, vf14
        vopmsub.xyz  vf20, vf14, vf6
        sub.s        $f0, $f0, $f0
        vopmula.xyz  ACC, vf15, vf14
        vopmsub.xyz  vf21, vf14, vf15
        vmul.xyz     vf20, vf20, vf17
        vopmula.xyz  ACC, vf16, vf14
        vopmsub.xyz  vf22, vf14, vf16
        vmul.xyz     vf21, vf21, vf18
        vaddy.x      vf20, vf20, vf20y
        vnop
        vmul.xyz     vf22, vf22, vf19
        vaddy.x      vf21, vf21, vf21y
        vaddz.x      vf20, vf20, vf20z
        vnop
        vaddy.x      vf22, vf22, vf22y
        vaddz.x      vf21, vf21, vf21z
        qmfc2.ni     t0, vf20
        mtc1         t0, $f1
        c.lt.s       $f0, $f1
        bc1t         @2
        vaddz.x      vf22, vf22, vf22z
        qmfc2.ni     t0, vf21
        mtc1         t0, $f1
        c.lt.s       $f0, $f1
        bc1t         @2
        qmfc2.ni     t0, vf22
        mtc1         t0, $f1
        c.lt.s       $f0, $f1
        bc1t         @2
        vmula.x      ACC, vf8, vf14
        vmadday.x    ACC, vf9, vf14y
        vmaddz.x     vf23, vf10, vf14z
        addi         t1, zero, 0x1
        vnop
        vnop
        qmfc2.ni     t0, vf23
        mtc1         t0, $f1
        c.eq.s       $f0, $f1
        bc1t         @1
        vnop
        vdiv         Q, vf12x, vf23x
        vadda.xyz    ACC, vf0, vf1
        vnop
        vnop
        vnop
        vnop
        vnop
        vwaitq
        vmsubq.xyz   vf1, vf14, Q
        vnop
        vnop
        vnop
    @1:
        sqc2         vf1, 0x0(v0)
        sw           t1, 0x0(a0)
    @2:
    }
    return result->chk;
}

/** Tests the segment @p line0-@p line1 against the column @p column. @return result->chk. */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
int clCheckSubLineToColumnPlus(struct _CL_HITRESULT *result, float (*line0)[4], float (*line1)[4], float (*column)[4]) {
    asm {
        .set noreorder
        addiu        v0, a0, 0x10
        sw           zero, 0x0(a0)
        lqc2         vf1, 0x0(line0)
        lqc2         vf2, 0x0(line1)
        lqc2         vf3, 0x0(column)
        lqc2         vf4, 0x10(column)
        vaddw.xz     vf5, vf3, vf4w
        vsubw.xz     vf6, vf3, vf4w
        vmax.xyz     vf7, vf1, vf2
        vmini.xyz    vf8, vf1, vf2
        vadd.y       vf5, vf0, vf3
        vadd.y       vf6, vf0, vf4
        vnop
        vnop
        ctc2.ni      zero, vi16
        vsub.xyz     vf5, vf5, vf8
        vsub.xyz     vf6, vf7, vf6
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni      t0, vi16
        andi         t0, t0, 0x80
        bnez         t0, @13
        vnop
        vsub.xz      vf1, vf1, vf3
        vsub.xz      vf2, vf2, vf3
        sub.s        $f0, $f0, $f0
        vmul.xz      vf5, vf1, vf1
        vmul.xz      vf6, vf2, vf2
        vsub.xyz     vf7, vf2, vf1
        vmul.w       vf8, vf4, vf4
        vaddz.x      vf5, vf5, vf5z
        vaddz.x      vf6, vf6, vf6z
        vmul.xz      vf9, vf7, vf7
        vaddw.x      vf8, vf0, vf8w
        vsqrt        Q, vf5x
        vsubw.x      vf10, vf0, vf4w
        vaddz.x      vf9, vf9, vf9z
        vwaitq
        vaddq.x      vf11, vf10, Q
        vsqrt        Q, vf6x
        qmfc2.ni     t0, vf9
        mtc1         t0, $f1
        vmul.x       vf12, vf11, vf11
        qmfc2.ni     t0, vf11
        mtc1         t0, $f2
        vwaitq
        vaddq.x      vf13, vf10, Q
        vnop
        c.le.s       $f2, $f0
        bc1t         @5
        vnop
        qmfc2.ni     t0, vf12
        mtc1         t0, $f2
        c.lt.s       $f1, $f2
        bc1t         @13
        vnop
        vmul.x       vf14, vf13, vf13
        qmfc2.ni     t0, vf13
        mtc1         t0, $f2
        c.le.s       $f2, $f0
        bc1t         @1
        vnop
        qmfc2.ni     t0, vf14
        mtc1         t0, $f2
        c.lt.s       $f1, $f2
        bc1t         @13
        vnop
    @1:
        qmfc2.ni     t0, vf7
        mtc1         t0, $f1
        c.eq.s       $f0, $f1
        bc1t         @2
        vnop
        vnop
        vnop
        vdiv         Q, vf7z, vf7x
        vwaitq
        vaddq.x      vf10, vf0, Q
        vaddaz.x     ACC, vf0, vf1z
        vmsub.x      vf11, vf10, vf1
        vaddaw.x     ACC, vf0, vf0w
        vmadd.x      vf12, vf10, vf10
        vadda.x      ACC, vf0, vf0
        vmsuba.x     ACC, vf11, vf11
        vmadd.x      vf13, vf8, vf12
        qmfc2.ni     t0, vf13
        mtc1         t0, $f1
        c.lt.s       $f1, $f0
        bc1t         @13
        vnop
        vsqrt        Q, vf13x
        vmul.x       vf14, vf10, vf11
        vsub.x       vf14, vf0, vf14
        vwaitq
        vaddq.x      vf13, vf0, Q
        vdiv         Q, vf0w, vf12x
        vadd.x       vf8, vf14, vf13
        vsub.x       vf9, vf14, vf13
        vaddx.z      vf10, vf0, vf10x
        vwaitq
        vmulq.x      vf8, vf8, Q
        vmulq.x      vf9, vf9, Q
        vaddax.z     ACC, vf0, vf11x
        vmaddx.z     vf8, vf10, vf8x
        vaddax.z     ACC, vf0, vf11x
        vmaddx.z     vf9, vf10, vf9x
        j            @3
        vnop
    @2:
        vadda.x      ACC, vf0, vf8
        vmsub.x      vf5, vf1, vf1
        vadd.x       vf8, vf0, vf1
        vadd.x       vf9, vf0, vf1
        vsqrt        Q, vf5x
        vwaitq
        vaddq.z      vf8, vf0, Q
        vsubq.z      vf9, vf0, Q
    @3:
        vsub.xz      vf10, vf8, vf1
        vsub.xz      vf11, vf9, vf1
        vaddz.x      vf12, vf0, vf10z
        vaddz.x      vf13, vf0, vf11z
        vmula.x      ACC, vf10, vf10
        vmsuba.x     ACC, vf11, vf11
        vmadda.x     ACC, vf12, vf12
        vmsub.x      vf14, vf13, vf13
        qmfc2.ni     t0, vf14
        mtc1         t0, $f1
        c.lt.s       $f0, $f1
        bc1t         @4
        vadd.xz      vf5, vf9, vf0
        vadd.xz      vf5, vf8, vf0
    @4:
        j            @6
        vnop
    @5:
        vadd.xz      vf5, vf1, vf0
    @6:
        vaddz.x      vf6, vf0, vf7z
        qmfc2.ni     t0, vf7
        mtc1         t0, $f1
        c.eq.s       $f0, $f1
        bc1f         @7
        qmfc2.ni     t0, vf6
        mtc1         t0, $f1
        c.eq.s       $f0, $f1
        bc1f         @8
        vnop
        j            @9
        vadd.y       vf5, vf0, vf1
    @7:
        vsub.x       vf8, vf5, vf1
        vnop
        vnop
        vdiv         Q, vf8x, vf7x
        vadda.y      ACC, vf0, vf1
        vwaitq
        j            @9
        vmaddq.y     vf5, vf7, Q
    @8:
        vsub.z       vf8, vf5, vf1
        vnop
        vnop
        vdiv         Q, vf8z, vf7z
        vadda.y      ACC, vf0, vf1
        vwaitq
        vmaddq.y     vf5, vf7, Q
    @9:
        vaddy.x      vf8, vf0, vf3y
        vaddy.x      vf9, vf0, vf4y
        vaddy.x      vf10, vf0, vf5y
        qmfc2.ni     t0, vf8
        mtc1         t0, $f1
        qmfc2.ni     t0, vf10
        mtc1         t0, $f2
        c.lt.s       $f1, $f2
        bc1t         @10
        qmfc2.ni     t0, vf9
        mtc1         t0, $f1
        c.lt.s       $f2, $f1
        bc1t         @11
        vnop
        j            @12
        vnop
    @10:
        vsuby.x      vf11, vf0, vf7y
        vadd.xyz     vf5, vf0, vf1
        qmfc2.ni     t0, vf11
        mtc1         t0, $f1
        c.eq.s       $f0, $f1
        bc1t         @12
        vnop
        vsub.y       vf8, vf3, vf1
        vdiv         Q, vf8y, vf7y
        vadda.xyz    ACC, vf0, vf1
        vwaitq
        vmaddq.xyz   vf5, vf7, Q
        j            @12
        vnop
    @11:
        vaddy.x      vf11, vf0, vf7y
        vadd.xyz     vf5, vf0, vf1
        qmfc2.ni     t0, vf11
        mtc1         t0, $f1
        c.eq.s       $f0, $f1
        bc1t         @12
        vnop
        vsub.y       vf8, vf4, vf1
        vdiv         Q, vf8y, vf7y
        vadda.xyz    ACC, vf0, vf1
        vwaitq
        vmaddq.xyz   vf5, vf7, Q
        j            @12
        vnop
    @12:
        vsub.w       vf5, vf5, vf5
        addi         t0, zero, 0x1
        sw           t0, 0x0(a0)
        vadd.xz      vf5, vf5, vf3
        sqc2         vf5, 0x0(v0)
    @13:
    }
    return result->chk;
}

/** Tests the wall edge @p wall0-@p wall1 against the column @p column. @return result->chk. */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
int clCheckSubWallToColumn(struct _CL_HITRESULT *result, float (*wall0)[4], float (*wall1)[4], float (*column)[4]) {
    asm {
        .set noreorder
        addiu        v0, a0, 0x20
        sw           zero, 0x0(a0)
        lqc2         vf1, 0x0(wall0)
        lqc2         vf2, 0x0(wall1)
        lqc2         vf3, 0x0(column)
        lqc2         vf4, 0x10(column)
        vaddw.xz     vf5, vf3, vf4w
        vsubw.xz     vf6, vf3, vf4w
        vmax.xyz     vf7, vf1, vf2
        vmini.xyz    vf8, vf1, vf2
        vadd.y       vf5, vf0, vf3
        vadd.y       vf6, vf0, vf4
        sub.s        $f0, $f0, $f0
        vnop
        vnop
        ctc2.ni      zero, vi16
        vsub.xyz     vf5, vf5, vf8
        vsub.xyz     vf6, vf7, vf6
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni      t0, vi16
        andi         t0, t0, 0x80
        bnez         t0, @3
        vnop
        vsub.xz      vf5, vf2, vf1
        vsub.xz      vf6, vf3, vf1
        vsub.xz      vf7, vf3, vf2
        vaddw.x      vf8, vf0, vf4w
        vmul.xz      vf9, vf5, vf5
        vmul.xz      vf10, vf5, vf6
        vmul.xz      vf11, vf6, vf6
        vmul.xz      vf12, vf7, vf7
        vmul.x       vf8, vf8, vf8
        vaddz.x      vf9, vf9, vf9z
        vaddz.x      vf10, vf10, vf10z
        vaddz.x      vf11, vf11, vf11z
        vaddz.x      vf12, vf12, vf12z
        qmfc2.ni     t0, vf9
        mtc1         t0, $f1
        c.eq.s       $f0, $f1
        bc1t         @3
        vaddw.x      vf13, vf0, vf0w
        vdiv         Q, vf10x, vf9x
        vsub.x       vf11, vf11, vf8
        vsub.x       vf12, vf12, vf8
        qmfc2.ni     t0, vf13
        mtc1         t0, $f1
        vadda.xz     ACC, vf1, vf0
        vwaitq
        vmaddq.xz    vf10, vf5, Q
        vaddq.x      vf14, vf0, Q
        vnop
        vnop
        vsub.xz      vf15, vf3, vf10
        vnop
        vnop
        vnop
        vaddz.x      vf16, vf0, vf15z
        vnop
        vadda.x      ACC, vf8, vf0
        vmsuba.x     ACC, vf15, vf15
        vmsubz.x     vf15, vf16, vf15z
        vnop
        vnop
        vnop
        qmfc2.ni     t0, vf15
        mtc1         t0, $f2
        c.lt.s       $f2, $f0
        bc1t         @3
        qmfc2.ni     t0, vf11
        mtc1         t0, $f2
        c.le.s       $f2, $f0
        bc1t         @1
        qmfc2.ni     t0, vf12
        mtc1         t0, $f2
        c.le.s       $f2, $f0
        bc1t         @1
        qmfc2.ni     t0, vf14
        mtc1         t0, $f2
        c.lt.s       $f2, $f0
        bc1t         @3
        c.lt.s       $f1, $f2
        bc1t         @3
        vnop
    @1:
        addi         t0, zero, 0x1
        sw           t0, 0x0(a0)
        vsub.xz      vf5, vf3, vf10
        vsub.xyz     vf1, vf1, vf1
        vnop
        vnop
        vmul.xz      vf6, vf5, vf5
        vnop
        vnop
        vnop
        vaddz.x      vf6, vf6, vf6z
        vnop
        vnop
        vnop
        qmfc2.ni     t0, vf6
        mtc1         t0, $f1
        c.eq.s       $f0, $f1
        bc1t         @2
        vnop
        vsqrt        Q, vf6x
        vaddw.x      vf7, vf0, vf4w
        vnop
        vnop
        vnop
        vnop
        vwaitq
        vaddq.x      vf6, vf0, Q
        vsubq.x      vf7, vf7, Q
        vnop
        vnop
        vnop
        vnop
        vnop
        vnop
        vnop
        vdiv         Q, vf7x, vf6x
        vnop
        vnop
        vnop
        vnop
        vnop
        vwaitq
        vmulq.xz     vf1, vf5, Q
    @2:
        sqc2         vf1, 0x0(v0)
    @3:
    }
    return result->chk;
}

/** Tests the columns @p clm0 and @p clm1 against each other. @return result->chk. */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
int clCheckSubColumnToColumn(struct _CL_HITRESULT *result, float (*clm0)[4], float (*clm1)[4]) {
    asm {
        .set noreorder
        addiu        v0, a0, 0x20
        sw           zero, 0x0(a0)
        lqc2         vf1, 0x0(clm0)
        lqc2         vf2, 0x10(clm0)
        lqc2         vf3, 0x0(clm1)
        lqc2         vf4, 0x10(clm1)
        vaddw.xz     vf5, vf1, vf2w
        vsubw.xz     vf6, vf3, vf4w
        vaddw.xz     vf7, vf3, vf4w
        vsubw.xz     vf8, vf1, vf2w
        vnop
        vnop
        ctc2.ni      zero, vi16
        vsub.xz      vf5, vf5, vf6
        vsub.xz      vf6, vf7, vf8
        vsub.y       vf7, vf1, vf4
        vsub.y       vf8, vf3, vf2
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni      t0, vi16
        andi         t0, t0, 0x80
        bnez         t0, @1
        vnop
        vsub.z       vf6, vf3, vf1
        vsub.xz      vf5, vf3, vf1
        vadd.w       vf7, vf2, vf4
        sub.s        $f0, $f0, $f0
        vaddz.x      vf6, vf0, vf6z
        vmula.x      ACC, vf5, vf5
        vaddw.x      vf7, vf0, vf7w
        vsub.yw      vf1, vf1, vf1
        vmadd.x      vf6, vf6, vf6
        vnop
        vnop
        vnop
        vsqrt        Q, vf6x
        vnop
        vnop
        vnop
        vnop
        vnop
        vwaitq
        vsubq.x      vf8, vf7, Q
        vaddq.x      vf6, vf0, Q
        vnop
        vnop
        qmfc2.ni     t0, vf8
        mtc1         t0, $f1
        c.lt.s       $f1, $f0
        bc1t         @1
        vaddw.x      vf8, vf8, vf0w
        vnop
        vnop
        vnop
        vdiv         Q, vf8x, vf6x
        vnop
        vnop
        vnop
        vnop
        vnop
        vwaitq
        vmulq.xz     vf5, vf5, Q
        vnop
        addi         t0, zero, 0x1
        sw           t0, 0x0(a0)
        sqc2         vf5, 0x0(v0)
    @1:
    }
    return result->chk;
}
