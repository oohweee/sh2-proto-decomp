/*
 * stg_forest.c: stage overlay for the forest and graveyard: the first save point, Angela at the
 * grave, the last scene and the chainsaw.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_forest).
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"

/* Next program step. Matching: the do/while leaves the original's nop before a case label it
 * falls into. */
#define EV_STEP(n) do { ev_p_step = (n); ev_s_step = 0; } while (0)

static int EvProgFirstSaveWell(void);
static int EvProgAngelaInGrave(void);
static int EvProgGraveSureQuiet(void);
static int EvProgGraveLookingFor(void);
static int EvProgLastScene(void);
static int EvProgGetChainsaw(void);
static void EvStageInit(void);
static void EvRoomInit(void);
static void EvSoundCallAfterLoad(void);
static void EvAllTimeFunc(void);
static float LinearTrim(float Yen, float Yst, float Xen, float Xst, float Parm);
static void Ca10_Hakaba_Angela_SetDrawEnv(void);

static struct _AnimeInfo pjames_stage_anim[2] = {
    { 0x4E21, 20, 1024, 1024, 1043, 0, 10 },
    { 0, 0, 0, 0, 0, 0, 0 },
};

static unsigned char ev_pos[354] = {
    0xDC, 0xE4, 0xBE, 0x46, 0xA6, 0xF2, 0x1D, 0x6B, 0xC3, 0xC7, 0xDC, 0x61, 0xD0, 0x63, 0xF5, 0x26,
    0x4A, 0x45, 0x64, 0x61, 0x00, 0x00, 0x59, 0xC2, 0xB5, 0x63, 0x2A, 0x0E, 0xD5, 0x46, 0x81, 0xF2,
    0x0D, 0xD3, 0xBA, 0xC7, 0xF6, 0xE6, 0x46, 0x52, 0x13, 0xE7, 0x26, 0x64, 0xE9, 0x40, 0x68, 0x65,
    0xCE, 0xF2, 0x32, 0x46, 0x26, 0x66, 0x60, 0x3B, 0xCC, 0xC6, 0xBC, 0x68, 0x0A, 0x63, 0xCE, 0xF2,
    0x32, 0x46, 0x26, 0x66, 0x60, 0x3B, 0xCC, 0xC6, 0xBC, 0x68, 0x0A, 0x63, 0x31, 0x6C, 0xE1, 0xE1,
    0xD4, 0x66, 0x55, 0xE5, 0x2A, 0x0E, 0xD5, 0x46, 0x81, 0xF2, 0x0D, 0xD3, 0xBA, 0xC7, 0xF6, 0xE6,
    0x46, 0x52, 0x00, 0xD0, 0x36, 0x46, 0x31, 0x66, 0x00, 0x28, 0xA0, 0xC6, 0x40, 0x66, 0x00, 0x08,
    0x1D, 0x46, 0x31, 0x66, 0x00, 0x34, 0x9E, 0xC6, 0x3A, 0x67, 0x00, 0xDC, 0x9B, 0xC6, 0x02, 0x69,
    0x00, 0x00, 0x16, 0x44, 0x4C, 0x64, 0x00, 0x40, 0x9C, 0xC6, 0x02, 0x69, 0x00, 0x00, 0xFA, 0xC3,
    0x4C, 0x64, 0x66, 0xBA, 0x11, 0xC8, 0x00, 0x80, 0xE5, 0x16, 0xA0, 0xC6, 0xE1, 0x61, 0xA8, 0xE0,
    0xA6, 0xBC, 0xD9, 0x47, 0x00, 0x80, 0x3E, 0x03, 0x18, 0x47, 0x6B, 0xE1, 0x4B, 0x60, 0x00, 0x00,
    0x7A, 0x46, 0x1A, 0x7A, 0x00, 0x70, 0xC6, 0xC6, 0xB0, 0xDC, 0x78, 0xDD, 0x78, 0xE1, 0x40, 0xD2,
    0x72, 0x5B, 0x71, 0x45, 0x1A, 0x7A, 0xAB, 0x99, 0x67, 0xC5, 0x88, 0xEB, 0x42, 0x6B, 0x4E, 0x69,
    0x3F, 0x6B, 0x72, 0x5B, 0x71, 0x45, 0x1A, 0x7A, 0xAB, 0x99, 0x67, 0xC5, 0x8B, 0xEB, 0x34, 0xE5,
    0x88, 0xEB, 0x42, 0x6B, 0x92, 0x89, 0x05, 0x46, 0x1A, 0x7A, 0x1D, 0x9A, 0xBF, 0x42, 0xB6, 0xED,
    0x06, 0xD6, 0x6F, 0xEB, 0x12, 0x6A, 0x9A, 0x61, 0xD2, 0x68, 0xEA, 0xC9, 0xE6, 0x45, 0x1A, 0x7A,
    0x46, 0xB6, 0x63, 0x41, 0x04, 0x64, 0x9A, 0x69, 0xE3, 0x6E, 0x0A, 0x68, 0x6A, 0x6E, 0xBD, 0xC5,
    0xB8, 0x1E, 0xF5, 0x3F, 0x1A, 0x7A, 0x7F, 0x54, 0x3B, 0xC5, 0xF4, 0x39, 0x72, 0xEC, 0xFD, 0xE9,
    0x11, 0xEC, 0x69, 0xEA, 0xA3, 0xE3, 0x06, 0x81, 0xA9, 0x40, 0x1A, 0x7A, 0x1F, 0xBC, 0xCC, 0xC5,
    0x18, 0xC7, 0x47, 0xEC, 0x20, 0xEC, 0x84, 0xEC, 0xF4, 0xEA, 0x82, 0x61, 0x88, 0x82, 0x60, 0x44,
    0x1A, 0x7A, 0x8F, 0x42, 0x62, 0x42, 0x5C, 0x59, 0xB7, 0xDD, 0x53, 0xD1, 0x3E, 0xE1, 0x00, 0xDC,
    0xD1, 0xDF, 0x46, 0xD7, 0x2A, 0xD4, 0x00, 0xAC, 0x6E, 0xC7, 0x00, 0x80, 0x00, 0x80, 0x54, 0xC4,
    0x78, 0x5D,
};

