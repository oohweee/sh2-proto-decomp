/*
 * utilexcl.c: one global lock that keeps the other game threads out of a
 * short critical section.
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "lib/sh_kernel.h"

static int excl_sid = -1;

/** Creates the lock (once). Returns 0 if it can't be created. */
int utilExclInit(void) {
    int sid;

    if (excl_sid == -1) {
        sid = CreateSema2(0, 1, 0);
        if (sid == -1) {
            return 0;
        }
        excl_sid = sid;
        SignalSema(sid);
    }
    return 1;
}

/** Takes the lock. Returns 0 before utilExclInit(). */
int utilExclLockOtherThread(void) {
    if (excl_sid == -1) {
        return 0;
    }
    WaitSema(excl_sid);
    return 1;
}

/** Releases the lock. Returns 0 before utilExclInit(). */
int utilExclUnlockOtherThread(void) {
    if (excl_sid == -1) {
        return 0;
    }
    SignalSemaLimit(excl_sid);
    return 1;
}
