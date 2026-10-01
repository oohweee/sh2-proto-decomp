/*
 * sh_sound.c: the game side of the sound driver. 2D sound effects (with an
 * optional position for pan/distance), 3D sound effects on the driver's 3D
 * channels, the per-stage sound effect banks and the BGM tracks, whose
 * volumes follow the ambient sound areas around the player (snd_select.c).
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "sh_vu0.h"
#include "fi_libvu0_inline.h"

static int SeChange2Dto3D(int se);
static int SeChange3Dto2D(int se);

/*
 * sqrt(v0 . v1) on the FPU (the length of v0 when v1 == v0). Not sh_vu0.h's _shLength (the
 * distance of two points), so it has its own name. Matching: not volatile: MWCC then optimizes
 * the block (the second load at +4 becomes mov.s), which the original shows.
 */
static inline float _shSqrtInnerProduct(float *v0, float *v1) {
    float r;

    __asm__("
    lwc1    %0, 0x0(%1)
    lwc1    $f8, 0x0(%2)
    lwc1    $f9, 0x4(%1)
    lwc1    $f10, 0x4(%2)
    mula.s  %0, $f8
    lwc1    %0, 0x8(%1)
    lwc1    $f8, 0x8(%2)
    madda.s $f9, $f10
    madd.s  %0, %0, $f8
    sqrt.s  %0, %0
    " : "=f"(r) : "r"(v0), "r"(v1));
    return r;
}

/* -1, 0 or 1 by the sign of x: sh_vu0.h's _shSign as a native asm block (Matching: kept local: the
 * native form switches off optimization in the caller). */
inline float fsign(float x) {
    asm {
        .set noreorder
        sub.s  $f8, $f8, $f8
        lui    t7, 0x3F80
        c.eq.s $f8, x
        bc1tl  L_end
        mov.s  x, $f8
        c.lt.s x, $f8
        mtc1   t7, x
        bc1tl  L_end
        neg.s  x, x
    L_end:
        .set reorder
    }
    return x;
}

int se_3d_channel_max;
struct Se_BgmBuffer bgm;
unsigned char snd_data_buffer[20480] __attribute__((aligned(64)));
float se_f_work;
static struct Se2D_ManageData se_2d_manage_data[4];
static struct Se3D_ChannelData se_3d_channel_data[8];
static int se_3d_channel_number;
static int se_load_data;
static int se_3d_load_data;
static struct _SOUND_WORK sound_work;

/** Waits (running the sound driver each frame) until it has reported idle wait times and is idle.
 * @param wait number of idle frames */
void SeWait(int wait) {
    int c;

    if (!dbFlag(1)) {
        do {
            verbose(4, "sh_sound.c:150> >>>>+++\n");
            for (c = wait; c > 0;) {
                if (!shSdStat()) {
                    verbose(4, "sh_sound.c:153> <<<<%d", c);
                    c--;
                }
                shSyncVEnd(0);
                shSdVSync();
            }
        } while (shSdStat());
        verbose(4, "sh_sound.c:160> <<<<%d", c);
    }
}

/** SeWait(60). */
void SeForceWait(void) {
    SeWait(60);
}

/** Starts the sound driver: data path, driver init with the sound data sector, output mode;
 * then waits for it.
 * @param sect disc sector of the sound data
 * @param mmode driver mode
 * @param path data path, or NULL */
void SeCallInit(int sect, int mmode, char *path) {
    if (!dbFlag(1)) {
        verbose(2, "sh_sound.c:182> ==========1\n");
        verbose(2, "sh_sound.c:184> sector: %d\n", sect);
        if (path) {
            sd_setpath(path);
        }
        shSdInit();
        shSdCall(1000, sect, mmode, 0);
        shSdCall(0x411, 0, 0, 0);
        verbose(2, "sh_sound.c:194> ==========2\n");
        shSdStat();
        SeForceWait();
    }
}

/** Stops all sounds (2D and 3D), clears the sound management data and the BGM state, and
 * resends the volumes. */
void SeCallReset(void) {
    int i;

    if (!dbFlag(1)) {
        SeWait(3);
        shSdCall(0, 0, 0, 0);
        SeWait(3);
        shSd3dAllStop();
        SeWait(3);
    }
    for (i = 0; i < 4; i++) {
        se_2d_manage_data[i].sd = 0;
    }
    se_3d_channel_max = 4;
    se_3d_channel_number = 0;
    for (i = 0; i < 8; i++) {
        se_3d_channel_data[i].sd = 0;
    }
    shQzero(&bgm, sizeof(bgm));
    if (!dbFlag(1)) {
        SeMasterVolumeChange();
        SeWait(3);
    }
    se_load_data = 0;
    se_3d_load_data = 0;
    shQzero(&sound_work, sizeof(sound_work));
}

/** Plays a 2D sound effect.
 * @param sd_no sound id (0: none)
 * @param volume volume, 0 to 1
 * @param stereo pan
 * @return 16 once started (or when sound is off), -1 if the driver failed, 15 for id 0 */
int SeCall(int sd_no, float volume, int stereo) {
    int ret;

    if (dbFlag(1)) {
        return 16;
    }
    if (sd_no == 0) {
        return 15;
    }
    ret = shSdCall(sd_no, stereo, ftoi(255.0f - 255.0f * volume), 0);
    if (ret != -1) {
        ret = 16;
    }
    return ret;
}

/** Plays a sound effect at a position: panned and attenuated by the distance to the camera and
 * player, or on a 3D channel (status bits choose which).
 * @param sd_no sound id (0: none)
 * @param volume volume, 0 to 1
 * @param pos position, or NULL
 * @param status flags: bits 0-1 channel choice (1: 3D channel if one is free), 4: no distance
 *        attenuation, 8: managed (tracked by Se2dManager)
 * @return the driver's result, or 15/16 when nothing was played */
int SeCallPos(int sd_no, float volume, float *pos, int status) {
    int direction;
    int distance;
    int work;
    int ret;
    int i;

    if (sd_no == 0) {
        return 15;
    }
    if (dbFlag(1)) {
        return 16;
    }
    if ((status & 8) && !(status & 3)) {
        for (i = 0; i < 4; i++) {
            if (sd_no == se_2d_manage_data[i].sd) {
                return 15;
            }
        }
        for (i = 0; i < 4; i++) {
            if (se_2d_manage_data[i].sd == 0) {
                break;
            }
        }
        /* Matching: the assert bakes its original line number into the object. */
#line 281
        assert_dw(i < 4);
        se_2d_manage_data[i].sd = sd_no;
        se_2d_manage_data[i].room = RoomNameJms();
        if (pos) {
            vcopy(pos, se_2d_manage_data[i].pos);
            se_2d_manage_data[i].pos_on = 1;
        } else {
            se_2d_manage_data[i].pos_on = 0;
        }
        se_2d_manage_data[i].vol = volume;
        se_2d_manage_data[i].status = status;
        se_2d_manage_data[i].timer = 0.0f;
        se_2d_manage_data[i].base = 0.0f;
    }
    if ((status & 3) == 1) {
        if (se_3d_channel_max > se_3d_channel_number) {
            work = SeChange2Dto3D(sd_no);
            if (work) {
                sd_no = work;
            }
        } else {
            sd_no = SeChange3Dto2D(sd_no);
            if (sd_no == 0) {
                return 15;
            }
        }
    }
    if ((status & 4) || !pos) {
        distance = 255 - ftoi(255.0f * volume);
    } else {
        distance = SeCallPosDistance(volume, pos);
    }
    if (pos) {
        direction = SeCallPosDirection(pos);
    } else {
        direction = 0;
    }
    if (status & 8) {
        se_2d_manage_data[i].pre_dist = distance;
        se_2d_manage_data[i].pre_pan = direction;
    }
    if (sd_no >= 40000) {
        ret = shSd3dCall(sd_no, direction, distance, 1.0f, 0.0f);
        if (ret != -1) {
            se_3d_channel_data[ret].sd = sd_no;
            se_3d_channel_data[ret].status = status;
            vcopy(pos, se_3d_channel_data[ret].pos);
            se_3d_channel_data[ret].vol = volume;
            se_3d_channel_data[ret].room = RoomNameJms();
            if ((status & 3) == 1) {
                se_3d_channel_number++;
            }
        } else if ((status & 3) == 1) {
            return SeCallPos(sd_no, volume, pos, status - (status & 3));
        }
    } else {
        ret = shSdCall(sd_no, direction, distance, 0);
        if (ret != -1) {
            ret = 16;
        }
    }
    return ret;
}

/** Updates the pan and volume of a playing positioned sound effect.
 * @param sd_no sound id (0: none)
 * @param volume volume, 0 to 1
 * @param pos position, or NULL (plain volume)
 * @param status flags
 * @return the driver's result */
int SeCallPosChange(int sd_no, float volume, float *pos, int status) {
    int direction;
    int distance;
    int ret;
    int work;
    int i;

    if (sd_no == 0) {
        return 15;
    }
    if (dbFlag(1)) {
        return 16;
    }
    if (pos == NULL) {
        direction = 0;
        distance = ftoi(255.0f - 255.0f * volume);
    } else {
        direction = SeCallPosDirection(pos);
        if (status & 4) {
            distance = 255 - ftoi(255.0f * volume);
        } else {
            distance = SeCallPosDistance(volume, pos);
        }
    }
    if (status & 8) {
        for (i = 0; i < 4; i++) {
            if (sd_no == se_2d_manage_data[i].sd) {
                break;
            }
        }
        work = ftoi(128.0f * shGetDT());
        if (distance < se_2d_manage_data[i].pre_dist - work) {
            distance = se_2d_manage_data[i].pre_dist - work;
        } else if (distance > work + se_2d_manage_data[i].pre_dist) {
            distance = work + se_2d_manage_data[i].pre_dist;
        }
        se_2d_manage_data[i].pre_dist = distance;
        if (direction < se_2d_manage_data[i].pre_pan - work) {
            direction = se_2d_manage_data[i].pre_pan - work;
        } else if (direction > work + se_2d_manage_data[i].pre_pan) {
            direction = work + se_2d_manage_data[i].pre_pan;
        }
        se_2d_manage_data[i].pre_pan = direction;
    }
    ret = shSdSeChange(sd_no, direction, distance, 0);
    if (ret != -1) {
        ret = 16;
    }
    return ret;
}

/** Returns the pan (-70 to 70) of a position as seen from the camera.
 * @param pos the position */
int SeCallPosDirection(float *pos) {
    float fv[4];
    float pos0[4];
    float sign;

    _sceVu0CopyVectorXYZ(pos0, pos);
    _sceVu0ApplyMatrix(fv, VbWvsMatrix.wvm, pos0);
    fv[0] -= 750.0f;
    fv[1] = 0.0f;
    _shNormalize(fv, fv);
    sign = fsign(-fv[0]);
    return ftoi(sign * (70.0f - 70.0f * fv[2]));
}

/** Returns the driver attenuation (0-255) of a sound at pos: volume scaled down with the distance
 * from the player and the camera's distance from him.
 * @param volume volume, 0 to 1
 * @param pos the position */
int SeCallPosDistance(float volume, float *pos) {
    float fvec[4];
    float pos0[4];
    float len;
    float work_0;
    float work_1;

    _sceVu0CopyVectorXYZ(pos0, pos);
    _sceVu0SubVector(fvec, vcWork.cam_pos, (float *)&sh2jms.player->pos);
    len = _shSqrtInnerProduct(fvec, fvec);
    work_0 = (len - 1500.0f) * 0.000100000005f;
    work_0 = 1.0f - fminf(fmaxf(0.0f, work_0), 1.0f);
    _sceVu0SubVector(fvec, pos0, (float *)&sh2jms.player->pos);
    len = _shSqrtInnerProduct(fvec, fvec);
    work_1 = (len - 1500.0f) * 0.000100000005f;
    work_1 = 1.0f - fminf(fmaxf(0.0f, work_1), 1.0f);
    return ftoi(255.0f - volume * (255.0f * work_0 * work_1));
}

/** Returns the distance factor (0 to 1) of a sound at pos, as used by SeCallPosDistance.
 * @param pos the position */
float SeCallPosDistanceF(float *pos) {
    float fvec[4];
    float pos0[4];
    float len;
    float work_0;
    float work_1;

    _sceVu0CopyVectorXYZ(pos0, pos);
    _sceVu0SubVector(fvec, vcWork.cam_pos, (float *)&sh2jms.player->pos);
    len = _shSqrtInnerProduct(fvec, fvec);
    work_0 = (len - 1500.0f) * 0.000100000005f;
    work_0 = 1.0f - fminf(fmaxf(0.0f, work_0), 1.0f);
    _sceVu0SubVector(fvec, pos0, (float *)&sh2jms.player->pos);
    len = _shSqrtInnerProduct(fvec, fvec);
    work_1 = (len - 1500.0f) * 0.000100000005f;
    work_1 = 1.0f - fminf(fmaxf(0.0f, work_1), 1.0f);
    return work_0 * work_1;
}

/** Returns non-zero if a sound is playing on a 3D channel.
 * @param sd_no sound id */
int Se3dPlayCheck(int sd_no) {
    int i;

    for (i = 0; i < 8; i++) {
        if (sd_no == se_3d_channel_data[i].sd) {
            return 1;
        }
    }
    return 0;
}

/** Loads the stage's sound effect banks (2D and 3D) and switches the BGM. */
void SeSoundLoad(void) {
    SeSoundEffectLoad(0);
    SeSoundEffect3dLoad(0);
    SeBgmChange();
}

/** Loads a sound effect bank.
 * @param data bank number, or 0 for the current stage's (and room's) */
void SeSoundEffectLoad(int data) {
    int room;

    if (!dbFlag(1)) {
        room = RoomNameJms();
        if (data == 0) {
            switch (playing.stage) {
            case 0:
                data = 0x139F;
                break;
            case 5:
                if ((game_flag.flag[7] >> 27) & 1) {
                    data = 0x13A1;
                } else {
                    data = 0x13A0;
                }
                break;
            case 14:
                if ((game_flag.flag[7] >> 27) & 1) {
                    data = 0x13A3;
                } else {
                    data = 0x13A2;
                }
                break;
            case 4:
                data = 0x138F;
                break;
            case 6:
            case 7:
            case 8:
            case 9:
            case 13:
                data = 0x1388;
                break;
            case 10:
            case 11:
                data = 0x138A;
                break;
            case 12:
                data = 0x1389;
                break;
            case 18:
            case 19:
            case 20:
            case 21:
                data = 0x1391;
                break;
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
                data = 0x1390;
                break;
            case 22:
                if (room == 0x30) {
                    data = 0x1397;
                } else {
                    data = 0x1390;
                }
                break;
            case 29:
            case 30:
                data = 0x138C;
                break;
            case 31:
                data = 0x139C;
                break;
            case 32:
                data = 0x139D;
                break;
            case 34:
                data = 0x139A;
                break;
            case 35:
            case 36:
                data = 0x1398;
                break;
            case 37:
                data = 0x138D;
                break;
            case 38:
                data = 0x139B;
                break;
            case 39:
            case 43:
            case 47:
                data = 0x1396;
                break;
            case 40:
                data = 0x1393;
                break;
            case 44:
                data = 0x1392;
                break;
            case 41:
            case 45:
                data = 0x1394;
                break;
            case 42:
            case 46:
                data = 0x1395;
                break;
            case 48:
            case 49:
            case 50:
            case 51:
                data = 0x138E;
                break;
            }
        }
        if (data != 0 && data != se_load_data) {
            se_load_data = data;
            shSdCall(0x3F2, 0, 0, 0);
            shSdCall(data, 0, 0, 0);
        }
    }
}

/** Loads a 3D sound effect bank.
 * @param data bank number, or 0 for the current stage's */
void SeSoundEffect3dLoad(int data) {
    static union fsFileIndex *se_file_list[35] = {
        NULL, data_sound_s_force_town_east_a_sfc, data_sound_s_force_town_east_b_sfc,
        data_sound_s_force_town_west_a_sfc, data_sound_s_force_town_west_b_sfc, data_sound_s_force_apart_e1f_sfc,
        data_sound_s_force_apart_e2f_sfc, data_sound_s_force_apart_e3f_sfc, data_sound_s_force_apart_w1f_sfc,
        data_sound_s_force_apart_w2f_sfc, data_sound_s_force_apart_stair_sfc, data_sound_s_force_hospital_1f_f_sfc,
        data_sound_s_force_hospital_2f_f_sfc, data_sound_s_force_hospital_3f_f_sfc, data_sound_s_force_hospital_rf_f_sfc,
        data_sound_s_force_iketani_sfc, data_sound_s_force_hospital_1f_b_sfc, data_sound_s_force_hospital_2f_b_sfc,
        data_sound_s_force_hospital_3f_b_sfc, data_sound_s_force_hospital_bf_b_sfc, data_sound_s_force_hospital_pass_sfc,
        data_sound_s_force_delusion_2_sfc, data_sound_s_force_delusion_3_sfc, data_sound_s_force_prison_sfc,
        data_sound_s_force_labyrinth_w_sfc, data_sound_s_force_labyrinth_e_sfc, data_sound_s_force_labyrinth_n_sfc,
        data_sound_s_force_eddie_boss_sfc, data_sound_s_force_hotel_bf_f_sfc, data_sound_s_force_hotel_1f_f_sfc,
        data_sound_s_force_hotel_2f_f_sfc, data_sound_s_force_hotel_bf_b_sfc, data_sound_s_force_hotel_1f_b_sfc,
        data_sound_s_force_hotel_2f_b_sfc, NULL
    };
    int room;
    int i;

    if (!dbFlag(1)) {
        room = RoomNameJms();
        if (data == 0) {
            switch (playing.stage) {
            case 5:
                if ((game_flag.flag[7] >> 27) & 1) {
                    data = 2;
                } else {
                    data = 1;
                }
                break;
            case 14:
                if ((game_flag.flag[7] >> 27) & 1) {
                    data = 4;
                } else {
                    data = 3;
                }
                break;
            case 6:
                data = 5;
                break;
            case 7:
                data = 6;
                break;
            case 9:
                data = 7;
                break;
            case 8:
                data = 7;
                break;
            case 10:
                data = 8;
                break;
            case 11:
                data = 9;
                break;
            case 12:
                data = 10;
                break;
            case 18:
                data = 11;
                break;
            case 19:
                data = 12;
                break;
            case 20:
                data = 13;
                break;
            case 21:
                data = 14;
                break;
            case 22:
                if (room == 0x30) {
                    data = 15;
                } else {
                    data = 16;
                }
                break;
            case 23:
                data = 16;
                break;
            case 24:
                data = 17;
                break;
            case 25:
                data = 18;
                break;
            case 26:
                data = 19;
                break;
            case 27:
                data = 20;
                break;
            case 29:
                data = 21;
                break;
            case 30:
                data = 22;
                break;
            case 31:
            case 32:
                data = 23;
                break;
            case 34:
                data = 26;
                break;
            case 35:
                data = 26;
                break;
            case 36:
                data = 26;
                break;
            case 37:
                data = 27;
                break;
            case 39:
                data = 28;
                break;
            case 40:
                data = 29;
                break;
            case 41:
                data = 30;
                break;
            case 43:
                data = 31;
                break;
            case 44:
                data = 32;
                break;
            case 45:
                data = 33;
                break;
            case 48:
            case 49:
            case 50:
            case 51:
                data = 34;
                break;
            }
        }
        if (data != 0 && data != se_3d_load_data) {
            se_3d_load_data = data;
            shSd3dAllStop();
            for (i = 0; i < 8; i++) {
                se_3d_channel_data[i].sd = 0;
            }
            FcRead(se_file_list[data], shSd3dAdrs());
        }
    }
}

/** Per-frame sound update: 2D sound effects, 3D channels and BGM. */
void SeSoundManager(void) {
    Se2dManager();
    Se3dManager();
    SeBgmManager();
}

/** Per-frame update of the managed 2D sound effects: stops the ones whose room the player has
 * left and updates their volume and timers. */
void Se2dManager(void) {
    float volume;
    int room;
    int i;

    if (sh2jms.player) {
        room = RoomNameJms();
    } else {
        room = 0;
    }
    for (i = 0; i < 4; i++) {
        if (se_2d_manage_data[i].sd == 0) {
            continue;
        }
        if (room && room != se_2d_manage_data[i].room) {
            SeStop(se_2d_manage_data[i].sd);
            continue;
        }
        switch (Sh2sys.step[2]) {
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            volume = 0.5f;
            break;
        case 14:
        default:
            switch (DramaDemoNumber()) {
            default:
                volume = 0.7f;
                break;
            case 0:
            case 1:
            case 14:
            case 2:
                volume = 1.0f;
                break;
            }
            break;
        }
        if (se_2d_manage_data[i].base > volume) {
            se_2d_manage_data[i].base = fmaxf(se_2d_manage_data[i].base - shGetDT(), volume);
        } else if (se_2d_manage_data[i].base < volume) {
            se_2d_manage_data[i].base = fminf(se_2d_manage_data[i].base + shGetDT(), volume);
        }
        if (se_2d_manage_data[i].pos_on) {
            SeCallPosChange(se_2d_manage_data[i].sd, se_2d_manage_data[i].vol * se_2d_manage_data[i].base, se_2d_manage_data[i].pos, se_2d_manage_data[i].status);
        } else {
            SeCallPosChange(se_2d_manage_data[i].sd, se_2d_manage_data[i].vol * se_2d_manage_data[i].base, NULL, se_2d_manage_data[i].status);
        }
        se_2d_manage_data[i].timer += shGetDT();
    }
}

/** Changes the volume of a managed 2D sound effect.
 * @param sd sound id
 * @param vol new volume (negative: fade out) */
void Se2dManageDataVolumeChange(int sd, float vol) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sd == se_2d_manage_data[i].sd) {
            break;
        }
    }
    if (i != 4) {
        if (vol < 0.0f) {
            SeStop(sd);
        } else {
            if (vol > 1.0f) {
                vol = 1.0f;
            }
            se_2d_manage_data[i].vol = vol;
        }
    }
}

