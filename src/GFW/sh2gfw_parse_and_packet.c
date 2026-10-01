/*
 * Background block parsing and packet building (GFW). A loaded block holds, per texture,
 * GIF tag headers, VU headers and triangle-strip geometry; the parse functions record where
 * each part is, and the packet functions build the DMA chains that upload each texture, select
 * the VU1 microprogram and send the geometry, plus a separate chain for semi-transparent parts.
 *
 * Matching: the #line directives in this file keep the assert strings on the original's lines.
 */
/*
 * Matching: this file declares sh2gfw_packet_Poly_LocalTexUsing again below, as it defines it:
 * with pB_man a pointer to the aligned typedef (compatible with the generated prototype; the
 * alignment gives the spilled parameter a 16-byte stack slot).
 */
#include "sh2.h"

#include "sdk/libvu0.h"

/*
 * Matching: the original's typedefs were 16-byte aligned; spilled pointer parameters of these
 * types get 16-byte stack slots.
 */
typedef struct sh2gfw_AREA_MAN sh2gfw_AREA_MAN_a16 __attribute__((aligned(16)));
typedef struct sh2gfw_BLOCK_MAN sh2gfw_BLOCK_MAN_a16 __attribute__((aligned(16)));

int sh2gfw_packet_Poly_LocalTexUsing(sh2gfw_BLOCK_MAN_a16 *pB_man, struct sh2gfw_AREA_MAN *pAMAN,
                                     Q_WORDDATA *b_p, Q_WORDDATA *g_p);

#define ONE_BLOCK_TUPK 1024
#define ONE_BLOCK_GPK 6144

static int TransFormTileNo(int tileno);
static void PacketGIF_Pre(void *pt, struct sh2gfw_GIFTAG_HEAD *pGIF_H, Q_WORDDATA **ppqwd);
static void PacketGIF_Post(struct sh2gfw_GIFTAG_HEAD *pGIF_H, Q_WORDDATA **ppqwd);

static union Q_WORDDATA EM_env[6] = {
    {0x8003, 0x10000000, 0xE, 0},
    {0x54, 0x80, 0x42, 0},
    {0, 0, 0x3F, 0},
    {0, 0, 6, 0},
    {0x8002, 0x10000000, 0xE, 0},
    {0x44, 0x80, 0x42, 0},
};

int one_block_tupk = 0;
int one_block_gpk = 0;

static union Q_WORDDATA areabuf[2][17664] __attribute__((aligned(128)));
static union Q_WORDDATA SemiTrans_Packet_Buffer[5][2048];
static union Q_WORDDATA BLOCK_TEXUNIT_PACK[5][1024];
static union Q_WORDDATA GEOM_PACK[5][6144];
static union Q_WORDDATA BLOCK_MAIN_PACK[512];

/** Returns the texture-change packet buffer of background block slot bno. */
Q_WORDDATA *sh2gfw_Get_BTADDR(int bno) {
    return BLOCK_TEXUNIT_PACK[bno];
}

/** Returns the geometry packet buffer of background block slot gno. */
Q_WORDDATA *sh2gfw_Get_PKTADDR(int gno) {
    return GEOM_PACK[gno];
}

/** Returns the buffer for the area data file. */
Q_WORDDATA *sh2gfw_Get_BGAREABUF(void) {
    return areabuf[0];
}

/** Returns the buffer for the semi-transparent texture file. */
Q_WORDDATA *sh2gfw_Get_TransLocalBuf(void) {
    return areabuf[1];
}

/** Returns the background main packet buffer. */
Q_WORDDATA *sh2gfw_Get_BLOCKmainPack(void) {
    return BLOCK_MAIN_PACK;
}

/** Returns the semi-transparent packet buffer of background block slot bno. */
Q_WORDDATA *sh2gfw_Get_SemiTransPKADDR(int bno) {
    return SemiTrans_Packet_Buffer[bno];
}

/** Asserts that a block's packet sizes fit their buffers (pb texture-change, pg geometry quadwords). */
void sh2gfw_DBG_packsize(int pb, int pg) {
#line 144
    assert_dw(pb<ONE_BLOCK_TUPK);
    assert(pg<ONE_BLOCK_GPK);
}

