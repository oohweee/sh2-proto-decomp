/*
 * Texture management and transfer (GFW): the pool of ALL_TEXNUM texture managers (a free and a
 * used list, with per-category counters), the DMA/GIF packets that upload a texture and its
 * CLUTs into one of the five GS texture slots, and the slot allocation for the transfers
 * (DMA channel 2, synchronized with channel 1 draws).
 */
/* Matching: the DWARF leaves out sh2gfw_SetSlot2Tex's unused third parameter; callers pass it. */

#include "sh2.h"
#include "sdk/libgraph.h"

#define ALL_TEXNUM 96

static struct sh2gfw_TexMAN *sh2gfw_get_newMANID(struct sh2gfw_ALLTEXSYNC_MAN *pATSM);
static void sh2gfw_init_SyncTexTag(struct sh2gfw_TexMAN *sTM, struct sh2gfw_TEX_HEAD *pTexHead,
                                   struct sh2gfw_CLUTS_HEAD *pClutHead, void *pMAN, unsigned short mode);
static int Fake_SetTexSlot(int sid, void *pk);
static void SetTexture2table(struct TexTable_List *pTL, void *pTexMan, int mark);
static int GetTexList_MinMark(void);

static union Q_WORDDATA fake_kick = {0x70000000, 0, 0, 0};
static int fake_cid[4];
static struct sh2gfw_TexTrans_Manage_Table TexTransTable;

/** Puts every texture manager on the free list and clears the counters. */
void sh2gfw_allinit_TexMANlist(struct sh2gfw_ALLTEXSYNC_MAN *pATSM) {
    int i;

    for (i = 0; i < ALL_TEXNUM; i++) {
        pATSM->TexMan[i].pPrev = &pATSM->TexMan[i - 1];
        pATSM->TexMan[i].pNext = &pATSM->TexMan[i + 1];
        pATSM->TexMan[i].check = i;
        pATSM->TexMan[i].mode = 0xFFFF;
    }
    pATSM->TexMan[0].pPrev = &pATSM->Empty_Head;
    pATSM->TexMan[ALL_TEXNUM - 1].pNext = &pATSM->Empty_Head;
    pATSM->Empty_Head.pNext = &pATSM->TexMan[0];
    pATSM->Empty_Head.pPrev = &pATSM->TexMan[ALL_TEXNUM - 1];
    pATSM->Used_Head.pNext = pATSM->Used_Head.pPrev = &pATSM->Used_Head;
    pATSM->g_BG = pATSM->st_BG = pATSM->l_BG = pATSM->bg_CHR = pATSM->human_CHR = pATSM->en_CHR = pATSM->ura_CHR =
        pATSM->x_CHR = pATSM->alltex_CHR = pATSM->oS_CHR = pATSM->oA_CHR = pATSM->wp_CHR = pATSM->alltexnum =
            pATSM->alltex_BG = pATSM->alltex_EFF = pATSM->trans_NOW_num = pATSM->alltexnum = 0;
}

/** Empty in this build. */
void sh2gfw_clear_TexMAN_TransParm(struct sh2gfw_ALLTEXSYNC_MAN *pATSM) {
}

/**
 * Takes a texture manager from the free list, builds its transfer packets for a texture and
 * its CLUTs, and counts it by category.
 * @param pATSM     the texture manager pool
 * @param pTexHead  the texture
 * @param pClutHead its CLUTs
 * @param pMAN      owner, stored in the manager
 * @param mode      owner kind: 0x5000-0x7FFF background, 0xD000 font, 0xE000 effect, else a
 *                  character id
 * @return the texture manager
 */