/** Returns how long a managed 2D sound effect has played.
 * @param sd sound id
 * @param clear non-zero to reset the timer
 * @return the time, or -1 if the sound isn't playing */
float Se2dManageDataTimer(int sd, int clear) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sd == se_2d_manage_data[i].sd) {
            break;
        }
    }
    if (i == 4) {
        return -1.0f;
    }
    if (clear) {
        se_2d_manage_data[i].timer = 0.0f;
    }
    return se_2d_manage_data[i].timer;
}

/** Per-frame update of the 3D channels: stops the ones out of the player's room and moves the
 * others. */
void Se3dManager(void) {
    int direction;
    int distance;
    int ret;
    int room;
    int i;

    if (dbFlag(1)) {
        return;
    }
    room = RoomNameJms();
    for (i = 0; i < 8; i++) {
        if (se_3d_channel_data[i].sd == 0) {
            continue;
        }
        if (room != se_3d_channel_data[i].room) {
            SeStop(se_3d_channel_data[i].sd);
            continue;
        }
        direction = SeCallPosDirection(se_3d_channel_data[i].pos);
        if (se_3d_channel_data[i].status & 4) {
            distance = 255 - ftoi(255.0f * se_3d_channel_data[i].vol);
        } else {
            distance = SeCallPosDistance(se_3d_channel_data[i].vol, se_3d_channel_data[i].pos);
        }
        ret = shSd3dMove(i, direction, distance, 0.0f, 0.0f);
        if (ret == -1) {
            se_3d_channel_data[i].sd = 0;
            if ((se_3d_channel_data[i].status & 3) == 1) {
                se_3d_channel_number--;
            }
        }
    }
}

