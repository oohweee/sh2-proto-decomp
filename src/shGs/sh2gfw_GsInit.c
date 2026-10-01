/*
 * GS set-up: the three display/draw environments used for triple buffering, the stencil
 * (shadow) buffers and shadow-filter work pages, and the default register packet. Local
 * variants of libgraph's sceGsSetDefDispEnv/sceGsSetDefDrawEnv fill the environments.
 */

#include "sh2.h"
#include "sdk/libgraph.h"

/**
 * Sets up the three display/draw environments (with and without clear) and their FRAME
 * register variants.
 * @param draw_psm, disp_psm pixel formats. @param w, h screen size. @param ztest Z test mode
 *        (0: none). @param zpsm Z buffer format.
 */
void shGs_InitDefTBuff(struct shGsAllEnv *shGsEnv, short draw_psm, short disp_psm, short w, short h, short ztest,
                       short zpsm) {
    unsigned int i;

    for (i = 0; i < 3; i++) {
        shGsSetDefTBuffDispEnv(&shGsEnv->DispEnv[i], disp_psm, w, h, 0, 0);
    }
    for (i = 0; i < 3; i++) {
        shGsSetDefTBuffDrawEnv(&shGsEnv->DrawEnv[i].draw, draw_psm, w, h, ztest, zpsm);
    }
    for (i = 0; i < 3; i++) {
        sceGsSetDefClear(&shGsEnv->DrawEnv[i].clear, ztest, 2048 - (w >> 1), 2048 - (h >> 1), w, h, 0, 0, 0, 0, 0);
    }
    for (i = 0; i < 3; i++) {
        *(u_long128 *)&shGsEnv->DrawEnv[i].giftag = 0;
        shGsEnv->DrawEnv[i].giftag.NLOOP = 19;
        shGsEnv->DrawEnv[i].giftag.EOP = 1;
        shGsEnv->DrawEnv[i].giftag.NREG = 1;
        shGsEnv->DrawEnv[i].giftag.REGS0 = 0xE;
        shGsEnv->DrawEnv[i].giftag_nc = shGsEnv->DrawEnv[i].giftag;
        shGsEnv->DrawEnv[i].giftag_nc.NLOOP = 13;
        shGsEnv->DrawEnv[i].draw_nc = shGsEnv->DrawEnv[i].draw;
    }
    for (i = 0; i < 3; i++) {
        shGsEnv->DispEnv[i].dispfb.FBP = shGsEnv->LoopEnv.GsNowDispFBPs[i];
        shGsEnv->DrawEnv[i].draw.frame1.FBP = shGsEnv->LoopEnv.GsDrawFBPs[i];
        shGsEnv->DrawEnv[i].draw_nc.frame1.FBP = shGsEnv->LoopEnv.GsDrawFBPs[i];

        shGsEnv->DrawEnv[i].gifad_frame_normal.ui32[3] = 0;
        shGsEnv->DrawEnv[i].gifad_frame_normal.ui32[2] = 0xE;
        shGsEnv->DrawEnv[i].gifad_frame_normal.ui32[1] = 0x10000000;
        shGsEnv->DrawEnv[i].gifad_frame_normal.ui32[0] = 0x8001;
        shGsEnv->DrawEnv[i].frame_normal.ui32[0] = shGsEnv->DrawEnv[i].draw.frame1.PSM << 24 |
                                                   shGsEnv->DrawEnv[i].draw.frame1.FBW << 16 |
                                                   shGsEnv->DrawEnv[i].draw.frame1.FBP;
        shGsEnv->DrawEnv[i].frame_normal.ui32[1] = 0;
        shGsEnv->DrawEnv[i].frame_normal.ul64[1] = 0x4C;

        shGsEnv->DrawEnv[i].gifad_frame_mskalpha.ul128 = shGsEnv->DrawEnv[i].gifad_frame_normal.ul128;
        shGsEnv->DrawEnv[i].frame_mskalpha.ui32[0] = shGsEnv->DrawEnv[i].draw.frame1.PSM << 24 |
                                                     shGsEnv->DrawEnv[i].draw.frame1.FBW << 16 |
                                                     shGsEnv->DrawEnv[i].draw.frame1.FBP;
        shGsEnv->DrawEnv[i].frame_mskalpha.ui32[1] = 0xFF000000;
        shGsEnv->DrawEnv[i].frame_mskalpha.ul64[1] = 0x4C;

        shGsEnv->DrawEnv[i].gifad_frame_mskDalpha.ul128 = shGsEnv->DrawEnv[i].gifad_frame_normal.ul128;
        shGsEnv->DrawEnv[i].frame_mskDalpha.ui32[0] = shGsEnv->DrawEnv[i].draw.frame1.PSM << 24 |
                                                      shGsEnv->DrawEnv[i].draw.frame1.FBW << 16 |
                                                      shGsEnv->DrawEnv[i].draw.frame1.FBP;
        shGsEnv->DrawEnv[i].frame_mskDalpha.ui32[1] = 0x80000000;
        shGsEnv->DrawEnv[i].frame_mskDalpha.ul64[1] = 0x4C;

        shGsEnv->DrawEnv[i].gifad_frame_mskRGB.ul128 = shGsEnv->DrawEnv[i].gifad_frame_normal.ul128;
        shGsEnv->DrawEnv[i].frame_mskRGB.ui32[0] = shGsEnv->DrawEnv[i].draw.frame1.PSM << 24 |
                                                   shGsEnv->DrawEnv[i].draw.frame1.FBW << 16 |
                                                   shGsEnv->DrawEnv[i].draw.frame1.FBP;
        shGsEnv->DrawEnv[i].frame_mskRGB.ui32[1] = 0xFFFFFF;
        shGsEnv->DrawEnv[i].frame_mskRGB.ul64[1] = 0x4C;
    }
    for (i = 0; i < 3; i++) {
        shGsEnv->DrawEnv[i].drawq2[0].ui32[0] = draw_psm << 24 | (w / 64) << 16 | shGsEnv->LoopEnv.GsDrawFBPs[i];
        shGsEnv->DrawEnv[i].drawq2[0].ui32[1] = 0;
        shGsEnv->DrawEnv[i].drawq2[0].ul64[1] = 0x4D;
        shGsEnv->DrawEnv[i].drawq2[1].ul64[1] = 0x4F;
        shGsEnv->DrawEnv[i].drawq2[1].ul64[0] = GS_SET_ZBUF(0x1C0, zpsm & 0xF, 1);
        shGsEnv->DrawEnv[i].drawq2[2].ul64[1] = 0x19;
        shGsEnv->DrawEnv[i].drawq2[2].ul64[0] = *(unsigned long *)&shGsEnv->DrawEnv[i].draw.xyoffset1;
        shGsEnv->DrawEnv[i].drawq2[3].ul64[1] = 0x41;
        shGsEnv->DrawEnv[i].drawq2[3].ul64[0] = *(unsigned long *)&shGsEnv->DrawEnv[i].draw.scissor1;
        shGsEnv->DrawEnv[i].drawq2[4].ul64[1] = 0x48;
        shGsEnv->DrawEnv[i].drawq2[4].ul64[0] = *(unsigned long *)&shGsEnv->DrawEnv[i].draw.test1;
        shGsEnv->DrawEnv[i].drawq2_nc[0].ul128 = shGsEnv->DrawEnv[i].drawq2[0].ul128;
        shGsEnv->DrawEnv[i].drawq2_nc[1].ul128 = shGsEnv->DrawEnv[i].drawq2[1].ul128;
        shGsEnv->DrawEnv[i].drawq2_nc[2].ul128 = shGsEnv->DrawEnv[i].drawq2[2].ul128;
        shGsEnv->DrawEnv[i].drawq2_nc[3].ul128 = shGsEnv->DrawEnv[i].drawq2[3].ul128;
        shGsEnv->DrawEnv[i].drawq2_nc[4].ul128 = shGsEnv->DrawEnv[i].drawq2[4].ul128;
    }
}

