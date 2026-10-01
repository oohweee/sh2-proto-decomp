/*
 * James as the player character: his update function, input (pad, sticks, key config), status
 * and animation selection for both control modes, battle reactions (damage, holds, deaths),
 * targeting, neck/chest aiming, flashlight position, and weapon/foot attack points.
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "fi_libvu0_inline.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"
#include "lib/libShPad.h"

#define BTL_ID_ENEMY_START 36
#define BTL_ID_ENEMY_END 66

/* Wraps an angle into [-PI, PI]. Matching: a macro, so the argument is evaluated again in each arm. */
#define ANGLE_WRAP(a) (((a) > 3.1415927f) ? (a) - 6.2831855f : (((a) < -3.1415927f) ? 6.2831855f + (a) : (a)))

static const struct _AnimeInfo pjames_anim[34] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 101, 11, 256, 1, 10, 1, 0 },
    { 102, 20, 1280, 14, 28, 1, 0 },
    { 103, 20, 1280, 34, 48, 1, 0 },
    { 104, 24, 1408, 51, 74, 1, 0 },
    { 105, 24, 1536, 75, 98, 1, 0 },
    { 106, 24, 1536, 99, 122, 1, 0 },
    { 107, 24, 1792, 123, 146, 1, 0 },
    { 108, 24, 2048, 147, 170, 1, 0 },
    { 109, 20, 2048, 171, 190, 0, 0 },
    { 110, 20, 2048, 191, 210, 0, 0 },
    { 111, 35, 1024, 221, 245, 0, 0 },
    { 112, 35, 1024, 256, 280, 0, 0 },
    { 113, 20, 1024, 281, 300, 1, 0 },
    { 114, 20, 1024, 301, 320, 1, 0 },
    { 115, 30, 384, 321, 350, 0, 0 },
    { 116, 30, 384, 351, 380, 0, 0 },
    { 117, 30, 384, 381, 410, 0, 0 },
    { 118, 30, 384, 411, 440, 0, 0 },
    { 119, 10, 1536, 445, 450, 0, 0 },
    { 120, 10, -1536, 445, 450, 0, 0 },
    { 121, 20, 1024, 453, 470, 0, 0 },
    { 122, 15, 1024, 472, 481, 0, 0 },
    { 123, 17, 1536, 486, 502, 0, 0 },
    { 124, 24, 1536, 516, 539, 1, 0 },
    { 125, 24, 1536, 540, 563, 1, 0 },
    { 126, 24, 2048, 564, 587, 1, 0 },
    { 127, 24, 2048, 588, 611, 1, 0 },
    { 128, 20, 1024, 612, 629, 0, 0 },
    { 129, 20, 1024, 632, 649, 0, 0 },
    { 130, 20, 1024, 652, 670, 0, 0 },
    { 131, 10, 2048, 672, 681, 0, 0 },
    { 132, 20, 768, 682, 701, 0, 0 },
    { 133, 20, 512, 702, 719, 0, 0 },
};

static const struct _AnimeInfo pjames_hg_anim[11] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 201, 20, 3584, 724, 741, 0, 0 },
    { 202, 20, -2560, 724, 741, 0, 0 },
    { 203, 15, 3584, 744, 756, 0, 0 },
    { 204, 15, -2560, 744, 756, 0, 0 },
    { 205, 15, 2048, 757, 771, 0, 0 },
    { 206, 15, -2048, 757, 771, 0, 0 },
    { 207, 15, 1792, 772, 786, 0, 0 },
    { 208, 30, 1792, 757, 786, 0, 0 },
    { 209, 45, 1792, 792, 831, 0, 0 },
    { 210, 45, 1792, 787, 831, 0, 0 },
};

static const struct _AnimeInfo pjames_sg_anim[17] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 251, 15, 2048, 722, 736, 0, 0 },
    { 252, 15, -1792, 722, 736, 0, 0 },
    { 253, 50, 1792, 736, 786, 0, 0 },
    { 254, 15, 2048, 787, 801, 0, 0 },
    { 255, 15, -2048, 787, 801, 0, 0 },
    { 256, 50, 1792, 801, 851, 0, 0 },
    { 257, 15, 2048, 852, 866, 0, 0 },
    { 258, 15, -2048, 852, 866, 0, 0 },
    { 259, 50, 1792, 866, 916, 0, 0 },
    { 260, 40, 1536, 917, 956, 0, 0 },
    { 261, 10, 2048, 957, 966, 0, 0 },
    { 262, 10, -2048, 957, 966, 0, 0 },
    { 263, 10, 2048, 967, 976, 0, 0 },
    { 264, 10, -2048, 967, 976, 0, 0 },
    { 265, 10, 2048, 977, 986, 0, 0 },
    { 266, 10, -2048, 977, 986, 0, 0 },
};

static const struct _AnimeInfo pjames_rg_anim[5] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 301, 20, 2048, 722, 741, 0, 0 },
    { 302, 20, -1792, 722, 741, 0, 0 },
    { 303, 31, 1280, 741, 771, 0, 0 },
    { 304, 40, 1536, 772, 811, 0, 0 },
};

static const struct _AnimeInfo pjames_sp_anim[17] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 501, 10, 1280, 722, 731, 0, 0 },
    { 502, 10, -1280, 722, 731, 0, 0 },
    { 503, 10, 1280, 727, 731, 0, 0 },
    { 504, 10, -1280, 727, 731, 0, 0 },
    { 505, 10, 256, 732, 741, 1, 0 },
    { 506, 6, 1024, 742, 747, 0, 0 },
    { 507, 6, -1024, 742, 747, 0, 0 },
    { 508, 26, 1536, 742, 767, 0, 0 },
    { 509, 20, 1536, 748, 767, 0, 0 },
    { 510, 32, 1536, 768, 799, 0, 0 },
    { 511, 26, 1536, 774, 799, 0, 0 },
    { 512, 15, 2048, 794, 808, 0, 0 },
    { 513, 15, -2048, 794, 808, 0, 0 },
    { 514, 16, 2048, 809, 824, 1, 0 },
    { 515, 26, 2048, 825, 850, 0, 0 },
    { 516, 10, 256, 851, 860, 1, 0 },
};

static const struct _AnimeInfo pjames_ka_anim[33] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 351, 12, 2048, 722, 733, 0, 0 },
    { 352, 12, -1794, 722, 733, 0, 0 },
    { 353, 12, 2048, 734, 745, 0, 0 },
    { 354, 12, -1794, 734, 745, 0, 0 },
    { 355, 12, 2048, 727, 733, 0, 0 },
    { 356, 12, -1794, 727, 733, 0, 0 },
    { 357, 12, 2048, 739, 745, 0, 0 },
    { 358, 12, -1794, 739, 745, 0, 0 },
    { 359, 10, 320, 746, 755, 1, 0 },
    { 360, 10, 320, 756, 765, 1, 0 },
    { 361, 10, 1536, 766, 775, 0, 0 },
    { 362, 10, -1536, 766, 775, 0, 0 },
    { 363, 22, 1536, 776, 797, 0, 0 },
    { 364, 22, -1024, 776, 797, 0, 0 },
    { 365, 18, 1536, 780, 797, 0, 0 },
    { 366, 18, -1024, 781, 797, 0, 0 },
    { 367, 12, 1024, 800, 809, 0, 0 },
    { 368, 12, -1024, 800, 809, 0, 0 },
    { 369, 22, 1536, 810, 831, 0, 0 },
    { 370, 22, -1024, 810, 831, 0, 0 },
    { 371, 18, 1536, 814, 831, 0, 0 },
    { 372, 18, -1024, 815, 831, 0, 0 },
    { 373, 12, 1536, 834, 843, 0, 0 },
    { 374, 12, -1024, 834, 843, 0, 0 },
    { 375, 30, 1024, 846, 873, 0, 0 },
    { 376, 30, -1024, 846, 873, 0, 0 },
    { 377, 30, 1024, 846, 873, 0, 0 },
    { 378, 30, -1024, 846, 873, 0, 0 },
    { 379, 30, 1024, 876, 903, 0, 0 },
    { 380, 30, -1024, 876, 903, 0, 0 },
    { 381, 30, 1024, 876, 903, 0, 0 },
    { 382, 30, -1024, 876, 903, 0, 0 },
};

static const struct _AnimeInfo pjames_pi_anim[33] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 401, 10, 2048, 722, 731, 0, 0 },
    { 402, 10, -2048, 722, 731, 0, 0 },
    { 403, 10, 2048, 732, 741, 0, 0 },
    { 404, 10, -2048, 732, 741, 0, 0 },
    { 405, 10, 256, 742, 751, 1, 0 },
    { 406, 10, 256, 752, 761, 1, 0 },
    { 407, 10, 2048, 762, 771, 0, 0 },
    { 408, 10, -2048, 762, 771, 0, 0 },
    { 409, 24, 1536, 772, 795, 0, 0 },
    { 410, 24, -768, 772, 795, 0, 0 },
    { 411, 18, 1536, 778, 795, 0, 0 },
    { 412, 18, -768, 778, 795, 0, 0 },
    { 413, 11, 1536, 796, 806, 0, 0 },
    { 414, 11, -1024, 796, 806, 0, 0 },
    { 415, 24, 1536, 807, 830, 0, 0 },
    { 416, 24, -768, 807, 830, 0, 0 },
    { 417, 18, 1536, 813, 830, 0, 0 },
    { 418, 18, -768, 813, 830, 0, 0 },
    { 419, 11, 1536, 831, 841, 0, 0 },
    { 420, 11, -1024, 831, 841, 0, 0 },
    { 421, 35, 1792, 842, 876, 0, 0 },
    { 422, 35, -1536, 842, 876, 0, 0 },
    { 423, 35, 1792, 842, 876, 0, 0 },
    { 424, 35, -1536, 842, 876, 0, 0 },
    { 425, 35, 1792, 877, 911, 0, 0 },
    { 426, 35, -1536, 877, 911, 0, 0 },
    { 427, 35, 1792, 877, 911, 0, 0 },
    { 428, 35, -1536, 877, 911, 0, 0 },
    { 429, 30, 1024, 912, 941, 0, 0 },
    { 430, 30, -896, 912, 941, 0, 0 },
    { 431, 30, 1024, 942, 971, 0, 0 },
    { 432, 30, -896, 942, 971, 0, 0 },
};

static const struct _AnimeInfo pjames_na_anim[17] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 451, 20, 1024, 722, 741, 0, 0 },
    { 452, 20, -1024, 722, 741, 0, 0 },
    { 453, 12, 1024, 742, 753, 0, 0 },
    { 454, 12, -1024, 742, 753, 0, 0 },
    { 455, 20, 1024, 754, 773, 0, 0 },
    { 456, 20, -1024, 754, 773, 0, 0 },
    { 457, 10, 256, 774, 783, 1, 0 },
    { 458, 10, 256, 784, 793, 1, 0 },
    { 459, 35, 768, 794, 828, 0, 0 },
    { 460, 35, -768, 794, 828, 0, 0 },
    { 461, 35, 768, 829, 863, 0, 0 },
    { 462, 35, -768, 829, 863, 0, 0 },
    { 463, 40, 768, 864, 903, 0, 0 },
    { 464, 40, -768, 864, 903, 0, 0 },
    { 465, 40, 768, 904, 943, 0, 0 },
    { 466, 40, -768, 904, 943, 0, 0 },
};

static const struct _AnimeInfo pjames_cs_anim[24] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 551, 25, 1280, 728, 746, 0, 0 },
    { 552, 25, -2048, 728, 746, 0, 0 },
    { 553, 15, 2048, 747, 761, 0, 0 },
    { 554, 15, -2048, 747, 758, 0, 0 },
    { 555, 25, 1280, 762, 786, 0, 0 },
    { 556, 25, -2048, 762, 786, 0, 0 },
    { 557, 15, 2048, 787, 801, 0, 0 },
    { 558, 15, -2048, 787, 798, 0, 0 },
    { 559, 10, 2048, 802, 811, 1, 0 },
    { 560, 10, 2048, 812, 821, 1, 0 },
    { 561, 1, 2048, 802, 802, 1, 0 },
    { 562, 1, 2048, 812, 812, 1, 0 },
    { 563, 20, 2048, 822, 841, 0, 0 },
    { 564, 20, -1536, 822, 841, 0, 0 },
    { 565, 50, 1024, 842, 891, 0, 0 },
    { 566, 50, -1536, 842, 891, 0, 0 },
    { 567, 50, 1024, 892, 941, 0, 0 },
    { 568, 50, -1536, 892, 941, 0, 0 },
    { 569, 25, 1024, 942, 966, 0, 0 },
    { 570, 25, -1536, 942, 966, 0, 0 },
    { 571, 25, 1024, 967, 991, 0, 0 },
    { 572, 25, -1536, 967, 991, 0, 0 },
    { 573, 30, 768, 992, 1021, 0, 0 },
};

static const struct _AnimeInfo pjames_demo_anim[30] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { 951, 11, 256, 1, 10, 1, 0 },
    { 952, 401, 2048, 0, 400, 0, 0 },
    { 953, 240, 2048, 0, 239, 0, 0 },
    { 954, 74, 2048, 0, 73, 0, 0 },
    { 955, 74, 2048, 0, 73, 0, 0 },
    { 956, 236, 2048, 0, 235, 0, 0 },
    { 957, 300, 2048, 0, 299, 0, 0 },
    { 958, 311, 2048, 0, 310, 0, 0 },
    { 959, 532, 2048, 0, 531, 0, 0 },
    { 960, 261, 2048, 0, 260, 0, 0 },
    { 961, 222, 2048, 0, 221, 0, 0 },
    { 962, 250, 2048, 0, 249, 0, 0 },
    { 963, 316, 2048, 0, 315, 0, 0 },
    { 964, 62, 2048, 0, 61, 0, 0 },
    { 965, 101, 2048, 0, 100, 0, 0 },
    { 966, 263, 2048, 0, 262, 0, 0 },
    { 967, 234, 2048, 0, 233, 0, 0 },
    { 968, 135, 2048, 0, 134, 0, 0 },
    { 969, 135, 2048, 0, 134, 0, 0 },
    { 970, 156, 2048, 0, 155, 0, 0 },
    { 971, 920, 2048, 0, 919, 0, 0 },
    { 972, 288, 2048, 0, 287, 0, 0 },
    { 973, 335, 2048, 0, 334, 0, 0 },
    { 974, 311, 2048, 0, 310, 0, 0 },
    { 975, 1332, 2048, 0, 1331, 0, 0 },
    { 976, 448, 2048, 0, 447, 0, 0 },
    { 977, 490, 2048, 0, 489, 0, 0 },
    { 978, 191, 2048, 0, 191, 0, 0 },
    { 979, 186, 2048, 0, 185, 0, 0 },
};

static const unsigned int pjames_act_with_wep_flag[9] = {
    0xFF, 0x55, 0x5, 0x0, 0xFF, 0xFF, 0x55, 0x5,
    0x0,
};

static const unsigned int pjames_upper_flag[32] = {
    0x41D9FC6, 0x41D9FD0, 0x41D9FD0, 0x41D9FD0, 0x41D9FD0, 0x41D9FC0, 0x41D9FC1, 0x41D9FC1,
    0x1C0101, 0xDBE01, 0xC9601, 0xD1A01, 0x19E01, 0x1AE01, 0x1CE01, 0x9601,
    0x11A01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x4000000, 0x81DFFD0, 0x10000000, 0x0, 0x41D9FC0, 0x0,
};

static const unsigned int pjames_lower_flag[32] = {
    0x41D9FC6, 0x41D9FD0, 0x41D9FD0, 0x41D9FD0, 0x41D9FD0, 0x41D9FC0, 0xC1D9FC1, 0xC1D9FC1,
    0x1C0101, 0xDBE01, 0xC9601, 0xD1A01, 0x19E01, 0x1AE01, 0x1CE01, 0x9601,
    0x11A01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xC019F00, 0x1D9FC0, 0x10019F00, 0x0, 0x41D9FC0, 0x80000000,
};

struct _AnimeInfo *jms_stage_anim;
struct shPlayerWork sh2jms;

/** Sets @p status bits in @p type. */
void player_flg_on(unsigned int *type, unsigned int status) {
    *type |= status;
}

/** Clears @p status bits in @p type. */
void player_flg_off(unsigned int *type, unsigned int status) {
    *type &= ~status;
}

/** Returns James's upper-body state flags masked by @p status. */
int upper_flg_on(unsigned int status) {
    return sh2jms.upper_st_flg & status;
}

/** Returns James's lower-body state flags masked by @p status. */
int lower_flg_on(unsigned int status) {
    return sh2jms.lower_st_flg & status;
}

/** Returns James's upper-body animation flags masked by @p status. */
int u_anime_flg_on(unsigned int status) {
    return sh2jms.u_anime_st_flg & status;
}

/** Returns James's lower-body animation flags masked by @p status. */
int l_anime_flg_on(unsigned int status) {
    return sh2jms.l_anime_st_flg & status;
}

/** Returns the current weapon's action flags masked by @p status. */
int actwithwep_flg_on(unsigned int status) {
    return sh2jms.act_with_wep & status;
}

/** Moves James's lower body to state @p status, remembering the previous one; resets the idle time. */
void lower_st_set(int status, struct shPlayerWork *w) {
    w->lower_prev = w->lower_now;
    w->lower_now = status;
    sh2jms.non_input = 0.0f;
}

/** Moves James's upper body to state @p status, remembering the previous one. */
void upper_st_set(int status, struct shPlayerWork *w) {
    w->upper_prev = w->upper_now;
    w->upper_now = status;
}

/** Loads the lower-body flags of state @p status. */
void lower_flg_set(int status, struct shPlayerWork *w) {
    w->lower_st_flg = pjames_lower_flag[status];
}

/** Loads the upper-body flags of state @p status. */
void upper_flg_set(int status, struct shPlayerWork *w) {
    w->upper_st_flg = pjames_upper_flag[status];
}

/** Loads the action flags of weapon class @p status (some are off in map mode). */
void actwithwep_flg_set(unsigned char status, struct shPlayerWork *w) {
    w->act_with_wep = pjames_act_with_wep_flag[status];
    if (status == 4 || status == 5) {
        if (sh2jms.map_mode) {
            player_flg_off(&w->act_with_wep, 2);
            player_flg_off(&w->act_with_wep, 8);
            player_flg_off(&w->act_with_wep, 0x20);
            player_flg_off(&w->act_with_wep, 0x80);
        }
    }
}

/** Resets James's control state (animation type, hold, flags) for his current weapon. */
void PlayerCheckInit(struct SubCharacter *this) {
    SCAnimeTypeSwitch(this, 1);
    this->battle.status |= 0x400;
    this->work[0] = sh2jms.weapon;
    sh2jms.hold_type = -1;
    sh2jms.lock_on = 0;
    actwithwep_flg_set(0, &sh2jms);
    sh2jms.now_cam_no = 0;
    sh2jms.cam_chg_flg = 0;
    sh2jms.allbody_now = 0;
    sh2jms.upper_now = 0xFF;
    sh2jms.lower_now = 0xFF;
    sh2jms.allbody_prev = 0xFF;
    sh2jms.upper_prev = 0xFF;
    sh2jms.lower_prev = 0xFF;
    sh2jms.event_status_now = 0xFF;
    sh2jms.event_status_prev = 0xFF;
    sh2jms.lower_st_flg = 0;
    sh2jms.upper_st_flg = 0;
    player_flg_on(&sh2jms.lower_st_flg, 1);
    player_flg_on(&sh2jms.upper_st_flg, 1);
    shQzero(sh2jms.pad, 0x20);
}

static signed char PlayerCheckKeyInputRoundWay(void) {
    struct PAD_INFO *pad;

    pad = sh2jms.pad;
    if (pad->lround || pad->lstickX < 0) {
        return -1;
    }
    if (pad->rround || pad->lstickX > 0) {
        return 1;
    }
    return 0;
}

