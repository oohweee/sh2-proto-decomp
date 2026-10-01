/*
 * Draw-environment settings (GFW): screen brightness, fog and clear colors, depth fog, the
 * background VU1 microcode mode, block position overrides, and the room's spot light mode.
 */
#include "sh2.h"
#include "sdk/libvu0.h"

float zzzzzz;
float hohoho;

/**
 * Sets the screen brightness (the copy filter color) from a level 0-7; 3 is neutral (0x80).
 * Level 0 falls through to level 1's value, as in the original.
 */
void sh2gfw_Set_Brightness(int pow) {
    switch (pow) {
    case 0:
        Env_ctl.CopyFilterColor.uc8[0] = Env_ctl.CopyFilterColor.uc8[1] = Env_ctl.CopyFilterColor.uc8[2] = 0x4B;
    case 1:
        Env_ctl.CopyFilterColor.uc8[0] = Env_ctl.CopyFilterColor.uc8[1] = Env_ctl.CopyFilterColor.uc8[2] = 0x5A;
        break;
    case 2:
        Env_ctl.CopyFilterColor.uc8[0] = Env_ctl.CopyFilterColor.uc8[1] = Env_ctl.CopyFilterColor.uc8[2] = 0x6E;
        break;
    case 4:
        Env_ctl.CopyFilterColor.uc8[0] = Env_ctl.CopyFilterColor.uc8[1] = Env_ctl.CopyFilterColor.uc8[2] = 0x8C;
        break;
    case 5:
        Env_ctl.CopyFilterColor.uc8[0] = Env_ctl.CopyFilterColor.uc8[1] = Env_ctl.CopyFilterColor.uc8[2] = 0x9B;
        break;
    case 6:
        Env_ctl.CopyFilterColor.uc8[0] = Env_ctl.CopyFilterColor.uc8[1] = Env_ctl.CopyFilterColor.uc8[2] = 0xB4;
        break;
    case 7:
        Env_ctl.CopyFilterColor.uc8[0] = Env_ctl.CopyFilterColor.uc8[1] = Env_ctl.CopyFilterColor.uc8[2] = 0xDC;
        break;
    case 3:
    default:
        Env_ctl.CopyFilterColor.uc8[0] = Env_ctl.CopyFilterColor.uc8[1] = Env_ctl.CopyFilterColor.uc8[2] = 0x80;
        break;
    }
}

/** Sets the GS FOGCOL register value in the default environment. */
void kari_sh2setGS_fogcolor(unsigned int fr, unsigned int fg, unsigned int fb) {
    shGs_AllEnv.DefaultEnv[7].ul64[1] = 0x3D;
    shGs_AllEnv.DefaultEnv[7].ul64[0] = (unsigned long)(unsigned char)fr | ((unsigned long)(unsigned char)fg << 8) | ((unsigned long)(unsigned char)fb << 16);
}

/** Sets the clear color of the three draw environments. */
void kari_sh2setGS_clearcolor(unsigned int cr, unsigned int cg, unsigned int cb, unsigned int ca) {
    int i;

    for (i = 0; i < 3; i++) {
        *(unsigned long *)&shGs_AllEnv.DrawEnv[i].clear.rgbaq = (unsigned long)cr | ((unsigned long)cg << 8) | ((unsigned long)cb << 16) | ((unsigned long)ca << 24) | ((unsigned long)0x3F800000 << 32);
    }
}

/**
 * Switches a background block's packets to the VU1 microprogram for light_mode: rewrites the
 * VIF OFFSET and MSCAL address of every VU tag in the block.
 */
void kari_sh2gfw_vu_change(struct sh2gfw_BLOCK_MAN *bm, unsigned int light_mode) {
    union Q_WORDDATA *qwd;
    unsigned int i;
    unsigned int j;
    unsigned int k;
    unsigned int igg;
    unsigned int ivumax;
    unsigned int vuk;
    unsigned int itopaddr;
    unsigned int basead;
    unsigned int offsetad;

    for (i = 0; i < 4; i++) {
        igg = bm->gifnum[i];
        for (j = 0; j < igg; j++) {
            ivumax = bm->vunum[i][j];
            for (k = 0; k < ivumax; k++) {
                qwd = &bm->pBlockPack[i][bm->idVU_tag[i][j][k]];
                itopaddr = qwd->uc8[2];
                vuk = sh2gfw_Change_VUmode(light_mode, itopaddr, &basead, &offsetad);
                qwd[0].ui32[0] = (vuk << 16) | 0x10000000;
                qwd[1].ui32[0] = 0x10000000;
                qwd[1].ui32[1] = 0;
                qwd[1].ui32[2] = offsetad | 0x2000000;
                qwd[1].ui32[3] = vuk | 0x14000000;
            }
        }
    }
}

/**
 * Rotates block 0x4000B by -90 degrees about y around a fixed point (with the zzzzzz/hohoho
 * debug offsets) if it is loaded.
 * @return 1 if the block was found
 */
