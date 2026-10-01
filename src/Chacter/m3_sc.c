/*
 * Sub-characters (SubCharacter/SubCharacterDisp): the pool of 32, the kind-sorted list,
 * creation and deletion with each kind's handler, the per-frame function/animation/matrix
 * passes, status switches, and animation accessors. Accessors ending in `_` take a track
 * `type`: 0 both tracks (getters: the main one), 2 the main track (anime), 1 the second
 * (anime2); the versions without `_` use 0.
 */

/* Matching: the handlers below are called without a prototype in the original (arguments
 * passed unconverted: scp is still in a0). */
#define SH2_LOCAL_shCharacterSetHumanLAULow
#define SH2_LOCAL_shCharacterSetHumanMARLow
#define SH2_LOCAL_shCharacterSetHumanDMARLow
#define SH2_LOCAL_shCharacterSetHumanAGLLow
#define SH2_LOCAL_shCharacterSetHumanRAGLLow
#define SH2_LOCAL_shCharacterSetHumanEDILow
#define SH2_LOCAL_shCharacterSetHumanMRYLow
#define SH2_LOCAL_shCharacterSetHumanMXXLow
#define SH2_LOCAL_shCharacterSetHumanBOTLow
#define SH2_LOCAL_shCharacterSetHumanINULow
#define SH2_LOCAL_shCharacterSetEnemySCULow
#define SH2_LOCAL_shCharacterSetEnemyMKNLow
#define SH2_LOCAL_shCharacterSetEnemyTYULow
#define SH2_LOCAL_shCharacterSetEnemyIKELow
#define SH2_LOCAL_shCharacterSetEnemyPAPLow
#define SH2_LOCAL_shCharacterSetEnemyEDBLow
#define SH2_LOCAL_shCharacterSetEnemyBOSLow
#define SH2_LOCAL_shCharacterSetEnemyREDLow
#define SH2_LOCAL_shCharacterSetEnemyONILow
#define SH2_LOCAL_shCharacterSetEnemyARMLow
#define SH2_LOCAL_shCharacterSetObjectNIKLow
#define SH2_LOCAL_shCharacterSetObjectLow
#define SH2_LOCAL_shCharacterSetWorldScreenItemLow
#define SH2_LOCAL_shCharacterSetItemScreenItemLow
/* Matching: called here with an extra argument, without a prototype. */
#define SH2_LOCAL_ClusterAnimeExec
#include "sh2.h"
#include "asm_helpers.h"
#include "libc/string.h"
#include "sdk/libvu0.h"

void shCharacterSetHumanLAULow();
void shCharacterSetHumanMARLow();
void shCharacterSetHumanDMARLow();
void shCharacterSetHumanAGLLow();
void shCharacterSetHumanRAGLLow();
void shCharacterSetHumanEDILow();
void shCharacterSetHumanMRYLow();
void shCharacterSetHumanMXXLow();
void shCharacterSetHumanBOTLow();
void shCharacterSetHumanINULow();
void shCharacterSetEnemySCULow();
void shCharacterSetEnemyMKNLow();
void shCharacterSetEnemyTYULow();
void shCharacterSetEnemyIKELow();
void shCharacterSetEnemyPAPLow();
void shCharacterSetEnemyEDBLow();
void shCharacterSetEnemyBOSLow();
void shCharacterSetEnemyREDLow();
void shCharacterSetEnemyONILow();
void shCharacterSetEnemyARMLow();
void shCharacterSetObjectNIKLow();
void shCharacterSetObjectLow();
void shCharacterSetWorldScreenItemLow();
void shCharacterSetItemScreenItemLow();
void ClusterAnimeExec();

struct shCharacterAll sh2chara;

static const unsigned char human_skelton[14] = { 41, 41, 56, 71, 88, 36, 73, 75, 64, 81, 75, 5, 41, 31 };
static const unsigned char enemy_skelton[14] = { 30, 17, 22, 16, 29, 35, 45, 30, 45, 41, 15, 30, 1, 1 };
static const unsigned char obj_outdoor_skelton[20] = { 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 5, 1, 1, 12, 10, 13, 6, 8 };
static const unsigned char obj_anime_skelton[69] = {
    11, 21, 3, 1, 1, 7, 6, 1, 7, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 30, 2, 2, 3, 1, 1, 2, 1, 1, 8, 5, 3, 1, 2, 8, 4,
    1, 1, 11, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 3, 36, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
static const unsigned char obj_stay_skelton[97] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 32, 36, 2, 1, 1, 1, 1, 1, 24, 1,
    2, 2, 1, 9, 1, 1, 1, 2, 13, 10, 1, 1, 1, 1, 1, 2, 4, 3, 3, 1, 1, 1, 6, 12, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 2, 1, 1, 1, 1, 1, 1, 2, 1, 5, 1, 1,
};

static struct SubCharacter *shCharacterGetFreeList(void) {
    struct SubCharacter *scp;

    scp = &sh2chara.free->sc;
    if (scp != NULL) {
        sh2chara.free = (struct SubCharacterDisp *)scp->next;
    }
    return scp;
}

static void AddFreeList(struct SubCharacter *scp) {
    scp->next = &sh2chara.free->sc;
    sh2chara.free = (struct SubCharacterDisp *)scp;
}

static void shCharacterSortList(struct SubCharacter *scp) {
    struct SubCharacter *pre;
    struct SubCharacter *next;

    next = sh2chara.head;
    if (next == NULL) {
        sh2chara.head = scp;
        scp->next = NULL;
        scp->pre = NULL;
        return;
    }
    if (sh2chara.player != NULL) {
        pre = next;
        next = next->next;
    } else {
        pre = NULL;
    }
    for (; next != NULL; pre = next, next = next->next) {
        if (scp->kind <= next->kind) {
            if (pre != NULL) {
                pre->next = scp;
            } else {
                sh2chara.head = scp;
            }
            next->pre = scp;
            scp->pre = pre;
            scp->next = next;
            return;
        }
    }
    pre->next = scp;
    scp->pre = pre;
    scp->next = NULL;
}