/**
 * Parses a block's header: its local textures (registered in the texture manager pool), the
 * area's global texture if it uses it, and its local-to-world matrix.
 * @param d_h    the block data
 * @param pB_man the block manager to fill
 * @param pAMAN  the area manager
 * @param pATSM  the texture manager pool
 * @return the offset of the global-texture part (0: none)
 */
int sh2gfw_parse_HEAD(Q_WORDDATA *d_h, struct sh2gfw_BLOCK_MAN *pB_man, struct sh2gfw_AREA_MAN *pAMAN,
                      struct sh2gfw_ALLTEXSYNC_MAN *pATSM) {
    unsigned int itex;
    unsigned int id;
    Q_WORDDATA *datahead;
    Q_WORDDATA *qwdd;

    one_block_tupk = 0;
    one_block_gpk = 0;
    pB_man->pB_H = (struct sh2gfw_BLOCK_HEAD *)d_h;
    pB_man->texnum = pB_man->pB_H->texnum;
    if (pB_man->pB_H->divflg) {
        verbose(1, "mapdived !!!\n");
    }
    if (pB_man->pB_H->toGlobaldef) {
        pB_man->pLTEX_H[0] = pAMAN->global_tex;
        pB_man->pLCLUT_H[0] = pAMAN->global_clut;
    } else {
        pB_man->pLTEX_H[0] = 0;
        pB_man->pLCLUT_H[0] = 0;
    }
    for (itex = 0; itex < pB_man->texnum; itex++) {
        pB_man->pLTEX_H[itex + 1] = (struct sh2gfw_TEX_HEAD *)((char *)d_h + pB_man->pB_H->toLocalTex[itex]);
        pB_man->pLCLUT_H[itex + 1] = (struct sh2gfw_CLUTS_HEAD *)((char *)d_h + pB_man->pB_H->toLocalcluts[itex]);
        pB_man->pTexMAN[itex + 1] = sh2gfw_set_TexToTrasMan(pATSM, pB_man->pLTEX_H[itex + 1],
                                                            pB_man->pLCLUT_H[itex + 1], pB_man, (itex + 1) | 0x7000);
    }
    pB_man->p_Matrices = d_h + (pB_man->pB_H->toRawblockdataParms >> 4);
    qwdd = pB_man->p_Matrices;
    verbose(1, "MapOrigin = x=%f z=%f\n", qwdd[3].fl32[0], qwdd[3].fl32[2]);
    sceVu0CopyMatrix(pB_man->Local_World, (float (*)[4])pB_man->p_Matrices);
    sceVu0InversMatrix(pB_man->World_Local, pB_man->Local_World);
    pB_man->bitmsk_data.ui32[0] = 0x3F;
    pB_man->bitmsk_data.ui32[1] = 0xFFC0;
    pB_man->bitmsk_data.ui32[2] = 0x8000;
    pB_man->bitmsk_data.ui32[3] = 0x40800000;
    return pB_man->pB_H->toGlobaldef;
}

/**
 * Walks a block's global-texture part (and its semi-transparent part), recording the headers.
 * @return the size of what was walked, in quadwords
 */
