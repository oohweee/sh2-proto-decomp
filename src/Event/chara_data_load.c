/*
 * chara_data_load.c: loads character data (models, animations, shadows,
 * clusters) into the character memory area and keeps track of what is loaded.
 *
 * mem_admin[] remembers what is resident; chara_data_use[] marks the used
 * 0x2000-byte blocks of the character area (CHRDATA).
 */
#include "sh2.h"

/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)

/* file sizes are rounded up to whole 0x2000-byte blocks */
#define CDL_BLOCK_SIZE(size) (((size) + 0x1FFF) & ~0x1FFF)

enum {
    Category_null,
    Category_item,
    Category_james,
    Category_weapon,
    Category_standard,
};

static void CharaDataLoadExec(struct CharaData_EntryList *entry_list, int entry_number, int status);
static void CharaDataLoadExecItem(struct CharaData_EntryList *entry_list_p);
static void CharaDataLoadExecJames(struct CharaData_EntryList *entry_list_p);
static void CharaDataLoadExecWeapon(struct CharaData_EntryList *entry_list_p);
static void CharaDataLoadExecStandard(struct CharaData_EntryList *entry_list, int *no, int *entry_no, int status);
static void CharaDataLoadExecStandardSub0(struct CharaData_EntryList *entry_list, int *no, int *entry_no, int status);
static u_long128 *CharaDataFreeSearch(int size);
static int CharaDeleteNoUseOne(void);
static void CharaDataInfoFree(struct CharaData_MemAdmin *admin_p, int del);
static void CharaDataInfoFreeSub(struct CharaData_MemAdmin_One *maop);
static void CharaDataUseFree(u_long128 *adr, int size);
static int SeekMemAdminCtgry(int category);
static int SeekMemAdminKind(int kind);

static struct CharaData_ItemFile bullet_and_drug_file[6] = {
    { 0x703, data_chr_item_x_handbul_mdl },
    { 0x724, data_chr_item_x_shotbul_mdl },
    { 0x723, data_chr_item_x_riflebul_mdl },
    { 0x700, data_chr_item_x_drink_mdl },
    { 0x701, data_chr_item_x_firstaid_mdl },
    { 0x733, data_chr_item_x_ample_mdl },
};

static struct CharaData_WeaponFile weapon_file[10] = {
    { 0, 0x800, data_chr_jms_jms_wpnone_anm, data_chr_wp_wp_kakuzai_mdl, data_chr_wp_rwp_kakuzai_mdl, data_chr_wp_wp_kakuzai_kg1 },
    { 4, 0x801, data_chr_jms_jms_wphand_anm, data_chr_wp_wp_handgun_mdl, data_chr_wp_rwp_handgun_mdl, data_chr_wp_wp_handgun_kg1 },
    { 6, 0x802, data_chr_jms_jms_wpshot_anm, data_chr_wp_wp_shotgun_mdl, data_chr_wp_rwp_shotgun_mdl, data_chr_wp_wp_shotgun_kg1 },
    { 8, 0x803, data_chr_jms_jms_wprifl_anm, data_chr_wp_wp_riflgun_mdl, data_chr_wp_rwp_riflgun_mdl, data_chr_wp_wp_riflgun_kg1 },
    { 10, 0x804, data_chr_jms_jms_wpsp_anm, data_chr_wp_wp_sp_mdl, data_chr_wp_rwp_sp_mdl, data_chr_wp_wp_sp_kg1 },
    { 11, 0x805, data_chr_jms_jms_wpkaku_anm, data_chr_wp_wp_kakuzai_mdl, data_chr_wp_rwp_kakuzai_mdl, data_chr_wp_wp_kakuzai_kg1 },
    { 12, 0x806, data_chr_jms_jms_wppipe_anm, data_chr_wp_wp_pipe_mdl, data_chr_wp_rwp_pipe_mdl, data_chr_wp_wp_pipe_kg1 },
    { 14, 0x807, data_chr_jms_jms_wpcsaw_anm, data_chr_wp_wp_csaw_mdl, data_chr_wp_rwp_csaw_mdl, data_chr_wp_wp_csaw_kg1 },
    { 13, 0x808, data_chr_jms_jms_wpnata_anm, data_chr_wp_wp_nata_mdl, data_chr_wp_rwp_nata_mdl, data_chr_wp_wp_nata_kg1 },
    { -1, 0, NULL, NULL, NULL, NULL },
};

static struct CharaData_StandardList enemy_list[15] = {
    { 0x200, data_chr_scu_scu_mdl, data_chr_scu_scu_anm, data_chr_scu_scu_kg1 },
    { 0x201, data_chr_mkn_mkn_mdl, data_chr_mkn_mkn_anm, data_chr_mkn_mkn_kg1 },
    { 0x202, data_chr_tyu_tyu_mdl, data_chr_tyu_tyu_anm, NULL },
    { 0x20C, data_chr_item_ty2_mdl, NULL, NULL },
    { 0x20D, data_chr_item_ty3_mdl, NULL, NULL },
    { 0x203, data_chr_ike_ike_mdl, data_chr_ike_ike_anm, data_chr_ike_ike_kg1 },
    { 0x204, data_chr_pap_pap_mdl, data_chr_pap_pap_anm, data_chr_pap_pap_kg1 },
    { 0x205, data_chr_edi_lll_edi_mdl, data_chr_edi_lll_edi_anm, data_chr_edi_hhh_edi_kg1 },
    { 0x206, data_chr_bos_bos_mdl, data_chr_bos_bos_anm, NULL },
    { 0x207, data_chr_nse_nse_mdl, data_chr_nse_nse_anm, data_chr_nse_nse_kg1 },
    { 0x20B, data_chr_xoo_xoo_mdl, data_chr_xoo_xoo_anm, data_chr_xoo_xoo_kg1 },
    { 0x208, data_chr_red_red_mdl, data_chr_red_red_anm, data_chr_red_red_kg1 },
    { 0x209, data_chr_oni_oni_mdl, data_chr_oni_oni_anm, data_chr_oni_oni_kg1 },
    { 0x20A, data_chr_arm_arm_mdl, data_chr_arm_arm_anm, data_chr_arm_arm_kg1 },
    { 0, NULL, NULL, NULL },
};

