/*
 * sh2gfw_drawloop_main.c: the per-frame draw loop of the graphics framework.
 */

#include "sh2.h"
#include "sdk/libvu0.h"

static void Exec_PreDraw_OVfunc(void);
static void Noise_and_FontDraw(int loopmode);
static void GSFilterExec4NextLoop(void);
static void Calc_Set_Matrix(void);
static void gs_loppstart_init(void);
static void kari_setdmatag_and_draw(void);
static void Draw_Character_And_Shadow(void);
static void Spack_All_Draw(void);
static int FOG_ROOM(void);
static int DrawSpecialPolyforDemo(void);
static int Check_DrawOrder(void);

static struct sh2gfw_CAMERA Shadow_Mini_Camera;
static unsigned long res;
static unsigned long res2;

/**
 * Sends the default GS environment to each of the three frame buffers in turn and waits for it;
 * also resets the debug font packet and the performance samples.
 */
void all_Frame_Buffer_Clear(void) {
    Q_WORDDATA *kick;

    sh2gfw_init_GSPACKMAN(&GSCTL_man);
    sh2gfw_RotateTBuff(&shGs_AllEnv);
    sh2gfw_setREF_gsctl(shGs_AllEnv.DefaultEnv);
    sh2gfw_InclimentLoopCounter(&shGs_AllEnv);
    sh2gfw_RotateTBuff(&shGs_AllEnv);
    sh2gfw_setREF_gsctl(shGs_AllEnv.DefaultEnv);
    sh2gfw_InclimentLoopCounter(&shGs_AllEnv);
    sh2gfw_RotateTBuff(&shGs_AllEnv);
    sh2gfw_setREF_gsctl(shGs_AllEnv.DefaultEnv);
    kick = sh2gfw_setEND_gsctl();
    d1cSend(kick);
    d1sSync(0, -1);
    shDBG_font_init();
    sh2gfw_init_Perf();
}

/** Start of a frame's drawing: resets the timers and the frame's packet and texture managers. */
void DrawLopp_Pre(void) {
    *T0_COUNT = 0;
    mct.ui32[0] = 0;
    gs_loppstart_init();
    sh2gfw_init_TexTrans_Manage_Table();
    d2tscClearSlots();
    sh2gfw_set_LoopDrawEnv(&shGs_AllEnv);
    spkResetOT();
    sh2gfw_Init_spkTexManage();
    shDBG_font_init();
    sh2gfw_init_Perf();
    mfontClear();
    sh2shd_reset_shadow();
}

/** Ends a 2D-only frame: sends the font and the GS filter, and waits for both DMA servers. */
void kari_drawloop_main_2dSYNC(void) {
    fjFontDrawExecVif1();
    GSFilterExec4NextLoop();
    d2sSync(0, -1);
    d1sSync(0, -1);
}

/** Ends a 3D frame: sends the GS filter, waits for both DMA servers and records the frame time. */
void draw_main_3dSYNC(void) {
    void kari_DBG_print_junbi(); /* Matching: an unprototyped block-scope declaration, as in the original. */

    GSFilterExec4NextLoop();
    d2sSync(0, -1);
    d1sSync(0, -1);
    res = *T0_COUNT;
    kari_DBG_print_junbi(&res, &res2);
}

/**
 * End of a frame: waits for both DMA servers, then for the next vblank (two when no vblank has
 * passed during the frame), and records the frame's vblank count.
 */
void DrawLopp_Post(void) {
    d2sSync(0, -1);
    d1sSync(0, -1);
    if (mct.ui32[0] == 0) {
        shSyncVStart(0);
        shSyncVStart(0);
    } else {
        shSyncVStart(0);
    }
    mct.ui32[3]++;
    sh2gfw_InclimentLoopCounter(&shGs_AllEnv);
    mct.ui32[1] = mct.ui32[0];
}

/** Returns the number of vblanks the last frame took. */
unsigned int Get_FrameRate(void) {
    return mct.ui32[1];
}

static void Exec_PreDraw_OVfunc(void) {
    if (stage && stage->gfw_func && stage->gfw_func->PreDraw) {
        stage->gfw_func->PreDraw();
    }
}

/**
 * Draws a 3D frame: camera, matrices, lights, background, characters and shadows, effects, then the
 * noise and font layer.
 */
void draw_main(void) {
    sh2gfw_Store_Perf2(*T0_COUNT, 8);
    sh2gfw_test_shcamera_main();
    Exec_PreDraw_OVfunc();
    *T1_COUNT = 0;
    Calc_Set_Matrix();
    sh2gfw_Set_JmsSpot_OnOrOff();
    Programed_Light_Set();
    kari_setdmatag_and_draw();
    Draw_Character_And_Shadow();
    fjMoveEffect();
    HH_Effect_Object_SPK_Texture_Post();
    Spack_All_Draw();
    Exec_PostDraw_OVfunc();
    if (!Check_Filter_Soft()) {
        Noise_and_FontDraw(0);
    }
    res2 = *T1_COUNT;
}

