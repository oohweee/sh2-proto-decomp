/* ef_cartridge.c: spent cartridges ejected from James's (and Eddie's) guns: they fly, bounce on
 * the floor and are drawn as small flat-shaded quads. */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "sh_vu0.h"
#include "fi_libvu0_inline.h"
#include "sdk/libgraph.h"
#include "sdk/libvu0.h"

#define PK_ADD(v) (*spack.pos++ = (v))

/* asm_helpers.h's lengthXZ reordered to use one register (no $f8). Matching: with lengthXZ,
 * efCartridgeDischargeDisp doesn't match. */
inline float vcLengthXZ(float *v) {
    float r;

    asm {
        lwc1   r, 0(v)
        mula.s r, r
        lwc1   r, 8(v)
        madd.s r, r, r
        sqrt.s r, r
    }
    return r;
}

static void efCartridgeDischargeDisp(struct _EF_CARTDISCH_TASK *ptr);
static void efCartridgeDischargeDel(struct _EF_CARTDISCH_TASK *ptr);
static void efCartridgeDrawCart(struct _EF_CARTDISCH_DATA *cart);
static void efCartridgeDischargeDrawPolyF4(struct _EF_CD_F4 *p);

struct EF_DRAW_DATA efCartDrawData;

/**
 * Ejects a cartridge from James's gun: starts a cartridge task at a point between the
 * weapon's start and end positions, with a random spin and ejection velocity.
 * @param kind weapon kind (1-3; other values do nothing)
 */
void EFCTSetDischargeCartridge(int kind) {
    struct _EF_CARTDISCH_TASK *ptr;
    float rot0[4];
    float m0[4][4];
    float pos0[4];
    float vec[4];
    float pos1[4];
    float svec[4];
    int room;

    if (kind == 1 || kind == 2 || kind == 3) {
        ptr = (struct _EF_CARTDISCH_TASK *)shTSKSetTask((void (*)(void *))EFCTCartridgeDischarge, 4);
        if (ptr != NULL) {
            ptr->exe.atr = 9;
            ptr->exe.mode = 0;
            shGetJamesWeaponStartPos(pos0, vec);
            shGetJamesWeaponEndPos(pos1, vec);
            switch (kind) {
            case 1:
                _shScaleVector(pos0, pos0, 0.6f);
                _shScaleVector(pos1, pos1, 0.4f);
                _shAddVector(pos0, pos0, pos1);
                vcopy(pos0, ptr->data.pos);
                break;
            case 2:
                _shSubVector(pos1, pos0, pos1);
                _shScaleVector(pos1, pos1, -0.33f);
                _shAddVector(pos0, pos0, pos1);
                vcopy(pos0, ptr->data.pos);
                break;
            case 3:
                _shSubVector(pos1, pos0, pos1);
                _shScaleVector(pos1, pos1, -0.1f);
                _shAddVector(pos0, pos0, pos1);
                vcopy(pos0, ptr->data.pos);
                break;
            }
            vwVectorToAngle(rot0, vec);
            vcopy(rot0, ptr->data.rot);
            ptr->data.vrot[0] = -9.0757122f + shRandF();
            ptr->data.vrot[1] = 0.0f;
            ptr->data.vrot[2] = 7.3303828f + shRandF();
            ptr->data.vrot[3] = 1.0f;
            if (kind != 3) {
                svec[0] = 750.0f + (500.0f * (shRandF() - 0.5f)) / 2.0f;
                svec[1] = -1000.0f + (500.0f * (shRandF() - 0.5f)) / 2.0f;
                svec[2] = -400.0f + (500.0f * (shRandF() - 0.5f)) / 2.0f;
            } else {
                svec[0] = (500.0f * (shRandF() - 0.5f)) / 3.0f;
                svec[1] = 0.0f;
                svec[2] = (500.0f * (shRandF() - 0.5f)) / 3.0f;
                ptr->data.pos[1] += (500.0f * (shRandF() - 0.5f)) / 5.0f;
            }
            svec[3] = 1.0f;
            _sceVu0UnitMatrix(m0);
            vwRotMatrixYXZ(rot0, m0);
            _sceVu0ApplyMatrix(ptr->data.mvec, m0, svec);
            ptr->data.life = 2.0f * shGetFPS();
            ptr->data.move = 1;
            ptr->data.kind = kind;
            room = RoomNameJms();
            *(u_long128 *)ptr->data.refsurf = 0;
            switch (room) {
            default:
                ptr->data.reflect = 0;
                break;
            case 1:
                ptr->data.reflect = 1;
                ptr->data.refsurf[0] = -20000.0f;
                break;
            case 0x24:
                ptr->data.reflect = 3;
                ptr->data.refsurf[2] = -99995.0f;
                break;
            }
        }
    }
}

