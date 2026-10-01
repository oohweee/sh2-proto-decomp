/*
 * dma2serv.c: the DMA channel 2 (GIF) server: a command server thread
 * (cmd_serv.c) that runs the channel's commands in order.
 */

#include "sh2.h"

static void *d2sCmdWorkP;

/**
 * Starts the server thread (priority `th_prio`, stack `stack` of `stackSize` bytes) with the
 * command queue `queue` of `queueSize` bytes. Does nothing if it runs already. Returns non-zero on
 * success.
 */
int d2sInit(int th_prio, void *stack, int stackSize, void *queue, int queueSize) {
    int ret = 0;

    if (d2sCmdWorkP == NULL) {
        ret = CmdServInit(queue, queueSize, stack, stackSize, th_prio);
        if (ret) {
            d2sCmdWorkP = queue;
        }
    }
    return ret;
}

/**
 * Waits for (`mode` 0) or checks command `cid` (CmdQueueSync()). Returns 0 when the server is not
 * running.
 */
int d2sSync(int mode, int cid) {
    int ret = 0;

    if (d2sCmdWorkP) {
        ret = CmdQueueSync(d2sCmdWorkP, mode, cid);
    }
    return ret;
}

/** Returns the ID of the command being run, or -1 when the server is not running. */
int d2sGetCurCmdId(void) {
    int ret = -1;

    if (d2sCmdWorkP) {
        ret = CmdQueueGetCurrentCommandId(d2sCmdWorkP);
    }
    return ret;
}

/** Compares command IDs `cid0` and `cid1` (CmdQueueCmpCmdId()). */
int d2sCmpCID(int cid0, int cid1) {
    return CmdQueueCmpCmdId(cid0, cid1);
}

/** Queues `f0` with no arguments. Returns the command ID, or -1. */
int d2sPutCmd0(int (*f0)(void)) {
    int ret = -1;

    if (d2sCmdWorkP) {
        ret = CmdQueuePut0(d2sCmdWorkP, f0);
    }
    return ret;
}

/** Queues `f1` with argument `a1`. Returns the command ID, or -1. */
int d2sPutCmd1(int (*f1)(void), u_long128 a1) {
    int ret = -1;

    if (d2sCmdWorkP) {
        ret = CmdQueuePut1(d2sCmdWorkP, f1, a1);
    }
    return ret;
}

/** Queues `f3` with arguments `a1`-`a3`. Returns the command ID, or -1. */
int d2sPutCmd3(int (*f3)(void), u_long128 a1, u_long128 a2, u_long128 a3) {
    int ret = -1;

    if (d2sCmdWorkP) {
        ret = CmdQueuePut3(d2sCmdWorkP, f3, a1, a2, a3);
    }
    return ret;
}
