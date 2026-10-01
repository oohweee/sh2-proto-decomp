/*
 * cmd_serv.c: the command server. A client queues a function pointer and up
 * to four 128-bit arguments in a ring buffer of quadwords; a dedicated thread
 * pops the commands and runs them in order. Every command gets a 31-bit id so
 * clients can wait for it with CmdQueueSync().
 *
 * `work` starts with a struct CmdServWork, the queue follows it.
 * serv->debug[] records where each thread is blocked (for debugging hangs).
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "lib/sh_kernel.h"


typedef int (*CmdFunc1)(u_long128);
typedef int (*CmdFunc2)(u_long128, u_long128);
typedef int (*CmdFunc3)(u_long128, u_long128, u_long128);
typedef int (*CmdFunc4)(u_long128, u_long128, u_long128, u_long128);
typedef int (*CmdFunc5)(u_long128, u_long128, u_long128, u_long128, u_long128);
typedef int (*CmdFunc6)(u_long128, u_long128, u_long128, u_long128, u_long128, u_long128);

#define QWRAP(i) while ((i) >= qsize) (i) -= qsize
#define GETARG(a) a = queue[idx++]; QWRAP(idx)
/* Matching: each CmdQueuePut*() queues inside a do { } while (0): the original's line table has a
   nop-only statement where it closes, and its loop weight decides CmdQueuePut3/4's registers. */

/* Stand-in for a NULL command: dumps its arguments. */
static int CmdServFuncNull(u_long128 a0, u_long128 a1, u_long128 a2, u_long128 a3, u_long128 a4, u_long128 a5) {
    union {
        u_long128 q;
        unsigned long w[4];
    } t;
    int n;

    n = 0;
    printf("CmdServ error: tid(%d) func==0\n", GetThreadId());
    t.q = a0;
    printf("                       a%d==0x%08x_%08x_%08x_%08x\n", n, t.w[3], t.w[2], t.w[1], t.w[0]);
    n++;
    t.q = a1;
    printf("                       a%d==0x%08x_%08x_%08x_%08x\n", n, t.w[3], t.w[2], t.w[1], t.w[0]);
    n++;
    t.q = a2;
    printf("                       a%d==0x%08x_%08x_%08x_%08x\n", n, t.w[3], t.w[2], t.w[1], t.w[0]);
    n++;
    t.q = a3;
    printf("                       a%d==0x%08x_%08x_%08x_%08x\n", n, t.w[3], t.w[2], t.w[1], t.w[0]);
    n++;
    t.q = a4;
    printf("                       a%d==0x%08x_%08x_%08x_%08x\n", n, t.w[3], t.w[2], t.w[1], t.w[0]);
    n++;
    t.q = a5;
    printf("                       a%d==0x%08x_%08x_%08x_%08x\n", n, t.w[3], t.w[2], t.w[1], t.w[0]);
    n++;
    return 0;
}