/* Matching: qwd is a Q_WORDDATA * (the 16-byte-aligned typedef) to get its 0x20(sp) slot. */
static void Noise_and_FontDraw(int loopmode) {
    Q_WORDDATA *qwd;

    if (loopmode) {
        d1sSync(0, -1);
    }
    fjFontDrawExec();
    if (Env_ctl.NoiseCondition.uc8[4]) {
        qwd = Noise_Packet;
        if (Env_ctl.light_mode == 1) {
            sh2gfw_SendDraw_Noise(&qwd, Env_ctl.NoiseCondition.uc8[2], Env_ctl.NoiseCondition.uc8[0], 1, loopmode);
        } else if (Env_ctl.light_mode == 2) {
            sh2gfw_SendDraw_Noise(&qwd, Env_ctl.NoiseCondition.uc8[2], Env_ctl.NoiseCondition.uc8[0], 1, loopmode);
        }
        d1cSend(Noise_Packet);
    }
    if (loopmode && DramaDemoNumber()) {
        void DemoFadeDraw2(); /* Matching: an unprototyped block-scope declaration, as in the original. */

        DemoFadeDraw2(0, 0, 0, 0);
    }
}

static void GSFilterExec4NextLoop(void) {
    Q_WORDDATA *qwd;
    Q_WORDDATA *SendAddr;
    int idd[3] = {2, 0, 1};
    int filtercommand;
    void Make_Filter_Packet(); /* Matching: an unprototyped block-scope declaration, as in the original. */

    qwd = &GSENV_CTL_PACK[0x100];
    filtercommand = sh2gfw_Get_FilterCommand();
    sh2gfw_RotateNext2(&shGs_AllEnv);
    sh2gfw_setCALL_gsctl(qwd);
    SendAddr = sh2gfw_setEND_gsctl();
    Make_Filter_Packet(qwd, 0);
    d1cSend(SendAddr);
    if (Check_Filter_Soft() == 1) {
        sh2gfw_InclimentLoopCounter(&shGs_AllEnv);
        Noise_and_FontDraw(1);
        sh2gfw_DeclimentLoopCounter(&shGs_AllEnv);
    }
}

/*
 * Matching: fitted stand-in for float code (docs/stand-ins.md; found by tools/constcount.py): it gives the
 * SetVu0ViewScreenClipMatrix calls the original's argument order (farz stack arg, ax, ay).
 */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f; }
static void Calc_Set_Matrix(void) {
    void sh2gfw_set_GSREGS2Vu(); /* Matching: an unprototyped block-scope declaration, as in the original. */

    sceVu0CopyMatrix(cam0.world_view, VbWvsMatrix.wvm);
    sh2gfw_Set_EyeDir();
    SetVu0ViewScreenClipMatrix(cam0.view_screen, cam0.view_clip, cam0.clip_screen, cam0.clip_volume,
                               VbScreenInfo.scr_z, VbScreenInfo.sx / 587.2727f, VbScreenInfo.sy / 484.0f,
                               VbScreenInfo.cx, VbScreenInfo.cy, VbScreenInfo.zmin, VbScreenInfo.zmax,
                               VbScreenInfo.nearz, VbScreenInfo.farz);
    sceVu0MulMatrix(cam0.world_clip, cam0.view_clip, VbWvsMatrix.wvm);
    sh2gfw_set_objclip_matrix();
    sceVu0CopyMatrix(cam0.world_screen, VbWvsMatrix.wsm);
    sceVu0CopyMatrix(VU1_PARMS.world_screen, cam0.world_screen);
    sceVu0CopyMatrix(VU1_PARMS.world_clip, cam0.world_clip);
    sceVu0CopyMatrix(VU1_PARMS.clip_screen, cam0.clip_screen);
    sh2gfw_Set_DepthFogFAFB();
    sceVu0CopyVector(Shadow_Mini_Camera.clip_volume, cam0.clip_volume);
    sceVu0CopyMatrix(Shadow_Mini_Camera.world_view, VbWvsMatrix.wvm);
    SetVu0ViewScreenClipMatrix(Shadow_Mini_Camera.view_screen, Shadow_Mini_Camera.view_clip,
                               Shadow_Mini_Camera.clip_screen, Shadow_Mini_Camera.clip_volume,
                               VbScreenInfo.scr_z, 0.75f * VbScreenInfo.sx / 587.2727f,
                               0.75f * VbScreenInfo.sy / 484.0f, VbScreenInfo.cx, VbScreenInfo.cy,
                               VbScreenInfo.zmin, VbScreenInfo.zmax, VbScreenInfo.nearz, VbScreenInfo.farz);
    sceVu0MulMatrix(Shadow_Mini_Camera.world_screen, Shadow_Mini_Camera.view_screen, Shadow_Mini_Camera.world_view);
    sceVu0MulMatrix(Shadow_Mini_Camera.world_clip, Shadow_Mini_Camera.view_clip, VbWvsMatrix.wvm);
    sh2gfw_Store_SpotLight();
    sh2gfw_set_GSREGS2Vu(&VU1_PARMS, &shGs_AllEnv);
}

