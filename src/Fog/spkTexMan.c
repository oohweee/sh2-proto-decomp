/*
 * spkTexMan.c: textures the fog/particle code (spk) uploads each frame. They
 * are queued for the texture-transfer thread; before the particle packet is
 * kicked, the transfers are waited on and the texture cache is flushed.
 *
 * Matching: the assert message bakes in "spkTexMan.c:91", so the assert
 * has to stay on line 91; the blank lines in sh2gfw_EnQue_spkTexture keep it there.
 */

#include "sh2.h"

#define ALL_GS_PAGENUM 5

/* DMA cnt + DIRECT 5: GIF tag (A+D x4), TEXFLUSH; the other three are set per frame. */
static union Q_WORDDATA SyncTexflush[6] = {
    {0x70000005, 0x00000000, 0x00000000, 0x50000005},
    {0x00008004, 0x10000000, 0x0000000E, 0x00000000},
    {0x00000000, 0x00000000, 0x0000003F, 0x00000000},
};

static struct spkTexManage AllUserTexManage;

/** Forgets every queued texture. */
void sh2gfw_Init_spkTexManage(void) {
    int i;

    AllUserTexManage.Registed_TexNum = 0;
    for (i = 0; i < ALL_GS_PAGENUM; i++) {
        AllUserTexManage.Work_Slotid[i] = -1;
        AllUserTexManage.Work_Commandid[i] = -1;
        AllUserTexManage.pTexMAN[i] = NULL;
    }
}

/** Sends texture pTM through the texture thread and remembers its command/slot ids. */
void sh2gfw_EnQue_spkTexture(void *pTM, int *cid, int *sid) {
    int i;

    sh2gfw_Thr_d2TextureSend(pTM, 1, cid, sid);
    i = AllUserTexManage.Registed_TexNum++;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 91
    assert_dw(i<ALL_GS_PAGENUM);
    AllUserTexManage.pTexMAN[i] = pTM;
    AllUserTexManage.Work_Slotid[i] = *sid;
    AllUserTexManage.Work_Commandid[i] = *cid;
}

/**
 * Waits for the queued texture transfers, flushes the texture cache (also setting the fog colour
 * and the test and Z-buffer registers), then kicks the particle packet (spkDmaKick()).
 */
void sh2gfw_ThrSyncKick_spkDmaKick(void) {
    void *pTop;
    int i;

    /* A+D data: FOGCOL (current fog color), TEST_1, ZBUF_1 */
    SyncTexflush[3].ul64[1] = 0x3D;
    *(unsigned int *)&SyncTexflush[3].ul64[0] = Env_ctl.fogcolor.ui32[0];
    SyncTexflush[4].ul64[1] = 0x47;
    SyncTexflush[4].ul64[0] = 0x5800D;
    SyncTexflush[5].ul64[1] = 0x4E;
    SyncTexflush[5].ul64[0] = 0x3A0001C0;
    for (i = 0; i < AllUserTexManage.Registed_TexNum; i++) {
        d1tscSync(AllUserTexManage.Work_Commandid[i]);
    }
    d1cSend(SyncTexflush);
    sh2gfw_Store_Perf2(*T0_COUNT, 6);
    pTop = spkDmaKick();
    sh2gfw_Store_Perf2(*T0_COUNT, 7);
    d1cSend(pTop);
    for (i = 0; i < AllUserTexManage.Registed_TexNum; i++) {
        d1tscFinishToUseSlot(AllUserTexManage.Work_Slotid[i]);
    }
    sh2gfw_Store_Perf2(*T0_COUNT, 11);
}
