/*
 * James's body states shared by both control modes (m3_play_2d, m3_play_3d): stand, relax,
 * alert, tired, ready, run, hold/aim, attack, kick, fall, damage and event, each as a
 * lower_* (legs) and upper_* (torso/arms) handler, plus attack checks and aiming.
 */

#include "sh2.h"

static int dt;
static float dtf;

/* Matching: a fitted float-constant stand-in (docs/stand-ins.md; found by tools/constcount.py) that fixes the -PI/PI
 * argument order below; lx/ly/lz apart from x/y/z give the original FPRs. */
static float __stripped_float_code_2(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f; }
static void PlayerCheckHuggedPos(float *l_pos, int dir, float mov) {
    float tgt_rot;
    int anim_counter;

    sh2jms.player->spd = 0.0f;
    sh2jms.player->spd_org = 0.0f;
    {
        float x, lx;
        float y, ly;
        float z, lz;

        lx = l_pos[0] * shCosF(sh2jms.player->battle.target->rot.y) + l_pos[2] * shSinF(sh2jms.player->battle.target->rot.y);
        ly = l_pos[1];
        lz = -l_pos[0] * shSinF(sh2jms.player->battle.target->rot.y) + l_pos[2] * shCosF(sh2jms.player->battle.target->rot.y);
        x = sh2jms.player->battle.target->pos.x + lx;
        y = sh2jms.player->battle.target->pos.y + ly;
        z = sh2jms.player->battle.target->pos.z + lz;
        if (dir) {
            tgt_rot = shAngleRegulate(3.1415927f + sh2jms.player->battle.target->rot.y);
        } else {
            tgt_rot = sh2jms.player->battle.target->rot.y;
        }
        close_to_value(&sh2jms.player->pos.x, x, mov);
        close_to_value(&sh2jms.player->pos.y, y, mov);
        close_to_value(&sh2jms.player->pos.z, z, mov);
    }
    close_to_angle_target(&sh2jms.player->rot.y, tgt_rot, -3.1415927f, 3.1415927f, 6.2831855f);
    if (sh2jms.hug_status == 2) {
        anim_counter = shCharacterAnimeCounterGet(sh2jms.player->battle.target);
        shCharacterAnimeCounterSet_(sh2jms.player, 1, anim_counter);
        shCharacterAnimeCounterSet_(sh2jms.player, 2, anim_counter);
    }
}

/** Caches this frame's delta (frames and seconds) for this file. */
void PlayerSetDT(void) {
    dt = shGetDF();
    dtf = shGetDT();
}

/** Updates James's per-frame status common to both control modes (map mode, ...). */
void PlayerUpdateStatus(struct SubCharacter *this) {
    if (!clPermitColumnExpansion()) {
        sh2jms.map_mode = 2;
    } else {
        switch (stage->glb_crd) {
        case 1:
        case 2:
        case 3:
        case 5:
            sh2jms.map_mode = 0;
            break;
        default:
            sh2jms.map_mode = 1;
            break;
        }
    }
    shCharacterAnimeSpeedAdd(this, 0);
    if (sh2jms.weapon == 8) {
        if (!l_anime_flg_on(4)) {
            shCharacterAnimeSpeedAddY_(this, 2, -0x100);
            shCharacterAnimeSpeedAddY_(this, 1, -0x100);
        }
        if (sh2jms.hold_type == -1 && sh2jms.lower_now >= JMS_ST_L_BACK && sh2jms.lower_now < JMS_ST_L_RUN1) {
            shCharacterAnimeSpeedAdd_(this, 2, -0x200);
            shCharacterAnimeSpeedAdd_(this, 1, -0x200);
        }
    }
    sh2jms.cannot_run = 0;
    sh2jms.se_on = 0;
    if (sh2jms.lower_now >= JMS_ST_L_RUN1 && sh2jms.lower_now < JMS_ST_L_WALL_F) {
        sh2jms.running_time += dtf;
    } else {
        sh2jms.running_time = 0.0f;
    }
    if (sh2jms.muteki_time) {
        sh2jms.muteki_time -= dtf;
        if (sh2jms.muteki_time < 0.0f) {
            sh2jms.muteki_time = 0.0f;
        }
    }
}

static int PlayerCheckKeyLStickTrg(int dir) {
    switch (dir) {
    case 2:
        return sh2jms.pad[0].lstickY > 0 && !(sh2jms.pad[1].lstickY > 0);
    case 4:
        return sh2jms.pad[0].lstickX < 0 && !(sh2jms.pad[1].lstickX < 0);
    case 6:
        return sh2jms.pad[0].lstickX > 0 && !(sh2jms.pad[1].lstickX > 0);
    case 8:
        return sh2jms.pad[0].lstickY < 0 && !(sh2jms.pad[1].lstickY < 0);
    }
    return 0;
}

/** While James is held, drains hugging_gauge (escaping) by a time step per button or stick direction pressed. */
void PlayerCheckHuggingButton(void) {
    float pow;
    float gauge;

    pow = 0.0f;
    gauge = shGetDT();
    if (shPadTrigger(0, key_config.action)) {
        pow += gauge;
    }
    if (shPadTrigger(0, key_config.dash)) {
        pow += gauge;
    }
    if (shPadTrigger(0, key_config.front_move)) {
        pow += gauge;
    }
    if (shPadTrigger(0, key_config.back_move)) {
        pow += gauge;
    }
    if (shPadTrigger(0, key_config.right_turn)) {
        pow += gauge;
    }
    if (shPadTrigger(0, key_config.left_turn)) {
        pow += gauge;
    }
    if (shPadTrigger(0, key_config.front_move)) {
        pow += gauge;
    }
    if (shPadTrigger(0, key_config.right_move)) {
        pow += gauge;
    }
    if (shPadTrigger(0, key_config.left_move)) {
        pow += gauge;
    }
    if (PlayerCheckKeyLStickTrg(2)) {
        pow += gauge;
    }
    if (PlayerCheckKeyLStickTrg(4)) {
        pow += gauge;
    }
    if (PlayerCheckKeyLStickTrg(6)) {
        pow += gauge;
    }
    if (PlayerCheckKeyLStickTrg(8)) {
        pow += gauge;
    }
    if (pow) {
        sh2jms.hugging_gauge -= pow;
        if (sh2jms.hugging_gauge < 0.0f) {
            sh2jms.hugging_gauge = 0.0f;
        }
    }
}

