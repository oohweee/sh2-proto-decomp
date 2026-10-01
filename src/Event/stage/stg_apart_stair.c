/*
 * stg_apart_stair.c: stage overlay for the apartment stairs: the yard key, the apartment maps, the
 * canned juice and the boss fight (siren, end).
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_apart_stair).
 */
#include "sh2.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static int EvProgUseYardKey(void);
static int EvProgGetApartMap(void);
static int EvProgGetApartWestMap(void);
static int EvProgGetCannedJuice(void);
static int EvProgApartBoss(void);
static int EvProgApartBossSiren(void);
static int EvProgApartBossEnd(void);
static void EvRoomInit(void);
static void EvAllTimeFunc(void);

static struct _AnimeInfo pjames_stage_anim[24] = {
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
    { 0x4E3D, 0xF, 1024, 0x5B8, 0x5C6, 0, 0 },
    { 0x4E3E, 0x18, 2048, 0x5C7, 0x5DE, 1, 0 },
    { 0x4E3F, 0x1E, 1024, 0x5DF, 0x5FC, 0, 0 },
    { 0x4E40, 0x1E, 1024, 0x5FD, 0x61A, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
};

static unsigned char ev_pos[482] = {
    0x72, 0x30, 0xC0, 0x47, 0xA4, 0xE6, 0xDE, 0xE5, 0xC0, 0xC7, 0x5D, 0x5F, 0x8D, 0xE3, 0xC4, 0x47,
    0, 0x80, 0xCA, 0x89, 0xC4, 0xC5, 0x5D, 0x5F, 0, 0x94, 0xC0, 0xC7, 0, 0x80, 0, 0xC0,
    0xA8, 0xC6, 0x40, 0x5A, 0xC0, 0xD9, 0x99, 0xC7, 0, 0x80, 0x3D, 0xFC, 0x30, 0xC7, 0x87, 0xD0,
    0x99, 0xC7, 0, 0x80, 0xEF, 0x14, 0x31, 0xC7, 0xDA, 0xE8, 0x99, 0xC7, 0, 0x80, 0x36, 0x39,
    0x31, 0xC7, 0x73, 0xF2, 0x99, 0xC7, 0, 0x80, 0x1C, 0x21, 0x31, 0xC7, 0x3A, 0x77, 0xC1, 0x47,
    0, 0x80, 0x19, 0x89, 0xC0, 0xC7, 0xCB, 0x5F, 0, 0xC4, 0x86, 0x46, 0, 0x80, 0, 0xF6,
    0xD1, 0x46, 0xBB, 0x5F, 0, 0x57, 0xC6, 0xC7, 0, 0x80, 0, 0xA8, 0xAC, 0xC6, 0x40, 0x5E,
    0xBC, 0x45, 0xA, 0x48, 0xC2, 0xE6, 0x1C, 0x63, 0x9A, 0xC6, 0xA4, 0xB2, 0, 0x5D, 0xBC, 0x45,
    0xA, 0x48, 0xC2, 0xE6, 0x1C, 0x63, 0x9A, 0xC6, 0x83, 0xD8, 0x18, 0x5A, 0xA4, 0xB2, 0, 0x5D,
    0x79, 0xB7, 0x9A, 0xC7, 0, 0x80, 0x74, 0x84, 0x5C, 0xC7, 0xD0, 0x5F, 0x79, 0xB7, 0x9A, 0xC7,
    0, 0x80, 0x79, 0xF4, 0x41, 0xC7, 0xCF, 0x5F, 0x39, 0x6C, 0x99, 0xC7, 0, 0x80, 2, 0x1D,
    0x30, 0xC7, 0xD0, 0x5F, 0x66, 0x7A, 0xC0, 0xC7, 0, 0x80, 2, 0x40, 0x83, 0xC6, 0xD9, 0x5F,
    0x1F, 0xFD, 0xB9, 0x47, 0xC4, 0xD4, 0x89, 0x1B, 0xA2, 0xC7, 0xD9, 0x5F, 0, 0xD5, 0xC4, 0xC7,
    0, 0x80, 0, 0x7C, 0xB5, 0xC6, 0x14, 0x66, 0x7A, 0x2C, 0xCD, 0xC7, 0, 0x80, 0x47, 0x2D,
    0xBB, 0x45, 0xC, 0x63, 0x80, 0x21, 0x9D, 0xC7, 0, 0x80, 0xFE, 0xFD, 0x76, 0xC7, 0xB, 0x63,
    0x7A, 0x4C, 0x9E, 0xC7, 0, 0x80, 0x47, 0x2D, 0xBB, 0x45, 0xC, 0x63, 0, 0x7B, 0xC0, 0xC7,
    8, 0xE7, 6, 0xB4, 0x8C, 0xC6, 0xC5, 0x5F, 0x80, 0xD, 0xCC, 0xC7, 0, 0x80, 0, 0xE8,
    0x28, 0x46, 0x4C, 0x60, 0, 0x7B, 0xC0, 0xC7, 8, 0xEB, 6, 0xB4, 0x8C, 0xC6, 0xC5, 0x5F,
    2, 0x36, 0x58, 0x47, 0, 0x80, 0, 0xE8, 0x28, 0x46, 0xD0, 0x5F, 0x80, 0x5E, 0x99, 0xC7,
    7, 0xE7, 2, 0x9A, 0x62, 0xC7, 0x4C, 0x60, 0x80, 0x5E, 0x99, 0xC7, 8, 0xEB, 2, 0x9A,
    0x62, 0xC7, 0x4C, 0x60, 0xFE, 0x32, 0x9A, 0x47, 0, 0x80, 0, 0xE8, 0x28, 0x46, 0xD0, 0x5F,
    0xCD, 0x36, 0x6C, 0xC7, 0x33, 0x3F, 0x81, 0xFD, 0xAB, 0xC6, 0x13, 0x60, 0xE3, 0x63, 0x42, 0xC7,
    0, 0x80, 0xFF, 0xEF, 0x2B, 0xC7, 0x5D, 0xDF, 0x40, 0x9E, 0xCD, 0x36, 0x6C, 0xC7, 0x3E, 0xE6,
    0x81, 0xFD, 0xAB, 0xC6, 0x13, 0x60, 0x66, 0xAC, 0xC1, 0xC7, 0, 0x80, 0x78, 0xF8, 7, 0x47,
    0x5D, 0x5F, 0xCD, 0x36, 0x6C, 0xC7, 0x3F, 0xEA, 0x81, 0xFD, 0xAB, 0xC6, 0x13, 0x60, 0x31, 0xDF,
    0x6C, 0x47, 0, 0x80, 0x7A, 0xF8, 7, 0x47, 0x5D, 0x5F, 0x40, 0x46, 0xA, 0x48, 0, 0x80,
    5, 0xAE, 0xA0, 0xC6, 0xDF, 0x5F, 0xFD, 0x8C, 0xC2, 0x47, 0, 0x80, 0xFB, 0xBE, 0x6D, 0xC7,
    0xDF, 0x5F, 0x40, 0x46, 0xA, 0x48, 0xA4, 0xE6, 0xF8, 0xDC, 0x94, 0xC6, 0xDF, 0x5F, 0x5A, 0x8D,
    0xC2, 0x47, 0, 0x80, 0x31, 0x82, 0x8D, 0xC6, 0xDF, 0x5F, 0x40, 0x50, 8, 0x48, 7, 0xE9,
    0xA, 0xE9, 0x93, 0xC6, 0x12, 0x5F, 3, 0x28, 7, 0x48, 0, 0x80, 0x27, 0x6D, 0x9D, 0xC6,
    0xCB, 0x5F,
};

static struct Event_List ev_list[36] = {
    { 0x80920000, 0x20003000, 0x400C4160, 0x10005B2 },
    { 0x920000, 0x20003000, 0x80000000, 0x1000000 },
    { 0, 0x20184000, 0xA0000000, 0x14549 },
    { 0x5E0000, 0x20240004, 0x30000000, 0x1005E },
    { 0x5E0000, 0x202E0004, 0x30000000, 0x1005E },
    { 0x5E0000, 0x20380004, 0x30000000, 0x1005E },
    { 0x5E0000, 0x20420004, 0x30000000, 0x1005E },
    { 0x8F85B2, 0x10000000, 0x30000000, 0x1408F },
    { 0x80900091, 0x10000000, 0x30000000, 0x18091 },
    { 0x80920093, 0x10000000, 0x30000000, 0x1C000 },
    { 0, 0x204C2000, 0x405811C0, 0x1000000 },
    { 0x190000, 0x20643000, 0x30000000, 0x8019 },
    { 0x1A0000, 0x20705004, 0x30000000, 0xC01A },
    { 0x1A0000, 0x207E6004, 0x30000000, 0xC01A },
    { 0, 0x20903000, 0x90000000, 0x7FC53B },
    { 0, 0x209C3000, 0x90000000, 0x7FC53C },
    { 0, 0x20A82000, 0x90000000, 0x7FC53D },
    { 0x805A0000, 0x20B42000, 0x40C011A0, 0x1000543 },
    { 0, 0xA0B421AC, 0x30000000, 0x405A },
    { 0, 0x20B42000, 0x80000000, 0x1000544 },
    { 0, 0x20CC1000, 0x40D821A0, 0x1C00000 },
    { 0x803D0000, 0x20E41000, 0x40F021A0, 0x1C00534 },
    { 0, 0x20E41004, 0x70000000, 0x1FFC03D },
    { 0, 0x20FC2000, 0x410810E0, 0x1000560 },
    { 0, 0x21142000, 0x41201100, 0x1000582 },
    { 0, 0x212C2000, 0x90000000, 0x13F8561 },
    { 0, 0x21382000, 0x41441120, 0x1000583 },
    { 0x803C0000, 0x21501000, 0x415C50C0, 0x100053E },
    { 0, 0x21501000, 0x80000000, 0x100053F },
    { 0, 0x216A1000, 0x417620E0, 0x100055F },
    { 0, 0x21821000, 0x418E2120, 0x1000581 },
    { 0, 0x219A4000, 0x41A63140, 0x100059C },
    { 0, 0x21B24000, 0x41BE3160, 0x10005B4 },
    { 0, 0x21CA4000, 0x90000000, 0x23FC5B6 },
    { 0, 0x21D63000, 0x90000000, 0x13F859F },
    { 0, 0, 0, 0 },
};

static struct Item_List gi_list[6] = {
    { -60082.9f, -21225.91f, 0xEB17, 0x3C5C, 0x20000311 },
    { -100185.01f, -20166.86f, 0xDBF1, 0x8000, 0x800003A7 },
    { -100147.92f, -20119.97f, 0xDBEF, 0x8000, 0x80000477 },
    { -99666.58f, -20003.62f, 0xCCC4, 0x33BA, 0xA0000479 },
    { -78963.0f, -45383.0f, 0xCCFF, 0xB688, 0xA0000313 },
    { 0.0f, 0.0f, 0, 0, 0xE0000000 },
};

static struct _CL_HITPOLY_PLANE clActWallList_ap88[2] = {
    { 1, 1, 0, 0x00000004, 0x0000000C, 0, { { 99850.0f, -2605.2f, -99415.0f, 1.0f }, { 99850.0f, -1322.0f, -99415.0f, 1.0f }, { 99850.0f, -1322.0f, -98579.5f, 1.0f }, { 99850.0f, -2605.2f, -98579.5f, 1.0f } } },
    { 0, 0, 0, 0x00000000, 0x00000000, 0, { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } } },
};

