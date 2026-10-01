/* savedata.c: packs the game state into SaveDataAll and restores it. */
#include "sh2.h"
#include "asm_helpers.h"
#include "libc/string.h"

/* Both argument orders of the 16-byte copy occur in this file (vcopy_dst_first and vcopy); the
 * order decides which address lands in v1. */

/** Sets the save point name shown in the save file (a message number).
 * @param msg_no message number */
void SetSavePointName(unsigned short msg_no) {
    SaveDataAll.d.scene = msg_no;
}

/** Packs the game state into SaveDataAll: James's position and heading, the play info, game
 * flags, items, key configuration and the characters' save data. */
void SetSaveData(void) {
    unsigned short scene;

    scene = SaveDataAll.d.scene;
    shQzero(&SaveDataAll, sizeof(SaveDataAll));
    SaveDataAll.d.version = 4;
    SaveDataAll.d.scene = scene;
    vcopy_dst_first(SaveDataAll.d.jms_pos, (float *)&sh2jms.player->pos);
    SaveDataAll.d.jms_pos[3] = sh2jms.player->rot.y;
    memcpy(&SaveDataAll.d.playing, &playing, sizeof(playing));
    memcpy(&SaveDataAll.d.game_flag, &game_flag, sizeof(game_flag));
    memcpy(&SaveDataAll.d.item, &item, sizeof(item));
    memcpy(&SaveDataAll.d.key_config, &key_config, sizeof(key_config));
    shCharacterSetSaveData(&SaveDataAll.d.chara);
}

/** Restores the game state from SaveDataAll (the key configuration only from version 4 on) and
 * applies the settings (SetGameEnvironment). */
void ExtGameData(void) {
    vcopy(SaveDataAll.d.jms_pos, connect_pos);
    memcpy(&playing, &SaveDataAll.d.playing, sizeof(playing));
    memcpy(&game_flag, &SaveDataAll.d.game_flag, sizeof(game_flag));
    memcpy(&item, &SaveDataAll.d.item, sizeof(item));
    if (SaveDataAll.d.version >= 4) {
        memcpy(&key_config, &SaveDataAll.d.key_config, sizeof(key_config));
    }
    shCharacterExtGameData(&SaveDataAll.d.chara);
    SetGameEnvironment();
}

/** Checks the loaded save data for out-of-range values (version, stage, play info, settings,
 * item counts).
 * @return 0 if it is valid, 1 if not */
int CheckSaveData(void) {
    struct Playing_Info *pi;
    unsigned short *in;

    pi = &SaveDataAll.d.playing;
    in = item.number; /* @bug the current items, not the loaded save's (SaveDataAll.d.item) */
    if (SaveDataAll.d.version != 4 && SaveDataAll.d.version != 3) {
        printf("savedata version error! (%d != %d)\n", SaveDataAll.d.version, 4);
        return 1;
    }
    if (pi->stage >= 53) {
        printf("stage number error! (%d)\n", pi->stage);
        return 1;
    }
    if (pi->time < 0.0f || pi->walk_distance < 0.0f || pi->run_distance < 0.0f ||
        pi->boat_clear_time < 0.0f || pi->boat_max_speed < 0.0f || pi->rank > 100 ||
        pi->total_time < 0.0f || pi->spray_pow < -1 || pi->spray_pow > 2 ||
        pi->jms_damage_total < 0.0f || pi->mar_damage_by_enemy < 0.0f || pi->mar_damage_by_jms < 0.0f) {
        printf("resultdata error!\n");
        return 1;
    }
    if (pi->battle_level > 4 || pi->riddle_level >= 4) {
        printf("leveldata error!\n");
        return 1;
    }
    if (pi->vibration > 3 || pi->auto_load > 1 || pi->sound > 2 || pi->weapon_control > 1 ||
        pi->blood_color > 3 || pi->view_control > 1 || pi->retreat_turn > 1 ||
        pi->walk_run_control > 1 || pi->auto_aiming > 1 || pi->view_mode > 1 ||
        pi->language > 5 || pi->subtitles > 1 || pi->control_type >= 2) {
        printf("configdata error!\n");
        return 1;
    }
    if (in[0] > 0 || in[1] > 999 || in[2] > 999 || in[3] > 999 || in[4] > 10 || in[5] > 999 ||
        in[6] > 6 || in[7] > 999 || in[8] > 4 || in[9] > 999 || in[10] > 8) {
        printf("item number error!\n");
        return 1;
    }
    return 0;
}

/** Applies the settings of the play info: sound output mode, volumes, brightness and screen
 * position. */
void SetGameEnvironment(void) {
    switch (playing.sound) {
    case 0:
        shSdCall(0x3ED, 0, 0, 0);
        shSdCall(0x411, 0, 0, 0);
        break;
    case 1:
        shSdCall(0x3ED, 0, 0, 0);
        shSdCall(0x410, 0, 0, 0);
        break;
    default:
        shSdCall(0x3EE, 0, 0, 0);
        shSdCall(0x411, 0, 0, 0);
        break;
    }
    SeMasterVolumeChange();
    sh2gfw_Set_Brightness(playing.brightness_level);
    shGs_SetDefaultDispArea();
    shGs_TrimDispArea(playing.screen_position_x / 4, playing.screen_position_y / 3);
}
