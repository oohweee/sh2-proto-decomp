#include "sh2.h"
#include "sdk/eekernel.h"

/* pss_common.c: helpers shared by the PSS movie player (one global semaphore). */

/* Matching: #line keeps the original line numbers (declarations moved to headers). */
#line 10
static int pssSema = -1;

/** Empty; a place to set a debugger breakpoint. */
void PssBreakPoint(void) {
}

static int CreateSemaPss(void) {
    struct SemaParam param;

    param.initCount = 1;
    param.maxCount = 1;
    param.option = 0;
    return CreateSema(&param);
}

/** Creates the semaphore on first use, then waits on it. */
int WaitSemaPss(void) {
    if (pssSema == -1) {
        pssSema = CreateSemaPss();
        /*
         * CreateSema fails only when the kernel is out of semaphores,
         * which is fatal for the movie player.
         */

        assert(pssSema!=-1);
    }
    return WaitSema(pssSema);
}

/** Releases the movie player's semaphore. */
int SignalSemaPss(void) {
    return SignalSema(pssSema);
}
