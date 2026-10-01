/*
 * loadbg_2x2.c: section bookkeeping for the 2x2-block background loader.
 * Outdoors each of the 8 slots holds one block (4 files); indoors the two
 * 3-slot sections hold one room (5 blocks of 4 files).
 */

#include "sh2.h"

#define LBM2x2_SECTIONS_I 2
#define LBM2x2_SECTIONS_O 8
#define LBM2x2_SECTFILES_I 20
#define LBM2x2_SECTFILES_O 4

struct _loadBg2x2_Ctrl lb2x2Ctrl;

static void loadBg2x2_ResetSectFileList(struct loadBgMem_File *filelist, int n) {
    int i;

    for (i = 0; i < n; i++) {
        filelist[i].file = NULL;
        filelist[i].ofsS = 0;
        filelist[i].ofsE = 0;
        filelist[i].addr = NULL;
    }
}

static void loadBg2x2_ResetSectionAll(void) {
    struct _loadBg2x2_Ctrl *ctrl;
    int i;
    int j;
    int ofsSI;
    int ofsS;

    ctrl = &lb2x2Ctrl;
    for (i = 0; i < 2; i++) {
        ofsSI = i * 0x1E0000;
        ctrl->SectI[i].ofsS = ofsSI;
        ctrl->SectI[i].ofsE = ofsSI + 0x5A0000;
        ctrl->SectI[i].files = 0;
        ctrl->SectI[i].filelist = ctrl->FileI[i];
        ctrl->SectI[i].upper = i;
        ctrl->SectI[i].reduceRate8 = 0;
        ctrl->SectI[i].overwrite = 0;
        ctrl->SectI[i].sectID = 1;
        loadBg2x2_ResetSectFileList(ctrl->SectI[i].filelist, LBM2x2_SECTFILES_I);
        for (j = 0; j < 4; j++) {
            ofsS = j * 0x1E0000;
            ctrl->SectO[i + j * 2].ofsS = ofsS;
            ctrl->SectO[i + j * 2].ofsE = ofsS + 0x1E0000;
            ctrl->SectO[i + j * 2].files = 0;
            ctrl->SectO[i + j * 2].filelist = ctrl->FileO[i + j * 2];
            ctrl->SectI[i].upper = i;
            ctrl->SectI[i].reduceRate8 = 0;
            ctrl->SectO[i + j * 2].overwrite = 0;
            ctrl->SectO[i + j * 2].sectID = 1;
            loadBg2x2_ResetSectFileList(ctrl->SectO[i + j * 2].filelist, LBM2x2_SECTFILES_O);
        }
        ctrl->SectList[i] = NULL;
    }
    ctrl->Sections = 0;
}

static void *loadBg2x2_GetLoadWork(void) {
    void *addr;

    addr = MemShareGetBgLoadWorkAddr();
    if (addr == NULL) {
        lb2x2Ctrl.load_cleanup = 1;
    }
    return addr;
}

static void *loadBg2x2_GetCacheWork(void) {
    void *addr;

    addr = MemShareGetBgLoadCacheAddr();
    if (addr != NULL) {
        lb2x2Ctrl.cache_cleanup = 0;
    } else {
        lb2x2Ctrl.cache_cleanup = 1;
    }
    return addr;
}

static void loadBg2x2_CheckSlotWork(void) {
    int slot;

    for (slot = 0; slot < 8; slot++) {
        if (MemShareGetBgLoadSectionWorkAddr(slot) != NULL) {
            lb2x2Ctrl.slot_cleanup[slot] &= 2;
        } else {
            lb2x2Ctrl.slot_cleanup[slot] = 3;
        }
    }
}

/**
 * Makes sure the 2x2 loader's cache has its work memory, setting the cache up on a new work area.
 */
