/*
 * loadbg_all.c: decides which 2x2 background blocks to load around the
 * player. Outdoors the current block and its seven nearest neighbours are
 * sorted by distance and requested with a reduce rate that grows with the
 * distance; indoors the room's four blocks are requested.
 */

#include "sh2.h"

#define SQ(x) ((x) * (x))

struct loadBgAll_2x2Info lbAll_2x2Info;

/** Makes sure the 2x2 loader has its work memory. */
void loadBgAll_Init(void) {
    loadBg2x2_CheckLoadWork();
}

static void loadBgAll_Set2x2Block(int id, int glb_crd, int block_no, int bx, int bz, float dist2) {
    int mid;

    mid = 0;
    if (glb_crd != 0 && block_no != 0) {
        mid = (glb_crd << 16) | block_no;
    }
    lbAll_2x2Info.block[id].mid = mid;
    lbAll_2x2Info.block[id].bx = bx;
    lbAll_2x2Info.block[id].bz = bz;
    lbAll_2x2Info.block[id].dist2 = dist2;
}

/*
 * Blocks: 0 current, 1 x-neighbour, 2 z-neighbour, 3 diagonal, 5/6 the
 * neighbours on the far side, 4/7 the next ones out. Sorted nearest first.
 */
static void loadBgAll_Sort2x2Block(void) {
    struct loadBgAll_2x2Block *block;
    struct loadBgAll_2x2Block **sort;
    float nx;
    float nz;
    float nxz;
    float fx;
    float fz;
    float fxz;

    block = lbAll_2x2Info.block;
    sort = lbAll_2x2Info.sort;
    *sort++ = &lbAll_2x2Info.block[0];
    nx = block[1].dist2;
    nz = block[2].dist2;
    nxz = block[3].dist2;
    fx = block[5].dist2;
    fz = block[6].dist2;
    fxz = block[7].dist2;
    if (nx <= nz) {
        *sort++ = &block[1];
        *sort++ = &block[2];
        if (nxz <= fz) {
            *sort++ = &block[3];
            *sort++ = &block[6];
            if (fx <= fxz) {
                *sort++ = &block[5];
                *sort++ = &block[7];
            } else {
                *sort++ = &block[7];
                *sort++ = &block[5];
            }
        } else {
            *sort++ = &block[6];
            if (nxz <= fx) {
                *sort++ = &block[3];
                if (fx <= fxz) {
                    *sort++ = &block[5];
                    *sort++ = &block[7];
                } else {
                    *sort++ = &block[7];
                    *sort++ = &block[5];
                }
            } else {
                *sort++ = &block[5];
                *sort++ = &block[3];
                *sort++ = &block[7];
            }
        }
    } else {
        *sort++ = &block[2];
        *sort++ = &block[1];
        if (nxz <= fx) {
            *sort++ = &block[3];
            *sort++ = &block[5];
            if (fz <= fxz) {
                *sort++ = &block[6];
                *sort++ = &block[7];
            } else {
                *sort++ = &block[7];
                *sort++ = &block[6];
            }
        } else {
            *sort++ = &block[5];
            if (nxz <= fz) {
                *sort++ = &block[3];
                if (fz <= fxz) {
                    *sort++ = &block[6];
                    *sort++ = &block[7];
                } else {
                    *sort++ = &block[7];
                    *sort++ = &block[6];
                }
            } else {
                *sort++ = &block[6];
                *sort++ = &block[3];
                *sort++ = &block[7];
            }
        }
    }
    *sort = NULL;
}

static int loadBgAll_SetLoadRequestOutdoor2x2(void) {
    struct loadBgAll_2x2Block **sort;
    float dist2;
    int ret;
    int reduceRate8;
    int cleanup;
    int idx;

    cleanup = 0;
    for (sort = lbAll_2x2Info.sort; *sort != NULL; sort++) {
        idx = *sort - lbAll_2x2Info.block;
        reduceRate8 = 0;
        if (lbAll_2x2Info.block[idx ^ 4].mid != 0) {
            dist2 = (*sort)->dist2;
            reduceRate8 += dist2 > SQ(7500.0f);
            reduceRate8 += dist2 > SQ(8500.0f);
            reduceRate8 += dist2 > SQ(9500.0f);
            reduceRate8 += dist2 > SQ(10500.0f);
            reduceRate8 += dist2 > SQ(11500.0f);
            reduceRate8 += dist2 > SQ(12500.0f);
            reduceRate8 += dist2 > SQ(14000.0f);
        }
        if (dist2 < SQ(17500.0f)) {
            ret = loadBg2x2_SetRequestOutdoor(0, reduceRate8, (*sort)->bx, (*sort)->bz, (*sort)->mid);
            cleanup += ret;
        }
    }
    return cleanup;
}

