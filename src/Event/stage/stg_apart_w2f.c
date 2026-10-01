/*
 * stg_apart_w2f.c: stage overlay for the apartments, west building 2F: the "Dear Tim" note, Lyne's
 * key, the stair key, the wallet in the toilet and the safe.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_apart_w2f).
 */
#include "sh2.h"
#include "sdk/libvu0.h"

/* Next program step. Matching: the do/while leaves the original's nop before a case label it
 * falls into. */
#define EV_STEP(n) do { ev_p_step = (n); ev_s_step = 0; } while (0)
#define EV_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 0x1F)) & 1)

static int EvProgReadDearTim(void);
static int EvProgUseLyneKey(void);
static int EvProgGetApartStairKey(void);
static int EvProgUseApartStairKey(void);
static int EvProgWalletInToilet(void);
static int EvProgWalletOutToilet(void);
static void EvProgSubWalletMemo(int x);
static int EvProgOpenSafe(void);
static void EvProgSubSafaLockDraw(void);
static void EvProgSubSafeLockRotate(void);
static void EvAllTimeFunc(void);

/* float to 12.4 fixed point on VU0. Matching: a local copy of asm_helpers.h's ftoi4; with that
 * header included, Parallel_Trim's data references come out different. */
inline int ftoi4(float f) {
    int r;

    asm {
        mfc1     r, f
        qmtc2.ni r, vf4
        vftoi4.x vf4, vf4
        qmfc2.ni r, vf4
    }
    return r;
}

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