/** Slows @p p down towards standing, at a rate that depends on the previous lower-body state. */
void PlayerSpeedDownToStand(struct SubCharacter *p) {
    switch (sh2jms.lower_prev) {
    case JMS_ST_L_RUN1:
    case JMS_ST_L_RUN2:
    case JMS_ST_L_RUN3:
    case JMS_ST_L_LSRUN:
    case JMS_ST_L_RSRUN:
        p->spd -= 20.0f * shGetDT();
        break;
    default:
        p->spd -= 10.0f * shGetDT();
        break;
    }
    p->spd_org = p->spd = (p->spd > 0.0f) ? p->spd : 0.0f;
    if (playing.control_type == 0) {
        if (p->spd == 0.0f) {
            p->spd_roty = 0.0f;
        }
    }
}

/** Sets attack_ok if James may attack while walking (holding/attacking with a weapon that allows it). */
void PlayerSetAttackWithWalkIsOk(void) {
    if (sh2jms.upper_now == JMS_ST_U_ATTACK || sh2jms.upper_now == JMS_ST_U_HOLD) {
        if (!l_anime_flg_on(2)) {
            if ((sh2jms.act_with_wep & 0x10) && sh2jms.atk_type < 4) {
                sh2jms.attack_ok = 1;
                return;
            }
        }
    }
    sh2jms.attack_ok = 0;
}

/** Sets attack_ok if James may attack while running (holding/attacking with a weapon that allows it). */
void PlayerSetAttackWithRunIsOk(void) {
    if (sh2jms.upper_now == JMS_ST_U_ATTACK || sh2jms.upper_now == JMS_ST_U_HOLD) {
        if (!l_anime_flg_on(2)) {
            if ((sh2jms.act_with_wep & 0x20) && sh2jms.atk_type < 4) {
                sh2jms.attack_ok = 1;
                return;
            }
        }
    }
    sh2jms.attack_ok = 0;
}

