/*
 * en_effect.c: particle effects of the enemies, run as tasks in task list 5: poison fog,
 * spray, the insect sprites and the TYU 2D quad, with their GS environment packets.
 */
#include "enemy.h"
#include "fi_libvu0_inline.h"
#include "sdk/eekernel.h"
#include "sdk/libvu0.h"

/* GS ST: s in the low word, t in the high word */
inline unsigned long pkST(float s, float t) {
    unsigned long r;

    asm {
        mfc1   r, s
        mfc1   t7, t
        pextlw r, t7, r
    }
    return r;
}

/* GS RGBAQ. Matching: parameter order fitted to the call's evaluation order (last to first). */
inline unsigned long pkRGBAQ(int a, unsigned long r, int b, int g, float q) {
    asm {
        pextlb r, g, r
        pextlb t7, a, b
        mfc1   t6, q
        pextlh r, t7, r
        pextlw r, t6, r
    }
    return r;
}

/* screen xyz of v (12.4 x/y, integer z) packed as a GS XYZ */
static inline unsigned long _shFtoiXYZ(float *v) {
    unsigned long r;

    __asm__ __volatile__("
    lqc2      vf5, 0x0(%1)
    vftoi4.xy vf4, vf5
    vftoi0.z  vf4, vf5
    qmfc2.ni  %0, vf4
    pexch     t7, %0
    pextuw    %0, zero, %0
    pextlw    %0, %0, t7
    " : "=r"(r) : "r"(v));
    return r;
}


int en_efct_kind_list[4] = { 30, 31, 32, -1 };

float sintable[16] = {
    0.0f,  0.3826834f,  0.7071068f,  0.9238795f,  1.0f,  0.9238795f,  0.7071068f,  0.3826834f,
    -0.0f, -0.3826834f, -0.7071068f, -0.9238795f, -1.0f, -0.9238795f, -0.7071068f, -0.3826834f,
};

struct EnEfctEnvData EnEfctEnvData;

#define EN_EFCT_ENV(buf, id)                         \
    spkStartPacketS(buf);                            \
    spkOpenDGiftagS(0x1000000000008000, 0xE);        \
    PK_ADD(0);                                       \
    PK_ADD(6);                                       \
    PK_ADD(0);                                       \
    PK_ADD(8);                                       \
    PK_ADD(0x44);                                    \
    PK_ADD(0x42);                                    \
    PK_ADD(0xFFFFFFFF00000061);                      \
    PK_ADD(0x14);                                    \
    PK_ADD(0x13A0001C0);                             \
    PK_ADD(0x4E);                                    \
    spkCloseGiftagS();                               \
    spkEndPacketS();                                 \
    spkSetEnvPacket(buf, id)

static unsigned int enMultColor(unsigned int color, int rate) {
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int a;

    r = ((color & 0xFF) * rate) >> 7;
    g = (((color >> 8) & 0xFF) * rate) >> 7;
    b = (((color >> 16) & 0xFF) * rate) >> 7;
    a = (((color >> 24) & 0xFF) * rate) >> 7;
    return r | (g << 8) | (b << 16) | (a << 24);
}

/** Builds the GS environment packets of the enemy effects (poison fog, insects, spray, TYU 2D). */
void enEfctInit(void) {
    SyncDCache(&EnEfctEnvData, (void *)((unsigned int)&EnEfctEnvData + sizeof(EnEfctEnvData)));
    InvalidDCache(&EnEfctEnvData, (void *)((unsigned int)&EnEfctEnvData + sizeof(EnEfctEnvData)));
    EN_EFCT_ENV(EnEfctEnvData.Poison, 2);
    EN_EFCT_ENV(EnEfctEnvData.Insect, 6);
    EN_EFCT_ENV(EnEfctEnvData.Spray, 7);
    EN_EFCT_ENV(EnEfctEnvData.Tyu2D, 8);
}

/* Matching: fitted stand-in for double code (docs/stand-ins.md): enEfctMoveSpray uses a2 for its
 * temporaries only after double code has been compiled. The original's line table leaves 17 lines
 * between enEfctInit and enEfctTexInit, room for a dead-stripped function. */
STRIPPED_DOUBLE_CODE()

/** Texture setup for the enemy effects (empty). */
void enEfctTexInit(void) {
}

/** Deletes every running enemy-effect task. */
void enEfctClear(void) {
    int kind;
    int n;
    struct _shTskTASK *pt;
    struct _shTskTASK *pnext;

    n = 0;
    while ((kind = en_efct_kind_list[n++]) != -1) {
        pt = shTSKSearchTaskWithAtr(kind, shTskTaskListTop[5]->exe.next, 5);
        while (pt) {
            pnext = shTSKSearchTaskWithAtr(kind, pt->exe.next, 5);
            shTSKCutTask(pt);
            pt = pnext;
        }
    }
}

/** Draws every running enemy-effect task, then sets the textures the drawn effects need. */
void enEfctDraw(void) {
    int kind;
    int n;
    struct _shTskTASK *pt;

    n = 0;
    EnEfctEnvData.SetPoisonTex = 0;
    EnEfctEnvData.SetSprayTex = 0;
    EnEfctEnvData.SetTYU2DTex = 0;
    while ((kind = en_efct_kind_list[n++]) != -1) {
        pt = shTSKSearchTaskWithAtr(kind, shTskTaskListTop[5]->exe.next, 5);
        while (pt) {
            switch (kind) {
            case 30:
                enEfctDrawPoisonFog((struct EnEFCT_TASK *)pt);
                break;
            case 31:
                enEfctDrawSpray((struct EnEFCT_TASK *)pt);
                break;
            case 32:
                enEfctDrawTYU2D((struct EnEFCT_TASK *)pt);
                break;
            }
            pt = shTSKSearchTaskWithAtr(kind, pt->exe.next, 5);
        }
    }
    n = 0;
    if (EnEfctEnvData.SetPoisonTex) {
        enEfctSetPoisonTex();
        n = 1;
    }
    if (EnEfctEnvData.SetInsectTex) {
        enEfctSetInsectTex();
        n = 1;
    }
    if (EnEfctEnvData.SetSprayTex) {
        enEfctSetSprayTex();
        n = 1;
    }
    if (EnEfctEnvData.SetTYU2DTex) {
        enEfctSetTYU2DTex();
        n = 1;
    }
    if (n) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0x80000000, 0);
        PK_ADD(0);
        PK_ADD(0x3F);
        spkCloseGiftag();
    }
    EnEfctEnvData.SetInsectTex_bak = EnEfctEnvData.SetInsectTex;
    EnEfctEnvData.SetInsectTex = 0;
}

