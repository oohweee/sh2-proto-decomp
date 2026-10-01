/*
 * Maria as a gameplay character: her AI (following James, keeping her distance from enemies,
 * reacting to damage), her status and animation state machines, and her effects and sounds.
 */
/* Matching: GameMariaDamagedCountUp is called without a prototype, as in the original (arguments
 * passed unconverted; the float damage goes as a double), so this file leaves out the generated
 * prototype (config/prototype_overrides.txt). */
#define SH2_LOCAL_GameMariaDamagedCountUp
#include "sh2.h"
void GameMariaDamagedCountUp();

/* The asserts below compare integer table entries with NULL, so this file's NULL was an integer
 * constant; sh2.h's ((void *)0) doesn't compile there. */
#undef NULL
#define NULL 0L
#include "asm_helpers.h"
#include "asm_libm.h"
#include "sh_vu0.h"
#include "m3_helpers.h"
#include "sdk/libvu0.h"

static const struct _AnimeInfo pmaria_anim[40] = {
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0, 0 },
    { 0xB, 0xA, 0x180, 0x0, 0x9, 1, 0 },
    { 0xC, 0x18, 0x600, 0xA, 0x21, 1, 0 },
    { 0xD, 0x18, 0x700, 0x22, 0x39, 1, 0 },
    { 0xE, 0x14, 0x400, 0x3A, 0x4D, 1, 0 },
    { 0xF, 0x1F, 0x200, 0x4E, 0x6C, 0, 0 },
    { 0x10, 0x1E, 0x400, 0x6D, 0x8A, 0, 0 },
    { 0x11, 0x1E, -0xC00, 0x6D, 0x8A, 0, 0 },
    { 0x12, 0xA, 0x80, 0x8B, 0x94, 1, 0 },
    { 0x13, 0x37, 0x280, 0x95, 0xCB, 0, 0 },
    { 0x14, 0xF, 0x400, 0xCC, 0xDA, 0, 0 },
    { 0x15, 0xF, 0x400, 0xDB, 0xE9, 0, 0 },
    { 0x16, 0xF, 0x400, 0xEA, 0xF8, 0, 0 },
    { 0x17, 0xF, 0x400, 0xF9, 0x107, 0, 0 },
    { 0x18, 0x1E, 0x200, 0x108, 0x125, 0, 0 },
    { 0x19, 0x1E, 0x200, 0x126, 0x143, 0, 0 },
    { 0x1A, 0xF, 0x400, 0x144, 0x152, 0, 0 },
    { 0x1B, 0xF, 0x400, 0x153, 0x161, 0, 0 },
    { 0x1C, 0x1E, 0x200, 0x162, 0x17F, 0, 0 },
    { 0x1D, 0x1E, 0x200, 0x180, 0x19D, 0, 0 },
    { 0x1E, 0xF, 0x400, 0x19E, 0x1AC, 0, 0 },
    { 0x1F, 0xF, 0x400, 0x1AD, 0x1BB, 0, 0 },
    { 0x20, 0x1E, 0x200, 0x1BC, 0x1D9, 0, 0 },
    { 0x21, 0x1F, 0x200, 0x1DA, 0x1F8, 0, 0 },
    { 0x22, 0xF, 0x400, 0x1F9, 0x207, 0, 0 },
    { 0x23, 0xF, 0x400, 0x208, 0x216, 0, 0 },
    { 0x24, 0x1F, 0x200, 0x217, 0x235, 0, 0 },
    { 0x25, 0x1E, 0x200, 0x236, 0x253, 0, 0 },
    { 0x26, 0x1E, 0x200, 0x254, 0x271, 0, 0 },
    { 0x27, 0xF, 0x400, 0x272, 0x280, 0, 0 },
    { 0x28, 0xF, 0x400, 0x281, 0x28F, 0, 0 },
    { 0x29, 0x1E, 0x200, 0x290, 0x2AD, 0, 0 },
    { 0x2A, 0x1E, 0x200, 0x2AE, 0x2CB, 0, 0 },
    { 0x2B, 0xF, 0x400, 0x2CC, 0x2DA, 0, 0 },
    { 0x2C, 0xF, 0x400, 0x2DB, 0x2E9, 0, 0 },
    { 0x2D, 0x1E, 0x200, 0x2EA, 0x307, 0, 0 },
    { 0x2E, 0x1E, 0x200, 0x308, 0x325, 0, 0 },
    { 0x2F, 0xF, 0x400, 0x326, 0x334, 0, 0 },
    { 0x30, 0xF, 0x400, 0x335, 0x344, 0, 0 },
    { 0x31, 0xF, 0x400, 0x345, 0x353, 0, 0 },
};
static const struct MariaAppearPoint maria_apeear_point_list[91] = {
    { 33, 0x33, 2, { 4609.0f, 3000.0f, 104185.0f, -1.61f } },
    { 8, 0x33, 2, { -20426.0f, 0.0f, -16729.0f, 0.7f } },
    { 51, 0x2C, 0, { -19305.0f, 0.0f, -59868.0f, -2.98f } },
    { 44, 0x33, 2, { -21330.0f, 0.0f, -17805.0f, 1.48f } },
    { 44, 0x2D, 0, { -59966.0f, 0.0f, -19476.0f, -2.88f } },
    { 45, 0x2C, 0, { -20744.0f, 0.0f, -60175.0f, 0.9f } },
    { 51, 0x29, 2, { -60750.0f, 113.0f, -100597.0f, 1.68f } },
    { 41, 0x33, 2, { -24139.0f, 0.0f, -23109.0f, 1.39f } },
    { 41, 0x3B, 2, { -104104.0f, 0.0f, 16973.0f, 1.65f } },
    { 59, 0x29, 2, { -60762.0f, -1879.0f, -100598.0f, 1.63f } },
    { 59, 0x35, 0, { -20290.0f, 0.0f, 60158.0f, 2.47f } },
    { 53, 0x3B, 2, { -102973.0f, 0.0f, 21799.0f, -3.09f } },
    { 59, 0x36, 0, { -19370.9f, 0.0f, 20200.0f, -1.45f } },
    { 54, 0x3B, 2, { -102304.0f, 0.0f, 21099.0f, -3.03f } },
    { 59, 0x3C, 2, { -51551.0f, 0.0f, 100445.0f, -2.06f } },
    { 60, 0x3B, 2, { -105302.0f, 0.0f, 16401.0f, -2.06f } },
    { 60, 0x37, 2, { -60206.0f, 0.0f, 58799.0f, 1.25f } },
    { 55, 0x3C, 2, { -53414.0f, 0.0f, 100586.0f, -1.45f } },
    { 60, 0x38, 0, { -139990.0f, 0.0f, 57214.0f, 0.0f } },
    { 56, 0x3C, 2, { -56359.0f, 0.0f, 99946.0f, -1.76f } },
    { 60, 0x39, 2, { -60601.0f, 0.0f, 19323.0f, 1.76f } },
    { 57, 0x3C, 2, { -59260.0f, 0.0f, 99893.0f, -1.76f } },
    { 60, 0x3A, 0, { -138991.0f, 0.0f, 18410.0f, -1.52f } },
    { 58, 0x3C, 2, { -68617.0f, 0.0f, 99864.0f, 1.0f } },
    { 41, 0x44, 2, { 56990.0f, 0.0f, 16987.0f, -1.5f } },
    { 68, 0x29, 2, { -60676.0f, -3934.0f, -100591.0f, 1.91f } },
    { 68, 0x45, 2, { 68455.0f, 0.0f, 60570.0f, -1.55f } },
    { 69, 0x44, 2, { 54600.0f, 0.0f, 16404.0f, 1.59f } },
    { 69, 0x40, 0, { 140133.0f, 0.0f, 59236.0f, -2.46f } },
    { 64, 0x45, 2, { 60322.0f, 0.0f, 60580.0f, 1.77f } },
    { 69, 0x43, 1, { 139570.0f, 0.0f, 98827.0f, 1.67f } },
    { 67, 0x45, 2, { 50819.0f, 0.0f, 60100.0f, 1.92f } },
    { 69, 0x42, 1, { 100120.0f, 0.0f, 18450.0f, 0.0f } },
    { 66, 0x45, 2, { 54266.0f, 0.0f, 59839.0f, 2.19f } },
    { 51, 0x2F, 1, { -20350.0f, 0.0f, -100252.0f, -1.44f } },
    { 47, 0x33, 2, { -22173.0f, 0.0f, -20754.0f, -0.04f } },
    { 47, 0x2E, 1, { -58361.0f, 0.0f, -61300.0f, -0.8f } },
    { 46, 0x2F, 1, { -20350.0f, 0.0f, -100252.0f, -1.44f } },
    { 51, 0x2E, 1, { -58361.0f, 0.0f, -61300.0f, -0.8f } },
    { 46, 0x33, 2, { -17830.0f, 0.0f, -20201.0f, 2.91f } },
    { 71, 0x59, 0, { 19479.0f, 0.0f, -221432.0f, 1.44f } },
    { 89, 0x47, 2, { -59404.0f, 2000.0f, -220010.0f, -0.93f } },
    { 89, 0x5A, 0, { 19483.0f, 0.0f, -302019.0f, 0.7f } },
    { 90, 0x59, 0, { 19479.0f, 0.0f, -221432.0f, 1.44f } },
    { 87, 0x58, 2, { 14600.0f, 0.0f, -180870.0f, 1.63f } },
    { 88, 0x57, 2, { 70126.0f, 0.0f, -179830.0f, -1.65f } },
    { 87, 0x55, 1, { 59775.0f, 0.0f, -140650.0f, 1.63f } },
    { 85, 0x57, 2, { 66461.0f, 0.0f, -180568.0f, -1.69f } },
    { 87, 0x56, 1, { 99669.0f, 0.0f, -180657.0f, 1.71f } },
    { 86, 0x57, 2, { 55759.0f, 0.0f, -180573.0f, 1.4f } },
    { 87, 0x48, 2, { -100591.0f, -4000.0f, -219234.0f, -3.13f } },
    { 72, 0x57, 2, { 59767.0f, 0.0f, -179031.0f, 2.23f } },
    { 72, 0x5B, 2, { 19793.0f, 0.0f, -260207.0f, -2.34f } },
    { 91, 0x48, 2, { -100213.0f, 3400.0f, -213986.0f, -0.62f } },
    { 88, 0x47, 2, { -60816.0f, -3879.0f, -220593.0f, 1.85f } },
    { 71, 0x58, 2, { 15954.0f, 0.0f, -180231.0f, 1.39f } },
    { 87, 0x46, 2, { -19683.0f, 0.0f, -299856.0f, -2.42f } },
    { 70, 0x57, 2, { 59409.0f, 0.0f, -179005.0f, 2.46f } },
    { 80, 0x46, 2, { -19683.0f, 0.0f, -299856.0f, -2.42f } },
    { 70, 0x50, 2, { -60580.0f, 0.0f, -139010.0f, 2.58f } },
    { 78, 0x46, 2, { -19683.0f, 0.0f, -299856.0f, -2.42f } },
    { 70, 0x4E, 2, { -104838.0f, 0.0f, -259120.0f, -1.41f } },
    { 88, 0x54, 0, { 20017.0f, 0.0f, -137803.0f, 2.84f } },
    { 84, 0x58, 2, { 18952.0f, 0.0f, -180201.0f, 2.98f } },
    { 78, 0x4F, 2, { -99502.0f, 0.0f, -302003.0f, 3.05f } },
    { 79, 0x4E, 2, { -104888.0f, 0.0f, -260621.0f, -0.66f } },
    { 80, 0x52, 0, { -19434.0f, 0.0f, -141449.0f, 1.92f } },
    { 82, 0x50, 2, { -67027.0f, 0.0f, -140544.0f, 1.59f } },
    { 80, 0x51, 0, { -59253.0f, 0.0f, -180733.0f, -2.33f } },
    { 81, 0x50, 2, { -60456.0f, 0.0f, -140561.0f, -1.68f } },
    { 80, 0x53, 2, { -22306.0f, 0.0f, -180981.0f, 3.06f } },
    { 83, 0x50, 2, { -49819.0f, 0.0f, -140594.0f, -0.22f } },
    { 79, 0x49, 2, { -68526.0f, -23.0f, -300024.0f, 2.42f } },
    { 73, 0x4F, 2, { -99600.0f, 0.0f, -300256.0f, -2.42f } },
    { 78, 0x4C, 0, { -139394.0f, 0.0f, -221693.0f, -1.73f } },
    { 76, 0x4E, 2, { -99568.0f, 0.0f, -260646.0f, 0.56f } },
    { 78, 0x4B, 0, { -140861.0f, 0.0f, -261616.0f, 1.55f } },
    { 75, 0x4E, 2, { -96298.0f, 0.0f, -260603.0f, -1.36f } },
    { 9, 0x8, 2, { -85238.0f, 0.0f, 66257.0f, -1.36f } },
    { 8, 0xC, 2, { 19630.0f, 0.0f, 21438.0f, -3.05f } },
    { 12, 0x8, 2, { -98778.0f, 0.0f, 32804.0f, -0.09f } },
    { 12, 0xD, 2, { -22675.0f, 0.0f, 22281.0f, 2.35f } },
    { 13, 0xC, 2, { 22616.0f, -2125.0f, 22398.0f, -2.98f } },
    { 13, 0x8, 2, { -86972.0f, 0.0f, 32485.0f, -1.25f } },
    { 8, 0xD, 2, { -17213.0f, 0.0f, 19587.0f, -1.57f } },
    { -1, 0x2C, 0, { -19305.0f, 0.0f, -59868.0f, -2.98f } },
    { -1, 0x42, 1, { 100120.0f, 0.0f, 18450.0f, 0.0f } },
    { -1, 0x47, 2, { -60233.0f, -4000.0f, -219437.0f, -2.29f } },
    { -1, 0x4F, 2, { -100561.0f, 0.0f, -302945.0f, 0.62f } },
    { -1, 0x8, 2, { -10570.0f, 0.0f, 45941.0f, 1.11f } },
    { 0, 0x0, 0, { 0.0f, 0.0f, 0.0f, 0.0f } },
};
static const unsigned int pmaria_sub_status_flag[9] = { 0xDA, 0x0, 0x4, 0x0, 0x0, 0x0, 0xC0, 0xC0, 0x0 };