int sh2gfw_parse_global(Q_WORDDATA *d_h, struct sh2gfw_BLOCK_MAN *pB_man, struct sh2gfw_AREA_MAN *pA_MAN) {
    unsigned int itex;
    unsigned int id;
    int igg;
    int ivv;
    Q_WORDDATA *datahead;
    int trnum;
    struct sh2gfw_VU_HEAD *pVU_H;
    struct sh2gfw_TRANSGEOM_HEAD *strge;
    struct sh2gfw_GEOM_HEAD *sge;

    igg = -1;
    datahead = d_h;
    pB_man->pBG_H = (struct sh2gfw_BLOCKGLOBAL_HEAD *)d_h;
    if (pB_man->pBG_H->gtexnum == 0) {
        pB_man->pTexMAN[0] = 0;
        pB_man->pLTEX_H[0] = 0;
        pB_man->pLCLUT_H[0] = 0;
        datahead += 1;
        verbose(1, "Not GlobalTex But SemiTrans\n");
    } else {
        pB_man->pTexMAN[0] = pA_MAN->gTexMAN;
        pB_man->pGSREGS_H[0] = (struct sh2gfw_GSREGS_HEAD *)(d_h + 1);
        datahead += 4;
        do {
            pB_man->pGIF_H[0][++igg] = (struct sh2gfw_GIFTAG_HEAD *)datahead;
            if (pB_man->pGIF_H[0][igg]->gsregs_amount) {
                datahead += (int)(pB_man->pGIF_H[0][igg]->gsregs_amount + 1) + 1;
            } else {
                datahead += 1;
            }
            ivv = -1;
            if (pB_man->pGIF_H[0][igg]->trans_flg) {
                pB_man->tr_gbl_gifnum++;
                do {
                    pVU_H = (struct sh2gfw_VU_HEAD *)datahead;
                    datahead += 2;
                    pB_man->vunum[0][igg]++;
                    pB_man->tr_gbl_vunum++;
                    verbose(1, "Global-Trans existed!\n");
                    do {
                        strge = (struct sh2gfw_TRANSGEOM_HEAD *)datahead;
                        datahead += strge->datasize >> 4;
                    } while (strge->toNextTRANSGEOM);
                } while (pVU_H->toNextVUPART);
            } else {
                do {
                    pVU_H = (struct sh2gfw_VU_HEAD *)datahead;
                    datahead += 2;
                    pB_man->vunum[0][igg]++;
                    pB_man->geom_amount[0][igg][++ivv] = 0;
                    do {
                        sge = (struct sh2gfw_GEOM_HEAD *)datahead;
                        pB_man->geom_amount[0][igg][ivv]++;
                        datahead += sge->datasize >> 4;
                    } while (sge->toNextGEOM);
                } while (pVU_H->toNextVUPART);
            }
        } while (!pB_man->pGIF_H[0][igg]->eop_flg);
        pB_man->gifnum[0] = igg + 1;
        if (pB_man->pGSREGS_H[0]->toNextDATA >= pB_man->pB_H->toLocaldef) {
            pB_man->tr_gifnum = 0;
            igg = datahead - d_h;
            return igg;
        }
    }
    verbose(1, "Semi-Trans Only Existed !!\n");
    trnum = sh2gfw_parse_SemiTransTex(datahead, pB_man);
    igg = trnum + (datahead - d_h);
    return igg;
}

/**
 * Walks a block's semi-transparent part, recording the headers.
 * @return its size in quadwords
 */
int sh2gfw_parse_SemiTransTex(Q_WORDDATA *datahead, struct sh2gfw_BLOCK_MAN *pB_man) {
    int trnum;
    int igg;
    Q_WORDDATA *d_h;
    struct sh2gfw_VU_HEAD *pVU_H;
    struct sh2gfw_TRANSGEOM_HEAD *strge;

    d_h = datahead;
    pB_man->pTR_GSREGS_H = (struct sh2gfw_GSREGS_HEAD *)datahead;
    trnum = -1;
    datahead += 3;
    do {
        pB_man->pTR_GIF_H[++trnum] = (struct sh2gfw_GIFTAG_HEAD *)datahead;
        if (pB_man->pTR_GIF_H[trnum]->gsregs_amount) {
            datahead += (int)(pB_man->pTR_GIF_H[trnum]->gsregs_amount + 1) + 1;
        } else {
            datahead += 1;
        }
        do {
            pVU_H = (struct sh2gfw_VU_HEAD *)datahead;
            datahead += 2;
            do {
                strge = (struct sh2gfw_TRANSGEOM_HEAD *)datahead;
                datahead += strge->datasize >> 4;
            } while (strge->toNextTRANSGEOM);
        } while (pVU_H->toNextVUPART);
    } while (!pB_man->pTR_GIF_H[trnum]->eop_flg);
    pB_man->tr_gifnum = trnum + 1;
    igg = datahead - d_h;
    return igg;
}

/**
 * Walks a block's local-texture parts, recording the headers.
 * @return their size in quadwords
 */
