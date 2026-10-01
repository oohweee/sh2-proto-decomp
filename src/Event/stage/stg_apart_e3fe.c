/*
 * stg_apart_e3fe.c: stage overlay for the apartments, east building 3F east: first meeting with
 * the triangle head, the yard and emergency keys.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_apart_e3fe).
 */
#include "sh2.h"

static int EvProgFirstMeetTrihead(void);
static int EvProgGetYardKey(void);
static int EvProgGetEmergencyKey(void);
static void EvStageInit(void);
static int EvCharaDataClear(int room);
static void EvAllTimeFunc(void);
static void ap18_ap21_Closet(void);
static void ap18_ap21_HighLight(void);

union Q_WORDDATA OutputVertex[54];
union Q_WORDDATA ScissorVertex[54];

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

static unsigned char ev_pos[248] = {
    0x02, 0x8F, 0x6D, 0x47, 0x00, 0x80, 0x0D, 0x51, 0x9A, 0x46, 0xD0, 0x5F, 0xFA, 0x1B, 0xAF, 0xC6,
    0x00, 0x80, 0x2C, 0x3B, 0xC6, 0xC7, 0xF9, 0x5F, 0x00, 0xBF, 0xA8, 0xC6, 0x00, 0x80, 0x40, 0x8F,
    0xC5, 0xC7, 0x77, 0x69, 0xB0, 0x68, 0x38, 0x57, 0xA5, 0xC6, 0x00, 0x80, 0x06, 0x74, 0xC6, 0xC7,
    0x04, 0x52, 0x20, 0x5E, 0xF1, 0x4D, 0x5F, 0x47, 0x00, 0x80, 0xE1, 0xD7, 0x36, 0x46, 0x7A, 0x48,
    0x66, 0x47, 0x00, 0x80, 0x1F, 0x84, 0x28, 0x46, 0xD0, 0x5F, 0x36, 0x93, 0x90, 0x46, 0x00, 0x80,
    0x00, 0xD8, 0x73, 0x47, 0xF8, 0x5F, 0x75, 0x45, 0x93, 0x46, 0x00, 0x80, 0xC0, 0x58, 0x63, 0x47,
    0xB0, 0x68, 0xA4, 0x6A, 0x7F, 0x48, 0x7F, 0x47, 0x00, 0x80, 0x1F, 0x84, 0x28, 0x46, 0xD0, 0x5F,
    0x00, 0x10, 0x8B, 0x47, 0x00, 0x80, 0x00, 0xC0, 0x28, 0x46, 0x40, 0x66, 0xB0, 0x64, 0x00, 0x60,
    0x6A, 0x47, 0x00, 0x80, 0x00, 0x00, 0x48, 0x46, 0x40, 0x62, 0x40, 0x66, 0x02, 0x8F, 0x6D, 0x47,
    0x00, 0x80, 0x03, 0x51, 0xCC, 0x46, 0xD0, 0x5F, 0x01, 0x8F, 0x6D, 0x47, 0x00, 0x80, 0x1C, 0x51,
    0xFE, 0x46, 0xCF, 0x5F, 0x31, 0xDF, 0x6C, 0x47, 0x00, 0x80, 0x7A, 0xF8, 0x07, 0x47, 0x5D, 0x5F,
    0xCD, 0x36, 0x6C, 0xC7, 0x3F, 0xEA, 0x81, 0xFD, 0xAB, 0xC6, 0x13, 0x60, 0xFE, 0x32, 0x9A, 0x47,
    0x00, 0x80, 0x00, 0xE8, 0x28, 0x46, 0xD0, 0x5F, 0x80, 0x5E, 0x99, 0xC7, 0x08, 0xEB, 0x02, 0x9A,
    0x62, 0xC7, 0x4C, 0x60, 0x03, 0xF6, 0x9A, 0xC6, 0x00, 0x80, 0x3D, 0x93, 0xC5, 0xC7, 0x8F, 0x5E,
    0x08, 0xCE, 0x9A, 0xC6, 0x00, 0x80, 0x00, 0xDA, 0xC0, 0xC7, 0x8F, 0x5E, 0xFB, 0xB5, 0xAD, 0xC6,
    0x00, 0x80, 0x80, 0x8E, 0xC7, 0xC7, 0x59, 0x5F,
};