static int dt;
static float dtf;


/** Sets @p status bits in @p type. */
void mar_flg_on(unsigned int *type, unsigned int status) {
    *type |= status;
}

/** Clears @p status bits in @p type. */
void mar_flg_off(unsigned int *type, unsigned int status) {
    *type &= ~status;
}

/** Returns Maria's sub-state flags masked by @p status. */
int mar_sub_flg_on(unsigned int status) {
    return sh2mar.sub_st_flg & status;
}

/** Returns Maria's animation flags masked by @p status. */
int mar_anime_flg_on(unsigned int status) {
    return sh2mar.anime_st_flg & status;
}

/** Moves Maria's main state to @p status, remembering the previous one. */
void mar_main_st_set(int status, struct shMariaWork *w) {
    w->main_status_prev = w->main_status_now;
    w->main_status_now = status;
}

/** Moves Maria's sub-state to @p status, remembering the previous one. */
void mar_sub_st_set(int status, struct shMariaWork *w) {
    w->sub_status_prev = w->sub_status_now;
    w->sub_status_now = status;
}

/** Loads the sub-state flags of sub-state @p status. */
void mar_sub_flg_set(int status, struct shMariaWork *w) {
    w->sub_st_flg = pmaria_sub_status_flag[status];
}

static void MariaBodyAngleCloseToTarget(float target) {
    float roty_tmp;
    float mov_angle;

    roty_tmp = shAngleRegulate(sh2mar.mar_p->rot.y - target);
    mov_angle = 15.0f * dtf * (roty_tmp / (3.1415927f - 0.05));
    if (roty_tmp >= 0.0f) {
        if (roty_tmp - mov_angle <= 0.0f) {
            sh2mar.mar_p->rot.y = target;
        } else {
            sh2mar.mar_p->rot.y -= mov_angle;
        }
    } else {
        if (roty_tmp - mov_angle >= 0.0f) {
            sh2mar.mar_p->rot.y = target;
        } else {
            sh2mar.mar_p->rot.y -= mov_angle;
        }
    }
    sh2mar.mar_p->rot.y = shAngleRegulate(sh2mar.mar_p->rot.y);
}

static void MariaSpeedDownToStand(struct SubCharacter *p) {
    switch (sh2mar.sub_status_prev) {
    case MAR_SUB_ST_RUN:
        p->spd -= 10.0f * dtf;
        break;
    default:
        p->spd -= 8.0f * dtf;
        break;
    }
    p->spd = (p->spd > 0.0f) ? p->spd : 0.0f;
}

static void MariaPushedAnyoneIsOn(struct SubCharacter *p) {
    float angle;

    if (sh2mar.sub_status_now != MAR_SUB_ST_ONESTEP) {
        if (p->spd) {
            sh2mar.pushed_dir = 0;
        } else if (p->pos.x == p->b_pos.x && p->pos.z == p->b_pos.z) {
            sh2mar.pushed_dir = 0;
        } else {
            angle = fabsf(shAngleRegulate(p->rot.y - shAtan2(p->pos.z - p->b_pos.z, p->pos.x - p->b_pos.x)));
            if (angle < 1.5707964f) {
                sh2mar.pushed_dir = 1;
            } else {
                sh2mar.pushed_dir = -1;
            }
        }
    }
}

static int shMariaSoundOn(struct SubCharacter *this, float vol, int se_name, int unused) {
    SeCallPos(se_name, vol, (float *)&this->pos, 0);
    return 0;
}