static unsigned char PlayerCheckKeyInputL180(void) {
    int i;
    int tmp;
    struct PAD_INFO *pad;

    pad = sh2jms.pad;
    if (pad[0].pad3d.lslide) {
        if (!pad[1].pad3d.lslide) {
            return 2;
        }
        for (i = 2; i < 2; i++) {
            if (!(tmp = pad[i].pad3d.lslide)) {
                return 2;
            }
        }
        return 1;
    }
    return 0;
}

static unsigned char PlayerCheckKeyInputR180(void) {
    int i;
    int tmp;
    struct PAD_INFO *pad;

    pad = sh2jms.pad;
    if (pad[0].pad3d.rslide) {
        if (!pad[1].pad3d.rslide) {
            return 2;
        }
        for (i = 2; i < 2; i++) {
            if (!(tmp = pad[i].pad3d.rslide)) {
                return 2;
            }
        }
        return 1;
    }
    return 0;
}

static unsigned char PlayerCheckKeyInputPrsAttack(void) {
    int i;
    unsigned char result;
    unsigned char value;
    struct PAD_INFO *pad;

    value = 0;
    pad = sh2jms.pad;
    for (i = 0; i < 10; i++) {
        if (!pad[i].attack0) {
            return 0;
        }
        value += pad[i].attack0;
    }
    result = (value * 2 + 10) / 20;
    return result;
}

static unsigned char PlayerCheckKeyInputTrgAttack(void) {
    struct PAD_INFO *pad;

    pad = sh2jms.pad;
    if (pad[0].attack1) {
        return 0;
    }
    if (pad[0].attack0) {
        if (!pad[1].attack0) {
            return pad[0].attack0;
        }
    }
    return 0;
}

static unsigned char PlayerCheckKeyInputTrgLight(void) {
    struct PAD_INFO *pad;

    pad = sh2jms.pad;
    if (pad[0].light_ && !pad[1].light_) {
        return pad[0].light_;
    }
    return 0;
}

static float PlayerCheckKeyInputStickDir(void) {
    struct PAD_INFO *pad;
    float x;
    float y;

    pad = sh2jms.pad;
    if (sh2jms.ctrl_unit == 1) {
        y = pad->lstickY;
        x = pad->lstickX;
    } else {
        if (pad->forward) {
            y = -127.0f;
        }
        if (pad->backward) {
            y = 127.0f;
        }
        if (pad->lround) {
            x = -127.0f;
        }
        if (pad->rround) {
            x = 127.0f;
        }
    }
    return shAtan2(-y, x);
}

static float PlayerCheckKeyStickClamp(float stick_val, float min, float max) {
    stick_val = (stick_val < min) ? min : ((stick_val > max) ? max : stick_val);
    return (stick_val - min) / (max - min);
}

static float PlayerCheckKeyInputStickPow(void) {
    float x;
    float y;
    float p;
    struct PAD_INFO *pad;

    pad = sh2jms.pad;
    switch (sh2jms.ctrl_unit) {
    case 1:
        x = sh2jms.lstick_x;
        y = sh2jms.lstick_y;
        break;
    case 0:
        x = y = 0.0f;
        if (pad->forward) {
            y = -1.0f;
        }
        if (pad->backward) {
            y = 1.0f;
        }
        if (pad->lround) {
            x = -1.0f;
        }
        if (pad->rround) {
            x = 1.0f;
        }
        x *= shSinF(pad->pad2d.dir);
        y = -y * shCosF(pad->pad2d.dir);
        break;
    default:
        x = y = 0.0f;
        break;
    }
    p = _shSqrt(x * x + y * y);
    return (p < 0.0f) ? 0.0f : ((p > 1.0f) ? 1.0f : p);
}

static unsigned char PlayerCheckKeyInputHold(unsigned char hold_prev, unsigned char hold) {
    if (!playing.weapon_control) {
        return hold;
    }
    if (shPadTrigger(0, key_config.ready)) {
        hold_prev = !hold_prev;
    }
    return hold_prev;
}

static unsigned char PlayerCheckKeyInputDash(unsigned char dash) {
    if (playing.walk_run_control == 0) {
        return dash;
    } else {
        return !dash;
    }
}

static void PlayerCheckKeyInput(void) {
    signed char pad_local_org[4];
    unsigned char pad_local[32];
    signed char asobi_x;
    signed char asobi_y;
    int i;
    struct PAD_INFO *pad;
    union shGameKeyData key;

    pad = sh2jms.pad;
    for (i = 9; i > 0; i--) {
        sh2jms.pad[i] = sh2jms.pad[i - 1];
    }
    libShPadRead(0, 0, (char *)pad_local);
    shSysKeyNormalize((char *)pad_local);
    for (i = 0; i < 4; i++) {
        pad_local_org[i] = pad_local[4 + i] - 0x80;
    }
    shSysKeyAdjust((char *)pad_local);
    shGameKeyConvert(&key, (char *)pad_local);
    pad->skip = pad_local[22];
    pad->pause = pad_local[22];
    pad->action = shPadTrigger(0, key_config.action);
    pad->attack0 = key.f.ACTION;
    if (sh2jms.weapon == 8) {
        pad->dash = 0;
    } else {
        pad->dash = PlayerCheckKeyInputDash(key.f.DASH ? 1 : 0);
    }
    pad->hold = PlayerCheckKeyInputHold(sh2jms.pad[1].hold, key.f.READY);
    pad->menu = shPadTrigger(0, key_config.item);
    pad->search = key.f.VIEW;
    pad->map = shPadTrigger(0, key_config.map);
    pad->light_ = key.f.LIGHT;
    pad->light = PlayerCheckKeyInputTrgLight();
    pad->rstickY = pad_local_org[1];
    pad->rstickX = pad_local_org[0];
    pad->lstickY = pad_local_org[3];
    pad->lstickX = pad_local_org[2];
    if (iabs_gcc(pad->lstickY) <= 37) {
        pad->lstickY = 0;
    }
    if (iabs_gcc(pad->lstickX) <= 37) {
        pad->lstickX = 0;
    }
    if (iabs_gcc(pad->rstickY) <= 37) {
        pad->rstickY = 0;
    }
    if (iabs_gcc(pad->rstickX) <= 37) {
        pad->rstickX = 0;
    }
    if (!playing.control_type) {
        asobi_x = iabs_gcc(pad->lstickY) / 2;
        asobi_x = (asobi_x < 0) ? 0 : ((asobi_x > 64) ? 64 : asobi_x);
        asobi_y = iabs_gcc(pad->lstickX) / 2;
        asobi_y = (asobi_y < 0) ? 0 : ((asobi_y > 64) ? 64 : asobi_y);
        if (iabs_gcc(pad->lstickY) < asobi_y) {
            pad->lstickY = 0;
        }
        if (iabs_gcc(pad->lstickX) < asobi_x) {
            pad->lstickX = 0;
        }
    } else {
        asobi_x = asobi_y = 0;
    }
    if (pad->lstickX >= 0) {
        sh2jms.lstick_x = PlayerCheckKeyStickClamp(pad->lstickX, (asobi_x <= 18) ? 0.0f : asobi_x, 104.0f);
    } else {
        sh2jms.lstick_x = -PlayerCheckKeyStickClamp(-pad->lstickX, (asobi_x <= 18) ? 0.0f : asobi_x, 104.0f);
    }
    if (pad->lstickY >= 0) {
        sh2jms.lstick_y = PlayerCheckKeyStickClamp(pad->lstickY, 0.0f, 104.0f);
    } else {
        sh2jms.lstick_y = -PlayerCheckKeyStickClamp(-pad->lstickY, 0.0f, 104.0f);
    }
    if (pad->rstickX >= 0) {
        sh2jms.rstick_x = PlayerCheckKeyStickClamp(pad->rstickX, 38.0f, 104.0f);
    } else {
        sh2jms.rstick_x = -PlayerCheckKeyStickClamp((float)-pad->rstickX, 38.0f, 104.0f);
    }
    if (pad->rstickY >= 0) {
        sh2jms.rstick_y = PlayerCheckKeyStickClamp(pad->rstickY, 38.0f, 104.0f);
    } else {
        sh2jms.rstick_y = -PlayerCheckKeyStickClamp(-pad->rstickY, 38.0f, 104.0f);
    }
    pad->forward = pad_local[10];
    pad->backward = pad_local[11];
    pad->lround = pad_local[9];
    pad->rround = pad_local[8];
    if (!pad->forward && !pad->backward && !pad->lround && !pad->rround && (pad->lstickX || pad->lstickY)) {
        sh2jms.ctrl_unit = 1;
    } else {
        sh2jms.ctrl_unit = 0;
    }
    pad->attack1 = PlayerCheckKeyInputPrsAttack();
    pad->attack2 = PlayerCheckKeyInputTrgAttack();
    switch (playing.control_type) {
    case 0:
        pad->pad3d.lslide = key.f.LSLIDE;
        pad->pad3d.rslide = key.f.RSLIDE;
        pad->pad3d.lturn180 = PlayerCheckKeyInputL180();
        pad->pad3d.rturn180 = PlayerCheckKeyInputR180();
        pad->pad3d.round_way = PlayerCheckKeyInputRoundWay();
        break;
    case 1:
        pad->pad2d.dir = PlayerCheckKeyInputStickDir();
        sh2jms.lstick_p = PlayerCheckKeyInputStickPow();
        break;
    }
    if (sh2jms.player->status & 0x4000) {
        shQzero(pad, 0x20);
        if (!sh2jms.event_anime) {
            if (shPadTrigger(0, key_config.front_move) || shPadTrigger(0, key_config.back_move) ||
                shPadTrigger(0, key_config.right_move) || shPadTrigger(0, key_config.left_move) ||
                shPadTrigger(0, key_config.right_turn) || shPadTrigger(0, key_config.left_turn) ||
                shPadTrigger(0, key_config.ready) || key.f.AY || key.f.AX) {
                sh2jms.event_anime = 0;
                EventCancel();
            }
        }
    }
}

static unsigned short PlayerDamageMotionNo(struct SubCharacter *this) {
    unsigned short damage_no[30][2][2] = {
        { { 0x4E28, 0x4E29 }, { 0x4E2A, 0x4E2B } },
        { { 0x4E28, 0x4E29 }, { 0x4E2A, 0x4E2B } },
        { { 0x4E2C, 0x4E2D }, { 0x4E2E, 0x4E2F } },
        { { 0x4E2C, 0x4E2D }, { 0x4E2E, 0x4E2F } },
        { { 0x4E2C, 0x4E2D }, { 0x4E2E, 0x4E2F } },
        { { 0x4E30, 0x4E31 }, { 0x4E32, 0x4E33 } },
        { { 0x4E2C, 0x4E2D }, { 0x4E2E, 0x4E2F } },
        { { 0x4E30, 0x4E31 }, { 0x4E32, 0x4E33 } },
        { { 0x4E30, 0x4E31 }, { 0x4E32, 0x4E33 } },
        { { 0x4E34, 0x4E34 }, { 0x4E34, 0x4E34 } },
        { { 0x4E39, 0x4E3A }, { 0x4E3B, 0x4E3C } },
        { { 0x4E3D, 0x4E3D }, { 0x4E2A, 0x4E2B } },
        { { 0x4E3E, 0x4E3E }, { 0x4E3F, 0x4E3F } },
        { { 0x4E35, 0x4E36 }, { 0x4E37, 0x4E38 } },
        { { 0x4E35, 0x4E36 }, { 0x4E37, 0x4E38 } },
        { { 0x4E45, 0x4E45 }, { 0x4E27, 0x4E27 } },
        { { 0x4E41, 0x4E42 }, { 0x4E43, 0x4E44 } },
        { { 0x4E39, 0x4E3A }, { 0x4E3B, 0x4E3C } },
        { { 0x4E46, 0x4E46 }, { 0x4E2A, 0x4E2B } },
        { { 0x4E47, 0x4E47 }, { 0x4E48, 0x4E48 } },
        { { 0x4E41, 0x4E42 }, { 0x4E43, 0x4E44 } },
        { { 0x4E4A, 0x4E4E }, { 0x4E27, 0x4E27 } },
        { { 0x4E4B, 0x4E4F }, { 0x4E4D, 0x4E51 } },
        { { 0x4E58, 0x4E58 }, { 0x4E27, 0x4E27 } },
        { { 0x4E56, 0x4E56 }, { 0x4E57, 0x4E57 } },
        { { 0x4E28, 0x4E29 }, { 0x4E2A, 0x4E2B } },
        { { 0x4E52, 0x4E52 }, { 0x4E27, 0x4E27 } },
        { { 0x4E54, 0x4E54 }, { 0x4E27, 0x4E27 } },
        { { 0x4E45, 0x4E45 }, { 0x4E27, 0x4E27 } },
        { { 0x4E45, 0x4E45 }, { 0x4E27, 0x4E27 } },
    };
    float direction[4];
    float roty;
    float roty2;
    int kind;
    int dead;

    kind = this->battle.id - BTL_ID_ENEMY_START;
    dead = sh2jms.dead;
    if (sh2jms.player->battle.target && sh2jms.player->battle.target->kind == 0x204) {
        roty = sh2jms.player->battle.target->rot.y;
    } else {
        _shNormalize(direction, this->battle.vec);
        roty = shAtan2(direction[2], direction[0]);
    }
    roty2 = roty - this->rot.y;
    roty2 = ANGLE_WRAP(roty2);
    if (roty2 >= -1.5707964f && roty2 < 1.5707964f) {
        sh2jms.hug_dir = 1;
    } else {
        sh2jms.hug_dir = 0;
    }
    /* Matching: #line puts the asserts below on their original source lines. */
#line 1082
    assert_dw(kind >= 0 && kind < BTL_ID_ENEMY_END - BTL_ID_ENEMY_START); /* Matching: do/while(0) form (its nop) */
    return damage_no[kind][dead][sh2jms.hug_dir];
}

static void PlayerCheckSideLine(struct SubCharacter *this) {
    float sp[4];
    float ep[4];

    sp[0] = this->pos.x;
    sp[2] = this->pos.z;
    sp[1] = ep[1] = -750.0 + this->pos.y;
    ep[0] = sp[0] + 600.0f * shSinF(1.5707964f + this->rot.y);
    ep[2] = sp[2] + 600.0f * shCosF(1.5707964f + this->rot.y);
    clCheckHitEyes(&sh2jms.r_side, (unsigned int)this, sp, ep, 0);
    ep[0] = sp[0] + 600.0f * shSinF(this->rot.y - 1.5707964f);
    ep[2] = sp[2] + 600.0f * shCosF(this->rot.y - 1.5707964f);
    clCheckHitEyes(&sh2jms.l_side, (unsigned int)this, sp, ep, 0);
}

static void PlayerCheckFootLine(struct SubCharacter *this) {
    float mat[4][4];
    float sp[4];
    float ep[4];

    GetPlayerPartsWorldMatrix(mat, 14);
    vcopy(mat[3], sp);
    vcopy(sp, ep);
    sp[1] -= 250.0f;
    ep[1] += 750.0f;
    clCheckHitEyesOnlyFloor(&sh2jms.r_foot, this, sp, ep);
    if (sh2jms.r_foot.kind == 1) {
        _shNormalize(sh2jms.r_foot.hobj.wall.nl, sh2jms.r_foot.hobj.wall.nl);
    }
    GetPlayerPartsWorldMatrix(mat, 13);
    vcopy(mat[3], sp);
    vcopy(sp, ep);
    sp[1] -= 250.0f;
    ep[1] += 750.0f;
    clCheckHitEyesOnlyFloor(&sh2jms.l_foot, this, sp, ep);
    if (sh2jms.l_foot.kind == 1) {
        _shNormalize(sh2jms.l_foot.hobj.wall.nl, sh2jms.l_foot.hobj.wall.nl);
    }
}

/** Does nothing. */
void PlayerSetHeightDummy(void) {
}

/** Sets James's ground height and normal from the floor below @p this. */
void PlayerSetHeight(struct SubCharacter *this) {
    float rot_tmp;
    float sp[4];
    float ep[4];

    vcopy(&this->pos, sp);
    vcopy(sp, ep);
    sp[1] -= 250.0f;
    ep[1] += 1500.0f;
    clCheckHitEyesOnlyFloor(&sh2jms.ft_floor, this, sp, ep);
    if (sh2jms.ft_floor.kind == 1) {
        this->grnd_height = sh2jms.ft_floor.hobj.wall.cp[1];
        *(struct FVEC *)this->grnd_normal = *(struct FVEC *)sh2jms.ft_floor.hobj.wall.nl;
    }
    sh2jms.dist_pos.y = this->grnd_height;
    SCFreefallSwitch(this, 0);
    if (sh2jms.dist_pos.y <= this->pos.y) {
        this->spd_y = 0.0f;
        this->pos.y = sh2jms.dist_pos.y;
    } else {
        rot_tmp = 9.8f * shGetDT();
        this->spd_y += rot_tmp * shGetDT();
        if (sh2jms.dist_pos.y > 100.0f + this->pos.y) {
            SCFreefallSwitch(this, 1);
            if (sh2jms.dist_pos.y > 250.0f + this->pos.y) {
                if (sh2jms.lower_now != JMS_ST_L_FALL) {
                    player_flg_on(&sh2jms.lower_st_flg, 0x1000000);
                    if (fabsf(shAngleRegulate(this->rot.y - shAtan2(this->pos.z - this->b_pos.z, this->pos.x - this->b_pos.x))) < 1.5707964f) {
                        sh2jms.fall_type = 0;
                    } else {
                        sh2jms.fall_type = 1;
                    }
                }
            }
        } else {
            this->spd_y = 0.0f;
            this->pos.y = sh2jms.dist_pos.y;
        }
    }
}

/** Sets James's ground height (while waiting after a room change). */
void PlayerSetHeightConnectWait(void) {
    PlayerSetHeight(sh2jms.player);
}

static void PlayerSetColumn_SetTarget(struct _CL_HITPOLY_COLUMN *mov, struct _CL_HITPOLY_COLUMN *atk, float *mov_z, float *atk_z) {
    int wep_local = sh2jms.weapon - 1;
    float mov_column_tbl[8][2][4] = {
        { { 0.35f, 0.05f, 0.35f - 0.05f, 0.05f }, { 0.55f, 0.23f, 0.45f, 0.23f } },
        { { 0.45f, 0.05f, 0.4f, 0.05f }, { 0.55f, 0.05f, 0.45f, 0.05f } },
        { { 0.65f, 0.2f, 0.5f, 0.2f }, { 0.65f, 0.2f, 0.5f, 0.2f } },
        { { 0.4f, 0.15f, 0.35f, 0.15f }, { 0.45f, 0.17f, 0.4f, 0.17f } },
        { { 0.4f, 0.2f, 0.35f, 0.2f }, { 0.45f, 0.2f, 0.4f, 0.2f } },
        { { 0.4f, 0.1f, 0.35f, 0.1f }, { 0.5f, 0.1f, 0.4f, 0.1f } },
        { { 0.45f, 0.12f, 0.4f, 0.12f }, { 0.45f, 0.12f, 0.4f, 0.12f } },
        { { 0.45f, 0.18f, 0.4f, 0.18f }, { 0.5f, 0.18f, 0.4f, 0.18f } },
    };

    mov->p[0][3] = atk->p[0][3] = 1.0f;
    mov->p[1][0] = atk->p[0][0] = 0.0f;
    mov->p[1][2] = atk->p[0][2] = 0.0f;
    mov->p[0][0] = atk->p[0][0] = sh2jms.player->pos.x;
    mov->p[0][1] = -50.0f + sh2jms.player->pos.y;
    atk->p[0][1] = sh2jms.player->pos.y;
    mov->p[0][2] = atk->p[0][2] = sh2jms.player->pos.z;
    mov->p[1][1] = -850.0f + sh2jms.player->pos.y;
    atk->p[1][1] = -900.0f + sh2jms.player->pos.y;
    mov->p[1][3] = 500.0f * 0.35f;
    atk->p[1][3] = 500.0f * (0.35f - 0.05f);
    *mov_z = 25.0f;
    *atk_z = 25.0f;
    switch (sh2jms.upper_now) {
    case JMS_ST_U_DAMAGE:
        if (sh2jms.dead) {
            mov->p[1][3] = 375.0f;
        }
        break;
    case JMS_ST_U_HOLD:
        mov->p[1][3] = 500.0f * mov_column_tbl[wep_local][0][0];
        *mov_z = 500.0f * mov_column_tbl[wep_local][0][1];
        atk->p[1][3] = 500.0f * mov_column_tbl[wep_local][0][2];
        *atk_z = 500.0f * mov_column_tbl[wep_local][0][3];
        if (sh2jms.weapon == 1) {
            if (sh2jms.hold_type == 1) {
                *mov_z = 115.0f;
                mov->p[1][3] = 275.0f;
            }
        }
        if (sh2jms.weapon == 8) {
            if (sh2jms.hold_type == 0) {
                *mov_z = 50.0f;
                *atk_z = 0.0f;
                mov->p[1][3] = 265.0f;
            } else {
                *mov_z = -50.0f;
                *atk_z = -75.0f;
                mov->p[1][3] = 265.0f;
            }
        }
        break;
    case JMS_ST_U_ATTACK:
        mov->p[1][3] = 500.0f * mov_column_tbl[wep_local][1][0];
        *mov_z = 500.0f * mov_column_tbl[wep_local][1][1];
        atk->p[1][3] = 500.0f * mov_column_tbl[wep_local][1][2];
        *atk_z = 500.0f * mov_column_tbl[wep_local][1][3];
        break;
    }
    if (sh2jms.map_mode == 2) {
        mov->p[1][3] = 500.0f * 0.35f;
        atk->p[1][3] = 500.0f * (0.35f - 0.05f);
    }
}

