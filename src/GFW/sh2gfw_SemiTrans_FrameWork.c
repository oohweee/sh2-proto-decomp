/*
 * Semi-transparent background geometry (GFW): VU0 transforms the semi-transparent triangle
 * strips of each visible tile into scratchpad memory, and the CPU turns every triangle into a
 * GIF packet appended to the spack packet buffer, textured from the semi-transparent texture.
 */
#include "sh2.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "sdk/eekernel.h"

/* VU0 microcode images (data segment). */
extern u_long128 D_002A38E0[];
extern u_long128 D_002A2F70[];


static struct sh2gfw_Vu0_TransMan Vu0TrMan;
static int Spr_WorkTop;
static struct sh2gfw_Vif1Pkbuf NoSortPkbuf;
static int vu0tospr_page = 0;
static int pktmp_page = 0;
static struct SemiTrans_SprWork *St_SprW = (struct SemiTrans_SprWork *)0x70002000;

static void Init_Vu0TrMan(void *pk) {
    memset(&Vu0TrMan, 0, sizeof(Vu0TrMan));
    Vu0TrMan.Qaddr = pk;
    Vu0TrMan.Qbase = pk;
    Vu0TrMan.Qinfo = pk;
}

static void Vu0Kick(void *tag) {
    while (*D0_CHCR & 0x100) {
    }
    *D0_TADR = (unsigned int)tag & 0x0FFFFFFF;
    *D0_QWC = 0;
    *D0_CHCR = 0x145;
}

/** Takes the next VIF1 packet buffer for unsorted packets (flushed from the cache, used uncached). */
void sh2gfw_Init_NoSortPkbuf(void) {
    memset(&NoSortPkbuf, 0, sizeof(NoSortPkbuf));
    NoSortPkbuf.pkTop = ktVif1PkBufNext();
    SyncDCache(NoSortPkbuf.pkTop, (char *)NoSortPkbuf.pkTop + 0x20000);
    InvalidDCache(NoSortPkbuf.pkTop, (char *)NoSortPkbuf.pkTop + 0x20000);
    NoSortPkbuf.pkTop = (void *)((unsigned int)NoSortPkbuf.pkTop | 0x20000000);
    NoSortPkbuf.pTail = NoSortPkbuf.pkTop;
}

static void Init_Vu04SemiTrans(int arg0) {
    static union Q_WORDDATA LoadVu0[8];
    union Q_WORDDATA *qwd;

    qwd = LoadVu0;
    if (sh2gfw_Get_NightOrDay()) {
        qwd[0].ui32[1] = (unsigned int)D_002A38E0;
    } else {
        qwd[0].ui32[1] = (unsigned int)D_002A2F70;
    }
    qwd[0].ui32[0] = 0x50000000;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0;
    qwd[1].ui32[0] = 0x30000010;
    qwd[1].ui32[1] = (unsigned int)&VU1_PARMS & 0x7FFFFFFF;
    qwd[1].ui32[2] = 0;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0x1000101;
    qwd[1].ui32[3] = 0x6C100010;
    qwd[2].ui32[0] = 0x3000000A;
    qwd[2].ui32[1] = (unsigned int)&VU1_PARMS.GifTag_mskRGB & 0x7FFFFFFF;
    qwd[2].ui32[2] = 0;
    qwd[2].ui32[3] = 0;
    qwd[2].ui32[2] = 0x1000101;
    qwd[2].ui32[3] = 0x6C0A0038;
    qwd[3].ul128 = 0;
    qwd[3].ui32[0] = 0x70000000;
    SyncDCache(LoadVu0, qwd + 4);
    Vu0Kick(LoadVu0);
}

