/*
 * syncv.c: V-blank synchronization.
 *
 * Interrupt handlers on the V-blank start and end INTC causes signal a
 * semaphore each; shSyncVStart()/shSyncVEnd() block on them.
 *
 * The *Init functions may be called again; they only fill in what's missing.
 *
 * Matching: verbose() messages bake "syncv.c:<line>" into their format
 * strings, so they must stay on lines 60, 62, 75, 82, 85, 92 and 116.
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "lib/sh_kernel.h"

/* Matching: #line keeps the original numbering (register defines moved to eeregs.h, declarations to headers). */
#line 31

#define POS __FILE__ ":" SH_STRINGIFY(__LINE__) "> "

int shSyncVStart_sid = -1;
int shSyncVStart_hid = -1;
int shSyncVEnd_sid = -1;
int shSyncVEnd_hid = -1;

/* INTC handler: `arg` is the semaphore to release. */
static int shSyncVHandler2(int, void *arg) {
    int sid;

    sid = (int)arg;
    iReleaseSema(sid);
    ExitHandler();
    return 1;
}

/*
 * Creates the semaphore *sid_p and installs the handler *hid_p on
 * `intc_cause`, each only if not done yet. Returns 1 on success, 0 on failure.
 *
 * Note: when *sid_p already exists but *hid_p doesn't, the handler is
 * installed with an uninitialized `sid` as its argument.
 */
static int shSyncVInitSub(int *sid_p, int *hid_p, int intc_cause) {
    int sid;
    int hid;

    verbose(2, POS "start to init syncv\n");
    if (*sid_p == -1) {
        verbose(2, POS "start to create semaphore\n");
        sid = CreateSema2(0, 0x100, NULL);
        /*
         * The semaphore counts V-blanks that nobody has waited for yet:
         * it starts at 0 and saturates at 256, so a thread that falls
         * behind catches up instead of the handler failing.
         *
         * CreateSema2() takes the counts directly instead of a
         * SemaParam; the name is unused.
         *
         */

        if (sid != -1) {
            verbose(2, POS "init semaphore\n");
            *sid_p = sid;
        } else {
            return 0;
        }
    }
    if (*hid_p == -1) {
        verbose(2, POS "start to add handler\n");
        hid = AddIntcHandler2(intc_cause, shSyncVHandler2, 0, (void *)sid);
        if (hid != -1) {
            verbose(2, POS "init handler\n");
            *hid_p = hid;
            EnableIntc(intc_cause);
        } else {
            return 0;
        }
    }
    verbose(2, POS "success to init syncv\n");
    return 1;
}
/** Sets up what is missing of the V-blank start semaphore and handler; non-zero when ready. */
int shSyncVStartInit(void) {
    return shSyncVInitSub(&shSyncVStart_sid, &shSyncVStart_hid, INTC_VBLANK_S);
}
/** Sets up what is missing of the V-blank end semaphore and handler; non-zero when ready. */
int shSyncVEndInit(void) {
    return shSyncVInitSub(&shSyncVEnd_sid, &shSyncVEnd_hid, INTC_VBLANK_E);
}

/*
 * Waits for the next signal on `sid`. With `mode` set, returns how many
 * timer 3 ticks the wait took (mod 0x10000); otherwise returns 0.
 * Returns -1 if `sid` was never created (the matching shSyncV*Init() hasn't
 * run).
 */
static int shSyncVSub(int mode, int sid) {
    int count;
    unsigned short cnt0;
    unsigned short cnt1;

    if (sid == -1) {
        verbose(2, POS "no init syncv\n");
        return -1;
    }
    if (mode) {
        cnt0 = *T3_COUNT;
        WaitSema(sid);
        cnt1 = *T3_COUNT;
        count = (unsigned short)(cnt1 - cnt0);
        return count;
    }
    WaitSema(sid);
    return 0;
}

/**
 * Waits for the next V-blank start. With `mode` set, returns the timer 3 ticks waited; -1 before
 * shSyncVStartInit().
 */
int shSyncVStart(int mode) {
    return shSyncVSub(mode, shSyncVStart_sid);
}

/**
 * Waits for the next V-blank end. With `mode` set, returns the timer 3 ticks waited; -1 before
 * shSyncVEndInit().
 */
int shSyncVEnd(int mode) {
    return shSyncVSub(mode, shSyncVEnd_sid);
}
