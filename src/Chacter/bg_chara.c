/*
 * Background characters (kind 3xx): item models stored in a map block's two extra file
 * slots (ex0/ex1), and binding the loaded models to their sub-characters.
 */

#include "sh2.h"
#include "libc/string.h"

/*
 * Matching: MapIdNo is an inline function, not a macro: assert() stringizes the call as
 * "MapIdNo(block[0])", and in BgCharaRelocateItemSetXZ the value goes straight
 * to s0 (a macro version loads it into v0 and copies).
 */

/* Matching: #line keeps the original line numbers (declarations moved to headers). */
#line 16
static inline int MapIdNo(int x) { return (unsigned short)x; }

static struct Model *model[2];

static struct {
    int id[2];
    void *adr[2];
    int old_id[2];
    void *old_adr[2];
    int cnt;
} d;

static int get_block_num(union fsFileIndex *id) {
    unsigned long size;

    size = FcGetFileSize(id);
    if (size) {
        return (size + 0x3FFFF) / 0x40000;
    }
    return -1;
}

/** Returns the size of file @p id in 256 KB blocks (rounded up), or -1 if it is empty. */
int BgCharaGetBlockSize(union fsFileIndex *id) {
    return get_block_num(id);
}

static int BgCharaRelocateItemSet(int glb_crd, int mapno, int chara_id) {
    union fsFileIndex *file[3];
    union fsFileIndex *cid;
    int result;
    int n;
    struct FilesBgBlock *bp;
    union fsFileIndex *p0;
    union fsFileIndex *p1;

    if ((chara_id >> 8) != 3) {
        return 0;
    }
    result = CharaDataFileSearch(file, chara_id);
    cid = file[0];
    /* Matching: the blank lines below keep the asserts on their original source lines. */



    assert(result);
    n = get_block_num(cid);
    bp = FilesGetBgBlock(glb_crd, mapno);
    assert(bp);
    p0 = bp->ex0;
    if ((unsigned int)p0 & 0x80000000) {
        p0 = (union fsFileIndex *)((unsigned int)p0 ^ 0x80000000);
    }
    p1 = bp->ex1;
    if ((unsigned int)p1 & 0x80000000) {
        p1 = (union fsFileIndex *)((unsigned int)p1 ^ 0x80000000);
    }
    if (n == 1) {
        if (!bp->ex0 || p0 == cid) {
            if (!bp->ex0) {
                bp->ex0 = cid;
            }
            return ((unsigned int)bp->ex0 & 0x80000000) ? 0 : 1;
        }
        if (!bp->ex1 || p1 == cid) {
            if (!bp->ex1) {
                bp->ex1 = cid;
            }
            return ((unsigned int)bp->ex1 & 0x80000000) ? 0 : 1;
        }
        printf("error ex1 and ex2 full slot!");
        printf("map %d\n", mapno);
        printf("ex0 %s\n", bp->ex0->index.name);
        printf("ex1 %s\n", bp->ex1->index.name);
        printf("add %s\n", cid->index.name);
    } else if (n == 2) {
        if ((!bp->ex0 || p0 == cid) && (!bp->ex1 || p1 == cid)) {
            if (!bp->ex0) {
                bp->ex0 = cid;
            }
            return ((unsigned int)bp->ex0 & 0x80000000) ? 0 : 1;
        }
        printf("error bg chara over size");
        printf("map %d\n", mapno);
        printf("ex0 %s\n", bp->ex0->index.name);
        printf("ex1 %s\n", bp->ex1->index.name);
        printf("add %s\n", cid->index.name);
    } else {
        printf("error bg chara over size");
        printf("map %d\n", mapno);
        printf("ex0 %s\n", bp->ex0 ? bp->ex0->index.name : "nil");
        printf("ex1 %s\n", bp->ex1 ? bp->ex1->index.name : "nil");
        printf("add %s\n", cid ? cid->index.name : "nil");
    }
    return 1;
}

static void BgCharaRelocateItemSetXZ(int glb_crd, float x, float z, int chara_id) {
    int block[4];
    int result;
    float r;
    int mapid;
    BlockNumber(block, glb_crd, x, z);
    mapid = MapIdNo(block[0]);
    assert(MapIdNo(block[0]));
    r = CharaGetBoundR(chara_id);
    result = BgCharaRelocateItemSet(glb_crd, mapid, chara_id);
}

/** Reserves an extra slot for background character @p chara_id in the map block at (@p x, @p z). */
void BgCharaRelocateSet(int glb_crd, float x, float z, int chara_id) {
    BgCharaRelocateItemSetXZ(glb_crd, x, z, chara_id);
}

/** Returns whether @p chara_id is a background character (kind 3xx). */
int BgCharaIsId(int chara_id) {
    return (chara_id >> 8) == 3;
}

/** Returns the loaded model of background character @p chara_id, or NULL if it isn't loaded. */
void *BgCharaIsLoad(int chara_id) {
    int i;

    if (!(chara_id >> 8) == 3) { /* @bug always false (precedence); kept as in the original */
        return NULL;
    }
    for (i = 0; i < 2; i++) {
        if (chara_id == d.id[i]) {
            if (LoadBgCharaIsLoadSlot(i)) {
                return d.adr[i];
            }
            break;
        }
    }
    return NULL;
}

static void init(void *adr, int chara_id) {
    struct SubCharacter *scp;

    sh2gfw_ModelDrawInit_for_BackgroundLoad(chara_id, adr, 0, 0, 0);
    for (scp = shCharacter_Manage_GetCharacterList(); scp; scp = scp->next) {
        if (scp->kind == chara_id) {
            shCharacter_Manage_SetDataAdresss(scp);
        }
    }
}

/** Clears the slot state. */
void BgCharaManInit(void) {
    memset(&d, 0, sizeof(d));
}

/**
 * Per-frame update: when @p cnt is 0, picks up the models in the two background-character
 * slots and binds newly loaded ones to their sub-characters.
 */
void BgCharaMan(int cnt) {
    int i;

    d.cnt = cnt;
    if (cnt == 0) {
        LoadBgGetUraCharaSlot((void **)&model[0], (void **)&model[1]);
        for (i = 0; i < 2; i++) {
            d.old_adr[i] = d.adr[i];
            d.old_id[i] = d.id[i];
            d.adr[i] = model[i];
            d.id[i] = model[i] ? model[i]->revision : 0;
            if (d.old_id[i] != d.id[i]) {
                if (d.id[i]) {
                    init(d.adr[i], d.id[i]);
                    printf("slot%d(%d)init ok %p\n", i, d.id[i], d.adr[i]);
                } else if (d.old_id[i]) {
                    printf("slot%d(%d)del ok %p\n", i, d.old_id[i], d.old_adr[i]);
                }
            }
        }
    }
}

/** Empties slot @p slot (the old model is remembered). */
void BgCharaDelSlot(int slot) {
    d.old_adr[slot] = d.adr[slot];
    d.old_id[slot] = d.id[slot];
    d.id[slot] = 0;
    d.adr[slot] = NULL;
}

/** Returns the character ID loaded in slot @p slot (0 if none). */
int BgCharaGetSlotId(int slot) {
    return d.id[slot];
}
