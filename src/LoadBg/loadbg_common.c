/*
 * loadbg_common.c: state shared by the background loaders: where the camera
 * is (in 20000-unit blocks) and which rooms/blocks are around it.
 */

#include "sh2.h"

/* Block size in world units. */
#define BLOCK_SIZE 20000

struct _loadBgCommon_Info_T _loadBgCommon_Info[1] = {0};

/* Matching: #line keeps the numbering of the lines below. */
#line 19
/** Reads a whole file into loadbuf; returns its size (0 when there is none). */
int _loadBgCommon_LoadData(void *loadbuf, union fsFileIndex *file, int limit) {
    int size;
    int fid;

    if (file) {
        size = FcGetFileSize(file);
        if (size > 0) {
            assert(size<=limit);
            do {
                fid = FcRead(file, loadbuf);
            } while (fid == -1);
            fsSync(0, fid);
        } else {
            size = 0;
        }
    } else {
        size = 0;
    }
    return size;
}

/*
 * (int)f rounded towards minus infinity: truncate, then subtract one for negative values (for a
 * negative whole number too, as the code does). Name invented. Matching: an inline function, not
 * asm in BPOSfromFPOS: the original's line table has one statement for the whole sequence (asm in
 * a function body gets an entry per instruction), and its DWARF has i in v0 (no register), which
 * MWCC gives a local that only copies an inline function's result.
 */
inline int floor_to_int(float f) {
    int i;

    asm {
        mfc1 i, f
        addi t7, zero, 1
        slt i, i, zero
        cvt.w.s f, f
        movz t7, zero, i
        mfc1 i, f
        sub i, i, t7
    }
    return i;
}

/** Block index of a world coordinate (floor(f / BLOCK_SIZE)). */
int BPOSfromFPOS(float f) {
    int i;
    int ret;

    i = floor_to_int(f);
    if (i < 0) {
        ret = -((BLOCK_SIZE - 1 - i) / BLOCK_SIZE);
    } else {
        ret = i / BLOCK_SIZE;
    }
    return ret;
}

/** World coordinate of the center of block i. */
float FPOSfromBPOS(int i) {
    float ret;

    ret = (int)(i * (float)BLOCK_SIZE + BLOCK_SIZE / 2);
    return ret;
}

/* Matching: stand-in for a dead-stripped function with double arithmetic (common.h), fitted, not
 * recovered: loadBgCommon_SetInfo allocates registers as after one (docs/stand-ins.md). It sits
 * in the gap before loadBgCommon_GetBlockNoOutdoor, where the original's line table leaves room
 * for code (41 lines; the file's usual gap between functions is 2). Its body is still fitted. */
STRIPPED_DOUBLE_CODE()
static int loadBgCommon_GetBlockNoOutdoor(int glb_crd, int bx, int bz) {
    int id;
    int blocks[4];
    struct _loadBgCommon_Info_T *info;

    info = _loadBgCommon_Info;
    BlockNumber(blocks, glb_crd, bx, bz);
    id = blocks[0] & 0xFFFF;
    if (_loadBgCommon_Info->hide_map) {
        if (blocks[0] == info->hide_map) {
            id = 0;
        }
    }
    return id;
}

/**
 * Updates the shared loader state for position (`px`, `pz`) in map `glb_crd`: the current block and
 * room, the IDs of the 5x5 blocks around it and the blocks to load. Returns info->unit (0 for an
 * invalid map).
 */
int loadBgCommon_SetInfo(int glb_crd, float px, float pz) {
    static int init;
    struct _loadBgCommon_Info_T *info;
    int outdoor;
    float bx;
    float bz;
    int ix;
    int iz;
    int icx;
    int icz;
    int minx;
    int minz;
    float fcx;
    float fcz;

    info = _loadBgCommon_Info;
    if (!init) {
        init = 1;
        loadBgAll_Init();
    }
    info->hide_map = info->hide_map_request;
    info->hide_map_request = 0;
    if (glb_crd <= 0) {
        return 0;
    }
    info->glb_crd = glb_crd;
    icx = BPOSfromFPOS(px);
    icz = BPOSfromFPOS(pz);
    fcx = FPOSfromBPOS(icx);
    fcz = FPOSfromBPOS(icz);
    minx = icx - 2;
    minz = icz - 2;
    info->px = px;
    info->pz = pz;
    info->fcx = fcx;
    info->fcz = fcz;
    info->icx = icx;
    info->icz = icz;
    info->minx = minx;
    info->minz = minz;
    outdoor = BgIsOut(glb_crd);
    info->outdoor = outdoor;
    info->RoomID = RoomName(glb_crd, fcx, fcz);
    for (ix = 0; ix < 5; ix++) {
        bx = FPOSfromBPOS(ix + minx);
        for (iz = 0; iz < 5; iz++) {
            bz = FPOSfromBPOS(minz + iz);
            info->AroundID[ix][iz] = outdoor ? loadBgCommon_GetBlockNoOutdoor(glb_crd, bx, bz) : RoomName(glb_crd, bx, bz);
        }
    }
    if (outdoor) {
        info->BlockID[0] = loadBgCommon_GetBlockNoOutdoor(glb_crd, fcx, fcz);
        for (ix = 1; ix < 4; ix++) {
            info->BlockID[ix] = 0;
        }
    } else {
        BlockNumber(info->BlockID, glb_crd, fcx, fcz);
for (ix = 0; ix < 4; ix++) {
            info->BlockID[ix] = info->BlockID[ix] & 0xFFFF;
        }
    }
    return info->unit;
}

/**
 * Asks for outdoor map block `mapid` to be hidden from the next update on (0: none). Returns the
 * previous request.
 */
/* Hides one outdoor map block from the next SetInfo on; returns the previous request. */
int loadBgCommon_HideMapBlockOutdoor(int mapid) {
    struct _loadBgCommon_Info_T *info;
    int prev;

    info = _loadBgCommon_Info;
    prev = info->hide_map_request;
    info->hide_map_request = mapid;
    return prev;
}
