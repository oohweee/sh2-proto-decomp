/*
 * fog.c: the fog particle system. Up to 700 particles drift with the wind, bounce
 * off collision walls and objects, and are drawn as sprites through the spack
 * packet builder. The per-particle work runs on the scratchpad (0x70000000) in
 * VU0 macro-mode asm.
 *
 * Most particle routines are C around inline VU0 asm blocks that use C variables:
 * the fog_part_* passes set their scratchpad pointers in C (the DWARF has the
 * locals, and the line table puts the pointer setup on C lines before the asm's
 * own lines). Where the original has no locals for the asm's inputs and puts all
 * their loads on the asm statement's line, the asm is a GCC-style statement taking
 * them as operands.
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "sh_vu0.h"
#include "fog_helpers.h"
#include "fi_libvu0_inline.h"
#include "sdk/eekernel.h"

typedef unsigned int u_int;

#define WALL_MAX 188
#define OBJ_MAX 168
#define PART_MAX 700

#define PK_ADD(v) (*spack.pos++ = (v))

/*
 * File-local asm helpers: the shared headers (asm_helpers.h, sh_vu0.h) don't have these.
 * Their original names and header are unknown.
 */

/* rows 0-2, row 3 = (0, 0, 0, 1) */
inline void mcopy3(void *s, void *d) {
    asm {
        lq   t7, 0x0(s)
        lq   t6, 0x10(s)
        sq   t7, 0x0(d)
        sq   t6, 0x10(d)
        lq   t7, 0x20(s)
        sqc2 vf0, 0x30(d)
        sq   t7, 0x20(d)
    }
}

/* floor toward -inf (off by one for negative integers) */
inline int ffloor(float f) {
    int r;

    asm {
        mfc1    r, f
        addi    t7, zero, 1
        slt     r, r, zero
        cvt.w.s f, f
        movz    t7, zero, r
        mfc1    r, f
        sub     r, r, t7
    }
    return r;
}

/* stores the S, T pair of an ST register at pk + 0x20 */
inline void set_st20(void *pk, float t, float s) {
    asm {
        daddiu t0, zero, 0x20
        addu   t0, t0, pk
        swc1   s, 0x0(t0)
        swc1   t, 0x4(t0)
    }
}

/* set_st20 at pk + 0x30, with the arguments the other way round */
inline void set_st30(void *pk, float s, float t) {
    asm {
        daddiu t0, zero, 0x30
        addu   t0, t0, pk
        swc1   s, 0x0(t0)
        swc1   t, 0x4(t0)
    }
}

static struct FOG_ASM_DATA1 fog_asm_data1 = {
    300.0f, 310.0f, 0.1f, 0.9f, 100.0f, 0.01f, 127.0f, 0.0f,
    -1.0f, 0.02f, 0.01f, 0.9f, 300.0f, 0.0033333334f, 2000.0f, 1.0f,
    2000.0f, 4000.0f, 0.002f, 0.001f, 1.0f, 1.0f, 0.0f, 1.0f,
    2000.0f, 2000.0f, 4000.0f, 0.0f,
};
static struct FOG_ASM_DATA2 fog_asm_data2 = {
    2048.0f, 2048.0f, 153600.0f, 16415.0f, 256.0f, 256.0f, 0.0f, 16351.0f,
    300.0f, 1.3333334f, 153600.0f, 0.0033333334f, 0.048828125f, 0.0f, 0.25f, 128.0f,
    64.0f, 576.0f, 0.001953125f, 0.0078125f, 112.0f, 0.125f, 32.0f, 0.03125f,
    0.125f, 0.25f, 0.0078125f, 512.0f,
};
static struct FOG_ASM_DATA_P fog_asm_data_p = {
    fwork.Part, pwork.packet, &fog_asm_data2, fwork.WorldScreenM,
    fwork.WorldViewM, fwork.CameraPosV, fwork.WorldPosV, fwork.LightPosV,
};
static struct FOG_ASM_DATA3 fog_asm_data3 = { 2000.0f, 250.0f, 1.0f, 1.0f };

struct FOG_WORK fwork;
struct FOG_PACK_WORK pwork __attribute__((aligned(64)));

/**
 * Clears the fog work areas, builds the default packets on first use, and sets up the area's
 * environment, collision, colour and particles.
 */
void fogInit(void) {
    shQzero(&fwork, sizeof(fwork));
    shQzero(&fwork2, sizeof(fwork2));
    if (pwork.pk_env == NULL) {
        fog_set_defpacket();
    }
    fog_asm_data_p.packet = (u_long128 *)((u_int)pwork.packet | 0x20000000);
    fogSetAreaEnvironment();
    fogSetCollision();
    fogSetColor(0x80, 0x80, 0x80, 0x80);
    fogInitScreen();
}

/**
 * Builds the fixed GS packets of the fog: one sprite packet per particle, the screen fill, the
 * environment and the TEX0 setup.
 */
void fog_set_defpacket(void) {
    unsigned long giftag[2] = { 0xF400000000008000, 0x052525252D21D210 };
    unsigned long giftag2[2] = { 0x6400000000008000, 0x000000000052D210 };
    int i;
    u_long128 *pos;
    u_long128 *epos;

    SyncDCache(pwork.packet, &pwork.packet[sizeof(pwork.packet) / sizeof(u_long128)]);
    InvalidDCache(pwork.packet, &pwork.pk_env);
    pos = pwork.packet;
    for (i = 0; i < PART_MAX; i++) {
        spkStartPacketS(pos);
        spkOpenGiftagS(giftag);
        PK_ADD(0x5D);
        PK_ADD(0x3F80000080808080);
        spack.pos += 2;
        PK_ADD(0x3F80000000808080);
        spack.pos += 10;
        spkCloseGiftagS();
        pos = spkEndPacketS();
    }
    pwork.pk_screen = pos;
    spkStartPacketS(pos);
    spkOpenGiftagS(giftag2);
    PK_ADD(0x56);
    PK_ADD(0x3F80000080808080);
    spack.pos++;
    PK_ADD(0x00FFFFFF70007000);
    spack.pos++;
    PK_ADD(0x00FFFFFF90009000);
    spkCloseGiftagS();
    pos = spkEndPacketS();
    pwork.pk_env = pos;
    spkStartPacketS(pos);
    spkOpenDGiftagS(0x1000000000008000, 0xE);
    PK_ADD(0);
    PK_ADD(0x3F);
    PK_ADD(0);
    PK_ADD(0x49);
    PK_ADD(1);
    PK_ADD(0x46);
    PK_ADD(0x13A0001C0);
    PK_ADD(0x4E);
    spkCloseGiftagS();
    pos = spkEndPacketS();
    epos = pos;
    spkStartPacketS(pos);
    spkOpenDGiftagS(0x1000000000000000, 0xE);
    pwork.pk_tex0 = spack.pos;
    PK_ADD(0);
    PK_ADD(6);
    PK_ADD(0);
    PK_ADD(8);
    PK_ADD(0x44);
    PK_ADD(0x42);
    PK_ADD(0xFFFFFFFF00000061);
    PK_ADD(0x14);
    PK_ADD(0x50000);
    PK_ADD(0x47);
    spkCloseGiftagS();
    pos = spkEndPacketS();
    spkSetEnvPacket(epos, 1);
    printf("fog packet size = %#x(%dkb)\n", ((u_int)pos & 0x0FFFFFFF) - (u_int)&pwork,
           (((u_int)pos & 0x0FFFFFFF) - (u_int)&pwork) / 1024 + (((((u_int)pos & 0x0FFFFFFF) - (u_int)&pwork) % 1024) ? 1 : 0));
}

/**
 * Switches to the fog environment `edata` (NULL turns the fog off): particle size, range, wind,
 * height limit, layering, alpha and count. For the current environment it only times the wind
 * changes.
 */
void fogSetEnvironment(struct FOG_ENV_DATA *edata) {
    float d;
    float m;
    unsigned char f;
    unsigned char f2;
    char Double;
    int old;

    if (edata == fwork.EnvNow) {
        if ((fwork.WindTimer -= shGetDF()) <= 0) {
            fogChangeWind(fwork.WindDef);
        }
        return;
    }
    fwork.EnvNow = edata;
    if (edata == NULL) {
        fwork.PartNum = 0;
        fwork.EnvNow = NULL;
        return;
    }
    f = edata->Flag;
    m = edata->MaxPos;
    f2 = 0;
    if (m > fwork.MaxPos + 2000.0f) {
        f2 = 1;
    }
    fwork.MaxPos = m;
    fog_asm_data1.maxpos = m;
    fog_asm_data1.maxpos_x2 = 2.0f * m;
    fog_asm_data1.screendiv = 4.0f / m;
    fog_asm_data1.r_maxpos = 2.0f / m;
    fog_asm_data3.maxpos = m;
    d = edata->PartSize;
    fwork.PartSize = d;
    fog_asm_data1.part_size = d;
    fog_asm_data1.wall_range = 10.0f + d;
    fog_asm_data2.part_size = d;
    fog_asm_data2.r_part_size = 1.0f / d;
    d = edata->EscapeRange;
    fwork.EscapeRange = d;
    fog_asm_data1.escape_range = d;
    fog_asm_data1.r_escape_range = 1.0f / d;
    d = edata->FloorY;
    fwork.FloorY = d;
    fog_asm_data2.floor_y = d;
    fwork.WaterY = edata->WaterY;
    old = fwork.WindDef;
    fwork.WindDef = edata->WindDef;
    if (fwork.WindDef != old) {
        if (fwork.Global == 5) {
            fogInitWind();
        } else {
            fogChangeWind(fwork.WindDef);
        }
    }
    if (f & 1) {
        fwork.LimitY = edata->LimitY;
        fwork.LimitY2 = edata->LimitY - edata->LimitHeight;
        fog_asm_data1.r_height = 1.0f / edata->LimitHeight;
    } else {
        fwork.LimitY = -3.4028235e38f;
    }
    fwork.Double = Double = edata->Double;
    if (Double > 1) {
        d = 1 << (Double - 1);
        fog_asm_data3.r_double = 1.0f / d;
    } else {
        d = 1.0f;
        fog_asm_data3.r_double = 1.0f;
    }
    fog_asm_data1.double_rate = 1 << Double;
    d = m / d;
    fog_asm_data1.higher_y = (Double < 2) ? m : d;
    fog_asm_data1.lower_y = (Double == 0) ? m : 0.0f;
    fog_asm_data1.y_max = (Double == 0) ? 2.0f * m : d;
    fog_asm_data1.higher_y2 = 2.0f * ((Double < 2) ? m : d);
    fwork.Alpha = edata->Alpha;
    d = (u_int)edata->Alpha;
    fog_asm_data1.alpha = d;
    fog_asm_data1.minus_alpha = -d / 32.0f;
    fwork.GridRate = edata->GridRate;
    if (sh2gfw_Get_NightOrDay() && fwork.Global < 5) {
        fogSetPartNum(edata->PartNum / 2);
    } else {
        fogSetPartNum(edata->PartNum);
    }
    fwork.Flag = (fwork.Flag & 0xFF00) | f;
    if (f2) {
        fogInitParticle();
    }
}