static void SetDrawParmsToVu0(struct sh2gfw_BLOCK_MAN *pB_man) {
    static union Q_WORDDATA LoadMatrixVU0[8];
    union Q_WORDDATA *qwd;

    qwd = (union Q_WORDDATA *)LoadMatrixVU0;
    LoadMatrixVU0[0].ui32[0] = 0x3000000C;
    LoadMatrixVU0[0].ui32[1] = (unsigned int)pB_man->Local_World & 0x7FFFFFFF;
    LoadMatrixVU0[0].ui32[2] = 0;
    LoadMatrixVU0[0].ui32[3] = 0;
    LoadMatrixVU0[0].ui32[2] = 0x1000101;
    LoadMatrixVU0[0].ui32[3] = 0x6C0C0000;
    qwd[1].ui32[0] = 0x30000012;
    qwd[1].ui32[1] = (unsigned int)&pB_man->blk_LightData & 0x7FFFFFFF;
    qwd[1].ui32[2] = 0;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0x1000101;
    qwd[1].ui32[3] = 0x6C120028;
    qwd[2].ui32[0] = 0x10000000;
    qwd[2].ui32[1] = 0;
    qwd[2].ui32[2] = 0x14000004;
    qwd[2].ui32[3] = 0;
    qwd[3].ul128 = 0;
    qwd[3].ui32[0] = 0x70000000;
    qwd[3].ui32[2] = 0x1000202;
    SyncDCache(LoadMatrixVU0, qwd + 4);
    Vu0Kick(LoadMatrixVU0);
}

static int Kick_Vu0TransCalcPacket(void) {
    union Q_WORDDATA *qwd;

    qwd = Vu0TrMan.Qaddr;
    while (*D0_CHCR & 0x100) {
    }
    while (*VIF0_STAT & 0x0F000003) {
    }
    *D0_QWC = 0;
    *D0_TADR = (unsigned int)qwd;
    Vu0TrMan.Qaddr = qwd + 2;
    Vu0TrMan.tsleng[vu0tospr_page] = qwd[1].ui32[1];
    while (*D9_CHCR & 0x100) {
    }
    *D0_CHCR = 0x145;
    vu0tospr_page ^= 1;
    return qwd[1].uc8[3] & 1;
}

static void MakePacketTemplate(struct sh2gfw_TexMAN *pT) {
    union Q_WORDDATA *worktop;
    struct SH2_SemiTrans_Triangle *stg;
    u_long128 *ul;

    St_SprW->info = *(union Q_WORDDATA *)Vu0TrMan.Qinfo;
    stg = St_SprW->sstg;
    ul = sh2gfw_Get_RegTEX0(pT, St_SprW->info.us16[0], 1);
    stg->tex0.ul128 = *ul;
    stg->alpha.ul64[1] = 0x42;
    stg->alpha.ul64[0] = 0x8000000044;
    stg->giftag.ul64[1] = 0x412412412EE;
    stg->giftag.ul64[0] = 0xB03DC00000008001;
    St_SprW->sstg[1] = *stg;
}

static u_long128 sh2_PMAXW(u_long128 rs, u_long128 rt);
static void *spkSetFakeData128(int dlen);
static void spkSetData128P(u_long128 *dat);

static void TrimSet_Packet_SprtoMemBuff(void) {
    union Q_WORDDATA *top1;
    union Q_WORDDATA *top2;
    union Q_WORDDATA maxw;
    int i;
    int tsleng;
    struct SH2_SemiTrans_Triangle *stg;
    union Q_WORDDATA *top0;
    struct SH2_SemiTrans_Triangle *pkSpr;

    stg = St_SprW->sstg;
    top0 = (union Q_WORDDATA *)(Spr_WorkTop + 0x60);
    top1 = top0 - 3;
    top2 = top0 - 6;
    tsleng = Vu0TrMan.tsleng[vu0tospr_page];
    for (i = 2; i < tsleng; i++, top0 += 3) {
        if (!(top0[2].ui32[3] & 0x8000)) {
            maxw.ul128 = sh2_PMAXW(top0->ul128, top1->ul128);
            maxw.ul128 = sh2_PMAXW(maxw.ul128, top2->ul128);
            pkSpr = &stg[pktmp_page ^= 1];
            spkOpenGiftag(pkSpr, maxw.ui32[3], 3);
            spkSetData128P(&pkSpr->tex0.ul128);
            spkSetData128P(&pkSpr->alpha.ul128);
            while (*D8_CHCR & 0x100) {
            }
            *D8_SADR = (unsigned int)top2 & 0x7FFF;
            *D8_QWC = 9;
            *D8_MADR = (unsigned int)spkSetFakeData128(9) & 0x0FFFFFFF;
            *D8_CHCR = 0x100;
            spkCloseGiftag();
        }
        top2 = top1;
        top1 = top0;
    }
}

