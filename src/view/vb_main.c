/*
 * vb_main.c: the view base: world/view/screen matrices (VbWvsMatrix), the screen parameters,
 * coordinate hierarchies (local-to-world matrices) and the reference view that builds the
 * world-to-view matrix from a camera position, target and roll.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static inline void sqvector(float *v0, float *v1) {
    __asm__ __volatile__("
    lqc2       vf4, 0x0(%1)
    vmove.xyzw vf5, vf4
    vmul.xyzw  vf4, vf4, vf5
    sqc2       vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}

inline void scalevector(float *v0, float *v1, float t) {
    asm __volatile__("
    mfc1      $15, %2
    lqc2      vf4, 0x0(%1)
    qmtc2.ni  $15, vf5
    vmulx.xyz vf4, vf4, vf5x
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "f"(t) : "$15");
}

inline void inversmatrix(float (*m0)[4], float (*m1)[4]) {
    asm __volatile__("
    lq          $8, 0x0(%1)
    lq          $9, 0x10(%1)
    lq          $10, 0x20(%1)
    lqc2        vf4, 0x30(%1)
    pextlw      $11, $9, $8
    pextuw      $12, $9, $8
    pextlw      $13, $0, $10
    pextuw      $14, $0, $10
    pcpyld      $8, $13, $11
    pcpyud      $9, $11, $13
    pcpyld      $10, $14, $12
    qmtc2.ni    $8, vf5
    qmtc2.ni    $9, vf6
    qmtc2.ni    $10, vf7
    vmulax.xyz  ACC, vf5, vf4x
    vmadday.xyz ACC, vf6, vf4y
    vmaddz.xyz  vf4, vf7, vf4z
    sq          $8, 0x0(%0)
    sq          $9, 0x10(%0)
    vsub.xyz    vf4, vf0, vf4
    sq          $10, 0x20(%0)
    sqc2        vf4, 0x30(%0)
    " : : "r"(m0), "r"(m1) : "$8", "$9", "$10", "$11", "$12", "$13", "$14");
}

unsigned int vbNRAAngRgl[4] = { 0x40490FDB, 0xC0490FDB, 0x40C90FDB, 0x3E22F983 };
struct _VbWVSMATRIX VbWvsMatrix;
struct _VbCOORDINATESTACK VbCoordinateStack;
struct _VbSCREENINFO VbScreenInfo;

/** Rebuilds the view-to-screen matrix from the screen parameters (VbScreenInfo). */
void vbCalcViewScreenMatrix(void) {
    sceVu0ViewScreenMatrix(VbWvsMatrix.vsm, VbScreenInfo.scr_z, VbScreenInfo.sx / 587.2727f, VbScreenInfo.sy / 484.0f,
                           VbScreenInfo.cx, VbScreenInfo.cy, VbScreenInfo.zmin, VbScreenInfo.zmax, VbScreenInfo.nearz,
                           VbScreenInfo.farz);
}

/** Initializes a coordinate system under a parent (identity matrices).
 * @param super parent coordinate system, or NULL
 * @param coord the coordinate system */
void vbInitCoordinate(struct _VbCOORDINATE *super, struct _VbCOORDINATE *coord) {
    coord->flg = 0;
    coord->super = super;
    unitmatrix(coord->coord);
    unitmatrix(coord->lw);
}

/** World-to-screen matrix = view-to-screen x world-to-view. */
void vbSetWorldScreenMatrix(void) {
    float work[4][4];

    mcopy(VbWvsMatrix.wvm, work);
    shMulMatrix(VbWvsMatrix.wsm, VbWvsMatrix.vsm, work);
}

/** Computes the local-to-world matrix of a coordinate system, walking up its parents and
 * reusing the parents' matrices that are already up to date.
 * @param coord the coordinate system
 * @param fflip the flag value that marks an up-to-date matrix this pass */
void vbGetLw(struct _VbCOORDINATE *coord, int fflip) {
    int pass;
    int i;
    float m0[4][4];
    struct _VbCOORDINATE *crd0;

    VbCoordinateStack.use = 0;
    crd0 = coord;
    do {
        VbCoordinateStack.coordstk[VbCoordinateStack.use] = crd0;
        VbCoordinateStack.use++;
        crd0 = crd0->super;
    } while (crd0);
    unitmatrix(m0);
    i = 0;
    pass = 0;
    while (VbCoordinateStack.use) {
        VbCoordinateStack.use--;
        crd0 = VbCoordinateStack.coordstk[VbCoordinateStack.use];
        if (crd0->flg == fflip && !pass) {
            mcopy(crd0->lw, m0);
        } else {
            pass = 1;
            shMulMatrix(crd0->lw, m0, crd0->coord);
            mcopy(crd0->lw, m0);
            crd0->flg = fflip;
        }
        i++;
    }
}

/** Builds the world-to-view matrix from a reference view (camera position vp, target vr, roll rz,
 * optionally inside a parent coordinate system) and updates the world-to-screen matrix.
 * @param rview the reference view
 * @return 0 if the camera and target coincide, else 1 */