/** Turns @p p towards @p target (if any). */
void PlayerCheckAimingToEnemy(struct SubCharacter *p, struct SubCharacter *target) {
    float to_target;
    float roty_tmp;
    float mov_angle;

    if (target) {
        to_target = shAtan2(target->pos.z - p->pos.z, target->pos.x - p->pos.x);
        roty_tmp = shAngleRegulate(p->rot.y - to_target);
        mov_angle = roty_tmp / (3.1415927f - 0.05) * (15.0f * shGetDT());
        if (roty_tmp >= 0.0f) {
            if (roty_tmp - mov_angle <= 0.0f) {
                p->rot.y = to_target;
            } else {
                p->rot.y -= mov_angle;
            }
        } else {
            if (roty_tmp - mov_angle >= 0.0f) {
                p->rot.y = to_target;
            } else {
                p->rot.y -= mov_angle;
            }
        }
        p->rot.y = shAngleRegulate(p->rot.y);
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 472
/** Checks the attack input and starts James's next attack or combo. */
void PlayerCheckAttack(struct SubCharacter *this) {
    struct _AnimeInfo *a_info;
    struct shPlayerWork *w;
    struct PAD_INFO *pad;

    a_info = shCharacterAnimeGetInfo_(this, 1);
    w = &sh2jms;
    /* Matching: pad is in the original's DWARF but never read (a dead store). */
    pad = &sh2jms.pad[0];
    if (sh2jms.upper_now == JMS_ST_U_HOLD && ((sh2jms.anime_pause & 2) || a_info->loop)) {
        PlayerRequestAttack(w, 0);
        sh2jms.atk_count = 0;
    }
    if (w->enemy_liedown && (w->upper_now < 0x12 || w->upper_now > 0x1D)) {
        PlayerRequestAttackFinish(w);
        player_flg_on(&w->upper_st_flg, 0x20000000);
    } else {
        player_flg_off(&w->upper_st_flg, 0x20000000);
    }
    if (w->upper_st_flg & 0x20000000) {
        if (!w->atk_reserve[0]) {
            return;
        }
        w->atk_type = w->atk_reserve[0];
        w->attack_no = (w->atk_type == 7) ? w->motion_no + 30 : w->motion_no + 25;
        shBattleAttackHitCheckInit(this);
        if (w->upper_now != JMS_ST_U_KICK) {
            upper_st_set(JMS_ST_U_KICK, w);
            upper_flg_set(JMS_ST_U_KICK, w);
            player_flg_on(&w->u_anime_st_flg, 0x40);
            lower_st_set(JMS_ST_L_KICK, w);
            lower_flg_set(JMS_ST_L_KICK, w);
            player_flg_on(&w->l_anime_st_flg, 0x40);
        }
        w->atk_reserve[0] = 0;
        w->hold_type = -1;
        w->lock_on = 0;
        return;
    }
    if (!(w->upper_st_flg & 0x10000000)) {
        return;
    }
    if (!w->atk_reserve[0]) {
        return;
    }
    shBattleAttackHitCheckInit(this);
    switch (w->weapon) {
    case 1:
        if (!w->shoot_val && w->reload_val) {
            w->atk_type = 0;
            w->attack_no = 3;
        } else {
            if (w->hold_type == 1 || u_anime_flg_on(0x40)) {
                w->hold_type = 1;
                w->atk_type = 2;
                w->attack_no = 2;
            } else {
                w->atk_type = 1;
                w->attack_no = 1;
            }
            PlayerGetTarget();
            if (w->target) {
                w->lock_on = 1;
            }
        }
        switch (w->lower_now) {
        case JMS_ST_L_BACK:
        case JMS_ST_L_WALK:
        case JMS_ST_L_RSWALK:
        case JMS_ST_L_LSWALK:
            break;
        default:
            player_flg_on(&w->lower_st_flg, 0x4000000);
            break;
        case JMS_ST_L_HOLD:
            player_flg_on(&w->lower_st_flg, 0x10000000);
            break;
        }
        break;
    case 2:
        if (!w->shoot_val && w->reload_val) {
            w->attack_no = 5;
        } else {
            w->atk_type = 2;
            w->attack_no = 4;
        }
        switch (w->lower_now) {
        case JMS_ST_L_BACK:
        case JMS_ST_L_WALK:
        case JMS_ST_L_RSWALK:
        case JMS_ST_L_LSWALK:
            player_flg_off(&w->lower_st_flg, 0x100);
            player_flg_off(&w->lower_st_flg, 0x200);
            player_flg_off(&w->lower_st_flg, 0x800);
            player_flg_off(&w->lower_st_flg, 0x400);
            player_flg_on(&w->lower_st_flg, 0x4000000);
            break;
        }
        break;
    case 3:
        if (!w->shoot_val && w->reload_val) {
            w->attack_no = 7;
        } else {
            w->atk_type = 2;
            w->attack_no = 6;
        }
        player_flg_on(&w->lower_st_flg, 0x10000000);
        break;
    case 4:
        w->atk_type = 2;
        if (!w->shoot_val && w->reload_val) {
            if (sh2jms.upper_prev == JMS_ST_U_ATTACK) {
                w->attack_no = 11;
            } else {
                w->attack_no = 10;
            }
        } else {
            if (w->atk_count) {
                w->atk_count = 1;
                w->attack_no = 9;
            } else {
                w->attack_no = 8;
            }
        }
        break;
    case 5:
        if (w->running) {
            w->atk_type = 3;
            if (w->atk_count) {
                w->atk_count = 1;
                w->attack_no = 13;
            } else {
                w->attack_no = 12;
            }
        } else {
            if (w->atk_reserve[0] == 3) {
                w->atk_type = 3;
                if (w->atk_count) {
                    w->atk_count = 1;
                    w->attack_no = 13;
                } else {
                    w->attack_no = 12;
                }
            }
            if (w->atk_reserve[0] == 4) {
                w->atk_type = 4;
                w->attack_no = 14;
            }
        }
        switch (w->lower_now) {
        case JMS_ST_L_BACK:
        case JMS_ST_L_WALK:
        case JMS_ST_L_LSWALK:
        case JMS_ST_L_RSWALK:
            if (w->atk_type == 4) {
                player_flg_off(&w->lower_st_flg, 0x100);
                player_flg_off(&w->lower_st_flg, 0x200);
                player_flg_off(&w->lower_st_flg, 0x800);
                player_flg_off(&w->lower_st_flg, 0x400);
                player_flg_on(&w->lower_st_flg, 0x4000000);
            }
            break;
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_RUN3:
        case JMS_ST_L_LSRUN:
        case JMS_ST_L_RSRUN:
            break;
        }
        break;
    case 6:
        if (w->atk_reserve[0] == 5) {
            w->atk_type = 5;
            w->attack_no = 18;
        }
        if (w->atk_reserve[0] == 4) {
            w->atk_type = 4;
            w->attack_no = 17;
        }
        if (w->atk_reserve[0] == 3) {
            w->atk_type = 3;
            if (w->atk_count) {
                w->atk_count = 1;
                w->attack_no = 16;
            } else {
                w->attack_no = 15;
            }
        }
        switch (w->lower_now) {
        case JMS_ST_L_BACK:
        case JMS_ST_L_WALK:
        case JMS_ST_L_RSWALK:
        case JMS_ST_L_LSWALK:
            if (w->atk_type != 3) {
                player_flg_off(&w->lower_st_flg, 0x100);
                player_flg_off(&w->lower_st_flg, 0x200);
                player_flg_off(&w->lower_st_flg, 0x800);
                player_flg_off(&w->lower_st_flg, 0x400);
                player_flg_on(&w->lower_st_flg, 0x4000000);
            }
            break;
        }
        break;
    case 7:
        if (w->atk_reserve[0] == 3) {
            w->atk_type = 3;
            w->attack_no = 23;
        }
        if (w->atk_reserve[0] == 5) {
            w->atk_type = 5;
            w->attack_no = 24;
        }
        switch (w->lower_now) {
        case JMS_ST_L_BACK:
        case JMS_ST_L_WALK:
        case JMS_ST_L_RSWALK:
        case JMS_ST_L_LSWALK:
            if (w->atk_type == 5) {
                player_flg_off(&w->lower_st_flg, 0x100);
                player_flg_off(&w->lower_st_flg, 0x200);
                player_flg_off(&w->lower_st_flg, 0x800);
                player_flg_off(&w->lower_st_flg, 0x400);
                player_flg_on(&w->lower_st_flg, 0x4000000);
            }
            break;
        default:
            player_flg_on(&w->lower_st_flg, 0x10000000);
            break;
        }
        break;
    case 8:
        if (w->atk_reserve[0] == 3) {
            w->atk_type = 3;
            if (w->hold_type == 0) {
                w->attack_no = 19;
            } else {
                w->attack_no = 20;
            }
        }
        if (w->atk_reserve[0] == 4) {
            w->atk_type = 4;
            if (w->hold_type == 0) {
                w->attack_no = 21;
            } else {
                w->attack_no = 22;
            }
        }
        player_flg_on(&w->lower_st_flg, 0x10000000);
        break;
    }
    if (w->upper_now != JMS_ST_U_ATTACK || w->upper_prev == JMS_ST_U_ATTACK) {
        upper_st_set(JMS_ST_U_ATTACK, w);
        upper_flg_set(JMS_ST_U_ATTACK, w);
    }
    assert(w->attack_no != 0);
    w->atk_reserve[0] = 0;
}

/** Lower-body stand state. */
void lower_stand(struct SubCharacter *p) {
    actwithwep_flg_set(0, &sh2jms);
    sh2jms.tired -= dt;
    PlayerSpeedDownToStand(p);
    if (p->spd < 0.1f) {
        player_flg_on(&sh2jms.lower_st_flg, 0x10);
    }
    if (sh2jms.tired > sh2jms.tired_max / 3) {
        player_flg_on(&sh2jms.lower_st_flg, 8);
    }
}

/** Upper-body stand state (goes tired when stamina is low). */
void upper_stand(struct SubCharacter *p) {
    if (p->spd < 0.1f) {
        player_flg_on(&sh2jms.upper_st_flg, 0x10);
    }
    if (sh2jms.tired > sh2jms.tired_max / 3) {
        player_flg_on(&sh2jms.upper_st_flg, 8);
    }
}

/** Lower-body relax state. */
void lower_relax(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 1) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
    }
    if (sh2jms.weapon == 8) {
        shCharacterAnimeSpeedAdd_(p, 2, -0xC0);
        shCharacterAnimeSpeedAdd_(p, 1, -0xC0);
    }
}