/** Moves the display area of all three buffers by (@p xx, @p yy). */
void shGs_TrimDispArea(int xx, int yy) {
    int i;

    for (i = 0; i < 3; i++) {
        shGs_AllEnv.DispEnv[i].dispfb.DBX += xx;
        shGs_AllEnv.DispEnv[i].dispfb.DBY += yy;
    }
}

/** Resets the display area of all three buffers to (0, 32). */
void shGs_SetDefaultDispArea(void) {
    int i;

    for (i = 0; i < 3; i++) {
        shGs_AllEnv.DispEnv[i].dispfb.DBX = 0;
        shGs_AllEnv.DispEnv[i].dispfb.DBY = 32;
    }
}

/**
 * Fills a display environment (like sceGsSetDefDispEnv; NTSC only).
 * @param psm pixel format. @param w, h size (w a multiple of 64). @param dx, dy display offset.
 */
void shGsSetDefTBuffDispEnv(struct sceGsDispEnv *disp, short psm, short w, short h, short dx, short dy) {
    if (w % 64) {
        printf("sceGsSetDefDispEnv:The value of w is incorrect\n");
        return;
    }
    *(unsigned long *)&disp->pmode = GS_SET_PMODE(0, 1, 1, 1, 0, 0);
    if (sceGsGetGParam()->interlace) {
        if (sceGsGetGParam()->field_mode) {
            *(unsigned long *)&disp->smode2 = GS_SET_SMODE2(1, 1, 0);
        } else {
            *(unsigned long *)&disp->smode2 = GS_SET_SMODE2(1, 0, 0);
        }
    } else {
        *(unsigned long *)&disp->smode2 = GS_SET_SMODE2(0, 1, 0);
    }
    if (sceGsGetGParam()->out_mode == 2) {
        *(unsigned long *)&disp->dispfb = ((unsigned long)psm & 0xF) << 15 | (unsigned long)((w >> 6) & 0x3F) << 9;
        disp->dispfb.DBY = 32;
    } else {
        *(unsigned long *)&disp->dispfb = ((unsigned long)psm & 0xF) << 15 | (unsigned long)((w >> 6) & 0x3F) << 9;
        disp->dispfb.DBY = 0;
    }
    if (sceGsGetGParam()->out_mode == 2) {
        if (sceGsGetGParam()->interlace == 1) {
            *(unsigned long *)&disp->display =
                (unsigned long)((h - 64) * 2 - 1) << 44 | (unsigned long)((2559 + w) / w - 1) << 23 |
                ((unsigned long)(dx * (2560 / w)) + 652) & 0xFFF | (unsigned long)((dy + 50) & 0xFFF) << 12 |
                (unsigned long)2559 << 32;
        } else {
            *(unsigned long *)&disp->display =
                (unsigned long)(h - 1) << 44 | (unsigned long)((2559 + w) / w - 1) << 23 |
                ((unsigned long)(dx * (2560 / w)) + 652) & 0xFFF | (unsigned long)((dy + 25) & 0xFFF) << 12 |
                (unsigned long)2559 << 32;
        }
    } else {
        printf("sceGsDefDispEnv:Not support displaymode except for NTSC yet!!\n");
    }
    *(unsigned long *)&disp->bgcolor = 0;
}

