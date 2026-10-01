/*
 * Character model data management (GFW): the ModelDW_Man table of loaded character models
 * (model, animation, cluster and shadow data per character id), loading them from disc, and
 * registering their textures (shared, mirrored and texture-less characters).
 *
 * Matching: the #line directives in this file keep the assert strings on the original's lines.
 */


#include "sh2.h"
#include "libc/string.h"


#define MODEL_ID 0xFFFF0003
#define MWORK_ID  0xFFFE0003

static int Delete_Model(void *modeldw);
static int Check_RevChara(int id);
static void init_CharaTex(struct sh2gfw_ALLTEXSYNC_MAN *pATSM, struct sh2gfw_ModelDraw_MAN *pMD);
static void init_ReverseCharaTex(struct sh2gfw_ALLTEXSYNC_MAN *pATSM, struct sh2gfw_ModelDraw_MAN *pMD);
static int Init_WithoutCharaTex(struct sh2gfw_ALLTEXSYNC_MAN *pATSM, struct sh2gfw_ModelDraw_MAN *pMD);

static int MDL_iniflg = 0;
u_long128 CHRDATA[524288];
static union Q_WORDDATA ANIME_DATA[1024] __attribute__((aligned(64)));
struct sh2gfw_ModelDraw_MAN ModelDW_Man[48];
struct sh2gfw_ModelDraw_MAN UniModelDW_Man;
struct sh2gfw_LoadModel_MEMMAN LoadModel_MemMan;
static struct sh2gfw_AllModelData_Man All_MDW;

/** Clears the first 32 model entries and the load bookkeeping. */
void sh2gfw_srInit_ModelDrawWork(void) {
    int i;

    for (i = 0; i < 32; i++) {
        memset(&ModelDW_Man[i], 0, sizeof(struct sh2gfw_ModelDraw_MAN));
    }
    memset(&All_MDW, 0, sizeof(struct sh2gfw_AllModelData_Man));
}

/** Returns the model entry of a character id, or NULL. */
struct sh2gfw_ModelDraw_MAN *sh2gfw_Get_pMD(int chara_id) {
    int i;
    int kind;
    int id;
    int cid;

    for (i = 0; i < 48; i++) {
        if (chara_id == ModelDW_Man[i].chara_id) {
            return &ModelDW_Man[i];
        }
    }
    return 0;
}

/** Returns 1 if character cid's model is loaded (its header and model ids check out), else 0. */
int sh2gfw_Check_ModelIsOnMemory(int cid) {
    struct sh2gfw_ModelDraw_MAN *pMD;
    struct sh2gfw_Model_Header *sMH;
    struct sh_Model *pModel;

    pMD = sh2gfw_Get_pMD(cid);
    if (pMD == 0) {
        return 0;
    }
    sMH = pMD->Model_Head;
    if (sMH == 0) {
        return 0;
    }
    if (sMH->chara_id != cid) {
        return 0;
    }
    pModel = (struct sh_Model *)((char *)sMH + sMH->toModel_offset);
    if (pModel->id != MODEL_ID && pModel->id != MWORK_ID) {
        return 0;
    }
    return 1;
}

/** Empty in this build. Declared int in the DWARF; returns nothing. */
int sh2gfw_LoadInit_CharaModelData(struct chr_mge_files *cdl) {
}

/**
 * Assigns a model entry to a character about to be loaded (reusing its entry, or for the boat
 * and weapon ranges any entry of the same range, else a free one) and queues it for loading.
 * Declared int in the DWARF; returns -1 when no entry is free, else nothing.
 * @param cdl              the character's files
 * @param ModelMemBuffer   model buffer (only the first is used)
 * @param AnimeMemBuffer   animation buffer (not used)
 * @param ClusterMemBuffer cluster buffer (not used)
 * @param Kg1MemBuffer     shadow data buffer (not used)
 */