/** Starts a poison-fog puff.
 * @param pos start position
 * @param vec direction and size of the puff (randomized a little) */
void enEfctSetPoisonFog(float *pos, float *vec) {
    struct EnEFCT_TASK *pt;

    pt = (struct EnEFCT_TASK *)shTSKSetTask((void (*)(void *))enEfctMovePoisonFog, 5);
    if (pt) {
        pt->exe.atr = 30;
        _sceVu0CopyVectorXYZ(pt->pfog.pos, pos);
        pt->step = 0;
        pt->count = 0;
        pt->pfog.alpha = 0;
        vcopy(vec, pt->pfog.vec);
        pt->pfog.vec[0] += shSway1f(-100.0f, 10.0f);
        pt->pfog.vec[1] += shSway1f(-100.0f, 10.0f);
        pt->pfog.vec[2] += shSway1f(-100.0f, 10.0f);
        pt->pfog.size = lengthXYZ(pt->pfog.vec);
        _shNormalize(pt->pfog.vec, pt->pfog.vec);
        pt->pfog.s = shRandF();
        pt->pfog.t = shRandF();
        switch (enGetWorldCondition()) {
        case 4:
            if (enLocalWork.This && enCheckDarkOrBright(enLocalWork.This->scp)) {
                pt->pfog.color = 0x303030;
            } else {
                pt->pfog.color = 0x181818;
            }
            break;
        default:
            pt->pfog.color = 0x808080;
            break;
        }
    }
}

/** Poison-fog task: the puff spreads, rises and fades out in three timed steps.
 * @param pt the task */