static struct CharaData_StandardList item_list[168] = {
    { 0x414, data_chr_item_b_ami_mdl, NULL, NULL },
    { 0x561, data_chr_item_bab_mdl, NULL, NULL },
    { 0x52B, data_chr_item_baf_mdl, NULL, NULL },
    { 0x405, data_chr_item_i_bear_mdl, NULL, NULL },
    { 0x42F, data_chr_item_b_bol_mdl, data_demo_bp_edi_2_b_bol_anm, data_chr_item_b_bol_kg1 },
    { 0x514, data_chr_item_box_mdl, NULL, NULL },
    { 0x526, data_chr_item_box_01_mdl, NULL, NULL },
    { 0x527, data_chr_item_box_02_mdl, NULL, NULL },
    { 0x300, data_chr_item_c1b_mdl, NULL, NULL },
    { 0x301, data_chr_item_c2b_mdl, NULL, NULL },
    { 0x302, data_chr_item_c3b_mdl, NULL, NULL },
    { 0x303, data_chr_item_c4b_mdl, NULL, NULL },
    { 0x304, data_chr_item_c5b_mdl, NULL, NULL },
    { 0x305, data_chr_item_c6b_mdl, NULL, NULL },
    { 0x306, data_chr_item_c7b_mdl, NULL, NULL },
    { 0x307, data_chr_item_c8b_mdl, NULL, NULL },
    { 0x308, data_chr_item_c9b_mdl, NULL, NULL },
    { 0x309, data_chr_item_cab_mdl, NULL, NULL },
    { 0x30A, data_chr_item_cbb_mdl, NULL, NULL },
    { 0x30B, data_chr_item_ccb_mdl, NULL, NULL },
    { 0x51F, data_chr_item_cda_mdl, NULL, NULL },
    { 0x520, data_chr_item_cdb_mdl, NULL, NULL },
    { 0x422, data_chr_item_b_clo_mdl, NULL, data_chr_item_b_clo_kg1 },
    { 0x521, data_chr_item_cua_mdl, NULL, NULL },
    { 0x522, data_chr_item_cub_mdl, NULL, data_chr_item_cub_kg1 },
    { 0x40A, data_chr_item_b_d00_mdl, NULL, data_chr_item_b_d00_kg1 },
    { 0x419, data_chr_item_b_d01_mdl, NULL, NULL },
    { 0x412, data_chr_item_b_d02_mdl, NULL, data_chr_item_b_d02_kg1 },
    { 0x410, data_chr_item_b_d03_mdl, NULL, data_chr_item_b_d03_kg1 },
    { 0x429, data_chr_item_b_d06_mdl, NULL, data_chr_item_b_d06_kg1 },
    { 0x431, data_chr_item_b_d08_mdl, data_demo_bp_edi_2_b_d08_anm, NULL },
    { 0x428, data_chr_item_d09_mdl, NULL, NULL },
    { 0x438, data_chr_item_b_d10_mdl, data_demo_killer_edi_e_b_d10_anm, data_chr_item_b_d10_kg1 },
    { 0x525, data_chr_item_b_dha_mdl, NULL, NULL },
    { 0x30D, data_chr_item_dhb_mdl, NULL, NULL },
    { 0x30E, data_chr_item_dhv_mdl, NULL, NULL },
    { 0x55B, data_chr_item_dlg_mdl, NULL, NULL },
    { 0x42B, data_chr_item_dm2_mdl, NULL, NULL },
    { 0x426, data_chr_item_b_do4_mdl, NULL, NULL },
    { 0x40C, data_chr_item_b_doo_mdl, NULL, data_chr_item_b_doo_kg1 },
    { 0x42C, data_chr_item_b_dor_mdl, NULL, data_chr_item_b_dor_kg1 },
    { 0x530, data_chr_item_b_ell_mdl, NULL, NULL },
    { 0x556, data_chr_item_evj_mdl, NULL, NULL },
    { 0x519, data_chr_item_evk_mdl, NULL, NULL },
    { 0x523, data_chr_item_fan_mdl, NULL, NULL },
    { 0x506, data_chr_item_gom_mdl, NULL, NULL },
    { 0x434, data_chr_item_i_headphone_mdl, NULL, NULL },
    { 0x518, data_chr_item_hed_mdl, NULL, NULL },
    { 0x55F, data_chr_item_hin_mdl, NULL, NULL },
    { 0x55E, data_chr_item_hou_mdl, NULL, NULL },
    { 0x528, data_chr_item_inu_mdl, NULL, NULL },
    { 0x312, data_chr_item_kab_mdl, NULL, NULL },
    { 0x55A, data_chr_item_kum_mdl, NULL, NULL },
    { 0x517, data_chr_item_lsi_mdl, NULL, NULL },
    { 0x55D, data_chr_item_mal_mdl, NULL, NULL },
    { 0x41A, data_chr_item_i_mdrug_mdl, NULL, NULL },
    { 0x516, data_chr_item_mne_mdl, NULL, NULL },
    { 0x51C, data_chr_item_nak_mdl, NULL, NULL },
    { 0x50C, data_chr_item_nat_mdl, NULL, NULL },
    { 0x52D, data_chr_item_nef_mdl, NULL, NULL },
    { 0x52C, data_chr_item_neo_mdl, NULL, NULL },
    { 0x52E, data_chr_item_nep_mdl, NULL, NULL },
    { 0x421, data_chr_item_b_nik_mdl, data_chr_item_b_nik_anm, data_chr_item_b_nik_kg1 },
    { 0x557, data_chr_item_noa_mdl, NULL, NULL },
    { 0x558, data_chr_item_nor_mdl, NULL, NULL },
    { 0x524, data_chr_item_org_mdl, NULL, NULL },
    { 0x51B, data_chr_item_b_ori_mdl, data_demo_kaiten_b_ori_anm, data_chr_item_b_ori_kg1 },
    { 0x411, data_chr_item_b_pia_mdl, NULL, NULL },
    { 0x42E, data_chr_item_b_piz_mdl, data_demo_bp_edi_2_b_piz_anm, data_chr_item_b_piz_kg1 },
    { 0x55C, data_chr_item_piz_mdl, NULL, NULL },
    { 0x415, data_chr_item_pxx_mdl, data_chr_item_pxx_anm, NULL },
    { 0x40B, data_chr_item_b_rei_mdl, data_demo_reizouko_b_rei_anm, data_chr_item_b_rei_kg1 },
    { 0x41C, data_chr_item_b_rop_mdl, data_demo_kubitsuri_b_rop_anm, NULL },
    { 0x532, data_chr_item_3sk_mdl, NULL, NULL },
    { 0x53C, data_chr_item_s01_mdl, NULL, NULL },
    { 0x53D, data_chr_item_s02_mdl, NULL, NULL },
    { 0x53E, data_chr_item_s03_mdl, NULL, NULL },
    { 0x53F, data_chr_item_s04_mdl, NULL, NULL },
    { 0x540, data_chr_item_s05_mdl, NULL, NULL },
    { 0x541, data_chr_item_s06_mdl, NULL, NULL },
    { 0x542, data_chr_item_s07_mdl, NULL, NULL },
    { 0x543, data_chr_item_s08_mdl, NULL, NULL },
    { 0x544, data_chr_item_s09_mdl, NULL, NULL },
    { 0x545, data_chr_item_s0a_mdl, NULL, NULL },
    { 0x546, data_chr_item_s0b_mdl, NULL, NULL },
    { 0x547, data_chr_item_s0c_mdl, NULL, NULL },
    { 0x548, data_chr_item_s0d_mdl, NULL, NULL },
    { 0x549, data_chr_item_s0e_mdl, NULL, NULL },
    { 0x54A, data_chr_item_s0f_mdl, NULL, NULL },
    { 0x54B, data_chr_item_s0g_mdl, NULL, NULL },
    { 0x54C, data_chr_item_s0h_mdl, NULL, NULL },
    { 0x54D, data_chr_item_s0i_mdl, NULL, NULL },
    { 0x54E, data_chr_item_s0j_mdl, NULL, NULL },
    { 0x54F, data_chr_item_s0k_mdl, NULL, NULL },
    { 0x550, data_chr_item_s0l_mdl, NULL, NULL },
    { 0x551, data_chr_item_s0m_mdl, NULL, NULL },
    { 0x552, data_chr_item_s0n_mdl, NULL, NULL },
    { 0x553, data_chr_item_s0o_mdl, NULL, NULL },
    { 0x554, data_chr_item_s0p_mdl, NULL, NULL },
    { 0x437, data_chr_item_b_sti_mdl, NULL, data_chr_item_b_sti_kg1 },
    { 0x427, data_chr_item_b_tan_mdl, data_demo_kakushi_b_tan_anm, data_chr_item_b_tan_kg1 },
    { 0x409, data_chr_item_b_tel_mdl, NULL, data_chr_item_b_tel_kg1 },
    { 0x51A, data_chr_item_tom_mdl, NULL, NULL },
    { 0x30C, data_chr_item_tlr_mdl, NULL, NULL },
    { 0x50E, data_chr_item_tvc_mdl, NULL, data_chr_item_tvc_kg1 },
    { 0x310, data_chr_item_ura0_mdl, NULL, NULL },
    { 0x311, data_chr_item_ura1_mdl, NULL, NULL },
    { 0x30F, data_chr_item_ura2_mdl, NULL, NULL },
    { 0x313, data_chr_item_ura3_mdl, NULL, NULL },
    { 0x72B, data_chr_item_x_battery_mdl, NULL, NULL },
    { 0x727, data_chr_item_x_bear_mdl, NULL, NULL },
    { 0x720, data_chr_item_x_canopen_mdl, NULL, NULL },
    { 0x71A, data_chr_item_x_cinderella_mdl, NULL, NULL },
    { 0x71F, data_chr_item_x_coinelder_mdl, NULL, NULL },
    { 0x71C, data_chr_item_x_coinprisoner_mdl, NULL, NULL },
    { 0x71D, data_chr_item_x_coinsnake_mdl, NULL, NULL },
    { 0x71E, data_chr_item_x_cup_mdl, NULL, NULL },
    { 0x741, data_chr_item_x_eggred_mdl, NULL, NULL },
    { 0x740, data_chr_item_x_eggrust_mdl, NULL, NULL },
    { 0x702, data_chr_item_x_handgun_mdl, NULL, NULL },
    { 0x73A, data_chr_item_x_horse_mdl, NULL, NULL },
    { 0x705, data_chr_item_x_jlight_mdl, NULL, NULL },
    { 0x72D, data_chr_item_x_juice_mdl, NULL, NULL },
    { 0x72F, data_chr_item_x_key202_mdl, NULL, NULL },
    { 0x73D, data_chr_item_x_key312_mdl, NULL, NULL },
    { 0x747, data_chr_item_x_key3f_mdl, NULL, NULL },
    { 0x735, data_chr_item_x_keybar_mdl, NULL, NULL },
    { 0x72A, data_chr_item_x_keybase_mdl, NULL, NULL },
    { 0x72E, data_chr_item_x_keyclock_mdl, NULL, NULL },
    { 0x732, data_chr_item_x_keycourt_mdl, NULL, NULL },
    { 0x70A, data_chr_item_x_keyelevator_mdl, NULL, NULL },
    { 0x72C, data_chr_item_x_keyemerg_mdl, NULL, NULL },
    { 0x73E, data_chr_item_x_keyemploy_mdl, NULL, NULL },
    { 0x742, data_chr_item_x_keyfalse_mdl, NULL, NULL },
    { 0x736, data_chr_item_x_keyfish_mdl, NULL, NULL },
    { 0x722, data_chr_item_x_keygate_mdl, NULL, NULL },
    { 0x71B, data_chr_item_x_keyhos_mdl, NULL, NULL },
    { 0x731, data_chr_item_x_keylyne_mdl, NULL, NULL },
    { 0x730, data_chr_item_x_keynorth_mdl, NULL, NULL },
    { 0x707, data_chr_item_x_keypurple_mdl, NULL, NULL },
    { 0x728, data_chr_item_x_keyrapis_mdl, NULL, NULL },
    { 0x729, data_chr_item_x_keyroof_mdl, NULL, NULL },
    { 0x73F, data_chr_item_x_keyspiral_mdl, NULL, NULL },
    { 0x734, data_chr_item_x_lightbulb_mdl, NULL, NULL },
    { 0x73C, data_chr_item_x_lighter_mdl, NULL, NULL },
    { 0x721, data_chr_item_x_lostmemory_mdl, NULL, NULL },
    { 0x719, data_chr_item_x_mermaid_mdl, NULL, NULL },
    { 0x709, data_chr_item_x_needle_mdl, NULL, NULL },
    { 0x717, data_chr_item_x_oil_mdl, NULL, NULL },
    { 0x739, data_chr_item_x_plate_female_mdl, NULL, NULL },
    { 0x738, data_chr_item_x_plate_kick_mdl, NULL, NULL },
    { 0x737, data_chr_item_x_plate_pig_mdl, NULL, NULL },
    { 0x714, data_chr_item_x_plier_mdl, NULL, NULL },
    { 0x713, data_chr_item_x_redrelig_mdl, NULL, NULL },
    { 0x712, data_chr_item_x_ringcopper_mdl, NULL, NULL },
    { 0x711, data_chr_item_x_ringlead_mdl, NULL, NULL },
    { 0x718, data_chr_item_x_snow_mdl, NULL, NULL },
    { 0x710, data_chr_item_x_spanner_mdl, NULL, NULL },
    { 0x70F, data_chr_item_x_thinner_mdl, NULL, NULL },
    { 0x70E, data_chr_item_x_video_mdl, NULL, NULL },
    { 0x70D, data_chr_item_x_waxdoll_mdl, NULL, NULL },
    { 0x746, data_chr_item_x_wp_csaw_mdl, NULL, data_chr_item_x_wp_csaw_kg1 },
    { 0x745, data_chr_item_x_wp_pipe_mdl, NULL, data_chr_item_x_wp_pipe_kg1 },
    { 0x73B, data_chr_item_x_wp_riflgun_mdl, NULL, data_chr_item_x_wp_riflgun_kg1 },
    { 0x743, data_chr_item_x_wp_shotgun_mdl, NULL, data_chr_item_x_wp_shotgun_kg1 },
    { 0x744, data_chr_item_x_wp_sp_mdl, NULL, data_chr_item_x_wp_sp_kg1 },
    { 0x534, data_chr_item_xag_mdl, NULL, data_chr_item_xag_kg1 },
    { 0, NULL, NULL, NULL },
};

