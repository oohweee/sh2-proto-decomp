/*
 * stg_overlay.c: loads the overlay (code + data) of the current stage and points
 * `stage` at its Stage_Data.
 */
#include "sh2.h"
#include "sdk/eekernel.h"

/* Stage names (DWARF: an anonymous enum, used through a typedef; the name STG_NAME is ours; not in
   the generated headers). */
typedef enum {
    Stg_null = 0,
    Stg_tgs_trial = 1,
    Stg_toilet = 2,
    Stg_observation = 3,
    Stg_forest = 4,
    Stg_town_east = 5,
    Stg_apart_e1f = 6,
    Stg_apart_e2f = 7,
    Stg_apart_e3fw = 8,
    Stg_apart_e3fe = 9,
    Stg_apart_w1f = 10,
    Stg_apart_w2f = 11,
    Stg_apart_stair = 12,
    Stg_apart_out = 13,
    Stg_town_west = 14,
    Stg_bowling = 15,
    Stg_to_heaven = 16,
    Stg_heaven_night = 17,
    Stg_hospital_1f_f = 18,
    Stg_hospital_2f_f = 19,
    Stg_hospital_3f_f = 20,
    Stg_hospital_rf_f = 21,
    Stg_hospital_1fw_b = 22,
    Stg_hospital_1fe_b = 23,
    Stg_hospital_2f_b = 24,
    Stg_hospital_3f_b = 25,
    Stg_hospital_bf_b = 26,
    Stg_hospital_pass = 27,
    Stg_society = 28,
    Stg_delusion_2 = 29,
    Stg_delusion_3 = 30,
    Stg_prison_n = 31,
    Stg_prison_s = 32,
    Stg_prison_bf = 33,
    Stg_labyrinth_w = 34,
    Stg_labyrinth_e = 35,
    Stg_labyrinth_n = 36,
    Stg_eddie_boss = 37,
    Stg_lake = 38,
    Stg_hotel_bf_f = 39,
    Stg_hotel_1f_f = 40,
    Stg_hotel_2f_f = 41,
    Stg_hotel_3f_f = 42,
    Stg_hotel_bf_b = 43,
    Stg_hotel_1f_b = 44,
    Stg_hotel_2f_b = 45,
    Stg_hotel_3f_b = 46,
    Stg_hotel_fire = 47,
    Stg_end_recovery = 48,
    Stg_end_maria = 49,
    Stg_end_suicide = 50,
    Stg_end_rebirth = 51,
    Stg_end_dog = 52,
    Stg_num = 53,
} STG_NAME;

/* the overlay load address (linker-defined) */
extern char _ovl_start_addr[];