int vbSetRefView(struct _VbRVIEW *rview) {
    float work0[4][4];
    float work1[4][4];
    float work2[4][4];
    float t[4];
    float t2[4];
    float len;
    float xzlen;
    float rz;

    _shSubVector(t, rview->vr, rview->vp);
    sqvector(t2, t);
    len = t2[0] + t2[1] + t2[2];
    len = _shSqrt(len);
    if (len == 0.0f) {
        return 0;
    }
    xzlen = t2[0] + t2[2];
    xzlen = _shSqrt(xzlen);
    rz = rview->rz;
    unitmatrix(work0);
    unitmatrix(work2);
    if (rz != 0.0f) {
        unitmatrix(work1);
        work1[0][0] = shCosF(rz);
        work1[1][0] = shSinF(rz);
        work1[0][1] = -1.0f * work1[1][0];
        work1[1][1] = work1[0][0];
        shMulMatrix(work2, work0, work1);
    }
    unitmatrix(work0);
    work0[2][2] = work0[1][1] = xzlen / len;
    work0[1][2] = t[1] / len;
    work0[2][1] = -1.0f * work0[1][2];
    shMulMatrix(work1, work2, work0);
    if (xzlen != 0.0f) {
        unitmatrix(work0);
        work0[0][0] = t[2] / xzlen;
        work0[0][2] = t[0] / xzlen;
        work0[2][0] = -1.0f * work0[0][2];
        work0[2][2] = work0[0][0];
        shMulMatrix(work2, work1, work0);
        mcopy(work2, work1);
    }
    scalevector(t, rview->vp, -1.0f);
    t[3] = 1.0f;
    vbApplyMatrixWithoutTr(work1[3], work1, t);
    if (rview->super) {
        vbGetLw(rview->super, 1);
        inversmatrix(work0, rview->super->lw);
        shMulMatrix(work2, work0, work1);
        mcopy(work2, VbWvsMatrix.wvm);
    } else {
        mcopy(work1, VbWvsMatrix.wvm);
    }
    vbSetWorldScreenMatrix();
    return 1;
}

/** Returns ang wrapped into [-pi, pi] (VU0 ftoi0/itof0). Asm in a C function's body: the DWARF has
 * ang and ret, which a function written wholly in asm doesn't get (sh_vu0.c's show (void)). Written
 * GCC-style like the rest of the file; an `asm {}` block gives the same code, DWARF and line table.
 * The splitter makes the `j` target a separate `func_0019594C` label, which the C object can't
 * provide; the code itself matches.
 * @param ang the angle in radians
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
float vbNormalizeRadianAngle(float ang) {
    float ret;

    __asm__ __volatile__("
        .set noreorder
        la         t0, vbNRAAngRgl
        lwc1       $f1, 0x0(t0)
        lqc2       vf2, 0x0(t0)
        c.lt.s     $f1, %1
        bc1t       L1
        lwc1       $f2, 0x4(t0)
        c.lt.s     %1, $f2
        bc1f       L2
        vaddy.x    vf2, vf0, vf2y
    L1:
        mfc1       t1, %1
        qmtc2.ni   t1, vf1
        vaddx.x    vf3, vf1, vf2x
        sub.s      $f0, $f0, $f0
        vmulw.x    vf3, vf3, vf2w
        vftoi0.xyzw vf3, vf3
        vitof0.xyzw vf3, vf3
        adda.s     %1, $f0
        lwc1       $f2, 0x8(t0)
        qmfc2.ni   t1, vf3
        mtc1       t1, $f1
        j          L2
        msub.s     %1, $f1, $f2
    L2:
        mov.s      %0, %1
        .set reorder
    " : "=f"(ret) : "f"(ang));
    return ret;
}

/** m0 = the transpose of m1's rotation part, with m1's translation row kept. GCC-style inline asm
 * (the original's DWARF has m0 and m1).
 * @param m0 result
 * @param m1 the matrix */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void vbTransposeMatrixWithoutTr(float (*m0)[4], float (*m1)[4]) {
    __asm__ __volatile__("
    lq       t8, 0x30(%1)
    lq       t0, 0x0(%1)
    lq       t1, 0x10(%1)
    lq       t2, 0x20(%1)
    qmfc2.ni t3, vf0
    pextlw   t4, t1, t0
    pextuw   t5, t1, t0
    pextlw   t6, t3, t2
    pextuw   t7, t3, t2
    pcpyld   t0, t6, t4
    pcpyud   t1, t4, t6
    pcpyld   t2, t7, t5
    sq       t0, 0x0(%0)
    sq       t1, 0x10(%0)
    sq       t2, 0x20(%0)
    sq       t8, 0x30(%0)
    " : : "r"(m0), "r"(m1));
}

/** v0 = m0 x v1 without the translation row (w passed through). GCC-style inline asm
 * (the original's DWARF has v0, m0 and v1).
 * @param v0 result
 * @param m0 the matrix
 * @param v1 the vector */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void vbApplyMatrixWithoutTr(float *v0, float (*m0)[4], float *v1) {
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
