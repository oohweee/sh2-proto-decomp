/*
 * texslot.c: the texture slots. Each slot is guarded by a semaphore and
 * remembers the ID of the texture it holds.
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "lib/sh_kernel.h"

#define TEX_SLOT_NUM 5

static struct texSlotCtrlT texSlotCtrl[TEX_SLOT_NUM] = {0};

/** Creates the semaphores of the slots not yet set up and forgets their textures. Returns 1. */
int texSlotInit(void) {
    int i;
    int sid;

    for (i = 0; i < TEX_SLOT_NUM; i++) {
        if (!texSlotCtrl[i].init) {
            sid = CreateSema2(1, 1, "for exclusively using one of tex slots");
            if (sid != -1) {
                texSlotCtrl[i].sid = sid;
                texSlotCtrl[i].init = 1;
                texSlotCtrl[i].cur_texid = -1;
            }
        }
    }
    return 1;
}

/** Takes slot `slot`, waiting until it is free. Returns 0 for an invalid slot. */
int texSlotBeginToUse(int slot) {
    if (0 <= slot && slot < TEX_SLOT_NUM) {
        WaitSema(texSlotCtrl[slot].sid);
        return 1;
    }
    return 0;
}

/** Releases slot `slot`. Returns 0 for an invalid slot. */
int texSlotFinishToUse(int slot) {
    if (0 <= slot && slot < TEX_SLOT_NUM) {
        SignalSema(texSlotCtrl[slot].sid);
        return 1;
    }
    return 0;
}

/** Returns the ID of the texture in slot `slot`, or -1. */
int texSlotGetCurTexID(int slot) {
    if (0 <= slot && slot < TEX_SLOT_NUM) {
        return texSlotCtrl[slot].cur_texid;
    }
    return -1;
}

/** Records texture `texid` in slot `slot`. Returns `texid`, or -1 for an invalid slot. */
int texSlotSetCurTexID(int slot, int texid) {
    if (0 <= slot && slot < TEX_SLOT_NUM) {
        texSlotCtrl[slot].cur_texid = texid;
        return texid;
    }
    return -1;
}

/** Forgets the texture of every slot. Returns 1. */
int texSlotClearAll(void) {
    int i;

    for (i = 0; i < TEX_SLOT_NUM; i++) {
        texSlotSetCurTexID(i, -1);
    }
    return 1;
}