int sh2gfw_Rotate_CD11(void) {
    int mapidno;
    int slot;
    struct sh2gfw_BLOCK_MAN *pB_man;

    mapidno = 0x4000B;
    for (slot = 0; slot < 5; slot++) {
        pB_man = &b_man[slot];
        if (pB_man->pB_H != NULL) {
            if (*(int *)pB_man->pB_H == mapidno) {
                float svt[4] = { -50000.0f, 0.0f, -70000.0f, 1.0f };
                float tmp[4];

                sceVu0RotMatrixY(pB_man->Local_World, (void *)pB_man->p_Matrices, -3.141592f / 2);
                sceVu0SubVector(tmp, (float *)&pB_man->p_Matrices[3], svt);
                tmp[0] += -1000.0f + hohoho;
                tmp[2] += -8000.0f + zzzzzz;
                tmp[3] = 0.0f;
                sceVu0ApplyMatrix(tmp, pB_man->Local_World, tmp);
                sceVu0AddVector(pB_man->Local_World[3], svt, tmp);
                sceVu0InversMatrix(pB_man->World_Local, pB_man->Local_World);
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Moves a loaded background block: its translation becomes its original one plus jv.
 * @return 1 if the block was found
 */
int sh2gfw_Jump_BlockPos(int mapidno, float *jv) {
    int slot;
    struct sh2gfw_BLOCK_MAN *pB_man;
    float (*sv)[4];

    for (slot = 0; slot < 5; slot++) {
        pB_man = &b_man[slot];
        if (pB_man->pB_H != NULL && *(int *)pB_man->pB_H == mapidno) {
            sv = (float (*)[4])pB_man->p_Matrices;
            sceVu0AddVector(pB_man->Local_World[3], (float *)(sv + 3), jv);
            sceVu0InversMatrix(pB_man->World_Local, pB_man->Local_World);
            return 1;
        }
    }
    return 0;
}

/**
 * Restores a loaded background block's original local-to-world matrix.
 * @return 1 if the block was found
 */
int sh2gfw_Reset_BlockPos(int mapidno) {
    int slot;
    struct sh2gfw_BLOCK_MAN *pB_man;

    for (slot = 0; slot < 5; slot++) {
        pB_man = &b_man[slot];
        if (pB_man->pB_H != NULL && *(int *)pB_man->pB_H == mapidno) {
            sceVu0CopyMatrix(pB_man->Local_World, (float (*)[4])pB_man->p_Matrices);
            sceVu0InversMatrix(pB_man->World_Local, pB_man->Local_World);
            return 1;
        }
    }
    return 0;
}

/** Sets the fog end distance (Env_ctl.fogparm[0]). */
void sh2gfw_Set_Fogfar(float fogfar) {
    Env_ctl.fogparm.fl32[0] = fogfar;
}

/** Sets the fog start distance (Env_ctl.fogparm[1]). */
void sh2gfw_Set_FogNear(float fogNear) {
    Env_ctl.fogparm.fl32[1] = fogNear;
}

/** Sets the fog maximum (Env_ctl.fogparm[2]). */
void sh2gfw_Set_FogMax(float fogMax) {
    Env_ctl.fogparm.fl32[2] = fogMax;
}

/** Sets the fog minimum (Env_ctl.fogparm[3]). */
void sh2gfw_Set_FogMin(float fogMin) {
    Env_ctl.fogparm.fl32[3] = fogMin;
}

/** Computes the VU1 depth-fog coefficients (VU1_PARMS.fog_d) from the fog distances and limits. */
void sh2gfw_Set_DepthFog(float fogfar, float fognear, float fmax, float fmin) {
    VU1_PARMS.fog_d.fl32[3] = fogfar * fognear * (fmin - fmax) / (fogfar - fognear);
    VU1_PARMS.fog_d.fl32[2] = (fogfar * fmax - fognear * fmin) / (fogfar - fognear);
    VU1_PARMS.fog_d.fl32[0] = (fmin - fmax) / (fognear - fogfar);
    VU1_PARMS.fog_d.fl32[1] = fmin - fognear * VU1_PARMS.fog_d.fl32[0];
    VU1_PARMS.phong_rgbamax.fl32[0] = 1.0f;
    VU1_PARMS.phong_rgbamax.fl32[1] = 0.3f;
}

/*
 * Matching: the arguments are loaded last to first, the way MWCC evaluates inline-call arguments:
 * the fog values were read through inline getters (names assumed, after the setters above).
 */
static inline float sh2gfw_Get_Fogfar(void) {
    return Env_ctl.fogparm.fl32[0];
}

static inline float sh2gfw_Get_FogNear(void) {
    return Env_ctl.fogparm.fl32[1];
}

static inline float sh2gfw_Get_FogMax(void) {
    return Env_ctl.fogparm.fl32[2];
}

static inline float sh2gfw_Get_FogMin(void) {
    return Env_ctl.fogparm.fl32[3];
}

/** Computes the VU1 depth-fog coefficients from the fog parameters in Env_ctl. */
void sh2gfw_Set_DepthFogFAFB(void) {
    sh2gfw_Set_DepthFog(sh2gfw_Get_Fogfar(), sh2gfw_Get_FogNear(), sh2gfw_Get_FogMax(), sh2gfw_Get_FogMin());
}

/** Sets the current room's spotLNum to -1 if bg is non-zero, else 0 (see sh2gde_CheckSpot_JmsOrBG). */
void sh2gde_SetSpot_JmsOrBG(int bg) {
    struct DrawEnvData *Now;

    Now = Get_NowDrawEnvData();
    if (bg) {
        Now->spotLNum = -1;
    } else {
        Now->spotLNum = 0;
    }
}

/**
 * Sets the current room's spotLNum from 0 back to -1 and re-enables the lens flare, except in
 * maps 0xD0005 and 0xD0009.
 */
void sh2gde_ResetSpot_Jms(void) {
    struct DrawEnvData *Now;

    Now = Get_NowDrawEnvData();
    /* Matching: the #line keeps the assert string below on the original's line. */
#line 733
    assert(Now!=NULL);
    if (Now->map_id_name == 0xD0005 || Now->map_id_name == 0xD0009) {
        return;
    }
    if (Now->spotLNum == 0) {
        Now->spotLNum = -1;
        light_flare_work[0].flare_inhibit_f = 0;
    }
}
