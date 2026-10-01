/*
 * title.c: the title screen (main menu, battle/riddle level menus, fade-outs
 * into the game), the "coming soon" end screen and the game-over screen.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "libc/stdlib.h"
#include "libc/string.h"


#include "math_const.h"

/* "title.c:<line>> " prefix of the log messages */
#define TLOG(s) __FILE__ ":" SH_STRINGIFY(__LINE__) "> " s

/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG_ON(n) (game_flag.flag[(n) >> 5] |= 1 << ((n) & 31))

#define STEP_CLR3()           \
    Sh2sys.step[3] = 0;       \
    Sh2sys.step[4] = 0;       \
    Sh2sys.step[5] = 0;       \
    Sh2sys.step[6] = 0;       \
    Sh2sys.step[7] = 0
#define STEP_CLR4()           \
    Sh2sys.step[4] = 0;       \
    Sh2sys.step[5] = 0;       \
    Sh2sys.step[6] = 0;       \
    Sh2sys.step[7] = 0
#define STEP_CLR5()           \
    Sh2sys.step[5] = 0;       \
    Sh2sys.step[6] = 0;       \
    Sh2sys.step[7] = 0

#define STEP2_SET(v)          \
    Sh2sys.step[2] = (v);     \
    STEP_CLR3()
#define STEP2_NEXT()          \
    Sh2sys.step[2]++;         \
    STEP_CLR3()
#define STEP2_PREV()          \
    Sh2sys.step[2]--;         \
    STEP_CLR3()
#define STEP3_NEXT()          \
    Sh2sys.step[3]++;         \
    STEP_CLR4()
#define STEP4_NEXT()          \
    Sh2sys.step[4]++;         \
    STEP_CLR5()

static int title_test_mode = 0;

Title_SprData TitleSpr[17] = {
    { 0, 0, 512, 96, 512, 84, { 128, 128, 128, 0 } },
    { 0, 96, 192, 128, 192, 32, { 128, 128, 128, 128 } },
    { 0, 128, 192, 160, 192, 32, { 128, 128, 128, 128 } },
    { 0, 160, 192, 192, 192, 32, { 128, 128, 128, 128 } },
    { 0, 192, 192, 224, 192, 32, { 128, 128, 128, 128 } },
    { 192, 96, 384, 128, 192, 32, { 128, 128, 128, 128 } },
    { 192, 256, 384, 288, 192, 32, { 128, 128, 128, 128 } },
    { 192, 288, 384, 320, 192, 32, { 128, 128, 128, 128 } },
    { 192, 320, 384, 352, 192, 32, { 128, 128, 128, 128 } },
    { 192, 352, 384, 384, 192, 32, { 128, 128, 128, 128 } },
    { 192, 384, 384, 416, 192, 32, { 128, 128, 128, 128 } },
    { 192, 416, 384, 448, 192, 29, { 128, 128, 128, 0 } },
    { 480, 96, 512, 128, 32, 32, { 128, 128, 128, 128 } },
    { 0, 224, 192, 256, 198, 33, { 128, 128, 128, 128 } },
    { 0, 256, 192, 288, 192, 32, { 128, 128, 128, 128 } },
    { 0, 288, 192, 320, 192, 32, { 128, 128, 128, 128 } },
    { 0, 320, 192, 352, 192, 32, { 128, 128, 128, 128 } },
};

Title_ChgColorInfo TitleSprChgColor[17] = {
    { { 255, 255, 255, 128 }, { 38, 38, 38, 8 }, 7.5f, 0.0f },
    { { 32, 32, 32, 128 }, { 128, 128, 128, 128 }, 1.5f, 0.0f },
    { { 32, 32, 32, 128 }, { 128, 128, 128, 128 }, 1.5f, 0.0f },
    { { 32, 32, 32, 128 }, { 128, 128, 128, 128 }, 1.5f, 0.0f },
    { { 32, 32, 32, 128 }, { 128, 128, 128, 128 }, 1.5f, 0.0f },
    { { 128, 128, 128, 128 }, { 128, 128, 128, 128 }, 0.0f, 0.0f },
    { { 128, 128, 128, 128 }, { 128, 128, 128, 128 }, 0.0f, 0.0f },
    { { 32, 32, 32, 128 }, { 128, 128, 128, 128 }, 1.5f, 0.0f },
    { { 32, 32, 32, 128 }, { 128, 128, 128, 128 }, 1.5f, 0.0f },
    { { 32, 32, 32, 128 }, { 128, 128, 128, 128 }, 1.5f, 0.0f },
    { { 32, 32, 32, 128 }, { 128, 128, 128, 128 }, 1.5f, 0.0f },
    { { 70, 70, 70, 38 }, { 255, 255, 255, 128 }, 3.6f, 0.0f },
    { { 96, 96, 255, 128 }, { 32, 32, 160, 128 }, 2.4f, 0.0f },
    { { 128, 128, 128, 128 }, { 128, 128, 128, 128 }, 0.0f, 0.0f },
    { { 128, 128, 128, 128 }, { 128, 128, 128, 128 }, 0.0f, 0.0f },
    { { 128, 128, 128, 128 }, { 128, 128, 128, 128 }, 0.0f, 0.0f },
    { { 128, 128, 128, 128 }, { 128, 128, 128, 128 }, 0.0f, 0.0f },
};

/* test start points: x, y, z, rotation, stage, jump menu id */
Title_StartPoint TitleUSPStartPointList[3] = {
    { -19606.6f, 18.83f, 20429.55f, 0.0f, 2, 16 },
    { -20150.0f, 0.0f, 21200.0f, PI, 7, 11 },
    { -60000.0f, 0.0f, 63600.0f, PI, 31, 12 },
};

Title_StartPoint TitleJPStartPointList[5] = {
    { -19606.6f, 18.83f, 20429.55f, 0.0f, 2, 16 },
    { -100000.0f, 0.0f, -22000.0f, 0.0f, 12, 7 },
    { -20000.0f, 0.0f, -16800.0f, 0.0f, 18, 8 },
    { -20800.0f, 0.0f, 21788.5f, 0.0f, 28, 9 },
    { -60400.0f, 0.0f, 16000.0f, -PI / 2, 40, 10 },
};

Title_Data TitleData;
static int title_after_data_set;
static int title_start_point;
static float analog_data;