static int loadBgAll_SetCheckRequestOutdoor2x2(void) {
    struct loadBgAll_2x2Block **sort;
    float dist2;
    int ret;

    ret = 0;
    for (sort = lbAll_2x2Info.sort; *sort != NULL; sort++) {
        dist2 = (*sort)->dist2;
        if (dist2 < SQ(5250.0f)) {
            ret++;
            loadBg2x2_SetRequestOutdoor(ret, 0, (*sort)->bx, (*sort)->bz, (*sort)->mid);
        }
    }
    return ret;
}

static int loadBgAll_SetCheckActivateOutdoor2x2(struct loadBgAll_2x2Block *block) {
    float dist2;

    dist2 = block->dist2;
    if (dist2 < SQ(10000.0f)) {
        loadBg2x2_SetRequestOutdoor(1, 0, block->bx, block->bz, block->mid);
    }
    return loadBg2x2_GetOutdoorBlockSection(block->bx, block->bz);
}

static int loadBgAll_2x2LoadRequest(int cleanup, int *ret3p, int *ret1p) {
    int ret1;
    int ret2;
    int ret3;

    loadBg2x2_SetRequest();
    if (cleanup) {
        loadBg2x2_CleanupNonRequest();
    }
    ret1 = loadBg2x2_LoadRequest();
    ret2 = loadBg2x2_CheckRequest(&ret3);
    if (ret1p != NULL) {
        *ret1p = ret1;
    }
    if (ret3p != NULL) {
        *ret3p = ret3;
    }
    return ret2;
}

static int loadBgAll_2x2CheckRequest(int *ret5p) {
    int ret4;
    int ret5;

    loadBg2x2_SetRequest();
    ret4 = loadBg2x2_CheckRequest(&ret5);
    if (ret5p != NULL) {
        *ret5p = ret5;
    }
    return ret4;
}

static int loadBgAll_2x2ActivateRequestOutdoor(int *ret2p) {
    struct loadBgAll_2x2Block *block;
    int retA[4];
    int retB[4];
    int ret1;
    int ret2;
    int i;
    int slot;

    ret1 = 0;
    ret2 = 0;
    for (i = 0; i <= 3; i++) {
        loadBg2x2_ClearRequest();
        block = &lbAll_2x2Info.block[i];
        slot = loadBgAll_SetCheckActivateOutdoor2x2(block);
        loadBg2x2_SetRequest();
        retA[i] = loadBg2x2_ActivateRequestOutdoor(slot, block->mid, &retB[i]);
        ret1 += retA[i];
        ret2 += retB[i];
    }
    loadBg2x2_ActivateRequestOutdoor(4, 0, NULL);
    if (ret2p != NULL) {
        *ret2p = ret2;
    }
    return ret1;
}

static int loadBgAll_ExecLoadRequestOutdoor2x2(int *needUnits, int *remUnits, int *reqUnits, int *miss) {
    int ret1;
    int ret2;
    int ret3;
    int ret4;
    int ret5;
    int ret6;
    int ret7;
    int cleanup;

    loadBg2x2_ClearRequest();
    cleanup = loadBgAll_SetLoadRequestOutdoor2x2();
    ret2 = loadBgAll_2x2LoadRequest(cleanup, &ret3, &ret1);
    loadBg2x2_ClearRequest();
    loadBgAll_SetCheckRequestOutdoor2x2();
    ret4 = loadBgAll_2x2CheckRequest(&ret5);
    ret6 = loadBgAll_2x2ActivateRequestOutdoor(&ret7);
    if (miss != NULL) {
        *miss = ret1;
    }
    if (reqUnits != NULL) {
        *reqUnits = ret3;
    }
    if (remUnits != NULL) {
        *remUnits = ret2;
    }
    if (needUnits != NULL) {
        *needUnits = ret5;
    }
    if (cleanup) {
        ret4 += ret1;
    }
    return ret4;
}

static int loadBgAll_SetLoadRequestIndoor2x2(int rid, int glb_crd, int *mid4) {
    int cleanup;

    loadBg2x2_ClearRequest();
    cleanup = loadBg2x2_SetRequestIndoor(rid, mid4);
    return cleanup != 0;
}

static int loadBgAll_2x2ActivateRequestIndoor(int *mid4, int *ret2p) {
    int ret1;
    int ret2;

    ret1 = loadBg2x2_ActivateRequestIndoor(mid4, &ret2);
    if (ret2p != NULL) {
        *ret2p = ret2;
    }
    return ret1;
}

