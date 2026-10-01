/*
 * Stencil-shadow filter (GFW): after the stencil shadows are drawn, copies the stencil buffer
 * into a work buffer, filters it by copying it back and forth between two work buffers, and
 * composites it onto the frame with the room's shadow color. Builds the GIF packets and the GS control chain.
 * The packet builders index pbuf[id + k] with id = 0 (each has an id local in the DWARF), as in
 * sh2gfw_2d_filters.
 */
#include "sh2.h"

union Q_WORDDATA STENCIL_workpacket[256];
static struct Shadow_Filter_Work ShadowFilterWork;

/**
 * Sets the shadow composite's alpha blend and test for night or day, and the shadow color.
 * @param NoD  night (non-zero) or day
 * @param arg1 not used
 * @param col  shadow color (RGBA)
 */
void sh2gfw_Set_FilterData(int NoD, int arg1, union Q_WORDDATA *col) {
    union Q_WORDDATA *alp;
    union Q_WORDDATA *tst;
    union Q_WORDDATA *alpd;
    union Q_WORDDATA *tstd;

    if (NoD) {
        alp = &ShadowFilterWork.GsAlpha;
        tst = &ShadowFilterWork.GsTest;
        alp->ul64[1] = 0x42;
        alp->ul64[0] = 0x8000000046;
        tst->ul64[1] = 0x47;
        tst->ul64[0] = 0x30002;
    } else {
        alpd = &ShadowFilterWork.GsAlpha;
        tstd = &ShadowFilterWork.GsTest;
        alpd->ul64[1] = 0x42;
        alpd->ul64[0] = 0x8000000052;
        tstd->ul64[1] = 0x47;
        tstd->ul64[0] = 0x30000;
    }
    ShadowFilterWork.ShadowColor = *col;
    ShadowFilterWork.factor = 1.0f;
}

/** Returns the shadow color scaled by the current factor (RGB from its red component). */
void *Get_ShadowColor(void) {
    ShadowFilterWork.ShadowNowColor.ui32[0] = ShadowFilterWork.ShadowNowColor.ui32[1] = ShadowFilterWork.ShadowNowColor.ui32[2] =
        ShadowFilterWork.ShadowColor.ui32[0] * ShadowFilterWork.factor;
    ShadowFilterWork.ShadowNowColor.ui32[3] = ShadowFilterWork.ShadowColor.ui32[3] * ShadowFilterWork.factor;
    return &ShadowFilterWork.ShadowNowColor;
}

/** Meant to set the shadow color factor; the clamped fc is discarded and the factor is always set to 1. */
void sh2gfw_Set_ShadowColorScaleFactor(float fc) {
    if (fc < 0.0f) {
        fc = 0.0f;
    } else if (fc * ShadowFilterWork.ShadowColor.ui32[3] > 255.0f) {
        fc = 255.0f / ShadowFilterWork.ShadowColor.ui32[3];
    }
    ShadowFilterWork.factor = 1.0f;
}

/**
 * Writes the GIF packet that composites the filtered shadow work buffer onto the frame with the
 * shadow color.
 * @return the end of the packet
 */
union Q_WORDDATA *shdw_Composite_Shadow(union Q_WORDDATA *pbuf) {
    int id;
    int tbp;

