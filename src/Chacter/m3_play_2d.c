/*
 * James's movement in 2D control mode (the pad's 2D settings, as opposed to m3_play_3d): the
 * lower- and upper-body state machines (stand, walk, run, turn, step, jump, attack, ...),
 * selected per frame from the pad input.
 */
#include "sh2.h"
#include "m3_helpers.h"

static int dt;
static float dtf;
static float angle;

static void PlayerChangeAngleToCameraY(struct SubCharacter *p) {
    float roty_tmp;
    float mov_angle;
    float spd_close_to;

    roty_tmp = shAngleRegulate(p->rot.y - p->spd_roty);
    mov_angle = roty_tmp / (3.1415927f - 0.05) * (20.0f * dtf);
    if (roty_tmp >= 0.0f) {
        if (roty_tmp - mov_angle <= 0.0f) {
            p->rot.y = p->spd_roty;
        } else {
            p->rot.y -= mov_angle;
        }
    } else {
        if (roty_tmp - mov_angle >= 0.0f) {
            p->rot.y = p->spd_roty;
        } else {
            p->rot.y -= mov_angle;
        }
    }
    p->rot.y = shAngleRegulate(p->rot.y);
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 234
static void lower_walk_2d_nata(struct SubCharacter *p) {
    short frame;
    float move_spd_tbl[24] = {
        0.9f, 1.05f, 0.45f, 0.045f, 0.3f, 0.9f, 1.05f, 1.2f, 1.425f, 1.5f, 1.425f, 1.35f,
        0.975f, 0.6f, 0.15f, 0.045f, 0.3f, 0.6f, 0.9f, 0.975f, 1.05f, 1.2f, 1.35f, 1.05f,
    };
    short motion_spd_tbl[24] = {
        -0x100, -0x100, -0x200, -0x280, -0x200, -0x100, -0x100, -0x100, -0x100, -0x100, -0x100, -0x100,
        -0x100, -0x180, -0x200, -0x280, -0x180, -0x100, -0x100, -0x100, -0x100, -0x100, -0x100, -0x100,
    };

    frame = shCharacterAnimeFrameGet_(p, 2);
    p->spd = move_spd_tbl[frame];
    assert_dw(frame >= 0 && frame <= 23); /* Matching: do/while(0) form (its nop) */
    shCharacterAnimeSpeedAdd_(p, 2, motion_spd_tbl[frame]);
    shCharacterAnimeSpeedAdd_(p, 1, motion_spd_tbl[frame]);
}

static void lower_stand_2d(struct SubCharacter *p) {
    lower_stand(p);
}

static void upper_stand_2d(struct SubCharacter *p) {
    upper_stand(p);
}

static void lower_relax_2d(struct SubCharacter *p) {
    lower_relax(p);
}

static void upper_relax_2d(struct SubCharacter *p) {
    upper_relax(p);
}

static void lower_alert_2d(struct SubCharacter *p) {
    lower_alert(p);
}

static void upper_alert_2d(struct SubCharacter *p) {
    upper_alert(p);
}

static void lower_tired_2d(struct SubCharacter *p) {
    lower_tired(p);
}

static void upper_tired_2d(struct SubCharacter *p) {
    upper_tired(p);
}

static void lower_ready_2d(struct SubCharacter *p) {
    lower_ready(p);
}

static void upper_ready_2d(struct SubCharacter *p) {
    upper_ready(p);
}

static void lower_readyoff_2d(struct SubCharacter *p) {
    lower_readyoff(p);
}

static void upper_readyoff_2d(struct SubCharacter *p) {
    upper_readyoff(p);
}

static void lower_walk_2d(struct SubCharacter *p) {
    short ana_spd;

    if (sh2jms.weapon == 8) {
        lower_walk_2d_nata(p);
    } else {
        switch (sh2jms.ctrl_unit) {
        case 0:
            if (p->spd > 0.0f && sh2jms.lower_prev == JMS_ST_L_BACK) {
                p->spd -= 7.5f * dtf;
                p->spd = (p->spd > 0.0f) ? p->spd : 0.0f;
            } else if (p->spd > 1.5f) {
                p->spd -= 5.0f * dtf;
                p->spd = (p->spd > 1.5f) ? p->spd : 1.5f;
            } else {
                p->spd += 2.5f * dtf;
                p->spd = (p->spd > 1.5f) ? 1.5f : p->spd;
            }
            break;
        case 1:
            p->spd = 1.5f * sh2jms.lstick_p;
            shCharacterAnimeSpeedAdd_(p, 2, ana_spd = 896.0f * -(1.0f - sh2jms.lstick_p));
            break;
        }
    }
    if (sh2jms.lock_on && sh2jms.pad[0].dash) {
        p->spd_org = 1.5f * p->spd;
        switch (sh2jms.ctrl_unit) {
        case 0:
            shCharacterAnimeSpeedAdd_(p, 2, 0x200);
            break;
        case 1:
            shCharacterAnimeSpeedAdd_(p, 2, ana_spd + 512.0f * sh2jms.lstick_p);
            break;
        }
        p->spd_org = (p->spd_org > 2.25f) ? 2.25f : p->spd_org;
    } else {
        p->spd_org = p->spd;
    }
    PlayerSetAttackWithWalkIsOk();
}

static void upper_walk_2d(struct SubCharacter *p) {
    if (sh2jms.weapon != 8) {
        switch (sh2jms.ctrl_unit) {
        case 1:
            shCharacterAnimeSpeedAdd_(p, 1, 896.0f * -(1.0f - sh2jms.lstick_p));
            break;
        }
    }
}

static void lower_back_2d(struct SubCharacter *p) {
    static float lstickY_tmp = 0.0f;
    short ana_spd;

    if (sh2jms.upper_now != JMS_ST_U_ATTACK) {
        lstickY_tmp = sh2jms.lstick_y;
    }
    switch (sh2jms.ctrl_unit) {
    case 0:
        if (p->spd > 0.0f && sh2jms.lower_prev != JMS_ST_L_BACK) {
            p->spd -= 10.0f * dtf;
            p->spd = (p->spd > 0.0f) ? p->spd : 0.0f;
            if (p->spd == 0.0f) {
                sh2jms.lower_prev = JMS_ST_L_BACK;
            }
        } else if (p->spd > 1.2f) {
            p->spd -= 5.0f * dtf;
            p->spd = (p->spd > 1.2f) ? p->spd : 1.2f;
        } else {
            p->spd += 5.0f * dtf;
            p->spd = (p->spd > 1.2f) ? 1.2f : p->spd;
        }
        break;
    case 1:
        p->spd = 1.2f * sh2jms.lstick_p;
        if (sh2jms.pad[0].dash) {
            p->spd *= 1.5f;
        }
        shCharacterAnimeSpeedAdd_(p, 2, ana_spd = 512.0f * -(1.0f - sh2jms.lstick_p));
        break;
    }
    if (sh2jms.pad[0].dash) {
        p->spd_org = 1.5f * p->spd;
        switch (sh2jms.ctrl_unit) {
        case 0:
            shCharacterAnimeSpeedAdd_(p, 2, 0x200);
            break;
        case 1:
            shCharacterAnimeSpeedAdd_(p, 2, sh2jms.pad[0].pad2d.pow * 10);
            break;
        }
        p->spd_org = (p->spd_org > 1.2f * 1.5f) ? 1.2f * 1.5f : p->spd_org;
    } else {
        p->spd_org = p->spd;
    }
    PlayerSetAttackWithWalkIsOk();
}

static void lower_lswalk_2d(struct SubCharacter *p) {
    short ana_spd;

    switch (sh2jms.ctrl_unit) {
    case 0:
        if (p->spd > 0.0f && sh2jms.lower_prev == JMS_ST_L_BACK) {
            p->spd -= 3.4f * 1.5f * dtf;
            p->spd = (p->spd > 0.0f) ? p->spd : 0.0f;
        } else if (p->spd > 1.3f) {
            p->spd -= 3.4f * p->spd;
            p->spd = (p->spd > 1.3f) ? p->spd : 1.3f;
        } else {
            p->spd += 1.7f * dtf;
            p->spd = (p->spd > 1.3f) ? 1.3f : p->spd;
        }
        break;
    case 1:
        p->spd = 1.3f * sh2jms.lstick_p;
        shCharacterAnimeSpeedAdd_(p, 2, ana_spd = 1001.0f * -(1.0f - sh2jms.lstick_p));
        break;
    }
    p->spd_org = p->spd;
    if (sh2jms.pad[0].dash) {
        p->spd_org = 1.5f * p->spd;
        switch (sh2jms.ctrl_unit) {
        case 0:
            shCharacterAnimeSpeedAdd_(p, 2, 0x200);
            break;
        case 1:
            shCharacterAnimeSpeedAdd_(p, 2, ana_spd + 512.0f * sh2jms.lstick_p);
            break;
        }
        p->spd_org = (p->spd_org > 1.3f * 1.5f) ? 1.3f * 1.5f : p->spd_org;
    }
    PlayerSetAttackWithWalkIsOk();
}

static void lower_rswalk_2d(struct SubCharacter *p) {
    short ana_spd;

    switch (sh2jms.ctrl_unit) {
    case 0:
        if (p->spd > 0.0f && sh2jms.lower_prev == JMS_ST_L_BACK) {
            p->spd -= 3.4f * 1.5f * dtf;
            p->spd = (p->spd > 0.0f) ? p->spd : 0.0f;
        } else if (p->spd > 1.3f) {
            p->spd -= 5.0f * p->spd;
            p->spd = (p->spd > 1.3f) ? p->spd : 1.3f;
        } else {
            p->spd += 1.7f * dtf;
            p->spd = (p->spd > 1.3f) ? 1.3f : p->spd;
        }
        break;
    case 1:
        p->spd = 1.3f * sh2jms.lstick_p;
        shCharacterAnimeSpeedAdd_(p, 2, ana_spd = 1024.0f * -(1.0f - sh2jms.lstick_p));
        break;
    }
    p->spd_org = p->spd;
    if (sh2jms.pad[0].dash) {
        p->spd_org = 1.5f * p->spd;
        switch (sh2jms.ctrl_unit) {
        case 0:
            shCharacterAnimeSpeedAdd_(p, 2, 0x200);
            break;
        case 1:
            shCharacterAnimeSpeedAdd_(p, 2, ana_spd + 512.0f * sh2jms.lstick_p);
            break;
        }
        p->spd_org = (p->spd_org > 1.3f * 1.5f) ? 1.3f * 1.5f : p->spd_org;
    }
    PlayerSetAttackWithWalkIsOk();
}

static void lower_run1_2d(struct SubCharacter *p) {
    static signed char lstickY_tmp = 0;
    float target_speed;

    lstickY_tmp = sh2jms.pad[0].lstickY;
    switch (sh2jms.ctrl_unit) {
    case 0:
        if (p->spd > 3.5f) {
            p->spd -= 3.0f * dtf;
            p->spd = (p->spd > 3.5f) ? p->spd : 3.5f;
        } else {
            p->spd += 3.0f * dtf;
            p->spd = (p->spd > 3.5f) ? 3.5f : p->spd;
        }
        break;
    case 1:
        target_speed = 3.5f * sh2jms.lstick_p;
        if (p->spd < target_speed) {
            p->spd += 3.0f * dtf;
        } else {
            p->spd -= 3.0f * dtf;
        }
        p->spd = (p->spd > 2.0f) ? p->spd : 2.0f;
        p->spd = (p->spd > target_speed) ? target_speed : p->spd;
        break;
    }
    p->spd_org = p->spd;
    if (p->spd >= 3.5f && !sh2jms.l_anime_st_flg) {
        player_flg_on(&sh2jms.lower_st_flg, 0x2000);
        player_flg_off(&sh2jms.lower_st_flg, 0x1000);
    }
    PlayerSetAttackWithRunIsOk();
}

static void upper_run1_2d(struct SubCharacter *p) {
    upper_run1(p);
}

static void lower_run2_2d(struct SubCharacter *p) {
    static signed char lstickY_tmp = 0;
    float target_speed;

    lstickY_tmp = sh2jms.pad[0].lstickY;
    if (ItemAmpolueEfficacy() == 0.0f) {
        sh2jms.tired += dt;
    }
    switch (sh2jms.ctrl_unit) {
    case 0:
        if (p->spd > 4.0f) {
            p->spd -= 3.0f * dtf;
            p->spd = (p->spd > 4.0f) ? p->spd : 4.0f;
        } else {
            p->spd += 3.0f * dtf;
            p->spd = (p->spd > 4.0f) ? 4.0f : p->spd;
        }
        if (p->spd == 4.0f && !sh2jms.l_anime_st_flg && sh2jms.tired < sh2jms.tired_max && !sh2jms.map_mode) {
            player_flg_on(&sh2jms.lower_st_flg, 0x4000);
            player_flg_off(&sh2jms.lower_st_flg, 0x2000);
        }
        break;
    case 1:
        target_speed = 4.0f * sh2jms.lstick_p;
        if (p->spd < target_speed) {
            p->spd += 3.0f * dtf;
            p->spd = (p->spd > target_speed) ? target_speed : p->spd;
        } else {
            p->spd -= 3.0f * dtf;
            p->spd = (p->spd > target_speed) ? p->spd : target_speed;
        }
        if (p->spd <= 3.5f && !sh2jms.l_anime_st_flg) {
            player_flg_on(&sh2jms.lower_st_flg, 0x1000);
            player_flg_off(&sh2jms.lower_st_flg, 0x2000);
        }
        if (p->spd >= 4.0f && !sh2jms.l_anime_st_flg && sh2jms.tired < sh2jms.tired_max && !sh2jms.map_mode) {
            player_flg_on(&sh2jms.lower_st_flg, 0x4000);
            player_flg_off(&sh2jms.lower_st_flg, 0x2000);
        }
        break;
    }
    p->spd_org = p->spd;
    PlayerSetAttackWithRunIsOk();
    if (!sh2jms.cannot_run) {
        player_flg_on(&sh2jms.lower_st_flg, 0x20000);
    }
}

static void upper_run2_2d(struct SubCharacter *p) {
    upper_run2(p);
}

static void lower_run3_2d(struct SubCharacter *p) {
    static signed char lstickY_tmp = 0;
    float target_speed;

    lstickY_tmp = sh2jms.pad[0].lstickY;
    if (ItemAmpolueEfficacy() == 0.0f) {
        sh2jms.tired += dt;
    }
    switch (sh2jms.ctrl_unit) {
    case 0:
        p->spd += 3.0f * dtf;
        p->spd = (p->spd > 4.8f) ? 4.8f : p->spd;
        if (sh2jms.tired >= sh2jms.tired_max && !sh2jms.l_anime_st_flg) {
            player_flg_on(&sh2jms.lower_st_flg, 0x2000);
            player_flg_off(&sh2jms.lower_st_flg, 0x4000);
        }
        break;
    case 1:
        target_speed = 4.8f * sh2jms.lstick_p;
        if (sh2jms.lower_prev == JMS_ST_L_READY) {
            p->spd = 4.0f;
            target_speed = 4.8f;
        }
        if (p->spd <= target_speed) {
            p->spd += 3.0f * dtf;
        } else {
            p->spd -= 3.0f * dtf;
        }
        p->spd = (p->spd > target_speed) ? target_speed : p->spd;
        if ((p->spd <= 4.0f || sh2jms.tired >= sh2jms.tired_max) && !sh2jms.l_anime_st_flg) {
            player_flg_on(&sh2jms.lower_st_flg, 0x2000);
            player_flg_off(&sh2jms.lower_st_flg, 0x4000);
        }
        break;
    }
    p->spd_org = p->spd;
    PlayerSetAttackWithRunIsOk();
    if (!sh2jms.cannot_run) {
        player_flg_on(&sh2jms.lower_st_flg, 0x20000);
    }
}

static void upper_run3_2d(struct SubCharacter *p) {
    upper_run3(p);
}

static void lower_lsrun_2d(struct SubCharacter *p) {
    float target_speed;

    if (ItemAmpolueEfficacy() == 0.0f) {
        sh2jms.tired += dt;
    }
    switch (sh2jms.ctrl_unit) {
    case 0:
        p->spd += 6.0f * dtf;
        p->spd = (p->spd > 3.8f) ? 3.8f : p->spd;
        break;
    case 1:
        target_speed = 3.8f * sh2jms.lstick_p;
        switch (sh2jms.map_mode) {
        case 0:
            break;
        case 1:
            target_speed *= 0.9f;
            break;
        }
        p->spd = target_speed;
        shCharacterAnimeSpeedAdd_(p, 2, 1536.0f * -(1.0f - sh2jms.lstick_p));
        break;
    }
    p->spd_org = p->spd;
    PlayerSetAttackWithRunIsOk();
    if (!sh2jms.cannot_run) {
        player_flg_on(&sh2jms.lower_st_flg, 0x20000);
    }
}

static void lower_rsrun_2d(struct SubCharacter *p) {
    float target_speed;

    if (ItemAmpolueEfficacy() == 0.0f) {
        sh2jms.tired += dt;
    }
    switch (sh2jms.ctrl_unit) {
    case 0:
        p->spd += 6.0f * dtf;
        p->spd = (p->spd > 3.8f) ? 3.8f : p->spd;
        break;
    case 1:
        target_speed = 3.8f * sh2jms.lstick_p;
        switch (sh2jms.map_mode) {
        case 0:
            break;
        case 1:
            target_speed *= 0.9f;
            break;
        }
        p->spd = target_speed;
        shCharacterAnimeSpeedAdd_(p, 2, 1536.0f * -(1.0f - sh2jms.lstick_p));
        break;
    }
    p->spd_org = p->spd;
    PlayerSetAttackWithRunIsOk();
    if (!sh2jms.cannot_run) {
        player_flg_on(&sh2jms.lower_st_flg, 0x20000);
    }
}

static void lower_jump_2d(void) {
}

static void upper_jump_2d(void) {
}

static void lower_guard_2d(struct SubCharacter *p) {
    unsigned short frame;

    frame = shCharacterAnimeFrameGet_(p, 2);
    if (sh2jms.lower_prev != JMS_ST_L_GUARD) {
        sh2jms.lower_prev = JMS_ST_L_GUARD;
        p->spd_y = 0.0f;
        p->spd = 0.0f;
        p->spd_roty = -3.1415927f;
        player_flg_off(&sh2jms.lower_st_flg, 0x400000);
        player_flg_off(&sh2jms.lower_st_flg, 0x800000);
    } else {
        if (frame <= 7) {
            p->spd = 1.75f;
        } else {
            PlayerSpeedDownToStand(p);
        }
        if (sh2jms.anime_pause & 1) {
            sh2jms.no_damage = 0;
            player_flg_on(&sh2jms.lower_st_flg, 1);
            player_flg_on(&sh2jms.lower_st_flg, 0x100000);
            player_flg_off(&sh2jms.l_anime_st_flg, 1);
        }
    }
}

static void upper_guard_2d(void) {
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
        player_flg_off(&sh2jms.u_anime_st_flg, 1);
    }
}

