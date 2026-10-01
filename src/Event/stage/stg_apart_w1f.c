/*
 * stg_apart_w1f.c: stage overlay for the apartments, west building 1F: the three-coin puzzle,
 * Angela with the knife, the prisoner coin, the white chrism, Lyne's key and the family picture.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_apart_w1f).
 */
#include "sh2.h"
#include "asm_helpers.h"

/* Next program step. Matching: the do/while leaves the original's nop before a case label it
 * falls into. */
#define EV_STEP(n) do { ev_p_step = (n); ev_s_step = 0; } while (0)
#define EV_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 0x1F)) & 1)

extern float coin_alpha[3];
extern unsigned char coin_kind;
extern unsigned char coin_hole;
extern unsigned char coin_onoff;

static int EvProgLookThreeCoin(void);
static int EvProgSetThreeCoin(void);
static void EvProgSubDrawCoin(void);
static int EvProgSubCoinCursor(void);
static int EvProgAngelaWithKnife(void);
static int EvProgGetCoinOfPrisoner(void);
static int EvProgGetWhiteChrism(void);
static int EvProgGetLyneKey(void);
static int EvProgLookFamilyPicture(void);
static void EvRoomInit(void);
static void EvAllTimeFunc(void);
static int EvBgmControl(void);
static void TrimColorFilter(void);

