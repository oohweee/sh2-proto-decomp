/*
 * stg_ovservation.c: stage overlay for the observation deck (opening): Mary's letter, the car, the
 * map hint and the last scene.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_observation).
 */
#include "sh2.h"
#include "asm_helpers.h"

/*
 * Game flag n: bit n & 31 of game_flag.flag[n >> 5]. Names invented; the form is stg_town_east.c's (a macro
 * of the same spelling gives the same code). Matching: a helper rather than the word and bit written out,
 * chosen for OB_DemoBlur's float-constant order (docs/matching-notes.md#stg_ovservation-game_flag).
 */
inline int GAME_FLAG(int n) {
    return (game_flag.flag[n >> 5] >> (n & 31)) & 1;
}

inline void GAME_FLAG_ON(int n) {
    game_flag.flag[n >> 5] |= 1 << (n & 31);
}

static int EvProgLetterFromMary(void);
static int EvProgItemInCar(void);
static int EvProgNothingTakeInCar(void);
static int EvProgHadBetterGetMap(void);
static int EvProgCantGoBack(void);
static int EvProgLastScene(void);
static void EvAllTimeFunc(void);
static int EvBgmControl(void);
static float LinearTrim(float Yen, float Yst, float Xen, float Xst, float Parm);
static void OB_DemoBlur(void);

static struct _AnimeInfo pjames_stage_anim[2] = {
    { 0x4E21, 0x14, 1024, 0x400, 0x413, 0, 10 },
    { 0, 0, 0, 0, 0, 0, 0 },
};

static unsigned char ev_pos[124] = {
    0x00, 0x88, 0x74, 0x46, 0x00, 0x80, 0x00, 0x50, 0x91, 0xC5, 0x40, 0xDA, 0x78, 0x5D, 0x78, 0xDD,
    0xB0, 0x58, 0xCB, 0xFB, 0x79, 0x46, 0x00, 0x80, 0x73, 0xBA, 0x53, 0xC5, 0xFF, 0x5C, 0xFF, 0xDF,
    0x9A, 0x7D, 0x0C, 0x45, 0x24, 0x5C, 0x00, 0xE0, 0x86, 0xC4, 0xB5, 0x63, 0x19, 0x60, 0x00, 0x08,
    0xCF, 0x46, 0x40, 0xD2, 0x00, 0x04, 0x8D, 0xC6, 0xE2, 0x70, 0xFB, 0x71, 0x00, 0x70, 0x14, 0xC6,
    0x00, 0x80, 0x00, 0x50, 0xC3, 0xC5, 0x91, 0x6F, 0x00, 0xF0, 0xA0, 0x46, 0x00, 0x80, 0x00, 0x60,
    0x86, 0xC5, 0xD0, 0x5F, 0x40, 0x62, 0x00, 0x58, 0x98, 0xC6, 0x00, 0x80, 0x00, 0xF8, 0xA7, 0x46,
    0xD0, 0x5F, 0x70, 0x7D, 0x0C, 0x45, 0x64, 0x61, 0x00, 0x00, 0x59, 0xC2, 0xB5, 0x63, 0xD0, 0x63,
    0xDC, 0xC0, 0xC4, 0x46, 0xA6, 0xF2, 0x1D, 0x77, 0xC1, 0xC7, 0xD0, 0x63,
};

