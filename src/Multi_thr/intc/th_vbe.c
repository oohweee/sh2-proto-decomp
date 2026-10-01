/*
 * th_vbe.c: the V-blank end thread: once per frame it runs the sound driver's
 * vsync, and every 300 frames polls the file server.
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "lib/sh_kernel.h"

static int finish_sid = -1;
static int vbe_tid = -1;

static void ThreadVbe(void *arg) {
    int wait_sid;
    int count;

    if ((int)arg != -1) {
        wait_sid = WaitSema((int)arg);
        SignalSema(wait_sid);
    }
    SignalSemaMax(finish_sid);

    count = 0;
    while (1) {
        count++;
        shSyncVEnd(0);
        shSdVSync();
        if (count % 300 == 0) {
            fsSync(1, -1);
        }
    }
}

/**
 * Starts the V-blank end thread (stack `stack` of `stackSize` bytes, priority `prio`, `option`).
 * The thread first waits on semaphore `wait_sid` unless it is -1. Returns the semaphore the thread
 * signals once it runs, or -1.
 */
int ThreadVbeStart(void *stack, int stackSize, int prio, int option, int wait_sid) {
    void *arg;

    arg = (void *)wait_sid;
    if (finish_sid == -1) {
        finish_sid = CreateSema2(0, 0x100, NULL);
    }
    if (finish_sid != -1 && vbe_tid == -1) {
        vbe_tid = CreateThread2(ThreadVbe, stack, stackSize, prio, option);
        if (vbe_tid != -1) {
            StartThread(vbe_tid, arg);
        }
    }
    if (vbe_tid != -1) {
        return finish_sid;
    }
    return -1;
}