struct sh2gfw_TexMAN *sh2gfw_set_TexToTrasMan(struct sh2gfw_ALLTEXSYNC_MAN *pATSM, struct sh2gfw_TEX_HEAD *pTexHead,
                                              struct sh2gfw_CLUTS_HEAD *pClutHead, void *pMAN, unsigned short mode) {
    struct sh2gfw_TexMAN *pSTM;

    pSTM = sh2gfw_get_newMANID(pATSM);
    sh2gfw_init_SyncTexTag(pSTM, pTexHead, pClutHead, pMAN, mode);
    pATSM->alltexnum++;
    pATSM->dbg_add++;
    switch (mode & 0xF000) {
    case 0x5000:
    case 0x6000:
    case 0x7000:
        if (mode >= 0x7000) {
            pATSM->g_BG++;
        } else if (mode < 7000 && mode > 6000) { /* decimal in the original */
            pATSM->st_BG++;
        } else {
            pATSM->l_BG++;
        }
        pATSM->alltex_BG++;
        break;
    case 0xE000:
        pATSM->alltex_EFF++;
        break;
    case 0xD000:
        pATSM->fonttex++;
        break;
    default:
        if (mode >= 0x100 && mode < 0x12E) {
            pATSM->human_CHR++;
        } else if (mode >= 0x200 && mode < 0x212) {
            pATSM->en_CHR++;
        } else if (mode >= 0x300 && mode < 0x314) {
            pATSM->ura_CHR++;
        } else if (mode >= 0x700 && mode < 0x748) {
            pATSM->x_CHR++;
        } else if (mode >= 0x800 && mode < 0x82B) {
            pATSM->wp_CHR++;
        } else if (mode >= 0x500 && mode < 0x562) {
            pATSM->oS_CHR++;
        } else if (mode >= 0x400 && mode < 0x446) {
            pATSM->oA_CHR++;
        } else {
            pATSM->bg_CHR++;
        }
        pATSM->alltex_CHR++;
        break;
    }
    /* Matching: the #line keeps the assert string below on the original's line. */
#line 235
    assert(pATSM->alltexnum<ALL_TEXNUM);
    return pSTM;
}

static struct sh2gfw_TexMAN *sh2gfw_get_newMANID(struct sh2gfw_ALLTEXSYNC_MAN *pATSM) {
    struct sh2gfw_TexMAN *plc;
    struct sh2gfw_TexMAN *pUs;
    struct sh2gfw_TexMAN *pTp;

    plc = &pATSM->Empty_Head;
    pUs = &pATSM->Used_Head;
    if (plc != plc->pNext) {
        if (plc->pNext != plc) {
            pTp = plc->pNext;
            pTp->pPrev->pNext = pTp->pNext;
            pTp->pNext->pPrev = pTp->pPrev;
            pTp->pNext = pUs->pNext;
            pTp->pPrev = pUs;
            pUs->pNext->pPrev = pTp;
            pUs->pNext = pTp;
        }
    }
    return pTp;
}

/** Returns a texture manager to the free list and uncounts it; returns pDel. */
struct sh2gfw_TexMAN *sh2gfw_del_TexMAN(struct sh2gfw_ALLTEXSYNC_MAN *pATSM, struct sh2gfw_TexMAN *pDel) {
    struct sh2gfw_TexMAN *plc;
    struct sh2gfw_TexMAN *pUs;
    struct sh2gfw_TexMAN *pTp;