/** Updates the volume and position of a sound on a 3D channel.
 * @param sd_no sound id (0: none)
 * @param volume volume, 0 to 1
 * @param pos position
 * @return the driver's result */
int Se3dControl(int sd_no, float volume, float *pos) {
    int i;

    if (dbFlag(1)) {
        return 16;
    }
    if (sd_no == 0) {
        return 15;
    }
    for (i = 0; i < 8; i++) {
        if (sd_no == se_3d_channel_data[i].sd) {
            break;
        }
    }
    if (i == 8) {
        return -1;
    }
    vcopy(pos, se_3d_channel_data[i].pos);
    se_3d_channel_data[i].vol = volume;
    return 16;
}

/** Stops a sound effect (2D or 3D) and frees its management slot.
 * @param sd_no sound id (0: none) */
void SeStop(int sd_no) {
    int work;
    int i;

    if (dbFlag(1)) {
        return;
    }
    if (sd_no == 0) {
        return;
    }
    if (sd_no < 40000) {
        shSdSeStop(sd_no);
        for (i = 0; i < 4; i++) {
            if (sd_no == se_2d_manage_data[i].sd) {
                break;
            }
        }
        if (i != 4) {
            se_2d_manage_data[i].sd = 0;
        }
        sd_no = SeChange2Dto3D(sd_no);
        if (sd_no == 0) {
            return;
        }
    } else {
        work = SeChange3Dto2D(sd_no);
        if (work) {
            shSdSeStop(work);
        }
    }
    for (i = 0; i < 8; i++) {
        if (sd_no == se_3d_channel_data[i].sd) {
            shSd3dStop(i);
            se_3d_channel_data[i].sd = 0;
            if ((se_3d_channel_data[i].status & 3) == 1) {
                se_3d_channel_number--;
            }
        }
    }
}