static void MariaCheckSoundLower(void) {
    int i;
    unsigned int material;
    unsigned short se;
    unsigned short se_r;
    unsigned short se_l;
    short frame;
    int pitch;
    struct MariaSoundInfo se_info[4] = { { 0.0f, -1, -1 }, { 0.0f, -1, -1 }, { 0.0f, -1, -1 }, { 0.0f, -1, -1 } };
    struct _AnimeInfo *a_info;

    a_info = shCharacterAnimeGetInfo(sh2mar.mar_p);
    frame = shCharacterAnimeFrameGet(sh2mar.mar_p);
    for (i = 0; i < 2; i++) {
        if (i) {
            if (sh2mar.l_foot.kind == 1) {
                material = sh2mar.l_foot.hobj.wall.pd->material;
            }
        } else {
            if (sh2mar.r_foot.kind == 1) {
                material = sh2mar.r_foot.hobj.wall.pd->material;
            }
        }
        switch (material) {
        case 12:
            se = 0x4A57;
            break;
        case 0:
            se = 0x4A57;
            break;
        case 1:
            se = 0x4A5B;
            break;
        case 2:
            se = 0x4A58;
            break;
        case 3:
            se = 0x4A5B;
            break;
        case 4:
            se = 0x4A5B;
            break;
        case 5:
            se = 0x4A5E;
            break;
        case 6:
            se = 0x4A5D;
            break;
        case 7:
            se = 0x4A5C;
            break;
        case 8:
            se = 0x4A59;
            break;
        case 9:
            se = 0x4A5E;
            break;
        case 10:
            se = 0x4A5A;
            break;
        case 11:
            se = 0x4A5E;
            break;
        }
        if (i) {
            se_l = se;
        } else {
            se_r = se;
        }
    }
    switch (sh2mar.sub_status_now) {
    case MAR_SUB_ST_ONESTEP:
        if (a_info->name == 0x30) {
            se_info[0].frame = 6;
            se_info[1].frame = 12;
        } else {
            se_info[0].frame = 0x16;
            se_info[1].frame = 0x1A;
        }
        se_info[0].vol = 0.3f;
        se_info[1].vol = 0.3f;
        break;
    case MAR_SUB_ST_RELAX:
        if (a_info->name == 0x10) {
            se_info[0].frame = 0x12;
            se_info[0].vol = 0.3f;
        }
        break;
    case MAR_SUB_ST_AFRAID:
        se_info[0].frame = 0xA;
        se_info[1].frame = 0x10;
        se_info[2].frame = 0x16;
        se_info[3].frame = 0x1A;
        se_info[0].vol = 0.3f;
        se_info[1].vol = 0.3f;
        se_info[2].vol = 0.3f;
        se_info[3].vol = 0.3f;
        break;
    case MAR_SUB_ST_WALK:
    case MAR_SUB_ST_RUN:
        se_info[0].frame = 5;
        se_info[1].frame = 0x11;
        se_info[0].vol = 0.6f;
        se_info[1].vol = 0.6f;
        break;
    case MAR_SUB_ST_DAMAGE:
        switch (a_info->name) {
        case 0x16:
            se_info[0].frame = 2;
            se_info[1].frame = 4;
            se_info[2].frame = 9;
            break;
        case 0x17:
            se_info[0].frame = 4;
            se_info[1].frame = 9;
            se_info[2].frame = 0xD;
            break;
        case 0x18:
            se_info[0].frame = 3;
            se_info[1].frame = 6;
            break;
        case 0x19:
            se_info[0].frame = 3;
            se_info[1].frame = 5;
            se_info[2].frame = 9;
            break;
        case 0x1A:
            se_info[0].frame = 5;
            se_info[1].frame = 8;
            break;
        case 0x1B:
            se_info[0].frame = 4;
            se_info[1].frame = 8;
            break;
        case 0x1C:
            se_info[0].frame = 5;
            se_info[1].frame = 8;
            break;
        case 0x1D:
            se_info[0].frame = 5;
            se_info[1].frame = 9;
            break;
        case 0x14:
            se_info[0].frame = 6;
            se_info[1].frame = 0xA;
            break;
        case 0x15:
            se_info[0].frame = 4;
            se_info[1].frame = 0xC;
            break;
        case 0x1E:
            se_info[0].frame = 6;
            se_info[1].frame = 0xC;
            break;
        case 0x1F:
            se_info[0].frame = 4;
            se_info[1].frame = 5;
            se_info[2].frame = 0xD;
            break;
        case 0x22:
            se_info[0].frame = 2;
            se_info[1].frame = 6;
            break;
        case 0x23:
            se_info[0].frame = 7;
            se_info[1].frame = 0xD;
            break;
        case 0x27:
            se_info[0].frame = 3;
            se_info[1].frame = 0xB;
            break;
        case 0x28:
            se_info[0].frame = 2;
            se_info[1].frame = 0xB;
            break;
        case 0x2F:
            se_info[0].frame = 0xA;
            break;
        }
        se_info[0].vol = 0.6f;
        se_info[1].vol = 0.6f;
        se_info[2].vol = 0.6f;
        break;
    }
    switch (sh2mar.sub_status_now) {
    case MAR_SUB_ST_ONESTEP:
        se_info[0].domain = 1;
        se_info[1].domain = 2;
        break;
    case MAR_SUB_ST_RELAX:
        if (a_info->name == 0x10) {
            se_info[0].domain = 2;
        }
        break;
    case MAR_SUB_ST_AFRAID:
        se_info[0].domain = 1;
        se_info[1].domain = 2;
        se_info[2].domain = 1;
        se_info[3].domain = 2;
        break;
    case MAR_SUB_ST_WALK:
    case MAR_SUB_ST_RUN:
        se_info[0].domain = 1;
        se_info[1].domain = 2;
        break;
    case MAR_SUB_ST_DAMAGE:
        se_info[0].domain = 1;
        se_info[1].domain = 2;
        se_info[2].domain = 1;
        break;
    }
    for (i = 0; i < 4; i++) {
        if (se_info[i].frame >= 0 && frame >= 0) {
            if (frame >= se_info[i].frame) {
                if (!sh2mar.se_foot[i]) {
                    switch (se_info[i].domain) {
                    case 1:
                        shMariaSoundOn(sh2mar.mar_p, se_info[i].vol, se_r, 0);
                        break;
                    case 2:
                        shMariaSoundOn(sh2mar.mar_p, se_info[i].vol, se_l, 0);
                        break;
                    }
                }
                sh2mar.se_foot[i] = 1;
            } else {
                sh2mar.se_foot[i] = 0;
            }
        }
    }
}

static void MariaCheckSoundUpper(void) {
    struct _AnimeInfo *anim_p = shCharacterAnimeGetInfo(sh2mar.mar_p);
    int i;
    unsigned short se;
    short frame;
    struct MariaSoundInfo se_info[4] = { { 0.0f, -1, -1 } };

    frame = shCharacterAnimeFrameGet(sh2mar.mar_p);
    switch (sh2mar.sub_status_now) {
    case MAR_SUB_ST_TIRED:
        se = 0x4A5F;
        break;
    case MAR_SUB_ST_DAMAGE:
        if (sh2mar.dead) {
            se = 0x4A61;
        } else {
            se = 0x4A60;
        }
        break;
    }
    switch (sh2mar.sub_status_now) {
    case MAR_SUB_ST_TIRED:
        se_info[0].frame = 1;
        se_info[0].vol = 0.5f;
        break;
    case MAR_SUB_ST_DAMAGE:
        if (sh2mar.dead) {
            se_info[0].frame = 1;
            se_info[0].vol = 1.0f;
        } else {
            se_info[0].frame = 1;
            se_info[0].vol = 0.7f;
        }
        break;
    }
    for (i = 0; i < 4; i++) {
        if (se_info[i].frame >= 0 && frame >= 0) {
            if (frame >= se_info[i].frame) {
                if (!sh2mar.se_upper[i]) {
                    shMariaSoundOn(sh2mar.mar_p, se_info[i].vol, se, 0);
                }
                sh2mar.se_upper[i] = 1;
            } else {
                sh2mar.se_upper[i] = 0;
            }
        }
    }
}

static void mar_sub_flg_check(int status) {
    if (!sh2mar.anime_st_flg && sh2mar.sub_status_now != status) {
        mar_sub_st_set(status, &sh2mar);
        mar_sub_flg_set(status, &sh2mar);
    }
}

static void mar_muteki_set(void) {
    if (sh2mar.sub_status_prev == MAR_SUB_ST_DAMAGE) {
        sh2mar.muteki_time = 2.0f;
        sh2mar.no_damage = 0;
        sh2mar.sub_status_prev = 0xFF;
    }
}

static void mar_timer_set(int mode) {
    switch (mode) {
    case 0:
        sh2mar.stand_time += dtf;
        sh2mar.move_time = 0.0f;
        sh2mar.afraid_time = 0.0f;
        sh2mar.relax_time = 0.0f;
        break;
    case 1:
        sh2mar.stand_time = 0.0f;
        sh2mar.move_time += dtf;
        sh2mar.afraid_time = 0.0f;
        sh2mar.relax_time = 0.0f;
        break;
    case 2:
        sh2mar.stand_time += dtf;
        sh2mar.move_time = 0.0f;
        sh2mar.afraid_time += dtf;
        sh2mar.relax_time = 0.0f;
        break;
    case 3:
        sh2mar.stand_time += dtf;
        sh2mar.move_time = 0.0f;
        sh2mar.afraid_time = 0.0f;
        sh2mar.relax_time += dtf;
        break;
    }
}

static void mar_main_stand(void) {
    struct shMariaWork *w;

    w = &sh2mar; /* unused, as in the original (w is in its DWARF) */
    if (mar_sub_flg_on(4)) {
        mar_sub_flg_check(MAR_SUB_ST_RELAX_OFF);
    } else if (mar_sub_flg_on(0x20)) {
        mar_sub_flg_check(MAR_SUB_ST_ONESTEP);
    } else if (mar_sub_flg_on(1)) {
        mar_sub_flg_check(MAR_SUB_ST_STAND);
        mar_muteki_set();
    }
}