static struct Event_List ev_list[20] = {
    { 0, 0x4000B000, 0x400E2060, 0 },
    { 0x800E0000, 0x401AD000, 0x40305000, 0 },
    { 0x800E0000, 0x403ED000, 0x40545000, 0 },
    { 0, 0x20622000, 0x406E1000, 0x4000000 },
    { 0, 0x206E1000, 0x40622000, 0x4000000 },
    { 0, 0x207A3000, 0x40864000, 0x4000000 },
    { 0, 0x20864000, 0x407A3000, 0x4000000 },
    { 0, 0x20925000, 0x40A050A0, 0x2400000 },
    { 0, 0x20AE6000, 0x30000000, 0x4000 },
    { 0x240000, 0x40C0C000, 0x30000000, 0x8024 },
    { 0x240000, 0x40D2C000, 0x30000000, 0x8024 },
    { 0x240000, 0x40E4D000, 0x30000000, 0x8024 },
    { 0x240000, 0x40FAD000, 0x30000000, 0x8024 },
    { 0x240000, 0x4110D000, 0x30000000, 0x8024 },
    { 0x240000, 0x4126D000, 0x30000000, 0x8024 },
    { 0x80248025, 0x213C8000, 0x30000000, 0xC000 },
    { 0x80240025, 0x213C8000, 0x30000000, 0x10000 },
    { 0x800C0026, 0x21562000, 0x30000000, 0x14026 },
    { 0x82000201, 0x10000000, 0x30000000, 0x18201 },
    { 0, 0, 0, 0 },
};

