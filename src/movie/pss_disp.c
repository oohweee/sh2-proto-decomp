#include "sh2.h"
#include "sdk/eekernel.h"
#include "sdk/libdma.h"
#include "sdk/libgifpk.h"
#include "sdk/libgraph.h"

/*
 * pss_disp.c: the movie player's display (its own unit in the DWARF): the
 * vblank handler converts each decoded frame with the VU1 CSC microcode into
 * the back buffer, copies it to the display buffer and draws the subtitles.
 */


#define UNCACHED(val) ((void *)(((unsigned int)(val) & 0x0FFFFFFF) | 0x20000000))

int csct;
unsigned int Movpictag[256] __attribute__((aligned(64)));
int handler_error;
volatile int isUp; /* volatile: the original loads it again for each use (isUp & 1, isUp ^ 1) */
int isFrameEnd;
volatile int vblankCount; /* volatile: the original's vblankCount++ loads it twice */
int isCountVblank;
/* The original places this pointer on a 64-byte boundary (0x3C bytes of padding before it). */
struct _PssCommonWork *pss_common_work __attribute__((aligned(64)));
struct PSS_EXEC_CTRL pssExecCtrl;
struct _PSS_SUBTITLE_CTRL pssSubTitleCtrl;

static void MovePicture(unsigned int *tags, unsigned short fbp);
static void SetDrawFBP(unsigned int *tags, unsigned short fbp);
static void SetUndoXYOFFSET(unsigned int *tags);

/** Sets up the movie's double buffer (512x512) with the game's display settings. */
void pssInitDisplay(void) {
    sceGsSetDefDBuff(&pss_common_work->db, 0, 512, 512, 0, 0, 1);
    pss_common_work->db.disp[0].display = shGs_AllEnv.DispEnv[0].display;
    pss_common_work->db.disp[1].display = shGs_AllEnv.DispEnv[0].display;
    pss_common_work->db.disp[1].dispfb.FBP += 0x80;
    pss_common_work->db.draw0.frame1.FBP = pss_common_work->db.disp[1].dispfb.FBP;
    pss_common_work->db.draw1.frame1.FBP = pss_common_work->db.disp[0].dispfb.FBP;
    pss_common_work->db.disp[0].dispfb.FBP += 8;
    pss_common_work->db.disp[1].dispfb.FBP += 8;
    FlushCache(0);
}

/**
 * Sets the draw buffer's half-pixel offset for the current field, swaps the double buffer and waits
 * for the GS.
 */
void pssDispClear(void) {
    sceGsSetHalfOffset((isUp & 1) ? (sceGsDrawEnv1 *)UNCACHED(&pss_common_work->db.draw1)
                                  : (sceGsDrawEnv1 *)UNCACHED(&pss_common_work->db.draw0),
                       2048, 2048, isUp ^ 1);
    sceGsSwapDBuff(&pss_common_work->db, 0);
    sceGsSyncPath(0, 0);
}

/** Clears the GS FINISH event flag. */
void clearFinish(void) {
    *GS_CSR |= 2;
}

/**
 * V-blank start interrupt handler: once a frame's worth of vblanks have passed, sends the next
 * decoded picture through the VU1 colour space conversion and swaps the display. `val` is unused.
 */