static void *spkSetFakeData128(int dlen) {
    void *top;

    top = (void *)((unsigned int)spack.pos & 0x0FFFFFFF);
    spack.pos = (unsigned long *)((u_long128 *)spack.pos + dlen);
    return top;
}

static void spkSetData128P(u_long128 *dat) {
    *((u_long128 *)spack.pos)++ = *dat;
}

/* Per-word signed maximum of two quadwords (MMI pmaxw); MWCC C has no 128-bit operators. */
static u_long128 sh2_PMAXW(u_long128 rs, u_long128 rt) {
    u_long128 rd;

    __asm__ __volatile__("pmaxw %0, %1, %2" : "=r"(rd) : "r"(rs), "r"(rt));
    return rd;
}

static void Transfer_Vu0toSpr(void) {
    int work_sadr;

    work_sadr = vu0tospr_page << 12;
    Spr_WorkTop = work_sadr + 0x70000000;
    while (*D0_CHCR & 0x100) {
    }
    while (*VIF0_STAT & 0x0F000003) {
    }
    while (*D9_CHCR & 0x100) {
    }
    *D9_SADR = work_sadr;
    *D9_MADR = 0x11004920;
    *D9_QWC = 0x6E;
    *D9_CHCR = 0x101;
}

static int Vu0ModeBuffer;

static int Change_Vu0CalcMode(void) {
    static union Q_WORDDATA Vu0_pkbuf[4] __attribute__((aligned(64)));
    union Q_WORDDATA *qwd;
    union Q_WORDDATA *pK;
    int mode;

    qwd = Vu0TrMan.Qaddr;
    Vu0TrMan.Qaddr = qwd + 1;
    Vu0TrMan.Qinfo = qwd;
    if (Vu0ModeBuffer == qwd->uc8[2]) {
        return 1;
    }
    pK = (union Q_WORDDATA *)((unsigned int)Vu0_pkbuf | 0x20000000);
    pK->ui32[0] = 0x70000000;
    pK->ui32[1] = 0;
    pK->ui32[2] = 0x1000202;
    switch (qwd->uc8[2]) {
    case 8:
        pK->ui32[3] = 0x14000006;
        Vu0ModeBuffer = 8;
        break;
    case 9:
        pK->ui32[3] = 0x14000008;
        Vu0ModeBuffer = 9;
        break;
    case 0:
    default:
        pK->ui32[3] = 0x14000002;
        Vu0ModeBuffer = 0;
        break;
    }
    while (*D0_CHCR & 0x100) {
    }
    while (*VIF0_STAT & 0x0F000003) {
    }
    Vu0Kick(Vu0_pkbuf);
    return 1;
}

static int Check_ViewClip(struct sh2gfw_BLOCK_MAN *pB_man, int gid, int divflg);

static int CheckGeom(struct sh2gfw_BLOCK_MAN *pB_m) {
    union Q_WORDDATA *qif;
    int divflg;

    qif = Vu0TrMan.Qinfo;
    divflg = pB_m->pB_H->divflg;
    while (!Check_ViewClip(pB_m, qif->us16[2], divflg)) {
        qif = (union Q_WORDDATA *)qif->ui32[2];
        if (qif->si32[0] == -1) {
            if (qif->si32[1] == -1) {
                return 0;
            }
            qif++;
            Vu0TrMan.Qinfo = qif;
            Vu0TrMan.Qaddr = qif;
        }
    }
    Vu0TrMan.Qinfo = qif;
    Vu0TrMan.Qaddr = qif;
    return 1;
}

static int Check_ViewClip(struct sh2gfw_BLOCK_MAN *pB_man, int gid, int divflg) {
    unsigned char *taif;
    int ret;

    if (gid & 0x100) {
        ret = pB_man->ObjCondition & (1 << (gid & 0xFF));
    } else if (divflg) {
        taif = (unsigned char *)pB_man->tileViewClipInfo;
        ret = taif[gid] - 0x10;
    } else {
        ret = 1;
    }
    return ret;
}