int sh2gfw_parse_local(Q_WORDDATA *d_h, struct sh2gfw_BLOCK_MAN *pB_man) {
    int ivv;
    int igg;
    unsigned int itex;
    Q_WORDDATA *datahead;
    struct sh2gfw_VU_HEAD *pVU_H;
    struct sh2gfw_GEOM_HEAD *sge;

    datahead = d_h;
    pB_man->pBL_H = (struct sh2gfw_BLOCKLOCAL_HEAD *)d_h;
    datahead += 1;
    for (itex = 1; itex < pB_man->pB_H->texnum + 1; itex++) {
        pB_man->pGSREGS_H[itex] = (struct sh2gfw_GSREGS_HEAD *)datahead;
        datahead += 3;
        igg = -1;
        do {
            pB_man->pGIF_H[itex][++igg] = (struct sh2gfw_GIFTAG_HEAD *)datahead;
            if (pB_man->pGIF_H[itex][igg]->gsregs_amount) {
                datahead += (int)(pB_man->pGIF_H[itex][igg]->gsregs_amount + 1) + 1;
            } else {
                datahead += 1;
            }
            ivv = -1;
            do {
                pVU_H = (struct sh2gfw_VU_HEAD *)datahead;
                pB_man->geom_amount[itex][igg][++ivv] = 0;
                datahead += 2;
                do {
                    sge = (struct sh2gfw_GEOM_HEAD *)datahead;
                    pB_man->geom_amount[itex][igg][ivv]++;
                    datahead += sge->datasize >> 4;
                } while (sge->toNextGEOM);
                pB_man->vunum[itex][igg]++;
            } while (pVU_H->toNextVUPART);
        } while (!pB_man->pGIF_H[itex][igg]->eop_flg);
        pB_man->gifnum[itex] = igg + 1;
    }
    return datahead - d_h;
}

static int TransFormTileNo(int tileno) {
    int ix;
    int iy;
    int gid;

    if (!(tileno & 0x8000)) {
        ix = tileno & 0xF;
        iy = tileno & 0xF0;
        if (ix >= 14 && ix < 16) {
            ix = 0;
        } else if (ix < 10 && ix >= 8) {
            ix = 7;
        }
        if (iy < 0xF1 && iy >= 0xE0) {
            iy = 0;
        } else if (iy >= 0x80 && iy < 0x91) {
            iy = 0x70;
        }
        gid = ix | (iy >> 4) << 3;
        if (gid > 63) {
            gid = 63;
        }
    } else {
        gid = (tileno & 0xFF) | 0x100;
    }
    return gid;
}

/**
 * Builds the packets of a block's global-texture geometry.
 * @param pB_man the block manager
 * @param pAMAN  the area manager (global texture)
 * @param b_p    texture-change packet buffer
 * @param g_p    geometry packet buffer
 * @param pb     receives the texture-change packet size in quadwords
 * @param pl     receives the geometry packet size in quadwords
 * @return the end of the texture-change packets
 */
