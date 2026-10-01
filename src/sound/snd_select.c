/*
 * snd_select.c: picks the ambient sound channels for the player's position. Each
 * sound area (struct _SOUND_DATA) has, per page, 7 channel settings packed 5 bits
 * each into chanstat[]: bit 0 on/off, bits 1-2 inner volume - 1, bits 3-4 outer
 * volume - 1. Pages 6 and up are fixed channel sets (sndExData).
 */
#include "sh2.h"

struct _NEAR_SOUND_DATA sndExData[9] = {
    { { 2, 0, 0, 0, 0, 0, 0 }, { 1, 0, 0, 0, 0, 0, 0 }, 0, 0, 0.0f },
    { { 2, 2, 0, 0, 0, 0, 0 }, { 1, 1, 0, 0, 0, 0, 0 }, 0, 0, 0.0f },
    { { 2, 0, 2, 0, 0, 0, 0 }, { 1, 0, 1, 0, 0, 0, 0 }, 0, 0, 0.0f },
    { { 2, 2, 2, 0, 0, 0, 0 }, { 1, 1, 1, 0, 0, 0, 0 }, 0, 0, 0.0f },
    { { 2, 0, 0, 2, 0, 0, 0 }, { 1, 0, 0, 1, 0, 0, 0 }, 0, 0, 0.0f },
    { { 2, 2, 0, 2, 0, 0, 0 }, { 1, 1, 0, 1, 0, 0, 0 }, 0, 0, 0.0f },
    { { 2, 0, 2, 2, 0, 0, 0 }, { 1, 0, 1, 1, 0, 0, 0 }, 0, 0, 0.0f },
    { { 2, 2, 2, 2, 0, 0, 0 }, { 1, 1, 1, 1, 0, 0, 0 }, 0, 0, 0.0f },
    { { 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0.0f },
};

int snd_local_on_num[9] = { 1, 2, 2, 3, 2, 3, 3, 4, 0 };

/** Ambient sound volume update (empty in this build). */
void sndVolumeMain(void) {
}

/* Copy the 7 channel settings of `page` from sd_p into the next near sound. */
#define SND_SET_NEAR_SOUND(page, err)                                                                   \
    for (j = 0; j < 7; j++) {                                                                           \
        if (!((sd_p->chanstat[j] >> ((page) * 5)) & 1)) {                                                \
            w_p->near_sound_ary[w_p->near_sound_num].inVol[j] = 0;                                      \
            w_p->near_sound_ary[w_p->near_sound_num].outVol[j] = 0;                                     \
        } else {                                                                                        \
            w_p->near_sound_ary[w_p->near_sound_num].inVol[j] = ((sd_p->chanstat[j] >> ((page) * 5 + 1)) & 3) + 1; \
            w_p->near_sound_ary[w_p->near_sound_num].outVol[j] = ((sd_p->chanstat[j] >> ((page) * 5 + 3)) & 3) + 1; \
            w_p->on_chan[j] = 1;                                                                        \
        }                                                                                               \
    }                                                                                                   \
    w_p->near_sound_ary[w_p->near_sound_num].errCode = (err)

#define SND_SET_NEAR_SOUND_PAGE()                                                                       \
    if (sd_p->flags & (1 << w_p->page)) {                                                               \
        SND_SET_NEAR_SOUND(w_p->page, 0);                                                               \
    } else {                                                                                            \
        SND_SET_NEAR_SOUND(0, 1);                                                                       \
    }

/** Collects the channel settings of the sound areas around the character (up to 8), or of the
 * default area when none is near, and counts the channels that are on.
 * @param w_p the sound work: character position and page in, near sounds and channels out */
void sndGetSoundAryByCharaPos(struct _SOUND_WORK *w_p) {
    int i;
    int j;
    struct _SOUND_DATA *sd_p;
    float get_min_x;
    float get_max_x;
    float get_min_z;
    float get_max_z;

    if (w_p->page >= 6) {
        w_p->near_sound_num = 1;
        w_p->near_sound_ary[0] = sndExData[w_p->page - 6];
        w_p->on_num = snd_local_on_num[w_p->page - 6];
        for (i = 0; i < 7; i++) {
            w_p->on_chan[i] = sndExData[w_p->page - 6].inVol[i];
        }
        return;
    }
    for (i = 0; i < 7; i++) {
        w_p->on_chan[i] = 0;
    }
    sd_p = w_p->sound_ary;
    w_p->near_sound_num = 0;
    for (sd_p++; !(sd_p->flags & 0x80000000); sd_p++) {
        get_min_x = w_p->chara_pos[0] - w_p->half_w;
        get_max_x = w_p->chara_pos[0] + w_p->half_w;
        get_min_z = w_p->chara_pos[2] - w_p->half_w;
        get_max_z = w_p->chara_pos[2] + w_p->half_w;
        if (sd_p->lim_sw.x0 <= get_max_x && sd_p->lim_sw.x2 >= get_min_x && sd_p->lim_sw.z2 <= get_max_z &&
            sd_p->lim_sw.z0 >= get_min_z && sd_p->lim_sw.min_hy >= w_p->chara_pos[1] &&
            sd_p->lim_sw.max_hy <= w_p->chara_pos[1]) {
            SND_SET_NEAR_SOUND_PAGE();
            w_p->near_sound_num++;
            /* Matching: the assert bakes its original line number into the object. */
#line 448
            assert_dw(w_p->near_sound_num <= 8);
        }
    }
    if (w_p->near_sound_num == 0) {
        sd_p = w_p->sound_ary;
        SND_SET_NEAR_SOUND_PAGE();
        w_p->near_sound_num = 1;
    }
    w_p->on_num = 0;
    for (i = 0; i < 7; i++) {
        if (w_p->on_chan[i] == 1) {
            w_p->on_num++;
        }
    }
}