static void PlayerSetColumn_CloseToTarget(struct _CL_HITPOLY_COLUMN *mov, struct _CL_HITPOLY_COLUMN *atk, float *mov_z, float *atk_z) {
    sh2jms.column_mov.p[0][0] = sh2jms.column_atk.p[0][0] = sh2jms.player->pos.x;
    sh2jms.column_mov.p[0][1] = sh2jms.column_atk.p[0][1] = -50.0f + sh2jms.player->pos.y;
    sh2jms.column_mov.p[0][2] = sh2jms.column_atk.p[0][2] = sh2jms.player->pos.z;
    sh2jms.column_mov.p[0][3] = sh2jms.column_atk.p[0][3] = 1.0f;
    sh2jms.column_mov.p[1][0] = sh2jms.column_atk.p[1][0] = 0.0f;
    sh2jms.column_mov.p[1][1] = sh2jms.column_atk.p[1][1] = -850.0f + sh2jms.player->pos.y;
    sh2jms.column_mov.p[1][2] = sh2jms.column_atk.p[1][2] = 0.0f;
    close_to_value(&sh2jms.column_mov.p[1][3], mov->p[1][3], 5.0f);
    close_to_value(&sh2jms.column_atk.p[1][3], atk->p[1][3], 5.0f);
    close_to_value(&sh2jms.col_mov_z_hosei, *mov_z, 5.0f);
    close_to_value(&sh2jms.col_atk_z_hosei, *atk_z, 5.0f);
    sh2jms.column_mov.p[0][0] += sh2jms.col_mov_z_hosei * shSinF(sh2jms.player->rot.y);
    sh2jms.column_mov.p[0][2] += sh2jms.col_mov_z_hosei * shCosF(sh2jms.player->rot.y);
    sh2jms.column_atk.p[0][0] += sh2jms.col_atk_z_hosei * shSinF(sh2jms.player->rot.y);
    sh2jms.column_atk.p[0][2] += sh2jms.col_atk_z_hosei * shCosF(sh2jms.player->rot.y);
}

static void PlayerSetHitColumn(struct SubCharacter *this) {
    float mat[4][4];
    float mov_z;
    float atk_z;
    struct _CL_HITPOLY_COLUMN col_mov;
    struct _CL_HITPOLY_COLUMN col_atk;

    atk_z = mov_z = 0.0f;
    if (!sh2jms.hug_status || sh2jms.hug_status == 3) {
        PlayerSetHeight(this);
    }
    PlayerSetColumn_SetTarget(&col_mov, &col_atk, &mov_z, &atk_z);
    PlayerSetColumn_CloseToTarget(&col_mov, &col_atk, &mov_z, &atk_z);
    if (sh2jms.dead <= 1) {
        if (sh2jms.ft_floor.kind == 1) {
            clSetCharaHitColumn(&sh2jms.column_mov, &sh2jms.column_atk, this, PlayerSetHeightDummy);
        } else {
            clSetCharaHitColumn(&sh2jms.column_mov, &sh2jms.column_atk, this, NULL);
        }
    }
    this->center_y = 0.5f * (sh2jms.column_mov.p[0][1] + sh2jms.column_mov.p[1][1]);
    GetPlayerPartsWorldMatrix(mat, 20);
    this->eye_y = mat[3][1];
}

static void PlayerSetSearchArea(struct SubCharacter *this) {
    int env;
    float look_forward[3][5] = {
        { 7.0f, 7.0f, 6.0f, 4.5f, 4.0f },
        { 5.0f, 5.0f, 3.8f, 3.0f, 2.8f },
        { 3.0f, 3.0f, 1.8f, 1.5f, 1.3f },
    };
    float look_radius[3][5] = {
        { 7.0f, 7.0f, 6.0f, 4.5f, 4.0f },
        { 5.0f, 5.0f, 3.8f, 3.0f, 2.8f },
        { 3.0f, 3.0f, 1.8f, 1.5f, 1.3f },
    };
    float feel_forward[3][5] = {
        { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f },
        { 0.3f, 0.3f, 0.3f, 0.3f, 0.3f },
        { 0.1f, 0.1f, 0.1f, 0.1f, 0.1f },
    };
    float feel_radius[3][5] = {
        { 6.5f, 6.5f, 5.0f, 3.0f, 2.5f },
        { 4.5f, 4.5f, 3.8f, 3.3f, 2.8f },
        { 3.0f, 3.0f, 2.3f, 2.0f, 1.5f },
    };

    if (!sh2gfw_Get_NightOrDay()) {
        env = 0;
        SCLightOnNowSwitch(this, 1);
    } else {
        if (LightSpotOnOffCheck()) {
            env = 1;
            SCLightOnNowSwitch(this, env);
        } else {
            env = 2;
            SCLightOnNowSwitch(this, 0);
        }
    }
    shBattleSetLookArea(this, look_forward[env][playing.battle_level], look_radius[env][playing.battle_level]);
    shBattleSetFeelArea(this, feel_forward[env][playing.battle_level], feel_radius[env][playing.battle_level]);
}

/* Matching: fitted float-constant stand-in (constcount.py), not recovered code: 32 constants
 * that PlayerCheckNeckAngle's float-constant argument order needs (docs/stand-ins.md). It sits
 * in the gap before PlayerCheckDamage, where the original's line table leaves room for code (16
 * lines; the file's usual gap between functions is 11). Its body is still fitted. */
static float __stripped_float_code_neck(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f + 53.0f + 55.0f + 57.0f + 59.0f + 61.0f + 63.0f + 65.0f; }
static void PlayerCheckDamage(struct SubCharacter *this) {
    struct _AnimeInfo *a_info;
    int se;

    sh2jms.pos.x = 0.0f;
    sh2jms.pos.y = 0.0f;
    sh2jms.pos.z = 0.0f;
    sh2jms.rot.x = 0.0f;
    sh2jms.rot.y = 0.0f;
    sh2jms.rot.z = 0.0f;
    GameMoveDistanceCountUp(this, sh2jms.lower_now);
    this->rot_spd.y = shAngleRegulate(this->rot.y - this->b_rot.y) * shGetFPS();
    vcopy(&this->pos, &this->b_pos);
    vcopy(&this->rot, &this->b_rot);
    shBattleGetResult(this);
    if (this->battle.atk_result) {
        switch (playing.battle_level) {
        case 2:
        case 3:
            a_info = shCharacterAnimeGetInfo_(this, 2);
            switch (sh2jms.weapon) {
            case 5:
            case 6: {
                int hanekaeri = 0;

                switch (this->battle.atk_result) {
                case 1:
                    if (sh2jms.lower_now == JMS_ST_L_ATTACK && a_info->speed > 0 &&
                        !(sh2jms.player->battle.target->battle.status & 4)) {
                        hanekaeri = 1;
                    }
                    break;
                default:
                    if (playing.battle_level != 2) {
                        if (sh2jms.lower_now == JMS_ST_L_ATTACK && a_info->speed > 0) {
                            hanekaeri = 1;
                        }
                    }
                    break;
                }
                if (hanekaeri) {
                    player_flg_on(&sh2jms.lower_st_flg, 0x10000000);
                    player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
                }
                break;
            }
            case 7:
            case 8:
                if (this->battle.atk_result != 1) {
                    if (playing.battle_level != 2) {
                        if (sh2jms.lower_now == JMS_ST_L_ATTACK && a_info->speed > 0) {
                            player_flg_on(&sh2jms.lower_st_flg, 0x10000000);
                            player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
                        }
                    }
                }
                break;
            }
            a_info = shCharacterAnimeGetInfo_(this, 1);
            switch (sh2jms.weapon) {
            case 5:
            case 6: {
                int hanekaeri = 0;

                switch (this->battle.atk_result) {
                case 1:
                    if (sh2jms.upper_now == JMS_ST_U_ATTACK && a_info->speed > 0 &&
                        !(sh2jms.player->battle.target->battle.status & 4)) {
                        hanekaeri = 1;
                    }
                    break;
                default:
                    if (playing.battle_level != 2) {
                        if (sh2jms.upper_now == JMS_ST_U_ATTACK && a_info->speed > 0) {
                            hanekaeri = 1;
                        }
                    }
                    break;
                }
                if (hanekaeri) {
                    sh2jms.strike_splash_flg = 1;
                    if (this->battle.atk_result != 1) {
                        switch (sh2jms.weapon) {
                        case 5:
                            se = 0x2B18;
                            break;
                        case 6:
                            se = 0x2B20;
                            break;
                        }
                        SeCallPos(se, 0.8f, (float *)&this->pos, 0);
                    }
                    player_flg_on(&sh2jms.upper_st_flg, 0x10000000);
                    player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
                }
                break;
            }
            case 7:
            case 8:
                switch (this->battle.atk_result) {
                case 1:
                    if (sh2jms.upper_now == JMS_ST_U_ATTACK && a_info->speed > 0) {
                        switch (sh2jms.weapon) {
                        case 8:
                            se = 0x2B1E;
                            break;
                        case 7:
                            se = 0x2B26;
                            break;
                        }
                        SeCallPos(se, 0.8f, (float *)&this->pos, 0);
                    }
                    break;
                case 2:
                case 3:
                    if (sh2jms.upper_now == JMS_ST_U_ATTACK && a_info->speed > 0) {
                        if (playing.battle_level != 2) {
                            sh2jms.strike_splash_flg = 1;
                            player_flg_on(&sh2jms.upper_st_flg, 0x10000000);
                            player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
                            switch (sh2jms.weapon) {
                            case 8:
                                se = 0x2B1D;
                                break;
                            case 7:
                                se = 0x2B30;
                                shSdSeStop(0x2B25);
                                break;
                            }
                            SeCallPos(se, 0.8f, (float *)&this->pos, 0);
                        }
                    }
                    break;
                }
                break;
            }
            break;
        }
    }
    if (sh2jms.hp_recover) {
        this->battle.hp += sh2jms.hp_recover;
        sh2jms.hp_recover = 0.0f;
        this->battle.hp = (this->battle.hp < 0.0f)
                              ? 0.0f
                              : ((this->battle.hp > this->battle.hp_max) ? this->battle.hp_max : this->battle.hp);
    }
    if (PlayerChectGuardSuccess()) {
        if (this->battle.damage && !sh2jms.no_damage) {
            sh2jms.no_damage = 1;
            player_flg_on(&sh2jms.lower_st_flg, 0x200000);
            this->battle.hp -= 0.1f * this->battle.damage;
            GameJamesDamagedCountUp(0.1f * this->battle.damage);
            this->battle.damage = 0.0f;
            sh2jms.shock = this->battle.shock;
        }
    }
    if (this->battle.damage && !sh2jms.no_damage) {
        EventCancel();
        sh2jms.no_damage = 1;
        player_flg_on(&sh2jms.lower_st_flg, 0x2000000);
        if (this->battle.hp <= 0.0f) {
            sh2jms.dead = 1;
        }
        sh2jms.damage_no = PlayerDamageMotionNo(this);
        this->battle.hp -= this->battle.damage;
        GameJamesDamagedCountUp(this->battle.damage);
        PlayerCheckHuggingAttack();
        if (sh2jms.dead) {
            sh2jms.d_shock = 3;
        } else {
            sh2jms.d_shock = 2;
        }
        this->battle.damage = 0.0f;
    }
    if (this->battle.damage) {
        this->battle.hp -= this->battle.damage;
        GameJamesDamagedCountUp(this->battle.damage);
    }
    this->battle.hp = (this->battle.hp < 0.0f)
                          ? 0.0f
                          : ((this->battle.hp > this->battle.hp_max) ? this->battle.hp_max : this->battle.hp);
    this->battle.damage = 0.0f;
    this->battle.shock = 0.0f;
    this->battle.hp_rate = 100.0f * this->battle.hp / this->battle.hp_max;
    sh2jms.hp = this->battle.hp;
    sh2jms.hp_max = this->battle.hp_max;
    sh2jms.tired_max = 600.0f * this->battle.hp_rate / 100.0f;
    sh2jms.tired = (sh2jms.tired < 0) ? 0 : ((sh2jms.tired > sh2jms.tired_max) ? sh2jms.tired_max : sh2jms.tired);
}

/* asm_helpers.h's lengthXZ. Matching: kept local, here: without a definition at this point,
 * PlayerCheckNeckAngle's float constants load in another order (see also the stand-in before PlayerCheckDamage). */
inline float LengthXZ(float *v) {
    float d;

    asm {
        lwc1 d, 0(v)
        lwc1 $f8, 8(v)
        mula.s d, d
        madd.s d, $f8, $f8
        sqrt.s d, d
    }
    return d;
}


/* Matching: propagated; they set the argument load order. Unverifiable: MWCC leaves no DWARF for a
 * propagated static const */
static const float neck_x_min = -0.4f, neck_x_max = 0.5f;
static void PlayerCheckNeckAngle(struct SubCharacter *this) {
    struct SubCharacterDisp *this_d;
    float pos[4];
    float tmp[4];

    switch (sh2jms.upper_now) {
    case JMS_ST_U_RELAX:
    case JMS_ST_U_ALERT:
    case JMS_ST_U_FALL:
    case JMS_ST_U_DAMAGE:
    case JMS_ST_U_KICK:
    case JMS_ST_U_GUARD:
        sh2jms.tgt_neck_angle.x = sh2jms.tgt_neck_angle.y = 0.0f;
        break;
    default:
        if (!PlayerSearchVIewButtonCheck()) {
            if (sh2jms.look_tgt) {
                if (sh2jms.target) {
                    vcopy(&sh2jms.target->pos, pos);
                    pos[1] = sh2jms.target->eye_y;
                } else {
                    if (sh2jms.hold_type == -1) {
                        vcopy(&sh2jms.look_tgt->pos, pos);
                        pos[1] = sh2jms.look_tgt->eye_y;
                    } else {
                        sh2jms.tgt_neck_angle.x = sh2jms.tgt_neck_angle.y = 0.0f;
                        break;
                    }
                }
                tmp[0] = pos[0] - this->pos.x;
                tmp[1] = pos[1] - this->eye_y;
                tmp[2] = pos[2] - this->pos.z;
                sh2jms.tgt_neck_angle.y = shAtan2(tmp[2], tmp[0]) - this->rot.y;
                sh2jms.tgt_neck_angle.x = -shAtan2(LengthXZ(tmp), tmp[1]);
                sh2jms.tgt_neck_angle.x = ANGLE_WRAP(sh2jms.tgt_neck_angle.x);
                sh2jms.tgt_neck_angle.y = ANGLE_WRAP(sh2jms.tgt_neck_angle.y);
            } else {
                sh2jms.tgt_neck_angle.x = sh2jms.tgt_neck_angle.y = 0.0f;
            }
        } else {
            switch (sh2jms.upper_now) {
            case JMS_ST_U_HOLD:
            case JMS_ST_U_ATTACK:
                sh2jms.tgt_neck_angle.x = sh2jms.tgt_neck_angle.y = 0.0f;
                break;
            default:
                sh2jms.tgt_neck_angle.y = 0.8f * sh2jms.rstick_x;
                if (0.8f * sh2jms.rstick_x < 0.0f) {
                    sh2jms.tgt_neck_angle.x = -0.5f * sh2jms.rstick_y;
                } else {
                    sh2jms.tgt_neck_angle.x = -0.4f * sh2jms.rstick_y;
                }
                break;
            }
        }
        break;
    }
    this_d = (struct SubCharacterDisp *)this;
    close_to_angle_target(&this_d->anime2.rot_neck.x, sh2jms.tgt_neck_angle.x, neck_x_min, neck_x_max, 20.0f);
    close_to_angle_target(&this_d->anime2.rot_neck.y, sh2jms.tgt_neck_angle.y, -0.8f, 0.8f, 20.0f);
    this_d->anime2.rot_body_neck.x = 0.2f * this_d->anime2.rot_neck.x;
    this_d->anime2.rot_body_neck.y = 0.2f * this_d->anime2.rot_neck.y;
}

/* Matching: fitted float-constant stand-in, not recovered code (docs/stand-ins.md). */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f; }
/** Turns James's arms towards his target while he holds or attacks, otherwise back to rest. */
void PlayerCheckBothArmsAngle(struct SubCharacter *this) {
    struct SubCharacterDisp *this_d;
    float pos[4];
    float tmp[4];

    if (sh2jms.target && sh2jms.upper_now >= JMS_ST_U_HOLD && sh2jms.upper_now <= JMS_ST_U_ATTACK) {
        if (sh2jms.weapon >= 5) {
            sh2jms.tgt_arms_angle.x = 0.0f;
        } else {
            pos[0] = sh2jms.target->pos.x;
            pos[2] = sh2jms.target->pos.z;
            pos[1] = sh2jms.target->center_y;
            tmp[0] = pos[0] - this->pos.x;
            tmp[1] = pos[1] - (-800.0 + this->pos.y);
            tmp[2] = pos[2] - this->pos.z;
            sh2jms.tgt_arms_angle.x = -shAtan2(LengthXZ(tmp), tmp[1]);
            sh2jms.tgt_arms_angle.x = ANGLE_WRAP(sh2jms.tgt_arms_angle.x);
        }
    } else {
        sh2jms.tgt_arms_angle.x = 0.0f;
    }
    this_d = (struct SubCharacterDisp *)this;
    if (sh2jms.weapon == 3) {
        if (sh2jms.upper_now == JMS_ST_U_HOLD && this_d->anime2.cur_frame.x <= 9) {
            sh2jms.tgt_arms_angle.x = 0.0f;
        } else {
            close_to_angle_target(&this_d->anime2.rot_arms.x, sh2jms.tgt_arms_angle.x, -0.8f, 0.8f, 20.0f);
        }
    } else {
        close_to_angle_target(&this_d->anime2.rot_arms.x, sh2jms.tgt_arms_angle.x, -1.0f, 1.0f, 20.0f);
    }
    if (sh2jms.weapon == 2) {
        if (sh2jms.tgt_arms_angle.x > 0.2f) {
            sh2jms.shotgun_dir = 2;
        } else if (sh2jms.tgt_arms_angle.x < -0.5f) {
            sh2jms.shotgun_dir = 0;
        } else {
            sh2jms.shotgun_dir = 1;
        }
    }
}