static void gs_loppstart_init(void) {
    Q_WORDDATA *kick;

    sh2gfw_init_GSPACKMAN(&GSCTL_man);
    sh2gfw_RotateTBuff(&shGs_AllEnv);
    sh2gfw_setREF_gsctl(shGs_AllEnv.DefaultEnv);
    kick = sh2gfw_setEND_gsctl();
    d1cSend(kick);
}

/* Matching: qwd is a Q_WORDDATA * (the 16-byte-aligned typedef) to get its 0x20(sp) slot. */
static void kari_setdmatag_and_draw(void) {
    Q_WORDDATA *qwd;
    Q_WORDDATA *buf;
    void sh2gfw_VUMATIRICES_registchain(); /* Matching: an unprototyped block-scope declaration, as in the original. */
    void sh2gfw_clear_TexMAN_TransParm(); /* (also unprototyped) */

    qwd = sh2gfw_Get_BLOCKmainPack();
    buf = qwd;
    kari_sh2gfw_VUMICRO_registchain(&qwd);
    sh2gfw_VUMATIRICES_registchain(&VU1_PARMS, &qwd, 0);
    sh2gfw_VUGSREGS_registchain(&VU1_PARMS, &qwd);
    sh2gfw_setEND_chain(&qwd);
    d1cSend(buf);
    sh2gfw_clear_TexMAN_TransParm(&AllTexSync_Man);
    sh2gfw_init_Trans(&AllTexSync_Man);
    sh2gfw_viewclip_block(&qwd);
}

static void Draw_Character_And_Shadow(void) {
    int count;
    struct sh2gfw_CAMERA *cam_tmp;

    cam_tmp = &cam0; /* Matching: dead; the original's DWARF has cam_tmp (as in sh2_shadow_main.c) */
    count = *T0_COUNT;
    if (Check_DrawOrder()) {
        sh2gfw_Exec_AnimeAndCollision();
        sh2gfw_Exec_Character_Draw(cam0.world_screen, 1);
        HH_Effect_Object_Texture_TransportPriority_Initialize();
        HH_Effect_Object_Texture_Send();
        HH_Class_Object_Execute();
        EFCTDoTask();
        HH_Effect_Object_Texture_Sync();
        HH_Class_Object_Packet_Kick();
        EFCTKickPacket();
        HH_Effect_Object_Texture_Finish();
        sh2gfw_test_MakeNoise();
        sh2gfw_ShadowControl_Main();
    } else {
        sh2gfw_Exec_AnimeAndCollision();
        sh2gfw_Exec_Character_Draw_ShadowOnly();
        if (sh2gfw_Get_NightOrDay()) {
            sh2gfw_ShadowControl_Main();
        }
        sh2gfw_Exec_Character_Draw(cam0.world_screen, 0);
        HH_Effect_Object_Texture_TransportPriority_Initialize();
        HH_Effect_Object_Texture_Send();
        HH_Class_Object_Execute();
        EFCTDoTask();
        HH_Effect_Object_Texture_Sync();
        HH_Class_Object_Packet_Kick();
        EFCTKickPacket();
        HH_Effect_Object_Texture_Finish();
        sh2gfw_test_MakeNoise();
    }
}

static int fog_check = 1;

static void Spack_All_Draw(void) {
    sh2gfw_Draw_SemiTransBG();
    if (shPadTrigger(3, 4)) {
        fog_check ^= 1;
    }
    if (Env_ctl.light_mode == 1) {
        if (fog_check) {
            if (BgIsOut(0) || FOG_ROOM()) {
                sh2gfw_send_fogtex();
                sh2gfw_fogtest_calcmain();
            }
        }
        fjDrawExec();
    } else {
        if (FOG_ROOM() && fog_check) {
            sh2gfw_send_fogtex();
            sh2gfw_fogtest_calcmain();
        }
        fjDrawExec();
    }
    sh2gfw_ThrSyncKick_spkDmaKick();
    if (sh2gfw_Get_NightOrDay()) {
        Kari_LensFlare_DrawExec();
    }
    sh2gde_ResetSpot_Jms();
    sh2gfw_Init_DemoLight_Work();
    DrawSpecialPolyforDemo();
}

static int FOG_ROOM(void) {
    int *mp;

    mp = Get_NowMapId();
    switch (*mp) {
    case 0x50001:
    case 0x90048:
    case 0xB00B9:
    case 0xB00BD:
    case 0xB00C1:
    case 0xC0029:
    case 0xC0047:
    case 0xC005B:
    case 0xD0035:
    case 0xD0049:
    case 0xE0005:
        return 1;
    }
    return 0;
}

static int DrawSpecialPolyforDemo(void) {
    if (stage && stage->gfw_func && stage->gfw_func->SpecDraw) {
        stage->gfw_func->SpecDraw();
        return 1;
    }
    return 0;
}

static int Check_DrawOrder(void) {
    int *mp;

    if (DramaDemoNumber()) {
        return 1;
    }
    mp = Get_NowMapId();
    switch (*mp) {
    case 0x90016:
    case 0xA007D:
        return 0;
    }
    return 1;
}
