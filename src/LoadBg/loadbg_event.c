/*
 * loadbg_event.c: loads event files into a 8KB-unit area through the
 * background loader's memory manager (loadbg_mem.c). Every file gets its own
 * section of whole units.
 */

#include "sh2.h"
#include "libc/string.h"

#define EVENT_UNIT_SIZE 0x2000
#define EVENT_UNITS 1024
#define EVENT_FILES 100
#define EVENT_SECTS 100

static struct loadBgEvent_Work d;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 40
static void initwork(void) {
    memset(d.filelist, 0, sizeof(d.filelist));
    d.SectList[0] = NULL;
}

static struct loadBgMem_File *getfilelist(void) {
    int i;

    for (i = 0; i < EVENT_UNITS; i++) {
        if (!d.filelist[i].file) {
            return &d.filelist[i];
        }
    }
    return NULL;
}

static struct loadBgMem_Sect *getfreesect(void) {
    int i;

    for (i = 0; i < EVENT_UNITS; i++) {
        if (!d.SectListBuf[i].ofsE) {
            return &d.SectListBuf[i];
        }
    }
    assert_dw(0);
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 70
static struct loadBgMem_Sect *getsect(void *adr) {
    int i;
    struct loadBgMem_Sect *p;

    for (i = 0; i < EVENT_UNITS; i++) {
        if (d.ctrl.Buffer + i * EVENT_UNIT_SIZE == adr) {
            p = getfreesect();
            p->ofsS = i * EVENT_UNIT_SIZE;
            return p;
        }
    }
    assert_dw(0);
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 87
static struct loadBgMem_Sect *setsect(void *adr, int size) {
    struct loadBgMem_Sect *p;

    p = getsect(adr);
    assert_dw(!p->ofsE);
    p->ofsE = p->ofsS + (size + EVENT_UNIT_SIZE - 1) / EVENT_UNIT_SIZE * EVENT_UNIT_SIZE;
    return p;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 106
static void addsectlist(struct loadBgMem_Sect *sect) {
    int i;
    struct loadBgMem_Sect **l;

    for (l = d.SectList, i = 0; i < EVENT_SECTS && *l; i++, l++) {
    }
    assert(!(*l && i==100));
    l[0] = sect;
    l[1] = NULL;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 142
/**
 * Sets up the event load area at `adr` (4-byte aligned, `size` bytes, in 8 KB units) and empties
 * it.
 */
void LoadBgEventInit(void *adr, unsigned int size) {
    int Units;

    assert(!((unsigned int)adr&0x3));
    Units = (size + EVENT_UNIT_SIZE - 1) / EVENT_UNIT_SIZE;
    _loadBgMem_InitLoad(&d.ctrl, EVENT_UNIT_SIZE, Units, d.UnitLArray, d.UnitRArray, adr);
    _loadBgMem_InitCache(&d.cache, EVENT_UNIT_SIZE, 4, d.UnitCArray, d.c_buf);
    initwork();
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 168
/** Adds event file `fileid` to the load list, to be loaded at `adr`. */
void LoadBgEventFileLoad(union fsFileIndex *fileid, void *adr) {
    struct loadBgMem_Sect *p;
    int size;

    size = FcGetFileSize(fileid);
    p = setsect(adr, size);
    assert_dw(p);
    p->files = 1;
    p->filelist = getfilelist();
    assert_dw(p->filelist);
    p->filelist->file = fileid;
    p->filelist->ofsE = size;
    addsectlist(p);
}

/**
 * Requests the files on the load list and runs the loads (at most 2 cache moves and 2 file reads).
 */
void LoadBgEventLoadSync(void) {
    _loadBgMem_ClearRequest(&d.ctrl);
    loadBgMem_SetRequest(&d.ctrl, d.SectList);
    _loadBgMem_SyncLoadUnits(&d.ctrl);
    loadBgMem_LoadRequest(&d.ctrl, &d.cache, 2, 2, NULL, NULL, NULL, NULL);
}

/** Returns the number of files on the load list. */
int LoadBgEventListCnt(void) {
    int i;
    struct loadBgMem_Sect **p;

    for (p = d.SectList, i = 0; *p && i < EVENT_SECTS; i++, p++) {
    }
    return i;
}

/** Returns the number of units of the listed files still to load. */
int LoadBgEventLoadCnt(void) {
    int cnt;

    _loadBgMem_ClearRequest(&d.ctrl);
    loadBgMem_SetRequest(&d.ctrl, d.SectList);
    _loadBgMem_SyncLoadUnits(&d.ctrl);
    cnt = loadBgMem_CheckRequest(&d.ctrl, NULL);
    return cnt;
}

/** Returns whether every listed file is loaded. */
int LoadBgEventIsLoad(void) {
    return !LoadBgEventLoadCnt();
}

/** Empties the load list. */
void LoadBgEventDispose(void) {
    initwork();
}