static void titleInit(void);
static void titleFadeIn(void);
static void titleChangeMainMenu(void);
static void titleCreateMainMenu(void);
static void titleMainSelect(void);
static int titleCheckPad(void);
static int titleGetCursorFromBattleLevel(unsigned char battle_level);
static void titleBattleSelect(void);
static void titleBacktoMainMenuFromLevelMenu(void);
static unsigned char titleGetBattleLevelFromCursor(int cur);
static int titleGetCursorFromRiddleLevel(unsigned char riddle_level);
static void titleRiddleSelect(void);
static unsigned char titleGetRiddleLevelFromCursor(int cur);
static void titleFadeOutNewGame(void);
static void titleFadeOut(void);
static void titleExit(void);
static void titleDrawTitle(void);
static void titleDrawSprite(short x, short y, short id);
static void titleDrawMainMenu(void);
static void titleDrawSubMenu(int sel);
static short titleGetCursorXPosBattle(int cur);
static void titleGetMenuEfctPosBattle(short *x_pos, short *w, int cur);
static short titleGetCursorXPosRiddle(int cur);
static void titleGetMenuEfctPosRiddle(short *x_pos, short *w, int cur);
static void titleChangeSpriteColor2(short id);
static void titleChangeCursorSpriteColor(void);
static void titleRenewChangeColorManagement(float *timer, float cycle, int repeat);
static void titleChangeColor(int *rgba, int *start_rgba, int *end_rgba, float timer, float cycle_time, float cycle_sin);
static void opd_work_init(void);
static int draw_opd_wowk_main(void);

/** Title screen frame: runs the current title step (Sh2sys.step[2]) and sends its packets.
 * @return the title mode (TitleData.mode), which says where the menu leads */
int TitleMain(void) {
    static void (*g0_step_func[8])(void) = {
        titleInit, titleFadeIn, titleMainSelect, titleBattleSelect,
        titleRiddleSelect, titleFadeOutNewGame, titleFadeOut, titleExit,
    };

    spkResetOT();
    fontClear();
    g0_step_func[Sh2sys.step[2]]();
    d1cSend(spkDmaKick());
    if (TitleData.mode == 5) {
        all_Frame_Buffer_Clear();
    }
    return TitleData.mode;
}

/** In the test modes (debug start), gives the items and game flags of the chosen start point,
 * once per start. */
void titleSetDataStartPoint(void) {
    int i;

    if (title_test_mode) {
        if (!jump_menu_select) {
            if (!title_after_data_set) {
                title_after_data_set = 1;
                ItemGet(0x11);
                ItemGet(0x12);
                item.light_switch = 1;
                if (title_test_mode == 2) {
                    switch (title_start_point) {
                    case 1:
                        ItemGet(4);
                        for (i = 0; i < 3; i++) {
                            ItemGet(5);
                        }
                        ItemGet(0xB);
                        for (i = 0; i < 5; i++) {
                            ItemGet(1);
                        }
                        for (i = 0; i < 1; i++) {
                            ItemGet(2);
                        }
                        ItemGet(0x10);
                        item.equip = 4;
                        GAME_FLAG_ON(25);
                        GAME_FLAG_ON(66);
                        GAME_FLAG_ON(35);
                        GAME_FLAG_ON(40);
                        GAME_FLAG_ON(41);
                        GAME_FLAG_ON(43);
                        GAME_FLAG_ON(49);
                        GAME_FLAG_ON(67);
                        playing.riddle_level = 0;
                        break;
                    case 2:
                        ItemGet(4);
                        for (i = 0; i < 3; i++) {
                            ItemGet(5);
                        }
                        ItemGet(6);
                        for (i = 0; i < 1; i++) {
                            ItemGet(7);
                        }
                        ItemGet(0xB);
                        ItemGet(0xC);
                        for (i = 0; i < 8; i++) {
                            ItemGet(1);
                        }
                        for (i = 0; i < 2; i++) {
                            ItemGet(2);
                        }
                        ItemGet(0xF);
                        ItemGet(0x10);
                        item.equip = 4;
                        GAME_FLAG_ON(28);
                        GAME_FLAG_ON(35);
                        playing.riddle_level = 0;
                        break;
                    }
                } else {
                    switch (title_start_point) {
                    case 1:
                        ItemGet(0xB);
                        ItemGet(0x10);
                        for (i = 0; i < 16; i++) {
                            ItemGet(1);
                        }
                        item.equip = 0xB;
                        break;
                    case 2:
                        GAME_FLAG_ON(15);
                        ItemGet(0xB);
                        ItemGet(4);
                        for (i = 0; i < 9; i++) {
                            ItemGet(5);
                        }
                        ItemGet(0xC);
                        ItemGet(0x10);
                        ItemGet(0xF);
                        for (i = 0; i < 16; i++) {
                            ItemGet(1);
                        }
                        item.equip = 4;
                        break;
                    case 3:
                        ItemGet(0xB);
                        ItemGet(4);
                        for (i = 0; i < 9; i++) {
                            ItemGet(5);
                        }
                        ItemGet(6);
                        for (i = 0; i < 9; i++) {
                            ItemGet(7);
                        }
                        ItemGet(0xC);
                        ItemGet(0x10);
                        ItemGet(0xF);
                        for (i = 0; i < 16; i++) {
                            ItemGet(1);
                        }
                        item.equip = 4;
                        break;
                    case 4:
                        ItemGet(4);
                        for (i = 0; i < 9; i++) {
                            ItemGet(5);
                        }
                        ItemGet(6);
                        for (i = 0; i < 9; i++) {
                            ItemGet(7);
                        }
                        ItemGet(8);
                        for (i = 0; i < 9; i++) {
                            ItemGet(9);
                        }
                        ItemGet(0xA);
                        ItemGet(0xB);
                        ItemGet(0xC);
                        ItemGet(0xD);
                        for (i = 0; i < 16; i++) {
                            ItemGet(1);
                        }
                        ItemGet(0xF);
                        ItemGet(0x10);
                        item.equip = 4;
                        break;
                    }
                }
            }
        }
    }
}

