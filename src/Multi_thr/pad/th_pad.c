/*
 * th_pad.c: the pad thread: initializes the pads, then once per frame reads
 * them and sends the DualShock 2 vibration levels.
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "sdk/libpad.h"
#include "lib/libShPad.h"
#include "lib/sh_kernel.h"

static int finish_sid = -1;
static int pad_tid = -1;

/*
 * The verbose() messages carry "<file>:<line>>" prefixes from the original source.
 *
 * Pad thread: initializes the pad library, signals finish_sid, then polls the
 * pads and sends the actuator (vibration) levels once per vsync.
 */
static void ThreadPad(void *arg) {
    int wait_sid;
    unsigned int pow0;
    unsigned int pow1;

    wait_sid = (int)arg;
    if (wait_sid != -1) {
        verbose(2, "th_pad.c:41> wait by semaphore(mtapman.irx).\n");
        SignalSema(WaitSema(wait_sid));
    }

    /* init pad */
    verbose(2, "th_pad.c:46> going to init pad.\n");
    libShPadEnable00();
    libShPadEnable10();
    libShPadInit();
    if (dbFlag(0x10)) {
        scePadSetWarningLevel(0);
    }
    libShPadStart(0, 0);
    libShPadStart(1, 0);
    libShPadSetMode(0, 0, 1, 1, 1, 1);
    verbose(2, "th_pad.c:102> finished to init pad.\n");
    SignalSemaMax(finish_sid);

    while (1) {
        shSyncVEnd(0);
        libShPadTrans();
        if (dbFlag(0x8)) {
            continue;
        }
        utilExclLockOtherThread();
        DSS_Wrapper_DualShock2_Sequencer();
        pow0 = DSS_Wrapper_DualShock2_Send_ActuaterLV_Get(0, 0);
        pow1 = DSS_Wrapper_DualShock2_Send_ActuaterLV_Get(0, 1);
        DSS_Wrapper_DualShock2_Send_ActuaterLV_Claer(0, 0);
        DSS_Wrapper_DualShock2_Send_ActuaterLV_Claer(0, 1);
        utilExclUnlockOtherThread();
        libShPadSend(0, 0, pow0, pow1);
    }
}

/**
 * Starts the pad thread (stack `stack` of `stackSize` bytes, priority `prio`, `option`). The thread
 * first waits on semaphore `wait_sid` unless it is -1. Returns the semaphore the thread signals
 * once the pads are initialized, or -1.
 */
int ThreadPadStart(void *stack, int stackSize, int prio, int option, int wait_sid) {
    void *arg;

    arg = (void *)wait_sid;
    if (finish_sid == -1) {
        finish_sid = CreateSema2(0, 256, 0);
    }
    if (finish_sid != -1 && pad_tid == -1) {
        pad_tid = CreateThread2(ThreadPad, stack, stackSize, prio, option);
        if (pad_tid != -1) {
            StartThread(pad_tid, arg);
        }
    }
    if (pad_tid != -1) {
        return finish_sid;
    }
    return -1;
}
