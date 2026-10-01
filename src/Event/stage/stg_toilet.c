/*
 * stg_toilet.c: stage overlay for the prologue in the toilet.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_toilet).
 */
#include "sh2.h"
#include "sdk/libvu0.h"

static int EvProgPrologueInToilet(void);
static void EvStageInit(void);
static int EvBgmControl(void);
static float LinearTrim(float Yen, float Yst, float Xen, float Xst, float Parm);
static void Toilet_Dof_Filter(void);

static struct _AnimeInfo pjames_stage_anim[2] = {
    { 0x4E21, 0x14, 0x400, 0x400, 0x413, 0, 10 },
};

static unsigned char ev_pos[136] = {
    0x00, 0x40, 0x9C, 0xC6, 0x00, 0x80, 0x00, 0x10, 0xA4, 0x46, 0xD0, 0x5F, 0xD0, 0x5F, 0x00, 0xD8,
    0xA4, 0x46, 0x00, 0x80, 0x00, 0x60, 0x86, 0xC5, 0xD0, 0x5F, 0x00, 0x6C, 0x1D, 0x46, 0x00, 0x80,
    0x00, 0x8D, 0x24, 0x46, 0x08, 0x5F, 0xE2, 0x9F, 0x25, 0x46, 0x00, 0x80, 0x66, 0x9A, 0x0D, 0x46,
    0xBA, 0x5D, 0x53, 0xCB, 0xB7, 0xC7, 0xB3, 0xD3, 0xF5, 0xE3, 0x72, 0xC7, 0x3F, 0x5E, 0xDD, 0xD1,
    0x00, 0xD4, 0x94, 0x46, 0x00, 0x80, 0x30, 0x52, 0x8F, 0xC6, 0xF7, 0x5F, 0x00, 0xDE, 0x55, 0xC7,
    0x00, 0x80, 0x00, 0x4C, 0xE5, 0xC6, 0xD0, 0x5F, 0xED, 0x04, 0x00, 0x00, 0xEA, 0x04, 0x00, 0x00,
    0xEB, 0x04, 0x00, 0x00, 0x00, 0xA8, 0x93, 0x46, 0x00, 0x80, 0x00, 0x20, 0x61, 0x04, 0x00, 0x00,
    0x59, 0x04, 0x00, 0x00, 0xE6, 0x04, 0x00, 0x00, 0xE7, 0x04, 0x00, 0x00, 0x95, 0x04, 0x00, 0x00,
    0xE9, 0x04, 0x00, 0x00, 0x8A, 0x04, 0x00, 0x00,
};

static struct Event_List ev_list[10] = {
    { 0x00200000, 0x10000000, 0x30000000, 0x00004020 },
    { 0x00000000, 0x4000B000, 0x400E2060, 0x00000000 },
    { 0x00000000, 0x201A2000, 0xA0000000, 0x000104F8 },
    { 0x00000000, 0x20264000, 0x403250A0, 0x00000000 },
    { 0x00000000, 0x20403000, 0x404C40A0, 0x00400000 },
    { 0x00000000, 0x20583000, 0x404C40A0, 0x00400000 },
    { 0x00000000, 0x20643000, 0x60000000, 0x00008266 },
    { 0x00000000, 0x20703000, 0x60000000, 0x00008266 },
    { 0x00000000, 0x207C1000, 0x60000000, 0x00004267 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
};

static int (*ev_prog[2])(void) = {
    NULL,
    EvProgPrologueInToilet,
};

static struct Stage_GfwFunc gfw_func = {
    NULL,
    NULL,
    Toilet_Dof_Filter,
    NULL,
};

struct Stage_Data stage_toilet = {
    ev_list,
    ev_pos,
    ev_prog,
    NULL,
    NULL,
    NULL,
    EvStageInit,
    NULL,
    NULL,
    6,
    1,
    pjames_stage_anim,
    EvBgmControl,
    &gfw_func,
    NULL,
    NULL,
    0,
};

static int EvProgPrologueInToilet(void) {
    static short toilet_anim[] = { 0x432 };
    static struct DramaDemo_MessageTime toilet_msg[] = { { 0x14D, 0x1F8 }, { 0xFFFF, 0xFFFF } };
    static struct DramaDemo_PlayInfo toilet = {
        1, MemShare_gp_data_buf, toilet_anim, toilet_msg, 0, 0, NULL, 0xEAAF, 0.0f, 0.0f, 0.0f,
    };
    static struct CharaData_DemoList chara_data[3] = {
        { 0x103, data_chr_jms_hhh_jms_notex_mdl, data_demo_first_toilet_hhh_jms_anm, data_chr_jms_hhh_jms_kg1,
          data_demo_first_toilet_hhh_jms_cls },
        { 0x123, data_chr_jms_rhhh_jms_mdl, NULL, NULL, NULL },
    };
    float vec0[4];
    int ret;

    Sh2sys.main_status |= 0x80;
    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_first_toilet_first_toilet_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(chara_data, 0);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        ScreenEffectFadeStart(3, 0.0f);
        ev_p_step = 0x28;
        ev_s_step = 0;
    case 0x28:
        EvSubMovieReady(data_movie_toilet_pss, NULL, 0);
        if (!EvSubMovieStart(1)) {
            break;
        }
        ev_p_step = 0x2B;
        ev_s_step = 0;
        break;
    case 0x2B:
        if (movieGetLastExitStatus()) {
            ev_p_step = 0x2C;
            ev_s_step = 0;
        } else {
            ev_p_step = 0x2F;
            ev_s_step = 0;
        }
        break;
    case 0x2C:
        EvSubMovieEnd();
        ScreenEffectFadeStart(4, 1.0f);
        ev_p_step = 0x16;
        ev_s_step = 0;
    case 0x16:
        ret = DramaDemoMain(&toilet);
        if (shCharacterGetSubCharacter(0x123, 0) == NULL) {
            *(u_long128 *)vec0 = 0;
            CharaWorkCreate(0x123, 0, vec0, vec0, 0);
        }
        if (!ret) {
            break;
        }
        ev_p_step = 0xD;
        ev_s_step = 0;
        break;
    case 0x2F:
        EvSubMovieEnd();
        ScreenEffectFadeStart(5, 0.0f);
        ev_p_step = 0xD;
        ev_s_step = 0;
    case 0xD:
        shCharacter_Manage_Delete(NULL, 0x103, 0);
        CharaDataDeleteOne(0x123);
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        {
            shCharacterSetPosAfterDemo(sh2jms.player, (float[4]){ -19606.6f, 0.0f, 20429.55f, 0.0f }, -1.5707964f);
        }
        shCharacterPlayerModelToPlayable();
        Sh2sys.main_status &= ~0x80;
        return 1;
    }
    return 0;
}

