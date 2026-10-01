/*
 * Background lighting and demo (cutscene) light overrides (GFW): the per-block VU1 light data
 * (three parallel lights, or the spot light plus two parallel lights), and the demo parallel,
 * point, spot and reflection lights that replace the room's lights during cutscenes.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sdk/libvu0.h"

static float DARK_Hosei = 0.5f;
static struct DemoLight_Work DemoLightWork;

/** Switches every demo light override off. */
void sh2gfw_Init_DemoLight_Work(void) {
    DemoLightWork.Parallel[0] = DemoLightWork.Parallel[1] = DemoLightWork.Parallel[2] = 0;
    DemoLightWork.Parallel_Cid[0] = DemoLightWork.Parallel_Cid[1] = DemoLightWork.Parallel_Cid[2] = 0;
    DemoLightWork.Spot = DemoLightWork.Amb = DemoLightWork.Ref = 0;
    DemoLightWork.Point[0] = DemoLightWork.Point[1] = DemoLightWork.Point[2] = 0;
    DemoLightWork.Point_Cid[0] = DemoLightWork.Point_Cid[1] = DemoLightWork.Point_Cid[2] = 0;
}

/**
 * Sets the demo reflection (highlight) light. Declared int in the DWARF; returns nothing.
 * @param dir direction
 * @param col color
 */
int sh2gfw_Set_DemoRefrectionHighLight(float *dir, float *col) {
    DemoLightWork.Ref = 1;
    vcopy_gcc(DemoLightWork.demo_RefLight_dir, dir);
    vcopy_gcc(DemoLightWork.demo_RefLight_col, col);
}

/** Copies the demo reflection light's direction and color if it is set; returns whether it is. */
int sh2gfw_Get_DemoRefrectionHightLight(float *dir, float *col) {
    if (DemoLightWork.Ref) {
        vcopy_gcc(dir, DemoLightWork.demo_RefLight_dir);
        vcopy_gcc(col, DemoLightWork.demo_RefLight_col);
    } else {
        return 0;
    }
}

/** Returns whether the demo reflection light is set. */
int sh2gfw_Check_DemoRefrectionHightLight(void) {
    return DemoLightWork.Ref;
}

/**
 * Sets demo parallel light |n| (0-2). Light 0 is only taken by day; for 1 and 2 a negative n
 * marks the override as suspended (-1) instead of active.
 * @return n + 1, or 0 if |n| > 2
 */
int sh2gfw_Set_PallarelLight(float *dir, float *color, int n) {
    int k;

    k = (n < 0) ? -n : n;
    if (k <= 2) {
        switch (k) {
        case 0:
            if (sh2gfw_Get_NightOrDay() == 0) {
                sceVu0CopyVector(DemoLightWork.demo_p_light0, dir);
                sceVu0ScaleVector(DemoLightWork.demo_p_color0, color, 1.0f);
                return n + 1;
            }
            break;
        case 1:
            sceVu0CopyVector(DemoLightWork.demo_p_light1, dir);
            sceVu0ScaleVector(DemoLightWork.demo_p_color1, color, 1.0f);
            if (n > 0) {
                DemoLightWork.Parallel[1] = 1;
            } else {
                DemoLightWork.Parallel[1] = -1;
            }
            break;
        case 2:
            sceVu0CopyVector(DemoLightWork.demo_p_light2, dir);
            sceVu0ScaleVector(DemoLightWork.demo_p_color2, color, 1.0f);
            if (n > 0) {
                DemoLightWork.Parallel[2] = 1;
            } else {
                DemoLightWork.Parallel[2] = -1;
            }
            break;
        }
        return n + 1;
    }
    return 0;
}

/** Returns the state of demo parallel light n (0 off, 1 active, -1 suspended). */
int sh2gfw_Check_ParallelDemoLight(int n) {
    return DemoLightWork.Parallel[n];
}

/** Copies demo parallel light n's direction and color if it is set; returns whether it is. */
int sh2gfw_Get_ParallelDemoLight(int n, float *dir, float *col) {
    float (*svt)[4];

    if (DemoLightWork.Parallel[n]) {
        svt = (float (*)[4])DemoLightWork.demo_p_light0 + n * 2;
        vcopy_gcc(dir, svt[0]);
        vcopy_gcc(col, svt[1]);
        return 1;
    }
    return 0;
}

