/*
 * sh_sd_call.c: the game's front end to the sound driver. Every call is serialized
 * with a semaphore (the loader thread plays sound too), and sound ids can be muted
 * with the debug flags.
 */
#include "sh2.h"
#include "sdk/eekernel.h"
#include "lib/sh_kernel.h"

static int excl_sid = -1;
static int cd_sd_now_access;

static int shSdWaitExcl(void) {
    return WaitSema(excl_sid);
}

static int shSdSignalExcl(void) {
    return SignalSema(excl_sid);
}

/** Creates the sound-driver semaphore (once) and releases it.
 * @return 1 */
int shSdInit(void) {
    if (excl_sid == -1) {
        excl_sid = CreateSema2(0, 1, 0);
    }
    shSdSignalExcl();
    return 1;
}

/* Is the sound driver streaming from the disc? */
static int sd_stat_now_cd_using(void) {
    int stat;
    int ret;

    stat = sd_stat();
    ret = (stat & 0xF) != 0;
    ret |= ((1 << ((stat >> 4) & 0xF)) & ~0x21) != 0;
    return ret;
}

/** Per-frame sound driver update; tells the disc loader when the driver starts and stops streaming
 * from the disc. Skipped while sound is off (debug flag 1). */
void shSdVSync(void) {
    if (!dbFlag(1)) {
        shSdWaitExcl();
        if (!cd_sd_now_access && sd_stat_now_cd_using()) {
            cd_sd_now_access = 1;
            shCdSdStart();
        }
        sd_vsync();
        if (cd_sd_now_access && !sd_stat_now_cd_using()) {
            shCdSdEnd();
            cd_sd_now_access = 0;
        }
        sd_stat();
        shSdSignalExcl();
    }
}

static int shSdCallCheck(int i0) {
    if (dbFlag(1)) {
        return 0;
    }
    switch (i0) {
    case 1500:
        if (!dbFlag(2)) {
            return 0;
        }
        break;
    default:
        if (i0 < 5000 || i0 > 5078) {
            if (i0 < 60000 || i0 > 60086) {
                if (i0 >= 6000 && i0 <= 6063) {
                    if (!dbFlag(2)) {
                        return 0;
                    }
                } else if (i0 >= 63000) {
                    if (!dbFlag(2)) {
                        return 0;
                    }
                }
            }
        }
        break;
    }
    return 1;
}

/** Starts a sound (sd_call), unless it is muted by the debug flags.
 * @param i0 sound id
 * @param i1 driver argument
 * @param i2 driver argument
 * @param i3 driver argument
 * @return the driver's result, 0 when muted */
int shSdCall(int i0, int i1, int i2, int i3) {
    int ret;

    if (!shSdCallCheck(i0)) {
        return 0;
    }
    shSdWaitExcl();
    ret = sd_call(i0, i1, i2, i3);
    shSdSignalExcl();
    return ret;
}

/** Stops a sound effect (sd_se_stop), unless it is muted by the debug flags.
 * @param i0 sound id
 * @return the driver's result, 0 when muted */
int shSdSeStop(int i0) {
    int ret;

    if (!shSdCallCheck(i0)) {
        return 0;
    }
    shSdWaitExcl();
    ret = sd_se_stop(i0);
    shSdSignalExcl();
    return ret;
}

/** Returns the sound driver status (sd_stat); 0 while sound is off. */
int shSdStat(void) {
    int ret;

    if (!shSdCallCheck(0)) {
        return 0;
    }
    shSdWaitExcl();
    ret = sd_stat();
    shSdSignalExcl();
    return ret;
}

/** Starts a 3D sound (sd_3d_call).
 * @param i0 sound id
 * @param i1 driver argument
 * @param i2 driver argument
 * @param f0 driver argument
 * @param f1 driver argument
 * @return the driver's result, 0 while sound is off */
int shSd3dCall(int i0, int i1, int i2, float f0, float f1) {
    int ret;

    if (!shSdCallCheck(0)) {
        return 0;
    }
    shSdWaitExcl();
    ret = sd_3d_call(i0, i1, i2, f0, f1);
    shSdSignalExcl();
    return ret;
}

/** Updates a 3D sound (sd_3d_move).
 * @param i0 sound slot
 * @param i1 driver argument
 * @param i2 driver argument
 * @param f0 driver argument
 * @param f1 driver argument
 * @return the driver's result, 0 while sound is off */
int shSd3dMove(int i0, int i1, int i2, float f0, float f1) {
    int ret;

    if (!shSdCallCheck(0)) {
        return 0;
    }
    shSdWaitExcl();
    ret = sd_3d_move(i0, i1, i2, f0, f1);
    shSdSignalExcl();
    return ret;
}

/** Stops a 3D sound (sd_3d_stop).
 * @param i0 sound slot */
void shSd3dStop(int i0) {
    if (shSdCallCheck(0)) {
        shSdWaitExcl();
        sd_3d_stop(i0);
        shSdSignalExcl();
    }
}

/** Stops all 3D sounds (sd_3d_allstop). */
void shSd3dAllStop(void) {
    if (shSdCallCheck(0)) {
        shSdWaitExcl();
        sd_3d_allstop();
        shSdSignalExcl();
    }
}

/** Changes a playing sound effect (sd_se_change).
 * @param i0 sound id
 * @param i1 driver argument
 * @param i2 driver argument
 * @param i3 driver argument
 * @return the driver's result, 0 while sound is off */
int shSdSeChange(int i0, int i1, int i2, int i3) {
    int ret;

    if (!shSdCallCheck(0)) {
        return 0;
    }
    shSdWaitExcl();
    ret = sd_se_change(i0, i1, i2, i3);
    shSdSignalExcl();
    return ret;
}

/** Passes a track command to the sound driver (sd_track).
 * @param i0 driver argument
 * @param i1 driver argument
 * @return the driver's result, 0 while sound is off */
int shSdTrack(int i0, int i1) {
    int ret;

    if (!shSdCallCheck(0)) {
        return 0;
    }
    shSdWaitExcl();
    ret = sd_track(i0, i1);
    shSdSignalExcl();
    return ret;
}

/** Passes a radio command to the sound driver (sd_radio).
 * @param i0 driver argument
 * @param i1 driver argument
 * @param i2 driver argument
 * @param i3 driver argument
 * @return the driver's result, 0 while sound is off */
int shSdRadio(int i0, int i1, int i2, int i3) {
    int ret;

    if (!shSdCallCheck(0)) {
        return 0;
    }
    shSdWaitExcl();
    ret = sd_radio(i0, i1, i2, i3);
    shSdSignalExcl();
    return ret;
}

/** Returns the sound driver's 3D sound work (sd_3d_adrs). */
void *shSd3dAdrs(void) {
    void *ret;

    shSdWaitExcl();
    ret = sd_3d_adrs();
    shSdSignalExcl();
    return ret;
}