static void lower_lstep_2d(void) {
}

static void upper_lstep_2d(void) {
}

static void lower_rstep_2d(void) {
}

static void upper_rstep_2d(void) {
}

static void lower_hold_2d(struct SubCharacter *p) {
    lower_hold(p);
}

static void upper_hold_2d(struct SubCharacter *p) {
    upper_hold(p);
}

static void lower_release_2d(struct SubCharacter *p) {
    lower_release(p);
}

static void upper_release_2d(struct SubCharacter *p) {
    upper_release(p);
}

static void lower_attack_2d(struct SubCharacter *p) {
    lower_attack(p);
}

static void upper_attack_2d(struct SubCharacter *p) {
    upper_attack(p);
}

static void lower_kick_2d(struct SubCharacter *p) {
    lower_kick(p);
}

static void upper_kick_2d(struct SubCharacter *p) {
    upper_kick(p);
}

static void lower_fall_2d(struct SubCharacter *p) {
    lower_fall(p);
}

static void upper_fall_2d(struct SubCharacter *p) {
    upper_fall(p);
}

static void lower_damage_2d(struct SubCharacter *p) {
    lower_damage(p);
}

static void upper_damage_2d(struct SubCharacter *p) {
    upper_damage(p);
}

static void lower_to_stand_2d(struct SubCharacter *p) {
    lower_to_stand(p);
}