/* Matching: fitted float-constant stand-in, not recovered code (docs/stand-ins.md). */
static float __stripped_float_code_2(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f; }
static void PlayerCheckBodyAngle(struct SubCharacter *this) {
    struct SubCharacterDisp *this_d;
    int wep;
    int hold = 0;
    int move;
    float angle_table[4][2][3] = {
        { { 0.0f, 0.4f, -0.6f }, { 0.0f, 0.8f, -0.2f } },
        { { 0.0f, 0.4f, -0.6f }, { 0.0f, 0.6f, -0.4f } },
        { { 0.0f, 0.3f, -1.0f }, { 0.0f, 1.0f, -0.3f } },
        { { 0.0f, 0.7f, -0.7f }, { 0.0f, 0.7f, -0.7f } },
    };

    switch (sh2jms.weapon) {
    case 5:
        wep = 0;
        break;
    case 6:
        wep = 1;
        break;
    case 7:
        wep = 2;
        break;
    case 4:
        wep = 3;
        break;
    }
    switch (sh2jms.lower_now) {
    case JMS_ST_L_RSWALK:
    case JMS_ST_L_RSRUN:
        move = 1;
        break;
    case JMS_ST_L_LSWALK:
    case JMS_ST_L_LSRUN:
        move = 2;
        break;
    default:
        move = 0;
        break;
    }
    switch (sh2jms.hold_type) {
    case 0:
        hold = 0;
        break;
    case 1:
        hold = 1;
        break;
    default:
        move = 0;
        break;
    }
    switch (sh2jms.weapon) {
    case 4:
    case 5:
    case 6:
    case 7:
        sh2jms.tgt_body_angle.y = angle_table[wep][hold][move];
        break;
    case 2:
    case 3:
        sh2jms.tgt_body_angle.y = -0.1f;
        break;
    default:
        sh2jms.tgt_body_angle.y = 0.0f;
        break;
    }
    this_d = (struct SubCharacterDisp *)this;
    if (sh2jms.tgt_body_angle.y) {
        close_to_angle_target(&this_d->anime2.rot_body.y, sh2jms.tgt_body_angle.y, -1.0f, sh2jms.tgt_body_angle.y, 10.0f);
    } else {
        close_to_angle_target(&this_d->anime2.rot_body.y, sh2jms.tgt_body_angle.y, sh2jms.tgt_body_angle.y, 1.0f, 10.0f);
    }
}

static void PlayerCheckSetParameterPhase1(struct SubCharacter *this) {
    int wep;

    PlayerCheckSideLine(this);
    PlayerSetSearchArea(this);
    wep = PlayerNowItemName(sh2jms.weapon);
    if (wep != -1) {
        sh2jms.shoot_val = ItemWeaponShoot(wep, 0);
        sh2jms.reload_val = ItemWeaponReload(wep, 0);
    }
    PlayerGetTargetInfo();
    PlayerCheckBothArmsAngle(this);
}

static void PlayerCheckSetParameterPhase2(struct SubCharacter *this) {
    PlayerCheckFootLine(this);
    PlayerSetHitColumn(this);
    PlayerCheckNeckAngle(this);
    PlayerCheckBodyAngle(this);
}

static void PlayerCheckModelParts(void) {
    struct _AnimeInfo *a_info;
    short frame;
    int status;

    a_info = shCharacterAnimeGetInfo_(sh2jms.player, 1);
    frame = shCharacterAnimeFrameGet_(sh2jms.player, 1);
    if ((sh2jms.player->status & 0x8000) || !((item.flag[0] >> 15) & 1)) {
        sh2jms.parts_light = 0;
    } else {
        sh2jms.parts_light = 21;
    }
    switch (sh2jms.upper_now) {
    case JMS_ST_U_RUN1:
    case JMS_ST_U_RUN2:
    case JMS_ST_U_RUN3:
    case JMS_ST_U_RSRUN:
    case JMS_ST_U_LSRUN:
    case JMS_ST_U_READY:
    case JMS_ST_U_READYOFF:
        status = 1;
        break;
    case JMS_ST_U_DAMAGE:
    case JMS_ST_U_FALL:
    case JMS_ST_U_WALL_F:
    case JMS_ST_U_TO_STAND:
        status = 2;
        break;
    case JMS_ST_U_EVENT:
        if (a_info->name == 0x65) {
            status = 0;
        } else {
            status = 2;
        }
        break;
    case JMS_ST_U_RELAX:
        if (sh2jms.weapon == 8) {
            status = 1;
        } else {
            status = 2;
        }
        break;
    case JMS_ST_U_HOLD:
    case JMS_ST_U_ATTACK:
    case JMS_ST_U_RELEASE:
        status = 3;
        break;
    case JMS_ST_U_KICK:
    default:
        status = 0;
        break;
    }
    if (shCharacterGetSubCharacter(0x10B, -1) && (sh2jms.player->status & 0x20000)) {
        sh2jms.parts_rhand = 5;
        sh2jms.parts_lhand = 16;
    } else {
        switch (sh2jms.weapon) {
        case 0:
        case 1:
        case 4:
            switch (status) {
            case 0:
            case 2:
            case 3:
                sh2jms.parts_rhand = 3;
                sh2jms.parts_lhand = 14;
                break;
            case 1:
                sh2jms.parts_rhand = 4;
                sh2jms.parts_lhand = 15;
                break;
            }
            switch (sh2jms.weapon) {
            case 1:
                sh2jms.parts_rhand = 7;
                break;
            case 4:
                sh2jms.parts_rhand = 11;
                break;
            }
            break;
        case 2:
        case 3:
            switch (status) {
            case 2:
                sh2jms.parts_lhand = 14;
                break;
            default:
                sh2jms.parts_lhand = 17;
                break;
            }
            sh2jms.parts_rhand = 8;
            break;
        case 5:
            switch (status) {
            case 0:
            case 1:
                sh2jms.parts_lhand = 19;
                break;
            case 3:
            case 2:
                sh2jms.parts_lhand = 14;
                break;
            }
            if (status == 3) {
                if (sh2jms.atk_type == 4) {
                    if (frame >= 8 && frame <= 21) {
                        sh2jms.parts_lhand = 19;
                    } else {
                        sh2jms.parts_lhand = 14;
                    }
                }
            }
            sh2jms.parts_rhand = 10;
            break;
        case 6:
            switch (status) {
            case 0:
            case 1:
                sh2jms.parts_lhand = 19;
                break;
            case 3:
                sh2jms.parts_lhand = 18;
                break;
            case 2:
                sh2jms.parts_lhand = 14;
                break;
            }
            sh2jms.parts_rhand = 9;
            break;
        case 8:
            switch (status) {
            case 2:
                sh2jms.parts_lhand = 14;
                break;
            default:
                sh2jms.parts_lhand = 20;
                break;
            }
            sh2jms.parts_rhand = 12;
            break;
        case 7:
            sh2jms.parts_rhand = 13;
            sh2jms.parts_lhand = 14;
            break;
        }
    }
    sh2gfw_Set_JMSequip(sh2jms.player, sh2jms.parts_lhand, sh2jms.parts_rhand, sh2jms.parts_light);
}

static void james_anim_set_all(struct _AnimeInfo *aip, int comp_type) {
    shCharacterAnimeSet(sh2jms.player, 2, comp_type, aip, (int)shCharacterGetAnimeAdrForPlay(sh2jms.player));
    shCharacterAnimeSet(sh2jms.player, 1, comp_type, aip, (int)shCharacterGetAnimeAdrForPlay(sh2jms.player));
}

static void james_anim_set(struct _AnimeInfo *aip, int body_type, int comp_type) {
    shCharacterAnimeSet(sh2jms.player, body_type, comp_type, aip, (int)shCharacterGetAnimeAdrForPlay(sh2jms.player));
}

static struct _AnimeInfo *PlayerGetStageAnime(int anime) {
    int i;

    i = 0;
    while (jms_stage_anim[i].name != 0L) {
        if (anime == jms_stage_anim[i].name) {
            return &jms_stage_anim[i];
        }
        i++;
    }
    return NULL;
}

/*
 * Matching: no DWARF survives for these enums; the ternaries below convert through signed 16-bit and
 * signed 8-bit types, so their constants came from enums with a negative member.
 */
enum _JMS_HUG_ANIME {
    JMS_HUG_ANIME_NONE = -1,
    JMS_HUG_ANIME_4E27 = 0x4E27,
    JMS_HUG_ANIME_4E3F = 0x4E3F,
    JMS_HUG_ANIME_4E40 = 0x4E40,
    JMS_HUG_ANIME_4E48 = 0x4E48,
    JMS_HUG_ANIME_4E49 = 0x4E49,
    JMS_HUG_ANIME_4E53 = 0x4E53,
    JMS_HUG_ANIME_4E54 = 0x4E54,
    JMS_HUG_ANIME_4E55 = 0x4E55,
    JMS_HUG_ANIME_4E57 = 0x4E57,
    JMS_HUG_ANIME_4E59 = 0x4E59,
};

enum _ANIME_COMP_TYPE {
    ANIME_COMP_PAUSE = -1,
    ANIME_COMP_4 = 4,
    ANIME_COMP_10 = 10,
};