static void titleInit(void) {
    static int fid;
    static int wait_loop;
    int i;

    switch (Sh2sys.step[3]) {
    case 0:
        TitleData.memcard = -1;
        TitleData.mode = 0;
        TitleData.sel = 0;
        TitleData.alpha = 0xFF;
        TitleData.alphar = 0.0f;
        TitleData.timer = 60.0f;
        for (i = 0; i < 10; i++) {
            TitleData.menu[i] = -1;
        }
        TitleData.pload0 = (unsigned char *)get_gp_data_buf_addr();
        TitleData.pload1 = (unsigned char *)get_gp_data_buf_addr() + 0x80000;
        playing.battle_level = 2;
        playing.riddle_level = 1;
        ScreenEffectInit();
        ScreenEffectFadeStart(3, 0.0f);
        fid = FcRead(data_pic_etc_start00_tex, TitleData.pload0);
        wait_loop = 0;
        mcStepInit();
        analog_data = 0.0f;
        STEP3_NEXT();
        break;
    case 1:
        wait_loop++;
        switch (fsSync(1, fid)) {
        case 1:
            /* Matching: the log string bakes its original line number into the object. */
#line 642
            printf(TLOG("title.tex: read finished(%d.%02d)\n"), wait_loop / 60,
                   wait_loop % 60 * 100 / 60);
            STEP3_NEXT();
            break;
        case 0:
            printf(TLOG("!!! illegal fid=%d\n"), fid);
            break;
        case -1:
            if (wait_loop % 60 == 0) {
                printf(TLOG("title.tex: now reading(%d.)...\n"), wait_loop / 60);
            }
            break;
        case -2:
            if (wait_loop % 60 == 0) {
                printf(TLOG("title.tex: now waiting(%d.)...\n"), wait_loop / 60);
            }
            break;
        default:
            printf(TLOG("illegal return value\n"));
            break;
        }
        break;
    case 2:
        fid = FcRead(data_pic_etc_start01_tex, TitleData.pload1);
        wait_loop = 0;
        STEP3_NEXT();
        break;

    case 3:
        titleChangeMainMenu();
        wait_loop++;
        switch (fsSync(1, fid)) {
        case 1:
            /* Matching: the log string bakes its original line number into the object. */
#line 681
            printf(TLOG("title.tex: read finished(%d.%02d)\n"), wait_loop / 60,
                   wait_loop % 60 * 100 / 60);
            STEP2_NEXT();
            break;
        case 0:
            printf(TLOG("!!! illegal fid=%d\n"), fid);
            break;
        case -1:
            if (wait_loop % 60 == 0) {
                printf(TLOG("title.tex: now reading(%d.)...\n"), wait_loop / 60);
            }
            break;
        case -2:
            if (wait_loop % 60 == 0) {
                printf(TLOG("title.tex: now waiting(%d.)...\n"), wait_loop / 60);
            }
            break;
        default:
            printf(TLOG("illegal return value\n"));
            break;
        }
        break;
    }
}

static void titleFadeIn(void) {
    TitleSprChgColor[0].timer = TitleSprChgColor[0].cycle;
    TitleSprChgColor[11].timer = 0.0f;
    switch (Sh2sys.step[3]) {
    case 0:
        titleChangeMainMenu();
        ScreenEffectFadeStart(4, 2.0f);
        titleDrawTitle();
        titleDrawMainMenu();
        STEP3_NEXT();
        break;
    default:
        titleChangeMainMenu();
        titleDrawTitle();
        titleDrawMainMenu();
        if (ScreenEffectFadeCheck()) {
            mc.status |= 8;
            STEP2_SET(2);
        } else if (shPadTrigger(0, key_config.cancel)) {
            ScreenEffectFadeStop();
            mc.status |= 8;
            STEP2_SET(2);
        }
        break;
    }
}

static void titleChangeMainMenu(void) {
    int card;

    if (title_test_mode == 2) {
        titleCreateMainMenu();
    } else {
        card = mcStartCheck2();
        if (card != TitleData.memcard) {
            TitleData.memcard = card;
            titleCreateMainMenu();
        }
    }
}

static void titleCreateMainMenu(void) {
    if (title_test_mode == 2) {
        TitleData.menu[0] = -1;
        TitleData.menu[1] = 3;
        TitleData.menu[2] = 0xE;
        TitleData.menu[3] = 0xD;
        TitleData.menu[4] = -1;
        TitleData.menu[5] = -1;
        if (TitleData.sel == 0) {
            TitleData.sel = 1;
        }
    } else if (title_test_mode == 1) {
        if (TitleData.memcard == 0) {
            if (TitleData.sel != 8) {
                TitleData.sel = 3;
            }
            TitleData.menu[1] = -1;
            TitleData.menu[2] = -1;
            TitleData.menu[3] = 3;
            TitleData.menu[4] = 0xE;
            TitleData.menu[5] = 0x10;
            TitleData.menu[6] = 0xD;
            TitleData.menu[7] = 0xF;
            TitleData.menu[8] = 4;
            TitleData.menu[9] = -1;
        } else if (TitleData.memcard == 1) {
            TitleData.sel = 2;
            TitleData.menu[1] = 1;
            TitleData.menu[2] = 2;
            TitleData.menu[3] = 3;
            TitleData.menu[4] = 0xE;
            TitleData.menu[5] = 0x10;
            TitleData.menu[6] = 0xD;
            TitleData.menu[7] = 0xF;
            TitleData.menu[8] = 4;
            TitleData.menu[9] = -1;
        } else if (TitleData.memcard == 2) {
            TitleData.sel = 2;
            TitleData.menu[1] = -1;
            TitleData.menu[2] = 2;
            TitleData.menu[3] = 3;
            TitleData.menu[4] = 0xE;
            TitleData.menu[5] = 0x10;
            TitleData.menu[6] = 0xD;
            TitleData.menu[7] = 0xF;
            TitleData.menu[8] = 4;
            TitleData.menu[9] = -1;
        } else {
            TitleData.sel = 2;
            TitleData.menu[1] = -1;
            TitleData.menu[2] = 1;
            TitleData.menu[3] = 3;
            TitleData.menu[4] = 0xE;
            TitleData.menu[5] = 0x10;
            TitleData.menu[6] = 0xD;
            TitleData.menu[7] = 0xF;
            TitleData.menu[8] = 4;
            TitleData.menu[9] = -1;
        }
    } else {
        if (TitleData.memcard == 0) {
            if (TitleData.sel != 4) {
                TitleData.sel = 3;
            }
            TitleData.menu[1] = -1;
            TitleData.menu[2] = -1;
            TitleData.menu[3] = 3;
            TitleData.menu[4] = 4;
        } else if (TitleData.memcard == 1) {
            TitleData.sel = 2;
            TitleData.menu[1] = 1;
            TitleData.menu[2] = 2;
            TitleData.menu[3] = 3;
            TitleData.menu[4] = 4;
        } else if (TitleData.memcard == 2) {
            TitleData.sel = 2;
            TitleData.menu[1] = -1;
            TitleData.menu[2] = 2;
            TitleData.menu[3] = 3;
            TitleData.menu[4] = 4;
        } else {
            TitleData.sel = 2;
            TitleData.menu[1] = -1;
            TitleData.menu[2] = 1;
            TitleData.menu[3] = 3;
            TitleData.menu[4] = 4;
        }
    }
}