static void mar_main_close_to(void) {
    struct shMariaWork *w;

    w = &sh2mar;
    if (mar_sub_flg_on(4)) {
        mar_sub_flg_check(MAR_SUB_ST_RELAX_OFF);
    } else if (mar_sub_flg_on(0x80) && w->dist_to_jms >= 2000.0f) {
        mar_sub_flg_check(MAR_SUB_ST_RUN);
    } else if (mar_sub_flg_on(0x40) && (w->dist_to_jms >= 1000.0f || !sh2mar.look_jms)) {
        mar_sub_flg_check(MAR_SUB_ST_WALK);
    } else if (mar_sub_flg_on(1)) {
        mar_sub_flg_check(MAR_SUB_ST_STAND);
        mar_muteki_set();
    }
}

static void mar_main_alert(void) {
    struct shMariaWork *w;

    w = &sh2mar; /* unused, as in the original (w is in its DWARF) */
    if (mar_sub_flg_on(8)) {
        mar_sub_flg_check(MAR_SUB_ST_AFRAID);
    } else if (mar_sub_flg_on(1)) {
        mar_sub_flg_check(MAR_SUB_ST_STAND);
        mar_muteki_set();
    }
}

static void mar_main_discover(void) {
    struct shMariaWork *w;

    w = &sh2mar; /* unused, as in the original (w is in its DWARF) */
}

static void mar_main_recover(void) {
    struct shMariaWork *w;

    w = &sh2mar; /* unused, as in the original (w is in its DWARF) */
    if (mar_sub_flg_on(0x10)) {
        mar_sub_flg_check(MAR_SUB_ST_TIRED);
    } else if (mar_sub_flg_on(0x20)) {
        mar_sub_flg_check(MAR_SUB_ST_ONESTEP);
    } else if (mar_sub_flg_on(1)) {
        mar_sub_flg_check(MAR_SUB_ST_STAND);
        mar_muteki_set();
    }
}

static void mar_main_boredom(void) {
    struct shMariaWork *w;

    w = &sh2mar; /* unused, as in the original (w is in its DWARF) */
    if (mar_sub_flg_on(2)) {
        mar_sub_flg_check(MAR_SUB_ST_RELAX);
    } else if (mar_sub_flg_on(0x20)) {
        mar_sub_flg_check(MAR_SUB_ST_ONESTEP);
    } else if (mar_sub_flg_on(1)) {
        mar_sub_flg_check(MAR_SUB_ST_STAND);
        mar_muteki_set();
    }
}

static void mar_main_damaged(void) {
    struct shMariaWork *w;

    w = &sh2mar; /* unused, as in the original (w is in its DWARF) */
    if (mar_sub_flg_on(0x100)) {
        mar_sub_flg_check(MAR_SUB_ST_DAMAGE);
    }
    if (mar_sub_flg_on(1)) {
        mar_sub_flg_check(MAR_SUB_ST_STAND);
        mar_muteki_set();
    }
}

static void mar_sub_stand(struct SubCharacter *p) {
    mar_timer_set(0);
    sh2mar.tired -= dt;
    MariaSpeedDownToStand(p);
    if (sh2mar.pushed_dir) {
        mar_flg_on(&sh2mar.sub_st_flg, 0x20);
    }
}

static void mar_sub_relax(struct SubCharacter *p) {
    mar_timer_set(3);
    sh2mar.tired -= dt;
    MariaSpeedDownToStand(p);
    switch (sh2mar.relax_flag) {
    case 0:
        if (sh2mar.anime_pause & 1) {
            sh2mar.relax_flag = 1;
            mar_flg_on(&sh2mar.anime_st_flg, 0x40);
        }
        break;
    case 1:
        if (sh2mar.relax_time >= 10.0f) {
            sh2mar.relax_flag = 2;
            mar_flg_on(&sh2mar.anime_st_flg, 0x40);
        }
        break;
    case 2:
        if (sh2mar.anime_pause & 1) {
            sh2mar.relax_time = 0.0f;
            sh2mar.relax_flag = 1;
            mar_flg_on(&sh2mar.anime_st_flg, 0x40);
        }
        break;
    }
    if (sh2mar.main_status_now != MAR_MAIN_ST_BOREDOM) {
        sh2mar.relax_flag = 0;
        mar_flg_on(&sh2mar.sub_st_flg, 4);
    }
    if (sh2mar.pushed_dir) {
        mar_flg_on(&sh2mar.sub_st_flg, 0x20);
    }
}

static void mar_sub_relax_off(struct SubCharacter *p) {
    mar_timer_set(0);
    MariaSpeedDownToStand(p);
    if (sh2mar.anime_pause & 1) {
        mar_flg_off(&sh2mar.sub_st_flg, 4);
        mar_flg_on(&sh2mar.sub_st_flg, 1);
    }
    if (sh2mar.pushed_dir) {
        mar_flg_on(&sh2mar.sub_st_flg, 0x20);
    }
}

static void mar_sub_afraid(struct SubCharacter *p) {
    short frame;

    frame = shCharacterAnimeFrameGet(p);
    mar_timer_set(2);
    sh2mar.tired -= dt;
    MariaSpeedDownToStand(p);
    if (sh2mar.anime_pause & 1) {
        mar_flg_off(&sh2mar.sub_st_flg, 8);
        mar_flg_on(&sh2mar.sub_st_flg, 1);
        sh2mar.stand_time = 0.0f;
    }
}

static void mar_sub_tired(struct SubCharacter *p) {
    mar_timer_set(0);
    sh2mar.tired -= dt;
    MariaSpeedDownToStand(p);
    if (sh2mar.tired < sh2mar.tired_max >> 1) {
        mar_flg_off(&sh2mar.sub_st_flg, 0x10);
        mar_flg_on(&sh2mar.sub_st_flg, 1);
    }
    if (sh2mar.pushed_dir) {
        mar_flg_on(&sh2mar.sub_st_flg, 0x20);
    }
}

static void mar_sub_onestep(struct SubCharacter *p) {
    mar_timer_set(1);
    sh2mar.tired -= dt;
    MariaSpeedDownToStand(p);
    if (sh2mar.anime_pause & 1) {
        mar_flg_off(&sh2mar.sub_st_flg, 0x20);
        mar_flg_on(&sh2mar.sub_st_flg, 1);
        sh2mar.stand_time = 0.0f;
    }
}

static void mar_sub_walk(struct SubCharacter *p) {
    mar_timer_set(1);
    if (p->spd > 1.4f) {
        p->spd -= 4.0f * dtf;
        p->spd = (p->spd > 1.4f) ? p->spd : 1.4f;
    } else {
        p->spd += 2.0f * dtf;
        p->spd = (p->spd > 1.4f) ? 1.4f : p->spd;
    }
    if (sh2mar.look_jms) {
        sh2mar.to_target = shAtan2(sh2jms.player->pos.z - p->pos.z, sh2jms.player->pos.x - p->pos.x);
    } else {
        sh2mar.to_target = shAtan2(sh2mar.tgt_pos[0][2] - p->pos.z, sh2mar.tgt_pos[0][0] - p->pos.x);
    }
    MariaBodyAngleCloseToTarget(sh2mar.to_target);
    if (sh2mar.move_time >= 1.06f) {
        mar_flg_on(&sh2mar.sub_st_flg, 1);
    }
}

static void mar_sub_run(struct SubCharacter *p) {
    mar_timer_set(1);
    sh2mar.tired += dt;
    p->spd += 3.0f * dtf;
    p->spd = (p->spd > 3.5f) ? 3.5f : p->spd;
    if (sh2mar.look_jms) {
        sh2mar.to_target = shAtan2(sh2jms.player->pos.z - p->pos.z, sh2jms.player->pos.x - p->pos.x);
    } else {
        sh2mar.to_target = shAtan2(sh2mar.tgt_pos[0][2] - p->pos.z, sh2mar.tgt_pos[0][0] - p->pos.x);
    }
    MariaBodyAngleCloseToTarget(sh2mar.to_target);
    if (sh2mar.move_time >= 0.91f) {
        mar_flg_on(&sh2mar.sub_st_flg, 1);
    }
}

static void mar_sub_damage(struct SubCharacter *p) {
    struct _AnimeInfo *a_info;
    float damage_angle;
    short cur_frame;

    a_info = shCharacterAnimeGetInfo(p);
    mar_timer_set(0);
    MariaSpeedDownToStand(p);
    cur_frame = shCharacterAnimeFrameGet(p);
    if (p->battle.id) {
        damage_angle = shAtan2(p->battle.vec[2], p->battle.vec[0]);
        p->spd_roty = shAngleRegulate(damage_angle - p->rot.y);
    } else if (a_info->name == 0x14) {
        p->spd_roty = 3.1415927f;
    } else {
        p->spd_roty = 0.0f;
    }
    if (!mar_anime_flg_on(2)) {
        switch (a_info->name) {
        case 0x14:
        case 0x16:
        case 0x1A:
        case 0x22:
            if (cur_frame > 1 && cur_frame < 7) {
                p->spd = 1.0f;
            } else {
                p->spd = 0.2f;
            }
            break;
        case 0x15:
        case 0x17:
        case 0x1B:
        case 0x23:
            if (cur_frame > 5) {
                p->spd = 0.3f;
            } else if (cur_frame > 2) {
                p->spd = 1.2f;
            }
            break;
        case 0x18:
        case 0x1C:
        case 0x20:
        case 0x24:
        case 0x26:
        case 0x29:
        case 0x2E:
            if (cur_frame > 1 && cur_frame < 15) {
                p->spd = 0.05f * cur_frame;
            }
            break;
        case 0x19:
        case 0x1D:
        case 0x21:
        case 0x25:
        case 0x2A:
        case 0x2D:
            if (cur_frame > 1 && cur_frame < 15) {
                p->spd = 0.05f * cur_frame;
            }
            break;
        case 0x1E:
        case 0x1F:
        case 0x27:
        case 0x28:
        case 0x2B:
        case 0x2C:
        case 0x2F:
            break;
        }
    }
    if (sh2mar.anime_pause & 1) {
        MariaStatusClear();
        if (sh2mar.dead) {
            sh2jms.dead = 2;
        } else {
            mar_flg_off(&sh2mar.sub_st_flg, 0x100);
            mar_flg_on(&sh2mar.sub_st_flg, 1);
        }
    }
}