/* sh2gfw_OV_PreDraw.h */
static float colvec[4] = { 0.3f, 0.3f, 0.3f, 0.0f };
static float colref[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
static float ldir[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
static struct DynamicLight DynamicLW = {
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f, 0.0f },
};

static void AP_Hosei_Light(void) {
    int *mp;

    mp = Get_NowMapId();
    switch (*mp) {
    case 0x9000A: {
        union Q_WORDDATA tmp;

        tmp = Env_ctl.compo_shadow_col;
        tmp.si32[3] = 10;
        sh2gfw_Set_FilterData(sh2gfw_Get_NightOrDay(), 0, &tmp);
        Env_ctl.camera_parms[3] = 50.0f;
        break;
    }
    case 0x90058:
        if (DramaDemoNumber() == 0x13) {
            sh2gfw_Set_DemoRefrectionHighLight(ldir, colref);
        } else {
            float inner;
            float factor;
            float tmp[4];
            float plane[4];
            float aimdir[4];

            sceVu0CopyVector(DynamicLW.BeforeCamDir, DynamicLW.NowCamDir);
            sceVu0CopyVector(DynamicLW.BeforeDir, DynamicLW.NowDir);
            sh2gde_Get_EyeDir(DynamicLW.NowCamDir);
            inner = _shInnerProduct(DynamicLW.NowCamDir, DynamicLW.BeforeDir);
            _shOuterProduct(plane, DynamicLW.NowCamDir, DynamicLW.BeforeDir);
            _shNormalize(plane, plane);
            _shOuterProduct(aimdir, DynamicLW.BeforeDir, plane);
            factor = 0.1f / (2.0f + inner);
            if (factor < 0.0333f) {
                factor = 0.0f;
            }
            _shScaleVector(aimdir, aimdir, factor);
            _shAddVector(tmp, aimdir, DynamicLW.BeforeDir);
            _shNormalize(DynamicLW.NowDir, tmp);
            sh2gfw_Set_DemoRefrectionHighLight(DynamicLW.NowDir, colref);
            sh2gfw_Set_PallarelLight(DynamicLW.NowDir, colvec, 1);
        }
        break;
    }
}

static int (*ev_prog[8])(void) = {
    NULL,
    EvProgUseYardKey,
    EvProgGetApartMap,
    EvProgGetApartWestMap,
    EvProgGetCannedJuice,
    EvProgApartBoss,
    EvProgApartBossSiren,
    EvProgApartBossEnd,
};

static struct Model_List mdl_list[2] = {
    { 1837, 0, 94, 0, { -78787.0f, -29.999998f, -45339.023f, 0.0f }, { -3.141592f, -0.929592f, -3.141592f, 0.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

static struct Stage_GfwFunc SpecialDrawFunctions = { NULL, AP_Hosei_Light, NULL, NULL };

static struct Enemy_List en_list[2] = {
    { 512, 9, 100800, -101000, -1700, -6433, 13, 0x140 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};

struct Stage_Data stage_apart_stair = {
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
    &SpecialDrawFunctions,
    NULL,
    NULL,
    0,
};

static int EvProgUseYardKey(void) {
    return EvSubItemUse0(0x1A, 4, 0x4A51, 0, (float[4]){ -98800.0f, -500.0f, -16800.0f, 0.0f }, 1);
}

static int EvProgGetApartMap(void) {
    return EvSubMapGet(data_pic_map_apartmape1f_tex, 2);
}

static int EvProgGetApartWestMap(void) {
    return EvSubMapGet(data_pic_map_apartmapw_tex, 3);
}

static int EvProgGetCannedJuice(void) {
    return EvSubItemGetAndAnim(0x2E, 1);
}

static int EvProgApartBoss(void) {
    static short boss_anim[3] = { 1171, 6306, 6009 };
    static struct DramaDemo_PlayInfo boss = { 0x13, MemShare_gp_data_buf, boss_anim, NULL, 0, 0, NULL, 0xEA62, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[4] = {
        { 0x102, data_chr_jms_hhl_jms_mdl, data_demo_ap_boss_hhl_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_ap_boss_hhl_jms_cls },
        { 0x208, data_chr_red_red_mdl, data_chr_red_red_anm, data_chr_red_red_kg1, NULL },
        { 0x200, data_chr_scu_scu_mdl, data_chr_scu_scu_anm, data_chr_scu_scu_kg1, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    static float vec[4][4] = {
        { 99846.15f, -2202.74f, -100531.95f, 0.0f },
        { 0.0f, -1.37444f, 0.0f, 0.0f },
        { 100242.95f, -1731.02f, -100380.95f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    static u_long128 *red_play_anim_adr;
    static u_long128 *scu_play_anim_adr;
    int ret;

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_ap_boss_ap_boss_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(chara_data, 0);
        red_play_anim_adr = (u_long128 *)(MemShare_gp_data_buf + ((FcGetFileSize(data_demo_ap_boss_ap_boss_dds) + 0x7FF) & ~0x7FF));
        scu_play_anim_adr = red_play_anim_adr + ((FcGetFileSize(data_demo_ap_boss_red_anm) + 0x7FF) & ~0x7FF);
        FcRead(data_demo_ap_boss_red_anm, red_play_anim_adr);
        FcRead(data_demo_ap_boss_scu_anm, scu_play_anim_adr);
        fsSync(0, -1);
        red_play_anim_adr = CharaDataAnimSetExtra(0x208, data_demo_ap_boss_red_anm, red_play_anim_adr, 0);
        scu_play_anim_adr = CharaDataAnimSetExtra(0x200, data_demo_ap_boss_scu_anm, scu_play_anim_adr, 0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        CharaAdminPlayableDisplay(0);
        ev_p_step = 0x16;
        ev_s_step = 0;
        break;
    case 0x16:
        ret = DramaDemoMain(&boss);
        shCharacterSetWeaponRED(shCharacterGetSubCharacter(0x208, 0), 1);
        if (ret) {
            ev_p_step = 0xD;
            ev_s_step = 0;
        }
        break;
    case 0xD:
        CharaDataDeleteOne(0x102);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        CharaDataAnimSetExtra(0x208, data_chr_red_red_anm, red_play_anim_adr, 1);
        CharaAdminReCreate(0x208, 8, 0, vec[0], vec[1], 6);
        CharaDataAnimSetExtra(0x200, data_chr_scu_scu_anm, scu_play_anim_adr, 1);
        CharaAdminReCreate(0x200, 9, 0, vec[2], vec[3], 0xD);
        Sh2sys.step[2] = 0xB;
        Sh2sys.step[3] = 0;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        ScreenEffectFadeStart(1, 1.0f);
        return 1;
    }
    return 0;
}

static int EvProgApartBossSiren(void) {
    SeCallPos(0x2716, 0.0f, NULL, 8);
    return 1;
}

static int EvProgApartBossEnd(void) {
    static float light[4] = { 0.3f, 0.9055f, 0.3f, 0.0f };
    static float clr[4] = { 2.2f, 2.2f, 2.4f, 0.0f };
    static struct Event_CamSetData cam_data = {
        { 99438.0f, -3961.0f, -100124.0f, 0.0f },
        { 99438.375f, -3960.13f, -100123.69f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        0.19635f,
    };

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        CharaAdminPlayableDisplay(0);
        shCharacter_Manage_Delete(NULL, 0x208, 8);
        SeCallPos(0x4A4F, 1.0f, (float[4]){ 98804.95f, -450.0f, -98578.195f, 0.0f }, 4);
        SeCallPos(0x3E95, 0.0f, (float[4]){ 98804.95f, -450.0f, -98578.195f, 0.0f }, 0xC);
        ev_timer = 0.0f;
        game_flag.flag[4] |= 0x80000;
        ev_p_step = 0x16;
        ev_s_step = 0;
    case 0x16:
        DramaDemoFade();
        vcSetEventCamParamRefView(cam_data.pos, NULL, cam_data.itr, NULL, cam_data.roll, 1);
        vcMoveAndSetCamera(0, 0, 0, 0, 0, 0, 0, 0);
        sh2gfw_Set_PallarelLight(light, clr, 2);
        ev_timer += shGetDT();
        if (ev_timer > 4.0f) {
            ev_p_step = 0xD;
            ev_s_step = 0;
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        game_flag.flag[4] |= 0x100000;
        Se2dManageDataTimer(0x2716, 1);
        return 1;
    }
    return 0;
}

static void EvRoomInit(void) {
    game_flag.flag[2] &= ~0x800000;
}

static void EvAllTimeFunc(void) {
    float vol;
    float time;
    int disp_ctrl_list[3];
    int work;

    disp_ctrl_list[0] = 0;
    switch (RoomNameJms()) {
    case 0x10:
        work = ((game_flag.flag[0] >> 25) & 1) ? -1 : 0;
        EvDispControlModelEntry(disp_ctrl_list, 0x19, work);
        break;
    case 0x20:
        work = ((game_flag.flag[0] >> 26) & 1) ? -1 : 0;
        EvDispControlModelEntry(disp_ctrl_list, 0x6E, work);
        break;
    case 0x21:
        if (!((game_flag.flag[4] >> 20) & 1)) {
            clAddDynamicWall(clActWallList_ap88);
        }
        if (!((game_flag.flag[4] >> 21) & 1) && ((game_flag.flag[4] >> 20) & 1)) {
            game_flag.flag[4] |= 0x200000;
            SeStop(0x2716);
        }
        time = Se2dManageDataTimer(0x2716, 0);
        if (!(time < 0.0f)) {
            if ((game_flag.flag[4] >> 20) & 1) {
                vol = 1.0f - time / 3.4f;
            } else {
                vol = time / 2.2f;
            }
            Se2dManageDataVolumeChange(0x2716, vol);
        }
        time = Se2dManageDataTimer(0x3E95, 0);
        if (!(time < 0.0f)) {
            vol = ((time < 4.5f) != 0) ? 2.0f * time : 1.0f - (time - 4.5f) / 1.5f;
            Se2dManageDataVolumeChange(0x3E95, vol);
        }
        break;
    }
    EvDispControlModelExec(disp_ctrl_list);
}
