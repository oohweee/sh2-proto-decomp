/*
 * fog_blow.c: fog blown out of vents. In three rooms (0x8E-0x90) fixed blow
 * points emit extra particles (slots 500 and up of the fog particles) that rise,
 * grow and fade; they are moved and packed alongside the main fog (fog.c).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "sh_vu0.h"
#include "fog_helpers.h"
#include "fi_libvu0_inline.h"

struct FOG_BLOW_POINT blow_point_ps185[14] = {
{ { 87700.0f, -300.0f, -59400.0f, 0.0f }, { 0.0f, 1.0f, -3.0f, 10.0f } },
    { { 90100.0f, -300.0f, -59400.0f, 0.0f }, { 0.0f, 1.0f, -3.0f, 10.0f } },
    { { 92500.0f, -300.0f, -59400.0f, 0.0f }, { 0.0f, 1.0f, -3.0f, 10.0f } },
    { { 94900.0f, -300.0f, -59400.0f, 0.0f }, { 0.0f, 1.0f, -3.0f, 10.0f } },
    { { 97300.0f, -300.0f, -59400.0f, 0.0f }, { 0.0f, 1.0f, -3.0f, 10.0f } },
    { { 99700.0f, -300.0f, -59400.0f, 0.0f }, { 0.0f, 1.0f, -3.0f, 10.0f } },
    { { 102100.0f, -300.0f, -59400.0f, 0.0f }, { 0.0f, 1.0f, -3.0f, 10.0f } },
    { { 87700.0f, -300.0f, -60600.0f, 0.0f }, { 0.0f, 0.0f, 3.0f, 10.0f } },
    { { 90100.0f, -300.0f, -60600.0f, 0.0f }, { 0.0f, 1.0f, 3.0f, 10.0f } },
    { { 92500.0f, -300.0f, -60600.0f, 0.0f }, { 0.0f, 1.0f, 3.0f, 10.0f } },
    { { 94900.0f, -300.0f, -60600.0f, 0.0f }, { 0.0f, 1.0f, 3.0f, 10.0f } },
    { { 97300.0f, -300.0f, -60600.0f, 0.0f }, { 0.0f, 1.0f, 3.0f, 10.0f } },
    { { 99700.0f, -300.0f, -60600.0f, 0.0f }, { 0.0f, 1.0f, 3.0f, 10.0f } },
    { { 102100.0f, -300.0f, -60600.0f, 0.0f }, { 0.0f, 1.0f, 3.0f, 10.0f } },
};

struct FOG_BLOW_POINT blow_point_ps189[10] = {
    { { 101600.0f, -975.0f, -99600.0f, 0.0f }, { -5.0f, 1.0f, 0.0f, 40.0f } },
    { { 100950.0f, -50.0f, -99750.0f, 0.0f }, { -4.0f, 0.0f, 2.0f, 40.0f } },
    { { 100850.0f, -50.0f, -100450.0f, 0.0f }, { -5.0f, 0.0f, -1.0f, 40.0f } },
    { { 101400.0f, -300.0f, -100800.0f, 0.0f }, { 0.0f, 2.0f, 0.0f, 40.0f } },
    { { 101000.0f, -650.0f, -101200.0f, 0.0f }, { 0.0f, 2.0f, 0.0f, 40.0f } },
    { { 100500.0f, -50.0f, -100600.0f, 0.0f }, { -1.0f, 0.0f, 5.0f, 40.0f } },
    { { 99600.0f, -50.0f, -100750.0f, 0.0f }, { 2.0f, 0.0f, 3.0f, 40.0f } },
    { { 99000.0f, -300.0f, -101250.0f, 0.0f }, { 0.0f, 2.0f, 0.0f, 40.0f } },
    { { 99150.0f, -50.0f, -100900.0f, 0.0f }, { 3.0f, 0.0f, 1.0f, 40.0f } },
    { { 99000.0f, -50.0f, -100200.0f, 0.0f }, { 3.0f, 0.0f, 2.0f, 40.0f } },
};

struct FOG_BLOW_POINT blow_point_ps193[16] = {
    { { 142900.0f, -1400.0f, -96800.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -97200.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -98000.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -98400.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -99400.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -99800.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -100600.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -101000.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -102000.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -102400.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -103200.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142900.0f, -1400.0f, -103600.0f, 0.0f }, { -3.0f, 2.0f, 0.0f, 60.0f } },
    { { 142200.0f, -1400.0f, -104100.0f, 0.0f }, { 0.0f, 2.0f, 3.0f, 60.0f } },
    { { 141800.0f, -1400.0f, -104100.0f, 0.0f }, { 0.0f, 2.0f, 3.0f, 60.0f } },
    { { 138600.0f, -1400.0f, -104100.0f, 0.0f }, { 0.0f, 2.0f, 3.0f, 60.0f } },
    { { 138200.0f, -1400.0f, -104100.0f, 0.0f }, { 0.0f, 2.0f, 3.0f, 60.0f } },
};

struct FOG_WORK2 fwork2;

/** Removes every blown particle. */
void fogInitParticle2(void) {
    fwork2.PartNum = 0;
}