static void CmdServThread(void *arg) {
    int qsize;
    int kick_sid;
    u_long128 *queue;
    u_long128 a0;
    u_long128 a1;
    u_long128 a2;
    u_long128 a3;
    u_long128 a4;
    u_long128 a5;
    int (*func)(void);
    int id;
    int qlen;
    int argc;
    int ret;
    int idx;
    struct CmdServWork *serv;
    int qhead;

    serv = arg;
    qsize = serv->qsize;
    queue = serv->queue;
    kick_sid = serv->kick_sid;
    serv->debug[1] = 1;
    SignalSema(serv->exclusive_sid);
    serv->debug[1] = 2;
    SignalSemaLimit(serv->sync_sid);
    while (1) {
        serv->debug[1] = 3;
        WaitSema(serv->exclusive_sid);
        serv->debug[1] = 4;
        qlen = serv->qlen;
        if (qlen <= 0) {
            serv->kick_req = 1;
            serv->debug[1] = 5;
            SignalSema(serv->exclusive_sid);
            serv->debug[1] = 6;
            WaitSema(kick_sid);
            serv->debug[1] = 7;
            continue;
        }
        idx = serv->qhead;
        func = *(int (**)(void))&queue[idx++];
        QWRAP(idx);
        argc = *(int *)&queue[idx++];
        QWRAP(idx);
        serv->debug[1] = 8;
        SignalSema(serv->exclusive_sid);
        serv->debug[1] = 9;
        WaitSema(serv->sync_sid);
        if (func == NULL) {
            func = (int (*)(void))CmdServFuncNull;
            printf("cmd_serv.c:208> CmdServ illegal func = %x\n", CmdServFuncNull);
        }
        if (argc < 0 || argc > 6) {
            printf("cmd_serv.c:211> CmdServ illegal argc = %d\n", argc);
        }
        if (argc <= 0) {
            ret = func();
        } else {
            GETARG(a0);
            if (argc <= 1) { ret = ((CmdFunc1)func)(a0); }
            else { GETARG(a1);
            if (argc <= 2) { ret = ((CmdFunc2)func)(a0, a1); }
            else { GETARG(a2);
            if (argc <= 3) { ret = ((CmdFunc3)func)(a0, a1, a2); }
            else { GETARG(a3);
            if (argc <= 4) { ret = ((CmdFunc4)func)(a0, a1, a2, a3); }
            else { GETARG(a4);
            if (argc <= 5) { ret = ((CmdFunc5)func)(a0, a1, a2, a3, a4); }
            else { GETARG(a5);
                ret = ((CmdFunc6)func)(a0, a1, a2, a3, a4, a5);
            }}}}}
        }

        serv->debug[1] = 10;
        WaitSema(serv->exclusive_sid);
        serv->debug[1] = 11;
        serv->last_ret = ret;
        serv->last_cmd = func;
        qlen = serv->qlen;
        qhead = serv->qhead;
        qlen -= argc + 2;
        if (qlen < 0) {
            printf("cmd_serv.c:240> CmdServ qlen(%d) error\n", qlen);
        }
        qhead += argc + 2;
        QWRAP(qhead);
        serv->qlen = qlen;
        serv->qhead = qhead;
        if (qlen == 0) {
            id = serv->next_id;
        } else {
            id = serv->oldest_id;
        }
        serv->debug[7] = serv->oldest_id;
        id = (id + 1) & 0x7FFFFFFF;
        serv->next_id = serv->oldest_id = id;
        SignalSemaLimit(serv->sync_sid);
        serv->debug[1] = 12;
        SignalSema(serv->exclusive_sid);
        serv->debug[1] = 13;
    }
}

/**
 * Sets up a command server in `work` (`workSize` bytes: the server's state, then the queue) and
 * starts its thread (stack `stack` of `stackSize` bytes, priority `prio`). Returns 0 when `work` is
 * too small or the thread can't be created.
 */
int CmdServInit(u_long128 *work, int workSize, void *stack, int stackSize, int prio) {
    struct CmdServWork *serv;
    int queue_size;
    int header_size;
    int excl_sid;
    int kick_sid;
    int sync_sid;
    int exec_tid;

    serv = (struct CmdServWork *)work;
    header_size = sizeof(struct CmdServWork) / sizeof(u_long128);
    queue_size = workSize / (int)sizeof(u_long128) - header_size;
    if (queue_size <= 0) {
        return 0;
    }
    serv->qsize = queue_size;
    serv->queue = work + header_size;
    kick_sid = -1;
    sync_sid = -1;
    excl_sid = CreateSema2(0, 1, NULL);
    if (excl_sid == -1) { goto error; }
    kick_sid = CreateSema2(0, 1, NULL);
    if (kick_sid == -1) { goto error; }
    sync_sid = CreateSema2(0, 256, NULL);
    if (sync_sid == -1) { goto error; }
    serv->exclusive_sid = excl_sid;
    serv->kick_sid = kick_sid;
    serv->sync_sid = sync_sid;
    exec_tid = CreateThread2(CmdServThread, stack, stackSize, prio, 0);
    if (exec_tid == -1) { goto error; }
    serv->cmdexec_tid = exec_tid;
    serv->kick_req = 0;
    serv->oldest_id = 0;
    serv->newest_id = -1;
    serv->next_id = 0;
    serv->qlen = 0;
    serv->qhead = 0;
    serv->qnext = 0;
    serv->last_ret = 0;
    serv->last_cmd = NULL;
    if (StartThread(exec_tid, work) == -1) { goto error; }
    return 1;

error:
    if (excl_sid != -1) { DeleteSema(excl_sid); }
    if (kick_sid != -1) { DeleteSema(kick_sid); }
    if (sync_sid != -1) { DeleteSema(sync_sid); }
    return 0;
}