    tbp = sh2gfw_GetFilterFBP(&shGs_AllEnv, 1) << 5;
    id = 0;
    pbuf[id].ul128 = 0;
    pbuf[id].ui32[0] = 0x1000000A;
    pbuf[id + 1].ul64[0] = 0x1000000000000005;
    pbuf[id + 1].ul64[1] = 0xE;
    pbuf[id + 2].ul64[0] = 0;
    pbuf[id + 2].ul64[1] = 0x3F;
    pbuf[id + 3] = ShadowFilterWork.GsTest;
    pbuf[id + 4] = ShadowFilterWork.GsAlpha;
    pbuf[id + 5].ul64[1] = 6;
    pbuf[id + 5].ul64[0] = tbp | 0x620010000;
    pbuf[id + 6].ul64[1] = 0x14;
    pbuf[id + 6].ul64[0] = 0x60;
    pbuf[id + 7].ul64[0] = 0x702B400000008001;
    pbuf[id + 7].ul64[1] = 0xE512512;
    pbuf[id + 8].fl32[0] = 1.0f;
    pbuf[id + 8].fl32[1] = 1.0f;
    pbuf[id + 8].fl32[2] = 1.0f;
    pbuf[id + 8].fl32[3] = 0.0f;
    pbuf[id + 9] = *(union Q_WORDDATA *)Get_ShadowColor();
    pbuf[id + 10].ui32[0] = 0x7000;
    pbuf[id + 10].ui32[1] = 0x7000;
    pbuf[id + 10].ui32[2] = 0x10;
    pbuf[id + 10].ui32[3] = 0;
    pbuf[id + 11].fl32[0] = 0.0f;
    pbuf[id + 11].fl32[1] = 0.0f;
    pbuf[id + 11].fl32[2] = 1.0f;
    pbuf[id + 11].fl32[3] = 0.0f;
    pbuf[id + 12] = *(union Q_WORDDATA *)Get_ShadowColor();
    pbuf[id + 13].ui32[0] = 0x9000;
    pbuf[id + 13].ui32[1] = 0x9000;
    pbuf[id + 13].ui32[2] = 0x10;
    pbuf[id + 13].ui32[3] = 0;
    pbuf[id + 14].ul64[1] = 0x3F;
    pbuf[id + 14].ul64[0] = 0;
    pbuf[id].ui32[0] = 0x1000000E;
    pbuf[id].ui32[1] = 0;
    pbuf[id].ui32[2] = 0x11000000;
    pbuf[id].ui32[3] = 0x5000000E;
    pbuf[id + 15].ul128 = 0;
    pbuf[id + 15].ui32[0] = 0x60000000;
    return &pbuf[id + 16];
}

/**
 * Writes the GIF packet that fills the screen with col.
 * @param prim not used
 * @return the end of the packet
 */
union Q_WORDDATA *shdw_fill_Work(union Q_WORDDATA *col, unsigned int prim, union Q_WORDDATA *pbuf) {
    int id;

    id = 0;
    pbuf[id].ul128 = 0;
    pbuf[id].ui32[0] = 0x1000000B;
    pbuf[id + 1].ul64[0] = ((unsigned long)(prim | 6) << 47) | 0x5000400000008001;
    pbuf[id + 1].ul64[1] = 0x5151E;
    pbuf[id + 2].ul64[1] = 0x3F;
    pbuf[id + 2].ul64[0] = 0;
    pbuf[id + 3] = *col;
    pbuf[id + 4].ui32[0] = 0x7800;
    pbuf[id + 4].ui32[1] = 0x7800;
    pbuf[id + 4].ui32[2] = 0x10;
    pbuf[id + 4].ui32[3] = 0;
    pbuf[id + 5] = *col;
    pbuf[id + 6].ui32[0] = 0x8800;
    pbuf[id + 6].ui32[1] = 0x8800;
    pbuf[id + 6].ui32[2] = 0x10;
    pbuf[id + 6].ui32[3] = 0;
    pbuf[id].ui32[0] = 0x10000006;
    pbuf[id].ui32[1] = 0;
    pbuf[id].ui32[2] = 0x11000000;
    pbuf[id].ui32[3] = 0x50000006;
    pbuf[id + 7].ul128 = 0;
    pbuf[id + 7].ui32[0] = 0x60000000;
    return &pbuf[id + 8];
}

/** Writes the GIF packet that clears the work buffer. Returns the end of the packet. */
union Q_WORDDATA *shdw_clear(union Q_WORDDATA *pbuf) {
    int j;
    int id;

