/*
 * Background data loading (GFW): reads a stage's area data and semi-transparent texture file,
 * registers the area, block-local and semi-transparent textures, and turns a loaded background
 * block into its VU1 packets.
 */
#include "sh2.h"
#include "libc/string.h"

/** Registers the texture of a loaded semi-transparent texture file (a struct sh2gfw_AREA_HEAD) in sh2_TR_MAN. */
void sh2gfw_Set_TrTex(void *pT) {
    struct sh2gfw_AREA_HEAD *pah;
    int *mp;
    int tmp;

    pah = pT;
    mp = Get_NowMapId();
    tmp = ((*mp & 0xFF0000) >> 12) | (*mp & 0xFF);
    sh2_TR_MAN.p_TRTexHead = (struct sh2gfw_TEX_HEAD *)((char *)pah + pah->toGlobalTexHead);
    sh2_TR_MAN.p_TRClutHead = (struct sh2gfw_CLUTS_HEAD *)((char *)pah + pah->toGlobalClutsHead);
    sh2_TR_MAN.p_TRTexMan = sh2gfw_set_TexToTrasMan(&AllTexSync_Man, sh2_TR_MAN.p_TRTexHead, sh2_TR_MAN.p_TRClutHead, &sh2_TR_MAN, tmp | 0x7000);
    sh2_TR_MAN.blockid = 1;
}

/**
 * Loads a map's semi-transparent texture file into the transfer buffer and hands it to the
 * background texture loader.
 * @param mapid the map
 * @param fl    the file (union fsFileIndex)
 * @return the file size, or -1 if it is too small (under 0x40000 bytes)
 */
int sh2gfw_LoadSet_SemiTransTEX(int mapid, void *fl) {
    int sz;
    int fid;
    union fsFileIndex *file;
    void *addr;
    int stg;
    int size;

    file = fl;
    loadBgTEX_Replace(1, 0, NULL, 0);
    size = FcGetFileSize(file);
    if (size < 0x40000) {
        if (size > 0) {
            printf("waning ! too TR-TEX small.\n");
        }
        return -1;
    }
    addr = sh2gfw_Get_TransLocalBuf();
    fid = FcRead(file, addr);
    if (fid == -1) {
        /* The message has the original's file name and line number (300) written into it. */
        printf("sh2gfw_read_process.c:300> TR tex load command failed.\n");
    } else {
        fsSync(0, fid);
        loadBgTEX_Replace(1, mapid, addr, size);
    }
    return size;
}

/** Reads a stage's area data file into the background area buffer (and waits for it). */
void sh2gfw_LOAD_AREADATA_ID(unsigned int stage) {
    int fid;
    struct FilesBgBlock *bg;
    union fsFileIndex *file;

    bg = FilesGetBgBlock(stage, 0);
    file = bg->map;
    if (file != NULL) {
        fid = FcRead(file, sh2gfw_Get_BGAREABUF());
        if (fid != -1) {
            fsSync(0, fid);
        }
    }
}

/** Sets up an area manager from loaded area data d_h and registers its global texture. Returns 0. */
int sh2gfw_Process_AREAtoMAN(union Q_WORDDATA *d_h, struct sh2gfw_AREA_MAN *pAMAN, struct sh2gfw_ALLTEXSYNC_MAN *pATSM) {
    union Q_WORDDATA *datahead;
    int *mp;
    int tmp;

    mp = Get_NowMapId();
    tmp = ((*mp & 0xFF0000) >> 12) | (*mp & 0xFF);
    datahead = d_h; /* Matching: dead; the DWARF has datahead (sh2gfw_parse_global starts the same way). */
    pAMAN->pA_H = (struct sh2gfw_AREA_HEAD *)d_h;
    pAMAN->global_tex = (struct sh2gfw_TEX_HEAD *)((char *)d_h + pAMAN->pA_H->toGlobalTexHead);
    pAMAN->global_clut = (struct sh2gfw_CLUTS_HEAD *)((char *)d_h + pAMAN->pA_H->toGlobalClutsHead);
    pAMAN->gTexMAN = sh2gfw_set_TexToTrasMan(pATSM, pAMAN->global_tex, pAMAN->global_clut, pAMAN, tmp | 0x6000);
    return 0;
}

/** Loads the area data of map_id's stage and hands it to the background texture loader. Returns map_id. */
unsigned int sh2gfw_process_AreaDATA(unsigned int map_id) {
    loadBgTEX_Replace(0, 0, NULL, 0);
    sh2gfw_LOAD_AREADATA_ID((unsigned short)(map_id >> 16));
    loadBgTEX_Replace(0, map_id, sh2gfw_Get_BGAREABUF(), 0x40001);
    return map_id;
}

/** Releases the local texture managers of a background block. */
void sh2gfw_Free_BlockLocalTex(struct sh2gfw_ALLTEXSYNC_MAN *pATSM, struct sh2gfw_BLOCK_MAN *pB_man) {
    int i;
    struct sh2gfw_TexMAN **pTexMAN;

    pTexMAN = (struct sh2gfw_TexMAN **)pB_man->pTexMAN;
    for (i = 1; i <= pB_man->texnum; i++) {
        sh2gfw_del_TexMAN(pATSM, pTexMAN[i]);
        pTexMAN[i] = NULL;
    }
}

/** Clears the semi-transparent texture manager (sh2_TR_MAN). */
void sh2gfw_AllClear_TrMAN(void) {
    memset(&sh2_TR_MAN, 0, sizeof(sh2_TR_MAN));
}