    plc = &pATSM->Empty_Head;
    pDel->pPrev->pNext = pDel->pNext;
    pDel->pNext->pPrev = pDel->pPrev;
    pDel->pPrev = plc;
    pDel->pNext = plc->pNext;
    plc->pNext->pPrev = pDel;
    plc->pNext = pDel;
    if (pDel->mode != 0xFFFF) {
        pATSM->alltexnum--;
    }
    switch (pDel->mode & 0xF000) {
    case 0x5000:
    case 0x6000:
    case 0x7000:
        if (pDel->mode >= 0x7000) {
            pATSM->g_BG--;
        } else if (pDel->mode < 7000 && pDel->mode >= 6000) {
            pATSM->st_BG--;
        } else {
            pATSM->l_BG--;
        }
        pATSM->alltex_BG--;
        break;
    case 0xD000:
        pATSM->fonttex--;
        break;
    case 0xE000:
        pATSM->alltex_EFF--;
        break;
    case 0xF000:
        break;
    default:
        if (pDel->mode >= 0x100 && pDel->mode < 0x12E) {
            pATSM->human_CHR--;
            pATSM->alltex_CHR--;
        } else if (pDel->mode >= 0x200 && pDel->mode < 0x212) {
            pATSM->en_CHR--;
            pATSM->alltex_CHR--;
        } else if (pDel->mode >= 0x300 && pDel->mode < 0x314) {
            pATSM->ura_CHR--;
            pATSM->alltex_CHR--;
        } else if (pDel->mode >= 0x700 && pDel->mode < 0x748) {
            pATSM->x_CHR--;
            pATSM->alltex_CHR--;
        } else if (pDel->mode >= 0x800 && pDel->mode < 0x82B) {
            pATSM->wp_CHR--;
            pATSM->alltex_CHR--;
        } else if (pDel->mode >= 0x500 && pDel->mode < 0x562) {
            pATSM->oS_CHR--;
            pATSM->alltex_CHR--;
        } else if (pDel->mode >= 0x400 && pDel->mode < 0x446) {
            pATSM->oA_CHR--;
            pATSM->alltex_CHR--;
        } else {
            pATSM->bg_CHR--;
            pATSM->alltex_CHR--;
        }
        break;
    }
    pDel->mode = 0xFFFF;
    return pDel;
}