static unsigned char ev_pos[374] = {
    0, 0xEC, 0xAE, 0x46, 0, 0x80, 0, 0xC0, 0xA8, 0xC6, 0x40, 0x62, 0x7A, 0xE5, 0x9C, 0x47,
    0x84, 0xE7, 0x20, 0x67, 0xA3, 0xC7, 0, 0x88, 0x90, 0x46, 0, 0x80, 0, 0xA8, 0xAC, 0xC6,
    0xDC, 0x68, 0x8E, 0x46, 0, 0x80, 0xFC, 0xFF, 0xAD, 0xC6, 0, 0, 0xCB, 0x59, 0x7F, 0x52,
    0x8D, 0x5B, 1, 0x59, 0x8D, 0x5B, 0x6D, 0x5A, 0xCB, 0x59, 0x6D, 0x5A, 0, 0x80, 0x33, 0,
    0xA3, 0x46, 0, 0x80, 0x23, 0x18, 0x9A, 0xC6, 0x9E, 0x59, 0xD9, 0x58, 0x80, 0x80, 0xC2, 0x47,
    0, 0x80, 0xC8, 0xA9, 0x3E, 0xC6, 0xD0, 0x5F, 0x1A, 0x50, 0xAD, 0x46, 0, 0x80, 0x54, 0x41,
    0xC2, 0xC7, 0xD0, 0x5F, 0x2C, 0xA0, 0x99, 0x46, 0, 0x80, 0xB5, 0xC7, 0xBF, 0xC7, 0x2B, 0xD2,
    0x49, 0xDC, 0, 0x80, 0x99, 0x46, 0, 0x80, 0, 0x75, 0xBC, 0xC7, 0xFF, 0x5A, 0xD9, 0xA5,
    0x9D, 0x46, 0, 0x80, 0x8E, 0x75, 0xBC, 0xC7, 0x6D, 0x5E, 0xED, 0xD2, 0xA1, 0x46, 0, 0x80,
    8, 0x10, 0xBE, 0xC7, 0x99, 0x5E, 0x9A, 0x9F, 0x8C, 0x46, 0, 0x80, 0, 0xC, 0xC6, 0xC7,
    0x1A, 0x6C, 0x78, 0x69, 0, 0xA0, 0x8C, 0x46, 0, 0x80, 0, 0x5D, 0xC0, 0xC7, 0x4B, 0x69,
    0xD0, 0x67, 0xEC, 0xD8, 0x93, 0x46, 0, 0x80, 0x4A, 9, 0xC6, 0xC7, 0xDB, 0x5F, 0xFE, 0xE3,
    0x98, 0x46, 0, 0x80, 0x1A, 9, 0xC6, 0xC7, 0x2D, 0x5F, 0x8D, 0xE3, 0xC4, 0x47, 0, 0x80,
    0xCA, 0x89, 0xC4, 0xC5, 0x5D, 0x5F, 0x72, 0x30, 0xC0, 0x47, 0xA4, 0xE6, 0xDE, 0xE5, 0xC0, 0xC7,
    0x5D, 0x5F, 0x80, 0x80, 0xC2, 0x47, 0, 0x80, 0xE3, 0x14, 0xBD, 0xC6, 0xD0, 0x5F, 0x80, 0x80,
    0xC2, 0x47, 0, 0x80, 0xE5, 0x14, 0xEF, 0xC6, 0xD0, 0x5F, 0x7D, 0xE7, 0xC4, 0x47, 0, 0x80,
    0xF4, 0xF2, 0xD9, 0xC6, 0xD0, 0x5F, 0x4A, 0xB4, 0x89, 0x46, 0, 0x80, 0xFE, 0xBC, 0x8D, 0xC6,
    0xD0, 0x5F, 0, 0x80, 0x89, 0x46, 0, 0x80, 0, 0x40, 0x9C, 0xC6, 0x1A, 0x6C, 0xB0, 0x68,
    0x7D, 0xE7, 0xC4, 0x47, 0, 0x80, 0xF5, 0x72, 0xB4, 0xC6, 0xD0, 0x5F, 0x7D, 0xE7, 0xC4, 0x47,
    0, 0x80, 0xF5, 0xF2, 0x8E, 0xC6, 0xD0, 0x5F, 2, 0x88, 0xC2, 0x47, 0, 0x80, 5, 0x88,
    0x5B, 0xC6, 0xB0, 0x64, 0x40, 0x66, 0x7D, 0xE7, 0xC4, 0x47, 0, 0x80, 0xB7, 0x66, 0x14, 0xC6,
    0xD0, 0x5F, 0x80, 0x80, 0xC2, 0x47, 0, 0x80, 0x8E, 0x53, 0xB5, 0xC5, 0xD0, 0x5F, 0x5A, 0x8D,
    0xC2, 0x47, 0, 0x80, 0x31, 0x82, 0x8D, 0xC6, 0xDF, 0x5F, 0x40, 0x46, 0xA, 0x48, 0xA4, 0xE6,
    0xF8, 0xDC, 0x94, 0xC6, 0xDF, 0x5F,
};