static void titleMainSelect(void) {
    int pad_ret;
    int select_game_start;
    int select_option;
    int select_load;

    titleChangeMainMenu();
    titleDrawTitle();
    titleDrawMainMenu();
    pad_ret = titleCheckPad();
    switch (pad_ret) {
    case 0:
        break;
    case 1:
        TitleData.timer = 60.0f;
        shSdCall(10000, 0, 0, 0);
        TitleSprChgColor[11].timer = 0.0f;
        break;
    case 2:
        TitleData.timer = 60.0f;
        TitleSprChgColor[12].timer = 0.0f;
        title_after_data_set = 0;
        if (title_test_mode == 2) {
            if (TitleData.sel != 4) {
                title_start_point = TitleData.sel - 1;
            }
            playing.battle_level = 2;
            playing.riddle_level = 0;
            SeCall(0x3A9A, 1.0f, 0);
            STEP2_SET(6);
        } else {
            if (title_test_mode == 1) {
                select_game_start = TitleData.sel >= 3 && TitleData.sel < 8;
                select_option = TitleData.sel == 8;
                select_load = TitleData.sel == 1;
            } else {
                select_game_start = TitleData.sel == 3;
                select_option = TitleData.sel == 4;
                select_load = TitleData.sel == 1;
            }
            if (select_game_start) {
                if (title_test_mode == 1) {
                    title_start_point = TitleData.sel - 3;
                }
                TitleData.menu[0] = -1;
                TitleData.menu[1] = 1;
                TitleData.menu[2] = 2;
                TitleData.menu[3] = 3;
                TitleData.menu[4] = -1;
                TitleData.menu[5] = -1;
                TitleData.menu[6] = -1;
                TitleData.menu[7] = -1;
                TitleData.menu[8] = -1;
                TitleData.menu[9] = -1;
                TitleData.sel = titleGetCursorFromBattleLevel(playing.battle_level);
                TitleData.alphar = 0.0f;
                TitleData.timer = 60.0f;
                shSdCall(0x2712, 0, 0, 0);
                STEP2_NEXT();
            } else {
                if (select_option || select_load) {
                    shSdCall(0x2712, 0, 0, 0);
                } else {
                    SeCall(0x3A9A, 1.0f, 0);
                }
                STEP2_SET(6);
            }
        }
        break;
    case 3:
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 1080
        assert(0);
    }
    TitleData.timer -= shGetDT();
    if (TitleData.timer <= 0.0f) {
        STEP2_SET(7);
    }
    TitleData.alphar += 0.06981317f;
    TitleData.alpha = 191.0f + 63.0f * shCosF(TitleData.alphar);
}

static int titleCheckPad(void) {
    int ret;
    int an;

    ret = 0;
    an = shPadPress(0, 0x80);
    if (an > 0x60 && an < 0xA0) {
        analog_data = 0.0f;
    } else if (an < 0x80) {
        if (analog_data == 0.0f) {
            analog_data = -1.000001f;
        } else {
            analog_data -= itof(0x60 - an) / 19.2f * shGetDT();
        }
    } else {
        if (analog_data == 0.0f) {
            analog_data = 1.000001f;
        } else {
            analog_data += itof(an - 0xA0) / 19.2f * shGetDT();
        }
    }
    if (shPadTrigger(0, 0x400) || shPadRepeat(0, 0x400) || analog_data < -1.0f) {
        analog_data += 1.0f;
        if (analog_data > 0.0f) {
            analog_data = 0.0f;
        }
        if (TitleData.menu[TitleData.sel - 1] != -1) {
            TitleData.sel--;
        } else {
            TitleData.sel = 9;
            while (TitleData.menu[TitleData.sel] == -1) {
                TitleData.sel--;
            }
        }
        ret = 1;
    } else if (shPadTrigger(0, 0x800) || shPadRepeat(0, 0x800) || analog_data > 1.0f) {
        analog_data -= 1.0f;
        if (analog_data < 0.0f) {
            analog_data = 0.0f;
        }
        if (TitleData.menu[TitleData.sel + 1] != -1) {
            TitleData.sel++;
        } else {
            TitleData.sel = 0;
            while (TitleData.menu[TitleData.sel] == -1) {
                TitleData.sel++;
            }
        }
        ret = 1;
    } else if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, 4)) {
        ret = 2;
    } else if (shPadTrigger(0, key_config.cancel)) {
        ret = 3;
    }
    return ret;
}

static int titleGetCursorFromBattleLevel(unsigned char battle_level) {
    int ret;

    switch (battle_level) {
    case 3:
        ret = 1;
        break;
    case 2:
        ret = 2;
        break;
    case 1:
        ret = 3;
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 1279
        assert(0);
    }
    return ret;
}

static void titleBattleSelect(void) {
    int pad_ret;

    titleDrawTitle();
    titleDrawSubMenu(0);
    TitleData.timer -= shGetDT();
    pad_ret = titleCheckPad();
    switch (pad_ret) {
    case 0:
        break;
    case 1:
        TitleData.timer = 60.0f;
        shSdCall(10000, 0, 0, 0);
        TitleSprChgColor[11].timer = 0.0f;
        break;
    case 2:
        TitleSprChgColor[12].timer = 0.0f;
        TitleSprChgColor[11].timer = 0.0f;
        playing.battle_level = titleGetBattleLevelFromCursor(TitleData.sel);
        TitleData.timer = 60.0f;
        TitleData.sel = titleGetCursorFromRiddleLevel(playing.riddle_level);
        TitleData.alphar = 0.0f;
        shSdCall(0x2712, 0, 0, 0);
        STEP2_NEXT();
        break;
    case 3:
        TitleSprChgColor[12].timer = 0.0f;
        TitleSprChgColor[11].timer = 0.0f;
        playing.battle_level = titleGetBattleLevelFromCursor(TitleData.sel);
        titleBacktoMainMenuFromLevelMenu();
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 1368
        assert(0);
    }
    if (TitleData.timer <= 0.0f) {
        TitleSprChgColor[12].timer = 0.0f;
        TitleSprChgColor[11].timer = 0.0f;
        playing.battle_level = titleGetBattleLevelFromCursor(TitleData.sel);
        titleBacktoMainMenuFromLevelMenu();
    }
    TitleData.alphar += 0.06981317f;
    TitleData.alpha = 191.0f + 63.0f * shCosF(TitleData.alphar);
}

static void titleBacktoMainMenuFromLevelMenu(void) {
    TitleData.memcard = mcStartCheck2();
    titleCreateMainMenu();
    TitleData.timer = 60.0f;
    TitleData.sel = 3;
    shSdCall(0x2713, 0, 0, 0);
    STEP2_SET(2);
}

static unsigned char titleGetBattleLevelFromCursor(int cur) {
    unsigned char ret;

    switch (cur) {
    case 1:
        ret = 3;
        break;
    case 2:
        ret = 2;
        break;
    case 3:
        ret = 1;
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 1452
        assert(0);
    }
    return ret;
}