int sh2gfw_LoadMemorySet_CharaModelData(struct chr_mge_files *cdl, u_long128 **ModelMemBuffer,
                                        u_long128 **AnimeMemBuffer, u_long128 **ClusterMemBuffer,
                                        u_long128 **Kg1MemBuffer) {
    int i;
    int k;
    u_long128 *modelbuf;
    u_long128 *animebuf;
    u_long128 *clusterbuf;
    u_long128 *kg1buf;
    struct sh2gfw_ModelDraw_MAN *pMD2;
    struct chr_mge_files *cdlist2;

    k = -1;
    modelbuf = *ModelMemBuffer;
    animebuf = *AnimeMemBuffer;
    clusterbuf = *ClusterMemBuffer;
    kg1buf = *Kg1MemBuffer;
    for (i = 0; i < 48; i++) {
        if (ModelDW_Man[i].chara_id == 0 && k == -1) {
            k = i;
        }
        if (cdl->mid >= 0x800 && cdl->mid < 0x820) {
            if (ModelDW_Man[i].chara_id >= 0x800 && ModelDW_Man[i].chara_id < 0x820) {
                break;
            }
        } else if (cdl->mid >= 0x820 && cdl->mid < 0x82A) {
            if (ModelDW_Man[i].chara_id >= 0x820 && ModelDW_Man[i].chara_id < 0x82A) {
                break;
            }
        } else if (cdl->mid == ModelDW_Man[i].chara_id) {
            break;
        }
    }
    if (i == 48) {
        if (k == -1) {
            return -1;
        }
        i = k;
    }
    cdlist2 = cdl; /* Matching: dead; the DWARF has cdlist2. Its value and place are a guess. */
    pMD2 = &ModelDW_Man[i];
    All_MDW.file_struct[All_MDW.n_load_character] = cdl;
    All_MDW.pMDM[All_MDW.n_load_character] = pMD2;
    pMD2->chara_id = cdl->mid;
    if (cdl->model_fid) {
#line 716
        assert_dw(modelbuf!=NULL);
        pMD2->Model_Head = modelbuf;
        pMD2->pModel_Header = pMD2->Model_Head;
    } else {
        pMD2->Model_Head = modelbuf;
        pMD2->pModel_Header = pMD2->Model_Head;
    }
    if (cdl->anime_fid) {
#line 729
        assert(animebuf!=NULL);
        pMD2->pAnime = animebuf;
    } else {
        pMD2->pAnime = animebuf;
    }
    if (cdl->cluster_fid) {
#line 745
        assert(clusterbuf!=NULL);
        pMD2->pCluster = clusterbuf;
    } else {
        pMD2->pCluster = clusterbuf;
    }
    if (cdl->shadow_fid) {
#line 756
        assert(kg1buf!=NULL);
        pMD2->pKg1Work = kg1buf;
    } else {
        pMD2->pKg1Work = kg1buf;
    }
    All_MDW.n_load_character++;
}

/** Starts reading the queued characters' model, animation, cluster and shadow files. Declared int; returns nothing. */
int sh2gfw_LOAD_CharaModelData(void) {
    int i;
    struct chr_mge_files *cdlist3;
    struct sh2gfw_ModelDraw_MAN *pMD3;

    for (i = 0; i < All_MDW.n_load_character; i++) {
        cdlist3 = All_MDW.file_struct[i];
        pMD3 = All_MDW.pMDM[i];
        if (cdlist3->model_fid) {
            All_MDW.fid_model[i] = FcRead(cdlist3->model_fid, pMD3->Model_Head);
        } else {
            All_MDW.fid_model[i] = -1;
        }
        if (cdlist3->anime_fid) {
            All_MDW.fid_anim[i] = FcRead(cdlist3->anime_fid, pMD3->pAnime);
        } else {
            All_MDW.fid_anim[i] = -1;
        }
        if (cdlist3->cluster_fid) {
            All_MDW.fid_clus[i] = FcRead(cdlist3->cluster_fid, pMD3->pCluster);
        } else {
            All_MDW.fid_clus[i] = -1;
        }
        if (cdlist3->shadow_fid) {
            All_MDW.fid_kg1[i] = FcRead(cdlist3->shadow_fid, pMD3->pKg1Work);
        } else {
            All_MDW.fid_kg1[i] = -1;
        }
    }
}