static u_long128 *chara_adress;
static struct CharaData_MemAdmin mem_admin[32];
static unsigned char chara_data_use[1024];
static union fsFileIndex *stage_anim;
static struct CharaData_Extra chara_data_extra[8];

/** Forgets all loaded character data and empties the character area. */
void CharaDataLoadInit(void) {
    shQzero(mem_admin, sizeof(mem_admin));
    shQzero(chara_data_use, sizeof(chara_data_use));
    stage_anim = NULL;
    chara_adress = CHRDATA;
}

/** Loads the models of the bullet and medicine items. */
void CharaDataLoadItem(void) {
    struct CharaData_EntryList entry_list[6];
    int i;

    shQzero(entry_list, sizeof(entry_list));
    for (i = 0; i < 6; i++) {
        entry_list[i].category = Category_item;
        entry_list[i].kind = bullet_and_drug_file[i].kind;
        entry_list[i].model.file = bullet_and_drug_file[i].file;
    }
    CharaDataLoadExec(entry_list, 6, 0);
}

/** Loads James's animation for the equipped weapon and the weapon's model. */
void CharaDataLoadWeapon(void) {
    struct CharaData_EntryList entry_list[3];
    int i;

    for (i = 0; weapon_file[i].equip != -1; i++) {
        if (item.equip == weapon_file[i].equip) {
            break;
        }
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 639
    assert_dw(weapon_file[i].equip != -1);
    shQzero(entry_list, sizeof(entry_list));
    entry_list[0].category = Category_james;
    if (stage->pc_model == 1) {
        entry_list[0].kind = 0x101;
    } else {
        entry_list[0].kind = 0x100;
    }
    entry_list[0].animation.file = weapon_file[i].james_anim;
    entry_list[1].category = Category_weapon;
    entry_list[1].kind = weapon_file[i].kind;
    entry_list[1].model.file = weapon_file[i].weapon_model;
    entry_list[1].shadow.file = weapon_file[i].shadow_model;
    if ((Sh2sys.main_status >> 4) & 1) {
        entry_list[2].category = Category_standard;
        entry_list[2].kind = weapon_file[i].kind + 0x20;
        entry_list[2].model.file = weapon_file[i].weapon_model_r;
        CharaDataLoadExec(entry_list, 3, 0);
    } else {
        CharaDataLoadExec(entry_list, 2, 0);
    }
}

/** Loads the model, animation and shadow of an enemy kind (if it is in the enemy list).
 * @param kind character kind */
void CharaDataLoadEnemy(int kind) {
    struct CharaData_EntryList entry_list;
    int i;

    for (i = 0; enemy_list[i].kind != 0; i++) {
        if (kind == enemy_list[i].kind) {
            break;
        }
    }
    if (enemy_list[i].kind != 0) {
        shQzero(&entry_list, sizeof(entry_list));
        entry_list.category = Category_standard;
        entry_list.kind = enemy_list[i].kind;
        entry_list.model.file = enemy_list[i].model;
        entry_list.animation.file = enemy_list[i].animation;
        entry_list.shadow.file = enemy_list[i].shadow;
        CharaDataLoadExec(&entry_list, 1, 0);
    }
}

/** Loads James (the stage's player model, with the animation for the equipped weapon) and
 * the weapon's model. */
void CharaDataLoadStage(void) {
    struct CharaData_EntryList entry_list[2];
    int i;

    for (i = 0; weapon_file[i].equip != -1; i++) {
        if (item.equip == weapon_file[i].equip) {
            break;
        }
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 699
    assert_dw(weapon_file[i].equip != -1);
    shQzero(entry_list, sizeof(entry_list));
    entry_list[0].category = Category_james;
    if (stage->pc_model == 1) {
        entry_list[0].kind = 0x101;
        entry_list[0].model.file = data_chr_jms_hll_jms_mdl;
    } else {
        entry_list[0].kind = 0x100;
        entry_list[0].model.file = data_chr_jms_lll_jms_mdl;
    }
    entry_list[0].animation.file = weapon_file[i].james_anim;
    entry_list[0].shadow.file = data_chr_jms_hhh_jms_kg1;
    entry_list[1].category = Category_weapon;
    entry_list[1].kind = weapon_file[i].kind;
    entry_list[1].model.file = weapon_file[i].weapon_model;
    entry_list[1].animation.file = data_chr_wp_jms_weapon_anm;
    entry_list[1].shadow.file = weapon_file[i].shadow_model;
    CharaDataLoadExec(entry_list, 2, 0);
}

/** Loads the character data of the models, items and enemies of a room, ageing (and
 * eventually freeing) what isn't needed any more.
 * @param room room number */
void CharaDataLoadRoom(int room) {
    struct CharaData_EntryList entry_list[32];
    struct Enemy_List *ep;
    struct Model_List *mp;
    int entry_number;
    int i;
    int j;

    if (stage->chara_data_clear != NULL && stage->chara_data_clear()) {
        CharaDataDeleteAll();
    }
    CharaDataDeleteExtra();
    entry_number = 0;
    shQzero(entry_list, sizeof(entry_list));
    for (i = 0; i < 32; i++) {
        if (mem_admin[i].priority == 1) {
            mem_admin[i].priority = 3;
        } else if (mem_admin[i].priority > 1) {
            mem_admin[i].priority++;
            if (mem_admin[i].priority > 0x40) {
                mem_admin[i].priority = 0x40;
            }
        }
    }
    if (GAME_FLAG(15)) {
        entry_list[entry_number].category = Category_standard;
        entry_list[entry_number].kind = 0x105;
        entry_list[entry_number].model.file = data_chr_mar_lll_mar_mdl;
        entry_list[entry_number].animation.file = data_chr_mar_lll_mar_anm;
        entry_list[entry_number].shadow.file = data_chr_mar_hhh_mar_kg1;
        entry_number++;
    }
    for (ep = stage->en_list; ep != NULL && ep->kind != 0; ep++) {
        if (!CharaAdminEnemyEntryCheck(ep, room)) {
            continue;
        }
        for (i = 0; enemy_list[i].kind != 0; i++) {
            if (ep->kind == enemy_list[i].kind) {
                break;
            }
        }
        if (enemy_list[i].kind == 0) {
            continue;
        }
        for (j = 0; j < entry_number; j++) {
            if (ep->kind == entry_list[j].kind) {
                break;
            }
        }
        if (entry_number > 0 && j != entry_number) {
            continue;
        }
        entry_list[entry_number].category = Category_standard;
        entry_list[entry_number].kind = enemy_list[i].kind;
        entry_list[entry_number].model.file = enemy_list[i].model;
        entry_list[entry_number].animation.file = enemy_list[i].animation;
        entry_list[entry_number].shadow.file = enemy_list[i].shadow;
        entry_number++;
    }
    for (mp = stage->mdl_list; mp != NULL && mp->kind != 0; mp++) {
        if (mp->kind >> 8 == 3) {
            BgCharaRelocateSet(stage->glb_crd, mp->pos[0], mp->pos[2], mp->kind);
            continue;
        }
        if (mp->flag_off && GAME_FLAG(mp->flag_off)) {
            continue;
        }
        if (room != RoomName(0, mp->pos[0], mp->pos[2])) {
            continue;
        }
        for (i = 0; item_list[i].kind != 0; i++) {
            if (mp->kind == item_list[i].kind) {
                break;
            }
        }
        /* Matching: the assert bakes its original line number into the object. */
#line 795
        assert(item_list[i].kind);
        for (j = 0; j < entry_number; j++) {
            if (mp->kind == entry_list[j].kind) {
                break;
            }
        }
        if (entry_number > 0 && j != entry_number) {
            continue;
        }
        entry_list[entry_number].category = Category_standard;
        entry_list[entry_number].kind = item_list[i].kind;
        entry_list[entry_number].model.file = item_list[i].model;
        entry_list[entry_number].animation.file = item_list[i].animation;
        entry_list[entry_number].shadow.file = item_list[i].shadow;
        entry_number++;
    }
    if (room == 1 || room == 0x24) {
        entry_list[entry_number].category = Category_standard;
        if (stage->pc_model == 1) {
            entry_list[entry_number].kind = 0x121;
            entry_list[entry_number].model.file = data_chr_jms_rhll_jms_mdl;
        } else {
            entry_list[entry_number].kind = 0x120;
            entry_list[entry_number].model.file = data_chr_jms_rlll_jms_mdl;
        }
        entry_number++;
        Sh2sys.main_status |= 0x10;
    } else {
        Sh2sys.main_status &= ~0x10;
    }
    CharaDataLoadExec(entry_list, entry_number, 0);
}

/** Loads the character data of a demo's character list.
 * @param dlp the list, ended by kind 0 (may be NULL)
 * @param status load flags (4: in the background) */
void CharaDataLoadDemo(struct CharaData_DemoList *dlp, int status) {
    struct CharaData_EntryList entry_list[32];
    int entry_number;
    int i;

    entry_number = 0;
    shQzero(entry_list, sizeof(entry_list));
    for (i = 0; dlp != NULL && dlp[i].kind != 0; i++) {
        entry_list[entry_number].category = Category_standard;
        entry_list[entry_number].kind = dlp[i].kind;
        entry_list[entry_number].model.file = dlp[i].model;
        entry_list[entry_number].animation.file = dlp[i].animation;
        entry_list[entry_number].shadow.file = dlp[i].shadow;
        entry_list[entry_number].cluster.file = dlp[i].cluster;
        entry_number++;
    }
    CharaDataLoadExec(entry_list, entry_number, status);
}

/** Cancels the model data load and frees the characters of a demo list.
 * @param dlp the list, ended by kind 0 (may be NULL) */
void CharaDataLoadCancel(struct CharaData_DemoList *dlp) {
    sh2gfw_Cancel_LOADCharaModelData();
    if (dlp == NULL) {
        return;
    }
    while (dlp->kind) {
        CharaDataDeleteOne(dlp->kind);
        dlp++;
    }
}

static void CharaDataLoadExec(struct CharaData_EntryList *entry_list, int entry_number, int status) {
    struct SubCharacter *scp;
    struct chr_mge_files load_files[32];
    int i;
    int j;
    int k;

    for (i = 0; i < entry_number; i++) {
        for (j = 0; j < 32; j++) {
            if (entry_list[i].kind == mem_admin[j].kind) {
                if (mem_admin[j].priority > 1) {
                    mem_admin[j].priority = 2;
                }
                break;
            }
        }
    }
    for (i = 0; i < entry_number; i++) {
        switch (entry_list[i].category) {
        case Category_item:
            CharaDataLoadExecItem(&entry_list[i]);
            break;
        case Category_james:
            CharaDataLoadExecJames(&entry_list[i]);
            break;
        case Category_weapon:
            CharaDataLoadExecWeapon(&entry_list[i]);
            break;
        case Category_standard:
            CharaDataLoadExecStandard(entry_list, &i, &entry_number, status);
            break;
        }
    }
    if (status & 4) {
        if (entry_list[i].kind != entry_list[i].delete) {
            sh2gfw_Delete_Model_from_CharaID(entry_list[i].delete);
        }
        for (i = 0; i < entry_number; i++) {
            if (entry_list[i].model.load && entry_list[i].model.file) {
                LoadBgEventFileLoad(entry_list[i].model.file, entry_list[i].model.adress);
            }
            if (entry_list[i].animation.load && entry_list[i].animation.file) {
                LoadBgEventFileLoad(entry_list[i].animation.file, entry_list[i].animation.adress);
            }
            if (entry_list[i].cluster.load && entry_list[i].cluster.file) {
                LoadBgEventFileLoad(entry_list[i].cluster.file, entry_list[i].cluster.adress);
            }
            if (entry_list[i].shadow.load && entry_list[i].shadow.file) {
                LoadBgEventFileLoad(entry_list[i].shadow.file, entry_list[i].shadow.adress);
            }
        }
    } else {
        shQzero(load_files, sizeof(load_files));
        j = 0;
        for (i = 0; i < entry_number; i++) {
            if (entry_list[i].category != Category_standard || entry_list[i].model.load ||
                entry_list[i].animation.load || entry_list[i].shadow.load || entry_list[i].cluster.load) {
                if (entry_list[i].model.load) {
                    load_files[j].model_fid = entry_list[i].model.file;
                }
                if (entry_list[i].animation.load) {
                    load_files[j].anime_fid = entry_list[i].animation.file;
                }
                if (entry_list[i].shadow.load) {
                    load_files[j].shadow_fid = entry_list[i].shadow.file;
                }
                if (entry_list[i].cluster.load) {
                    load_files[j].cluster_fid = entry_list[i].cluster.file;
                }
                load_files[j].mid = entry_list[i].kind;
                j++;
            }
        }
        if (j != 0) {
            load_files[j].model_fid = load_files[j].anime_fid = load_files[j].shadow_fid =
                load_files[j].cluster_fid = NULL;
            load_files[j].mid = -1;
            sh2gfw_LoadInit_CharaModelData(load_files);
        }
        k = 0;
        for (i = 0; i < entry_number; i++) {
            if (entry_list[i].category != Category_standard || entry_list[i].model.load ||
                entry_list[i].animation.load || entry_list[i].shadow.load || entry_list[i].cluster.load) {
                sh2gfw_LoadMemorySet_CharaModelData(&load_files[k], &entry_list[i].model.adress,
                                                    &entry_list[i].animation.adress,
                                                    &entry_list[i].cluster.adress, &entry_list[i].shadow.adress);
                k++;
            }
        }
        if (j != 0) {
            sh2gfw_LOAD_CharaModelData();
        }
        if (!(status & 1)) {
            fsSync(0, -1);
            for (i = 0; i < entry_number; i++) {
                if (entry_list[i].delete) {
                    sh2gfw_Delete_Model_from_CharaID(entry_list[i].delete);
                }
            }
            sh2gfw_SyncInit_ChacterModelData();
            for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = scp->next) {
                shCharacter_Manage_SetDataAdresss(scp);
            }
        }
    }
}

static void CharaDataLoadExecItem(struct CharaData_EntryList *entry_list_p) {
    int size;
    int i;

    i = SeekMemAdminKind(entry_list_p->kind);
    if (i == 32) {
        i = SeekMemAdminCtgry(Category_null);
        mem_admin[i].category = entry_list_p->category;
        mem_admin[i].priority = 0;
        mem_admin[i].kind = entry_list_p->kind;
        size = CDL_BLOCK_SIZE(FcGetFileSize(entry_list_p->model.file));
        entry_list_p->model.adress = CharaDataFreeSearch(size);
        entry_list_p->model.load = 1;
        mem_admin[i].model.file = entry_list_p->model.file;
        mem_admin[i].model.adress = entry_list_p->model.adress;
        mem_admin[i].model.size = size;
    } else {
        entry_list_p->delete = entry_list_p->kind;
        entry_list_p->model.adress = mem_admin[i].model.adress;
    }
}

static void CharaDataLoadExecJames(struct CharaData_EntryList *entry_list_p) {
    static union fsFileIndex *stage_anim_list[53] = {
        NULL,
        data_chr_jms_jms_stg_tgs_trial_anm,
        data_chr_jms_jms_stg_toilet_anm,
        data_chr_jms_jms_stg_ovservation_anm,
        data_chr_jms_jms_stg_forest_anm,
        data_chr_jms_jms_stg_town_east_anm,
        data_chr_jms_jms_stg_apart_e1f_anm,
        data_chr_jms_jms_stg_apart_e2f_anm,
        data_chr_jms_jms_stg_apart_e3fw_anm,
        data_chr_jms_jms_stg_apart_e3fe_anm,
        data_chr_jms_jms_stg_apart_w1f_anm,
        data_chr_jms_jms_stg_apart_w2f_anm,
        data_chr_jms_jms_stg_apart_stair_anm,
        data_chr_jms_jms_stg_apart_out_anm,
        data_chr_jms_jms_stg_town_west_anm,
        data_chr_jms_jms_stg_bowling_anm,
        data_chr_jms_jms_stg_to_heaven_anm,
        data_chr_jms_jms_stg_heaven_night_anm,
        data_chr_jms_jms_stg_hospital_1f_f_anm,
        data_chr_jms_jms_stg_hospital_2f_f_anm,
        data_chr_jms_jms_stg_hospital_3f_f_anm,
        data_chr_jms_jms_stg_hospital_rf_f_anm,
        data_chr_jms_jms_stg_hospital_1fw_b_anm,
        data_chr_jms_jms_stg_hospital_1fe_b_anm,
        data_chr_jms_jms_stg_hospital_2f_b_anm,
        data_chr_jms_jms_stg_hospital_3f_b_anm,
        data_chr_jms_jms_stg_hospital_bf_b_anm,
        data_chr_jms_jms_stg_hospital_pass_anm,
        data_chr_jms_jms_stg_society_anm,
        data_chr_jms_jms_stg_delusion_2_anm,
        data_chr_jms_jms_stg_delusion_3_anm,
        data_chr_jms_jms_stg_prison_n_anm,
        data_chr_jms_jms_stg_prison_s_anm,
        data_chr_jms_jms_stg_prison_bf_anm,
        data_chr_jms_jms_stg_labyrinth_w_anm,
        data_chr_jms_jms_stg_labyrinth_e_anm,
        data_chr_jms_jms_stg_labyrinth_n_anm,
        data_chr_jms_jms_stg_eddie_boss_anm,
        data_chr_jms_jms_stg_lake_anm,
        data_chr_jms_jms_stg_hotel_bf_f_anm,
        data_chr_jms_jms_stg_hotel_1f_f_anm,
        data_chr_jms_jms_stg_hotel_2f_f_anm,
        data_chr_jms_jms_stg_hotel_3f_f_anm,
        data_chr_jms_jms_stg_hotel_bf_b_anm,
        data_chr_jms_jms_stg_hotel_1f_b_anm,
        data_chr_jms_jms_stg_hotel_2f_b_anm,
        data_chr_jms_jms_stg_hotel_3f_b_anm,
        data_chr_jms_jms_stg_hotel_fire_anm,
        data_chr_jms_jms_stg_end_recovery_anm,
        data_chr_jms_jms_stg_end_maria_anm,
        data_chr_jms_jms_stg_end_suicide_anm,
        data_chr_jms_jms_stg_end_rebirth_anm,
        data_chr_jms_jms_stg_end_dog_anm,
    };
    int size;
    int i;
    int j;

    i = SeekMemAdminCtgry(Category_james);
    if (i == 32) {
        i = SeekMemAdminCtgry(Category_null);
        mem_admin[i].category = Category_james;
        mem_admin[i].priority = 0;
        mem_admin[i].kind = entry_list_p->kind;
        size = 0x138000;
        entry_list_p->model.adress = CharaDataFreeSearch(size);
        entry_list_p->model.load = 1;
        mem_admin[i].model.file = entry_list_p->model.file;
        mem_admin[i].model.adress = entry_list_p->model.adress;
        mem_admin[i].model.size = size;
        size = 0xE2000;
        entry_list_p->animation.adress = CharaDataFreeSearch(size);
        entry_list_p->animation.load = 1;
        mem_admin[i].animation.file = entry_list_p->animation.file;
        mem_admin[i].animation.adress = entry_list_p->animation.adress;
        mem_admin[i].animation.size = size;
        size = 0x4000;
        entry_list_p->shadow.adress = CharaDataFreeSearch(size);
        entry_list_p->shadow.load = 1;
        mem_admin[i].shadow.file = entry_list_p->shadow.file;
        mem_admin[i].shadow.adress = entry_list_p->shadow.adress;
        mem_admin[i].shadow.size = size;
    } else {
        entry_list_p->delete = mem_admin[i].kind;
        mem_admin[i].kind = entry_list_p->kind;
        entry_list_p->model.adress = mem_admin[i].model.adress;
        if (entry_list_p->model.file != NULL && entry_list_p->model.file != mem_admin[i].model.file) {
            entry_list_p->model.load = 1;
            sh2gfw_Delete_Model_from_CharaID(entry_list_p->delete);
            mem_admin[i].model.file = entry_list_p->model.file;
        }
        entry_list_p->animation.adress = mem_admin[i].animation.adress;
        if (entry_list_p->animation.file != NULL &&
            entry_list_p->animation.file != mem_admin[i].animation.file) {
            entry_list_p->animation.load = 1;
            mem_admin[i].animation.file = entry_list_p->animation.file;
        }
        entry_list_p->shadow.adress = mem_admin[i].shadow.adress;
        if (entry_list_p->shadow.file != NULL && entry_list_p->shadow.file != mem_admin[i].shadow.file) {
            entry_list_p->shadow.load = 1;
            mem_admin[i].shadow.file = entry_list_p->shadow.file;
        }
    }
    if (stage_anim_list[playing.stage] != stage_anim) {
        stage_anim = stage_anim_list[playing.stage];
        FcRead(stage_anim, (char *)entry_list_p->animation.adress + 0x84000);
        jms_stage_anim = stage->stg_anim_info;
    }
}

static void CharaDataLoadExecWeapon(struct CharaData_EntryList *entry_list_p) {
    int size;
    int i;
    int j;

    i = SeekMemAdminCtgry(Category_weapon);
    if (i == 32) {
        i = SeekMemAdminCtgry(Category_null);
        mem_admin[i].category = Category_weapon;
        mem_admin[i].priority = 0;
        mem_admin[i].kind = entry_list_p->kind;
        size = 0xC000;
        entry_list_p->model.adress = CharaDataFreeSearch(size);
        entry_list_p->model.load = 1;
        mem_admin[i].model.file = entry_list_p->model.file;
        mem_admin[i].model.adress = entry_list_p->model.adress;
        mem_admin[i].model.size = size;
        size = 0x2000;
        entry_list_p->animation.adress = CharaDataFreeSearch(size);
        entry_list_p->animation.load = 1;
        mem_admin[i].animation.file = entry_list_p->animation.file;
        mem_admin[i].animation.adress = entry_list_p->animation.adress;
        mem_admin[i].animation.size = size;
        size = 0x2000;
        entry_list_p->shadow.adress = CharaDataFreeSearch(size);
        entry_list_p->shadow.load = 1;
        mem_admin[i].shadow.file = entry_list_p->shadow.file;
        mem_admin[i].shadow.adress = entry_list_p->shadow.adress;
        mem_admin[i].shadow.size = size;
    } else {
        entry_list_p->delete = mem_admin[i].kind;
        entry_list_p->model.adress = mem_admin[i].model.adress;
        if (entry_list_p->model.file != NULL && entry_list_p->model.file != mem_admin[i].model.file) {
            entry_list_p->model.load = 1;
            mem_admin[i].model.file = entry_list_p->model.file;
        }
        entry_list_p->animation.adress = mem_admin[i].animation.adress;
        entry_list_p->shadow.adress = mem_admin[i].shadow.adress;
        if (entry_list_p->shadow.file != NULL && entry_list_p->shadow.file != mem_admin[i].shadow.file) {
            entry_list_p->shadow.load = 1;
            mem_admin[i].shadow.file = entry_list_p->shadow.file;
        }
        mem_admin[i].kind = entry_list_p->kind;
    }
}

static void CharaDataLoadExecStandard(struct CharaData_EntryList *entry_list, int *no, int *entry_no, int status) {
    struct CharaData_EntryList *entry_list_p;
    struct CharaData_EntryList_One *elop;
    struct CharaData_MemAdmin *map;
    struct CharaData_MemAdmin_One *maop;
    u_long128 *adr;
    int i;
    int j;

    entry_list_p = &entry_list[*no];
    for (i = 0; i < 32; i++) {
        if (entry_list_p->kind == mem_admin[i].kind) {
            if (mem_admin[i].model.file == entry_list_p->model.file &&
                mem_admin[i].animation.file == entry_list_p->animation.file &&
                mem_admin[i].shadow.file == entry_list_p->shadow.file &&
                mem_admin[i].cluster.file == entry_list_p->cluster.file) {
                mem_admin[i].priority = 1;
                return;
            }
            break;
        }
    }
    if (i == 32) {
        map = NULL;
    } else {
        entry_list_p->delete = mem_admin[i].kind;
        map = &mem_admin[i];
    }
    for (j = 0; j < 4; j++) {
        maop = NULL;
        switch (j) {
        default:
        case 0:
            elop = &entry_list_p->model;
            if (map != NULL) {
                maop = &map->model;
            }
            break;
        case 1:
            elop = &entry_list_p->animation;
            if (map != NULL) {
                maop = &map->animation;
            }
            break;
        case 2:
            elop = &entry_list_p->shadow;
            if (map != NULL) {
                maop = &map->shadow;
            }
            break;
        case 3:
            elop = &entry_list_p->cluster;
            if (map != NULL) {
                maop = &map->cluster;
            }
            break;
        }
        if (elop->file == NULL) {
            if (maop != NULL && maop->file != NULL) {
                entry_list_p->delete = map->kind;
                elop->load = 1;
                CharaDataInfoFree(map, 1 << j);
            }
            continue;
        }
        if (maop != NULL) {
            if (maop->file == elop->file) {
                elop->adress = maop->adress;
                continue;
            }
            entry_list_p->delete = map->kind;
            elop->load = 1;
            CharaDataInfoFree(map, 1 << j);
        }
        while (1) {
            adr = CharaDataFreeSearch(CDL_BLOCK_SIZE(FcGetFileSize(elop->file)));
            if (adr != NULL) {
                break;
            }
            if (CharaDeleteNoUseOne()) {
                continue;
            }
            /* Matching: the assert bakes its original line number into the object. */
#line 1270
            assert_dw(!( status & ( 1 << 1 ) ));
            entry_list_p->delete = 0;
            CharaDataUseFree(entry_list_p->model.adress, CDL_BLOCK_SIZE(FcGetFileSize(entry_list_p->model.file)));
            CharaDataUseFree(entry_list_p->animation.adress,
                             CDL_BLOCK_SIZE(FcGetFileSize(entry_list_p->animation.file)));
            CharaDataUseFree(entry_list_p->shadow.adress, CDL_BLOCK_SIZE(FcGetFileSize(entry_list_p->shadow.file)));
            CharaDataUseFree(entry_list_p->cluster.adress,
                             CDL_BLOCK_SIZE(FcGetFileSize(entry_list_p->cluster.file)));
            CharaDataLoadExecStandardSub0(entry_list, no, entry_no, status);
            return;
        }
        elop->adress = adr;
        elop->load = 1;
    }
    if (map == NULL) {
        i = SeekMemAdminCtgry(Category_null);
        map = &mem_admin[i];
        map->category = entry_list_p->category;
        map->kind = entry_list_p->kind;
    }
    map->priority = 1;
    if (entry_list_p->model.load) {
        mem_admin[i].model.file = entry_list_p->model.file;
        mem_admin[i].model.adress = entry_list_p->model.adress;
        mem_admin[i].model.size = CDL_BLOCK_SIZE(FcGetFileSize(entry_list_p->model.file));
    }
    if (entry_list_p->animation.load) {
        mem_admin[i].animation.file = entry_list_p->animation.file;
        mem_admin[i].animation.adress = entry_list_p->animation.adress;
        mem_admin[i].animation.size = CDL_BLOCK_SIZE(FcGetFileSize(entry_list_p->animation.file));
    }
    if (entry_list_p->shadow.load) {
        mem_admin[i].shadow.file = entry_list_p->shadow.file;
        mem_admin[i].shadow.adress = entry_list_p->shadow.adress;
        mem_admin[i].shadow.size = CDL_BLOCK_SIZE(FcGetFileSize(entry_list_p->shadow.file));
    }
    if (entry_list_p->cluster.load) {
        mem_admin[i].cluster.file = entry_list_p->cluster.file;
        mem_admin[i].cluster.adress = entry_list_p->cluster.adress;
        mem_admin[i].cluster.size = CDL_BLOCK_SIZE(FcGetFileSize(entry_list_p->cluster.file));
    }
}

static void CharaDataLoadExecStandardSub0(struct CharaData_EntryList *entry_list, int *no, int *entry_no, int status) {
    struct CharaData_EntryList entry_list_bak[32];
    int i;
    int j;

    memcpy(entry_list_bak, &entry_list[*no], (*entry_no - *no + 1) * sizeof(struct CharaData_EntryList));
    shQzero(entry_list, sizeof(struct CharaData_EntryList) * 32);
    for (i = 0, j = 0; i < 32; i++) {
        if (mem_admin[i].category != Category_null && mem_admin[i].priority != 0) {
            if (mem_admin[i].priority == 1) {
                entry_list[j].category = mem_admin[i].category;
                entry_list[j].kind = mem_admin[i].kind;
                entry_list[j].model.file = mem_admin[i].model.file;
                entry_list[j].animation.file = mem_admin[i].animation.file;
                entry_list[j].shadow.file = mem_admin[i].shadow.file;
                entry_list[j].cluster.file = mem_admin[i].cluster.file;
                j++;
            }
            CharaDataInfoFree(&mem_admin[i], 0xF);
        }
    }
    for (i = 0; i < j; i++) {
        CharaDataLoadExecStandard(entry_list, &i, &j, status | 2);
    }
    memcpy(&entry_list[i], entry_list_bak, (*entry_no - *no) * sizeof(struct CharaData_EntryList));
    *entry_no += i - *no;
    *no = i;
    CharaDataLoadExecStandard(entry_list, no, entry_no, status | 2);
}

/** Switches James's animation and the weapon model to already loaded copies, and re-links
 * the models to them.
 * @param kind weapon character kind
 * @param file_anm James's animation file
 * @param adr_anm where it is loaded
 * @param file_mdl the weapon's model file
 * @param adr_mdl where it is loaded */
void CharaDataWeaponTranslation(int kind, union fsFileIndex *file_anm, void *adr_anm, union fsFileIndex *file_mdl,
                                void *adr_mdl) {
    struct SubCharacter *scp;
    struct chr_mge_files load_files[3];
    int i;
    int j;

    i = SeekMemAdminCtgry(Category_james);
    j = SeekMemAdminCtgry(Category_weapon);
    sh2gfw_Delete_Model_from_CharaID(mem_admin[i].kind);
    mem_admin[i].animation.file = file_anm;
    shMemCopy(mem_admin[i].animation.adress, adr_anm, CDL_BLOCK_SIZE(FcGetFileSize(file_anm)));
    mem_admin[j].kind = kind;
    mem_admin[j].model.file = file_mdl;
    shMemCopy(mem_admin[j].model.adress, adr_mdl, CDL_BLOCK_SIZE(FcGetFileSize(file_mdl)));
    shQzero(load_files, sizeof(load_files));
    load_files[0].mid = mem_admin[i].kind;
    load_files[1].mid = mem_admin[j].kind;
    load_files[2].mid = -1;
    sh2gfw_LoadInit_CharaModelData(load_files);
    sh2gfw_LoadMemorySet_CharaModelData(&load_files[0], &mem_admin[i].model.adress, &mem_admin[i].animation.adress,
                                        &mem_admin[i].cluster.adress, &mem_admin[i].shadow.adress);
    sh2gfw_LoadMemorySet_CharaModelData(&load_files[1], &mem_admin[j].model.adress, &mem_admin[j].animation.adress,
                                        &mem_admin[j].cluster.adress, &mem_admin[j].shadow.adress);
    sh2gfw_LOAD_CharaModelData();
    sh2gfw_SyncInit_ChacterModelData();
    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = scp->next) {
        shCharacter_Manage_SetDataAdresss(scp);
    }
}