static void sh2gfw_init_SyncTexTag(struct sh2gfw_TexMAN *sTM, struct sh2gfw_TEX_HEAD *pTexHead,
                                   struct sh2gfw_CLUTS_HEAD *pClutHead, void *pMAN, unsigned short mode) {
    unsigned int bw;
    unsigned int bh;
    unsigned int bw_64;
    unsigned int clm;
    unsigned int i;
    unsigned int dpsm;
    union Q_WORDDATA *pqwd;

    pqwd = &sTM->DMACNT;
    sTM->mark = 0xFFFF;
    sTM->mode = mode;
    sTM->Manage = pMAN;
    sTM->TexHead = pTexHead;
    sTM->ClutHead = pClutHead;
    sTM->tex = (char *)pTexHead + pTexHead->padbyte + sizeof(struct sh2gfw_TEX_HEAD);
    sTM->clut = (char *)pClutHead + sizeof(struct sh2gfw_CLUTS_HEAD);
    bw = pTexHead->w >> (pTexHead->bitshift ? 1 : 0);
    bw_64 = bw >> 6;
    bh = pTexHead->h >> pTexHead->bitshift;
    dpsm = pTexHead->drawpsm;

    sTM->DMACNT.ui32[0] = 0x10000007;
    sTM->DMACNT.ui32[1] = 0;
    sTM->DMACNT.ui32[2] = 0;
    sTM->DMACNT.ui32[3] = 0;
    sTM->GIFA_D_TEX.ui32[3] = 0;
    sTM->GIFA_D_TEX.ui32[2] = 0xE;
    sTM->GIFA_D_TEX.ui32[1] = 0x10000000;
    sTM->GIFA_D_TEX.ui32[0] = 0x8005;
    sTM->GS_LABEL.ul64[1] = 0x62;
    sTM->GS_LABEL.ui32[1] = 0xFFFF;
    sTM->GS_LABEL.ui32[0] = 0;
    sTM->GS_TEX_BITBLT.ul64[1] = 0x50;
    sTM->GS_TEX_BITBLT.ul64[0] = GS_SET_BITBLTBUF(0, bw_64, pTexHead->sendpsm, 0, bw_64, pTexHead->sendpsm);
    sTM->GS_TEX_TRXREG.ul64[1] = 0x51;
    sTM->GS_TEX_TRXREG.ul64[0] = 0;
    sTM->GS_TEX_TRXPOS.ul64[1] = 0x52;
    sTM->GS_TEX_TRXPOS.ui32[0] = bw;
    sTM->GS_TEX_TRXPOS.ui32[1] = bh;
    sTM->GS_TEX_TRXDIR.ul64[1] = 0x53;
    sTM->GS_TEX_TRXDIR.ul64[0] = 0;
    sTM->GIFIMAGE_TEX.ui32[3] = 0;
    sTM->GIFIMAGE_TEX.ui32[2] = 0;
    sTM->GIFIMAGE_TEX.ui32[1] = 0x8000000;
    sTM->GIFIMAGE_TEX.ui32[0] = (pTexHead->datasize >> 4) | 0x8000;
    sTM->DMAREF_TEXTRANS.ui32[0] = (pTexHead->datasize >> 4) | 0x30000000;
    sTM->DMAREF_TEXTRANS.ui32[1] = (unsigned int)sTM->tex & 0x7FFFFFFF;
    sTM->DMAREF_TEXTRANS.ui32[2] = 0;
    sTM->DMAREF_TEXTRANS.ui32[3] = 0;

    if ((dpsm & 0x13) == 0x13 || (dpsm & 0x14)) {
        pqwd[9].ui32[0] = 0x10000006;
        pqwd[9].ui32[1] = 0;
        pqwd[9].ui32[2] = 0;
        pqwd[9].ui32[3] = 0;
        pqwd[10].ui32[3] = 0;
        pqwd[10].ui32[2] = 0xE;
        pqwd[10].ui32[1] = 0x10000000;
        pqwd[10].ui32[0] = 0x8004;
        pqwd[11].ul64[1] = 0x50;
        pqwd[11].ul64[0] = GS_SET_BITBLTBUF(0, 1, 0, 0, 1, 0);
        pqwd[12].ul64[1] = 0x51;
        pqwd[12].ul64[0] = 0;
        pqwd[13].ul64[1] = 0x52;
        pqwd[13].ui32[0] = pClutHead->clw;
        pqwd[13].ui32[1] = pClutHead->clh;
        pqwd[14].ul64[1] = 0x53;
        pqwd[14].ul64[0] = 0;
        bw = pClutHead->clutssize >> 4;
        pqwd[15].ui32[3] = 0;
        pqwd[15].ui32[2] = 0;
        pqwd[15].ui32[1] = 0x8000000;
        pqwd[15].ui32[0] = bw | 0x8000;
        pqwd[16].ui32[0] = bw | 0x30000000;
        pqwd[16].ui32[1] = (unsigned int)((char *)pClutHead + sizeof(struct sh2gfw_CLUTS_HEAD)) & 0x7FFFFFFF;
        pqwd[16].ui32[2] = 0;
        pqwd[16].ui32[3] = 0;
        pqwd[17].ui32[0] = 0x30000002;
        pqwd[17].ui32[1] = (unsigned int)shGs_AllEnv.GsSync_DummyLABEL & 0x7FFFFFFF;
        pqwd[17].ui32[2] = 0;
        pqwd[17].ui32[3] = 0;
        pqwd[18].ui32[0] = 0x70000002;
        pqwd[18].ui32[1] = 0;
        pqwd[18].ul64[1] = 0;
        pqwd[19].ui32[3] = 0;
        pqwd[19].ui32[2] = 0xE;
        pqwd[19].ui32[1] = 0x10000000;
        pqwd[19].ui32[0] = 0x8001;
        pqwd[20].ul64[1] = 0x62;
        pqwd[20].ui32[1] = 0xFFFF0000;
        pqwd[20].ui32[0] = 0;
        clm = pClutHead->clutamount;
        pqwd = sTM->TEX0_for_CLUT;
        for (i = 0; i < clm; i++) {
            pqwd[0].ui32[3] = 0;
            pqwd[0].ui32[2] = 0xE;
            pqwd[0].ui32[1] = 0x10000000;
            pqwd[0].ui32[0] = 0x8001;
            pqwd += 3;
        }
    } else {
        sTM->DMACNT_CLUT.ui32[0] = 0x70000002;
        sTM->DMACNT_CLUT.ui32[1] = 0;
        sTM->DMACNT_CLUT.ul64[1] = 0;
        sTM->GIFA_D_CLUT.ui32[3] = 0;
        sTM->GIFA_D_CLUT.ui32[2] = 0xE;
        sTM->GIFA_D_CLUT.ui32[1] = 0x10000000;
        sTM->GIFA_D_CLUT.ui32[0] = 0x8001;
        sTM->GS_CLUT_BITBLT.ul64[1] = 0x62;
        sTM->GS_CLUT_BITBLT.ui32[1] = 0xFFFF0000;
        sTM->GS_CLUT_BITBLT.ui32[0] = 0;
    }
    sTM->GIFA_D_REGS.ui32[3] = 0;
    sTM->GIFA_D_REGS.ui32[2] = 0xE;
    sTM->GIFA_D_REGS.ui32[1] = 0x10000000;
    sTM->GIFA_D_REGS.ui32[0] = 0x8003;
    sTM->GS_TEXFLUSH.ul64[1] = 0x3F;
    sTM->GS_TEXFLUSH.ul64[0] = 0;
}