/**
 * Waits for the queued character files, sets up each model and its textures, and initializes
 * the model system on first use.
 */
void sh2gfw_SyncInit_ChacterModelData(void) {
    int i;
    int dac;
    int imdl2;
    int ianm2;
    int iclus2;
    int ikg12;
    struct chr_mge_files *cdlist;
    struct sh2gfw_ModelDraw_MAN *pMD4;

    dac = 0;
    for (i = 0; i < All_MDW.n_load_character; i++) {
        /* Matching: dead load; the DWARF has cdlist and the line table a statement here (only its i * 4
         * survives). */
        cdlist = All_MDW.file_struct[i];
        pMD4 = All_MDW.pMDM[i];
        if (All_MDW.fid_model[i] != -1) {
            fsSync(0, All_MDW.fid_model[i]);
            pMD4->sh_Model = (char *)pMD4->pModel_Header +
                             ((struct sh2gfw_Model_Header *)pMD4->pModel_Header)->toModel_offset;
            pMD4->chara_id = ((struct sh2gfw_Model_Header *)pMD4->pModel_Header)->chara_id;
            sh2gfw_init_CharaModel_TextureData(pMD4);
            dac++;
        } else if (pMD4->Model_Head) {
            pMD4->sh_Model = (char *)pMD4->pModel_Header +
                             ((struct sh2gfw_Model_Header *)pMD4->pModel_Header)->toModel_offset;
            pMD4->chara_id = ((struct sh2gfw_Model_Header *)pMD4->pModel_Header)->chara_id;
            sh2gfw_init_CharaModel_TextureData(pMD4);
        }
        if (All_MDW.fid_anim[i] != -1) {
            if (pMD4->chara_id == 0x800) {
                dac++;
            }
            fsSync(0, All_MDW.fid_anim[i]);
        }
        if (All_MDW.fid_clus[i] != -1) {
            fsSync(0, All_MDW.fid_clus[i]);
        }
        if (All_MDW.fid_kg1[i] != -1) {
            fsSync(0, All_MDW.fid_kg1[i]);
        }
    }
    All_MDW.n_load_character = 0;
    All_MDW.n_active_character += dac;
    if (All_MDW.n_active_character > 48) {
        All_MDW.n_active_character = 48;
    }
    if (!MDL_iniflg) {
        ModelInit();
        MDL_iniflg ^= 1;
    }
}

/** Drops the queued character loads, freeing their entries. Returns the number dropped. */
int sh2gfw_Cancel_LOADCharaModelData(void) {
    int i;

    for (i = 0; i < All_MDW.n_load_character; i++) {
#line 935
        assert_dw(All_MDW.pMDM[i]->chara_id!=0);
        All_MDW.pMDM[i]->chara_id = 0;
    }
    All_MDW.n_load_character = 0;
    return i;
}

/**
 * Sets up the model entry of a character whose data was loaded elsewhere (replacing its old
 * entry, else taking a free one) and registers its textures.
 * @param chara_id        character id
 * @param memhead_model   model data
 * @param memhead_anime   animation data
 * @param memhead_cluster cluster data
 * @param memhead_kg1     shadow data
 */
