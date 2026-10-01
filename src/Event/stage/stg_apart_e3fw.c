/*
 * stg_apart_e3fw.c: stage overlay for the apartments, east building 3F west: Laura kicking the
 * key, the handgun.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_apart_e3fw).
 */
#include "sh2.h"

static int EvProgLauraKickKey(void);
static int EvProgGetHandgun(void);
static void EvRoomInit(void);
static void EvProgSubScreamOn(void);

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

static unsigned char ev_pos[144] = {
    0x9F, 0x7A, 0x5B, 0x47, 0x00, 0x80, 0xF6, 0xBF, 0x28, 0x46, 0xB0, 0x64, 0x29, 0x35, 0x9C, 0x46,
    0x00, 0x80, 0x3B, 0x3F, 0x9E, 0x46, 0x70, 0xDA, 0xC1, 0x55, 0xDA, 0xD2, 0x88, 0x5D, 0xFF, 0x57,
    0xED, 0x5A, 0xE6, 0x38, 0x1C, 0x47, 0x00, 0x80, 0xFB, 0xDC, 0x35, 0x46, 0x5D, 0x5F, 0x7D, 0xF8,
    0x1F, 0x47, 0x00, 0x80, 0x1F, 0x84, 0x28, 0x46, 0xD0, 0x5F, 0x32, 0x93, 0x90, 0x46, 0x00, 0x80,
    0x00, 0x30, 0xAF, 0x46, 0xF9, 0x5F, 0xFD, 0x42, 0x93, 0x46, 0x00, 0x80, 0x00, 0x2F, 0x8E, 0x46,
    0xB0, 0x68, 0xA4, 0x6A, 0x7D, 0x88, 0x3A, 0x47, 0x00, 0x80, 0x1F, 0x84, 0x28, 0x46, 0xD0, 0x5F,
    0x02, 0x36, 0x58, 0x47, 0x00, 0x80, 0x00, 0xE8, 0x28, 0x46, 0xD0, 0x5F, 0x00, 0x7B, 0xC0, 0xC7,
    0x08, 0xEB, 0x06, 0xB4, 0x8C, 0xC6, 0xC5, 0x5F, 0x01, 0x2C, 0xA6, 0x46, 0x00, 0x80, 0x08, 0xCE,
    0x9A, 0x46, 0x8F, 0x5E, 0x08, 0x1F, 0x93, 0x46, 0x00, 0x80, 0x03, 0xF6, 0x9A, 0x46, 0x8F, 0x5E,
};