static struct _AnimeInfo pjames_stage_anim[21] = {
    { 0x4E21, 20, 1024, 1024, 1043, 0, 10 },
    { 0x4E27, 30, 1024, 1044, 1073, 0, 0 },
    { 0x4E28, 15, 768, 1074, 1088, 0, 0 },
    { 0x4E29, 15, 768, 1089, 1103, 0, 0 },
    { 0x4E2A, 30, 768, 1104, 1133, 0, 0 },
    { 0x4E2B, 30, 768, 1134, 1163, 0, 0 },
    { 0x4E2C, 15, 1024, 1168, 1178, 0, 0 },
    { 0x4E2D, 15, 1024, 1183, 1193, 0, 0 },
    { 0x4E2E, 30, 640, 1194, 1223, 0, 0 },
    { 0x4E2F, 30, 640, 1224, 1253, 0, 0 },
    { 0x4E30, 15, 1024, 1258, 1268, 0, 0 },
    { 0x4E31, 15, 1024, 1273, 1283, 0, 0 },
    { 0x4E32, 30, 1024, 1284, 1313, 0, 0 },
    { 0x4E33, 30, 1024, 1314, 1343, 0, 0 },
    { 0x4E34, 30, 1024, 1344, 1373, 0, 0 },
    { 0x4E39, 15, 1024, 1378, 1388, 0, 0 },
    { 0x4E3A, 15, 1024, 1393, 1403, 0, 0 },
    { 0x4E3B, 30, 1024, 1404, 1433, 0, 0 },
    { 0x4E3C, 30, 1024, 1434, 1463, 0, 0 },
    { 0x4E45, 15, 1024, 1464, 1478, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
};

static unsigned char ev_pos[302] = {
    0x8E, 0xC2, 0x6A, 0x47, 0x00, 0x80, 0x0A, 0x13, 0xAB, 0xC6, 0x63, 0x5D, 0xB3, 0x5E, 0x68, 0x47,
    0x00, 0x80, 0x91, 0x7E, 0x8B, 0xC6, 0xF4, 0x5F, 0x00, 0x00, 0x61, 0x47, 0x00, 0x80, 0x00, 0x40,
    0x9C, 0xC6, 0x1A, 0x6C, 0xB0, 0x68, 0x00, 0xE8, 0x64, 0x47, 0x00, 0x80, 0x00, 0xA8, 0xA2, 0xC6,
    0x7F, 0x57, 0xB5, 0x25, 0x66, 0x47, 0x00, 0x80, 0xF4, 0xDE, 0x6F, 0xC7, 0x08, 0x60, 0xC9, 0x4C,
    0x16, 0xE6, 0x6D, 0x47, 0x00, 0x80, 0x66, 0xCB, 0xBF, 0xC7, 0x14, 0x60, 0x71, 0xAE, 0x68, 0x47,
    0x00, 0x80, 0xF1, 0xCD, 0x6F, 0xC7, 0x3B, 0x5F, 0x0D, 0x55, 0x6B, 0x47, 0x00, 0x80, 0x4D, 0x40,
    0xC0, 0xC7, 0x00, 0x90, 0x62, 0x47, 0x00, 0x80, 0x00, 0xD8, 0x6F, 0xC7, 0x1A, 0x6C, 0x78, 0x69,
    0x00, 0x47, 0x6A, 0x47, 0x00, 0x80, 0x80, 0x95, 0xC1, 0xC7, 0x80, 0x80, 0xC2, 0x47, 0x00, 0x80,
    0x74, 0x48, 0x7F, 0xC7, 0xD0, 0x5F, 0x80, 0x80, 0xC2, 0x47, 0x00, 0x80, 0x3A, 0x24, 0x8C, 0xC7,
    0xD0, 0x5F, 0x7D, 0xE7, 0xC4, 0x47, 0x00, 0x80, 0xBF, 0xDB, 0x86, 0xC7, 0xD0, 0x5F, 0x7D, 0xE7,
    0xC4, 0x47, 0x00, 0x80, 0x7E, 0xF7, 0x7A, 0xC7, 0xD0, 0x5F, 0x7D, 0xE7, 0xC4, 0x47, 0x00, 0x80,
    0x7E, 0x37, 0x68, 0xC7, 0xD0, 0x5F, 0x26, 0x1A, 0x61, 0x47, 0x00, 0x80, 0xFE, 0xBC, 0x8D, 0xC6,
    0xD0, 0x5F, 0x7D, 0xE7, 0xC4, 0x47, 0x00, 0x80, 0x7E, 0x77, 0x55, 0xC7, 0xD0, 0x5F, 0x80, 0xE7,
    0xC4, 0x47, 0x00, 0x80, 0x72, 0xD7, 0x45, 0xC7, 0xD0, 0x5F, 0x80, 0x80, 0xC2, 0x47, 0x00, 0x80,
    0x76, 0x68, 0x37, 0xC7, 0xD0, 0x5F, 0x80, 0x80, 0xC2, 0x47, 0x00, 0x80, 0x74, 0x68, 0x50, 0xC7,
    0xD0, 0x5F, 0x0D, 0xE8, 0x72, 0x47, 0x00, 0x80, 0xA8, 0x42, 0x68, 0xC7, 0xD0, 0x5F, 0x8D, 0xE3,
    0xC4, 0x47, 0x00, 0x80, 0x3D, 0xFF, 0x3D, 0xC7, 0x5D, 0x5F, 0xFD, 0x8C, 0xC2, 0x47, 0x00, 0x80,
    0xFB, 0xBE, 0x6D, 0xC7, 0xDF, 0x5F, 0x40, 0x46, 0x0A, 0x48, 0x00, 0x80, 0x05, 0xAE, 0xA0, 0xC6,
    0xDF, 0x5F, 0x8E, 0xC2, 0x6A, 0x47, 0x00, 0x80, 0x0A, 0xDB, 0xAB, 0xC6, 0x41, 0x60,
};

static struct Event_List ev_list[32] = {
    { 0, 0x20004004, 0xA0000000, 0x0001C5A0 },
    { 0x00704078, 0x500C22F4, 0x30000000, 0x00008000 },
    { 0x00704079, 0x500C2304, 0x30000000, 0x00008000 },
    { 0x0070407A, 0x500C2314, 0x30000000, 0x00008000 },
    { 0x808E0070, 0x200C2004, 0x30000000, 0x00018070 },
    { 0x80700000, 0x200C2004, 0x60000000, 0x0002C000 },
    { 0x808D0070, 0x200C2004, 0x30000000, 0x00008000 },
    { 0x008D0070, 0x200C2004, 0x30000000, 0x00004000 },
    { 0x059D0000, 0x4018B004, 0x10000000, 1437 },
    { 0x800A0074, 0x20264004, 0x30000000, 0x00014074 },
    { 0x8055006D, 0x20325000, 0x50402000, 0x0040C06D },
    { 0, 0x20325000, 0x40402000, 0x00400000 },
    { 0, 0x20402000, 0x40325000, 0x00400000 },
    { 0, 0x204C1000, 0x90000000, 0x007F8000 },
    { 0x806D0077, 0x20580004, 0x30000000, 0x00010077 },
    { 0x059E0000, 0x4062B004, 0x10000000, 1438 },
    { 0, 0x20700004, 0x30000000, 0x0001C06E },
    { 0, 0x207A3000, 0x90000000, 0x007FC592 },
    { 0, 0x20863000, 0x90000000, 0x007FC593 },
    { 0, 0x20924000, 0x90000000, 0x007FC594 },
    { 0, 0x209E4000, 0x90000000, 0x007FC595 },
    { 0, 0x20AA4000, 0x40B63000, 0x00400596 },
    { 0, 0x20B63000, 0x40AA4000, 0x00400596 },
    { 0, 0x20C24000, 0x90000000, 0x007FC597 },
    { 0, 0x20CE4000, 0x90000000, 0x007FC598 },
    { 0, 0x20DA3000, 0x90000000, 0x007FC599 },
    { 0, 0x20E63000, 0x40F24000, 0x0040059A },
    { 0, 0x20F24000, 0x40E63000, 0x0040059A },
    { 0, 0x20FE4000, 0x90000000, 0x013F859B },
    { 0, 0x210A3000, 0x41164180, 0x0100059C },
    { 0, 0x21224000, 0x90000000, 0x007FC000 },
    { 0, 0, 0, 0 },
};

static struct Item_List gi_list[9] = {
    { 58720.01f, -20375.0f, 0xDF9C, 0x8000, 0x80000315 },
    { 58676.0f, -20456.99f, 0xDF9C, 0x8000, 0x80000317 },
    { 58732.0f, -20471.99f, 0xDF9C, 0x8000, 0x80000319 },
    { 58145.0f, -21025.0f, 0xDEA7, 0x8000, 0x8000031B },
    { 58183.37f, -21046.0f, 0xDEB4, 0x8000, 0xC000031D },
    { 59062.01f, -59143.96f, 0xDAB0, 0x3785, 0x2000031F },
    { 59120.01f, -59129.96f, 0xDAB8, 0xB866, 0x20000321 },
    { 98879.78f, -58295.53f, 0xDF8F, 0x8000, 0xA0000323 },
    { 0.0f, 0.0f, 0, 0, 0xE0000000 },
};

static int (*ev_prog[8])(void) = {
    NULL,
    EvProgLookThreeCoin,
    EvProgSetThreeCoin,
    EvProgAngelaWithKnife,
    EvProgGetCoinOfPrisoner,
    EvProgGetWhiteChrism,
    EvProgGetLyneKey,
    EvProgLookFamilyPicture,
};

static struct Stage_GfwFunc gfw_func = { NULL, TrimColorFilter, NULL, NULL };

static struct Model_List mdl_list[20] = {
    { 1841, 0, 112, 142, { 59205.004f, -425.99988f, -17771.004f, 0.0f }, { -1.570796f, -0.000001f, 0.960021f, 0.0f } },
    { 1823, 0, 142, 131, { 59153.992f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1823, 0, 142, 132, { 59194.008f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1823, 0, 142, 133, { 59234.2f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1823, 0, 142, 134, { 59276.605f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1823, 0, 142, 135, { 59315.812f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1821, 0, 142, 126, { 59153.992f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1821, 0, 142, 127, { 59194.008f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1821, 0, 142, 128, { 59234.2f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1821, 0, 142, 129, { 59276.605f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1821, 0, 142, 130, { 59315.812f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1820, 0, 142, 136, { 59153.992f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1820, 0, 142, 137, { 59194.008f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1820, 0, 142, 138, { 59234.2f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1820, 0, 0, 139, { 59276.605f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1820, 0, 142, 140, { 59315.812f, -504.80032f, -17824.6f, 0.0f }, { -0.36f, 0.0f, 0.0f, 0.0f } },
    { 1815, 0, 116, 10, { 58685.004f, -472.9999f, -20760.996f, 0.0f }, { 3.141593f, 0.671593f, -3.141592f, 0.0f } },
    { 1820, 0, 119, 85, { 60245.082f, -303.16223f, -98432.62f, 0.0f }, { -1.570796f, -0.000001f, 0.0f, 0.0f } },
    { 1820, 1, 119, 85, { 60244.957f, -303.16223f, -101574.99f, 0.0f }, { 1.570796f, -0.0f, 0.0f, 0.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

static struct Enemy_List en_list[5] = {
    { 513, 57, 100100, -51200, 0, 0, 0, 0 },
    { 513, 58, 100300, -65300, 0, 0, 0, 3 },
    { 514, 59, 99900, -71000, 0, 0, 0, 0 },
    { 513, 60, 100500, -72000, -3, 0, 0, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};

struct Stage_Data stage_apart_w1f = {
    ev_list,
    ev_pos,
    ev_prog,
    gi_list,
    mdl_list,
    en_list,
    NULL,
    EvRoomInit,
    EvAllTimeFunc,
    9,
    1,
    pjames_stage_anim,
    EvBgmControl,
    &gfw_func,
    NULL,
    NULL,
    0,
};

static float key_lyne[2][4] = {
    { 59205.004f, -425.99988f, -17771.004f, 0.0f },
    { -1.570796f, -0.000001f, 0.960021f, 0.0f },
};

float coin_alpha[3];
unsigned char coin_kind;
unsigned char coin_hole;
unsigned char coin_onoff;
static unsigned char cam_change;

static int EvProgLookThreeCoin(void) {
    int i;

    switch (ev_p_step) {
    case 3:
    case 8:
    case 5:
    case 6:
    case 7:
    case 4:
    case 0x1E:
    case 0x1F:
    case 0x20:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        if (ev_p_step == 6 || ev_p_step == 7 || ev_p_step == 4 || ev_p_step == 0x20) {
            EvProgSubDrawCoin();
        }
        if (ev_p_step == 0x1E || ev_p_step == 0x1F || ev_p_step == 0x20) {
            EvSubPictureFilter();
        }
        EvSubPictureEnd();
    }
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        for (i = 0; i < 3; i++) {
            coin_alpha[i] = 1.0f;
        }
        EV_STEP(2);
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_p_desk_hint_tex, NULL)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
    case 3:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(8);
        break;
    case 8:
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(0x1E);
        }
        break;
    case 0x1E:
        if (!EvSubMessage(4)) {
            break;
        }
        EV_STEP(0x1F);
        break;
    case 0x1F:
        if (playing.riddle_level == 0) {
            game_flag.flag[19] |= 2;
            if (!EvSubMessage(0x40)) {
                break;
            }
        } else if (playing.riddle_level == 1) {
            game_flag.flag[3] |= 0x8000000;
            if (!EvSubMessage(1)) {
                break;
            }
        } else if (playing.riddle_level == 2) {
            game_flag.flag[3] |= 0x10000000;
            if (!EvSubMessage(2)) {
                break;
            }
        } else {
            game_flag.flag[3] |= 0x20000000;
            if (!EvSubMessage(3)) {
                break;
            }
        }
        EV_STEP(5);
        break;
    case 5:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_p_desk_coin_tex, data_pic_apt_p_desk_coin_coin_tex)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(6);
        break;
    case 6:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(7);
        break;
    case 7:
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(0x20);
        }
        break;
    case 0x20:
        if (!EvSubMessage(5)) {
            break;
        }
        EV_STEP(4);
        ScreenEffectFadeStart(1, 0.0f);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        ScreenEffectFadeStart(4, 0.0f);
        return 1;
    }
    return 0;
}

static int EvProgSetThreeCoin(void) {
    static short chara_kind[4] = { 1821, 1823, 1820, 0 };
    static float chara_vec[5][2][4] = {
        {
            { 59153.992f, -504.80032f, -17824.6f, 0.0f },
            { -0.36f, 0.0f, 0.0f, 0.0f },
        },
        {
            { 59194.008f, -504.80032f, -17824.6f, 0.0f },
            { -0.36f, 0.0f, 0.0f, 0.0f },
        },
        {
            { 59234.2f, -504.80032f, -17824.6f, 0.0f },
            { -0.36f, 0.0f, 0.0f, 0.0f },
        },
        {
            { 59276.605f, -504.80032f, -17824.6f, 0.0f },
            { -0.36f, 0.0f, 0.0f, 0.0f },
        },
        {
            { 59315.812f, -504.80032f, -17824.6f, 0.0f },
            { -0.36f, 0.0f, 0.0f, 0.0f },
        },
    };
    int i;
    int j;

    switch (ev_p_step) {
    case 3:
    case 0xA:
    case 8:
    case 4:
    case 7:
    case 0x10:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgSubDrawCoin();
        if (ev_p_step == 8) {
            EvSubPictureCursor(0);
        }
        EvSubPictureEnd();
    }
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        for (i = 0; i < 3; i++) {
            coin_alpha[i] = 1.0f;
        }
        if (((game_flag.flag[3] >> 24) & 1) || ((game_flag.flag[3] >> 25) & 1) || ((game_flag.flag[3] >> 26) & 1)) {
            coin_onoff = 1;
        } else {
            coin_onoff = 0;
        }
        ev_cursor_x = ev_cursor_y = 0.0f;
        EV_STEP(2);
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_p_desk_coin_tex, data_pic_apt_p_desk_coin_coin_tex)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
    case 3:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xA);
        break;
    case 0xA:
        if (coin_onoff) {
            if (!EvSubMessage(6)) {
                break;
            }
        } else {
            if (!EvSubMessage(7)) {
                break;
            }
        }
        EV_STEP(8);
        break;
    case 8:
        if (EvProgSubCoinCursor()) {
            EV_STEP(7);
        } else if (shPadTrigger(0, key_config.cancel)) {
            ScreenEffectFadeStart(1, 0.0f);
            EV_STEP(4);
        }
        break;
    case 7:
        if (coin_onoff) {
            coin_alpha[coin_kind] += shGetDT();
            if (coin_alpha[coin_kind] > 1.0f) {
                coin_alpha[coin_kind] = 1.0f;
                ItemUse(coin_kind + 0x2F);
                EV_STEP(0x10);
            }
        } else {
            coin_alpha[coin_kind] -= shGetDT();
            if (coin_alpha[coin_kind] < 0.0f) {
                coin_alpha[coin_kind] = 0.0f;
                game_flag.flag[(coin_kind * 5 + 0x7E + coin_hole) >> 5] &= ~(1 << ((coin_kind * 5 + 0x7E + coin_hole) & 0x1F));
                ItemGet(coin_kind + 0x2F);
                EV_STEP(8);
            }
        }
        break;
    case 0x10:
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            ScreenEffectFadeStart(1, 0.0f);
            EV_STEP(4);
        }
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        for (i = 0; i < 15; i++) {
            if (EV_FLAG(i + 0x7E)) {
                break;
            }
        }
        if (i < 15) {
            game_flag.flag[4] |= 0x2000;
        } else {
            game_flag.flag[4] &= ~0x2000;
        }
        EV_STEP(0xD);
        if ((playing.riddle_level == 0 && (game_flag.flag[4] & 1) && ((game_flag.flag[4] >> 3) & 1) && ((game_flag.flag[4] >> 12) & 1)) ||
            (playing.riddle_level == 1 && ((game_flag.flag[4] >> 2) & 1) && ((game_flag.flag[4] >> 4) & 1) && ((game_flag.flag[4] >> 10) & 1)) ||
            (playing.riddle_level == 2 && ((game_flag.flag[4] >> 1) & 1) && ((game_flag.flag[4] >> 4) & 1) && ((game_flag.flag[4] >> 12) & 1)) ||
            (playing.riddle_level == 3 && (game_flag.flag[4] & 1) && ((game_flag.flag[4] >> 3) & 1) && ((game_flag.flag[4] >> 11) & 1))) {
            game_flag.flag[4] |= 0x4000;
            SeCall(0x3E8A, 1.0f, 0);
            CharaWorkCreate(0x731, 0, key_lyne[0], key_lyne[1], 0);
            EV_STEP(0xD);
        } else {
            EV_STEP(0xD);
        }
        for (i = 0; i < 3; i++) {
            shCharacter_Manage_Delete(NULL, chara_kind[i], 0);
        }
        if (!((game_flag.flag[4] >> 14) & 1)) {
            for (i = 0; i < 3; i++) {
                for (j = 0; j < 5; j++) {
                    if (EV_FLAG(i * 5 + 0x7E + j)) {
                        CharaWorkCreate(chara_kind[i], 0, chara_vec[j][0], chara_vec[j][1], 0);
                    }
                }
            }
        }
        break;
    case 0xD:
        ScreenEffectFadeStart(4, 0.0f);
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        game_flag.flag[3] &= ~0x1000000;
        game_flag.flag[3] &= ~0x2000000;
        game_flag.flag[3] &= ~0x4000000;
        return 1;
    }
    return 0;
}

static void EvProgSubDrawCoin(void) {
    static short coin_xy[3][5][2] = {
        {
            { -3159, -730 },
            { -1787, -703 },
            { -410, -667 },
            { 972, -642 },
            { 2358, -609 },
        },
        {
            { -3167, -697 },
            { -1794, -668 },
            { -420, -641 },
            { 968, -614 },
            { 2352, -581 },
        },
        {
            { -3169, -729 },
            { -1796, -704 },
            { -416, -671 },
            { 964, -636 },
            { 2345, -610 },
        },
    };
    static unsigned char coin_uv[3][2] = { { 0, 0x40 }, { 0, 0 }, { 0, 0x80 } };
    static unsigned char coin_uw = 48;
    static unsigned char coin_vh = 64;
    struct PicDraw_Data pic;
    int i;
    int j;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    for (i = 0; i < 5; i++) {
        if (EV_FLAG(i + 0x7E)) {
            j = 0;
        } else if (EV_FLAG(i + 0x83)) {
            j = 1;
        } else if (EV_FLAG(i + 0x88)) {
            j = 2;
        } else {
            continue;
        }
        pic.x0 = coin_xy[j][i][0];
        pic.y0 = coin_xy[j][i][1];
        pic.x1 = coin_xy[j][i][0] + coin_uw * 16;
        pic.y1 = coin_xy[j][i][1] + coin_vh * 16;
        pic.status |= 2;
        pic.us0 = coin_uv[j][0] * 16;
        pic.vt0 = coin_uv[j][1] * 16;
        pic.us1 = (coin_uw + coin_uv[j][0] - 1) * 16;
        pic.vt1 = (coin_vh + coin_uv[j][1] - 1) * 16;
        pic.status |= 4;
        pic.otp = 3;
        pic.a = ftoi(128.0f * coin_alpha[j]);
        pic.alpha_a = 0;
        pic.alpha_b = 1;
        pic.alpha_c = 0;
        pic.alpha_d = 1;
        pic.alpha_fix = 0x80;
        pic.status |= 0x20;
        PictureDraw(&pic);
    }
}

/** Coin puzzle, on enter: finds the hole under the cursor and updates the coin flags for it. */
static int EvProgSubCoinCursor(void) {
    static float hole_pos[5][2] = {
        { -173.0f, -11.0f },
        { -88.0f, -10.0f },
        { -2.0f, -9.0f },
        { 84.0f, -8.0f },
        { 171.0f, -7.0f },
    };
    float px;
    float py;
    int i;
    int j;

    if (!shPadTrigger(0, key_config.enter)) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        px = 1.25f * (ev_cursor_x - hole_pos[i][0]);
        py = ev_cursor_y - hole_pos[i][1];
        px = sqr(px);
        py = sqr(py);
        if (px + py < 1089.0f) {
            break;
        }
    }
    if (i == 5) {
        return 0;
    }
    if (coin_onoff) {
        if (EV_FLAG(i + 0x7E) || EV_FLAG(i + 0x83) || EV_FLAG(i + 0x88)) {
            return 0;
        }
        if ((game_flag.flag[3] >> 24) & 1) {
            coin_kind = 0;
        }
        if ((game_flag.flag[3] >> 25) & 1) {
            coin_kind = 1;
        }
        if ((game_flag.flag[3] >> 26) & 1) {
            coin_kind = 2;
        }
        game_flag.flag[(coin_kind * 5 + 0x7E + i) >> 5] |= 1 << ((coin_kind * 5 + 0x7E + i) & 0x1F);
        coin_alpha[coin_kind] = 0.0f;
        SeCall(0x3E89, 1.0f, (2 - i) * 16);
    } else {
        for (j = 0; j < 3; j++) {
            if (EV_FLAG(j * 5 + 0x7E + i)) {
                break;
            }
        }
        if (j == 3) {
            return 0;
        }
        coin_kind = j;
    }
    coin_hole = i;
    return 1;
}

static int EvProgAngelaWithKnife(void) {
    static short knife_anim[40] = {
        0x43A, 0xDBC, 0x27A6, 0x279C, 0x43B, 0xDBD, 0x27A7, 0x279D, 0x43C, 0xDBE, 0x27A8, 0x279E,
        0x43D, 0xDBF, 0x27A9, 0x279F, 0x43E, 0xDC0, 0x27AA, 0x27A0, 0x43F, 0xDC1, 0x27AB, 0x27A1,
        0x440, 0xDC2, 0x27AC, 0x27A2, 0x441, 0xDC3, 0x27AD, 0x27A3, 0x442, 0xDC4, 0x27AE, 0x27A4,
        0x443, 0xDC5, 0x27AF, 0x27A5,
    };
    static struct DramaDemo_MessageTime knife_msg[52] = {
        { 0x15, 0x36 }, { 0x36, 0x7B }, { 0xA5, 0xD8 }, { 0xD8, 0x114 }, { 0x114, 0x159 }, { 0x159, 0x1D1 },
        { 0x1D1, 0x204 }, { 0x273, 0x2A9 }, { 0x2A9, 0x309 }, { 0x309, 0x363 }, { 0x363, 0x3C9 },
        { 0x3C9, 0x43E }, { 0x456, 0x4C8 }, { 0x4C8, 0x55B }, { 0x588, 0x5B8 }, { 0x5EE, 0x657 },
        { 0x657, 0x6E1 }, { 0x6E1, 0x756 }, { 0x756, 0x79E }, { 0x79E, 0x816 }, { 0x816, 0x873 },
        { 0x8CA, 0x921 }, { 0x921, 0x936 }, { 0x936, 0x9AE }, { 0x9AE, 0xA05 }, { 0xA05, 0xA3B },
        { 0xA56, 0xA8F }, { 0xA8F, 0xAFB }, { 0xB31, 0xBD0 }, { 0xC0F, 0xC63 }, { 0xC84, 0xD14 },
        { 0xD14, 0xD47 }, { 0xD7A, 0xE10 }, { 0xE6A, 0xEA6 }, { 0xEA6, 0xEF1 }, { 0xEF1, 0x1011 },
        { 0x1011, 0x1068 }, { 0x1068, 0x1161 }, { 0x1161, 0x11C1 }, { 0x11C1, 0x1200 }, { 0x1200, 0x122A },
        { 0x122A, 0x1284 }, { 0x1284, 0x1365 }, { 0x1365, 0x13C5 }, { 0x13EC, 0x1449 }, { 0x1449, 0x14A0 },
        { 0x14A0, 0x14EE }, { 0x14EE, 0x155D }, { 0x15A5, 0x15C6 }, { 0x15EA, 0x1635 }, { 0x1635, 0x1722 },
        { 0xFFFF, 0xFFFF },
    };
    static struct DramaDemo_MessageTime knife_msg_mov[2] = { { 201, 279 }, { 0xFFFF, 0xFFFF } };
    static struct DramaDemo_PlayInfo knife = { 18, MemShare_gp_data_buf, knife_anim, knife_msg, 13, 0, NULL, 60043, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[9] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_knife_agl_hhh_jms_anm, NULL, data_demo_knife_agl_hhh_jms_cls },
        { 291, data_chr_jms_rhhh_jms_mdl, NULL, NULL, NULL },
        { 263, data_chr_agl_agl_mdl, data_demo_knife_agl_agl_anm, NULL, data_demo_knife_agl_agl_cls },
        { 295, data_chr_agl_ragl_mdl, NULL, NULL, NULL },
        { 1059, data_chr_item_i_knife_mdl, data_demo_knife_agl_i_knife_anm, NULL, NULL },
        { 1091, data_chr_item_ri_knife_mdl, NULL, NULL, NULL },
        { 1060, data_chr_item_i_photo_mdl, data_demo_knife_agl_i_photo_anm, NULL, NULL },
        { 1092, data_chr_item_ri_photo_mdl, NULL, NULL, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    static float jms_pos[4] = { 60185.0f, 0.0f, -98580.0f, 0.0f };
    static float jms_rot = 0.0269496f;
    float vec[4];
    int ret;

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_knife_agl_knife_agl_dds, MemShare_gp_data_buf);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        CharaDataLoadDemo(chara_data, 0);
        fsSync(0, -1);
        ScreenEffectFadeStart(3, 0.0f);
        EV_STEP(0x28);
    case 0x28:
        EvSubMovieReady(data_movie_knife_pss, knife_msg_mov, 12);
        if (!EvSubMovieStart(1)) {
            break;
        }
        EV_STEP(0x2B);
        break;
    case 0x2B:
        if (movieGetLastExitStatus()) {
            EV_STEP(0x2C);
        } else {
            EV_STEP(0x2F);
        }
        break;
    case 0x2C:
        EvSubMovieEnd();
        ScreenEffectFadeStart(5, 0.0f);
        EV_STEP(0x16);
    case 0x16:
        ret = DramaDemoMain(&knife);
        if (shCharacterGetSubCharacter(0x123, 0) == NULL) {
            *(u_long128 *)vec = 0;
            CharaWorkCreate(0x123, 0, vec, vec, 0);
            CharaWorkCreate(0x127, 0, vec, vec, 0);
            CharaWorkCreate(0x443, 0, vec, vec, 0);
            CharaWorkCreate(0x444, 0, vec, vec, 0);
        }
        if (demo_frame > total_demo_frame - 30.0f) {
            ScreenEffectFadeStart(2, 1.0f);
        }
        if (!ret) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0x2F:
        EvSubMovieEnd();
        EV_STEP(0xD);
    case 0xD:
        ScreenEffectFadeStart(3, 0.0f);
        CharaDataDeleteOne(0x103);
        CharaDataDeleteOne(0x123);
        CharaDataDeleteOne(0x107);
        CharaDataDeleteOne(0x127);
        CharaDataDeleteOne(0x423);
        CharaDataDeleteOne(0x443);
        CharaDataDeleteOne(0x424);
        CharaDataDeleteOne(0x444);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        shCharacterSetPosAfterDemo(sh2jms.player, jms_pos, jms_rot);
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        ScreenEffectFadeStart(4, 0.0f);
        ItemGet(0x15);
        return 1;
    }
    return 0;
}

static int EvProgGetCoinOfPrisoner(void) {
    return EvSubItemGetAndAnim(0x31, 8);
}

static int EvProgGetWhiteChrism(void) {
    return EvSubItemGetAndAnim(0x49, 0xA);
}

static int EvProgGetLyneKey(void) {
    return EvSubItemGetAndAnim(0x1C, 9);
}

static int EvProgLookFamilyPicture(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x4E21);
        EV_STEP(0x1B);
    case 0x1B:
        if (!PlayerEventAnimeSuccessFrame()) {
            break;
        }
        shCharacterAnimePause(sh2jms.player);
        EV_STEP(0xA);
        break;
    case 0xA:
        if (!EvSubMessage(0)) {
            break;
        }
        EV_STEP(2);
        break;
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_p_family_tex, NULL)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(8);
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            ScreenEffectFadeStart(1, 0.0f);
            EV_STEP(4);
        }
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0x1C);
        shCharacterAnimeRestart(sh2jms.player);
        ScreenEffectFadeStart(4, 0.0f);
    case 0x1C:
        if (!shCharacterAnimeIsEnd(sh2jms.player)) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static void EvRoomInit(void) {
    switch (RoomNameJms()) {
    case 0x24:
        cam_change = 0;
        break;
    }
}

static void EvAllTimeFunc(void) {
    int disp_ctrl_list[3];

    disp_ctrl_list[0] = 0;
    switch (RoomNameJms()) {
    case 0x22:
        EvDispControlModelEntry(disp_ctrl_list, 0x4E, ((game_flag.flag[4] >> 14) & 1) ? 0 : 1);
        break;
    case 0x24:
        if (!((game_flag.flag[2] >> 21) & 1) || ((game_flag.flag[3] >> 13) & 1)) {
            EvDispControlModelEntry(disp_ctrl_list, 0x57, 0);
        }
        if (shCharacterGetSubCharacter(0x71C, 0) == NULL && shCharacterGetSubCharacter(0x71C, 1) != NULL) {
            shCharacter_Manage_Delete(NULL, 0x71C, 1);
        }
        break;
    }
    EvDispControlModelExec(disp_ctrl_list);
}

static int EvBgmControl(void) {
    if (((Sh2sys.main_status >> 6) & 1) && DramaDemoNumber() == 0x12) {
        return 4;
    }
    return 0;
}

static void TrimColorFilter(void) {
    float ddt;
    int ix;

    switch (playing.brightness_level) {
    case 0:
        ix = 0;
        break;
    case 1:
        ix = 1;
        break;
    case 2:
        ix = 2;
        break;
    case 3:
    default:
        ix = 3;
        break;
    case 4:
        ix = 4;
        break;
    case 5:
        ix = 5;
        break;
    case 6:
        ix = 6;
        break;
    case 7:
        ix = 7;
        break;
    }
}
