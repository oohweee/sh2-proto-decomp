/*
 * The rowing boat on the lake (BOT, model 0x10B): stick input to oar strokes, the boat's
 * movement, waves and rolling, oar effects and sounds, the lake's boundary walls, and keeping
 * James (m3_boat_jms) in it.
 */

/* This file has its own static close_to_value(short *, short, short), so it leaves out the
 * prototype of the global float one from another file. */
#define SH2_LOCAL_close_to_value
#include "sh2.h"
#include "m3_helpers.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "libc/math.h"
#include "sdk/libvu0.h"
#include "lib/libShPad.h"

static struct _CL_HITPOLY_PLANE lake_active_wall_list[12] = {
    {1, 1, 0, 4, 2, 0, {{40000.0f, 3490.0f, -40000.0f, 1.0f}, {40000.0f, 3490.0f, -80000.0f, 1.0f}, {40000.0f, 3690.0f, -80000.0f, 1.0f}, {40000.0f, 3690.0f, -40000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{40000.0f, 3490.0f, -80000.0f, 1.0f}, {30000.0f, 3490.0f, -100000.0f, 1.0f}, {30000.0f, 3690.0f, -100000.0f, 1.0f}, {40000.0f, 3690.0f, -80000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{30000.0f, 3490.0f, -100000.0f, 1.0f}, {-40000.0f, 3490.0f, -100000.0f, 1.0f}, {-40000.0f, 3690.0f, -100000.0f, 1.0f}, {30000.0f, 3690.0f, -100000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{-40000.0f, 3490.0f, -100000.0f, 1.0f}, {-50000.0f, 3490.0f, -80000.0f, 1.0f}, {-50000.0f, 3690.0f, -80000.0f, 1.0f}, {-40000.0f, 3690.0f, -100000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{-45000.0f, 3490.0f, -100000.0f, 1.0f}, {-45000.0f, 3490.0f, -20000.0f, 1.0f}, {-45000.0f, 3690.0f, -20000.0f, 1.0f}, {-45000.0f, 3690.0f, -100000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{-50000.0f, 3490.0f, -40000.0f, 1.0f}, {-40000.0f, 3490.0f, -20000.0f, 1.0f}, {-40000.0f, 3690.0f, -20000.0f, 1.0f}, {-50000.0f, 3690.0f, -40000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{-40000.0f, 3490.0f, -21000.0f, 1.0f}, {30000.0f, 3490.0f, -21000.0f, 1.0f}, {30000.0f, 3690.0f, -21000.0f, 1.0f}, {-40000.0f, 3690.0f, -21000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{30000.0f, 3490.0f, -20000.0f, 1.0f}, {40000.0f, 3490.0f, -40000.0f, 1.0f}, {40000.0f, 3690.0f, -40000.0f, 1.0f}, {30000.0f, 3690.0f, -20000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{-40500.0f, 3490.0f, -61000.0f, 1.0f}, {-45000.0f, 3490.0f, -58000.0f, 1.0f}, {-45000.0f, 3690.0f, -58000.0f, 1.0f}, {-40500.0f, 3690.0f, -61000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{-42000.0f, 3490.0f, -78000.0f, 1.0f}, {-40500.0f, 3490.0f, -61000.0f, 1.0f}, {-40500.0f, 3690.0f, -61000.0f, 1.0f}, {-42000.0f, 3690.0f, -78000.0f, 1.0f}}},
    {1, 1, 0, 4, 2, 0, {{-45000.0f, 3490.0f, -80000.0f, 1.0f}, {-42000.0f, 3490.0f, -78000.0f, 1.0f}, {-42000.0f, 3690.0f, -78000.0f, 1.0f}, {-45000.0f, 3690.0f, -80000.0f, 1.0f}}},
    {0, 0, 0, 0, 0, 0, {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}},
};

static int dt;
static float dtf;
struct shBoatWork sh2bot;
struct FMAT boat_localworld_matrix;
static struct FVEC rot;
static struct FVEC pos;
static float pos_aspd;
static float rot_aspd;
static float lstick_buf[20];
static float rstick_buf[20];
static float lstick_power;
static float rstick_power;
static float round_power;
static float straight_power;
static float lstick_total_power;
static float rstick_total_power;
static unsigned short lstick_status;
static unsigned short rstick_status;
static float key_lstick_power;
static float key_rstick_power;
static float key_lstick_dir;
static float key_rstick_dir;
static int oar_status_left[2];
static int oar_status_right[2];
static float wv_energy_x;
static float wv_energy_y;
static float wv_energy_z;
static float wv_timer_x;
static float wv_timer_y;
static float wv_timer_z;
static float wv_shinpuku_x;
static float wv_shinpuku_z;
static float wv_height_x;
static float wv_height_z;
static float wv_height_y;
static int wv_status_x;
static int wv_status_z;
static int wv_type_x;
static int wv_type_z;
static short anm_speed_left;
static short anm_speed_right;
static float oar_pos_left[4];
static float oar_pos_right[4];

/** Sets @p status bits in @p type. */
void bot_flg_on(unsigned int *type, unsigned int status) {
    *type |= status;
}

/** Clears @p status bits in @p type. */
void bot_flg_off(unsigned int *type, unsigned int status) {
    *type &= ~status;
}

/** Returns the boat's animation flags masked by @p status. */
int bot_anime_flg_on(unsigned int status) {
    return sh2bot.anime_st_flg & status;
}

/** Moves boat work @p w to state @p status, remembering the previous one. */
void bot_st_set(int status, struct shBoatWork *w) {
    w->status_prev = w->status_now;
    w->status_now = status;
}

static void close_to_value(short *now, short tgt, short mov) {
    if (*now != tgt) {
        if (*now > tgt) {
            *now -= mov;
            if (tgt > *now) {
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

static int HumanBOTInit(void) {
    int i1;

    lstick_status = 0;
    rstick_status = 0;
    round_power = 0.0f;
    straight_power = 0.0f;
    lstick_total_power = 0.0f;
    rstick_total_power = 0.0f;
    for (i1 = 0; i1 < 2; i1++) {
        oar_status_left[i1] = 0;
        oar_status_right[i1] = 0;
    }
    wv_energy_x = 0.0f;
    wv_energy_z = 0.0f;
    wv_timer_x = 0.0f;
    wv_timer_z = 0.0f;
    wv_shinpuku_x = 0.0f;
    wv_shinpuku_z = 0.0f;
    wv_height_x = 0.0f;
    wv_height_z = 0.0f;
    wv_height_y = 0.0f;
    wv_status_x = 0;
    wv_status_z = 0;
    wv_type_x = 0;
    wv_type_z = 0;
    anm_speed_left = 0;
    anm_speed_right = 0;
    for (i1 = 0; i1 < 20; i1++) {
        lstick_buf[i1] = 0.0f;
        rstick_buf[i1] = 0.0f;
    }
    mizTestLakeWaveInit();
    sh2bot.anim_change = 0;
    sh2bot.anime_change_now = 0;
    sh2bot.anime_change_prev = 0;
    return 0;
}

static float BoatCheckKeyInputStickDir(float x, float y) {
    return shAtan2(-y, x);
}

static float BoatCheckKeyInputStickPow(float x, float y, float dir) {
    float pow_x;
    float pow_y;
    float pow;

    pow_x = x * shSinF(dir);
    pow_y = -y * shCosF(dir);
    pow = _shSqrt(pow_x * pow_x + pow_y * pow_y);
    return (pow < 0.0f) ? 0.0f : ((pow > 1.0f) ? 1.0f : pow);
}

static int check_value(float key, float *buf) {
    int i;

    for (i = 0; i < 20; i++, buf++) {
        if (key != *buf) {
            return 0;
        }
    }
    return 1;
}

static inline float BoatStickPowerL(float rate) {
    return rate * key_lstick_power * shCosF(key_lstick_dir) + rate * (rate * key_lstick_power * shSinF(key_lstick_dir));
}

static void BoatCheckKeyInput(void) {
    static unsigned char pad_local[32];
    static int left;
    static int right;
    union shGameKeyData key;
    int i;
    float rot_left;
    float rot_right;
    float lstick_real_power;
    float rstick_real_power;
    int anime_type[3][3] = {{0, 1, 1}, {2, 0, 1}, {2, 1, 0}};

    libShPadRead(0, 0, (char *)pad_local);
    shSysKeyNormalize((char *)pad_local);
    shSysKeyAdjust((char *)pad_local);
    shGameKeyConvert(&key, (char *)pad_local);
    {
        char ry = key.f.CY;
        char rx = key.f.CX;
        char ly = key.f.AY;
        char lx = key.f.AX;

        key_lstick_dir = BoatCheckKeyInputStickDir(lx, ly);
        key_rstick_dir = BoatCheckKeyInputStickDir(rx, ry);
        key_lstick_power = BoatCheckKeyInputStickPow(lx, ly, key_lstick_dir);
        key_rstick_power = BoatCheckKeyInputStickPow(rx, ry, key_rstick_dir);
    }
    if (!playing.battle_level) {
        lstick_power = BoatStickPowerL(0.5f);
        rstick_power = 0.5f * key_lstick_power * shCosF(key_lstick_dir) - 0.5f * (0.5f * key_lstick_power * shSinF(key_lstick_dir));
        if (lstick_power < 0.0f && rstick_power >= 0.0f) {
            left = 1;
            right = 2;
        } else if (lstick_power < 0.0f && rstick_power >= 0.0f) {
            left = 2;
            right = 1;
        } else {
            left = 1;
            right = 1;
        }
    } else {
        rot_left = shAngleRegulate(key_lstick_dir - lstick_buf[0]);
        rot_right = shAngleRegulate(key_rstick_dir - rstick_buf[0]);
        lstick_status = 0;
        lstick_status |= 1;
        if (check_value(key_lstick_dir, lstick_buf)) {
            lstick_power = 0.0f;
            lstick_status |= 2;
            left = 0;
        } else {
            lstick_power = -rot_left;
            if (rot_left > 0.0f) {
                lstick_status |= 4;
                left = 2;
            } else if (rot_left < 0.0f) {
                lstick_status |= 8;
                left = 1;
            } else {
                switch (left) {
                    case 0:
                        lstick_status |= 2;
                        break;
                    case 1:
                        lstick_status |= 8;
                        break;
                    case 2:
                        lstick_status |= 4;
                        break;
                }
            }
        }
        for (i = 19; i > 0; i--) {
            lstick_buf[i] = lstick_buf[i - 1];
        }
        lstick_buf[0] = key_lstick_dir;
        lstick_power = (lstick_power < -0.5f) ? -0.5f : ((lstick_power > 0.5f) ? 0.5f : lstick_power);
        rstick_status = 0;
        rstick_status |= 1;
        if (check_value(key_rstick_dir, rstick_buf)) {
            rstick_power = 0.0f;
            rstick_status |= 2;
            right = 0;
        } else {
            rstick_power = rot_right;
            if (rot_right > 0.0f) {
                rstick_status |= 4;
                right = 1;
            } else if (rot_right < 0.0f) {
                rstick_status |= 8;
                right = 2;
            } else {
                switch (right) {
                    case 0:
                        rstick_status |= 2;
                        break;
                    case 1:
                        rstick_status |= 4;
                        break;
                    case 2:
                        rstick_status |= 8;
                        break;
                }
            }
        }
        for (i = 19; i > 0; i--) {
            rstick_buf[i] = rstick_buf[i - 1];
        }
        rstick_buf[0] = key_rstick_dir;
        rstick_power = (rstick_power < -0.5f) ? -0.5f : ((rstick_power > 0.5f) ? 0.5f : rstick_power);
    }
    lstick_real_power = lstick_power;
    rstick_real_power = rstick_power;
    round_power = 2.0f * (lstick_real_power - rstick_real_power);
    straight_power = 0.5f * (lstick_real_power + rstick_real_power);
    if (sh2bot.anime_change_timer == 0.0f) {
        sh2bot.anime_change_prev = sh2bot.anime_change_now;
        sh2bot.anime_change_now = anime_type[left][right];
        if (sh2bot.boat_p->spd >= 2.0f) {
            sh2bot.anime_change_now = 0;
        }
        if (sh2bot.anime_change_prev != sh2bot.anime_change_now) {
            sh2bot.anime_change_timer = 1.5f;
        }
    }
}

static void BoatSetHeightDummy(void) {
}

static void BoatSetHeight(struct SubCharacter *this) {
    this->pos.y = 500.0f * (7.18f + wv_height_y);
    if (sh2jms.player->status & 0x20000) {
        this->eye_y = this->center_y = -10000.0f;
    } else {
        this->eye_y = this->center_y = -10000.0f;
    }
}

static void BoatCheckSetParameter(struct SubCharacter *this) {
    struct _CL_HITPOLY_COLUMN column;

    GameBoatTimerCountUp();
    GameBoatMaxSpeedCheck(this->spd);
    sh2bot.anim_change = 0;
    if (!bot_anime_flg_on(4)) {
        sh2bot.anime_change_timer -= dtf;
        if (sh2bot.anime_change_timer < 0.0f) {
            sh2bot.anime_change_timer = 0.0f;
        }
    }
    column.kind = 1;
    column.weight = 2;
    column.material = 6;
    column.shape = 3;
    column.p[0][3] = sh2jms.column_atk.p[0][3] = 1.0f;
    column.p[1][0] = sh2jms.column_atk.p[0][0] = 0.0f;
    column.p[1][2] = sh2jms.column_atk.p[0][2] = 0.0f;
    column.p[0][0] = sh2bot.boat_p->pos.x;
    column.p[0][1] = 50.0f + sh2bot.boat_p->pos.y;
    column.p[0][2] = sh2bot.boat_p->pos.z;
    column.p[1][1] = -50.0f + sh2bot.boat_p->pos.y;
    column.p[1][3] = 1650.0f;
    BoatSetHeight(this);
    clSetCharaHitColumn(&column, &column, this, BoatSetHeightDummy);
}

static void boat_both(void) {
    switch (sh2bot.anime_change_now) {
        case 0:
            break;
        case 1:
            bot_st_set(1, &sh2bot);
            break;
        case 2:
            bot_st_set(2, &sh2bot);
            break;
    }
}

static void boat_right(void) {
    switch (sh2bot.anime_change_now) {
        case 0:
            bot_st_set(0, &sh2bot);
            break;
        case 1:
            break;
        case 2:
            bot_st_set(2, &sh2bot);
            break;
    }
}

static void boat_left(void) {
    switch (sh2bot.anime_change_now) {
        case 0:
            bot_st_set(0, &sh2bot);
            break;
        case 1:
            bot_st_set(1, &sh2bot);
            break;
        case 2:
            break;
    }
}

static void (*func_list_main[3])(struct SubCharacter *) = {
    (void (*)(struct SubCharacter *))boat_both,
    (void (*)(struct SubCharacter *))boat_right,
    (void (*)(struct SubCharacter *))boat_left,
};

static void BoatCheckStatus(struct SubCharacter *this) {
    void (*boat_main_func)(struct SubCharacter *);

    boat_main_func = func_list_main[sh2bot.status_now];
    boat_main_func(this);
}

static inline int BoatCheckZero(float val, float range) {
    return (val >= -range && val < range) ? 1 : 0;
}

static void BoatCheckControl(struct SubCharacter *this) {
    this->spd -= 0.15f * this->spd * dtf;
    this->rot.w -= 0.2f * this->rot.w * dtf;
    if (BoatCheckZero(this->spd, 0.05f) && !straight_power) {
        this->spd = 0.0f;
    }
    if (BoatCheckZero(this->rot.w, 0.025f) && !round_power) {
        this->rot.w = 0.0f;
    }
    pos_aspd = 1.5f * straight_power;
    this->spd += pos_aspd * dtf;
    this->spd = (this->spd < -3.0f) ? -3.0f : ((this->spd > 4.0f) ? 4.0f : this->spd);
    rot_aspd = 0.25f * round_power / (1.0f + fabsf(this->spd));
    this->rot.w += rot_aspd * dtf;
    this->rot.w = (this->rot.w < -1.5f) ? -1.5f : ((this->rot.w > 1.5f) ? 1.5f : this->rot.w);
    this->pos_spd.x = this->spd * dtf * shSinF(PlayerAngleWrap(this->rot.y + this->spd_roty));
    this->pos_spd.z = this->spd * dtf * shCosF(PlayerAngleWrap(this->rot.y + this->spd_roty));
    this->pos_spd.y = this->spd_y * dtf;
    rot.y = this->rot.w * dtf;
    pos.x = this->pos_spd.x;
    pos.y = this->pos_spd.y;
    pos.z = this->pos_spd.z;
    pos.x = 500.0f * pos.x;
    pos.y = 500.0f * pos.y;
    pos.z = 500.0f * pos.z;
    if (wv_energy_y != 0.0f) {
        wv_timer_y += dtf;
        if (wv_timer_y > 2.0f) {
            wv_timer_y -= 2.0f;
        }
        wv_height_y = -wv_energy_y * sinf(3.1415927f * wv_timer_y);
        wv_energy_y -= 0.01f * wv_energy_y;
        if (BoatCheckZero(wv_energy_y, 0.0005f)) {
            wv_energy_y = 0.0f;
            wv_timer_y = 0.0f;
            wv_height_y = 0.0f;
        }
    }
    if (wv_energy_x != 0.0f) {
        wv_timer_x += dtf;
        if (wv_timer_x > 2.0f) {
            wv_timer_x -= 2.0f;
        }
        if (wv_timer_x >= 0.5f && wv_timer_x < 1.5f) {
            wv_status_x = -1;
        } else {
            wv_status_x = 1;
        }
        wv_shinpuku_x = sqrtf(2.0f * wv_energy_x / 1480.4408f);
        wv_height_x = wv_shinpuku_x * sinf(3.1415927f * wv_timer_x);
        wv_energy_x -= 0.02f * wv_energy_x;
        if (BoatCheckZero(wv_energy_x, 0.0005f)) {
            wv_energy_x = 0.0f;
            wv_timer_x = 0.0f;
            wv_height_x = 0.0f;
        }
        this->rot.z = atan2f(wv_height_x, 0.5f);
    }
    if (wv_energy_z != 0.0f) {
        wv_timer_z += dtf;
        if (wv_timer_z > 2.0f) {
            wv_timer_z -= 2.0f;
        }
        if (wv_timer_z >= 0.5f && wv_timer_z < 1.5f) {
            wv_status_z = -1;
        } else {
            wv_status_z = 1;
        }
        wv_shinpuku_z = sqrtf(2.0f * wv_energy_z / 1480.4408f);
        wv_height_z = wv_shinpuku_z * sinf(3.1415927f * wv_timer_z);
        wv_energy_z -= 0.02f * wv_energy_z;
        if (BoatCheckZero(wv_energy_z, 0.0005f)) {
            wv_energy_z = 0.0f;
            wv_timer_z = 0.0f;
            wv_height_z = 0.0f;
        }
        this->rot.x = atan2f(wv_height_z, 2.0f);
    }
}

static void BoatCheckAnimeSpeed(struct SubCharacter *scp) {
    short purpose_speed_left;
    short purpose_speed_right;
    struct SubCharacter *boat_jms;
    struct SubCharacterDisp *scp_d;

    purpose_speed_left = 8.0f * (4096.0f * lstick_power);
    purpose_speed_right = 8.0f * (4096.0f * rstick_power);
    if (iabs_gcc(purpose_speed_left) > iabs_gcc(anm_speed_left)) {
        close_to_value(&anm_speed_left, purpose_speed_left, 0x40);
    } else {
        close_to_value(&anm_speed_left, purpose_speed_left, 0x10);
    }
    if (iabs_gcc(purpose_speed_right) > iabs_gcc(anm_speed_right)) {
        close_to_value(&anm_speed_right, purpose_speed_right, 0x40);
    } else {
        close_to_value(&anm_speed_right, purpose_speed_right, 0x10);
    }
    anm_speed_left = (anm_speed_left < -0x800) ? -0x800 : ((anm_speed_left > 0x800) ? 0x800 : anm_speed_left);
    anm_speed_right = (anm_speed_right < -0x800) ? -0x800 : ((anm_speed_right > 0x800) ? 0x800 : anm_speed_right);
    switch (sh2bot.anime_change_now) {
        case 0:
            shCharacterAnimeSpeedAdd(scp, sh2bot.anime_speed = (anm_speed_left + anm_speed_left) & ~1);
            break;
        case 1:
            shCharacterAnimeSpeedAdd(scp, sh2bot.anime_speed = anm_speed_right * 2);
            break;
        case 2:
            shCharacterAnimeSpeedAdd(scp, sh2bot.anime_speed = anm_speed_left * 2);
            break;
    }
}

static const struct _AnimeInfo boat_anim[5] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0x33, 0x96, 0, 0, 0x95, 1, -1},
    {0x34, 0x96, 0, 0x96, 0x12B, 1, -1},
    {0x35, 0x96, 0, 0x12C, 0x1C1, 1, -1},
    {0x36, 0x18, 0x20, 0x1C2, 0x1D9, 1, 0},
};

static void boat_anim_set_all(struct _AnimeInfo *aip, int comp_type) {
    shCharacterAnimeSet(sh2bot.boat_p, 0, comp_type, aip, (int)shCharacterGetAnimeAdrForPlay(sh2bot.boat_p));
}

static void BoatCheckAnime(struct SubCharacter *scp) {
    static int anime_change_check = -1;
    struct shBoatWork *w;
    struct SubCharacterDisp *scp_d;
    struct _AnimeInfo *aip;

    w = &sh2bot;
    scp_d = (struct SubCharacterDisp *)sh2bot.boat_p;
    sh2bot.anim_change = 0;
    if (!bot_anime_flg_on(4) && (w->status_now != anime_change_check || bot_anime_flg_on(8))) {
        switch (w->status_now) {
            case 0:
                aip = (struct _AnimeInfo *)&boat_anim[1];
                break;
            case 1:
                aip = (struct _AnimeInfo *)&boat_anim[2];
                break;
            case 2:
                aip = (struct _AnimeInfo *)&boat_anim[3];
                break;
        }
        boat_anim_set_all(aip, 10);
        bot_flg_on(&w->anime_st_flg, 4);
        anime_change_check = w->status_now;
        mar_flg_off(&w->anime_st_flg, 8);
        sh2bot.anim_change = 1;
    }
    if (scp_d->anime.comp_type < 3) {
        bot_flg_off(&w->anime_st_flg, 2);
    }
    if (scp_d->anime.comp_type < 9) {
        bot_flg_off(&w->anime_st_flg, 4);
    } else {
        shCharacterAnimeSpeedAddY(scp, 0x40);
    }
    if (scp_d->anime.comp_type == -1) {
        bot_flg_on(&w->anime_pause, 1);
    } else {
        bot_flg_off(&w->anime_pause, 1);
    }
}

static void BoatCheckOarStatus(struct SubCharacter *scp) {
    int i;
    struct shSkelton *top;

    top = scp->sk_top;
    for (i = 0; i < 3; i++) {
        top = top->next;
    }
    vcopy_gcc(oar_pos_left, top->src_m.d[3]);
    top = top->next;
    vcopy_gcc(oar_pos_right, top->src_m.d[3]);
    sceVu0ApplyMatrix(oar_pos_left, (float (*)[4])&boat_localworld_matrix, oar_pos_left);
    sceVu0ApplyMatrix(oar_pos_right, (float (*)[4])&boat_localworld_matrix, oar_pos_right);
    oar_status_left[1] = oar_status_left[0];
    oar_status_right[1] = oar_status_right[0];
    if (oar_pos_left[1] > 3500.0f) {
        oar_status_left[0] = 1;
    } else {
        oar_status_left[0] = 0;
    }
    if (oar_pos_right[1] > 3500.0f) {
        oar_status_right[0] = 1;
    } else {
        oar_status_right[0] = 0;
    }
}

static void BoatCheckEffect(void) {
    int kind_l;
    int kind_r;

    kind_r = kind_l = -1;
    if (oar_status_right[0] == 0 && oar_status_right[1] == 1) {
        kind_r = 1;
    }
    if (oar_status_right[0] == 1 && oar_status_right[1] == 0) {
        kind_r = 0;
    }
    if (oar_status_left[0] == 0 && oar_status_left[1] == 1) {
        kind_l = 1;
    }
    if (oar_status_left[0] == 1 && oar_status_left[1] == 0) {
        kind_l = 0;
    }
    if (kind_r != -1) {
        HH_Effect_Object_WaterSplash_Impact_Post(oar_pos_right, kind_r);
    }
    if (kind_l != -1) {
        HH_Effect_Object_WaterSplash_Impact_Post(oar_pos_left, kind_l);
    }
}

static void BoatCheckSound(void) {
}

static void BoatCheckRollEnergy(struct SubCharacter *scp) {
    int init_check_x;
    int init_check_y;
    int init_check_z;
    float energy_x;
    float energy_y;
    float energy_z;
    struct _shLakeWaveInfo *wip;
    float angle;

    if (wv_energy_x == 0.0f && round_power != 0.0f) {
        init_check_x = 1;
    } else {
        init_check_x = 0;
    }
    if (wv_energy_z == 0.0f && straight_power != 0.0f) {
        init_check_z = 1;
    } else {
        init_check_z = 0;
    }
    init_check_y = 0;
    energy_x = 0.05f * round_power;
    energy_z = 0.05f * straight_power;
    energy_y = 0.0f;
    wip = mizTestLakeWaveMain(scp);
    if (wip) {
        angle = wip->distance[1] - scp->rot.y;
        angle = (angle > 3.1415927f) ? angle - 6.2831855f : ((angle < -3.1415927f) ? 6.2831855f + angle : angle);
        energy_x += 2.0f * (dtf * (wip->energy * sinf(angle)));
        energy_z += 4.0f * (dtf * (wip->energy * cosf(angle)));
        energy_y += 0.05 * wip->energy;
        if (wv_energy_x != 0.0f) {
            init_check_x = 0;
        } else {
            init_check_x = 1;
        }
        if (wv_energy_z != 0.0f) {
            init_check_z = 0;
        } else {
            init_check_z = 1;
        }
        if (wv_energy_y != 0.0f) {
            init_check_y = 0;
        } else {
            init_check_y = 1;
        }
    }
    if (init_check_y) {
        wv_timer_y = 0.0f;
        wv_energy_y += energy_y;
    }
    if (init_check_x) {
        if (energy_x > 0.0f) {
            wv_timer_x = 0.0f;
            wv_status_x = 1;
        } else {
            wv_timer_x = 1.0f;
            wv_status_x = -1;
        }
        switch (wv_status_x) {
            case 1:
                wv_type_x = (unsigned char)(wv_height_x < 0.0f);
                break;
            case -1:
                wv_type_x = (unsigned char)((wv_height_x > 0.0f) ? 2 : 3);
                break;
        }
    }
    if (init_check_z) {
        if (energy_z > 0.0f) {
            wv_timer_z = 0.0f;
            wv_status_z = 1;
        } else {
            wv_timer_z = 1.0f;
            wv_status_z = -1;
        }
        switch (wv_status_z) {
            case 1:
                wv_type_z = (unsigned char)(wv_height_z < 0.0f);
                break;
            case -1:
                wv_type_z = (unsigned char)((wv_height_z > 0.0f) ? 2 : 3);
                break;
        }
    }
    switch (wv_type_x) {
        case 0:
            wv_energy_x += energy_x;
            if (wv_energy_x < 0.0f) {
                wv_energy_x = -wv_energy_x;
                wv_timer_x = 0.5f;
                wv_type_x = 2;
            }
            break;
        case 1:
            wv_energy_x += energy_x;
            if (wv_energy_x < 0.0f) {
                wv_energy_x = -wv_energy_x;
                wv_timer_x = 1.0f;
                wv_type_x = 3;
            }
            break;
        case 2:
            wv_energy_x -= energy_x;
            if (wv_energy_x < 0.0f) {
                wv_energy_x = -wv_energy_x;
                wv_timer_x = 0.0f;
                wv_type_x = 0;
            }
            break;
        case 3:
            wv_energy_x -= energy_x;
            if (wv_energy_x < 0.0f) {
                wv_energy_x = -wv_energy_x;
                wv_timer_x = 1.5f;
                wv_type_x = 1;
            }
            break;
    }
    switch (wv_type_z) {
        case 0:
            wv_energy_z += energy_z;
            if (wv_energy_z < 0.0f) {
                wv_energy_z = -wv_energy_z;
                wv_timer_z = 0.5f;
                wv_type_z = 2;
            }
            break;
        case 1:
            wv_energy_z += energy_z;
            if (wv_energy_z < 0.0f) {
                wv_energy_z = -wv_energy_z;
                wv_timer_z = 1.0f;
                wv_type_z = 3;
            }
            break;
        case 2:
            wv_energy_z -= energy_z;
            if (wv_energy_z < 0.0f) {
                wv_energy_z = -wv_energy_z;
                wv_timer_z = 0.0f;
                wv_type_z = 0;
            }
            break;
        case 3:
            wv_energy_z -= energy_z;
            if (wv_energy_z < 0.0f) {
                wv_energy_z = -wv_energy_z;
                wv_timer_z = 1.5f;
                wv_type_z = 1;
            }
            break;
    }
}

static void BoatCheckSetFirst(struct SubCharacter *this) {
    shCharacterAnimeFrameSet(this, 0x3C);
}

static inline void BoatUpdateMatrixZYX(struct SubCharacter *scp) {
    float rot_xz[4];

    rot_xz[0] = scp->rot.x;
    rot_xz[1] = 0.0f;
    rot_xz[2] = scp->rot.z;
    rot_xz[3] = 1.0f;
    sceVu0RotMatrix(boat_localworld_matrix.d, kt_unit_matrix.d, rot_xz);
    sceVu0RotMatrixY(boat_localworld_matrix.d, boat_localworld_matrix.d, scp->rot.y);
}

static void HumanBOTFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;
    int count;

    count = *T0_COUNT; /* unused, as in the original */
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 0.0f;
    pos_aspd = 0.0f;
    rot_aspd = 0.0f;
    switch (this->step) {
        case 0: {
            float pos[4];
            float rot[4];

            vcopy_gcc(pos, &this->pos);
            vcopy_gcc(rot, &this->rot);
            HumanBOTInit();
            if (!PlayerNowDemoEventMode()) {
                SCAnimeTypeSwitch(this, 1);
                aip = (struct _AnimeInfo *)&boat_anim[1];
                shCharacterAnimeSet(this, 0, 0, aip, (int)shCharacterGetAnimeAdrForPlay(this));
                sh2bot.status_now = sh2bot.status_prev = BOAT_ST_BOTH;
            }
            vcopy_gcc(&this->pos, pos);
            vcopy_gcc(&this->rot, rot);
            BoatSetHeight(this);
            shCharacterSetPosAfterDemo(this, (float *)&this->pos, this->rot.y);
            this->step++;
            break;
        }
        case 1:
            if (sh2jms.player->status & 0x2000) {
                break;
            }
            if (!(this->status & 4)) {
                this->step = 0;
                HumanBOTFunction(this);
            }
            dt = shGetDF();
            dtf = shGetDT();
            clAddDynamicWall(lake_active_wall_list);
            if (sh2jms.player->status & 0x20000) {
                BoatCheckKeyInput();
                BoatCheckSetParameter(this);
                BoatCheckStatus(this);
                BoatCheckAnime(this);
                BoatCheckControl(this);
                BoatCheckAnimeSpeed(this);
            } else {
                BoatCheckSetFirst(this);
            }
            BoatUpdateMatrixZYX(this);
            boat_localworld_matrix.d[3][0] = this->pos.x;
            boat_localworld_matrix.d[3][1] = this->pos.y;
            boat_localworld_matrix.d[3][2] = this->pos.z;
            boat_localworld_matrix.d[3][3] = 1.0f;
            BoatCheckOarStatus(this);
            BoatCheckEffect();
            BoatCheckSound();
            SCRotZYXSwitch(this, 1);
            BoatCheckRollEnergy(this);
            SCAddPos(this, &pos);
            SCAddRot(this, &rot);
            this->rot.y = shAngleRegulate(this->rot.y);
            if (sh2jms.player->status & 0x20000) {
                BoatPlayerCheckControl();
            }
            break;
    }
}

/** Installs the boat's update function, clears sh2bot and makes @p scp the boat. */
void shCharacterSetHumanBOTLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanBOTFunction);
    shCharacterBoatWorkInit();
    sh2bot.boat_p = scp;
}

/** Clears the boat work area sh2bot. */
void shCharacterBoatWorkInit(void) {
    shQzero(&sh2bot, sizeof(sh2bot));
}

static const struct _AnimeInfo d_boat_anim[4] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0x38, 0xBB8, 0x800, 0, 0xBB7, 0, 0},
    {0x39, 0x436, 0x800, 0, 0x435, 0, 0},
    {0x3A, 0x768, 0x800, 0, 0x767, 0, 0},
};

static int dboat_anime_adr_list[4] = {0, 0x819C, 0x13774, 0};

/**
 * Plays drama (event) animation @p anime_id on the boat model.
 * @return 0, or -1 if @p scp isn't the boat model.
 */
int shCharacterHumanBOTAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x10B) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_boat_anim[anime_id - 55];
        shCharacterAnimeSet(scp, 0, 2, aip, dboat_anime_adr_list[anime_id - 55] + (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 55));
        return 0;
    }
    return -1;
}

/** While James is in the boat, copies the boat's position, rotation and matrix to the camera's player info. */
void shUpdateBoatJamesPosAfterAnime(void) {
    struct shCharaInfo *cip;

    if (sh2jms.player->status & 0x20000) {
        cip = GetPlayerInfoForCameraCtrl();
        cip->pos = sh2bot.boat_p->pos;
        cip->rot = sh2bot.boat_p->rot;
        cip->pos_spd = sh2bot.boat_p->pos_spd;
        cip->rot_spd = sh2bot.boat_p->rot_spd;
        cip->mat = sh2bot.boat_p->mat;
    }
}