static struct Event_List ev_list[33] = {
    { 0, 0x20004000, 0x400C01A0, 0x65 },
    { 0x80660000, 0x20160004, 0x30000000, 0x18000 },
    { 0x80660000, 0x20209004, 0x60000000, 0x14000 },
    { 0x660000, 0x20209004, 0x30000000, 0x14000 },
    { 0x6C0000, 0x203E5004, 0x30000000, 0x1C25F },
    { 0, 0xA04C31CC, 0x30000000, 0x8071 },
    { 0x80710000, 0x204C3000, 0x40584000, 0x4005B0 },
    { 0x80710000, 0x20584000, 0x404C3000, 0x4005B0 },
    { 0, 0x204C3004, 0x30000000, 0x406F },
    { 0, 0x204C3000, 0x80000000, 0x4005B1 },
    { 0x720000, 0x20645004, 0x30000000, 0xC072 },
    { 0, 0x20722000, 0xA0000000, 0x205BB },
    { 0, 0x207E2000, 0x90000000, 0x7FC000 },
    { 0, 0x208A4000, 0x90000000, 0x7FC000 },
    { 0x5BA0000, 0x4096B004, 0x10000000, 0x5BA },
    { 0x5B90000, 0x40A4B004, 0x10000000, 0x5B9 },
    { 0, 0x20B21000, 0x90000000, 0x7F8000 },
    { 0, 0x20BE1000, 0x90000000, 0x7F8000 },
    { 0, 0xA0CA41DC, 0x30000000, 0x10073 },
    { 0x80730000, 0x20CA4000, 0x40D63180, 0x10005B2 },
    { 0, 0x20CA4000, 0x80000000, 0x10005B3 },
    { 0, 0x20E23000, 0x90000000, 0x7F85A8 },
    { 0, 0x20EE3000, 0x90000000, 0x7FC5A9 },
    { 0, 0x20FA4000, 0x41063000, 0x4005AA },
    { 0, 0x21063000, 0x40FA4000, 0x4005AA },
    { 0x5B80000, 0x4112B004, 0x10000000, 0x5B8 },
    { 0, 0x21204000, 0x90000000, 0x7FC5AB },
    { 0, 0x212C4000, 0x90000000, 0x7FC5AC },
    { 0x5AD0000, 0x4138B004, 0x10000000, 0x5AD },
    { 0, 0x21464000, 0x90000000, 0x7FC5AE },
    { 0, 0x21523000, 0x90000000, 0x7FC5AF },
    { 0, 0x215E3000, 0x416A4180, 0x10005B4 },
    { 0, 0, 0, 0 },
};

static struct Item_List gi_list[7] = {
    { 21052.99f, -19645.0f, 0xDD40, 0x38E4, 0x20200293 },
    { 20922.0f, -19718.0f, 0xDD40, 0x3BC2, 0x20200295 },
    { 21003.99f, -19674.0f, 0xDD40, 0x383D, 0x20200297 },
    { 20967.99f, -19724.0f, 0xDD40, 0x8000, 0x20200299 },
    { 17902.0f, -20178.99f, 0xDFD8, 0x8000, 0xC0000325 },
    { 18516.5f, -96834.01f, 0xDC7C, 0x8000, 0xA0000327 },
    { 0.0f, 0.0f, 0, 0, 0xE0000000 },
};

/* sh2gfw_OV_CharaDrawHook.h */
static float LinearTrim(float Yen, float Yst, float Xen, float Xst, float Parm) {
    return Yst + (Yen - Yst) * (Parm - Xst) / (Xen - Xst);
}

static void Parallel_Trim(void *sp, float *ambient) {
    int i;
    int *mp;

    mp = Get_NowMapId();
    switch (*mp) {
    case 0x90048: {
        float fac;
        struct SubCharacter *scp;
        float (*lcm)[4];

        lcm = light_work.lcms[0];
        for (i = 0; i < 4; i++) {
            if (lcm[i][0] == 2.0f) {
                break;
            }
        }
        if (i == 4) {
            return;
        }
        scp = sp;
        if (scp->pos.x >= 18000.0f) {
            if (scp->pos.z <= -98500.0f) {
                fac = LinearTrim(2.0f, 0.1f, 18000.0f, 22000.0f, scp->pos.x);
            } else {
                fac = LinearTrim(2.0f, 0.1f, 18000.0f, 20600.0f, scp->pos.x);
            }
            lcm[i][0] = lcm[i][1] = lcm[i][2] = fac;
        }
        break;
    }
    }
    if (DramaDemoNumber() == 6) {
        struct SubCharacter *scp;
        float fac;
        float ftmp;

        scp = sp;
        if (scp->pos.z >= 49000.0f) {
            fac = LinearTrim(0.2f, 1.0f, 53000.0f, 49000.0f, scp->pos.z);
            ftmp = ambient[3];
            sceVu0ScaleVector(ambient, ambient, fac);
            ambient[3] = ftmp;
        }
    }
}

