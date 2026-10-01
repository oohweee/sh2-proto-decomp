/*
 * loadbg_kg2.c: bookkeeping for the shadow (KG2) data of the loaded
 * background blocks, one slot per block.
 */

#include "sh2.h"

#define LBM_KG2_SLOTS 4

struct loadBgKG2_Ctrl lbKG2_Ctrl = {0};

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 21
static int _loadBgKG2_Delete(int slot) {
    void *addr;
    assert(0<=slot && slot<LBM_KG2_SLOTS);
    addr = lbKG2_Ctrl.addr[slot];
    lbKG2_Ctrl.mapid[slot] = 0;
    lbKG2_Ctrl.addr[slot] = NULL;
    lbKG2_Ctrl.size[slot] = 0;
    return addr != NULL;
}

static int _loadBgKG2_Regist(int slot, int mapid, void *addr, int size) {
    struct SHADOW_OUTDOOR_HEAD *header;
    assert(0<=slot && slot<LBM_KG2_SLOTS);
    if (mapid == lbKG2_Ctrl.mapid[slot] && lbKG2_Ctrl.addr[slot] == addr && lbKG2_Ctrl.size[slot] == size) {
        return 0;
    }
    lbKG2_Ctrl.mapid[slot] = mapid;
    lbKG2_Ctrl.addr[slot] = addr;
    lbKG2_Ctrl.size[slot] = size;
    if (addr) {
        header = addr;
        header->kind = (mapid >> 16) & 0xFFFF;
        header->map_id = mapid & 0xFFFF;
    }
    return 1;
}

static int _loadBgKG2_Replace(int slot, int mapid, void *addr, int size) {
    int ret;

    ret = 0;
    if (mapid != lbKG2_Ctrl.mapid[slot] || lbKG2_Ctrl.addr[slot] != addr || lbKG2_Ctrl.size[slot] != size) {
        if (_loadBgKG2_Delete(slot)) {
            ret += 1;
        }
    }
    if (mapid && addr && size > 0) {
        if (_loadBgKG2_Regist(slot, mapid, addr, size)) {
            ret += 2;
        }
    }
    return ret;
}

/**
 * Puts the shadow data `addr` (`size` bytes, map `mapid`; NULL to remove) in slot `slot`. Returns 0
 * for no change, +1 when old data was removed, +2 when new data was registered.
 */
int loadBgKG2_Replace(int slot, int mapid, void *addr, int size) {
    int ret;

    ret = _loadBgKG2_Replace(slot, mapid, addr, size);
    switch (ret) {
    case 1:
        verbose(1, "- kg2(%d)\n", slot);
        break;
    case 2:
        verbose(1, " +kg2(%d):0x%08x(@0x%08x+0x%08x)\n", slot, mapid, addr, size);
        break;
    case 3:
        verbose(1, "-+kg2(%d):0x%08x(@0x%08x+0x%08x)\n", slot, mapid, addr, size);
        break;
    }
    return ret;
}

/** Returns a NULL-terminated list of the loaded shadow data. */
void **loadBgKG2_GetLoadedDataAddrList(void) {
    int i;
    int j;
    void *addr;

    j = 0;
    for (i = 0; i < LBM_KG2_SLOTS; i++) {
        addr = lbKG2_Ctrl.addr[i];
        if (addr) {
            lbKG2_Ctrl.list[j++] = addr;
        }
    }
    lbKG2_Ctrl.list[j] = NULL;
    return lbKG2_Ctrl.list;
}
