/*
 * loadbg_mem.c: unit-based background streaming. A load control keeps a
 * buffer of UnitSize-byte units (UnitL) and a request list (UnitR); a
 * cache keeps an LRU list of spare units (UnitC).
 */

#include "sh2.h"

/**
 * Compares loaded unit `UnitL` with requested unit `UnitR`: whether each is empty, whether they
 * hold the same file unit, and whether UnitL's read is finished (fid < 0). Returns -4/-5 for a NULL
 * unit, otherwise a code from -3 to 4.
 */
int _loadBgMem_CmpUnitLR(struct _loadBgMem_UnitL *UnitL, struct _loadBgMem_UnitR *UnitR) {
    if (UnitL == NULL) {
        return -4;
    }
    if (UnitR == NULL) {
        return -5;
    }
    if (UnitL->file == NULL) {
        return UnitR->file == NULL ? 4 : 3;
    }
    if (UnitR->file == NULL) {
        return UnitL->fid < 0 ? 2 : -3;
    }
    if (UnitL->file == UnitR->file && UnitL->unitID == UnitR->unitID) {
        return UnitL->fid < 0 ? 1 : -2;
    }
    return UnitL->fid < 0 ? 0 : -1;
}

/** Returns whether cache unit `UnitC` holds unit `unitID` of `file` (-3 for a NULL unit). */
int _loadBgMem_CheckUnitC(struct _loadBgMem_UnitC *UnitC, union fsFileIndex *file, unsigned short unitID) {
    if (UnitC == NULL) {
        return -3;
    }
    if (UnitC->file != file) {
        return 0;
    }
    return !(UnitC->unitID != unitID);
}

/**
 * Checks the read of loaded unit `UnitL`. Returns 0 while it is running, 1 or 2 when it is done, -1
 * for an empty unit and -2 for NULL.
 */
int _loadBgMem_SyncUnitL(struct _loadBgMem_UnitL *UnitL) {
    int fid;

    if (UnitL == NULL) {
        return -2;
    }
    if (UnitL->file == NULL) {
        UnitL->unitID = 0;
        UnitL->fid = -1;
        return -1;
    }
    fid = UnitL->fid;
    if (fid < 0) {
        return 1;
    }
    if (fsSync(1, fid) < 0) {
        return 0;
    }
    UnitL->fid = -1;
    return 2;
}

/**
 * Empties loaded unit `UnitL` once its read has finished. Returns 1 when it is (or was) empty, 0
 * while the read is running, -1 for NULL.
 */
int _loadBgMem_ClearUnitL(struct _loadBgMem_UnitL *UnitL) {
    switch (_loadBgMem_SyncUnitL(UnitL)) {
    case -2:
    default:
        return -1;
    case 1:
    case 2:
        UnitL->file = NULL;
        UnitL->unitID = 0;
        UnitL->fid = -1;
    case -1:
        return 1;
    case 0:
        return 0;
    }
}

/** Empties cache unit `UnitC`. Returns 1 if it held data, 0 if it was empty, -1 for NULL. */
int _loadBgMem_ClearUnitC(struct _loadBgMem_UnitC *UnitC) {
    if (UnitC == NULL) {
        return -1;
    }
    if (UnitC->file != NULL) {
        UnitC->file = NULL;
        UnitC->unitID = 0;
        return 1;
    }
    return 0;
}