static int loadBgAll_ExecLoadRequestIndoor2x2(int rid, int glb_crd, int *blocks, int *reqUnits, int *miss) {
    int ret1;
    int ret2;
    int ret3;
    int ret4;
    int ret5;
    int mid4[4];
    int cleanup;
    int i;
    int mid;
    int block_no;

    for (i = 0; i < 4; i++) {
        mid = 0;
        block_no = blocks[i];
        if (block_no != 0) {
            mid = (glb_crd << 16) | block_no;
        }
        mid4[i] = mid;
    }
    cleanup = loadBgAll_SetLoadRequestIndoor2x2(rid, glb_crd, mid4);
    ret2 = loadBgAll_2x2LoadRequest(cleanup, &ret3, &ret1);
    ret4 = loadBgAll_2x2ActivateRequestIndoor(mid4, &ret5);
    if (ret2 < ret4) {
        ret2 = ret4;
    }
    if (ret3 < ret5) {
        ret3 = ret5;
    }
    if (miss != NULL) {
        *miss = ret1;
    }
    if (reqUnits != NULL) {
        *reqUnits = ret3;
    }
    if (cleanup) {
        ret2 += ret1;
    }
    return ret2;
}

/**
 * Requests the background blocks around the current position (outdoors the nearest 2x2 blocks by
 * distance, indoors the room's blocks) and runs the loads. Stores the units still loading in
 * `*loading` and the units requested in `*require` (either may be NULL); returns the units in use.
 */
int loadBgAll_PrepareAround(int *loading, int *require) {
    struct _loadBgCommon_Info_T *info;
    int nbdx;
    int nbdz;
    int cpbx;
    int cpbz;
    float dx;
    float dz;
    int icx;
    int icz;
    int glb_crd;
    int rid;
    int prio;
    int force;
    int lockUnits;
    int needUnits;
    int remUnits;
    int reqUnits;
    int miss;
    float pfcx;
    float pfcz;

    info = _loadBgCommon_Info;
    loadBg2x2_CheckLoadWork();
    glb_crd = _loadBgCommon_Info->glb_crd;
    if (glb_crd == 0) {
        return 0;
    }
    icx = info->icx;
    icz = info->icz;
    cpbx = icx - info->minx;
    cpbz = icz - info->minz;
    rid = info->AroundID[cpbx][cpbz];
    if (BgIsOut(glb_crd)) {
        loadBgAll_Set2x2Block(0, glb_crd, rid, icx, icz, 0.0f);
        pfcx = info->px - info->fcx;
        pfcz = info->pz - info->fcz;
        nbdx = (pfcx >= 0.0f) ? 1 : -1;
        nbdz = (pfcz >= 0.0f) ? 1 : -1;
        dx = 10000.0f + ((nbdx < 0) ? pfcx : -pfcx);
        dz = 10000.0f + ((nbdz < 0) ? pfcz : -pfcz);
        loadBgAll_Set2x2Block(1, glb_crd, info->AroundID[cpbx + nbdx][cpbz], icx + nbdx, icz, SQ(dx));
        loadBgAll_Set2x2Block(2, glb_crd, info->AroundID[cpbx][cpbz + nbdz], icx, icz + nbdz, SQ(dz));
        loadBgAll_Set2x2Block(3, glb_crd, info->AroundID[cpbx + nbdx][cpbz + nbdz], icx + nbdx, icz + nbdz, SQ(dx) + SQ(dz));
        if (dx <= dz) {
            loadBgAll_Set2x2Block(4, glb_crd, info->AroundID[cpbx + nbdx + nbdx][cpbz], icx + nbdx + nbdx, icz, SQ(20000.0f + dx));
            loadBgAll_Set2x2Block(7, glb_crd, info->AroundID[cpbx + nbdx][cpbz - nbdz], icx + nbdx, icz - nbdz, SQ(dx) + SQ(20000.0f - dz));
        } else {
            loadBgAll_Set2x2Block(4, glb_crd, info->AroundID[cpbx][cpbz + nbdz + nbdz], icx, icz + nbdz + nbdz, SQ(20000.0f + dz));
            loadBgAll_Set2x2Block(7, glb_crd, info->AroundID[cpbx - nbdx][cpbz + nbdz], icx - nbdx, icz + nbdz, SQ(dz) + SQ(20000.0f - dx));
        }
        loadBgAll_Set2x2Block(5, glb_crd, info->AroundID[cpbx - nbdx][cpbz], icx - nbdx, icz, SQ(20000.0f - dx));
        loadBgAll_Set2x2Block(6, glb_crd, info->AroundID[cpbx][cpbz - nbdz], icx, icz - nbdz, SQ(20000.0f - dz));
        loadBgAll_Sort2x2Block();
        lockUnits = loadBgAll_ExecLoadRequestOutdoor2x2(&needUnits, &remUnits, &reqUnits, &miss);
    } else {
        remUnits = loadBgAll_ExecLoadRequestIndoor2x2(rid, glb_crd, info->BlockID, &reqUnits, &miss);
        needUnits = reqUnits;
        lockUnits = remUnits;
    }
    if (loading != NULL) {
        *loading = remUnits;
    }
    if (require != NULL) {
        *require = reqUnits;
    }
    info->unit = reqUnits;
    info->load = remUnits;
    info->need = needUnits;
    info->lock = lockUnits;
    info->miss = miss;
    return lockUnits;
}