/**
 * Ejects six cartridges for Eddie's gun, scattered around pos.
 * @param pos ejection point
 */
void EFCTSetDischargeCartridgeEddie(float *pos) {
    struct _EF_CARTDISCH_TASK *ptr;
    float rot0[4];
    float m0[4][4];
    float vec[4];
    float svec[4];
    int i;

    for (i = 0; i < 6; i++) {
        ptr = (struct _EF_CARTDISCH_TASK *)shTSKSetTask((void (*)(void *))EFCTCartridgeDischarge, 4);
        if (ptr != NULL) {
            ptr->exe.atr = 9;
            ptr->exe.mode = 0;
            vcopy(pos, ptr->data.pos);
            ptr->data.pos[1] += (500.0f * (shRandF() - 0.5f)) / 5.0f;
            vwVectorToAngle(rot0, vec);
            vcopy(rot0, ptr->data.rot);
            ptr->data.vrot[0] = -9.0757122f + shRandF();
            ptr->data.vrot[1] = 0.0f;
            ptr->data.vrot[2] = 7.3303828f + shRandF();
            ptr->data.vrot[3] = 1.0f;
            svec[0] = (500.0f * (shRandF() - 0.5f)) / 3.0f;
            svec[1] = 0.0f;
            svec[2] = (500.0f * (shRandF() - 0.5f)) / 3.0f;
            svec[3] = 1.0f;
            _sceVu0UnitMatrix(m0);
            vwRotMatrixYXZ(rot0, m0);
            _sceVu0ApplyMatrix(ptr->data.mvec, m0, svec);
            ptr->data.life = 2.0f * shGetFPS();
            ptr->data.move = 1;
            ptr->data.kind = 1;
            ptr->data.reflect = 0;
        }
    }
}

/**
 * Task function of one cartridge: mode 0 moves and draws it, mode 1 deletes it.
 * @param ptr the task (a _EF_CARTDISCH_TASK)
 */
void EFCTCartridgeDischarge(struct _shTskTASK *ptr) {
    switch (ptr->exe.mode) {
    case 0:
        efCartridgeDischargeDisp((struct _EF_CARTDISCH_TASK *)ptr);
        break;
    case 1:
        efCartridgeDischargeDel((struct _EF_CARTDISCH_TASK *)ptr);
        break;
    }
}

static void efCartridgeDischargeDisp(struct _EF_CARTDISCH_TASK *ptr) {
    float mv[4];
    float oldpos[4];
    struct _CL_VHIT_RESULT res;
    float nml[4];

    if (ptr->data.move == 1) {
        vcopy(ptr->data.pos, oldpos);
        _shScaleVector(mv, ptr->data.mvec, shGetDT());
        _shAddVector(ptr->data.pos, ptr->data.pos, mv);
        ptr->data.mvec[1] += 10.0f / shGetDT();
        _shScaleVector(mv, ptr->data.vrot, shGetDT());
        _shAddVector(ptr->data.rot, ptr->data.rot, mv);
        clCheckHitEyesOnlyFloorThru(&res, NULL, oldpos, ptr->data.pos);
        if (res.kind == 1) {
            if (fabsf(ptr->data.mvec[1]) > 750.0f) {
                vcopy(res.hobj.wall.cp, ptr->data.pos);
                ptr->data.mvec[1] *= -0.3f;
                ptr->data.mvec[0] *= 0.3f;
                ptr->data.mvec[2] *= 0.3f;
                switch (res.hobj.wall.pd->material) {
                case 0:
                case 1:
                case 5:
                case 7:
                case 10:
                    SeCallPos(0x2B23, 1.0f, res.hobj.wall.cp, 0);
                    break;
                case 6:
                    break;
                }
            } else {
                *(u_long128 *)ptr->data.mvec = 0;
                *(u_long128 *)ptr->data.vrot = 0;
                ptr->data.rot[0] = 0.0f;
                ptr->data.rot[2] = 0.0f;
                ptr->data.move = 0;
            }
        }
        clCheckHitEyesOnlyWall(&res, oldpos, ptr->data.pos);
        if (res.kind == 1) {
            _shNormalize(nml, res.hobj.wall.nl);
            if (nml[1] <= -0.5f) {
                if (fabsf(ptr->data.mvec[1]) > 750.0f) {
                    vcopy(res.hobj.wall.cp, ptr->data.pos);
                    ptr->data.mvec[1] *= -0.3f;
                    ptr->data.mvec[0] *= 0.3f;
                    ptr->data.mvec[2] *= 0.3f;
                    SeCallPos(0x2B23, 1.0f, res.hobj.wall.cp, 0);
                } else {
                    *(u_long128 *)ptr->data.mvec = 0;
                    *(u_long128 *)ptr->data.vrot = 0;
                    ptr->data.rot[0] = 0.0f;
                    ptr->data.rot[2] = 0.0f;
                    ptr->data.move = 0;
                }
            } else {
                float ang;
                float npos[4];
                float m0[4][4];

                _sceVu0ZeroVector(npos);
                ang = shAtanV(res.hobj.wall.nl);
                npos[2] = vcLengthXZ(ptr->data.mvec);
                _sceVu0UnitMatrix(m0);
                shRotMatrixY(m0, m0, ang);
                _sceVu0ApplyMatrix(npos, m0, npos);
                ptr->data.mvec[0] = npos[0];
                ptr->data.mvec[2] = npos[2];
                vcopy(res.hobj.wall.cp, ptr->data.pos);
            }
        } else if (res.kind == 2) {
            float ang;
            float vec[4];
            float m0[4][4];
            struct _CL_HITPOLY_COLUMN *col;

            col = (struct _CL_HITPOLY_COLUMN *)res.hobj.wall.pd;
            _shSubVector(vec, res.hobj.wall.cp, col->p[0]);
            ang = shAtanV(vec);
            vec[2] = vcLengthXZ(ptr->data.mvec);
            _sceVu0UnitMatrix(m0);
            shRotMatrixY(m0, m0, ang);
            _sceVu0ApplyMatrix(vec, m0, vec);
            ptr->data.mvec[0] = vec[0];
            ptr->data.mvec[2] = vec[2];
            SeCallPos(0x2B23, 1.0f, res.hobj.wall.cp, 0);
        }
    }
    ptr->data.life--;
    if (ptr->data.life <= 0) {
        ptr->exe.mode++;
    }
    efCartridgeDrawCart(&ptr->data);
}