static int titleGetCursorFromRiddleLevel(unsigned char riddle_level) {
    int ret;

    switch (riddle_level) {
    case 2:
        ret = 1;
        break;
    case 1:
        ret = 2;
        break;
    case 0:
        ret = 3;
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 1494
        assert_dw(0);
    }
    return ret;
}

static void titleRiddleSelect(void) {
    int pad_ret;

    titleDrawTitle();
    titleDrawSubMenu(1);
    TitleData.timer -= shGetDT();
    pad_ret = titleCheckPad();
    switch (pad_ret) {
    case 0:
        break;
    case 1:
        TitleData.timer = 60.0f;
        shSdCall(10000, 0, 0, 0);
        TitleSprChgColor[11].timer = 0.0f;
        break;
    case 2:
        TitleSprChgColor[12].timer = 0.0f;
        TitleSprChgColor[11].timer = 0.0f;
        playing.riddle_level = titleGetRiddleLevelFromCursor(TitleData.sel);
        title_after_data_set = 0;
        TitleData.timer = 60.0f;
        SeCall(0x3A9A, 1.0f, 0);
        STEP2_SET(5);
        break;
    case 3:
        TitleSprChgColor[12].timer = 0.0f;
        TitleSprChgColor[11].timer = 0.0f;
        playing.riddle_level = titleGetRiddleLevelFromCursor(TitleData.sel);
        TitleData.sel = titleGetCursorFromBattleLevel(playing.battle_level);
        TitleData.alphar = 0.0f;
        TitleData.timer = 60.0f;
        shSdCall(0x2713, 0, 0, 0);
        STEP2_PREV();
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 1580
        assert(0);
    }
    if (TitleData.timer <= 0.0f) {
        TitleSprChgColor[12].timer = 0.0f;
        TitleSprChgColor[11].timer = 0.0f;
        playing.riddle_level = titleGetRiddleLevelFromCursor(TitleData.sel);
        titleBacktoMainMenuFromLevelMenu();
    }
    TitleData.alphar += 0.06981317f;
    TitleData.alpha = 191.0f + 63.0f * shCosF(TitleData.alphar);
}

static unsigned char titleGetRiddleLevelFromCursor(int cur) {
    unsigned char ret;

    switch (cur) {
    case 1:
        ret = 2;
        break;
    case 2:
        ret = 1;
        break;
    case 3:
        ret = 0;
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 1639
        assert(0);
    }
    return ret;
}

static void titleFadeOutNewGame(void) {
    switch (Sh2sys.step[3]) {
    case 0:
        ScreenEffectFadeStart(2, 2.0f);
        TitleData.alpha = 0xFF;
        titleDrawTitle();
        if (title_test_mode != 2) {
            titleDrawSubMenu(2);
        }
        STEP3_NEXT();
        break;
    default:
        if (ScreenEffectFadeCheck()) {
            if (title_test_mode == 0) {
                connect_pos[0] = TitleJPStartPointList[0].pos_x;
                connect_pos[1] = TitleJPStartPointList[0].pos_y;
                connect_pos[2] = TitleJPStartPointList[0].pos_z;
                connect_pos[3] = TitleJPStartPointList[0].rot;
                playing.stage = TitleJPStartPointList[0].stg;
            } else if (title_test_mode == 1) {
                connect_pos[0] = TitleJPStartPointList[title_start_point].pos_x;
                connect_pos[1] = TitleJPStartPointList[title_start_point].pos_y;
                connect_pos[2] = TitleJPStartPointList[title_start_point].pos_z;
                connect_pos[3] = TitleJPStartPointList[title_start_point].rot;
                playing.stage = TitleJPStartPointList[title_start_point].stg;
            }
            TitleData.mode = 3;
        } else {
            TitleSprChgColor[0].timer = TitleSprChgColor[0].cycle;
            titleDrawTitle();
            if (title_test_mode != 2) {
                titleDrawSubMenu(2);
            }
        }
        break;
    }
}

static void titleFadeOut(void) {
    switch (Sh2sys.step[3]) {
    case 0:
        ScreenEffectFadeStart(2, 2.0f);
        TitleData.alphar = 0.0f;
        TitleData.alpha = 191.0f + 63.0f * shCosF(TitleData.alphar);
        titleDrawTitle();
        titleDrawMainMenu();
        STEP3_NEXT();
        break;
    default:
        if (ScreenEffectFadeCheck()) {
            if (title_test_mode == 2) {
                connect_pos[0] = TitleUSPStartPointList[title_start_point].pos_x;
                connect_pos[1] = TitleUSPStartPointList[title_start_point].pos_y;
                connect_pos[2] = TitleUSPStartPointList[title_start_point].pos_z;
                connect_pos[3] = TitleUSPStartPointList[title_start_point].rot;
                playing.stage = TitleUSPStartPointList[title_start_point].stg;
            }
            TitleData.mode = TitleData.menu[TitleData.sel];
        } else {
            TitleData.alphar += PI / 2;
            TitleData.alpha = 191.0f + 63.0f * shCosF(TitleData.alphar);
            TitleSprChgColor[0].timer = TitleSprChgColor[0].cycle;
            titleDrawTitle();
            titleDrawMainMenu();
        }
        break;
    }
}

static void titleExit(void) {
    switch (Sh2sys.step[3]) {
    case 0:
        ScreenEffectFadeStart(2, 2.0f);
        TitleData.alphar = 0.0f;
        TitleData.alpha = 191.0f + 63.0f * shCosF(TitleData.alphar);
        titleDrawTitle();
        titleDrawMainMenu();
        STEP3_NEXT();
        break;
    default:
        if (ScreenEffectFadeCheck()) {
            TitleData.mode = 5;
        }
        if (TitleData.timer > 0.0f) {
            TitleData.mode = 6;
        } else {
            TitleData.alphar += 0.06981317f;
            TitleData.alpha = 191.0f + 63.0f * shCosF(TitleData.alphar);
            titleDrawTitle();
            titleDrawMainMenu();
        }
        break;
    }
}