/** Copies a loaded extra file into the character area.
 * @param file the file (for its size)
 * @param adress where it is now
 * @return its new address in the character area */
u_long128 *CharaDataExtraTranslation(union fsFileIndex *file, void *adress) {
    u_long128 *free_adr;
    int size;

    size = FcGetFileSize(file);
    free_adr = CharaDataFreeSearch(CDL_BLOCK_SIZE(size));
    /* Matching: the assert bakes its original line number into the object. */
#line 1427
    assert_dw(free_adr);
    shMemCopy(free_adr, adress, (size + 0x3F) & ~0x3F);
    return free_adr;
}

/** Makes a character kind use another animation file (already in memory).
 * @param kind character kind
 * @param file the animation file
 * @param adress where it is loaded
 * @param free non-zero to free the old animation
 * @return the old animation's address */
u_long128 *CharaDataAnimSetExtra(int kind, union fsFileIndex *file, u_long128 *adress, int free) {
    struct SubCharacter *scp;
    struct chr_mge_files load_files[2];
    u_long128 *ret_adr;
    int i;
    int j;

    i = SeekMemAdminKind(kind);
    ret_adr = mem_admin[i].animation.adress;
    if (free) {
        for (j = 0; j < 8; j++) {
            /* (the original indexes with i, not j) */
            if (chara_data_extra[i].adress == ret_adr) {
                chara_data_extra[i].adress = NULL;
                chara_data_extra[i].size = 0;
            }
        }
        CharaDataInfoFree(&mem_admin[i], 2);
    }
    mem_admin[i].animation.file = file;
    mem_admin[i].animation.adress = adress;
    mem_admin[i].animation.size = CDL_BLOCK_SIZE(FcGetFileSize(file));
    shQzero(load_files, sizeof(load_files));
    load_files[0].mid = kind;
    load_files[1].mid = -1;
    sh2gfw_LoadInit_CharaModelData(load_files);
    sh2gfw_LoadMemorySet_CharaModelData(&load_files[0], &mem_admin[i].model.adress, &mem_admin[i].animation.adress,
                                        &mem_admin[i].cluster.adress, &mem_admin[i].shadow.adress);
    sh2gfw_LOAD_CharaModelData();
    sh2gfw_Delete_Model_from_CharaID(kind);
    sh2gfw_SyncInit_ChacterModelData();
    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = scp->next) {
        shCharacter_Manage_SetDataAdresss(scp);
    }
    return ret_adr;
}

