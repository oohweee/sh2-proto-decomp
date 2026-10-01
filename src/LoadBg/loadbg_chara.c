/*
 * loadbg_chara.c: streaming of the "ura" background characters (the
 * ex0/ex1 files of the current map block) into two load slots.
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "libc/math.h" /* fabsf: the library call, not the inline one in asm_libm.h */
#include "libc/string.h"

#define MAXSECT 2

static struct loadBgChara_Work d = {
    { 0 },
    { { 0 } },
    { { 0 } },
    { 0 },
    { { 0 } },
    { 0 },
    {
        { 0, 0x80000, 1, &d.filelist[0], 0, 0, 0, 0 },
        { 0, 0x80000, 1, &d.filelist[1], 0, 1, 0, 0 },
    },
};

static void initwork(void) {
    memset(d.filelist, 0, sizeof(d.filelist));
    d.SectList[0] = NULL;
    d.memflg = 0;
}

static void memspy(void) {
    char *adr;

    adr = MemShareGetBgCharaWorkAddr();
    if (adr == NULL) {
        printf("first bg NULL\n");
        d.ctrlp = NULL;
        adr = MemShareGetBgCharaWorkAddr();
    }
    if (adr == NULL) {
        printf("bg adr NULL\n");
        d.ctrlp = NULL;
    } else if (d.ctrlp == NULL) {
        printf("init adr NULL\n");
        LoadBgCharaInit();
    }
}

static void subcharaupdate(int chara_id) {
    struct SubCharacter *scp;

    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = scp->next) {
        if (scp->kind == chara_id) {
            shCharacter_Manage_SetDataAdresss(scp);
        }
    }
}

static void init_clean(void) {
    int i;
    int id;

    for (i = 0; i < 2; i++) {
        if (d.filelist[i].file != NULL && LoadBgCharaIsLoadSlot(i)) {
            id = BgCharaGetSlotId(i);
            if (id != 0) {
                printf("slot%d(%d) clear\n", i, id);
                sh2gfw_Delete_Model_from_CharaID(id);
                subcharaupdate(id);
                BgCharaDelSlot(i);
            }
        }
    }
}

/* (blank lines: the asserts below have to sit on lines 152 and 154) */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 142
/**
 * Sets up the loader of the background characters in the shared work memory and empties both slots.
 */
void LoadBgCharaInit(void) {
    char *adr;
    char *cadr;

    printf("init bg chara\n");
    MemShareGetBgCharaWorkAddr();
    adr = MemShareGetBgCharaWorkAddr();
    assert_dw(adr);
    cadr = adr + 0x80000;
    assert(!((unsigned int)adr&0x3));
    init_clean();
    initwork();
    _loadBgMem_InitLoad(&d.ctrl, 0x8000, 16, d.UnitLArray, d.UnitRArray, adr);
    _loadBgMem_InitCache(&d.cache, 0x8000, 8, d.UnitCArray, cadr);
    d.SectList[0] = &d.SectListBuf[0];
    d.SectList[1] = &d.SectListBuf[1];
    d.SectList[2] = NULL;
    d.ctrlp = &d.ctrl;
    BgCharaManInit();
}

/** Returns whether both slots are loaded (no load units pending). */
int LoadBgCharaIsLoad(void) {
    int cnt;

    _loadBgMem_ClearRequest(d.ctrlp);
    loadBgMem_SetRequest(d.ctrlp, d.SectList);
    _loadBgMem_SyncLoadUnits(d.ctrlp);
    cnt = loadBgMem_CheckRequest(d.ctrlp, NULL);
    return !cnt || !d.ctrlp;
}

/* (blank lines: the assert below has to sit on line 192) */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 188
/** Returns whether slot `no` is loaded. */
int LoadBgCharaIsLoadSlot(int no) {
    int cnt;

    assert_dw(no>=0 && no<MAXSECT);
    d.CheckList[0] = &d.SectListBuf[no];
    _loadBgMem_ClearRequest(d.ctrlp);
    loadBgMem_SetRequest(d.ctrlp, d.CheckList);
    _loadBgMem_SyncLoadUnits(d.ctrlp);
    cnt = loadBgMem_CheckRequest(d.ctrlp, NULL);
    return !cnt || !d.ctrlp;
}

static int nowpos(void) {
    int glb_crd;
    float px;
    float pz;
    float dx;
    float dz;
    int nowid;
    int mapid[4];

    nowid = 0;
    if (sh2jms.player != NULL) {
        dx = 15.0f * sh2jms.pos.x;
        dz = 15.0f * sh2jms.pos.z;
        dx = fclamp(dx, -1000.0f, 1000.0f);
        dz = fclamp(dz, -1000.0f, 1000.0f);
        dx = dz = 0.0f;
        px = sh2jms.player->pos.x + dx;
        pz = sh2jms.player->pos.z + dz;
        if (stage != NULL) {
            glb_crd = stage->glb_crd;
            BlockNumber(mapid, glb_crd, px, pz);
            nowid = mapid[0];
        }
    }
    return nowid;
}