static void shCharacterTopOfList(struct SubCharacter *scp) {
    if (sh2chara.head != NULL) {
        sh2chara.head->pre = scp;
    }
    scp->next = sh2chara.head;
    scp->pre = NULL;
    sh2chara.head = scp;
}

static void shCharacterCutList(struct SubCharacter *scp) {
    struct SubCharacter *pre;
    struct SubCharacter *next;

    pre = scp->pre;
    next = scp->next;
    if (pre != NULL) {
        pre->next = next;
        scp->pre = NULL;
    } else {
        sh2chara.head = next;
    }
    if (next != NULL) {
        next->pre = pre;
        scp->next = NULL;
    }
}

static void shCharacterInitialize(struct SubCharacter *scp, int id, int model) {
    shCharacterSortList(scp);
    scp->status = 0x11;
    scp->sub_status = 0;
    scp->sub_st = 0;
    scp->id = id;
    scp->step = 0;
    scp->model_type = 0;
    scp->pos = (struct FVEC){ 0.0f, 0.0f, 0.0f, 0.0f };
    scp->b_pos = (struct FVEC){ 0.0f, 0.0f, 0.0f, 0.0f };
    scp->rot = (struct FVEC){ 0.0f, 0.0f, 0.0f, 0.0f };
    scp->mat = kt_unit_matrix;
    scp->eye_y = scp->center_y = scp->spd = scp->spd_org = scp->spd_y = scp->spd_roty = 0.0f;
    shQzero(&scp->battle, sizeof(struct shBattleInfo));
    *(int *)scp->work = 0;
    scp->function = NULL;
}

/* Matching: the ZYX rotation vector is built by an inline helper (UpdateMatrix's DWARF has no
 * locals). */
static inline void UpdateMatrixZYX(struct SubCharacter *scp, struct FVEC *rot) {
    float rot_xz[4];

    rot_xz[0] = rot->x;
    rot_xz[1] = 0.0f;
    rot_xz[2] = rot->z;
    rot_xz[3] = 1.0f;
    sceVu0RotMatrix(scp->mat.d, kt_unit_matrix.d, rot_xz);
    sceVu0RotMatrixY(scp->mat.d, scp->mat.d, rot->y);
}

static void UpdateMatrix(struct SubCharacter *scp, struct FVEC *rot, struct FVEC *trans) {
    if (scp->status & 0x80) {
        UpdateMatrixZYX(scp, rot);
    } else {
        sceVu0RotMatrix(scp->mat.d, kt_unit_matrix.d, (float *)rot);
    }
    scp->mat.d[3][0] = trans->x;
    scp->mat.d[3][1] = trans->y;
    scp->mat.d[3][2] = trans->z;
    scp->mat.d[3][3] = 1.0f;
}

static int shCharacterNeckAngleExec(struct shAnime3d *ap) {
    struct shSkelton *stp;

    for (stp = ap->top; stp != NULL; stp = stp->next) {
        if (stp->untouchable == NULL) {
            shCharacterAnimePartsControl(ap, stp, &ap->rot_neck);
        }
    }
    return 0;
}

static int shCharacterKneeAngleExec(struct shAnime3d *ap) {
    struct shSkelton *stp;

    for (stp = ap->top; stp != NULL; stp = stp->next) {
        if (stp->untouchable == NULL) {
            shCharacterAnimePartsControl(ap, stp, &ap->rot_body_neck);
        }
    }
    return 0;
}

/** Moves @p scp by @p pos (xyz); sets pos.w to 1. */
void SCAddPos(struct SubCharacter *scp, struct FVEC *pos) {
    scp->pos.x += pos->x;
    scp->pos.y += pos->y;
    scp->pos.z += pos->z;
    scp->pos.w = 1.0f;
}

/** Sets @p scp's rotation. */
void SCSetRot(struct SubCharacter *scp, struct FVEC *rot) {
    scp->rot = *rot;
}

/** Adds @p rot to @p scp's rotation (xyz). */
void SCAddRot(struct SubCharacter *scp, struct FVEC *rot) {
    scp->rot.x += rot->x;
    scp->rot.y += rot->y;
    scp->rot.z += rot->z;
}

static void shCharacterSetClusterAnimeWork(struct SubCharacterDisp *scp_d, int index) {
    if (scp_d->cluster_anime != NULL) {
        ClusterAnimeDelete(scp_d->cluster_anime, index);
        scp_d->cluster_anime = NULL;
    }
    if (scp_d->models[0] != NULL) {
        scp_d->cluster_anime = ClusterAnimeNew(Model3NClusters(scp_d->models[0]), index);
    }
}