int sh2gfw_packet_Poly_GlobalTexUsing(struct sh2gfw_BLOCK_MAN *pB_man, sh2gfw_AREA_MAN_a16 *pAMAN,
                                      Q_WORDDATA *b_p, Q_WORDDATA *g_p, int *pb, int *pl) {
    Q_WORDDATA *d_pt;
    Q_WORDDATA *b_pack;
    Q_WORDDATA *g_pack;
    unsigned int gnum;
    unsigned int vunum;
    int i;
    int j;
    int tslmax;
    int execaddr;
    int ivv;
    struct sh2gfw_GIFTAG_HEAD *pGIF_H;
    struct sh2gfw_VU_HEAD *pVU_H;
    struct sh2gfw_GEOM_HEAD *pGEOM_H;

    b_pack = b_p;
    g_pack = g_p;
    gnum = pB_man->gifnum[0];
    pB_man->pBlockPack[0] = b_p;
    for (i = 0; i < gnum; i++) {
        ivv = -1;
        pGIF_H = pB_man->pGIF_H[0][i];
        d_pt = (Q_WORDDATA *)pGIF_H;
        if (pGIF_H->trans_flg) {
            continue;
        }
        b_pack->ui32[0] = 0x30000002;
        b_pack->ui32[1] = (unsigned int)&pAMAN->gTexMAN->TEX0_for_CLUT[pGIF_H->id * 3];
        b_pack->ui32[2] = 0x11000000;
        b_pack->ui32[3] = 0x50000002;
        b_pack++;
        b_pack->ui32[0] = 0x10000002;
        b_pack->ui32[1] = 0;
        b_pack->ui32[2] = 0x11000000;
        b_pack->ui32[3] = 0x50000002;
        b_pack++;
        b_pack->ui32[3] = 0;
        b_pack->ui32[2] = 0xE;
        b_pack->ui32[1] = 0x10000000;
        b_pack->ui32[0] = 0x8001;
        b_pack++;
        b_pack->ul64[1] = 0x3F;
        b_pack->ul64[0] = 0;
        b_pack++;
        PacketGIF_Pre(pAMAN->gTexMAN, pGIF_H, &b_pack);
        d_pt += 1;
        vunum = pB_man->vunum[0][i];
        for (j = 0; j < vunum; j++) {
            pVU_H = (struct sh2gfw_VU_HEAD *)d_pt;
            pB_man->idVU_tag[0][i][++ivv] = b_pack - b_p;
            tslmax = sh2gfw_set_VuSend(pB_man->pTexMAN[0], pGIF_H, pVU_H, &execaddr, &b_pack);
            d_pt += 2;
            do {
                pGEOM_H = (struct sh2gfw_GEOM_HEAD *)d_pt;
                if (pGEOM_H->tileno & 0x8000) {
                    int gid;

                    gid = TransFormTileNo(pGEOM_H->tileno);
                    b_pack->ui32[0] = gid << 16 | 0x20000000;
                    b_pack->ui32[1] = (unsigned int)g_pack;
                    b_pack->ui32[2] = 0;
                    b_pack->ui32[3] = 0x1000101;
                    b_pack->ui32[2] = pGEOM_H->tileno;
                } else {
                    int gid;

                    gid = TransFormTileNo(pGEOM_H->tileno);
                    b_pack->ui32[0] = gid << 16 | 0x20000000;
                    b_pack->ui32[1] = (unsigned int)g_pack;
                    b_pack->ui32[2] = 0;
                    b_pack->ui32[3] = 0x1000101;
                }
                b_pack++;
                sh2gfw_Geom_MakePacket(pVU_H, pGEOM_H, tslmax, tslmax * 2 + 4, execaddr, &g_pack);
                g_pack[-1].ui32[0] = 0x20000000;
                g_pack[-1].ui32[1] = (unsigned int)b_pack;
                g_pack[-1].ul64[1] = 0;
                d_pt += pGEOM_H->datasize >> 4;
            } while (pGEOM_H->toNextGEOM);
        }
        PacketGIF_Post(pGIF_H, &b_pack);
    }
    b_pack->ui32[0] = 0x20000000;
    b_pack->ui32[1] = 0;
    b_pack->ul64[1] = 0;
    b_pack++;
    pB_man->bp_leng[0] = b_pack - pB_man->pBlockPack[0];
    *pb = b_pack - b_p;
    *pl = g_pack - g_p;
    one_block_tupk += *pb;
    one_block_gpk += *pl;
    return (int)b_pack;
}

/**
 * Builds the packets of a block's local-texture geometry, one texture after another.
 * @param pB_man the block manager
 * @param pAMAN  not used
 * @param b_p    texture-change packet buffer
 * @param g_p    geometry packet buffer
 */
