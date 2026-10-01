/*
 * gamemain.c: the per-frame game state machine. Sh2sys.step[1] selects the
 * top-level game state (logos, title, demo, playable...), step[2] the state
 * inside PlayableMain.
 */

#include "sh2.h"

#define DB_FLOW_CHECK(name) \
    ___dbFlowSetCheckPoint("`" name "'(" __FILE__ ":" SH_STRINGIFY(__LINE__) ")")

#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)

#define SH2SYS_STEP0(v) { Sh2sys.step[0] = (v); Sh2sys.step[1] = 0; Sh2sys.step[2] = 0; Sh2sys.step[3] = 0; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_STEP1(v) { Sh2sys.step[1] = (v); Sh2sys.step[2] = 0; Sh2sys.step[3] = 0; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_STEP2(v) { Sh2sys.step[2] = (v); Sh2sys.step[3] = 0; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_STEP3(v) { Sh2sys.step[3] = (v); Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_NEXT1() { Sh2sys.step[1]++; Sh2sys.step[2] = 0; Sh2sys.step[3] = 0; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_NEXT2() { Sh2sys.step[2]++; Sh2sys.step[3] = 0; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_NEXT3() { Sh2sys.step[3]++; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }

static int LoadBgSync(int mode, int nonblock);

/** Returns the shared memory buffer (MemShare_gp_data_buf). */
char *get_gp_data_buf_addr(void) {
    return MemShare_gp_data_buf;
}

/* min(a, b) with slt/movn through t7 (inline asm in the original; see item.c). */
inline int imin(int a, int b) {
    asm { slt t7, b, a; movn a, b, t7 }
    return a;
}
/**
 * One frame of the game: runs the current top-level state (Sh2sys.step[1]), then syncs the draw.
 * Returns the frame's draw type (0: 2D, 1: 3D, -1: none).
 */
int GameMain(void) {
    int synctype;
    int fonton;
    int glb_crd;
    float px;
    float pz;
    int cnt;

    synctype = 0;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 127
    DB_FLOW_CHECK("pad");
    shPadSet();
    DB_FLOW_CHECK("snd vol");
    sndVolumeMain();
    DB_FLOW_CHECK("sound");
    if (!(Sh2sys.main_status >> 5 & 1)) {
        SeSoundManager();
    }

    DB_FLOW_CHECK("loadbg setinfo");
    glb_crd = 0;
    pz = px = 0.0f;
    if (sh2jms.player) {
        px = sh2jms.player->pos.x;
        pz = sh2jms.player->pos.z;
        if (stage) {
            glb_crd = stage->glb_crd;
        }
    }
    loadBgCommon_SetInfo(glb_crd, px, pz);
    if (Sh2sys.step[1] == 0xD && Sh2sys.step[2] == 4) {
        if (Sh2sys.pre_playable) {
            shSetDF(imin(3, Get_FrameRate()));

        } else {
            shSetDF(2);
        }
        Sh2sys.pre_playable = 1;
    } else {
        shSetDF(2);
        Sh2sys.pre_playable = 0;
    }
    shSetDFreal(Get_FrameRate());
    switch (Sh2sys.step[1]) {
    case 0:

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 178
        DB_FLOW_CHECK("s:menu");
        SH2SYS_NEXT1();
    case 1:

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 189
        DB_FLOW_CHECK("s:bootfirst");
        SH2SYS_NEXT1();
    case 2:


        DB_FLOW_CHECK("s:logo mc");
        logoCheckingMemcard();
        break;
    case 3:


        DB_FLOW_CHECK("s:logo 1");
        logoDrawWarningCESA();
        break;
    case 4:


        DB_FLOW_CHECK("s:logo 2");
        logoDrawWarningSCE();
        break;
    case 5:


        DB_FLOW_CHECK("s:logo 3");
        logoDrawKonamiLogo();
        break;
    case 12:

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 228
        DB_FLOW_CHECK("s:op movie");
        if (!MoviePlayOPMovie()) {
            sh2gfw_ForceSet_MovieDrawLoopCounter();
            MemShareWaitRealloc(0);
            SH2SYS_STEP1(7);
        }
        synctype = -1;
        break;
    case 6:

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 242
        DB_FLOW_CHECK("s:reset");
        SH2SYS_NEXT1();
    case 7:
    case 9:
    case 11:



        DB_FLOW_CHECK("s:title");
        switch (TitleMain()) {
        case 0:
            break;
        case 1:
            SH2SYS_STEP1(0x11);
            mcStepInit();
            break;
        case 2:
            ExtGameData();
            Sh2sys.main_status |= 0x20;
            SH2SYS_STEP1(0xD);
            SH2SYS_STEP2(1);
            mcStepInit();
            break;
        case 3:
        default:
            SH2SYS_STEP1(0xD);
            mcStepInit();
            break;
        case 4:
            switch (Sh2sys.step[1]) {
            default:
            case 7:
                SH2SYS_STEP1(0xE);
                break;
            case 9:
                SH2SYS_STEP1(0xF);
                break;
            case 11:
                SH2SYS_STEP1(0x10);
                break;
            }
            SH2SYS_STEP2(7);
            SH2SYS_STEP3(0);
            break;
        case 5:
        case 6:
            SH2SYS_STEP1(0xC);
            break;
        }
        break;
    case 13:
        synctype = PlayableMain();
        fonton = 1;
        break;
    case 8:
    case 10:

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 316
        DB_FLOW_CHECK("s:demoplay");
        cnt = Sh2sys.step[2];
        if (shPadTrigger(0, 4)) {
            cnt = 600;
        }
        if (cnt < 600) {
            dbfntlocate(0x100, 0x100);
            dbfntprintf("playdemo:%3d/%3d", cnt, 600);
            SH2SYS_NEXT2();
        } else {
            SH2SYS_NEXT1();
        }
        break;
    case 14:
    case 15:
    case 16:

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 339
        DB_FLOW_CHECK("s:config");
        synctype = PlayableMain();
        fonton = 1;
        if (Sh2sys.step[3] == 1) {
            switch (Sh2sys.step[1]) {
            default: case 14:
                SH2SYS_STEP1(7);
                break;
            case 15:
                SH2SYS_STEP1(9);
                break;
            case 16:
                SH2SYS_STEP1(0xB);
                break;
            }
        }
        /* Matching: #line keeps the original numbering after the added brace. */
#line 354
        break;
    case 17:
        DB_FLOW_CHECK("s:mc load");
        mcLoadMenu();
        break;
    }
    WaitSemaPss();
    NowLoadingCheck();
    if (!PauseDisp()) {
        switch (synctype) {
        case 0:

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 377
            DB_FLOW_CHECK("2d sync");
            kari_drawloop_main_2dSYNC();
            break;
        case 1:
            DB_FLOW_CHECK("3d sync");
            draw_main_3dSYNC();
            break;
        case -1:
            break;
        }

        DB_FLOW_CHECK("screen effect");
        ScreenEffectManager();
    }
    NowLoadingDraw();
    if (fonton) {



        DB_FLOW_CHECK("font");
        fontEachTurn();
    }
    SignalSemaPss();
    return synctype;
}