/** Starts cluster animation data @p anime (an address) on @p scp. */
void shCharacterClusterAnimeSet(struct SubCharacter *scp, int anime) {
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    ClusterAnimeSet(scp_d->cluster_anime, (void *)anime);
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 570
/**
 * Binds model data to @p scp: takes its skeleton nodes, allocates the model work area and the
 * cluster-animation work. @param model, anime model and animation data addresses.
 */
void SCSetModel(struct SubCharacter *scp, int model, int anime) {
    void *model_adr;
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    model_adr = (void *)model;
    if (scp == NULL) {
        assert_dw(0); /* Matching: do/while(0) form (its nop) */
    }
    if (model_adr != NULL) {
        if (scp_d->anime.top == NULL) {
            shCharacterAnimeSetSkelton(&scp_d->anime, shCharacterGetSkeletons(Model3NSkeletons(model_adr),
                                                                              (unsigned char *)Model3SkeletonStructure(model_adr)));
            scp->sk_top = scp_d->anime.top;
        }
        scp_d->work = shCh_ASC_Malloc(Model3WorkSize(model_adr));
        Model3InitWork(model_adr, scp_d->work);
    }
    scp_d->models[0] = model_adr;
    scp_d->models[1] = model_adr;
    scp_d->models[2] = model_adr;
    shCharacterSetClusterAnimeWork(scp_d, scp->index);
    sh2chara.total++;
}

/** Returns @p scp's drama animation data address. @param anime_index unused. */
void *shCharacterGetAnimeAdrForDrama(struct SubCharacter *scp, int anime_index) {
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    return (void *)scp_d->anime_adr;
}

/** Returns @p scp's gameplay animation data address. */
void *shCharacterGetAnimeAdrForPlay(struct SubCharacter *scp) {
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    return (void *)scp_d->anime_adr;
}

/** Returns @p scp's cluster-animation data address. */
void *shCharacterGetClusterAnimeAdr(struct SubCharacter *scp) {
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    return (void *)scp_d->clani_adr;
}

/** Makes @p scp the player (moved to the head of the list), or clears the player if NULL. */
void shCharacterSetPlayer(struct SubCharacter *scp) {
    if (scp == NULL) {
        sh2chara.head = sh2chara.player = scp;
    } else {
        sh2chara.player = scp;
        shCharacterCutList(scp);
        shCharacterTopOfList(scp);
        shCharacterSetPlayerLow(scp);
    }
}

/** Returns @p scp's kind (model ID). */
short shCharacterGetModelID(struct SubCharacter *scp) {
    return scp->kind;
}

/** Returns the first sub-character of kind @p kind (and id @p id, unless -1), or NULL. */
struct SubCharacter *shCharacterGetSubCharacter(unsigned short kind, short id) {
    struct SubCharacter *pre;
    struct SubCharacter *next;

    pre = sh2chara.head;
    next = pre->next;
    if (pre->kind == kind && (id == -1 || id == pre->id)) {
        return pre;
    }
    for (; next != NULL; next = next->next) {
        if (next->kind == kind && (id == -1 || id == next->id)) {
            return next;
        }
    }
    return NULL;
}

/** Returns the number of skeleton nodes of character kind @p kind. */
int shCharacterGetSkeltonNum(short kind) {
    switch (kind >> 8) {
    case 1:
        return human_skelton[kind - 0x100];
    case 2:
        return enemy_skelton[kind - 0x200];
    case 3:
        return obj_outdoor_skelton[kind - 0x300];
    case 8:
        return 2;
    case 4:
        return obj_anime_skelton[kind - 0x400];
    case 5:
        return obj_stay_skelton[kind - 0x500];
    case 6:
    case 7:
        return 1;
    default:
        return 0;
    }
}

/** Returns the size of one animation frame for character kind @p id. */
int shCharacterAnimeOneFrameSize(unsigned short id) {
    unsigned short result;

    switch (id) {
    case 0x100:
    case 0x101:
        result = 0x210;
        break;
    case 0x105:
        result = 0x1D0;
        break;
    case 0x10B:
        result = 0x46;
        break;
    case 0x106:
        result = 0x3A0;
        break;
    case 0x108:
        result = 0x32C;
        break;
    case 0x107:
        result = 0x3B8;
        break;
    case 0x104:
        result = 0x468;
        break;
    case 0x109:
        result = 0x404;
        break;
    case 0x10A:
        result = 0x3B8;
        break;
    case 0x10D:
        result = 0x190;
        break;
    case 0x200:
        result = 0x184;
        break;
    case 0x201:
        result = 0xE4;
        break;
    case 0x202:
        result = 0x11A;
        break;
    case 0x208:
    case 0x20E:
    case 0x210:
    case 0x20F:
        result = 0x240;
        break;
    case 0x207:
    case 0x20B:
        result = 0x184;
        break;
    case 0x203:
        result = 0xCE;
        break;
    case 0x204:
        result = 0x17E;
        break;
    case 0x205:
        result = 0x1C4;
        break;
    case 0x206:
        result = 0x23A;
        break;
    case 0x209:
        result = 0x210;
        break;
    case 0x20A:
        result = 0xC8;
        break;
    case 0x400:
        result = 0x98;
        break;
    case 0x401:
        result = 0x10E;
        break;
    case 0x801:
        result = 0x22;
        break;
    case 0x802:
        result = 0x22;
        break;
    case 0x803:
        result = 0x22;
        break;
    case 0x805:
        result = 0x22;
        break;
    case 0x806:
        result = 0x22;
        break;
    case 0x804:
        result = 0x22;
        break;
    case 0x807:
        result = 0x22;
        break;
    case 0x808:
        result = 0x22;
        break;
    case 0x40B:
        result = 0x28;
        break;
    case 0x421:
        result = 0x6A;
        break;
    case 0x408:
        result = 0x5E;
        break;
    case 0x42E:
        result = 0x16;
        break;
    default:
        result = 0;
        break;
    }
    return result;
}

/** Clears the sub-character pool and links all 32 entries into the free list. */
void shCharacterInitSubCharacter(void) {
    int i;
    struct SubCharacterDisp *scp_d;

    memset(&sh2chara, 0, sizeof(sh2chara));
    scp_d = sh2chara.work;
    sh2chara.free = sh2chara.work;
    for (i = 0; i < 31; i++, scp_d++) {
        scp_d->sc.next = &(scp_d + 1)->sc;
    }
    scp_d->sc.next = NULL;
    for (i = 0; i < 32; i++) {
        sh2chara.work[i].sc.index = i;
    }
}

static void shCharacterSetHandler(struct SubCharacter *scp) {
    switch (scp->kind) {
    case 0x100:
    case 0x101:
        shCharacterSetPlayer(scp);
        break;
    case 0x120:
    case 0x121:
        shCharacterSetHumanRPJMSLow(scp);
        break;
    case 0x102:
    case 0x103:
        shCharacterSetHumanDJMSLow(scp);
        break;
    case 0x122:
    case 0x123:
        shCharacterSetHumanRDJMSLow(scp);
        break;
    case 0x104:
        shCharacterSetHumanLAULow(scp);
        break;
    case 0x105:
        shCharacterSetHumanMARLow(scp);
        break;
    case 0x106:
        shCharacterSetHumanDMARLow(scp);
        break;
    case 0x107:
        shCharacterSetHumanAGLLow(scp);
        break;
    case 0x127:
        shCharacterSetHumanRAGLLow(scp);
        break;
    case 0x108:
        shCharacterSetHumanEDILow(scp);
        break;
    case 0x109:
        shCharacterSetHumanMRYLow(scp);
        break;
    case 0x10A:
        shCharacterSetHumanMXXLow(scp);
        break;
    case 0x10B:
        shCharacterSetHumanBOTLow(scp);
        break;
    case 0x10D:
        shCharacterSetHumanINULow(scp);
        break;
    case 0x200:
        shCharacterSetEnemySCULow(scp);
        break;
    case 0x201:
        shCharacterSetEnemyMKNLow(scp);
        break;
    case 0x202:
        shCharacterSetEnemyTYULow(scp);
        break;
    case 0x203:
        shCharacterSetEnemyIKELow(scp);
        break;
    case 0x204:
        shCharacterSetEnemyPAPLow(scp);
        break;
    case 0x205:
        shCharacterSetEnemyEDBLow(scp);
        break;
    case 0x206:
        shCharacterSetEnemyBOSLow(scp);
        break;
    case 0x207:
    case 0x20B:
        shCharacterSetEnemyNSELow(scp);
        break;
    case 0x208:
        shCharacterSetEnemyREDLow(scp);
        break;
    case 0x209:
        shCharacterSetEnemyONILow(scp);
        break;
    case 0x20A:
        shCharacterSetEnemyARMLow(scp);
        break;
    case 0x20C:
    case 0x20D:
        shCharacterSetEnemyTY23Low(scp);
        break;
    default:
        switch (scp->kind >> 8) {
        case 8:
            if (scp->kind & 0x20) {
                shCharacterSetWeaponRLow(scp);
            } else {
                shCharacterSetWeaponLow(scp);
                JamesWeaponSet(scp->kind & 0xF);
            }
            break;
        case 4:
            switch (scp->kind) {
            case 0x443:
            case 0x444:
                shCharacterSetRObjectLow(scp);
                break;
            case 0x421:
                shCharacterSetObjectNIKLow(scp);
                break;
            default:
                shCharacterSetObjectLow(scp);
                break;
            }
            break;
        case 3:
        case 5:
            shCharacterSetStayObjectLow(scp);
            break;
        case 7:
            shCharacterSetWorldScreenItemLow(scp);
            break;
        case 6:
            shCharacterSetItemScreenItemLow(scp);
            break;
        }
        break;
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1218
/**
 * Takes a sub-character from the pool, initializes it as kind @p chr_id and installs its
 * kind's handler. @param id its ID. @param model, anime, clani data addresses.
 */
struct SubCharacter *shCharacterCreate(unsigned int id, int model, int anime, int clani, int chr_id) {
    struct SubCharacter *scp;
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)shCharacterGetFreeList();
    scp = &scp_d->sc;
    if (scp_d == NULL) {
        assert_dw(0); /* Matching: do/while(0) form (its nop) */
    }
    scp_d->sc.kind = chr_id;
    shCharacterInitialize(&scp_d->sc, id, model);
    scp_d->model_adr = model;
    scp_d->anime_adr = anime;
    scp_d->clani_adr = clani;
    shCharacterSetHandler(&scp_d->sc);
    return scp;
}

/** Frees @p scp's skeleton, work area and cluster animation and returns it to the pool. */
void shCharacterDelete(struct SubCharacter *scp) {
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    if (scp != NULL) {
        if (scp == sh2chara.player) {
            shCharacterSetPlayer(NULL);
        }
        shCharacterFreeSkeltons(scp->sk_top);
        scp_d->anime.top = NULL;
        shCh_ASC_Free(scp_d->work);
        scp_d->work = NULL;
        ClusterAnimeDelete(scp_d->cluster_anime, scp->index);
        shCharacterCutList(scp);
        scp->kind = 0;
        scp->id = 0;
        scp->sk_top = NULL;
        scp->pre = NULL;
        scp->next = NULL;
        scp->function = NULL;
        scp->enemy_p = NULL;
        AddFreeList(scp);
        sh2chara.total--;
    }
}

/** Runs @p scp's gameplay animation for this frame (James and Maria animate upper and lower body separately). */
void shCharacterPlayingExecAnimeOne(struct SubCharacter *scp) {
    struct SubCharacterDisp *scp_d;
    struct shSkelton *stp;
    struct SubCharacter *scp_wp;
    unsigned char weapon;

    scp_d = (struct SubCharacterDisp *)scp;
    switch (scp->kind) {
    case 0x100:
    case 0x101:
        sh_PJMS_SetUntouchUpper(scp_d->anime.top);
        shCharacterPlayingAnimeExecMain(&scp_d->anime, 0);
        sh_PJMS_ResetUntouchUpper(scp_d->anime.top);
        sh_PJMS_SetUntouchUnder(scp_d->anime2.top);
        shCharacterPlayingAnimeExecMain(&scp_d->anime2, 1);
        sh_PJMS_ResetUntouchUnder(scp_d->anime2.top);
        mizSetUntouchWithoutKnee(scp_d->anime2.top);
        shCharacterKneeAngleExec(&scp_d->anime2);
        mizResetUntouchWithoutKnee(scp_d->anime2.top);
        if (scp->status & 0x20000) {
            sh_PJMS_SetUntouchUnder(scp_d->anime2.top);
            shCharacterNeckAngleExec(&scp_d->anime2);
            sh_PJMS_ResetUntouchUnder(scp_d->anime2.top);
        }
        break;
    case 0x105:
        shCharacterPlayingAnimeExecMain(&scp_d->anime, 0);
        MariaSetUntouchWithoutNeck(scp_d->anime.top);
        shCharacterNeckAngleExec(&scp_d->anime);
        MariaResetUntouchWithoutNeck(scp_d->anime.top);
        break;
    default:
        shCharacterPlayingAnimeExecMain(&scp_d->anime, 0);
        break;
    }
    switch (scp->kind) {
    case 0x101:
    case 0x100:
        weapon = PlayerGetJamesWeapon();
        if ((scp_wp = shCharacterGetSubCharacter(weapon + 0x800, -1)) != NULL) {
            shUpdateWeaponMatrixAfterAnime(scp_wp, scp->kind);
        }
        break;
    case 0x10B:
        shUpdateBoatJamesPosAfterAnime();
        break;
    case 0x203:
        enIKETrans(scp->enemy_p);
        break;
    case 0x20A:
        enARMTrans(scp->enemy_p);
        break;
    }
    switch (scp->kind) {
    case 0x101:
    case 0x100:
        shGetJamesLightPos_Calc();
        shLensFlareExec(scp, 3.0f, 0);
        if (PlayerReverseLightCalcIsOn()) {
            shGetJamesLightPos_Calc_Reverse();
            shLensFlareExec(scp, 3.0f, 1);
        }
        break;
    }
}

/** Runs @p scp's drama animation and cluster animation for this frame. */
void shCharacterDramaExecAnimeOne(struct SubCharacter *scp) {
    struct SubCharacterDisp *scp_d;
    struct SubCharacter *scp_wp;
    unsigned char weapon;

    scp_d = (struct SubCharacterDisp *)scp;
    shCharacterDramaAnimeExecMain(&scp_d->anime);
    ClusterAnimeExec(scp_d->cluster_anime, &scp_d->anime, scp);
    switch (scp->kind) {
    case 0x102:
    case 0x101:
    case 0x100:
        weapon = PlayerGetJamesWeapon();
        if ((scp_wp = shCharacterGetSubCharacter(weapon + 0x800, -1)) != NULL) {
            shUpdateWeaponMatrixAfterAnime(scp_wp, scp->kind);
        }
        break;
    }
    switch (scp->kind) {
    case 0x103:
    case 0x102:
    case 0x101:
    case 0x100:
        shGetJamesLightPos_Calc();
        shLensFlareExec(scp, 3.0f, 0);
        break;
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1645
/** Makes mirrored copy @p scp share the skeleton and animation of the character whose kind is 0x20 lower. */
void shCharacterAnimeCopyForReverseModel(struct SubCharacter *scp) {
    struct SubCharacter *org;
    struct SubCharacterDisp *org_d;
    struct SubCharacterDisp *scp_d;
    struct SubCharacter *scp_wp;
    unsigned char weapon;

    scp_d = (struct SubCharacterDisp *)scp;
    org = shCharacterGetSubCharacter(scp->kind - 0x20, -1);
    org_d = (struct SubCharacterDisp *)org;
    if (org == NULL) {
        assert(0);
    }
    scp_d->anime.top = org_d->anime.top;
    scp_d->cluster_anime = org_d->cluster_anime;
    switch (scp->kind) {
    case 0x121:
    case 0x120:
        weapon = PlayerGetJamesWeapon();
        if ((scp_wp = shCharacterGetSubCharacter(weapon + 0x820, -1)) != NULL) {
            shUpdateWeaponMatrixAfterAnime(scp_wp, scp->kind);
        }
        break;
    }
    switch (scp->kind) {
    case 0x121:
    case 0x120:
    case 0x123:
    case 0x122:
        break;
    }
}

/** Sets or clears @p scp's demo-event flag (status 0x2000). */
void SCNowDemoEventSwitch(struct SubCharacter *scp, int flag) {
    if (flag) {
        scp->status |= 0x2000;
    } else {
        scp->status &= ~0x2000;
    }
}

/** Sets or clears @p scp's playable-event flag (status 0x4000). */
void SCNowPlayableEventSwitch(struct SubCharacter *scp, int flag) {
    if (flag) {
        scp->status |= 0x4000;
    } else {
        scp->status &= ~0x4000;
    }
}

/** Sets or clears @p scp's static-model flag (status 0x100). */
void SCStayModelSwitch(struct SubCharacter *scp, int flag) {
    if (flag) {
        scp->status |= 0x100;
    } else {
        scp->status &= ~0x100;
    }
}

/** Selects gameplay (@p flag non-zero, status 0x4) or drama animation for @p scp. */
void SCAnimeTypeSwitch(struct SubCharacter *scp, int flag) {
    if (flag) {
        scp->status |= 0x4;
    } else {
        scp->status &= ~0x4;
    }
}

/** Selects ZYX rotation order for @p scp's matrix (status 0x80). */
void SCRotZYXSwitch(struct SubCharacter *scp, int flag) {
    if (flag) {
        scp->status |= 0x80;
    } else {
        scp->status &= ~0x80;
    }
}

/** Sets or clears @p scp's free-fall flag (status 0x10000). */
void SCFreefallSwitch(struct SubCharacter *scp, int sw) {
    if (sw) {
        scp->status |= 0x10000;
    } else {
        scp->status &= ~0x10000;
    }
}

/** Sets or clears @p scp's light-on flag (status 0x200). */
void SCLightOnNowSwitch(struct SubCharacter *scp, int sw) {
    if (sw) {
        scp->status |= 0x200;
    } else {
        scp->status &= ~0x200;
    }
}

/** Runs this frame's animation of every visible, animated sub-character. */
void shCharacterExecAnimeAll(void) {
    struct SubCharacter *scp;

    for (scp = sh2chara.head; scp != NULL; scp = scp->next) {
        if (!(scp->status & 0x100) && (scp->status & 0x10)) {
            if (scp->model_type) {
                shCharacterAnimeCopyForReverseModel(scp);
            } else if (scp->status & 0x4) {
                shCharacterPlayingExecAnimeOne(scp);
            } else {
                shCharacterDramaExecAnimeOne(scp);
            }
        }
    }
    demo_status &= ~0x40;
}

/** Rebuilds every sub-character's matrix from its rotation and position. */
void shCharacterUpdateAll(void) {
    struct SubCharacter *scp;

    for (scp = sh2chara.head; scp != NULL; scp = scp->next) {
        if (scp->status & 1) {
            scp->status &= ~1;
        }
        UpdateMatrix(scp, &scp->rot, &scp->pos);
    }
}

/** Sets @p scp's per-frame function. */
void shCharacterSetFunction(struct SubCharacter *scp, void (*func)(struct SubCharacter *)) {
    scp->function = func;
}

/** Runs every sub-character's per-frame function, then updates their matrices. */
void shCharacterExecFunctionAll(void) {
    struct SubCharacter *scp;
    struct SubCharacter *next;

    for (scp = sh2chara.head; scp != NULL; scp = next) {
        next = scp->next;
        if (scp->function != NULL) {
            scp->function(scp);
        }
    }
    shCharacterUpdateAll();
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 2294
/**
 * Starts animation @p anim_info on one of @p scp's tracks.
 * @param ctrl_type track (0/2 main, 1 second). @param inter_type interpolation mode.
 * @param anime animation data address.
 */
void shCharacterAnimeSet(struct SubCharacter *scp, int ctrl_type, int inter_type, struct _AnimeInfo *anim_info, int anime) {
    void *anime_adr;
    struct shAnime3d *anim;
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    switch (ctrl_type) {
    case 0:
    case 2:
        anim = &scp_d->anime;
        break;
    case 1:
        anim = &scp_d->anime2;
        break;
    }
    scp_d->anime.scale = 1.0f;
    scp_d->anime2.scale = 1.0f;
    anim->p_anime = anim->anime;
    anim->p_frame_top = anim->frame_top;
    anime_adr = (void *)anime;
    anim->anime = anime_adr;
    anim->frame_size = shCharacterAnimeOneFrameSize(scp->kind);
    if (scp->kind >> 8 == 8) {
        anim->first_bone_type = 1;
    } else {
        anim->first_bone_type = 0;
    }
    anim->comp_type = inter_type;
    switch ((char)inter_type) {
    case 0:
    case 2:
        anim->total_count = 0;
        if (anim_info->speed < 0) {
            anim->cur_frame.x = anim->cur_frame.y = anim_info->end;
        } else {
            anim->cur_frame.x = anim->cur_frame.y = anim_info->start;
        }
        anim->frame_top = (char *)anim->anime + anim->frame_size * anim->cur_frame.y;
        anim->c_count.x = anim->c_count.y = 0;
        anim->c_speed.x = anim->c_speed.y = 0;
        anim->anim_a = anim_info;
        anim->anim_b = anim_info;
        break;
    case 4:
    case 6:
    case 8:
        switch (inter_type) {
        case 4:
            if (anim_info->speed < 0) {
                anim->cur_frame.y = anim_info->end;
            } else {
                anim->cur_frame.y = anim_info->start;
            }
            break;
        case 8:
            anim->cur_frame.y = scp_d->anime.cur_frame.y;
            break;
        }
        anim->frame_top = (char *)anim->anime + anim->frame_size * anim->cur_frame.y;
        anim->c_count.y = 0;
        anim->c_speed.y = 0;
        anim->total_speed.y = 0;
        if (anim->anim_a == NULL) {
            anim->anim_a = anim_info;
        } else {
            anim->anim_a = anim->anim_b;
        }
        anim->anim_b = anim_info;
        break;
    case 10:
        anim->cur_frame.y = anim->cur_frame.x;
        anim->c_count.y = 0;
        anim->c_speed.y = 0x80;
        anim->total_speed.y = 0;
        if (anim->anim_a == NULL) {
            assert_dw(0); /* Matching: do/while(0) form (its nop) */
        }
        anim->anim_a = anim->anim_b;
        anim->anim_b = anim_info;
        break;
    }
}

/** Sets a static model's scale and applies it. */
void shCharacterStayObjectScaleSet(struct SubCharacter *scp, float scale) {
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    scp_d->anime.scale = scale;
    shCharacterStayModelScale(&scp_d->anime);
}

/** Poses and scales an item-screen model from @p data. */
void shCharacterItemScreenObjectSet(struct SubCharacter *scp, struct shItemScreenObjectSettingData *data) {
    struct SubCharacterDisp *scp_d;
    struct FVEC rot_tmp = { 0.0f, 0.0f, 0.0f, 1.0f };
    struct shSkelton *sp;

    scp_d = (struct SubCharacterDisp *)scp;
    sp = scp_d->anime.top;
    shCharacterStayModelExecItem(sp, &data->rot.x);
    scp_d->anime.scale = data->scale;
    shCharacterStayModelScale(&scp_d->anime);
    SCSetRot(scp, &rot_tmp);
}

/** Gets the position of node @p n of a static model (@p rot is zeroed). */
void shCharacterStayObjectNthPartsGet1st(struct SubCharacter *scp, int n, float *pos, float *rot) {
    int i;
    struct SubCharacterDisp *scp_d;
    struct shSkelton *sp;

    scp_d = (struct SubCharacterDisp *)scp;
    sp = scp_d->anime.top;
    i = 0;
    while (i < n) {
        sp = sp->next;
        i++;
    }
    pos[0] = sp->des_t.x;
    pos[1] = sp->des_t.y;
    pos[2] = sp->des_t.z;
    rot[0] = rot[1] = rot[2] = 0.0f;
}

/** Sets the position and rotation of node @p n of a static model. */
void shCharacterStayObjectNthPartsSet(struct SubCharacter *scp, int n, float *pos, float *rot) {
    int i;
    struct SubCharacterDisp *scp_d;
    struct shSkelton *sp;

    scp_d = (struct SubCharacterDisp *)scp;
    sp = scp_d->anime.top;
    scp_d->anime.scale = 1.0f;
    for (i = 0; i < n; i++) {
        sp = sp->next;
    }
    shCharacterStayModelExecNthParts(sp, pos, rot);
    sp->xx = rot[0];
    sp->yy = rot[1];
    sp->zz = rot[2];
}

/** Returns the animation speed of track @p type. */
short shCharacterAnimeSpeedGet_(struct SubCharacter *scp, unsigned int type) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;

    scp_d = (struct SubCharacterDisp *)scp;
    switch (type) {
    case 0:
    case 2:
        anime = &scp_d->anime;
        break;
    case 1:
        anime = &scp_d->anime2;
        break;
    }
    return anime->total_speed.x;
}

/** Sets the animation speed offset (x) of both tracks. */
void shCharacterAnimeSpeedAdd(struct SubCharacter *scp, short add) {
    shCharacterAnimeSpeedAdd_(scp, 0, add);
}

/** Sets the animation speed offset (x) of track @p type to @p add. */
void shCharacterAnimeSpeedAdd_(struct SubCharacter *scp, unsigned int type, short add) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;
    struct shAnime3d *anime2;

    scp_d = (struct SubCharacterDisp *)scp;
    anime = &scp_d->anime;
    anime2 = &scp_d->anime2;
    switch (type) {
    case 0:
        anime->c_speed.x = add;
        anime2->c_speed.x = add;
        break;
    case 2:
        anime->c_speed.x = add;
        break;
    case 1:
        anime2->c_speed.x = add;
        break;
    }
}

/** Sets the animation speed offset (y) of both tracks. */
void shCharacterAnimeSpeedAddY(struct SubCharacter *scp, short add) {
    shCharacterAnimeSpeedAddY_(scp, 0, add);
}

/** Sets the animation speed offset (y) of track @p type to @p add. */
void shCharacterAnimeSpeedAddY_(struct SubCharacter *scp, unsigned int type, short add) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;
    struct shAnime3d *anime2;

    scp_d = (struct SubCharacterDisp *)scp;
    anime = &scp_d->anime;
    anime2 = &scp_d->anime2;
    switch (type) {
    case 0:
        anime->c_speed.y = add;
        anime2->c_speed.y = add;
        break;
    case 2:
        anime->c_speed.y = add;
        break;
    case 1:
        anime2->c_speed.y = add;
        break;
    }
}

/** Pauses both animation tracks. */
void shCharacterAnimePause(struct SubCharacter *scp) {
    shCharacterAnimePause_(scp, 0);
}

/** Pauses animation track @p type. */
void shCharacterAnimePause_(struct SubCharacter *scp, unsigned int type) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;
    struct shAnime3d *anime2;

    scp_d = (struct SubCharacterDisp *)scp;
    anime = &scp_d->anime;
    anime2 = &scp_d->anime2;
    switch (type) {
    case 0:
        anime->comp_type = -1;
        anime2->comp_type = -1;
        break;
    case 2:
        anime->comp_type = -1;
        break;
    case 1:
        anime2->comp_type = -1;
        break;
    }
}

/** Resumes both animation tracks. */
void shCharacterAnimeRestart(struct SubCharacter *scp) {
    shCharacterAnimeRestart_(scp, 0);
}

/** Resumes animation track @p type. */
void shCharacterAnimeRestart_(struct SubCharacter *scp, unsigned int type) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;
    struct shAnime3d *anime2;

    scp_d = (struct SubCharacterDisp *)scp;
    anime = &scp_d->anime;
    anime2 = &scp_d->anime2;
    switch (type) {
    case 0:
        if (anime->comp_type == -1) {
            anime->comp_type = 2;
        }
        if (anime2->comp_type == -1) {
            anime2->comp_type = 2;
        }
        break;
    case 2:
        if (anime->comp_type == -1) {
            anime->comp_type = 2;
        }
        break;
    case 1:
        if (anime2->comp_type == -1) {
            anime2->comp_type = 2;
        }
        break;
    }
}

