/*
 * ps2_scf.c: thread-safe wrappers around the libscf system configuration calls
 * (language, aspect, time zone, ... from the console's settings).
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "sdk/libscf.h"
#include "lib/sh_kernel.h"

/* Defaults for the T10K (development tool): JST, 4:3, YYYYMMDD, English, S/PDIF off. */
static struct sceScfT10kConfig ps2ScfDefault[1] = {
    { 540, 0, 0, 1, 1, 0, 0 },
};

static int excl_sid = -1;

/** Creates the lock on first use and sets the development tool (T10K) defaults. */
void ps2ScfInit(void) {
    if (excl_sid == -1) {
        excl_sid = CreateSema2(0, 1, 0);
        sceScfSetT10kConfig(ps2ScfDefault);
        SignalSema(excl_sid);
    }
}

/** sceScfGetLanguage() under the lock. */
int ps2ScfGetLanguage(void) {
    int ret;

    WaitSema(excl_sid);
    ret = sceScfGetLanguage();
    SignalSema(excl_sid);
    return ret;
}

/** sceScfGetAspect() under the lock. */
int ps2ScfGetAspect(void) {
    int ret;

    WaitSema(excl_sid);
    ret = sceScfGetAspect();
    SignalSema(excl_sid);
    return ret;
}

/** sceScfGetSpdif() under the lock. */
int ps2ScfGetSpdif(void) {
    int ret;

    WaitSema(excl_sid);
    ret = sceScfGetSpdif();
    SignalSema(excl_sid);
    return ret;
}

/** sceScfGetTimeZone() under the lock. */
int ps2ScfGetTimeZone(void) {
    int ret;

    WaitSema(excl_sid);
    ret = sceScfGetTimeZone();
    SignalSema(excl_sid);
    return ret;
}

/** sceScfGetDateNotation() under the lock. */
int ps2ScfGetDateNotation(void) {
    int ret;

    WaitSema(excl_sid);
    ret = sceScfGetDateNotation();
    SignalSema(excl_sid);
    return ret;
}

/** sceScfGetSummerTime() under the lock. */
int ps2ScfGetSummerTime(void) {
    int ret;

    WaitSema(excl_sid);
    ret = sceScfGetSummerTime();
    SignalSema(excl_sid);
    return ret;
}

/** sceScfGetTimeNotation() under the lock. */
int ps2ScfGetTimeNotation(void) {
    int ret;

    WaitSema(excl_sid);
    ret = sceScfGetTimeNotation();
    SignalSema(excl_sid);
    return ret;
}

/** Converts `rtc` in place from the RTC's time (JST) to local time, under the lock. */
void ps2ScfGetLocalTimefromRTC(struct sceCdCLOCK *rtc) {
    WaitSema(excl_sid);
    sceScfGetLocalTimefromRTC(rtc);
    SignalSema(excl_sid);
}