/** Points a character's animation at another address.
 * @param scp the character
 * @param adr the new animation data
 * @return the old address */
u_long128 *CharaDataAnimAdressExchange(struct SubCharacter *scp, u_long128 *adr) {
    u_long128 *ret;
    int i;

    i = SeekMemAdminKind(scp->kind);
    ret = mem_admin[i].animation.adress;
    mem_admin[i].animation.adress = adr;
    shCharacter_Manage_SetJamesAnimeAdresss(scp, (unsigned int)adr);
    return ret;
}

/** Loads an extra file into the character area, freeing unused data until it fits.
 * @param file the file
 * @param status load flags (4: in the background)
 * @return its address */
u_long128 *CharaDataLoadExtra(union fsFileIndex *file, int status) {
    u_long128 *adr;
    int size;
    int del;
    int i;

    size = CDL_BLOCK_SIZE(FcGetFileSize(file));
    while (1) {
        adr = CharaDataFreeSearch(size);
        if (adr != NULL) {
            break;
        }
        del = CharaDeleteNoUseOne();
        /* Matching: the assert bakes its original line number into the object. */
#line 1520
        assert_dw(del);
    }
    if (status & 4) {
        LoadBgEventFileLoad(file, adr);
    } else {
        FcRead(file, adr);
    }
    if (status & 0x100) {
        CharaDataUseFree(adr, size);
    } else if (status & 0x200) {
        for (i = 0; i < 8; i++) {
            if (chara_data_extra[i].adress == NULL) { break; }
        }

        assert_dw(i < 8);
        chara_data_extra[i].adress = adr;
        chara_data_extra[i].size = size;
    }
    return adr;
}