/** Returns whether the main animation track has ended. */
int shCharacterAnimeIsEnd(struct SubCharacter *scp) {
    return shCharacterAnimeIsEnd_(scp, 0);
}

/** Returns whether animation track @p type has ended. */
int shCharacterAnimeIsEnd_(struct SubCharacter *scp, unsigned int type) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;

    scp_d = (struct SubCharacterDisp *)scp;
    switch (type) {
    case 0:
    case 2:
        anime = &scp_d->anime;
        break;
    case 1:
        anime = &scp_d->anime2;
        break;
    }
    if (anime->anim_b->loop) {
        return 0;
    }
    if (anime->total_speed.x < 0) {
        return anime->total_count == 0;
    }
    return anime->total_count == (anime->anim_b->end - anime->anim_b->start) << 12;
}

/** Returns the main track's current frame. */
short shCharacterAnimeFrameGet(struct SubCharacter *scp) {
    return shCharacterAnimeFrameGet_(scp, 0);
}

/** Returns track @p type's current frame. */
short shCharacterAnimeFrameGet_(struct SubCharacter *scp, unsigned int type) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;

    scp_d = (struct SubCharacterDisp *)scp;
    switch (type) {
    case 0:
    case 2:
        anime = &scp_d->anime;
        break;
    case 1:
        anime = &scp_d->anime2;
        break;
    }
    switch (anime->comp_type) {
    case 10:
        return (anime->cur_frame.y - anime->cur_frame.x == 1) ? (short)(anime->cur_frame.x - anime->anim_a->start)
                                                               : (short)(anime->cur_frame.y - anime->anim_a->start);
    default:
        return (anime->cur_frame.y - anime->cur_frame.x == 1) ? (short)(anime->cur_frame.x - anime->anim_b->start)
                                                               : (short)(anime->cur_frame.y - anime->anim_b->start);
    }
}