/**
 * Queues `func` with no arguments on server `work` and wakes the server thread if it sleeps.
 * Returns the command ID, or -1 when the queue is full or `func` is NULL.
 */
int CmdQueuePut0(u_long128 *work, int (*func)(void)) {
    struct CmdServWork *serv;
    int qsize;
    u_long128 *queue;
    int id;
    int kick_req;
    int qlen;
    int qnext;

    serv = (struct CmdServWork *)work;
    qsize = serv->qsize;
    queue = serv->queue;
    WaitSema(serv->exclusive_sid);
    qlen = serv->qlen;
    kick_req = 0;
    id = -1;
    if (qlen + 2 <= qsize) {
        if (func != NULL) {
            do {
                qnext = serv->qnext;
                *(int (**)(void))&queue[qnext++] = func;
                QWRAP(qnext);
                *(int *)&queue[qnext++] = 0;
                QWRAP(qnext);
                id = serv->newest_id;
                id = (id + 1) & 0x7FFFFFFF;
                serv->newest_id = id;
                serv->qlen = qlen + 2;
                serv->qnext = qnext;
                kick_req = serv->kick_req;
                serv->kick_req = 0;
            } while (0);
        }
    }
    SignalSema(serv->exclusive_sid);
    if (kick_req) {
        SignalSema(serv->kick_sid);
    }
    return id;
}

/** CmdQueuePut0() with argument `a0`. */
int CmdQueuePut1(u_long128 *work, int (*func)(void), u_long128 a0) {
    struct CmdServWork *serv;
    int qsize;
    u_long128 *queue;
    int id;
    int kick_req;
    int qlen;
    int qnext;

    serv = (struct CmdServWork *)work;
    qsize = serv->qsize;
    queue = serv->queue;
    WaitSema(serv->exclusive_sid);
    qlen = serv->qlen;
    kick_req = 0;
    id = -1;
    if (qlen + 3 <= qsize) {
        if (func != NULL) {
            do {
                qnext = serv->qnext;
                *(int (**)(void))&queue[qnext++] = func;
                QWRAP(qnext);
                *(int *)&queue[qnext++] = 1;
                QWRAP(qnext);
                queue[qnext++] = a0;
                QWRAP(qnext);
                id = serv->newest_id;
                id = (id + 1) & 0x7FFFFFFF;
                serv->newest_id = id;
                serv->qlen = qlen + 3;
                serv->qnext = qnext;
                kick_req = serv->kick_req;
                serv->kick_req = 0;
            } while (0);
        }
    }
    SignalSema(serv->exclusive_sid);
    if (kick_req) {
        SignalSema(serv->kick_sid);
    }
    return id;
}

/** CmdQueuePut0() with arguments `a0`, `a1`. */
int CmdQueuePut2(u_long128 *work, int (*func)(void), u_long128 a0, u_long128 a1) {
    struct CmdServWork *serv;
    int qsize;
    u_long128 *queue;
    int id;
    int kick_req;
    int qlen;
    int qnext;

    serv = (struct CmdServWork *)work;
    qsize = serv->qsize;
    queue = serv->queue;
    WaitSema(serv->exclusive_sid);
    qlen = serv->qlen;
    kick_req = 0;
    id = -1;
    if (qlen + 4 <= qsize) {
        if (func != NULL) {
            do {
                qnext = serv->qnext;
                *(int (**)(void))&queue[qnext++] = func;
                QWRAP(qnext);
                *(int *)&queue[qnext++] = 2;
                QWRAP(qnext);
                queue[qnext++] = a0;
                QWRAP(qnext);
                queue[qnext++] = a1;
                QWRAP(qnext);
                id = serv->newest_id;
                id = (id + 1) & 0x7FFFFFFF;
                serv->newest_id = id;
                serv->qlen = qlen + 4;
                serv->qnext = qnext;
                kick_req = serv->kick_req;
                serv->kick_req = 0;
            } while (0);
        }
    }
    SignalSema(serv->exclusive_sid);
    if (kick_req) {
        SignalSema(serv->kick_sid);
    }
    return id;
}

