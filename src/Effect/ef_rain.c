/* ef_rain.c: rain drops falling around the camera, with a small spray where they hit. */
#include "sh2.h"
#include "fog_param.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "fi_libvu0_inline.h"
#include "sdk/libgraph.h"
#include "sdk/libvu0.h"

#define PK_ADD(v) (*spack.pos++ = (v))

struct RGBA4 {
    unsigned char c[4];
};

/* One RGBA colour as bytes, 4-aligned (invented name; see efRainDropDrawLINE). */
typedef unsigned char RGBA_ROW[4] __attribute__((aligned(4)));

static void efRainDropInitSpray(struct _EF_RAINDROP_DATA *dat, float *pos);
static int efRainDropDrawSpray(struct _EF_RAINDROP_DATA *dat);
static void efRainDropDrawLINE(struct _EF_RAIN_LINE *p);

struct EF_DRAW_DATA efRainDrawData;

/*
 * Matching: fitted stand-in for double code (docs/stand-ins.md): efRainDropDrawLINE keeps its
 * temporaries in a2/a3/t0, the software-double register mode. It stands before the file's first
 * function (EFCTSetRainDrop), where the original's lines also hold the file's declarations, so
 * whether code stood here is unknown (docs/stand-ins.md); the file's other gaps are all usual.
 */
STRIPPED_DOUBLE_CODE()

/**
 * Starts lev rain-drop tasks (15 drops each).
 * @param lev number of tasks
 * @return 1, or 0 if the task list ran out
 */
int EFCTSetRainDrop(int lev) {
    int i;
    struct _EF_EF_RAINDROP_TASK *ptr;

    for (i = 0; i < lev; i++) {
        ptr = (struct _EF_EF_RAINDROP_TASK *)shTSKSetTask((void (*)(void *))EFCTRainDropMain, 4);
        if (ptr != NULL) {
            ptr->exe.atr = 10;
            ptr->exe.mode = 0;
        } else {
            return 0;
        }
    }
    return 1;
}

/** Deletes every rain-drop task. */
void EFCTDelRainDrop(void) {
    struct _shTskTASK *ptr;
    struct _shTskTASK *seekp;

    seekp = shTskTaskListTop[4]->exe.next;
    while (1) {
        ptr = shTSKSearchTaskWithAtr(10, seekp, 4);
        if (ptr == NULL) {
            break;
        }
        seekp = ptr->exe.next;
        shTSKDelTask(ptr);
    }
}

/**
 * Task function of a group of 15 rain drops: places them in front of the camera, makes them
 * fall as lines, and when the last one hits the floor draws a short spray there and starts over.
 * @param ptr the task (a _EF_EF_RAINDROP_TASK)
 */
void EFCTRainDropMain(struct _shTskTASK *ptr) {
    struct _EF_RAIN_LINE line;
    int j;
    float campos[4];
    struct _EF_RAINDROP_DATA *pt;
    struct _CL_VHIT_RESULT res;
    float dif;
    float camang[4];
    float pos[4];
    float m0[4][4];

    pt = &((struct _EF_EF_RAINDROP_TASK *)ptr)->data;
    vcGetNowCamPos(campos);
    line.rgba[0] = 0x80;
    line.rgba[1] = 0x80;
    line.rgba[2] = 0x80;
    line.rgba[3] = 0x80;
    HH_ClassWrapper_AmbientColor_Get(efRainDrawData.ambcol);
    efRainDrawData.spoton = HH_ClassWrapper_SpotLight_Enable_Check();
    if (efRainDrawData.spoton) {
        HH_ClassWrapper_SpotLight_EnvironmentParameter_Get(efRainDrawData.lightpos, efRainDrawData.lightdir,
                                                           efRainDrawData.lightcol, efRainDrawData.lightpar);
    }
    sceVu0CopyMatrix(efRainDrawData.wcm, cam0.view_clip);
    shMulMatrix(efRainDrawData.wcm, efRainDrawData.wcm, VbWvsMatrix.wvm);
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0000, 0);
    PK_ADD(GS_SET_ALPHA(0, 1, 0, 1, 0x80));
    PK_ADD(GS_REG_ALPHA_1);
    PK_ADD(GS_SET_TEST(1, 1, 0, 0, 0, 0, 1, 3));
    PK_ADD(GS_REG_TEST_1);
    switch (ptr->exe.mode) {
    case 0:
        _sceVu0UnitVector(pos);
        pos[2] = 1000.0f;
        vwGetViewAngle(camang);
        _sceVu0UnitMatrix(m0);
        shRotMatrixY(m0, m0, camang[1]);
        _sceVu0ApplyMatrix(pos, m0, pos);
        _shAddVector(campos, campos, pos);
        for (j = 0; j < 15; j++) {
            pt->p[j][0] = campos[0] + 500.0f * (5.0f * (shRandF() - 0.5f));
            pt->p[j][1] = (campos[1] + -1.0f * (500.0f * (8.0f * (shRandF() - 0.5f)))) - 1000.0f;
            pt->p[j][2] = campos[2] + 500.0f * (5.0f * (shRandF() - 0.5f));
            pt->p[j][3] = 1.0f;
        }
        ptr->exe.mode++;
        break;
    case 1:
        dif = 4000.0f / shGetFPS();
        for (j = 0; j < 15; j++) {
            pt->p[j][1] += dif;
            vcopy(pt->p[j], line.v[0]);
            vcopy(pt->p[j], line.v[1]);
            line.v[1][1] += 100.0f;
            efRainDropDrawLINE(&line);
        }
        line.v[0][1] -= 250.0f;
        clCheckHitEyesOnlyFloorCeil(&res, line.v[0], line.v[1]);
        if (res.kind == 1) {
            efRainDropInitSpray(pt, res.hobj.wall.cp);
            ptr->exe.mode++;
        } else if (!(line.v[1][1] < 2500.0f + campos[1])) {
            ptr->exe.mode = 0;
        }
        break;
    case 2:
        if (efRainDropDrawSpray(pt)) {
            ptr->exe.mode = 0;
        }
        break;
    }
    spkCloseGiftag();
}