/** Makes a texture manager upload the image at texaddr instead of its own. */
void sh2gfw_Change_TexBody(void *pT, void *texaddr) {
    struct sh2gfw_TexMAN *pTM;

    pTM = pT;
    pTM->DMAREF_TEXTRANS.ui32[1] = (unsigned int)texaddr;
}

/** Makes a texture manager upload its own image again. */
void sh2gfw_Reset_TexBody(void *pT) {
    struct sh2gfw_TexMAN *pTM;

    pTM = pT;
    pTM->DMAREF_TEXTRANS.ui32[1] = (unsigned int)pTM->tex;
}

/**
 * Returns a TEX0 register value (A+D quadword) of a texture.
 * @param clutid CLUT number
 * @param flg    0: TEX0 for that CLUT; 1: the variant after it; other: the plain TEX0_1
 */
u_long128 *sh2gfw_Get_RegTEX0(struct sh2gfw_TexMAN *sTM, unsigned int clutid, unsigned int flg) {
    if (flg == 0) {
        return &sTM->TEX0_for_CLUT[clutid * 3].ul128;
    } else if (flg == 1) {
        return &(&sTM->TEX0_for_CLUT[clutid * 3])[1].ul128;
    } else {
        return &sTM->GS_TEX0_1.ul128;
    }
}

/** Returns the texture function (TFX) stored for CLUT clutid of a texture. */
int sh2gfw_Get_TFX(void *sTMV, int clutid) {
    struct sh2gfw_TexMAN *sTM;
    struct sh2gfw_CLUTS_HEAD *sCH;

    sTM = sTMV;
    sCH = sTM->ClutHead;
    return sCH->fmt[clutid];
}

/**
 * Queues a texture upload: picks a GS texture slot, points the packets at it and sends them on
 * DMA channel 2.
 * @param ptm    the struct sh2gfw_TexMAN
 * @param mode   passed to sh2gfw_SetSlot2Tex
 * @param cid    receives the transfer's command id
 * @param slotid receives the slot
 * @return the command id
 */
int sh2gfw_Thr_d2TextureSend(void *ptm, int mode, int *cid, int *slotid) {
    struct sh2gfw_TexMAN *pTM;
    int slot;
    int comid;

    pTM = ptm;
    slot = sh2gfw_EnQue_TexSlot(ptm);
    sh2gfw_SetSlot2Tex(pTM, slot, mode);
    d2tscBeginToUseSlot(slot);
    comid = d2tscSend(slot, (int)pTM, &pTM->DMACNT);
    *cid = comid;
    *slotid = slot;
    pTM->commandid = comid;
    pTM->slotid = slot;
    return comid;
}

/**
 * Sends a draw packet on DMA channel 1 once texture transfer cid is done, then frees its slot.
 * @return the channel-1 command id
 */