/** Selects the blow points of room `room` (none for rooms without any) and resets their timers. */
void fogSetBlowPoint(int room) {
    int i;

    switch (room) {
    case 0x8E:
        fwork2.BlowPoint = blow_point_ps185;
        fwork2.BlowPointNum = 14;
        fwork2.BlowInterval = 20;
        break;
    case 0x8F:
        fwork2.BlowPoint = blow_point_ps189;
        fwork2.BlowPointNum = 10;
        fwork2.BlowInterval = 30;
        break;
    case 0x90:
        fwork2.BlowPoint = blow_point_ps193;
        fwork2.BlowPointNum = 16;
        fwork2.BlowInterval = 60;
        break;
    default:
        fwork2.BlowPointNum = 0;
        break;
    }
    fwork2.BlowPointID = room;
    i = 0;
    while (i < 16) {
        fwork2.BlowTimer[i] = 0;
        i++;
    }
}

/**
 * Emits a particle from blow point `blow` if it is within 2000 units (x and z) of the fog volume
 * and fewer than 200 are alive.
 */
void fogBlow(struct FOG_BLOW_POINT *blow) {
    struct FOG_PART_DATA *pd;
    float vec[4];

    if (fwork2.PartNum > 199) {
        return;
    }
    _shSubVector(vec, blow->pos, fwork.LocalPosV);
    if (!(fabsf(vec[0]) <= 2000.0f && fabsf(vec[2]) <= 2000.0f)) {
        return;
    }
    pd = &fwork.Part[500 + fwork2.PartNum++];
    _sceVu0CopyVectorXYZ(pd->pos, blow->pos);
    shRandV_Scale(vec, 0.1f);
    _shAddVector(pd->mv, blow->vec, vec);
    pd->mv[3] = 0.0f;
    *(u_long128 *)pd->dd = 0;
    *(u_long128 *)pd->degree = 0;
    *(u_long128 *)&pd->tdx = 0;
    pd->tdx = shRandF();
    pd->tdy = shRandF();
    *(u_long128 *)&pd->alp_now = 0;
    pd->alp_now = fwork.Alpha / 4;
    pd->dd[0] = blow->vec[3];
}

/**
 * Per-frame update of the blown particles: emits new ones on each blow point's timer, then moves,
 * fades and removes them.
 *
 * Matching: `if (pd->erase != 0)` rather than `if (pd->erase)` (the same code) is chosen for the
 * float-constant order: with it the shSway1f() call below loads -alp / 32.0f before 0.02f, as the
 * original does (docs/toolchain.md, "Root cause").
 */