static union fsFileIndex *sdb_list[53] = {
    data_sound_snd_data_null_sdb, data_sound_snd_data_tgs_trial_sdb, data_sound_snd_data_toilet_sdb,
    data_sound_snd_data_observation_sdb, data_sound_snd_data_forest_sdb, data_sound_snd_data_town_east_sdb,
    data_sound_snd_data_apart_e1f_sdb, data_sound_snd_data_apart_e2f_sdb, data_sound_snd_data_apart_e3fw_sdb,
    data_sound_snd_data_apart_e3fe_sdb, data_sound_snd_data_apart_w1f_sdb, data_sound_snd_data_apart_w2f_sdb,
    data_sound_snd_data_apart_stair_sdb, data_sound_snd_data_apart_out_sdb, data_sound_snd_data_town_west_sdb,
    data_sound_snd_data_bowling_sdb, data_sound_snd_data_to_heaven_sdb, data_sound_snd_data_heaven_night_sdb,
    data_sound_snd_data_hospital_1f_f_sdb, data_sound_snd_data_hospital_2f_f_sdb, data_sound_snd_data_hospital_3f_f_sdb,
    data_sound_snd_data_hospital_rf_f_sdb, data_sound_snd_data_hospital_1fw_b_sdb, data_sound_snd_data_hospital_1fe_b_sdb,
    data_sound_snd_data_hospital_2f_b_sdb, data_sound_snd_data_hospital_3f_b_sdb, data_sound_snd_data_hospital_bf_b_sdb,
    data_sound_snd_data_hospital_pass_sdb, data_sound_snd_data_society_sdb, data_sound_snd_data_delusion_2_sdb,
    data_sound_snd_data_delusion_3_sdb, data_sound_snd_data_prison_n_sdb, data_sound_snd_data_prison_s_sdb,
    data_sound_snd_data_prison_bf_sdb, data_sound_snd_data_labyrinth_w_sdb, data_sound_snd_data_labyrinth_e_sdb,
    data_sound_snd_data_labyrinth_n_sdb, data_sound_snd_data_eddie_boss_sdb, data_sound_snd_data_lake_sdb,
    data_sound_snd_data_hotel_bf_f_sdb, data_sound_snd_data_hotel_1f_f_sdb, data_sound_snd_data_hotel_2f_f_sdb,
    data_sound_snd_data_hotel_3f_f_sdb, data_sound_snd_data_hotel_bf_b_sdb, data_sound_snd_data_hotel_1f_b_sdb,
    data_sound_snd_data_hotel_2f_b_sdb, data_sound_snd_data_hotel_3f_b_sdb, data_sound_snd_data_hotel_fire_sdb,
    data_sound_snd_data_end_recovery_sdb, data_sound_snd_data_end_maria_sdb, data_sound_snd_data_end_suicide_sdb,
    data_sound_snd_data_end_rebirth_sdb, data_sound_snd_data_end_dog_sdb
};