/** Marks demo parallel light n active again. */
void sh2gfw_Revive_ParallelDemoLight(int n) {
    DemoLightWork.Parallel[n] = 1;
}

/** Sets demo point light n (0-2) with its position, color and falloff distances. */
void sh2gfw_Set_DemoPointLight(int n, float *pos, float *col, float falloff_start, float falloff_end) {
    struct AAA *pl;

    pl = (struct AAA *)DemoLightWork.demo_pointcol0 + n;
    DemoLightWork.Point[n] = 1;
    vcopy_gcc(pl->pos, pos);
    vcopy_gcc(pl->col, col);
    pl->decay[0] = falloff_start;
    pl->decay[1] = falloff_end;
}

/** Returns the number of demo point lights set. */
int sh2gfw_Check_DemoPointLight(void) {
    return DemoLightWork.Point[0] + DemoLightWork.Point[1] + DemoLightWork.Point[2];
}

/** Copies demo point light n's position, color and falloff if it is set; returns whether it is. */
int sh2gfw_Get_DemoPointLight(int n, float *pos, float *col, float *decay) {
    struct AAA *pl;

    if (DemoLightWork.Point[n]) {
        pl = (struct AAA *)DemoLightWork.demo_pointcol0;
        pl += n;
        vcopy_gcc(pos, pl->pos);
        vcopy_gcc(col, pl->col);
        vcopy_gcc(decay, pl->decay);
        return 1;
    }
    return 0;
}

/** Returns whether the demo spot light is set. */
int sh2gfw_Check_DemoSpotLight(void) {
    return DemoLightWork.Spot;
}

/** Copies the demo spot light's decay parameters (falloff start, cos(spread), falloff end, spread). */
void sh2gfw_Get_DemoSpotParm(float *svt) {
    vcopy_gcc(svt, DemoLightWork.demo_spotdecay);
}

/** Copies the demo spot light's position, direction and color. */
void sh2gfw_Get_DemoSpotPosDirCol(float *pos, float *dir, float *col) {
    vcopy_gcc(pos, DemoLightWork.demo_spotpos);
    vcopy_gcc(dir, DemoLightWork.demo_spotdir);
    vcopy_gcc(col, DemoLightWork.demo_spotcol);
}

/**
 * Sets the demo spot light.
 * @param spread_pi cone half-angle in radians
 */
void sh2gfw_Set_SpotLight(float *dir, float *pos, float *color, int arg3, float falloff_start, float falloff_end, float spread_pi) {
    DemoLightWork.Spot = 1;
    vcopy_gcc(DemoLightWork.demo_spotpos, pos);
    vcopy_gcc(DemoLightWork.demo_spotdir, dir);
    vcopy_gcc(DemoLightWork.demo_spotcol, color);
    DemoLightWork.demo_spotdecay[0] = falloff_start;
    DemoLightWork.demo_spotdecay[1] = shCosF(spread_pi);
    DemoLightWork.demo_spotdecay[2] = falloff_end;
    DemoLightWork.demo_spotdecay[3] = spread_pi;
}

/**
 * Fills a background block's VU1 light data with three parallel lights (in block-local space),
 * the eye direction, their colors and the ambient (brightened by the gun flash and the dark
 * light factor).
 * @param pbman the struct sh2gfw_BLOCK_MAN
 */