void sh2gfw_ModelDrawInit_for_BackgroundLoad(int chara_id, void *memhead_model, void *memhead_anime,
                                             void *memhead_cluster, void *memhead_kg1) {
    int i;
    int ze;
    struct sh2gfw_ModelDraw_MAN *pMD;

    ze = -1;
    for (i = 0; i < 48; i++) {
        pMD = &ModelDW_Man[i];
        if (chara_id == pMD->chara_id) {
            Delete_Model(pMD);
            ze = i;
            break;
        }
        if (ze == -1 && pMD->chara_id == 0) {
            ze = i;
        }
    }
#line 1044
    assert_dw(ze!=-1);
    pMD = &ModelDW_Man[ze];
    pMD->chara_id = chara_id;
    pMD->Model_Head = memhead_model;
    pMD->pModel_Header = memhead_model;
    pMD->sh_Model = (char *)pMD->pModel_Header + ((struct sh2gfw_Model_Header *)pMD->pModel_Header)->toModel_offset;
    pMD->pAnime = memhead_anime;
    pMD->pCluster = memhead_cluster;
    pMD->pKg1Work = memhead_kg1;
    sh2gfw_init_CharaModel_TextureData(pMD);
}

/** Creates a character (struct SubCharacter) from a loaded model, or returns NULL if it isn't loaded. */
void *sh2gfw_CreateSubCharacter(int chara_id) {
    struct sh2gfw_ModelDraw_MAN *pMD;
    struct SubCharacter *pSubc;

    pMD = sh2gfw_Get_pMD(chara_id);
    if (pMD == 0) {
        return 0;
    }
    pSubc = shCharacterCreate(0, (int)pMD->sh_Model, (int)pMD->pAnime, (int)pMD->pCluster, chara_id);
    SCSetModel(pSubc, (int)pMD->sh_Model, (int)pMD->pAnime);
    return pSubc;
}

static int Delete_Model(void *modeldw) {
    struct sh2gfw_ModelDraw_MAN *pMD;
    int i;
    int ret;

    pMD = modeldw;
    ret = pMD->chara_id;
#line 1123
    assert_dw(pMD->chara_id!=0);
    pMD->chara_id = 0;
    for (i = 0; i < pMD->TB_change_VU1num + pMD->TB_change_VU0num; i++) {
        if (sh2gfw_del_TexMAN(&AllTexSync_Man, pMD->pTexMAN[i]) == 0) {
#line 1142
            assert(0);
        }
    }
    return ret;
}

/** Frees a character's model entry and textures; returns its id, or -1 if it isn't loaded. */
int sh2gfw_Delete_Model_from_CharaID(int chara_id) {
    struct sh2gfw_ModelDraw_MAN *pMD;

    pMD = sh2gfw_Get_pMD(chara_id);
    if (pMD == 0 || pMD->sh_Model == 0) {
        return -1;
    }
    return Delete_Model(pMD);
}

/** Resets the model and animation load buffers ("kari": provisional). */
void sh2gfw_kari_clear_LM(void) {
    LoadModel_MemMan.index_model = 0;
    LoadModel_MemMan.pLM_head = CHRDATA;
    LoadModel_MemMan.index_anime = 0;
    LoadModel_MemMan.pLA_head = (u_long128 *)ANIME_DATA;
}

static int Check_RevChara(int id) {
    if (id < 0x12E) {
        return 1;
    }
    if (id == 0x443 || id == 0x444) {
        return 1;
    }
    if (id >= 0x800 && id < 0x82A) {
        return 1;
    }
    return 0;
}

/**
 * Registers a loaded model's textures: none for texture-less models, the base character's
 * for mirrored ones (the 0x20 bit of James/boat-type ids), else its own.
 */
void sh2gfw_init_CharaModel_TextureData(struct sh2gfw_ModelDraw_MAN *pMD) {
    struct sh2gfw_Model_Header *smh;

    smh = pMD->Model_Head;
    if (smh->NoTextureID) {
        printf("NoTex Character\n");
        Init_WithoutCharaTex(&AllTexSync_Man, pMD);
    } else if (!Check_RevChara(smh->chara_id)) {
        init_CharaTex(&AllTexSync_Man, pMD);
    } else if (smh->chara_id & 0x20) {
        printf("Mirror Character!\n");
        init_ReverseCharaTex(&AllTexSync_Man, pMD);
    } else {
        init_CharaTex(&AllTexSync_Man, pMD);
    }
}