/** Upper-body relax state: ends when its animation stops. */
void upper_relax(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
    }
}

/** Lower-body alert state: ends when its animation stops. */
void lower_alert(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 1) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
    }
}

/** Upper-body alert state: ends when its animation stops. */
void upper_alert(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
    }
}

/** Lower-body tired (out of breath) state. */
void lower_tired(struct SubCharacter *p) {
    struct _AnimeInfo *a_info;

    sh2jms.tired -= dt;
    PlayerSpeedDownToStand(p);
    a_info = shCharacterAnimeGetInfo_(p, 2);
    if (sh2jms.tired <= sh2jms.tired_max / 3) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
        player_flg_off(&sh2jms.lower_st_flg, 8);
    }
    if (sh2jms.tired <= sh2jms.tired_max * 2 / 3 && a_info->name == 0x72) {
        player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
    }
}

/** Upper-body tired (out of breath) state. */
void upper_tired(struct SubCharacter *p) {
    struct _AnimeInfo *a_info;

    a_info = shCharacterAnimeGetInfo_(p, 2);
    if (sh2jms.tired <= sh2jms.tired_max / 3) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
        player_flg_off(&sh2jms.upper_st_flg, 8);
    }
    if (sh2jms.tired <= sh2jms.tired_max * 2 / 3 && a_info->name == 0x72) {
        player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
    }
}

/** Lower-body ready (weapon raised) state. */
void lower_ready(struct SubCharacter *p) {
    PlayerSpeedDownToStand(p);
    if (sh2jms.anime_pause & 1) {
        player_flg_on(&sh2jms.lower_st_flg, 0x20);
        if (sh2jms.tired <= sh2jms.tired_max / 3) {
            player_flg_on(&sh2jms.lower_st_flg, 0x4000);
        } else {
            player_flg_on(&sh2jms.lower_st_flg, 0x2000);
        }
    }
}

/** Upper-body ready (weapon raised) state. */
void upper_ready(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 0x20);
        if (sh2jms.tired <= sh2jms.tired_max / 3) {
            player_flg_on(&sh2jms.upper_st_flg, 0x4000);
        } else {
            player_flg_on(&sh2jms.upper_st_flg, 0x2000);
        }
    }
}

/** Lower-body weapon-lowering state: ends when its animation stops. */
void lower_readyoff(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 1) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
    }
}

/** Upper-body weapon-lowering state: ends when its animation stops. */
void upper_readyoff(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
    }
}

/** Upper-body run state, first speed stage. */
void upper_run1(struct SubCharacter *p) {
    if (p->spd >= 3.5f && !sh2jms.u_anime_st_flg) {
        player_flg_on(&sh2jms.upper_st_flg, 0x2000);
        player_flg_off(&sh2jms.upper_st_flg, 0x1000);
    }
}

/** Upper-body run state, second speed stage. */
void upper_run2(struct SubCharacter *p) {
    switch (sh2jms.ctrl_unit) {
    case 0:
        if (p->spd == 4.0f && !sh2jms.u_anime_st_flg && sh2jms.tired < sh2jms.tired_max) {
            player_flg_on(&sh2jms.upper_st_flg, 0x4000);
            player_flg_off(&sh2jms.upper_st_flg, 0x2000);
        }
        break;
    case 1:
        if (p->spd <= 3.5f && !sh2jms.u_anime_st_flg) {
            player_flg_on(&sh2jms.upper_st_flg, 0x1000);
            player_flg_off(&sh2jms.upper_st_flg, 0x2000);
        }
        if (p->spd >= 4.0f && !sh2jms.u_anime_st_flg && sh2jms.tired < sh2jms.tired_max) {
            player_flg_on(&sh2jms.upper_st_flg, 0x4000);
            player_flg_off(&sh2jms.upper_st_flg, 0x2000);
        }
        break;
    }
}

/** Upper-body run state, third speed stage (exhausted). */
void upper_run3(struct SubCharacter *p) {
    switch (sh2jms.ctrl_unit) {
    case 0:
        if (sh2jms.tired >= sh2jms.tired_max && !sh2jms.u_anime_st_flg) {
            player_flg_on(&sh2jms.upper_st_flg, 0x2000);
            player_flg_off(&sh2jms.upper_st_flg, 0x4000);
        }
        break;
    case 1:
        if ((p->spd <= 4.0f || sh2jms.tired >= sh2jms.tired_max) && !sh2jms.u_anime_st_flg) {
            player_flg_on(&sh2jms.upper_st_flg, 0x2000);
            player_flg_off(&sh2jms.upper_st_flg, 0x4000);
        }
        break;
    }
}

