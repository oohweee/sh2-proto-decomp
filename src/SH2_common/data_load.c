/*
 * data_load.c: loads the message (text) files for the current language, and
 * a DMA memory copy through the scratchpad.
 */

#include "sh2.h"
#include "sdk/eekernel.h"

/* Messages of the current stage and menus; the common messages go to msg_station. */
unsigned short msg_station[0x800] __attribute__((aligned(64)));
unsigned short msg_buffer[0x8000] __attribute__((aligned(64)));

/** Loads message file group `msg` unless it is already in memory; returns the read's fid (-2: nothing read). */
int DataLoadMessage(int msg) {
    static union fsFileIndex *file_common[6] = {data_etc_message_common_msg_j_mes, data_etc_message_common_msg_e_mes};
    static union fsFileIndex *msg_station_on_memory = NULL;
    static union fsFileIndex *file_item[6] = {data_etc_message_item_msg_j_mes, data_etc_message_item_msg_e_mes};
    static union fsFileIndex *file_option[6] = {data_etc_message_option_msg_j_mes, data_etc_message_option_msg_e_mes};
    static union fsFileIndex *file_memo[6] = {data_etc_message_memo_msg_j_mes, data_etc_message_memo_msg_e_mes};
    static union fsFileIndex *file_m_card[6] = {data_etc_message_m_card_msg_j_mes, data_etc_message_m_card_msg_e_mes};
    static union fsFileIndex *file_result[6] = {data_etc_message_result_msg_j_mes, data_etc_message_result_msg_e_mes};
    static union fsFileIndex *file_stage[53][6] = {
        {NULL},
        {data_etc_message_stage_tgs_trial_msg_j_mes, data_etc_message_stage_tgs_trial_msg_e_mes},
        {data_etc_message_stage_toilet_msg_j_mes, data_etc_message_stage_toilet_msg_e_mes},
        {data_etc_message_stage_observation_msg_j_mes, data_etc_message_stage_observation_msg_e_mes},
        {data_etc_message_stage_forest_msg_j_mes, data_etc_message_stage_forest_msg_e_mes},
        {data_etc_message_stage_town_east_msg_j_mes, data_etc_message_stage_town_east_msg_e_mes},
        {data_etc_message_stage_apart_e1f_msg_j_mes, data_etc_message_stage_apart_e1f_msg_e_mes},
        {data_etc_message_stage_apart_e2f_msg_j_mes, data_etc_message_stage_apart_e2f_msg_e_mes},
        {data_etc_message_stage_apart_e3fw_msg_j_mes, data_etc_message_stage_apart_e3fw_msg_e_mes},
        {data_etc_message_stage_apart_e3fe_msg_j_mes, data_etc_message_stage_apart_e3fe_msg_e_mes},
        {data_etc_message_stage_apart_w1f_msg_j_mes, data_etc_message_stage_apart_w1f_msg_e_mes},
        {data_etc_message_stage_apart_w2f_msg_j_mes, data_etc_message_stage_apart_w2f_msg_e_mes},
        {data_etc_message_stage_apart_stair_msg_j_mes, data_etc_message_stage_apart_stair_msg_e_mes},
        {data_etc_message_stage_apart_out_msg_j_mes, data_etc_message_stage_apart_out_msg_e_mes},
        {data_etc_message_stage_town_west_msg_j_mes, data_etc_message_stage_town_west_msg_e_mes},
        {data_etc_message_stage_bowling_msg_j_mes, data_etc_message_stage_bowling_msg_e_mes},
        {data_etc_message_stage_to_heaven_msg_j_mes, data_etc_message_stage_to_heaven_msg_e_mes},
        {data_etc_message_stage_heaven_night_msg_j_mes, data_etc_message_stage_heaven_night_msg_e_mes},
        {data_etc_message_stage_hospital_1f_f_msg_j_mes, data_etc_message_stage_hospital_1f_f_msg_e_mes},
        {data_etc_message_stage_hospital_2f_f_msg_j_mes, data_etc_message_stage_hospital_2f_f_msg_e_mes},
        {data_etc_message_stage_hospital_3f_f_msg_j_mes, data_etc_message_stage_hospital_3f_f_msg_e_mes},
        {data_etc_message_stage_hospital_rf_f_msg_j_mes, data_etc_message_stage_hospital_rf_f_msg_e_mes},
        {data_etc_message_stage_hospital_1fw_b_msg_j_mes, data_etc_message_stage_hospital_1fw_b_msg_e_mes},
        {data_etc_message_stage_hospital_1fe_b_msg_j_mes, data_etc_message_stage_hospital_1fe_b_msg_e_mes},
        {data_etc_message_stage_hospital_2f_b_msg_j_mes, data_etc_message_stage_hospital_2f_b_msg_e_mes},
        {data_etc_message_stage_hospital_3f_b_msg_j_mes, data_etc_message_stage_hospital_3f_b_msg_e_mes},
        {data_etc_message_stage_hospital_bf_b_msg_j_mes, data_etc_message_stage_hospital_bf_b_msg_e_mes},
        {data_etc_message_stage_hospital_pass_msg_j_mes, data_etc_message_stage_hospital_pass_msg_e_mes},
        {data_etc_message_stage_society_msg_j_mes, data_etc_message_stage_society_msg_e_mes},
        {data_etc_message_stage_delusion_2_msg_j_mes, data_etc_message_stage_delusion_2_msg_e_mes},
        {data_etc_message_stage_delusion_3_msg_j_mes, data_etc_message_stage_delusion_3_msg_e_mes},
        {data_etc_message_stage_prison_n_msg_j_mes, data_etc_message_stage_prison_n_msg_e_mes},
        {data_etc_message_stage_prison_s_msg_j_mes, data_etc_message_stage_prison_s_msg_e_mes},
        {data_etc_message_stage_prison_bf_msg_j_mes, data_etc_message_stage_prison_bf_msg_e_mes},
        {data_etc_message_stage_labyrinth_w_msg_j_mes, data_etc_message_stage_labyrinth_w_msg_e_mes},
        {data_etc_message_stage_labyrinth_e_msg_j_mes, data_etc_message_stage_labyrinth_e_msg_e_mes},
        {data_etc_message_stage_labyrinth_n_msg_j_mes, data_etc_message_stage_labyrinth_n_msg_e_mes},
        {data_etc_message_stage_eddie_boss_msg_j_mes, data_etc_message_stage_eddie_boss_msg_e_mes},
        {data_etc_message_stage_lake_msg_j_mes, data_etc_message_stage_lake_msg_e_mes},
        {data_etc_message_stage_hotel_bf_f_msg_j_mes, data_etc_message_stage_hotel_bf_f_msg_e_mes},
        {data_etc_message_stage_hotel_1f_f_msg_j_mes, data_etc_message_stage_hotel_1f_f_msg_e_mes},
        {data_etc_message_stage_hotel_2f_f_msg_j_mes, data_etc_message_stage_hotel_2f_f_msg_e_mes},
        {data_etc_message_stage_hotel_3f_f_msg_j_mes, data_etc_message_stage_hotel_3f_f_msg_e_mes},
        {data_etc_message_stage_hotel_bf_b_msg_j_mes, data_etc_message_stage_hotel_bf_b_msg_e_mes},
        {data_etc_message_stage_hotel_1f_b_msg_j_mes, data_etc_message_stage_hotel_1f_b_msg_e_mes},
        {data_etc_message_stage_hotel_2f_b_msg_j_mes, data_etc_message_stage_hotel_2f_b_msg_e_mes},
        {data_etc_message_stage_hotel_3f_b_msg_j_mes, data_etc_message_stage_hotel_3f_b_msg_e_mes},
        {data_etc_message_stage_hotel_fire_msg_j_mes, data_etc_message_stage_hotel_fire_msg_e_mes},
        {data_etc_message_stage_end_recovery_msg_j_mes, data_etc_message_stage_end_recovery_msg_e_mes},
        {data_etc_message_stage_end_maria_msg_j_mes, data_etc_message_stage_end_maria_msg_e_mes},
        {data_etc_message_stage_end_suicide_msg_j_mes, data_etc_message_stage_end_suicide_msg_e_mes},
        {data_etc_message_stage_end_rebirth_msg_j_mes, data_etc_message_stage_end_rebirth_msg_e_mes},
        {data_etc_message_stage_end_dog_msg_j_mes, data_etc_message_stage_end_dog_msg_e_mes},
    };
    static union fsFileIndex *msg_buffer_on_memory = NULL;
    int fid;

    fid = -2;
    switch (msg) {
    case 0:
        if (file_common[playing.language] != msg_station_on_memory) {
            fid = FcRead(file_common[playing.language], msg_station);
            msg_station_on_memory = file_common[playing.language];
        }
        break;
    case 1:
        if (file_item[playing.language] != msg_buffer_on_memory) {
            fid = FcRead(file_item[playing.language], msg_buffer);
            msg_buffer_on_memory = file_item[playing.language];
        }
        break;
    case 2:
        if (file_option[playing.language] != msg_buffer_on_memory) {
            fid = FcRead(file_option[playing.language], msg_buffer);
            msg_buffer_on_memory = file_option[playing.language];
        }
        break;
    case 3:
        if (file_memo[playing.language] != msg_buffer_on_memory) {
            fid = FcRead(file_memo[playing.language], msg_buffer);
            msg_buffer_on_memory = file_memo[playing.language];
        }
        break;
    case 4:
        if (file_m_card[playing.language] != msg_buffer_on_memory) {
            fid = FcRead(file_m_card[playing.language], msg_buffer);
            msg_buffer_on_memory = file_m_card[playing.language];
        }
        break;
    case 5:
        if (file_result[playing.language] != msg_buffer_on_memory) {
            fid = FcRead(file_result[playing.language], msg_buffer);
            msg_buffer_on_memory = file_result[playing.language];
        }
        break;
    case 6:
        if (file_stage[playing.stage][playing.language] != msg_buffer_on_memory) {
            fid = FcRead(file_stage[playing.stage][playing.language], msg_buffer);
            msg_buffer_on_memory = file_stage[playing.stage][playing.language];
        }
        break;
    }
    return fid;
}