static struct Event_List ev_list[14] = {
    { 0x00430000, 0x20004004, 0x30000000, 0x00004000 },
    { 0x00590000, 0x20004000, 0x60000000, 0x00010000 },
    { 0x05840000, 0x30004000, 0x10000000, 0x00000584 },
    { 0x00420000, 0x200C7000, 0x30000000, 0x00008042 },
    { 0x00000000, 0x20223000, 0x90000000, 0x013FC577 },
    { 0x00000000, 0x202E1000, 0x403A2000, 0x00400578 },
    { 0x00000000, 0x203A2000, 0x402E1000, 0x00400578 },
    { 0x05850000, 0x4046B004, 0x10000000, 0x00000585 },
    { 0x05860000, 0x4046B004, 0x10000000, 0x00000586 },
    { 0x00000000, 0x20541000, 0x90000000, 0x007FC579 },
    { 0x00000000, 0x20601000, 0x406C2180, 0x01000582 },
    { 0x00000000, 0x20784000, 0x90000000, 0x007FC000 },
    { 0x00000000, 0x20843000, 0x90000000, 0x007FC000 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
};

static struct Item_List gi_list[4] = {
    { 20967.28f, 19368.4f, 0xDBE9, 0x8000, 0x20000303 },
    { 21034.08f, 19456.0f, 0xDBE9, 0x3C66, 0x20000305 },
    { 21020.08f, 19402.0f, 0xDBE9, 0x3CE2, 0x20000307 },
    { 0.0f, 0.0f, 0, 0, 0xE0000000 },
};

static int (*ev_prog[3])(void) = {
    NULL,
    EvProgLauraKickKey,
    EvProgGetHandgun,
};

static struct Model_List mdl_list[4] = {
    { 1794, 0, 66, 0, { 19979.719f, -333.7622f, 20395.0f, 0.0f }, { 1.570796f, -0.0f, -1.523667f, 0.0f } },
    { 1842, 0, 67, 0, { 56560.535f, -3.41766f, 11205.266f, 0.0f }, { -1.597323f, -0.06904f, 0.189038f, 0.0f } },
    { 1842, 0, 89, 67, { 57170.984f, -3.53376f, 11713.185f, 0.0f }, { -1.512634f, -0.053164f, 0.41216f, 0.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

static struct Enemy_List en_list[5] = {
    { 512, 78, 60500, 30000, 0, 0, 0, 0x83 },
    { 512, 79, 60200, 28000, 0, -12867, 0, 0x80 },
    { 512, 80, 74000, 11200, 0, 6433, 0, 0x81 },
    { 512, 81, 51500, 11300, 0, -6433, 0, 0x83 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};

struct Stage_Data stage_apart_e3fw = {
    ev_list,
    ev_pos,
    ev_prog,
    gi_list,
    mdl_list,
    en_list,
    NULL,
    EvRoomInit,
    NULL,
    9,
    1,
    pjames_stage_anim,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
};

static int EvProgLauraKickKey(void) {
    static short kick_anim[3] = { 1003, 2503, 10001 };
    static struct DramaDemo_MessageTime kick_msg[5] = {
        { 0x1A2, 0x1C3 },
        { 0x1F3, 0x20B },
        { 0x20B, 0x235 },
        { 0x235, 0x262 },
        { 0xFFFF, 0xFFFF },
    };
    static struct DramaDemo_PlayInfo kick =
    { 9, MemShare_gp_data_buf, kick_anim, kick_msg, 5, 0, NULL, 60035, 25.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList kagikeri_data[4] = {
        { 259, data_chr_jms_hhh_jms_mdl, data_demo_kagikeri_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_kagikeri_hhh_jms_cls },
        { 260, data_chr_lau_lau_mdl, data_demo_kagikeri_lau_anm, data_chr_lau_lau_kg1, data_demo_kagikeri_lau_cls },
        { 1026, data_chr_item_i_keycou_mdl, data_demo_kagikeri_i_keycou_anm, NULL, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    static float yard_key_after_data[2][4] = {
        { 57170.984f, -3.53376f, 11713.185f, 0.0f },
        { -1.512634f, -0.053164f, 0.41216f, 0.0f },
    };
    void DSR_Entry0(); /* Matching: called without a prototype in the original (arguments passed unconverted). */

    if (demo_frame > 405.5f && demo_frame <= 406.7f) {
        DSR_Entry0(__otn_kick_key_00, 0, 1.0);
    }
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        FcRead(data_demo_kagikeri_kagikeri_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(kagikeri_data, 1);
        ev_p_step = 0xA;
        ev_s_step = 0;
    case 0xA:
        if (!EvSubQuestion(3)) {
            break;
        }
        if (fontGetStatus() == 0) {
            ev_p_step = 2;
            ev_s_step = 0;
        } else {
            CharaDataLoadCancel(kagikeri_data);
            ev_p_step = 0xD;
            ev_s_step = 0;
        }
        break;
    case 2:
        if (fsSync(1, -1) < 0) {
            break;
        }
        CharaDataLoadDemo(kagikeri_data, 0);
        CharaAdminPlayableDisplay(0);
        shCharacter_Manage_Delete(NULL, 0x732, 0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        ev_p_step = 0x16;
        ev_s_step = 0;
    case 0x16:
        if (!DramaDemoMain(&kick)) {
            break;
        }
        ev_p_step = 6;
        ev_s_step = 0;
        break;
    case 6:
        CharaDataDeleteOne(0x103);
        CharaDataDeleteOne(0x104);
        CharaDataDeleteOne(0x402);
        CharaWorkCreate(0x732, 0, yard_key_after_data[0], yard_key_after_data[1], 0);
        CharaAdminPlayableDisplay(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        vcReturnPreAutoCamWork(1);
        game_flag.flag[2] |= 8;
        ev_p_step = 0xD;
        ev_s_step = 0;
    case 0xD:
        if ((game_flag.flag[2] >> 2) & 1) {
            EvProgSubScreamOn();
        }
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgGetHandgun(void) {
    int ret;

    ret = EvSubItemGetAndAnim(4, 0);
    if (ret && ((game_flag.flag[2] >> 3) & 1)) {
        EvProgSubScreamOn();
    }
    return ret;
}

static void EvRoomInit(void) {
}

static void EvProgSubScreamOn(void) {
    struct SubCharacter *scp;

    game_flag.flag[2] |= 0x10;
    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = scp->next) {
        if ((scp->kind >> 8) == 2 && RoomName(0, scp->pos.x, scp->pos.z) == 0x15 && scp->battle.hp <= 0.0f) {
            game_flag.enemy[scp->id >> 5] |= 1 << (scp->id & 0x1F);
            shCharacter_Manage_Delete(scp, 0, 0);
        }
    }
}