static void titleDrawTitle(void) {
    Title_ChgColorInfo *chg_color_info;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)TitleData.pload0, 0, -1, -1);
    shQzero(&TitleData.pic0, sizeof(TitleData.pic0));
    TitleData.pic0.ap = (struct sh2gfw_AREA_HEAD *)TitleData.pload0;
    TitleData.pic0.tex = -1;
    TitleData.pic0.clut = -1;
    TitleData.pic0.status |= 1;
    TitleData.pic0.otp = 1;
    TitleData.pic0.x0 = -0x1000;
    TitleData.pic0.y0 = -0xE00;
    TitleData.pic0.x1 = 0x1000;
    TitleData.pic0.y1 = 0xE00;
    TitleData.pic0.status |= 2;
    TitleData.pic0.us0 = 0;
    TitleData.pic0.vt0 = 0;
    TitleData.pic0.us1 = 0x2000;
    TitleData.pic0.vt1 = 0x2000;
    TitleData.pic0.status |= 4;
    PictureDraw(&TitleData.pic0);
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)TitleData.pload1, 2, -1, -1);
    chg_color_info = &TitleSprChgColor[0];
    titleRenewChangeColorManagement(&chg_color_info->timer, chg_color_info->cycle, 1);
    titleChangeColor(TitleSpr[0].rgba, chg_color_info->start_rgba, chg_color_info->end_rgba, chg_color_info->timer,
                     chg_color_info->cycle, 0.5f);
    titleDrawSprite(-0x101, -0x67, 0);
}

static void titleDrawSprite(short x, short y, short id) {
    shQzero(&TitleData.pic0, sizeof(TitleData.pic0));
    TitleData.pic0.ap = (struct sh2gfw_AREA_HEAD *)TitleData.pload1;
    TitleData.pic0.tex = -1;
    TitleData.pic0.clut = -1;
    TitleData.pic0.status |= 1;
    TitleData.pic0.r = TitleSpr[id].rgba[0];
    TitleData.pic0.g = TitleSpr[id].rgba[1];
    TitleData.pic0.b = TitleSpr[id].rgba[2];
    TitleData.pic0.status |= 0x10;
    TitleData.pic0.a = TitleSpr[id].rgba[3];
    TitleData.pic0.alpha_a = 0;
    TitleData.pic0.alpha_b = 1;
    TitleData.pic0.alpha_c = 0;
    TitleData.pic0.alpha_d = 1;
    TitleData.pic0.alpha_fix = 0x80;
    TitleData.pic0.status |= 0x20;
    if (id == 11) {
        TitleData.pic0.otp = 3;
    } else {
        TitleData.pic0.otp = 5;
    }
    TitleData.pic0.x0 = x * 16;
    TitleData.pic0.y0 = y * 16;
    TitleData.pic0.x1 = (x + TitleSpr[id].w) * 16;
    TitleData.pic0.y1 = (y + TitleSpr[id].h) * 16;
    TitleData.pic0.status |= 2;
    TitleData.pic0.us0 = TitleSpr[id].u0 * 16;
    TitleData.pic0.vt0 = TitleSpr[id].v0 * 16;
    TitleData.pic0.us1 = TitleSpr[id].u1 * 16;
    TitleData.pic0.vt1 = TitleSpr[id].v1 * 16;
    TitleData.pic0.status |= 4;
    PictureDraw(&TitleData.pic0);
}

static void titleDrawMainMenu(void) {
    short cpos[8];
    short cposy[8];
    int i;
    int j;
    short ef_pos[7];
    short w[7];

    if (title_test_mode == 2) {
        cpos[0] = -0x5E;
        cpos[1] = -0x56;
        cpos[2] = -0x46;
        cpos[3] = -0x4E;
        cposy[0] = 0x4D;
        cposy[1] = 0x63;
        cposy[2] = 0x79;
        cposy[3] = 0x8F;
    } else if (title_test_mode == 1) {
        cpos[0] = -0x44;
        cpos[1] = -0x5C;
        cpos[2] = -0x5E;
        cpos[3] = -0x56;
        cpos[4] = -0x4A;
        cpos[5] = -0x46;
        cpos[6] = -0x3E;
        cpos[7] = -0x4E;
        cposy[0] = 0x8;
        cposy[1] = 0x1E;
        cposy[2] = 0x34;
        cposy[3] = 0x4A;
        cposy[4] = 0x60;
        cposy[5] = 0x76;
        cposy[6] = 0x8C;
        cposy[7] = 0xA2;
    } else {
        cpos[0] = -0x44;
        cpos[1] = -0x5C;
        cpos[2] = -0x5E;
        cpos[3] = -0x4E;
        cposy[0] = 0x4A;
        cposy[1] = 0x60;
        cposy[2] = 0x76;
        cposy[3] = 0x8C;
    }
    if (title_test_mode == 2) {
        ef_pos[0] = -0x50;
        ef_pos[1] = -0x47;
        ef_pos[2] = -0x32;
        ef_pos[3] = -0x3C;
        w[0] = 0xA0;
        w[1] = 0x8E;
        w[2] = 0x64;
        w[3] = 0x78;
    } else if (title_test_mode == 1) {
        ef_pos[0] = -0x30;
        ef_pos[1] = -0x4E;
        ef_pos[2] = -0x50;
        ef_pos[3] = -0x47;
        ef_pos[4] = -0x3A;
        ef_pos[5] = -0x32;
        ef_pos[6] = -0x2A;
        ef_pos[7] = -0x3C;
        w[0] = 0x60;
        w[1] = 0x9C;
        w[2] = 0xA0;
        w[3] = 0x8E;
        w[4] = 0x74;
        w[5] = 0x64;
        w[6] = 0x54;
        w[7] = 0x78;
    } else {
        ef_pos[0] = -0x30;
        ef_pos[1] = -0x4E;
        ef_pos[2] = -0x50;
        ef_pos[3] = -0x3C;
        w[0] = 0x60;
        w[1] = 0x9C;
        w[2] = 0xA0;
        w[3] = 0x78;
    }
    titleChangeSpriteColor2(11);
    TitleSpr[11].w = w[TitleData.sel - 1] + 8;
    titleDrawSprite(ef_pos[TitleData.sel - 1] - 4, cposy[TitleData.sel - 1] + 1, 11);
    if (title_test_mode == 2) {
        titleDrawSprite(-0x60, cposy[0], 3);
        titleDrawSprite(-0x60, cposy[1], 0xE);
        titleDrawSprite(-0x63, cposy[2], 0xD);
        titleChangeCursorSpriteColor();
        titleDrawSprite(cpos[TitleData.sel - 1], cposy[TitleData.sel - 1], 0xC);
    } else if (title_test_mode == 1) {
        for (i = 0; i < 2; i++) {
            if (TitleData.menu[i + 1] != -1) {
                j = TitleData.menu[i + 1] - 1;
                titleDrawSprite(-0x60, cposy[i], j + 1);
            }
        }
        titleDrawSprite(-0x60, cposy[2], 3);
        titleDrawSprite(-0x60, cposy[3], 0xE);
        titleDrawSprite(-0x60, cposy[4], 0x10);
        titleDrawSprite(-0x63, cposy[5], 0xD);
        titleDrawSprite(-0x60, cposy[6], 0xF);
        titleDrawSprite(-0x60, cposy[7], 4);
        titleChangeCursorSpriteColor();
        titleDrawSprite(cpos[TitleData.sel - 1], cposy[TitleData.sel - 1], 0xC);
    } else {
        for (i = 0; i < 4; i++) {
            if (TitleData.menu[i + 1] != -1) {
                j = TitleData.menu[i + 1] - 1;
                titleDrawSprite(-0x60, cposy[i], j + 1);
                if (TitleData.sel == i + 1) {
                    titleChangeCursorSpriteColor();
                    titleDrawSprite(cpos[j], cposy[i], 0xC);
                }
            }
        }
    }
}