    id = 0;
    pbuf[id].ui32[0] = 0x10000004;
    pbuf[id].ui32[1] = 0;
    pbuf[id].ui32[2] = 0x11000000;
    pbuf[id].ui32[3] = 0x50000004;
    pbuf[id + 1].ul64[0] = 0x1000000000000003;
    pbuf[id + 1].ul64[1] = 0xE;
    pbuf[id + 2].ul64[1] = 0x3F;
    pbuf[id + 2].ul64[0] = 0;
    pbuf[id + 3].ul64[1] = 0x47;
    pbuf[id + 3].ul64[0] = 0x38002;
    pbuf[id + 4].ul64[1] = 0x42;
    pbuf[id + 4].ul64[0] = 0x42;
    pbuf[id + 5].ul64[0] = 0x5023400000008001;
    pbuf[id + 5].ul64[1] = 0xE5151;
    pbuf[id + 6].ui32[3] = 0x80;
    pbuf[id + 6].ui32[2] = 0x80;
    pbuf[id + 6].ui32[1] = 0x80;
    pbuf[id + 6].ui32[0] = 0x80;
    pbuf[id + 7].ui32[0] = 0x7000;
    pbuf[id + 7].ui32[1] = 0x7000;
    pbuf[id + 7].ui32[2] = 0x10;
    pbuf[id + 7].ui32[3] = 0;
    pbuf[id + 8].ui32[3] = 0x80;
    pbuf[id + 8].ui32[2] = 0x80;
    pbuf[id + 8].ui32[1] = 0x80;
    pbuf[id + 8].ui32[0] = 0x80;
    pbuf[id + 9].ui32[0] = 0x9000;
    pbuf[id + 9].ui32[1] = 0x9000;
    pbuf[id + 9].ui32[2] = 0x10;
    pbuf[id + 9].ui32[3] = 0;
    pbuf[id + 10].ul64[1] = 0x3F;
    pbuf[id + 10].ul64[0] = 0;
    pbuf[id].ui32[0] = 0x1000000A;
    pbuf[id].ui32[1] = 0;
    pbuf[id].ui32[2] = 0x11000000;
    pbuf[id].ui32[3] = 0x5000000A;
    pbuf[id + 11].ul128 = 0;
    pbuf[id + 11].ui32[0] = 0x60000000;
    return &pbuf[id + 12];
}

/**
 * Writes the GIF packet that copies the stencil buffer (texture base tbp) into the work buffer.
 * @return the end of the packet
 */
union Q_WORDDATA *shdw_StencilBuf2Work(unsigned int tbp, unsigned int prim, union Q_WORDDATA *pbuf) {
    int id;