int sh2gfw_Thr_d1d2SyncKick(void *pkaddr, int cid, int slotid) {
    int d1cid;

    d1tscSync(cid);
    d1cid = d1cSend(pkaddr);
    d1tscFinishToUseSlot(slotid);
    return d1cid;
}

static int Fake_SetTexSlot(int sid, void *pk) {
    d2tscBeginToUseSlot(sid);
    return d2tscSend(sid, (int)pk + sid, pk);
}

/** Occupies texture slots 1-4 with dummy transfers, so the shadow pass can use their GS memory. */
void sh2gfw_Lock_AllTexSlot_For_Shadow(void) {
    int i;

    for (i = 1; i < 5; i++) {
        fake_cid[i] = Fake_SetTexSlot(i, &fake_kick);
    }
}

/** Releases the slots taken by sh2gfw_Lock_AllTexSlot_For_Shadow. */
void sh2gfw_UnLock_AllTexSlot_For_Shadow(void) {
    int i;

    for (i = 1; i < 5; i++) {
        sh2gfw_Thr_d1d2SyncKick(&fake_kick, fake_cid[i], i);
    }
}

/** Empties the texture slot queues. */
void sh2gfw_init_TexTrans_Manage_Table(void) {
    int i;

    for (i = 0; i < 5; i++) {
        TexTransTable.TexSlot_List[i].list_head = -1;
        TexTransTable.TexSlot_List[i].list_tail = 0;
        TexTransTable.TexSlot_List[i].last_mark = -1;
        TexTransTable.TexSlot_List[i].list_element[0].pTexMan = 0;
    }
    TexTransTable.mark_now = -1;
    TexTransTable.kick = 0;
}

/**
 * Picks the GS texture slot for a texture upload: the slot whose last texture is the same one,
 * else the least recently used slot, and records it.
 * @return the slot
 */
int sh2gfw_EnQue_TexSlot(void *pTexMan) {
    int i;
    int j;
    struct TexTable_List *tl;

    TexTransTable.TexKick_List[++TexTransTable.mark_now].pTexMan = pTexMan;
    for (i = 0; i < 5; i++) {
        tl = &TexTransTable.TexSlot_List[i];
        j = tl->list_tail ? tl->list_tail - 1 : 0;
        if (tl->list_element[j].pTexMan == pTexMan) {
            SetTexture2table(tl, pTexMan, TexTransTable.mark_now);
            TexTransTable.TexKick_List[TexTransTable.mark_now].SlotNo = i;
            TexTransTable.TexKick_List[TexTransTable.mark_now].Index = tl->list_tail - 1;
            break;
        }
    }
    if (i == 5) {
        j = GetTexList_MinMark();
        tl = &TexTransTable.TexSlot_List[j];
        SetTexture2table(tl, pTexMan, TexTransTable.mark_now);
        TexTransTable.TexKick_List[TexTransTable.mark_now].SlotNo = j;
        TexTransTable.TexKick_List[TexTransTable.mark_now].Index = tl->list_tail - 1;
    }
    return TexTransTable.TexKick_List[TexTransTable.mark_now].SlotNo;
}

static void SetTexture2table(struct TexTable_List *pTL, void *pTexMan, int mark) {
    int tail;

    tail = pTL->list_tail;
    pTL->list_element[tail].pTexMan = pTexMan;
    pTL->list_element[tail].mark = mark;
    pTL->last_mark = mark;
    pTL->list_tail++;
}

/**
 * Points a texture manager's packets at GS texture slot slotno: the BITBLTBUF destinations of
 * the image and CLUTs, and the TEX0 register values.
 * @param mode not used
 * @return the slot's texture base pointer
 */
