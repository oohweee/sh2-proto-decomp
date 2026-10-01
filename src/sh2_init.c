/*
 * sh2_init.c: cold init (once at power on) and hot init (a step machine run by the
 * main loop on every (re)start, one step per few frames).
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "sdk/libmc.h"

static void PlayingInfoColdInit(void);
static void PlayingInfoHotInit(void);

#define SH2SYS_STEP1_NEXT()      \
    Sh2sys.step[1]++;            \
    Sh2sys.step[2] = 0;          \
    Sh2sys.step[3] = 0;          \
    Sh2sys.step[4] = 0;          \
    Sh2sys.step[5] = 0;          \
    Sh2sys.step[6] = 0;          \
    Sh2sys.step[7] = 0

/**
 * Power-on initialization: the system layer, console settings, graphics and the game's data that
 * stays loaded.
 */
void systemColdInit(void) {
    int fid;
    int sid;

    fid = init_sh2_filesys();
    sh2ScfInit();
    init_PS2();
    DBG_data_loadGS();
    mdl_SpecularMappingLoadTexture();
    fsSync(0, fid);
    sid = init_sh2_devsys();
    init_sh2_dmac();
    spkInit();
    fontInit();
    WaitSema(sid);
    shPadInit();
    sceMcInit();
    PlayingInfoColdInit();
    DataLoadMessage(0);
    sh2gfw_util_zeroq((union Q_WORDDATA *)&AllTexSync_Man, 0x1F05);
    sh2gfw_allinit_TexMANlist(&AllTexSync_Man);
    sh2gfw_kari_clear_LM();
    shCharacter_Manage_Init();
    sh2gfw_srInit_ModelDrawWork();
    FcRead(data_pic_etc_carsol_tex, cursor_adr);
    CharaDataLoadInit();
    while (1) {
        if (!(shSdStat() & 0xF)) {
            break;
        }
        shSyncVEnd(0);
    }
    MovieInit();
}

/**
 * One step of the hot init (Sh2sys.step[1]) run before the game starts or restarts. Returns
 * non-zero when done.
 */
int systemHotInit(void) {
    static int count;
    int step;
    int ret;
    int wait;
    int next;

    step = Sh2sys.step[1];
    ret = 0;
    wait = 2;
    if (dbFlag(4)) {
        wait = 100;
    }
    if (step > 0) {
        DrawLopp_Pre();
        dbfntlocate(0x100, 0x100);
        dbfntprintf("Hot Init: %d(%d)", step, count);
    } else {
        count = 0;
    }
    if (--count <= 0) {
        switch (step) {
        case 0:
            MemShareWaitRealloc(0);
            step_init_ONE();
            all_Frame_Buffer_Clear();
            ScreenEffectInit();
            break;
        case 1:
            mcInit();
            break;
        case 2:
            sh2gfw_srInit_ModelDrawWork();
            break;
        case 3:
            CharaDataDeleteAll();
            CharaDataLoadItem();
            enInitEnemy();
            break;
        case 4:
            shCharacterInitSubCharacter();
            break;
        case 5:
            shCharacterInitSkeltons();
            break;
        case 6:
            shCharacterInitCluster();
            break;
        case 7:
            kari_ChAlloc_Init();
            break;
        case 8:
            shCharacterPlayerWorkInitAtPowerOn();
            shCharacterPlayerWorkInitAtGameStart();
            shCharacterMariaWorkInit();
            shCharacterMariaWorkInitAtGameStart();
            break;
        case 9:
            EventProgInit();
            break;
        case 10:
            SeCallReset();
            break;
        case 11:
            stage = NULL;
            playing.stage = 0;
            PlayingInfoHotInit();
            break;
        case 12:
            demo_number = 0;
            Sh2sys.main_status &= ~0x40;
            break;
        case 13:
            SeSoundLoad();
            break;
        case 14:
        default:
            if (!(shSdStat() & 0xF) && fsSync(1, -1) >= 0) {
                ret = 1;
            }
            break;
        }
        count = wait;
        SH2SYS_STEP1_NEXT();
    }
    fsSync(0, -1);
    Sh2sys.soft_reset = 0;
    DrawLopp_Post();
    return ret;
}

static void PlayingInfoColdInit(void) {
    playing.enemy_off = 0;
    playing.voice_off = 0;
    playing.memo_select = 0;
    playing.clear_end_kind = 0;
    playing.clear_end_number = 0;
    playing.battle_level = 2;
    playing.riddle_level = 1;
    playing.control_type = 0;
    playing.brightness_level = 3;
    playing.screen_position_x = 0;
    playing.screen_position_y = 0;
    playing.vibration = 2;
    playing.auto_load = 0;
    playing.sound = 0;
    playing.bgm_volume = 15;
    playing.se_volume = 15;
    playing.weapon_control = 0;
    playing.blood_color = 0;
    playing.view_control = 0;
    playing.retreat_turn = 0;
    playing.walk_run_control = 0;
    playing.auto_aiming = 1;
    playing.view_mode = 0;
    playing.bullet_adjust = 1;
    playing.language = 1;
    playing.subtitles = 1;
    PlayingInfoHotInit();
}

static void PlayingInfoHotInit(void) {
    int i;

    playing.savecount = 0;
    playing.item_get = 0;
    playing.kill_by_shot = 0;
    playing.kill_by_fight = 0;
    playing.time = 0.0f;
    playing.walk_distance = 0.0f;
    playing.run_distance = 0.0f;
    playing.boat_clear_time = 0.0f;
    playing.boat_max_speed = 0.0f;
    playing.jms_damage_total = 0.0f;
    playing.mar_damage_by_enemy = 0.0f;
    playing.mar_damage_by_jms = 0.0f;
    playing.rank = 6;
    playing.total_score = 0;
    playing.total_time = 0.0f;
    for (i = 0; i < 7; i++) {
        playing.stage_check_point[i] = 0.0f;
        playing.stage_score[i] = 0;
    }
}