/** Returns whether the player is within 2750 units of the edge of a 20000-unit map block. */
int LoadBgCharaIsMapEdge(void) {
    float px;
    float pz;
    int x;
    int z;

    px = sh2jms.player->pos.x;
    pz = sh2jms.player->pos.z;
    px = fabsf(px);
    pz = fabsf(pz);
    x = ftoi(px);
    z = ftoi(pz);
    x %= 20000;
    z %= 20000;
    if (x > 2750.0f && x < 17250.0f && z > 2750.0f && z < 17250.0f) {
        return 0;
    }
    return 1;
}

/* (blank lines: the assert in olddel has to sit on line 257) */


static void olddel(union fsFileIndex *id, int slot) {
    struct sh2gfw_Model_Header *p;

    if (LoadBgCharaIsLoadSlot(slot) && d.filelist[slot].file != NULL && d.filelist[slot].file != id) {
        p = d.filelist[slot].addr;
        assert(p);
        if (p->chara_id != 0) {
            printf("slot%d(%d) clear\n", slot, p->chara_id);
            sh2gfw_Delete_Model_from_CharaID(p->chara_id);
            subcharaupdate(p->chara_id);
            BgCharaDelSlot(slot);
        }
    }
}

static int chkslot(void **tmp, void *ex) {
    if (ex != NULL) {
        if (ex == d.filelist[0].file) {
            tmp[0] = ex;
        } else if (ex == d.filelist[1].file) {
            tmp[1] = ex;
        } else {
            return 1;
        }
    }
    return 0;
}

static void setslot(struct FilesBgBlock *bgfiles) {
    void *tmpslot[2] = { 0 };
    int exchg[2] = { 0 };
    int slot;
    void *tbl[2] = { bgfiles->ex0, bgfiles->ex1 };

    if (bgfiles != NULL) {
        if (tbl[0] != NULL) {
            exchg[0] = chkslot(tmpslot, tbl[0]);
        }
        if (tbl[1] != NULL) {
            exchg[1] = chkslot(tmpslot, tbl[1]);
        }
        if (exchg[0]) {
            slot = tmpslot[0] ? 1 : 0;
            olddel(tbl[0], slot);
            d.filelist[slot].file = tbl[0];
            d.filelist[slot].ofsE = FcGetFileSize(tbl[0]);
            printf("slot%d(%s) load start\n", slot, ((union fsFileIndex *)tbl[0])->index.name);
            if (BgCharaGetBlockSize(tbl[0]) == 2) {
                slot = slot ? 0 : 1;
                printf("big block slot%d clear\n", slot);
                olddel(tbl[0], slot);
                d.filelist[slot].file = NULL;
                d.filelist[slot].ofsE = 0;
            }
        }
        if (exchg[1] && tbl[0] != tbl[1]) {
            slot = (tmpslot[0] || exchg[0]) ? 1 : 0;
            olddel(tbl[1], slot);
            d.filelist[slot].file = tbl[1];
            d.filelist[slot].ofsE = FcGetFileSize(tbl[1]);
            printf("slot%d(%s) load start\n", slot, ((union fsFileIndex *)tbl[1])->index.name);
        }
    }
}

/**
 * Per-frame update: requests the background characters of the player's current map block, runs
 * one step of their loading and passes the result to the character manager.
 */
void LoadBgCharaLoadSync(void) {
    int mapid;
    int glb_crd;
    int mapblock;
    struct FilesBgBlock *bgfiles;
    int cnt;

    memspy();
    mapid = nowpos();
    if (mapid != 0) {
        glb_crd = (mapid >> 16) & 0xFFFF;
        mapblock = mapid & 0xFFFF;
        bgfiles = FilesGetBgBlock(glb_crd, mapblock);
    }
    setslot(bgfiles);
    _loadBgMem_ClearRequest(d.ctrlp);
    loadBgMem_SetRequest(d.ctrlp, d.SectList);
    _loadBgMem_SyncLoadUnits(d.ctrlp);
    cnt = loadBgMem_LoadRequest(d.ctrlp, &d.cache, 2, 1, NULL, NULL, NULL, NULL);
    BgCharaMan(cnt);
}

/** Stores the addresses of the two loaded slots (NULL when empty) in `*slot0` and `*slot1`. */
void LoadBgGetUraCharaSlot(void **slot0, void **slot1) {
    if (d.ctrlp != NULL) {
        *slot0 = d.filelist[0].file ? d.filelist[0].addr : NULL;
        *slot1 = d.filelist[1].file ? d.filelist[1].addr : NULL;
    }
}