/** Sets the current frame of both tracks. */
void shCharacterAnimeFrameSet(struct SubCharacter *scp, unsigned short frame) {
    shCharacterAnimeFrameSet_(scp, 0, frame);
}

/** Sets track @p type's current frame. */
void shCharacterAnimeFrameSet_(struct SubCharacter *scp, unsigned int type, unsigned short frame) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;

    scp_d = (struct SubCharacterDisp *)scp;
    switch (type) {
    case 0:
    case 2:
        anime = &scp_d->anime;
        break;
    case 1:
        anime = &scp_d->anime2;
        break;
    }
    anime->cur_frame.x = frame;
    anime->cur_frame.y = frame;
    anime->total_count = frame << 12;
    anime->c_count.x = 0;
    anime->frame_top = (char *)anime->anime + anime->frame_size * anime->cur_frame.x;
}

/** Returns the main track's frame counter. */
int shCharacterAnimeCounterGet(struct SubCharacter *scp) {
    return shCharacterAnimeCounterGet_(scp, 0);
}

/** Returns track @p type's frame counter. */
int shCharacterAnimeCounterGet_(struct SubCharacter *scp, unsigned int type) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;

    scp_d = (struct SubCharacterDisp *)scp;
    switch (type) {
    case 0:
    case 2:
        anime = &scp_d->anime;
        break;
    case 1:
        anime = &scp_d->anime2;
        break;
    }
    return anime->total_count;
}