int sh2gfw_packet_Poly_LocalTexUsing(sh2gfw_BLOCK_MAN_a16 *pB_man, struct sh2gfw_AREA_MAN *pAMAN,
                                     Q_WORDDATA *b_p, Q_WORDDATA *g_p) {
    Q_WORDDATA *d_pt;
    Q_WORDDATA *b_pack;
    Q_WORDDATA *g_pack;
    struct sh2gfw_TexMAN *pTM;
    unsigned int gnum;
    unsigned int vunum;
    int itex;
    int i;
    int j;
    int tslmax;
    int execaddr;
    int ivv;
    struct sh2gfw_GIFTAG_HEAD *pGIF_H;
    struct sh2gfw_VU_HEAD *pVU_H;
    struct sh2gfw_GEOM_HEAD *pGEOM_H;

    b_pack = b_p;
    g_pack = g_p;
    for (itex = 1; itex < pB_man->texnum + 1; itex++) {
        gnum = pB_man->gifnum[itex];
        /*
         * Matching: dead load. The original's line table has a statement here (line 839, between
         * gnum and pBlockPack) whose only surviving code is itex * 4 (computed and spilled before
         * b_pack is loaded). Which 4-byte array of pB_man it read is unknown (pLTEX_H, pLCLUT_H
         * and pGSREGS_H give the same code; pTexMAN and pBlockPack don't).
         */
        d_pt = (Q_WORDDATA *)pB_man->pLTEX_H[itex];
        pB_man->pBlockPack[itex] = b_pack;
        pTM = pB_man->pTexMAN[itex];
        for (i = 0; i < gnum; i++) {
            ivv = -1;
            pGIF_H = pB_man->pGIF_H[itex][i];
            d_pt = (Q_WORDDATA *)pGIF_H;
            b_pack->ui32[0] = 0x30000002;
            b_pack->ui32[1] = (unsigned int)&pTM->TEX0_for_CLUT[pGIF_H->id * 3];
            b_pack->ui32[2] = 0x11000000;
            b_pack->ui32[3] = 0x50000002;
            b_pack++;
            b_pack->ui32[0] = 0x10000002;
            b_pack->ui32[1] = 0;
            b_pack->ui32[2] = 0x11000000;
            b_pack->ui32[3] = 0x50000002;
            b_pack++;
            b_pack->ui32[3] = 0;
            b_pack->ui32[2] = 0xE;
            b_pack->ui32[1] = 0x10000000;
            b_pack->ui32[0] = 0x8001;
            b_pack++;
            b_pack->ul64[1] = 0x3F;
            b_pack->ul64[0] = 0;
            b_pack++;
            PacketGIF_Pre(pTM, pGIF_H, &b_pack);
            d_pt += 1;
            vunum = pB_man->vunum[itex][i];
            for (j = 0; j < vunum; j++) {
                pVU_H = (struct sh2gfw_VU_HEAD *)d_pt;
                pB_man->idVU_tag[itex][i][++ivv] = b_pack - pB_man->pBlockPack[itex];
                tslmax = sh2gfw_set_VuSend(pB_man->pTexMAN[itex], pGIF_H, pVU_H, &execaddr, &b_pack);
                d_pt += 2;
                do {
                    pGEOM_H = (struct sh2gfw_GEOM_HEAD *)d_pt;
                    if (pGEOM_H->tileno & 0x8000) {
                        int gid;

                        gid = TransFormTileNo(pGEOM_H->tileno);
                        b_pack->ui32[0] = gid << 16 | 0x20000000;
                        b_pack->ui32[1] = (unsigned int)g_pack;
                        b_pack->ui32[2] = 0;
                        b_pack->ui32[3] = 0x1000101;
                        b_pack->ui32[2] = pGEOM_H->tileno;
                    } else {
                        int gid;

                        gid = TransFormTileNo(pGEOM_H->tileno);
                        b_pack->ui32[0] = gid << 16 | 0x20000000;
                        b_pack->ui32[1] = (unsigned int)g_pack;
                        b_pack->ui32[2] = 0;
                        b_pack->ui32[3] = 0x1000101;
                    }
                    b_pack++;
                    sh2gfw_Geom_MakePacket(pVU_H, pGEOM_H, tslmax, tslmax * 2 + 4, execaddr, &g_pack);
                    g_pack[-1].ui32[0] = 0x20000000;
                    g_pack[-1].ui32[1] = (unsigned int)b_pack;
                    g_pack[-1].ul64[1] = 0;
                    d_pt += pGEOM_H->datasize >> 4;
                } while (pGEOM_H->toNextGEOM);
            }
            PacketGIF_Post(pGIF_H, &b_pack);
        }
        b_pack->ui32[0] = 0x20000000;
        b_pack->ui32[1] = 0;
        b_pack->ul64[1] = 0;
        b_pack++;
        pB_man->bp_leng[itex] = b_pack - pB_man->pBlockPack[itex];
    }
    one_block_tupk += b_pack - b_p;
    one_block_gpk += g_pack - g_p;
    sh2gfw_DBG_packsize(one_block_tupk, one_block_gpk);
    return (int)b_pack;
}