static void efRainDropInitSpray(struct _EF_RAINDROP_DATA *dat, float *pos) {
    vcopy(pos, dat->v[0]);
    dat->v[0][1] -= 25.0f;
    dat->v[0][3] = 1.0f;
    vcopy(dat->v[0], dat->v[1]);
    vcopy(dat->v[0], dat->v[2]);
    vcopy(dat->v[0], dat->v[3]);
    dat->v[1][0] = dat->v[0][0] + 500.0f * ((shRandF() - 0.5f) / 10.0f);
    dat->v[2][2] = dat->v[0][2] + 500.0f * ((shRandF() - 0.5f) / 10.0f);
    dat->v[3][0] = dat->v[0][0] + 500.0f * ((shRandF() - 0.5f) / 10.0f);
    dat->v[3][2] = dat->v[0][2] + 500.0f * ((shRandF() - 0.5f) / 10.0f);
    dat->life = shRandI() % 2;
}

static int efRainDropDrawSpray(struct _EF_RAINDROP_DATA *dat) {
    int i;
    struct _EF_RAIN_LINE line;

    line.rgba[0] = 0xFF;
    line.rgba[1] = 0xFF;
    line.rgba[2] = 0xFF;
    line.rgba[3] = 0xFF;
    for (i = 0; i < 4; i++) {
        vcopy(dat->v[i], line.v[0]);
        vcopy(dat->v[i], line.v[1]);
        line.v[1][1] -= 10.0f;
        efRainDropDrawLINE(&line);
    }
    if (dat->life) {
        dat->life--;
        return 0;
    }
    return 1;
}


/* FAKEMATCH: rgba is declared through an aligned(4) typedef of its row type, which nothing shows the
 * original had: it gives rgba the original's 4-aligned stack slot between q and w
 * (docs/matching-notes.md#ef_rain-efraindropdrawline). */
static void efRainDropDrawLINE(struct _EF_RAIN_LINE *p) {
    float q;
    float z;
    int sp[2][4];
    int i;
    unsigned char fog;
    unsigned char alpha;
    RGBA_ROW rgba[2];
    float w;
    float zn;
    float perc;

    q = 1.0f;
    for (i = 0; i < 2; i++) {
        if (HH_ClassWrapper_RotTrans_PerspectiveProjection_Clip(sp[i], &w, VbWvsMatrix.wsm, efRainDrawData.wcm, p->v[i])) {
            return;
        }
    }
    zn = VbScreenInfo.farz;
    perc = 64.0f / zn;
    alpha = iclamp((unsigned int)(sp[0][2] * perc), 0, 0x40);
    fog = iclamp((int)(FogParamB(Env_ctl.fogparm.fl32) + w * FogParamA(Env_ctl.fogparm.fl32)), 0, 0xFF);
    *(struct RGBA4 *)rgba[0] = *(struct RGBA4 *)p->rgba;
    rgba[1][0] = rgba[0][0] - 0x40;
    rgba[1][1] = rgba[0][1] - 0x40;
    rgba[1][2] = rgba[0][2] - 0x40;
    if (efRainDrawData.spoton) {
        float perc;

        perc = HH_ClassWrapper_SpotLight_ColorRatio_Calculator(efRainDrawData.lightpos, efRainDrawData.lightdir, p->v[0],
                                                              efRainDrawData.lightpar[0], efRainDrawData.lightpar[2]);
        perc = fclamp(0.3f + perc, 0.0f, 1.0f);
        alpha = iclamp((int)(80.0f * perc), 0, 0x50);
    } else {
        perc = efRainDrawData.ambcol[2] / 2.0f;
        alpha = iclamp((int)(alpha * perc), 0, 0xFF);
    }
    spkCloseOpenDGiftag(0x5400000000008000, 0x41410);
    PK_ADD(GS_SET_PRIM(1, 1, 0, 1, 1, 0, 0, 0, 0));
    PK_ADD(GS_SET_RGBAQ(rgba[1][0], rgba[1][1], rgba[1][2], alpha, *(unsigned int *)&q));
    PK_ADD(GS_SET_XYZF(sp[0][0], sp[0][1], sp[0][2], fog));
    PK_ADD(GS_SET_RGBAQ(rgba[0][0], rgba[0][1], rgba[0][2], alpha, *(unsigned int *)&q));
    PK_ADD(GS_SET_XYZF(sp[1][0], sp[1][1], sp[1][2], fog));
}