static void upper_to_stand_2d(struct SubCharacter *p) {
    upper_to_stand(p);
}

static void lower_wall_f_2d(struct SubCharacter *p) {
    unsigned short frame;

    frame = shCharacterAnimeFrameGet_(p, 2);
    if (frame == 4 || frame > 9) {
        p->spd = 0.0f;
    } else if (frame > 4 && frame < 10) {
        p->spd += 5.0f * dtf;
        p->spd = (p->spd > 1.2f) ? 1.2f : p->spd;
    }
    p->spd_org = p->spd;
    if (sh2jms.anime_pause & 1) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
    }
}

static void upper_wall_f_2d(struct SubCharacter *p) {
    upper_wall_f(p);
}

static void lower_event_2d(struct SubCharacter *p) {
    lower_event(p);
}

static void upper_event_2d(struct SubCharacter *p) {
    upper_event(p);
}

static void PlayerUpdateStatus2D(struct SubCharacter *this) {
    dt = shGetDF();
    dtf = shGetDT();
    PlayerSetDT();
    PlayerUpdateStatus(this);
}

static void PlayerUpdateStatusLower2D(struct SubCharacter *this) {
    struct shPlayerWork *w;
    struct PAD_INFO *p;
    struct PAD_2D *p2d;
    struct FVEC cam_pos;
    struct FVEC watch_pos;

    w = &sh2jms;
    p = &sh2jms.pad[0];
    p2d = &sh2jms.pad[0].pad2d;
    vcGetNowCamPos((float *)&cam_pos);
    vcGetNowWatchPos((float *)&watch_pos);
    if (!vcRetCamMvSmoothF() && !sh2jms.cam_chg_flg) {
        sh2jms.cam_chg_flg = 1;
        sh2jms.now_cam_no = !sh2jms.now_cam_no;
    }
    sh2jms.cam_rot_y[sh2jms.now_cam_no] = shAtan2(watch_pos.z - cam_pos.z, watch_pos.x - cam_pos.x);
    switch (sh2jms.upper_now) {
    case JMS_ST_U_EVENT:
        angle = this->spd_roty;
        break;
    case JMS_ST_U_DAMAGE:
    case JMS_ST_U_FALL:
    case JMS_ST_U_WALL_F:
        angle = this->spd_roty = this->rot.y;
        break;
    default:
        if (sh2jms.lock_on) {
            if (sh2jms.lstick_p) {
                this->spd_roty = shAngleRegulate(
                    sh2jms.cam_rot_y[sh2jms.cam_chg_flg ? 1 - sh2jms.now_cam_no : sh2jms.now_cam_no] + p2d->dir);
            } else {
                this->spd_roty = 0.0f;
                sh2jms.cam_chg_flg = 0;
            }
            angle = this->rot.y;
            {
                float lower_motion_angle;

                lower_motion_angle = shAngleRegulate(this->spd_roty - this->rot.y);
                if (lower_motion_angle >= 2.3561945f || lower_motion_angle < -2.3561945f) {
                    p2d->lower_motion = 2;
                } else if (lower_motion_angle >= 0.7853982f) {
                    p2d->lower_motion = 3;
                } else if (lower_motion_angle >= -0.7853982f) {
                    p2d->lower_motion = 1;
                } else {
                    p2d->lower_motion = 4;
                }
            }
        } else {
            if (sh2jms.lstick_p) {
                angle = shAngleRegulate(
                    sh2jms.cam_rot_y[sh2jms.cam_chg_flg ? 1 - sh2jms.now_cam_no : sh2jms.now_cam_no] + p2d->dir);
            } else {
                angle = this->rot.y;
                sh2jms.cam_chg_flg = 0;
            }
            this->spd_roty = angle;
            p2d->lower_motion = 0;
        }
        break;
    }
    if (lower_flg_on(0x1000000)) {
        if (w->lower_now != JMS_ST_L_FALL) {
            lower_st_set(JMS_ST_L_FALL, w);
            lower_flg_set(JMS_ST_L_FALL, w);
            player_flg_on(&w->l_anime_st_flg, 0x40);
        }
        return;
    }
    if (lower_flg_on(0x200000)) {
        if (w->lower_now != JMS_ST_L_GUARD) {
            lower_st_set(JMS_ST_L_GUARD, w);
            lower_flg_set(JMS_ST_L_GUARD, w);
            player_flg_on(&w->l_anime_st_flg, 0x40);
        }
        return;
    }
    if (lower_flg_on(0x2000000)) {
        if (w->lower_now != JMS_ST_L_DAMAGE) {
            lower_st_set(JMS_ST_L_DAMAGE, w);
            lower_flg_set(JMS_ST_L_DAMAGE, w);
            player_flg_on(&w->l_anime_st_flg, 0x40);
        }
        return;
    }
    if (lower_flg_on(0x80000000)) {
        if (w->lower_now != JMS_ST_L_EVENT) {
            lower_st_set(JMS_ST_L_EVENT, w);
            lower_flg_set(JMS_ST_L_EVENT, w);
            player_flg_on(&w->l_anime_st_flg, 0x40);
        }
        return;
    }
    if (w->upper_now == JMS_ST_U_ATTACK) {
        switch (w->lower_now) {
        case JMS_ST_L_WALK:
        case JMS_ST_L_BACK:
        case JMS_ST_L_LSWALK:
        case JMS_ST_L_RSWALK:
            if (sh2jms.weapon == 1 || sh2jms.weapon == 4) {
                if (actwithwep_flg_on(0x40) && !u_anime_flg_on(0x40)) {
                    return;
                }
            } else {
                if (actwithwep_flg_on(0x40) && !u_anime_flg_on(0x40)) {
                    return;
                }
            }
            break;
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_RUN3:
        case JMS_ST_L_LSRUN:
        case JMS_ST_L_RSRUN:
            if (sh2jms.weapon == 1 || sh2jms.weapon == 4) {
                if (actwithwep_flg_on(0x80) && !u_anime_flg_on(0x40)) {
                    return;
                }
            } else {
                if (actwithwep_flg_on(0x80) && !u_anime_flg_on(0x40)) {
                    return;
                }
            }
            break;
        case JMS_ST_L_HOLD:
        case JMS_ST_L_ATTACK:
            if (sh2jms.weapon == 1 || sh2jms.weapon == 4) {
                if (!l_anime_flg_on(0x40)) {
                    return;
                }
            } else {
                if (!l_anime_flg_on(0x40)) {
                    return;
                }
            }
            break;
        }
    }
    if (this->eye.kind == 1 && sh2jms.inner_to_wall <= -0.85f && sh2jms.running_time >= 3.0f &&
        lower_flg_on(0x20000)) {
        lower_st_set(JMS_ST_L_WALL_F, w);
        lower_flg_set(JMS_ST_L_WALL_F, w);
    }
    if (sh2jms.lstick_p && p2d->lower_motion <= 1) {
        PlayerCheckStraightLine(this, this->spd_roty);
        if (p->dash && !sh2jms.cannot_run) {
            if (lower_flg_on(0x4000) && !sh2jms.map_mode &&
                (w->hold_type == -1 || (w->upper_now != JMS_ST_U_ATTACK && actwithwep_flg_on(8)) ||
                 (w->upper_now == JMS_ST_U_ATTACK && actwithwep_flg_on(0x80)))) {
                if (!sh2jms.l_anime_st_flg || l_anime_flg_on(0x40)) {
                    if (w->lower_now != JMS_ST_L_RUN3) {
                        lower_st_set(JMS_ST_L_RUN3, w);
                        lower_flg_set(JMS_ST_L_RUN3, w);
                        if (this->spd < 3.0f) {
                            this->spd = 3.0f;
                        }
                    }
                }
                return;
            }
            if (lower_flg_on(0x2000) &&
                (w->hold_type == -1 || (w->upper_now != JMS_ST_U_ATTACK && actwithwep_flg_on(8)) ||
                 (w->upper_now == JMS_ST_U_ATTACK && actwithwep_flg_on(0x80)))) {
                if (!sh2jms.l_anime_st_flg || l_anime_flg_on(0x40)) {
                    if (w->lower_now != JMS_ST_L_RUN2) {
                        lower_st_set(JMS_ST_L_RUN2, w);
                        lower_flg_set(JMS_ST_L_RUN2, w);
                        if (this->spd < 2.5f) {
                            this->spd = 2.5f;
                        }
                    }
                }
                return;
            }
            if (lower_flg_on(0x1000) &&
                (w->hold_type == -1 || (w->upper_now != JMS_ST_U_ATTACK && actwithwep_flg_on(8)) ||
                 (w->upper_now == JMS_ST_U_ATTACK && actwithwep_flg_on(0x80)))) {
                if (!sh2jms.l_anime_st_flg || l_anime_flg_on(0x40)) {
                    if (w->lower_now != JMS_ST_L_RUN1) {
                        lower_st_set(JMS_ST_L_RUN1, w);
                        lower_flg_set(JMS_ST_L_RUN1, w);
                        if (this->spd < 1.5f) {
                            this->spd = 1.5f;
                        }
                    }
                }
                return;
            }
        }
        if ((!p->dash || w->hold_type != -1 || sh2jms.cannot_run) && lower_flg_on(0x200) &&
            ((w->upper_now != JMS_ST_U_ATTACK && actwithwep_flg_on(4)) ||
             (w->upper_now == JMS_ST_U_ATTACK && actwithwep_flg_on(0x40)))) {
            if (!sh2jms.l_anime_st_flg || l_anime_flg_on(0x40)) {
                if (w->lower_now != JMS_ST_L_WALK) {
                    lower_st_set(JMS_ST_L_WALK, w);
                    lower_flg_set(JMS_ST_L_WALK, w);
                    if (this->spd < 0.6f) {
                        this->spd = 0.6f;
                    }
                }
            }
            return;
        }
    }
    if (sh2jms.lstick_p && p2d->lower_motion == 2 && lower_flg_on(0x100) &&
        ((w->upper_now != JMS_ST_U_ATTACK && actwithwep_flg_on(4)) ||
         (w->upper_now == JMS_ST_U_ATTACK && actwithwep_flg_on(0x40)))) {
        if (!sh2jms.l_anime_st_flg || l_anime_flg_on(0x40)) {
            if (w->lower_now != JMS_ST_L_BACK) {
                lower_st_set(JMS_ST_L_BACK, w);
                lower_flg_set(JMS_ST_L_BACK, w);
                this->spd = 0.6f;
            }
        }
        return;
    }
    if (sh2jms.lstick_p && p2d->lower_motion == 4) {
        {
            float roty;

            switch (w->lower_prev) {
            case JMS_ST_L_WALK:
            case JMS_ST_L_RUN1:
            case JMS_ST_L_RUN2:
            case JMS_ST_L_RUN3:
                roty = this->spd_roty;
                break;
            default:
                roty = -1.5707964f;
                break;
            }
            PlayerCheckStraightLine(this, roty);
        }
        if (p->dash && !sh2jms.cannot_run && lower_flg_on(0x8000) &&
            (w->hold_type == -1 || (w->upper_now != JMS_ST_U_ATTACK && actwithwep_flg_on(8)) ||
             (w->upper_now == JMS_ST_U_ATTACK && actwithwep_flg_on(0x80)))) {
            if (!sh2jms.l_anime_st_flg || l_anime_flg_on(0x40)) {
                if (w->lower_now != JMS_ST_L_LSRUN) {
                    lower_st_set(JMS_ST_L_LSRUN, w);
                    lower_flg_set(JMS_ST_L_LSRUN, w);
                    this->spd = 2.0f;
                }
            }
            return;
        }
        if ((!p->dash || w->hold_type != -1 || sh2jms.cannot_run) && lower_flg_on(0x400) &&
            ((w->upper_now != JMS_ST_U_ATTACK && actwithwep_flg_on(4)) ||
             (w->upper_now == JMS_ST_U_ATTACK && actwithwep_flg_on(0x40)))) {
            if (!sh2jms.l_anime_st_flg || l_anime_flg_on(0x40)) {
                if (w->lower_now != JMS_ST_L_LSWALK) {
                    lower_st_set(JMS_ST_L_LSWALK, w);
                    lower_flg_set(JMS_ST_L_LSWALK, w);
                    if (this->spd < 0.6f) {
                        this->spd = 0.6f;
                    }
                }
            }
            return;
        }
    }
    if (sh2jms.lstick_p && p2d->lower_motion == 3) {
        {
            float roty;

            switch (w->lower_prev) {
            case JMS_ST_L_WALK:
            case JMS_ST_L_RUN1:
            case JMS_ST_L_RUN2:
            case JMS_ST_L_RUN3:
                roty = this->spd_roty;
                break;
            default:
                roty = 1.5707964f;
                break;
            }
            PlayerCheckStraightLine(this, roty);
        }
        if (p->dash && !sh2jms.cannot_run && lower_flg_on(0x10000) &&
            (w->hold_type == -1 || (w->upper_now != JMS_ST_U_ATTACK && actwithwep_flg_on(8)) ||
             (w->upper_now == JMS_ST_U_ATTACK && actwithwep_flg_on(0x80)))) {
            if (!sh2jms.l_anime_st_flg || l_anime_flg_on(0x40)) {
                if (w->lower_now != JMS_ST_L_RSRUN) {
                    lower_st_set(JMS_ST_L_RSRUN, w);
                    lower_flg_set(JMS_ST_L_RSRUN, w);
                    this->spd = 2.0f;
                }
            }
            return;
        }
        if ((!p->dash || w->hold_type != -1 || sh2jms.cannot_run) && lower_flg_on(0x800) &&
            ((w->upper_now != JMS_ST_U_ATTACK && actwithwep_flg_on(4)) ||
             (w->upper_now == JMS_ST_U_ATTACK && actwithwep_flg_on(0x40)))) {
            if (!sh2jms.l_anime_st_flg || l_anime_flg_on(0x40)) {
                if (w->lower_now != JMS_ST_L_RSWALK) {
                    lower_st_set(JMS_ST_L_RSWALK, w);
                    lower_flg_set(JMS_ST_L_RSWALK, w);
                    if (this->spd < 0.6f) {
                        this->spd = 0.6f;
                    }
                }
            }
            return;
        }
    }
    if (!w->l_anime_st_flg && shPadPress(0, key_config.dash) && sh2jms.weapon != 8 && lower_flg_on(0x10) &&
        !PlayerSearchVIewButtonCheck()) {
        if (w->lower_now != JMS_ST_L_READY) {
            lower_st_set(JMS_ST_L_READY, w);
            lower_flg_set(JMS_ST_L_READY, w);
        }
        return;
    }
    if (lower_flg_on(0x20) && !PlayerSearchVIewButtonCheck()) {
        if (w->lower_now != JMS_ST_L_READYOFF) {
            lower_st_set(JMS_ST_L_READYOFF, w);
            lower_flg_set(JMS_ST_L_READYOFF, w);
        }
        return;
    }
    if (lower_flg_on(8) && !PlayerSearchVIewButtonCheck()) {
        if (w->lower_now != JMS_ST_L_TIRED) {
            lower_st_set(JMS_ST_L_TIRED, w);
            lower_flg_set(JMS_ST_L_TIRED, w);
        }
        return;
    }
    if (lower_flg_on(1)) {
        if (w->lower_now != JMS_ST_L_STAND) {
            lower_st_set(JMS_ST_L_STAND, w);
            lower_flg_set(JMS_ST_L_STAND, w);
            sh2jms.no_damage = 0;
            sh2jms.player->battle.id = 0;
            if (sh2jms.lower_prev == JMS_ST_L_DAMAGE) {
                sh2jms.muteki_time = 2.0f;
            }
        }
        return;
    }
    if (!shPadPress(0, key_config.ready)) {
        w->non_input += dtf;
    }
    if (w->non_input >= 10.0f && !PlayerSearchVIewButtonCheck()) {
        if (w->enemy_around) {
            if (lower_flg_on(4)) {
                if (w->lower_now != JMS_ST_L_ALERT) {
                    lower_st_set(JMS_ST_L_ALERT, w);
                    lower_flg_set(JMS_ST_L_ALERT, w);
                }
                return;
            }
        } else if (lower_flg_on(2) && w->lower_now != JMS_ST_L_RELAX) {
            lower_st_set(JMS_ST_L_RELAX, w);
            lower_flg_set(JMS_ST_L_RELAX, w);
        }
    }
}