/**
 * Fills a draw environment (like sceGsSetDefDrawEnv) with the frame base left at 0.
 * @param psm pixel format. @param w, h size. @param ztest Z test mode (0: none).
 * @param zpsm Z buffer format. @return the number of register settings (8).
 */
int shGsSetDefTBuffDrawEnv(struct sceGsDrawEnv1 *draw, short psm, short w, short h, short ztest, short zpsm) {
    unsigned int zb;

    draw->frame1addr = 0x4C;
    *(unsigned long *)&draw->frame1 = GS_SET_FRAME(0, (w >> 6) & 0x3F, psm & 0xF, 0);
    draw->zbuf1addr = 0x4E;
    zb = 512 - ((zpsm & 2) ? 64 : 128);
    if (ztest == 0) {
        *(unsigned long *)&draw->zbuf1 = GS_SET_ZBUF(zb, zpsm & 0xF, 1);
    } else {
        *(unsigned long *)&draw->zbuf1 = GS_SET_ZBUF(zb, zpsm & 0xF, 0);
    }
    draw->xyoffset1addr = 0x18;
    *(unsigned long *)&draw->xyoffset1 = GS_SET_XYOFFSET((2048 - (long)(w >> 1)) << 4, (2048 - (long)(h >> 1)) << 4);
    draw->scissor1addr = 0x40;
    *(unsigned long *)&draw->scissor1 = GS_SET_SCISSOR(0, w - 1, 0, h - 1);
    draw->prmodecontaddr = 0x1A;
    draw->prmodecont.AC = 1;
    draw->colclampaddr = 0x46;
    draw->colclamp.CLAMP = 1;
    draw->dtheaddr = 0x45;
    if (psm & 2) {
        draw->dthe.DTHE = 1;
    } else {
        draw->dthe.DTHE = 0;
    }
    draw->test1addr = 0x47;
    if (ztest) {
        *(unsigned long *)&draw->test1 = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ztest & 3);
    } else {
        *(unsigned long *)&draw->test1 = 0;
    }
    asm volatile("sync.l");
    return 8;
}

