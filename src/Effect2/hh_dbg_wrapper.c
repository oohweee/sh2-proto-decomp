/*
 * hh_dbg_wrapper.c: debug helpers for the HH effect code: pad checks, an on-screen printf (empty
 * in this build) and an EE timer 0 stopwatch that prints its last and largest reading.
 */
#include "sh2.h"
#include "libc/stdio.h"

extern unsigned int pre_paddata;
extern unsigned int pre_paddata2;
unsigned int _t0_count;
static unsigned int _t0_count_max = 0;

/**
 * Checks pad buttons.
 * @param ControllerID pad number
 * @param Mode         0: just pressed, 1: held, 2: repeat, 3: never
 * @param Check_Assign button mask
 * @return non-zero if the buttons are in that state
 */
unsigned int HH_DBG_Wrapper_Controller_KeyAssign_Check(unsigned int ControllerID, unsigned int Mode,
                                                       unsigned int Check_Assign) {
    unsigned int result;

    result = 0;
    switch (Mode) {
    case 0:
        result = shPadTrigger(ControllerID, Check_Assign);
        break;
    case 1:
        result = shPadPress(ControllerID, Check_Assign);
        break;
    case 2:
        result = shPadRepeat(ControllerID, Check_Assign);
        break;
    case 3:
        break;
    }
    return result;
}

/** Prints str at screen position (x, y). Empty in this build (so the DWARF has no parameters). */
void HH_DBG_Wrapper_Printf(unsigned int x, unsigned int y, unsigned char *str) {
}

/** Starts the stopwatch: reads EE timer 0. */
void HH_DBG_Wrapper_T0_COUNT_Get(void) {
    _t0_count = *T0_COUNT;
}

/**
 * Stops the stopwatch: computes the timer 0 ticks since HH_DBG_Wrapper_T0_COUNT_Get, keeps the
 * largest (reset by a pad button) and prints both.
 */
void HH_DBG_Wrapper_T0_COUNT_Delta(void) {
    static char tmp[40];
    int current_t0_count;
    int delta;

    current_t0_count = *T0_COUNT;
    delta = current_t0_count - _t0_count;
    if (delta > _t0_count_max) {
        _t0_count_max = delta;
    }
    if (HH_DBG_Wrapper_Controller_KeyAssign_Check(0, 0, 0x8000)) {
        _t0_count_max = 0;
    }
    /* Matching: the casts make tmp evaluate first, as in the original, so HH_DBG_Wrapper_Printf's str
     * wasn't a char * (unsigned char * is a guess: the empty function has no parameters in the DWARF). */
    sprintf(tmp, "T0_COUNT Delta = %d\n", delta);
    HH_DBG_Wrapper_Printf(256, 224, (unsigned char *)tmp);
    sprintf(tmp, "T0_COUNT Max   = %d\n", _t0_count_max);
    HH_DBG_Wrapper_Printf(256, 216, (unsigned char *)tmp);
}