/*
 * Matching: fitted stand-in for double code (docs/stand-ins.md): later functions use a2 for
 * temporaries. Here, not before EvProgSubSafaLockDraw where the original's line table leaves no
 * room: sh2gfw_OV_CharaDrawHook.h (whose LinearTrim and Parallel_Trim are above) has more lines
 * than those two, and its dead-stripped functions were compiled at this point.
 */
STRIPPED_DOUBLE_CODE()

static int (*ev_prog[8])(void) = {
    NULL, EvProgReadDearTim,
    EvProgUseLyneKey,
    EvProgGetApartStairKey,
    EvProgUseApartStairKey,
    EvProgWalletInToilet,
    EvProgWalletOutToilet,
    EvProgOpenSafe,
};

static struct Model_List mdl_list[2] = {
    { 1840, 0, 114, 0, { 19400.0f, -260.47495f, -98336.04f, 0.0f }, { -1.570796f, 0.0f, -0.650159f, 0.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

static struct Enemy_List en_list[5] = {
    { 512, 74, 100200, -31300, 0, 0, 0, 0 },
    { 513, 75, 100000, -25700, 0, 0, 0, 3 },
    { 513, 76, 100000, -7300, 0, 6433, 0, 1 },
    { 513, 77, 100500, -8300, 0, -6433, 0, 3 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};

static struct Stage_GfwFunc SpecialDrawFunctions = { NULL, NULL, NULL, (void (*)(void))Parallel_Trim };

struct Stage_Data stage_apart_w2f = {
    ev_list,
    ev_pos,
    ev_prog,
    gi_list,
    mdl_list,
    en_list,
    NULL,
    NULL,
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

static int EvProgReadDearTim(void) {
    static float door_pos[4] = { 99584.99f, -500.0f, -12452.445f, 1.0f };

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        game_flag.flag[45] |= 0x20000;
        SeCallPos(0x4A3C, 1.0f, door_pos, 0);
        fontMessageNum(msg_station, 4);
        ev_p_step = 0x1E;
        ev_s_step = 0;
        break;
    case 0x1E:
        if (fontGetStatus() == -2 || ev_cancel) {
            fontClear();
            if (!ev_cancel) {
                ev_p_step = 0x1F;
                ev_s_step = 0;
            } else {
                ev_prog_flag_set = 0;
                ev_p_step = 0xD;
                ev_s_step = 0;
            }
        }
        break;
    case 0x1F:
        if (EvSubMessage(0)) {
            ev_p_step = 0x20;
            ev_s_step = 0;
        }
        break;
    case 0x20:
        if (EvSubMessage(1)) {
            ev_p_step = 0xD;
            ev_s_step = 0;
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgUseLyneKey(void) {
    return EvSubItemUse0(0x1C, 9, 0x4A51, 0, (float[4]){ 99585.0f, -500.0f, -12452.445f }, 1);
}

static int EvProgGetApartStairKey(void) {
    return EvSubItemGetAndAnim(0x1D, 8);
}

static int EvProgUseApartStairKey(void) {
    return EvSubItemUse0(0x1D, 0xA, 0x4A51, 0, (float[4]){ 100807.1f, -500.0f, -6053.4854f }, 1);
}

static int EvProgWalletInToilet(void) {
    static short dirty_anim[2] = { 1170, 10096 };
    static struct DramaDemo_PlayInfo dirty = { 0x11, MemShare_gp_data_buf, dirty_anim, NULL, 0, 0, NULL, 0xEA8A, 44.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[3] = {
        { 0x102, data_chr_jms_hhl_jms_mdl, data_demo_kitanai_hhl_jms_anm, NULL, NULL },
        { 0x416, data_chr_item_i_purse_mdl, data_demo_kitanai_i_purse_anm, NULL, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        FcRead(data_demo_kitanai_kitanai_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(chara_data, 1);
        ev_p_step = 0x1E;
        ev_s_step = 0;
    case 0x1E:
        if (!EvSubQuestion(2)) {
            break;
        }
        if (fontGetStatus() == 0) {
            ev_p_step = 2;
            ev_s_step = 0;
        } else {
            CharaDataLoadCancel(chara_data);
            ev_p_step = 0xD;
            ev_s_step = 0;
        }
        break;
    case 2:
        if (fsSync(1, -1) < 0) {
            break;
        }
        CharaDataLoadDemo(chara_data, 0);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        ev_p_step = 0x16;
        ev_s_step = 0;
    case 0x16:
        DramaDemoMain(&dirty);
        if (demo_frame > total_demo_frame - 30.0f) {
            ScreenEffectFadeStart(2, 1.0f);
            if (ScreenEffectFadeCheck()) {
                EV_STEP(0x1F);
            }
        }
        if (shPadTrigger(0, key_config.skip)) {
            ScreenEffectFadeStart(3, 0.5f);
            ev_p_step = 0x1F;
            ev_s_step = 0;
        }
        break;
    case 0x1F:
        EvProgSubWalletMemo(0);
        CharaDataDeleteOne(0x102);
        CharaDataDeleteOne(0x416);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        CharaAdminPlayableDisplay(1);
        ScreenEffectFadeStart(5, 0.0f);
        ev_p_step = 0x20;
        ev_s_step = 0;
    case 0x20:
        EvSubPictureStart();
        if (fontGetStatus() == -1) {
            break;
        }
        fontClear();
        game_flag.flag[3] |= 0x40;
        vcReturnPreAutoCamWork(1);
        ScreenEffectFadeStart(3, 0.0f);
        ScreenEffectFadeStart(4, 0.0f);
        ev_p_step = 0xD;
        ev_s_step = 0;
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgWalletOutToilet(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x4E21);
        ev_p_step = 0x1B;
        ev_s_step = 0;
        break;
    case 0x1B:
        if (PlayerEventAnimeSuccessFrame()) {
            shCharacterAnimePause(sh2jms.player);
            ev_p_step = 0xA;
            ev_s_step = 0;
        }
        break;
    case 0xA:
        EvProgSubWalletMemo(1);
        ev_p_step = 0x10;
        ev_s_step = 0;
    case 0x10:
        if (fontGetStatus() == -2 || ev_cancel) {
            fontClear();
            shCharacterAnimeRestart(sh2jms.player);
            ev_p_step = 0x1C;
            ev_s_step = 0;
        }
        break;
    case 0x1C:
        if (shCharacterAnimeIsEnd(sh2jms.player) || ev_cancel) {
            ev_p_step = 0xD;
            ev_s_step = 0;
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static void EvProgSubWalletMemo(int x) {
    unsigned char c_work[4];
    int work;
    int i;

    switch (playing.riddle_level) {
    case 0:
        for (i = 0; i < 4; i++) {
            if (i == 0) {
                work = game_flag.safe[0] + 1;
            } else if (i == 2) {
                work = (game_flag.safe[i] - game_flag.safe[i - 1] + 20) % 20;
            } else {
                work = (game_flag.safe[i - 1] - game_flag.safe[i] + 20) % 20;
            }
            c_work[0] = work / 10 + '0';
            c_work[1] = work % 10 + '0';
            c_work[2] = 0;
            fontSetMes(i, dicSetStr(c_work));
        }
        break;
    case 1:
        for (i = 0; i < 4; i++) {
            c_work[0] = (game_flag.safe[i] + 1) / 10 + '0';
            c_work[1] = (game_flag.safe[i] + 1) % 10 + '0';
            c_work[2] = 0;
            fontSetMes(i, dicSetStr(c_work));
        }
        break;
    case 2:
        for (i = 0; i < 4; i++) {
            if (game_flag.safe[i] + 1 >= 20) {
                c_work[0] = 'X';
                c_work[1] = 'X';
                c_work[2] = 0;
            } else if (game_flag.safe[i] + 1 >= 10) {
                if (i & 1) {
                    c_work[0] = 'X';
                    c_work[1] = (game_flag.safe[i] + 1) % 10 + '0';
                    c_work[2] = 0;
                } else {
                    c_work[0] = 'V';
                    c_work[1] = 'V';
                    c_work[2] = (game_flag.safe[i] + 1) % 10 + '0';
                    c_work[3] = 0;
                }
            } else {
                c_work[0] = game_flag.safe[i] + 1 + '0';
                c_work[1] = 0;
            }
            fontSetMes(i, dicSetStr(c_work));
        }
        break;
    default:
        for (i = 0; i < 4; i++) {
            if (game_flag.safe[i] + 1 >= 10) {
                c_work[0] = game_flag.safe[i] + 1 - 10 + 'a';
                c_work[2] = 0;
            } else {
                c_work[0] = game_flag.safe[i] + 1 + '0';
                c_work[1] = 0;
            }
            fontSetMes(i, dicSetStr(c_work));
        }
        break;
    }
    if (x) {
        fontMessageNum(msg_buffer, 4);
    } else {
        fontMessageNum(msg_buffer, 3);
    }
}

static int EvProgOpenSafe(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        ev_p_step = 2;
        ev_s_step = 0;
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_p_safe_close_tex, data_pic_apt_p_safe_close2_tex)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        ev_timer = 0.0f;
        ev_p_step = 3;
        ev_s_step = 0;
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgSubSafaLockDraw();
        EvSubPictureEnd();
        if (ScreenEffectFadeCheck()) {
            ev_p_step = 8;
            ev_s_step = 0;
        }
        break;
    case 8:
        EvProgSubSafeLockRotate();
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgSubSafaLockDraw();
        EvSubPictureEnd();
        if (EV_FLAG(0x6C)) {
            ev_timer += shGetDT();
            if (ev_timer > 0.2f) {
                ScreenEffectFadeStart(1, 0.0f);
                ev_p_step = 4;
                ev_s_step = 0;
            }
        } else if (shPadTrigger(0, key_config.cancel)) {
            ScreenEffectFadeStart(1, 0.0f);
            ev_p_step = 4;
            ev_s_step = 0;
        }
        break;
    case 4:
        if (ScreenEffectFadeCheck()) {
            ev_p_step = 0xD;
            ev_s_step = 0;
        }
        break;
    case 0xD:
        if (EV_FLAG(0x6C)) {
            SeCallPos(0x3E88, 1.0f, (float[4]){ 20878.2f, -410.0f, -19711.13f }, 0);
        }
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        ScreenEffectFadeStart(4, 0.0f);
        return 1;
    }
    return 0;
}

static void EvProgSubSafaLockDraw(void) {
    struct PicDraw_Data pic;
    float cosrot;
    float sinrot;
    float pos[4][2];
    float rot;
    int lock;
    int i;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.a = 0x80;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    lock = 0;
    for (i = 0; i < 5; i++) {
        if (EV_FLAG(i + 0x67)) {
            lock += 1 << i;
        }
    }
    rot = (lock + 1) * 2.0 * 3.1415927f / 20.0;
    sinrot = shSinF(-rot);
    cosrot = shCosF(-rot);
    pos[0][0] = 0.8f * (-80.0f * cosrot - -64.0f * sinrot);
    pos[0][1] = -80.0f * sinrot + -64.0f * cosrot;
    pos[1][0] = 0.8f * (80.0f * cosrot - -64.0f * sinrot);
    pos[1][1] = 80.0f * sinrot + -64.0f * cosrot;
    pos[2][0] = 0.8f * (-80.0f * cosrot - 64.0f * sinrot);
    pos[2][1] = -80.0f * sinrot + 64.0f * cosrot;
    pos[3][0] = 0.8f * (80.0f * cosrot - 64.0f * sinrot);
    pos[3][1] = 80.0f * sinrot + 64.0f * cosrot;
    pic.x0 = ftoi4(pos[0][0] - 14.5f);
    pic.y0 = ftoi4(pos[0][1] - 4.75f);
    pic.x1 = ftoi4(pos[1][0] - 14.5f);
    pic.y1 = ftoi4(pos[1][1] - 4.75f);
    pic.status |= 2;
    pic.x2 = ftoi4(pos[2][0] - 14.5f);
    pic.y2 = ftoi4(pos[2][1] - 4.75f);
    pic.x3 = ftoi4(pos[3][0] - 14.5f);
    pic.y3 = ftoi4(pos[3][1] - 4.75f);
    pic.status |= 0x80;
    pic.us0 = 0;
    pic.vt0 = 0;
    pic.us1 = 0x7F0;
    pic.vt1 = 0x7F0;
    pic.status |= 4;
    pic.otp = 3;
    PictureDraw(&pic);
}

static void EvProgSubSafeLockRotate(void) {
    int lock;
    int i;

    if (EV_FLAG(0x6C)) {
        return;
    }
    lock = 0;
    for (i = 0; i < 5; i++) {
        if (EV_FLAG(i + 0x67)) {
            lock += 1 << i;
        }
    }
    if (shPadRepeat(0, 0x500)) {
        SeCall(0x3E87, 1.0f, 0);
        lock--;
        if (lock < 0) {
            lock = 19;
        }
        if (game_flag.rotate[2] >= 0) {
            if (lock == game_flag.safe[3]) {
                game_flag.flag[3] |= 0x1000;
            }
        } else if (game_flag.rotate[1] >= 0) {
            for (i = 0; i < 4; i++) {
                game_flag.rotate[i] = -1;
            }
        } else if (game_flag.rotate[0] >= 0) {
            if (lock == game_flag.safe[1]) {
                game_flag.rotate[1] = lock;
            }
        } else if (lock == game_flag.safe[0]) {
            game_flag.rotate[0] = lock;
        }
    } else if (shPadRepeat(0, 0xA00)) {
        SeCall(0x3E87, 1.0f, 0);
        lock++;
        if (lock >= 20) {
            lock = 0;
        }
        if (game_flag.rotate[2] >= 0) {
            for (i = 0; i < 4; i++) {
                game_flag.rotate[i] = -1;
            }
        } else if (game_flag.rotate[1] >= 0) {
            if (lock == game_flag.safe[2]) {
                game_flag.rotate[2] = lock;
            }
        } else if (game_flag.rotate[0] >= 0) {
            for (i = 0; i < 4; i++) {
                game_flag.rotate[i] = -1;
            }
        } else if (lock == game_flag.safe[0]) {
            game_flag.rotate[0] = lock;
        }
    }
    for (i = 0; i < 5; i++) {
        if (lock & (1 << i)) {
            game_flag.flag[(i + 0x67) >> 5] |= 1 << ((i + 0x67) & 0x1F);
        } else {
            game_flag.flag[(i + 0x67) >> 5] &= ~(1 << ((i + 0x67) & 0x1F));
        }
    }
}

static void EvAllTimeFunc(void) {
    int disp_ctrl_list[5];
    int room;

    disp_ctrl_list[0] = 0;
    room = RoomNameJms();
    if (room == 0x27) {
        EvDispControlModelEntry(disp_ctrl_list, 0x41, -1);
        if (EV_FLAG(0x66)) {
            EvDispControlModelEntry(disp_ctrl_list, 0x41, 0);
        }
        if (EV_FLAG(0x6C)) {
            EvDispControlModelEntry(disp_ctrl_list, 0x43, 0);
        } else {
            EvDispControlModelEntry(disp_ctrl_list, 0x43, 1);
        }
    }
    EvDispControlModelExec(disp_ctrl_list);
}