/**
 * Turns a loaded background block into its packets: parses the header and the global,
 * semi-transparent and local texture parts, then builds the polygon packets (global-texture,
 * local-texture and semi-transparent).
 * @param datahead the block data
 * @param pB_man   the block manager to fill
 * @param pA_man   the area manager
 * @param b_pack   buffer for the texture-change packets
 * @param g_pack   buffer for the geometry packets
 * @param t_pack   buffer for the semi-transparent packets
 * @param stid     stored as the block id (the caller passes BgIsOut of the stage)
 * @param mapid    the block's map id
 * @return 1
 */
unsigned int sh2gfw_process_blockLOCAL_main(union Q_WORDDATA *datahead, struct sh2gfw_BLOCK_MAN *pB_man, struct sh2gfw_AREA_MAN *pA_man, union Q_WORDDATA *b_pack, union Q_WORDDATA *g_pack, union Q_WORDDATA *t_pack, int stid, int mapid) {
    int ib;
    int ig;
    int il;
    int pb;
    int pg;
    union Q_WORDDATA *qwd;
    /* Matching: called without a prototype, with pA_man as well (the DWARF lacks that unused parameter). */
    int sh2gfw_packet_Poly_LocalTexUsing();

    pb = 0;
    pg = 0;
    sh2gfw_Init_VuCounter();
    ib = sh2gfw_parse_HEAD(datahead, pB_man, pA_man, &AllTexSync_Man);
    if (ib != 0) {
        verbose(1, "global_tex_use!\n");
        ib >>= 4;
        ig = sh2gfw_parse_global(&datahead[ib], pB_man, pA_man);
    } else if (pB_man->pB_H->transtexnum != 0) {
        verbose(1, "Only TransTex_use!\n");
        if (sh2_TR_MAN.blockid != 0) {
            ig = sh2gfw_parse_SemiTransTex(&datahead[ib], pB_man);
        } else {
            ig = pB_man->pB_H->toLocaldef >> 4;
        }
    } else {
        verbose(1, "Only_LocalTex_use!\n");
        ig = pB_man->pB_H->toLocaldef >> 4;
    }
    sh2gfw_parse_local(&datahead[ig + ib], pB_man);
    if (ib != 0) {
        if (pB_man->pBG_H->gtexnum != 0) {
            sh2gfw_packet_Poly_GlobalTexUsing(pB_man, pA_man, b_pack, g_pack, &pb, &pg);
        }
    }
    sh2gfw_packet_Poly_LocalTexUsing(pB_man, pA_man, &b_pack[pb], &g_pack[pg]);
    il = sh2gfw_packet_Poly_SemiTrans(pB_man, t_pack);
    verbose(1, "0x%08X 0x%08X \n", ig << 4, il << 4);
    pB_man->blockid = stid;
    pB_man->pB_H->block_id = mapid;
    sh2gfw_Check_ChrClip_FLG(mapid);
    return 1;
}

/** Sets up the area manager (global background texture) from loaded area data. */
void sh2gfw_Set_GB_Tex(void *addr) {
    struct sh2gfw_AREA_MAN *pA_man;

    pA_man = &Area_Data_Man;
    sh2gfw_util_zeroq((union Q_WORDDATA *)pA_man, sizeof(Area_Data_Man) / 16);
    sh2gfw_Process_AREAtoMAN(addr, pA_man, &AllTexSync_Man);
}

/** Releases the area's global texture and clears the area manager. */
void sh2gfw_Delete_GB_Tex(void) {
    struct sh2gfw_AREA_MAN *pA_man;

    pA_man = &Area_Data_Man;
    sh2gfw_del_TexMAN(&AllTexSync_Man, pA_man->gTexMAN);
    sh2gfw_util_zeroq((union Q_WORDDATA *)pA_man, sizeof(Area_Data_Man) / 16);
}

/** Sets up the semi-transparent texture manager from a loaded file. */
void sh2gfw_Set_TR_Tex(void *addr) {
    sh2gfw_util_zeroq((union Q_WORDDATA *)&sh2_TR_MAN, sizeof(sh2_TR_MAN) / 16);
    sh2gfw_Set_TrTex(addr);
}

/** Releases the semi-transparent texture and clears its manager. */
void sh2gfw_Delete_TR_Tex(void) {
    sh2gfw_del_TexMAN(&AllTexSync_Man, sh2_TR_MAN.p_TRTexMan);
    sh2gfw_util_zeroq((union Q_WORDDATA *)&sh2_TR_MAN, sizeof(sh2_TR_MAN) / 16);
}

/** Sets up background block slot from the block data at addr (map mapid). */
void sh2gfw_Set_BlockLocal(int slot, void *addr, int mapid) {
    int isout;
    struct sh2gfw_AREA_MAN *pA_man;
    struct sh2gfw_BLOCK_MAN *pB_man;

    pB_man = &b_man[slot];
    isout = BgIsOut((unsigned short)(mapid >> 16));
    sh2gfw_util_zeroq((union Q_WORDDATA *)pB_man, sizeof(struct sh2gfw_BLOCK_MAN) / 16);
    pA_man = &Area_Data_Man;
    sh2gfw_process_blockLOCAL_main(addr, pB_man, pA_man, sh2gfw_Get_BTADDR(slot), sh2gfw_Get_PKTADDR(slot), sh2gfw_Get_SemiTransPKADDR(slot), isout, mapid);
    sh2gfw_Incliment_VertNumIndices();
}

/** Releases background block slot and its textures. */
void sh2gfw_Delete_BlockLocal(int slot) {
    struct sh2gfw_BLOCK_MAN *pB_man;

    pB_man = &b_man[slot];
    sh2gfw_Free_BlockLocalTex(&AllTexSync_Man, pB_man);
    sh2gfw_util_zeroq((union Q_WORDDATA *)pB_man, sizeof(struct sh2gfw_BLOCK_MAN) / 16);
}