/** Sets up the three stencil (shadow) buffer environments. @param w, h size. @param ztest, zpsm Z settings. */
void shGs_InitGsStencilBuff(struct shGsAllEnv *shGsEnv, short w, short h, short ztest, short zpsm) {
    unsigned int i;

    for (i = 0; i < 3; i++) {
        shGs_InitStencilDrawEnv(&shGsEnv->StencilBuf[i], w, h, ztest, zpsm);
    }
    for (i = 0; i < 3; i++) {
        sceGsSetDefClear(&shGsEnv->StencilBuf[i].clear, ztest, 2048 - (w >> 1), 2048 - (h >> 1), w, h, 0x80, 0x80,
                         0x80, 0, 0);
    }
    for (i = 0; i < 3; i++) {
        *(u_long128 *)&shGsEnv->StencilBuf[i].giftag = 0;
        shGsEnv->StencilBuf[i].giftag.NLOOP = 22;
        shGsEnv->StencilBuf[i].giftag.EOP = 1;
        shGsEnv->StencilBuf[i].giftag.NREG = 1;
        shGsEnv->StencilBuf[i].giftag.REGS0 = 0xE;
        shGsEnv->StencilBuf[i].giftag_nc = shGsEnv->StencilBuf[i].giftag;
        shGsEnv->StencilBuf[i].giftag_nc.NLOOP = 15;
    }
    for (i = 0; i < 3; i++) {
        shGsEnv->StencilBuf[i].draw.frame1.FBP = shGsEnv->LoopEnv.GsShadowFBP[i];
        shGsEnv->StencilBuf[i].draw_nc.frame1.FBP = shGsEnv->LoopEnv.GsShadowFBP[i];
        shGsEnv->StencilBuf[i].frame.ui32[0] = (w / 64) << 16 | 0xA << 24 | shGsEnv->LoopEnv.GsShadowFBP[i];
        shGsEnv->StencilBuf[i].frame.ui32[1] = 0xFF000000;
        shGsEnv->StencilBuf[i].frame.ul64[1] = 0x4C;

        shGsEnv->StencilBuf[i].gifad_frame_normal.ui32[3] = 0;
        shGsEnv->StencilBuf[i].gifad_frame_normal.ui32[2] = 0xE;
        shGsEnv->StencilBuf[i].gifad_frame_normal.ui32[1] = 0x10000000;
        shGsEnv->StencilBuf[i].gifad_frame_normal.ui32[0] = 0x8001;
        shGsEnv->StencilBuf[i].frame_normal.ul128 = shGsEnv->StencilBuf[i].frame.ul128;
        shGsEnv->StencilBuf[i].frame_normal.ui32[1] = 0;

        shGsEnv->StencilBuf[i].gifad_frame_mskalpha.ui32[3] = 0;
        shGsEnv->StencilBuf[i].gifad_frame_mskalpha.ui32[2] = 0xE;
        shGsEnv->StencilBuf[i].gifad_frame_mskalpha.ui32[1] = 0x10000000;
        shGsEnv->StencilBuf[i].gifad_frame_mskalpha.ui32[0] = 0x8001;
        shGsEnv->StencilBuf[i].frame_mskalpha.ul128 = shGsEnv->StencilBuf[i].frame.ul128;

        shGsEnv->StencilBuf[i].gifad_frame_mskDalpha.ui32[3] = 0;
        shGsEnv->StencilBuf[i].gifad_frame_mskDalpha.ui32[2] = 0xE;
        shGsEnv->StencilBuf[i].gifad_frame_mskDalpha.ui32[1] = 0x10000000;
        shGsEnv->StencilBuf[i].gifad_frame_mskDalpha.ui32[0] = 0x8001;
        shGsEnv->StencilBuf[i].frame_mskDalpha.ul128 = shGsEnv->StencilBuf[i].frame.ul128;
        shGsEnv->StencilBuf[i].frame_mskDalpha.ui32[1] = 0x80000000;
    }
    for (i = 0; i < 3; i++) {
        shGsEnv->StencilBuf[i].alpha1.ul64[1] = 0x42;
        shGsEnv->StencilBuf[i].alpha1.ul64[0] = 0x48;
        shGsEnv->StencilBuf[i].alpha1_nc.ul128 = shGsEnv->StencilBuf[i].alpha1.ul128;
        shGsEnv->StencilBuf[i].drawq2[0].ul64[1] = 0x4D;
        shGsEnv->StencilBuf[i].drawq2[0].ul64[0] = shGsEnv->StencilBuf[i].frame.ul64[0];
        shGsEnv->StencilBuf[i].drawq2[1].ul64[1] = 0x4F;
        shGsEnv->StencilBuf[i].drawq2[1].ul64[0] = *(unsigned long *)&shGsEnv->StencilBuf[i].draw.zbuf1;
        shGsEnv->StencilBuf[i].drawq2[2].ul64[1] = 0x19;
        shGsEnv->StencilBuf[i].drawq2[2].ul64[0] = *(unsigned long *)&shGsEnv->StencilBuf[i].draw.xyoffset1;
        shGsEnv->StencilBuf[i].drawq2[3].ul64[1] = 0x41;
        shGsEnv->StencilBuf[i].drawq2[3].ul64[0] = *(unsigned long *)&shGsEnv->StencilBuf[i].draw.scissor1;
        shGsEnv->StencilBuf[i].drawq2[4].ul64[1] = 0x43;
        shGsEnv->StencilBuf[i].drawq2[4].ul64[0] = 0x42;
        shGsEnv->StencilBuf[i].drawq2[5].ul64[1] = 0x48;
        shGsEnv->StencilBuf[i].drawq2[5].ul64[0] = *(unsigned long *)&shGsEnv->StencilBuf[i].draw.test1;
        shGsEnv->StencilBuf[i].alpha1_nc.ul128 = shGsEnv->StencilBuf[i].alpha1.ul128;
        shGsEnv->StencilBuf[i].drawq2_nc[0].ul128 = shGsEnv->StencilBuf[i].drawq2[0].ul128;
        shGsEnv->StencilBuf[i].drawq2_nc[1].ul128 = shGsEnv->StencilBuf[i].drawq2[1].ul128;
        shGsEnv->StencilBuf[i].drawq2_nc[2].ul128 = shGsEnv->StencilBuf[i].drawq2[2].ul128;
        shGsEnv->StencilBuf[i].drawq2_nc[3].ul128 = shGsEnv->StencilBuf[i].drawq2[3].ul128;
        shGsEnv->StencilBuf[i].drawq2_nc[4].ul128 = shGsEnv->StencilBuf[i].drawq2[4].ul128;
        shGsEnv->StencilBuf[i].drawq2_nc[5].ul128 = shGsEnv->StencilBuf[i].drawq2[5].ul128;
    }
}