void sh2gfw_setparal3LGT_blk(void *pbman, float *light_dir0, float *light_dir1, float *light_dir2, float *col0, float *col1, float *col2, float *ambient) {
    float work[4][4];
    float tmp[4];
    struct sh2gfw_BLOCK_MAN *pB_man;
    struct sh2gfw_LIGHT_ENV *slv;

    pB_man = pbman;
    slv = &pB_man->blk_LightData;
    sceVu0ApplyMatrix(work[0], pB_man->World_Local, light_dir0);
    sceVu0ApplyMatrix(work[1], pB_man->World_Local, light_dir1);
    sceVu0ApplyMatrix(work[2], pB_man->World_Local, light_dir2);
    sh2gde_getCameraDir(work[3]);
    sceVu0ApplyMatrix(slv->light_col[3], pB_man->World_Local, work[3]);
    sh2gfw_Vu0NormalLightPhongMatrix(slv->light_vect, work[0], work[1], work[2], work[3]);
    {
        float iv[4] = { 160.0f, 160.0f, 160.0f, 250.0f };
        float gcol[4];
        float gref;

        VU1_PARMS.rgbamax = *(union Q_WORDDATA *)iv;
        sceVu0ScaleVector(slv->light_col[0], col0, 4.0f);
        sceVu0ScaleVector(slv->light_col[1], col1, 4.0f);
        sceVu0ScaleVector(slv->light_col[2], col2, 4.0f);
        sceVu0CopyVector(slv->BaseVertexColor.fl32, Env_ctl.BaseVertexColor);
        sceVu0CopyVector(slv->VertexAmbient.fl32, Env_ctl.VertexAmbient);
        gref = 2.0f * sh2gfw_Get_GunLight();
        sceVu0ScaleVector(gcol, ambient, 4.0f);
        gcol[0] += gref;
        gcol[1] += gref;
        gcol[2] += gref;
        sceVu0CopyVector(slv->light_amb, gcol);
        sceVu0CopyVector(slv->BaseAmbient.fl32, Env_ctl.BaseAmbient);
        slv->BaseAmbient.fl32[0] += gref;
        slv->BaseAmbient.fl32[1] += gref;
        slv->BaseAmbient.fl32[2] += gref;
        sceVu0ScaleVector(tmp, ambient, 4.0f * DARK_Hosei * sh2gfw_Get_DarkTrimFactor());
        tmp[3] = 0.0f;
        sceVu0AddVector(slv->light_amb, slv->light_amb, tmp);
        sceVu0ScaleVector(tmp, Env_ctl.BaseAmbient, 4.0f * DARK_Hosei * sh2gfw_Get_DarkTrimFactor());
        tmp[3] = 0.0f;
        sceVu0AddVector(slv->BaseAmbient.fl32, slv->BaseAmbient.fl32, tmp);
        slv->ALPHA_clear.fl32[0] = 128.0f;
        slv->ALPHA_clear.fl32[1] = 127.0f;
        slv->ALPHA_clear.ui32[2] = sh2gfw_Get_NightOrDay();
        slv->ALPHA_clear.ui32[3] = 0x80;
    }
}

/**
 * Sets a background block's lights for the frame: three parallel lights by day; at night the
 * flashlight or room spot light plus two parallel lights, switching the block's VU1 microcode
 * to match.
 * @param pb_man the struct sh2gfw_BLOCK_MAN
 */
void sh2gfw_LightSet_ForBG(void *pb_man) {
    struct sh2gfw_BLOCK_MAN *pB_man;
    float pl0[4];
    float pl1[4];
    float pl2[4];
    float pc0[4];
    float pc1[4];
    float pc2[4];
    float ab[4];
    int DemoFlg;

    pB_man = pb_man;
    Get_DemoOrPresetLight(pl0, pc0, 0);
    Get_DemoOrPresetLight(pl1, pc1, 1);
    Get_DemoOrPresetLight(pl2, pc2, 2);
    Get_DemoOrPresetAmbient(ab);
    if (sh2gfw_Get_NightOrDay() == 0) {
        sh2gfw_setparal3LGT_blk(pB_man, pl0, pl1, pl2, pc0, pc1, pc2, ab);
    } else {
        DemoFlg = sh2gfw_Check_DemoSpotLight();
        sh2gfw_Store_SpotLight();
        if (sh2gfw_Check_JmsSpotOnOff() && LightSpotOnOffCheck()) {
            sh2gfw_setsp1para2LGT_blk(pB_man, &Env_ctl.SpotL0, pl1, pc1, pl2, pc2, ab);
        } else if (DemoFlg || !sh2gde_CheckSpot_JmsOrBG()) {
            if (sh2gfw_Check_MicroCode() == 0) {
                kari_sh2gfw_vu_change(pB_man, 2);
                sh2gfw_Set_MicroChange();
            }
            sh2gfw_setsp1para2LGT_blk(pB_man, &Env_ctl.SpotL0, pl1, pc1, pl2, pc2, ab);
        } else {
            if (sh2gfw_Check_MicroCode()) {
                kari_sh2gfw_vu_change(pB_man, 1);
                sh2gfw_Set_MicroChange();
            }
            sh2gfw_setparal3LGT_blk(pB_man, pl0, pl1, pl2, pc0, pc1, pc2, ab);
        }
    }
}