static union fsFileIndex *StgOverlayGetFileID(STG_NAME stg_name) {
    switch (stg_name) {
    default:
        return NULL;
    case Stg_toilet:
        return root_stage_toilet_bin;
    case Stg_observation:
        return root_stage_observation_bin;
    case Stg_forest:
        return root_stage_forest_bin;
    case Stg_town_east:
        return root_stage_town_east_bin;
    case Stg_apart_e1f:
        return root_stage_apart_e1f_bin;
    case Stg_apart_e2f:
        return root_stage_apart_e2f_bin;
    case Stg_apart_e3fw:
        return root_stage_apart_e3fw_bin;
    case Stg_apart_e3fe:
        return root_stage_apart_e3fe_bin;
    case Stg_apart_w1f:
        return root_stage_apart_w1f_bin;
    case Stg_apart_w2f:
        return root_stage_apart_w2f_bin;
    case Stg_apart_stair:
        return root_stage_apart_stair_bin;
    case Stg_apart_out:
        return root_stage_apart_out_bin;
    case Stg_town_west:
        return root_stage_town_west_bin;
    case Stg_bowling:
        return root_stage_bowling_bin;
    case Stg_to_heaven:
        return root_stage_to_heaven_bin;
    case Stg_heaven_night:
        return root_stage_heaven_night_bin;
    case Stg_hospital_1f_f:
        return root_stage_hospital_1f_f_bin;
    case Stg_hospital_2f_f:
        return root_stage_hospital_2f_f_bin;
    case Stg_hospital_3f_f:
        return root_stage_hospital_3f_f_bin;
    case Stg_hospital_rf_f:
        return root_stage_hospital_rf_f_bin;
    case Stg_hospital_1fw_b:
        return root_stage_hospital_1fw_b_bin;
    case Stg_hospital_1fe_b:
        return root_stage_hospital_1fe_b_bin;
    case Stg_hospital_2f_b:
        return root_stage_hospital_2f_b_bin;
    case Stg_hospital_3f_b:
        return root_stage_hospital_3f_b_bin;
    case Stg_hospital_bf_b:
        return root_stage_hospital_bf_b_bin;
    case Stg_hospital_pass:
        return root_stage_hospital_pass_bin;
    case Stg_society:
        return root_stage_society_bin;
    case Stg_delusion_2:
        return root_stage_delusion_2_bin;
    case Stg_delusion_3:
        return root_stage_delusion_3_bin;
    case Stg_prison_n:
        return root_stage_prison_n_bin;
    case Stg_prison_s:
        return root_stage_prison_s_bin;
    case Stg_prison_bf:
        return root_stage_prison_bf_bin;
    case Stg_labyrinth_w:
        return root_stage_labyrinth_w_bin;
    case Stg_labyrinth_e:
        return root_stage_labyrinth_e_bin;
    case Stg_labyrinth_n:
        return root_stage_labyrinth_n_bin;
    case Stg_eddie_boss:
        return root_stage_eddie_boss_bin;
    case Stg_lake:
        return root_stage_lake_bin;
    case Stg_hotel_bf_f:
        return root_stage_hotel_bf_f_bin;
    case Stg_hotel_1f_f:
        return root_stage_hotel_1f_f_bin;
    case Stg_hotel_2f_f:
        return root_stage_hotel_2f_f_bin;
    case Stg_hotel_3f_f:
        return root_stage_hotel_3f_f_bin;
    case Stg_hotel_bf_b:
        return root_stage_hotel_bf_b_bin;
    case Stg_hotel_1f_b:
        return root_stage_hotel_1f_b_bin;
    case Stg_hotel_2f_b:
        return root_stage_hotel_2f_b_bin;
    case Stg_hotel_3f_b:
        return root_stage_hotel_3f_b_bin;
    case Stg_hotel_fire:
        return root_stage_hotel_fire_bin;
    case Stg_end_recovery:
        return root_stage_end_recovery_bin;
    case Stg_end_maria:
        return root_stage_end_maria_bin;
    case Stg_end_suicide:
        return root_stage_end_suicide_bin;
    case Stg_end_rebirth:
        return root_stage_end_rebirth_bin;
    case Stg_end_dog:
        return root_stage_end_dog_bin;
    }
}

