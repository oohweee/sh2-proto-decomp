/*
 * loadbg_cam.c: bookkeeping for the camera (CAM) data of the loaded
 * background blocks. Slot 0 is the global camera file, slots 1.. are the
 * blocks. Every change re-registers the camera road lists with the view
 * camera (vcWork).
 */

#include "sh2.h"

#define LBM_CAM_SLOTS 17

struct loadBgCAM_Ctrl lbCAM_Ctrl = {0};

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 25
static int _loadBgCAM_Delete(int slot) {
    void *addr;
    assert(0<=slot && slot<LBM_CAM_SLOTS);
    addr = lbCAM_Ctrl.addr[slot];
    lbCAM_Ctrl.mapid[slot] = 0;
    lbCAM_Ctrl.addr[slot] = NULL;
    lbCAM_Ctrl.size[slot] = 0;
    if (!addr) {
        return 0;
    }
    loadBgCAM_vcReset();
    return 1;
}



static int _loadBgCAM_Regist(int slot, int mapid, void *addr, int size) {
    assert(0<=slot && slot<LBM_CAM_SLOTS);
    if (mapid == lbCAM_Ctrl.mapid[slot] && lbCAM_Ctrl.addr[slot] == addr && lbCAM_Ctrl.size[slot] == size) {
        return 0;
    }
    lbCAM_Ctrl.mapid[slot] = mapid;
    lbCAM_Ctrl.addr[slot] = addr;
    lbCAM_Ctrl.size[slot] = size;
    if (addr) {
        vcConvertCamFile(addr);
        loadBgCAM_vcReset();
    }
    return 1;
}

static int _loadBgCAM_Replace(int slot, int mapid, void *addr, int size) {
    int ret;

    ret = 0;
    if (mapid != lbCAM_Ctrl.mapid[slot] || lbCAM_Ctrl.addr[slot] != addr || lbCAM_Ctrl.size[slot] != size) {
        if (_loadBgCAM_Delete(slot)) {
            ret += 1;
        }
    }
    if (mapid && addr && size > 0) {
        if (_loadBgCAM_Regist(slot, mapid, addr, size)) {
            ret += 2;
        }
    }
    return ret;
}

/**
 * Puts the global camera data `addr` (`size` bytes, map `mapid`; NULL to remove) in slot 0. Returns
 * 0 for no change, +1 when old data was removed, +2 when new data was registered.
 */
int loadBgCAM_ReplaceG(int mapid, void *addr, int size) {
    int ret;

    ret = _loadBgCAM_Replace(0, mapid, addr, size);
    switch (ret) {
    case 1:
        verbose(1, "- cam(G)\n");
        break;
    case 2:
        verbose(1, " +cam(G):0x%08x(@0x%08x+0x%08x)\n", mapid, addr, size);
        break;
    case 3:
        verbose(1, "-+cam(G):0x%08x(@0x%08x+0x%08x)\n", mapid, addr, size);
        break;
    }
    return ret;
}

/**
 * Puts the camera data `addr` (`size` bytes, map `mapid`; NULL to remove) in block slot `slot`.
 * Returns 0 for no change, +1 when old data was removed, +2 when new data was registered.
 */
int loadBgCAM_Replace(int slot, int mapid, void *addr, int size) {
    int ret;

    ret = _loadBgCAM_Replace(slot + 1, mapid, addr, size);
    switch (ret) {
    case 1:
        verbose(1, "- cam(%d)\n", slot);
        break;
    case 2:
        verbose(1, " +cam(%d):0x%08x(@0x%08x+0x%08x)\n", slot, mapid, addr, size);
        break;
    case 3:
        verbose(1, "-+cam(%d):0x%08x(@0x%08x+0x%08x)\n", slot, mapid, addr, size);
        break;
    }
    return ret;
}

/** Returns a NULL-terminated list of the loaded camera data. */
void **loadBgCAM_GetLoadedDataAddrList(void) {
    int i;
    int j;
    void *addr;

    j = 0;
    for (i = 0; i < LBM_CAM_SLOTS; i++) {
        addr = lbCAM_Ctrl.addr[i];
        if (addr) {
            lbCAM_Ctrl.list[j++] = addr;
        }
    }
    lbCAM_Ctrl.list[j] = NULL;
    return lbCAM_Ctrl.list;
}

/** Gives the view camera the list of loaded camera road data. */
void loadBgCAM_vcReset(void) {
    void **list;

    list = loadBgCAM_GetLoadedDataAddrList();
    vcWork.vc_road_ary_list = (struct _VC_ROAD_DATA **)list;
}