static struct Event_List ev_list[22] = {
    { 0x005B0000, 0x20004000, 0x500C3000, 0x0040457E },
    { 0x00000000, 0x20004000, 0x400C3000, 0x00400000 },
    { 0x00000000, 0x200C3000, 0x40004000, 0x00400000 },
    { 0x05890000, 0x4018B004, 0x10000000, 0x00000589 },
    { 0x058A0000, 0x4018B004, 0x10000000, 0x0000058A },
    { 0x00590000, 0x20265004, 0x30000000, 0x00008059 },
    { 0x005C0000, 0x20340004, 0x30000000, 0x0000C05C },
    { 0x00000000, 0x203E1000, 0x404A2000, 0x0040057A },
    { 0x00000000, 0x204A2000, 0x403E1000, 0x0040057A },
    { 0x05870000, 0x4056B004, 0x10000000, 0x00000587 },
    { 0x05880000, 0x4056B004, 0x10000000, 0x00000588 },
    { 0x00000000, 0x20641000, 0x90000000, 0x007FC57B },
    { 0x057C0000, 0x4070B004, 0x10000000, 0x0000057C },
    { 0x057D0000, 0x407EB004, 0x10000000, 0x0000057D },
    { 0x00000000, 0x208C4000, 0x90000000, 0x007FC57F },
    { 0x00000000, 0x20984000, 0x90000000, 0x007FC580 },
    { 0x00000000, 0x20A42000, 0x40B01180, 0x01000581 },
    { 0x00000000, 0x20BC1000, 0x40C82180, 0x01000583 },
    { 0x00000000, 0x20D41000, 0x90000000, 0x007FC000 },
    { 0x00000000, 0x20E02000, 0x90000000, 0x007FC000 },
    { 0x00000000, 0x20EC1000, 0x90000000, 0x007FC000 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
};

static struct Item_List gi_list[5] = {
    { 22615.16f, 59042.9f, 0xDAA0, 0x3AE1, 0x20000309 },
    { 22507.57f, 59033.32f, 0xDAA0, 0x3C86, 0x2000030B },
    { 21007.69f, 59263.01f, 0xDEC2, 0xBC3D, 0xA000030D },
    { 21891.17f, 62063.79f, 0xDDAF, 0x8000, 0x8000030F },
    { 0.0f, 0.0f, 0, 0, 0xE0000000 },
};

static int (*ev_prog[4])(void) = {
    NULL,
    EvProgFirstMeetTrihead,
    EvProgGetYardKey,
    EvProgGetEmergencyKey,
};

static struct Model_List mdl_list[4] = {
    { 1836, 0, 89, 0, { -21050.0f, -426.00003f, -101465.0f, 0.0f }, { 1.570796f, -0.0f, 0.698132f, 0.0f } },
    { 1842, 0, 67, 0, { 56560.535f, -3.41766f, 11205.266f, 0.0f }, { -1.597323f, -0.06904f, 0.189038f, 0.0f } },
    { 1842, 0, 92, 67, { 57170.984f, -3.53376f, 11713.185f, 0.0f }, { -1.512634f, -0.053164f, 0.41216f, 0.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

static struct Enemy_List en_list[9] = {
    { 512, 78, 60500, 30000, 0, 0, 0, 0x83 },
    { 512, 79, 60200, 28000, 0, -12867, 0, 0x80 },
    { 512, 80, 74000, 11200, 0, 6433, 0, 0x81 },
    { 512, 81, 51500, 11300, 0, -6433, 0, 0x83 },
    { 512, 82, 17700, 59400, 0, -9650, 0, 0 },
    { 512, 83, 18000, 58900, 0, 12867, 0, 3 },
    { 513, 6, -21548, -97828, -66, 12867, 13, 0x100 },
    { 513, 7, -21832, -99490, -44, 12867, 14, 0x100 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};

static struct Stage_GfwFunc gfw_func = {
    ap18_ap21_Closet,
    ap18_ap21_HighLight,
    NULL,
    NULL,
};

/* See stg_apart_e1f.c for the cast. */
struct Stage_Data stage_apart_e3fe = {
    ev_list,
    ev_pos,
    ev_prog,
    gi_list,
    mdl_list,
    en_list,
    EvStageInit,
    NULL,
    EvAllTimeFunc,
    9,
    1,
    pjames_stage_anim,
    NULL,
    &gfw_func,
    (int (*)(void))EvCharaDataClear,
    NULL,
    0,
};

/* The position/rotation vectors are compound literals (the DWARF has no locals for them). */
static int EvProgFirstMeetTrihead(void) {
    static short first_anim[21] = {
        1020, 6302, 6102, 6105, 10061, 10064, 10067, 1021, 6303, 6103, 6106,
        10062, 10065, 10068, 1022, 6304, 6104, 6107, 10063, 10066, 10069,
    };
    static struct DramaDemo_MessageTime first_msg[2] = {
        { 0x844, 0x86B },
        { 0xFFFF, 0xFFFF },
    };
    static struct DramaDemo_PlayInfo first = { 14, MemShare_gp_data_buf, first_anim, first_msg, 0, 0, NULL, 60076, 25.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList first_data[7] = {
        { 513, data_chr_mkn_mkn_mdl, data_demo_sankaku1_mkn_anm, data_chr_mkn_mkn_kg1, NULL },
        { 1036, data_chr_item_b_doo_mdl, data_demo_sankaku1_b_doo_anm, NULL, NULL },
        { 259, data_chr_jms_hhh_jms_mdl, data_demo_sankaku1_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_sankaku1_hhh_jms_cls },
        { 520, data_chr_red_red_mdl, data_demo_sankaku1_red_anm, data_chr_red_red_kg1, NULL },
        { 1037, data_chr_item_i_handgun_mdl, data_demo_sankaku1_i_handgun_anm, NULL, NULL },
        { 1038, data_chr_item_i_magazine_mdl, data_demo_sankaku1_i_magazine_anm, NULL, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    static float vec[4][4] = {
        { -21548.1f, -66.19f, -97828.0f, 0.0f },
        { 0.0f, 3.1415927f, 0.0f, 0.0f },
        { -21832.35f, -43.685f, -99490.0f, 0.0f },
        { 0.0f, 3.1415927f, 0.0f, 0.0f },
    };
    u_long128 *adr;

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_sankaku1_sankaku_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(first_data, 0);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        CharaWorkCreate(0x208, 0, (float[4]){ 0.0f, 0.0f, 0.0f, 1.0f }, (float[4]){ 0.0f, 0.0f, 0.0f, 1.0f }, 0);
        shCharacterSetWeaponRED(shCharacterGetSubCharacter(0x208, 0), 0);
        ev_p_step = 0x16;
        ev_s_step = 0;
    case 0x16:
        if (!DramaDemoMain(&first)) {
            break;
        }
        ScreenEffectFadeStart(2, 0.0f);
        ev_p_step = 4;
        ev_s_step = 0;
        break;
    case 4:
        DramaDemoSkipLast(&first);
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        ev_p_step = 0xD;
        ev_s_step = 0;
        break;
    case 0xD:
        SeCallPos(0x3E86, 1.0f, (float[4]){ -21300.0f, -550.0f, -101150.0f, 0.0f }, 0);
        CharaDataDeleteOne(0x103);
        CharaDataDeleteOne(0x208);
        CharaDataDeleteOne(0x40C);
        CharaDataDeleteOne(0x40D);
        CharaDataDeleteOne(0x40E);
        CharaAdminPlayableDisplay(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        adr = CharaDataLoadExtra(data_chr_mkn_mkn_anm, 0x400);
        fsSync(0, -1);
        CharaDataAnimSetExtra(0x201, data_chr_mkn_mkn_anm, adr, 1);
        CharaAdminReCreate(0x201, 6, 0, vec[0], vec[1], 0xD);
        CharaAdminReCreate(0x201, 7, 1, vec[2], vec[3], 0xE);
        sh2jms.player->pos.x = -21300.0f;
        sh2jms.player->pos.z = -100950.0f;
        sh2jms.player->rot.y = 0.0f;
        vcReturnPreAutoCamWork(1);
        ScreenEffectFadeStart(4, 0.0f);
        game_flag.flag[2] |= 0x8000000;
        return 1;
    }
    return 0;
}

static int EvProgGetYardKey(void) {
    return EvSubItemGetAndAnim(0x1A, 1);
}

static int EvProgGetEmergencyKey(void) {
    return EvSubItemGetAndAnim(0x1B, 2);
}

static unsigned char cam_change = 0;

static void EvStageInit(void) {
    cam_change = 0;
}

static int EvCharaDataClear(int room) {
    if (room == 0x13 && !((game_flag.flag[2] >> 27) & 1)) {
        return 1;
    }
    return 0;
}

static void EvAllTimeFunc(void) {
    static struct Event_CamSetData cam_data = {
        { -20820.0f, -530.0f, -101460.0f, 0.0f },
        { -20820.996f, -529.97546f, -101459.93f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        0.098177f,
    };
    int disp_ctrl_list[3];

    disp_ctrl_list[0] = 0;
    switch (RoomNameJms()) {
    case 0x13:
        if ((game_flag.flag[2] >> 27) & 1) {
            EvDispControlModelEntry(disp_ctrl_list, 0x13, 0);
            if (sh2jms.player->pos.x > -21600.0f && sh2jms.player->pos.z < -101150.0f) {
                if (!((game_flag.flag[2] >> 25) & 1)) {
                    vcSetEventCamParamRefView(cam_data.pos, NULL, cam_data.itr, NULL, cam_data.roll, 1);
                    vcMoveAndSetCamera(0, 0, 0, 0, 0, 0, 0, 0);
                    cam_change = 1;
                }
            } else if (cam_change) {
                vcReturnPreAutoCamWork(1);
                cam_change = 0;
            }
        } else {
            sh2shd_add_map_to_shadow_off_work(0x12);
            sh2shd_off_obj(0x12, 0x21);
            EvDispControlModelEntry(disp_ctrl_list, 0x13, -1);
        }
        break;
    }
    EvDispControlModelExec(disp_ctrl_list);
}

static float ap18_ap24_ClosetData[32][4] = {
    { -21550.0f, -900.0f, -101250.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 128.0f },
    { -21050.0f, -900.0f, -101250.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 128.0f },
    { -21550.0f, -828.0f, -101250.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 64.0f },
    { -21050.0f, -828.0f, -101250.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 64.0f },
    { -21550.0f, -780.0f, -101250.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 64.0f },
    { -21050.0f, -780.0f, -101250.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 64.0f },
    { -21550.0f, -708.0f, -101250.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 128.0f },
    { -21050.0f, -708.0f, -101250.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 128.0f },
};

static float hcol[4] = {
    1.0f,
    1.0f,
    1.0f,
    0.0f,
};

static float hdir[4] = {
    -0.198359f,
    0.319f,
    0.928f,
    0.0f,
};

static void ap18_ap21_Closet(void) {
    static union Q_WORDDATA env[4];
    Q_WORDDATA *kickbuf;
    Q_WORDDATA *scikickbuf;
    int i;

    kickbuf = OutputVertex;
    scikickbuf = ScissorVertex;
    if (!((Sh2sys.main_status >> 6) & 1)) {
        Env_ctl.camera_parms[3] = 100.0f;
        return;
    }
    Env_ctl.camera_parms[3] = 100.0f;
    if (!(demo_frame >= 360.5f && demo_frame < 1698.0f)) {
        return;
    }
    env[0].ui32[0] = 0x10000002;
    env[0].ui32[1] = 0;
    env[0].ui32[2] = 0x11000000;
    env[0].ui32[3] = 0x50000002;
    env[1].ui32[3] = 0;
    env[1].ui32[2] = 0xE;
    env[1].ui32[1] = 0x10000000;
    env[1].ui32[0] = 0x8001;
    env[2].ul64[1] = 0x42;
    env[2].ul64[0] = 0x6000000044;
    env[3].ui32[0] = 0x70000000;
    env[3].ui32[1] = 0;
    env[3].ul64[1] = 0;
    for (i = 0; i < 2; i++) {
        MakeScissorPacket(ap18_ap24_ClosetData[i * 16], (void **)&kickbuf, (void **)&scikickbuf, 0, 0x4C, 4);
    }
    kickbuf->ul128 = 0;
    kickbuf->ui32[0] = 0x70000000;
    scikickbuf->ul128 = 0;
    scikickbuf->ui32[0] = 0x70000000;
    d1cSend(env);
    d1cSend(OutputVertex);
    d1cSend(ScissorVertex);
}

static void ap18_ap21_HighLight(void) {
    if (DramaDemoNumber() == 0xE && demo_frame >= 360.5f) {
        sh2gfw_Set_DemoRefrectionHighLight(hdir, hcol);
    }
}
