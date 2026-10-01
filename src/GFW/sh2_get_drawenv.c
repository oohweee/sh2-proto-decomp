/*
 * Draw-environment queries (GFW): the camera eye direction, James's facing, and the flashlight
 * / demo spot light parameters in the layouts the character lighting and shadows use.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static float shEyeDirVec[4];
static float shJmsDirVec[4];
static float shEyeToJmsDirVec[4];

/** Per frame: computes the camera eye direction, the camera-to-James vector and James's facing direction. */
void sh2gfw_Set_EyeDir(void) {
    float wvm[4][4] __attribute__((aligned(16)));
    float vv[4][4] __attribute__((aligned(16))) = {
        { 0.0f, 0.0f, 1.0f, 0.0f },
    };

    sceVu0CopyMatrix(wvm, VbWvsMatrix.wvm);
    sceVu0InversMatrix(wvm, wvm);
    sceVu0ApplyMatrix(shEyeDirVec, wvm, (float *)vv);
    vwGetViewPosition(shEyeToJmsDirVec);
    sceVu0SubVector(shEyeToJmsDirVec, (float *)&sh2jms.player->pos, shEyeToJmsDirVec);
    sceVu0UnitMatrix(wvm);
    sceVu0RotMatrixY(wvm, wvm, sh2jms.player->rot.y);
    sceVu0ApplyMatrix(shJmsDirVec, wvm, (float *)vv);
}

/** Copies the camera eye direction to svt. */
void sh2gde_Get_EyeDir(float *svt) {
    sceVu0CopyVector(svt, shEyeDirVec);
}

/** Returns the dot product of the eye direction and James's facing direction. */
float sh2gde_Get_InnerVectorJmsCam(void) {
    return _shInnerProduct(shJmsDirVec, shEyeDirVec);
}

/**
 * Gets the flashlight (or demo spot light) position, direction and decay parameters.
 * The decay parameters come in the order sh_Kari_LightSetSpot expects.
 */
void kari_sh2gde_getspotParams(float *spotpos, float *spotdir, float *decayparms) {
    float tmp[4] __attribute__((aligned(16)));

    if (sh2gfw_Check_DemoSpotLight()) {
        sh2gde_get_spotposdir(spotpos, spotdir);
        sh2gfw_Get_DemoSpotParm(tmp);
        decayparms[0] = tmp[1];
        decayparms[1] = tmp[3];
        decayparms[2] = tmp[0];
        decayparms[3] = tmp[2];
    } else {
        shGetJamesLightPosOriginal(spotpos, spotdir);
        tmp[0] = Env_ctl.SpotL0.decayparm.fl32[1];
        tmp[1] = Env_ctl.SpotL0.decayparm.fl32[3];
        tmp[2] = Env_ctl.SpotL0.decayparm.fl32[2];
        tmp[3] = Env_ctl.SpotL0.decayparm.fl32[0];
    }
    sceVu0CopyVector(decayparms, tmp);
}

/** kari_sh2gde_getspotParams for the shadow code (also queries James's light position). */
void sh2gde_getspotParams_for_Shadow(float *spotpos, float *spotdir, float *decayparms) {
    float tmp[4] __attribute__((aligned(16)));

    if (sh2gfw_Check_DemoSpotLight()) {
        sh2gde_get_spotposdir(spotpos, spotdir);
        sh2gfw_Get_DemoSpotParm(tmp);
        decayparms[0] = tmp[1];
        decayparms[1] = tmp[3];
        decayparms[2] = tmp[0];
        decayparms[3] = tmp[2];
    } else {
        shGetJamesLightPosOriginal(spotpos, spotdir);
        shGetJamesLightPos(tmp, spotdir);
        tmp[0] = Env_ctl.SpotL0.decayparm.fl32[1];
        tmp[1] = Env_ctl.SpotL0.decayparm.fl32[3];
        tmp[2] = Env_ctl.SpotL0.decayparm.fl32[2];
        tmp[3] = Env_ctl.SpotL0.decayparm.fl32[0];
    }
    sceVu0CopyVector(decayparms, tmp);
}

/**
 * Gets the flashlight (or demo spot light) position, direction, color and decay parameters.
 * @param getmode 1: position and direction from James's light; 0: from Env_ctl.SpotL0
 */
void sh2gde_getspotKTParams(float *spotpos, float *spotdir, float *spotcol, float *decayparms, int getmode) {
    float pos[4] __attribute__((aligned(16)));
    float dir_vec[4] __attribute__((aligned(16)));
    float tmp[4] __attribute__((aligned(16)));

    if (getmode) {
        if (sh2gfw_Check_DemoSpotLight()) {
            sh2gfw_Get_DemoSpotPosDirCol(pos, dir_vec, spotcol);
        } else {
            shGetJamesLightPosOriginal(pos, dir_vec);
            sceVu0CopyVector(spotcol, (float *)&Env_ctl.SpotL0.color);
        }
        sceVu0CopyVector(spotpos, pos);
        sceVu0CopyVector(spotdir, dir_vec);
    } else {
        if (sh2gfw_Check_DemoSpotLight()) {
            sh2gfw_Get_DemoSpotPosDirCol(spotpos, spotdir, spotcol);
        } else {
            sh2gde_get_spotposdir(spotpos, spotdir);
            sceVu0CopyVector(spotcol, (float *)&Env_ctl.SpotL0.color);
        }
    }
    if (sh2gfw_Check_DemoSpotLight()) {
        sh2gfw_Get_DemoSpotParm(tmp);
        decayparms[0] = tmp[1];
        decayparms[1] = tmp[3];
        decayparms[2] = tmp[0];
        decayparms[3] = tmp[2];
    } else {
        tmp[0] = Env_ctl.SpotL0.decayparm.fl32[1];
        tmp[1] = Env_ctl.SpotL0.decayparm.fl32[3];
        tmp[2] = Env_ctl.SpotL0.decayparm.fl32[0];
        tmp[3] = Env_ctl.SpotL0.decayparm.fl32[2];
        sceVu0CopyVector(decayparms, tmp);
    }
}

/** Copies the camera eye direction to cameradir. */
void sh2gde_getCameraDir(float *cameradir) {
    sh2gde_Get_EyeDir(cameradir);
}

/** Copies the flashlight position and direction from Env_ctl.SpotL0. */
void sh2gde_get_spotposdir(float *pos, float *dir) {
    vcopy_dst_first(pos, Env_ctl.SpotL0.position.fl32);
    vcopy_dst_first(dir, Env_ctl.SpotL0.dirvec.fl32);
}

/** Returns the room's spotLNum; callers take 0 to mean the room's own spot light replaces James's flashlight. */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 321
int sh2gde_CheckSpot_JmsOrBG(void) {
    struct DrawEnvData *Now;

    Now = Get_NowDrawEnvData();
    assert(Now!=NULL);
    return Now->spotLNum;
}

/** Returns the room's background spot light position (DrawEnvData Ld_2). */
float (*sh2gde_Get_BGSpotPos(void))[4] {
    struct DrawEnvData *Now;

    Now = Get_NowDrawEnvData();
    return &Now->Ld_2;
}

/** Returns the room's background spot light direction (DrawEnvData Ld_3). */
float (*sh2gde_Get_BGSpotDir(void))[4] {
    struct DrawEnvData *Now;

    Now = Get_NowDrawEnvData();
    return &Now->Ld_3;
}