static void efCartridgeDischargeDel(struct _EF_CARTDISCH_TASK *ptr) {
    shTSKDelTask((struct _shTskTASK *)ptr);
}

static void efCartridgeDrawCart(struct _EF_CARTDISCH_DATA *cart) {
    struct _EF_CD_F4 f4;
    float v[4][4];
    int dang;
    float v0[4];
    float v1[4];
    float m0[4][4];
    float m1[4][4];
    float m2[4][4];
    float br;
    float rl;

    HH_ClassWrapper_AmbientColor_Get(efCartDrawData.ambcol);
    efCartDrawData.spoton = HH_ClassWrapper_SpotLight_Enable_Check();
    if (sh2gfw_Get_NightOrDay() && efCartDrawData.spoton) {
        HH_ClassWrapper_SpotLight_EnvironmentParameter_Get(efCartDrawData.lightpos, efCartDrawData.lightdir,
                                                           efCartDrawData.lightcol, efCartDrawData.lightpar);
    }
    sceVu0CopyMatrix(efCartDrawData.wcm, cam0.view_clip);
    shMulMatrix(efCartDrawData.wcm, efCartDrawData.wcm, VbWvsMatrix.wvm);
    spkOpenDGiftag(0x1000000000008000, 0xE, 0x80000000, 0);
    PK_ADD(GS_SET_ALPHA(0, 1, 0, 1, 0x80));
    PK_ADD(GS_REG_ALPHA_1);
    PK_ADD(GS_SET_TEST(1, 1, 0, 0, 0, 0, 1, 3));
    PK_ADD(GS_REG_TEST_1);
    spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
    switch (cart->kind) {
    case 1:
        br = 2.0f;
        rl = 5.0f;
        break;
    case 2:
        br = 4.0f;
        rl = 5.0f;
        break;
    case 3:
        br = 2.5f;
        rl = 7.5f;
        break;
    }
    f4.v[0][3] = 1.0f;
    f4.v[1][3] = 1.0f;
    f4.v[2][3] = 1.0f;
    f4.v[3][3] = 1.0f;
    v[0][3] = 1.0f;
    v[1][3] = 1.0f;
    v[2][3] = 1.0f;
    v[3][3] = 1.0f;
    v0[0] = br;
    v0[1] = 0.0f;
    v0[2] = 0.0f;
    v0[3] = 1.0f;
    f4.rgba[0] = 0xB6;
    f4.rgba[1] = 0x95;
    f4.rgba[2] = 0x10;
    f4.rgba[3] = 0x80;
    _sceVu0UnitMatrix(m0);
    shRotMatrixZ(m0, m0, cart->rot[2]);
    shRotMatrixY(m0, m0, cart->rot[1]);
    shRotMatrixX(m0, m0, cart->rot[0]);
    _sceVu0TransMatrix(m0, m0, cart->pos);
    if (cart->reflect) {
        mcopy(m0, m1);
        _shScaleVector(m1[cart->reflect - 1], m1[cart->reflect - 1], -1.0f);
        m1[3][cart->reflect - 1] =
            cart->refsurf[cart->reflect - 1] + -1.0f * (m0[3][cart->reflect - 1] - cart->refsurf[cart->reflect - 1]);
    }
    v[0][2] = rl;
    v[1][2] = rl;
    v[2][2] = -rl;
    v[3][2] = -rl;
    for (dang = 0; dang < 360; dang += 45) {
        _sceVu0UnitMatrix(m2);
        shRotMatrixZ(m2, m2, vbNormalizeRadianAngle(0.017453292f * dang));
        _sceVu0ApplyMatrix(v1, m2, v0);
        v[0][0] = v1[0];
        v[0][1] = v1[1];
        v[3][0] = v1[0];
        v[3][1] = v1[1];
        _sceVu0UnitMatrix(m2);
        shRotMatrixZ(m2, m2, vbNormalizeRadianAngle(0.017453292f * (dang + 45)));
        _sceVu0ApplyMatrix(v1, m2, v0);
        v[1][0] = v1[0];
        v[1][1] = v1[1];
        v[2][0] = v1[0];
        v[2][1] = v1[1];
        _sceVu0ApplyMatrix(f4.v[0], m0, v[0]);
        _sceVu0ApplyMatrix(f4.v[1], m0, v[1]);
        _sceVu0ApplyMatrix(f4.v[2], m0, v[2]);
        _sceVu0ApplyMatrix(f4.v[3], m0, v[3]);
        efCartridgeDischargeDrawPolyF4(&f4);
        if (cart->reflect) {
            _sceVu0ApplyMatrix(f4.v[0], m1, v[0]);
            _sceVu0ApplyMatrix(f4.v[1], m1, v[1]);
            _sceVu0ApplyMatrix(f4.v[2], m1, v[2]);
            _sceVu0ApplyMatrix(f4.v[3], m1, v[3]);
            efCartridgeDischargeDrawPolyF4(&f4);
        }
        f4.rgba[0] += 10;
        f4.rgba[1] += 8;
        f4.rgba[2] += 1;
    }
    spkCloseGiftag();
}