static int LoadBgSync(int mode, int nonblock) {
    int halt;
    int rest;
    int require;
    int loading;

    do {
        halt = 0;
        require = 0;
        loading = 0;
        halt += loadBgAll_PrepareAround(&loading, &require);
        switch (mode) {
        case 0:
            break;
        case 2:
            break;
        case 1:
            halt = loading;
            break;
        }
        if (halt == 0) {
            LoadBgCharaLoadSync();
            rest = !LoadBgCharaIsLoad();
            if (rest) {
                require++;
            }
            if (LoadBgCharaIsMapEdge()) {
                rest = 0;
            }
            halt += rest;
        }
        if (halt == 0) {
            LoadBgEventLoadSync();
            rest = LoadBgEventLoadCnt();
            if (rest) {
                require += LoadBgEventListCnt();
            }
        }
        if (ev_p_step) {
            nonblock = 0;
        }
    } while (!nonblock && halt);
    if (halt) {
        dbfntlocate(0xB0, 0x100);
        dbfntprintf("Now load back ground.\n    Please wait: %d/%d", halt, require);
    }
    return halt;
}

/**
 * One frame of the playable game (Sh2sys.step[2]): loading, the stage step, pause, and the move to
 * other rooms. Returns the frame's draw type, as GameMain().
 */
