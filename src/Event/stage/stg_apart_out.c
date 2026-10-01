/*
 * stg_apart_out.c: stage overlay for outside the apartments: the snake and old man coins, the
 * murder news, the passages between the east and west buildings.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_apart_out).
 */
#include "sh2.h"
#include "asm_helpers.h"

static int EvProgGetCoinOfSnake(void);
static int EvProgGetCoinOfOldman(void);
static int EvProgMurderNewsRead(void);
static int EvProgApartEastToWest(void);
static int EvProgApartWestToEast(void);
static void EvStageInit(void);
static void EvAllTimeFunc(void);

static struct _AnimeInfo pjames_stage_anim[21] = {
    { 0x4E21, 0x14, 1024, 0x400, 0x413, 0, 10 },
    { 0x4E27, 0x1E, 1024, 0x414, 0x431, 0, 0 },
    { 0x4E28, 0xF, 768, 0x432, 0x440, 0, 0 },
    { 0x4E29, 0xF, 768, 0x441, 0x44F, 0, 0 },
    { 0x4E2A, 0x1E, 768, 0x450, 0x46D, 0, 0 },
    { 0x4E2B, 0x1E, 768, 0x46E, 0x48B, 0, 0 },
    { 0x4E2C, 0xF, 1024, 0x490, 0x49A, 0, 0 },
    { 0x4E2D, 0xF, 1024, 0x49F, 0x4A9, 0, 0 },
    { 0x4E2E, 0x1E, 640, 0x4AA, 0x4C7, 0, 0 },
    { 0x4E2F, 0x1E, 640, 0x4C8, 0x4E5, 0, 0 },
    { 0x4E30, 0xF, 1024, 0x4EA, 0x4F4, 0, 0 },
    { 0x4E31, 0xF, 1024, 0x4F9, 0x503, 0, 0 },
    { 0x4E32, 0x1E, 1024, 0x504, 0x521, 0, 0 },
    { 0x4E33, 0x1E, 1024, 0x522, 0x53F, 0, 0 },
    { 0x4E34, 0x1E, 1024, 0x540, 0x55D, 0, 0 },
    { 0x4E39, 0xF, 1024, 0x562, 0x56C, 0, 0 },
    { 0x4E3A, 0xF, 1024, 0x571, 0x57B, 0, 0 },
    { 0x4E3B, 0x1E, 1024, 0x57C, 0x599, 0, 0 },
    { 0x4E3C, 0x1E, 1024, 0x59A, 0x5B7, 0, 0 },
    { 0x4E45, 0xF, 1024, 0x5B8, 0x5C6, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
};
static unsigned char ev_pos[194] = {
    0x00, 0xEC, 0xAE, 0x46, 0x00, 0x80, 0x00, 0xC0, 0xA8, 0xC6, 0x40, 0x62, 0x8D, 0xFF, 0xE9, 0xC7,
    0x00, 0x80, 0xFB, 0xDC, 0x35, 0x46, 0x5D, 0x5F, 0x0C, 0x0B, 0xC9, 0xC7, 0x00, 0x80, 0x00, 0x30,
    0x0E, 0x46, 0xEE, 0x60, 0xC0, 0xF6, 0xA9, 0x47, 0x1A, 0x7A, 0xB6, 0x62, 0x8A, 0xC7, 0x4C, 0x5C,
    0x79, 0xD7, 0xBF, 0x4F, 0x52, 0xE1, 0xA1, 0xDB, 0x63, 0xE0, 0x00, 0x00, 0x00, 0x80, 0x0E, 0xAF,
    0xD1, 0xC7, 0x00, 0x80, 0x3B, 0x39, 0x7C, 0x45, 0xE3, 0x62, 0x4D, 0xF6, 0xCF, 0xC7, 0xD2, 0xD4,
    0x3B, 0x39, 0x7C, 0x45, 0xE3, 0x62, 0x7A, 0x2C, 0xCD, 0xC7, 0x00, 0x80, 0x47, 0x2D, 0xBB, 0x45,
    0x0C, 0x63, 0x00, 0xD5, 0xC4, 0xC7, 0x00, 0x80, 0x00, 0x7C, 0xB5, 0xC6, 0x14, 0x66, 0x7A, 0x4C,
    0x9E, 0xC7, 0x00, 0x80, 0x47, 0x2D, 0xBB, 0x45, 0x0C, 0x63, 0x80, 0x21, 0x9D, 0xC7, 0x00, 0x80,
    0xFE, 0xFD, 0x76, 0xC7, 0x0B, 0x63, 0x1F, 0xFD, 0xB9, 0x47, 0xC4, 0xD4, 0x89, 0x1B, 0xA2, 0xC7,
    0xD9, 0x5F, 0x66, 0x7A, 0xC0, 0xC7, 0x00, 0x80, 0x02, 0x40, 0x83, 0xC6, 0xD9, 0x5F, 0xA3, 0x4F,
    0xC3, 0x47, 0xC4, 0xD4, 0x0B, 0xEF, 0x96, 0xC7, 0xD9, 0x5F, 0xF3, 0xFC, 0x45, 0xC7, 0x00, 0x80,
    0xD7, 0x9D, 0x6C, 0xC7, 0xD9, 0x5F, 0x00, 0xF7, 0xA3, 0xC7, 0x1A, 0x7A, 0x00, 0x40, 0x83, 0x45,
    0x6C, 0x63,
};
static struct Event_List ev_list[16] = {
    { 0x80634062, 0x10000000, 0x30000000, 0x00010061 },
    { 0xC063C062, 0x10000000, 0x40004160, 0x00000562 },
    { 0x80654064, 0x10000000, 0x30000000, 0x00014000 },
    { 0xC065C064, 0x10000000, 0x400C30E0, 0x01800562 },
    { 0x80760000, 0x20182000, 0x30000000, 0x0000C000 },
    { 0x805F0000, 0x20182000, 0x30000000, 0x00008076 },
    { 0x005F0000, 0x20182000, 0x60000000, 0x0000C000 },
    { 0x00750000, 0x20248000, 0x30000000, 0x00004075 },
    { 0x00000000, 0x203E1000, 0x404A20A0, 0x03800000 },
    { 0x00000000, 0x20562000, 0x40621180, 0x01C00536 },
    { 0x803D0000, 0x206E2000, 0x407A1180, 0x01C00534 },
    { 0x00000000, 0x206E2000, 0x80000000, 0x01C00535 },
    { 0x00000000, 0x20861000, 0x40922180, 0x00400543 },
    { 0x00000000, 0x209E4000, 0x40AA30C0, 0x01000540 },
    { 0x00000000, 0x20B61000, 0x90000000, 0x03BFC000 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
};
static int (*ev_prog[6])(void) = {
    NULL,
    EvProgGetCoinOfSnake,
    EvProgGetCoinOfOldman,
    EvProgMurderNewsRead,
    EvProgApartEastToWest,
    EvProgApartWestToEast,
};
static struct Model_List mdl_list[4] = {
    { 1377, 0, 0, 0, { 86999.375f, 591.0353f, -71274.836f, 0.0f }, { 7.0e-6f, 0.410013f, -2.0e-6f, 0.0f } },
    { 1823, 0, 118, 0, { -103369.96f, -494.99976f, 9170.001f, 0.0f }, { 1.960796f, -0.0f, 0.0f, 0.0f } },
    { 1821, 0, 117, 0, { 87014.99f, 324.99997f, -71260.016f, 0.0f }, { 1.570796f, -0.0f, 0.0f, 0.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};
static struct Enemy_List en_list[4] = {
    { 512, 84, 84000, -72500, 0, 0, 0, 0x440 },
    { 512, 85, 85000, -71000, 0, 0, 0, 0x440 },
    { 512, 86, 86800, -71700, 0, 0, 0, 0x440 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};
struct Stage_Data stage_apart_out = { ev_list, ev_pos, ev_prog, NULL, mdl_list, en_list, EvStageInit, NULL, EvAllTimeFunc, 2, 0, pjames_stage_anim, NULL, NULL, NULL, NULL, 0 };

static int EvProgGetCoinOfSnake(void) {
    return EvSubItemGetAndAnim(0x2F, 5);
}

static int EvProgGetCoinOfOldman(void) {
    static int px = -692;
    static int py = 1444;

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        ev_p_step = 2;
        ev_s_step = 0;
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_p_dust_out_tex, data_pic_apt_p_dust_out_coin_tex)) {
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
        EvSubPictureLayer(px, py, px + 0x800, py + 0x800, 0x80);
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
        EvSubPictureLayer(px, py, px + 0x800, py + 0x800, 0x80);
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
        EvSubPictureLayer(px, py, px + 0x800, py + 0x800, 0x80);
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubItemGet(0x30, 6)) {
            break;
        }
        ev_p_step = 0x1B;
        ev_s_step = 0;
        break;
    case 0x1B:
        ev_timer -= shGetDT();
        if (ev_timer < 0.0f) {
            ev_timer = 0.0f;
            ev_p_step = 0x20;
            ev_s_step = 0;
        }
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureLayer(px, py, px + 0x800, py + 0x800, (int)itof((int)(128.0f * ev_timer)));
        EvSubPictureEnd();
        break;
    case 0x20:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubMessage(0)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        ev_p_step = 4;
        ev_s_step = 0;
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
        game_flag.flag[3] |= 1;
        return 1;
    }
    return 0;
}

static int EvProgMurderNewsRead(void) {
    return EvSubPictureDisplay(data_pic_apt_p_dust_out_tex, 1);
}

static int EvProgApartEastToWest(void) {
    static short anim_a[] = { 0x417 };
    static short anim_b[] = { 0x418 };
    static struct DramaDemo_PlayInfo door_a = { 16, MemShare_gp_data_buf, anim_a, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct DramaDemo_PlayInfo door_b = { 16, MemShare_gp_data_buf + 0x8000, anim_b, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[2] = {
        { 259, data_chr_jms_hhh_jms_mdl, data_demo_tobira_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    int ret;

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_tobira_tobira_a_dds, MemShare_gp_data_buf);
        FcRead(data_demo_tobira_tobira_b_dds, MemShare_gp_data_buf + 0x8000);
        fsSync(0, -1);
        CharaDataLoadDemo(chara_data, 0);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        if ((game_flag.flag[3] >> 1) & 1) {
            ev_p_step = 0x18;
            ev_s_step = 0;
        } else {
            ev_p_step = 0x17;
            ev_s_step = 0;
        }
        return EvProgApartEastToWest();
    case 0x17:
        if (!DramaDemoMain(&door_a)) {
            break;
        }
        if (shPadTrigger(0, key_config.skip)) {
            ev_p_step = 0xD;
            ev_s_step = 0;
        } else {
            ev_p_step = 0xA;
            ev_s_step = 0;
        }
        break;
    case 0xA:
        if (!EvSubMessage(4)) {
            break;
        }
        ev_p_step = 0x18;
        ev_s_step = 0;
        break;
    case 0x18:
        ret = DramaDemoMain(&door_b);
        if (demo_frame > total_demo_frame - 15.0f) {
            ScreenEffectFadeStart(1, 0.5f);
        }
        if (!ret) {
            break;
        }
        ev_p_step = 0xD;
        ev_s_step = 0;
        break;
    case 0xD:
        ScreenEffectFadeStart(3, 0.0f);
        CharaDataDeleteOne(0x103);
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        return 1;
    }
    return 0;
}

static int EvProgApartWestToEast(void) {
    static short anim_c[] = { 0x419 };
    static struct DramaDemo_PlayInfo door_c = { 16, MemShare_gp_data_buf, anim_c, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[2] = {
        { 259, data_chr_jms_hhh_jms_mdl, data_demo_tobira_c_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    int ret;

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_tobira_c_tobira_c_dds, MemShare_gp_data_buf);
        fsSync(0, -1);
        CharaDataLoadDemo(chara_data, 0);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        ev_p_step = 0x18;
        ev_s_step = 0;
    case 0x18:
        ret = DramaDemoMain(&door_c);
        if (demo_frame > total_demo_frame - 15.0f) {
            ScreenEffectFadeStart(1, 0.5f);
        }
        if (!ret) {
            break;
        }
        ev_p_step = 0xD;
        ev_s_step = 0;
        break;
    case 0xD:
        ScreenEffectFadeStart(3, 0.0f);
        CharaDataDeleteOne(0x103);
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        return 1;
    }
    return 0;
}

static void EvStageInit(void) {
    game_flag.flag[2] &= ~0x800000;
}

static void EvAllTimeFunc(void) {
    int disp_ctrl_list[3];

    disp_ctrl_list[0] = 0;
    if (RoomNameJms() != 7) {
        if ((game_flag.flag[2] >> 31) & 1) {
            EvDispControlModelEntry(disp_ctrl_list, 0x2E, 2);
        }
        EvDispControlModelEntry(disp_ctrl_list, 0x2E, 1);
    }
    EvDispControlModelExec(disp_ctrl_list);
}
