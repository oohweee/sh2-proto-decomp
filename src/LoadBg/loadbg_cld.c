/*
 * loadbg_cld.c: bookkeeping for the collision (CLD) data of the loaded
 * background blocks, one slot per block.
 */

#include "sh2.h"

#define LBM_CLD_SLOTS 16

struct loadBgCLD_Ctrl lbCLD_Ctrl = {0};

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 19
static int _loadBgCLD_Delete(int slot) {
    void *addr;
    assert(0<=slot && slot<LBM_CLD_SLOTS);
    addr = lbCLD_Ctrl.addr[slot];
    lbCLD_Ctrl.mapid[slot] = 0;
    lbCLD_Ctrl.addr[slot] = NULL;
    lbCLD_Ctrl.size[slot] = 0;
    return addr != NULL;
}


static int _loadBgCLD_Regist(int slot, int mapid, void *addr, int size) {
    assert(0<=slot && slot<LBM_CLD_SLOTS);
    if (mapid == lbCLD_Ctrl.mapid[slot] && lbCLD_Ctrl.addr[slot] == addr && lbCLD_Ctrl.size[slot] == size) {
        return 0;
    }
    lbCLD_Ctrl.mapid[slot] = mapid;
    lbCLD_Ctrl.addr[slot] = addr;
    lbCLD_Ctrl.size[slot] = size;
    return 1;
}

static int _loadBgCLD_Replace(int slot, int mapid, void *addr, int size) {
    int ret;

    ret = 0;
    if (mapid != lbCLD_Ctrl.mapid[slot] || lbCLD_Ctrl.addr[slot] != addr || lbCLD_Ctrl.size[slot] != size) {
        if (_loadBgCLD_Delete(slot)) {
            ret += 1;
        }
    }
    if (mapid && addr && size > 0) {
        if (_loadBgCLD_Regist(slot, mapid, addr, size)) {
            ret += 2;
        }
    }
    return ret;
}

/**
 * Puts the collision data `addr` (`size` bytes, map `mapid`; NULL to remove) in slot `slot`.
 * Returns 0 for no change, +1 when old data was removed, +2 when new data was registered.
 */
int loadBgCLD_Replace(int slot, int mapid, void *addr, int size) {
    int ret;

    ret = _loadBgCLD_Replace(slot, mapid, addr, size);
    switch (ret) {
    case 1:
        verbose(1, "- cld(%d)\n", slot);
        break;
    case 2:
        verbose(1, " +cld(%d):0x%08x(@0x%08x+0x%08x)\n", slot, mapid, addr, size);
        break;
    case 3:
        verbose(1, "-+cld(%d):0x%08x(@0x%08x+0x%08x)\n", slot, mapid, addr, size);
        break;
    }
    return ret;
}

/** Returns a NULL-terminated list of the loaded collision data. */
void **loadBgCLD_GetLoadedDataAddrList(void) {
    int i;
    int j;
    void *addr;

    j = 0;
    for (i = 0; i < LBM_CLD_SLOTS; i++) {
        addr = lbCLD_Ctrl.addr[i];
        if (addr) {
            lbCLD_Ctrl.list[j++] = addr;
        }
    }
    lbCLD_Ctrl.list[j] = NULL;
    return lbCLD_Ctrl.list;
}
