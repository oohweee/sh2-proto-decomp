/*
 * James's flashlight and gun-flash lighting (Chacter_Draw): switching the spot light on and off
 * (with a fade and a flicker), the matching VU1 microcode switch for the background, the
 * gun-muzzle reflection, and the per-frame camera-following parallel light and highlight.
 */
#include "sh2.h"
#include "sdk/libvu0.h"

/* Matching: these sh_vu0.h helpers are declared here and defined after Programed_Light_Set (sh_vu0.h is
 * included after it), so it calls out-of-line copies placed after it, as the original does
 * (docs/matching-notes.md#sh2_jmsspot_man-programed_light_set). */
static inline void _shAddVector(float *v0, float *v1, float *v2);
static inline void _shScaleVector(float *v0, float *v1, float s);
static inline void _shNormalize(float *v0, float *v1);
static inline void _shOuterProduct(float *v0, float *v1, float *v2);
static inline float _shInnerProduct(float *v0, float *v1);

struct James_SpotLight_Man JmsSpotMan = {
    0, 0, 0, 0, 60, 0, 1, 0, 450, 0, 0, 0, 3, 0, 100, 0,
    0.0f, 7.0f, 0.001f, 0.0f, 1500.0f, 0.001f, 0.0f, 1.0f, 1.0f,
};

static float colvec[4] __attribute__((aligned(16))) = {0.3f, 0.3f, 0.3f, 0.0f};
static float colref[4] __attribute__((aligned(16))) = {1.0f, 1.0f, 1.0f, 0.0f};
static struct DynamicLight DynamicLW __attribute__((aligned(16))) = {
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f, 0.0f},
};

/** Resets the flashlight state (JmsSpotMan) to its defaults. */
void sh2gfw_ALLInit_SpotMan(void) {
    JmsSpotMan.GunFlg = 0;
    JmsSpotMan.Spot_Light_OnOff = 0;
    JmsSpotMan.MicroCodeMode = 0;
    JmsSpotMan.MicroChange = 0;
    JmsSpotMan.SpotTime = 0;
    JmsSpotMan.MaxTime = 60;
    JmsSpotMan.MinTime = 1;
    JmsSpotMan.SDt = 0;
    JmsSpotMan.DarkTrimMax = 450;
    JmsSpotMan.DarkDt = 0;
    JmsSpotMan.NowDarkParm = 0;
    JmsSpotMan.FlickerRatio = 3;
    JmsSpotMan.FlickerTime = 0;
    JmsSpotMan.FlickerMaxTime = 100;
    JmsSpotMan.FlickerFlg = 0;
    JmsSpotMan.Gun_Refrection = 0.0f;
    JmsSpotMan.MaxIntensity = 7.0f;
    JmsSpotMan.MinIntensity = 0.001f;
    JmsSpotMan.NowIntensity = 0.0f;
    JmsSpotMan.MaxAlpha = 1500.0f;
    JmsSpotMan.MinAlpha = 0.001f;
    JmsSpotMan.NowAlpha = 0.0f;
    JmsSpotMan.IntensityFactor = 1.0f;
    JmsSpotMan.DarkFactor = 1.0f;
}

/** Per frame: fades the gun-flash reflection out by 0.15 unless a shot was fired this frame. */
void sh2gfw_Propagate_JmsGunLight(void) {
    if (!JmsSpotMan.GunFlg) {
        if (JmsSpotMan.Gun_Refrection > 0.15f) {
            JmsSpotMan.Gun_Refrection -= 0.15f;
        } else {
            JmsSpotMan.Gun_Refrection = 0.0f;
        }
    } else {
        JmsSpotMan.GunFlg = 0;
    }
}

/** Starts a gun flash (reflection 0.75). */
void sh2gfw_Set_JmsGunLight(void) {
    JmsSpotMan.Gun_Refrection = 0.75f;
    JmsSpotMan.GunFlg = 1;
}

/** Returns James's reflection level: 0.075, plus the fading gun flash once the shot frame is over. */
float sh2gfw_Get_JmsGunLight(void) {
    if (JmsSpotMan.GunFlg) {
        return 0.075f;
    }
    return 0.075f + JmsSpotMan.Gun_Refrection;
}

/** Returns the fading gun-flash level, 0 on the shot frame itself. */
float sh2gfw_Get_GunLight(void) {
    if (JmsSpotMan.GunFlg) {
        return 0.0f;
    }
    return JmsSpotMan.Gun_Refrection;
}