static struct Event_List ev_list[13] = {
    { 0x80200021, 0x10000000, 0x30000000, 0x00004021 },
    { 0x00180000, 0x20006000, 0x30000000, 0x00008018 },
    { 0x80180000, 0x20006000, 0x30000000, 0x0000C000 },
    { 0x80180000, 0x20125000, 0x60000000, 0x00010000 },
    { 0x00188022, 0x20125000, 0x60000000, 0x00014000 },
    { 0x00220000, 0x20125000, 0x60000000, 0x00010000 },
    { 0x00180000, 0x4020B000, 0x30000000, 0x00010022 },
    { 0x00000000, 0x402EB000, 0x30000000, 0x00014000 },
    { 0x00000000, 0x203C3000, 0x60000000, 0x00004000 },
    { 0x00000000, 0x4048B000, 0x40563040, 0x00000000 },
    { 0x00000000, 0x4062B000, 0x40703080, 0x00000000 },
    { 0x82000201, 0x10000000, 0x30000000, 0x00018201 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
};

static int (*ev_prog[7])(void) = {
    NULL,
    EvProgLetterFromMary,
    EvProgItemInCar,
    EvProgNothingTakeInCar,
    EvProgHadBetterGetMap,
    EvProgCantGoBack,
    EvProgLastScene,
};

static struct Stage_GfwFunc SpecialDrawFunctions = { NULL, NULL, OB_DemoBlur, NULL };

struct Stage_Data stage_observation = {
    ev_list,
    ev_pos,
    ev_prog,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    EvAllTimeFunc,
    5,
    0,
    pjames_stage_anim,
    EvBgmControl,
    &SpecialDrawFunctions,
    NULL,
    NULL,
    0,
};

static int EvProgLetterFromMary(void) {
    static short letter_anim[] = { 1120 };
    static struct DramaDemo_MessageTime letter_msg[22] = {
        { 0x23A, 0x2FA },
        { 0x2FA, 0x34E },
        { 0x366, 0x3F6 },
        { 0x3F6, 0x468 },
        { 0x468, 0x501 },
        { 0x501, 0x56A },
        { 0x56A, 0x5AF },
        { 0x67E, 0x6E1 },
        { 0x6E1, 0x7B9 },
        { 0x7B9, 0x813 },
        { 0x813, 0x8E8 },
        { 0x8E8, 0x957 },
        { 0x957, 0x9DE },
        { 0x9DE, 0xA92 },
        { 0xA92, 0xB2E },
        { 0xB2E, 0xC03 },
        { 0xC03, 0xC84 },
        { 0xC84, 0xD08 },
        { 0xD08, 0xE13 },
        { 0xE13, 0xE91 },
        { 0xE91, 0xF72 },
        { 0xFFFF, 0xFFFF },
    };
    static struct DramaDemo_PlayInfo letter = { 2, MemShare_gp_data_buf, letter_anim, letter_msg, 7, 0, NULL, 60080, 570.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[2] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_tenbou_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_tenbou_hhh_jms_cls },
        { 0, NULL, NULL, NULL, NULL },
    };
    int ret;

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_tenbou_tenbou_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(chara_data, 0);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        ev_p_step = 0x16;
        ev_s_step = 0;
    case 0x16:
        ret = DramaDemoMain(&letter);
        /* Matching: the test and its body on one line: the original's line table has no
         * statement of its own for the body. */
        if (demo_frame > 1400.0f) { GAME_FLAG_ON(517); }
        if (shPadTrigger(0, key_config.skip) || (ret && !(shSdStat() & 0xF0))) {
            ev_p_step = 0xD;
            ev_s_step = 0;
        }
        break;
    case 0xD:
        CharaDataDeleteOne(0x103);
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        {
            shCharacterSetPosAfterDemo(sh2jms.player, (float[4]){ 19350.25f, 0.0f, -507.74f, 0.0f }, 0.0f);
        }
        GAME_FLAG_ON(1322);
        GAME_FLAG_ON(1276);
        return 1;
    }
    return 0;
}

static int EvProgItemInCar(void) {
    static int px = -774;
    static int py = -506;

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        ev_p_step = 2;
        ev_s_step = 0;
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_out_p_incar_tex, data_pic_out_p_incar_map_tex)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        ev_timer = 1.0f;
        ev_p_step = 3;
        ev_s_step = 0;
        break;
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureLayer(px, py, px + 0x1000, py + 0x1000, 0x80);
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        ev_p_step = 8;
        ev_s_step = 0;
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureLayer(px, py, px + 0x1000, py + 0x1000, 0x80);
        EvSubPictureEnd();
        if (!shPadTrigger(0, key_config.enter + key_config.cancel)) {
            break;
        }
        ev_p_step = 0x1E;
        ev_s_step = 0;
        break;
    case 0x1E:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureLayer(px, py, px + 0x1000, py + 0x1000, 0x80);
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubMessage(0x1C)) {
            break;
        }
        ev_p_step = 0x1B;
        ev_s_step = 0;
        break;
    case 0x1B:
        ev_timer -= shGetDT();
        if (ev_timer < 0.0f) {
            ev_timer = 0.0f;
            ev_p_step = 4;
            ev_s_step = 0;
            ScreenEffectFadeStart(1, 0.0f);
        }
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureLayer(px, py, px + 0x1000, py + 0x1000, (int)itof((int)(128.0f * ev_timer)));
        EvSubPictureEnd();
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        ev_p_step = 0xD;
        ev_s_step = 0;
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        ScreenEffectFadeStart(4, 0.0f);
        return 1;
    }
    return 0;
}

