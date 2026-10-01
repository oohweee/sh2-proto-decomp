/*
 * Per-character light setup (Chacter_Draw). Before each character is drawn, fills the light_n
 * table from the room's draw environment (parallel, spot, point and fake lights), James's
 * flashlight and gun flash, and demo lights; mirrors the lights for characters drawn reversed.
 * Also keeps the per-character shadow light (ShadowLightWork) and picks James's shadow mode.
 *
 * Matching: the #line directives in this file keep the assert strings on the original's lines.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

#define SHADOW_LIGHT_MAX 32

static int Chr_ID_Buf;
static struct FakeLight_Work FakeLightWork;
static struct ShadowLight_Work ShadowLightWork __attribute__((aligned(16)));
static struct ShadowLight_Work ShadowParallel_LightWork __attribute__((aligned(16)));
static float SpotHilightPos[4];
static int OtherLightFlg;
static int Reverse_Draw;

/* v0.xyz = v1.xyz squared per component; v0.w = v1.w. Inline asm in the original; not shared. */
static inline void _shSqVectorXYZ(float *v0, float *v1) {
    __asm__ __volatile__("
    lqc2       vf4, 0x0(%1)
    vmove.xyzw vf5, vf4
    vmul.xyz   vf4, vf4, vf5
    sqc2       vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}

static void Init_FakeLightWork(void) {
    FakeLightWork.FakeLightNum = 0;
}

static void Set_FakeLight(int nn) {
    if (FakeLightWork.FakeLightNum < 2) {
        FakeLightWork.UniFakeLightNo[FakeLightWork.FakeLightNum++] = nn;
    }
}

static int Get_FakeLightNum(void) {
    return FakeLightWork.FakeLightNum;
}

static int Get_FakeLightId(int n) {
    return FakeLightWork.UniFakeLightNo[n];
}

static void UpdateFake_DirandColor(int no) {
    struct Light *pl;
    struct SubCharacter *scp;
    float tmp[4];
    float color[4];
    float DirVec[4];
    float sc;
    int flg;
    float fvt[4];
    struct PointLightData *pld;
    float dirfac;
    struct SpotLightData *sld;

    pl = LightPointer(no);
    flg = 0;
    scp = UniModelDW_Man.testSubChar;
    sceVu0SubVector(tmp, (float *)&scp->pos, pl->pos);
    sceVu0Normalize(pl->dir, tmp);
    if (pl->fakekind == 2) {
        _shSqVectorXYZ(fvt, tmp);
        if (!(fvt[0] < fvt[1]) || !(fvt[2] < fvt[1])) {
            tmp[1] = 0.0f;
            sceVu0Normalize(tmp, tmp);
        } else {
            tmp[2] = 0.0f;
            sceVu0Normalize(tmp, tmp);
            flg = 1;
        }
    }
    sc = _shLength((float *)&scp->pos, pl->pos);
    sc = fclamp(pl->f_b + pl->f_a * sc, 0.0f, 1.0f);
    switch (pl->fakekind) {
        case 1:
            pld = pl->DrawEnv_LightData;
            vcopy_gcc(color, pld->Col);
            break;
        case 2:
            sld = pl->DrawEnv_LightData;
            sceVu0CopyVector(DirVec, sld->Dir);
            if (flg) {
                DirVec[2] = 0.0f;
            } else {
                DirVec[1] = 0.0f;
            }
            sceVu0Normalize(DirVec, DirVec);
            dirfac = sceVu0InnerProduct(tmp, DirVec);
            dirfac = fclamp(pl->s_b + pl->s_a * dirfac, 0.0f, 1.0f);
            sc *= dirfac;
            vcopy_gcc(color, sld->Col);
            break;
        default:
            sc = 0.0f;
            break;
    }
    if (sc <= 1e-05f) {
        pl->kind = 0;
    } else {
        pl->kind = 1;
        _shScaleVector(pl->color, color, sc);
    }
}

/**
 * Returns whether a character is lit: 1 if its stored shadow light's influence is above 0.1,
 * 0 otherwise or if it has none.
 */
int sh2gfw_Check_CharaDarkOrBright(void *SubChara) {
    static float inf_hosei = 0.1f;
    int i;

    for (i = 0; i < ShadowLightWork.Store_Num; i++) {
        if (ShadowLightWork.ShadowLd[i].SubChara == SubChara) {
            return ShadowLightWork.ShadowLd[i].influence_factor - inf_hosei > 0.0f;
        }
    }
    return 0;
}

static void sh2gfw_Init_ShadowLight(void) {
    ShadowLightWork.Store_Num = 0;
    ShadowParallel_LightWork.Store_Num = 0;
}

/**
 * Stores the scene's parallel lights for the shadow code, on the first call after
 * sh2gfw_set_CommonCharacterLight only.
 * @param pald array of struct Light pointers
 * @param pnum number of lights
 */
void sh2gfw_Store_ShadowParallelLight(void *pald, int pnum) {
    struct Light *pL;
    int i;

    pL = pald;
    i = ShadowParallel_LightWork.Store_Num++;
    if (i > 0) {
        return;
    }
    for (; i < pnum; i++) {
        ShadowParallel_LightWork.ShadowLd[i].lightKind = 1;
        ShadowParallel_LightWork.ShadowLd[i].pDed = pL->DrawEnv_LightData;
        pL++;
    }
}

/**
 * Stores the shadow light of the character being drawn: the given light, or the room's forced
 * shadow light if it has one. Skipped for character kind 0x202 and when the table is full.
 * @param pl the struct Light that lights the character most
 */
#line 479
void sh2gfw_Store_ShadowLight(void *pl) {
    int ltype;
    void *pldata;
    int sn;
    struct Light *pL;

    pL = pl;
    pldata = Get_NowRoomShadowForceLight(&ltype);
    if (ShadowLightWork.Store_Num > SHADOW_LIGHT_MAX - 1) {
        return;
    }
    if (UniModelDW_Man.testSubChar->kind == 0x202) {
        return;
    }
    sn = ShadowLightWork.Store_Num;
    if (pldata == NULL || ltype == 4) {
        ShadowLightWork.ShadowLd[sn].SubChara = UniModelDW_Man.testSubChar;
        ShadowLightWork.ShadowLd[sn].pDed = pL->DrawEnv_LightData;
        ShadowLightWork.ShadowLd[sn].lightKind = pL->kind;
        ShadowLightWork.ShadowLd[sn].influence_factor = pL->influence;
        ShadowLightWork.ShadowLd[sn].force_flg = 0;
    } else {
        ShadowLightWork.ShadowLd[sn].lightKind = ltype;
        ShadowLightWork.ShadowLd[sn].SubChara = UniModelDW_Man.testSubChar;
        ShadowLightWork.ShadowLd[sn].pDed = pldata;
        ShadowLightWork.ShadowLd[sn].influence_factor = 1.0f;
        ShadowLightWork.ShadowLd[sn].force_flg = 1;
    }
    ShadowLightWork.Store_Num++;
    assert_dw(ShadowLightWork.Store_Num<=SHADOW_LIGHT_MAX);
}

/**
 * Returns the shadow light of a character and its parameters.
 * @param mode 0: the last stored light (the character just drawn); 1: search for pScp
 * @param pos  receives the light position (point/spot lights)
 * @param dir  receives the light direction
 * @param svt  receives the cone and falloff parameters (point/spot lights)
 * @param pScp the character
 * @return the shadow light kind for sh2shd_Draw_ShadowChar (0 flashlight, 4 parallel,
 *         5 point/spot), or -1 for no shadow
 */
int sh2gfw_Get_ShadowLight(int mode, float *pos, float *dir, float *svt, void *pScp) {
    int sn;
    int lt;
    struct PointLightData *pld;
    struct SpotLightData *sld;
    struct ParallelLightData *pald;
    void *tmp;
    struct SubCharacter *scp;
    float sv[4];
    float dummy[4];

    if (mode) {
        for (sn = 0; sn < SHADOW_LIGHT_MAX; sn++) {
            if (ShadowLightWork.ShadowLd[sn].SubChara == pScp) {
                break;
            }
        }
        if (sn == SHADOW_LIGHT_MAX) {
            return 0;
        }
    } else {
        sn = ShadowLightWork.Store_Num - 1;
    }
    if (!sh2gfw_Get_NightOrDay()) {
        scp = UniModelDW_Man.testSubChar;
        if (scp->kind >= 0x100 && scp->kind <= 0x211) {
            return 0;
        }
        return -1;
    }
    if (ShadowLightWork.ShadowLd[sn].SubChara != pScp) {
        return -1;
    }
    tmp = Get_NowRoomShadowForceLight(&lt);
    if (lt == 1) {
        sceVu0CopyVector(dir, tmp);
        return 4;
    }
    if (sn < 0) {
        return -1;
    }
    tmp = ShadowLightWork.ShadowLd[sn].pDed;
    if (tmp == NULL) {
        sh2gde_getspotKTParams(pos, dir, sv, svt, 0);
        shGetJamesLightPos(dummy, dir);
        return 0;
    }
    switch (ShadowLightWork.ShadowLd[sn].lightKind) {
        case 1:
            pald = tmp;
            sceVu0CopyVector(dir, pald->Dir);
            return 4;
        case 2:
            pld = tmp;
            scp = ShadowLightWork.ShadowLd[sn].SubChara;
            sceVu0CopyVector(pos, pld->Pos);
            sceVu0SubVector(dir, (float *)&scp->pos, pld->Pos);
            sceVu0Normalize(dir, dir);
            svt[0] = 0.866f;
            svt[1] = pld->DecayParm[0];
            svt[2] = 1.2f * pld->DecayParm[1];
            return 5;
        case 3:
            sld = tmp;
            sceVu0CopyVector(pos, sld->Pos);
            sceVu0CopyVector(dir, sld->Dir);
            svt[0] = sld->DecayParm[0];
            svt[1] = sld->DecayParm[2];
            svt[2] = 1.2f * sld->DecayParm[3];
            return 5;
    }
    return -1;
}

/** Returns the light kind of the last stored shadow light, or -1 if there is none. */
int sh2gfw_Get_ShadowLightKind(void) {
    int sn;

    sn = ShadowLightWork.Store_Num - 1;
    if (sn < 0) {
        return -1;
    }
    return ShadowLightWork.ShadowLd[sn].lightKind;
}

/** Returns the last stored shadow light (struct ShadowLight_Data), or NULL. */
void *sh2gfw_Get_ShadowLightPointer(void) {
    int sn;

    sn = ShadowLightWork.Store_Num - 1;
    if (sn < 0) {
        return NULL;
    }
    return &ShadowLightWork.ShadowLd[sn];
}

/**
 * Draws the shadow of James (or the boat), choosing the shadow mode from the light kind, the
 * flashlight state and whether the camera faces him.
 * @param mode 0: lights of the character just drawn; 1: search the stored lights for scp
 * @param pm   the character's struct sh2gfw_ModelDraw_MAN
 * @param scp  the character
 */
void Draw_Jms_Shadow(int mode, void *pm, struct SubCharacter *scp) {
    int ShadowLightInfo;
    int LightInfo;
    int sn;
    struct sh2gfw_ModelDraw_MAN *pMD;
    float spot_pos[4];
    float spot_dir[4];
    float spot_pam[4];
    struct ShadowLight_Data *pShadow;
    struct Light *pL;
    float inpro;

    pMD = pm;
    if (pMD->pKg1Work) {
        pShadow = sh2gfw_Get_ShadowLightPointer();
        sh2gfw_Get_ShadowLight(mode, spot_pos, spot_dir, spot_pam, scp);
        LightInfo = sh2gfw_Get_ShadowLightKind();
        if (!sh2gfw_Get_NightOrDay()) {
            sh2shd_Draw_ShadowChar(scp, pMD->pKg1Work, scp->kind, scp->id, 2, spot_pos, spot_dir, spot_pam);
            return;
        }
        inpro = sh2gde_Get_InnerVectorJmsCam();
        sn = sh2gfw_Check_JmsSpotOnOff();
        switch (LightInfo) {
            case 1:
                ShadowLightInfo = 4;
                break;
            case 2:
                if (sn && !pShadow->force_flg) {
                    ShadowLightInfo = 3;
                } else {
                    ShadowLightInfo = 5;
                }
                break;
            case 3:
                if (sh2gde_CheckSpot_JmsOrBG()) {
                    if (sn && !pShadow->force_flg) {
                        if (inpro < 0.0f) {
                            ShadowLightInfo = 1;
                        } else {
                            ShadowLightInfo = 3;
                        }
                    } else {
                        ShadowLightInfo = 5;
                    }
                } else {
                    ShadowLightInfo = 5;
                    sceVu0CopyVector(spot_pos, (float *)sh2gde_Get_BGSpotPos());
                    sceVu0CopyVector(spot_dir, (float *)sh2gde_Get_BGSpotDir());
                }
                break;
            default:
                if (sn) {
                    ShadowLightInfo = 3;
                } else {
                    ShadowLightInfo = 9;
                }
                break;
        }
        sh2shd_Draw_ShadowChar(scp, pMD->pKg1Work, scp->kind, scp->id, ShadowLightInfo, spot_pos, spot_dir, spot_pam);
    }
}

static void Reverse_Light(void) {
    int i;
    struct Light *pL;

    if (RoomNameJms() == 0x24) {
        for (i = 0; i < 12; i++) {
            pL = LightPointer(i);
            if (pL->kind) {
                pL->dir[2] *= -1.0f;
                pL->pos[2] = -199990.0f - pL->pos[2];
            }
        }
    } else {
        for (i = 0; i < 12; i++) {
            pL = LightPointer(i);
            if (pL->kind) {
                pL->dir[0] *= -1.0f;
                pL->pos[0] = -40000.0f - pL->pos[0];
            }
        }
    }
}

/* Matching: these three are defined with `()`: sh2gfw_change_lights passes them an argument. */
static void sh2gfw_set_JmsLIGHT() {
    float coltmp[4];
    float spotpos[4];
    float spotdir[4];
    float spotcol[4];
    float decayparms[4];
    struct SubCharacter *scp;
    unsigned int l_mode;

    l_mode = Env_ctl.light_mode;
    sh2gde_getspotKTParams(spotpos, spotdir, spotcol, decayparms, 1);
    scp = UniModelDW_Man.testSubChar;
    if (!(scp->kind & 0x20)) {
        if (Reverse_Draw) {
            Reverse_Light();
        }
        switch (l_mode) {
            case 1:
                break;
            case 2:
                LightDelete(0);
                if (sh2gde_CheckSpot_JmsOrBG() && ((item.flag[0] >> 15) & 1)) {
                    sceVu0ScaleVector(spotdir, spotdir, -1.0f);
                    sceVu0ScaleVector(coltmp, spotcol, 0.075f);
                    {
                        float svt[4] = {7.0f, 7.0f, 7.0f, 1.0f};

                        sceVu0ScaleVector(svt, svt, sh2gfw_Get_JmsGunLight() - 0.075f);
                        sceVu0AddVector(coltmp, svt, coltmp);
                    }
                    coltmp[3] = 16.0f;
                    spotdir[3] = 1.0f;
                    LightSetParallel(1, spotdir, coltmp);
                    break;
                }
                if (sh2gfw_Get_JmsGunLight() - 0.075f <= 0.001f) {
                    LightDelete(1);
                } else {
                    sceVu0ScaleVector(spotdir, spotdir, -1.0f);
                    sceVu0ScaleVector(coltmp, spotcol, 0.075f);
                    {
                        float svt[4] = {7.0f, 7.0f, 7.0f, 1.0f};

                        sceVu0ScaleVector(svt, svt, sh2gfw_Get_JmsGunLight() - 0.075f);
                        sceVu0AddVector(coltmp, svt, coltmp);
                    }
                    coltmp[3] = 16.0f;
                    spotdir[3] = 1.0f;
                    LightSetParallel(1, spotdir, coltmp);
                }
                if (!sh2gde_CheckSpot_JmsOrBG()) {
                    sh_Kari_LightSetSpot(0, (float *)sh2gde_Get_BGSpotPos(), (float *)sh2gde_Get_BGSpotDir(), spotcol, decayparms);
                } else if (sh2gfw_Check_DemoSpotLight()) {
                    sh2gde_getspotKTParams(spotpos, spotdir, spotcol, decayparms, 0);
                    sh_Kari_LightSetSpot(0, spotpos, spotdir, spotcol, decayparms);
                    sh_Set_DrawEnvLightData(0, NULL);
                }
                break;
        }
        Reverse_Draw = 0;
    } else if (!Reverse_Draw) {
        Reverse_Light();
        Reverse_Draw = 1;
    }
    LightUpdateInfoByScene();
}

static void sh2gfw_set_OtherOtherLIGHT() {
    unsigned int l_mode;
    struct SubCharacter *scp;

    l_mode = Env_ctl.light_mode;
    scp = UniModelDW_Man.testSubChar;
    switch (l_mode) {
        case 1:
            if (sh2gfw_Check_DemoSpotLight() && !OtherLightFlg) {
                float spotpos2[4];
                float spotdir2[4];
                float spotcol2[4];
                float decayparms2[4];

                sh2gde_getspotKTParams(spotpos2, spotdir2, spotcol2, decayparms2, 0);
                sh_Kari_LightSetSpot(0, spotpos2, spotdir2, spotcol2, decayparms2);
                sh_Set_DrawEnvLightData(0, NULL);
                LightUpdateInfoByScene();
            }
            break;
        case 2:
            sh2gfw_Check_DemoRefrectionHightLight();
            if (OtherLightFlg && scp->kind != 0x516) {
                break;
            }
            if (scp->kind != 0x516) {
                LightDelete(1);
            } else if ((item.flag[0] >> 15) & 1) {
                LightDelete(1);
            } else {
                float spotdir[4] = {0.0f, -0.1788f, -0.9839f, 0.0f};
                float coltmp[4] = {1.0f, 1.0f, 1.0f, 0.0f};

                LightSetParallel(1, spotdir, coltmp);
            }
            {
            float spotpos[4];
            float spotdir[4];
            float spotcol[4];
            float decayparms[4];
            void *lp;
            int flg;

            flg = 0;
            sh2gde_getspotKTParams(spotpos, spotdir, spotcol, decayparms, 0);
            if (sh2gde_CheckSpot_JmsOrBG()) {
                if (sh2gfw_Get_JmsGunLight() - 0.075f > 0.001f) {
                    float svt[4] = {7.0f, 7.0f, 7.0f, 1.0f};

                    sceVu0ScaleVector(svt, svt, sh2gfw_Get_JmsGunLight() - 0.075f);
                    sceVu0AddVector(spotcol, svt, spotcol);
                    spotcol[3] = 1.0f;
                    flg = 1;
                }
                lp = NULL;
            } else {
                sceVu0CopyVector(spotpos, (float *)sh2gde_Get_BGSpotPos());
                sceVu0CopyVector(spotdir, (float *)sh2gde_Get_BGSpotDir());
                lp = sh2gde_Get_BGSpotPos();
            }
            if (flg || (sh2gfw_Check_JmsSpotOnOff() && Check_NowSpotFakeOrJms() >= 0)) {
                sh_Kari_LightSetSpot(0, spotpos, spotdir, spotcol, decayparms);
                sh_Set_DrawEnvLightData(0, lp);
            } else if (sh2gfw_Check_DemoSpotLight()) {
                sh_Kari_LightSetSpot(0, spotpos, spotdir, spotcol, decayparms);
                sh_Set_DrawEnvLightData(0, lp);
            } else {
                LightDelete(0);
            }
            sh2gfw_Check_DemoRefrectionHightLight();
            LightUpdateInfoByScene();
            }
            break;
    }
    OtherLightFlg++;
}

static void sh2gfw_set_OtherLIGHT() {
    int flg;
    struct SubCharacter *scp;
    unsigned int l_mode;
    int ckind;

    l_mode = Env_ctl.light_mode;
    flg = 0;
    scp = UniModelDW_Man.testSubChar;
    ckind = scp->kind;
    if (!(ckind & 0x20) || (ckind > 0x12E && ckind <= 0x7FF && ckind != 0x443 && ckind != 0x444)) {
        if (Reverse_Draw) {
            Reverse_Light();
        }
        switch (l_mode) {
            case 1:
                if (sh2gfw_Check_DemoSpotLight()) {
                    float spotpos2[4];
                    float spotdir2[4];
                    float spotcol2[4];
                    float decayparms2[4];

                    sh2gde_getspotKTParams(spotpos2, spotdir2, spotcol2, decayparms2, 0);
                    sh_Kari_LightSetSpot(0, spotpos2, spotdir2, spotcol2, decayparms2);
                    sh_Set_DrawEnvLightData(0, NULL);
                    LightUpdateInfoByScene();
                }
                break;
            case 2:
                if (scp->kind != 0x516) {
                    LightDelete(1);
                } else if ((item.flag[0] >> 15) & 1) {
                    LightDelete(1);
                } else {
                    float spotdir[4] = {0.0f, -0.1788f, -0.9839f, 0.0f};
                    float coltmp[4] = {1.0f, 1.0f, 1.0f, 0.0f};

                    LightSetParallel(1, spotdir, coltmp);
                }
                {
                float spotpos[4];
                float spotdir[4];
                float spotcol[4];
                float decayparms[4];
                void *lp;

                sh2gde_getspotKTParams(spotpos, spotdir, spotcol, decayparms, 0);
                if (sh2gde_CheckSpot_JmsOrBG()) {
                    if (sh2gfw_Get_JmsGunLight() - 0.075f > 0.001f) {
                        float svt[4] = {7.0f, 7.0f, 7.0f, 1.0f};

                        sceVu0ScaleVector(svt, svt, sh2gfw_Get_JmsGunLight() - 0.075f);
                        sceVu0AddVector(spotcol, svt, spotcol);
                        spotcol[3] = 1.0f;
                        flg = 1;
                    }
                    lp = NULL;
                } else {
                    sceVu0CopyVector(spotpos, (float *)sh2gde_Get_BGSpotPos());
                    sceVu0CopyVector(spotdir, (float *)sh2gde_Get_BGSpotDir());
                    lp = sh2gde_Get_BGSpotPos();
                }
                if (flg || (sh2gfw_Check_JmsSpotOnOff() && Check_NowSpotFakeOrJms() >= 0)) {
                    sh_Kari_LightSetSpot(0, spotpos, spotdir, spotcol, decayparms);
                    sh_Set_DrawEnvLightData(0, lp);
                } else if (sh2gfw_Check_DemoSpotLight()) {
                    sh_Kari_LightSetSpot(0, spotpos, spotdir, spotcol, decayparms);
                    sh_Set_DrawEnvLightData(0, lp);
                } else {
                    LightDelete(0);
                }
                sh2gfw_Check_DemoRefrectionHightLight();
                }
                break;
        }
        Reverse_Draw = 0;
    } else if (!Reverse_Draw) {
        Reverse_Light();
        Reverse_Draw = 1;
    }
    LightUpdateInfoByScene();
    OtherLightFlg = 1;
}

/** Forgets the previously lit character (see sh2gfw_check_LightChr). */
void sh2gfw_clear_LightChr(void) {
    Chr_ID_Buf = 0;
}

#define IS_JAMES_KIND(id) (((id) >= 0x100 && (id) <= 0x103) || ((id) >= 0x120 && (id) <= 0x123) || ((id) >= 0x800 && (id) <= 0x82A))

/**
 * Classifies the switch from the previously lit character to cid, and remembers cid.
 * @return 0 James after James, 1 other after James, 2 James after other, 3 other after other
 */
int sh2gfw_check_LightChr(int cid) {
    int id;

    if (IS_JAMES_KIND(Chr_ID_Buf)) {
        if (IS_JAMES_KIND(cid)) {
            id = 0;
        } else {
            id = 1;
        }
    } else {
        if (IS_JAMES_KIND(cid)) {
            id = 2;
        } else {
            id = 3;
        }
    }
    Chr_ID_Buf = cid;
    return id;
}

/**
 * Sets the lights for the character about to be drawn: updates the fake lights for its
 * position, then James's or the other characters' light setup.
 * @param pmh the character's struct sh2gfw_Model_Header
 * @return the sh2gfw_check_LightChr class
 */
int sh2gfw_change_lights(void *pmh) {
    int ih;
    struct sh2gfw_Model_Header *pMH;
    int iu;

    pMH = pmh;
    ih = sh2gfw_check_LightChr(pMH->chara_id);
    if (Get_NowFakePointNum() || Get_NowFakeSpotNum()) {
        for (iu = 0; iu < Get_FakeLightNum(); iu++) {
            UpdateFake_DirandColor(Get_FakeLightId(iu));
        }
        UpdateParallels();
    }
    switch (ih) {
        case 0:
            sh2gfw_set_JmsLIGHT(ih);
            break;
        case 1:
            sh2gfw_set_OtherLIGHT(ih);
            break;
        case 2:
            sh2gfw_set_JmsLIGHT(ih);
            break;
        case 3:
            sh2gfw_set_OtherOtherLIGHT(ih);
            break;
    }
    return ih;
}

/*
 * Matching: colour copy through sceVu0ScaleVector(.., 1.0f): an inline function with a void * source (no DWARF,
 * name not recovered). The implicit conversion is what makes the original load f12, a1, a0; the calls
 * written with (float *)& casts go direct.
 */
static inline void CopyColor(float *dst, void *src) {
    sceVu0ScaleVector(dst, src, 1.0f);
}

/**
 * Per-frame light setup shared by all characters: clears the lights, sets the scene's parallel
 * and reflection lights (night or day), then the room's spot, point, fake and demo lights.
 */
#line 1646
void sh2gfw_set_CommonCharacterLight(void) {
    int lno;
    int i;
    float coltmp[4];
    float dir[4];

    LightDeleteAll();
    sh2gfw_clear_LightChr();
    sh2gfw_Propagate_JmsGunLight();
    sh2gfw_Init_ShadowLight();
    Init_FakeLightWork();
    OtherLightFlg = 0;
    Reverse_Draw = 0;
    kari_sh2gde_getspotParams(SpotHilightPos, dir, coltmp);
    SpotHilightPos[0] = 0.0f;
    if (sh2gfw_Get_NightOrDay()) {
        int ik;
        float ddir[4];

        ik = Get_NowParallelNum();
        switch (ik) {
            case 0:
                CopyColor(coltmp, Env_ctl.p_color1);
                LightSetParallel(2, (float *)&Env_ctl.p_light1, coltmp);
                CopyColor(coltmp, Env_ctl.p_color2);
                LightSetParallel(3, (float *)&Env_ctl.p_light2, coltmp);
                break;
            case 1:
                CopyColor(coltmp, Env_ctl.p_color1);
                LightSetParallel(2, (float *)&Env_ctl.p_light1, coltmp);
                Get_NowPallarelCol(coltmp, 3 - ik);
                LightSetParallel(3, (float *)&Env_ctl.p_light2, coltmp);
                break;
            case 2:
                Get_NowPallarelCol(coltmp, 3 - ik++);
                LightSetParallel(2, (float *)&Env_ctl.p_light1, coltmp);
                Get_NowPallarelCol(coltmp, 3 - ik);
                LightSetParallel(3, (float *)&Env_ctl.p_light2, coltmp);
                break;
        }
        if (!sh2gfw_Check_DemoRefrectionHightLight()) {
            coltmp[0] = coltmp[1] = coltmp[2] = coltmp[3] = 1.0f;
            LightSetReflection(4, (float *)&Env_ctl.p_light2, coltmp);
        } else {
            sh2gfw_Get_DemoRefrectionHightLight(ddir, coltmp);
            LightSetReflection(4, ddir, coltmp);
        }
    } else {
        int ik;
        float ddir[4];

        ik = Get_NowParallelNum();
        switch (ik) {
            case 0:
                CopyColor(coltmp, Env_ctl.p_color1);
                LightSetParallel(2, (float *)&Env_ctl.p_light1, coltmp);
                CopyColor(coltmp, Env_ctl.p_color2);
                LightSetParallel(3, (float *)&Env_ctl.p_light2, coltmp);
                break;
            case 1:
                sceVu0ScaleVector(coltmp, (float *)&Env_ctl.p_color1, 1.0f);
                LightSetParallel(2, (float *)&Env_ctl.p_light1, coltmp);
                Get_NowPallarelCol(coltmp, 3 - ik);
                LightSetParallel(3, (float *)&Env_ctl.p_light2, coltmp);
                break;
            case 2:
                Get_NowPallarelCol(coltmp, 3 - ik++);
                LightSetParallel(2, (float *)&Env_ctl.p_light1, coltmp);
                Get_NowPallarelCol(coltmp, 3 - ik);
                LightSetParallel(3, (float *)&Env_ctl.p_light2, coltmp);
                break;
        }
        if (BgIsOut(0)) {
            sceVu0ScaleVector(coltmp, (float *)&Env_ctl.p_color0, 1.0f);
            LightSetParallel(1, (float *)&Env_ctl.p_light0, coltmp);
        }
        if (!sh2gfw_Check_DemoRefrectionHightLight()) {
            coltmp[0] = coltmp[1] = coltmp[2] = 0.5f;
            LightSetReflection(4, (float *)&Env_ctl.p_light2, coltmp);
        } else {
            sh2gfw_Get_DemoRefrectionHightLight(ddir, coltmp);
            LightSetReflection(4, ddir, coltmp);
        }
    }
    {
    struct Light *pl;
    struct DrawEnvData *ded;
    struct PointLightData *pld;
    struct SpotLightData *sld;
    float coltmp[4];
    float ppos[4];
    float pcol[4];
    float pdecay[4];

    for (i = 2; i <= 3; i++) {
        pl = LightPointer(i);
        sh2gfw_Get_ParallelDemoLight(i - 1, pl->dir, pl->color);
    }
    lno = 5;
    ded = Get_NowDrawEnvData();
    sld = (struct SpotLightData *)ded->Ld_0;
    for (i = 0; i <= ded->spotLNum; i++) {
        sceVu0ScaleVector(coltmp, sld->Col, 1.0f);
        coltmp[3] = 1.0f;
        sh_Kari_LightSetSpot(lno, sld->Pos, sld->Dir, coltmp, sld->DecayParm);
        sh_Set_DrawEnvLightData(lno++, sld++);
    }
    pld = (struct PointLightData *)sld;
    for (i = 0; i < ded->pointLNum; i++) {
        sceVu0ScaleVector(coltmp, pld->Col, 1.0f);
        LightSetPoint(lno, pld->Pos, coltmp, pld->DecayParm[0], pld->DecayParm[1]);
        sh_Set_DrawEnvLightData(lno++, pld++);
    }
    lno = 2;
    for (i = 0; i < ded->FakePointNum; i++) {
        sceVu0ScaleVector(coltmp, pld->Col, 1.0f);
        LightSetFakePoint(lno, pld->Pos, coltmp, pld->DecayParm[0], pld->DecayParm[1]);
        Set_FakeLight(lno);
        sh_Set_DrawEnvLightData(lno++, pld++);
    }
    sld = (struct SpotLightData *)pld;
    for (i = 0; i < ded->FakeSpotNum; i++) {
        sceVu0ScaleVector(coltmp, pld->Col, 1.0f);
        LightSetFakeSpot(lno, sld->Pos, sld->Dir, sld->Col, sld->DecayParm[2], sld->DecayParm[3], 1.0f, sld->DecayParm[0]);
        Set_FakeLight(lno);
        sh_Set_DrawEnvLightData(lno++, sld++);
    }
    if (ded->FakeSpotNum == -1) {
        kari_sh2gde_getspotParams(sld->Pos, sld->Dir, sld->DecayParm);
        LightSetFakeSpot(lno, sld->Pos, sld->Dir, sld->Col, sld->DecayParm[2], sld->DecayParm[3], 1.0f, sld->DecayParm[0]);
        Set_FakeLight(lno);
        sh_Set_DrawEnvLightData(lno++, sld);
    }
    if (sh2gfw_Check_DemoPointLight()) {
        for (i = 0; i < 3; i++) {
            if (sh2gfw_Get_DemoPointLight(i, ppos, pcol, pdecay)) {
                LightSetPoint(lno++, ppos, pcol, pdecay[0], pdecay[1]);
            }
        }
    }
    assert(lno-5<=6);
    }
    LightUpdateInfoByScene();
}
