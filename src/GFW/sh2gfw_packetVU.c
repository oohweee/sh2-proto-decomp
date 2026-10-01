/*
 * Background VU1 microprogram selection (GFW): the table of VU1 microprograms (entry, BASE,
 * OFFSET, XGKICK addresses per program), choosing a program from the light mode, blending and
 * shading of a geometry block, and the VIF codes that switch programs in the packet stream.
 */
#include "sh2.h"
#include "sdk/libvu0.h"

struct sh2gfw_VU1_AllMicroMAN AllMicro_Man = {
    NULL,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    {
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0x2, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x4, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x4, 0x32, 0x68, 0x13C, 0x78, 0x1C4, 0, 0 },
        { 0x8, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0xA, 0, 0, 0, 0, 0, 0, 0 },
        { 0xC, 0, 0, 0, 0, 0, 0, 0 },
        { 0xE, 0, 0, 0, 0, 0, 0, 0 },
        { 0x10, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x12, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x14, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x16, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x18, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x1A, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x1C, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x1E, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x20, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x22, 0x2E, 0x6C, 0x109, 0x78, 0x1C4, 0, 0 },
        { 0x24, 0x2E, 0x6C, 0x109, 0x78, 0x1C4, 0, 0 },
        { 0x26, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x28, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0x2A, 0x50, 0xA4, 0, 0x78, 0x1C4, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
};

short GifVu2MicExecAddr[2][8][16] = {
    {
        { 0x2, 0x2, 0x22, 0x2, 0x18, 0x2, 0x2, 0x2, 0x12, 0x1E, 0, 0, 0, 0, 0, 0 },
        { 0x14, 0x14, 0x22, 0x14, 0x14, 0x14, 0x14, 0x14, 0x2A, 0x26, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0x2, 0x2, 0x22, 0x2, 0x2, 0x2, 0x2, 0x2, 0x12, 0x1E, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0x22, 0x1A, 0x22, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x12, 0x1A, 0, 0, 0, 0, 0, 0 },
        { 0x14, 0x14, 0x22, 0x14, 0x14, 0x14, 0x14, 0x14, 0x12, 0x14, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0x8, 0x8, 0x24, 0x8, 0x10, 0x8, 0x8, 0x8, 0x12, 0x20, 0, 0, 0, 0, 0, 0 },
        { 0x16, 0x16, 0x24, 0x16, 0x16, 0x16, 0x16, 0x16, 0x2A, 0x28, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0x8, 0x8, 0x24, 0x8, 0x8, 0x8, 0x8, 0x8, 0x12, 0x20, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0x24, 0x1C, 0x24, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x12, 0x1C, 0, 0, 0, 0, 0, 0 },
        { 0x16, 0x16, 0x24, 0x16, 0x16, 0x16, 0x16, 0x16, 0x12, 0x16, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },
};

short Vuchange_LightTable[32][2] = {
    { 0, 0 },
    { 0x2, 0x8 },
    { 0x4, 0x8 },
    { 0x6, 0x8 },
    { 0x2, 0x8 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0x18, 0x10 },
    { 0x12, 0x12 },
    { 0x14, 0x16 },
    { 0x14, 0x16 },
    { 0x18, 0x10 },
    { 0x1A, 0x1C },
    { 0x1A, 0x1C },
    { 0x1E, 0x20 },
    { 0x1E, 0x20 },
    { 0x22, 0x24 },
    { 0x22, 0x24 },
    { 0x26, 0x28 },
    { 0x26, 0x28 },
    { 0x2A, 0x2A },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
};

/** Returns the VIF BASE value of the microprogram entered at execaddr. */
int sh2gfw_get_VuBase(int execaddr) {
    return AllMicro_Man.MF_data[execaddr >> 1].base;
}

/** Returns the VIF OFFSET value of the microprogram entered at execaddr. */
int sh2gfw_get_VuOffset(int execaddr) {
    return AllMicro_Man.MF_data[execaddr >> 1].offset;
}

/** Forgets the last microprogram used (see sh2gfw_Check_VuCounter). */
void sh2gfw_Init_VuCounter(void) {
    AllMicro_Man.BeforeVu = -1;
}

/** Returns 1 if microprogram vc was also the last one used; otherwise records it and returns 0. */
int sh2gfw_Check_VuCounter(int vc) {
    if (AllMicro_Man.BeforeVu == vc) {
        return 1;
    }
    AllMicro_Man.BeforeVu = vc;
    return 0;
}

/** Returns the first XGKICK buffer address of the microprogram entered at execaddr. */
int sh2gfw_GetVuKick_Addr(int execaddr) {
    return AllMicro_Man.MF_data[execaddr >> 1].kickoffset;
}

/** Returns the second XGKICK buffer address of the microprogram entered at execaddr. */
int sh2gfw_GetVuKick_Addr2(int execaddr) {
    return AllMicro_Man.MF_data[execaddr >> 1].kickoffset2;
}

/** Initializes the VU1 parameter block: identity matrices, zero color minimum, the frame-mask GIF tags. */
void sh2gfw_init_VU_PARAMS(struct sh2gfw_VU_PARMS *VU_PARMS) {
    float unit[4][4] __attribute__((aligned(16)));
    float zero[4][4] __attribute__((aligned(16)));

    sceVu0UnitMatrix(zero);
    zero[0][0] = 0.0f;
    zero[1][1] = 0.0f;
    zero[2][2] = 0.0f;
    zero[3][3] = 0.0f;
    sceVu0UnitMatrix(unit);
    sceVu0CopyMatrix(VU_PARMS->world_screen, unit);
    sceVu0CopyMatrix(VU_PARMS->world_clip, unit);
    sceVu0CopyMatrix(VU_PARMS->clip_screen, unit);
    sceVu0CopyMatrix((float (*)[4])&VU_PARMS->fog_d, unit);
    VU_PARMS->rgbamin.ui32[0] = 0;
    VU_PARMS->rgbamin.ui32[1] = 0;
    VU_PARMS->rgbamin.ui32[2] = 0;
    VU_PARMS->GifTag_mskRGB.ui32[3] = 0;
    VU_PARMS->GifTag_mskRGB.ui32[2] = 0xE;
    VU_PARMS->GifTag_mskRGB.ui32[1] = 0x10000000;
    VU_PARMS->GifTag_mskRGB.ui32[0] = 0x8001;
    VU_PARMS->GifTag_mskALPHA.ui32[3] = 0;
    VU_PARMS->GifTag_mskALPHA.ui32[2] = 0xE;
    VU_PARMS->GifTag_mskALPHA.ui32[1] = 0x10000000;
    VU_PARMS->GifTag_mskALPHA.ui32[0] = 0x8001;
    VU_PARMS->GifTag_mskNORMAL.ui32[3] = 0;
    VU_PARMS->GifTag_mskNORMAL.ui32[2] = 0xE;
    VU_PARMS->GifTag_mskNORMAL.ui32[1] = 0x10000000;
    VU_PARMS->GifTag_mskNORMAL.ui32[0] = 0x8001;
    VU_PARMS->GifTag_mskZ.ul128 = VU_PARMS->GifTag_mskRGB.ul128;
    VU_PARMS->GifTag_unmskZ.ul128 = VU_PARMS->GifTag_mskRGB.ul128;
}

/** Meant to clear a block's light data (19 quadwords); as written it clears the first quadword 19 times. */
void sh2gfw_allinit_lightenv(struct sh2gfw_LIGHT_ENV *slv) {
    unsigned int i;

    for (i = 0; i < 19; i++) {
        *(u_long128 *)slv = 0;
    }
}

/** Copies the current frame-mask and Z-buffer register values into the VU1 parameter block. */
void sh2gfw_set_GSREGS2Vu(struct sh2gfw_VU_PARMS *vupam) {
    vupam->Frame_mskRGB = shGs_AllEnv.Now_DrawEnv.frame_mskRGB;
    vupam->Frame_mskALPHA = shGs_AllEnv.Now_DrawEnv.frame_mskalpha;
    vupam->Frame_mskNORMAL = shGs_AllEnv.Now_DrawEnv.frame_normal;
    vupam->mskZ = shGs_AllEnv.GsReg_ZBUF_B[1];
    vupam->unmskZ = shGs_AllEnv.GsReg_ZBUF_A[1];
}

/**
 * Chooses the microprogram for a geometry block and writes the VIF codes that select it:
 * BASE, OFFSET and MSCAL, preceded by an ITOP when it is the same program as the last block,
 * else by a FLUSH.
 * @param pT    the block's texture data (for its TFX)
 * @param pGH   the GIF tag header (alpha blend mode)
 * @param vuh   the VU header (shading kind)
 * @param exec  receives the microprogram entry address
 * @param ppqwd write pointer, advanced past the two quadwords
 * @return the microprogram's maximum batch size in vertices
 */
int sh2gfw_set_VuSend(void *pT, struct sh2gfw_GIFTAG_HEAD *pGH, struct sh2gfw_VU_HEAD *vuh, int *exec, union Q_WORDDATA **ppqwd) {
    union Q_WORDDATA *b_pack;
    int tfx;
    unsigned int execaddr;
    unsigned int bpack;
    unsigned int basead;
    unsigned int offstad;

    b_pack = *ppqwd;
    tfx = sh2gfw_Get_TFX(pT, pGH->id);
    execaddr = sh2gfw_get_VUmode(Env_ctl.light_mode, tfx, pGH, vuh);
    basead = sh2gfw_get_VuBase(execaddr);
    offstad = sh2gfw_get_VuOffset(execaddr);
    if (sh2gfw_Check_VuCounter(execaddr)) {
        b_pack[0].ui32[0] = (execaddr << 16) | 0x10000000;
        b_pack[0].ui32[2] = execaddr | 0x04000000;
        b_pack[0].ui32[3] = basead | 0x03000000;
        b_pack[0].ui32[2] |= ((pGH->abe & 0xF) << 20) | ((tfx & 0xF) << 16);
    } else {
        b_pack[0].ui32[0] = (execaddr << 16) | 0x10000000;
        b_pack[0].ui32[2] = 0x11000000;
        b_pack[0].ui32[3] = basead | 0x03000000;
        b_pack[0].ui32[2] |= ((pGH->abe & 0xF) << 20) | ((tfx & 0xF) << 16);
    }
    b_pack[1].ui32[0] = 0x10000000;
    b_pack[1].ui32[1] = 0;
    b_pack[1].ui32[2] = offstad | 0x02000000;
    b_pack[1].ui32[3] = execaddr | 0x14000000;
    *ppqwd = b_pack + 2;
    *exec = execaddr;
    /* Matching: execaddr is unsigned (DWARF), the shift here is signed (sra) */
    return AllMicro_Man.MF_data[(int)execaddr >> 1].tslengmax;
}

/**
 * Returns the microprogram entry for a light mode, alpha blend mode and shading kind (from
 * GifVu2MicExecAddr); TFX 2 and blend mode 5 force the shading kind.
 */
int sh2gfw_get_VUmode(int light_mode, int tfx, struct sh2gfw_GIFTAG_HEAD *pGIF_H, struct sh2gfw_VU_HEAD *pVU_H) {
    int execaddr;

    if (tfx == 2) {
        pVU_H->vukind = 4;
    }
    if (pVU_H->vukind == 3) {
        verbose(1, "EnvMap!\n");
    } else if (pGIF_H->abe == 5) {
        verbose(1, "BlinnEV!\n");
        pVU_H->vukind = 2;
    } else if (pVU_H->vukind == 2) {
        verbose(1, "M-Blinn!\n");
    }
    execaddr = GifVu2MicExecAddr[light_mode - 1][pGIF_H->abe][pVU_H->vukind];
    return execaddr;
}

/**
 * Returns the microprogram that replaces vuid in light mode light_mode (Vuchange_LightTable),
 * and vuid's BASE and OFFSET values.
 */
int sh2gfw_Change_VUmode(unsigned int light_mode, unsigned int vuid, unsigned int *baseaddr, unsigned int *offsetaddr) {
    int execaddr;
    int id;

    id = vuid >> 1;
    execaddr = Vuchange_LightTable[id][light_mode - 1U];
    *baseaddr = sh2gfw_get_VuBase(vuid);
    *offsetaddr = sh2gfw_get_VuOffset(vuid);
    return execaddr;
}