void enEfctMovePoisonFog(struct EnEFCT_TASK *pt) {
    float vec[4];
    float d;

    switch (pt->step) {
    case 0:
        d = (float)pt->count / enCalcTimer(6);
        _shScaleVector(vec, pt->pfog.vec, 0.2f * pt->pfog.size / enCalcTimer(6));
        _shAddVectorXYZ(pt->pfog.pos, pt->pfog.pos, vec);
        pt->pfog.alpha = 32.0f * d;
        pt->pfog.scattar = 0.5f * d;
        if ((pt->count += shGetDF()) >= enCalcTimer(6)) {
            pt->step++;
        }
        break;
    case 1:
        d = (float)(pt->count - enCalcTimer(6)) / enCalcTimer(6);
        _shScaleVector(vec, pt->pfog.vec, 0.25f * pt->pfog.size / enCalcTimer(6));
        vec[1] += 100.0f * shGetDT();
        _shAddVectorXYZ(pt->pfog.pos, pt->pfog.pos, vec);
        pt->pfog.alpha = 32;
        pt->pfog.scattar = 0.5f + 0.5f * d;
        if ((pt->count += shGetDF()) >= enCalcTimer(6) + enCalcTimer(6)) {
            pt->step++;
        }
        break;
    case 2:
        d = (float)(pt->count - enCalcTimer(6) - enCalcTimer(6)) / enCalcTimer(10);
        _shScaleVector(vec, pt->pfog.vec, 0.3f * pt->pfog.size / enCalcTimer(10));
        vec[1] += 100.0f * shGetDT();
        _shAddVectorXYZ(pt->pfog.pos, pt->pfog.pos, vec);
        pt->pfog.alpha = 32.0f * (1.0f - d);
        pt->pfog.scattar = 1.0f + 0.2f * d;
        if ((pt->count += shGetDF()) >= enCalcTimer(6) + enCalcTimer(6) + enCalcTimer(10)) {
            shTSKDelTask((struct _shTskTASK *)pt);
        }
        break;
    }
}

/** Draws a poison-fog puff as a screen-facing fan of 16 triangles.
 * @param pt the task */
void enEfctDrawPoisonFog(struct EnEFCT_TASK *pt) {
    float v0[4];
    float vec[4];
    float q;
    float s;
    float dx;
    float dy;
    float ds;
    float dt;
    int i;

    q = _shRotTransPersQ(v0, VbWvsMatrix.wsm, pt->pfog.pos);
    dt = 0.25f;
    dx = VbScreenInfo.scr_z * (q * (0.25f * pt->pfog.size * pt->pfog.scattar));
    dy = 1.3333334f * dx;
    if (v0[0] + dx < 1792.0f || v0[0] - dx > 2304.0f || v0[1] + dy < 1792.0f || v0[1] - dy > 2304.0f ||
        v0[2] < 0.0f || v0[3] < 128.0f || v0[3] > 32767.0f) {
        return;
    }
    spkOpenDGiftag(0x102EC00000008000, 0xE, ftoi4(v0[3]), 2);
    PK_ADD(pt->pfog.color | ((unsigned long)pt->pfog.alpha << 24) | 0x3F80000000000000);
    PK_ADD(1);
    PK_ADD(pkST(pt->pfog.s, pt->pfog.t));
    PK_ADD(2);
    PK_ADD(_shFtoiXYZ(v0));
    PK_ADD(0xD);
    vcopy(v0, vec);
    PK_ADD(pt->pfog.color | 0x3F80000000000000);
    PK_ADD(1);
    for (i = 0; i < 17; i++) {
        vec[0] = v0[0] + dx * sintable[(i + 4) & 15];
        vec[1] = v0[1] + dy * sintable[i & 15];
        PK_ADD(pkST(pt->pfog.s + dt * sintable[(i + 4) & 15], pt->pfog.t + dt * sintable[i & 15]));
        PK_ADD(2);
        PK_ADD(_shFtoiXYZ(vec));
        PK_ADD(5);
    }
    spkCloseGiftag();
    EnEfctEnvData.SetPoisonTex = 1;
}

/** Sets the poison-fog texture (TEX0) in its environment packet. */
void enEfctSetPoisonTex(void) {
    *(unsigned long *)&EnEfctEnvData.Poison[1] = HH_Effect_Object_Texture_GS_Register_Tex0_Get(2, 0);
}

/** Draws an insect enemy as two screen-facing animated quads, the second offset by twin_dist.
 * @param p the insect's enemy work (struct EnLOCAL_DATA *) */