static int Next_Info(void) {
    union Q_WORDDATA *qif;

    qif = Vu0TrMan.Qinfo;
    qif = (union Q_WORDDATA *)qif->ui32[2];
    if (qif->si32[0] == -1) {
        qif++;
    }
    Vu0TrMan.Qinfo = qif;
    return qif[-1].si32[1];
}

static void Exec_Calc_Sort(void *pTexMAN) {
    int flg;

    flg = Kick_Vu0TransCalcPacket();
    MakePacketTemplate(pTexMAN);
    while (flg == 0) {
        Transfer_Vu0toSpr();
        flg = Kick_Vu0TransCalcPacket();
        TrimSet_Packet_SprtoMemBuff();
    }
    Transfer_Vu0toSpr();
    vu0tospr_page ^= 1;
    while (*D9_CHCR & 0x100) {
    }
    TrimSet_Packet_SprtoMemBuff();
    while (*D8_CHCR & 0x100) {
    }
}

static void sh2gfw_Draw_SemiTexUsing(struct sh2gfw_BLOCK_MAN *pB_man) {
    if (pB_man->pKT_GTR || pB_man->pKT_LTR) {
        SetDrawParmsToVu0(pB_man);
        Vu0ModeBuffer = -1;
        if (pB_man->pKT_GTR) {
            Init_Vu0TrMan(pB_man->pKT_GTR);
            while (CheckGeom(pB_man)) {
                Change_Vu0CalcMode();
                Exec_Calc_Sort(pB_man->pTexMAN[0]);
                if (Next_Info() == -1) {
                    break;
                }
            }
        }
        if (pB_man->pKT_LTR && sh2_TR_MAN.p_TRTexMan) {
            Init_Vu0TrMan(pB_man->pKT_LTR);
            while (CheckGeom(pB_man)) {
                Change_Vu0CalcMode();
                Exec_Calc_Sort(sh2_TR_MAN.p_TRTexMan);
                if (Next_Info() == -1) {
                    break;
                }
            }
        }
    }
}

static int Check_UseTransTex(void) {
    int slot;
    int ret;
    int gbl;
    int lca;

    ret = 0;
    gbl = 0;
    lca = 0;
    for (slot = 0; slot < 5; slot++) {
        if (b_man[slot].pB_H) {
            gbl += b_man[slot].tr_gbl_gifnum;
            lca += b_man[slot].tr_gifnum;
        }
    }
    if (gbl) {
        ret |= 1;
    }
    if (lca) {
        ret |= 2;
    }
    return ret;
}

/**
 * Draws the semi-transparent parts of every loaded background block that has any (local
 * semi-transparent texture only; a global one is an assertion failure).
 * @return bit 0: blocks use the global texture, bit 1: blocks use the local one
 */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 1166
int sh2gfw_Draw_SemiTransBG(void) {
    int tnum;
    int comid;
    int slotid;
    char buf[16];
    int slot;

    tnum = Check_UseTransTex();
    if (tnum & 1) {
        printf("Global_Semitrans Used!\n");
        assert(0);
    }
    if (tnum & 2) {
        if (sh2_TR_MAN.p_TRTexMan) {
            sh2gfw_EnQue_spkTexture(sh2_TR_MAN.p_TRTexMan, &comid, &slotid);
        } else if (shPadGetPort() == 2 || shPadGetPort() == 3) {
            sprintf(buf, "%s", "No SemiTex!");
            shDBG_print_string(buf, 8, 0x42);
        }
    }
    sh2gfw_Store_Perf2(*T0_COUNT, 4);
    if (tnum) {
        Init_Vu04SemiTrans(0);
        sh2gfw_Init_NoSortPkbuf();
        for (slot = 0; slot < 5; slot++) {
            if (b_man[slot].pB_H) {
                sh2gfw_Draw_SemiTexUsing(&b_man[slot]);
            }
        }
    }
    sh2gfw_Store_Perf2(*T0_COUNT, 5);
    return tnum;
}