static void efCartridgeDischargeDrawPolyF4(struct _EF_CD_F4 *p) {
    float q;
    float z;
    int sp[4][4];
    int i;
    unsigned char fog;
    unsigned char rgba[4];
    float w;
    float perc;

    q = 1.0f;
    for (i = 0; i < 4; i++) {
        if (HH_ClassWrapper_RotTrans_PerspectiveProjection_Clip(sp[i], &w, VbWvsMatrix.wsm, efCartDrawData.wcm, p->v[i])) {
            return;
        }
    }
    {
        float para;
        float perc;

        para = (Env_ctl.fogparm.fl32[0] * Env_ctl.fogparm.fl32[2] - Env_ctl.fogparm.fl32[1] * Env_ctl.fogparm.fl32[3]) / (Env_ctl.fogparm.fl32[0] - Env_ctl.fogparm.fl32[1]);
        perc = (Env_ctl.fogparm.fl32[0] * Env_ctl.fogparm.fl32[1] * (Env_ctl.fogparm.fl32[3] - Env_ctl.fogparm.fl32[2])) / (Env_ctl.fogparm.fl32[0] - Env_ctl.fogparm.fl32[1]);
        fog = iclamp((int)(para + w * perc), 0, 0xFF);
    }
    if (sh2gfw_Get_NightOrDay() && efCartDrawData.spoton) {
        perc = HH_ClassWrapper_SpotLight_ColorRatio_Calculator(efCartDrawData.lightpos, efCartDrawData.lightdir, p->v[0],
                                                              efCartDrawData.lightpar[0], efCartDrawData.lightpar[2]);
        for (i = 0; i < 3; i++) {
            rgba[i] = fclamp(fclamp(p->rgba[i] * perc, 0.0f, 255.0f), 128.0f * efCartDrawData.ambcol[i], 255.0f);
        }
    } else {
        for (i = 0; i < 3; i++) {
            z = efCartDrawData.ambcol[i] / 2.0f;
            z = p->rgba[i] * z;
            rgba[i] = fclamp(z, 0.0f, 255.0f);
        }
    }
    PK_ADD(GS_SET_PRIM(4, 0, 0, 1, 0, 0, 0, 0, 0));
    PK_ADD(GS_SET_RGBAQ(rgba[0], rgba[1], rgba[2], 0xFF, *(unsigned int *)&q));
    PK_ADD(GS_SET_XYZF(sp[0][0], sp[0][1], sp[0][2], fog));
    PK_ADD(GS_SET_XYZF(sp[1][0], sp[1][1], sp[1][2], fog));
    PK_ADD(GS_SET_XYZF(sp[3][0], sp[3][1], sp[3][2], fog));
    PK_ADD(GS_SET_XYZF(sp[2][0], sp[2][1], sp[2][2], fog));
}