/** Fills a stencil buffer's draw environment. @param w, h size. @param ztest, zpsm Z settings. @return 8. */
int shGs_InitStencilDrawEnv(struct shGsStencilDrawEnv *ssb, short w, short h, short ztest, short zpsm) {
    unsigned int zb;

    ssb->draw.frame1addr = 0x4C;
    *(unsigned long *)&ssb->draw.frame1 = GS_SET_FRAME(0, (w >> 6) & 0x3F, 0xA, 0);
    ssb->draw.zbuf1addr = 0x4E;
    zb = 0x1C0;
    *(unsigned long *)&ssb->draw.zbuf1 = GS_SET_ZBUF(zb, zpsm & 0xF, 1);
    ssb->draw.xyoffset1addr = 0x18;
    *(unsigned long *)&ssb->draw.xyoffset1 = GS_SET_XYOFFSET((2048 - (long)(w >> 1)) << 4, (2048 - (long)(h >> 1)) << 4);
    ssb->draw.scissor1addr = 0x40;
    *(unsigned long *)&ssb->draw.scissor1 = GS_SET_SCISSOR(0, w - 1, 0, h - 1);
    ssb->draw.prmodecontaddr = 0x1A;
    ssb->draw.prmodecont.AC = 1;
    ssb->draw.colclampaddr = 0x46;
    ssb->draw.colclamp.CLAMP = 1;
    ssb->draw.dtheaddr = 0x45;
    ssb->draw.dthe.DTHE = 0;
    ssb->draw.test1addr = 0x47;
    *(unsigned long *)&ssb->draw.test1 = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ztest & 3);
    asm volatile("sync.l");
    return 8;
}