/**
 * Finds size >> 13 free 8 KiB blocks in a row in chara_adress, marks them used and returns them;
 * NULL when there is no room.
 */
/* Matching: this function alone was compiled without common-subexpression elimination: the
   original evaluates size >> 13 again at each use, and the pragma reproduces it exactly (the
   rest of the file needs it on). */
#pragma push
#pragma opt_common_subs off
static u_long128 *CharaDataFreeSearch(int size) {
    int i;
    int j;

    for (i = 0; i < 1024; i++) {
        for (j = 0; j < size >> 13 && i + j < 1024; j++) {
            if (chara_data_use[i + j] == 1) {
                break;
            }
        }
        if (j == size >> 13) {
            break;
        }
        i += j;
    }
    if (i >= 1024) {
        return NULL;
    }
    for (j = 0; j < size >> 13; j++) {
        chara_data_use[i + j] = 1;
    }
    return chara_adress + (i << 13) / sizeof(u_long128);
}
#pragma pop

static int CharaDeleteNoUseOne(void) {
    int del_prio;
    int del_no;
    int i;

    del_no = -1;
    del_prio = 2;
    for (i = 0; i < 32; i++) {
        if (mem_admin[i].category != Category_null && mem_admin[i].priority > del_prio) {
            del_prio = mem_admin[i].priority;
            del_no = i;
        }
    }
    if (del_no == -1) {
        return 0;
    }
    CharaDataInfoFree(&mem_admin[del_no], 0xF);
    return 1;
}