static void PlayerUpdateStatusUpper2D(struct SubCharacter *this) {
    struct shPlayerWork *w;
    struct PAD_INFO *p;
    struct PAD_INFO *p_pre;
    struct PAD_3D *p3d;

    w = &sh2jms;
    p = &sh2jms.pad[0];
    /* Matching: p_pre and p3d are in the original's DWARF but never read (dead stores). */
    p_pre = &sh2jms.pad[1];
    p3d = &sh2jms.pad[0].pad3d;
    switch (sh2jms.lower_now) {
    case JMS_ST_L_GUARD:
        if (w->upper_now != JMS_ST_U_GUARD) {
            upper_st_set(JMS_ST_U_GUARD, w);
            upper_flg_set(JMS_ST_U_GUARD, w);
            player_flg_on(&w->u_anime_st_flg, 0x40);
        }
        return;
    case JMS_ST_L_DAMAGE:
        if (w->upper_now != JMS_ST_U_DAMAGE) {
            upper_st_set(JMS_ST_U_DAMAGE, w);
            upper_flg_set(JMS_ST_U_DAMAGE, w);
            player_flg_on(&w->u_anime_st_flg, 0x40);
        }
        return;
    case JMS_ST_L_FALL:
        if (w->upper_now != JMS_ST_U_FALL) {
            upper_st_set(JMS_ST_U_FALL, w);
            upper_flg_set(JMS_ST_U_FALL, w);
            player_flg_on(&w->u_anime_st_flg, 0x40);
        }
        return;
    case JMS_ST_L_WALL_F:
        if (w->upper_now != JMS_ST_U_WALL_F) {
            upper_st_set(JMS_ST_U_WALL_F, w);
            upper_flg_set(JMS_ST_U_WALL_F, w);
        }
        return;
    case JMS_ST_L_EVENT:
        if (w->upper_now != JMS_ST_U_EVENT) {
            upper_st_set(JMS_ST_U_EVENT, w);
            upper_flg_set(JMS_ST_U_EVENT, w);
        }
        return;
    }
    if (w->weapon && !u_anime_flg_on(2)) {
        if (p->hold) {
            if (upper_flg_on(0x4000000)) {
                if (w->upper_now != JMS_ST_U_HOLD) {
                    upper_st_set(JMS_ST_U_HOLD, w);
                    upper_flg_set(JMS_ST_U_HOLD, w);
                    actwithwep_flg_set(w->weapon, w);
                    if (sh2jms.upper_prev != JMS_ST_U_ATTACK) {
                        switch (playing.battle_level) {
                        case 0:
                        case 1:
                            ItemWeaponReload(PlayerNowItemName(sh2jms.weapon), 1);
                            break;
                        }
                    }
                    PlayerGetTarget();
                    PlayerCheckBothArmsAngle(this);
                } else if (w->target) {
                    if (shPadTrigger(0, key_config.right_move)) {
                        PlayerChangeTarget(1);
                    }
                    if (shPadTrigger(0, key_config.left_move)) {
                        PlayerChangeTarget(-1);
                    }
                    PlayerCheckBothArmsAngle(this);
                }
                if (w->weapon == 1 && !w->target && w->lock_on) {
                    player_flg_on(&w->u_anime_st_flg, 0x40);
                    if (w->lower_now == JMS_ST_L_HOLD) {
                        player_flg_on(&w->l_anime_st_flg, 0x40);
                    }
                }
                if (w->target) {
                    w->lock_on = 1;
                } else {
                    w->lock_on = 0;
                }
                return;
            }
        } else if (upper_flg_on(0x8000000)) {
            if (w->upper_now != JMS_ST_U_RELEASE) {
                upper_st_set(JMS_ST_U_RELEASE, w);
                upper_flg_set(JMS_ST_U_RELEASE, w);
                player_flg_on(&w->u_anime_st_flg, 0x40);
                actwithwep_flg_set(0, w);
                sh2jms.target = NULL;
            }
            return;
        }
    }
    switch (w->lower_now) {
    case JMS_ST_L_STAND:
        if (upper_flg_on(1) && w->upper_now != JMS_ST_U_STAND) {
            upper_st_set(JMS_ST_U_STAND, w);
            upper_flg_set(JMS_ST_U_STAND, w);
        }
        break;
    case JMS_ST_L_RELAX:
        if (upper_flg_on(2) && w->upper_now != JMS_ST_U_RELAX) {
            upper_st_set(JMS_ST_U_RELAX, w);
            upper_flg_set(JMS_ST_U_RELAX, w);
        }
        break;
    case JMS_ST_L_ALERT:
        if (upper_flg_on(4) && w->upper_now != JMS_ST_U_ALERT) {
            upper_st_set(JMS_ST_U_ALERT, w);
            upper_flg_set(JMS_ST_U_ALERT, w);
        }
        break;
    case JMS_ST_L_TIRED:
        if (upper_flg_on(8) && w->upper_now != JMS_ST_U_TIRED) {
            upper_st_set(JMS_ST_U_TIRED, w);
            upper_flg_set(JMS_ST_U_TIRED, w);
        }
        break;
    case JMS_ST_L_READY:
        if (!w->u_anime_st_flg && upper_flg_on(0x10) && w->upper_now != JMS_ST_U_READY) {
            upper_st_set(JMS_ST_U_READY, w);
            upper_flg_set(JMS_ST_U_READY, w);
        }
        break;
    case JMS_ST_L_READYOFF:
        if (upper_flg_on(0x20) && w->upper_now != JMS_ST_U_READYOFF) {
            upper_st_set(JMS_ST_U_READYOFF, w);
            upper_flg_set(JMS_ST_U_READYOFF, w);
            player_flg_on(&w->u_anime_st_flg, 2);
            player_flg_off(&w->upper_st_flg, 0x4000);
        }
        break;
    case JMS_ST_L_BACK:
        if (upper_flg_on(0x100) && w->upper_now != JMS_ST_U_BACK) {
            upper_st_set(JMS_ST_U_BACK, w);
            upper_flg_set(JMS_ST_U_BACK, w);
            if (sh2jms.act_with_wep & 1) {
                player_flg_on(&w->upper_st_flg, 0x4000000);
            }
        }
        break;
    case JMS_ST_L_WALK:
        if (upper_flg_on(0x200) && w->upper_now != JMS_ST_U_WALK) {
            upper_st_set(JMS_ST_U_WALK, w);
            upper_flg_set(JMS_ST_U_WALK, w);
            if (sh2jms.act_with_wep & 1) {
                player_flg_on(&w->upper_st_flg, 0x4000000);
            }
        }
        break;
    case JMS_ST_L_RUN1:
        if (upper_flg_on(0x1000) && w->upper_now != JMS_ST_U_RUN1) {
            upper_st_set(JMS_ST_U_RUN1, w);
            upper_flg_set(JMS_ST_U_RUN1, w);
            if (sh2jms.act_with_wep & 2) {
                player_flg_on(&w->upper_st_flg, 0x4000000);
            }
        }
        break;
    case JMS_ST_L_RUN2:
        if (upper_flg_on(0x2000) && w->upper_now != JMS_ST_U_RUN2) {
            upper_st_set(JMS_ST_U_RUN2, w);
            upper_flg_set(JMS_ST_U_RUN2, w);
            if (sh2jms.act_with_wep & 2) {
                player_flg_on(&w->upper_st_flg, 0x4000000);
            }
        }
        break;
    case JMS_ST_L_RUN3:
        if (upper_flg_on(0x4000) && w->upper_now != JMS_ST_U_RUN3) {
            upper_st_set(JMS_ST_U_RUN3, w);
            upper_flg_set(JMS_ST_U_RUN3, w);
            if (sh2jms.act_with_wep & 2) {
                player_flg_on(&w->upper_st_flg, 0x4000000);
            }
        }
        break;
    case JMS_ST_L_ATTACK:
    case JMS_ST_L_KICK:
    case JMS_ST_L_FALL:
    case JMS_ST_L_TO_STAND:
        break;
    }
}