int vblankHandler(int val) {
    sceDmaChan *dmaGif_loadimage;
    sceDmaChan *dmaGif2;
    VoBufTag *tag;
    void *adr;

    dmaGif_loadimage = sceDmaGetChan(1);
    dmaGif2 = sceDmaGetChan(2);

    isUp = (*GS_CSR >> 13) & 1;

    if (isCountVblank && pssExecCtrl.status == 4) {
        vblankCount++;
        handler_error = sceGsSyncPath(1, 0);
        if (!handler_error) {
            tag = voBufGetTag(&voBuf);
            if (!tag) {
                frd++;
                ExitHandler();
                return 0;
            }

            sceGsSetHalfOffset((isUp & 1) ? (sceGsDrawEnv1 *)UNCACHED(&pss_common_work->db.draw1)
                                          : (sceGsDrawEnv1 *)UNCACHED(&pss_common_work->db.draw0),
                               2048, 2048, isUp ^ 1);

            if (isUp && tag->status == 2) {
                sceGsSwapDBuff(&pss_common_work->db, 0);
                SetDrawFBP(Movpictag, 0x100);
                iFlushCache(0);
                sceDmaSend(dmaGif2, Movpictag);
                cscVu1Xyz2offset(&videoDec.csc, csct, 0, 0x7000, 0x7000);
                cscVu1Kick(tag->v);
                tag->status = 1;
                sceGsSyncPath(0, 0);

                SetDrawFBP(Movpictag, pss_common_work->db.disp[1].dispfb.FBP - 8);
                iFlushCache(0);
                sceDmaSend(dmaGif2, Movpictag);
                sceGsSyncPath(0, 0);

                MovePicture(Movpictag, 0x100);
                sceDmaSend(dmaGif2, Movpictag);
            } else if (!isUp && tag->status == 1) {
                sceGsSwapDBuff(&pss_common_work->db, 1);
                SetDrawFBP(Movpictag, 0x140);
                iFlushCache(0);
                sceDmaSend(dmaGif2, Movpictag);
                cscVu1Xyz2offset(&videoDec.csc, csct, 1, 0x7000, 0x7000);
                cscVu1Kick(tag->v);
                tag->status = 0;
                isFrameEnd = 1;
                sceGsSyncPath(0, 0);

                SetDrawFBP(Movpictag, pss_common_work->db.disp[0].dispfb.FBP - 8);
                iFlushCache(0);
                sceDmaSend(dmaGif2, Movpictag);
                sceGsSyncPath(0, 0);

                MovePicture(Movpictag, 0x140);
                sceDmaSend(dmaGif2, Movpictag);
            }
            sceGsSyncPath(0, 0);

            if (isUp & 1) {
                SetUndoXYOFFSET(Movpictag);
                sceDmaSend(dmaGif2, Movpictag);
                sceGsSyncPath(0, 0);
            }

            spkResetOT();
            if (pssDrawSubTitle()) {
                adr = fontTexLoad(0x3000, 0x3600);
                iFlushCache(0);
                sceDmaSend(dmaGif_loadimage, adr);
                sceGsSyncPath(0, 0);

                adr = fontFlushNoSPR();
                iFlushCache(0);
                sceDmaSend(dmaGif_loadimage, adr);
                sceDmaSend(dmaGif_loadimage, fontAfterEnv());
                sceGsSyncPath(0, 0);
            }
        }
    } else if (sys_mpeg.frameCount < pssExecCtrl.stFrame) {
        vblankCount++;
        handler_error = sceGsSyncPath(1, 0);
        if (!handler_error) {
            VoBufTag *tag;

            tag = voBufGetTag(&voBuf);
            if (!tag) {
                frd++;
                ExitHandler();
                return 0;
            }

            if (isUp && tag->status == 2) {
                tag->status = 1;
            } else if (!isUp && tag->status == 1) {
                tag->status = 0;
                isFrameEnd = 1;
            }
        }

        if (isFrameEnd) {
            voBufDecCount(&voBuf);
            isFrameEnd = 0;
        }
    }

    ExitHandler();
    return 0;
}

/** Draws the subtitle of the current frame, moving to the next message when its time is over. */
int pssDrawSubTitle(void) {
    if (pssSubTitleCtrl.adr_msg_time) {
        if (pssSubTitleCtrl.adr_msg_time[pssSubTitleCtrl.msg_no].end < pssExecCtrl.framecnt) {
            fontClear();
            pssSubTitleCtrl.msg_no++;
        }
        if (pssSubTitleCtrl.adr_msg_time[pssSubTitleCtrl.msg_no].start < pssExecCtrl.framecnt) {
            fontMessageNum(pssSubTitleCtrl.msg_bufp, pssSubTitleCtrl.msg_start + pssSubTitleCtrl.msg_no);
            return 1;
        }
    }
    return 0;
}

/** GS FINISH interrupt handler: counts the frame when a picture has been drawn. `val` is unused. */
int handler_endimage(int val) {
    clearFinish();
    if (isFrameEnd) {
        voBufDecCount(&voBuf);
        isFrameEnd = 0;
        pssExecCtrl.framecnt++;
    }
    ExitHandler();
    return 0;
}

/**
 * Waits until the V-sync field is no longer `waitEven`, then starts counting vblanks for the
 * picture timing.
 */
void startDisplay(int waitEven) {
    while (sceGsSyncV(0) == waitEven) {
    }
    frd = 0;
    isCountVblank = 1;
    vblankCount = 0;
}

/** Waits for the GS path, then stops counting vblanks. */
void endDisplay(void) {
    sceGsSyncPath(0, 0);
    isCountVblank = 0;
    frd = 0;
}