/** Loads the BGM data of the current stage and starts the tracks it calls for. */
void SeBgmChange(void) {
    int room;
    int next;

    if (dbFlag(1)) {
        return;
    }
    if (Sh2sys.step[0] == 2) {
        shQzero(snd_data_buffer, sizeof(snd_data_buffer));
        FcRead(sdb_list[0], snd_data_buffer);
        fsSync(0, -1);
        bgm.sdb_no = 0;
    } else {
        next = playing.stage;
        if (bgm.sdb_no != next) {
            shQzero(snd_data_buffer, sizeof(snd_data_buffer));
            FcRead(sdb_list[next], snd_data_buffer);
            bgm.sdb_no = next;
        }
    }
    room = RoomNameJms();
    switch (playing.stage) {
    case 2:
    case 3:
    case 4:
        if ((game_flag.flag[1] >> 7) & 1) {
            SeBgmCall(0);
        } else if ((game_flag.flag[1] >> 4) & 1) {
            SeBgmCall(3);
        } else {
            SeBgmCall(1);
        }
        break;
    case 5:
        SeBgmCall(3);
        break;
    case 6:
        if (!((game_flag.flag[2] >> 21) & 1) && room == 0x1E) {
            SeBgmCall(7);
        } else if ((game_flag.flag[2] >> 27) & 1) {
            SeBgmCall(9);
        } else {
            SeBgmCall(4);
        }
        break;
    case 7:
    case 8:
        if ((game_flag.flag[2] >> 27) & 1) {
            SeBgmCall(9);
        } else {
            SeBgmCall(5);
        }
        break;
    case 9:
        if ((game_flag.flag[2] >> 27) & 1) {
            SeBgmCall(9);
        } else {
            SeBgmCall(6);
        }
        break;
    case 10:
        if (!((game_flag.flag[3] >> 13) & 1) && room == 0x24) {
            SeBgmCall(8);
        } else {
            SeBgmCall(10);
        }
        break;
    case 11:
        SeBgmCall(10);
        break;
    case 12:
        SeBgmCall(12);
        break;
    case 14:
        if ((game_flag.flag[5] >> 2) & 1) {
            SeBgmCall(14);
        } else {
            SeBgmCall(11);
        }
        break;
    case 15:
    case 16:
        SeBgmCall(14);
        break;
    case 17:
        SeBgmCall(15);
        break;
    case 18:
        SeBgmCall(16);
        break;
    case 19:
    case 21:
        SeBgmCall(16);
        break;
    case 20:
        SeBgmCall(17);
        break;
    case 22:
        if (room == 0x34) {
            SeBgmCall(18);
        } else if (room == 0x30) {
            SeBgmCall(19);
        } else {
            SeBgmCall(20);
        }
        break;
    case 23:
        SeBgmCall(23);
        break;
    case 24:
        SeBgmCall(20);
        break;
    case 25:
        SeBgmCall(21);
        break;
    case 27:
        SeBgmCall(22);
        break;
    case 29:
    case 30:
        SeBgmCall(24);
        break;
    case 31:
    case 32:
        SeBgmCall(25);
        break;
    case 33:
        SeBgmCall(26);
        break;
    case 34:
    case 35:
        SeBgmCall(27);
        break;
    case 36:
        if (((game_flag.flag[11] >> 16) & 1) || room == 0x89) {
            SeBgmCall(30);
        } else {
            SeBgmCall(27);
        }
        break;
    case 37:
        SeBgmCall(31);
        break;
    case 38:
        SeBgmCall(32);
        break;
    case 39:
        SeBgmCall(35);
        break;
    case 40:
        if (!((game_flag.flag[14] >> 24) & 1)) {
            SeBgmCall(35);
        } else {
            SeBgmCall(34);
        }
        break;
    case 41:
    case 42:
        SeBgmCall(36);
        break;
    case 43:
        SeBgmCall(38);
    case 44:
        if (room == 0xAC) {
            SeBgmCall(39);
        } else {
            SeBgmCall(38);
        }
        break;
    case 45:
    case 46:
        SeBgmCall(40);
        break;
    case 47:
        SeBgmCall(41);
        break;
    case 48:
    case 49:
    case 50:
    case 51:
        SeBgmCall(40);
        break;
    default:
        SeBgmCall(0);
        break;
    }
}

/** Switches to a BGM piece: sets it as the next one and fades out the playing tracks.
 * @param bgm_no BGM number */
void SeBgmCall(int bgm_no) {
    static int bgm_list[43][2] = {
        { 0x0, 0x0 }, { 0x13C9, 0xC3C2 }, { 0x13B9, 0xC365 }, { 0x13CA, 0xC3C3 },
        { 0x13A6, 0xC352 }, { 0x13CD, 0xC3C6 }, { 0x13BD, 0xC3B7 }, { 0x13A4, 0xC350 },
        { 0x13B1, 0xC35D }, { 0x13BB, 0xC3B5 }, { 0x13BC, 0xC3B6 }, { 0x13CC, 0xC3C5 },
        { 0x13D4, 0xC3CD }, { 0x13A7, 0xC353 }, { 0x13A8, 0xC354 }, { 0x13B5, 0xC361 },
        { 0x13CE, 0xC3C7 }, { 0x13C5, 0xC3BF }, { 0x13D2, 0xC3CB }, { 0x13B3, 0xC35F },
        { 0x13BF, 0xC3B9 }, { 0x13BA, 0xC3B4 }, { 0x13C0, 0xC3BA }, { 0x13B8, 0xC364 },
        { 0x13C1, 0xC3BB }, { 0x13C4, 0xC3BE }, { 0x13C3, 0xC3BD }, { 0x13CB, 0xC3C4 },
        { 0x13B2, 0xC35E }, { 0x13D6, 0xC3D0 }, { 0x13B7, 0xC363 }, { 0x13CF, 0xC3C8 },
        { 0x13D5, 0xC3CE }, { 0x13B4, 0xC360 }, { 0x13C6, 0xC3C0 }, { 0x13C7, 0xC3CF },
        { 0x13D0, 0xC3C9 }, { 0x13AC, 0xC358 }, { 0x13C2, 0xC3BC }, { 0x13D3, 0xC3CC },
        { 0x13D1, 0xC3CA }, { 0x13C8, 0xC3C1 }, { 0x13A5, 0xC351 }
    };
    int i;

    if (bgm.current == bgm_list[bgm_no][1]) {
        return;
    }
    bgm.next = bgm_list[bgm_no][1];
    bgm.read = bgm_list[bgm_no][0];
    for (i = 0; i < 7; i++) {
        if (bgm.track[i].status != 0) {
            if (bgm.track[i].status == 3) {
                if (bgm.track[i].fade_out_type == 3) {
                    bgm.track[i].fade_out_type = 1;
                }
            } else {
                bgm.track[i].status = 3;
                bgm.track[i].fade_out_type = 1;
            }
        }
    }
}