/* Matching: K&R definition; called with arguments it ignores (the DWARF shows none). */
static void PlayerUpdateStatusLower2nd2D() {
    struct shPlayerWork *w;

    w = &sh2jms;
    switch (sh2jms.upper_now) {
    case JMS_ST_U_HOLD:
        if (lower_flg_on(0x4000000) && w->lower_now != JMS_ST_L_HOLD) {
            lower_st_set(JMS_ST_L_HOLD, w);
            lower_flg_set(JMS_ST_L_HOLD, w);
        }
        break;
    case JMS_ST_U_RELEASE:
        if (lower_flg_on(0x8000000) && w->lower_now != JMS_ST_L_RELEASE) {
            lower_st_set(JMS_ST_L_RELEASE, w);
            lower_flg_set(JMS_ST_L_RELEASE, w);
            player_flg_on(&w->l_anime_st_flg, 0x40);
        }
        break;
    case JMS_ST_U_ATTACK:
        if (lower_flg_on(0x10000000)) {
            if (w->lower_now != JMS_ST_L_ATTACK || w->lower_prev == JMS_ST_L_ATTACK) {
                lower_st_set(JMS_ST_L_ATTACK, w);
                lower_flg_set(JMS_ST_L_ATTACK, w);
            }
        } else if (lower_flg_on(0x4000000) && w->lower_now != JMS_ST_L_HOLD) {
            lower_st_set(JMS_ST_L_HOLD, w);
            lower_flg_set(JMS_ST_L_HOLD, w);
            player_flg_off(&w->lower_st_flg, 0x1000);
            player_flg_off(&w->lower_st_flg, 0x2000);
            player_flg_off(&w->lower_st_flg, 0x4000);
            player_flg_off(&w->lower_st_flg, 0x10000);
            player_flg_off(&w->lower_st_flg, 0x8000);
            player_flg_off(&w->lower_st_flg, 0x100);
            player_flg_off(&w->lower_st_flg, 0x200);
            player_flg_off(&w->lower_st_flg, 0x800);
            player_flg_off(&w->lower_st_flg, 0x400);
        }
        break;
    case JMS_ST_U_KICK:
        break;
    }
    switch (w->lower_now) {
    case JMS_ST_L_RUN1:
    case JMS_ST_L_RUN2:
    case JMS_ST_L_RUN3:
    case JMS_ST_L_RSRUN:
    case JMS_ST_L_LSRUN:
        w->running = 1;
        break;
    default:
        w->running = 0;
        break;
    }
}