/**
 * Copies datasize bytes from sr to ds: 4KB at a time into the scratchpad
 * (toSPR, channel 9) and back out (fromSPR, channel 8), alternating between
 * two scratchpad pages.
 */
void shMemCopy(void *ds, void *sr, int datasize) {
    static int bufpage;
    int now;

    bufpage ^= 1;
    SyncDCache(sr, (char *)sr + datasize);
    InvalidDCache(sr, (char *)sr + datasize);
    SyncDCache(ds, (char *)ds + datasize);
    InvalidDCache(ds, (char *)ds + datasize);
    now = datasize;
    while (*D9_CHCR & 0x100) {
    }
    while (now >= 0x1000) {
        *D9_SADR = 0x70002000 + (bufpage << 12);
        *D9_MADR = (unsigned int)sr + datasize - now;
        *D9_QWC = 0x100;
        while (*D8_CHCR & 0x100) {
        }
        *D9_CHCR = 0x101;
        *D8_SADR = 0x70002000 + (bufpage << 12);
        *D8_MADR = (unsigned int)ds + datasize - now;
        *D8_QWC = 0x100;
        while (*D9_CHCR & 0x100) {
        }
        *D8_CHCR = 0x100;
        bufpage ^= 1;
        now -= 0x1000;
    }
    while (*D8_CHCR & 0x100) {
    }
    if (now) {
        *D9_SADR = 0x70002000 + (bufpage << 12);
        *D9_MADR = (unsigned int)sr + datasize - now;
        *D9_QWC = now >> 4;
        while (*D9_CHCR & 0x100) {
        }
        *D9_CHCR = 0x101;
        *D8_SADR = 0x70002000 + (bufpage << 12);
        *D8_MADR = (unsigned int)ds + datasize - now;
        *D8_QWC = now >> 4;
        while (*D9_CHCR & 0x100) {
        }
        *D8_CHCR = 0x100;
        while (*D8_CHCR & 0x100) {
        }
        return;
    }
}