/**
 * Builds the packets of a block's semi-transparent geometry (global-texture parts first).
 * @return their size in quadwords
 */
int sh2gfw_packet_Poly_SemiTrans(struct sh2gfw_BLOCK_MAN *pB_man, Q_WORDDATA *b_p) {
    int i;
    int j;
    int kk = 0;
    Q_WORDDATA *head;
    struct sh2gfw_TRANSGEOM_HEAD *pTRGEOM_H;
    Q_WORDDATA *pHG;
    Q_WORDDATA *pHL;
    union Q_WORDDATA info = {0};
    struct sh2gfw_VU_HEAD *pVU_H;
    Q_WORDDATA *d_pt;

    pHG = b_p;
    if (pB_man->pTexMAN[0] && pB_man->tr_gbl_gifnum) {
        info.uc8[1] = 1;
        for (i = 0; i < pB_man->gifnum[0]; i++) {
            d_pt = (Q_WORDDATA *)pB_man->pGIF_H[0][i];
            if (((struct sh2gfw_GIFTAG_HEAD *)d_pt)->trans_flg) {
                kk++;
                info.uc8[0] = ((struct sh2gfw_GIFTAG_HEAD *)d_pt)->id;
                d_pt += 1;
                for (j = 0; j < pB_man->vunum[0][i]; j++) {
                    d_pt += 2;
                    do {
                        pTRGEOM_H = (struct sh2gfw_TRANSGEOM_HEAD *)d_pt;
                        sh2gfw_SemiTrans_GeomPacket(pTRGEOM_H, TransFormTileNo(pTRGEOM_H->tileno), &info, &pHG);
                        d_pt += pTRGEOM_H->datasize >> 4;
                    } while (pTRGEOM_H->toNextTRANSGEOM);
                }
                pHG->si32[0] = -1;
                pHG->si32[1] = 0;
                pHG++;
            }
        }
        if (kk) {
            pHG[-1].si32[1] = -1;
        } else {
            pHG->si32[1] = -1;
        }
    }
    if (pHG == b_p) {
        pB_man->pKT_GTR = 0;
    } else {
        pB_man->pKT_GTR = b_p;
    }
    pHL = pHG;
    info.uc8[1] = 0;
    for (i = 0; i < pB_man->tr_gifnum; i++) {
        head = (Q_WORDDATA *)pB_man->pTR_GIF_H[i];
        d_pt = head + 1;
        info.uc8[0] = ((struct sh2gfw_GIFTAG_HEAD *)head)->id;
        do {
            pVU_H = (struct sh2gfw_VU_HEAD *)d_pt;
            info.uc8[2] = pVU_H->vukind;
            d_pt += 2;
            do {
                pTRGEOM_H = (struct sh2gfw_TRANSGEOM_HEAD *)d_pt;
                sh2gfw_SemiTrans_GeomPacket(pTRGEOM_H, TransFormTileNo(pTRGEOM_H->tileno), &info, &pHG);
                d_pt += pTRGEOM_H->datasize >> 4;
            } while (pTRGEOM_H->toNextTRANSGEOM);
        } while (pVU_H->toNextVUPART);
        pHG->si32[0] = -1;
        pHG->si32[1] = 0;
        pHG++;
    }
    if (pB_man->tr_gifnum) {
        pHG[-1].si32[1] = -1;
    } else {
        pHG->si32[1] = -1;
    }
    if (pHL == pHG) {
        pB_man->pKT_LTR = 0;
    } else {
        pB_man->pKT_LTR = pHL;
    }
    if ((int)pHG - (int)b_p >= 0x200000) {
        printf("Hantoumei_Over!\n");
#line 1116
        assert_dw(0);
    }
    return pHG - b_p;
}