static void EvStageInit(void) {
}

static int EvBgmControl(void) {
    if ((game_flag.flag[1] >> 4) & 1) {
        return 1;
    }
    if (!((game_flag.flag[1] >> 1) & 1)) {
        return 4;
    }
    return 0;
}

static float wvp[4] = { 0.0f, 0.0f, 450.0f, 1.0f };

/** Sends a GS packet that draws a black sprite over the screen with alpha KeyAlpha, at the screen
 * depth of the point wvp (450 in front of the camera).
 * @param KeyAlpha alpha of the sprite
 * @return that depth (screen z >> 4) */
int Kari_hisyakai(int KeyAlpha) {
    static union Q_WORDDATA qwd[32];
    int ivt[4];
    float wsm[4][4];
    int id;

    sceVu0CopyMatrix(wsm, cam0.view_screen);
    sceVu0RotTransPers(ivt, wsm, wvp, 1);
    qwd[0].ui32[0] = 0x10000007;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0x50000007;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8006;
    qwd[2] = shGs_AllEnv.Now_DrawEnv.frame_normal;
    qwd[3].ul64[1] = 0x47;
    qwd[3].ul64[0] = 0x58002;
    qwd[4].ul64[1] = 0x4E;
    qwd[4].ul64[0] = 0x3A0001C0;
    qwd[5].ul64[0] = 0x60;
    qwd[5].ul64[1] = 0x14;
    qwd[6].ul64[1] = 0x8;
    qwd[6].ul64[0] = 0x5;
    qwd[7].ul64[1] = 0x42;
    qwd[7].ul64[0] = 0xFF00000068;
    qwd[8].ui32[0] = 0x10000005;
    qwd[8].ui32[1] = 0;
    qwd[8].ui32[2] = 0;
    qwd[8].ui32[3] = 0x50000005;
    qwd[9].ul64[0] = 0x4023400000008001;
    qwd[9].ul64[1] = 0x5151;
    qwd[10].ui32[0] = 0;
    qwd[10].ui32[1] = 0;
    qwd[10].ui32[2] = 0;
    qwd[10].ui32[3] = KeyAlpha;
    qwd[11].ul64[0] = 0x0000700000007000;
    qwd[11].ul64[1] = ivt[2];
    qwd[12].ui32[0] = 0;
    qwd[12].ui32[1] = 0;
    qwd[12].ui32[2] = 0;
    qwd[12].ui32[3] = KeyAlpha;
    qwd[13].ul64[0] = 0x0000900000009000;
    qwd[13].ul64[1] = ivt[2];
    qwd[14].ul128 = 0;
    qwd[14].ui32[0] = 0x70000000;
    d1cSend(qwd);
    id = ivt[2] >> 4;
    return id;
}

static float LinearTrim(float Yen, float Yst, float Xen, float Xst, float Parm) {
    return Yst + (Yen - Yst) * (Parm - Xst) / (Xen - Xst);
}

/*
 * Matching: `toilet_dt = toilet_dt + shGetDT()` rather than `+=` (the same code): the spelling sets
 * the order of the LinearTrim() constant arguments (18.5f first, then 96.0f, 1.0f, 0.0f, as in the
 * original); see docs/toolchain.md, "Root cause". `demo_switch = demo_switch + 1` does the same.
 */
static void Toilet_Dof_Filter(void) {
    static int demo_switch;
    static float toilet_dt;
    float dal;
    struct FilterParams *pfp;

    if (DramaDemoNumber() == 1) {
        pfp = sh2gfw_Get_FilterCommandParams();
        if (demo_switch) {
            toilet_dt = toilet_dt + shGetDT();
        }
        demo_switch++;
        sh2gfw_Reset_FilterCommand();
        shGsFilterWork.GsFilterKind = 8;
        dal = LinearTrim(96.0f, 1.0f, 18.5f, 0.0f, toilet_dt);
        if (dal > 96.0f) {
            dal = 96.0f;
        }
        pfp->S1_iter = 3;
        pfp->S1_alpha = dal;
        pfp->S1_shift = 12;
        pfp->S1_baseIX = -4;
        pfp->S2_iter = 3;
        pfp->S2_alpha = 97.0f - dal;
        pfp->S2_shift = 12;
        pfp->S2_baseIX = -4;
        pfp->KeyAlpha = 8;
        pfp->DOF_ZDepth = Kari_hisyakai(pfp->KeyAlpha);
    } else {
        if (shGsFilterWork.GsFilterKind < 14) {
            sh2gfw_Reset_FilterCommand();
        }
        toilet_dt = 0.0f;
    }
}