/** Frees the data of all standard characters (not James, weapons or items). */
void CharaDataDeleteAll(void) {
    int i;

    for (i = 0; i < 32; i++) {
        if (mem_admin[i].category == Category_standard) {
            CharaDataInfoFree(&mem_admin[i], 0xF);
        }
    }
}

/** Deletes the characters of a kind (not the players) and frees its data.
 * @param kind character kind */
void CharaDataDeleteOne(int kind) {
    struct SubCharacter *scp;
    struct SubCharacter *next;
    int i;

    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = next) {
        next = scp->next;
        if (scp->kind == kind && scp->kind != 0x100 && scp->kind != 0x101 && scp->kind >> 8 != 8) {
            shCharacter_Manage_Delete(scp, 0, 0);
        }
    }
    for (i = 0; i < 32; i++) {
        if (kind == mem_admin[i].kind) {
            CharaDataInfoFree(&mem_admin[i], 0xF);
        }
    }
}

static void CharaDataInfoFree(struct CharaData_MemAdmin *admin_p, int del) {
    if (admin_p->model.file != NULL && (del & 1)) {
        CharaDataInfoFreeSub(&admin_p->model);
    }
    if (admin_p->animation.file != NULL && (del & 2)) {
        CharaDataInfoFreeSub(&admin_p->animation);
    }
    if (admin_p->shadow.file != NULL && (del & 4)) {
        CharaDataInfoFreeSub(&admin_p->shadow);
    }
    if (admin_p->cluster.file != NULL && (del & 8)) {
        CharaDataInfoFreeSub(&admin_p->cluster);
    }
    if (del == 0xF) {
        sh2gfw_Delete_Model_from_CharaID(admin_p->kind);
        shQzero(admin_p, sizeof(struct CharaData_MemAdmin));
    }
}

