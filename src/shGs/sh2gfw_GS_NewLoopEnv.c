/*
 * GS environment rotation for triple buffering: shGsAllEnv keeps three display/draw
 * environments (indexed by loop3) plus stencil/shadow-filter buffers, and these functions
 * select the current one, send its register settings (sh2gfw_setREF_*), and step the loop
 * counters.
 */

#include "sh2.h"
#include "sdk/libgraph.h"

/** Shows the current display buffer and sends the current draw environment. @return loop3. */
int sh2gfw_RotateTBuff(struct shGsAllEnv *shGsEnv) {
    sceGsPutDispEnv(&shGsEnv->DispEnv[shGsEnv->loop3]);
    sh2gfw_setREF_gsctl((union Q_WORDDATA *)&shGsEnv->DrawEnv[shGsEnv->loop3]);
    sh2gfw_setREF_TEXFLUSH();
    return shGsEnv->loop3;
}

/** Sends the current draw environment again (without clear), between texture flushes. @return loop3. */
int sh2gfw_ReturnTBuff(struct shGsAllEnv *shGsEnv) {
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_setREF_gsctl((union Q_WORDDATA *)&shGsEnv->DrawEnv[shGsEnv->loop3].giftag_nc);
    sh2gfw_setREF_TEXFLUSH();
    return shGsEnv->loop3;
}

/** Sends the draw environment of the buffer before the current one (without clear). @return loop3. */
int sh2gfw_RotateNext2(struct shGsAllEnv *stdb) {
    unsigned int idd[3] = { 2, 0, 1 };

    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_setREF_gsctl((union Q_WORDDATA *)&stdb->DrawEnv[idd[stdb->loop3]].giftag_nc);
    return stdb->loop3;
}

/** Copies the current draw environment to Now_DrawEnv. */
void sh2gfw_set_LoopDrawEnv(struct shGsAllEnv *stdb) {
    stdb->Now_DrawEnv = stdb->DrawEnv[stdb->loop3];
}

/** Returns the address of Now_DrawEnv's normal FRAME register setting. */
u_long128 *sh2gfw_Get_FrameNormalRegAddr(void) {
    return &shGs_AllEnv.Now_DrawEnv.frame_normal.ul128;
}

/** Returns the address of Now_DrawEnv's alpha-masked FRAME register setting. */
u_long128 *sh2gfw_Get_FrameAlphaRegAddr(void) {
    return &shGs_AllEnv.Now_DrawEnv.frame_mskalpha.ul128;
}

/** Sends the current stencil (shadow) buffer's draw environment. */
void sh2gfw_StartShadowEnv(struct shGsAllEnv *stdb) {
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_setREF_gsctl((union Q_WORDDATA *)&stdb->StencilBuf[stdb->loop3]);
    sh2gfw_setREF_TEXFLUSH();
}

/** Returns the current stencil buffer's frame base pointer. */
unsigned int sh2gfw_GetStencilFBP(struct shGsAllEnv *stdb) {
    return stdb->StencilBuf[stdb->loop3].draw.frame1.FBP;
}

/** Returns the frame base pointer of shadow-filter work page @p page (0/1) of the current buffer. */
unsigned int sh2gfw_GetFilterFBP(struct shGsAllEnv *stdb, unsigned int page) {
    return stdb->StencilWork[stdb->loop3 * 2 + page].frame.ui32[0] & 0x1FF;
}

/** Sends the draw environment of shadow-filter work page @p page of the current buffer. */
void sh2gfw_StartShadowFilter(struct shGsAllEnv *stdb, unsigned int page) {
    sh2gfw_setREF_TEXFLUSH();
    sh2gfw_setREF_gsctl(&stdb->StencilWork[stdb->loop3 * 2 + page].giftag);
    sh2gfw_setREF_TEXFLUSH();
}

/**
 * Sends a FRAME mask setting for a shadow-filter work page.
 * @param mode 0 normal, 1 alpha masked, other: the mskDalpha setting. @param pageno work page (0/1).
 */
void sh2gfw_ChangeMask_ShadowFilter(struct shGsAllEnv *stdb, int mode, int pageno) {
    unsigned int id;

    id = stdb->loop3 * 2;
    switch (mode) {
    case 0:
        sh2gfw_setREF_gsctl(&stdb->StencilWork[id + pageno].gifad_normal);
        break;
    case 1:
        sh2gfw_setREF_gsctl(&stdb->StencilWork[id + pageno].gifad_mskalpha);
        break;
    default:
        sh2gfw_setREF_gsctl(&stdb->StencilWork[id + pageno].gifad_mskDalpha);
        break;
    }
}

/** Sets the current stencil buffer's clear colour (8 bits per channel). */
void sh2gfw_ChangeClear_StencilBuf(struct shGsAllEnv *stdb, unsigned int r, unsigned int g, unsigned int b, unsigned int a) {
    union Q_WORDDATA *qwd;

    qwd = (union Q_WORDDATA *)&stdb->StencilBuf[stdb->loop3].clear.rgbaq;
    qwd->ui32[0] = r | (g << 8) | (b << 16) | (a << 24);
}

/** Advances to the next buffer (loop3 cycles 0-2) and counts the frame. @return the new loop count. */
unsigned int sh2gfw_InclimentLoopCounter(struct shGsAllEnv *stdb) {
    stdb->loop3++;
    stdb->loop3 %= 3;
    stdb->loop_counter++;
    stdb->loop++;
    return stdb->loop;
}

/** Steps back to the previous buffer, or resets all counters to 2 when loop_counter is 0. */
unsigned int sh2gfw_DeclimentLoopCounter(struct shGsAllEnv *stdb) {
    if (stdb->loop_counter) {
        if (stdb->loop3 == 0) {
            stdb->loop3 = 2;
        } else {
            stdb->loop3--;
        }
        stdb->loop_counter--;
        stdb->loop--;
    } else {
        stdb->loop3 = 2;
        stdb->loop_counter = 2;
        stdb->loop = 2;
    }
}

/** Jumps the global environment to buffer 2 (for movie playback), counting the skipped frames.
 * @return the old loop3. */
int sh2gfw_ForceSet_MovieDrawLoopCounter(void) {
    switch (shGs_AllEnv.loop3) {
    case 0:
        shGs_AllEnv.loop3 = 2;
        shGs_AllEnv.loop_counter += 2;
        shGs_AllEnv.loop += 2;
        return 0;
    case 1:
        shGs_AllEnv.loop3 = 2;
        shGs_AllEnv.loop_counter += 1;
        shGs_AllEnv.loop += 1;
        return 1;
    case 2:
        break;
    }
    return 2;
}

/** Returns the current display buffer's frame base pointer. */
unsigned int sh2gfw_GetNowDispFBP(struct shGsAllEnv *stdb) {
    return stdb->DispEnv[stdb->loop3].dispfb.FBP;
}

/** Returns texture page @p page's base pointer for the current buffer. */
unsigned int sh2gfw_GetTexTBP0(struct shGsAllEnv *stdb, unsigned int page) {
    return stdb->LoopEnv.GsTexTBPs[page + stdb->loop3 * 8];
}

/** Returns CLUT page @p page's base pointer. */
unsigned int sh2gfw_GetSendingClutTBP0(struct shGsAllEnv *stdb, unsigned int page) {
    return stdb->LoopEnv.GsClutPage[page];
}

/** Returns the base pointer of texture page 1 (2D graphics) for the current buffer. */
unsigned int sh2gfw_Get_BaseTBP0for2D(void) {
    return sh2gfw_GetTexTBP0(&shGs_AllEnv, 1);
}