/** Sets up the shadow-filter work pages (two per buffer). @param w, h size. */
void shGs_InitGsTinyStencilBuff(struct shGsAllEnv *shGsEnv, short w, short h) {
    unsigned int i;
    unsigned int fbp;

    shGsEnv->StencilWork[0].giftag.ui32[3] = 0;
    shGsEnv->StencilWork[0].giftag.ui32[2] = 0xE;
    shGsEnv->StencilWork[0].giftag.ui32[1] = 0x10000000;
    shGsEnv->StencilWork[0].giftag.ui32[0] = 0x8005;
    shGsEnv->StencilWork[0].frame.ui32[0] = (w >> 6) << 16;
    shGsEnv->StencilWork[0].frame.ui32[1] = 0;
    shGsEnv->StencilWork[0].frame.ul64[1] = 0x4C;
    shGsEnv->StencilWork[0].scissor.ul64[0] = GS_SET_SCISSOR(0, w - 1, 0, h - 1);
    shGsEnv->StencilWork[0].scissor.ul64[1] = 0x40;
    shGsEnv->StencilWork[0].xyoffset.ul64[1] = 0x18;
    shGsEnv->StencilWork[0].xyoffset.ul64[0] = GS_SET_XYOFFSET((2048 - (long)(w >> 1)) << 4, (2048 - (long)(h >> 1)) << 4);
    shGsEnv->StencilWork[0].zbuf.ul64[1] = 0x4E;
    shGsEnv->StencilWork[0].zbuf.ul64[0] = GS_SET_ZBUF(0x1C0, 0xA, 1);
    shGsEnv->StencilWork[0].test.ul64[1] = 0x47;
    shGsEnv->StencilWork[0].test.ul64[0] = 0x38002;
    shGsEnv->StencilWork[0].gifad_normal.ui32[3] = 0;
    shGsEnv->StencilWork[0].gifad_normal.ui32[2] = 0xE;
    shGsEnv->StencilWork[0].gifad_normal.ui32[1] = 0x10000000;
    shGsEnv->StencilWork[0].gifad_normal.ui32[0] = 0x8001;
    shGsEnv->StencilWork[0].frame_normal.ul128 = shGsEnv->StencilWork[0].frame.ul128;
    shGsEnv->StencilWork[0].gifad_mskalpha.ul128 = shGsEnv->StencilWork[0].gifad_normal.ul128;
    shGsEnv->StencilWork[0].frame_mskalpha.ul128 = shGsEnv->StencilWork[0].frame.ul128;
    shGsEnv->StencilWork[0].frame_mskalpha.ui32[1] = 0xFF000000;
    shGsEnv->StencilWork[0].gifad_mskDalpha.ul128 = shGsEnv->StencilWork[0].gifad_normal.ul128;
    shGsEnv->StencilWork[0].frame_mskDalpha.ul128 = shGsEnv->StencilWork[0].frame.ul128;
    shGsEnv->StencilWork[0].frame_mskDalpha.ui32[1] = 0x80000000;
    for (i = 1; i < 6; i++) {
        shGsEnv->StencilWork[i] = shGsEnv->StencilWork[0];
    }
    for (i = 0; i < 6; i += 2) {
        fbp = shGsEnv->DispEnv[(i / 2 + 1) % 3].dispfb.FBP + 0x40;
        shGsEnv->StencilWork[i].frame.ui32[0] = (w >> 6) << 16 | fbp;
        shGsEnv->StencilWork[i].frame.ui32[1] = 0;
        shGsEnv->StencilWork[i].frame.ul64[1] = 0x4C;
        shGsEnv->StencilWork[i + 1].frame.ui32[0] = (w >> 6) << 16 | (fbp + 0x20);
        shGsEnv->StencilWork[i + 1].frame.ui32[1] = 0;
        shGsEnv->StencilWork[i + 1].frame.ul64[1] = 0x4C;
        shGsEnv->StencilWork[i].frame_normal.ul128 = shGsEnv->StencilWork[i].frame.ul128;
        shGsEnv->StencilWork[i + 1].frame_normal.ul128 = shGsEnv->StencilWork[i + 1].frame.ul128;
        shGsEnv->StencilWork[i].frame_mskalpha.ul128 = shGsEnv->StencilWork[i].frame.ul128;
        shGsEnv->StencilWork[i].frame_mskalpha.ui32[1] = 0xFF000000;
        shGsEnv->StencilWork[i + 1].frame_mskalpha.ul128 = shGsEnv->StencilWork[i + 1].frame.ul128;
        shGsEnv->StencilWork[i + 1].frame_mskalpha.ui32[1] = 0xFF000000;
        shGsEnv->StencilWork[i].frame_mskDalpha.ul128 = shGsEnv->StencilWork[i].frame.ul128;
        shGsEnv->StencilWork[i].frame_mskDalpha.ui32[1] = 0x80000000;
        shGsEnv->StencilWork[i + 1].frame_mskDalpha.ul128 = shGsEnv->StencilWork[i + 1].frame.ul128;
        shGsEnv->StencilWork[i + 1].frame_mskDalpha.ui32[1] = 0x80000000;
    }
}