/** Clears the pending microcode-change count. */
void sh2gfw_Clear_MicroChange(void) {
    JmsSpotMan.MicroChange = 0;
}

/** Requests a microcode change. */
void sh2gfw_Set_MicroChange(void) {
    JmsSpotMan.MicroChange++;
}

/** Toggles the microcode mode if a change was requested. */
void sh2gfw_Change_MicroFLGS(void) {
    if (JmsSpotMan.MicroChange) {
        JmsSpotMan.MicroCodeMode ^= 1;
    }
}

/** Returns the microcode mode (0: normal, 1: spot light). */
int sh2gfw_Check_MicroCode(void) {
    return JmsSpotMan.MicroCodeMode;
}

/** Sets the microcode mode to xx - 1 (xx is the kari_sh2gfw_vu_change mode, 1 or 2). */
void sh2gfw_Set_MicroCode(int xx) {
    JmsSpotMan.MicroCodeMode = xx - 1;
}

/** Returns the current dark-light factor (DarkFactor). */
float sh2gfw_Get_DarkTrimFactor(void) {
    return JmsSpotMan.DarkFactor;
}

static float sh2gfw_Get_MaxDarkScaleFactor(void) {
    int *mp;

    if (!((item.flag[0] >> 15) & 1)) {
        return 1.0f;
    }
    mp = Get_NowMapId();
    switch (*mp) {
        case 0xA003D:
        case 0xA007D:
        case 0xA0091:
        case 0xA00E6:
            return 0.0f;
    }
    return 0.3f;
}

/** Returns whether the flashlight is on. */
int sh2gfw_Check_JmsSpotOnOff(void) {
    return JmsSpotMan.Spot_Light_OnOff;
}

/**
 * Switches the flashlight on (at night, where the map allows it): spot-light microcode for the
 * background, light mode 2, and a fade-in over MaxTime frames.
 */
void sh2gfw_On_JmsSPOT(void) {
    int slot;

    if (sh2gfw_Get_NightOrDay() && sh2gde_CheckSpot_JmsOrBG()) {
        for (slot = 0; slot < 4; slot++) {
            if (b_man[slot].pB_H) {
                kari_sh2gfw_vu_change(&b_man[slot], 2);
            }
        }
        sh2gfw_Set_MicroCode(2);
        SetLightMode(2);
        Set_DrawEnvData(0, 1);
        JmsSpotMan.Spot_Light_OnOff = 1;
        JmsSpotMan.SDt = 2;
        JmsSpotMan.MaxTime = 60;
    }
}

static int sh2gfw_TrueOff_JmsSPOT(void) {
    int slot;

    if (sh2gfw_Get_NightOrDay() && sh2gde_CheckSpot_JmsOrBG()) {
        if (sh2gfw_Check_DemoSpotLight()) {
            for (slot = 0; slot < 4; slot++) {
                if (b_man[slot].pB_H) {
                    kari_sh2gfw_vu_change(&b_man[slot], 2);
                }
            }
            sh2gfw_Set_MicroCode(2);
        } else {
            for (slot = 0; slot < 4; slot++) {
                if (b_man[slot].pB_H) {
                    kari_sh2gfw_vu_change(&b_man[slot], 1);
                }
            }
            sh2gfw_Set_MicroCode(1);
        }
        Env_ctl.SpotL0.color.fl32[0] = Env_ctl.SpotL0.color.fl32[1] = Env_ctl.SpotL0.color.fl32[2] = 0.001f;
        Env_ctl.SpotL0.color.fl32[3] = 1500.0f;
        JmsSpotMan.Spot_Light_OnOff = 0;
        JmsSpotMan.DarkDt = 1;
        sh2gfw_Set_ShadowColorScaleFactor(0.0f);
    }
}

/** Starts fading the flashlight out (at night, where the map allows it). */
void sh2gfw_Off_JmsSPOT(void) {
    if (sh2gfw_Get_NightOrDay() && sh2gde_CheckSpot_JmsOrBG()) {
        JmsSpotMan.SpotTime = JmsSpotMan.MaxTime;
        JmsSpotMan.SDt = -10;
    }
}

/** Switches the flashlight off at once (at night, where the map allows it). */
void sh2gfw_ForceOff_JmsSPOT(void) {
    if (sh2gfw_Get_NightOrDay() && sh2gde_CheckSpot_JmsOrBG()) {
        JmsSpotMan.SpotTime = 0;
        JmsSpotMan.SDt = -1;
        sh2gfw_TrueOff_JmsSPOT();
        JmsSpotMan.DarkDt = 0;
        JmsSpotMan.SDt = 0;
    }
}

