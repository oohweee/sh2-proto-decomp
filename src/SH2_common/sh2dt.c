/*
 * sh2dt.c: frame timing. The game runs at 60 vblanks per second; "DF" is the
 * number of vblanks per game frame, "DT" the length of a game frame in seconds.
 */

#include "sh2.h"

static int sh2FramePerInt;
static int sh2FramePerIntSave;
static float sh2DeltaTimePerFrame;
static float sh2FramePerSec;
static int sh2FramePerIntReal;
static float sh2DeltaTimePerFrameReal;

/**
 * Sets the game frame length to `fpi` vblanks (and remembers it for shResetDF()); derives DT and
 * FPS.
 */
void shSetDF(int fpi) {
    sh2FramePerInt = fpi;
    sh2FramePerIntSave = fpi;
    sh2DeltaTimePerFrame = (float)fpi / 60.0f;
    sh2FramePerSec = 60.0f / (float)fpi;
}

/** Returns the game frame length in vblanks (0 while time is frozen). */
int shGetDF(void) {
    return sh2FramePerInt;
}

/** Returns the game frame length in seconds. */
float shGetDT(void) {
    return sh2DeltaTimePerFrame;
}

/** Returns the game frames per second. */
float shGetFPS(void) {
    return sh2FramePerSec;
}

/** Restores the frame rate last set with shSetDF (after shSetDFZero). */
void shResetDF(void) {
    shSetDF(sh2FramePerIntSave);
}

/** Freezes game time: zero frames and zero delta time. */
void shSetDFZero(void) {
    sh2FramePerInt = 0;
    sh2DeltaTimePerFrame = 0.0f;
    sh2FramePerSec = 0.0f;
}

/** Sets the real frame length to `fpi` vblanks (not frozen by shSetDFZero()). */
void shSetDFreal(int fpi) {
    sh2FramePerIntReal = fpi;
    sh2DeltaTimePerFrameReal = (float)fpi / 60.0f;
}

/** Returns the real frame length in seconds. */
float shGetDTreal(void) {
    return sh2DeltaTimePerFrameReal;
}