void loadBg2x2_CheckCacheWork(void) {
    int try;
    void *addr;

    if (lb2x2Ctrl.load != NULL) {
        try = 2;
        while (try > 0) {
            addr = loadBg2x2_GetCacheWork();
            if (addr != NULL) {
                if (lb2x2Ctrl.cache == NULL) {
                    lb2x2Ctrl.cache = loadBg2x2_CacheCtrl;
                    _loadBgMem_InitCache(loadBg2x2_CacheCtrl, 0x10000, 8, loadBg2x2_CacheUnit, addr);
                }
                break;
            }
            lb2x2Ctrl.cache = NULL;
            try--;
        }
    }
}

/**
 * Makes sure the 2x2 loader has its load work memory, setting the load control up on a new work
 * area.
 */
void loadBg2x2_CheckLoadWork(void) {
    int try;
    void *addr;

    try = 2;
    while (try > 0) {
        addr = loadBg2x2_GetLoadWork();
        if (addr != NULL) {
            if (lb2x2Ctrl.load == NULL) {
                lb2x2Ctrl.load = loadBg2x2_LoadCtrl;
                _loadBgMem_InitLoad(loadBg2x2_LoadCtrl, 0x10000, 120, loadBg2x2_LoadUnit, loadBg2x2_RequestUnit, addr);
                loadBg2x2_ResetSectionAll();
            }
            break;
        }
        lb2x2Ctrl.load = NULL;
        try--;
    }
    loadBg2x2_CheckCacheWork();
    loadBg2x2_CheckSlotWork();
}

/** Empties every section and the request list. */
void loadBg2x2_ClearRequest(void) {
    loadBg2x2_ResetSectionAll();
    _loadBgMem_ClearRequest(lb2x2Ctrl.load);
}

/** Turns the sections into the request list. */
void loadBg2x2_SetRequest(void) {
    loadBgMem_SetRequest(lb2x2Ctrl.load, lb2x2Ctrl.SectList);
}

/** Empties the loaded units nobody requests, and marks the slots clean when that succeeds. */
void loadBg2x2_CleanupNonRequest(void) {
    int slot;

    if (_loadBgMem_CleanupNonRequest(lb2x2Ctrl.load)) {
        for (slot = 0; slot < 8; slot++) {
            lb2x2Ctrl.slot_cleanup[slot] &= 1;
        }
    }
}

/** Returns the section (0-3) of outdoor block (`bx`, `bz`): the low bits of its coordinates. */
int loadBg2x2_GetOutdoorBlockSection(int bx, int bz) {
    return (bx & 1) + (bz & 1) * 2;
}

/** Returns the phase (0 or 1) of outdoor block (`bx`, `bz`) from bit 1 of its coordinates. */
int loadBg2x2_GetOutdoorBlockPhase(int bx, int bz) {
    return (((bx & 2) + (bz & 2)) & 2) >> 1;
}

/** Returns the cleanup state of the slot of outdoor block (`bx`, `bz`). */
int loadBg2x2_CheckLoadBufferOutdoor(int bx, int bz) {
    int cleanup;
    int bgslot;

    bgslot = loadBg2x2_GetSlotOutdoor(bx, bz);
    cleanup = lb2x2Ctrl.slot_cleanup[bgslot];
    return cleanup;
}
/* (blank lines in this file keep the asserts on their original source lines) */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 295
/**
 * Requests the files of outdoor block (`bx`, `bz`) (map `mid`) with priority `prio` and reduce rate
 * `reduceRate8`. Returns 0 when there is no load work.
 */