/** Switches the flashlight on at full intensity at once (at night, where the map allows it). */
void sh2gfw_ForceOn_JmsSPOT(void) {
    if (sh2gfw_Get_NightOrDay() && sh2gde_CheckSpot_JmsOrBG()) {
        sh2gfw_On_JmsSPOT();
        JmsSpotMan.SpotTime = JmsSpotMan.MaxTime;
        sh2gfw_Set_ShadowColorScaleFactor(1.0f);
    }
}

static float LinearTrim(float Yen, float Yst, float Xen, float Xst, float Parm) {
    return Yst + (Yen - Yst) * (Parm - Xst) / (Xen - Xst);
}

static void Flicker_Jms_Spot(void) {
    unsigned int ira;
    unsigned int irc;
    unsigned int isd;

    JmsSpotMan.FlickerTime++;
    if (JmsSpotMan.FlickerTime >= JmsSpotMan.FlickerMaxTime) {
        JmsSpotMan.FlickerTime = 0;
        if (JmsSpotMan.FlickerFlg) {
            sh2gfw_ForceOn_JmsSPOT();
        } else {
            sh2gfw_ForceOff_JmsSPOT();
        }
    } else {
        isd = Env_ctl.random_seeds.ui32[0];
        ira = 0x19660D;
        irc = 0x3C6EF35F;
        isd = isd * ira + irc;
        Env_ctl.random_seeds.ui32[0] = isd;
        JmsSpotMan.IntensityFactor = isd / 4294967296.0f;
        JmsSpotMan.NowIntensity = JmsSpotMan.MaxIntensity - JmsSpotMan.FlickerRatio * JmsSpotMan.IntensityFactor;
    }
    Env_ctl.SpotL0.color.fl32[0] = Env_ctl.SpotL0.color.fl32[1] = Env_ctl.SpotL0.color.fl32[2] = JmsSpotMan.NowIntensity;
    sh2gfw_Set_ShadowColorScaleFactor(JmsSpotMan.IntensityFactor);
}

static void Calculate_Jms_Spot(void) {
    float factor;

    JmsSpotMan.SpotTime += JmsSpotMan.SDt;
    if (JmsSpotMan.SDt > 0 && JmsSpotMan.SpotTime >= JmsSpotMan.MaxTime) {
        JmsSpotMan.SpotTime = JmsSpotMan.MaxTime;
        JmsSpotMan.SDt = 0;
    } else if (JmsSpotMan.SDt < 0 && JmsSpotMan.SpotTime <= JmsSpotMan.MinTime) {
        JmsSpotMan.SpotTime = JmsSpotMan.MinTime;
        JmsSpotMan.SDt = 0;
    }
    factor = LinearTrim(1.0f, 0.0f, JmsSpotMan.MaxTime, JmsSpotMan.MinTime, JmsSpotMan.SpotTime);
    JmsSpotMan.NowIntensity = factor;
    Env_ctl.SpotL0.color.fl32[0] = Env_ctl.SpotL0.color.fl32[1] = Env_ctl.SpotL0.color.fl32[2] = JmsSpotMan.MaxIntensity * factor;
    JmsSpotMan.IntensityFactor = factor;
    sh2gfw_Set_ShadowColorScaleFactor(factor);
    if (JmsSpotMan.NowIntensity <= 0.2 && !JmsSpotMan.SDt) {
        sh2gfw_TrueOff_JmsSPOT();
    }
}

static int Light_Spot_OnOffCheck2(void) {
    return ((item.flag[0] >> 15) & 1) != 0;
}

/**
 * Per-frame scripted lighting: turns a direction that lags behind the camera, sets the
 * reflection highlight along it, keeps the flashlight state in step with the item flag and the
 * map, runs the flashlight fade or flicker, and sets the dim parallel "dark" light that stands
 * in for it when it is off.
 */