    id = 0;
    pbuf[id].ul128 = 0;
    pbuf[id].ui32[0] = 0x1000000A;
    pbuf[id + 1].ul64[0] = 0x1000000000000003;
    pbuf[id + 1].ul64[1] = 0xE;
    pbuf[id + 2].ul64[0] = 0;
    pbuf[id + 2].ul64[1] = 0x3F;
    pbuf[id + 3].ul64[1] = 6;
    pbuf[id + 3].ul64[0] = (unsigned long)tbp | 0xE64A20000;
    pbuf[id + 4].ul64[1] = 0x14;
    pbuf[id + 4].ul64[0] = 0;
    pbuf[id + 5].ul64[0] = ((unsigned long)(prim | 0x16) << 47) | 0x7000400000008001;
    pbuf[id + 5].ul64[1] = 0xE512512;
    pbuf[id + 6].fl32[0] = 0.0f;
    pbuf[id + 6].fl32[1] = 0.0f;
    pbuf[id + 6].fl32[2] = 1.0f;
    pbuf[id + 6].fl32[3] = 0.0f;
    pbuf[id + 7].ui32[3] = 0x80;
    pbuf[id + 7].ui32[2] = 0x80;
    pbuf[id + 7].ui32[1] = 0x80;
    pbuf[id + 7].ui32[0] = 0x80;
    pbuf[id + 8].ui32[0] = 0x7800;
    pbuf[id + 8].ui32[1] = 0x7800;
    pbuf[id + 8].ui32[2] = 0x10;
    pbuf[id + 8].ui32[3] = 0;
    pbuf[id + 9].fl32[0] = 1.0f;
    pbuf[id + 9].fl32[1] = 1.0f;
    pbuf[id + 9].fl32[2] = 1.0f;
    pbuf[id + 9].fl32[3] = 0.0f;
    pbuf[id + 10].ui32[3] = 0x80;
    pbuf[id + 10].ui32[2] = 0x80;
    pbuf[id + 10].ui32[1] = 0x80;
    pbuf[id + 10].ui32[0] = 0x80;
    pbuf[id + 11].ui32[0] = 0x8800;
    pbuf[id + 11].ui32[1] = 0x8800;
    pbuf[id + 11].ui32[2] = 0x10;
    pbuf[id + 11].ui32[3] = 0;
    pbuf[id + 12].ul64[0] = 0;
    pbuf[id + 12].ul64[1] = 0x3F;
    pbuf[id].ui32[0] = 0x1000000C;
    pbuf[id].ui32[1] = 0;
    pbuf[id].ui32[2] = 0x11000000;
    pbuf[id].ui32[3] = 0x5000000C;
    pbuf[id + 13].ul128 = 0;
    pbuf[id + 13].ui32[0] = 0x60000000;
    return &pbuf[id + 14];
}

/**
 * Writes the GIF packet of one filter pass: draws the work buffer at texture base tbp as a
 * sprite into the current target.
 * @return the end of the packet
 */
union Q_WORDDATA *shdw_Work2Work(unsigned int tbp, union Q_WORDDATA *pbuf) {
    int id;

    id = 0;
    pbuf[id].ul128 = 0;
    pbuf[id].ui32[0] = 0x1000000A;
    pbuf[id + 1].ul64[0] = 0x1000000000000003;
    pbuf[id + 1].ul64[1] = 0xE;
    pbuf[id + 2].ul64[0] = 0;
    pbuf[id + 2].ul64[1] = 0x3F;
    pbuf[id + 3].ul64[1] = 6;
    pbuf[id + 3].ul64[0] = (unsigned long)tbp | 0xE20010000;
    pbuf[id + 4].ul64[1] = 0x14;
    pbuf[id + 4].ul64[0] = 0x60;
    pbuf[id + 5].ul64[0] = 0x700B400000008001;
    pbuf[id + 5].ul64[1] = 0xE512512;
    pbuf[id + 6].fl32[0] = 1.0f;
    pbuf[id + 6].fl32[1] = 1.0f;
    pbuf[id + 6].fl32[2] = 1.0f;
    pbuf[id + 6].fl32[3] = 0.0f;
    pbuf[id + 7].ui32[3] = 0x80;
    pbuf[id + 7].ui32[2] = 0x80;
    pbuf[id + 7].ui32[1] = 0x80;
    pbuf[id + 7].ui32[0] = 0x80;
    pbuf[id + 8].ui32[0] = 0x7800;
    pbuf[id + 8].ui32[1] = 0x7800;
    pbuf[id + 8].ui32[2] = 0x10;
    pbuf[id + 8].ui32[3] = 0;
    pbuf[id + 9].fl32[0] = 0.0f;
    pbuf[id + 9].fl32[1] = 0.0f;
    pbuf[id + 9].fl32[2] = 1.0f;
    pbuf[id + 9].fl32[3] = 0.0f;
    pbuf[id + 10].ui32[3] = 0x80;
    pbuf[id + 10].ui32[2] = 0x80;
    pbuf[id + 10].ui32[1] = 0x80;
    pbuf[id + 10].ui32[0] = 0x80;
    pbuf[id + 11].ui32[0] = 0x8800;
    pbuf[id + 11].ui32[1] = 0x8800;
    pbuf[id + 11].ui32[2] = 0x10;
    pbuf[id + 11].ui32[3] = 0;
    pbuf[id + 12].ul64[0] = 0;
    pbuf[id + 12].ul64[1] = 0x3F;
    pbuf[id].ui32[0] = 0x1000000C;
    pbuf[id].ui32[1] = 0;
    pbuf[id].ui32[2] = 0x11000000;
    pbuf[id].ui32[3] = 0x5000000C;
    pbuf[id + 13].ul128 = 0;
    pbuf[id + 13].ui32[0] = 0x60000000;
    return &pbuf[id + 14];
}