/** Sets the alpha blend of the environment-map pass for night or day. */
void sh2gfw_Set_SpecularAlpha(void) {
    if (sh2gfw_Get_NightOrDay()) {
        if (BgIsOut(0)) {
            EM_env[1].ul64[1] = 0x42;
            EM_env[1].ul64[0] = 0x8000000058;
        } else {
            EM_env[1].ul64[1] = 0x42;
            EM_env[1].ul64[0] = 0x8000000058;
        }
    } else {
        EM_env[1].ul64[1] = 0x42;
        EM_env[1].ul64[0] = 0x8000000054;
    }
}

static void PacketGIF_Pre(void *pt, struct sh2gfw_GIFTAG_HEAD *pGIF_H, Q_WORDDATA **ppqwd) {
    Q_WORDDATA *pqwd;
    u_long128 *pk;
    struct sh2gfw_TexMAN *pTM;

    pqwd = *ppqwd;
    pTM = pt;
#line 1247
    assert_dw(pGIF_H->gsregs_amount==0);
    EM_env[2].ul64[1] = 8;
    EM_env[2].ul64[0] = 5;
    EM_env[3].ul64[0] = sh2_SpecularMappingTEX0();
    pqwd[0].ui32[0] = 0x30000006;
    pqwd[0].ui32[1] = (unsigned int)EM_env & 0x7FFFFFFF;
    pqwd[0].ui32[2] = 0;
    pqwd[0].ui32[3] = 0;
    pqwd[0].ui32[2] = 0x1000101;
    pqwd[0].ui32[3] = 0x6C060042;
    pk = sh2gfw_Get_RegTEX0(pTM, pGIF_H->id, 1);
    pqwd[1].ui32[0] = 0x30000001;
    pqwd[1].ui32[1] = (unsigned int)pk & 0x7FFFFFFF;
    pqwd[1].ui32[2] = 0;
    pqwd[1].ui32[3] = 0;
    pqwd[1].ui32[2] = 0x1000101;
    pqwd[1].ui32[3] = 0x6C010048;
    pqwd[2].ui32[0] = 0x30000002;
    pqwd[2].ui32[1] = (unsigned int)&EM_env[4] & 0x7FFFFFFF;
    pqwd[2].ui32[2] = 0;
    pqwd[2].ui32[3] = 0;
    pqwd[2].ui32[2] = 0x1000101;
    pqwd[2].ui32[3] = 0x6C020049;
    pqwd[3].ui32[0] = 0x30000001;
    pqwd[3].ui32[1] = (unsigned int)(pk + 1) & 0x7FFFFFFF;
    pqwd[3].ui32[2] = 0;
    pqwd[3].ui32[3] = 0;
    pqwd[3].ui32[2] = 0x1000101;
    pqwd[3].ui32[3] = 0x6C01004B;
    *ppqwd = pqwd + 4;
}

static void PacketGIF_Post(struct sh2gfw_GIFTAG_HEAD *pGIF_H, Q_WORDDATA **ppqwd) {
    Q_WORDDATA *pqwd;
    u_long128 *pk;

    pqwd = *ppqwd;
    switch (pGIF_H->abe) {
    case 0:
    case 3:
    case 5:
        break;
    case 1:
    case 6:
        pk = sh2gfw_Get_FrameNormalRegAddr();
        break;
    default:
#line 1304
        assert_dw(0);
    }
    *ppqwd = pqwd;
}

/** Clears a block manager's VU/GIF bookkeeping, semi-transparent state and first texture/packet pointers. */
void sh2gfw_init_zeroQ_BMAN(struct sh2gfw_BLOCK_MAN *pB_man) {
    int i;
    u_long128 *pl;

    pl = (u_long128 *)pB_man->vunum;
    for (i = 0; i < 36; i += 4) {
        pl[i] = 0;
        pl[i + 1] = 0;
        pl[i + 2] = 0;
        pl[i + 3] = 0;
    }
    pB_man->pKT_GTR = 0;
    pB_man->pKT_LTR = 0;
    pB_man->tr_gbl_gifnum = 0;
    pB_man->tr_gifnum = 0;
    pB_man->tr_gbl_vunum = 0;
    pB_man->tr_vunum = 0;
    pB_man->view_clip_flg = 0;
    pB_man->view_tile = 0;
    *(u_long128 *)pB_man->pTexMAN = 0;
    *(u_long128 *)pB_man->pBlockPack = 0;
}