void fogMoveParticle2(void) {
    struct FOG_PART_DATA *pd;
    struct FOG_PART_DATA *pdo;
    int n;
    int df;
    int n1;
    int room;
    float alp;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    room = RoomNameJms();
    if (fwork2.BlowPointID != room) {
        fogSetBlowPoint(room);
    }
    fwork2.Gravity = 4.9f * shGetDT();
    fwork2.Expand = 50.0f * shGetDT();
    df = shGetDF();
    for (n = 0; n < fwork2.BlowPointNum; n++) {
        if ((fwork2.BlowTimer[n] -= df) <= 0) {
            n1 = -fwork2.BlowInterval / 2;
            fwork2.BlowTimer[n] = fwork2.BlowInterval + ftoi(shSway1f(itof(n1), 1.0f));
            fogBlow(&fwork2.BlowPoint[n]);
        }
    }
    pdo = &fwork.Part[500];
    for (n = 0; n < fwork2.PartNum;) {
        fogCopyPart(pdo, pd);
        if (pd->erase != 0) {
            pd->alp_now *= 0.75f;
            if (pd->erase == 100 || pd->alp_now <= 1.0f) {
                if (--fwork2.PartNum == 0) {
                    break;
                }
                if (n < fwork2.PartNum) {
                    fogCopyPart(&fwork.Part[fwork2.PartNum + 499], pd);
                }
            }
        } else if (pd->bounce > 0) {
            pd->erase = 1;
        }
        fog_part_blow();
        alp = fwork.Alpha;
        pd->alp_add = 0.9f * (pd->alp_add + shSway1f(-alp / 32.0f, 0.02f) - 0.01f * (pd->alp_now - alp));
        pd->alp_now += pd->alp_add;
        if (pd->alp_now < 0.0f) {
            pd->alp_now = 0.0f;
        }
        if (pd->alp_now > 127.0f) {
            pd->alp_now = 127.0f;
        }
        fog_part_clamp2();
        fogCopyPart(pd, pdo);
        n++;
        pdo++;
    }
}

/** Moves the particle in the scratchpad work slot: gravity, growth, drag and wind. */
void fog_part_blow(void) {
    struct FOG_PART_DATA *pd;
    float *FVector;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    FVector = (float *)0x70003FF0;
    pd->mv[1] += fwork2.Gravity;
    pd->dd[0] += fwork2.Expand;
    if (pd->dd[0] > fwork.PartSize) {
        pd->dd[0] = fwork.PartSize;
    }
    _shScaleVectorXYZ(pd->mv, pd->mv, 0.95f);
    _shScaleVector(FVector, pd->mv, fwork.SpeedLevel);
    _shAddVector(pd->pos, pd->pos, FVector);
    pd->bounce = 0;
}