static int MariaCheckJamesLook(float *sp, float *ep) {
    struct _CL_VHIT_RESULT result;

    clCheckHitEyes(&result, (unsigned int)sh2mar.mar_p, sp, ep, 0);
    if (result.kind == 3 && result.hobj.chara.sc == sh2jms.player) {
        return 1;
    }
    return 0;
}

static int JamesCheckMariaLook(float *sp, float *ep) {
    struct _CL_VHIT_RESULT result;

    clCheckHitEyes(&result, (unsigned int)sh2jms.player, sp, ep, 0);
    if (result.kind == 3 && result.hobj.chara.sc == sh2mar.mar_p) {
        return 1;
    }
    return 0;
}

static int CheckJamesLook1(float *sp, float *ep) {
    struct _CL_VHIT_RESULT result;

    clCheckHitEyes(&result, 0, sp, ep, 0);
    if (result.kind == 3 && result.hobj.chara.sc == sh2jms.player) {
        return 1;
    }
    return 0;
}

static int CheckJamesLook2(float *sp, float *ep) {
    struct _CL_VHIT_RESULT result;

    clCheckHitEyes(&result, (unsigned int)sh2jms.player, sp, ep, 0);
    return result.kind == 0;
}

static int MariaCheckLook1(float *sp, float *ep) {
    struct _CL_VHIT_RESULT result;

    clCheckHitEyes(&result, (unsigned int)sh2mar.mar_p, sp, ep, 0);
    if (result.kind == 0 || (result.kind == 3 && result.hobj.chara.sc == sh2jms.player)) {
        return 1;
    }
    return 0;
}