/** Restarts the wind and every particle. */
void fogInitScreen(void) {
    fogInitWind();
    fogInitParticle();
    fogInitParticle2();
}

/** Stops the wind, restarts the area's default wind and resets the grid densities. */
void fogInitWind(void) {
    *(u_long128 *)fwork.WindV = 0;
    fogChangeWind(fwork.WindDef);
    shFill(fwork.GridDense, 0x3F800000, 512);
}

/** Picks a new wind of type `wind` (direction, speed, randomness) and sets its duration. */
void fogChangeWind(int wind) {
    float *Wind;
    float *Wind2;
    float WindSpeed;
    float WindRandom;
    float a1;
    float a2;

    Wind = (float *)0x70003FF0;
    Wind2 = (float *)0x70003FE0;
    WindSpeed = 0.4f;
    WindRandom = 0.1f;
    switch (wind) {
    case 1:
    case 3:
        Wind[0] = shRandF() - 0.5f;
        Wind[1] = 0.0f;
        Wind[2] = shRandF() - 0.5f;
        break;
    case 2:
        *(u_long128 *)Wind = 0;
        WindRandom = 0.0f;
        break;
    case 17:
        Wind[0] = shRandF() - 0.5f;
        Wind[1] = 0.0f;
        Wind[2] = shRandF() - 0.5f;
        WindSpeed = 0.2f;
        WindRandom = 0.0f;
        break;
    case 5:
        shSinCosV(Wind, 3.1415927f + 3.1415927f * (shRandF() - 0.5f) / 4.0f);
        Wind[1] = 0.1f * shRandF();
        break;
    case 6:
        shSinCosV(Wind, 3.1415927f * (shRandF() - 0.5f) / 4.0f);
        Wind[1] = 0.1f * shRandF();
        break;
    case 7:
        shSinCosV(Wind, 3.1415927f * (shRandF() - 0.5f) / 4.0f - 1.5707964f);
        Wind[1] = 0.1f * shRandF();
        break;
    case 8:
        shSinCosV(Wind, 1.5707964f + 3.1415927f * (shRandF() - 0.5f) / 4.0f);
        Wind[1] = 0.1f * shRandF();
        break;
    case 9:
        shSinCosV(Wind, 2.3561945f + 3.1415927f * (shRandF() - 0.5f) / 4.0f);
        Wind[1] = 0.1f * shRandF();
        break;
    case 10:
        shSinCosV(Wind, 0.7853982f + 3.1415927f * (shRandF() - 0.5f) / 4.0f);
        Wind[1] = 0.1f * shRandF();
        break;
    case 11:
        shSinCosV(Wind, 3.1415927f * (shRandF() - 0.5f) / 4.0f - 0.7853982f);
        Wind[1] = 0.1f * shRandF();
        break;
    case 12:
        shSinCosV(Wind, 3.1415927f * (shRandF() - 0.5f) / 4.0f - 2.3561945f);
        Wind[1] = 0.1f * shRandF();
        break;
    case 4:
        Wind[0] = 0.1f * (shRandF() - 0.5f);
        Wind[1] = 0.0f;
        Wind[2] = 1.0f;
        WindSpeed = 0.5f;
        WindRandom = 0.0f;
        break;
    case 13:
        Wind[0] = 0.5f;
        Wind[1] = 0.1f;
        Wind[2] = 0.0f;
        WindSpeed = 0.2f;
        break;
    case 14:
        Wind[0] = -(0.2f * shRandF()) - 0.2f;
        Wind[1] = 0.1f + 0.2f * shRandF();
        Wind[2] = 0.8f + 0.5f * shRandF();
        WindSpeed = 1.5f;
        break;
    case 15:
        shSinCosV(Wind, 2.6179938f + 0.08726646f * (shRandF() - 0.5f));
        Wind[1] = -0.1f;
        WindSpeed = 0.3f;
        WindRandom = 0.0f;
        break;
    case 16:
        Wind[0] = shRandF() - 0.5f;
        Wind[1] = 0.0f;
        Wind[2] = shRandF() - 0.5f;
        WindSpeed = 0.1f;
        WindRandom = 0.0f;
        break;
    case 18:
        Wind[0] = 1.0f + shRandF();
        Wind[1] = 0.1f * shRandF();
        Wind[2] = 0.1f + 0.1f * shRandF();
        WindSpeed = 0.7f;
        break;
    case 0:
    default:
        Wind[0] = shRandF() - 0.5f;
        Wind[1] = 0.1f * shRandF();
        Wind[2] = shRandF() - 0.5f;
        break;
    }
    Wind[3] = 0.0f;
    _shNormalize(Wind, Wind);
    _sceVu0ScaleVector(Wind, Wind, WindSpeed);
    shRandV_Scale(Wind2, 0.5f);
    _shNormalize(Wind2, Wind2);
    _sceVu0ScaleVector(Wind2, Wind2, WindRandom);
    _sceVu0AddVector(Wind, Wind, Wind2);
    if (fwork.WindV[0] != 0.0f || fwork.WindV[1] != 0.0f || fwork.WindV[2] != 0.0f) {
        a1 = shAtanV(fwork.WindV);
        a2 = shAngleRegulate(a1 - shAtanV(Wind));
        if (fabsf(a2) > 1.0471976f) {
            a1 += 1.0471976f * _shSignIP(a2);
            shSinCosV_Scale(Wind2, a1, _shLengthXZ(Wind));
            Wind[0] = Wind2[0];
            Wind[2] = Wind2[2];
        }
    }
    vcopy(Wind, fwork.WindV);
    fwork.WindTimer += ftoi(shSway1f(-300.0f, 0.1f));
    if ((float)fwork.WindTimer < 300.0f || (float)fwork.WindTimer > 900.0f) {
        fwork.WindTimer = 600;
    }
}

/** Places every particle anew and resets the fog's view state. */
void fogInitParticle(void) {
    int i;
    struct FOG_PART_DATA *pd;

    pd = fwork.Part;
    for (i = fwork.PartNum; i > 0; i--) {
        fog_init_part_sub(pd++);
    }
    *(u_long128 *)fwork.sc_degree = 0;
    fwork.sc_degree[2] = -1.0f;
    fwork.sc_tdx = shRandF();
    fwork.sc_tdy = shRandF();
}