/*
 * Records at which VU1 and VU0 parts a model's texture block changes (the TB_change_* and
 * TB_index_* tables). A macro: its loop variables are locals of each user in the DWARF.
 */
#define SET_TEXBLOCK_CHANGE(pMD, sMH, pModel)                                                         \
    pMD->TB_change_VU1num = 0;                                                                         \
    pMD->TB_change_VU1now = 0;                                                                         \
    pMD->TB_change_VU0num = 0;                                                                         \
    pMD->TB_change_VU0now = 0;                                                                         \
    if (sMH->texnum > 1) {                                                                             \
        n_parts = pModel->n_vu1_parts;                                                                 \
        part = parts_top = (struct Part *)((char *)pModel + pModel->vu1_parts_offset);                \
        buff = 0xFFFF;                                                                                 \
        for (i = 0; i < n_parts; i++, part = (struct Part *)((char *)part + part->size)) {            \
            pitex = (short *)((char *)part + part->text_pos_indices_offset);                          \
            pTP = (struct TextPos *)((char *)pModel + pModel->text_poses_offset);                     \
            texblk_index = pTP[*pitex].block_index;                                                    \
            if (texblk_index != buff) {                                                                \
                pMD->TB_change_VU1[pMD->TB_change_VU1num] = i;                                         \
                pMD->TB_index_VU1[pMD->TB_change_VU1num++] = texblk_index;                             \
                buff = texblk_index;                                                                   \
            }                                                                                          \
        }                                                                                              \
        pMD->TB_change_VU1[pMD->TB_change_VU1num] = n_parts;                                           \
        pMD->TB_index_VU1[pMD->TB_change_VU1num] = 0xFFFF;                                             \
        if (sMH->texnum > pMD->TB_change_VU1num) {                                                     \
            n_parts = pModel->n_vu0_parts;                                                             \
            part = parts_top = (struct Part *)((char *)pModel + pModel->vu0_parts_offset);            \
            for (i = 0; i < n_parts; i++, part = (struct Part *)((char *)part + part->size)) {        \
                pitex = (short *)((char *)part + part->text_pos_indices_offset);                      \
                pTP = (struct TextPos *)((char *)pModel + pModel->text_poses_offset);                 \
                texblk_index = pTP[*pitex].block_index;                                                \
                if (texblk_index != buff) {                                                            \
                    pMD->TB_change_VU0[pMD->TB_change_VU0num] = i;                                     \
                    pMD->TB_index_VU0[pMD->TB_change_VU0num++] = texblk_index;                         \
                    buff = texblk_index;                                                               \
                }                                                                                      \
            }                                                                                          \
        }                                                                                              \
        pMD->TB_change_VU0[pMD->TB_change_VU0num] = n_parts;                                           \
        pMD->TB_index_VU0[pMD->TB_change_VU0num] = 0xFFFF;                                             \
    } else {                                                                                           \
        pMD->TB_change_VU1[pMD->TB_change_VU1num++] = 0;                                               \
        pMD->TB_change_VU1[pMD->TB_change_VU1num] = pModel->n_vu1_parts;                               \
    }