/** Sets track @p type's frame counter. */
void shCharacterAnimeCounterSet_(struct SubCharacter *scp, unsigned int type, int counter) {
    struct SubCharacterDisp *scp_d;
    struct shAnime3d *anime;

    scp_d = (struct SubCharacterDisp *)scp;
    switch (type) {
    case 0:
    case 2:
        anime = &scp_d->anime;
        break;
    case 1:
        anime = &scp_d->anime2;
        break;
    }
    anime->total_count = counter;
    anime->cur_frame.x = anime->cur_frame.y = counter >> 12;
    anime->c_count.x = counter % 0x1000;
    anime->frame_top = (char *)anime->anime + anime->frame_size * anime->cur_frame.x;
}

/** Returns the main track's animation info. */
struct _AnimeInfo *shCharacterAnimeGetInfo(struct SubCharacter *scp) {
    return shCharacterAnimeGetInfo_(scp, 0);
}

/** Returns track @p ctrl_type's animation info. */
struct _AnimeInfo *shCharacterAnimeGetInfo_(struct SubCharacter *scp, int ctrl_type) {
    struct shAnime3d *ap;
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    switch (ctrl_type) {
    case 0:
    case 2:
        ap = &scp_d->anime;
        break;
    case 1:
        ap = &scp_d->anime2;
        break;
    default:
        printf("error\n");
        return NULL;
    }
    return ap->anim_b;
}

