/*
 * loadinit.c: the load-and-init server: a command server thread (cmd_serv.c)
 * that runs loading and initialization commands in the background.
 */

#include "sh2.h"

static void *lisCmdWorkP;

/**
 * Starts the server thread (priority `th_prio`, stack `stack` of `stackSize` bytes) with the
 * command queue `queue` of `queueSize` bytes. Does nothing if it runs already. Returns non-zero on
 * success.
 */
int lisInit(int th_prio, void *stack, int stackSize, void *queue, int queueSize) {
    int ret = 0;

    if (lisCmdWorkP == NULL) {
        ret = CmdServInit(queue, queueSize, stack, stackSize, th_prio);
        if (ret) {
            lisCmdWorkP = queue;
        }
    }
    return ret;
}

/** Waits for (`mode` 0) or checks command `cid`. Returns 0 when the server is not running. */
int lisSync(int mode, int cid) {
    int ret = 0;

    if (lisCmdWorkP != NULL) {
        ret = CmdQueueSync(lisCmdWorkP, mode, cid);
    }
    return ret;
}

/** Stores the server's queue status in `stat`. Returns 0 when the server is not running. */
int lisGetStat(struct CmdServStat *stat) {
    int ret;

    ret = (lisCmdWorkP != NULL) ? CmdServGetStat(lisCmdWorkP, stat) : 0;
    return ret;
}

/** Queues `f0` with no arguments. Returns the command ID, or -1. */
int lisPutCmd0(int (*f0)(void)) {
    int ret = -1;

    if (lisCmdWorkP != NULL) {
        ret = CmdQueuePut0(lisCmdWorkP, f0);
    }
    return ret;
}