/** Lower-body hold (weapon held, aiming) state. */
void lower_hold(struct SubCharacter *p) {
    struct _AnimeInfo *a_info;
    short frame;
    int check;

    a_info = shCharacterAnimeGetInfo_(p, 2);
    frame = shCharacterAnimeFrameGet_(p, 2);
    PlayerSpeedDownToStand(p);
    switch (sh2jms.weapon) {
    case 3:
        if (!(sh2jms.anime_pause & 1) && frame >= 4 && frame < 15) {
            if (!playing.control_type) {
                p->spd_roty = 0.0f;
            }
            p->spd_org = p->spd = 25.0 * dtf;
        }
        break;
    case 5:
        if (a_info->name == 0x15F || a_info->name == 0x161) {
            if (!(sh2jms.anime_pause & 1) && frame >= 2 && frame < 9) {
                if (!playing.control_type) {
                    p->spd_roty = 0.0f;
                }
                p->spd_org = p->spd = 20.0 * dtf;
            }
        }
        break;
    }
    if ((sh2jms.anime_pause & 1) || (a_info->loop && a_info->name >= 200)) {
        player_flg_on(&sh2jms.lower_st_flg, 0x8000000);
        player_flg_on(&sh2jms.lower_st_flg, 0x10000000);
        if (!a_info->loop) {
            switch (sh2jms.weapon) {
            case 5:
            case 6:
            case 8:
            case 4:
                sh2jms.hold_loop[1] = 1;
                break;
            case 7:
                sh2jms.hold_loop[1] = 1;
                break;
            }
        }
        if (!sh2jms.hold_chg[1]) {
            switch (sh2jms.weapon) {
            case 5:
            case 6:
            case 8:
            case 7:
                switch (sh2jms.hold_type) {
                case 0:
                    if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                        sh2jms.hold_chg[1] = 1;
                    }
                    break;
                case 1:
                    if (sh2jms.l_side.kind == 1 && sh2jms.r_side.kind != 1) {
                        sh2jms.hold_chg[1] = 1;
                    }
                    break;
                }
                break;
            }
        }
        if (sh2jms.upper_now != JMS_ST_U_ATTACK || !(sh2jms.anime_pause & 8)) {
            if (sh2jms.hold_chg[1]) {
                player_flg_off(&sh2jms.lower_st_flg, 0x8000000);
                player_flg_off(&sh2jms.lower_st_flg, 0x10000000);
                player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
                sh2jms.hold_loop[1] = 0;
            } else if (sh2jms.hold_loop[1]) {
                player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
            }
        } else {
            sh2jms.hold_loop[1] = 0;
            sh2jms.hold_chg[1] = 0;
        }
    }
    if (sh2jms.weapon == 2 && (sh2jms.anime_pause & 1)) {
        check = 0;
        switch (a_info->name) {
        case 0xFB:
        case 0xFD:
        case 0x100:
        case 0x103:
        case 0x104:
        case 0x106:
        case 0x108:
            if (sh2jms.shotgun_dir != 1) {
                check = 1;
            }
            break;
        case 0xFE:
        case 0x105:
        case 0x10A:
            if (sh2jms.shotgun_dir != 2) {
                check = 1;
            }
            break;
        case 0x101:
        case 0x107:
        case 0x109:
            if (sh2jms.shotgun_dir != 0) {
                check = 1;
            }
            break;
        case 0xFC:
        case 0xFF:
        case 0x102:
            break;
        }
        if (check) {
            sh2jms.hold_chg[1] = 1;
            player_flg_off(&sh2jms.lower_st_flg, 0x8000000);
            player_flg_off(&sh2jms.lower_st_flg, 0x10000000);
            player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
        }
    }
    sh2jms.attack_ok = 0;
}

/** Upper-body hold (weapon held, aiming) state. */
void upper_hold(struct SubCharacter *p) {
    struct _AnimeInfo *a_info;
    int check;

    a_info = shCharacterAnimeGetInfo_(p, 1);
    if (sh2jms.lock_on) {
        PlayerCheckAimingToEnemy(p, sh2jms.target);
    }
    if ((sh2jms.anime_pause & 2) || (a_info->loop && a_info->name >= 200)) {
        player_flg_on(&sh2jms.upper_st_flg, 0x8000000);
        player_flg_on(&sh2jms.upper_st_flg, 0x10000000);
        if (!a_info->loop) {
            switch (sh2jms.weapon) {
            case 5:
            case 6:
            case 8:
            case 7:
            case 4:
                sh2jms.hold_loop[0] = 1;
                break;
            }
        }
        if (!sh2jms.hold_chg[0]) {
            switch (sh2jms.weapon) {
            case 5:
            case 6:
            case 8:
            case 7:
                switch (sh2jms.hold_type) {
                case 0:
                    if (sh2jms.r_side.kind == 1 && sh2jms.l_side.kind != 1) {
                        sh2jms.hold_chg[0] = 1;
                    }
                    break;
                case 1:
                    if (sh2jms.l_side.kind == 1 && sh2jms.r_side.kind != 1) {
                        sh2jms.hold_chg[0] = 1;
                    }
                    break;
                }
                break;
            }
        }
        if (sh2jms.hold_chg[0]) {
            player_flg_off(&sh2jms.upper_st_flg, 0x8000000);
            player_flg_off(&sh2jms.upper_st_flg, 0x10000000);
            player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
            sh2jms.hold_loop[0] = 0;
        } else if (sh2jms.hold_loop[0]) {
            player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
        }
    }
    if (sh2jms.weapon == 2 && (sh2jms.anime_pause & 2)) {
        check = 0;
        switch (a_info->name) {
        case 0xFB:
        case 0xFD:
        case 0x100:
        case 0x103:
        case 0x104:
        case 0x106:
        case 0x108:
            if (sh2jms.shotgun_dir != 1) {
                check = 1;
            }
            break;
        case 0xFE:
        case 0x105:
        case 0x10A:
            if (sh2jms.shotgun_dir != 2) {
                check = 1;
            }
            break;
        case 0x101:
        case 0x107:
        case 0x109:
            if (sh2jms.shotgun_dir != 0) {
                check = 1;
            }
            break;
        case 0xFC:
        case 0xFF:
        case 0x102:
            break;
        }
        if (check) {
            sh2jms.hold_chg[0] = 1;
            player_flg_off(&sh2jms.upper_st_flg, 0x8000000);
            player_flg_off(&sh2jms.upper_st_flg, 0x10000000);
            player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
        }
    }
}

