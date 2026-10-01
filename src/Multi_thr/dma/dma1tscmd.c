/*
 * dma1tscmd.c: texture slot commands queued on the DMA channel 1 (VIF1) server thread.
 *
 * Matching: the command functions are queued through d1sPutCmdN(), called through a
 * cast function pointer (the original code gets `jalr`, not `jal`, for these calls).
 */

#include "sh2.h"

/** Server side of d1tscSync(): waits for DMA channel 2 command `d2cid`. */
int d1tscCmdSync(int d2cid) {
    return d2sSync(0, d2cid);
}

/** Queues a wait for DMA channel 2 command `d2cid` on the channel 1 server. */
int d1tscSync(int d2cid) {
    return ((int (*)(int (*)(int), int))d1sPutCmd1)(d1tscCmdSync, d2cid);
}

/** Server side of d1tscFinishToUseSlot(): releases texture slot `slot`. */
int d1tscCmdFinishToUseSlot(int slot) {
    return texSlotFinishToUse(slot);
}

/** Queues the release of texture slot `slot` on the channel 1 server. */
int d1tscFinishToUseSlot(int slot) {
    return ((int (*)(int (*)(int), int))d1sPutCmd1)(d1tscCmdFinishToUseSlot, slot);
}