static void PlayerCheckAnimeUpper(void) {
    static int anime_change_check_upper = -1;
    struct SubCharacterDisp *scp_d;
    struct _AnimeInfo *aip;

    scp_d = (struct SubCharacterDisp *)sh2jms.player;
    if (!u_anime_flg_on(2)) {
        if (sh2jms.upper_now != anime_change_check_upper || u_anime_flg_on(0x40)) {
            switch (sh2jms.upper_now) {
            case JMS_ST_U_STAND:
                aip = (struct _AnimeInfo *)&pjames_anim[1];
                james_anim_set(aip, 1, 8);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_LTURN:
                aip = (struct _AnimeInfo *)&pjames_anim[10];
                james_anim_set(aip, 1, 2);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_RTURN:
                aip = (struct _AnimeInfo *)&pjames_anim[9];
                james_anim_set(aip, 1, 2);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_LROUND:
                aip = (struct _AnimeInfo *)&pjames_anim[3];
                james_anim_set(aip, 1, 4);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_RROUND:
                aip = (struct _AnimeInfo *)&pjames_anim[2];
                james_anim_set(aip, 1, 4);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_WALK: {
                int comp_type;

                if (sh2jms.upper_prev >= JMS_ST_U_WALK && sh2jms.upper_prev <= JMS_ST_U_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.u_anime_st_flg, 4);
                } else {
                    comp_type = 8;
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[4];
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_LSWALK: {
                int comp_type;

                if (sh2jms.upper_prev >= JMS_ST_U_WALK && sh2jms.upper_prev <= JMS_ST_U_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.u_anime_st_flg, 4);
                } else {
                    comp_type = 8;
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[25];
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_RSWALK: {
                int comp_type;

                if (sh2jms.upper_prev >= JMS_ST_U_WALK && sh2jms.upper_prev <= JMS_ST_U_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.u_anime_st_flg, 4);
                } else {
                    comp_type = 8;
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[24];
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_BACK:
                aip = (struct _AnimeInfo *)&pjames_anim[5];
                james_anim_set(aip, 1, 8);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_JUMP:
                aip = (struct _AnimeInfo *)&pjames_anim[21];
                james_anim_set(aip, 1, 4);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_RUN1: {
                int comp_type;

                if (sh2jms.upper_prev >= JMS_ST_U_WALK && sh2jms.upper_prev <= JMS_ST_U_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.u_anime_st_flg, 4);
                } else {
                    comp_type = 8;
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[6];
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_RUN2: {
                int comp_type;

                if (sh2jms.upper_prev >= JMS_ST_U_WALK && sh2jms.upper_prev <= JMS_ST_U_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.u_anime_st_flg, 4);
                } else {
                    comp_type = 8;
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[7];
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_RUN3: {
                int comp_type;

                if (sh2jms.upper_prev >= JMS_ST_U_WALK && sh2jms.upper_prev <= JMS_ST_U_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.u_anime_st_flg, 4);
                } else {
                    comp_type = 8;
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[8];
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_LSRUN: {
                int comp_type;

                if (sh2jms.upper_prev >= JMS_ST_U_WALK && sh2jms.upper_prev <= JMS_ST_U_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.u_anime_st_flg, 4);
                } else {
                    comp_type = 8;
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[27];
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_RSRUN: {
                int comp_type;

                if (sh2jms.upper_prev >= JMS_ST_U_WALK && sh2jms.upper_prev <= JMS_ST_U_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.u_anime_st_flg, 4);
                } else {
                    comp_type = 8;
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[26];
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_GUARD:
                aip = (struct _AnimeInfo *)&pjames_anim[23];
                james_anim_set(aip, 1, 2);
                break;
            case JMS_ST_U_READY:
                aip = (struct _AnimeInfo *)&pjames_anim[19];
                james_anim_set(aip, 1, 4);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_READYOFF:
                aip = (struct _AnimeInfo *)&pjames_anim[20];
                james_anim_set(aip, 1, 4);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_ALERT:
                if (sh2jms.weapon == 7) {
                    aip = (struct _AnimeInfo *)&pjames_cs_anim[23];
                } else {
                    aip = (struct _AnimeInfo *)&pjames_anim[18];
                }
                james_anim_set(aip, 1, 4);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_RELAX: {
                int anime;

                aip = shCharacterAnimeGetInfo_(sh2jms.player, 2);
                anime = aip->name;
                aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                james_anim_set(aip, 1, 4);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            }
            case JMS_ST_U_HOLD: {
                int comp_type;
                int anime;
                int anime_on;

                anime_on = 1;
                if (sh2jms.upper_prev == JMS_ST_U_ATTACK) {
                    switch (sh2jms.weapon) {
                    case 0:
                        break;
                    case 1:
                        if (!sh2jms.lock_on && !sh2jms.reload) {
                            comp_type = 4;
                            sh2jms.hold_type = 0;
                            aip = (struct _AnimeInfo *)&pjames_hg_anim[6];
                        } else {
                            if (!sh2jms.reload) {
                                sh2jms.hold_type = 1;
                            } else {
                                sh2jms.hold_type = 0;
                                sh2jms.reload = 0;
                            }
                            anime_on = 0;
                        }
                        break;
                    case 2:
                        if (sh2jms.hold_chg[0] && sh2jms.shotgun_dir != sh2jms.shotgun_prev) {
                            switch (sh2jms.shotgun_prev) {
                            case 0:
                                if (sh2jms.shotgun_dir == 2) {
                                    anime = 0x10A;
                                } else {
                                    anime = 0x108;
                                }
                                break;
                            case 1:
                                if (sh2jms.shotgun_dir == 2) {
                                    anime = 0x105;
                                } else {
                                    anime = 0x107;
                                }
                                break;
                            case 2:
                                if (sh2jms.shotgun_dir == 1) {
                                    anime = 0x106;
                                } else {
                                    anime = 0x109;
                                }
                                break;
                            }
                            comp_type = 4;
                            sh2jms.shotgun_prev = sh2jms.shotgun_dir;
                            sh2jms.hold_type = 1;
                            aip = (struct _AnimeInfo *)&pjames_sg_anim[anime - 250];
                        } else {
                            anime_on = 0;
                        }
                        break;
                    case 3:
                        anime_on = 0;
                        break;
                    case 4:
                        shSdSeStop(0x2B27);
                        sh2jms.csaw_se_vol = 0.0f;
                        comp_type = 4;
                        aip = (struct _AnimeInfo *)&pjames_sp_anim[(sh2jms.hold_loop[0] ? 0x1F9 : 0x1FB) - 500];
                        break;
                    case 5:
                        if (sh2jms.hold_chg[0]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x169;
                                sh2jms.hold_type = 1;
                            } else {
                                anime = 0x16A;
                                sh2jms.hold_type = 0;
                            }
                            comp_type = 4;
                        } else {
                            if (sh2jms.hold_loop[0]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x168;
                                } else {
                                    anime = 0x167;
                                }
                                comp_type = 4;
                            } else {
                                if (sh2jms.atk_type == 3) {
                                    if (!sh2jms.hold_type) {
                                        anime = 0x16F;
                                    } else {
                                        anime = 0x175;
                                    }
                                    comp_type = 4;
                                } else {
                                    anime = 0;
                                    anime_on = 0;
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_ka_anim[anime - 350];
                        break;
                    case 6:
                        if (sh2jms.hold_chg[0]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x197;
                                sh2jms.hold_type = 1;
                            } else {
                                anime = 0x198;
                                sh2jms.hold_type = 0;
                            }
                            comp_type = 4;
                        } else {
                            if (sh2jms.hold_loop[0]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x196;
                                } else {
                                    anime = 0x195;
                                }
                                comp_type = 4;
                            } else {
                                if (sh2jms.atk_type == 3) {
                                    if (!sh2jms.hold_type) {
                                        anime = 0x19D;
                                    } else {
                                        anime = 0x1A3;
                                    }
                                    comp_type = 4;
                                } else {
                                    anime = 0;
                                    anime_on = 0;
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_pi_anim[anime - 400];
                        break;
                    case 8:
                        if (sh2jms.hold_chg[0]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x1C8;
                                sh2jms.hold_type = 1;
                            } else {
                                anime = 0x1C7;
                                sh2jms.hold_type = 0;
                            }
                            comp_type = 4;
                        } else {
                            if (sh2jms.hold_loop[0]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x1CA;
                                } else {
                                    anime = 0x1C9;
                                }
                                comp_type = 4;
                            } else {
                                anime = 0;
                                anime_on = 0;
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_na_anim[anime - 450];
                        break;
                    case 7:
                        shSdSeStop(0x2B25);
                        shSdSeStop(0x2B30);
                        if (sh2jms.hold_chg[0]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x233;
                                sh2jms.hold_type = 1;
                            } else {
                                anime = 0x234;
                                sh2jms.hold_type = 0;
                            }
                            comp_type = 4;
                        } else {
                            if (sh2jms.hold_loop[0]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x230;
                                } else {
                                    anime = 0x22F;
                                }
                                comp_type = 2;
                            } else {
                                anime = 0;
                                anime_on = 0;
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_cs_anim[anime - 550];
                        break;
                    }
                    if (anime_on) {
                        james_anim_set(aip, 1, comp_type);
                        player_flg_on(&sh2jms.u_anime_st_flg, 2);
                    }
                } else {
                    if (sh2jms.upper_prev == JMS_ST_U_RELEASE) {
                        comp_type = 6;
                    } else {
                        comp_type = 4;
                    }
                    switch (sh2jms.weapon) {
                    case 0:
                        break;
                    case 1:
                        if (u_anime_flg_on(0x40)) {
                            anime = 0xCE;
                            sh2jms.hold_type = 0;
                        } else {
                            if (sh2jms.lock_on) {
                                anime = 0xCB;
                                sh2jms.hold_type = 1;
                            } else {
                                anime = 0xC9;
                                sh2jms.hold_type = 0;
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_hg_anim[anime - 200];
                        break;
                    case 2:
                        if (sh2jms.hold_chg[0] && sh2jms.shotgun_dir != sh2jms.shotgun_prev) {
                            switch (sh2jms.shotgun_prev) {
                            case 0:
                                if (sh2jms.shotgun_dir == 2) {
                                    anime = 0x10A;
                                } else {
                                    anime = 0x108;
                                }
                                break;
                            case 1:
                                if (sh2jms.shotgun_dir == 2) {
                                    anime = 0x105;
                                } else {
                                    anime = 0x107;
                                }
                                break;
                            case 2:
                                if (sh2jms.shotgun_dir == 1) {
                                    anime = 0x106;
                                } else {
                                    anime = 0x109;
                                }
                                break;
                            }
                        } else {
                            switch (sh2jms.shotgun_dir) {
                            case 0:
                                anime = 0x101;
                                break;
                            case 1:
                                anime = 0xFB;
                                break;
                            case 2:
                                anime = 0xFE;
                                break;
                            }
                        }
                        sh2jms.shotgun_prev = sh2jms.shotgun_dir;
                        sh2jms.hold_type = 1;
                        aip = (struct _AnimeInfo *)&pjames_sg_anim[anime - 250];
                        break;
                    case 3:
                        sh2jms.hold_type = 1;
                        aip = (struct _AnimeInfo *)&pjames_rg_anim[1];
                        break;
                    case 4:
                        if (sh2jms.hold_loop[0]) {
                            anime = 0x1F9;
                        } else {
                            anime = 0x1F5;
                            sh2jms.hold_type = 1;
                        }
                        aip = (struct _AnimeInfo *)&pjames_sp_anim[anime - 500];
                        break;
                    case 5:
                        if (sh2jms.hold_chg[0]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x169;
                                sh2jms.hold_type = 1;
                            } else {
                                anime = 0x16A;
                                sh2jms.hold_type = 0;
                            }
                        } else {
                            if (sh2jms.hold_loop[0]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x168;
                                } else {
                                    anime = 0x167;
                                }
                                comp_type = 4;
                            } else {
                                if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                                    anime = 0x15F;
                                    sh2jms.hold_type = 1;
                                } else {
                                    anime = 0x161;
                                    sh2jms.hold_type = 0;
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_ka_anim[anime - 350];
                        break;
                    case 6:
                        if (sh2jms.hold_chg[0]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x197;
                                sh2jms.hold_type = 1;
                            } else {
                                anime = 0x198;
                                sh2jms.hold_type = 0;
                            }
                        } else {
                            if (sh2jms.hold_loop[0]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x196;
                                } else {
                                    anime = 0x195;
                                }
                                comp_type = 4;
                            } else {
                                if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                                    anime = 0x191;
                                    sh2jms.hold_type = 1;
                                } else {
                                    anime = 0x193;
                                    sh2jms.hold_type = 0;
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_pi_anim[anime - 400];
                        break;
                    case 8:
                        if (sh2jms.hold_chg[0]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x1C8;
                                sh2jms.hold_type = 1;
                            } else {
                                anime = 0x1C7;
                                sh2jms.hold_type = 0;
                            }
                        } else {
                            if (sh2jms.hold_loop[0]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x1CA;
                                } else {
                                    anime = 0x1C9;
                                }
                                comp_type = 4;
                            } else {
                                if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                                    anime = 0x1C3;
                                    sh2jms.hold_type = 1;
                                } else {
                                    anime = 0x1C5;
                                    sh2jms.hold_type = 0;
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_na_anim[anime - 450];
                        break;
                    case 7:
                        if (sh2jms.hold_chg[0]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x233;
                                sh2jms.hold_type = 1;
                            } else {
                                anime = 0x234;
                                sh2jms.hold_type = 0;
                            }
                        } else {
                            if (sh2jms.hold_loop[0]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x230;
                                } else {
                                    anime = 0x22F;
                                }
                                comp_type = 2;
                            } else {
                                sh2jms.csaw_se_vol = 0.7f;
                                SeCallPos(0x2B24, sh2jms.csaw_se_vol, (float *)&sh2jms.player->pos, 0);
                                if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                                    anime = 0x227;
                                    sh2jms.hold_type = 1;
                                } else {
                                    anime = 0x22B;
                                    sh2jms.hold_type = 0;
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_cs_anim[anime - 550];
                        break;
                    }
                    james_anim_set(aip, 1, comp_type);
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                }
                sh2jms.atk_type = 0;
                break;
            }
            case JMS_ST_U_RELEASE: {
                int anime;
                int comp_type;

                switch (sh2jms.weapon) {
                case 0:
                    break;
                case 1:
                    if (sh2jms.lock_on || (sh2jms.upper_prev == JMS_ST_U_ATTACK && !sh2jms.reload)) {
                        anime = 0xCC;
                        sh2jms.reload = 0;
                    } else {
                        anime = 0xCA;
                    }
                    aip = (struct _AnimeInfo *)&pjames_hg_anim[anime - 200];
                    break;
                case 2:
                    switch (sh2jms.shotgun_dir) {
                    case 0:
                        anime = 0x102;
                        break;
                    case 1:
                        anime = 0xFC;
                        break;
                    case 2:
                        anime = 0xFF;
                        break;
                    }
                    aip = (struct _AnimeInfo *)&pjames_sg_anim[anime - 250];
                    break;
                case 3:
                    anime = 0x12E;
                    aip = (struct _AnimeInfo *)&pjames_rg_anim[anime - 300];
                    break;
                case 4:
                    shSdSeStop(0x2B27);
                    sh2jms.csaw_se_vol = 0.0f;
                    anime = 0x1F6;
                    aip = (struct _AnimeInfo *)&pjames_sp_anim[anime - 500];
                    break;
                case 5:
                    if (!sh2jms.hold_type) {
                        anime = 0x162;
                    } else {
                        anime = 0x160;
                    }
                    aip = (struct _AnimeInfo *)&pjames_ka_anim[anime - 350];
                    break;
                case 6:
                    if (!sh2jms.hold_type) {
                        anime = 0x194;
                    } else {
                        anime = 0x192;
                    }
                    aip = (struct _AnimeInfo *)&pjames_pi_anim[anime - 400];
                    break;
                case 8:
                    if (!sh2jms.hold_type) {
                        anime = 0x1C6;
                    } else {
                        anime = 0x1C4;
                    }
                    aip = (struct _AnimeInfo *)&pjames_na_anim[anime - 450];
                    break;
                case 7:
                    shSdSeStop(0x2B25);
                    shSdSeStop(0x2B30);
                    if (!sh2jms.hold_type) {
                        anime = 0x22E;
                    } else {
                        anime = 0x22A;
                    }
                    aip = (struct _AnimeInfo *)&pjames_cs_anim[anime - 550];
                    break;
                }
                sh2jms.hold_type = -1;
                sh2jms.atk_type = 0;
                sh2jms.atk_reserve[1] = 0;
                if (sh2jms.weapon != 7) {
                    if (anime < scp_d->anime2.anim_a->name) {
                        comp_type = 4;
                    } else {
                        comp_type = 6;
                    }
                } else {
                    comp_type = 4;
                }
                james_anim_set(aip, 1, comp_type);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            }
            case JMS_ST_U_ATTACK: {
                int anime;
                int comp_type;

                comp_type = 2;
                switch (sh2jms.weapon) {
                case 0:
                    break;
                case 1:
                    if (!sh2jms.shoot_val && sh2jms.reload_val) {
                        if (sh2jms.hold_type) {
                            anime = 0xD2;
                        } else {
                            anime = 0xD1;
                        }
                        sh2jms.reload = 1;
                        sh2jms.hold_type = 0;
                    } else {
                        if (sh2jms.hold_type == 1 || u_anime_flg_on(0x40)) {
                            anime = 0xCF;
                        } else {
                            anime = 0xD0;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_hg_anim[anime - 200];
                    break;
                case 2:
                    if (!sh2jms.shoot_val && sh2jms.reload_val) {
                        anime = 0x104;
                    } else {
                        switch (sh2jms.shotgun_dir) {
                        case 0:
                            anime = 0x103;
                            break;
                        case 1:
                            anime = 0xFD;
                            break;
                        case 2:
                            anime = 0x100;
                            break;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_sg_anim[anime - 250];
                    break;
                case 3:
                    if (!sh2jms.shoot_val && sh2jms.reload_val) {
                        anime = 0x130;
                    } else {
                        anime = 0x12F;
                    }
                    aip = (struct _AnimeInfo *)&pjames_rg_anim[anime - 300];
                    break;
                case 4:
                    if (!sh2jms.shoot_val && sh2jms.reload_val) {
                        if (sh2jms.upper_prev == JMS_ST_U_ATTACK) {
                            anime = 0x1FF;
                        } else {
                            anime = 0x1FE;
                        }
                    } else {
                        if (sh2jms.atk_count) {
                            anime = 0x1FD;
                        } else {
                            anime = 0x1FC;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_sp_anim[anime - 500];
                    break;
                case 5:
                    if (sh2jms.hold_type) {
                        switch (sh2jms.atk_type) {
                        case 3:
                            if (sh2jms.atk_count) {
                                anime = 0x16D;
                            } else {
                                anime = 0x16B;
                            }
                            break;
                        case 4:
                            anime = 0x177;
                            break;
                        }
                    } else {
                        switch (sh2jms.atk_type) {
                        case 3:
                            if (sh2jms.atk_count) {
                                anime = 0x173;
                            } else {
                                anime = 0x171;
                            }
                            break;
                        case 4:
                            anime = 0x17B;
                            break;
                        }
                    }
                    if (sh2jms.player->battle.atk_result) {
                        comp_type = 6;
                        switch (sh2jms.atk_type) {
                        case 3:
                            switch (scp_d->anime2.anim_a->name) {
                            case 0x16B:
                            case 0x171:
                                scp_d->anime2.total_count -= 0x4000;
                                break;
                            }
                            if (sh2jms.hold_type) {
                                anime = 0x16E;
                            } else {
                                anime = 0x174;
                            }
                            break;
                        case 4:
                            anime++;
                            break;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_ka_anim[anime - 350];
                    break;
                case 6:
                    if (sh2jms.hold_type) {
                        switch (sh2jms.atk_type) {
                        case 3:
                            if (sh2jms.atk_count) {
                                anime = 0x19B;
                            } else {
                                anime = 0x199;
                            }
                            break;
                        case 4:
                            anime = 0x1A5;
                            break;
                        case 5:
                            anime = 0x1AD;
                            break;
                        }
                    } else {
                        switch (sh2jms.atk_type) {
                        case 3:
                            if (sh2jms.atk_count) {
                                anime = 0x1A1;
                            } else {
                                anime = 0x19F;
                            }
                            break;
                        case 4:
                            anime = 0x1A9;
                            break;
                        case 5:
                            anime = 0x1AF;
                            break;
                        }
                    }
                    if (sh2jms.player->battle.atk_result) {
                        comp_type = 6;
                        switch (sh2jms.atk_type) {
                        case 3:
                            switch (scp_d->anime2.anim_a->name) {
                            case 0x199:
                            case 0x19F:
                                scp_d->anime2.total_count -= 0x6000;
                                break;
                            }
                            if (sh2jms.hold_type) {
                                anime = 0x19C;
                            } else {
                                anime = 0x1A2;
                            }
                            break;
                        case 4:
                        case 5:
                            anime++;
                            break;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_pi_anim[anime - 400];
                    break;
                case 8:
                    if (sh2jms.hold_type) {
                        switch (sh2jms.atk_type) {
                        case 3:
                            anime = 0x1CB;
                            break;
                        case 4:
                            anime = 0x1CF;
                            break;
                        }
                    } else {
                        switch (sh2jms.atk_type) {
                        case 3:
                            anime = 0x1CD;
                            break;
                        case 4:
                            anime = 0x1D1;
                            break;
                        }
                    }
                    if (sh2jms.player->battle.atk_result) {
                        comp_type = 6;
                        anime++;
                    }
                    aip = (struct _AnimeInfo *)&pjames_na_anim[anime - 450];
                    break;
                case 7:
                    if (sh2jms.hold_type) {
                        switch (sh2jms.atk_type) {
                        case 3:
                            anime = 0x235;
                            break;
                        case 5:
                            anime = 0x239;
                            break;
                        }
                    } else {
                        switch (sh2jms.atk_type) {
                        case 3:
                            anime = 0x237;
                            break;
                        case 5:
                            anime = 0x23B;
                            break;
                        }
                    }
                    if (sh2jms.player->battle.atk_result) {
                        comp_type = 6;
                        anime++;
                    } else {
                        SeCallPos(0x2B25, sh2jms.csaw_se_vol, (float *)&sh2jms.player->pos, 0);
                    }
                    aip = (struct _AnimeInfo *)&pjames_cs_anim[anime - 550];
                    break;
                }
                sh2jms.strike_splash_flg = 0;
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_TIRED: {
                int comp_type;
                int anime;

                if (anime_change_check_upper == sh2jms.lower_now) {
                    comp_type = 10;
                    anime = 0x71;
                    player_flg_on(&sh2jms.u_anime_st_flg, 2);
                } else {
                    comp_type = 4;
                    if (sh2jms.tired >= sh2jms.tired_max * 2 / 3) {
                        anime = 0x72;
                    } else {
                        anime = 0x71;
                    }
                    player_flg_on(&sh2jms.u_anime_st_flg, 4);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                james_anim_set(aip, 1, comp_type);
                break;
            }
            case JMS_ST_U_KICK: {
                int anime;

                if (sh2jms.atk_type == 7) {
                    anime = 0x85;
                } else {
                    anime = 0x84;
                }
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                james_anim_set(aip, 1, 4);
                break;
            }
            case JMS_ST_U_WALL_F: {
                int anime;

                switch (sh2jms.lower_prev) {
                case JMS_ST_L_RUN1:
                case JMS_ST_L_RUN2:
                case JMS_ST_L_RUN3:
                    anime = 0x80;
                    break;
                case JMS_ST_L_RSRUN:
                    anime = 0x81;
                    break;
                case JMS_ST_L_LSRUN:
                    anime = 0x82;
                    break;
                default:
                    /* Matching: #line puts the asserts below on their original source lines. */
#line 3888
                    assert(0);
                    break;
                }
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                james_anim_set(aip, 1, 4);
                break;
            }
            case JMS_ST_U_FALL:
                aip = (struct _AnimeInfo *)&pjames_anim[sh2jms.fall_type + 11];
                james_anim_set(aip, 1, 4);
                player_flg_on(&sh2jms.u_anime_st_flg, 2);
                break;
            case JMS_ST_U_DAMAGE: {
                int anime;

                switch (sh2jms.hug_status) {
                case 0:
                case 1:
                    anime = sh2jms.damage_no;
                    break;
                case 2:
                    switch (sh2jms.player->battle.id) {
                    case 0x2F:
                    case 0x30:
                        anime = 0x4E3E;
                        break;
                    case 0x36:
                    case 0x37:
                        anime = 0x4E47;
                        break;
                    case 0x39:
                    case 0x3A:
                        if (sh2jms.hug_dir) {
                            anime = 0x4E4F;
                        } else {
                            anime = 0x4E4B;
                        }
                        break;
                    case 0x3B:
                    case 0x3C:
                        anime = 0x4E56;
                        break;
                    case 0x3E:
                    case 0x3F:
                        aip = shCharacterAnimeGetInfo_(sh2jms.player, 2);
                        anime = aip->name;
                        break;
                    }
                    break;
                case 3:
                case 4:
                    switch (sh2jms.player->battle.id) {
                    case 0x30:
                    case 0x2F:
                        anime = sh2jms.dead ? JMS_HUG_ANIME_4E3F : JMS_HUG_ANIME_4E40;
                        break;
                    case 0x37:
                    case 0x36:
                        anime = sh2jms.dead ? JMS_HUG_ANIME_4E48 : JMS_HUG_ANIME_4E49;
                        break;
                    case 0x3A:
                    case 0x39:
                        if (sh2jms.dead) {
                            if (sh2jms.hug_dir) {
                                anime = 0x4E51;
                            } else {
                                anime = 0x4E4D;
                            }
                        } else {
                            if (sh2jms.hug_dir) {
                                anime = 0x4E50;
                            } else {
                                anime = 0x4E4C;
                            }
                        }
                        break;
                    case 0x3C:
                    case 0x3B:
                        anime = sh2jms.dead ? JMS_HUG_ANIME_4E57 : JMS_HUG_ANIME_4E59;
                        break;
                    case 0x3E:
                    case 0x3F:
                        anime = sh2jms.dead ? JMS_HUG_ANIME_4E27 : JMS_HUG_ANIME_4E53;
                        break;
                    }
                    break;
                }
                sh2jms.damage_no = anime;
                aip = PlayerGetStageAnime(anime);
                /* Matching: #line puts the asserts below on their original source lines. */
#line 3989
                assert(aip);
                james_anim_set(aip, 1, 4);
                break;
            }
            case JMS_ST_U_EVENT: {
                int anime;
                int comp_type;

                if (sh2jms.event_move_mode) {
                    switch (sh2jms.event_status_now) {
                    case 0:
                        anime = 0x65;
                        comp_type = 4;
                        break;
                    case 1:
                        anime = 0x68;
                        comp_type = (sh2jms.event_status_prev == 2) ? ANIME_COMP_10 : ANIME_COMP_4;
                        break;
                    case 2:
                        anime = 0x6B;
                        comp_type = (sh2jms.event_status_prev == 1) ? ANIME_COMP_10 : ANIME_COMP_4;
                        break;
                    }
                    if (comp_type == 4) {
                        player_flg_on(&sh2jms.u_anime_st_flg, 2);
                    } else {
                        player_flg_on(&sh2jms.u_anime_st_flg, 4);
                    }
                    aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                } else {
                    anime = sh2jms.event_anime & 0x7FFFFFFF;
                    if (sh2jms.event_anime & 0x80000000) {
                        comp_type = 2;
                    } else {
                        player_flg_on(&sh2jms.u_anime_st_flg, 2);
                        comp_type = 4;
                    }
                    if (anime == 0x65) {
                        aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                    } else {
                        aip = PlayerGetStageAnime(anime);
                        /* Matching: #line puts the asserts below on their original source lines. */
#line 4038
                        assert_dw(aip); /* Matching: do/while(0) form (its nop) */
                    }
                }
                james_anim_set(aip, 1, comp_type);
                break;
            }
            }
            anime_change_check_upper = sh2jms.upper_now;
            player_flg_off(&sh2jms.u_anime_st_flg, 0x40);
        }
    }
    if (scp_d->anime2.comp_type < 3 || scp_d->anime2.comp_type > 8) {
        player_flg_off(&sh2jms.u_anime_st_flg, 2);
    }
    if (scp_d->anime2.comp_type <= 8) {
        player_flg_off(&sh2jms.u_anime_st_flg, 4);
    }
    if (scp_d->anime2.comp_type == -1) {
        if (!(sh2jms.anime_pause & 8)) {
            sh2jms.anime_pause |= 2;
        }
    } else {
        sh2jms.anime_pause &= 5;
    }
    sh2jms.hold_chg[0] = 0;
    sh2jms.hold_loop[0] = 0;
}

static void PlayerCheckAnimeLower(void) {
    static int anime_change_check_lower = -1;
    struct SubCharacterDisp *scp_d;
    struct _AnimeInfo *aip;

    scp_d = (struct SubCharacterDisp *)sh2jms.player;
    if (!l_anime_flg_on(2)) {
        if (sh2jms.lower_now != anime_change_check_lower || l_anime_flg_on(0x40)) {
            switch (sh2jms.lower_now) {
            case JMS_ST_L_STAND:
                aip = (struct _AnimeInfo *)&pjames_anim[1];
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            case JMS_ST_L_LTURN:
                aip = (struct _AnimeInfo *)&pjames_anim[10];
                james_anim_set(aip, 2, 2);
                break;
            case JMS_ST_L_RTURN:
                aip = (struct _AnimeInfo *)&pjames_anim[9];
                james_anim_set(aip, 2, 2);
                break;
            case JMS_ST_L_LROUND:
                aip = (struct _AnimeInfo *)&pjames_anim[3];
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            case JMS_ST_L_RROUND:
                aip = (struct _AnimeInfo *)&pjames_anim[2];
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            case JMS_ST_L_WALK: {
                int comp_type;

                if (sh2jms.lower_prev >= JMS_ST_L_WALK && sh2jms.lower_prev <= JMS_ST_L_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.l_anime_st_flg, 4);
                } else {
                    comp_type = 4;
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[4];
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_LSWALK: {
                int comp_type;

                if (sh2jms.lower_prev >= JMS_ST_L_WALK && sh2jms.lower_prev <= JMS_ST_L_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.l_anime_st_flg, 4);
                } else {
                    comp_type = 4;
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[25];
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_RSWALK: {
                int comp_type;

                if (sh2jms.lower_prev >= JMS_ST_L_WALK && sh2jms.lower_prev <= JMS_ST_L_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.l_anime_st_flg, 4);
                } else {
                    comp_type = 4;
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[24];
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_BACK:
                aip = (struct _AnimeInfo *)&pjames_anim[5];
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            case JMS_ST_L_JUMP:
                aip = (struct _AnimeInfo *)&pjames_anim[21];
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            case JMS_ST_L_RUN1: {
                int comp_type;

                if (sh2jms.lower_prev >= JMS_ST_L_WALK && sh2jms.lower_prev <= JMS_ST_L_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.l_anime_st_flg, 4);
                } else {
                    comp_type = 4;
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[6];
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_RUN2: {
                int comp_type;

                if (sh2jms.lower_prev >= JMS_ST_L_WALK && sh2jms.lower_prev <= JMS_ST_L_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.l_anime_st_flg, 4);
                } else {
                    comp_type = 4;
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[7];
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_RUN3: {
                int comp_type;

                if (sh2jms.lower_prev >= JMS_ST_L_WALK && sh2jms.lower_prev <= JMS_ST_L_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.l_anime_st_flg, 4);
                } else {
                    comp_type = 4;
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[8];
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_LSRUN: {
                int comp_type;

                if (sh2jms.lower_prev >= JMS_ST_L_WALK && sh2jms.lower_prev <= JMS_ST_L_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.l_anime_st_flg, 4);
                } else {
                    comp_type = 4;
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[27];
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_RSRUN: {
                int comp_type;

                if (sh2jms.lower_prev >= JMS_ST_L_WALK && sh2jms.lower_prev <= JMS_ST_L_RSRUN) {
                    comp_type = 10;
                    player_flg_on(&sh2jms.l_anime_st_flg, 4);
                } else {
                    comp_type = 4;
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[26];
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_GUARD:
                aip = (struct _AnimeInfo *)&pjames_anim[23];
                james_anim_set(aip, 2, 2);
                break;
            case JMS_ST_L_READY:
                aip = (struct _AnimeInfo *)&pjames_anim[19];
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            case JMS_ST_L_READYOFF:
                aip = (struct _AnimeInfo *)&pjames_anim[20];
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            case JMS_ST_L_ALERT:
                if (sh2jms.weapon == 7) {
                    aip = (struct _AnimeInfo *)&pjames_cs_anim[23];
                } else {
                    aip = (struct _AnimeInfo *)&pjames_anim[18];
                }
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            case JMS_ST_L_RELAX: {
                int anime;

                anime = shRandI() % 3 + 115;
                aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            }
            case JMS_ST_L_HOLD: {
                int comp_type;
                int anime;
                int anime_on;

                anime_on = 1;
                if (sh2jms.lower_prev == JMS_ST_L_ATTACK) {
                    switch (sh2jms.weapon) {
                    case 0:
                        break;
                    case 1:
                        if (!sh2jms.lock_on && !sh2jms.reload) {
                            comp_type = 4;
                            aip = (struct _AnimeInfo *)&pjames_hg_anim[6];
                        } else {
                            anime_on = 0;
                        }
                        break;
                    case 2:
                        if (sh2jms.hold_chg[1] && sh2jms.shotgun_dir != sh2jms.shotgun_prev) {
                            switch (sh2jms.shotgun_prev) {
                            case 0:
                                if (sh2jms.shotgun_dir == 2) {
                                    anime = 0x10A;
                                } else {
                                    anime = 0x108;
                                }
                                break;
                            case 1:
                                if (sh2jms.shotgun_dir == 2) {
                                    anime = 0x105;
                                } else {
                                    anime = 0x107;
                                }
                                break;
                            case 2:
                                if (sh2jms.shotgun_dir == 1) {
                                    anime = 0x106;
                                } else {
                                    anime = 0x109;
                                }
                                break;
                            }
                            comp_type = 4;
                            sh2jms.hold_type = 1;
                            aip = (struct _AnimeInfo *)&pjames_sg_anim[anime - 250];
                        } else {
                            anime_on = 0;
                        }
                        break;
                    case 3:
                        anime_on = 0;
                        break;
                    case 4:
                        comp_type = 4;
                        aip = (struct _AnimeInfo *)&pjames_sp_anim[(sh2jms.hold_loop[1] ? 0x1F9 : 0x1FB) - 500];
                        break;
                    case 5:
                        if (sh2jms.hold_chg[1]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x169;
                            } else {
                                anime = 0x16A;
                            }
                            comp_type = 4;
                        } else {
                            if (sh2jms.hold_loop[1]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x168;
                                } else {
                                    anime = 0x167;
                                }
                                comp_type = 4;
                            } else {
                                if (sh2jms.atk_type == 3) {
                                    if (!sh2jms.hold_type) {
                                        anime = 0x16F;
                                    } else {
                                        anime = 0x175;
                                    }
                                    comp_type = 4;
                                } else {
                                    anime = 0;
                                    anime_on = 0;
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_ka_anim[anime - 350];
                        break;
                    case 6:
                        if (sh2jms.hold_chg[1]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x197;
                            } else {
                                anime = 0x198;
                            }
                            comp_type = 4;
                        } else {
                            if (sh2jms.hold_loop[1]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x196;
                                } else {
                                    anime = 0x195;
                                }
                                comp_type = 4;
                            } else {
                                if (sh2jms.atk_type == 3) {
                                    if (!sh2jms.hold_type) {
                                        anime = 0x19D;
                                    } else {
                                        anime = 0x1A3;
                                    }
                                    comp_type = 4;
                                } else {
                                    anime = 0;
                                    anime_on = 0;
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_pi_anim[anime - 400];
                        break;
                    case 8:
                        if (sh2jms.hold_chg[1]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x1C8;
                            } else {
                                anime = 0x1C7;
                            }
                            comp_type = 4;
                        } else {
                            if (sh2jms.hold_loop[1]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x1CA;
                                } else {
                                    anime = 0x1C9;
                                }
                                comp_type = 4;
                            } else {
                                anime = 0;
                                anime_on = 0;
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_na_anim[anime - 450];
                        break;
                    case 7:
                        if (sh2jms.hold_chg[1]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x233;
                            } else {
                                anime = 0x234;
                            }
                            comp_type = 4;
                        } else {
                            if (sh2jms.hold_loop[1]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x230;
                                } else {
                                    anime = 0x22F;
                                }
                                comp_type = 2;
                            } else {
                                anime = 0;
                                anime_on = 0;
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_cs_anim[anime - 550];
                        break;
                    }
                    if (anime_on) {
                        james_anim_set(aip, 2, comp_type);
                        player_flg_on(&sh2jms.l_anime_st_flg, 2);
                    }
                } else {
                    if (sh2jms.lower_prev == JMS_ST_L_RELEASE) {
                        comp_type = 6;
                    } else {
                        comp_type = 4;
                    }
                    switch (sh2jms.weapon) {
                    case 0:
                        break;
                    case 1:
                        if (l_anime_flg_on(0x40)) {
                            anime = 0xCE;
                        } else {
                            if (sh2jms.lock_on || sh2jms.hold_type == 1) {
                                anime = 0xCB;
                            } else {
                                anime = 0xC9;
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_hg_anim[anime - 200];
                        break;
                    case 2:
                        if (sh2jms.hold_chg[1] && sh2jms.shotgun_dir != sh2jms.shotgun_prev) {
                            switch (sh2jms.shotgun_prev) {
                            case 0:
                                if (sh2jms.shotgun_dir == 2) {
                                    anime = 0x10A;
                                } else {
                                    anime = 0x108;
                                }
                                break;
                            case 1:
                                if (sh2jms.shotgun_dir == 2) {
                                    anime = 0x105;
                                } else {
                                    anime = 0x107;
                                }
                                break;
                            case 2:
                                if (sh2jms.shotgun_dir == 1) {
                                    anime = 0x106;
                                } else {
                                    anime = 0x109;
                                }
                                break;
                            }
                        } else {
                            switch (sh2jms.shotgun_dir) {
                            case 0:
                                anime = 0x101;
                                break;
                            case 1:
                                anime = 0xFB;
                                break;
                            case 2:
                                anime = 0xFE;
                                break;
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_sg_anim[anime - 250];
                        break;
                    case 3:
                        aip = (struct _AnimeInfo *)&pjames_rg_anim[1];
                        break;
                    case 4:
                        aip = (struct _AnimeInfo *)&pjames_sp_anim[(sh2jms.hold_loop[1] ? 0x1F9 : 0x1F5) - 500];
                        break;
                    case 5:
                        if (sh2jms.hold_chg[1]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x169;
                            } else {
                                anime = 0x16A;
                            }
                        } else {
                            if (sh2jms.hold_loop[1]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x168;
                                } else {
                                    anime = 0x167;
                                }
                                comp_type = 4;
                            } else {
                                aip = shCharacterAnimeGetInfo_(sh2jms.player, 1);
                                anime = aip->name;
                                if (anime == 0x161 || anime == 0x168) {
                                    anime = 0x161;
                                } else {
                                    if (anime == 0x15F || anime == 0x167) {
                                        anime = 0x15F;
                                    } else {
                                        if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                                            anime = 0x15F;
                                        } else {
                                            anime = 0x161;
                                        }
                                    }
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_ka_anim[anime - 350];
                        break;
                    case 6:
                        if (sh2jms.hold_chg[1]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x197;
                            } else {
                                anime = 0x198;
                            }
                        } else {
                            if (sh2jms.hold_loop[1]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x196;
                                } else {
                                    anime = 0x195;
                                }
                                comp_type = 2;
                            } else {
                                aip = shCharacterAnimeGetInfo_(sh2jms.player, 1);
                                anime = aip->name;
                                if (anime == 0x193 || anime == 0x196) {
                                    anime = 0x193;
                                } else {
                                    if (anime == 0x191 || anime == 0x195) {
                                        anime = 0x191;
                                    } else {
                                        if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                                            anime = 0x191;
                                        } else {
                                            anime = 0x193;
                                        }
                                    }
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_pi_anim[anime - 400];
                        break;
                    case 8:
                        if (sh2jms.hold_chg[1]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x1C8;
                            } else {
                                anime = 0x1C7;
                            }
                        } else {
                            if (sh2jms.hold_loop[1]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x1CA;
                                } else {
                                    anime = 0x1C9;
                                }
                                comp_type = 4;
                            } else {
                                aip = shCharacterAnimeGetInfo_(sh2jms.player, 1);
                                anime = aip->name;
                                if (anime == 0x1C5 || anime == 0x1CA) {
                                    anime = 0x1C5;
                                } else {
                                    if (anime == 0x1C3 || anime == 0x1C9) {
                                        anime = 0x1C3;
                                    } else {
                                        if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                                            anime = 0x1C3;
                                        } else {
                                            anime = 0x1C5;
                                        }
                                    }
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_na_anim[anime - 450];
                        break;
                    case 7:
                        if (sh2jms.hold_chg[1]) {
                            if (!sh2jms.hold_type) {
                                anime = 0x233;
                            } else {
                                anime = 0x234;
                            }
                        } else {
                            if (sh2jms.hold_loop[1]) {
                                if (!sh2jms.hold_type) {
                                    anime = 0x230;
                                } else {
                                    anime = 0x22F;
                                }
                                comp_type = 2;
                            } else {
                                aip = shCharacterAnimeGetInfo_(sh2jms.player, 1);
                                switch (aip->name) {
                                case 0x22B:
                                    if (sh2jms.hold_type == -1) {
                                        anime = 0x22B;
                                    } else {
                                        anime = 0x22D;
                                    }
                                    break;
                                case 0x230:
                                    anime = 0x22D;
                                    break;
                                case 0x227:
                                    if (sh2jms.hold_type == -1) {
                                        anime = 0x227;
                                    } else {
                                        anime = 0x229;
                                    }
                                    break;
                                case 0x22F:
                                    anime = 0x229;
                                    break;
                                default:
                                    if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                                        if (sh2jms.hold_type == -1) {
                                            anime = 0x227;
                                        } else {
                                            anime = 0x229;
                                        }
                                    } else {
                                        if (sh2jms.hold_type == -1) {
                                            anime = 0x22B;
                                        } else {
                                            anime = 0x22D;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                        aip = (struct _AnimeInfo *)&pjames_cs_anim[anime - 550];
                        break;
                    }
                    james_anim_set(aip, 2, comp_type);
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                }
                break;
            }
            case JMS_ST_L_RELEASE: {
                int anime;
                int comp_type;

                switch (sh2jms.weapon) {
                case 0:
                    break;
                case 1:
                    if (sh2jms.lock_on || (sh2jms.lower_prev == JMS_ST_L_ATTACK && !sh2jms.reload)) {
                        anime = 0xCC;
                    } else {
                        anime = 0xCA;
                    }
                    aip = (struct _AnimeInfo *)&pjames_hg_anim[anime - 200];
                    break;
                case 2:
                    switch (sh2jms.shotgun_dir) {
                    case 0:
                        anime = 0x102;
                        break;
                    case 1:
                        anime = 0xFC;
                        break;
                    case 2:
                        anime = 0xFF;
                        break;
                    }
                    aip = (struct _AnimeInfo *)&pjames_sg_anim[anime - 250];
                    break;
                case 3:
                    anime = 0x12E;
                    aip = (struct _AnimeInfo *)&pjames_rg_anim[anime - 300];
                    break;
                case 4:
                    anime = 0x1F6;
                    aip = (struct _AnimeInfo *)&pjames_sp_anim[anime - 500];
                    break;
                case 5:
                    if (!sh2jms.hold_type) {
                        anime = 0x162;
                    } else {
                        anime = 0x160;
                    }
                    aip = (struct _AnimeInfo *)&pjames_ka_anim[anime - 350];
                    break;
                case 6:
                    if (!sh2jms.hold_type) {
                        anime = 0x194;
                    } else {
                        anime = 0x192;
                    }
                    aip = (struct _AnimeInfo *)&pjames_pi_anim[anime - 400];
                    break;
                case 8:
                    if (!sh2jms.hold_type) {
                        anime = 0x1C6;
                    } else {
                        anime = 0x1C4;
                    }
                    aip = (struct _AnimeInfo *)&pjames_na_anim[anime - 450];
                    break;
                case 7:
                    if (!sh2jms.hold_type) {
                        anime = 0x22E;
                    } else {
                        anime = 0x22A;
                    }
                    aip = (struct _AnimeInfo *)&pjames_cs_anim[anime - 550];
                    break;
                }
                if (sh2jms.weapon != 7) {
                    if (anime < scp_d->anime.anim_a->name) {
                        comp_type = 4;
                    } else {
                        comp_type = 6;
                    }
                } else {
                    comp_type = 4;
                }
                james_anim_set(aip, 2, comp_type);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            }
            case JMS_ST_L_ATTACK: {
                int anime;
                int comp_type;

                comp_type = 2;
                switch (sh2jms.weapon) {
                case 0:
                    break;
                case 1:
                    if (!sh2jms.shoot_val && sh2jms.reload_val) {
                        if (sh2jms.hold_type) {
                            anime = 0xD2;
                        } else {
                            anime = 0xD1;
                        }
                    } else {
                        if (sh2jms.hold_type == 1 || l_anime_flg_on(0x40)) {
                            anime = 0xCF;
                        } else {
                            anime = 0xD0;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_hg_anim[anime - 200];
                    break;
                case 2:
                    if (!sh2jms.shoot_val && sh2jms.reload_val) {
                        anime = 0x104;
                    } else {
                        switch (sh2jms.shotgun_dir) {
                        case 0:
                            anime = 0x103;
                            break;
                        case 1:
                            anime = 0xFD;
                            break;
                        case 2:
                            anime = 0x100;
                            break;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_sg_anim[anime - 250];
                    break;
                case 3:
                    if (!sh2jms.shoot_val && sh2jms.reload_val) {
                        anime = 0x130;
                    } else {
                        anime = 0x12F;
                    }
                    aip = (struct _AnimeInfo *)&pjames_rg_anim[anime - 300];
                    break;
                case 4:
                    if (!sh2jms.shoot_val && sh2jms.reload_val) {
                        if (sh2jms.lower_prev == JMS_ST_L_ATTACK) {
                            anime = 0x1FF;
                        } else {
                            anime = 0x1FE;
                        }
                    } else {
                        if (sh2jms.atk_count) {
                            anime = 0x1FD;
                        } else {
                            anime = 0x1FC;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_sp_anim[anime - 500];
                    break;
                case 5:
                    if (sh2jms.hold_type) {
                        switch (sh2jms.atk_type) {
                        case 3:
                            if (sh2jms.atk_count) {
                                anime = 0x16D;
                            } else {
                                anime = 0x16B;
                            }
                            break;
                        case 4:
                            anime = 0x177;
                            break;
                        }
                    } else {
                        switch (sh2jms.atk_type) {
                        case 3:
                            if (sh2jms.atk_count) {
                                anime = 0x173;
                            } else {
                                anime = 0x171;
                            }
                            break;
                        case 4:
                            anime = 0x17B;
                            break;
                        }
                    }
                    if (sh2jms.player->battle.atk_result) {
                        comp_type = 6;
                        switch (sh2jms.atk_type) {
                        case 3:
                            switch (scp_d->anime.anim_a->name) {
                            case 0x16B:
                            case 0x171:
                                scp_d->anime.total_count -= 0x4000;
                                break;
                            }
                            if (sh2jms.hold_type) {
                                anime = 0x16E;
                            } else {
                                anime = 0x174;
                            }
                            break;
                        case 4:
                            anime++;
                            break;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_ka_anim[anime - 350];
                    break;
                case 6:
                    if (sh2jms.hold_type) {
                        switch (sh2jms.atk_type) {
                        case 3:
                            if (sh2jms.atk_count) {
                                anime = 0x19B;
                            } else {
                                anime = 0x199;
                            }
                            break;
                        case 4:
                            anime = 0x1A5;
                            break;
                        case 5:
                            anime = 0x1AD;
                            break;
                        }
                    } else {
                        switch (sh2jms.atk_type) {
                        case 3:
                            if (sh2jms.atk_count) {
                                anime = 0x1A1;
                            } else {
                                anime = 0x19F;
                            }
                            break;
                        case 4:
                            anime = 0x1A9;
                            break;
                        case 5:
                            anime = 0x1AF;
                            break;
                        }
                    }
                    if (sh2jms.player->battle.atk_result) {
                        comp_type = 6;
                        switch (sh2jms.atk_type) {
                        case 3:
                            switch (scp_d->anime.anim_a->name) {
                            case 0x199:
                            case 0x19F:
                                scp_d->anime.total_count -= 0x6000;
                                break;
                            }
                            if (sh2jms.hold_type) {
                                anime = 0x19C;
                            } else {
                                anime = 0x1A2;
                            }
                            break;
                        case 4:
                        case 5:
                            anime++;
                            break;
                        }
                    }
                    aip = (struct _AnimeInfo *)&pjames_pi_anim[anime - 400];
                    break;
                case 8:
                    if (sh2jms.hold_type) {
                        switch (sh2jms.atk_type) {
                        case 3:
                            anime = 0x1CB;
                            break;
                        case 4:
                            anime = 0x1CF;
                            break;
                        }
                    } else {
                        switch (sh2jms.atk_type) {
                        case 3:
                            anime = 0x1CD;
                            break;
                        case 4:
                            anime = 0x1D1;
                            break;
                        }
                    }
                    if (sh2jms.player->battle.atk_result) {
                        comp_type = 6;
                        anime++;
                    }
                    aip = (struct _AnimeInfo *)&pjames_na_anim[anime - 450];
                    break;
                case 7:
                    if (sh2jms.hold_type) {
                        switch (sh2jms.atk_type) {
                        case 3:
                            anime = 0x235;
                            break;
                        case 5:
                            anime = 0x239;
                            break;
                        }
                    } else {
                        switch (sh2jms.atk_type) {
                        case 3:
                            anime = 0x237;
                            break;
                        case 5:
                            anime = 0x23B;
                            break;
                        }
                    }
                    if (sh2jms.player->battle.atk_result) {
                        comp_type = 6;
                        anime++;
                    }
                    aip = (struct _AnimeInfo *)&pjames_cs_anim[anime - 550];
                    break;
                }
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_TIRED: {
                int comp_type;
                int anime;

                if (anime_change_check_lower == sh2jms.lower_now) {
                    comp_type = 10;
                    anime = 0x71;
                    player_flg_on(&sh2jms.l_anime_st_flg, 2);
                } else {
                    comp_type = 4;
                    if (sh2jms.tired >= sh2jms.tired_max * 2 / 3) {
                        anime = 0x72;
                    } else {
                        anime = 0x71;
                    }
                    player_flg_on(&sh2jms.l_anime_st_flg, 4);
                }
                aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                james_anim_set(aip, 2, comp_type);
                break;
            }
            case JMS_ST_L_KICK: {
                int anime;

                if (sh2jms.atk_type == 7) {
                    anime = 0x85;
                } else {
                    anime = 0x84;
                }
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                james_anim_set(aip, 2, 4);
                break;
            }
            case JMS_ST_L_WALL_F: {
                int anime;

                switch (sh2jms.lower_prev) {
                case JMS_ST_L_RUN1:
                case JMS_ST_L_RUN2:
                case JMS_ST_L_RUN3:
                    anime = 0x80;
                    break;
                case JMS_ST_L_RSRUN:
                    anime = 0x81;
                    break;
                case JMS_ST_L_LSRUN:
                    anime = 0x82;
                    break;
                default:
                    /* Matching: #line puts the asserts below on their original source lines. */
#line 5241
                    assert(0);
                    break;
                }
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                james_anim_set(aip, 2, 4);
                break;
            }
            case JMS_ST_L_FALL:
                aip = (struct _AnimeInfo *)&pjames_anim[sh2jms.fall_type + 11];
                james_anim_set(aip, 2, 4);
                player_flg_on(&sh2jms.l_anime_st_flg, 2);
                break;
            case JMS_ST_L_DAMAGE: {
                int anime;

                switch (sh2jms.hug_status) {
                case 0:
                case 1:
                    anime = sh2jms.damage_no;
                    break;
                case 2:
                    switch (sh2jms.player->battle.id) {
                    case 0x2F:
                    case 0x30:
                        anime = 0x4E3E;
                        break;
                    case 0x36:
                    case 0x37:
                        anime = 0x4E47;
                        break;
                    case 0x39:
                    case 0x3A:
                        if (sh2jms.hug_dir) {
                            anime = 0x4E4F;
                        } else {
                            anime = 0x4E4B;
                        }
                        break;
                    case 0x3B:
                    case 0x3C:
                        anime = 0x4E56;
                        break;
                    case 0x3E:
                    case 0x3F:
                        anime = (shRandI() & 1) ? JMS_HUG_ANIME_4E54 : JMS_HUG_ANIME_4E55;
                        break;
                    }
                    break;
                case 3:
                case 4:
                    switch (sh2jms.player->battle.id) {
                    case 0x30:
                    case 0x2F:
                        anime = sh2jms.dead ? JMS_HUG_ANIME_4E3F : JMS_HUG_ANIME_4E40;
                        break;
                    case 0x37:
                    case 0x36:
                        anime = sh2jms.dead ? JMS_HUG_ANIME_4E48 : JMS_HUG_ANIME_4E49;
                        break;
                    case 0x3A:
                    case 0x39:
                        if (sh2jms.dead) {
                            if (sh2jms.hug_dir) {
                                anime = 0x4E51;
                            } else {
                                anime = 0x4E4D;
                            }
                        } else {
                            if (sh2jms.hug_dir) {
                                anime = 0x4E50;
                            } else {
                                anime = 0x4E4C;
                            }
                        }
                        break;
                    case 0x3C:
                    case 0x3B:
                        anime = sh2jms.dead ? JMS_HUG_ANIME_4E57 : JMS_HUG_ANIME_4E59;
                        break;
                    case 0x3E:
                    case 0x3F:
                        anime = sh2jms.dead ? JMS_HUG_ANIME_4E27 : JMS_HUG_ANIME_4E53;
                        break;
                    }
                    break;
                }
                aip = PlayerGetStageAnime(anime);
                /* Matching: #line puts the asserts below on their original source lines. */
#line 5341
                assert(aip);
                james_anim_set(aip, 2, 2);
                break;
            }
            case JMS_ST_L_EVENT: {
                int anime;
                int comp_type;

                if (sh2jms.event_move_mode) {
                    switch (sh2jms.event_status_now) {
                    case 0:
                        anime = 0x65;
                        comp_type = 4;
                        break;
                    case 1:
                        anime = 0x68;
                        comp_type = (sh2jms.event_status_prev == 2) ? ANIME_COMP_10 : ANIME_COMP_4;
                        break;
                    case 2:
                        anime = 0x6B;
                        comp_type = (sh2jms.event_status_prev == 1) ? ANIME_COMP_10 : ANIME_COMP_4;
                        break;
                    }
                    if (comp_type == 4) {
                        player_flg_on(&sh2jms.l_anime_st_flg, 2);
                    } else {
                        player_flg_on(&sh2jms.l_anime_st_flg, 4);
                    }
                    aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                    sh2jms.event_anime = anime;
                } else {
                    anime = sh2jms.event_anime & 0x7FFFFFFF;
                    if (sh2jms.event_anime & 0x80000000) {
                        comp_type = 2;
                    } else {
                        comp_type = 4;
                        player_flg_on(&sh2jms.l_anime_st_flg, 2);
                    }
                    if (anime == 0x65) {
                        aip = (struct _AnimeInfo *)&pjames_anim[anime - 100];
                    } else {
                        aip = PlayerGetStageAnime(anime);
                        /* Matching: #line puts the asserts below on their original source lines. */
#line 5390
                        assert_dw(aip); /* Matching: do/while(0) form (its nop) */
                    }
                }
                james_anim_set(aip, 2, comp_type);
                break;
            }
            }
            anime_change_check_lower = sh2jms.lower_now;
            player_flg_off(&sh2jms.l_anime_st_flg, 0x40);
        }
    }
    if (scp_d->anime.comp_type < 3 || scp_d->anime.comp_type > 6) {
        player_flg_off(&sh2jms.l_anime_st_flg, 2);
    }
    if (scp_d->anime.comp_type <= 8) {
        player_flg_off(&sh2jms.l_anime_st_flg, 4);
    }
    if (scp_d->anime.comp_type == -1) {
        if (!(sh2jms.anime_pause & 4)) {
            sh2jms.anime_pause |= 1;
        }
    } else {
        sh2jms.anime_pause &= 0xA;
    }
    sh2jms.hold_chg[1] = 0;
    sh2jms.hold_loop[1] = 0;
}


static void PlayerCheckAnime(void) {
    PlayerCheckAnimeLower();
    PlayerCheckAnimeUpper();
}

/** Picks and starts James's rowing animation from the boat's state. */
void BoatPlayerCheckAnime(void) {
    struct SubCharacterDisp *scp_d;
    struct _AnimeInfo *aip;

    scp_d = (struct SubCharacterDisp *)sh2jms.player;
    if (sh2bot.anim_change) {
        switch (sh2bot.status_now) {
        case 0:
            aip = PlayerGetStageAnime(0x4E24);
            break;
        case 1:
            aip = PlayerGetStageAnime(0x4E25);
            break;
        case 2:
            aip = PlayerGetStageAnime(0x4E26);
            break;
        }
        james_anim_set(aip, 2, 10);
        james_anim_set(aip, 1, 10);
        player_flg_on(&sh2jms.l_anime_st_flg, 4);
        player_flg_on(&sh2jms.u_anime_st_flg, 4);
        player_flg_off(&sh2jms.l_anime_st_flg, 0x40);
        player_flg_off(&sh2jms.u_anime_st_flg, 0x40);
    }
    if (scp_d->anime.comp_type <= 8) {
        player_flg_off(&sh2jms.l_anime_st_flg, 4);
        player_flg_off(&sh2jms.u_anime_st_flg, 4);
    } else {
        shCharacterAnimeSpeedAddY_(sh2jms.player, 2, 0x40);
        shCharacterAnimeSpeedAddY_(sh2jms.player, 1, 0x40);
    }
}

static void PlayerFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;
    float pos[4];
    float rot[4];

    switch (this->step) {
    case 0:
        vcopy(&this->pos, pos);
        vcopy(&this->rot, rot);
        PlayerCheckInit(this);
        if (this->status & 0x20000) {
            if (this->status & 4) {
                aip = PlayerGetStageAnime(0x4E24);
            } else {
                aip = (struct _AnimeInfo *)&pjames_demo_anim[1];
            }
        } else {
            if (this->status & 4) {
                aip = (struct _AnimeInfo *)&pjames_anim[1];
            } else {
                aip = (struct _AnimeInfo *)&pjames_demo_anim[1];
            }
        }
        james_anim_set_all(aip, 0);
        vcopy(pos, &this->pos);
        vcopy(rot, &this->rot);
        shCharacterSetPosAfterDemo(this, (float *)&this->pos, this->rot.y);
        sh2jms.dist_pos.y = this->grnd_height = this->pos.y;
        PlayerCheckSetParameterPhase2(this);
        this->step++;
        break;
    case 1:
        if (this->status & 4) {
            if (this->status & 0x2000) {
                sh2jms.tired = 0;
                sh2jms.running_time = 0.0f;
            } else {
                PlayerCheckKeyInput();
                PlayerCheckDamage(this);
                PlayerCheckSetParameterPhase1(this);
                if (shCharacterGetSubCharacter(0x10B, -1) && (this->status & 0x20000)) {
                    PlayerCheckModelParts();
                } else {
                    switch (playing.control_type) {
                    case 0:
                        PlayerCheckControl3D(this);
                        break;
                    case 1:
                        PlayerCheckControl2D(this);
                        break;
                    }
                    PlayerCheckAnime();
                    PlayerCheckModelParts();
                    switch (playing.control_type) {
                    case 0:
                        PlayerUpdatePosition3D(this);
                        break;
                    case 1:
                        PlayerUpdatePosition2D(this);
                        break;
                    }
                    SCAddPos(this, &sh2jms.pos);
                    PlayerCheckSetParameterPhase2(this);
                    PlayerCheckEffect();
                    PlayerCheckSound();
                    PlayerCheckDualShock();
                }
            }
        }
        break;
    case 2:
        break;
    }
}

/** Gets the flashlight position and direction (as adjusted for the body). */
void shGetJamesLightPos(float *pos, float *vec) {
    vcopy(sh2jms.light_pos_revise, pos);
    vcopy(sh2jms.light_vec_revise, vec);
}

/** Gets the flashlight position and direction (unadjusted). */
void shGetJamesLightPosOriginal(float *pos, float *vec) {
    vcopy(sh2jms.light_pos, pos);
    vcopy(sh2jms.light_vec, vec);
}

static float __stripped_float_code_chest(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f + 53.0f + 55.0f + 57.0f + 59.0f + 61.0f + 63.0f + 65.0f + 67.0f + 69.0f + 71.0f; } /* Matching: dead-stripped float-constant stand-in (constcount.py); fitted, not recovered: 35 constants */
/* @bug stores light_pos_revise[3] twice and never light_vec_revise[3] (as in the original binary). */
static void shGetJamesLightPos_Calc_Chest(void) {
    struct SubCharacter *p;
    struct shSkelton *top;
    struct FMAT lw_mat;
    struct FMAT light_mat;
    struct FVEC pos0;
    struct FVEC pos1;
    int i1;

    p = sh2jms.player;
    if (p->status & 0x2000) {
        p = shCharacterGetSubCharacter(0x103, -1);
        if (!p) {
            p = shCharacterGetSubCharacter(0x102, -1);
        }
        if (!p) {
            p = sh2jms.player;
        }
    }
    if (p) {
        top = p->sk_top;
        lw_mat = p->mat;
        for (i1 = 0; i1 < 3; i1++) {
            top = top->next;
        }
        light_mat = *(struct FMAT *)&top->src_m;
        light_mat.d[3][0] = light_mat.d[3][1] = light_mat.d[3][2] = 0.0f;
        light_mat.d[3][3] = 1.0f;
        pos0.x = 60.98f;
        pos0.y = -78.37f;
        pos0.z = 94.97f;
        pos0.w = 0.0f;
        sceVu0ApplyMatrix((float *)&pos0, (float (*)[4])&light_mat, (float *)&pos0);
        pos0.x += top->src_m.d[3][0];
        pos0.y += top->src_m.d[3][1];
        pos0.z += top->src_m.d[3][2];
        pos0.w = top->src_m.d[3][3];
        sceVu0ApplyMatrix(sh2jms.light_pos, (float (*)[4])&lw_mat, (float *)&pos0);
        pos1.x = -9.0f;
        pos1.y = -34.0f;
        pos1.z = 1117.0f;
        pos1.w = 0.0f;
        sceVu0ApplyMatrix((float *)&pos1, (float (*)[4])&light_mat, (float *)&pos1);
        pos1.x += top->src_m.d[3][0];
        pos1.y += top->src_m.d[3][1];
        pos1.z += top->src_m.d[3][2];
        pos1.w = 1.0f;
        vcopy(&pos1, sh2jms.light_vec);
        if (PlayerSearchVIewButtonCheck()) {
            sh2jms.light_vec_neck[2] = -PlayerGetNeckAngleX();
            sh2jms.light_vec_neck[3] = PlayerGetNeckAngleY();
        } else {
            sh2jms.light_vec_neck[2] = 0.0f;
            sh2jms.light_vec_neck[3] = 0.0f;
        }
        close_to_angle_target(&sh2jms.light_vec_neck[0], sh2jms.light_vec_neck[2], -3.1415927f, 3.1415927f, 30.0f);
        close_to_angle_target(&sh2jms.light_vec_neck[1], sh2jms.light_vec_neck[3], -3.1415927f, 3.1415927f, 30.0f);
        {
            float neck[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            float m[4][4];
            float mat[4][4];

            neck[0] = -sh2jms.light_vec_neck[0];
            neck[1] = sh2jms.light_vec_neck[1];
            _sceVu0UnitMatrix(m);
            shRotMatrixZ(mat, m, neck[2]);
            shRotMatrixX(mat, mat, neck[0]);
            shRotMatrixY(mat, mat, neck[1]);
            _sceVu0ApplyMatrix(sh2jms.light_vec, mat, sh2jms.light_vec);
        }
        sceVu0ApplyMatrix(sh2jms.light_vec, (float (*)[4])&lw_mat, sh2jms.light_vec);
        sh2jms.light_vec[0] -= sh2jms.light_pos[0];
        sh2jms.light_vec[1] -= sh2jms.light_pos[1];
        sh2jms.light_vec[2] -= sh2jms.light_pos[2];
        sceVu0Normalize(sh2jms.light_vec, sh2jms.light_vec);
        {
            float mat[4][4];
            float vec[4];
            float pos_x;
            float pos_z;

            GetPlayerPartsWorldMatrix(mat, 8);
            if (sh2jms.player->status & 0x2000) {
                pos_x = p->pos.x;
                pos_z = p->pos.z;
                vcopy(sh2jms.light_vec, vec);
            } else {
                pos_x = sh2jms.column_mov.p[0][0];
                pos_z = sh2jms.column_mov.p[0][2];
                vec[0] = shSinF(p->rot.y);
                vec[2] = shCosF(p->rot.y);
                vec[1] = sh2jms.light_vec[1];
                vec[3] = 0.0f;
                _shNormalize(vec, vec);
            }
            sh2jms.light_pos_revise[0] = pos_x + 500.0f * (0.35f - 0.05f) * shSinF(3.1415927f + p->rot.y);
            sh2jms.light_pos_revise[2] = pos_z + 500.0f * (0.35f - 0.05f) * shCosF(3.1415927f + p->rot.y);
            sh2jms.light_pos_revise[1] = 50.0 + mat[3][1];
            sh2jms.light_pos_revise[3] = 1.0f;
            if (sh2jms.hold_type == -1) {
                if (sh2jms.light_vec_inner_rate < 0.75f) {
                    sh2jms.light_vec_inner_rate += shGetDT();
                }
            } else {
                if (sh2jms.light_vec_inner_rate > 0.25f) {
                    sh2jms.light_vec_inner_rate -= shGetDT();
                }
            }
            sh2jms.light_vec_inner_rate = (sh2jms.light_vec_inner_rate < 0.25f) ? 0.25f : ((sh2jms.light_vec_inner_rate > 0.75f) ? 0.75f : sh2jms.light_vec_inner_rate);
            sh2jms.light_vec_revise[0] = vec[0] * (1.0f - sh2jms.light_vec_inner_rate) + sh2jms.light_vec[0] * sh2jms.light_vec_inner_rate;
            sh2jms.light_vec_revise[2] = vec[2] * (1.0f - sh2jms.light_vec_inner_rate) + sh2jms.light_vec[2] * sh2jms.light_vec_inner_rate;
            sh2jms.light_vec_revise[1] = vec[1] * (1.0f - sh2jms.light_vec_inner_rate) + sh2jms.light_vec[1] * sh2jms.light_vec_inner_rate;
            sh2jms.light_pos_revise[3] = 1.0f;
        }
    }
}
static void shGetJamesLightPos_Calc_Hand(void) {
    struct SubCharacter *p;
    struct shSkelton *top;
    struct FMAT lw_mat;
    struct FMAT light_mat;
    struct FVEC pos0;
    struct FVEC pos1;
    float j_light_vec[4] = { 0.0f, 500.0f, 0.0f, 0.0f };
    static float xv = 0.0f;
    static float yv = 0.0f;
    static float zv = 500.0f;
    int pad;
    float xp;
    float yp;
    float zp;

    pad = shPadGetPort();
    xp = -15.0f;
    yp = 0.0f;
    zp = 13.0f;
    p = shCharacterGetSubCharacter(0x418, -1);
    if (!p) {
        p = shCharacterGetSubCharacter(0x705, -1);
    }
    if (p) {
        top = p->sk_top;
        lw_mat = p->mat;
        light_mat = *(struct FMAT *)&top->src_m;
        light_mat.d[3][0] = light_mat.d[3][1] = light_mat.d[3][2] = 0.0f;
        light_mat.d[3][3] = 1.0f;
        pos0.x = xp;
        pos0.y = yp;
        pos0.z = zp;
        pos0.w = 0.0f;
        sceVu0ApplyMatrix((float *)&pos0, (float (*)[4])&light_mat, (float *)&pos0);
        pos0.x += top->src_m.d[3][0];
        pos0.y += top->src_m.d[3][1];
        pos0.z += top->src_m.d[3][2];
        pos0.w = top->src_m.d[3][3];
        sceVu0ApplyMatrix(sh2jms.light_pos, (float (*)[4])&lw_mat, (float *)&pos0);
        pos1.x = xv;
        pos1.y = yv;
        pos1.z = zv;
        pos1.w = 0.0f;
        sceVu0ApplyMatrix((float *)&pos1, (float (*)[4])&light_mat, (float *)&pos1);
        pos1.x += top->src_m.d[3][0];
        pos1.y += top->src_m.d[3][1];
        pos1.z += top->src_m.d[3][2];
        pos1.w = 1.0f;
        vcopy(&pos1, sh2jms.light_vec);
        sceVu0ApplyMatrix(sh2jms.light_vec, (float (*)[4])&lw_mat, sh2jms.light_vec);
        sh2jms.light_vec[0] -= sh2jms.light_pos[0];
        sh2jms.light_vec[1] -= sh2jms.light_pos[1];
        sh2jms.light_vec[2] -= sh2jms.light_pos[2];
        sceVu0Normalize(sh2jms.light_vec, sh2jms.light_vec);
    }
}

/** Updates the flashlight position from James's hand (status 0x8000) or chest. */
void shGetJamesLightPos_Calc(void) {
    if (sh2jms.player->status & 0x8000) {
        shGetJamesLightPos_Calc_Hand();
    } else {
        shGetJamesLightPos_Calc_Chest();
    }
}

/** Gets the world matrix of James's skeleton node @p parts_name. */
void GetPlayerPartsMatrixForCameraCtrl(float (*mat)[4], unsigned int parts_name) {
    int i1;
    struct shSkelton *sk;

    sk = sh2jms.player->sk_top;
    for (i1 = 0; i1 < parts_name; i1++) {
        sk = sk->next;
    }
    sceVu0MulMatrix(mat, (float (*)[4])&sh2jms.player->mat, (float (*)[4])&sk->src_m);
}

/** Gets the world matrix of James's skeleton node @p parts_name. */
void GetPlayerPartsWorldMatrix(float (*mat)[4], unsigned int parts_name) {
    GetPlayerPartsMatrixForCameraCtrl(mat, parts_name);
}

/** Gets the local matrix of James's skeleton node @p parts_name. */
void GetPlayerPartsLocalMatrix(struct FMAT *dest, int parts_name) {
    int i1;
    struct SubCharacter *p;
    struct shSkelton *sk;

    p = sh2jms.player;
    if (p->status & 0x2000) {
        p = shCharacterGetSubCharacter(0x103, -1);
        if (!p) {
            p = shCharacterGetSubCharacter(0x102, -1);
        }
        if (!p) {
            p = sh2jms.player;
        }
    }
    if (p) {
        sk = p->sk_top;
        for (i1 = 0; i1 < parts_name; i1++) {
            sk = sk->next;
        }
        *dest = *(struct FMAT *)&sk->src_m;
    }
}

/** Returns James's position/rotation block (from pos on) for the camera code. */
struct shCharaInfo *GetPlayerInfoForCameraCtrl(void) {
    return (struct shCharaInfo *)&sh2jms.player->pos;
}

/** Clears the player work area sh2jms. */
void shCharacterPlayerWorkInitAtPowerOn(void) {
    shQzero(&sh2jms, sizeof(sh2jms));
}

/** Sets James's starting HP (by battle level), stamina and equipment state. */
void shCharacterPlayerWorkInitAtGameStart(void) {
    switch (playing.battle_level) {
    case 0:
    case 1:
        sh2jms.hp = sh2jms.hp_max = 200.0f;
        break;
    case 2:
    case 3:
        sh2jms.hp = sh2jms.hp_max = 100.0f;
        break;
    }
    sh2jms.tired_max = 600;
    sh2jms.light_vec_inner_rate = 0.5f;
    sh2jms.spray_set = 200;
    sh2jms.spray_time = 20.0f;
    sh2jms.hold_type = -1;
    actwithwep_flg_set(0, &sh2jms);
    sh2jms.allbody_now = 0;
    sh2jms.upper_now = 0;
    sh2jms.lower_now = 0;
    sh2jms.allbody_prev = 0xFF;
    sh2jms.upper_prev = 0xFF;
    sh2jms.lower_prev = 0xFF;
    sh2jms.event_status_now = 0xFF;
    sh2jms.event_status_prev = 0xFF;
    player_flg_on(&sh2jms.lower_st_flg, 1);
    player_flg_on(&sh2jms.upper_st_flg, 1);
    sh2jms.column_mov.kind = sh2jms.column_atk.kind = 1;
    sh2jms.column_mov.weight = sh2jms.column_atk.weight = 2;
    sh2jms.column_mov.material = sh2jms.column_atk.material = 6;
    sh2jms.column_mov.shape = sh2jms.column_atk.shape = 3;
}

/** Installs James's update function and makes @p scp the player, with sh2jms's HP. */
void shCharacterSetPlayerLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, PlayerFunction);
    sh2jms.player = scp;
    sh2jms.player->battle.hp = sh2jms.hp;
    sh2jms.player->battle.hp_max = sh2jms.hp_max;
}

/** Rebuilds James's target list and drops a target that is no longer visible or alive. */
void PlayerGetTargetInfo(void) {
    sh2jms.enemy_atk_area = shBattleCheckTargetChara(sh2jms.player);
    sh2jms.enemy_around = shBattleAroundTargetEnemy();
    sh2jms.look_tgt = (struct SubCharacter *)shBattleGetTargetChara(sh2jms.player, 0);
    if (sh2jms.target) {
        if (!sh2jms.look_tgt || (sh2jms.target->battle.status & 2)) {
            sh2jms.target = NULL;
        }
    }
    sh2jms.enemy_liedown = shBattleGetNearDeadlyTargetEnemy(sh2jms.player);
}

/** Picks a target enemy if James has none and one is in range. */
void PlayerGetTarget(void) {
    if (sh2jms.enemy_atk_area) {
        if (!sh2jms.target) {
            sh2jms.target = shBattleGetTargetEnemy(sh2jms.player);
        }
    }
}

/** Switches James's target to the next enemy to one side. @param key 1 or -1 (see shBattleChangeTargetEnemy). */
void PlayerChangeTarget(int key) {
    if (!sh2jms.target) {
        /* Matching: #line puts the asserts below on their original source lines. */
#line 7311
        assert_dw(0); /* Matching: do/while(0) form (its nop) */
    }
    if (sh2jms.enemy_atk_area) {
        sh2jms.target = shBattleChangeTargetEnemy(sh2jms.player, key);
    }
}

#define NOW_ACTION_LEVEL_NUM 5

/** Handles James being held by an enemy: escape timing by battle level and weapon. */
void PlayerCheckHuggingAttack(void) {
    int status;
    float hurihodoki_timer[5][5] = {
        { 0.07f, 0.08f, 0.1f, 0.13f, 0.1f },
        { 0.35f, 0.4f, 0.5f, 0.65f, 0.5f },
        { 0.7f, 0.8f, 1.0f, 1.3f, 1.0f },
        { 1.4f, 1.6f, 2.0f, 2.6f, 2.0f },
        { 1.4f, 1.6f, 2.0f, 2.6f, 2.0f },
    };

    switch (sh2jms.player->battle.id) {
    case 0x2F:
        status = 0;
        break;
    case 0x36:
        status = 1;
        break;
    case 0x39:
        status = 2;
        break;
    case 0x3B:
        status = 3;
        break;
    case 0x3E:
        status = 4;
        break;
    default:
        return;
    }
    sh2jms.hug_status = 1;
    /* Matching: #line puts the asserts below on their original source lines. */
#line 7365
    assert(playing.battle_level <= NOW_ACTION_LEVEL_NUM);
    sh2jms.hugging_gauge = hurihodoki_timer[playing.battle_level][status];
}

/** Returns whether James guards the incoming enemy attack (guard button, facing the attacker). */
int PlayerChectGuardSuccess(void) {
    float roty;
    int ac_level;
    int guard_success;

    guard_success = 0;
    if (!shBattleCheckAttackByEnemy()) {
        sh2jms.guard_check = 0;
        return 0;
    }
    switch (playing.battle_level) {
    case 0:
    case 1:
        ac_level = 1;
        break;
    case 2:
    case 3:
        ac_level = 2;
        break;
    }
    switch (ac_level) {
    case 1:
        if (sh2jms.pad[0].dash || shPadPress(0, key_config.dash)) {
            sh2jms.guard_check = 1;
        } else {
            sh2jms.guard_check = 0;
        }
        break;
    case 2:
        if (sh2jms.guard_check) {
            if (!shPadPress(0, key_config.dash)) {
                sh2jms.guard_check = 0;
            }
        } else {
            if (shPadTrigger(0, key_config.dash)) {
                sh2jms.guard_check = 1;
            }
        }
        break;
    }
    if (sh2jms.player->battle.target) {
        if (sh2jms.guard_check) {
            if (sh2jms.upper_now <= JMS_ST_U_RTURN && sh2jms.upper_now != JMS_ST_U_WALL_F) {
                switch (sh2jms.player->battle.id) {
                case 0x24:
                case 0x25:
                    switch (ac_level) {
                    case 1:
                        guard_success = 1;
                        break;
                    case 2:
                        guard_success = 0;
                        break;
                    }
                    break;
                case 0x26:
                case 0x27:
                case 0x28:
                case 0x29:
                case 0x2A:
                case 0x2B:
                case 0x2E:
                case 0x35:
                case 0x38:
                case 0x3D:
                    guard_success = 1;
                    break;
                }
            }
        }
    }
    if (guard_success) {
        roty = sh2jms.player->battle.target->rot.y - sh2jms.player->rot.y;
        roty = ANGLE_WRAP(roty);
        if (fabsf(roty) > 1.5707964f) {
            return 1;
        }
    }
    return 0;
}

/* Matching: fitted float-constant stand-in, not recovered code (docs/stand-ins.md). */
static float __stripped_float_code_3(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f; }
/** Reserves attack slot @p num from the attack buttons, per weapon (ammo and reload checked for guns). */
void PlayerRequestAttack(struct shPlayerWork *w, int num) {
    unsigned char attack;
    unsigned char atk2;
    unsigned char atk1;

    atk2 = w->pad[0].attack2;
    atk1 = w->pad[0].attack1;
    attack = (atk2) ? atk2 : atk1;
    if (!w->atk_reserve[num]) {
        if (attack) {
            switch (w->weapon) {
            case 0:
                /* Matching: #line puts the asserts below on their original source lines. */
#line 7495
                assert(0);
                break;
            case 1:
                if (!sh2jms.shoot_val && !sh2jms.reload_val) {
                    if (atk2) {
                        SeCallPos(0x2B16, 0.5f, (float *)&w->player->pos, 0);
                    }
                } else {
                    w->atk_reserve[num] = w->lock_on ? 2 : 1;
                    if (num) {
                        sh2jms.hold_type = 1;
                    }
                }
                break;
            case 2:
            case 3:
                if (!sh2jms.shoot_val && !sh2jms.reload_val) {
                    if (atk2) {
                        SeCallPos((w->weapon == 2) ? 0x2B2C : 0x2B29, 0.5f, (float *)&w->player->pos, 0);
                    }
                } else {
                    w->atk_reserve[num] = 2;
                }
                break;
            case 4:
                w->atk_reserve[num] = 2;
                break;
            case 5:
                if (w->atk_count) {
                    w->atk_reserve[num] = 3;
                } else {
                    w->atk_reserve[num] = (attack == 1 || num) ? 3 : 4;
                }
                break;
            case 8:
                w->atk_reserve[num] = (attack == 1) ? 3 : 4;
                break;
            case 6:
                if ((w->pad[0].dash || w->atk_count || num) && playing.battle_level >= 3) {
                    w->atk_reserve[num] = 3;
                } else {
                    w->atk_reserve[num] = (attack == 1) ? 5 : 4;
                }
                break;
            case 7:
                if (sh2jms.atk_count) {
                    w->atk_reserve[num] = 5;
                } else {
                    w->atk_reserve[num] = (attack > 0 && attack <= 2) ? 3 : 5;
                }
                break;
            }
        }
    }
}

/** Reserves a finishing attack from the attack buttons, per weapon. */
void PlayerRequestAttackFinish(struct shPlayerWork *w) {
    unsigned char attack;
    unsigned char atk2;
    unsigned char atk1;

    atk2 = w->pad[0].attack2;
    atk1 = w->pad[0].attack1;
    attack = atk2 ? atk2 : atk1;
    if (attack) {
        if (!w->atk_reserve[0]) {
            if (attack == 3) {
                w->atk_reserve[0] = 7;
            } else {
                w->atk_reserve[0] = 6;
            }
        }
    }
}

/**
 * Casts eye- and foot-height rays 900 units ahead along direction @p spd_roty and stops James
 * running if a wall is within 250 units.
 */
void PlayerCheckStraightLine(struct SubCharacter *this, float spd_roty) {
    float sp[4];
    float ep[4];
    float roty;
    float me[4];
    float wall[4];

    sp[0] = this->pos.x;
    sp[2] = this->pos.z;
    if (!playing.control_type) {
        roty = this->rot.y + spd_roty;
    } else {
        roty = spd_roty;
    }
    sh2jms.rot_y = roty;
    ep[0] = sp[0] + 900.0f * shSinF(roty);
    ep[2] = sp[2] + 900.0f * shCosF(roty);
    sp[1] = ep[1] = this->eye_y;
    clCheckHitEyes(&this->eye, (unsigned int)this, sp, ep, 0);
    sp[1] = -150.0f + this->pos.y;
    ep[1] = -150.0f + this->pos.y + 900.0f * shSinF(-0.5235988f) / shCosF(-0.5235988f);
    clCheckHitEyes(&sh2jms.foot, (unsigned int)this, sp, ep, 0);
    if (this->eye.kind == 1 || sh2jms.foot.kind == 1) {
        me[0] = ep[0] - sp[0];
        me[1] = ep[1] - sp[1];
        me[2] = ep[2] - sp[2];
        me[3] = 1.0f;
        vcopy(sh2jms.foot.hobj.wall.nl, wall);
        _shNormalize(me, me);
        _shNormalize(wall, wall);
        sh2jms.inner_to_wall = _shInnerProduct(me, wall);
        sh2jms.dist_to_wall = distXZ(sh2jms.foot.hobj.wall.cp, sp);
        if (sh2jms.dist_to_wall < 250.0f) {
            sh2jms.cannot_run = 1;
            player_flg_off(&sh2jms.lower_st_flg, 0x20000);
        }
    }
}


static void shGetJamesFootPos(float *pos, float *vec, int kind) {
    float pos0[4];
    float pos1[4];
    float vec0[4];
    struct FMAT lw_mat;
    struct FMAT mat;
    struct shSkelton *stp;
    int i;
    int sk_num;
    float wep_range_test[2][4] = {
        { 400.0f, 80.0f, 540.0f, 0.0f },
        { 677.0f, -10.0f, 79.0f, 0.0f },
    };

    if (kind) {
        sk_num = 26;
    } else {
        sk_num = 30;
    }
    lw_mat = sh2jms.player->mat;
    stp = sh2jms.player->sk_top;
    for (i = 0; i < sk_num; i++) {
        stp = stp->next;
    }
    mat = *(struct FMAT *)&stp->src_m;
    vcopy(stp->src_m.d[3], pos0);
    mat.d[3][0] = mat.d[3][1] = mat.d[3][2] = 0.0f;
    mat.d[3][3] = 1.0f;
    sceVu0ApplyMatrix(pos0, (float (*)[4])&lw_mat, pos0);
    vcopy(pos0, pos);
    vcopy(wep_range_test[kind], pos1);
    sceVu0ApplyMatrix(pos1, (float (*)[4])&mat, pos1);
    pos1[0] += (stp + 1)->src_m.d[3][0];
    pos1[1] += (stp + 1)->src_m.d[3][1];
    pos1[2] += (stp + 1)->src_m.d[3][2];
    pos1[3] = (stp + 1)->src_m.d[3][3];
    sceVu0ApplyMatrix(pos1, (float (*)[4])&lw_mat, pos1);
    _shSubVector(vec0, pos1, pos0);
    _shNormalize(vec, vec0);
}

/** Gets the start point and direction of James's kick. */
void shGetJamesKickStartPos(float *pos, float *vec) {
    shGetJamesFootPos(pos, vec, 0);
}

/** Gets the start point and direction of James's stomp. */
void shGetJamesTrampStartPos(float *pos, float *vec) {
    shGetJamesFootPos(pos, vec, 1);
}

/** Returns the item number of weapon @p wep (-1 for none). */
int PlayerNowItemName(unsigned char wep) {
    int wep_name[9] = { -1, 4, 6, 8, 10, 11, 12, 14, 13 };

    return wep_name[wep];
}

/** Returns whether the search-view button is held (per the view-control setting), outside events. */
int PlayerSearchVIewButtonCheck(void) {
    if (!(sh2jms.player->status & 0x2000) && !(sh2jms.player->status & 0x4000)) {
        switch (playing.view_control) {
        case 0:
            return sh2jms.pad[0].search;
        case 1:
            return !sh2jms.pad[0].search;
        }
    }
    return 0;
}

/** Returns James's neck angle about x. */
float PlayerGetNeckAngleX(void) {
    struct SubCharacterDisp *d;

    d = (struct SubCharacterDisp *)sh2jms.player;
    return d->anime2.rot_neck.x;
}

/** Returns James's neck angle about y. */
float PlayerGetNeckAngleY(void) {
    struct SubCharacterDisp *d;

    d = (struct SubCharacterDisp *)sh2jms.player;
    return d->anime2.rot_neck.y;
}

/** Sets up James's hit columns after a room change. */
void PlayerInitOnConnect(void) {
    sh2jms.column_mov.kind = sh2jms.column_atk.kind = 1;
    sh2jms.column_mov.weight = sh2jms.column_atk.weight = 2;
    sh2jms.column_mov.material = 6;
    sh2jms.column_atk.material = 0;
    sh2jms.column_mov.shape = sh2jms.column_atk.shape = 3;
    sh2jms.column_mov.p[0][3] = sh2jms.column_atk.p[0][3] = 1.0f;
    sh2jms.column_mov.p[1][0] = sh2jms.column_atk.p[0][0] = 0.0f;
    sh2jms.column_mov.p[1][2] = sh2jms.column_atk.p[0][2] = 0.0f;
    sh2jms.column_mov.p[0][0] = sh2jms.column_atk.p[0][0] = sh2jms.player->pos.x;
    sh2jms.column_mov.p[0][1] = sh2jms.column_atk.p[0][1] = -50.0f + sh2jms.player->pos.y;
    sh2jms.column_mov.p[0][2] = sh2jms.column_atk.p[0][2] = sh2jms.player->pos.z;
    sh2jms.column_mov.p[1][1] = sh2jms.column_atk.p[1][1] = -850.0f + sh2jms.player->pos.y;
    sh2jms.column_mov.p[1][3] = sh2jms.column_atk.p[1][3] = 200.0f;
    sh2jms.col_mov_z_hosei = 0.0f;
    sh2jms.col_atk_z_hosei = 0.0f;
    sh2jms.player->spd = sh2jms.player->spd_org = 0.0f;
    PlayerSetReverseMode();
    sh2jms.room_name_prev = sh2jms.room_name_now;
    sh2jms.room_name_now = RoomNameJms();
    if (sh2jms.room_name_prev == 0x5D && sh2jms.room_name_now == 0x5E) {
        sh2jms.player->grnd_height = 0.0f;
        sh2jms.dist_pos.y = 0.0f;
    }
}

/** Clears James's hold, lock-on, run and attack state. */
void PlayerStatusClear(void) {
    sh2jms.hold_type = -1;
    sh2jms.lock_on = 0;
    sh2jms.running = 0;
    sh2jms.hold_chg[1] = 0;
    sh2jms.hold_chg[0] = 0;
    sh2jms.hold_loop[1] = 0;
    sh2jms.hold_loop[0] = 0;
    sh2jms.reload = 0;
    sh2jms.atk_type = 0;
    sh2jms.atk_reserve[0] = 0;
    sh2jms.atk_reserve[1] = 0;
    sh2jms.csaw_se_vol = 0.0f;
    sh2jms.player->battle.atk_result = 0;
    sh2jms.cam_chg_flg = 0;
    sh2jms.shotgun_prev = sh2jms.shotgun_dir;
    sh2jms.anime_pause = 0;
    shQzero(sh2jms.pad, 0x20);
}

/**
 * Turns angle @p now towards @p tgt (clamped to [@p min, @p max]) at a rate proportional to
 * the difference. @param spd turn speed (per second).
 */
void close_to_angle_target(float *now, float tgt, float min, float max, float spd) {
    float rot_tmp;
    float mov_angle;
    float tgt_tmp;

    tgt_tmp = tgt;
    if (tgt_tmp > max) {
        tgt_tmp = max;
    }
    if (tgt_tmp < min) {
        tgt_tmp = min;
    }
    rot_tmp = shAngleRegulate(*now - tgt_tmp);
    mov_angle = rot_tmp / (3.1415927f - 0.05) * (spd * shGetDT());
    if (rot_tmp >= 0.0f) {
        if (rot_tmp - mov_angle <= 0.0f) {
            *now = tgt_tmp;
        } else {
            *now -= mov_angle;
        }
    } else {
        if (rot_tmp - mov_angle >= 0.0f) {
            *now = tgt_tmp;
        } else {
            *now -= mov_angle;
        }
    }
    *now = shAngleRegulate(*now);
}

/** Moves @p now towards @p tgt by @p mov, without overshooting. */
void close_to_value(float *now, float tgt, float mov) {
    if (*now != tgt) {
        if (*now > tgt) {
            *now -= mov;
            if (*now < tgt) {
                *now = tgt;
            }
        } else {
            *now += mov;
            if (*now > tgt) {
                *now = tgt;
            }
        }
    }
}

/** Returns whether the mirrored flashlight is in use (sh2jms.light_reverse). */
int PlayerReverseLightCalcIsOn(void) {
    return sh2jms.light_reverse;
}

/** Returns whether James is on a water road (sh2jms.water_road). */
int PlayerWaterRoadIsOn(void) {
    return sh2jms.water_road;
}
