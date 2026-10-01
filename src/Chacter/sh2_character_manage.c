/*
 * Character management: creating sub-characters with an ID, deleting them, binding their
 * model/animation data, and recreating them from save data.
 */

#include "sh2.h"
#include "asm_helpers.h"

static int id_counter;

/** Resets the automatic ID counter and the battle enemy-check work. */
int shCharacter_Manage_Init(void) {
    id_counter = 0x1000;
    shBattleInitEnemyCheckWork();
}

/**
 * Creates a sub-character and runs its update function once.
 * @param kind character kind. @param id ID to give it, or -1 for the next automatic one.
 * @param pos start position. @param rot start rotation. @param status initial enemy status.
 * @return the character's ID, or -1 if it couldn't be created.
 */
int shCharacter_Manage_Create(short kind, short id, float *pos, float *rot, unsigned int status) {
    struct SubCharacter *scp;

    scp = sh2gfw_CreateSubCharacter(kind);
    if (scp) {
        vcopy_dst_first((float *)&scp->pos, pos);
        vcopy_dst_first((float *)&scp->rot, rot);
        scp->en_first_status = status;
        scp->battle.status |= 0x400;
        if (id != -1) {
            scp->id = id;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 127
            assert_dw(id < 0x1000); /* Matching: do/while(0) form (its nop) */
        } else {
            scp->id = id_counter;
            printf("id %d\n", id_counter);
            id_counter++;
            if (id_counter == 0x7FFF) {
                id_counter = 0x1000;
            }
        }
        scp->function(scp);
        return scp->id;
    }
    return -1;
}

/**
 * Deletes a sub-character (and its enemy AI, for enemies and kind 0x421).
 * @param scp the character, or NULL to look it up by @p kind and @p id.
 * @return 1 if one was deleted, 0 if not found.
 */
int shCharacter_Manage_Delete(struct SubCharacter *scp, short kind, short id) {
    struct SubCharacter *del_scp;
    int delete_on = 0;

    if (scp) {
        del_scp = scp;
    } else {
        for (del_scp = sh2chara.head; del_scp; del_scp = del_scp->next) {
            if (del_scp->kind == kind && del_scp->id == id) {
                delete_on = 1;
                break;
            }
        }
        if (!delete_on) {
            del_scp = NULL;
        }
    }
    if (!del_scp) {
        return 0;
    }
    if ((del_scp->kind >> 8) == 2 || del_scp->kind == 0x421) {
        enDeleteEnemy(del_scp->enemy_p);
    }
    shCharacterDelete(del_scp);
    return 1;
}

/** Returns the head of the sub-character list. */
struct SubCharacter *shCharacter_Manage_GetCharacterList(void) {
    return sh2chara.head;
}

/**
 * Binds a character to its model, animation and cluster-animation data (sh2gfw_Get_pMD); on
 * first binding also sets the model up and runs its update function.
 * @return 1 on success, 0 if @p scp is NULL or its data isn't loaded.
 */
int shCharacter_Manage_SetDataAdresss(struct SubCharacter *scp) {
    struct SubCharacterDisp *scp_d;
    struct sh2gfw_ModelDraw_MAN *pMD;
    void SCSetModel(); /* Matching: called without a prototype in the original (arguments passed unconverted). */

    if (!scp) {
        return 0;
    }
    pMD = sh2gfw_Get_pMD(scp->kind);
    if (!pMD) {
        return 0;
    }
    scp_d = (struct SubCharacterDisp *)scp;
    if (scp_d->model_adr == 0) {
        SCSetModel(scp, (int)pMD->sh_Model, (int)pMD->pAnime);
        scp_d->model_adr = (unsigned int)pMD->sh_Model;
        scp_d->anime_adr = (unsigned int)pMD->pAnime;
        scp_d->clani_adr = (unsigned int)pMD->pCluster;
        scp_d->anime.anime = (void *)scp_d->anime_adr;
        scp->function(scp);
    } else {
        scp_d->model_adr = (unsigned int)pMD->sh_Model;
        scp_d->anime_adr = (unsigned int)pMD->pAnime;
        scp_d->clani_adr = (unsigned int)pMD->pCluster;
        scp_d->anime.anime = (void *)scp_d->anime_adr;
        scp_d->models[0] = (void *)scp_d->model_adr;
        scp_d->models[1] = (void *)scp_d->model_adr;
        scp_d->models[2] = (void *)scp_d->model_adr;
    }
    return 1;
}

/** Points James's animation data at @p address. @param scp the character. */
void shCharacter_Manage_SetJamesAnimeAdresss(struct SubCharacter *scp, unsigned int address) {
    struct SubCharacterDisp *scp_d;

    scp_d = (struct SubCharacterDisp *)scp;
    scp_d->anime_adr = address;
    scp_d->anime.anime = (void *)scp_d->anime_adr;
}

/** Recreates the characters in @p chara (from a memory-card load) and restores James's stats. */
int shCharacter_Manage_Create_After_MC_Load(struct _Character_Info *chara) {
    int i;
    struct SubCharacter *scp;

    for (i = 0; i < chara->total; i++) {
        scp = shCharacterCreate(0, 0, 0, 0, chara->ci_sc[i].kind);
        scp->status = chara->ci_sc[i].status;
        scp->sub_status = chara->ci_sc[i].sub_status;
        scp->sub_st = chara->ci_sc[i].sub_st;
        scp->id = chara->ci_sc[i].id;
        scp->pos = chara->ci_sc[i].pos;
        scp->rot = chara->ci_sc[i].rot;
        scp->pos_spd = chara->ci_sc[i].pos_spd;
        scp->rot_spd = chara->ci_sc[i].rot_spd;
        scp->b_pos = chara->ci_sc[i].b_pos;
        scp->b_rot = chara->ci_sc[i].b_rot;
        scp->en_first_status = chara->ci_sc[i].en_first_status;
        scp->eye_y = chara->ci_sc[i].eye_y;
        scp->center_y = chara->ci_sc[i].center_y;
        scp->spd = chara->ci_sc[i].spd;
        scp->spd_org = chara->ci_sc[i].spd_org;
        scp->spd_y = chara->ci_sc[i].spd_y;
        scp->spd_roty = chara->ci_sc[i].spd_roty;
        scp->battle.hp = chara->ci_sc[i].battle_hp;
        scp->battle.hp_max = chara->ci_sc[i].battle_hp_max;
        scp->battle.hp_rate = chara->ci_sc[i].battle_hp_rate;
        scp->battle.status = chara->ci_sc[i].battle_status;
        if (chara->ci_sc[i].battle_status & 2) {
            scp->en_first_status = 5;
        }
    }
    sh2jms.tired = chara->tired;
    sh2jms.tired_max = chara->tired_max;
    sh2jms.spirit = chara->spirit;
    sh2jms.weapon = chara->weapon;
    sh2jms.spray_time = chara->spray_time;
    sh2jms.running_time = chara->running_time;
    sh2jms.tired = chara->tired;
    sh2jms.tired_max = chara->tired_max;
    sh2jms.spirit = chara->spirit;
    sh2jms.weapon = chara->weapon;
    sh2jms.spray_set = chara->spray_set;
    sh2jms.room_name_now = -1;
}