/** Clamps the particle in the scratchpad work slot to the fog volume (VU0 asm). */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fog_part_clamp2(void) {
    struct FOG_PART_DATA *pd;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    __asm__ __volatile__("
    .set noreorder
    lqc2         vf4, 0x0(%0)
    lqc2         vf5, 0x50(%0)
    vsub.xyz     vf4, vf4, vf29
    vabs.xz      vf6, vf4
    vsuby.xz     vf6, vf6, vf24y
    vnop
    vnop
    vnop
    vnop
    vnop
    addi         t1, zero, 0xA0
    cfc2.ni      t0, vi17
    and          t0, t0, t1
    addi         t2, zero, 0x64
    bnel         t0, t1, clamp2_1
    sw           t2, 0x5C(%0)
clamp2_1:
    vaddy.xz     vf6, vf4, vf24y
    vsubx.xz     vf7, vf6, vf23x
    qmfc2.ni     t0, vf7
    vmuly.xz     vf6, vf6, vf23y
    sll          t1, t0, 0
    bltzl        t1, clamp2_2
    vmulx.x      vf5, vf5, vf6x
clamp2_2:
    pexcw        t1, t0
    bltzl        t1, clamp2_3
    vmulz.x      vf5, vf5, vf6z
clamp2_3:
    vsuby.xz     vf6, vf4, vf24y
    vaddx.xz     vf7, vf6, vf23x
    vsub.xz      vf6, vf0, vf6
    qmfc2.ni     t0, vf7
    vmuly.xz     vf6, vf6, vf23y
    sll          t1, t0, 0
    bgtzl        t1, clamp2_4
    vmulx.x      vf5, vf5, vf6x
clamp2_4:
    pexcw        t1, t0
    bgtzl        t1, clamp2_5
    vmulz.x      vf5, vf5, vf6z
clamp2_5:
    qmfc2.ni     t0, vf5
    sw           t0, 0x48(%0)
    vadd.xyz     vf4, vf4, vf29
    sqc2         vf4, 0x0(%0)
    .set reorder
    " : : "r"(pd));
}

/** Fills the sprite packets of the blown particles, starting at `ppos`. */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fogMakePacket2(u_long128 *ppos) {
    if (!(fwork.Flag & 0x100) || fwork2.PartNum <= 0) {
        return;
    }
    __asm__ __volatile__("
    .set noreorder
    xor          t0, t0, t0
    addu         t1, zero, %1
    addu         t2, zero, %4
packet2_1:
    lqc2         vf4, 0x0(t1)
    vmulax.xyzw  ACC, vf12, vf4x
    vmadday.xyzw ACC, vf13, vf4y
    vmaddaz.xyzw ACC, vf14, vf4z
    vmaddw.xyzw  vf5, vf15, vf4w
    vdiv         Q, vf0w, vf5w
    lqc2         vf29, 0x40(t1)
    vaddz.xy     vf8, vf0, vf29z
    qmfc2.ni     t4, vf8
    blez         t4, packet2_10
    nop
    vwaitq
    vmulq.xyz    vf5, vf5, Q
    vaddq.x      vf6, vf0, Q
    vaddz.y      vf10, vf0, vf5z
    vsub.xyw     vf9, vf5, vf20
    qmfc2.ni     t4, vf10
    vmulx.z      vf8, vf20, vf6x
    bltz         t4, packet2_10
    vabs.xyw     vf9, vf9
    vsubz.xy     vf9, vf9, vf8z
    vsub.xyw     vf8, vf21, vf9
    vnop
    vnop
    vnop
    vnop
    vnop
    cfc2.ni      t4, vi16
    andi         t4, t4, 0x2
    bnez         t4, packet2_10
    nop
    beql         %2, zero, packet2_2
    addi         t3, zero, 0x80
    vsub.xyz     vf8, vf4, vf27
    vmul.xyz     vf8, vf8, vf8
    vaddy.x      vf8, vf8, vf8y
    vaddz.x      vf8, vf8, vf8z
    vsqrt        Q, vf8x
    vwaitq
    vmulq.y      vf8, vf25, Q
    vsuby.x      vf8, vf25, vf8y
    vftoi0.x     vf8, vf8
    qmfc2.ni     t3, vf8
    sll          t3, t3, 0
    blez         t3, packet2_10
    nop
packet2_2:
    lqc2         vf8, 0x20(t1)
    vdiv         Q, vf0w, vf8x
    vadd.x       vf22, vf0, vf8
    vmulw.x      vf8, vf8, vf26w
    vaddx.z      vf22, vf0, vf8x
    vwaitq
    vmulq.w      vf22, vf0, Q
    vsub.y       vf8, vf23, vf4
    addi         t7, zero, 0x64
    qmfc2.ni     t4, vf8
    blezl        t4, packet2_10
    sw           t7, 0x5C(t1)
    vaddw.xy     vf7, vf0, vf0w
    vsuby.x      vf8, vf22, vf8y
    qmfc2.ni     t4, vf8
    sll          t4, t4, 0
    blez         t4, packet2_3
    vaddy.x      vf8, vf0, vf8y
    vmulw.x      vf7, vf8, vf22w
    vsubz.x      vf8, vf7, vf7z
    qmfc2.ni     t4, vf8
    sll          t4, t4, 0
    bgez         t4, packet2_3
    vaddx.y      vf8, vf0, vf7x
    vmulw.y      vf7, vf8, vf7w
packet2_3:
    vaddw.y      vf8, vf0, vf5w
    vsub.y       vf8, vf8, vf24
    qmfc2.ni     t4, vf8
    bgez         t4, packet2_4
    vsubx.w      vf8, vf5, vf24x
    vmulz.w      vf8, vf8, vf24z
    vmulw.z      vf29, vf29, vf8w
packet2_4:
    vmulx.z      vf29, vf29, vf7x
    vaddz.x      vf11, vf0, vf5z
    vftoi0.x     vf11, vf11
    qmfc2.ni     t7, vf11
    lqc2         vf8, 0x10(t1)
    vmulz.x      vf6, vf6, vf22z
    vmulax.xy    ACC, vf16, vf8x
    vmadday.xy   ACC, vf17, vf8y
    vmaddz.xy    vf8, vf18, vf8z
    vmuly.x      vf7, vf8, vf8y
    qmfc2.ni     t4, vf7
    sll          t4, t4, 0
    bltzl        t4, packet2_5
    vsub.x       vf7, vf0, vf7
packet2_5:
    vftoi4.xy    vf11, vf5
    qmfc2.ni     t5, vf11
    pexch        t5, t5
    pextlw       t5, t7, t5
    sd           t5, 0x28(t2)
    vsqrt        Q, vf7x
    vabs.xy      vf8, vf8
    vaddz.x      vf7, vf0, vf26z
    vwaitq
    vmulq.x      vf7, vf7, Q
    vsqrt        Q, vf8x
    vaddx.xy     vf9, vf0, vf26x
    vwaitq
    vmulq.x      vf9, vf9, Q
    vsqrt        Q, vf8y
    vwaitq
    vmulq.y      vf9, vf9, Q
    bltzl        t4, packet2_6
    vsub.x       vf7, vf0, vf7
packet2_6:
    vaddw.xy     vf10, vf0, vf0w
    vsubx.xy     vf10, vf10, vf7x
    vsub.xy      vf10, vf10, vf9
    vmaxy.xy     vf10, vf10, vf26y
    vmulx.xy     vf10, vf10, vf6x
    vmul.y       vf10, vf10, vf22
    vsub.xy      vf11, vf5, vf10
    vftoi4.xy    vf11, vf11
    qmfc2.ni     t4, vf11
    pexch        t4, t4
    pextlw       t4, t7, t4
    sd           t4, 0x40(t2)
    sd           t4, 0x80(t2)
    vmul.y       vf10, vf10, vf7
    vadd.xy      vf11, vf5, vf10
    vftoi4.xy    vf11, vf11
    qmfc2.ni     t4, vf11
    pexch        t4, t4
    pextlw       t4, t7, t4
    sd           t4, 0x60(t2)
    vaddw.xy     vf10, vf0, vf0w
    vaddx.xy     vf10, vf10, vf7x
    vsub.xy      vf10, vf10, vf9
    vmaxy.xy     vf10, vf10, vf26y
    vmulx.xy     vf10, vf10, vf6x
    vmul.y       vf10, vf10, vf22
    vsub.x       vf10, vf0, vf10
    vsub.xy      vf11, vf5, vf10
    vftoi4.xy    vf11, vf11
    qmfc2.ni     t4, vf11
    pexch        t4, t4
    pextlw       t4, t7, t4
    sd           t4, 0x50(t2)
    vmul.y       vf10, vf10, vf7
    vadd.xy      vf11, vf5, vf10
    vftoi4.xy    vf11, vf11
    qmfc2.ni     t4, vf11
    pexch        t4, t4
    pextlw       t4, t7, t4
    sd           t4, 0x70(t2)
    vsub.xyz     vf9, vf4, vf28
    vmul.xyz     vf8, vf9, vf9
    vaddy.x      vf8, vf8, vf8y
    vaddz.x      vf8, vf8, vf8z
    vrsqrt       Q, vf0w, vf8x
    lqc2         vf8, 0x30(t1)
    vwaitq
    vmulq.xyz    vf9, vf9, Q
    vsub.xyz     vf8, vf9, vf8
    sqc2         vf9, 0x30(t1)
    vmulax.xy    ACC, vf16, vf8x
    vmadday.xy   ACC, vf17, vf8y
    vmaddz.xy    vf8, vf18, vf8z
    vmulz.xy     vf8, vf8, vf23z
    vsub.xy      vf9, vf29, vf8
    qmfc2.ni     t4, vf9
    vftoi0.xy    vf8, vf9
    vitof0.xy    vf8, vf8
    vsub.xy      vf9, vf9, vf8
    sll          t5, t4, 0
    blezl        t5, packet2_7
    vaddw.x      vf9, vf9, vf0w
packet2_7:
    blezl        t4, packet2_8
    vaddw.y      vf9, vf9, vf0w
packet2_8:
    qmfc2.ni     t4, vf9
    sd           t4, 0x40(t1)
    sd           t4, 0x20(t2)
    vsubx.xy     vf8, vf9, vf23x
    qmfc2.ni     t4, vf8
    sd           t4, 0x38(t2)
    sd           t4, 0x78(t2)
    vadd.x       vf8, vf9, vf23
    qmfc2.ni     t4, vf8
    sd           t4, 0x48(t2)
    vmulx.y      vf8, vf7, vf23x
    vadd.y       vf8, vf9, vf8
    qmfc2.ni     t4, vf8
    sd           t4, 0x58(t2)
    vsub.x       vf8, vf9, vf23
    qmfc2.ni     t4, vf8
    sd           t4, 0x68(t2)
    slti         t4, t3, 0x20
    beqz         t4, packet2_9
    nop
    qmtc2.ni     t4, vf8
    vitof0.x     vf8, vf8
    vmulx.z      vf29, vf29, vf8x
    vmulw.z      vf29, vf29, vf25w
packet2_9:
    qmtc2.ni     t3, vf8
    vitof0.x     vf8, vf8
    vmulz.w      vf8, vf30, vf29z
    vmulx.xyz    vf8, vf30, vf8x
    vftoi0.xyzw  vf8, vf8
    qmfc2.ni     t4, vf8
    ppach        t4, zero, t4
    lui          t5, 0xFF
    ppacb        t4, zero, t4
    ori          at, zero, 0xFFFF
    addu         t5, t5, at
    srl          t6, t4, 24
    sw           t4, 0x18(t2)
    and          t5, t5, t4
    andi         t6, t6, 0xFF
    sw           t5, 0x30(t2)
    beqz         t6, packet2_10
    vaddw.x      vf11, vf0, vf5w
    lw           t4, 0x0(%3)
    vsub.y       vf11, vf0, vf0
    addu         t5, zero, t2
    vftoi4.xy    vf11, vf11
    addu         t4, t4, t7
    addiu        t7, zero, 0x1
    qmfc2.ni     t6, vf11
    jal          spkSetOTPacketS_asm
    sw           t4, 0x0(%3)
    addiu        t2, t2, 0x90
packet2_10:
    addi         t0, t0, 0x1
    bne          t0, %0, packet2_1
    addiu        t1, t1, 0x60
    .set reorder
    " : : "r"(fwork2.PartNum), "r"(fwork.Part + 500), "r"(fwork.Flag & 4), "r"(&fwork.SumW), "r"(ppos));
}
