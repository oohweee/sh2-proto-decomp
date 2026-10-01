/*
 * connect.c: the load sequence run when the player moves to another stage or room
 * (Sh2sys.step[3] steps through it; the heavy parts run on the loader thread).
 */

#include "sh2.h"
#include "asm_helpers.h"

/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)

#define SH2SYS_STEP3_NEXT()      \
    Sh2sys.step[3]++;            \
    Sh2sys.step[4] = 0;          \
    Sh2sys.step[5] = 0;          \
    Sh2sys.step[6] = 0;          \
    Sh2sys.step[7] = 0

float connect_pos[4];

static int connectStageInit(void) {
    step_init_STAGE();
    CharaDataLoadStage();
    if ((Sh2sys.main_status >> 3) & 1) {
        ConnectCharaWorkReset();
        Sh2sys.main_status &= ~8;
    }
    ConnectCharaWorkJamesSet();
    CheckModeJumpDataSet();
    titleSetDataStartPoint();
    if (stage->stage_init) {
        stage->stage_init();
    }
    LoadBgCharaInit();
    LoadBgEventInit(0, 0);
    return 0;
}

static int connectRoomInit(void) {
    void map_DATA_LOAD(); /* Matching: an unprototyped block-scope declaration, as in the original. */

    CharaDataLoadRoom(RoomName(0, connect_pos[0], connect_pos[2]));
    map_DATA_LOAD(0);
    HH_Effect_Object_Texture_DesignateEntryLevel_Initialize(3);
    HH_Class_Object_Initialize();
    ConnectCharaWorkAdminClear();
    if (!BgIsOut(stage->glb_crd)) {
        ConnectCharaWorkAdminIn();
    } else {
        ConnectCharaWorkAdminOut(0);
    }
    PlayerInitOnConnect();
    if (GAME_FLAG(15)) {
        MariaInitOnConnect();
    }
    vcopy(connect_pos, sys.hero.pos);
    enEfctClear();
    vcInitCamera(NULL);
    loadBgCAM_vcReset();
    if (stage->room_init) {
        stage->room_init();
    }
    return 1;
}

static int connectPlayableInit(void) {
    ConnectCharaWorkWeapon();
    LightSpotOnOffSet();
    DataLoadMessage(0);
    DataLoadMessage(6);
    return 1;
}

/**
 * One frame of the stage/room change: overlay load, stage init and room init on the loader thread,
 * then the player's position after the move (a step machine on Sh2sys.step[3]). Returns non-zero
 * when done.
 */
int connectMain(void) {
    switch (Sh2sys.step[3]) {
    case 0:
        StgOverlay();
        SH2SYS_STEP3_NEXT();
        break;
    case 1:
        ((int (*)(int (*)(void)))lisPutCmd0)(connectStageInit);
        SH2SYS_STEP3_NEXT();
        break;
    case 2:
        if (lisSync(1, -1) >= 0) {
            SH2SYS_STEP3_NEXT();
        }
        break;
    case 3:
        shCharacterSetPosAfterDemo(sh2jms.player, connect_pos, connect_pos[3]);
        SH2SYS_STEP3_NEXT();
        break;
    case 4:
        ((int (*)(int (*)(void)))lisPutCmd0)(connectRoomInit);
        SH2SYS_STEP3_NEXT();
        break;
    case 5:
        if (lisSync(1, -1) >= 0) {
            SH2SYS_STEP3_NEXT();
        }
        break;
    case 6:
        ((int (*)(int (*)(void)))lisPutCmd0)(connectPlayableInit);
        SH2SYS_STEP3_NEXT();
        break;
    case 7:
        if (lisSync(1, -1) >= 0 && fsSync(1, -1) >= 0) {
            return 1;
        }
        break;
    }
    return 0;
}