/** CmdQueuePut0() with arguments `a0`-`a2`. */
int CmdQueuePut3(u_long128 *work, int (*func)(void), u_long128 a0, u_long128 a1, u_long128 a2) {
    struct CmdServWork *serv;
    int qsize;
    u_long128 *queue;
    int id;
    int kick_req;
    int qlen;
    int qnext;

    serv = (struct CmdServWork *)work;
    qsize = serv->qsize;
    queue = serv->queue;
    WaitSema(serv->exclusive_sid);
    qlen = serv->qlen;
    kick_req = 0;
    id = -1;
    if (qlen + 5 <= qsize) {
        if (func != NULL) {
            do {
                qnext = serv->qnext;
                *(int (**)(void))&queue[qnext++] = func;
                QWRAP(qnext);
                *(int *)&queue[qnext++] = 3;
                QWRAP(qnext);
                queue[qnext++] = a0;
                QWRAP(qnext);
                queue[qnext++] = a1;
                QWRAP(qnext);
                queue[qnext++] = a2;
                QWRAP(qnext);
                id = serv->newest_id;
                id = (id + 1) & 0x7FFFFFFF;
                serv->newest_id = id;
                serv->qlen = qlen + 5;
                serv->qnext = qnext;
                kick_req = serv->kick_req;
                serv->kick_req = 0;
            } while (0);
        }
    }
    SignalSema(serv->exclusive_sid);
    if (kick_req) {
        SignalSema(serv->kick_sid);
    }
    return id;
}

/** CmdQueuePut0() with arguments `a0`-`a3`. */
int CmdQueuePut4(u_long128 *work, int (*func)(void), u_long128 a0, u_long128 a1, u_long128 a2, u_long128 a3) {
    struct CmdServWork *serv;
    int qsize;
    u_long128 *queue;
    int id;
    int kick_req;
    int qlen;
    int qnext;

    serv = (struct CmdServWork *)work;
    qsize = serv->qsize;
    queue = serv->queue;
    WaitSema(serv->exclusive_sid);
    qlen = serv->qlen;
    kick_req = 0;
    id = -1;
    if (qlen + 6 <= qsize) {
        if (func != NULL) {
            do {
                qnext = serv->qnext;
                *(int (**)(void))&queue[qnext++] = func;
                QWRAP(qnext);
                *(int *)&queue[qnext++] = 4;
                QWRAP(qnext);
                queue[qnext++] = a0;
                QWRAP(qnext);
                queue[qnext++] = a1;
                QWRAP(qnext);
                queue[qnext++] = a2;
                QWRAP(qnext);
                queue[qnext++] = a3;
                QWRAP(qnext);
                id = serv->newest_id;
                id = (id + 1) & 0x7FFFFFFF;
                serv->newest_id = id;
                serv->qlen = qlen + 6;
                serv->qnext = qnext;
                kick_req = serv->kick_req;
                serv->kick_req = 0;
            } while (0);
        }
    }
    SignalSema(serv->exclusive_sid);
    if (kick_req) {
        SignalSema(serv->kick_sid);
    }
    return id;
}

/**
 * Checks command `id` (-1: the newest) of server `work`; with `mode` 0, waits until it has run.
 * Returns 1 when it has run, 0 for an ID not given out yet (or when called from the server thread),
 * and -1 (running) or -2 (queued) when `mode` is non-zero and it hasn't run.
 */
