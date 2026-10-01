/*
 * dma2tscmd.c: texture slot commands queued on the DMA channel 2 (GIF) server thread: each
 * texture is sent to its slot only when the slot holds another texture.
 *
 * Matching: the command functions are queued through d2sPutCmdN(), called through a
 * cast function pointer (the original code gets `jalr`, not `jal`, for these calls).
 */

#include "sh2.h"

/** Server side of d2tscBeginToUseSlot(): takes texture slot `slot` (waits until it is free). */
int d2tscCmdBeginToUseSlot(int slot) {
    return texSlotBeginToUse(slot);
}

/** Queues taking texture slot `slot` on the channel 2 server. */
int d2tscBeginToUseSlot(int slot) {
    return ((int (*)(int (*)(int), int))d2sPutCmd1)(d2tscCmdBeginToUseSlot, slot);
}

/**
 * Server side of d2tscSend(): sends packet `tag` unless slot `slot` already holds texture `texid`.
 * Returns 1, or the send's result.
 */
int d2tscCmdSend(int slot, int texid, void *tag) {
    int ret;
    int cur_texid;

    ret = 1;
    cur_texid = texSlotGetCurTexID(slot);
    if (texid != cur_texid) {
        texSlotSetCurTexID(slot, texid);
        ret = d2cCmdSend(tag);
    }
    return ret;
}

/** Queues the send of texture `texid` (packet `tag`) to slot `slot` on the channel 2 server. */
int d2tscSend(int slot, int texid, void *tag) {
    return ((int (*)(int (*)(int, int, void *), int, int, void *))d2sPutCmd3)(d2tscCmdSend, slot, texid, tag);
}

/** Server side of d2tscClearSlots(): forgets the texture of every slot. */
int d2tscCmdClearSlots(void) {
    return texSlotClearAll();
}

/** Queues forgetting the texture of every slot on the channel 2 server. */
int d2tscClearSlots(void) {
    return ((int (*)(int (*)(void)))d2sPutCmd0)(d2tscCmdClearSlots);
}