static void (*func_list_lower[32])(struct SubCharacter *) = {
    lower_stand_2d,   lower_relax_2d,    lower_alert_2d,     lower_tired_2d,   lower_ready_2d,
    lower_readyoff_2d, NULL,             NULL,               lower_back_2d,    lower_walk_2d,
    lower_lswalk_2d,  lower_rswalk_2d,   lower_run1_2d,      lower_run2_2d,    lower_run3_2d,
    lower_lsrun_2d,   lower_rsrun_2d,    lower_wall_f_2d,    NULL,             NULL,
    (void (*)(struct SubCharacter *))lower_jump_2d, lower_guard_2d,
    (void (*)(struct SubCharacter *))lower_lstep_2d, (void (*)(struct SubCharacter *))lower_rstep_2d,
    lower_fall_2d,    lower_damage_2d,   lower_hold_2d,      lower_release_2d, lower_attack_2d,
    lower_kick_2d,    lower_to_stand_2d, lower_event_2d,
};

static void (*func_list_upper[32])(struct SubCharacter *) = {
    upper_stand_2d,   upper_relax_2d,    upper_alert_2d,     upper_tired_2d,   upper_ready_2d,
    upper_readyoff_2d, NULL,             NULL,               NULL,             upper_walk_2d,
    NULL,             NULL,              upper_run1_2d,      upper_run2_2d,    upper_run3_2d,
    NULL,             NULL,              upper_wall_f_2d,    NULL,             NULL,
    (void (*)(struct SubCharacter *))upper_jump_2d, (void (*)(struct SubCharacter *))upper_guard_2d,
    (void (*)(struct SubCharacter *))upper_lstep_2d, (void (*)(struct SubCharacter *))upper_rstep_2d,
    upper_fall_2d,    upper_damage_2d,   upper_hold_2d,      upper_release_2d, upper_attack_2d,
    upper_kick_2d,    upper_to_stand_2d, upper_event_2d,
};