int PlayableMain(void) {
    int halt;
    int synctype;

    synctype = 0;
    WaitSemaPss();
    RadioNoise();
    switch (Sh2sys.step[2]) {
    case 0:
        NowLoadingEnable();

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 567
        DB_FLOW_CHECK("g0:start");
        ScreenEffectFadeStart(2, 1.2f);
        FlagInit();
        ItemDataInit();
        SH2SYS_NEXT2();
        break;
    case 1:
        NowLoadingEnable();


        DB_FLOW_CHECK("g0:connect");
        ScreenEffectFadeStart(2, 1.2f);
        if ((mcAfterLoadMenu() != 0) & (connectMain() != 0)) {
            Sh2sys.main_status &= ~0x20;
            SH2SYS_NEXT2();
        }
        break;
    case 2:
        NowLoadingEnable();

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 595
        DB_FLOW_CHECK("g0:connect wait");
        halt = LoadBgSync(1, 1);
        if (Sh2sys.step[3] != 3) {
            SH2SYS_NEXT3();
        } else if (!halt) {
            JumpMenuPosNormal();
            PlayerSetHeightConnectWait();
            if (GAME_FLAG(15)) {
                MariaSetHeightConnectWait();
            }
            SH2SYS_NEXT2();
        }
        break;
    case 3:
        NowLoadingEnable();

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 617
        DB_FLOW_CHECK("g0:sound load");
        if (Sh2sys.step[3] == 0) {
            SeSoundLoad();
            SH2SYS_NEXT3();
        }
        if (!(shSdStat() & 0xF) && fsSync(1, -1) >= 0) {
            if (stage->sound_call_after_load) {
                stage->sound_call_after_load();
            }
            /* Matching: #line keeps the original numbering after the added brace. */
#line 625
            ScreenEffectFadeStart(4, 1.2f);
            SH2SYS_NEXT2();
        }
        break;
    case 4: {
        int cdstat;
        DB_FLOW_CHECK("g0:playable");
        Sh2sys.soft_reset = 1;
        cdstat = fsGetTrayStat();
        if (cdstat) {
            switch (cdstat) {
            case 1:
                PauseSetType(5);
                Sh2sys.step[2] = 0xF;
                break;
            case 2:
                PauseSetType(7);
                break;
            case 3:
                PauseSetType(6);
                Sh2sys.step[2] = 0xF;
                break;
            case 4:
                PauseSetType(8);
                Sh2sys.step[2] = 0xF;
                break;
            }
            synctype = 0;
        } else if (!ev_p_step && shPadTrigger(0, key_config.pause)) {
            Sh2sys.step[2] = 0xF;
            PauseSetType(2);
        } else if (LoadBgSync(0, 1)) {
            PauseSetType(4);
            synctype = 0;
        } else {

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 680
            DB_FLOW_CHECK("g0:playable:event main");
            EventMain();
            if (!(Sh2sys.main_status & 1)) {



                DB_FLOW_CHECK("g0:playable:draw main");
                draw_main();
                synctype = 1;
            } else {
                Sh2sys.main_status &= ~1;
                clFrameInitCollisionData();
            }
            if (!(Sh2sys.main_status >> 6 & 1)) {
                GameTimerCountUp();
            }
        }
        break;
    }
    case 5:

        DB_FLOW_CHECK("g0:chizu");
        ChizuMain();
        GameTimerCountUp();
        break;
    case 6:


        DB_FLOW_CHECK("g0:item");
        itemmain();
        d1cSend(spkDmaKick());
        GameTimerCountUp();
        break;
    case 7:


        DB_FLOW_CHECK("g0:option");
        option_main();
        d1cSend(spkDmaKick());
        GameTimerCountUp();
        break;
    case 8:


        DB_FLOW_CHECK("g0:memo");
        MemoMain();
        d1cSend(spkDmaKick());
        GameTimerCountUp();
        break;
    case 9:


        DB_FLOW_CHECK("g0:mc save");
        mcSaveMenu();
        break;
    case 10:


        DB_FLOW_CHECK("g0:result");
        ResultMain();
        break;
    case 11:

        DB_FLOW_CHECK("g0:end");
        Sh2sys.soft_reset = 0;
        if (GameendMain()) {
            SH2SYS_STEP0(2);
        }
        /* Matching: #line keeps the original numbering after the added brace. */
#line 747
        break;
    case 12:

        DB_FLOW_CHECK("g0:over");
        Sh2sys.soft_reset = 0;
        if (GameoverMain()) {
            SH2SYS_STEP0(2);
        }
        /* Matching: #line keeps the original numbering after the added brace. */
#line 754
        break;
    case 13:
        DB_FLOW_CHECK("g0:movie");
        Sh2sys.soft_reset = 0;
        if (MovieWaitReady()) {
            SH2SYS_NEXT2();
        }
        /* Matching: #line keeps the original numbering after the added brace. */
#line 760
        break;
    case 14:
        DB_FLOW_CHECK("g0:movie main");
        Sh2sys.soft_reset = 0;
        HH_Effect_Object_Texture_DesignateEntryLevel_Discard(1);
        MoviePlayFromReady();
        SignalSemaPss();
        if (MovieMain() <= 0) {
            sh2gfw_ForceSet_MovieDrawLoopCounter();
            HH_Effect_Object_Texture_AlwaysTexture_Initialize();
            MemShareWaitRealloc(0);
            SH2SYS_STEP2(4);
        }
        WaitSemaPss();
        if (!LoadBgSync(0, 1)) {
            shPadSet();

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 788
            DB_FLOW_CHECK("g0:event main after movie");
            EventMain();
            synctype = -1;
        }
        break;
    case 15: {
        int ptype;
        int cdstat;

        ptype = 2;
        cdstat = fsGetTrayStat();
        if (cdstat) {
            switch (cdstat) {
            case 1:
                ptype = 5;
                break;
            case 2:
                ptype = 7;
                break;
            case 3:
                ptype = 6;
                break;
            case 4:
                ptype = 8;
                break;
            }
        }
        PauseSetType(ptype);
        if (ptype == 2 && shPadTrigger(0, key_config.pause | 0xC)) {
            Sh2sys.step[2] = 4;
        }
        synctype = 0;
        break;
    }
    }
    SignalSemaPss();
    return synctype;
}