/** Per-frame BGM update: switches pieces and fades the tracks toward the volumes of the current
 * BGM page. */
void SeBgmManager(void) {
    static int bgm_read_check = 0;
    static int fade_prio[5] = { 0, 2, 3, 1, 4 };
    static float track_fade_time[5] = { 0.0f, 3.0f, 1.5f, 6.0f, 0.0f };
    float volume;
    int in;
    int out;
    int work;
    int i;
    int j;

    if (bgm.current != bgm.next && bgm.next != 0) {
        for (i = 0; i < 7; i++) {
            if (bgm.track[i].status) {
                break;
            }
        }
        if (i == 7) {
            shSdCall(0x3F3, 0, 0, 0);
            shSdCall(bgm.read, 0, 0, 0);
            bgm.current = bgm.next;
            bgm_read_check = 0;
        }
    } else if (bgm.read) {
        if (!(shSdStat() & 0xF)) {
            shSdCall(bgm.next, 0x7F, 0, 0);
            bgm.read = 0;
            for (i = 0; i < 7; i++) {
                shSdTrack(i + 1, 0);
            }
        }
    }
    if (bgm.current && !bgm.read) {
        sound_work.sound_ary = (struct _SOUND_DATA *)snd_data_buffer;
        if (!sh2jms.player) {
            __asm__ __volatile__("
            sqc2 vf0, 0x0(%0)
            " : : "r"(sound_work.chara_pos));
        } else {
            sound_work.chara_pos[0] = sh2jms.player->pos.x;
            sound_work.chara_pos[1] = sh2jms.player->pos.y;
            sound_work.chara_pos[2] = sh2jms.player->pos.z;
            sound_work.chara_pos[3] = 1.0f;
        }
        sound_work.half_w = 50.0f;
        sound_work.page = BgmPageSet();
        if (Sh2sys.step[2] == 4) {
            sndGetSoundAryByCharaPos(&sound_work);
        }
        /* Matching: the assert bakes its original line number into the object. */
#line 1256
        assert(sound_work.on_num <= 7);
        for (i = 0; i < 7; i++) {
            in = out = 0;
            for (j = 0; j < sound_work.near_sound_num; j++) {
                work = sound_work.near_sound_ary[j].inVol[i];
                if (fade_prio[in] < fade_prio[work]) { in = work; }
                work = sound_work.near_sound_ary[j].outVol[i];
                if (fade_prio[out] < fade_prio[work]) { out = work; }
            }

            assert(in == 0 || ( in != 0 && out != 0 ));
            if (in == 0) {
                if (bgm.track[i].status) {
                    bgm.track[i].status = 3;
                }
            } else if (bgm.track[i].status == 0 || bgm.track[i].status == 3) {
                bgm.track[i].status = 1;
                bgm.track[i].fade_in_type = in;
                bgm.track[i].fade_out_type = out;
            }
        }
    }
    for (i = 0; i < 7; i++) {
        if (bgm.track[i].status == 1) {
            if (bgm.track[i].fade_in_type == 4) {
                bgm.track[i].volume = 1.0f;
            } else {
                bgm.track[i].volume += shGetDT() / track_fade_time[bgm.track[i].fade_in_type];
            }
            if (bgm.track[i].volume >= 1.0f) {
                bgm.track[i].status = 2;
                bgm.track[i].volume = 1.0f;
            }
            shSdTrack(i + 1, ftoi(127.0f * bgm.track[i].volume));
        } else if (bgm.track[i].status == 3) {
            if (bgm.track[i].fade_out_type == 4) {
                bgm.track[i].volume = 0.0f;
            } else {
                bgm.track[i].volume -= shGetDT() / track_fade_time[bgm.track[i].fade_out_type];
            }
            if (bgm.track[i].volume <= 0.0f) {
                bgm.track[i].status = 0;
                bgm.track[i].volume = 0.0f;
            }
            shSdTrack(i + 1, ftoi(127.0f * bgm.track[i].volume));
        }
    }
    switch (Sh2sys.step[2]) {
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        volume = 0.5f;
        break;
    case 14:
    default:
        switch (DramaDemoNumber()) {
        default:
            volume = 0.7f;
            break;
        case 0:
        case 1:
        case 14:
        case 2:
            volume = 1.0f;
            break;
        }
        break;
    }
    if (bgm.volume > volume) {
        bgm.volume = fmaxf(bgm.volume - shGetDT() / 2.0f, volume);
        shSdCall(0x3FC, ftoi(127.0f * bgm.volume), 0, 0);
    } else if (bgm.volume < volume) {
        bgm.volume = fminf(bgm.volume + shGetDT() / 2.0f, volume);
        shSdCall(0x3FC, ftoi(127.0f * bgm.volume), 0, 0);
    }
}

/* squared XZ distance from James */
static inline float JamesDist2XZ(struct SubCharacter *scp) {
    float dx;
    float dz;

    dx = scp->pos.x - sh2jms.player->pos.x;
    dx *= dx;
    dz = scp->pos.z - sh2jms.player->pos.z;
    dz *= dz;
    return dx + dz;
}

/**
 * Returns the current BGM page. That is the stage's bgm_control() result (0 before system step 4
 * or without one) when it is above 3; otherwise 3 while an enemy within 6000 of James (XZ) is in
 * battle (battle.status 0x400 set, 0x2 clear), else 2 while James's HP is under 20%, else the result.
 */
int BgmPageSet(void) {
    struct SubCharacter *scp;
    int ret;

    if (Sh2sys.step[2] >= 4 && stage && stage->bgm_control) {
        ret = stage->bgm_control();
    } else {
        ret = 0;
    }
    if (ret >= 6) {
        return ret;
    }
    if (ret > 3) {
        return ret;
    }
    for (scp = shCharacter_Manage_GetCharacterList(); scp; scp = scp->next) {
        if ((scp->kind >> 8) == 2 && ((scp->battle.status >> 10) & 1) && !((scp->battle.status >> 1) & 1) &&
            JamesDist2XZ(scp) < 36000000.0f) {
            break;
        }
    }
    if (scp) {
        return 3;
    }
    if (sh2jms.player && shBattleGetJamesHP_Rate() < 0.2f) {
        return 2;
    }
    return ret;
}

/** Sends the BGM and sound-effect volumes of the settings to the driver. */
void SeMasterVolumeChange(void) {
    if (!dbFlag(1)) {
        shSdCall(0x402, playing.bgm_volume * 8, 0, 0);
        shSdCall(0x401, playing.se_volume * 8, 0, 0);
    }
}

static struct Sd3dChangeData change_list[242] = {
    { 0x2EE9, 0x9D1D, 1 },
    { 0x2EF3, 0x9D1C, 1 },
    { 0x2FA8, 0x9D20, 1 },
    { 0x2FA9, 0x9D21, 1 },
    { 0x2FB0, 0x9D1E, 1 },
    { 0x2FB1, 0x9D1F, 1 },
    { 0x30D4, 0x9D22, 2 },
    { 0x3EE4, 0x9D27, 2 },
    { 0x3EE6, 0x9D28, 2 },
    { 0x3EE8, 0x9D29, 2 },
    { 0x3EF1, 0x9D2A, 2 },
    { 0x3EF6, 0x9D2B, 2 },
    { 0x3EF7, 0x9D2C, 2 },
    { 0x2F44, 0x9D24, 2 },
    { 0x2F47, 0x9D25, 2 },
    { 0x2F4B, 0x9D26, 2 },
    { 0x30D4, 0x9D2D, 3 },
    { 0x3EE4, 0x9D31, 3 },
    { 0x3EE6, 0x9D32, 3 },
    { 0x3EE8, 0x9D33, 3 },
    { 0x3EF1, 0x9D34, 3 },
    { 0x3EF6, 0x9D35, 3 },
    { 0x3EF7, 0x9D36, 3 },
    { 0x2EE9, 0x9D30, 3 },
    { 0x2EF3, 0x9D2F, 3 },
    { 0x3EE4, 0x9D3C, 4 },
    { 0x3EE6, 0x9D3D, 4 },
    { 0x3EE8, 0x9D3E, 4 },
    { 0x3EF1, 0x9D3F, 4 },
    { 0x3EF6, 0x9D40, 4 },
    { 0x3EF7, 0x9D41, 4 },
    { 0x2F44, 0x9D39, 4 },
    { 0x2F47, 0x9D3A, 4 },
    { 0x2F4B, 0x9D3B, 4 },
    { 0x2FDA, 0x9D37, 4 },
    { 0x2FDB, 0x9D38, 4 },
    { 0x2FA8, 0x9C45, 5 },
    { 0x2FA9, 0x9C46, 5 },
    { 0x2FB0, 0x9C43, 5 },
    { 0x2FB1, 0x9C44, 5 },
    { 0x2EE9, 0x9C42, 5 },
    { 0x2EF3, 0x9C41, 5 },
    { 0x2EE9, 0x9C4C, 6 },
    { 0x2EF3, 0x9C4B, 6 },
    { 0x2FA8, 0x9C4F, 6 },
    { 0x2FA9, 0x9C50, 6 },
    { 0x2FB0, 0x9C4D, 6 },
    { 0x2FB1, 0x9C4E, 6 },
    { 0x30D4, 0x9C4A, 6 },
    { 0x2FA8, 0x9C57, 7 },
    { 0x2FA9, 0x9C58, 7 },
    { 0x2FB0, 0x9C55, 7 },
    { 0x2FB1, 0x9C56, 7 },
    { 0x2EE9, 0x9C54, 7 },
    { 0x2EF3, 0x9C53, 7 },
    { 0x30D4, 0x9C52, 7 },
    { 0x3EE4, 0x9C67, 8 },
    { 0x3EE6, 0x9C68, 8 },
    { 0x3EE8, 0x9C69, 8 },
    { 0x3EF1, 0x9C6A, 8 },
    { 0x3EF6, 0x9C6B, 8 },
    { 0x30D4, 0x9C62, 8 },
    { 0x2FA8, 0x9C65, 8 },
    { 0x2FA9, 0x9C66, 8 },
    { 0x2FB0, 0x9C63, 8 },
    { 0x2FB1, 0x9C64, 8 },
    { 0x30D4, 0x9C6D, 9 },
    { 0x2EE9, 0x9C6F, 9 },
    { 0x2EF3, 0x9C6E, 9 },
    { 0x30D4, 0x9C59, 10 },
    { 0x2EE9, 0x9C5B, 10 },
    { 0x2EF3, 0x9C5A, 10 },
    { 0x3EE4, 0x9C5C, 10 },
    { 0x3EE6, 0x9C5D, 10 },
    { 0x3EE8, 0x9C5E, 10 },
    { 0x3EF1, 0x9C5F, 10 },
    { 0x3EF6, 0x9C60, 10 },
    { 0x3EF7, 0x9C61, 10 },
    { 0x30D4, 0x9C8E, 11 },
    { 0x3EE4, 0x9C92, 11 },
    { 0x3EE6, 0x9C93, 11 },
    { 0x3EE8, 0x9C94, 11 },
    { 0x3EF1, 0x9C95, 11 },
    { 0x3EF6, 0x9C96, 11 },
    { 0x3EF7, 0x9C97, 11 },
    { 0x2F44, 0x9C8F, 11 },
    { 0x2F47, 0x9C90, 11 },
    { 0x2F4B, 0x9C91, 11 },
    { 0x30D4, 0x9CA1, 12 },
    { 0x3EE4, 0x9CA5, 12 },
    { 0x3EE6, 0x9CA6, 12 },
    { 0x3EE8, 0x9CA7, 12 },
    { 0x3EF1, 0x9CA8, 12 },
    { 0x3EF6, 0x9CA9, 12 },
    { 0x3EF7, 0x9CAA, 12 },
    { 0x2F44, 0x9CA2, 12 },
    { 0x2F47, 0x9CA3, 12 },
    { 0x2F4B, 0x9CA4, 12 },
    { 0x30D4, 0x9CB5, 13 },
    { 0x3EE4, 0x9CB9, 13 },
    { 0x3EE6, 0x9CBA, 13 },
    { 0x3EE8, 0x9CBB, 13 },
    { 0x3EF1, 0x9CBC, 13 },
    { 0x3EF6, 0x9CBD, 13 },
    { 0x3EF7, 0x9CBE, 13 },
    { 0x2F44, 0x9CB6, 13 },
    { 0x2F47, 0x9CB7, 13 },
    { 0x2F4B, 0x9CB8, 13 },
    { 0x30D4, 0x9CCF, 14 },
    { 0x3EE4, 0x9CD3, 14 },
    { 0x3EE6, 0x9CD4, 14 },
    { 0x3EE8, 0x9CD5, 14 },
    { 0x3EF1, 0x9CD6, 14 },
    { 0x3EF6, 0x9CD7, 14 },
    { 0x3EF7, 0x9CD8, 14 },
    { 0x2F44, 0x9CD0, 14 },
    { 0x2F47, 0x9CD1, 14 },
    { 0x2F4B, 0x9CD2, 14 },
    { 0x4844, 0x9CF5, 15 },
    { 0x4850, 0x9CF6, 15 },
    { 0x484C, 0x9CF4, 15 },
    { 0x3EE4, 0x9C88, 16 },
    { 0x3EE6, 0x9C89, 16 },
    { 0x3EE8, 0x9C8A, 16 },
    { 0x3EF1, 0x9C8B, 16 },
    { 0x3EF6, 0x9C8C, 16 },
    { 0x3EF7, 0x9C8D, 16 },
    { 0x2F44, 0x9C85, 16 },
    { 0x2F47, 0x9C86, 16 },
    { 0x2F4B, 0x9C87, 16 },
    { 0x3EE4, 0x9C9B, 17 },
    { 0x3EE6, 0x9C9C, 17 },
    { 0x3EE8, 0x9C9D, 17 },
    { 0x3EF1, 0x9C9E, 17 },
    { 0x3EF6, 0x9C9F, 17 },
    { 0x3EF7, 0x9CA0, 17 },
    { 0x2F44, 0x9C98, 17 },
    { 0x2F47, 0x9C99, 17 },
    { 0x2F4B, 0x9C9A, 17 },
    { 0x3EE4, 0x9CAF, 18 },
    { 0x3EE6, 0x9CB0, 18 },
    { 0x3EE8, 0x9CB1, 18 },
    { 0x3EF1, 0x9CB2, 18 },
    { 0x3EF6, 0x9CB3, 18 },
    { 0x3EF7, 0x9CB4, 18 },
    { 0x2F44, 0x9CAC, 18 },
    { 0x2F47, 0x9CAD, 18 },
    { 0x2F4B, 0x9CAE, 18 },
    { 0x3EE4, 0x9CC3, 19 },
    { 0x3EE6, 0x9CC4, 19 },
    { 0x3EE8, 0x9CC5, 19 },
    { 0x3EF1, 0x9CC6, 19 },
    { 0x3EF6, 0x9CC7, 19 },
    { 0x3EF7, 0x9CC8, 19 },
    { 0x2F44, 0x9CC0, 19 },
    { 0x2F47, 0x9CC1, 19 },
    { 0x2F4B, 0x9CC2, 19 },
    { 0x3EE4, 0x9CC9, 20 },
    { 0x3EE6, 0x9CCA, 20 },
    { 0x3EE8, 0x9CCB, 20 },
    { 0x3EF1, 0x9CCC, 20 },
    { 0x3EF6, 0x9CCD, 20 },
    { 0x3EF7, 0x9CCE, 20 },
    { 0x2EE9, 0x9C71, 21 },
    { 0x2EF3, 0x9C70, 21 },
    { 0x2FA8, 0x9C75, 21 },
    { 0x2FA9, 0x9C76, 21 },
    { 0x2FB0, 0x9C73, 21 },
    { 0x2FB1, 0x9C74, 21 },
    { 0x2EE9, 0x9C79, 21 },
    { 0x2EF3, 0x9C78, 21 },
    { 0x2FA8, 0x9C7D, 21 },
    { 0x2FA9, 0x9C7E, 21 },
    { 0x2FB0, 0x9C7B, 21 },
    { 0x2FB1, 0x9C7C, 21 },
    { 0x2EE9, 0x9D12, 23 },
    { 0x2EF3, 0x9D11, 23 },
    { 0x2FA8, 0x9D15, 23 },
    { 0x2FA9, 0x9D16, 23 },
    { 0x2FB0, 0x9D13, 23 },
    { 0x2FB1, 0x9D14, 23 },
    { 0x3EE4, 0x9D17, 23 },
    { 0x3EE6, 0x9D18, 23 },
    { 0x3EE8, 0x9D19, 23 },
    { 0x3EF1, 0x9D1A, 23 },
    { 0x3EF6, 0x9D1B, 23 },
    { 0x2FDA, 0x9D09, 24 },
    { 0x2FDB, 0x9D0A, 24 },
    { 0x2EE9, 0x9D0C, 24 },
    { 0x2EF3, 0x9D0B, 24 },
    { 0x2FA8, 0x9D0F, 24 },
    { 0x2FA9, 0x9D10, 24 },
    { 0x2FB0, 0x9D0D, 24 },
    { 0x2FB1, 0x9D0E, 24 },
    { 0x477C, 0x9CF9, 25 },
    { 0x2EE9, 0x9CF8, 25 },
    { 0x2EF3, 0x9CF7, 25 },
    { 0x3EE4, 0x9CFA, 25 },
    { 0x3EE6, 0x9CFB, 25 },
    { 0x3EE8, 0x9CFC, 25 },
    { 0x3EF1, 0x9CFD, 25 },
    { 0x3EF6, 0x9CFE, 25 },
    { 0x3EF7, 0x9CFF, 25 },
    { 0x2EE9, 0x9D02, 26 },
    { 0x2EF3, 0x9D01, 26 },
    { 0x3EE4, 0x9D03, 26 },
    { 0x3EE6, 0x9D04, 26 },
    { 0x3EE8, 0x9D05, 26 },
    { 0x3EF1, 0x9D06, 26 },
    { 0x3EF6, 0x9D07, 26 },
    { 0x3EF7, 0x9D08, 26 },
    { 0x4718, 0x9C7F, 27 },
    { 0x477C, 0x9CF2, 28 },
    { 0x4783, 0x9CF3, 28 },
    { 0x30D4, 0x9CF1, 28 },
    { 0x477C, 0x9CE0, 29 },
    { 0x4783, 0x9CE1, 29 },
    { 0x30D4, 0x9CDF, 29 },
    { 0x477C, 0x9CE7, 30 },
    { 0x4783, 0x9CE8, 30 },
    { 0x30D4, 0x9CE6, 30 },
    { 0x3EF1, 0x9CE9, 30 },
    { 0x3EF6, 0x9CEA, 30 },
    { 0x3EF7, 0x9CEB, 30 },
    { 0x477C, 0x9CEE, 31 },
    { 0x4783, 0x9CEF, 31 },
    { 0x30D4, 0x9CED, 31 },
    { 0x3EF1, 0x9CDC, 32 },
    { 0x3EF6, 0x9CDD, 32 },
    { 0x3EF7, 0x9CDE, 32 },
    { 0x30D4, 0x9CDB, 32 },
    { 0x2FDA, 0x9CD9, 32 },
    { 0x2FDB, 0x9CDA, 32 },
    { 0x477C, 0x9CE3, 33 },
    { 0x4783, 0x9CE4, 33 },
    { 0x30D4, 0x9CE2, 33 },
    { 0x49A2, 0x9C82, 34 },
    { 0x49A3, 0x9C83, 34 },
    { 0x49AA, 0x9C80, 34 },
    { 0x49AB, 0x9C81, 34 },
    { 0x49AE, 0x9C84, 34 },
    { 0x0, 0x0, 0 }
};

static int SeChange2Dto3D(int se) {
    int i;

    if (se >= 40000) {
        return se;
    }
    for (i = 0; change_list[i].sd_se; i++) {
        if (se_3d_load_data == change_list[i].file && se == change_list[i].sd_se) {
            return change_list[i].sd_3d;
        }
    }
    return 0;
}

static int SeChange3Dto2D(int se) {
    int i;

    if (se < 40000) {
        return se;
    }
    for (i = 0; change_list[i].sd_3d; i++) {
        if (se == change_list[i].sd_3d) {
            return change_list[i].sd_se;
        }
    }
    return 0;
}