/**
 * Places particle `pd` at a random position around the local position, with the environment's
 * alpha.
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fog_init_part_sub(struct FOG_PART_DATA *pd) {
    __asm__ __volatile__("
    .set noreorder
    lui        t7, 0x3F00
    lqc2       vf10, 0x0(%1)
    qmtc2.ni   t7, vf11
    lqc2       vf9, 0x0(%2)
    jal        shRandV_asm
    vmove.w    vf6, vf0
    jal        shRandV_asm
    vadd.xyz   vf6, vf4, vf4
    jal        shRandV_asm
    vadd.xyz   vf7, vf4, vf4
    vsubw.xyz  vf6, vf6, vf0w
    vsubw.xyz  vf7, vf7, vf0w
    vsubx.xyz  vf8, vf4, vf11x
    vmulx.xyz  vf6, vf6, vf10x
    vmulz.xyz  vf7, vf7, vf10z
    vadd.xyz   vf6, vf6, vf9
    vsub.w     vf7, vf7, vf7
    beqz       %3, L_nodbl
    vsub.w     vf8, vf8, vf8
    vabs.y     vf6, vf6
    vsub.y     vf6, vf0, vf6
    vmulw.y    vf6, vf6, vf10w
L_nodbl:
    sqc2       vf6, 0x0(%0)
    sqc2       vf7, 0x10(%0)
    sqc2       vf8, 0x20(%0)
    vsub.xyzw  vf6, vf6, vf6
    jal        shRandV_asm
    vsub.w     vf7, vf7, vf7
    jal        shRandV_asm
    vadd.x     vf7, vf0, vf4
    qmtc2.ni   %4, vf8
    vaddx.y    vf7, vf0, vf4x
    vitof0.x   vf8, vf8
    vsubw.z    vf6, vf6, vf0w
    vaddz.x    vf7, vf0, vf8z
    vsub.yzw   vf8, vf8, vf8
    sqc2       vf6, 0x30(%0)
    sqc2       vf7, 0x40(%0)
    sqc2       vf8, 0x50(%0)
    .set reorder
    " : : "r"(pd), "r"(&fog_asm_data3), "r"(fwork.LocalPosV), "r"(fwork.Double), "r"(fwork.Alpha));
}

/** Places the particle at the scratchpad work slot (0x70003F00) at a new random position. */
/* the particle to renew is the one on the scratchpad (callers pass one, unused) */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fog_part_newpos() {
    struct FOG_PART_DATA *pd;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    __asm__ __volatile__("
    .set noreorder
    lui        t7, 0x3F00
    lqc2       vf10, 0x0(%1)
    qmtc2.ni   t7, vf9
    lqc2       vf9, 0x0(%2)
    jal        shRandV_asm
    vmove.w    vf6, vf0
    jal        shRandV_asm
    vadd.xyz   vf6, vf4, vf4
    jal        shRandV_asm
    vadd.xyz   vf7, vf4, vf4
    vsubw.xyz  vf6, vf6, vf0w
    vsubw.xyz  vf7, vf7, vf0w
    vsubx.xyz  vf8, vf4, vf11x
    vmuly.xyz  vf6, vf6, vf10y
    vmulz.xyz  vf7, vf7, vf10z
    vadd.xyz   vf6, vf6, vf9
    vsub.w     vf7, vf7, vf7
    vmulw.y    vf6, vf6, vf10w
    vsub.w     vf8, vf8, vf8
    sqc2       vf6, 0x0(%0)
    sqc2       vf7, 0x10(%0)
    sqc2       vf8, 0x20(%0)
    vsub.xyzw  vf6, vf6, vf6
    jal        shRandV_asm
    vsub.zw    vf7, vf7, vf7
    jal        shRandV_asm
    vadd.x     vf7, vf0, vf4
    vsubw.z    vf6, vf6, vf0w
    vaddx.y    vf7, vf0, vf4x
    sqc2       vf6, 0x30(%0)
    sqc2       vf7, 0x40(%0)
    sq         zero, 0x50(%0)
    .set reorder
    " : : "r"(pd), "r"(&fog_asm_data3), "r"(fwork.fewdense));
}

/** Removes every collision wall. */
void fogResetWall(void) {
    fwork.WallNum = 0;
}

/**
 * Adds a collision wall from the quad `Vector` (four float[4] corners): its bounds, centre and
 * normal.
 */
/* Matching: the asserts bake in the original line numbers (the helpers above lived in headers). */
#line 594
void fogSetWall(void *Vector) {
    float (*wv)[4];
    struct FOG_WALL_DATA *wall;
    int i;
    float cv[4];

    wv = Vector;
    wall = &fwork.Wall[fwork.WallNum++];
    fjAssert(fwork.WallNum <= WALL_MAX);
    shSetMiniMaxN(wall->min, wall->max, wv, 4);
    wall->min[0] -= 10.0f;
    wall->min[2] -= 10.0f;
    wall->max[0] += 10.0f;
    wall->max[2] += 10.0f;
    wall->max[3] = 0.0f;
    wall->min[3] = 0.0f;
    *(u_long128 *)cv = 0;
    for (i = 0; i < 4; i++) {
        _sceVu0AddVector(cv, cv, wv[i]);
    }
    _sceVu0ScaleVector(wall->v0, cv, 0.25f);
    shCreateNormal(wall->normal, wv[0], wv[1], wv[2]);
}

/** Removes every collision object. */
void fogResetObj(void) {
    fwork.ObjMax = 0;
}



/**
 * Adds object `ID` (or updates it) at `Center` with radius `Size`; the object pushes particles away
 * and starts at rest.
 */
void fogSetObj(unsigned long ID, void *Center, float Size) {
    struct FOG_OBJ_DATA *od;

    od = fogGetObj(ID);
    if (od == NULL) {
        fjAssert(fwork.ObjMax < OBJ_MAX);
        od = &fwork.Obj[fwork.ObjMax++];
    }
    vcopy(Center, od->pos);
    *(u_long128 *)od->mv = 0;
    od->pos[3] = Size;
    od->mv[3] = fwork.EscapeRange;
    od->rer = 1.0f / fwork.EscapeRange;
    od->id = ID;
    od->type = 0;
}

/** Moves object `ID` to `Center`; its motion vector is the difference from its last position. */
void fogMoveObj(unsigned long ID, void *Center) {
    struct FOG_OBJ_DATA *od;
    float es;

    od = fogGetObj(ID);
    if (od) {
        es = od->mv[3];
        _sceVu0SubVectorXYZ(od->mv, Center, od->pos);
        od->mv[3] = es;
        vcopy3(Center, od->pos);
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 679
/** fogSetObj() for the second kind of collision object. */
void fogSetObj2(unsigned long ID, void *Center, float Size) {
    struct FOG_OBJ_DATA *od;

    od = fogGetObj(ID);
    if (od == NULL) {
        fjAssert(fwork.ObjMax < OBJ_MAX);
        od = &fwork.Obj[fwork.ObjMax++];
    }
    vcopy(Center, od->pos);
    *(u_long128 *)od->mv = 0;
    od->pos[3] = Size;
    od->mv[3] = fwork.EscapeRange;
    od->rer = 1.0f / fwork.EscapeRange;
    od->id = ID;
    od->type = 1;
}

/** Removes object `ID`, moving the last object into its slot. */
void fogEraseObj(unsigned long ID) {
    struct FOG_OBJ_DATA *od;
    struct FOG_OBJ_DATA *od2;

    od = fogGetObj(ID);
    if (od) {
        od2 = &fwork.Obj[--fwork.ObjMax];
        if (od2->id != ID) {
            fogCopyObj(od2, od);
        }
    }
}

/** Sets the radius of object `ID` to `Size`. */
void fogSetObjSize(unsigned long ID, float Size) {
    struct FOG_OBJ_DATA *od;

    od = fogGetObj(ID);
    if (od) {
        od->pos[3] = Size;
    }
}

/** Returns collision object `ID`, or NULL. */
struct FOG_OBJ_DATA *fogGetObj(unsigned long ID) {
    struct FOG_OBJ_DATA *pObj;
    int i;

    pObj = fwork.Obj;
    for (i = fwork.ObjMax; i > 0; i--) {
        if (pObj->id == ID) {
            return pObj;
        }
        pObj++;
    }
    return NULL;
}

/**
 * Per-frame particle update on the scratchpad: wind, collisions with walls and objects, grid
 * density, alpha and the respawn of lost particles.
 */
void fogMoveParticle(void) {
    int n;
    int n1;
    int n2;
    struct FOG_PART_DATA *pd;
    struct FOG_PART_DATA *pdo;
    float *FVector;
    float d;
    float dm;
    float *dp;
    int p;
    struct FOG_OBJ_DATA *od;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    fogSetLocalPosV();
    fogSetCollision();
    fogSetAreaEnvironment();
    if (fwork.PartNum <= 0) {
        return;
    }
    fogSetSpeedLevel(60.0f * shGetDT());
    if (distXZ(fwork.Part[0].pos, fwork.WorldPosV) > 30000.0f && fwork.Global != 5) {
        fogInitScreen();
    }
    __asm__ __volatile__("
    lqc2 vf20, 0x0(%0)
    lqc2 vf21, 0x10(%0)
    lqc2 vf22, 0x20(%0)
    lqc2 vf23, 0x30(%0)
    lqc2 vf24, 0x40(%0)
    lqc2 vf25, 0x50(%0)
    lqc2 vf26, 0x60(%0)
    lqc2 vf28, 0x0(%1)
    lqc2 vf29, 0x0(%2)
    " : : "r"(&fog_asm_data1), "r"(&fwork.LimitY), "r"(fwork.LocalPosV));
    SyncDCache(fwork.GridDense, (char *)fwork.GridDense + sizeof(fwork.GridDense));
    spkDmatoSPR(sizeof(fwork.GridDense) / 16, 0, fwork.GridDense);
    asm {
        .set noreorder
        lui     v0, 0x7000
        ori     t0, v0, 0x800
        addi    t1, zero, 0x20
    L_clear:
        addiu   t1, t1, -0x1
        sq      zero, 0x0(t0)
        sq      zero, 0x10(t0)
        sq      zero, 0x20(t0)
        sq      zero, 0x30(t0)
        bnez    t1, L_clear
        addiu   t0, t0, 0x40
        .set reorder
    }
    if (fwork.StayPoint[3]) {
        fog_set_stay();
    }
    if (fwork.WallNum > 0) {
        SyncDCache(fwork.Wall, (char *)fwork.Wall + fwork.WallNum * sizeof(struct FOG_WALL_DATA));
        spkDmatoSPR(fwork.WallNum * 4, 0x1000, fwork.Wall);
    }
    pdo = fwork.Part;
    n1 = fwork.WallNum;
    spkDmaWaittoSPR(0);
    n = fwork.PartNum;
    while (n > 0) {
        fogCopyPart(pdo, pd);
        if (pd->pos[0] < -1e8f || pd->pos[0] > 1e8f || pd->pos[1] < -1e8f || pd->pos[1] > 1e8f ||
            pd->pos[2] < -1e8f || pd->pos[2] > 1e8f) {
            fog_part_newpos();
        }
        if (pd->erase) {
            pd->alp_now *= 0.75f;
            if (pd->alp_now <= 1.0f) {
                fog_part_newpos();
            }
        } else if (pd->bounce > 0) {
            pd->erase = 1;
        }
        if (n1) {
            fog_part_wall();
        } else {
            float *FVector;

            FVector = (float *)0x70003FF0;
            _sceVu0ScaleVector(FVector, pd->mv, fwork.SpeedLevel);
            _sceVu0AddVector(pd->pos, pd->pos, FVector);
            pd->bounce = 0;
        }
        _sceVu0AddVectorXYZ(pd->mv, pd->mv, fwork.WindV);
        fog_part_grid();
        fog_part_alp();
        if (pd->mv[1] < -2.0f) {
            pd->erase = 1;
        }
        fogCopyPart(pd, pdo);
        n--;
        pdo++;
    }
    InvalidDCache(fwork.GridDense, (char *)fwork.GridDense + sizeof(fwork.GridDense));
    spkDmafromSPR(sizeof(fwork.GridDense) / 16, 0x800, fwork.GridDense);
    FVector = (float *)0x70003FF0;
    dp = (float *)0x70000800;
    p = 0;
    dm = 3.4028235e38f;
    for (n = 0; n < 512; n++) {
        d = *dp++;
        if (d < dm) {
            dm = d;
            p = n;
        }
    }
    n = p >> 6;
    if (n >= 4) {
        n -= 8;
    }
    FVector[0] = n * (fwork.MaxPos / 4.0f);
    n = (p >> 3) & 7;
    if (n >= 4) {
        n -= 8;
    }
    FVector[1] = n * (fwork.MaxPos / 4.0f);
    n = p & 7;
    if (n >= 4) {
        n -= 8;
    }
    FVector[2] = n * (fwork.MaxPos / 4.0f);
    FVector[3] = 0.0f;
    _sceVu0AddVector(fwork.fewdense, FVector, fwork.LocalPosV);
    spkDmaWaitfromSPR(0);
    if (fwork.Flag & 0x100) {
        fogMoveParticle2();
    }
    fog_load_objdata();
    pdo = fwork.Part;
    n1 = fwork.ObjNum;
    n2 = fwork.ObjNum2;
    if (fwork.Flag & 0x80) {
        n2 = 0;
        n1 = 0;
    }
    n = fwork.PartNum;
    while (n > 0) {
        fogCopyPart(pdo, pd);
        asm {
            lqc2       vf4, 0x0(pd)
            vadd.w     vf6, vf0, vf0
            vsub.xyz   vf5, vf4, vf29
            vadd.w     vf7, vf6, vf0
            vmulw.xyz  vf5, vf5, vf24w
            vaddw.xyz  vf5, vf5, vf6w
            vmaxx.xyz  vf5, vf5, vf0x
            vminiw.xyz vf5, vf5, vf7w
            vftoi0.xyz vf5, vf5
            qmfc2.ni   t0, vf5
            dsrl       t2, t0, 30
            or         t1, t0, t2
            pextuw     t2, zero, t0
            sll        t2, t2, 4
            or         t1, t1, t2
            sw         t1, 0x4C(pd)
        }
        if (n1) {
            fog_part_obj();
        }
        if (n2) {
            fog_part_obj2();
        }
        fog_part_clamp();
        fogCopyPart(pd, pdo);
        n--;
        pdo++;
    }
    od = fwork.Obj;
    for (n = fwork.ObjNum; n > 0; n--) {
        od->mv[2] = 0.0f;
        od->mv[1] = 0.0f;
        od->mv[0] = 0.0f;
        od++;
    }
}

/**
 * Sorts the collision objects near the fog volume into the scratchpad lists for the particle asm
 * (object types 0 and 1).
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fog_load_objdata(void) {
    __asm__ __volatile__("
    .set noreorder
    sw         zero, 0x8(%5)
    beqz       %3, @15
    sw         zero, 0xC(%5)
    sw         %1, 0x0(%5)
    sw         %2, 0x4(%5)
    lui        t7, 0x4020
    addu       t1, %0, zero
    qmtc2.ni   t7, vf7
    xor        t0, t0, t0
@loop1:
    lqc2       vf4, 0x0(t1)
    vsub.xyz   vf6, vf4, vf29
    vmulw.xyz  vf5, vf6, vf24w
    vabs.xyz   vf6, vf5
    vsubx.xyz  vf6, vf6, vf7x
    vnop
    vnop
    vnop
    vnop
    addi       t6, zero, 0xE0
    cfc2.ni    t7, vi17
    and        t7, t7, t6
    bne        t7, t6, @14
    vaddx.xyz  vf5, vf5, vf7x
    vftoi0.xyz vf5, vf5
    qmfc2.ni   t5, vf5
    sll        t2, t5, 0
    dsrl32     t3, t5, 0
    pextuw     t4, zero, t5
    sw         t0, 0x10(%5)
    sw         t1, 0x14(%5)
    lw         t0, 0x20(t1)
    xor        t7, t7, t7
    bnez       t0, @8
    xor        t6, t6, t6
@loop2:
    andi       t0, t6, 0x1
    bnez       t0, @1
    slti       t1, t2, 0x1
    bnez       t1, @7
    nop
    b          @2
    addiu      t5, t2, -0x1
@1:
    slti       t1, t2, 0x4
    beqz       t1, @7
    addu       t5, t2, zero
@2:
    andi       t0, t6, 0x2
    bnez       t0, @3
    slti       t1, t3, 0x1
    bnez       t1, @7
    addiu      t0, t3, -0x1
    b          @4
    sll        t0, t0, 2
@3:
    slti       t1, t3, 0x4
    beqz       t1, @7
    sll        t0, t3, 2
@4:
    or         t5, t5, t0
    andi       t0, t6, 0x4
    bnez       t0, @5
    slti       t1, t4, 0x1
    bnez       t1, @7
    addiu      t0, t4, -0x1
    b          @6
    sll        t0, t0, 4
@5:
    slti       t1, t4, 0x4
    beqz       t1, @7
    sll        t0, t4, 4
@6:
    or         t5, t5, t0
    addi       t1, zero, 0x1
    dsllv      t1, t1, t5
    or         t7, t7, t1
@7:
    slti       t1, t6, 0x7
    addi       t6, t6, 0x1
    bnez       t1, @loop2
    nop
    lw         t1, 0x14(%5)
    lw         t0, 0x0(%5)
    lq         t5, 0x10(t1)
    ld         t6, 0x20(t1)
    sqc2       vf4, 0x0(t0)
    sq         t5, 0x10(t0)
    sd         t6, 0x20(t0)
    sd         t7, 0x28(t0)
    lw         t5, 0x8(%5)
    addiu      t0, t0, 0x30
    addi       t5, t5, 0x1
    sw         t0, 0x0(%5)
    sw         t5, 0x8(%5)
    b          @14
    lw         t0, 0x10(%5)
@8:
    slti       t1, t3, 0x1
    addi       t3, t3, -0x1
    movn       t3, zero, t1
@loop3:
    xor        t6, t6, t6
@loop4:
    andi       t0, t6, 0x1
    bnez       t0, @9
    slti       t1, t2, 0x1
    bnez       t1, @13
    nop
    b          @10
    addiu      t5, t2, -0x1
@9:
    slti       t1, t2, 0x4
    beqz       t1, @13
    addu       t5, t2, zero
@10:
    andi       t0, t6, 0x2
    bnez       t0, @11
    slti       t1, t4, 0x1
    bnez       t1, @13
    addiu      t0, t4, -0x1
    b          @12
    sll        t0, t0, 4
@11:
    slti       t1, t4, 0x4
    beqz       t1, @13
    sll        t0, t4, 4
@12:
    or         t5, t5, t0
    sll        t0, t3, 2
    or         t5, t5, t0
    addi       t0, zero, 0x1
    dsllv      t0, t0, t5
    or         t7, t7, t0
@13:
    slti       t1, t6, 0x3
    addi       t6, t6, 0x1
    bnez       t1, @loop4
    slti       t1, t3, 0x3
    addi       t3, t3, 0x1
    bnez       t1, @loop3
    nop
    lw         t1, 0x14(%5)
    lw         t0, 0x4(%5)
    lq         t5, 0x10(t1)
    ld         t6, 0x20(t1)
    sqc2       vf4, 0x0(t0)
    sq         t5, 0x10(t0)
    sd         t6, 0x20(t0)
    sd         t7, 0x28(t0)
    sw         zero, 0x14(t0)
    lw         t5, 0xC(%5)
    addiu      t0, t0, 0x30
    addi       t5, t5, 0x1
    sw         t0, 0x4(%5)
    sw         t5, 0xC(%5)
    lw         t0, 0x10(%5)
@14:
    addi       t0, t0, 0x1
    bne        t0, %3, @loop1
    addiu      t1, t1, 0x30
@15:
    lw         t0, 0x8(%5)
    lw         t1, 0xC(%5)
    sh         t0, 0x0(%4)
    sh         t1, 0x2(%4)
    .set reorder
    " : : "r"(fwork.Obj), "r"(0x70000000), "r"(0x70001F80), "r"(fwork.ObjMax), "r"(&fwork.ObjNum), "r"(0x70003F80));
}

/** Converts the stay point into grid coordinates for the particle asm. */
/* thins the fog in the grid cell of the stay point */
void fog_set_stay(void) {
    float *fv;
    float *dn;
    int px;
    int py;
    int pz;
    int tx;
    int ty;
    int tz;
    int i;

    fv = (float *)0x70003FF0;
    dn = (float *)0x70000800;
    _sceVu0SubVector(fv, fwork.StayPoint, fwork.LocalPosV);
    _sceVu0ScaleVector(fv, fv, 4.0f / fwork.MaxPos);
    if (fwork.Double) {
        fv[1] *= (float)(1 << fwork.Double);
    }
    px = ffloor(fv[0] - 0.5f) & 7;
    py = ffloor(fv[1] - 0.5f) & 7;
    pz = ffloor(fv[2] - 0.5f) & 7;
    for (i = 0; i < 8; i++) {
        tx = px + (i & 1);
        ty = py + ((i >> 1) & 1);
        tz = pz + (i >> 2);
        if (tx >= 0 && ty >= 0 && tz >= 0 && tx <= 7 && ty <= 7 && tz <= 7) {
            dn[(pz << 6) | (px | (py << 3))] -= fwork.PartNum;
        }
    }
}

/**
 * Particle-against-wall collision (VU0 asm, scratchpad data).
 * NON_MATCHING: the original has wall in t1 and no code for `addu t1, wall, zero` (its line table
 * has no entry for that line, where fog_part_obj has its copy of od): its compiler coalesced wall
 * with the copy's t1. Ours keeps wall in a1 and the copy (4 bytes longer). With the asm using
 * wall itself instead of t1, only that register differs (6 words), but the line table fits the
 * copy. The asm is the original's (DWARF locals pd and wall; the pointers set on C lines, then a
 * line-table entry per instruction).
 */
void fog_part_wall(void) {
    struct FOG_PART_DATA *pd;
    struct FOG_WALL_DATA *wall;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    wall = (struct FOG_WALL_DATA *)0x70001000;
    asm {
        .set noreorder
        lh      v1, fwork+0x15E56
        nop
        lqc2    vf4, 0x0(pd)
        lqc2    vf5, 0x10(pd)
        xor     t0, t0, t0
        addu    t1, wall, zero
        xor     t2, t2, t2
    @loop:
        lqc2    vf13, 0x10(t1)
        lqc2    vf12, 0x0(t1)
        vsub.xyz vf9, vf4, vf13
        vmul.xyz vf6, vf9, vf12
        vaddy.x vf6, vf6, vf6y
        vaddz.x vf6, vf6, vf6z
        vabs.x  vf7, vf6
        vsuby.x vf10, vf7, vf20y
        vsubx.x vf11, vf10, vf21x
        qmfc2.ni t3, vf11
        sll     t3, t3, 0
        bgez    t3, @4
        vmulx.xyz vf8, vf12, vf6x
        vsub.xyz vf8, vf4, vf8
        lqc2    vf14, 0x20(t1)
        lqc2    vf15, 0x30(t1)
        ctc2.ni zero, vi16
        vsub.xyz vf11, vf8, vf14
        vsub.xyz vf11, vf15, vf8
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni t3, vi16
        vaddx.y vf6, vf0, vf6x
        andi    t3, t3, 0x80
        bnez    t3, @4
        nop
        qmfc2.ni t3, vf6
        blezl   t3, @1
        addi    t2, t2, 0x64
    @1:
        vmul.xyz vf6, vf5, vf12
        qmfc2.ni t3, vf10
        vaddy.x vf6, vf6, vf6y
        sll     t3, t3, 0
        vaddz.x vf6, vf6, vf6z
        bgez    t3, @2
        nop
        addi    t2, t2, 0x1
        vmulx.xyz vf8, vf12, vf10x
        b       @3
        vsub.xyz vf4, vf4, vf8
    @2:
        vsub.x  vf10, vf21, vf10
        vmuly.x vf10, vf10, vf21y
        vmul.x  vf6, vf6, vf10
    @3:
        vmulx.xyz vf8, vf12, vf6x
        vsub.xyz vf8, vf5, vf8
        vmul.xyz vf7, vf8, vf8
        vmul.xyz vf6, vf5, vf5
        vaddy.x vf7, vf7, vf7y
        vaddy.x vf6, vf6, vf6y
        vaddz.x vf7, vf7, vf7z
        vaddz.x vf6, vf6, vf6z
        vdiv    Q, vf6x, vf7x
        vwaitq
        vaddq.x vf6, vf0, Q
        cfc2.ni t3, vi16
        .word   0x4A0603BD
        andi    t3, t3, 0x20
        vwaitq
        vmulq.xyz vf5, vf8, Q
        bnel    t3, zero, @4
        vmove.xyz vf5, vf8
    @4:
        addiu   t0, t0, 0x1
        bne     t0, v1, @loop
        addiu   t1, t1, 0x40
        vmulw.xyz vf8, vf5, vf28w
        vadd.xyz vf4, vf4, vf8
        sqc2    vf5, 0x10(pd)
        sqc2    vf4, 0x0(pd)
        sw      t2, 0x58(pd)
        .set reorder
    }
}

/** Grid density pass over the particles (VU0 asm, scratchpad data). */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fog_part_grid(void) {
    struct FOG_PART_DATA *pd;
    float *GridDense;
    float *GridDenseNew;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    GridDenseNew = (float *)0x70000000;
    GridDense = (float *)0x70000800;
    asm {
        .set noreorder
        lqc2    vf4, 0x0(pd)
        vaddw.xyz vf12, vf0, vf0w
        vmulz.xyz vf6, vf4, vf24z
        qmfc2.ni s0, vf12
        vmulw.y vf6, vf6, vf25w
        qmfc2.ni t0, vf6
        vftoi0.xyz vf5, vf6
        pcgtw   t1, zero, t0
        vitof0.xyz vf5, vf5
        pand    t1, s0, t1
        qmtc2.ni t1, vf8
        vsub.xyz vf5, vf5, vf8
        vsub.xyz vf7, vf6, vf5
        vftoi0.xyz vf5, vf5
        qmfc2.ni t1, vf7
        qmfc2.ni t0, vf5
        mtc1    t1, f8
        dsrl32  t3, t0, 0
        dsrl32  t7, t1, 0
        pexew   t4, t0
        pexew   t1, t1
        mtc1    t7, f9
        mtc1    t1, f10
        vsub.xyz vf8, vf12, vf7
        andi    t2, t0, 0x7
        andi    t3, t3, 0x7
        andi    t4, t4, 0x7
        qmfc2.ni t0, vf8
        addi    t5, t2, 0x1
        addi    t6, t3, 0x1
        addi    t7, t4, 0x1
        mtc1    t0, f11
        dsrl32  t1, t0, 0
        pexew   t0, t0
        mtc1    t1, f12
        mtc1    t0, f13
        andi    t5, t5, 0x7
        andi    t6, t6, 0x7
        andi    t7, t7, 0x7
        sll     t2, t2, 2
        sll     t3, t3, 5
        sll     t4, t4, 8
        sll     t5, t5, 2
        sll     t6, t6, 5
        sll     t7, t7, 8
        addu    t4, t4, GridDense
        addu    t7, t7, GridDense
        add.s   f14, f12, f13
        add.s   f15, f9, f13
        add.s   f13, f12, f10
        add.s   f12, f9, f10
        addu    t1, t3, t4
        addu    t4, t4, t6
        addu    t3, t3, t7
        addu    t6, t6, t7
        addu    t0, t2, t1
        addu    t1, t5, t1
        lwc1    f9, 0x0(t0)
        lwc1    f10, 0x0(t1)
        add.s   f9, f9, f11
        add.s   f10, f10, f8
        add.s   f9, f9, f14
        add.s   f10, f10, f14
        swc1    f9, 0x0(t0)
        swc1    f10, 0x0(t1)
        addu    t0, t2, t4
        addu    t1, t5, t4
        lwc1    f9, 0x0(t0)
        lwc1    f10, 0x0(t1)
        add.s   f9, f9, f11
        add.s   f10, f10, f8
        add.s   f9, f9, f15
        add.s   f10, f10, f15
        swc1    f9, 0x0(t0)
        swc1    f10, 0x0(t1)
        addu    t0, t2, t3
        addu    t1, t5, t3
        lwc1    f9, 0x0(t0)
        lwc1    f10, 0x0(t1)
        add.s   f9, f9, f11
        add.s   f10, f10, f8
        add.s   f9, f9, f13
        add.s   f10, f10, f13
        swc1    f9, 0x0(t0)
        swc1    f10, 0x0(t1)
        addu    t0, t2, t6
        addu    t1, t5, t6
        lwc1    f9, 0x0(t0)
        lwc1    f10, 0x0(t1)
        lqc2    vf4, 0x20(pd)
        add.s   f9, f9, f11
        add.s   f10, f10, f8
        add.s   f9, f9, f12
        add.s   f10, f10, f12
        vadd.xyz vf6, vf6, vf4
        swc1    f9, 0x0(t0)
        swc1    f10, 0x0(t1)
        qmfc2.ni t0, vf6
        vftoi0.xyz vf5, vf6
        pcgtw   t1, zero, t0
        vitof0.xyz vf5, vf5
        pand    t1, s0, t1
        qmtc2.ni t1, vf8
        vsub.xyz vf5, vf5, vf8
        vsub.xyz vf7, vf6, vf5
        vftoi0.xyz vf5, vf5
        vadd.xyz vf8, vf0, vf0
        qmfc2.ni t0, vf5
        vadd.xyz vf9, vf0, vf0
        dsrl32  t3, t0, 0
        pexew   t4, t0
        andi    t2, t0, 0x7
        andi    t3, t3, 0x7
        andi    t4, t4, 0x7
        addi    t5, t2, 0x1
        addi    t6, t3, 0x1
        addi    t7, t4, 0x1
        andi    t5, t5, 0x7
        andi    t6, t6, 0x7
        andi    t7, t7, 0x7
        pextlw  t0, t3, t2
        pextlw  t1, t6, t5
        pcpyld  t0, t4, t0
        pcpyld  t1, t7, t1
        por     s1, zero, t0
        por     s2, zero, t1
        sll     t2, t2, 2
        sll     t3, t3, 5
        sll     t4, t4, 8
        sll     t5, t5, 2
        sll     t6, t6, 5
        sll     t7, t7, 8
        addu    t4, t4, GridDenseNew
        addu    t7, t7, GridDenseNew
        addu    s0, t3, t4
        addu    t4, t4, t6
        addu    t3, t3, t7
        addu    t6, t6, t7
        addu    t0, t2, s0
        addu    t1, t5, s0
        lw      t0, 0x0(t0)
        lw      t1, 0x0(t1)
        nop
        nop
        qmtc2.ni t0, vf10
        qmtc2.ni t1, vf11
        vsubx.xyz vf9, vf9, vf10x
        vaddx.x vf8, vf8, vf11x
        vsubx.yz vf9, vf9, vf11x
        addu    t0, t2, t4
        addu    t1, t5, t4
        lw      t0, 0x0(t0)
        lw      t1, 0x0(t1)
        nop
        nop
        qmtc2.ni t0, vf10
        qmtc2.ni t1, vf11
        vaddx.y vf8, vf8, vf10x
        vsubx.xz vf9, vf9, vf10x
        vaddx.xy vf8, vf8, vf11x
        vsubx.z vf9, vf9, vf11x
        addu    t0, t2, t3
        addu    t1, t5, t3
        lw      t0, 0x0(t0)
        lw      t1, 0x0(t1)
        nop
        nop
        qmtc2.ni t0, vf10
        qmtc2.ni t1, vf11
        vaddx.z vf8, vf8, vf10x
        vsubx.xy vf9, vf9, vf10x
        vaddx.xz vf8, vf8, vf11x
        vsubx.y vf9, vf9, vf11x
        addu    t0, t2, t6
        addu    t1, t5, t6
        lw      t0, 0x0(t0)
        lw      t1, 0x0(t1)
        nop
        nop
        qmtc2.ni t0, vf10
        qmtc2.ni t1, vf11
        vaddx.yz vf8, vf8, vf10x
        vsubx.x vf9, vf9, vf10x
        vaddx.xyz vf8, vf8, vf11x
        addi    t2, s1, -0x1
        addi    t5, s2, 0x1
        andi    t2, t2, 0x7
        andi    t5, t5, 0x7
        sll     t2, t2, 2
        sll     t5, t5, 2
        addu    t0, t2, s0
        addu    t1, t5, s0
        lwc1    f8, 0x0(t0)
        lwc1    f9, 0x0(t1)
        addu    t0, t2, t4
        addu    t1, t5, t4
        lwc1    f10, 0x0(t0)
        lwc1    f11, 0x0(t1)
        addu    t0, t2, t3
        addu    t1, t5, t3
        add.s   f8, f8, f10
        add.s   f9, f9, f11
        lwc1    f10, 0x0(t0)
        lwc1    f11, 0x0(t1)
        addu    t0, t2, t6
        addu    t1, t5, t6
        add.s   f8, f8, f10
        add.s   f9, f9, f11
        lwc1    f10, 0x0(t0)
        lwc1    f11, 0x0(t1)
        add.s   f8, f8, f10
        add.s   f9, f9, f11
        mfc1    t0, f8
        mfc1    t1, f9
        qmtc2.ni t0, vf10
        qmtc2.ni t1, vf11
        vsubx.x vf8, vf8, vf10x
        vaddx.x vf9, vf9, vf11x
        andi    t2, s1, 0x7
        andi    t5, s2, 0x7
        dsrl32  t3, s1, 0
        dsrl32  t6, s2, 0
        pexew   t4, s1
        pexew   t7, s2
        sll     s3, t4, 8
        sll     s4, t7, 8
        sll     s0, t2, 2
        addi    s1, t3, -0x1
        addi    s2, t6, -0x1
        andi    s1, s1, 0x7
        andi    s2, s2, 0x7
        sll     s1, s1, 5
        sll     s2, s2, 5
        or      t1, s0, s3
        addu    s1, s1, GridDenseNew
        addu    s2, s2, GridDenseNew
        addu    t0, t1, s1
        addu    t1, t1, s2
        lwc1    f8, 0x0(t0)
        lwc1    f9, 0x0(t1)
        or      t1, s0, s4
        addu    t0, t1, s1
        addu    t1, t1, s2
        sll     s0, t5, 2
        lwc1    f10, 0x0(t0)
        lwc1    f11, 0x0(t1)
        or      t1, s0, s3
        addu    t0, t1, s1
        addu    t1, t1, s2
        add.s   f8, f8, f10
        add.s   f9, f9, f11
        lwc1    f10, 0x0(t0)
        lwc1    f11, 0x0(t1)
        or      t1, s0, s4
        addu    t0, t1, s1
        addu    t1, t1, s2
        add.s   f8, f8, f10
        add.s   f9, f9, f11
        lwc1    f10, 0x0(t0)
        lwc1    f11, 0x0(t1)
        add.s   f8, f8, f10
        add.s   f9, f9, f11
        mfc1    t0, f8
        mfc1    t1, f9
        qmtc2.ni t0, vf10
        qmtc2.ni t1, vf11
        vsubx.y vf8, vf8, vf10x
        vaddx.y vf9, vf9, vf11x
        sll     s3, t3, 5
        sll     s4, t6, 5
        sll     s0, t2, 2
        addi    s1, t5, -0x1
        addi    s2, t7, -0x1
        andi    s1, s1, 0x7
        andi    s2, s2, 0x7
        sll     s1, s1, 8
        sll     s2, s2, 8
        or      t1, s0, s3
        addu    s1, s1, GridDenseNew
        addu    s2, s2, GridDenseNew
        addu    t0, t1, s1
        addu    t1, t1, s2
        lwc1    f8, 0x0(t0)
        lwc1    f9, 0x0(t1)
        or      t1, s0, s4
        addu    t0, t1, s1
        addu    t1, t1, s2
        sll     s0, t5, 2
        lwc1    f10, 0x0(t0)
        lwc1    f11, 0x0(t1)
        or      t1, s0, s3
        addu    t0, t1, s1
        addu    t1, t1, s2
        add.s   f8, f8, f10
        add.s   f9, f9, f11
        lwc1    f10, 0x0(t0)
        lwc1    f11, 0x0(t1)
        or      t1, s0, s4
        addu    t0, t1, s1
        addu    t1, t1, s2
        add.s   f8, f8, f10
        add.s   f9, f9, f11
        lwc1    f10, 0x0(t0)
        lwc1    f11, 0x0(t1)
        add.s   f8, f8, f10
        add.s   f9, f9, f11
        mfc1    t0, f8
        mfc1    t1, f9
        qmtc2.ni t0, vf10
        qmtc2.ni t1, vf11
        vsubx.z vf8, vf8, vf10x
        vaddx.z vf9, vf9, vf11x
        vsub.xyz vf10, vf12, vf7
        vmul.xyz vf8, vf8, vf7
        vmul.xyz vf9, vf9, vf10
        lqc2    vf4, 0x10(pd)
        vadd.xyz vf8, vf8, vf9
        vmulx.xyz vf8, vf8, vf25x
        vsub.xyz vf4, vf4, vf8
        vmulw.xyz vf4, vf4, vf20w
        sqc2    vf4, 0x10(pd)
        .set reorder
    }
}

/** Particle-against-object collision (VU0 asm, scratchpad data). */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fog_part_obj(void) {
    struct FOG_PART_DATA *pd;
    struct FOG_OBJ_DATA *od;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    od = (struct FOG_OBJ_DATA *)0x70000000;
    asm {
        .set noreorder
        lh      a1, fwork+0x15E5A
        nop
        lqc2    vf4, 0x0(pd)
        lqc2    vf5, 0x10(pd)
        xor     t0, t0, t0
        addu    t1, od, zero
        lw      t4, 0x4C(pd)
        addi    t3, zero, 0x1
        lw      t2, 0x58(pd)
        dsllv   t3, t3, t4
    @loop:
        ld      t4, 0x28(t1)
        lqc2    vf6, 0x0(t1)
        and     t4, t4, t3
        beqz    t4, @3
        vsub.xyz vf9, vf4, vf6
        lqc2    vf7, 0x10(t1)
        vmul.xyz vf10, vf9, vf9
        vaddw.x vf11, vf20, vf6w
        vaddy.x vf10, vf10, vf10y
        vaddw.x vf12, vf11, vf7w
        vaddz.x vf10, vf10, vf10z
        vmul.x  vf13, vf12, vf12
        vsub.x  vf13, vf10, vf13
        qmfc2.ni t4, vf13
        sll     t4, t4, 0
        bgez    t4, @3
        nop
        vnop
        vnop
        .word   0x4A0A03BD
        addi    t2, t2, 0x1
        vwaitq
        vaddq.x vf10, vf0, Q
        vdiv    Q, vf0w, vf10x
        vsub.x  vf13, vf10, vf11
        vaddw.x vf11, vf0, vf0w
        qmfc2.ni t4, vf13
        lqc2    vf14, 0x20(t1)
        sll     t4, t4, 0
        vwaitq
        vmulq.xyz vf9, vf9, Q
        blezl   t4, @1
        addi    t2, t2, 0x1
        vsub.x  vf11, vf12, vf10
        vmuly.x vf11, vf11, vf14y
    @1:
        vsub.xyz vf8, vf5, vf7
        vmul.xyz vf13, vf8, vf9
        vaddy.x vf13, vf13, vf13y
        vaddz.x vf13, vf13, vf13z
        qmfc2.ni t4, vf13
        vmul.x  vf11, vf13, vf11
        sll     t4, t4, 0
        bgezl   t4, @2
        vmulz.x vf11, vf11, vf20z
    @2:
        vmulx.xyz vf8, vf9, vf11x
        vsub.xyz vf5, vf5, vf8
    @3:
        addiu   t0, t0, 0x1
        bne     t0, a1, @loop
        addiu   t1, t1, 0x30
        sqc2    vf5, 0x10(pd)
        sw      t2, 0x58(pd)
        .set reorder
    }
}

/** Particle collision with the second kind of object (VU0 asm, scratchpad data). */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fog_part_obj2(void) {
    struct FOG_PART_DATA *pd;
    struct FOG_OBJ_DATA *od;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    od = (struct FOG_OBJ_DATA *)0x70001F80;
    asm {
        .set noreorder
        lh      a1, fwork+0x15E5C
        nop
        lqc2    vf4, 0x0(pd)
        lqc2    vf5, 0x10(pd)
        xor     t0, t0, t0
        addu    t1, od, zero
        lw      t4, 0x4C(pd)
        addi    t3, zero, 0x1
        lw      t2, 0x58(pd)
        dsllv   t3, t3, t4
    @loop:
        ld      t4, 0x28(t1)
        lqc2    vf6, 0x0(t1)
        and     t4, t4, t3
        beqz    t4, @3
        vsub.xyz vf9, vf4, vf6
        vaddx.y vf9, vf9, vf20x
        qmfc2.ni t4, vf9
        blez    t4, @3
        lqc2    vf7, 0x10(t1)
        vmul.xz vf10, vf9, vf9
        vaddw.x vf11, vf20, vf6w
        vaddz.x vf10, vf10, vf10z
        vaddw.x vf12, vf11, vf7w
        vmul.x  vf13, vf12, vf12
        vsub.x  vf13, vf10, vf13
        qmfc2.ni t4, vf13
        sll     t4, t4, 0
        bgez    t4, @3
        nop
        vnop
        vnop
        .word   0x4A0A03BD
        addi    t2, t2, 0x1
        vwaitq
        vaddq.x vf10, vf0, Q
        vdiv    Q, vf0w, vf10x
        vsub.x  vf13, vf10, vf11
        vaddw.x vf11, vf0, vf0w
        qmfc2.ni t4, vf13
        lqc2    vf14, 0x20(t1)
        sll     t4, t4, 0
        vwaitq
        vmulq.xz vf9, vf9, Q
        blezl   t4, @1
        addi    t2, t2, 0x1
        vsub.x  vf11, vf12, vf10
        vmuly.x vf11, vf11, vf14y
    @1:
        vsub.xz vf8, vf5, vf7
        vmul.xz vf13, vf8, vf9
        vaddz.x vf13, vf13, vf13z
        qmfc2.ni t4, vf13
        vmul.x  vf11, vf13, vf11
        sll     t4, t4, 0
        bgezl   t4, @2
        vmulz.x vf11, vf11, vf20z
    @2:
        vmulx.xz vf8, vf9, vf11x
        vsub.xz vf5, vf5, vf8
    @3:
        addiu   t0, t0, 0x1
        bne     t0, a1, @loop
        addiu   t1, t1, 0x30
        sqc2    vf5, 0x10(pd)
        sw      t2, 0x58(pd)
        .set reorder
    }
}

/** Particle alpha update (VU0 asm, scratchpad data). */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fog_part_alp(void) {
    struct FOG_PART_DATA *pd;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    asm {
        .set noreorder
        vaddz.x vf10, vf0, vf25z
        lqc2    vf8, 0x0(pd)
        lqc2    vf9, 0x50(pd)
        vsubx.y vf11, vf8, vf28x
        qmfc2.ni t7, vf11
        bgez    t7, @1
        vsub.y  vf11, vf8, vf28
        qmfc2.ni t7, vf11
        bgezl   t7, @1
        vsub.x  vf10, vf10, vf10
        vmuly.x vf10, vf10, vf11y
        vmuly.x vf10, vf10, vf25y
    @1:
        vsubz.y vf11, vf8, vf28z
        qmfc2.ni t7, vf11
        bgtzl   t7, @2
        vaddz.x vf10, vf0, vf21z
    @2:
        qmfc2.ni t7, vf22
        mtc1    t7, f12
        dsrl32  t7, t7, 0
        jal     shSway1f_asm
        mtc1    t7, f13
        mfc1    t7, f4
        qmtc2.ni t7, vf11
        vaddx.y vf9, vf9, vf11x
        vsub.x  vf10, vf9, vf10
        vmulz.x vf10, vf10, vf22z
        vsubx.y vf9, vf9, vf10x
        vmulw.y vf9, vf9, vf22w
        vaddy.x vf9, vf9, vf9y
        vmax.x  vf9, vf9, vf0
        vminiz.x vf9, vf9, vf21z
        sqc2    vf9, 0x50(pd)
        .set reorder
    }
}

/** Clamps the particles to the fog volume (VU0 asm, scratchpad data). */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fog_part_clamp(void) {
    struct FOG_PART_DATA *pd;

    pd = (struct FOG_PART_DATA *)0x70003F00;
    asm {
        .set noreorder
        lqc2    vf4, 0x0(pd)
        lqc2    vf5, 0x50(pd)
        vsub.xyz vf4, vf4, vf29
        vaddx.xz vf6, vf4, vf24x
        vaddx.y vf6, vf4, vf26x
        qmfc2.ni t0, vf6
        sll     t1, t0, 0
        bgez    t1, @1
        nop
        vaddy.x vf4, vf4, vf24y
        vsub.z  vf4, vf0, vf4
    @1:
        bltzl   t0, @2
        vaddz.y vf4, vf4, vf26z
    @2:
        pexcw   t1, t0
        bgez    t1, @3
        nop
        vaddy.z vf4, vf4, vf24y
        vsub.x  vf4, vf0, vf4
    @3:
        vsubx.xz vf6, vf4, vf24x
        vsub.y  vf6, vf4, vf26
        qmfc2.ni t0, vf6
        sll     t1, t0, 0
        blez    t1, @4
        nop
        vsuby.x vf4, vf4, vf24y
        vsub.z  vf4, vf0, vf4
    @4:
        blez    t0, @5
        nop
        vsubz.y vf4, vf4, vf26z
        vadd.x  vf5, vf0, vf0
    @5:
        pexcw   t1, t0
        blez    t1, @6
        nop
        vsuby.z vf4, vf4, vf24y
        vsub.x  vf4, vf0, vf4
    @6:
        sqc2    vf5, 0x50(pd)
        vaddx.xz vf6, vf4, vf24x
        vaddx.y vf6, vf4, vf26x
        vsubx.xyz vf7, vf6, vf23x
        qmfc2.ni t0, vf7
        vmuly.xyz vf6, vf6, vf23y
        sll     t1, t0, 0
        bltzl   t1, @7
        vmulx.x vf5, vf5, vf6x
    @7:
        bltzl   t0, @8
        vmuly.x vf5, vf5, vf6y
    @8:
        pexcw   t1, t0
        bltzl   t1, @9
        vmulz.x vf5, vf5, vf6z
    @9:
        vsubx.xyz vf6, vf4, vf24x
        vaddx.xyz vf7, vf6, vf23x
        vsub.xyz vf6, vf0, vf6
        qmfc2.ni t0, vf7
        vmuly.xyz vf6, vf6, vf23y
        sll     t1, t0, 0
        bgtzl   t1, @10
        vmulx.x vf5, vf5, vf6x
    @10:
        bgtzl   t0, @11
        vmuly.x vf5, vf5, vf6y
    @11:
        pexcw   t1, t0
        bgtzl   t1, @12
        vmulz.x vf5, vf5, vf6z
    @12:
        qmfc2.ni t0, vf5
        sw      t0, 0x48(pd)
        vadd.xyz vf4, vf4, vf29
        sqc2    vf4, 0x0(pd)
        .set reorder
    }
}

/**
 * Fills the particle sprite packets from the particle positions and adds them to the ordering
 * table.
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void fogMakePacket(void) {
    float near_w;

    near_w = 64.0f + (fwork.SumW / 64) / fwork.Projection;
    if (fwork.PartNum <= 0) {
        return;
    }
    spkSetOTPacketS(pwork.pk_env, 0x8000FFFF, 0);
    asm {
        .set noreorder
        lhu        t0, fwork+0x15E68
        andi       t0, t0, 0x8
        beqz       t0, @1
        nop
        lwc1       f12, fwork+0x15DA4
        jal        fogSetFloorY
        nop
    @1:
        lhu        t0, fwork+0x15E68
        andi       t0, t0, 0x10
        beqz       t0, @2
        nop
        lui        t0, 0x4280
        mtc1       t0, near_w
    @2:
        lwc1       f22, fwork+0x15E44
        lui        t0, 0x442F
        mtc1       t0, f21
        nop
        c.le.s     f22, f21
        nop
        bc1t       @3
        nop
        lui        t0, 0x4280
        mtc1       t0, f20
        nop
        add.s      f20, f20, f22
        sub.s      near_w, f20, f21
    @3:
        lui        t0, 0x46FF
        ori        t0, t0, 0xFE00
        mtc1       t0, f20
        nop
        sub.s      f21, f20, near_w
        lui        t0, 0x4000
        mtc1       t0, f20
        nop
        div.s      f21, f21, f20
        add.s      f20, f21, near_w
        swc1       f20, fog_asm_data2+0xC
        swc1       f21, fog_asm_data2+0x1C
        swc1       near_w, fog_asm_data2+0x40
        lui        t0, 0x4400
        mtc1       t0, f20
        nop
        add.s      f20, f20, near_w
        swc1       f20, fog_asm_data2+0x44
        .set reorder
    }
    fwork.SumW = 0;
    __asm__ __volatile__("
    .set noreorder
    lw         t5, fwork+0x15E6C
    nop
    lw         t0, 0x8(%0)
    lw         t1, 0xC(%0)
    lw         t2, 0x10(%0)
    lqc2       vf20, 0x0(t0)
    lqc2       vf21, 0x10(t0)
    lqc2       vf22, 0x20(t0)
    lqc2       vf23, 0x30(t0)
    lqc2       vf24, 0x40(t0)
    lqc2       vf25, 0x50(t0)
    lqc2       vf26, 0x60(t0)
    lqc2       vf12, 0x0(t1)
    lqc2       vf13, 0x10(t1)
    lqc2       vf14, 0x20(t1)
    lqc2       vf15, 0x30(t1)
    lqc2       vf16, 0x0(t2)
    lqc2       vf17, 0x10(t2)
    lqc2       vf18, 0x20(t2)
    lqc2       vf19, 0x30(t2)
    lw         t3, 0x14(%0)
    lw         t4, 0x18(%0)
    lqc2       vf28, 0x0(t3)
    lqc2       vf27, 0x0(t4)
    vsub.xyz   vf9, vf27, vf28
    vmul.xyz   vf8, vf9, vf9
    vaddy.x    vf8, vf8, vf8y
    vaddz.x    vf8, vf8, vf8z
    vdiv       Q, vf8y, vf8x
    lw         t4, 0x1C(%0)
    lqc2       vf27, 0x0(t4)
    vwaitq
    vsubq.w    vf8, vf0, Q
    .word      0x4B8803BD
    pextlb     t5, zero, t5
    pextlh     t5, zero, t5
    qmtc2.ni   t5, vf30
    vwaitq
    vaddq.z    vf7, vf0, Q
    vdiv       Q, vf0w, vf7z
    vitof0.xyzw vf30, vf30
    vmulw.xyzw vf30, vf30, vf24w
    vwaitq
    vmulq.w    vf7, vf0, Q
    xor        t0, t0, t0
    lw         t1, 0x0(%0)
    lw         t2, 0x4(%0)
@loop:
    lqc2       vf4, 0x0(t1)
    vmulax.xyzw ACC, vf12, vf4x
    vmadday.xyzw ACC, vf13, vf4y
    vmaddaz.xyzw ACC, vf14, vf4z
    vmaddw.xyzw vf5, vf15, vf4w
    vdiv       Q, vf0w, vf5w
    lqc2       vf29, 0x40(t1)
    vaddz.xy   vf8, vf0, vf29z
    qmfc2.ni   t4, vf8
    blez       t4, @12
    nop
    vwaitq
    vmulq.xyz  vf5, vf5, Q
    vaddq.x    vf6, vf0, Q
    vaddz.y    vf10, vf0, vf5z
    vsub.xyw   vf9, vf5, vf20
    qmfc2.ni   t4, vf10
    vmulx.z    vf8, vf20, vf6x
    bltz       t4, @12
    vabs.xyw   vf9, vf9
    vsubz.xy   vf9, vf9, vf8z
    vsub.xyw   vf8, vf21, vf9
    vnop
    vnop
    vnop
    vnop
    vnop
    cfc2.ni    t4, vi16
    andi       t4, t4, 0x2
    bnez       t4, @12
    nop
    beql       %2, zero, @4
    addi       t3, zero, 0x80
    vsub.xyz   vf8, vf4, vf27
    vmul.xyz   vf8, vf8, vf8
    vaddy.x    vf8, vf8, vf8y
    vaddz.x    vf8, vf8, vf8z
    .word      0x4A0803BD
    vwaitq
    vmulq.y    vf8, vf25, Q
    vsuby.x    vf8, vf25, vf8y
    vftoi0.x   vf8, vf8
    qmfc2.ni   t3, vf8
    sll        t3, t3, 0
    blez       t3, @12
    nop
@4:
    vsub.y     vf8, vf23, vf4
    qmfc2.ni   t4, vf8
    blez       t4, @12
    vaddw.xy   vf7, vf0, vf0w
    vsuby.x    vf8, vf22, vf8y
    qmfc2.ni   t4, vf8
    sll        t4, t4, 0
    blez       t4, @5
    vaddy.x    vf8, vf0, vf8y
    vmulw.x    vf7, vf8, vf22w
    vsubz.x    vf8, vf7, vf7z
    qmfc2.ni   t4, vf8
    sll        t4, t4, 0
    bgez       t4, @5
    vaddx.y    vf8, vf0, vf7x
    vmulw.y    vf7, vf8, vf7w
@5:
    vaddw.y    vf8, vf0, vf5w
    vsub.y     vf8, vf8, vf24
    qmfc2.ni   t4, vf8
    bgez       t4, @6
    vsubx.w    vf8, vf5, vf24x
    vmulz.w    vf8, vf8, vf24z
    vmulw.z    vf29, vf29, vf8w
@6:
    vmulx.z    vf29, vf29, vf7x
    vaddz.x    vf11, vf0, vf5z
    vftoi0.x   vf11, vf11
    qmfc2.ni   t7, vf11
    lqc2       vf8, 0x10(t1)
    vmulz.x    vf6, vf6, vf22z
    vmulax.xy  ACC, vf16, vf8x
    vmadday.xy ACC, vf17, vf8y
    vmaddz.xy  vf8, vf18, vf8z
    vmuly.x    vf7, vf8, vf8y
    qmfc2.ni   t4, vf7
    sll        t4, t4, 0
    bltzl      t4, @7
    vsub.x     vf7, vf0, vf7
@7:
    vftoi4.xy  vf11, vf5
    qmfc2.ni   t5, vf11
    pexch      t5, t5
    pextlw     t5, t7, t5
    sd         t5, 0x28(t2)
    .word      0x4A0703BD
    vabs.xy    vf8, vf8
    vaddz.x    vf7, vf0, vf26z
    vwaitq
    vmulq.x    vf7, vf7, Q
    .word      0x4A0803BD
    vaddx.xy   vf9, vf0, vf26x
    vwaitq
    vmulq.x    vf9, vf9, Q
    .word      0x4A8803BD
    vwaitq
    vmulq.y    vf9, vf9, Q
    bltzl      t4, @8
    vsub.x     vf7, vf0, vf7
@8:
    vaddw.xy   vf10, vf0, vf0w
    vsubx.xy   vf10, vf10, vf7x
    vsub.xy    vf10, vf10, vf9
    vmaxy.xy   vf10, vf10, vf26y
    vmulx.xy   vf10, vf10, vf6x
    vmul.y     vf10, vf10, vf22
    vsub.xy    vf11, vf5, vf10
    vftoi4.xy  vf11, vf11
    qmfc2.ni   t4, vf11
    pexch      t4, t4
    pextlw     t4, t7, t4
    sd         t4, 0x40(t2)
    sd         t4, 0x80(t2)
    vmul.y     vf10, vf10, vf7
    vadd.xy    vf11, vf5, vf10
    vftoi4.xy  vf11, vf11
    qmfc2.ni   t4, vf11
    pexch      t4, t4
    pextlw     t4, t7, t4
    sd         t4, 0x60(t2)
    vaddw.xy   vf10, vf0, vf0w
    vaddx.xy   vf10, vf10, vf7x
    vsub.xy    vf10, vf10, vf9
    vmaxy.xy   vf10, vf10, vf26y
    vmulx.xy   vf10, vf10, vf6x
    vmul.y     vf10, vf10, vf22
    vsub.x     vf10, vf0, vf10
    vsub.xy    vf11, vf5, vf10
    vftoi4.xy  vf11, vf11
    qmfc2.ni   t4, vf11
    pexch      t4, t4
    pextlw     t4, t7, t4
    sd         t4, 0x50(t2)
    vmul.y     vf10, vf10, vf7
    vadd.xy    vf11, vf5, vf10
    vftoi4.xy  vf11, vf11
    qmfc2.ni   t4, vf11
    pexch      t4, t4
    pextlw     t4, t7, t4
    sd         t4, 0x70(t2)
    vsub.xyz   vf9, vf4, vf28
    vmul.xyz   vf8, vf9, vf9
    vaddy.x    vf8, vf8, vf8y
    vaddz.x    vf8, vf8, vf8z
    vrsqrt     Q, vf0w, vf8x
    lqc2       vf8, 0x30(t1)
    vwaitq
    vmulq.xyz  vf9, vf9, Q
    vsub.xyz   vf8, vf9, vf8
    sqc2       vf9, 0x30(t1)
    vmulax.xy  ACC, vf16, vf8x
    vmadday.xy ACC, vf17, vf8y
    vmaddz.xy  vf8, vf18, vf8z
    vmulz.xy   vf8, vf8, vf23z
    vsub.xy    vf9, vf29, vf8
    qmfc2.ni   t4, vf9
    vftoi0.xy  vf8, vf9
    vitof0.xy  vf8, vf8
    vsub.xy    vf9, vf9, vf8
    sll        t5, t4, 0
    blezl      t5, @9
    vaddw.x    vf9, vf9, vf0w
@9:
    blezl      t4, @10
    vaddw.y    vf9, vf9, vf0w
@10:
    qmfc2.ni   t4, vf9
    sd         t4, 0x40(t1)
    sd         t4, 0x20(t2)
    vsubx.xy   vf8, vf9, vf23x
    qmfc2.ni   t4, vf8
    sd         t4, 0x38(t2)
    sd         t4, 0x78(t2)
    vadd.x     vf8, vf9, vf23
    qmfc2.ni   t4, vf8
    sd         t4, 0x48(t2)
    vmulx.y    vf8, vf7, vf23x
    vadd.y     vf8, vf9, vf8
    qmfc2.ni   t4, vf8
    sd         t4, 0x58(t2)
    vsub.x     vf8, vf9, vf23
    qmfc2.ni   t4, vf8
    sd         t4, 0x68(t2)
    slti       t4, t3, 0x20
    beqz       t4, @11
    nop
    qmtc2.ni   t4, vf8
    vitof0.x   vf8, vf8
    vmulx.z    vf29, vf29, vf8x
    vmulw.z    vf29, vf29, vf25w
@11:
    qmtc2.ni   t3, vf8
    vitof0.x   vf8, vf8
    vmulz.w    vf8, vf30, vf29z
    vmulx.xyz  vf8, vf30, vf8x
    vftoi0.xyzw vf8, vf8
    qmfc2.ni   t4, vf8
    ppach      t4, zero, t4
    lui        t5, 0xFF
    ppacb      t4, zero, t4
    ori        at, zero, 0xFFFF
    addu       t5, t5, at
    srl        t6, t4, 24
    sw         t4, 0x18(t2)
    and        t5, t5, t4
    andi       t6, t6, 0xFF
    sw         t5, 0x30(t2)
    beqz       t6, @12
    vaddw.x    vf11, vf0, vf5w
    lw         t4, 0x0(%3)
    vsub.y     vf11, vf0, vf0
    addu       t5, zero, t2
    vftoi4.xy  vf11, vf11
    addu       t4, t4, t7
    addiu      t7, zero, 0x1
    qmfc2.ni   t6, vf11
    jal        spkSetOTPacketS_asm
    sw         t4, 0x0(%3)
    addiu      t2, t2, 0x90
@12:
    addi       t0, t0, 0x1
    bne        t0, %1, @loop
    addiu      t1, t1, 0x60
    addiu      sp, sp, -0x10
    sq         a0, 0x0(sp)
    jal        fogMakePacket2
    addu       a0, zero, t2
    lq         a0, 0x0(sp)
    addiu      sp, sp, 0x10
    .set reorder
    " : : "r"(&fog_asm_data_p), "r"(fwork.PartNum), "r"(fwork.Flag & 4), "r"(&fwork.SumW));
    if (fwork.Global <= 5 || fwork.Global == 0x6D) {
        fog_view_screen_fog();
        vcopy(fwork.CameraPosV, fwork.OldCameraV);
    }
}

/** Moves the screen-space fog texture with the wind and the camera motion. */
/* scrolls the full-screen fog layer with the camera and the wind */
void fog_view_screen_fog(void) {
    float *FVector;
    float *DVector;
    void *IMatrix;
    float tdx0;
    float tdy0;
    float tdx1;
    float tdy1;
    float tdx2;
    float tdy2;
    int a;

    FVector = (float *)0x70003FF0;
    DVector = (float *)0x70003FE0;
    _sceVu0SubVectorXYZ(DVector, fwork.OldCameraV, fwork.CameraPosV);
    _sceVu0ScaleVector(DVector, DVector, 0.1f / fwork.Projection);
    _sceVu0ScaleVector(FVector, fwork.WindV, 0.005f * fwork.SpeedLevel);
    _sceVu0AddVector(FVector, FVector, DVector);
    IMatrix = fwork.WorldViewM;
    _shApplyRotMatrix(FVector, IMatrix, FVector);
    tdx0 = fwork.sc_tdx - 0.25f * FVector[0];
    tdy0 = fwork.sc_tdy - 0.25f * FVector[1];
    tdx0 -= itof(ffloor(tdx0));
    tdy0 -= itof(ffloor(tdy0));
    fwork.sc_tdx = tdx0;
    fwork.sc_tdy = tdy0;
    tdx1 = tdx0 - 0.125f;
    tdy1 = tdy0 - 0.125f;
    tdx2 = 0.125f + tdx0;
    tdy2 = 0.125f + tdy0;
    a = ((fwork.Color >> 24) * fwork.Alpha) >> 9;
    if (a == 0) {
        a = 1;
    }
    ((unsigned long *)pwork.pk_screen)[3] = (unsigned long)((fwork.Color & 0xFFFFFF) | (a << 24)) | ((unsigned long)0x3F800000 << 32);
    set_st20(pwork.pk_screen, tdy1, tdx1);
    set_st30(pwork.pk_screen, tdx2, tdy2);
    spkSetOTPacketS(pwork.pk_screen, -1, 1);
}

/** Sets the fog colour to (`r`, `g`, `b`, `a`). */
void fogSetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    fwork.Color = r | (g << 8) | (b << 16) | (a << 24);
}

/**
 * Sets the number of particles to `PartNum` (at most PART_MAX); new particles are placed at random.
 */
void fogSetPartNum(int PartNum) {
    int i;

    if (PartNum <= 0) {
        fwork.PartNum = 0;
        return;
    }
    if (PartNum > PART_MAX) {
        PartNum = PART_MAX;
    }
    i = fwork.PartNum;
    if (i < PartNum) {
        for (; i < PartNum; i++) {
            fog_part_newpos(&fwork.Part[i]);
        }
    }
    fwork.PartNum = PartNum;
    fog_asm_data1.gridrate = fwork.GridRate / PartNum;
}

/** Sets the projection distance `Projection` used to size the particles on screen. */
void fogSetProjection(float Projection) {
    fwork.Projection = Projection;
    fog_asm_data2.part_size_proj = fog_asm_data2.part_size_proj2 = fwork.PartSize * Projection;
    fog_asm_data2.proj = Projection;
}

/** Sets the floor height `FloorY` the particles stay above. */
void fogSetFloorY(float FloorY) {
    fwork.FloorY = FloorY;
    fog_asm_data2.floor_y = FloorY;
}

/** Copies the world-to-screen matrix `WorldScreenM`. */
void fogSetWorldScreenM(void *WorldScreenM) {
    mcopy(WorldScreenM, fwork.WorldScreenM);
}

/** Copies the rotation rows of the world-to-view matrix `WorldViewM`. */
void fogSetWorldViewM(void *WorldViewM) {
    mcopy3(WorldViewM, fwork.WorldViewM);
}

/** Sets the fog's world position to `WorldPosV`, unless a stay position is set. */
void fogSetWorldPosV(void *WorldPosV) {
    if (!(fwork.Flag & 0x40)) {
        vcopy(WorldPosV, fwork.WorldPosV);
    }
}

/** Fixes the fog's world position at `WorldPosV` until fogResetStayPos(). */
void fogSetStayPos(void *WorldPosV) {
    fwork.Flag |= 0x40;
    vcopy(WorldPosV, fwork.WorldPosV);
}

/** Lets fogSetWorldPosV() move the fog again. */
void fogResetStayPos(void) {
    fwork.Flag &= ~0x40;
}

/** Sets the point `StayPoint` the particles gather around. */
void fogSetStayPoint(void *StayPoint) {
    _sceVu0CopyVectorXYZ(fwork.StayPoint, StayPoint);
}

/** Clears the stay point. */
void fogResetStayPoint(void) {
    *(u_long128 *)fwork.StayPoint = 0;
}

/** Sets the camera position to `CameraPosV`. */
void fogSetCameraPosV(void *CameraPosV) {
    vcopy(CameraPosV, fwork.CameraPosV);
}

/** Derives the fog volume's local position from the world and camera positions. */
void fogSetLocalPosV(void) {
    float *FVector;
    float *TVector;
    float d;
    float dx;
    float mp;

    FVector = (float *)0x70003FF0;
    TVector = (float *)0x70003FE0;
    _sceVu0SubVector(FVector, fwork.WorldPosV, fwork.CameraPosV);
    d = _shVectorLength(FVector);
    if (fwork.Flag & 0x20) {
        vcopy(fwork.LocalPosV, TVector);
        dx = FVector[0] / fabsf(FVector[2]);
    }
    if (d > 2000.0f) {
        _sceVu0ScaleVector(FVector, FVector, 2000.0f / d);
        _sceVu0AddVector(fwork.LocalPosV, fwork.CameraPosV, FVector);
    } else {
        vcopy(fwork.WorldPosV, fwork.LocalPosV);
        if (fwork.Double) {
            fwork.LocalPosV[1] = fwork.FloorY;
        }
    }
    if (fwork.Flag & 0x20) {
        mp = fwork.MaxPos;
        dx *= TVector[2] - fwork.WorldPosV[2];
        if (dx > mp) {
            dx = mp;
        }
        if (dx < -mp) {
            dx = -mp;
        }
        fwork.LocalPosV[0] += dx;
        fwork.LocalPosV[1] = TVector[1];
        fwork.LocalPosV[2] = TVector[2];
    }
}

/** Sets the wind speed multiplier `SpeedLevel`. */
void fogSetSpeedLevel(float SpeedLevel) {
    fwork.SpeedLevel = SpeedLevel;
}

/** Returns the address of the TEX0 register data in the fog's environment packet. */
unsigned long *fogTex0Adr(void) {
    return pwork.pk_tex0;
}