/** Lower-body release (stop holding the weapon) state. */
void lower_release(struct SubCharacter *p) {
    short frame;

    frame = shCharacterAnimeFrameGet_(p, 2);
    PlayerSpeedDownToStand(p);
    switch (sh2jms.weapon) {
    case 3:
        if (!(sh2jms.anime_pause & 1) && frame >= 4 && frame < 15) {
            if (!playing.control_type) {
                p->spd_roty = -3.1415927f;
                p->spd_org = p->spd = 25.0 * dtf;
            } else {
                p->spd_org = p->spd = -25.0 * dtf;
            }
        }
        break;
    case 5:
        if (!(sh2jms.anime_pause & 1) && frame >= 2 && frame < 9) {
            if (!playing.control_type) {
                p->spd_roty = -3.1415927f;
                p->spd_org = p->spd = 20.0 * dtf;
            } else {
                p->spd_org = p->spd = -20.0 * dtf;
            }
        }
        break;
    }
    if (sh2jms.anime_pause & 1) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
        player_flg_off(&sh2jms.lower_st_flg, 0x8000000);
    }
}

/** Upper-body release state: ends the hold and the lock-on when its animation stops. */
void upper_release(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
        player_flg_on(&sh2jms.upper_st_flg, 8);
        player_flg_off(&sh2jms.upper_st_flg, 0x8000000);
        sh2jms.lock_on = 0;
    }
    sh2jms.lock_on = 0;
    if (sh2jms.lower_now == JMS_ST_L_LTURN || sh2jms.lower_now == JMS_ST_L_RTURN ||
        sh2jms.lower_now == JMS_ST_L_JUMP || sh2jms.lower_now == JMS_ST_L_GUARD ||
        sh2jms.lower_now == JMS_ST_L_RSTEP || sh2jms.lower_now == JMS_ST_L_LSTEP) {
        player_flg_off(&sh2jms.upper_st_flg, 0x8000000);
        sh2jms.lock_on = 0;
    }
}