/**
 * Runs the shadow filter: builds the packets and chains the stencil copy, filter passes and
 * composite into the GS control chain, then sends it.
 * @param arg0 not used
 */
void shdw_shadow_filter_main(int arg0) {
    unsigned int id;
    union Q_WORDDATA *sqt_clear;
    union Q_WORDDATA *filw;
    union Q_WORDDATA *sb2w;
    union Q_WORDDATA *w2w0;
    union Q_WORDDATA *w2w1;
    union Q_WORDDATA *kcs;
    unsigned int tbp1;

    id = sh2gfw_GetFilterFBP(&shGs_AllEnv, 0) << 5;
    tbp1 = sh2gfw_GetFilterFBP(&shGs_AllEnv, 1) << 5;
    sqt_clear = STENCIL_workpacket;
    sb2w = shdw_clear(sqt_clear);
    w2w0 = shdw_StencilBuf2Work(sh2gfw_GetStencilFBP(&shGs_AllEnv) << 5, 0x40, sb2w);
    w2w1 = shdw_Work2Work(id, w2w0);
    filw = shdw_Work2Work(tbp1, w2w1);
    kcs = shdw_fill_Work(&Env_ctl.compo_Fill_col, 0, filw);
    shdw_Composite_Shadow(kcs);
    sh2gfw_setCALL_gsctl(sqt_clear);
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_StartShadowFilter(&shGs_AllEnv, 0);
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_setREF_gsctl(shGs_AllEnv.GsReg_TEXA_A);
    sh2gfw_setREF_gsctl(shGs_AllEnv.GsReg_TEST_B);
    sh2gfw_setREF_gsctl(shGs_AllEnv.GsReg_ALPHA_D);
    sh2gfw_setCALL_gsctl(sb2w);
    sh2gfw_ChangeMask_ShadowFilter(&shGs_AllEnv, 1, 0);
    sh2gfw_setREF_gsctl(shGs_AllEnv.GsReg_TEST_C);
    sh2gfw_setCALL_gsctl(filw);
    sh2gfw_setREF_gsctl(shGs_AllEnv.GsReg_TEST_B);
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_StartShadowFilter(&shGs_AllEnv, 1);
    sh2gfw_setCALL_gsctl(w2w0);
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_StartShadowFilter(&shGs_AllEnv, 0);
    sh2gfw_setCALL_gsctl(w2w1);
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_StartShadowFilter(&shGs_AllEnv, 1);
    sh2gfw_setCALL_gsctl(w2w0);
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_ReturnTBuff(&shGs_AllEnv);
    sh2gfw_setREF_gsctl(shGs_AllEnv.GsReg_TEST_B);
    sh2gfw_setREF_gsctl(shGs_AllEnv.GsReg_ZBUF_B);
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_setREF_gsctl(&shGs_AllEnv.DrawEnv[shGs_AllEnv.loop3].gifad_frame_mskalpha);
    sh2gfw_setCALL_gsctl(kcs);
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_setREF_gsctl(shGs_AllEnv.DefaultEnv);
    d1cSend(sh2gfw_setEND_gsctl());
}