int sh2gfw_SetSlot2Tex(struct sh2gfw_TexMAN *sTM, int slotno, int mode) {
    unsigned int bw;
    unsigned int bh;
    unsigned int bw_64;
    unsigned int tBP;
    unsigned int cBP;
    unsigned int kk;
    unsigned int i;
    unsigned int tfx;
    struct sh2gfw_TEX_HEAD *sTH;
    struct sh2gfw_CLUTS_HEAD *sCH;
    union Q_WORDDATA *pqwd;

    pqwd = &sTM->DMACNT;
    sTH = sTM->TexHead;
    sCH = sTM->ClutHead;
    bw = sTH->w >> (sTH->bitshift ? 1 : 0);
    bw_64 = bw >> 6;
    bh = sTH->h; /* Matching: dead; the DWARF has bh. Its value, by analogy with bw, is a guess. */
    slotno = 5 - slotno;
    tBP = sh2gfw_GetTexTBP0(&shGs_AllEnv, slotno);
    cBP = sh2gfw_GetSendingClutTBP0(&shGs_AllEnv, slotno);
    pqwd[2].ui32[0] = slotno;
    pqwd[3].ul64[1] = 0x50;
    pqwd[3].ul64[0] = GS_SET_BITBLTBUF(0, bw_64, sTH->sendpsm, tBP, bw_64, sTH->sendpsm);
    if ((sTH->drawpsm & 0x13) == 0x13 || (sTH->drawpsm & 0x14)) {
        pqwd[11].ul64[1] = 0x50;
        pqwd[11].ul64[0] = GS_SET_BITBLTBUF(0, 1, 0, cBP, 1, 0);
        pqwd[20].ui32[0] = slotno << 16;
        pqwd = &sTM->TEX0_for_CLUT[1];
        kk = sCH->clutamount;
        for (i = 0; i < kk; i++) {
            tfx = sCH->fmt[i];
            pqwd[0].ul64[1] = 6;
            pqwd[0].ul64[0] = GS_SET_TEX0(tBP, sTH->w >> 6, sTH->drawpsm, sTH->bitw, sTH->bith, 1, tfx, cBP, 0, 0,
                                              0, 1);
            pqwd[1].ul128 = pqwd[0].ul128;
            pqwd[1].ui32[1] &= ~0x18;
            pqwd[1].ui32[1] |= 0x18;
            pqwd += 3;
            cBP += 4;
        }
    } else {
        sTM->DMACNT_CLUT.ui32[0] = 0x70000002;
        sTM->DMACNT_CLUT.ui32[1] = 0;
        sTM->DMACNT_CLUT.ul64[1] = 0;
        sTM->GIFA_D_CLUT.ui32[3] = 0;
        sTM->GIFA_D_CLUT.ui32[2] = 0xE;
        sTM->GIFA_D_CLUT.ui32[1] = 0x10000000;
        sTM->GIFA_D_CLUT.ui32[0] = 0x8001;
        sTM->GS_CLUT_BITBLT.ul64[1] = 0x62;
        sTM->GS_CLUT_BITBLT.ui32[1] = 0xFFFF0000;
        sTM->GS_CLUT_BITBLT.ui32[0] = slotno << 16;
    }
    sTM->GS_TEX0_1.ul64[1] = 6;
    sTM->GS_TEX0_1.ul64[0] = GS_SET_TEX0(tBP, sTH->w >> 6, sTH->drawpsm, sTH->bitw, sTH->bith, 1, 0, cBP, 0, 0, 0, 0);
    sTM->GS_TEX0_2.ul64[1] = 7;
    sTM->GS_TEX0_2.ul64[0] = GS_SET_TEX0(tBP, sTH->w >> 6, sTH->drawpsm, sTH->bitw, sTH->bith, 1, 0, cBP, 0, 0, 0, 0);
    return tBP;
}

static int GetTexList_MinMark(void) {
    int i;
    int tmp;

    tmp = 0;
    for (i = 1; i < 5; i++) {
        if (TexTransTable.TexSlot_List[tmp].last_mark > TexTransTable.TexSlot_List[i].last_mark) {
            tmp = i;
        }
    }
    return tmp;
}