/** Links cache unit `UnitC` into the LRU list after `prevC`. Returns 0 if either is NULL. */
int _loadBgMem_AddUnitC(struct _loadBgMem_UnitC *UnitC, struct _loadBgMem_UnitC *prevC) {
    struct _loadBgMem_UnitC *nextC;

    if (UnitC == NULL) {
        return 0;
    }
    if (prevC == NULL) {
        return 0;
    }
    nextC = prevC->nextC;
    if (nextC != NULL) {
        nextC->prevC = UnitC;
    }
    prevC->nextC = UnitC;
    UnitC->nextC = nextC;
    UnitC->prevC = prevC;
    return 1;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 341
/** Unlinks cache unit `UnitC` from the LRU list of `cache`. */
void _loadBgMem_TakeoutUnitC(struct _loadBgMem_CacheCtrl *cache, struct _loadBgMem_UnitC *UnitC) {
    struct _loadBgMem_UnitC *headC;
    struct _loadBgMem_UnitC *tailC;
    struct _loadBgMem_UnitC *prevC;
    struct _loadBgMem_UnitC *nextC;

    if (cache != NULL && UnitC != NULL) {
        headC = cache->headC;
        tailC = cache->tailC;
        if (headC == NULL) {
            assert(headC==0&&tailC==0);
        }
        prevC = UnitC->prevC;
        nextC = UnitC->nextC;
        if (headC == UnitC) {
            assert_dw(prevC==0);
            cache->headC = nextC;
        } else {
            assert(prevC!=0);
            prevC->nextC = nextC;
        }
        if (tailC == UnitC) {
            assert(nextC==0);
            cache->tailC = prevC;
        } else {
            assert(nextC!=0);
            nextC->prevC = prevC;
        }
        UnitC->prevC = NULL;
        UnitC->nextC = NULL;
    }
}


/** Links the unlinked cache unit `UnitC` at the tail (oldest end) of the LRU list of `cache`. */
void _loadBgMem_ShiftUnitC(struct _loadBgMem_CacheCtrl *cache, struct _loadBgMem_UnitC *UnitC) {
    struct _loadBgMem_UnitC *tailC;
    struct _loadBgMem_UnitC *headC;

    if (cache != NULL && UnitC != NULL) {
        assert_dw(UnitC->prevC==0);
        assert(UnitC->nextC==0);
        tailC = cache->tailC;
        if (tailC == NULL) {
            headC = cache->headC;

            assert_dw(headC==0&&tailC==0);
        }
        cache->tailC = UnitC;
        if (tailC != NULL) { tailC->nextC = UnitC; }
        else { cache->headC = UnitC; }
        assert_dw(cache->headC!=0);
        UnitC->prevC = tailC;
        UnitC->nextC = NULL;
    }
}


/** Links the unlinked cache unit `UnitC` at the head (newest end) of the LRU list of `cache`. */
void _loadBgMem_PushUnitC(struct _loadBgMem_CacheCtrl *cache, struct _loadBgMem_UnitC *UnitC) {
    struct _loadBgMem_UnitC *headC;
    struct _loadBgMem_UnitC *tailC;

    if (cache != NULL && UnitC != NULL) {
        assert_dw(UnitC->prevC==0);
        assert(UnitC->nextC==0);
        headC = cache->headC;
        if (headC == NULL) {
            tailC = cache->tailC;

            assert_dw(headC==0&&tailC==0);
        }
        cache->headC = UnitC;
        if (headC != NULL) { headC->prevC = UnitC; }
        else { cache->tailC = UnitC; }
        assert_dw(cache->tailC!=0);
        UnitC->prevC = NULL;
        UnitC->nextC = headC;
    }
}
/** Returns the least recently used unit of `cache`. */
struct _loadBgMem_UnitC *_loadBgMem_SearchOldestUnitC(struct _loadBgMem_CacheCtrl *cache) {
    if (cache == NULL) {
        return NULL;
    }
    return cache->tailC;
}
/** Returns the unit of `cache` holding unit `unitID` of `file`, or NULL. */
struct _loadBgMem_UnitC *_loadBgMem_SearchUnitC(struct _loadBgMem_CacheCtrl *cache, union fsFileIndex *file, unsigned short unitID) {
    struct _loadBgMem_UnitC *UnitC;

    if (cache == NULL) {
        return NULL;
    }
    if (file == NULL) {
        return NULL;
    }
    for (UnitC = cache->headC; UnitC != NULL; UnitC = UnitC->nextC) {
        if (_loadBgMem_CheckUnitC(UnitC, file, unitID) == 1) {
            break;
        }
    }
    return UnitC;
}
/** Returns the cache unit holding the data requested by `UnitR`, or NULL. */
struct _loadBgMem_UnitC *_loadBgMem_SearchUnitR2C(struct _loadBgMem_CacheCtrl *cache, struct _loadBgMem_UnitR *UnitR) {
    union fsFileIndex *file;
    unsigned short unitID;

    if (UnitR == NULL) {
        return NULL;
    }
    file = UnitR->file;
    unitID = UnitR->unitID;
    return _loadBgMem_SearchUnitC(cache, file, unitID);
}

/** Returns the cache unit holding the data of loaded unit `UnitL`, or NULL. */
struct _loadBgMem_UnitC *_loadBgMem_SearchUnitL2C(struct _loadBgMem_CacheCtrl *cache, struct _loadBgMem_UnitL *UnitL) {
    union fsFileIndex *file;
    unsigned short unitID;

    if (UnitL == NULL) {
        return NULL;
    }
    file = UnitL->file;
    unitID = UnitL->unitID;
    return _loadBgMem_SearchUnitC(cache, file, unitID);
}

/** Moves the data of cache unit `UnitC` into loaded unit `UnitL` of `ctrl`. */
void _loadBgMem_MoveMemC2L(struct _loadBgMem_LoadCtrl *ctrl, struct _loadBgMem_CacheCtrl *cache, struct _loadBgMem_UnitL *UnitL, struct _loadBgMem_UnitC *UnitC) {
    int UnitSize;
    char *BufL;
    char *BufC;
    int unitNoL;
    int unitNoC;

    assert(ctrl!=0);
    assert_dw(cache!=0);



    UnitSize = ctrl->UnitSize;
    BufL = ctrl->Buffer;
    BufC = cache->Buffer;
    assert(UnitL!=0);
    assert_dw(UnitC!=0);
    unitNoL = UnitL->unitNo;
    unitNoC = UnitC->unitNo;
    BufL += unitNoL * UnitSize;
    BufC += unitNoC * UnitSize;
    UtilMemCpy(BufL, BufC, UnitSize);
    UnitL->file = UnitC->file;
    UnitL->unitID = UnitC->unitID;
    UnitL->fid = -1;
}



/** Moves the data of loaded unit `UnitL` of `ctrl` into cache unit `UnitC`. */
void _loadBgMem_MoveMemL2C(struct _loadBgMem_LoadCtrl *ctrl, struct _loadBgMem_CacheCtrl *cache, struct _loadBgMem_UnitL *UnitL, struct _loadBgMem_UnitC *UnitC) {
    int UnitSize;
    char *BufL;
    char *BufC;
    int unitNoL;
    int unitNoC;

    assert(ctrl!=0);
    assert_dw(cache!=0);



    UnitSize = ctrl->UnitSize;
    BufL = ctrl->Buffer;
    BufC = cache->Buffer;
    assert(UnitL!=0);
    assert_dw(UnitC!=0);
    unitNoL = UnitL->unitNo;
    unitNoC = UnitC->unitNo;
    BufL += unitNoL * UnitSize;
    BufC += unitNoC * UnitSize;
    UtilMemCpy(BufC, BufL, UnitSize);
    UnitC->file = UnitL->file;
    UnitC->unitID = UnitL->unitID;
}

/** Swaps the data of loaded unit `UnitL` of `ctrl` and cache unit `UnitC`. */
void _loadBgMem_SwapMemLC(struct _loadBgMem_LoadCtrl *ctrl, struct _loadBgMem_CacheCtrl *cache, struct _loadBgMem_UnitL *UnitL, struct _loadBgMem_UnitC *UnitC) {
    int UnitSize;
    char *BufL;
    char *BufC;
    int unitNoL;
    int unitNoC;
    union fsFileIndex *file;
    unsigned short unitID;

    assert(ctrl!=0);
    assert_dw(cache!=0);



    UnitSize = ctrl->UnitSize;
    BufL = ctrl->Buffer;
    BufC = cache->Buffer;
    assert(UnitL!=0);
    assert_dw(UnitC!=0);
    unitNoL = UnitL->unitNo;
    unitNoC = UnitC->unitNo;
    UtilMemSwap(BufL + unitNoL * UnitSize, BufC + unitNoC * UnitSize, UnitSize);
    file = UnitL->file;
    unitID = UnitL->unitID;
    UnitL->file = UnitC->file;
    UnitL->unitID = UnitC->unitID;
    UnitC->file = file;
    UnitC->unitID = unitID;
    UnitL->fid = -1;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 567
/**
 * Starts reading the data requested by `UnitR` from its file into loaded unit `UnitL` of `ctrl`.
 */
int _loadBgMem_LoadMemR2L(struct _loadBgMem_LoadCtrl *ctrl, struct _loadBgMem_UnitL *UnitL, struct _loadBgMem_UnitR *UnitR) {
    int UnitSize;
    char *BufL;
    int offset;
    union fsFileIndex *file;
    int fid;
    int unitID;
    int unitNo;

    assert(ctrl!=0);


    UnitSize = ctrl->UnitSize;
    BufL = ctrl->Buffer;
    assert_dw(UnitL!=0);
    assert_dw(UnitR!=0);
    unitNo = UnitL->unitNo;
    assert_dw(unitNo==UnitR->unitNo);
    BufL += unitNo * UnitSize;
    file = UnitR->file;
    unitID = UnitR->unitID;
    offset = unitID * UnitSize;
    fid = FcReadPart(file, BufL, offset, UnitSize);
    if (fid >= 0) {
        UnitL->file = file;
        UnitL->unitID = unitID;
        UnitL->fid = fid;
        return 1;
    }
    return 0;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 631
/**
 * Sets up load control `ctrl` with `Units` units of `UnitSize` bytes (a multiple of 2048) in
 * `Buffer` (64-byte aligned), and its unit arrays `UnitLArray` and `UnitRArray`.
 */
int _loadBgMem_InitLoad(struct _loadBgMem_LoadCtrl *ctrl, int UnitSize, int Units, struct _loadBgMem_UnitL *UnitLArray, struct _loadBgMem_UnitR *UnitRArray, char *Buffer) {
    int i;
    struct _loadBgMem_UnitL *UnitL;
    struct _loadBgMem_UnitR *UnitR;

    if (ctrl != NULL) {
        assert((UnitSize%2048)==0);
        assert_dw((((int)Buffer)%64)==0);
        ctrl->UnitSize = UnitSize;
        ctrl->Units = Units;
        ctrl->UnitLArray = UnitLArray;
        ctrl->UnitRArray = UnitRArray;
        ctrl->Buffer = Buffer;
        for (i = 0; i < Units; i++, UnitLArray++, UnitRArray++, Buffer += UnitSize) {
            UnitL = UnitLArray;
            UnitL->file = NULL;
            UnitL->unitID = 0;
            UnitL->unitNo = i;
            UnitL->fid = -1;
            UnitL->pad = 0;
            UnitR = UnitRArray;
            UnitR->file = NULL;
            UnitR->unitID = 0;
            UnitR->unitNo = i;
            UnitR->sectID = 0;
            UnitR->nextR = NULL;
        }
    }
    return ctrl != NULL;
}



/**
 * Sets up cache `cache` with `Units` units of `UnitSize` bytes in `Buffer`, linked in LRU order
 * from `UnitCArray`.
 */
int _loadBgMem_InitCache(struct _loadBgMem_CacheCtrl *cache, int UnitSize, int Units, struct _loadBgMem_UnitC *UnitCArray, char *Buffer) {
    int i;
    struct _loadBgMem_UnitC *UnitC;
    struct _loadBgMem_UnitC *headC;
    struct _loadBgMem_UnitC *tailC;

    if (cache != NULL) {
        assert((UnitSize%2048)==0);
        assert_dw((((int)Buffer)%64)==0);
        cache->UnitSize = UnitSize;
        cache->Units = Units;
        cache->UnitCArray = UnitCArray;
        cache->Buffer = Buffer;
        for (i = 0, UnitC = UnitCArray; i < Units; i++, UnitC++, Buffer += UnitSize) {
            UnitC->file = NULL;
            UnitC->unitID = 0;
            UnitC->unitNo = i;
            UnitC->prevC = NULL;
            UnitC->nextC = NULL;
            UtilMemSet(Buffer, 0, UnitSize);
        }
        headC = NULL;
        tailC = NULL;
        for (i = 0, UnitC = UnitCArray; i < Units; i++, UnitC++) {
            if (!_loadBgMem_AddUnitC(UnitC, tailC)) {
                headC = UnitC;
            }
            tailC = UnitC;
        }
        cache->headC = headC;
        cache->tailC = tailC;
    }
    return cache != NULL;
}
/** Empties the request list of `ctrl`. */
int _loadBgMem_ClearRequest(struct _loadBgMem_LoadCtrl *ctrl) {
    struct _loadBgMem_UnitR *UnitR;
    int i;
    int Units;

    if (ctrl == NULL) {
        return 0;
    }
    Units = ctrl->Units;
    for (i = 0, UnitR = ctrl->UnitRArray; i < Units; i++, UnitR++) {
        UnitR->file = NULL;
        UnitR->unitID = 0;
        UnitR->unitNo = i;
        UnitR->sectID = 0;
        UnitR->nextR = NULL;
    }
    ctrl->headR = NULL;
    return 1;
}
/** Builds the request list of `ctrl` from the units of the sections in `SectList`. */
int loadBgMem_SetRequest(struct _loadBgMem_LoadCtrl *ctrl, struct loadBgMem_Sect **SectList) {
    struct _loadBgMem_UnitR *UnitRArray;
    struct _loadBgMem_UnitR *headR;
    struct _loadBgMem_UnitR *tailR;
    struct loadBgMem_Sect *Sect;
    struct loadBgMem_Sect **Sect_p;
    int UnitSize;
    int Units;
    char *Buffer;
    int ret;
    int Dir;
    int sUnits;
    int sUnits0;
    int overwrite;
    int sectID;
    int reduceRate8;
    struct loadBgMem_File *sFile;
    int sFiles;
    struct _loadBgMem_UnitR *UnitR;
    char *addr;
    int sofsS;
    int sofsE;
    int unitNoS;
    int unitNoE;
    union fsFileIndex *file;
    int fUnits;
    int unitID;
    int fofsS;
    int fofsE;
    int unitIDS;
    int unitIDE;

    if (ctrl == NULL) {
        return 0;
    }
    ret = 0;
    Units = ctrl->Units;
    UnitSize = ctrl->UnitSize;
    UnitRArray = ctrl->UnitRArray;
    Buffer = ctrl->Buffer;
    headR = NULL; tailR = NULL;
    for (Sect_p = SectList; (Sect = *Sect_p) != NULL; Sect_p++) {
        Dir = Sect->upper ? -1 : 1;
        overwrite = Sect->overwrite;
        sectID = Sect->sectID;
        sFiles = Sect->files;
        reduceRate8 = Sect->reduceRate8;
        sofsS = Sect->ofsS;
        sofsE = Sect->ofsE;
        assert(sofsS%UnitSize==0);
        assert_dw(sofsE%UnitSize==0);
        unitNoS = sofsS / UnitSize;
        unitNoE = sofsE / UnitSize;
        sUnits = unitNoE - unitNoS;
        UnitR = &UnitRArray[unitNoS];
        assert_dw(unitNoE<=Units);

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 810
        if (Dir < 0) {
            UnitR += sUnits - 1;
        }
        sUnits0 = sUnits;
        if (reduceRate8 > 0) {
            if (reduceRate8 >= 8) {
                reduceRate8 = 8;
            }
            sUnits = sUnits * (8 - reduceRate8) / 8;
        }
        for (sFile = Sect->filelist; sFiles > 0 && sUnits > 0; sFiles--, sFile++) {
            file = sFile->file;
            fofsS = sFile->ofsS;
            fofsE = sFile->ofsE;
            unitIDS = fofsS / UnitSize;
            unitIDE = (fofsE + UnitSize - 1) / UnitSize;
            fUnits = unitIDE - unitIDS;
            unitID = (Dir > 0) ? unitIDS : unitIDE - 1;
            addr = Buffer + (UnitR - UnitRArray) * UnitSize;
            for (; fUnits > 0 && sUnits > 0; fUnits--, sUnits--, UnitR += Dir, unitID += Dir) {
                if (file != NULL) {
                    if (UnitR->sectID == 0 || (overwrite && UnitR->sectID > sectID)) {
                        UnitR->file = file;
                        UnitR->unitID = unitID;
                        UnitR->sectID = sectID;
                        ret++;
                        if (UnitR->nextR == NULL) {
                            if (tailR != NULL) {
                                tailR->nextR = UnitR;
                            } else {
                                assert(headR==0);
                                headR = UnitR;
                            }
                            tailR = UnitR;
                            UnitR->nextR = NULL;
                        }
                    }
                }
            }

            if (fUnits <= 0) {
                if (Dir < 0) {
                    addr = Buffer + (UnitR + 1 - UnitRArray) * UnitSize;
                }
                sFile->addr = addr;
            }
            assert(fUnits<=sUnits0-sUnits);
        }
    }
    ctrl->headR = headR;
    return ret;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 870
/** Returns the number of units of `ctrl` whose reads are still running. */
int _loadBgMem_SyncLoadUnits(struct _loadBgMem_LoadCtrl *ctrl) {
    struct _loadBgMem_UnitL *UnitL;
    int Units;
    int ret;

    if (ctrl == NULL) {
        return 0;
    }
    for (Units = ctrl->Units, UnitL = ctrl->UnitLArray, ret = 0; Units > 0; Units--, UnitL++) {
        switch (_loadBgMem_SyncUnitL(UnitL)) {
        case 0:
            ret++;
            break;
        case 1:
        case 2:
        case -1:
            break;
        case -2:
        default:
            assert_dw(0);
        }
    }
    return ret;
}
/**
 * Checks the requests of `ctrl` against the loaded units. Stores the number of units still to load
 * in `*reqUnits` (if not NULL) and returns it.
 */
int loadBgMem_CheckRequest(struct _loadBgMem_LoadCtrl *ctrl, int *reqUnits) {
    struct _loadBgMem_UnitL *UnitL;
    struct _loadBgMem_UnitR *UnitR;
    int UnitSize;
    int Units;
    int ret;
    int req;

    if (ctrl == NULL) {
        if (reqUnits != NULL) {
            *reqUnits = 1;
        }
        return 1;
    }
    Units = ctrl->Units;
    UnitSize = ctrl->UnitSize; /* Matching: dead; the DWARF has UnitSize, the line table a line for it */
    UnitL = ctrl->UnitLArray;
    UnitR = ctrl->UnitRArray;
    ret = 0;
    req = 0;
    for (; Units > 0; Units--, UnitL++, UnitR++) {
        switch (_loadBgMem_CmpUnitLR(UnitL, UnitR)) {
        case 3:
            ret++;
            req++;
            break;
        case 1:
            req++;
            break;
        case 0:
            ret++;
            req++;
            break;
        case -1:
            ret++;
            req++;
            break;
        case -2:
            ret++;
            req++;
            break;
        case 4: case 2: case -3:
            break;
        case -4: case -5: default:
/* Matching: #line keeps the assert on its original line. */
#line 942
            assert_dw(0);
        }
    }

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 951
    for (Units = 0, UnitR = ctrl->headR; UnitR != NULL; UnitR = UnitR->nextR) {
        Units++;
    }
    assert_dw(Units==req);
    if (reqUnits != NULL) {
        *reqUnits = req;
    }
    return ret;
}

/**
 * Empties the loaded units of `ctrl` that hold data no longer requested. Returns 0 if a read still
 * running kept one from being emptied.
 */
int _loadBgMem_CleanupNonRequest(struct _loadBgMem_LoadCtrl *ctrl) {
    struct _loadBgMem_UnitL *UnitL;
    struct _loadBgMem_UnitR *UnitR;
    int i;
    int Units;
    int result;
    int ret;

    if (ctrl == NULL) {
        return 0;
    }
    Units = ctrl->Units;
    ret = 1;
    for (i = 0, UnitL = ctrl->UnitLArray, UnitR = ctrl->UnitRArray; i < Units; i++, UnitL++, UnitR++) {
        switch (_loadBgMem_CmpUnitLR(UnitL, UnitR)) {
        case 2:
        case -3:
            result = _loadBgMem_ClearUnitL(UnitL);
            if (result != 1) {
                ret = 0;
            }
            break;
        case -2:
        case -1:
        case 0:
        case 1:
        case 3:
        case 4:
            break;
        case -4:
        case -5:
        default:
            assert_dw(!"???");
        }
    }
    return ret;
}

/** Loads requested units of `ctrl`, from `cache` if there, within the per-call limits. */
int loadBgMem_LoadRequest(struct _loadBgMem_LoadCtrl *ctrl, struct _loadBgMem_CacheCtrl *cache, int cache_access_limit, int file_access_limit, int *cache_in_access_count, int *cache_out_access_count, int *file_access_count, int *miss_access_count) {
    struct _loadBgMem_UnitL *UnitLArray;
    struct _loadBgMem_UnitR *UnitR;
    int UnitSize;
    int fa_cnt;
    int ca_in;
    int ca_out;
    int ca_hit;
    int miss;
    struct _loadBgMem_UnitL *UnitL;
    struct _loadBgMem_UnitC *UnitC;
    int result;

    ca_in = 0;
    ca_out = 0;
    ca_hit = 0;
    miss = 0;
    if (ctrl == NULL) {
        if (cache_in_access_count != NULL) {
            *cache_in_access_count = 0;
        }
        if (cache_out_access_count != NULL) {
            *cache_out_access_count = 0;
        }
        if (file_access_count != NULL) {
            *file_access_count = 0;
        }
        if (miss_access_count != NULL) {
            *miss_access_count = 1;
        }
        return 1;
    }
    UnitSize = ctrl->UnitSize;
    if (cache != NULL) {
        assert_dw(UnitSize==cache->UnitSize);
    }
    UnitLArray = ctrl->UnitLArray;
    fa_cnt = _loadBgMem_SyncLoadUnits(ctrl);
    for (UnitR = ctrl->headR; UnitR != NULL; UnitR = UnitR->nextR) {
        UnitL = &UnitLArray[UnitR->unitNo];
        result = _loadBgMem_CmpUnitLR(UnitL, UnitR);
        switch (result) {
        case 0:
        case 3:
            UnitC = _loadBgMem_SearchUnitR2C(cache, UnitR);
            if (UnitC != NULL) {
                _loadBgMem_TakeoutUnitC(cache, UnitC);
                if (ca_in + ca_out < cache_access_limit) {
                    ca_hit++;
                    if (result == 3) {
                        _loadBgMem_MoveMemC2L(ctrl, cache, UnitL, UnitC);
                        _loadBgMem_ClearUnitC(UnitC);
                        _loadBgMem_ShiftUnitC(cache, UnitC);
                        ca_out++;
                    } else {
                        struct _loadBgMem_UnitC *findC;

                        findC = _loadBgMem_SearchUnitL2C(cache, UnitL);
                        if (findC != NULL) {
                            _loadBgMem_TakeoutUnitC(cache, findC);
                            _loadBgMem_PushUnitC(cache, findC);
                            _loadBgMem_MoveMemC2L(ctrl, cache, UnitL, UnitC);
                            _loadBgMem_ClearUnitC(UnitC);
                            _loadBgMem_ShiftUnitC(cache, UnitC);
                            ca_out++;
                        } else {
                            _loadBgMem_SwapMemLC(ctrl, cache, UnitL, UnitC);
                            ca_in++;
                            ca_out++;
                            _loadBgMem_PushUnitC(cache, UnitC);
                        }
                    }
                } else {
                    miss++;
                    _loadBgMem_PushUnitC(cache, UnitC);
                }
            } else if (fa_cnt < file_access_limit) {
                UnitC = _loadBgMem_SearchOldestUnitC(cache);
                if (result == 3 || UnitC == NULL) {
                    _loadBgMem_LoadMemR2L(ctrl, UnitL, UnitR);
                    fa_cnt++;
                } else if (ca_in + ca_out < cache_access_limit) {
                    struct _loadBgMem_UnitC *findC;

                    findC = _loadBgMem_SearchUnitL2C(cache, UnitL);
                    if (findC != NULL) {
                        _loadBgMem_TakeoutUnitC(cache, findC);
                        _loadBgMem_PushUnitC(cache, findC);
                    } else {
                        _loadBgMem_TakeoutUnitC(cache, UnitC);
                        _loadBgMem_MoveMemL2C(ctrl, cache, UnitL, UnitC);
                        _loadBgMem_PushUnitC(cache, UnitC);
                    }
                    _loadBgMem_LoadMemR2L(ctrl, UnitL, UnitR);
                    ca_in++;
                    fa_cnt++;
                } else {
                    miss += 2;
                }
            } else {
                miss++;
            }
            break;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1152
        case -1:
            miss++;
            break;
        case -3:
        case -2:
        case 1:
        case 2:
        case 4:
            break;
        case -4:
        case -5:
        default:
            assert(0);
        }
    }
    if (cache_in_access_count != NULL) {
        *cache_in_access_count = ca_in;
    }
    if (cache_out_access_count != NULL) {
        *cache_out_access_count = ca_out;
    }
    if (file_access_count != NULL) {
        *file_access_count = fa_cnt;
    }
    if (miss_access_count != NULL) {
        *miss_access_count = miss;
    }
    return fa_cnt + miss;
}