static int EvProgNothingTakeInCar(void) {
    return EvSubPictureDisplay(data_pic_out_p_incar_tex, 3);
}

static int EvProgHadBetterGetMap(void) {
    static float pos[4];

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        ev_p_step = 0xA;
        ev_s_step = 0;
    case 0xA:
        if (!EvSubMessage(2)) {
            break;
        }
        ev_p_step = 0x1B;
        ev_s_step = 0;
        break;
    case 0x1B:
        pos[0] = sh2jms.player->pos.x;
        pos[1] = sh2jms.player->pos.y;
        pos[2] = sh2jms.player->pos.z - 500.0f;
        pos[3] = 0.0f;
        ev_p_step = 0x1C;
        ev_s_step = 0;
    case 0x1C:
        PlayerEventMove(pos);
        if (!PlayerEventMoveIsEnd()) {
            break;
        }
        ev_p_step = 0xD;
        ev_s_step = 0;
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgCantGoBack(void) {
    static float pos[4];

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        ev_p_step = 0xA;
        ev_s_step = 0;
    case 0xA:
        if (!EvSubMessage(0)) {
            break;
        }
        ev_p_step = 0x1B;
        ev_s_step = 0;
        break;
    case 0x1B:
        pos[0] = sh2jms.player->pos.x - 500.0f;
        pos[1] = sh2jms.player->pos.y;
        pos[2] = sh2jms.player->pos.z;
        pos[3] = 0.0f;
        ev_p_step = 0x1C;
        ev_s_step = 0;
    case 0x1C:
        PlayerEventMove(pos);
        if (!PlayerEventMoveIsEnd()) {
            break;
        }
        ev_p_step = 0xD;
        ev_s_step = 0;
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgLastScene(void) {
    static short anim[] = { 0x3D1, 0x2C };
    static struct DramaDemo_PlayInfo info = { 89, MemShare_gp_data_buf, anim, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[2] = {
        { 261, data_chr_mar_lll_mar_mdl, data_demo_mar_isho_i_lll_mar_anm, data_chr_mar_hhh_mar_kg1, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    static u_long128 *anim_adr;

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_mar_isho_i_end_maria_i_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(chara_data, 0);
        anim_adr = (u_long128 *)(MemShare_gp_data_buf +
                                 ((FcGetFileSize(data_demo_mar_isho_i_end_maria_i_dds) + 0x7FF) & ~0x7FF));
        FcRead(data_demo_mar_isho_i_lll_jms_anm, anim_adr);
        fsSync(0, -1);
        anim_adr = CharaDataAnimAdressExchange(sh2jms.player, anim_adr);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        ev_p_step = 0x16;
        ev_s_step = 0;
    case 0x16:
        if (!DramaDemoMain(&info)) {
            break;
        }
        ev_p_step = 0xD;
        ev_s_step = 0;
        break;
    case 0xD:
        CharaDataDeleteOne(0x104);
        anim_adr = CharaDataAnimAdressExchange(sh2jms.player, anim_adr);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        return 1;
    }
    return 0;
}

static void EvAllTimeFunc(void) {
    int disp_ctrl_list[3];

    disp_ctrl_list[0] = 0;
    EvDispControlModelEntry(disp_ctrl_list, 3, GAME_FLAG(24) ? -1 : 0);
    EvDispControlModelExec(disp_ctrl_list);
}

static int EvBgmControl(void) {
    if ((Sh2sys.main_status >> 6) & 1) {
        if (!GAME_FLAG(517)) {
            return 4;
        }
        return 0xE;
    }
    return GAME_FLAG(36) != 0;
}

static float LinearTrim(float Yen, float Yst, float Xen, float Xst, float Parm) {
    return Yst + (Yen - Yst) * (Parm - Xst) / (Xen - Xst);
}

static void OB_DemoBlur(void) {
    float blt;
    float tm;

    if (DramaDemoNumber()) {
        tm = demo_frame;
        if (tm > 1500.0f) {
            sh2gfw_Reset_FilterCommand();
        } else {
            if (tm < 0.0f) {
                tm = 0.0f;
            }
            blt = LinearTrim(0.0f, 1.0f, 1500.0f, 0.0f, tm);
            sh2gfw_Set_FilterBlur(blt);
        }
    }
}