/** Lower-body attack state (per weapon). */
void lower_attack(struct SubCharacter *p) {
    struct _AnimeInfo *a_info;
    short frame;

    a_info = shCharacterAnimeGetInfo_(p, 2);
    frame = shCharacterAnimeFrameGet_(p, 2);
    PlayerSpeedDownToStand(p);
    if (!playing.control_type) {
        p->spd_roty = 0.0f;
    }
    switch (sh2jms.weapon) {
    case 5:
        if (sh2jms.atk_type == 4) {
            if (a_info->speed > 0) {
                if (frame >= 4 && frame < 10) {
                    p->spd_org = p->spd = 30.0 * dtf;
                }
                if (frame >= 11 && frame < 20) {
                    p->spd_org = p->spd = -20.0 * dtf;
                }
            }
        }
        break;
    case 6:
        if (sh2jms.atk_type == 5) {
            if (a_info->speed > 0) {
                if (frame >= 2 && frame < 10) {
                    p->spd_org = p->spd = 35.0 * dtf;
                }
                if (frame >= 14 && frame < 25) {
                    p->spd_org = p->spd = -20.0 * dtf;
                }
            }
        }
        break;
    case 8:
        if (sh2jms.atk_type == 4) {
            if (sh2jms.hold_type == 0) {
                if (a_info->speed > 0) {
                    if (frame >= 10 && frame < 14) {
                        p->spd_org = p->spd = 5.0 * dtf;
                    }
                    if (frame >= 14 && frame < 17) {
                        p->spd_org = p->spd = 30.0 * dtf;
                    }
                    if (frame >= 2 && frame < 5) {
                        p->spd_org = p->spd = 10.0 * dtf;
                    }
                    if (frame >= 25 && frame < 31) {
                        p->spd_org = p->spd = -28.0 * dtf;
                    }
                }
            } else {
                if (a_info->speed > 0) {
                    if (frame >= 14 && frame < 18) {
                        p->spd_org = p->spd = 33.0 * dtf;
                    }
                    if (frame > 0 && frame < 4) {
                        p->spd_org = p->spd = -3.0 * dtf;
                    }
                    if (frame >= 25 && frame < 31) {
                        p->spd_org = p->spd = -24.0 * dtf;
                    }
                }
            }
        }
        break;
    case 7:
        if (a_info->speed > 0 && sh2jms.atk_type == 5) {
            if (sh2jms.hold_type == 0) {
                if (frame > 0 && frame < 7) {
                    p->spd_org = p->spd = 30.0 * dtf;
                }
                if (frame >= 13 && frame < 19) {
                    p->spd_org = p->spd = -25.0 * dtf;
                }
            } else {
                if (frame >= 2 && frame < 7) {
                    p->spd_org = p->spd = 30.0 * dtf;
                }
                if (frame >= 12 && frame < 18) {
                    p->spd_org = p->spd = -25.0 * dtf;
                }
            }
        }
        break;
    }
    sh2jms.attack_ok = 1;
    if (sh2jms.anime_pause & 1) {
        if (sh2jms.pad[0].hold) {
            if (sh2jms.atk_reserve[0] || sh2jms.atk_reserve[1]) {
                sh2jms.lower_prev = JMS_ST_L_ATTACK;
                player_flg_on(&sh2jms.lower_st_flg, 0x10000000);
                player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
            } else {
                player_flg_on(&sh2jms.lower_st_flg, 0x4000000);
            }
        } else {
            player_flg_on(&sh2jms.lower_st_flg, 0x8000000);
        }
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1864
/** Upper-body attack state: queues the weapon's hit checks. */
void upper_attack(struct SubCharacter *p) {
    struct _AnimeInfo *a_info;
    int wep;

    a_info = shCharacterAnimeGetInfo_(p, 1);
    wep = PlayerNowItemName(sh2jms.weapon);
    if (sh2jms.lock_on) {
        PlayerCheckAimingToEnemy(p, sh2jms.target);
    }
    if (sh2jms.attack_ok) {
        if (sh2jms.anime_pause & 8) {
            sh2jms.anime_pause -= 8;
        }
        shCharacterAnimeRestart_(p, 1);
    } else {
        shCharacterAnimePause_(p, 1);
        sh2jms.anime_pause |= 8;
    }
    if (!sh2jms.u_anime_st_flg) {
        if (!sh2jms.atk_reserve[0] && shBattleRequestNextAttackIsOk(sh2jms.attack_no, shCharacterAnimeFrameGet_(p, 1))) {
            PlayerRequestAttack(&sh2jms, 1);
        }
    }
    if ((sh2jms.anime_pause & 0xA) == 2) {
        if (sh2jms.pad[0].hold) {
            sh2jms.atk_reserve[0] = sh2jms.atk_reserve[1];
            sh2jms.atk_reserve[1] = 0;
            if (sh2jms.atk_reserve[0]) {
                sh2jms.atk_count++;
                sh2jms.upper_prev = JMS_ST_U_ATTACK;
                player_flg_on(&sh2jms.upper_st_flg, 0x10000000);
                player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
            } else {
                shCharacterAnimePause_(p, 1);
                player_flg_on(&sh2jms.upper_st_flg, 0x4000000);
                sh2jms.shotgun_dir = sh2jms.shotgun_prev = 1;
            }
        } else {
            shCharacterAnimePause_(p, 1);
            player_flg_on(&sh2jms.upper_st_flg, 0x8000000);
        }
        if (sh2jms.atk_type == 3 && a_info->speed > 0) {
            if (sh2jms.hold_type == 0) {
                sh2jms.hold_type = 1;
            } else {
                sh2jms.hold_type = 0;
            }
        }
        sh2jms.player->battle.atk_result = 0;
    } else {
        if (!sh2jms.shoot_val && sh2jms.reload_val) {
            struct _AnimeInfo *a_info;
            unsigned short name1;
            unsigned short name2;
            unsigned short frame;

            a_info = shCharacterAnimeGetInfo_(p, 1);
            switch (sh2jms.weapon) {
            case 1:
                name1 = 0xD1;
                name2 = 0xD2;
                frame = 0x15;
                break;
            case 2:
                name1 = 0x104;
                name2 = name1;
                frame = 0xC;
                break;
            case 3:
                name1 = 0x130;
                name2 = name1;
                frame = 0x11;
                break;
            case 4:
                name1 = 0x1FE;
                name2 = 0x1FF;
                frame = 7;
                break;
            default:
                return;
            }
            if (a_info->name == name1 || a_info->name == name2) {
                if (shCharacterAnimeFrameGet_(p, 1) >= frame) {
                    ItemWeaponReload(wep, 1);
                    if (sh2jms.weapon == 4) {
                        if (sh2jms.spray_set) {
                            sh2jms.spray_set--;
                        }
                        sh2jms.spray_time = 20.0f * sh2jms.spray_set / 200.0f;
                    }
                    sh2jms.lock_on = 0;
                    sh2jms.hold_type = 0;
                }
            }
        } else {
            if (!sh2jms.attack_no) {
                assert(0);
            }
            switch (sh2jms.weapon) {
            case 8:
            case 7:
            case 4:
                if (sh2jms.attack_ok) {
                    shBattleAttackHitCheckToEnemy(p, sh2jms.weapon, sh2jms.attack_no);
                }
                break;
            default:
                if (!p->battle.atk_result && sh2jms.attack_ok) {
                    shBattleAttackHitCheckToEnemy(p, sh2jms.weapon, sh2jms.attack_no);
                }
                break;
            }
        }
    }
}

/** Lower-body kick state (stomping an enemy lying down). */
void lower_kick(struct SubCharacter *p) {
    PlayerSpeedDownToStand(p);
    if (sh2jms.enemy_liedown) {
        PlayerCheckAimingToEnemy(p, sh2jms.enemy_liedown);
    }
    sh2jms.attack_ok = 1;
    if (sh2jms.anime_pause & 1) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
        player_flg_off(&sh2jms.lower_st_flg, 0x20000000);
    }
}

/* Matching: blank lines keep the next assert on its original source line. */

/** Upper-body kick state: queues the kick's hit check. */
void upper_kick(struct SubCharacter *p) {
    if (p->battle.atk_result != 1) {
        if (!sh2jms.attack_no) {
            assert(0);
        }
        shBattleAttackHitCheckToEnemy(p, sh2jms.weapon, sh2jms.attack_no);
    }
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
        player_flg_off(&sh2jms.upper_st_flg, 0x20000000);
    }
}

/** Lower-body fall state. */
void lower_fall(struct SubCharacter *p) {
    unsigned short frame;

    frame = shCharacterAnimeFrameGet_(p, 2);
    if (frame <= 20) {
        if (p->spd > 0.9f) {
            PlayerSpeedDownToStand(p);
        } else {
            p->spd += 2.5f * dtf;
            p->spd_org = p->spd = (p->spd > 0.9f) ? 0.9f : p->spd;
        }
    } else {
        PlayerSpeedDownToStand(p);
    }
    if (sh2jms.anime_pause & 1) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
    }
}

/** Upper-body fall state: ends when its animation stops. */
void upper_fall(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
        PlayerStatusClear();
    }
}

