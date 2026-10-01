/*
 * stg_apart_e1f.c: stage overlay for the apartments, east building 1F: Eddie vomiting (and after),
 * the tourist guide.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_apart_e1f).
 */
#include "sh2.h"

/* Next program step. Matching: the do/while leaves the original's nop before a case label it
 * falls into. */
#define EV_STEP(n) do { ev_p_step = (n); ev_s_step = 0; } while (0)

static int EvProgVomitEddie(void);
static int EvProgVomitEddieAfter(void);
static int EvProgTouristGuideRead(void);
static int EvCharaDataClear(int room);
static void EvRoomInit(void);
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

static unsigned char ev_pos[244] = {
    0x08, 0xCE, 0x9A, 0xC6, 0x00, 0x80, 0x80, 0xC8, 0xC5, 0x47, 0x8F, 0x5E, 0xF7, 0x69, 0x9A, 0x46,
    0x00, 0x80, 0x80, 0xD7, 0xC5, 0x47, 0x8F, 0x5E, 0x00, 0xB6, 0x99, 0x46, 0x00, 0x80, 0x80, 0x5D,
    0xC7, 0x47, 0x40, 0x62, 0xD8, 0xF5, 0x40, 0xC7, 0x00, 0x80, 0xE1, 0x71, 0x81, 0xC7, 0xD0, 0x5F,
    0xFB, 0x25, 0xAF, 0xC6, 0x00, 0x80, 0xD3, 0x64, 0xC0, 0x47, 0xF9, 0x5F, 0x00, 0xBF, 0xA8, 0xC6,
    0x00, 0x80, 0xBA, 0x10, 0xC1, 0x47, 0xA4, 0x6A, 0xB0, 0x68, 0x03, 0xF6, 0x9A, 0xC6, 0x00, 0x80,
    0xC0, 0x07, 0xC1, 0x47, 0x8F, 0x5E, 0x0F, 0x5B, 0xAB, 0xC6, 0x00, 0x80, 0x00, 0x7A, 0x6E, 0xC7,
    0x00, 0x00, 0xC4, 0x59, 0xBC, 0x59, 0xC4, 0x59, 0x00, 0xA8, 0x45, 0xC7, 0x00, 0x80, 0x00, 0xE9,
    0x68, 0xC7, 0xB0, 0x64, 0x40, 0x66, 0xD8, 0xF5, 0x40, 0xC7, 0x00, 0x80, 0xC0, 0xAA, 0x4D, 0xC7,
    0xD0, 0x5F, 0xD8, 0xF5, 0x40, 0xC7, 0x00, 0x80, 0xC2, 0xAA, 0x34, 0xC7, 0xCF, 0x5F, 0x00, 0x50,
    0xB2, 0xC6, 0x00, 0x80, 0x66, 0x36, 0x70, 0xC7, 0xF9, 0x5F, 0x00, 0xE0, 0xAB, 0xC6, 0x00, 0x80,
    0x00, 0xDE, 0x6E, 0xC7, 0x6C, 0x6B, 0xB0, 0x68, 0xE3, 0x63, 0x42, 0xC7, 0x00, 0x80, 0xFF, 0xEF,
    0x2B, 0xC7, 0x5D, 0xDF, 0x40, 0x9E, 0xCD, 0x36, 0x6C, 0xC7, 0x33, 0x3F, 0x81, 0xFD, 0xAB, 0xC6,
    0x13, 0x60, 0xF3, 0xFC, 0x45, 0xC7, 0x00, 0x80, 0xD7, 0x9D, 0x6C, 0xC7, 0xD9, 0x5F, 0xA3, 0x4F,
    0xC3, 0x47, 0xC4, 0xD4, 0x0B, 0xEF, 0x96, 0xC7, 0xD9, 0x5F, 0x00, 0xE8, 0x32, 0xC7, 0x00, 0x80,
    0x00, 0x29, 0x6F, 0xC7, 0x40, 0x62, 0x40, 0x62, 0x00, 0xE7, 0xAB, 0xC6, 0x00, 0x80, 0xFC, 0x46,
    0x71, 0xC7, 0x93, 0x5F,
};