static void titleDrawSubMenu(int sel) {
    titleDrawSprite(-0xC4, 0x3C, 5);
    titleDrawSprite(0xB, 0x3C, 6);
    if (sel == 0) {
        short x_pos;

        TitleSpr[11].h -= 7;
        titleChangeSpriteColor2(11);
        titleGetMenuEfctPosBattle(&x_pos, &TitleSpr[11].w, TitleData.sel);
        titleDrawSprite(x_pos, TitleData.sel * 18 + 0x42, 11);
        TitleSpr[11].h += 7;
        titleDrawSprite(-0xC4, 0x50, 7);
        titleDrawSprite(-0xC4, 0x62, 8);
        titleDrawSprite(-0xC4, 0x73, 9);
        titleDrawSprite(0xB, 0x50, 7);
        titleDrawSprite(0xB, 0x62, 8);
        titleDrawSprite(0xB, 0x73, 9);
        titleChangeCursorSpriteColor();
        titleDrawSprite(titleGetCursorXPosBattle(TitleData.sel), TitleData.sel * 18 + 0x3E, 0xC);
    } else if (sel == 1) {
        short x_pos;

        TitleSpr[11].h -= 7;
        titleChangeSpriteColor2(11);
        titleGetMenuEfctPosRiddle(&x_pos, &TitleSpr[11].w, TitleData.sel);
        titleDrawSprite(x_pos, TitleData.sel * 18 + 0x42, 11);
        TitleSpr[11].h += 7;
        titleDrawSprite(0xB, 0x50, 7);
        titleDrawSprite(0xB, 0x62, 8);
        titleDrawSprite(0xB, 0x73, 9);
        switch (playing.battle_level) {
        case 1:
            titleDrawSprite(-0xC4, 0x73, 9);
            break;
        case 2:
            titleDrawSprite(-0xC4, 0x62, 8);
            break;
        case 3:
            titleDrawSprite(-0xC4, 0x50, 7);
            break;
        }
        {
            int i;
            short x_pos;

            for (i = 0; i < 4; i++) {
                TitleSpr[11].rgba[i] = TitleSprChgColor[11].end_rgba[i];
            }
            TitleSpr[11].h -= 7;
            titleGetMenuEfctPosBattle(&x_pos, &TitleSpr[11].w, titleGetCursorFromBattleLevel(playing.battle_level));
            titleDrawSprite(x_pos, titleGetCursorFromBattleLevel(playing.battle_level) * 18 + 0x42, 11);
            TitleSpr[11].h += 7;
        }
        titleChangeCursorSpriteColor();
        titleDrawSprite(titleGetCursorXPosRiddle(TitleData.sel), TitleData.sel * 18 + 0x3E, 0xC);
    } else {
        switch (playing.battle_level) {
        case 1:
            titleDrawSprite(-0xC4, 0x73, 9);
            break;
        case 2:
            titleDrawSprite(-0xC4, 0x62, 8);
            break;
        case 3:
            titleDrawSprite(-0xC4, 0x50, 7);
            break;
        }
        switch (playing.riddle_level) {
        case 0:
            titleDrawSprite(0xB, 0x73, 9);
            break;
        case 1:
            titleDrawSprite(0xB, 0x62, 8);
            break;
        case 2:
            titleDrawSprite(0xB, 0x50, 7);
            break;
        }
    }
}

static short titleGetCursorXPosBattle(int cur) {
    short ret;

    switch (cur) {
    case 1:
        ret = -0xA0;
        break;
    case 2:
        ret = -0xAC;
        break;
    case 3:
        ret = -0x9D;
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 2505
        assert(0);
    }
    return ret;
}

static void titleGetMenuEfctPosBattle(short *x_pos, short *w, int cur) {
    switch (cur) {
    case 1:
        *x_pos = -0x8D;
        *w = 0x52;
        break;
    case 2:
        *x_pos = -0x9C;
        *w = 0x70;
        break;
    case 3:
        *x_pos = -0x8A;
        *w = 0x4C;
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 2562
        assert_dw(0);
    }
}

static short titleGetCursorXPosRiddle(int cur) {
    short ret;

    switch (cur) {
    case 1:
        ret = 0x2F;
        break;
    case 2:
        ret = 0x23;
        break;
    case 3:
        ret = 0x32;
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 2608
        assert(0);
    }
    return ret;
}

static void titleGetMenuEfctPosRiddle(short *x_pos, short *w, int cur) {
    switch (cur) {
    case 1:
        *x_pos = 0x42;
        *w = 0x52;
        break;
    case 2:
        *x_pos = 0x33;
        *w = 0x70;
        break;
    case 3:
        *x_pos = 0x45;
        *w = 0x4C;
        break;
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 2664
        assert_dw(0);
    }
}

/* Matching: fitted stand-in for double code (docs/stand-ins.md), not recovered: titleChangeColor
 * uses a2 for temporaries as after one. It sits in the gap before titleChangeSpriteColor2, where
 * the original's line table leaves room for code (196 lines; the file's usual gap between
 * functions is 16). Its body is still fitted. */
STRIPPED_DOUBLE_CODE()
static void titleChangeSpriteColor2(short id) {
    Title_ChgColorInfo *chg_color_info;

    chg_color_info = &TitleSprChgColor[id];
    titleRenewChangeColorManagement(&chg_color_info->timer, chg_color_info->cycle, 0);
    titleChangeColor(TitleSpr[id].rgba, chg_color_info->start_rgba, chg_color_info->end_rgba, chg_color_info->timer,
                     chg_color_info->cycle, 0.5f);
}

static void titleChangeCursorSpriteColor(void) {
    Title_ChgColorInfo *chg_color_info;

    chg_color_info = &TitleSprChgColor[12];
    titleRenewChangeColorManagement(&chg_color_info->timer, chg_color_info->cycle, 1);
    titleChangeColor(TitleSpr[12].rgba, chg_color_info->start_rgba, chg_color_info->end_rgba, chg_color_info->timer,
                     chg_color_info->cycle, 1.0f);
}

