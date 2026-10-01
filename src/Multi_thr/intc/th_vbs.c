/*
 * th_vbs.c: the V-blank start thread: counts frames and runs the debug flow
 * check (dbflow.c) once per frame.
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "lib/sh_kernel.h"

/*
 * V-blank start counts: since ThreadVbs started, and in total.
 * Matching: declared in this order because MWCC emits .bss statics in reverse order.
 */
static unsigned long vcount_total;
static unsigned long vcount;

static int finish_sid = -1;
static int vbs_tid = -1;

static void shMtIncVStartCount(void) {
    vcount++;
    vcount_total++;
}

static void ThreadVbs(void *arg) {
    int wait_sid;
    int ret;

    if ((int)arg != -1) {
        wait_sid = WaitSema((int)arg);
        SignalSema(wait_sid);
    }
    SignalSemaMax(finish_sid);

    vcount = 0;
    while (1) {
        shSyncVStart(0);
        shMtIncVStartCount();
        ret = dbFlowCheck();
        dbFlowCheckSleepTime(ret);
        Vcallback_test(GetThreadId());
    }
}

/**
 * Starts the V-blank start thread (stack `stack` of `stackSize` bytes, priority `prio`, `option`).
 * The thread first waits on semaphore `wait_sid` unless it is -1. Returns the semaphore the thread
 * signals once it runs, or -1.
 */
int ThreadVbsStart(void *stack, int stackSize, int prio, int option, int wait_sid) {
    void *arg;

    arg = (void *)wait_sid;
    if (finish_sid == -1) {
        finish_sid = CreateSema2(0, 0x100, NULL);
    }
    if (finish_sid != -1 && vbs_tid == -1) {
        vbs_tid = CreateThread2(ThreadVbs, stack, stackSize, prio, option);
        if (vbs_tid != -1) {
            StartThread(vbs_tid, arg);
        }
    }
    if (vbs_tid != -1) {
        return finish_sid;
    }
    return -1;
}