static void MovePicture(unsigned int *tags, unsigned short fbp) {
    unsigned long giftag_eop[2] = {0x1000000000008000, 0x000000000000000E};
    sceGifPacket packet;

    sceGifPkInit(&packet, (u_long128 *)UNCACHED(tags));
    sceGifPkReset(&packet);
    sceGifPkEnd(&packet, 0, 0, 0);
    sceGifPkOpenGifTag(&packet, *(u_long128 *)giftag_eop);

    sceGifPkAddGsAD(&packet, GS_REG_TEXFLUSH, 0);
    sceGifPkAddGsAD(&packet, GS_REG_TEX1_1, GS_SET_TEX1(0, 0, 0, 1, 0, 0, 0));
    sceGifPkAddGsAD(&packet, GS_REG_TEX0_1, GS_SET_TEX0(fbp * 32, 8, 0, 9, 8, 0, 1, 0, 0, 0, 0, 0));
    sceGifPkAddGsAD(&packet, GS_REG_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
    sceGifPkAddGsAD(&packet, GS_REG_UV, GS_SET_UV(0, 0));
    sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x7000, 0x7400, 0));
    sceGifPkAddGsAD(&packet, GS_REG_UV, GS_SET_UV(0x2000, 0xC00));
    sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x9000, 0x8C00, 0));

    if (pssGetMaskSwitch()) {
        sceGifPkAddGsAD(&packet, GS_REG_PRIM, GS_SET_PRIM(6, 0, 0, 0, 0, 0, 1, 0, 0));
        sceGifPkAddGsAD(&packet, GS_REG_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0x80, 0));
        sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x7000, 0x7000, 1));
        sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x9000, 0x7540, 1));
        sceGifPkAddGsAD(&packet, GS_REG_PRIM, GS_SET_PRIM(6, 0, 0, 0, 0, 0, 1, 0, 0));
        sceGifPkAddGsAD(&packet, GS_REG_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0x80, 0));
        sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x7000, 0x8AC0, 1));
        sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x9000, 0x9000, 1));
    } else {
        sceGifPkAddGsAD(&packet, GS_REG_PRIM, GS_SET_PRIM(6, 0, 0, 0, 0, 0, 1, 0, 0));
        sceGifPkAddGsAD(&packet, GS_REG_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0x80, 0));
        sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x7000, 0x7000, 1));
        sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x9000, 0x7400, 1));
        sceGifPkAddGsAD(&packet, GS_REG_PRIM, GS_SET_PRIM(6, 0, 0, 0, 0, 0, 1, 0, 0));
        sceGifPkAddGsAD(&packet, GS_REG_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0x80, 0));
        sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x7000, 0x8C00, 1));
        sceGifPkAddGsAD(&packet, GS_REG_XYZ2, GS_SET_XYZ(0x9000, 0x9000, 1));
    }

    sceGifPkCloseGifTag(&packet);
    sceGifPkTerminate(&packet);
}

static void SetDrawFBP(unsigned int *tags, unsigned short fbp) {
    unsigned long giftag_eop[2] = {0x1000000000008000, 0x000000000000000E};
    sceGifPacket packet;

    sceGifPkInit(&packet, (u_long128 *)UNCACHED(tags));
    sceGifPkReset(&packet);
    sceGifPkEnd(&packet, 0, 0, 0);
    sceGifPkOpenGifTag(&packet, *(u_long128 *)giftag_eop);
    sceGifPkAddGsAD(&packet, GS_REG_FRAME_1, GS_SET_FRAME(fbp, 8, 0, 0));
    sceGifPkCloseGifTag(&packet);
    sceGifPkTerminate(&packet);
}

static void SetUndoXYOFFSET(unsigned int *tags) {
    unsigned long giftag_eop[2] = {0x1000000000008000, 0x000000000000000E};
    sceGifPacket packet;

    sceGifPkInit(&packet, (u_long128 *)UNCACHED(tags));
    sceGifPkReset(&packet);
    sceGifPkEnd(&packet, 0, 0, 0);
    sceGifPkOpenGifTag(&packet, *(u_long128 *)giftag_eop);
    sceGifPkAddGsAD(&packet, GS_REG_XYOFFSET_1, GS_SET_XYOFFSET(0x7000, 0x7000));
    sceGifPkCloseGifTag(&packet);
    sceGifPkTerminate(&packet);
}