void Programed_Light_Set(void) {
    float inner;
    float factor;
    float MaxFactor;
    float tmp[4];
    float plane[4];
    float aimdir[4];
    int ipara;
    int slot;

    ipara = sh2gfw_Check_ParallelDemoLight(1);
    sceVu0CopyVector(DynamicLW.BeforeCamDir, DynamicLW.NowCamDir);
    sceVu0CopyVector(DynamicLW.BeforeDir, DynamicLW.NowDir);
    sh2gde_Get_EyeDir(DynamicLW.NowCamDir);
    inner = _shInnerProduct(DynamicLW.NowCamDir, DynamicLW.BeforeDir);
    _shOuterProduct(plane, DynamicLW.NowCamDir, DynamicLW.BeforeDir);
    _shNormalize(plane, plane);
    _shOuterProduct(aimdir, DynamicLW.BeforeDir, plane);
    inner = 0.1f / (2.0f + inner);
    if (inner < 0.0333f) {
        inner = 0.0f;
    }
    _shScaleVector(aimdir, aimdir, inner);
    _shAddVector(tmp, aimdir, DynamicLW.BeforeDir);
    _shNormalize(DynamicLW.NowDir, tmp);
    if (!sh2gfw_Check_DemoRefrectionHightLight() && !DramaDemoNumber()) {
        if (sh2gfw_Get_NightOrDay()) {
            sh2gfw_Set_DemoRefrectionHighLight(DynamicLW.NowDir, colref);
        } else {
            sceVu0ScaleVector(tmp, colref, 0.7f);
            sh2gfw_Set_DemoRefrectionHighLight(DynamicLW.NowDir, tmp);
        }
    }
    if (!sh2gde_CheckSpot_JmsOrBG()) {
        if (!sh2gfw_Check_JmsSpotOnOff() && sh2gfw_Get_NightOrDay()) {
            for (slot = 0; slot < 4; slot++) {
                if (b_man[slot].pB_H) {
                    kari_sh2gfw_vu_change(&b_man[slot], 2);
                }
            }
            sh2gfw_Set_MicroCode(2);
            SetLightMode(2);
            Set_DrawEnvData(0, 1);
            JmsSpotMan.Spot_Light_OnOff = 1;
            JmsSpotMan.SDt = 2;
            JmsSpotMan.MaxTime = 60;
            JmsSpotMan.SpotTime = JmsSpotMan.MaxTime;
        }
    } else if (sh2gfw_Check_JmsSpotOnOff() == 1 && !Light_Spot_OnOffCheck2()) {
        JmsSpotMan.Spot_Light_OnOff = 0;
        sh2gfw_ForceOff_JmsSPOT();
    }
    if (JmsSpotMan.FlickerTime) {
        Flicker_Jms_Spot();
    } else if (JmsSpotMan.SDt) {
        Calculate_Jms_Spot();
    }
    MaxFactor = sh2gfw_Get_MaxDarkScaleFactor();
    if (MaxFactor > 0.0f && sh2gfw_Get_NightOrDay()) {
        if (sh2gfw_Check_JmsSpotOnOff()) {
            if (JmsSpotMan.SDt > 0) {
                MaxFactor = JmsSpotMan.DarkFactor - JmsSpotMan.IntensityFactor;
                if (!(MaxFactor < 0.001f)) {
                    sceVu0ScaleVector(tmp, colvec, MaxFactor);
                    JmsSpotMan.DarkFactor = MaxFactor;
                    if (!ipara) {
                        sh2gfw_Set_PallarelLight(DynamicLW.NowDir, tmp, 1);
                    }
                }
                JmsSpotMan.NowDarkParm = 0;
            }
        } else if (!ipara) {
            if (JmsSpotMan.DarkDt && JmsSpotMan.DarkTrimMax >= JmsSpotMan.NowDarkParm) {
                JmsSpotMan.NowDarkParm = JmsSpotMan.NowDarkParm + JmsSpotMan.DarkDt;
                MaxFactor = LinearTrim(MaxFactor, 0.0f, JmsSpotMan.DarkTrimMax, 0.0f, JmsSpotMan.NowDarkParm);
            } else {
                JmsSpotMan.DarkDt = 0;
                JmsSpotMan.NowDarkParm = 0;
            }
            sceVu0ScaleVector(tmp, colvec, MaxFactor);
            JmsSpotMan.DarkFactor = MaxFactor;
            sh2gfw_Set_PallarelLight(DynamicLW.NowDir, tmp, 1);
            sh2gfw_Set_ShadowColorScaleFactor(MaxFactor);
        } else {
            sh2gfw_Set_ShadowColorScaleFactor(1.0f);
        }
    }
}

/* Matching: the deferred definitions (see the declarations at the top). */
#include "sh_vu0.h"