static struct Stage_Data *StgOverlayGetStageData(STG_NAME stg_name) {
    switch (stg_name) {
    default:
        return &stage_tgs_trial;
    case Stg_toilet:
        return &stage_toilet;
    case Stg_observation:
        return &stage_observation;
    case Stg_forest:
        return &stage_forest;
    case Stg_town_east:
        return &stage_town_east;
    case Stg_apart_e1f:
        return &stage_apart_e1f;
    case Stg_apart_e2f:
        return &stage_apart_e2f;
    case Stg_apart_e3fw:
        return &stage_apart_e3fw;
    case Stg_apart_e3fe:
        return &stage_apart_e3fe;
    case Stg_apart_w1f:
        return &stage_apart_w1f;
    case Stg_apart_w2f:
        return &stage_apart_w2f;
    case Stg_apart_stair:
        return &stage_apart_stair;
    case Stg_apart_out:
        return &stage_apart_out;
    case Stg_town_west:
        return &stage_town_west;
    case Stg_bowling:
        return &stage_bowling;
    case Stg_to_heaven:
        return &stage_to_heaven;
    case Stg_heaven_night:
        return &stage_heaven_night;
    case Stg_hospital_1f_f:
        return &stage_hospital_1f_f;
    case Stg_hospital_2f_f:
        return &stage_hospital_2f_f;
    case Stg_hospital_3f_f:
        return &stage_hospital_3f_f;
    case Stg_hospital_rf_f:
        return &stage_hospital_rf_f;
    case Stg_hospital_1fw_b:
        return &stage_hospital_1fw_b;
    case Stg_hospital_1fe_b:
        return &stage_hospital_1fe_b;
    case Stg_hospital_2f_b:
        return &stage_hospital_2f_b;
    case Stg_hospital_3f_b:
        return &stage_hospital_3f_b;
    case Stg_hospital_bf_b:
        return &stage_hospital_bf_b;
    case Stg_hospital_pass:
        return &stage_hospital_pass;
    case Stg_society:
        return &stage_society;
    case Stg_delusion_2:
        return &stage_delusion_2;
    case Stg_delusion_3:
        return &stage_delusion_3;
    case Stg_prison_n:
        return &stage_prison_n;
    case Stg_prison_s:
        return &stage_prison_s;
    case Stg_prison_bf:
        return &stage_prison_bf;
    case Stg_labyrinth_w:
        return &stage_labyrinth_w;
    case Stg_labyrinth_e:
        return &stage_labyrinth_e;
    case Stg_labyrinth_n:
        return &stage_labyrinth_n;
    case Stg_eddie_boss:
        return &stage_eddie_boss;
    case Stg_lake:
        return &stage_lake;
    case Stg_hotel_bf_f:
        return &stage_hotel_bf_f;
    case Stg_hotel_1f_f:
        return &stage_hotel_1f_f;
    case Stg_hotel_2f_f:
        return &stage_hotel_2f_f;
    case Stg_hotel_3f_f:
        return &stage_hotel_3f_f;
    case Stg_hotel_bf_b:
        return &stage_hotel_bf_b;
    case Stg_hotel_1f_b:
        return &stage_hotel_1f_b;
    case Stg_hotel_2f_b:
        return &stage_hotel_2f_b;
    case Stg_hotel_3f_b:
        return &stage_hotel_3f_b;
    case Stg_hotel_fire:
        return &stage_hotel_fire;
    case Stg_end_recovery:
        return &stage_end_recovery;
    case Stg_end_maria:
        return &stage_end_maria;
    case Stg_end_suicide:
        return &stage_end_suicide;
    case Stg_end_rebirth:
        return &stage_end_rebirth;
    case Stg_end_dog:
        return &stage_end_dog;
    }
}

static union fsFileIndex *last_stage_bin;

/** Loads the overlay of the current stage (playing.stage) if it isn't loaded yet, clears its
 * bss and tells the debugger (MWNotifyOverlayLoaded); then points `stage` at its Stage_Data. */
/*
 * Matches once linked: _ovl_start_addr is an absolute linker symbol (0x01F01E00), which the
 * original ELF does not relocate, so diff_unit.py compares our zeroed lui/addiu as bytes.
 */
void StgOverlay(void) {
    union fsFileIndex *stage_bin;
    void *overlay_load_addr;
    int fid;
    char *addr;
    int size;

    stage_bin = StgOverlayGetFileID(playing.stage);
    if (last_stage_bin != stage_bin) {
        last_stage_bin = stage_bin;
        if (stage_bin) {
            overlay_load_addr = _ovl_start_addr;
            FlushCache(2);
            while (1) {
                fid = FcRead(stage_bin, overlay_load_addr);
                if (fid != -1) {
                    break;
                }
                shSyncVEnd(0);
            }
            fsSync(0, fid);
            addr = overlay_load_addr;
            addr += 0x80;
            addr += ((int *)overlay_load_addr)[3];
            addr += ((int *)overlay_load_addr)[4];
            size = ((int *)overlay_load_addr)[5];
            UtilMemSet(addr, 0, size);
            MWNotifyOverlayLoaded(overlay_load_addr);
        }
    } else {
        verbose(3, "stg_overlay.c:352> stage data is already loaded.\n");
    }
    stage = StgOverlayGetStageData(playing.stage);
}