static void init_CharaTex(struct sh2gfw_ALLTEXSYNC_MAN *pATSM, struct sh2gfw_ModelDraw_MAN *pMD) {
    struct sh_Model *pModel;
    struct sh2gfw_TEX_HEAD *pTH;
    struct sh2gfw_CLUTS_HEAD *pCH;
    struct sh2gfw_Model_Header *sMH;
    int i;
    int j;
    int mode;
    struct TextPos *pTP;
    short *pitex;
    int texblk_index;
    int n_parts;
    struct Part *parts_top;
    struct Part *part;
    int buff;

    sMH = pMD->Model_Head;
    pModel = (struct sh_Model *)((char *)sMH + sMH->toModel_offset);
#line 1463
    assert((pModel->id==MODEL_ID)||(pModel->id==MWORK_ID));
    for (i = 0; i < sMH->texnum; i++) {
        pTH = (struct sh2gfw_TEX_HEAD *)((char *)sMH + ((unsigned int *)(sMH + 1))[i]);
        pCH = (struct sh2gfw_CLUTS_HEAD *)((char *)sMH + ((unsigned int *)(sMH + 1))[i + sMH->texnum]);
        for (j = pCH->clutamount; j != 0; j--) {
            if (pCH->transparency[j - 1]) {
                break;
            }
        }
        /* Matching: dead; the DWARF has mode and the loop above only computes j. What mode was set to is a
         * guess. */
        mode = j;
        sMH->pTexMAN[i] = sh2gfw_set_TexToTrasMan(pATSM, pTH, pCH, sMH, sMH->chara_id);
        pModel->pTexMAN[i] = sMH->pTexMAN[i];
        pMD->pTexMAN[i] = sMH->pTexMAN[i];
    }
    SET_TEXBLOCK_CHANGE(pMD, sMH, pModel);
}

static void init_ReverseCharaTex(struct sh2gfw_ALLTEXSYNC_MAN *pATSM, struct sh2gfw_ModelDraw_MAN *pMD) {
    struct sh_Model *pModel;
    struct sh2gfw_TEX_HEAD *pTH;
    struct sh2gfw_CLUTS_HEAD *pCH;
    struct sh2gfw_Model_Header *sMH;
    struct sh2gfw_Model_Header *Real_sMH;
    struct sh2gfw_ModelDraw_MAN *Real_pMD;
    int i;
    int real;
    struct TextPos *pTP;
    short *pitex;
    int texblk_index;
    int n_parts;
    struct Part *parts_top;
    struct Part *part;
    int buff;

    sMH = pMD->Model_Head;
    pModel = (struct sh_Model *)((char *)sMH + sMH->toModel_offset);
#line 1930
    assert_dw((pModel->id==MODEL_ID)||(pModel->id==MWORK_ID));
    real = sMH->chara_id & ~0x20;
    Real_pMD = sh2gfw_Get_pMD(real);
#line 1935
    assert(Real_pMD!=NULL);
    Real_sMH = Real_pMD->Model_Head;
    for (i = 0; i < sMH->texnum; i++) {
        pTH = (struct sh2gfw_TEX_HEAD *)((char *)Real_sMH + ((unsigned int *)(Real_sMH + 1))[i]);
        pCH = (struct sh2gfw_CLUTS_HEAD *)((char *)Real_sMH +
                                           ((unsigned int *)(Real_sMH + 1))[i + Real_sMH->texnum]);
        sMH->pTexMAN[i] = sh2gfw_set_TexToTrasMan(pATSM, pTH, pCH, sMH, sMH->chara_id);
        pModel->pTexMAN[i] = sMH->pTexMAN[i];
        pMD->pTexMAN[i] = sMH->pTexMAN[i];
    }
    SET_TEXBLOCK_CHANGE(pMD, sMH, pModel);
}