/** Lower-body damage state (hit reactions, being held). */
void lower_damage(struct SubCharacter *p) {
    struct _AnimeInfo *a_info = shCharacterAnimeGetInfo_(p, 2);
    float damage_angle;
    unsigned short cur_frame;
    float hugging_pos[5][4] = {
        { 52.133198f, 1550.9032f, 36.49471f, 0.0f },
        { 43.538513f, -93.47943f, 353.91226f, 0.0f },
        { 1.8835068e-05f, 0.0f, 328.05243f, 0.0f },
        { 0.0f, 0.0f, 176.79146f, 0.0f },
        { 0.0f, 1496.4872f, 2e-06f, 0.0f },
    };

    cur_frame = shCharacterAnimeFrameGet_(p, 2);
    damage_angle = shAtan2(p->battle.vec[2], p->battle.vec[0]);
    p->spd_roty = shAngleRegulate(damage_angle - p->rot.y);
    if (!l_anime_flg_on(2)) {
        switch (sh2jms.damage_no) {
        case 0x4E28:
        case 0x4E29:
            if (cur_frame > 0 && cur_frame < 8) {
                p->spd = 1.2f * 0.8f;
            } else {
                p->spd = 0.0f;
            }
            break;
        case 0x4E2C:
        case 0x4E2D:
            if (cur_frame > 0 && cur_frame < 8) {
                p->spd = 0.3f;
            } else {
                p->spd = 0.0f;
            }
            break;
        case 0x4E30:
            if (cur_frame > 0 && cur_frame < 8) {
                p->spd = 1.2f;
            } else {
                p->spd = 0.0f;
            }
            break;
        case 0x4E31:
            if (cur_frame > 0 && cur_frame < 8) {
                p->spd = -1.2f;
            } else {
                p->spd = 0.0f;
            }
            break;
        case 0x4E35:
        case 0x4E36:
            p->spd = 0.0f;
            break;
        case 0x4E37:
        case 0x4E38:
            p->spd = 0.0f;
            break;
        case 0x4E39:
        case 0x4E3A:
            if (cur_frame > 0 && cur_frame < 4) {
                p->spd = -0.3f;
            } else {
                p->spd = 0.0f;
            }
            break;
        case 0x4E41:
        case 0x4E42:
            if (cur_frame > 4) {
                p->spd = 0.0f;
            } else {
                p->spd = 0.6f;
            }
            break;
        case 0x4E45:
            p->spd = 0.0f;
            break;
        case 0x4E3D:
        case 0x4E3E:
        case 0x4E3F:
            PlayerCheckHuggedPos(hugging_pos[1], 1, 500.0 * dtf);
            break;
        case 0x4E46:
        case 0x4E47:
        case 0x4E48:
            PlayerCheckHuggedPos(hugging_pos[0], 1, 500.0 * dtf);
            break;
        case 0x4E4A:
        case 0x4E4B:
        case 0x4E4D:
            PlayerCheckHuggedPos(hugging_pos[2], 1, 500.0 * dtf);
            break;
        case 0x4E4E:
        case 0x4E4F:
        case 0x4E51:
            PlayerCheckHuggedPos(hugging_pos[3], 0, 500.0 * dtf);
            break;
        case 0x4E58:
        case 0x4E56:
        case 0x4E57:
            PlayerCheckHuggedPos(hugging_pos[4], 1, 500.0 * dtf);
            break;
        default:
            p->spd = 0.0f;
            break;
        }
    }
    p->spd_org = p->spd;
    if (sh2jms.hugging_gauge || sh2jms.hug_status == 3) {
        switch (sh2jms.hug_status) {
        case 1:
            if (sh2jms.anime_pause & 1) {
                player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
                player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
                sh2jms.hug_status = 2;
            }
            break;
        case 2:
            if (!sh2jms.dead) {
                PlayerCheckHuggingButton();
                if (sh2jms.hugging_gauge == 0.0f) {
                    player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
                    player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
                    sh2jms.hug_status = 3;
                } else {
                    if (p->battle.hp <= 0.0f) {
                        sh2jms.dead = 1;
                    }
                }
            } else {
                player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
                player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
                if (sh2jms.player->battle.target->kind != 0x203) {
                    sh2jms.hug_status = 3;
                } else {
                    sh2jms.hug_status = 4;
                }
            }
            break;
        case 4:
        case 3:
            if (sh2jms.anime_pause & 1) {
                if (sh2jms.dead) {
                    if (sh2jms.player->battle.target->kind == 0x203) {
                        sh2jms.dead = 3;
                    } else {
                        sh2jms.dead = 2;
                    }
                }
                if (sh2jms.player->battle.target->kind != 0x203 || sh2jms.hug_status == 3) {
                    sh2jms.hug_status = 0;
                }
            }
            break;
        }
    } else {
        if (sh2jms.anime_pause & 1) {
            PlayerStatusClear();
            if (sh2jms.dead) {
                sh2jms.dead = 2;
            }
            if (!sh2jms.dead) {
                if (!sh2jms.motion_no) {
                    player_flg_on(&sh2jms.lower_st_flg, 1);
                    player_flg_on(&sh2jms.upper_st_flg, 1);
                } else {
                    player_flg_on(&sh2jms.upper_st_flg, 1);
                    player_flg_on(&sh2jms.lower_st_flg, 1);
                }
                player_flg_off(&sh2jms.lower_st_flg, 0x2000000);
                player_flg_off(&sh2jms.upper_st_flg, 0x2000000);
            }
        }
    }
}

/** Upper-body damage state (empty). */
void upper_damage(struct SubCharacter *p) {
}

/** Lower-body to-stand state (empty). */
void lower_to_stand(struct SubCharacter *p) {
}

/** Upper-body to-stand state (empty). */
void upper_to_stand(struct SubCharacter *p) {
}

/** Upper-body bumped-into-a-wall state: ends when its animation stops. */
void upper_wall_f(struct SubCharacter *p) {
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
        PlayerStatusClear();
    }
}

/** Lower-body event state: special handling for some event animations. */
void lower_event(struct SubCharacter *p) {
    switch (sh2jms.event_anime) {
    case 0x68:
    case 0x6B:
        break;
    case 0x4E23: {
        float tgt_pos[4] = { -62093.0f, 0.0f, 60734.0f, 0.0f };
        float target;

        p->spd = 0.0f; p->spd_org = 0.0f;
        target = shAtan2(tgt_pos[2] - p->pos.z, tgt_pos[0] - p->pos.x);
        close_to_angle_target(&p->rot.y, target, -3.1415927f, 3.1415927f, 8.0f);
        break;
    }
    default:
        p->spd = 0.0f; p->spd_org = 0.0f;
        break;
    }
}

/** Upper-body event state: ends the event animation when the event releases James. */
void upper_event(struct SubCharacter *p) {
    if (!(p->status & 0x4000)) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
        player_flg_on(&sh2jms.upper_st_flg, 1);
        player_flg_off(&sh2jms.lower_st_flg, 0x80000000);
        PlayerStatusClear();
        sh2jms.event_anime = 0;
    }
}