void enEfctDrawInsect(void *p) {
    struct EnLOCAL_DATA *dp;
    float v0[4];
    float v1[4];
    float m0[4][4];
    float q;
    float s;
    float dx;
    float dy;
    unsigned short u;
    unsigned short v;

    dp = p;
    _sceVu0CopyVectorXYZ(v0, (float *)&dp->scp->pos);
    q = _shRotTransPersQ(v0, VbWvsMatrix.wsm, v0);
    dx = VbScreenInfo.scr_z * (dp->size * q);
    dy = 32.0f * (1.3333334f * dx) / 32.0f;
    if (v0[0] + dx < 1792.0f || v0[0] - dx > 2304.0f || v0[1] + dy < 1792.0f || v0[1] - dy > 2304.0f ||
        v0[2] < 0.0f || v0[3] < 128.0f || v0[3] > 32767.0f) {
        return;
    }
    u = (dp->anim % 2) * 32;
    v = (dp->anim / 2) * 32;
    spkOpenDGiftag(0xA400000000008000, 0x53D353D310, ftoi4(v0[3]), 6);
    PK_ADD(0x156);
    PK_ADD(0x3F80000080808080);
    PK_ADD((unsigned long)u << 4 | (unsigned long)v << 20);
    vcopy(v0, v1);
    v1[0] -= dx;
    v1[1] -= dy;
    PK_ADD(_shFtoiXYZ(v1));
    PK_ADD((unsigned long)(u + 32) << 4 | (unsigned long)(v + 32) << 20);
    v1[0] += dx;
    v1[1] += dy;
    PK_ADD(_shFtoiXYZ(v1));
    _sceVu0CopyVectorXYZ(v0, (float *)&dp->scp->pos);
    _sceVu0UnitMatrix(m0);
    shRotMatrixZ(m0, m0, dp->ins.view_rot[2]);
    shRotMatrixY(m0, m0, dp->ins.view_rot[1]);
    shRotMatrixX(m0, m0, dp->ins.view_rot[0]);
    v1[0] = v1[1] = v1[2] = dp->ins.twin_dist;
    _shApplyRotMatrix(v1, m0, v1);
    _shAddVectorXYZ(v0, v0, v1);
    q = _shRotTransPersQ(v0, VbWvsMatrix.wsm, v0);
    dx = VbScreenInfo.scr_z * (dp->size * q);
    dy = 32.0f * (1.3333334f * dx) / 32.0f;
    PK_ADD((unsigned long)u << 4 | (unsigned long)v << 20);
    vcopy(v0, v1);
    v1[0] -= dx;
    v1[1] -= dy;
    PK_ADD(_shFtoiXYZ(v1));
    PK_ADD((unsigned long)(u + 32) << 4 | (unsigned long)(v + 32) << 20);
    v1[0] += dx;
    v1[1] += dy;
    PK_ADD(_shFtoiXYZ(v1));
    spkCloseGiftag();
    EnEfctEnvData.SetInsectTex = 1;
}

/** Sets the insect texture (TEX0) in its environment packet. */
void enEfctSetInsectTex(void) {
    *(unsigned long *)&EnEfctEnvData.Insect[1] = HH_Effect_Object_Texture_GS_Register_Tex0_Get(0x12, 0);
}

/** Starts a spray puff.
 * @param pos start position
 * @param vec direction and size of the puff (randomized a little)
 * @param color RGBA colour, darkened in dark worlds
 * @param timer length of the effect in frames */
void enEfctSetSpray(float *pos, float *vec, unsigned int color, int timer) {
    struct EnEFCT_TASK *pt;

    pt = (struct EnEFCT_TASK *)shTSKSetTask((void (*)(void *))enEfctMoveSpray, 5);
    if (pt) {
        pt->exe.atr = 31;
        _sceVu0CopyVectorXYZ(pt->spray.pos, pos);
        pt->step = 0;
        pt->count = 0;
        pt->spray.alpha = 0;
        vcopy(vec, pt->spray.vec);
        pt->spray.vec[0] += shSway1f(-50.0f, 1.0f);
        pt->spray.vec[1] += shSway1f(-50.0f, 1.0f);
        pt->spray.vec[2] += shSway1f(-50.0f, 1.0f);
        pt->spray.size = lengthXYZ(pt->spray.vec);
        _shNormalize(pt->spray.vec, pt->spray.vec);
        pt->spray.s = shRandF();
        pt->spray.t = shRandF();
        pt->spray.timer = 0.6f * timer;
        pt->spray.timer1 = 0.3f * timer;
        pt->spray.timer2 = 0.3f * timer;
        switch (enGetWorldCondition()) {
        case 4:
            if (enCheckDarkOrBrightPlayer()) {
                color = enMultColor(color, 48);
            } else {
                color = enMultColor(color, 24);
            }
        }
        pt->spray.color = color;
    }
}

/** Spray task: the puff spreads, rises and fades out in three timed steps.
 * @param pt the task */
