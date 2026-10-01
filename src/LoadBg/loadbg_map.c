/*
 * loadbg_map.c: bookkeeping for the map data of the loaded background:
 * slot 0 is the area (GB) texture, slot 1 the room (TR) texture, and
 * slots 2.. the map blocks. Each change is handed to the graphics side.
 */

#include "sh2.h"

#define LBM_TEX_SLOTS 2
#define LBM_MAP_SLOTS 5

/* printf/verbose text tagged with "<file>:<line>> " like the asserts. */
#define LOG_HEAD __FILE__ ":" SH_STRINGIFY(__LINE__) "> "

static int loadBgTEX_AreaInit(int init);
static int loadBgTEX_RoomInit(int init);
static int loadBgMAP_BlockInit(int slot, int init);

struct loadBgTEX_Ctrl lbMAP_Ctrl = {0};

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 26
static int _loadBgMAP_Delete(int slot) {
    void *addr;
    assert(0<=slot && slot<LBM_TEX_SLOTS+LBM_MAP_SLOTS);
    addr = lbMAP_Ctrl.addr[slot];
    lbMAP_Ctrl.mapid[slot] = 0;
    lbMAP_Ctrl.addr[slot] = NULL;
    lbMAP_Ctrl.size[slot] = 0;
    if (!addr) {
        return 0;
    }
    switch (slot) {
    case 0:
        return loadBgTEX_AreaInit(0);
    case 1:
        return loadBgTEX_RoomInit(0);
    default:
        return loadBgMAP_BlockInit(slot - LBM_TEX_SLOTS, 0);
    }
}

static int _loadBgMAP_Regist(int slot, int mapid, void *addr, int size) {
    assert(0<=slot && slot<LBM_TEX_SLOTS+LBM_MAP_SLOTS);
    if (mapid == lbMAP_Ctrl.mapid[slot] && lbMAP_Ctrl.addr[slot] == addr && lbMAP_Ctrl.size[slot] == size) {
        return 0;
    }
    lbMAP_Ctrl.mapid[slot] = mapid;
    lbMAP_Ctrl.addr[slot] = addr;
    lbMAP_Ctrl.size[slot] = size;
    switch (slot) {
    case 0:
        return loadBgTEX_AreaInit(1);
    case 1:
        return loadBgTEX_RoomInit(1);
    default:
        return loadBgMAP_BlockInit(slot - LBM_TEX_SLOTS, 1);
    }
}

static int _loadBgMAP_Replace(int slot, int mapid, void *addr, int size) {
    int ret;

    ret = 0;
    if (mapid != lbMAP_Ctrl.mapid[slot] || lbMAP_Ctrl.addr[slot] != addr || lbMAP_Ctrl.size[slot] != size) {
        if (_loadBgMAP_Delete(slot)) {
            ret += 1;
        }
    }
    if (mapid) {
        if (_loadBgMAP_Regist(slot, mapid, addr, size)) {
            ret += 2;
        }
    }
    return ret;
}

/**
 * Puts the texture data `addr` (`size` bytes, map `mapid`; NULL to remove) in texture slot `slot`
 * (0 area, 1 room). Returns 0 for no change, +1 when old data was removed, +2 when new data was
 * registered.
 */
int loadBgTEX_Replace(int slot, int mapid, void *addr, int size) {
    int ret;

    ret = _loadBgMAP_Replace(slot, mapid, addr, size);
    switch (ret) {
    case 1:
        verbose(1, "- map(%c)\n", slot ? 'T' : 'G');
        break;
    case 2:
        verbose(1, " +map(%c):0x%08x(@0x%08x+0x%08x)\n", slot ? 'T' : 'G', mapid, addr, size);
        break;
    case 3:
        verbose(1, "-+map(%c):0x%08x(@0x%08x+0x%08x)\n", slot ? 'T' : 'G', mapid, addr, size);
        break;
    }
    return ret;
}

/**
 * Puts the map block data `addr` (`size` bytes, map `mapid`; NULL to remove) in map slot `slot`.
 * Returns 0 for no change, +1 when old data was removed, +2 when new data was registered.
 */
int loadBgMAP_Replace(int slot, int mapid, void *addr, int size) {
    int ret;

    ret = _loadBgMAP_Replace(slot + LBM_TEX_SLOTS, mapid, addr, size);
    switch (ret) {
    case 1:
        verbose(1, "- map(%d)\n", slot);
        break;
    case 2:
        verbose(1, " +map(%d):0x%08x(@0x%08x+0x%08x)\n", slot, mapid, addr, size);
        break;
    case 3:
        verbose(1, "-+map(%d):0x%08x(@0x%08x+0x%08x)\n", slot, mapid, addr, size);
        break;
    }
    return ret;
}

/** Removes the data of every map slot. */
void loadBgMAP_AllClear(void) {
    int i;

    for (i = 0; i < LBM_MAP_SLOTS; i++) {
        loadBgMAP_Replace(i, 0, NULL, 0);
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 153
static int loadBgTEX_AreaInit(int init) {
    int mapid;
    void *addr;
    int size;

    addr = lbMAP_Ctrl.addr[0];
    if (init) {
        mapid = lbMAP_Ctrl.mapid[0];
        size = lbMAP_Ctrl.size[0];
        if (mapid && addr) {
            if (size > 0x40000) {
                sh2gfw_Set_GB_Tex(addr);
                verbose(1, LOG_HEAD "GB tex: registed\n");
            } else {
                printf(LOG_HEAD "GB tex: probably, illegal size(%d)!!\n", size);
            }
        } else {
            printf(LOG_HEAD "GB tex: not exist!!\n");
        }
    } else {
        sh2gfw_Delete_GB_Tex();



        verbose(1, LOG_HEAD "GB tex: removed\n");
    }
    return 1;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 185
static int loadBgTEX_RoomInit(int init) {
    int mapid;
    void *addr;
    int size;

    addr = lbMAP_Ctrl.addr[1];
    if (init) {
        mapid = lbMAP_Ctrl.mapid[1];
        size = lbMAP_Ctrl.size[1];
        if (mapid && addr) {
            if (size > 0x40000) {
                sh2gfw_Set_TR_Tex(addr);
                verbose(1, LOG_HEAD "TR tex: registed\n");
            } else {
                printf(LOG_HEAD "TR tex: probably, illegal size(%d)!!\n", size);
            }
        } else {
            printf(LOG_HEAD "TR tex: not exist!!\n");
        }
    } else {
        sh2gfw_Delete_TR_Tex();



        verbose(1, LOG_HEAD "TR tex: removed\n");
    }
    return 1;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 217
static int loadBgMAP_BlockInit(int slot, int init) {
    int mapid; /* @bug only set when init; the "removed" log prints whatever the caller left in its register */
    void *addr;
    int size;

    addr = lbMAP_Ctrl.addr[slot + LBM_TEX_SLOTS];
    if (init) {
        mapid = lbMAP_Ctrl.mapid[slot + LBM_TEX_SLOTS];
        size = lbMAP_Ctrl.size[slot + LBM_TEX_SLOTS];
        if (mapid && addr && size > 0) {
            sh2gfw_Set_BlockLocal(slot, addr, mapid);
            verbose(1, LOG_HEAD "Block(0x%08x): registed\n", mapid);
        } else {
            printf(LOG_HEAD "Block(0x%08x): not exist!!\n", mapid);
        }
        return 1;
    } else {



        verbose(1, LOG_HEAD "Block(0x%08x): removed\n", mapid);
        sh2gfw_Delete_BlockLocal(slot);
        return 1;
    }
}