/** Builds the default GS register packet (DefaultEnv). */
void shGs_InitDefaultRegsEnv(struct shGsAllEnv *shGsEnv) {
    unsigned int id;
    unsigned int i;
    union Q_WORDDATA *qwd;

    shGsEnv->DefaultEnv[1].ul64[1] = 0x42;
    shGsEnv->DefaultEnv[1].ul64[0] = 0x44;
    shGsEnv->DefaultEnv[2].ul64[1] = 0x47;
    shGsEnv->DefaultEnv[2].ul64[0] = 0x5000D;
    shGsEnv->DefaultEnv[3].ul64[1] = 0x8;
    shGsEnv->DefaultEnv[3].ul64[0] = 0;
    shGsEnv->DefaultEnv[4].ul64[1] = 0x46;
    shGsEnv->DefaultEnv[4].ul64[0] = 1;
    shGsEnv->DefaultEnv[5].ul64[1] = 0x4E;
    shGsEnv->DefaultEnv[5].ul64[0] = 0x3A0001C0;
    shGsEnv->DefaultEnv[6].ul64[0] = 0x60;
    shGsEnv->DefaultEnv[6].ul64[1] = 0x14;
    shGsEnv->DefaultEnv[7].ul64[1] = 0x3D;
    shGsEnv->DefaultEnv[7].ul64[0] = 0;
    shGsEnv->DefaultEnv[8].ul64[1] = 0x3B;
    shGsEnv->DefaultEnv[8].ul64[0] = 0x8000000040;
    shGsEnv->DefaultEnv[0].ui32[3] = 0;
    shGsEnv->DefaultEnv[0].ui32[2] = 0xE;
    shGsEnv->DefaultEnv[0].ui32[1] = 0x10000000;
    shGsEnv->DefaultEnv[0].ui32[0] = 0x8009;

    shGsEnv->StencilEnv[1].ul64[1] = 0x42;
    shGsEnv->StencilEnv[1].ul64[0] = 0x42;
    shGsEnv->StencilEnv[2].ul64[1] = 0x47;
    shGsEnv->StencilEnv[2].ul64[0] = 0x70000;
    shGsEnv->StencilEnv[0].ui32[3] = 0;
    shGsEnv->StencilEnv[0].ui32[2] = 0xE;
    shGsEnv->StencilEnv[0].ui32[1] = 0x10000000;
    shGsEnv->StencilEnv[0].ui32[0] = 0x8002;

    shGsEnv->GsReg_ALPHA_A[0].ui32[3] = 0;
    shGsEnv->GsReg_ALPHA_A[0].ui32[2] = 0xE;
    shGsEnv->GsReg_ALPHA_A[0].ui32[1] = 0x10000000;
    shGsEnv->GsReg_ALPHA_A[0].ui32[0] = 0x8001;
    shGsEnv->GsReg_ALPHA_A[1].ul128 = shGsEnv->DefaultEnv[1].ul128;
    qwd = shGsEnv->GsReg_ALPHA_A;
    for (i = 2; i < 10; i += 2) {
        qwd[i] = qwd[0];
        qwd[i + 1] = qwd[1];
    }
    shGsEnv->GsReg_ALPHA_B[1].ul64[1] = 0x42;
    shGsEnv->GsReg_ALPHA_B[1].ul64[0] = 0x48;
    shGsEnv->GsReg_ALPHA_C[1].ul64[1] = 0x42;
    shGsEnv->GsReg_ALPHA_C[1].ul64[0] = 0x42;
    shGsEnv->GsReg_ALPHA_D[1].ul64[1] = 0x42;
    shGsEnv->GsReg_ALPHA_D[1].ul64[0] = 0x88;

    shGsEnv->GsReg_TEST_A[0].ul128 = shGsEnv->GsReg_ALPHA_A[0].ul128;
    shGsEnv->GsReg_TEST_A[1].ul128 = shGsEnv->DefaultEnv[2].ul128;
    qwd = shGsEnv->GsReg_TEST_A;
    for (i = 2; i < 10; i += 2) {
        qwd[i] = qwd[0];
        qwd[i + 1] = qwd[1];
    }
    shGsEnv->GsReg_TEST_B[1].ul64[1] = 0x47;
    shGsEnv->GsReg_TEST_B[1].ul64[0] = 0x38000;
    shGsEnv->GsReg_TEST_C[1].ul64[1] = 0x47;
    shGsEnv->GsReg_TEST_C[1].ul64[0] = 0x3C000;
    shGsEnv->GsReg_TEST_D[1].ul64[1] = 0x47;
    shGsEnv->GsReg_TEST_D[1].ul64[0] = 0x34000;

    shGsEnv->GsReg_FBA_A[0].ul128 = shGsEnv->GsReg_TEST_A[0].ul128;
    shGsEnv->GsReg_FBA_A[1].ul64[1] = 0x4A;
    shGsEnv->GsReg_FBA_A[1].ul64[0] = 0;
    shGsEnv->GsReg_FBA_B[0].ul128 = shGsEnv->GsReg_TEST_A[0].ul128;
    shGsEnv->GsReg_FBA_B[1].ul64[1] = 0x4A;
    shGsEnv->GsReg_FBA_B[1].ul64[0] = 1;

    shGsEnv->GsReg_TEXA_A[0].ul128 = shGsEnv->GsReg_TEST_A[0].ul128;
    shGsEnv->GsReg_TEXA_A[1].ul64[1] = 0x3B;
    shGsEnv->GsReg_TEXA_A[1].ul64[0] = 0x8080;
    qwd = shGsEnv->GsReg_TEXA_A;
    for (i = 2; i < 4; i += 2) {
        qwd[i] = qwd[0];
        qwd[i + 1] = qwd[1];
    }
    shGsEnv->GsReg_TEXA_B[1].ul64[1] = 0x3B;
    shGsEnv->GsReg_TEXA_B[1].ul64[0] = 0x80;

    shGsEnv->GsReg_ZBUF_A[0].ul128 = shGsEnv->GsReg_TEXA_A[0].ul128;
    shGsEnv->GsReg_ZBUF_A[1].ul128 = shGsEnv->DefaultEnv[5].ul128;
    shGsEnv->GsReg_ZBUF_B[0].ul128 = shGsEnv->GsReg_ZBUF_A[0].ul128;
    shGsEnv->GsReg_ZBUF_B[1].ul64[1] = 0x4E;
    shGsEnv->GsReg_ZBUF_B[1].ul64[0] = 0x10A0001C0;

    shGsEnv->GsSync_DummyLABEL[0].ui32[3] = 0;
    shGsEnv->GsSync_DummyLABEL[0].ui32[2] = 0xE;
    shGsEnv->GsSync_DummyLABEL[0].ui32[1] = 0x10000000;
    shGsEnv->GsSync_DummyLABEL[0].ui32[0] = 0x8001;
    shGsEnv->GsSync_DummyLABEL[1].ul64[1] = 0x62;
    shGsEnv->GsSync_DummyLABEL[1].ui32[1] = 0;
    shGsEnv->GsSync_DummyLABEL[1].ui32[0] = 0;
    shGsEnv->GsSync_DummyTEXFLUSH[0].ui32[3] = 0;
    shGsEnv->GsSync_DummyTEXFLUSH[0].ui32[2] = 0xE;
    shGsEnv->GsSync_DummyTEXFLUSH[0].ui32[1] = 0x10000000;
    shGsEnv->GsSync_DummyTEXFLUSH[0].ui32[0] = 0x8001;
    shGsEnv->GsSync_DummyTEXFLUSH[1].ul64[1] = 0x3F;
    shGsEnv->GsSync_DummyTEXFLUSH[1].ul64[0] = 0;
}