void enEfctMoveSpray(struct EnEFCT_TASK *pt) {
    float vec[4];
    float d;
    float rt;

    switch (pt->step) {
    case 0:
        rt = 1.0f / itof(pt->spray.timer1);
        d = rt * pt->count;
        _shScaleVector(vec, pt->spray.vec, 0.3f * pt->spray.size * rt);
        _shAddVectorXYZ(pt->spray.pos, pt->spray.pos, vec);
        pt->spray.alpha = 32.0f * d;
        pt->spray.scattar = 0.2f * d;
        if ((pt->count += shGetDF()) >= pt->spray.timer1) {
            pt->step++;
        }
        break;
    case 1:
        rt = 1.0f / itof(pt->spray.timer2);
        d = rt * (pt->count - pt->spray.timer1);
        _shScaleVector(vec, pt->spray.vec, 0.3f * pt->spray.size * rt);
        vec[1] += 100.0f * shGetDT();
        _shAddVectorXYZ(pt->spray.pos, pt->spray.pos, vec);
        pt->spray.alpha = 32;
        pt->spray.scattar = 0.2f + 0.2f * d;
        if ((pt->count += shGetDF()) >= pt->spray.timer) {
            pt->step++;
        }
        break;
    case 2:
        rt = 1.0f / itof(pt->spray.timer);
        d = rt * (pt->count - pt->spray.timer);
        _shScaleVector(vec, pt->spray.vec, 0.5f * pt->spray.size * rt);
        vec[1] += 100.0f * shGetDT();
        _shAddVectorXYZ(pt->spray.pos, pt->spray.pos, vec);
        pt->spray.alpha = 32.0f * (1.0f - d);
        pt->spray.scattar = 0.4f + 0.4f * d;
        if ((pt->count += shGetDF()) >= pt->spray.timer * 2) {
            shTSKDelTask((struct _shTskTASK *)pt);
        }
        break;
    }
}

/** Draws a spray puff as a screen-facing fan of 16 triangles.
 * @param pt the task */
void enEfctDrawSpray(struct EnEFCT_TASK *pt) {
    float v0[4];
    float vec[4];
    float q;
    float s;
    float dx;
    float dy;
    float ds;
    float dt;
    int i;
    int r;
    int g;
    int b;
    int a;

    q = _shRotTransPersQ(v0, VbWvsMatrix.wsm, pt->spray.pos);
    dx = VbScreenInfo.scr_z * (q * (0.25f * pt->spray.size * pt->spray.scattar));
    dy = 1.3333334f * dx;
    dt = 0.125f;
    if (v0[0] + dx < 1792.0f || v0[0] - dx > 2304.0f || v0[1] + dy < 1792.0f || v0[1] - dy > 2304.0f ||
        v0[2] < 0.0f || v0[3] < 128.0f || v0[3] > 32767.0f) {
        return;
    }
    r = iclamp(pt->spray.color & 0xFF, 0, 0xCC);
    g = iclamp((((pt->spray.color >> 8) & 0xFF) * 144) >> 7, 0, 0xE2);
    b = iclamp((((pt->spray.color >> 16) & 0xFF) * 160) >> 7, 0, 0xFF);
    a = iclamp((pt->spray.alpha * ((pt->spray.color >> 24) & 0xFF)) >> 7, 0, 0xFF);
    spkOpenDGiftag(0x102EC00000008000, 0xE, ftoi4(v0[3]), 7);
    PK_ADD((unsigned long)r | (unsigned long)g << 8 | (unsigned long)b << 16 | (unsigned long)a << 24 |
           0x3F80000000000000);
    PK_ADD(1);
    PK_ADD(pkST(pt->spray.s, pt->spray.t));
    PK_ADD(2);
    PK_ADD(_shFtoiXYZ(v0));
    PK_ADD(0xD);
    vcopy(v0, vec);
    PK_ADD((unsigned long)r | (unsigned long)g << 8 | (unsigned long)b << 16 | 0x3F80000000000000);
    PK_ADD(1);
    for (i = 0; i < 17; i++) {
        vec[0] = v0[0] + dx * sintable[(i + 4) & 15];
        vec[1] = v0[1] + dy * sintable[i & 15];
        PK_ADD(pkST(pt->spray.s + dt * sintable[(i + 4) & 15], pt->spray.t + dt * sintable[i & 15]));
        PK_ADD(2);
        PK_ADD(_shFtoiXYZ(vec));
        PK_ADD(5);
    }
    spkCloseGiftag();
    EnEfctEnvData.SetSprayTex = 1;
}

/** Sets the spray texture (TEX0) in its environment packet. */
void enEfctSetSprayTex(void) {
    *(unsigned long *)&EnEfctEnvData.Spray[1] = HH_Effect_Object_Texture_GS_Register_Tex0_Get(2, 0);
}

