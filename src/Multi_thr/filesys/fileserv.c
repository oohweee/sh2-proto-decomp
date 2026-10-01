/*
 * fileserv.c: the file server thread's client side. Every request is queued on
 * the command server (cmdserv) and executed by the file thread.
 */

#include "sh2.h"

/* Matching: CmdQueuePutN are called through these casts (the original gets `jalr`, and
 * passes the queued functions plain 32-bit arguments). */
typedef int (*CmdPut0Func)(u_long128 *work, void *func);
typedef int (*CmdPut1Func)(u_long128 *work, void *func, int a0);
typedef int (*CmdPut2Func)(u_long128 *work, void *func, int a0, int a1);
typedef int (*CmdPut3Func)(u_long128 *work, void *func, int a0, int a1, int a2);
typedef int (*CmdPut4Func)(u_long128 *work, void *func, int a0, int a1, int a2, int a3);

static void *fsCmdWork;

/**
 * Starts the file server thread (priority `th_prio`, stack `stack` of `stackSize` bytes) with the
 * command queue `queue` of `queueSize` bytes. Does nothing if it runs already. Returns non-zero on
 * success.
 */
int fsInit(int th_prio, void *stack, int stackSize, void *queue, int queueSize) {
    int ret = 0;

    if (fsCmdWork == NULL) {
        ret = CmdServInit(queue, queueSize, stack, stackSize, th_prio);
        if (ret) {
            fsCmdWork = queue;
        }
    }
    return ret;
}

/**
 * Waits for (`mode` 0) or checks command `fid`. Before waiting it syncs to a V-blank end (a second
 * one when the first came within 10 timer ticks).
 */
int fsSync(int mode, int fid) {
    int ret;

    if (mode == 0 && shSyncVEnd(1) < 10) {
        shSyncVEnd(0);
    }
    ret = CmdQueueSync(fsCmdWork, mode, fid);
    return ret;
}

/** Stores the file server's queue status in `stat`. */
int fsGetStat(struct CmdServStat *stat) {
    return CmdServGetStat(fsCmdWork, stat);
}

/** Returns the state of the disc check (fsCmdGetTrayStat()). */
int fsGetTrayStat(void) {
    return fsCmdGetTrayStat();
}

/** Queues shSifInit() on the file server. Returns the command ID. */
int fcSifInit(void) {
    return ((CmdPut0Func)CmdQueuePut0)(fsCmdWork, shSifInit);
}

/** Queues shCdInitW() on the file server. Returns the command ID. */
int fcCdInitW(int cb_prio, void *stack, int stackSize) {
    return ((CmdPut3Func)CmdQueuePut3)(fsCmdWork, shCdInitW, cb_prio, (int)stack, stackSize);
}

/** Queues loading IOP module `module` on the file server. Returns the command ID. */
int fcIopLoadMod(char *module) {
    return ((CmdPut1Func)CmdQueuePut1)(fsCmdWork, shIopLoadMod, (int)module);
}

/** Queues fsCmdDiskSelectC() (files from the CD only). Returns the command ID. */
int fcDiskSelectC(void) {
    return ((CmdPut0Func)CmdQueuePut0)(fsCmdWork, fsCmdDiskSelectC);
}

/** Queues fsCmdDiskSelectCH() (CD first, then HD). Returns the command ID. */
int fcDiskSelectCH(void) {
    return ((CmdPut0Func)CmdQueuePut0)(fsCmdWork, fsCmdDiskSelectCH);
}

/** Queues fsCmdDiskSelectHC() (HD first, then CD). Returns the command ID. */
int fcDiskSelectHC(void) {
    return ((CmdPut0Func)CmdQueuePut0)(fsCmdWork, fsCmdDiskSelectHC);
}

/** Queues fsCmdHdInit(`mode`). Returns the command ID. */
int fcHdInit(int mode) {
    return ((CmdPut1Func)CmdQueuePut1)(fsCmdWork, fsCmdHdInit, mode);
}

/** Queues the disc selection for `mode` (default: CD only). Returns the command ID. */
int fcDiskSelect(int mode) {
    switch (mode) {
    default:
        return fcDiskSelectC();
    case 1:
        return fcDiskSelectCH();
    case 2:
        return fcDiskSelectHC();
    }
}

/** Queues fsCmdExecDevSelect(`mode`). Returns the command ID. */
int fcExecDevSelect(int mode) {
    return ((CmdPut1Func)CmdQueuePut1)(fsCmdWork, fsCmdExecDevSelect, mode);
}

/** Queues fsCmdSetParamForCheckDisk() with the same arguments. Returns the command ID. */
int fcSetParamForCheckDisk(int media_permission, union fsFile **fplist, void **buflist, int (*check_func)(union fsFile **, void **)) {
    return ((CmdPut4Func)CmdQueuePut4)(fsCmdWork, fsCmdSetParamForCheckDisk, media_permission, (int)fplist, (int)buflist, (int)check_func);
}

/** Queues the disc check fsCmdCdCheckDisk(`force_check`). Returns the command ID. */
int fcCdCheckDisk(int force_check) {
    return ((CmdPut1Func)CmdQueuePut1)(fsCmdWork, fsCmdCdCheckDisk, force_check);
}

/* Reads are DMA transfers: the buffer has to be 64-byte aligned. */
static void checkReadAlign(void *buffer) {
    if ((int)buffer & 0x3F) {
        printf("fileserv.c:493> buffer alignment error! 0x%08x\n", buffer);
        while (1) {}
    }
}

/**
 * Queues a read of file `fp` into `buf` (64-byte aligned; halts otherwise). Returns the command ID.
 */
int fcRead(union fsFile *fp, void *buf) {
    checkReadAlign(buf);
    return ((CmdPut2Func)CmdQueuePut2)(fsCmdWork, fsCmdRead, (int)fp, (int)buf);
}

/** Queues a read of `size` bytes at `offset` of file `fp` into `buf`. Returns the command ID. */
int fcReadPart(union fsFile *fp, void *buf, int offset, int size) {
    return ((CmdPut4Func)CmdQueuePut4)(fsCmdWork, fsCmdReadPart, (int)fp, (int)buf, offset, size);
}

/** Queues fsCmdFixFile(`fp`). Returns the command ID. */
int fcFixFile(union fsFile *fp) {
    return ((CmdPut1Func)CmdQueuePut1)(fsCmdWork, fsCmdFixFile, (int)fp);
}