/** Sets Env_ctl.SpotL0's position and direction from James's flashlight, the room's spot light or
 * the demo spot light. */
void sh2gfw_Store_SpotLight(void) {
    if (sh2gde_CheckSpot_JmsOrBG()) {
        if ((Sh2sys.main_status >> 6) & 1) {
            shGetJamesLightPosOriginal(Env_ctl.SpotL0.position.fl32, Env_ctl.SpotL0.dirvec.fl32);
        } else {
            shGetJamesLightPos(Env_ctl.SpotL0.position.fl32, Env_ctl.SpotL0.dirvec.fl32);
        }
    } else {
        sceVu0CopyVector(Env_ctl.SpotL0.position.fl32, *sh2gde_Get_BGSpotPos());
        sceVu0CopyVector(Env_ctl.SpotL0.dirvec.fl32, *sh2gde_Get_BGSpotDir());
    }
    if (DemoLightWork.Spot) {
        sceVu0CopyVector(Env_ctl.SpotL0.position.fl32, DemoLightWork.demo_spotpos);
        sceVu0CopyVector(Env_ctl.SpotL0.dirvec.fl32, DemoLightWork.demo_spotdir);
    }
}

/**
 * Fills a background block's VU1 light data with a spot light and two parallel lights.
 * @param pbman  the struct sh2gfw_BLOCK_MAN
 * @param sm     the spot light's struct sh2gfw_SPOTL_MATRIX (the demo spot light overrides it)
 * @param light0 first parallel light direction
 * @param color0 its color
 * @param light1 second parallel light direction
 * @param color1 its color
 * @param amb    ambient color
 */
void sh2gfw_setsp1para2LGT_blk(void *pbman, void *sm, float *light0, float *color0, float *light1, float *color1, float *amb) {
    float work[4][4];
    float dir_vec[4];
    float pos[4];
    float tmp[4];
    float coloralpha;
    struct sh2gfw_BLOCK_MAN *pB_man;
    struct sh2gfw_LIGHT_ENV *slv;
    struct sh2gfw_SPOTL_MATRIX *splm;

    pB_man = pbman;
    if (DemoLightWork.Spot) {
        splm = (struct sh2gfw_SPOTL_MATRIX *)DemoLightWork.demo_spotcol;
    } else {
        splm = sm;
    }
    slv = &pB_man->blk_LightData;
    sh2gde_get_spotposdir(pos, dir_vec);
    sh2gde_getCameraDir(work[3]);
    sceVu0ApplyMatrix(work[0], pB_man->World_Local, dir_vec);
    sceVu0ApplyMatrix(work[1], pB_man->World_Local, light0);
    sceVu0ApplyMatrix(work[2], pB_man->World_Local, light1);
    vwGetViewPosition(dir_vec);
    sceVu0ApplyMatrix(slv->light_col[3], pB_man->World_Local, dir_vec);
    sh2gfw_Vu0NormalLightPhongMatrix(slv->light_vect, work[0], work[1], work[2], work[3]);
    sceVu0ApplyMatrix(slv->light_vect[3], pB_man->World_Local, pos);
    shGetJamesLightPosOriginal(work[0], work[1]);
    work[1][3] = 0.0f;
    work[0][3] = 1.0f;
    sceVu0ApplyMatrix(slv->LightRealPos, pB_man->World_Local, work[0]);
    sceVu0ApplyMatrix(slv->LightRealDir, pB_man->World_Local, work[1]);
    {
        float iv[4] = { 160.0f, 160.0f, 160.0f, 250.0f };
        float gcol[4];
        float gref;

        slv->light_pam[0][1] = -splm->decayparm.fl32[1];
        slv->light_pam[1][0] = 1.0f / (splm->decayparm.fl32[0] - splm->decayparm.fl32[2]);
        slv->light_pam[0][0] = -slv->light_pam[1][0] * splm->decayparm.fl32[2];
        VU1_PARMS.rgbamax = *(union Q_WORDDATA *)iv;
        coloralpha = splm->color.fl32[3];
        sceVu0ScaleVector(slv->light_col[0], splm->color.fl32, 4.0f);
        sceVu0ScaleVector(slv->light_col[1], color0, 4.0f);
        sceVu0ScaleVector(slv->light_col[2], color1, 4.0f);
        sceVu0ScaleVector(gcol, amb, 4.0f);
        gref = 2.0f * sh2gfw_Get_GunLight();
        gcol[0] += gref;
        gcol[1] += gref;
        gcol[2] += gref;
        sceVu0CopyVector(slv->light_amb, gcol);
        sceVu0CopyVector(slv->BaseAmbient.fl32, Env_ctl.BaseAmbient);
        slv->BaseAmbient.fl32[0] += gref;
        slv->BaseAmbient.fl32[1] += gref;
        slv->BaseAmbient.fl32[2] += gref;
        sceVu0ScaleVector(tmp, amb, 4.0f * DARK_Hosei * sh2gfw_Get_DarkTrimFactor());
        tmp[3] = 0.0f;
        sceVu0AddVector(slv->light_amb, slv->light_amb, tmp);
        sceVu0ScaleVector(tmp, Env_ctl.BaseAmbient, DARK_Hosei * sh2gfw_Get_DarkTrimFactor());
        tmp[3] = 0.0f;
        sceVu0AddVector(slv->BaseAmbient.fl32, slv->BaseAmbient.fl32, tmp);
        sceVu0CopyVector(slv->BaseVertexColor.fl32, Env_ctl.BaseVertexColor);
        sceVu0CopyVector(slv->VertexAmbient.fl32, Env_ctl.VertexAmbient);
        slv->light_col[0][3] = coloralpha;
        slv->ALPHA_clear.fl32[0] = 128.0f;
        slv->ALPHA_clear.fl32[1] = 127.0f;
        slv->ALPHA_clear.ui32[2] = sh2gfw_Check_JmsSpotOnOff();
        slv->ALPHA_clear.ui32[3] = 0x7F;
    }
}