/** Switches James to drama animation. */
void shCharacterPlayerModelToDrama(void) {
    struct SubCharacter *p;

    p = shCharacterGetSubCharacter(0x100, -1);
    if (p == NULL) {
        p = shCharacterGetSubCharacter(0x101, -1);
    }
    if (p != NULL) {
        SCAnimeTypeSwitch(p, 0);
    }
}

/** Switches James back to gameplay animation (restarting his function). */
void shCharacterPlayerModelToPlayable(void) {
    struct SubCharacter *p;

    p = shCharacterGetSubCharacter(0x100, -1);
    if (p == NULL) {
        p = shCharacterGetSubCharacter(0x101, -1);
    }
    if (p != NULL) {
        p->step = 0;
        SCAnimeTypeSwitch(p, 1);
    }
}

/** Switches Maria to drama animation. */
void shCharacterMariaModelToDrama(void) {
    struct SubCharacter *p;

    p = shCharacterGetSubCharacter(0x105, -1);
    if (p != NULL) {
        SCAnimeTypeSwitch(p, 0);
    }
}

/** Switches Maria back to gameplay animation (restarting her function). */
void shCharacterMariaModelToPlayable(void) {
    struct SubCharacter *p;

    p = shCharacterGetSubCharacter(0x105, -1);
    if (p != NULL) {
        p->step = 0;
        SCAnimeTypeSwitch(p, 1);
    }
}

