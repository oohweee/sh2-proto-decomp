/*
 * Character drawing driver (Chacter_Draw): per frame, runs the character logic and animation,
 * then draws every visible character with its model data (ModelDW_Man) and its shadow, after
 * a distance/view-cone clip test. Also the DMA channel-1 synchronization of the character
 * packets and the texture-noise effect.
 */
#include "sh2.h"
#include "sh_vu0.h"
#include "libc/string.h"
#include "sdk/libvu0.h"

static float chr_clip_cos = 0.15f;

static struct CheckDrawChara_Work CheckCD_W;
static struct sh2gfw_Chara_SyncCid Chr_SyncCid;
union Q_WORDDATA dummy_OTFake_packet[4] __attribute__((aligned(64)));

/* Returns 1 / sqrt(x) (rsqrt.s). Inline asm in the original; no other file has it. */
static inline float _shRsqrt(float x) {
    float r;

    __asm__ __volatile__("
    lui     t7, 0x3F80
    mtc1    t7, %0
    rsqrt.s %0, %0, %1
    " : "=f"(r) : "f"(x));
    return r;
}

/* Matching: an inline function (name ours); written in place, sh2_Model_SyncOT and
 * sh2gfw_Thr_Chracter_d1d2SyncKick don't match. */
static inline void *PhysAddr(void *p) {
    return (void *)((unsigned int)p & 0x0FFFFFFF);
}

static void Init_CheckDrawChara_Work(void) {
    vwGetViewPosition(CheckCD_W.camera_pos);
    sh2gde_getCameraDir(CheckCD_W.camera_dir);
    CheckCD_W.farz = Env_ctl.camera_parms2[2];
    CheckCD_W.scz = VbScreenInfo.scr_z;
    CheckCD_W.hsx = 2048.0f;
    CheckCD_W.clip_cs = CheckCD_W.scz * _shRsqrt(CheckCD_W.hsx * CheckCD_W.hsx + CheckCD_W.scz * CheckCD_W.scz);
}

static int Check_Draw_Chara(struct SubCharacter *scp) {
    float vec[4];
    float dir[4];
    float len;
    float inp;

    if (!sh2gfw_Get_ChrClip_FLG()) {
        return 1;
    }
    if (!sh2gfw_Check_ClipOKChar(scp)) {
        return 1;
    }
    _shSubVector(vec, (float *)&scp->pos, CheckCD_W.camera_pos);
    len = _shVectorLength(vec);
    if (!(len <= 500.0f + CheckCD_W.farz)) {
        return 0;
    }
    if (len < 2000.0f) {
        if (scp->kind < 0x12E) {
            sceVu0ScaleVector(vec, vec, 1.0f / len);
            inp = _shInnerProduct(vec, CheckCD_W.camera_dir);
            if (inp < chr_clip_cos) {
                if (BgIsOut(0)) {
                    return 0;
                }
                return 1;
            }
            return 1;
        }
        return 1;
    }
    _shNormalize(dir, vec);
    inp = _shInnerProduct(dir, CheckCD_W.camera_dir);
    if (!(inp < CheckCD_W.clip_cs)) {
        return 1;
    }
    return 0;
}

static void sh2gfw_init_Chara_SyncCid(void) {
    memset(&Chr_SyncCid, 0, sizeof(Chr_SyncCid));
}

/** Records the channel-1 DMA id of the last character packet in a two-entry ring. */
void sh2gfw_set_d1cid(void) {
    Chr_SyncCid.SyncCid[Chr_SyncCid.page_id] = UniModelDW_Man.d1cid;
    Chr_SyncCid.page_id ^= 1;
    Chr_SyncCid.counter++;
}

/** Waits for the character DMA recorded two sh2gfw_set_d1cid calls ago, once there is one. */
void sh2gfw_sync_d1cid(void) {
    if (Chr_SyncCid.counter >= 2) {
        d1sSync(0, Chr_SyncCid.SyncCid[Chr_SyncCid.page_id]);
    } else {
        return; /* Matching: an explicit return in the original. */
    }
}

static void sh2gfw_Draw_Character_Pre(void) {
    sh2gfw_init_Chara_SyncCid();
    sh2_Model_MakeMatrixParams();
    Model3DrawPre();
    sh2gfw_set_CommonCharacterLight();
    Init_CheckDrawChara_Work();
}

static void sh2_CharacterDrawOne(void *vscp, struct FMAT *ws_mat) {
    struct SubCharacterDisp *scp_d;
    struct SubCharacter *scp;
    struct FMAT *mwm;
    int i;
    int n_skeletons;
    struct FMAT *mat;
    struct shSkelton *sp;

    scp = vscp;
    mwm = &scp->mat;
    scp_d = vscp;
    if (scp_d->models[0] == NULL || scp_d->work == NULL) {
        return;
    }
    sp = scp_d->anime.top;
    if (sp != NULL) {
        n_skeletons = Model3NSkeletons(scp_d->models[0]);
        mat = (struct FMAT *)Model3WorkMatrices(scp_d->work);
        for (i = 0; i < n_skeletons; i++) {
            mat[i] = sp->src_m;
            sp = sp->next;
        }
        Model3Draw_n(scp_d, scp_d->models[0], scp_d->work, (float (*)[4])mwm, (float (*)[4])ws_mat, 0);
    }
}

static void sh2gfw_Character_DrawOne(void *subchar, struct sh2gfw_ModelDraw_MAN *pMD, float (*wld_scr)[4]) {
    struct SubCharacter *pSubc;

    pSubc = subchar;
    UniModelDW_Man = *pMD;
    UniModelDW_Man.nowtex = 0;
    UniModelDW_Man.testSubChar = pSubc;
    sh2gfw_change_lights(UniModelDW_Man.Model_Head);
    sh2gfw_sync_d1cid();
    sh2_CharacterDrawOne(pSubc, (struct FMAT *)wld_scr);
}

/** Runs character logic, enemy tasks, animation and battle/collision checks for the frame, with timer marks. */
void sh2gfw_Exec_AnimeAndCollision(void) {
    clFrameInitCollisionData();
    shBattleInit();
    shCharacterExecFunctionAll();
    enExecTask();
    clCollectCharaPosition();
    sh2gfw_Store_Perf2(*T0_COUNT, 12);
    shCharacterExecAnimeAll();
    sh2gfw_Store_Perf2(*T0_COUNT, 10);
    shBattleExec();
    clBattleCheckExec();
    sh2gfw_Store_Perf2(*T0_COUNT, 13);
}

static inline int IsEnemyKind(int kind) {
    if (kind >= 0x104 && kind <= 0x1FF) {
        return 1;
    }
    return 0;
}

static inline int IsNpcKind(int kind) {
    if (kind >= 0x200 && kind <= 0x2FF) {
        return 1;
    }
    return 0;
}

static inline int IsBossKind(int kind) {
    if (kind >= 0x400 && kind <= 0x5FF) {
        return 1;
    }
    return 0;
}

/* Matching: ret is never set to 0 below (nor in IsBoatKind); the original has no store either. */
static inline int IsJamesKind(int kind) {
    int ret;

    if (kind >= 0x100 && kind <= 0x103) {
        ret = 1;
    }
    return ret;
}

static inline int IsBoatKind(int kind) {
    int ret;

    if (kind >= 0x800 && kind <= 0x829) {
        ret = 1;
    }
    return ret;
}

/**
 * Draws every visible character that has model data, plus shadows when comm is set.
 * @param wld_scr world-to-screen matrix
 * @param comm    non-zero to draw the shadows too (enemies, NPCs, bosses: stencil shadows from
 *                the strongest shadow light; James and the boat: Draw_Jms_Shadow)
 */
void sh2gfw_Exec_Character_Draw(float (*wld_scr)[4], int comm) {
    int i;
    int imax;
    float spot_pos[4];
    float spot_dir[4];
    float spot_pam[4];
    struct SubCharacter *scp;
    int ix;

    sh2gfw_Draw_Character_Pre();
    imax = 32;
    for (scp = sh2chara.head; scp != NULL; scp = scp->next) {
        if (scp->status & 0x10) {
            for (i = 0; i < imax; i++) {
                if (scp->kind == ModelDW_Man[i].chara_id && Check_Draw_Chara(scp)) {
                    sh2gfw_Character_DrawOne(scp, &ModelDW_Man[i], wld_scr);
                    if (comm && (IsEnemyKind(scp->kind) || IsNpcKind(scp->kind) || IsBossKind(scp->kind))) {
                        if (ModelDW_Man[i].pKg1Work) {
                            ix = sh2gfw_Get_ShadowLight(0, spot_pos, spot_dir, spot_pam, scp);
                            if (ix >= 0) {
                                sh2shd_Draw_ShadowChar(scp, ModelDW_Man[i].pKg1Work, scp->kind, scp->id, ix, spot_pos, spot_dir, spot_pam);
                            }
                        }
                    } else if (comm && (IsJamesKind(scp->kind) || IsBoatKind(scp->kind))) {
                        Draw_Jms_Shadow(0, &ModelDW_Man[i], scp);
                    }
                }
            }
        }
    }
    ModelDrawPost();
    sh2gfw_Store_Perf2(*T0_COUNT, 3);
}

/** Draws only the shadows of the visible characters (shadow-light mode 1). */
void sh2gfw_Exec_Character_Draw_ShadowOnly(void) {
    int i;
    int imax;
    float spot_pos[4];
    float spot_dir[4];
    float spot_pam[4];
    struct SubCharacter *scp;
    int ix;

    imax = 32;
    for (scp = sh2chara.head; scp != NULL; scp = scp->next) {
        if (scp->status & 0x10) {
            for (i = 0; i < imax; i++) {
                if (scp->kind == ModelDW_Man[i].chara_id && Check_Draw_Chara(scp)) {
                    if (IsEnemyKind(scp->kind) || IsNpcKind(scp->kind) || IsBossKind(scp->kind)) {
                        if (ModelDW_Man[i].pKg1Work) {
                            ix = sh2gfw_Get_ShadowLight(1, spot_pos, spot_dir, spot_pam, scp);
                            if (ix >= 0) {
                                sh2shd_Draw_ShadowChar(scp, ModelDW_Man[i].pKg1Work, scp->kind, scp->id, ix, spot_pos, spot_dir, spot_pam);
                            }
                        }
                    } else if (IsJamesKind(scp->kind) || IsBoatKind(scp->kind)) {
                        Draw_Jms_Shadow(1, &ModelDW_Man[i], scp);
                    }
                }
            }
        }
    }
}

/**
 * Sends the current character's packet on DMA channel 1: through the texture-sync kick for
 * each VU0 texture change, else directly. Stores the channel id in UniModelDW_Man.d1cid.
 * Matching: declared int in the DWARF, but no value is returned.
 * @param pktop the packet
 */
int sh2_Model_SyncOT(void *pktop) {
    struct sh2gfw_ModelDraw_MAN *pMD;
    int d1cid;
    int i;
    int id;

    pMD = &UniModelDW_Man;
    if (UniModelDW_Man.TB_change_VU0num) {
        for (i = -1; i < pMD->TB_change_VU0num - 1; i++) {
            id = pMD->TB_change_VU1num + (pMD->TB_change_VU0num + i);
            d1cid = sh2gfw_Thr_d1d2SyncKick(PhysAddr(pktop), pMD->chr_cid[id], pMD->chr_slotid[id]);
        }
    } else {
        d1cid = d1cSend((void *)((unsigned int)pktop & 0x0FFFFFFF));
    }
    pMD->d1cid = d1cid;
}

/**
 * Kicks a small dummy packet through the texture-sync path for each VU0 texture change of the
 * current character, and returns the channel-1 id (UniModelDW_Man.d1cid).
 */
int sh2_Model_DummySyncOT(void) {
    int d1cid;
    struct sh2gfw_ModelDraw_MAN *pMD;
    union Q_WORDDATA *qwd;
    int i;
    int id;

    pMD = &UniModelDW_Man;
    qwd = (union Q_WORDDATA *)(((unsigned int)dummy_OTFake_packet & 0x0FFFFFFF) | 0x20000000);
    qwd[0].ui32[0] = 0x10000002;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0x11000000;
    qwd[0].ui32[3] = 0x50000002;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8001;
    qwd[2].ul64[1] = 0x3F;
    qwd[2].ul64[0] = 0;
    qwd[3].ui32[0] = 0x70000000;
    qwd[3].ui32[1] = 0;
    qwd[3].ul64[1] = 0;
    if (UniModelDW_Man.TB_change_VU0num) {
        for (i = -1; i < pMD->TB_change_VU0num - 1; i++) {
            id = pMD->TB_change_VU1num + (pMD->TB_change_VU0num + i);
            d1cid = sh2gfw_Thr_d1d2SyncKick(dummy_OTFake_packet, pMD->chr_cid[id], pMD->chr_slotid[id]);
        }
        pMD->d1cid = d1cid;
    }
    return pMD->d1cid;
}

/** Returns the VU1 texture-change entry texno of the current character. */
int sh2gfw_get_ModelChangeTB(int texno) {
    return UniModelDW_Man.TB_change_VU1[texno];
}

/** Returns the VU1 texture index entry texno of the current character. */
int sh2gfw_get_ModelIndexTB(int texno) {
    return UniModelDW_Man.TB_index_VU1[texno];
}

/**
 * Sends pktop on channel 1 after the texture transfer (cid/slot index) of the current
 * character.
 */
void sh2gfw_Thr_Chracter_d1d2SyncKick(void *pktop, int index) {
    sh2gfw_Thr_d1d2SyncKick(PhysAddr(pktop), UniModelDW_Man.chr_cid[index], UniModelDW_Man.chr_slotid[index]);
}

/** Returns the character id of the character being drawn. */
int sh2gfw_get_Charaid(void) {
    return UniModelDW_Man.chara_id;
}

/** Returns the number of VU1 texture changes of the current character. */
int sh2gfw_get_TBChangeVU1num(void) {
    return UniModelDW_Man.TB_change_VU1num;
}

/** Sets the channel-1 id of the current character. */
void sh2gfw_set_CharaD1CID(int cid) {
    UniModelDW_Man.d1cid = cid;
}

/** Replaces every texture of character charaid with the noise buffer (CalcTex_buffer). */
void sh2gfw_SetNoise_CharaTexture(int charaid) {
    int i;
    struct sh2gfw_Model_Header *sMH;
    struct sh2gfw_ModelDraw_MAN *pMD;

    pMD = sh2gfw_Get_pMD(charaid);
    if (pMD) {
        sMH = pMD->pModel_Header;
        for (i = 0; i < sMH->texnum; i++) {
            sh2gfw_Change_TexBody(pMD->pTexMAN[i], CalcTex_buffer);
        }
    }
}

/** Restores the textures of character charaid after sh2gfw_SetNoise_CharaTexture. */
void sh2gfw_RemoveNoise_CharaTexture(int charaid) {
    int i;
    struct sh2gfw_Model_Header *sMH;
    struct sh2gfw_ModelDraw_MAN *pMD;

    pMD = sh2gfw_Get_pMD(charaid);
    if (pMD) {
        sMH = pMD->pModel_Header;
        for (i = 0; i < sMH->texnum; i++) {
            sh2gfw_Reset_TexBody(pMD->pTexMAN[i]);
        }
    }
}