static struct _CL_HITPOLY_PLANE clActWallList_ca19[3] = {
    { 1, 1, 0, 4, 12, 0, {
            { -60005.0f, 93.0f, -894.0f, 1.0f },
            { -60005.0f, -665.0f, -894.0f, 1.0f },
            { -60005.0f, -665.0f, 1037.0f, 1.0f },
            { -60005.0f, 93.0f, 1037.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 12, 0, {
            { -62699.5f, -665.0f, -513.0f, 1.0f },
            { -62699.5f, 93.0f, -513.0f, 1.0f },
            { -62469.0f, 93.0f, 1037.0f, 1.0f },
            { -62469.0f, -665.0f, 1037.0f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
};

static struct _CL_HITPOLY_PLANE clActWallList_ca20[6] = {
    { 1, 1, 0, 4, 12, 0, {
            { -60005.0f, 93.0f, -894.0f, 1.0f },
            { -60005.0f, -665.0f, -894.0f, 1.0f },
            { -60005.0f, -665.0f, 1037.0f, 1.0f },
            { -60005.0f, 93.0f, 1037.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 12, 0, {
            { -62699.5f, -665.0f, -513.0f, 1.0f },
            { -62699.5f, 93.0f, -513.0f, 1.0f },
            { -62469.0f, 93.0f, 1037.0f, 1.0f },
            { -62469.0f, -665.0f, 1037.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 12, 0, {
            { -62005.0f, 93.0f, -894.0f, 1.0f },
            { -62005.0f, -665.0f, -894.0f, 1.0f },
            { -60005.0f, -665.0f, -894.0f, 1.0f },
            { -60005.0f, 93.0f, -894.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 12, 0, {
            { -62352.0f, 93.0f, -834.0f, 1.0f },
            { -62352.0f, -665.0f, -834.0f, 1.0f },
            { -62005.0f, -665.0f, -894.0f, 1.0f },
            { -62005.0f, 93.0f, -894.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 12, 0, {
            { -62699.5f, 93.0f, -513.0f, 1.0f },
            { -62699.5f, -665.0f, -513.0f, 1.0f },
            { -62352.0f, -665.0f, -834.0f, 1.0f },
            { -62352.0f, 93.0f, -834.0f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
};

static int (*ev_prog[7])(void) = {
    NULL,
    EvProgFirstSaveWell,
    EvProgAngelaInGrave,
    EvProgGraveSureQuiet,
    EvProgGraveLookingFor,
    EvProgGetChainsaw,
    EvProgLastScene,
};

static struct Stage_GfwFunc SpecialDrawFunctions = { NULL, Ca10_Hakaba_Angela_SetDrawEnv, NULL, NULL };

static struct Model_List mdl_list[5] = {
    { 768, 0, 0, 0, { -74272.875f, -0.000244f, -2451.8445f, 0.0f }, { -0.000001f, -1.071186f, -0.0f, 0.0f } },
    { 771, 0, 0, 0, { -49623.37f, 0.000305f, 1921.686f, 0.0f }, { 3.141591f, 1.361593f, -3.141591f, 0.0f } },
    { 1862, 0, 38, 12, { -61271.492f, -508.48743f, -577.02716f, 0.0f }, { -0.149076f, -1.085207f, 0.389348f, 0.0f } },
    { 1373, 0, 513, 12, { -60819.81f, 0.0f, -0.0f, 0.0f }, { -0.0f, -0.0f, 0.0f, 0.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

struct Stage_Data stage_forest = {
    ev_list,
    ev_pos,
    ev_prog,
    NULL,
    mdl_list,
    NULL,
    EvStageInit,
    EvRoomInit,
    EvAllTimeFunc,
    1,
    0,
    pjames_stage_anim,
    NULL,
    &SpecialDrawFunctions,
    NULL,
    EvSoundCallAfterLoad,
    0,
};

static char *dds_adr = NULL;
static char *dds_adr_h = NULL;
static char *dds_adr_i = NULL;
static float forest_se_x = 0.0f;
static float forest_se_y = 0.0f;
static float forest_se_r = 0.0f;

static float agl_pos_0[2][4] = {
    { 2430.6165f, 2636.6313f, -1708.8447f, 0.0f },
    { -0.000001f, -0.000008f, 0.0f, 0.0f },
};

static float agl_pos_1[2][4] = {
    { 781.1213f, 2505.674f, -394.2832f, 0.0f },
    { 0.0f, 0.0f, 0.000002f, 0.0f },
};

static int EvProgFirstSaveWell(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        EV_STEP(2);
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_out_p_redpaper_tex, NULL)) {
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
            EV_STEP(0xA);
        }
        break;
    case 0xA:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubMessage(0)) {
            break;
        }
        ScreenEffectFadeStart(0xB, 0.0f);
        EV_STEP(4);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
    case 0xD:
        game_flag.flag[0] |= 0x20;
        SetSavePointName(2);
        SeCall(0x2743, 1.0f, 0);
        Sh2sys.step[2] = 9;
        Sh2sys.step[3] = 0;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgAngelaInGrave(void) {
    static short grave_anim[10] = { 0x41F, 0xDB5, 0x420, 0xDB6, 0x421, 0xDB7, 0x422, 0xDB8, 0x423, 0xDB9 };
    static struct DramaDemo_MessageTime movie_msg[6] = {
        { 564, 624 },
        { 630, 720 },
        { 720, 831 },
        { 831, 921 },
        { 987, 1026 },
        { 0xFFFF, 0xFFFF },
    };
    static struct DramaDemo_MessageTime grave_msg[29] = {
        { 0x00C, 0x0A5 }, { 0x0A5, 0x0ED }, { 0x0ED, 0x168 }, { 0x168, 0x1A4 }, { 0x1A4, 0x1E9 },
        { 0x1E9, 0x207 }, { 0x207, 0x22E }, { 0x22E, 0x2A6 }, { 0x2CD, 0x3A2 }, { 0x3A2, 0x402 },
        { 0x402, 0x447 }, { 0x447, 0x4EC }, { 0x4EC, 0x56D }, { 0x56D, 0x5BB }, { 0x5BB, 0x5F1 },
        { 0x5F1, 0x696 }, { 0x696, 0x708 }, { 0x708, 0x753 }, { 0x76E, 0x7D7 }, { 0x7D7, 0x828 },
        { 0x828, 0x8C7 }, { 0x8C7, 0x92D }, { 0x92D, 0x9ED }, { 0x9ED, 0xA6E }, { 0xAB0, 0xBA9 },
        { 0xBB8, 0xC30 }, { 0xC30, 0xC90 }, { 0xC90, 0xCC6 }, { 0xFFFF, 0xFFFF },
    };
    static struct DramaDemo_PlayInfo grave = { 3, NULL, grave_anim, grave_msg, 10, 0, NULL, 60023, 0.0f, 0.0f, 0.0f };
    struct SubCharacter *scp;
    int hide_ca11;

    hide_ca11 = 0;
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        ScreenEffectFadeStart(7, 1.5f);
        SeBgmCall(2);
        EV_STEP(0x10);
    case 0x10:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        if (shSdStat() & 0xF) {
            break;
        }
        hide_ca11 = 1;
        {
            shCharacterSetPosAfterDemo(sh2jms.player, (float[4]){ -20.845f, 2473.03f, -1332.315f, 0.0f }, -2.3561945f);
        }
        EV_STEP(0x28);
        break;
    case 0x28:
        hide_ca11 = 1;
        EvSubMovieReady(data_movie_hakaba_pss, movie_msg, 5);
        if (!EvSubMovieStart(1)) {
            break;
        }
        EV_STEP(0x2B);
        break;
    case 0x2B:
        if (movieGetLastExitStatus()) {
            hide_ca11 = 1;
            EV_STEP(0x2C);
        } else {
            EV_STEP(0x2F);
        }
        break;
    case 0x2C:
        hide_ca11 = 1;
        EvSubMovieEnd();
        grave.adr_dds_top = dds_adr;
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        ScreenEffectFadeStart(5, 0.0f);
        EV_STEP(0x16);
    case 0x16:
        if (DramaDemoMain(&grave)) {
            EV_STEP(0xD);
        } else {
            hide_ca11 = 1;
        }
        break;
    case 0x2F:
        EvSubMovieEnd();
        EV_STEP(0xD);
        break;
    case 0xD:
        ScreenEffectFadeStart(5, 0.0f);
        shCharacter_Manage_Delete(NULL, 0x103, 0);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        if (shRandI() & 1) {
            game_flag.flag[1] |= 0x20;
        }
        {
            shCharacterSetPosAfterDemo(sh2jms.player, (float[4]){ -20.845f, 2473.03f, -1332.315f, 0.0f }, -2.3561945f);
        }
        scp = shCharacterGetSubCharacter(0x107, 0);
        shCharacterHumanAGLAnimeSetP(scp, 0xF3E);
        shCharacterSetPosAfterDemo(scp, agl_pos_1[0], 3.1415927f);
        SeBgmCall(3);
        return 1;
    }
    if (hide_ca11) {
        loadBgCommon_HideMapBlockOutdoor(0x1000B);
    }
    return 0;
}

static int EvProgGraveSureQuiet(void) {
    static short quiet_anim[2] = { 0x424, 0xDBA };
    static struct DramaDemo_MessageTime quiet_msg[4] = { { 0x57, 0x8D }, { 0x8D, 0x12F }, { 0x12F, 0x168 }, { 0xFFFF, 0xFFFF } };
    static struct DramaDemo_PlayInfo grave = { 4, NULL, quiet_anim, quiet_msg, 38, 0, NULL, 60024, 0.0f, 0.0f, 0.0f };
    struct SubCharacter *scp;

    switch (ev_p_step) {
    case 0:
        grave.adr_dds_top = dds_adr_h;
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        EV_STEP(0x16);
    case 0x16:
        if (!DramaDemoMain(&grave)) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        shCharacter_Manage_Delete(NULL, 0x103, 0);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        {
            shCharacterSetPosAfterDemo(sh2jms.player, (float[4]){ -179.005f, 2487.65f, -1286.69f, 0.0f }, -1.321038f);
        }
        scp = shCharacterGetSubCharacter(0x107, 0);
        shCharacterHumanAGLAnimeSetP(scp, 0xF3E);
        shCharacterSetPosAfterDemo(scp, agl_pos_1[0], 3.1415927f);
        if (shRandI() & 1) {
            game_flag.flag[1] |= 0x20;
        } else {
            game_flag.flag[1] &= ~0x20;
        }
        return 1;
    }
    return 0;
}

static int EvProgGraveLookingFor(void) {
    static short looking_anim[2] = { 0x425, 0xDBB };
    static struct DramaDemo_MessageTime looking_msg[3] = { { 0x54, 0x96 }, { 0x96, 0xC3 }, { 0xFFFF, 0xFFFF } };
    static struct DramaDemo_PlayInfo grave = { 4, NULL, looking_anim, looking_msg, 41, 0, NULL, 60025, 0.0f, 0.0f, 0.0f };
    struct SubCharacter *scp;

    switch (ev_p_step) {
    case 0:
        grave.adr_dds_top = dds_adr_i;
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        EV_STEP(0x16);
    case 0x16:
        if (!DramaDemoMain(&grave)) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        shCharacter_Manage_Delete(NULL, 0x103, 0);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        {
            shCharacterSetPosAfterDemo(sh2jms.player, (float[4]){ -493.0f, 2510.19f, -1093.55f, 0.0f }, -1.727716f);
        }
        scp = shCharacterGetSubCharacter(0x107, 0);
        shCharacterHumanAGLAnimeSetP(scp, 0xF3E);
        shCharacterSetPosAfterDemo(scp, agl_pos_1[0], 3.1415927f);
        if (shRandI() & 1) {
            game_flag.flag[1] |= 0x20;
        } else {
            game_flag.flag[1] &= ~0x20;
        }
        return 1;
    }
    return 0;
}

static int EvProgLastScene(void) {
    static short anim[2] = { 0x3CF, 0x9EC };
    static struct DramaDemo_PlayInfo info = { 86, MemShare_gp_data_buf, anim, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[2] = {
        { 260, data_chr_lau_lau_mdl, data_demo_mry_yarinaoshi_i_lau_anm, NULL, data_demo_mry_yarinaoshi_i_lau_cls },
        { 0, NULL, NULL, NULL, NULL },
    };
    static u_long128 *anim_adr = NULL;

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_mry_yarinaoshi_i_mry_yarinaoshi_i_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(chara_data, 0);
        anim_adr = CharaDataLoadExtra(data_demo_mry_yarinaoshi_i_lll_jms_anm, 0x200);
        fsSync(0, -1);
        anim_adr = CharaDataAnimAdressExchange(sh2jms.player, anim_adr);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        EV_STEP(0x16);
    case 0x16:
        if (!DramaDemoMain(&info)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(4);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0x28);
        break;
    case 0x28:
        EvSubMovieReady(data_movie_ending_pss, NULL, 0);
        if (!EvSubMovieStart(1)) {
            break;
        }
        EV_STEP(0x2B);
        break;
    case 0x2B:
        if (!movieGetLastExitStatus()) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        EvSubMovieEnd();
        anim_adr = CharaDataAnimAdressExchange(sh2jms.player, anim_adr);
        playing.clear_end_kind |= 1;
        switch (playing.riddle_level) {
        case 0:
            playing.clear_end_kind |= 0x20;
            break;
        case 1:
            playing.clear_end_kind |= 0x40;
            break;
        case 2:
            playing.clear_end_kind |= 0x80;
            break;
        default:
            playing.clear_end_kind -= 0x80;
            break;
        }
        playing.clear_end_number++;
        Sh2sys.step[2] = 10;
        Sh2sys.step[3] = 0;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        return 1;
    }
    return 0;
}

static int EvProgGetChainsaw(void) {
    return EvSubItemGetAndAnim(0xE, 4);
}

static void EvStageInit(void) {
    CharaDataDeleteAll();
}

static void EvRoomInit(void) {
    static struct CharaData_DemoList chara_data[3] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_haka_agl_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_haka_agl_hhh_jms_cls },
        { 263, data_chr_agl_agl_mdl, data_demo_haka_agl_agl_anm, data_chr_agl_agl_kg1, data_demo_haka_agl_agl_cls },
        { 0, NULL, NULL, NULL, NULL },
    };
    struct SubCharacter *scp;
    float vec0[4];

    if (!(game_flag.flag[16] & 1)) {
        CharaDataLoadDemo(chara_data, 0);
        dds_adr = (char *)CharaDataLoadExtra(data_demo_haka_agl_haka_agl_dds, 0x200);
        dds_adr_h = (char *)CharaDataLoadExtra(data_demo_haka_agl_haka_agl_h_dds, 0x200);
        dds_adr_i = (char *)CharaDataLoadExtra(data_demo_haka_agl_haka_agl_i_dds, 0x200);
        fsSync(0, -1);
        *(u_long128 *)vec0 = 0;
        vec0[1] = 3.1415927f;
        if ((game_flag.flag[1] >> 4) & 1) {
            scp = CharaWorkCreate(0x107, 0, agl_pos_1[0], vec0, 0);
            shCharacterHumanAGLAnimeSetP(scp, 0xF3E);
        } else {
            scp = CharaWorkCreate(0x107, 0, agl_pos_0[0], vec0, 0);
            shCharacterHumanAGLAnimeSetP(scp, 0xF3D);
        }
    }
}

static void EvSoundCallAfterLoad(void) {
    SeCallPos(0x2742, 1.0f, NULL, 8);
    forest_se_x = shRandF();
    forest_se_y = shRandF();
    forest_se_r = 0.0f;
}

static void EvAllTimeFunc(void) {
    int disp_ctrl_list[5] = { 0 };
    float se_vol;

    disp_ctrl_list[0] = 0;
    if ((game_flag.flag[0] >> 12) & 1) {
        clAddDynamicWall(clActWallList_ca19);
        clAddDynamicWall(clActWallList_ca20);
        EvDispControlModelEntry(disp_ctrl_list, 0x14, 0);
    } else {
        EvDispControlModelEntry(disp_ctrl_list, 0x14, -1);
    }
    if ((Sh2sys.main_status >> 6) & 1) {
        EvDispControlModelEntry(disp_ctrl_list, 0xA, 0);
    } else {
        EvDispControlModelEntry(disp_ctrl_list, 0xA, -1);
    }
    EvDispControlModelExec(disp_ctrl_list);
    forest_se_r += (0.5f + 1.5f * forest_se_x) * (3.1415927f * (2.0f * shGetDT())) / 4.0f;
    if (forest_se_r > 6.2831855f) {
        forest_se_r -= 6.2831855f;
        forest_se_x = shRandF();
        forest_se_y = shRandF();
    }
    se_vol = 0.8f + 0.2f * forest_se_y * shSinF(forest_se_r);
    if (sh2jms.player->pos.x > -20000.0f && sh2jms.player->pos.z > -20000.0f) {
        se_vol *= 1.0f - (20000.0f + fminf(sh2jms.player->pos.x, sh2jms.player->pos.z)) / 20000.0f;
        if (se_vol <= 0.0f) {
            se_vol = 0.001f;
        }
    }
    Se2dManageDataVolumeChange(0x2742, se_vol);
}

static float FogParameters[4] = { 17635.0f, 10000.0f, 21.0f, 234.0f };
static float FarNear[4] = { 18000.0f, 0.0f, 2.4264705f, 0.0f };
static float Asahi_dir[4] = { -0.93f, 0.116f, -0.349f, 0.1f };
static float Asahi_col[4] = { 0.25f, 0.2f, 0.2f, 0.1f };

static float LinearTrim(float Yen, float Yst, float Xen, float Xst, float Parm) {
    return Yst + (Yen - Yst) * (Parm - Xst) / (Xen - Xst);
}

static void Ca10_Hakaba_Angela_SetDrawEnv(void) {
    float origin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float nowpos[4];
    float dist;
    float fogfar;
    float fognear;
    float az;
    float fnpow;
    float ffpow;
    float farcliptrim;
    void sh2gfw_Set_FogMin(); /* Matching: called without a prototype in the original (arguments passed unconverted). */

    vwGetViewPosition(nowpos);
    if (DramaDemoNumber()) {
        dist = 0.0f;
        farcliptrim = dist;
    } else if (EventProgressCheck() == 2) {
        dist = distXZ(origin, nowpos);
        farcliptrim = 2000.0f;
    } else {
        dist = 10000.0f;
        farcliptrim = 2000.0f;
    }
    if (dist > 10000.0f) {
        dist = 10000.0f;
    }
    az = LinearTrim(FarNear[0], 7500.0f, 0.0f, 10000.0f, dist);
    fogfar = LinearTrim(FogParameters[0], 5500.0f, 0.0f, 10000.0f, dist);
    fognear = LinearTrim(FogParameters[1], 3000.0f, 0.0f, 10000.0f, dist);
    fnpow = LinearTrim(FogParameters[3], 235.0f, 0.0f, 10000.0f, dist);
    ffpow = LinearTrim(FogParameters[2], 20.0f, 0.0f, 10000.0f, dist);
    Env_ctl.camera_parms2[2] = az + farcliptrim;
    sh2gfw_Set_Fogfar(fogfar);
    sh2gfw_Set_FogNear(fognear);
    sh2gfw_Set_FogMax(ffpow);
    sh2gfw_Set_FogMin(fnpow);
    if (!sh2gfw_Check_ParallelDemoLight(0)) {
        sh2gfw_Set_PallarelLight(Asahi_dir, Asahi_col, 0);
    }
}