/** Returns 1 at night, 0 by day (Env_ctl.light_mode - 1). */
int sh2gfw_Get_NightOrDay(void) {
    return Env_ctl.light_mode - 1;
}

/** Sets night (nod non-zero) or day lighting. Declared int in the DWARF; returns nothing. */
int SetLightMode(int nod) {
    Env_ctl.light_mode = nod ? 2 : 1;
}

/** Copies parallel light g's direction and color: the active demo override, else the room's. */
void Get_DemoOrPresetLight(float *pl, float *pc, int g) {
    if (DemoLightWork.Parallel[g] <= 0) {
        switch (g) {
        case 0:
            sceVu0CopyVector(pl, Env_ctl.p_light0);
            sceVu0CopyVector(pc, Env_ctl.p_color0);
            break;
        case 1:
            sceVu0CopyVector(pl, Env_ctl.p_light1);
            sceVu0CopyVector(pc, Env_ctl.p_color1);
            break;
        case 2:
            sceVu0CopyVector(pl, Env_ctl.p_light2);
            sceVu0CopyVector(pc, Env_ctl.p_color2);
            break;
        }
    } else {
        switch (g) {
        case 0:
            sceVu0CopyVector(pl, DemoLightWork.demo_p_light0);
            sceVu0CopyVector(pc, DemoLightWork.demo_p_color0);
            break;
        case 1:
            sceVu0CopyVector(pl, DemoLightWork.demo_p_light1);
            sceVu0CopyVector(pc, DemoLightWork.demo_p_color1);
            break;
        case 2:
            sceVu0CopyVector(pl, DemoLightWork.demo_p_light2);
            sceVu0CopyVector(pc, DemoLightWork.demo_p_color2);
            break;
        }
    }
}

/** Copies the ambient color: the demo override if set, else the room's. */
void Get_DemoOrPresetAmbient(float *ab) {
    if (DemoLightWork.Amb == 0) {
        sceVu0CopyVector(ab, Env_ctl.ambient);
    } else {
        sceVu0CopyVector(ab, DemoLightWork.demo_ambient);
    }
}

/** Forces the flashlight off in a fixed list of maps (only when BgIsOut(0) is 0). */
void sh2gfw_Set_JmsSpot_OnOrOff(void) {
    int *mp;

    if (BgIsOut(0) == 0) {
        mp = Get_NowMapId();
        switch (*mp) {
        case 0x90054:
        case 0xB0055:
        case 0xB00BD:
        case 0xB00C1:
        case 0xC005B:
        case 0xD0015:
        case 0xD0049:
        case 0xE0001:
            sh2gfw_ForceOff_JmsSPOT();
            break;
        }
    }
}