static struct Event_List ev_list[23] = {
    { 0x00550000, 0x20002000, 0x500C1000, 0x00404055 },
    { 0x00568055, 0x10000000, 0x40002000, 0x00400056 },
    { 0x00000000, 0x20002000, 0x400C1000, 0x00400000 },
    { 0x00000000, 0x200C1000, 0x40002000, 0x00400000 },
    { 0x80550058, 0x20183000, 0x30000000, 0x00008000 },
    { 0x00000000, 0x20244000, 0x40303000, 0x00400537 },
    { 0x00000000, 0x20303000, 0x40244000, 0x00400537 },
    { 0x05450000, 0x403CB004, 0x10000000, 0x00000545 },
    { 0x05460000, 0x403CB004, 0x10000000, 0x00000546 },
    { 0x00000000, 0x204A1000, 0x90000000, 0x007FC000 },
    { 0x00000000, 0x20566004, 0x30000000, 0x0000C054 },
    { 0x05380000, 0x4068B004, 0x10000000, 0x00000538 },
    { 0x00000000, 0x20764000, 0x90000000, 0x007F8539 },
    { 0x00000000, 0x20824000, 0x408E3000, 0x0040053A },
    { 0x00000000, 0x208E3000, 0x40824000, 0x0040053A },
    { 0x05470000, 0x409AB004, 0x10000000, 0x00000547 },
    { 0x05480000, 0x409AB004, 0x10000000, 0x00000548 },
    { 0x803C0000, 0x20A85000, 0x40B61180, 0x0100053E },
    { 0x00000000, 0x20A85004, 0x70000000, 0x013FC03C },
    { 0x00000000, 0x20C23000, 0x40CE41A0, 0x01000000 },
    { 0x05420000, 0x40DAB004, 0x10000000, 0x00000542 },
    { 0x00000000, 0x20E84000, 0x90000000, 0x007FC000 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
};

static struct Item_List gi_list[4] = {
    { -18818.0f, -60080.0f, 0xDC9C, 0x8000, 0x20000291 },
    { -18068.49f, -100861.48f, 0xDC0F, 0xB947, 0x200002F3 },
    { -18048.98f, -100816.96f, 0xDC0F, 0x3CD8, 0x200002F5 },
    { 0.0f, 0.0f, 0, 0, 0xE0000000 },
};

static struct _CL_HITPOLY_PLANE clActWallList_ap100[4] = {
    { 1, 1, 0, 0x00000004, 0x0000000C, 0, { { 19528.0f, -1200.0f, 101820.0f, 1.0f }, { 19528.0f, 0.0f, 101820.0f, 1.0f }, { 19528.0f, 0.0f, 101445.0f, 1.0f }, { 19528.0f, -1200.0f, 101445.0f, 1.0f } } },
    { 1, 1, 0, 0x00000004, 0x0000000C, 0, { { 19528.0f, -1200.0f, 101820.0f, 1.0f }, { 19651.0f, -1200.0f, 101820.0f, 1.0f }, { 19651.0f, 0.0f, 101820.0f, 1.0f }, { 19528.0f, 0.0f, 101820.0f, 1.0f } } },
    { 1, 1, 0, 0x00000004, 0x0000000C, 0, { { 19528.0f, 0.0f, 101445.0f, 1.0f }, { 19650.0f, 0.0f, 101445.0f, 1.0f }, { 19650.0f, -1200.0f, 101445.0f, 1.0f }, { 19528.0f, -1200.0f, 101445.0f, 1.0f } } },
    { 0, 0, 0, 0x00000000, 0x00000000, 0, { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } } },
};

static int (*ev_prog[4])(void) = {
    NULL,
    EvProgVomitEddie,
    EvProgVomitEddieAfter,
    EvProgTouristGuideRead,
};