/** Starts a TYU 2D effect: a flat lit quad lying on a surface.
 * @param pos position (lowered by 10)
 * @param rot rotation of the quad */
void enEfctSetTYU2D(float *pos, float *rot) {
    struct EnEFCT_TASK *pt;

    pt = (struct EnEFCT_TASK *)shTSKSetTask((void (*)(void *))enEfctMoveTYU2D, 5);
    if (pt) {
        pt->exe.atr = 32;
        _sceVu0CopyVectorXYZ(pt->tyu2d.pos, pos);
        pt->tyu2d.pos[1] -= 10.0f;
        vcopy(rot, pt->tyu2d.rot);
    }
}

/** TYU 2D task move function (does nothing).
 * @param pt the task */
void enEfctMoveTYU2D(struct EnEFCT_TASK *pt) {
    void *tmp;

    tmp = pt; /* Matching: reconstructed dead store: the DWARF lists pt and tmp (both referenced), the code is empty. */
}

/** Draws a TYU 2D effect: a 150x150 quad rotated by its rot, lit by the stage lights.
 * @param pt the task */
void enEfctDrawTYU2D(struct EnEFCT_TASK *pt) {
    float pos[4][4];
    float vec[4];
    float vec2[4];
    float mat[4][4];
    float rot[4][4];
    float q[4];
    float w;
    float *ppos;
    int i;
    int r;
    int g;
    int b;
    float amb[4];
    float lpos[4];
    float ldir[4];
    float lcolor[4];
    float para[4];
    float rate;

    ppos = pt->tyu2d.pos;
    _sceVu0UnitMatrix(rot);
    vzero(vec);
    shRotMatrixZ(rot, rot, pt->tyu2d.rot[2]);
    shRotMatrixX(rot, rot, pt->tyu2d.rot[0]);
    shRotMatrixY(rot, rot, pt->tyu2d.rot[1]);
    vec[0] = -75.0f;
    vec[2] = 75.0f;
    mcopy(VbWvsMatrix.wsm, mat);
    w = 0.0f;
    for (i = 0; i < 4; i++) {
        _sceVu0ApplyMatrix(vec2, rot, vec);
        _shAddVector(pos[i], ppos, vec2);
        pos[i][3] = 1.0f;
        q[i] = _shRotTransPersQ(pos[i], mat, pos[i]);
        if (pos[i][0] < 0.0f || pos[i][0] >= 4096.0f || pos[i][1] < 0.0f || pos[i][1] >= 4096.0f ||
            pos[i][2] < 0.0f) {
            return;
        }
        if (pos[i][3] > w) {
            w = pos[i][3];
        }
        vec[0] = -vec[0];
        if (i == 1) {
            vec[2] = -vec[2];
        }
    }
    sceVu0ScaleVector(amb, Env_ctl.ambient, 2.0f);
    HH_ClassWrapper_SpotLight_EnvironmentParameter_Get(lpos, ldir, lcolor, para);
    rate = HH_ClassWrapper_SpotLight_ColorRatio_Calculator(lpos, ldir, pt->tyu2d.pos, para[0], para[2]);
    _shScaleVector(lcolor, lcolor, rate);
    _shAddVector(lcolor, lcolor, amb);
    _shScaleVector(lcolor, lcolor, 64.0f);
    _sceVu0ClampVector(lcolor, lcolor, 0.0f, 255.0f);
    r = ftoi(lcolor[0]);
    g = ftoi(lcolor[1]);
    b = ftoi(lcolor[2]);
    spkOpenDGiftag(0xD400000000008000, 0x512512D12D120, ftoi4(w), 8);
    PK_ADD(0x54);
    for (i = 0; i < 4; i++) {
        PK_ADD(pkST(((i & 1) ? 1.0f : 0.0f) * q[i], ((i & 2) ? 1.0f : 0.0f) * q[i]));
        PK_ADD(pkRGBAQ(0x80, r, b, g, q[i]));
        PK_ADD(_shFtoiXYZ(pos[i]));
    }
    spkCloseGiftag();
    EnEfctEnvData.SetTYU2DTex = 1;
}

/** Sets the TYU 2D texture (TEX0) in its environment packet. */
void enEfctSetTYU2DTex(void) {
    *(unsigned long *)&EnEfctEnvData.Tyu2D[1] = HH_Effect_Object_Texture_GS_Register_Tex0_Get(0x13, 0);
}
