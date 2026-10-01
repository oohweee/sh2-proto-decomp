/*
 * dma2cmd.c: the commands of the DMA channel 2 (GIF) server thread: every send is queued on
 * the server (dma2serv.c) and flushes the data cache first if needed.
 *
 * Matching: the command functions are queued through d2sPutCmdN(), called through a
 * cast function pointer (the original code gets `jalr`, not `jal`, for these calls).
 */

#include "sh2.h"
#include "sdk/eekernel.h"

static int last_regist_cid;
static int last_flush_cid; /* Matching: declared second (MWCC emits .bss statics in reverse order). */

/** Records `cid` (if valid) as the ID of the last command queued. */
void d2cSubCmdUpdataLastRegistCid(int cid) {
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
int d2cSubSubCmdCheckFlushCache(void) {
    int ret;
    int cur_cid;

    ret = 0;
    cur_cid = d2sGetCurCmdId();
    utilExclLockOtherThread();
    if (d2sCmpCID(last_flush_cid, cur_cid) < 0) {
        last_flush_cid = last_regist_cid;
        ret = 1;
    }
    utilExclUnlockOtherThread();
    return ret;
}

/** Flushes the data cache if d2cSubSubCmdCheckFlushCache() says so. */
void d2cSubCmdFlushCache(void) {
    if (d2cSubSubCmdCheckFlushCache()) {
        FlushCache(0);
    }
}

static int d2cCmdInit(void) {
    return shDmaGifInit();
}

/** Queues the channel's initialization on the server thread. Returns the command ID. */
int d2cInit(void) {
    int ret;

    ret = ((int (*)(int (*)(void)))d2sPutCmd0)(d2cCmdInit);
    d2cSubCmdUpdataLastRegistCid(ret);
    return ret;
}

/**
 * Server side of d2cSend(): flushes the data cache if needed and sends packet `tag`
 * (shDmaGifSend()).
 */
int d2cCmdSend(void *tag) {
    d2cSubCmdFlushCache();
    return shDmaGifSend(tag);
}