/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 2609
/** Runs James's lower- and upper-body state functions and computes his movement for this frame (2D control). */
void PlayerUpdatePosition2D(struct SubCharacter *this) {
    void (*lower_func)(struct SubCharacter *);
    void (*upper_func)(struct SubCharacter *);
    float cos_x;
    float cos_z;

    assert(sh2jms.lower_now >= 0 && sh2jms.lower_now < 32);
    assert(sh2jms.upper_now >= 0 && sh2jms.upper_now < 32);
    lower_func = func_list_lower[sh2jms.lower_now];
    lower_func(this);
    upper_func = func_list_upper[sh2jms.upper_now];
    upper_func(this);
    if (PlayerWaterRoadIsOn()) {
        this->spd_org *= 0.65f;
    }
    switch (sh2jms.upper_now) {
    case JMS_ST_U_DAMAGE:
    case JMS_ST_U_FALL:
        this->pos_spd.x = this->spd_org * dtf * shSinF(PlayerAngleWrap(this->rot.y + this->spd_roty));
        this->pos_spd.z = this->spd_org * dtf * shCosF(PlayerAngleWrap(this->rot.y + this->spd_roty));
        break;
    default:
        if (sh2jms.lock_on) {
            this->pos_spd.x = this->spd_org * dtf * shSinF(PlayerAngleWrap(this->spd_roty));
            this->pos_spd.z = this->spd_org * dtf * shCosF(PlayerAngleWrap(this->spd_roty));
        } else {
            PlayerChangeAngleToCameraY(this);
            this->pos_spd.x = this->spd_org * dtf * shSinF(PlayerAngleWrap(this->rot.y));
            this->pos_spd.z = this->spd_org * dtf * shCosF(PlayerAngleWrap(this->rot.y));
        }
        break;
    }
    this->pos_spd.y = this->spd_y;
    cos_x = shCosF(sh2jms.r_foot.hobj.wall.nl[0]);
    cos_z = shCosF(sh2jms.r_foot.hobj.wall.nl[2]);
    this->pos_spd.x = cos_x * (this->pos_spd.x * cos_x);
    this->pos_spd.z = cos_z * (this->pos_spd.z * cos_z);
    sh2jms.pos.x = this->pos_spd.x;
    sh2jms.pos.y = this->pos_spd.y;
    sh2jms.pos.z = this->pos_spd.z;
    sh2jms.pos.x = 500.0f * sh2jms.pos.x;
    sh2jms.pos.y = 500.0f * sh2jms.pos.y;
    sh2jms.pos.z = 500.0f * sh2jms.pos.z;
}

static void PlayerCheckAttack2D(struct SubCharacter *this) {
    PlayerCheckAttack(this);
}

/** Updates James's lower- and upper-body states from the input (2D control) and checks attacks. */
void PlayerCheckControl2D(struct SubCharacter *this) {
    PlayerUpdateStatus2D(this);
    PlayerUpdateStatusLower2D(this);
    PlayerUpdateStatusUpper2D(this);
    PlayerCheckAttack2D(this);
    PlayerUpdateStatusLower2nd2D(this);
}