static void CharaDataInfoFreeSub(struct CharaData_MemAdmin_One *maop) {
    if (maop->adress >= chara_adress && maop->adress < chara_adress + 0x800000 / sizeof(u_long128)) {
        CharaDataUseFree(maop->adress, maop->size);
    }
    shQzero(maop, sizeof(struct CharaData_MemAdmin_One));
}

static void CharaDataUseFree(u_long128 *adr, int size) {
    int work;
    int i;

    if (adr == NULL || adr < chara_adress) {
        return;
    }
    work = (adr - chara_adress) >> 9;
    for (i = 0; i < size >> 13; i++) {
        chara_data_use[i + work] = 0;
    }
}

/** Frees the extra files. */
void CharaDataDeleteExtra(void) {
    int i;

    for (i = 0; i < 8; i++) {
        if (chara_data_extra[i].adress != NULL) {
            CharaDataUseFree(chara_data_extra[i].adress, chara_data_extra[i].size);
        }
    }
    shQzero(chara_data_extra, sizeof(chara_data_extra));
}

/** Looks up the model, animation and shadow files of an item character.
 * @param file result, 3 entries (left unchanged if the kind isn't found)
 * @param kind character kind
 * @return 0 */
int CharaDataFileSearch(union fsFileIndex **file, int kind) {
    int i;

    for (i = 0; item_list[i].kind != 0; i++) {
        if (kind == item_list[i].kind) {
            file[0] = item_list[i].model;
            file[1] = item_list[i].animation;
            file[2] = item_list[i].shadow;
            return 1;
        }
    }
    return 0;
}

/** Starts a background load into the character area. */
void CharaDataBackLoadInit(void) {
    LoadBgEventInit(chara_adress, 0x800000);
}

/** Sets up the models of a demo list once their background load has finished, and re-links
 * all characters to their data.
 * @param dlp the list, ended by kind 0 (may be NULL) */
void CharaDataBackInit(struct CharaData_DemoList *dlp) {
    struct SubCharacter *scp;
    int i;
    int j;

    for (i = 0; dlp != NULL && dlp[i].kind != 0; i++) {
        j = SeekMemAdminKind(dlp[i].kind);
        sh2gfw_Delete_Model_from_CharaID(dlp[i].kind);
        sh2gfw_ModelDrawInit_for_BackgroundLoad(dlp[i].kind, mem_admin[j].model.adress,
                                                mem_admin[j].animation.adress, mem_admin[j].cluster.adress,
                                                mem_admin[j].shadow.adress);
    }
    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = scp->next) {
        shCharacter_Manage_SetDataAdresss(scp);
    }
}

static int SeekMemAdminCtgry(int category) {
    int i;

    for (i = 0; i < 32; i++) {
        if (category == mem_admin[i].category) {
            break;
        }
    }
    if (i < 32) {
        return i;
    }
    if (category == Category_null) {
        CharaDeleteNoUseOne();
    }
    for (i = 0; i < 32; i++) {
        if (category == mem_admin[i].category) {
            break;
        }
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 1791
    assert_dw(category != Category_null || i < 32);
    return i;
}

static int SeekMemAdminKind(int kind) {
    int i;

    for (i = 0; i < 32; i++) {
        if (kind == mem_admin[i].kind) {
            break;
        }
    }
    return i;
}