/** Places @p scp at @p pos facing @p roty, stopped, after a demo. */
void shCharacterSetPosAfterDemo(struct SubCharacter *scp, float *pos, float roty) {
    scp->pos.x = scp->b_pos.x = pos[0];
    scp->pos.y = scp->b_pos.y = pos[1];
    scp->pos.z = scp->b_pos.z = pos[2];
    scp->rot.y = scp->b_rot.y = roty;
    scp->rot.x = scp->b_rot.x = 0.0f;
    scp->rot.z = scp->b_rot.z = 0.0f;
    scp->spd = scp->spd_org = 0.0f;
}

/** Gets the world matrix of node @p parts_name of character @p kind/@p id, for shadows. */
void shCharacterGetPartsMatrixForShadow(float (*mat)[4], unsigned short kind, unsigned short id, unsigned int parts_name) {
    int i1;
    struct SubCharacter *p;
    struct shSkelton *sk;

    p = shCharacterGetSubCharacter(kind, id);
    if (p != NULL) {
        sk = p->sk_top;
        i1 = 0;
        while (i1 < parts_name) {
            sk = sk->next;
            i1++;
        }
        sceVu0MulMatrix(mat, (float (*)[4])&p->mat, (float (*)[4])&sk->src_m);
    }
}

/** Gets the position, ground normal and ground height of character @p kind/@p id, for shadows. */
void shCharacterGetGroundInfoForShadow(float *pos, float *normal, float *height, unsigned short kind, unsigned short id) {
    struct SubCharacter *p;

    p = shCharacterGetSubCharacter(kind, id);
    if (p != NULL) {
        vcopy(&p->pos, pos);
        vcopy(p->grnd_normal, normal);
        *height = p->grnd_height;
    }
}