static int MariaCheckLook2(float *sp, float *ep) {
    struct _CL_VHIT_RESULT result;

    clCheckHitEyes(&result, (unsigned int)sh2jms.player, sp, ep, 0);
    if (result.kind == 3 && result.hobj.chara.sc == sh2mar.mar_p) {
        return 1;
    }
    return 0;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
/* Matching: MariaSetStraightRoot is a K&R definition; called with arguments it ignores (the DWARF shows none). */
#line 1323
/* Matching: _shCopyVector (not vcopy) for the first copy: its source address is computed first. */
static void MariaSetStraightRoot() {
    int i;
    int j;
    float sp0[4];
    float sp1[4];
    float ep0[4];
    float ep1[4];

    sp0[0] = sh2mar.mar_p->pos.x;
    sp0[2] = sh2mar.mar_p->pos.z;
    sp0[1] = sh2mar.mar_p->pos.y - 100.0f;
    ep0[0] = sh2jms.player->pos.x;
    ep0[2] = sh2jms.player->pos.z;
    ep0[1] = sh2jms.player->pos.y - 100.0f;
    if (!sh2mar.tgt_pointer) {
        i = 0;
        if (MariaCheckJamesLook(sp0, ep0) && JamesCheckMariaLook(ep0, sp0)) {
            i = 1;
        }
        if (!(sh2mar.look_jms = i)) {
            sh2mar.tgt_pointer++;
        }
    } else {
        _shCopyVector(sp1, sh2mar.tgt_pos[sh2mar.tgt_pointer - 1]);
        if (!CheckJamesLook1(sp1, ep0) || !CheckJamesLook2(ep0, sp1)) {
            sh2mar.tgt_pointer++;
        }
    }
    if (sh2mar.tgt_pointer == 5) {
        sh2mar.tgt_pointer--;
        vcopy(sh2mar.tgt_pos[0], (float *)&sh2mar.mar_p->pos);
    }
    vcopy(ep0, sh2mar.tgt_pos[sh2mar.tgt_pointer]);
    if (sh2mar.tgt_pointer > 0) {
        for (i = 0; i < 1; i++) {
            vcopy(sh2mar.tgt_pos[i + 1], ep1);
            if (MariaCheckLook1(sp0, ep1) && MariaCheckLook2(ep1, sp0)) {
                j = 0;
                while (j < sh2mar.tgt_pointer) {
                    vcopy(sh2mar.tgt_pos[j + 1], sh2mar.tgt_pos[j]);
                    j++;
                }
                sh2mar.tgt_pointer--;
            }
        }
    }
    if (sh2mar.tgt_pointer < 0) {
        assert(sh2mar.tgt_pointer >= 0);
    }
}

static void (*func_list_main[7])(struct SubCharacter *) = {
    (void (*)(struct SubCharacter *))mar_main_stand,   (void (*)(struct SubCharacter *))mar_main_close_to,
    (void (*)(struct SubCharacter *))mar_main_alert,   (void (*)(struct SubCharacter *))mar_main_discover,
    (void (*)(struct SubCharacter *))mar_main_recover, (void (*)(struct SubCharacter *))mar_main_boredom,
    (void (*)(struct SubCharacter *))mar_main_damaged,
};

static void (*func_list_sub[9])(struct SubCharacter *) = {
    mar_sub_stand, mar_sub_relax, mar_sub_relax_off, mar_sub_afraid, mar_sub_tired,
    mar_sub_onestep, mar_sub_walk, mar_sub_run, mar_sub_damage,
};

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1653
#define ANGLE_WRAP(a) ((a) > 3.1415927f ? (a) - 6.2831855f : ((a) < -3.1415927f ? 6.2831855f + (a) : (a)))

enum {
    BTL_ID_ENEMY_TO_MARIA_START = 0x24,
    BTL_ID_ENEMY_TO_MARIA_END = 0x34
};

static unsigned short MariaDamageMotionNo(struct SubCharacter *this) {
    static int test;
    unsigned short damage_motion_james_body_index[2] = { 0x14, 0x15 };
    unsigned short damage_motion_james_weapon_index[24][2][2] = {
        { { 0x1C, 0x1D }, { 0x1C, 0x1D } }, { { 0x1C, 0x1D }, { 0x1C, 0x1D } }, { { 0, 0 }, { 0, 0 } },
        { { 0x1C, 0x1D }, { 0x1C, 0x1D } }, { { 0, 0 }, { 0, 0 } },         { { 0x1C, 0x1D }, { 0x1C, 0x1D } },
        { { 0, 0 }, { 0, 0 } },             { { 0x16, 0x17 }, { 0x16, 0x17 } }, { { 0x16, 0x17 }, { 0x16, 0x17 } },
        { { 0, 0 }, { 0, 0 } },             { { 0, 0 }, { 0, 0 } },             { { 0x27, 0x28 }, { 0x2D, 0x2D } },
        { { 0x27, 0x28 }, { 0x2D, 0x2D } }, { { 0x22, 0x23 }, { 0x24, 0x25 } }, { { 0x27, 0x28 }, { 0x2D, 0x2D } },
        { { 0x27, 0x28 }, { 0x2D, 0x2D } }, { { 0x22, 0x23 }, { 0x24, 0x25 } }, { { 0x1E, 0x1F }, { 0x20, 0x21 } },
        { { 0x26, 0x26 }, { 0x26, 0x26 } }, { { 0x26, 0x26 }, { 0x26, 0x26 } }, { { 0x26, 0x26 }, { 0x26, 0x26 } },
        { { 0x26, 0x26 }, { 0x26, 0x26 } }, { { 0x27, 0x28 }, { 0x2D, 0x2D } }, { { 0x2B, 0x2C }, { 0x2D, 0x2E } },
    };
    unsigned short damage_motion_enemy_index[16][2][2] = {
        { { 0x16, 0x17 }, { 0x18, 0x19 } }, { { 0x16, 0x17 }, { 0x18, 0x19 } }, { { 0x1A, 0x1B }, { 0x1C, 0x1D } },
        { { 0x1A, 0x1B }, { 0x1C, 0x1D } }, { { 0x22, 0x23 }, { 0x24, 0x25 } }, { { 0x1E, 0x1F }, { 0x20, 0x21 } },
        { { 0x22, 0x23 }, { 0x24, 0x25 } }, { { 0x1E, 0x1F }, { 0x20, 0x21 } }, { { 0x26, 0x26 }, { 0x26, 0x26 } },
        { { 0x26, 0x26 }, { 0x26, 0x26 } }, { { 0x2B, 0x2C }, { 0x2D, 0x2E } }, { { 0, 0 }, { 0, 0 } },
        { { 0, 0 }, { 0, 0 } },             { { 0x27, 0x28 }, { 0x29, 0x2A } }, { { 0x27, 0x28 }, { 0x29, 0x2A } },
        { { 0x2F, 0x2F }, { 0x18, 0x19 } },
    };
    float direction[4];
    float roty;
    float roty2;
    int kind;
    int dir;

    if (!this->battle.id) {
        roty = shAngleRegulate(sh2jms.player->rot.y + sh2jms.player->spd_roty);
    } else {
        _shNormalize(direction, this->battle.vec);
        roty = shAtan2(direction[2], direction[0]);
    }
    roty2 = roty - this->rot.y;
    roty = ANGLE_WRAP(roty2);
    if (roty2 >= -1.5707964f && roty2 < 1.5707964f) {
        dir = 1;
    } else {
        dir = 0;
    }
    if (!this->battle.id) {
        return damage_motion_james_body_index[dir];
    }
    if (this->battle.id <= 0x22) {
        kind = this->battle.id - 1;
        assert(damage_motion_james_weapon_index[kind][sh2mar.dead][dir]!=NULL);
        return damage_motion_james_weapon_index[kind][sh2mar.dead][dir];
    }


    /* Matching: the assert_dw below is the do/while(0) form (its nop). */
    test = kind = this->battle.id - 0x24;
    assert_dw(kind >= 0 && kind < BTL_ID_ENEMY_TO_MARIA_END-BTL_ID_ENEMY_TO_MARIA_START);
    assert(dir >= 0 && dir < 2);
    assert(damage_motion_enemy_index[kind][sh2mar.dead][dir]!=NULL);
    return damage_motion_enemy_index[kind][sh2mar.dead][dir];
}

static int MariaTouchJamesCheck(void) {
    float to_jms[4] = { 0 };
    float jms_vec[4] = { 0 };
    float inner;

    if (sh2mar.dist_to_jms < 365.0f && sh2jms.player->spd > 3.0f) {
        to_jms[0] = sh2jms.player->pos.x - sh2mar.mar_p->pos.x;
        to_jms[2] = sh2jms.player->pos.z - sh2mar.mar_p->pos.z;
        _shNormalize(to_jms, to_jms);
        jms_vec[0] = shSinF(sh2jms.rot_y);
        jms_vec[2] = shCosF(sh2jms.rot_y);
        inner = _shInnerProduct(to_jms, jms_vec);
        if (inner < -0.85f) {
            return 1;
        }
    }
    return 0;
}

static void MariaCheckFootLine(struct SubCharacter *this) {
    float mat[4][4];
    float sp[4];
    float ep[4];

    GetMariaPartsWorldMatrix(mat, 8);
    vcopy(mat[3], sp);
    vcopy(sp, ep);
    sp[1] -= 250.0f;
    ep[1] += 750.0f;
    clCheckHitEyesOnlyFloor(&sh2mar.r_foot, this, sp, ep);
    if (sh2jms.r_foot.kind == 1) {
        _shNormalize(sh2mar.r_foot.hobj.wall.nl, sh2mar.r_foot.hobj.wall.nl);
    }
    GetMariaPartsWorldMatrix(mat, 7);
    vcopy(mat[3], sp);
    vcopy(sp, ep);
    sp[1] -= 250.0f;
    ep[1] += 750.0f;
    clCheckHitEyesOnlyFloor(&sh2mar.l_foot, this, sp, ep);
    if (sh2mar.l_foot.kind == 1) {
        _shNormalize(sh2mar.l_foot.hobj.wall.nl, sh2mar.l_foot.hobj.wall.nl);
    }
}

static void MariaSetSearchArea(struct SubCharacter *this) {
    if (!sh2gfw_Get_NightOrDay()) {
        shBattleSetLookArea(this, 4.0f, 4.0f);
    } else {
        shBattleSetLookArea(this, 1.2f, 1.2f);
    }
    shBattleSetFeelArea(this, 0.2f, 2.0f);
}

static void MariaSetColumn_SetTarget(struct _CL_HITPOLY_COLUMN *mov, struct _CL_HITPOLY_COLUMN *atk, float *mov_z, float *atk_z) {
    struct _AnimeInfo *a_info;

    a_info = shCharacterAnimeGetInfo(sh2mar.mar_p);
    mov->p[0][3] = atk->p[0][3] = 1.0f;
    mov->p[1][0] = atk->p[0][0] = 0.0f;
    mov->p[1][2] = atk->p[0][2] = 0.0f;
    mov->p[0][0] = atk->p[0][0] = sh2mar.mar_p->pos.x;
    mov->p[0][1] = atk->p[0][1] = -50.0f + sh2mar.mar_p->pos.y;
    mov->p[0][2] = atk->p[0][2] = sh2mar.mar_p->pos.z;
    mov->p[1][1] = atk->p[1][1] = -800.0f + sh2mar.mar_p->pos.y;
    *mov_z = 0.0f;
    *atk_z = 0.0f;
    switch (sh2mar.sub_status_now) {
    case MAR_SUB_ST_AFRAID:
        break;
    case MAR_SUB_ST_DAMAGE:
        switch (a_info->name) {
        case 0x15:
            *mov_z = 100.0f;
            break;
        }
        break;
    }
    if (!clPermitColumnExpansion()) {
        mov->p[1][3] = atk->p[1][3] = 150.0f;
    }
}

/* Matching: K&R definition; called with arguments it ignores (the DWARF shows none). */
static void MariaSetColumn_CloseToTarget() {
    sh2mar.column_mov.p[0][0] = sh2mar.column_atk.p[0][0] = sh2mar.mar_p->pos.x;
    sh2mar.column_mov.p[0][1] = sh2mar.column_atk.p[0][1] = -50.0f + sh2mar.mar_p->pos.y;
    sh2mar.column_mov.p[0][2] = sh2mar.column_atk.p[0][2] = sh2mar.mar_p->pos.z;
    sh2mar.column_mov.p[0][3] = sh2mar.column_atk.p[0][3] = 1.0f;
    sh2mar.column_mov.p[1][0] = sh2mar.column_atk.p[1][0] = 0.0f;
    sh2mar.column_mov.p[1][1] = sh2mar.column_atk.p[1][1] = -850.0f + sh2mar.mar_p->pos.y;
    sh2mar.column_mov.p[1][2] = sh2mar.column_atk.p[1][2] = 0.0f;
}

/** Does nothing. */
void MariaSetHeightDummy(void) {
}

/** Sets Maria's ground height and normal from the floor below @p this. */
void MariaSetHeight(struct SubCharacter *this) {
    float sp[4];
    float ep[4];

    vcopy((float *)&this->pos, sp);
    vcopy(sp, ep);
    sp[1] -= 250.0f;
    ep[1] += 1500.0f;
    clCheckHitEyesOnlyFloor(&sh2mar.ft_floor, this, sp, ep);
    if (sh2jms.ft_floor.kind == 1) {
        this->grnd_height = sh2mar.ft_floor.hobj.wall.cp[1];
        *(struct FVEC *)this->grnd_normal = *(struct FVEC *)sh2mar.ft_floor.hobj.wall.nl;
    }
    sh2mar.dist_pos.y = this->grnd_height;
    if (sh2mar.dist_pos.y <= this->pos.y) {
        this->spd_y = 0.0f;
        this->pos.y = sh2mar.dist_pos.y;
    } else {
        this->spd_y += 9.8f * shGetDT() * shGetDT();
        if (sh2jms.dist_pos.y > 100.0f + this->pos.y) {
            SCFreefallSwitch(this, 1);
        } else {
            this->spd_y = 0.0f;
            this->pos.y = sh2jms.dist_pos.y;
        }
        this->spd_y = 0.0f;
        this->pos.y = sh2mar.dist_pos.y;
    }
}

static void MariaSetHitColumn(struct SubCharacter *this) {
    struct _CL_HITPOLY_COLUMN col_mov;
    struct _CL_HITPOLY_COLUMN col_atk;
    float mov_z;
    float atk_z;
    float mat[4][4];

    MariaSetHeight(this);
    MariaSetColumn_SetTarget(&col_mov, &col_atk, &mov_z, &atk_z);
    MariaSetColumn_CloseToTarget(&col_mov, &col_atk, &mov_z, &atk_z);
    if (sh2mar.l_foot.kind == 1) {
        clSetCharaHitColumn(&sh2mar.column_mov, &sh2mar.column_atk, this, MariaSetHeightDummy);
    } else {
        clSetCharaHitColumn(&sh2mar.column_mov, &sh2mar.column_atk, this, NULL);
    }
    this->center_y = 0.5f * (sh2mar.column_mov.p[0][1] + sh2mar.column_mov.p[1][1]);
    GetMariaPartsWorldMatrix(mat, 9);
    this->eye_y = mat[3][1];
}

static void MariaCheckNeckAngle(struct SubCharacter *this) {
    struct SubCharacterDisp *this_d;
    float pos[4];
    float tmp[4];

    switch (sh2mar.sub_status_now) {
    case MAR_SUB_ST_TIRED:
    case MAR_SUB_ST_AFRAID:
    case MAR_SUB_ST_DAMAGE:
        sh2mar.tgt_neck_angle.x = sh2mar.tgt_neck_angle.y = 0.0f;
        break;
    default:
        if (sh2mar.look_tgt) {
            vcopy((float *)&sh2mar.look_tgt->pos, pos);
            pos[1] = sh2mar.look_tgt->eye_y;
            tmp[0] = pos[0] - this->pos.x;
            tmp[1] = pos[1] - this->eye_y;
            tmp[2] = pos[2] - this->pos.z;
            sh2mar.tgt_neck_angle.y = shAtan2(tmp[2], tmp[0]) - this->rot.y;
            sh2mar.tgt_neck_angle.x = -shAtan2(lengthXZ(tmp), tmp[1]);
            sh2mar.tgt_neck_angle.x = ANGLE_WRAP(sh2mar.tgt_neck_angle.x);
            sh2mar.tgt_neck_angle.y = ANGLE_WRAP(sh2mar.tgt_neck_angle.y);
        } else {
            sh2mar.tgt_neck_angle.x = sh2mar.tgt_neck_angle.y = 0.0f;
        }
        break;
    }
    this_d = (struct SubCharacterDisp *)this;
    close_to_angle_target(&this_d->anime.rot_neck.x, sh2mar.tgt_neck_angle.x, -0.4f, 0.5f, 10.0f);
    close_to_angle_target(&this_d->anime.rot_neck.y, sh2mar.tgt_neck_angle.y, -0.8f, 0.8f, 10.0f);
}

static void MariaUpdateStatusInitial(struct SubCharacter *this) {
    sh2mar.enemy_atk_area = shBattleCheckTargetChara(sh2mar.mar_p);
    sh2mar.enemy_around = sh2jms.enemy_around;
    sh2mar.look_tgt = (struct SubCharacter *)shBattleGetTargetChara(sh2mar.mar_p, 0);
    sh2mar.dist_to_jms = _shLength((float *)&sh2jms.player->pos, (float *)&this->pos);
    if (sh2mar.sub_status_now == MAR_SUB_ST_RUN) {
        sh2mar.tired += dt;
        sh2mar.tired = (sh2mar.tired < 0) ? 0 : ((sh2mar.tired > sh2mar.tired_max) ? sh2mar.tired_max : sh2mar.tired);
    }
    if (sh2mar.muteki_time) {
        sh2mar.muteki_time -= dtf;
        if (sh2mar.muteki_time < 0.0f) {
            sh2mar.muteki_time = 0.0f;
        }
    }
}

/* Matching: K&R definition; called with arguments it ignores (the DWARF shows none). */
static void MariaUpdateStatusExecPhase1() {
    if (!sh2mar.no_damage && !sh2mar.random_status) {
        if (sh2mar.active_type >= 2) {
            if (sh2mar.dist_to_jms > 1000.0f || !sh2mar.look_jms) {
                mar_main_st_set(MAR_MAIN_ST_CLOSE_TO, &sh2mar);
                return;
            }
            if (sh2mar.tired >= 400) {
                mar_main_st_set(MAR_MAIN_ST_RECOVER, &sh2mar);
                return;
            }
            if (sh2mar.enemy_around && sh2mar.stand_time >= 6.0f) {
                mar_main_st_set(MAR_MAIN_ST_ALERT, &sh2mar);
                return;
            }
            if (sh2mar.look_obj) {
                mar_main_st_set(MAR_MAIN_ST_DISCOVER, &sh2mar);
                return;
            }
        }
        if (sh2mar.stand_time >= 6.0f || sh2jms.lower_now == JMS_ST_L_RELAX) {
            mar_main_st_set(MAR_MAIN_ST_BOREDOM, &sh2mar);
        } else {
            mar_main_st_set(MAR_MAIN_ST_STAND, &sh2mar);
        }
    }
}

static void MariaUpdateStatusExecPhase2(struct SubCharacter *this) {
    void (*mar_main_func)(struct SubCharacter *);

    mar_main_func = func_list_main[sh2mar.main_status_now];
    mar_main_func(this);
}

static void MariaUpdateStatusExec(struct SubCharacter *this) {
    MariaUpdateStatusExecPhase1(this);
    MariaUpdateStatusExecPhase2(this);
}

/** Gets the world matrix of Maria's skeleton node @p parts_name. */
void GetMariaPartsWorldMatrix(float (*mat)[4], unsigned int parts_name) {
    int i1;
    struct shSkelton *sk;

    sk = sh2mar.mar_p->sk_top;
    for (i1 = 0; i1 < parts_name; i1++) {
        sk = sk->next;
    }
    sceVu0MulMatrix(mat, (float (*)[4])&sh2mar.mar_p->mat, (float (*)[4])&sk->src_m);
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 2236
enum {
    BTL_ID_JAMES_KICK_START = 0x19,
    BTL_ID_JAMES_KICK_END = 0x23
};
/** Collects Maria's battle results for this frame: HP recovery, damage, death. */
void MariaCheckDamage(struct SubCharacter *this) {
    MariaPushedAnyoneIsOn(this);
    vcopy((float *)&this->pos, (float *)&this->b_pos);
    vcopy((float *)&this->rot, (float *)&this->b_rot);
    shBattleGetResult(this);
    if (sh2mar.hp_recover) {
        this->battle.hp += sh2mar.hp_recover;
        sh2mar.hp_recover = 0.0f;
        this->battle.hp = (this->battle.hp < 0.0f)
                              ? 0.0f
                              : ((this->battle.hp > this->battle.hp_max) ? this->battle.hp_max : this->battle.hp);
    }
    if (this->battle.damage || MariaTouchJamesCheck()) {
        if (!sh2mar.no_damage) {
            if (this->battle.id >= 0x19 && this->battle.id <= 0x23) {
                this->battle.damage = 0.0f;
            } else {
                sh2mar.no_damage = 1;
                sh2mar.relax_flag = 0;
                mar_main_st_set(MAR_MAIN_ST_DAMAGED, &sh2mar);
                mar_flg_on(&sh2mar.sub_st_flg, 0x100);
                /* Matching: do/while(0) form (its nop) */
                assert_dw(this->battle.id < BTL_ID_JAMES_KICK_START || this->battle.id > BTL_ID_JAMES_KICK_END);
                if (this->battle.damage) {
                    if (this->battle.id >= 8 && this->battle.id <= 0x18) {
                        this->battle.damage *= 0.1f;
                    }
                    this->battle.hp -= this->battle.damage;
                    GameMariaDamagedCountUp(this->battle.id, this->battle.damage);
                    switch (this->battle.id) {
                    case 1:
                    case 2:
                    case 4:
                    case 6:
                    case 0x13:
                    case 0x14:
                    case 0x15:
                    case 0x16:
                    case 0x2C:
                    case 0x2D:
                        sh2mar.dead = 1;
                        break;
                    }
                    if (this->battle.hp <= 0.0f) {
                        sh2mar.dead = 1;
                    }
                }
                sh2mar.damage_no = MariaDamageMotionNo(this);
                this->battle.damage = 0.0f;
            }
        }
    }
    if (this->battle.damage) {
        this->battle.hp -= this->battle.damage;
    }
    this->battle.hp += dtf;
    this->battle.hp = (this->battle.hp < 0.0f)
                          ? 0.0f
                          : ((this->battle.hp > this->battle.hp_max) ? this->battle.hp_max : this->battle.hp);
    this->battle.damage = 0.0f;
    this->battle.shock = 0.0f;
    this->battle.hp_rate = 100.0f * this->battle.hp / this->battle.hp_max;
    sh2mar.tired = (sh2mar.tired < 0) ? 0 : ((sh2mar.tired > sh2mar.tired_max) ? sh2mar.tired_max : sh2mar.tired);
}

/** First per-frame parameter pass: timing, route to James, search areas. */
void MariaCheckSetParameterPhase1(struct SubCharacter *this) {
    dt = shGetDF();
    dtf = shGetDT();
    if (sh2mar.active_type >= 2) {
        MariaSetStraightRoot(this);
    }
    MariaSetSearchArea(this);
}

/** Second per-frame parameter pass: foot line, hit columns, neck angle. */
void MariaCheckSetParameterPhase2(struct SubCharacter *this) {
    MariaCheckFootLine(this);
    MariaSetHitColumn(this);
    MariaCheckNeckAngle(this);
}

/** Runs Maria's AI: updates and executes her status. */
void MariaCheckControl(struct SubCharacter *this) {
    MariaUpdateStatusInitial(this);
    MariaUpdateStatusExec(this);
}

static void maria_anim_set_all(struct _AnimeInfo *aip, int comp_type) {
    shCharacterAnimeSet(sh2mar.mar_p, 0, comp_type, aip, (int)shCharacterGetAnimeAdrForPlay(sh2mar.mar_p));
}

/** Picks and starts Maria's animation for her current state. */
void MariaCheckAnime(struct SubCharacter *scp) {
    static int anime_change_check;
    struct shMariaWork *w;
    struct SubCharacterDisp *scp_d;
    struct _AnimeInfo *aip;

    w = &sh2mar;
    scp_d = (struct SubCharacterDisp *)sh2mar.mar_p;
    if (!mar_anime_flg_on(2) && (w->sub_status_now != anime_change_check || mar_anime_flg_on(0x40))) {
        switch (w->sub_status_now) {
        case MAR_SUB_ST_STAND:
            aip = (struct _AnimeInfo *)&pmaria_anim[1];
            maria_anim_set_all(aip, 4);
            mar_flg_on(&w->anime_st_flg, 2);
            break;
        case MAR_SUB_ST_RELAX: {
            int anime;

            switch (sh2mar.relax_flag) {
            case 0:
                anime = 0x10;
                break;
            case 1:
                anime = 0x12;
                break;
            case 2:
                anime = 0x13;
                break;
            }
            aip = (struct _AnimeInfo *)&pmaria_anim[anime - 10];
            maria_anim_set_all(aip, 4);
            mar_flg_on(&w->anime_st_flg, 2);
            break;
        }
        case MAR_SUB_ST_RELAX_OFF:
            aip = (struct _AnimeInfo *)&pmaria_anim[7];
            maria_anim_set_all(aip, 4);
            mar_flg_on(&w->anime_st_flg, 2);
            break;
        case MAR_SUB_ST_AFRAID:
            aip = (struct _AnimeInfo *)&pmaria_anim[5];
            maria_anim_set_all(aip, 4);
            mar_flg_on(&w->anime_st_flg, 2);
            break;
        case MAR_SUB_ST_TIRED:
            aip = (struct _AnimeInfo *)&pmaria_anim[4];
            maria_anim_set_all(aip, 4);
            mar_flg_on(&w->anime_st_flg, 2);
            break;
        case MAR_SUB_ST_ONESTEP:
            switch (sh2mar.pushed_dir) {
            case -1:
                aip = (struct _AnimeInfo *)&pmaria_anim[39];
                break;
            case 1:
                aip = (struct _AnimeInfo *)&pmaria_anim[38];
                break;
            case 0:
                break;
            }
            maria_anim_set_all(aip, 4);
            mar_flg_on(&w->anime_st_flg, 2);
            break;
        case MAR_SUB_ST_WALK: {
            int comp_type;

            switch (w->sub_status_prev) {
            case MAR_SUB_ST_RUN:
                comp_type = 10;
                mar_flg_on(&w->anime_st_flg, 4);
                break;
            case MAR_SUB_ST_RELAX_OFF:
                comp_type = 2;
                break;
            default:
                comp_type = 4;
                mar_flg_on(&w->anime_st_flg, 2);
                break;
            }
            aip = (struct _AnimeInfo *)&pmaria_anim[2];
            maria_anim_set_all(aip, comp_type);
            break;
        }
        case MAR_SUB_ST_RUN: {
            int comp_type;

            if (w->sub_status_prev == MAR_SUB_ST_WALK) {
                comp_type = 10;
                mar_flg_on(&w->anime_st_flg, 4);
            } else {
                comp_type = 4;
                mar_flg_on(&w->anime_st_flg, 2);
            }
            aip = (struct _AnimeInfo *)&pmaria_anim[3];
            maria_anim_set_all(aip, comp_type);
            break;
        }
        case MAR_SUB_ST_DAMAGE:
            aip = (struct _AnimeInfo *)&pmaria_anim[sh2mar.damage_no - 10];
            maria_anim_set_all(aip, 4);
            mar_flg_on(&w->anime_st_flg, 2);
            break;
        }
        anime_change_check = w->sub_status_now;
        mar_flg_off(&w->anime_st_flg, 0x40);
    }
    if (scp_d->anime.comp_type <= 2) {
        mar_flg_off(&w->anime_st_flg, 2);
    }
    if (scp_d->anime.comp_type <= 8) {
        mar_flg_off(&w->anime_st_flg, 4);
    }
    if (scp_d->anime.comp_type == -1) {
        mar_flg_on(&w->anime_pause, 1);
    } else {
        mar_flg_off(&w->anime_pause, 1);
    }
}

/** Runs Maria's sub-state function and moves her for this frame. */
void MariaUpdatePosition(struct SubCharacter *this) {
    void (*mar_sub_func)(struct SubCharacter *);
    float cos_x;
    float cos_z;

    mar_sub_func = func_list_sub[sh2mar.sub_status_now];
    mar_sub_func(this);
    if (sh2mar.sub_status_now == MAR_SUB_ST_DAMAGE) {
        this->pos_spd.x = this->spd * dtf * shSinF(PlayerAngleWrap(this->rot.y + this->spd_roty));
        this->pos_spd.z = this->spd * dtf * shCosF(PlayerAngleWrap(this->rot.y + this->spd_roty));
    } else {
        this->pos_spd.x = this->spd * dtf * shSinF(PlayerAngleWrap(sh2mar.to_target));
        this->pos_spd.z = this->spd * dtf * shCosF(PlayerAngleWrap(sh2mar.to_target));
        this->spd_roty = 0.0f;
    }
    this->pos_spd.y = this->spd_y;
    cos_x = 1.0f;
    cos_z = 1.0f;
    this->pos_spd.x = cos_x * (this->pos_spd.x * cos_x);
    this->pos_spd.z = cos_z * (this->pos_spd.z * cos_z);
    sh2mar.pos.x = this->pos_spd.x;
    sh2mar.pos.y = this->pos_spd.y;
    sh2mar.pos.z = this->pos_spd.z;
    sh2mar.pos.x = 500.0f * sh2mar.pos.x;
    sh2mar.pos.y = 500.0f * sh2mar.pos.y;
    sh2mar.pos.z = 500.0f * sh2mar.pos.z;
    SCAddPos(this, &sh2mar.pos);
}

/** Plays Maria's lower- and upper-body animation sounds for this frame. */
void MariaCheckSound(void) {
    MariaCheckSoundLower();
    MariaCheckSoundUpper();
}

/** Sets up Maria's hit columns and places her at the entry point for the room change James just made. */
/* Matching: `!= 0` in the loop test: without it MWCC re-sign-extends the loaded room_name_prev. */
void MariaInitOnConnect(void) {
    int i1;

    i1 = -1;
    /* the kind is set four times in the original; the last store wins */
    sh2mar.column_mov.kind = sh2mar.column_atk.kind = 1;
    sh2mar.column_mov.kind = sh2mar.column_atk.kind = 2;
    sh2mar.column_mov.kind = sh2mar.column_atk.kind = 6;
    sh2mar.column_mov.kind = sh2mar.column_atk.kind = 3;
    sh2mar.column_mov.p[0][3] = sh2mar.column_atk.p[0][3] = 1.0f;
    sh2mar.column_mov.p[1][0] = sh2mar.column_atk.p[0][0] = 0.0f;
    sh2mar.column_mov.p[1][2] = sh2mar.column_atk.p[0][2] = 0.0f;
    if (sh2jms.room_name_prev) {
        for (i1 = 0; maria_apeear_point_list[i1].room_name_prev != 0; i1++) {
            if (maria_apeear_point_list[i1].room_name_prev == sh2jms.room_name_prev &&
                sh2jms.room_name_now == maria_apeear_point_list[i1].room_name_now) {
                sh2mar.mar_p->pos.x = sh2mar.mar_p->b_pos.x = maria_apeear_point_list[i1].pos[0];
                sh2mar.mar_p->pos.y = sh2mar.mar_p->b_pos.y = maria_apeear_point_list[i1].pos[1];
                sh2mar.mar_p->pos.z = sh2mar.mar_p->b_pos.z = maria_apeear_point_list[i1].pos[2];
                sh2mar.mar_p->rot.y = sh2mar.mar_p->b_rot.y = maria_apeear_point_list[i1].pos[3];
                sh2mar.active_type = maria_apeear_point_list[i1].active_type;
                break;
            }
        }
    }
    if (i1 == -1 || !maria_apeear_point_list[i1].room_name_prev) {
        sh2mar.mar_p->pos.x = sh2jms.player->pos.x + 500.0f * shSinF(sh2jms.player->rot.y);
        sh2mar.mar_p->pos.y = sh2jms.player->pos.y;
        sh2mar.mar_p->pos.z = sh2jms.player->pos.z + 500.0f * shCosF(sh2jms.player->rot.y);
        sh2mar.mar_p->rot.y = 3.1415927f + sh2jms.player->pos.y;
        sh2mar.active_type = 2;
    }
    sh2mar.column_mov.p[0][0] = sh2mar.column_atk.p[0][0] = sh2mar.mar_p->pos.x;
    sh2mar.column_mov.p[0][1] = sh2mar.column_atk.p[0][1] = -50.0f + sh2mar.mar_p->pos.y;
    sh2mar.column_mov.p[0][2] = sh2mar.column_atk.p[0][2] = sh2mar.mar_p->pos.z;
    sh2mar.column_mov.p[1][1] = sh2mar.column_atk.p[1][1] = -850.0f + sh2mar.mar_p->pos.y;
    if (sh2mar.active_type == 1) {
        sh2mar.column_mov.p[1][3] = sh2mar.column_atk.p[1][3] = 50.0f;
    } else {
        sh2mar.column_mov.p[1][3] = sh2mar.column_atk.p[1][3] = 150.0f;
    }
    sh2mar.col_mov_z_hosei = 0.0f;
    sh2mar.col_atk_z_hosei = 0.0f;
    sh2mar.mar_p->spd_org = 0.0f;
    sh2mar.mar_p->spd = 0.0f;
    sh2mar.tgt_pointer = 0;
    sh2mar.afraid_time = 0.0f;
    sh2mar.relax_time = 0.0f;
    sh2mar.muteki_time = 0.0f;
    sh2mar.move_time = 0.0f;
    sh2mar.stand_time = 0.0f;
    sh2mar.dist_to_jms = 0.0f;
    sh2mar.tired_max = 0;
    sh2mar.tired = 0;
    sh2mar.no_damage = 0;
    sh2mar.main_status_now = MAR_MAIN_ST_STAND;
    sh2mar.main_status_prev = MAR_MAIN_ST_STAND;
    sh2mar.sub_status_now = MAR_SUB_ST_STAND;
    sh2mar.sub_status_prev = MAR_SUB_ST_STAND;
    mar_flg_on(&sh2mar.sub_st_flg, 1);
    mar_sub_st_set(MAR_SUB_ST_STAND, &sh2mar);
    mar_sub_flg_set(MAR_SUB_ST_STAND, &sh2mar);
    sh2mar.look_jms = 1;
}

/** Sets Maria's ground height (while waiting after a room change). */
void MariaSetHeightConnectWait(void) {
    MariaSetHeight(sh2mar.mar_p);
}

/** Clears Maria's current battle hit id and attack result. */
void MariaStatusClear(void) {
    sh2mar.mar_p->battle.id = 0;
    sh2mar.mar_p->battle.atk_result = 0;
}