static int Init_WithoutCharaTex(struct sh2gfw_ALLTEXSYNC_MAN *pATSM, struct sh2gfw_ModelDraw_MAN *pMD) {
    int chara[2];
    int cid;
    int i;
    struct sh_Model *pModel;
    struct sh2gfw_ModelDraw_MAN *TexPmd;
    struct sh2gfw_Model_Header *sMH;
    struct sh2gfw_Model_Header *Real_sMH;
    struct sh2gfw_TEX_HEAD *pTH;
    struct sh2gfw_CLUTS_HEAD *pCH;
    int difpth;
    int difpch;
    struct TextPos *pTP;
    short *pitex;
    int texblk_index;
    int n_parts;
    struct Part *parts_top;
    struct Part *part;
    int buff;

    TexPmd = 0;
    sMH = pMD->Model_Head;
    pModel = (struct sh_Model *)((char *)sMH + sMH->toModel_offset);
    switch (sMH->chara_id) {
    case 0x102:
    case 0x103:
        cid = 2;
        chara[0] = 0x100;
        chara[1] = 0x101;
        break;
    case 0x121:
    case 0x122:
    case 0x123:
        cid = 2;
        chara[0] = 0x100;
        chara[1] = 0x101;
        break;
    case 0x106:
        cid = 1;
        chara[0] = 0x105;
        break;
    case 0x127:
        cid = 1;
        chara[0] = 0x107;
        break;
    case 0x108:
        cid = 1;
        chara[0] = 0x205;
        break;
    case 0x205:
        cid = 1;
        chara[0] = 0x108;
        break;
    case 0x443:
    case 0x444:
    case 0x820:
    case 0x821:
    case 0x822:
    case 0x823:
    case 0x824:
    case 0x825:
    case 0x826:
    case 0x827:
    case 0x828:
    case 0x829:
        cid = 1;
        chara[0] = sMH->chara_id - 0x20;
        break;
    default:
#line 2100
        assert_dw(0);
    }
    for (i = 0; i < cid; i++) {
        TexPmd = sh2gfw_Get_pMD(chara[i]);
        if (TexPmd) {
            break;
        }
    }
    if (TexPmd == 0) {
        printf("Shared texture model Initialize  failure!\n");
#line 2111
        assert(0);
    }
    Real_sMH = TexPmd->Model_Head;
    for (i = 0; i < Real_sMH->texnum; i++) {
        pTH = (struct sh2gfw_TEX_HEAD *)((unsigned int)Real_sMH + ((unsigned int *)((char *)Real_sMH + sizeof(struct sh2gfw_Model_Header)))[i]);
        pCH = (struct sh2gfw_CLUTS_HEAD *)((unsigned int)Real_sMH +
                                           ((unsigned int *)((char *)Real_sMH + sizeof(struct sh2gfw_Model_Header)))[i + Real_sMH->texnum]);
        sMH->pTexMAN[i] = sh2gfw_set_TexToTrasMan(pATSM, pTH, pCH, sMH, sMH->chara_id);
        pModel->pTexMAN[i] = sMH->pTexMAN[i];
        pMD->pTexMAN[i] = sMH->pTexMAN[i];
    }
    for (i = Real_sMH->texnum; i < sMH->texnum; i++) {
        pTH = (struct sh2gfw_TEX_HEAD *)((unsigned int)sMH + ((unsigned int *)((char *)sMH + sizeof(struct sh2gfw_Model_Header)))[i]);
        pCH = (struct sh2gfw_CLUTS_HEAD *)((unsigned int)sMH + ((unsigned int *)((char *)sMH + sizeof(struct sh2gfw_Model_Header)))[i + sMH->texnum]);
        difpth = Real_sMH->toTexHead_offset + ((char *)pTH - (char *)sMH) - sMH->toTexHead_offset;
        difpch = Real_sMH->toClutsHead_offset + ((char *)pCH - (char *)sMH) - sMH->toClutsHead_offset;
        pTH = (struct sh2gfw_TEX_HEAD *)((unsigned int)Real_sMH + difpth);
        pCH = (struct sh2gfw_CLUTS_HEAD *)((unsigned int)Real_sMH + difpch);
#line 2144
        assert_dw(pTH->check==0x9999);
        sMH->pTexMAN[i] = sh2gfw_set_TexToTrasMan(pATSM, pTH, pCH, sMH, sMH->chara_id);
        pModel->pTexMAN[i] = sMH->pTexMAN[i];
        pMD->pTexMAN[i] = sMH->pTexMAN[i];
        printf("caution! shared texture amount is not equal\n");
    }
    SET_TEXBLOCK_CHANGE(pMD, sMH, pModel);
    return 1;
}