static struct Model_List mdl_list[2] = {
    { 1371, 0, 0, 0, { -22212.006f, 0.0f, 102151.49f, 0.0f }, { -0.0f, -0.0f, 0.0f, 0.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

static struct Enemy_List en_list[7] = {
    { 512, 51, -50000, -67000, 0, 0, 0, 0 },
    { 512, 52, -50000, -63000, 0, 0, 0, 1 },
    { 512, 53, -46500, -60800, 0, 6433, 0, 3 },
    { 512, 54, -50000, -47000, 0, 12867, 0, 3 },
    { 512, 55, -18300, -60800, 0, 1429, 0, 0 },
    { 512, 56, -21800, -58600, 0, 0, 0, 3 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};

/*
 * The original Stage_Data member is most likely `int (*chara_data_clear)()` (the DWARF can't
 * tell `()` from `(void)`); the cast works around the generated `(void)` prototype.
 */
struct Stage_Data stage_apart_e1f = {
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
    NULL,
    NULL,
    (int (*)(void))EvCharaDataClear,
    NULL,
    0,
};

static float edi_vec[2][4] = {
    { 19376.432f, 357.8001f, 101908.87f, 0.0f },
    { -0.0f, 0.0f, -0.0f, 0.0f },
};

static char *dds_adr_i;
static char *dds_adr_h;

static int EvProgVomitEddie(void) {
    static short vomit_anim[6] = {
        1050,
        4503,
        1051,
        4504,
        1052,
        4505,
    };
    static struct DramaDemo_MessageTime vomit_msg[27] = {
        { 0xAE, 0xFF },
        { 0xFF, 0x123 },
        { 0x123, 0x18F },
        { 0x18F, 0x1E3 },
        { 0x1E3, 0x258 },
        { 0x258, 0x2BB },
        { 0x2BB, 0x333 },
        { 0x333, 0x3ED },
        { 0x3ED, 0x483 },
        { 0x483, 0x501 },
        { 0x513, 0x537 },
        { 0x537, 0x58B },
        { 0x5C4, 0x62A },
        { 0x64B, 0x6C3 },
        { 0x6DB, 0x711 },
        { 0x723, 0x798 },
        { 0x798, 0x816 },
        { 0x8AC, 0x97E },
        { 0x9A8, 0xA14 },
        { 0xA2C, 0xA77 },
        { 0xA77, 0xAB9 },
        { 0xADA, 0xB4F },
        { 0xB4F, 0xB94 },
        { 0xBC4, 0xC0F },
        { 0xC3F, 0xC6C },
        { 0xC6C, 0xCED },
        { 0xFFFF, 0xFFFF },
    };
    static struct DramaDemo_PlayInfo vomit = { 13, MemShare_gp_data_buf, vomit_anim, vomit_msg, 2, 0, NULL, 60018, 0.0f, 0.0f, 0.0f };

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_gero_edi_gero_edi_dds, MemShare_gp_data_buf);
        fsSync(0, -1);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        ScreenEffectFadeStart(3, 0.0f);
        EV_STEP(0x28);
    case 0x28:
        EvSubMovieReady(data_movie_gero_pss, NULL, 0);
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
        if (!DramaDemoMain(&vomit)) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0x2F:
        EvSubMovieEnd();
        EV_STEP(0xD);
    case 0xD:
        shCharacter_Manage_Delete(NULL, 0x103, 0);
        shCharacter_Manage_Delete(NULL, 0x108, 0);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        game_flag.flag[2] |= 0x800000;
        return 1;
    }
    return 0;
}

static int EvProgVomitEddieAfter(void) {
    static short anim_h[2] = { 0x41D, 0x119A };
    static short anim_i[2] = { 0x41E, 0x119B };
    static struct DramaDemo_MessageTime msg_h[3] = {
        { 0x1C, 0x67 },
        { 0x67, 0xA3 },
        { 0xFFFF, 0xFFFF },
    };
    static struct DramaDemo_MessageTime msg_i[5] = {
        { 0x3D, 0x64 },
        { 0x70, 0x136 },
        { 0x136, 0x181 },
        { 0x181, 0x1CC },
        { 0xFFFF, 0xFFFF },
    };
    static struct DramaDemo_PlayInfo info_h = { 13, NULL, anim_h, msg_h, 28, 0, NULL, 60020, 28.0f, 0.0f, 0.0f };
    static struct DramaDemo_PlayInfo info_i = { 13, NULL, anim_i, msg_i, 30, 0, NULL, 60019, 55.0f, 0.0f, 0.0f };
    struct SubCharacter *scp;
    int ret;

    switch (ev_p_step) {
    case 0:
        info_h.adr_dds_top = dds_adr_h;
        info_i.adr_dds_top = dds_adr_i;
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        EV_STEP(0x16);
    case 0x16:
        ret = DramaDemoMain(((game_flag.flag[2] >> 23) & 1) ? &info_h : &info_i);
        if (!ret) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        shCharacter_Manage_Delete(NULL, 0x103, 0);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        if (!((game_flag.flag[2] >> 23) & 1)) {
            sh2jms.player->rot.y = -2.5585005f;
        }
        scp = shCharacterGetSubCharacter(0x108, 0);
        scp->pos.x = edi_vec[0][0];
        scp->pos.y = edi_vec[0][1];
        scp->pos.z = edi_vec[0][2];
        scp->rot.z = 0.0f;
        scp->rot.x = 0.0f;
        scp->rot.y = 3.1415927f;
        shCharacterHumanEDIAnimeSetP(scp, 0x12C1);
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        return 1;
    }
    return 0;
}

/*
 * Matching: the step change is a do { } while (0) macro: where it falls through into the next case
 * label (0x1E -> 2) it leaves a nop behind.
 */
static int EvProgTouristGuideRead(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        if ((game_flag.flag[2] >> 20) & 1) {
            EV_STEP(2);
        }
        EV_STEP(0x1E);
        break;
    case 0x1E:
        if (!EvSubMessage(1)) {
            break;
        }
        EV_STEP(2);
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_p_tourist_tex, NULL)) {
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
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(4);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
        ScreenEffectFadeStart(4, 0.0f);
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvCharaDataClear(int room) {
    if (room == 0x1E && !((game_flag.flag[2] >> 24) & 1)) {
        return 1;
    }
    return 0;
}

static void EvRoomInit(void) {
    static struct CharaData_DemoList chara_data[3] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_gero_edi_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_gero_edi_hhh_jms_cls },
        { 264, data_chr_edi_hhh_edi_mdl, data_demo_gero_edi_hhh_edi_anm, data_chr_edi_hhh_edi_kg1, data_demo_gero_edi_hhh_edi_cls },
        { 0, NULL, NULL, NULL, NULL },
    };
    struct SubCharacter *scp;
    int room;

    room = RoomNameJms();
    if (room == 0x1E && !((game_flag.flag[2] >> 24) & 1)) {
        CharaDataLoadDemo(chara_data, 0);
        if ((game_flag.flag[2] >> 21) & 1) {
            scp = CharaWorkCreate(0x108, 0, edi_vec[0], (float[4]){ 0.0f, 3.1415927f, 0.0f, 0.0f }, 0);
            shCharacterHumanEDIAnimeSetP(scp, 0x12C1);
            dds_adr_h = (char *)CharaDataLoadExtra(data_demo_gero_edi_gero_edi_h_dds, 0x200);
            dds_adr_i = (char *)CharaDataLoadExtra(data_demo_gero_edi_gero_edi_i_dds, 0x200);
            fsSync(0, -1);
        }
    }
}

static void EvAllTimeFunc(void) {
    int room;

    room = RoomNameJms();
    if (room == 0x1E && !((game_flag.flag[2] >> 24) & 1)) {
        clAddDynamicWall(clActWallList_ap100);
    }
}
