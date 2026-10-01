/*
 * dma1cmd.c: the commands of the DMA channel 1 (VIF1) server thread: every send is queued on
 * the server (dma1serv.c) and flushes the data cache first if needed.
 *
 * Matching: the command functions are queued through d1sPutCmdN(), called through a
 * cast function pointer (the original code gets `jalr`, not `jal`, for these calls).
 */

#include "sh2.h"
#include "sdk/eekernel.h"

static int last_regist_cid;
static int last_flush_cid; /* Matching: declared second (MWCC emits .bss statics in reverse order). */

/** Records `cid` (if valid) as the ID of the last command queued. */
void d1cSubCmdUpdataLastRegistCid(int cid) {
    utilExclLockOtherThread();
    if (cid >= 0) {
        last_regist_cid = cid;
    }
    utilExclUnlockOtherThread();
}

/**
 * Returns 1 (once) when commands were queued since the last data cache flush the server thread has
 * passed, 0 otherwise.
 */
int d1cSubSubCmdCheckFlushCache(void) {
    int ret;
    int cur_cid;

    ret = 0;
    cur_cid = d1sGetCurCmdId();
    utilExclLockOtherThread();
    if (d1sCmpCID(last_flush_cid, cur_cid) < 0) {
        last_flush_cid = last_regist_cid;
        ret = 1;
    }
    utilExclUnlockOtherThread();
    return ret;
}

/** Flushes the data cache if d1cSubSubCmdCheckFlushCache() says so. */
void d1cSubCmdFlushCache(void) {
    if (d1cSubSubCmdCheckFlushCache()) {
        FlushCache(0);
    }
}

static int d1cCmdInit(void) {
    return shDmaVif1Init();
}

/** Queues the channel's initialization on the server thread. Returns the command ID. */
int d1cInit(void) {
    int ret;

    ret = ((int (*)(int (*)(void)))d1sPutCmd0)(d1cCmdInit);
    d1cSubCmdUpdataLastRegistCid(ret);
    return ret;
}

/**
 * Server side of d1cSend(): flushes the data cache if needed and sends packet `tag`
 * (shDmaVif1Send()).
 */
int d1cCmdSend(void *tag) {
    d1cSubCmdFlushCache();
    return shDmaVif1Send(tag);
}

/** Queues the DMA send of packet `tag` on the server thread. Returns the command ID. */
int d1cSend(void *tag) {
    int ret;

    ret = ((int (*)(int (*)(void *), void *))d1sPutCmd1)(d1cCmdSend, tag);
    d1cSubCmdUpdataLastRegistCid(ret);
    return ret;
}
