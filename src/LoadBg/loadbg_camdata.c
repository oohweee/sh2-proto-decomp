/*
 * loadbg_camdata.c: loads a background's camera data (.cam) into one of the
 * VC_DATA_MAX camera buffers and registers it with the camera system.
 *
 * Matching: the assert message bakes in "loadbg_camdata.c:30", so the
 * assert has to stay on line 30 of this file.
 */

#include "sh2.h"

#define VC_DATA_MAX 1

/* Camera data buffers, 64-byte aligned for DMA. */
char CAMbuf[VC_DATA_MAX][2048] __attribute__((aligned(64)));
int CAMsize[VC_DATA_MAX];

static int BgCam_LoadData(void *loadbuf, union fsFileIndex *file) {
    return _loadBgCommon_LoadData(loadbuf, file, 0x800);
}

/**
 * Loads `file` into camera slot `slot` and hands it to loadBgCAM_ReplaceG.
 * The previous camera data is released first (mapid 0, no buffer).
 */
void loadBgCAM_LoadData(int slot, union fsFileIndex *file, int mapid) {
    /* The slot index must be valid. */



    assert(0<=slot && slot<VC_DATA_MAX);
    loadBgCAM_ReplaceG(0, NULL, 0);
    CAMsize[slot] = BgCam_LoadData(CAMbuf[slot], file);
    loadBgCAM_ReplaceG(mapid, CAMbuf[slot], CAMsize[slot]);
}