static void titleRenewChangeColorManagement(float *timer, float cycle, int repeat) {
    if (repeat == 1) {
        *timer += shGetDT();
        if (*timer >= cycle) {
            *timer = shGetDT();
        }
    } else if (*timer <= cycle) {
        *timer += shGetDT();
    }
}

static void titleChangeColor(int *rgba, int *start_rgba, int *end_rgba, float timer, float cycle_time, float cycle_sin) {
    int diff_color[4];
    float ratio;
    int i;

    ratio = shSinF(PI * (cycle_sin * (timer / cycle_time)));
    for (i = 0; i < 4; i++) {
        diff_color[i] = end_rgba[i] - start_rgba[i];
        rgba[i] = start_rgba[i] + ratio * diff_color[i];
    }
}

/** The "coming soon" end screen, one step per call (Sh2sys.step[4]).
 * @return non-zero once it has finished */
int GameendMain(void) {
    static float game_over_timer;
    struct PicDraw_Data pic;

    switch (Sh2sys.step[4]) {
    case 0:
        FcRead(data_pic_etc_comingsoon_tex, TitleData.pload0);
        STEP4_NEXT();
    case 1:
        if (fsSync(1, -1) < 0 || !ScreenEffectFadeCheck()) {
            break;
        }
        STEP4_NEXT();
    case 2:
        ScreenEffectFadeStart(4, 0.5f);
        game_over_timer = 0.0f;
        STEP4_NEXT();
    case 3:
        PictureLoadImage((struct sh2gfw_AREA_HEAD *)TitleData.pload0, 0, -1, -1);
        shQzero(&pic, sizeof(pic));
        pic.ap = (struct sh2gfw_AREA_HEAD *)TitleData.pload0;
        pic.tex = -1;
        pic.clut = -1;
        pic.status |= 1;
        pic.otp = 1;
        PictureDraw(&pic);
        d1cSend(spkDmaKick());
        game_over_timer += shGetDT();
        if (shPadTrigger(0, key_config.action) || shPadTrigger(0, key_config.cancel) || game_over_timer > 5.0f) {
            return 1;
        }
        break;
    }
    return 0;
}

static struct _OPD_W opd_w[6];
static struct _OPD_W org_title;

static void opd_work_init(void) {
    int loop;

    bzero(opd_w, sizeof(opd_w));
    bzero(&org_title, sizeof(org_title));
    for (loop = 0; loop < 2; loop++) {
        opd_w[loop].u0 = 0.0f;
        opd_w[loop].u1 = 160.0f;
        opd_w[loop].v0 = (loop + 1) * 48;
        opd_w[loop].v1 = 47.0f + opd_w[loop].v0;
        opd_w[loop].x0 = -80.0f;
        opd_w[loop].x1 = 80.0f;
        opd_w[loop].y0 = -24.0f;
        opd_w[loop].y1 = 47.0f + opd_w[loop].y0;
        opd_w[loop + 2] = opd_w[loop];
        opd_w[loop + 4] = opd_w[loop];
    }
    org_title.u0 = 0.0f;
    org_title.u1 = 160.0f;
    org_title.v0 = 0.0f;
    org_title.v1 = 48.0f;
    org_title.x0 = -80.0f;
    org_title.x1 = 80.0f;
    org_title.y0 = -24.0f;
    org_title.y1 = 47.0f + org_title.y0;
}

/** The game-over screen, one step per call (Sh2sys.step[4]).
 * @return non-zero once it has finished */
int GameoverMain(void) {
    static float game_over_timer;
    static int fid;

    switch (Sh2sys.step[4]) {
    case 0:
        game_over_timer = 0.0f;
        STEP4_NEXT();
    case 1:
        fontClear();
        fid = FcRead(data_pic_etc_gameover1_tex, get_gp_data_buf_addr());
        game_over_timer = 0.0f;
        if (fid != -1) {
            fsSync(0, fid);
        }
        STEP4_NEXT();
        opd_work_init();
        ScreenEffectFadeStart(4, 0.0f);
    case 2:
        if (draw_opd_wowk_main()) {
            STEP4_NEXT();
        }
        break;
    case 3:
        return 1;
    }
    return 0;
}

static int draw_opd_wowk_main(void) {
    struct PicDraw_Data pic;
    int loop;
    int rnd;

    switch (org_title.step) {
    case 0:
        org_title.timer++;
        if (org_title.timer == 0x20) {
            org_title.step++;
            org_title.timer = 0;
        }
        break;
    case 1:
        org_title.rgb += 4;
        if (org_title.rgb == 0x60) {
            org_title.step++;
        }
        break;
    case 2:
        org_title.timer++;
        if (org_title.timer == 0x40) {
            org_title.step++;
            org_title.timer = 0;
        }
        break;
    case 3:
        org_title.rgb -= 8;
        if (org_title.rgb == 0) {
            org_title.step++;
        }
        break;
    case 4:
        return 1;
    }
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(), 0, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr();
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.otp = 1;
    pic.a = 0x80;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    for (loop = 0; loop < 6; loop++) {
        rnd = rand() % 30 - 15;
        if (org_title.step >= 2) {
            pic.b = pic.g = pic.r = org_title.rgb;
            pic.status |= 0x10;
        }
        pic.us0 = ftoi4(opd_w[loop].u0);
        pic.vt0 = ftoi4(opd_w[loop].v0);
        pic.us1 = ftoi4(opd_w[loop].u1);
        pic.vt1 = ftoi4(opd_w[loop].v1);
        pic.status |= 4;
        pic.x0 = ftoi4(opd_w[loop].x0);
        pic.y0 = ftoi4(rnd + opd_w[loop].y0);
        pic.x1 = ftoi4(opd_w[loop].x1);
        pic.y1 = ftoi4(rnd + opd_w[loop].y1);
        pic.status |= 2;
        PictureDraw(&pic);
    }
    pic.b = pic.g = pic.r = org_title.rgb;
    pic.status |= 0x10;
    pic.us0 = ftoi4(org_title.u0);
    pic.vt0 = ftoi4(org_title.v0);
    pic.us1 = ftoi4(org_title.u1);
    pic.vt1 = ftoi4(org_title.v1);
    pic.status |= 4;
    pic.x0 = ftoi4(org_title.x0);
    pic.y0 = ftoi4(org_title.y0);
    pic.x1 = ftoi4(org_title.x1);
    pic.y1 = ftoi4(org_title.y1);
    pic.status |= 2;
    PictureDraw(&pic);
    d1cSend(spkDmaKick());
    return 0;
}