/* Matching: newer_than_oldest and older_than_newest are each computed in two steps (shift, then the
 * sign test), as in the original's line table (statements at 852/853 and 855/856). */
int CmdQueueSync(u_long128 *work, int mode, int id) {
    struct CmdServWork *serv;
    int ret;
    int excl_sid;
    int sync_sid;
    int curr_tid;
    int check_id;
    int newest_id;
    int oldest_id;
    int newer_than_oldest;
    int older_than_newest;

    serv = (struct CmdServWork *)work;
    curr_tid = GetThreadId();
    if (curr_tid == serv->cmdexec_tid) {
        return 0;
    }
    excl_sid = serv->exclusive_sid; /* Matching: dead; the original's DWARF has excl_sid */
    sync_sid = serv->sync_sid;
    serv->debug[3] = serv->debug[1];
    while (1) {
        serv->debug[0] = 1;
        WaitSema(serv->exclusive_sid);
        serv->debug[0] = 2;
        oldest_id = serv->oldest_id;
        newest_id = serv->newest_id;
        check_id = id;
        if (id == -1) {
            check_id = newest_id;
        }
        serv->debug[2] = check_id;
        serv->debug[8] = newest_id;
        serv->debug[9] = oldest_id;
        if (oldest_id == check_id) {
            ret = -1;
        } else {
            newer_than_oldest = (oldest_id - check_id) << 1;
            newer_than_oldest = newer_than_oldest < 0;
            older_than_newest = (check_id - newest_id) << 1;
            older_than_newest = older_than_newest <= 0;
            if (newer_than_oldest && older_than_newest) {
                ret = -2;
            } else if (!older_than_newest) {
                ret = 0;
            } else {
                ret = 1;
            }
        }
        serv->debug[4] = serv->oldest_id;
        serv->debug[5] = serv->debug[1];
        serv->debug[6] = ret;
        if (ret >= 0 || mode != 0) {
            serv->debug[0] = 3;
            SignalSema(serv->exclusive_sid);
            serv->debug[0] = 4;
            break;
        }
        serv->debug[0] = 5;
        SignalSema(serv->exclusive_sid);
        serv->debug[0] = 6;
        serv->debug[0] = 7;
        WaitSema(sync_sid);
        SignalSemaLimit(sync_sid);
        serv->debug[0] = 8;
    }
    return ret;
}

/**
 * Returns the ID of the command being run when called from the server thread of `work`, otherwise
 * -1.
 */
int CmdQueueGetCurrentCommandId(u_long128 *work) {
    struct CmdServWork *serv;

    serv = (struct CmdServWork *)work;
    if (serv->cmdexec_tid == GetThreadId()) {
        return serv->oldest_id;
    }
    return -1;
}

/**
 * Compares the 31-bit command IDs `cid0` and `cid1` with wrap-around: negative, zero or positive as
 * `cid0` is older, the same or newer.
 */
int CmdQueueCmpCmdId(int cid0, int cid1) {
    int cmp;

    cmp = (cid0 - cid1) << 1;
    return cmp >> 1;
}

/** Stores the status of server `work` (queue length and size, current and last IDs) in `stat`. */
int CmdServGetStat(u_long128 *work, struct CmdServStat *stat) {
    int ret;
    struct CmdServWork *serv;
    int qsize;
    int qlen;
    int id;
    int last_id;
    int clen;

    if (stat) {
        serv = (struct CmdServWork *)work;
        WaitSema(serv->exclusive_sid);
        qsize = serv->qsize;
        qlen = serv->qlen;
        id = serv->oldest_id;
        last_id = serv->newest_id;
        SignalSema(serv->exclusive_sid);
        clen = CmdQueueCmpCmdId(last_id, id) + 1;
        if (clen < 0) {
            clen = 0;
        }
        stat->qsize = qsize;
        stat->qlen = qlen;
        stat->id = id;
        stat->last_id = last_id + 1;
        stat->clen = clen;
        ret = 1;
    } else {
        ret = 0;
    }
    return ret;
}