int loadBg2x2_SetRequestOutdoor(int prio, int reduceRate8, int bx, int bz, int mid) {
    struct _loadBg2x2_Ctrl *ctrl;
    struct loadBgMem_Sect *SectO;
    struct loadBgMem_File *sFile;
    int n;
    int i;
    int sectNo;

    ctrl = &lb2x2Ctrl;


    if (lb2x2Ctrl.load == NULL) {
        return 0;
    }
    if (loadBg2x2_CheckLoadBufferOutdoor(bx, bz)) {
        return 1;
    }

    assert_dw(ctrl->Sections<LBM2x2_SECTIONS_O);
    SectO = ctrl->SectO;
    sectNo = loadBg2x2_GetSlotOutdoor(bx, bz);
    SectO += sectNo;
    sFile = SectO->filelist;
    n = 0;
    if (mid != 0) {
        union fsFileIndex *file;
        struct FilesBgBlock *filesbg;
        int glb;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 338
        glb = (mid >> 16) & 0xFFFF;
        if (glb != 0) {
            assert(BgIsOut(glb));
        }
        filesbg = FilesGetBgBlock(glb, mid & 0xFFFF);
        file = filesbg->cam; sFile[n++].file = file;
        file = filesbg->cld; sFile[n++].file = file;
        file = filesbg->kg2; sFile[n++].file = file;
        file = filesbg->map; sFile[n++].file = file;
    } else {
        int j;

        for (j = 0; j < 4; j++) {
            sFile[n++].file = NULL;
        }
    }
    assert_dw(n==LBM2x2_SECTFILES_O);
    SectO->files = n;
    SectO->sectID = prio + 1;
    SectO->reduceRate8 = reduceRate8;
    ctrl->SectList[ctrl->Sections++] = SectO;
    for (i = 0; i < n; i++) {
        union fsFileIndex *file;
        int size;

        file = sFile[i].file;
        size = 0;
        if (file != NULL) {
            size = FcGetFileSize(file);
        }
        if (size <= 0) {
            size = 0;
            sFile[i].file = NULL;
        }
        sFile[i].ofsS = 0;
        sFile[i].ofsE = size;
        sFile[i].addr = NULL;
    }
    return 0;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 399
/** Returns the section (0 or 1) that room `roomid` uses. */
int loadBg2x2_GetIndoorRoomSection(int roomid) {
    int sect;

    sect = roomid & 1;
    switch (roomid) {
    case 29:
        sect ^= 1;
    }
    return sect;
}

/** Returns the cleanup state of the slots of room `roomid`. */
int loadBg2x2_CheckLoadBufferIndoor(int roomid) {
    int cleanup;
    int bgslot;

    cleanup = 0;
    bgslot = loadBg2x2_GetUseSlotIndoor(roomid);
    cleanup |= lb2x2Ctrl.slot_cleanup[bgslot];
    cleanup |= lb2x2Ctrl.slot_cleanup[bgslot ^ 1];
    cleanup |= lb2x2Ctrl.slot_cleanup[2];
    cleanup |= lb2x2Ctrl.slot_cleanup[3];
    cleanup |= lb2x2Ctrl.slot_cleanup[4];
    cleanup |= lb2x2Ctrl.slot_cleanup[5];
    return cleanup;
}

/**
 * Requests the files of room `roomid`'s four blocks `mid4`. Returns 0 when there is no load work.
 */
int loadBg2x2_SetRequestIndoor(int roomid, int *mid4) {
    struct _loadBg2x2_Ctrl *ctrl;
    struct loadBgMem_Sect *SectI;
    struct loadBgMem_File *sFile;
    int n;
    int slot;
    int i;
    int sectNo;
    int mid;

    ctrl = &lb2x2Ctrl;


    if (lb2x2Ctrl.load == NULL) {
        return 0;
    }
    if (loadBg2x2_CheckLoadBufferIndoor(roomid)) {
        return 1;
    }
    SectI = ctrl->SectI;
    assert(ctrl->Sections<=LBM2x2_SECTIONS_I);
    sectNo = loadBg2x2_GetIndoorRoomSection(roomid);
    SectI += sectNo;
    sFile = SectI->filelist;
    ctrl->SectList[ctrl->Sections++] = SectI;
    n = 0;
    for (slot = 0; slot < 5; slot++) {
        if (slot == 4) {
            if (((mid4[0] >> 16) & 0xFFFF) == 5) {
                mid = mid4[3] + 1;
            } else {
                mid = 0;
            }
        } else {
            mid = mid4[slot];
        }
        if (mid != 0) {
            union fsFileIndex *file;
            struct FilesBgBlock *filesbg;
            int glb;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 474
            glb = (mid >> 16) & 0xFFFF;
            assert(!BgIsOut(glb));
            filesbg = FilesGetBgBlock(glb, mid & 0xFFFF);
            file = filesbg->cam; sFile[n++].file = file;
            file = filesbg->cld; sFile[n++].file = file;
            file = filesbg->kg2; sFile[n++].file = file;
            file = filesbg->map; sFile[n++].file = file;
        } else {
            int j;

            for (j = 0; j < 4; j++) {
                sFile[n++].file = NULL;
            }
        }
    }

    assert(n==LBM2x2_SECTFILES_I);
    SectI->files = LBM2x2_SECTFILES_I;
    SectI->sectID = 1;
    SectI->reduceRate8 = 0;
    for (i = 0; i < n; i++) {
        union fsFileIndex *file;
        int size;

        file = sFile[i].file;
        size = 0;
        if (file != NULL) {
            size = FcGetFileSize(file);
        }
        if (size <= 0) {
            size = 0;
            sFile[i].file = NULL;
        }
        sFile[i].ofsS = 0;
        sFile[i].ofsE = size;
        sFile[i].addr = NULL;
    }
    return 0;
}
/**
 * loadBgMem_CheckRequest() on the 2x2 loader: returns the units still to load, also stored in
 * `*reqUnits`.
 */
int loadBg2x2_CheckRequest(int *reqUnits) {
    struct _loadBg2x2_Ctrl *ctrl;
    int ret;

    ctrl = &lb2x2Ctrl;
    ret = loadBgMem_CheckRequest(ctrl->load, reqUnits);
    return ret;
}
/**
 * loadBgMem_LoadRequest() on the 2x2 loader, with its cache and at most 20 cache moves and 5 file
 * reads.
 */
int loadBg2x2_LoadRequest(void) {
    struct _loadBg2x2_Ctrl *ctrl;
    int ret;

    ctrl = &lb2x2Ctrl;
    ret = loadBgMem_LoadRequest(ctrl->load, ctrl->cache, 20, 5, ctrl->cache_in_access_count,
                                ctrl->cache_out_access_count, ctrl->file_access_count, ctrl->miss_access_count);
    return ret;
}
/**
 * Registers the loaded data of outdoor slot `slot` (map `mid`) with the background data managers
 * (loadBgMAP_Replace() etc.). Returns the units still to load, also stored in `*reqUnits`.
 */
int loadBg2x2_ActivateRequestOutdoor(int slot, int mid, int *reqUnits) {
    struct _loadBg2x2_Ctrl *ctrl;
    struct loadBgMem_Sect *Sect;
    int ret;
    int block_no;
    struct loadBgMem_File *sFile;
    union fsFileIndex *file;
    int size;
    char *addr;

    ctrl = &lb2x2Ctrl;
    if (slot == 4) {
        loadBgMAP_Replace(slot, 0, NULL, 0);
    } else {
        Sect = ctrl->SectList[0];
        if (Sect != NULL) {
            assert(ctrl->SectList[1]==0);
        }
        ret = loadBgMem_CheckRequest(ctrl->load, reqUnits);
        block_no = mid & 0xFFFF;
        if (Sect == NULL || *reqUnits == 0 || ret > 0 || block_no == 0) {
            loadBgCAM_Replace(slot, 0, NULL, 0);
            loadBgCLD_Replace(slot, 0, NULL, 0);
            loadBgKG2_Replace(slot, 0, NULL, 0);
            loadBgMAP_Replace(slot, 0, NULL, 0);
        } else {
            sFile = Sect->filelist;
            file = sFile[0].file; /* Matching: dead; the DWARF has file, the line table a line for it */
            size = sFile[0].ofsE;
            addr = sFile[0].addr;
            loadBgCAM_Replace(slot, mid, addr, size);
            file = sFile[1].file;
            size = sFile[1].ofsE;
            addr = sFile[1].addr;
            loadBgCLD_Replace(slot, mid, addr, size);
            file = sFile[2].file;
            size = sFile[2].ofsE;
            addr = sFile[2].addr;
            loadBgKG2_Replace(slot, mid, addr, size);
            file = sFile[3].file;
            size = sFile[3].ofsE;
            addr = sFile[3].addr;
            loadBgMAP_Replace(slot, mid, addr, size);
        }
    }
    return ret;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 592
/**
 * Registers the loaded data of the room's blocks `mid4` with the background data managers. Returns
 * the units still to load, also stored in `*reqUnits`.
 */
int loadBg2x2_ActivateRequestIndoor(int *mid4, int *reqUnits) {
    struct _loadBg2x2_Ctrl *ctrl;
    struct loadBgMem_Sect *Sect;
    int ret;
    struct loadBgMem_File *sFile;
    int slot;
    int mid;
    int block_no;
    union fsFileIndex *file;
    int size;
    char *addr;

    ctrl = &lb2x2Ctrl;
    sFile = NULL;
    Sect = ctrl->SectList[0];
    if (Sect != NULL) {
        assert_dw(ctrl->SectList[1]==0);
    }
    ret = loadBgMem_CheckRequest(ctrl->load, reqUnits);
    if (Sect != NULL) {
        sFile = Sect->filelist;
    }
    for (slot = 0; slot < 5; slot++) {
        if (slot == 4) {
            if (((mid4[0] >> 16) & 0xFFFF) == 5) {
                mid = mid4[3] + 1;
            } else {
                mid = 0;
            }
        } else {
            mid = mid4[slot];
        }
        block_no = mid & 0xFFFF;
        if (Sect == NULL || *reqUnits == 0 || ret > 0 || block_no == 0) {
            if (slot <= 3) {
                loadBgCAM_Replace(slot, 0, NULL, 0);
            }
            if (slot <= 3) {
                loadBgCLD_Replace(slot, 0, NULL, 0);
            }
            if (slot <= 3) {
                loadBgKG2_Replace(slot, 0, NULL, 0);
            }
            loadBgMAP_Replace(slot, 0, NULL, 0);
            if (Sect != NULL) {
                sFile += 4;
            }
        } else {
            if (slot <= 3) {
                file = sFile[0].file; /* Matching: dead; the DWARF has file, the line table a line for it */
                size = sFile[0].ofsE;
                addr = sFile[0].addr;
                loadBgCAM_Replace(slot, mid, addr, size);
            }
            if (slot <= 3) {
                file = sFile[1].file;
                size = sFile[1].ofsE;
                addr = sFile[1].addr;
                loadBgCLD_Replace(slot, mid, addr, size);
            }
            if (slot <= 3) {
                file = sFile[2].file;
                size = sFile[2].ofsE;
                addr = sFile[2].addr;
                loadBgKG2_Replace(slot, mid, addr, size);
            }
            file = sFile[3].file;
            size = sFile[3].ofsE;
            addr = sFile[3].addr;
            loadBgMAP_Replace(slot, mid, addr, size);
            sFile += 4;
        }
    }
    return ret;
}

/** Returns the load slot of outdoor block (`bx`, `bz`). */
int loadBg2x2_GetSlotOutdoor(int bx, int bz) {
    int sectNo;
    int blockSect;
    int blockPhase;

    blockSect = loadBg2x2_GetOutdoorBlockSection(bx, bz);
    blockPhase = loadBg2x2_GetOutdoorBlockPhase(bx, bz);
    sectNo = blockPhase + blockSect * 2;
    return sectNo;
}

/** Returns the slot (0 or 7) a new room `roomid` loads into. */
int loadBg2x2_GetFreeSlotIndoor(int roomid) {
    return loadBg2x2_GetIndoorRoomSection(roomid) ? 0 : 7;
}

/**
 * Returns the load slot in use for room `roomid` (7 or 0, the opposite of
 * loadBg2x2_GetFreeSlotIndoor()).
 */
int loadBg2x2_GetUseSlotIndoor(int roomid) {
    return loadBg2x2_GetIndoorRoomSection(roomid) ? 7 : 0;
}
