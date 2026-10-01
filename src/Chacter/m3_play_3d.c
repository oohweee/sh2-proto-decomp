/*
 * James's movement in 3D control mode (the pad's 3D settings, as opposed to m3_play_2d): the
 * lower- and upper-body state machines (stand, walk, run, turn, step, jump, attack, ...), selected per
 * frame from the stick input.
 */
#include "sh2.h"
#include "m3_helpers.h"
#include "asm_libm.h"

static int dt;
static float dtf;

/* Wraps an angle into [-PI, PI]. Matching: a macro, so the argument is evaluated again in each arm. */
#define ANGLE_WRAP(a) (((a) > 3.1415927f) ? (a) - 6.2831855f : (((a) < -3.1415927f) ? 6.2831855f + (a) : (a)))

/* Matching: no prototype; called with an argument it ignores (the DWARF shows none). */
static void PlayerUpdateStatusStand3D();

/*
 * Analog turn steps: dtf * (spd * |lstick_x|), times round_way. Matching: inline functions in the original
 * (no DWARF): ANGLE_WRAP() re-evaluates them in each arm, and the int->float conversion of the
 * turn direction comes after the product, which the expression written out in place doesn't give.
 */
static inline float PlayerStickRotSpd(float spd) {
    return dtf * (spd * fabsf(sh2jms.lstick_x));
}

static inline float PlayerRotSpdByStick(float spd) {
    return sh2jms.pad[0].pad3d.round_way * PlayerStickRotSpd(spd);
}


/*
 * Matching: stand-in for a function the Metrowerks linker dead-stripped (config/stripped_functions.txt).
 * Name unknown; the body is a guess: a copy of m3_play_2d's PlayerChangeAngleToCameraY, which nothing calls.
 * Its double-precision math is what matters: once MWCC has compiled a software-double operation,
 * later functions in the file avoid a0/a1 for temporaries, which is what upper_walk_3d & co. show.
 */
static void __stripped_m3_play_3d_code(struct SubCharacter *p) {
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

static int PlayerCheckLturn180(void) {
    struct PAD_INFO *pad;
    struct PAD_3D *p3d;

    p3d = &sh2jms.pad[0].pad3d;
    /* Matching: pad is in the original's DWARF but never read (a dead store). */
    pad = &sh2jms.pad[0];
    if (p3d->lslide) {
        if (p3d->lturn180 > 0 && p3d->rturn180 == 2) {
            return 1;
        }
    } else if (p3d->lturn180 == 2 && p3d->rturn180 == 2) {
        return 2;
    }
    return 0;
}

static int PlayerCheckRturn180(void) {
    struct PAD_INFO *pad;
    struct PAD_3D *p3d;

    p3d = &sh2jms.pad[0].pad3d;
    /* Matching: pad is in the original's DWARF but never read (a dead store). */
    pad = &sh2jms.pad[0];
    if (p3d->rslide) {
        if (p3d->rturn180 > 0 && p3d->lturn180 == 2) {
            return 1;
        }
    } else if (p3d->rturn180 == 2 && p3d->lturn180 == 2) {
        return 2;
    }
    return 0;
}

static int PlayerCheckTurn180(void) {
    struct PAD_3D *p3d;
    int l;
    int r;

    p3d = &sh2jms.pad[0].pad3d;
    l = PlayerCheckLturn180();
    r = PlayerCheckRturn180();
    if (!l && !r) {
        return 0;
    }
    switch (p3d->round_way) {
    case -1:
        return -1;
    case 1:
        return 1;
    }
    if (l >= r) {
        return -1;
    }
    return 1;
}

static void lower_lround_3d_nata(struct SubCharacter *p, float *spd) {
    switch (sh2jms.ctrl_unit) {
    case 1:
        *spd = sh2jms.lstick_x;
        break;
    case 0:
        *spd = -1.0f;
        break;
    }
    shCharacterAnimeSpeedAdd_(p, 2, -0x180);
    shCharacterAnimeSpeedAdd_(p, 1, -0x180);
}

static void lower_rround_3d_nata(struct SubCharacter *p, float *spd) {
    switch (sh2jms.ctrl_unit) {
    case 1:
        *spd = sh2jms.lstick_x;
        break;
    case 0:
        *spd = 1.0f;
        break;
    }
    shCharacterAnimeSpeedAdd_(p, 2, -0x180);
    shCharacterAnimeSpeedAdd_(p, 1, -0x180);
}

static void lower_walk_3d_nata(struct SubCharacter *p) {
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

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 316
    assert_dw(frame >= 0 && frame <= 23); /* Matching: do/while(0) form (its nop) */
    shCharacterAnimeSpeedAdd_(p, 2, motion_spd_tbl[frame]);
    shCharacterAnimeSpeedAdd_(p, 1, motion_spd_tbl[frame]);
}

static void lower_back_3d_nata(struct SubCharacter *p) {
    short frame;
    float move_spd_tbl[24] = {
        0.9f, 1.05f, 1.2f, 0.0f, 0.3f, 0.9f, 1.05f, 1.2f, 1.35f, 1.425f, 1.35f, 1.2f,
        0.975f, 0.75f, 0.525f, 0.075f, 0.0f, 0.3f, 0.45f, 0.675f, 1.35f, 1.2f, 1.35f, 1.05f,
    };
    short motion_spd_tbl[24] = {
        -0x140, -0x140, -0x240, -0x2C0, -0x240, -0x140, -0x140, -0x140, -0x140, -0x140, -0x140, -0x140,
        -0x140, -0x1F2, -0x240, -0x2C0, -0x1C0, -0x140, -0x140, -0x140, -0x140, -0x140, -0x140, -0x140,
    };

    frame = shCharacterAnimeFrameGet_(p, 2);
    p->spd = move_spd_tbl[frame];

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 423
    assert_dw(frame >= 0 && frame <= 23); /* Matching: do/while(0) form (its nop) */
    shCharacterAnimeSpeedAdd_(p, 2, motion_spd_tbl[frame]);
    shCharacterAnimeSpeedAdd_(p, 1, motion_spd_tbl[frame]);
}

static void lower_lswalk_3d_nata(struct SubCharacter *p) {
    short frame;
    float move_spd_tbl[24] = {
        0.9f, 1.05f, 0.45f, 0.15f, 0.0f, 0.9f, 1.05f, 1.2f, 1.425f, 1.5f, 1.425f, 1.35f,
        0.975f, 0.75f, 0.525f, 0.075f, 0.0f, 0.0f, 0.225f, 0.675f, 0.9f, 1.2f, 1.35f, 1.05f,
    };
    short motion_spd_tbl[24] = {
        -0x180, -0x180, -0x240, -0x2C0, -0x240, -0x180, -0x180, -0x180, -0x180, -0x180, -0x180, -0x180,
        -0x180, -0x1F2, -0x240, -0x2C0, -0x1C0, -0x180, -0x180, -0x180, -0x180, -0x180, -0x180, -0x180,
    };

    frame = shCharacterAnimeFrameGet_(p, 2);
    p->spd = move_spd_tbl[frame];

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 530
    assert_dw(frame >= 0 && frame <= 23); /* Matching: do/while(0) form (its nop) */
    shCharacterAnimeSpeedAdd_(p, 2, motion_spd_tbl[frame]);
    shCharacterAnimeSpeedAdd_(p, 1, motion_spd_tbl[frame]);
}

static void lower_rswalk_3d_nata(struct SubCharacter *p) {
    short frame;
    float move_spd_tbl[24] = {
        0.9f, 1.05f, 0.45f, 0.15f, 0.0f, 0.9f, 1.05f, 1.2f, 1.425f, 1.5f, 1.425f, 1.35f,
        0.975f, 0.75f, 0.525f, 0.075f, 0.0f, 0.0f, 0.225f, 0.675f, 0.9f, 1.2f, 1.35f, 1.05f,
    };
    short motion_spd_tbl[24] = {
        -0x180, -0x180, -0x240, -0x2C0, -0x240, -0x180, -0x180, -0x180, -0x180, -0x180, -0x180, -0x180,
        -0x180, -0x1C0, -0x240, -0x2C0, -0x1C0, -0x180, -0x180, -0x180, -0x180, -0x180, -0x180, -0x180,
    };

    frame = shCharacterAnimeFrameGet_(p, 2);
    p->spd = move_spd_tbl[frame];

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 665
    assert_dw(frame >= 0 && frame <= 23); /* Matching: do/while(0) form (its nop) */
    shCharacterAnimeSpeedAdd_(p, 2, motion_spd_tbl[frame]);
    shCharacterAnimeSpeedAdd_(p, 1, motion_spd_tbl[frame]);
}

static void lower_stand_3d(struct SubCharacter *p) {
    lower_stand(p);
}

static void upper_stand_3d(struct SubCharacter *p) {
    upper_stand(p);
}

static void lower_relax_3d(struct SubCharacter *p) {
    lower_relax(p);
}

static void upper_relax_3d(struct SubCharacter *p) {
    upper_relax(p);
}

static void lower_alert_3d(struct SubCharacter *p) {
    lower_alert(p);
}

static void upper_alert_3d(struct SubCharacter *p) {
    upper_alert(p);
}

static void lower_tired_3d(struct SubCharacter *p) {
    lower_tired(p);
}

static void upper_tired_3d(struct SubCharacter *p) {
    upper_tired(p);
}

static void lower_ready_3d(struct SubCharacter *p) {
    lower_ready(p);
}

static void upper_ready_3d(struct SubCharacter *p) {
    upper_ready(p);
}

static void lower_readyoff_3d(struct SubCharacter *p) {
    lower_readyoff(p);
}

static void upper_readyoff_3d(struct SubCharacter *p) {
    upper_readyoff(p);
}

static void lower_lround_3d(struct SubCharacter *p) {
    float spd;

    if (sh2jms.weapon == 8) {
        lower_lround_3d_nata(p, &spd);
    } else {
        switch (sh2jms.ctrl_unit) {
        case 1:
            spd = 2.4f * sh2jms.lstick_x;
            break;
        case 0:
            spd = -2.4f;
            break;
        }
        if (sh2jms.pad[0].dash) {
            spd *= 1.5f;
        }
    }
    PlayerSpeedDownToStand(p);
    p->rot.y = ANGLE_WRAP(p->rot.y + spd * dtf);
}

static void upper_lround_3d(void) {
}

static void lower_rround_3d(struct SubCharacter *p) {
    float spd;

    if (sh2jms.weapon == 8) {
        lower_rround_3d_nata(p, &spd);
    } else {
        switch (sh2jms.ctrl_unit) {
        case 1:
            spd = 2.4f * sh2jms.lstick_x;
            break;
        case 0:
            spd = 2.4f;
            break;
        }
        if (sh2jms.pad[0].dash) {
            spd *= 1.5f;
        }
    }
    PlayerSpeedDownToStand(p);
    p->rot.y = ANGLE_WRAP(p->rot.y + spd * dtf);
}

static void upper_rround_3d(void) {
}

/* NON_MATCHING: our compiler build sign-extends the `short` ana_spd (the DWARF's type) again before the int
 * add, a compiler-build difference; 0x200 also loads in another order; linked from the original
 * (docs/matching-notes.md#m3_play_3d-lower_walk_3d). */
static void lower_walk_3d(struct SubCharacter *p) {
    static float lstickY_tmp = 0.0f;
    short ana_spd = 0;
    float rot_tmp;

    if (sh2jms.upper_now != JMS_ST_U_ATTACK) {
        lstickY_tmp = sh2jms.lstick_y;
    }
    if (sh2jms.weapon == 8) {
        lower_walk_3d_nata(p);
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
            p->spd = 1.5f * fabsf(lstickY_tmp);
            shCharacterAnimeSpeedAdd_(p, 2, ana_spd = 896.0f * -(1.0f - fabsf(lstickY_tmp)));
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
            shCharacterAnimeSpeedAdd_(p, 2, ana_spd + (int)(512.0f * (1.0f - fabsf(lstickY_tmp))));
            break;
        }
        p->spd_org = (p->spd_org > 2.25f) ? 2.25f : p->spd_org;
    } else {
        p->spd_org = p->spd;
    }
    if (p->spd_roty != 0.0f) {
        switch (sh2jms.lower_prev) {
        case JMS_ST_L_LSWALK:
        case JMS_ST_L_RSWALK:
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_RUN3:
        case JMS_ST_L_LSRUN:
        case JMS_ST_L_RSRUN:
            close_to_value(&p->spd_roty, 0.0f, 2.0f * dtf);
            break;
        default:
            p->spd_roty = 0.0f;
            break;
        }
    }
    if (!sh2jms.lock_on) {
        if (sh2jms.weapon == 8) {
            if (p->spd > 0.5f) {
                rot_tmp = sh2jms.pad[0].pad3d.round_way * (0.875f * dtf);
            } else {
                rot_tmp = 0.0f;
            }
        } else {
            switch (sh2jms.ctrl_unit) {
            case 0:
                rot_tmp = sh2jms.pad[0].pad3d.round_way * (1.9f * dtf);
                break;
            case 1:
                rot_tmp = PlayerRotSpdByStick(1.9f);
                break;
            }
        }
        if (sh2jms.map_mode == 2) {
            rot_tmp *= 1.3f;
        }
        p->rot.y = ANGLE_WRAP(p->rot.y + rot_tmp);
    }
    PlayerSetAttackWithWalkIsOk();
}

static void upper_walk_3d(struct SubCharacter *p) {
    short ana_spd;

    if (sh2jms.weapon != 8) {
        switch (sh2jms.ctrl_unit) {
        case 1:
            shCharacterAnimeSpeedAdd_(p, 1, ana_spd = 896.0f * -(1.0f - fabsf(sh2jms.lstick_y)));
            break;
        }
    }
}

/* NON_MATCHING: our compiler build sign-extends the `short` ana_spd (the DWARF's type) again before the int
 * add, a compiler-build difference; linked from the original
 * (docs/matching-notes.md#m3_play_3d-lower_back_3d). */
static void lower_back_3d(struct SubCharacter *p) {
    static float lstickY_tmp = 0.0f;
    short ana_spd;
    float rot_tmp;
    int turn_way;

    p->spd_roty = -3.1415927f;
    if (sh2jms.upper_now != JMS_ST_U_ATTACK) {
        lstickY_tmp = sh2jms.lstick_y;
    }
    if (sh2jms.weapon == 8) {
        lower_back_3d_nata(p);
    } else {
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
            p->spd = 1.2f * fabsf(sh2jms.lstick_y);
            shCharacterAnimeSpeedAdd_(p, 2, ana_spd = 1024.0f * -(1.0f - fabsf(sh2jms.lstick_y)));
            break;
        }
    }
    p->spd_org = p->spd;
    if (sh2jms.pad[0].dash) {
        switch (sh2jms.ctrl_unit) {
        case 0:
            shCharacterAnimeSpeedAdd_(p, 2, 0x200);
            break;
        case 1:
            shCharacterAnimeSpeedAdd_(p, 2, ana_spd + (int)(512.0f * fabsf(sh2jms.lstick_y)));
            break;
        }
        p->spd_org = 1.5f * p->spd;
        p->spd_org = (p->spd_org > 1.2f * 1.5f) ? 1.2f * 1.5f : p->spd_org;
    }
    if (!sh2jms.lock_on) {
        if (!playing.retreat_turn) {
            turn_way = sh2jms.pad[0].pad3d.round_way;
        } else {
            turn_way = -sh2jms.pad[0].pad3d.round_way;
        }
        if (sh2jms.weapon == 8) {
            if (p->spd > 0.5f) {
                rot_tmp = turn_way * (0.65f * dtf);
            } else {
                rot_tmp = 0.0f;
            }
        } else {
            switch (sh2jms.ctrl_unit) {
            case 0:
                rot_tmp = turn_way * (1.3f * dtf);
                break;
            case 1:
                rot_tmp = turn_way * PlayerStickRotSpd(1.3f);
                break;
            }
        }
        if (sh2jms.map_mode == 2) {
            rot_tmp *= 1.3f;
        }
        p->rot.y = ANGLE_WRAP(p->rot.y + rot_tmp);
    }
    PlayerSetAttackWithWalkIsOk();
}

/* NON_MATCHING: our compiler build sign-extends the `short` ana_spd (the DWARF's type) again before the int
 * add, a compiler-build difference; linked from the original
 * (docs/matching-notes.md#m3_play_3d-upper_back_3d). */
static void upper_back_3d(struct SubCharacter *p) {
    short ana_spd;

    p->spd_roty = -3.1415927f;
    if (sh2jms.weapon != 8) {
        switch (sh2jms.ctrl_unit) {
        case 0:
            if (sh2jms.pad[0].dash) {
                shCharacterAnimeSpeedAdd_(p, 1, 0x200);
            }
            break;
        case 1:
            shCharacterAnimeSpeedAdd_(p, 1, ana_spd = 1024.0f * -(1.0f - fabsf(sh2jms.lstick_y)));
            shCharacterAnimeSpeedAdd_(p, 1, ana_spd + (int)(sh2jms.pad[0].dash * (512.0f * fabsf(sh2jms.lstick_y))));
            break;
        }
    }
}

static void lower_lswalk_3d(struct SubCharacter *p) {
    if (sh2jms.weapon == 8) {
        lower_lswalk_3d_nata(p);
    } else if (p->spd > 0.0f && sh2jms.lower_prev == JMS_ST_L_BACK) {
        p->spd -= 3.4f * 1.5f * dtf;
        p->spd = (p->spd > 0.0f) ? p->spd : 0.0f;
    } else if (p->spd > 1.3f) {
        p->spd -= 3.4f * dtf;
        p->spd = (p->spd > 1.3f) ? p->spd : 1.3f;
    } else {
        p->spd += 1.7f * dtf;
        p->spd = (p->spd > 1.3f) ? 1.3f : p->spd;
    }
    if (sh2jms.lock_on * sh2jms.pad[0].dash) {
        p->spd_org = 1.5f * p->spd;
        shCharacterAnimeSpeedAdd_(p, 2, 0x200);
        p->spd_org = (p->spd_org > 1.2f * 1.5f) ? 1.2f * 1.5f : p->spd_org;
    } else {
        p->spd_org = p->spd;
    }
    if (p->spd_roty != -1.5707964f) {
        switch (sh2jms.lower_prev) {
        case JMS_ST_L_WALK:
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_RUN3:
            p->spd_roty -= 2.0f * dtf;
            if (p->spd_roty < -1.5707964f) {
                p->spd_roty = -1.5707964f;
            }
            break;
        default:
            p->spd_roty = -1.5707964f;
            break;
        }
    }
    if (!sh2jms.lock_on) {
        if (sh2jms.weapon == 8) {
            if (p->spd > 0.5f) {
                p->rot.y = ANGLE_WRAP(p->rot.y + sh2jms.pad[0].pad3d.round_way * (0.875f * dtf));
            }
        } else {
            switch (sh2jms.ctrl_unit) {
            case 0:
                p->rot.y = ANGLE_WRAP(p->rot.y + sh2jms.pad[0].pad3d.round_way * (1.9f * dtf));
                break;
            case 1:
                p->rot.y = ANGLE_WRAP(p->rot.y + PlayerRotSpdByStick(1.9f));
                break;
            }
        }
    }
    PlayerSetAttackWithWalkIsOk();
}

static void upper_lswalk_3d(void) {
}

static void lower_rswalk_3d(struct SubCharacter *p) {
    if (sh2jms.weapon == 8) {
        lower_rswalk_3d_nata(p);
    } else if (p->spd > 0.0f && sh2jms.lower_prev == JMS_ST_L_BACK) {
        p->spd -= 3.4f * 1.5f * dtf;
        p->spd = (p->spd > 0.0f) ? p->spd : 0.0f;
    } else if (p->spd > 1.3f) {
        p->spd -= 3.4f * dtf;
        p->spd = (p->spd > 1.3f) ? p->spd : 1.3f;
    } else {
        p->spd += 1.7f * dtf;
        p->spd = (p->spd > 1.3f) ? 1.3f : p->spd;
    }
    if (sh2jms.lock_on && sh2jms.pad[0].dash) {
        p->spd_org = 1.5f * p->spd;
        shCharacterAnimeSpeedAdd_(p, 2, 0x200);
        p->spd_org = (p->spd_org > 1.2f * 1.5f) ? 1.2f * 1.5f : p->spd_org;
    } else {
        p->spd_org = p->spd;
    }
    if (p->spd_roty != 1.5707964f) {
        switch (sh2jms.lower_prev) {
        case JMS_ST_L_WALK:
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_RUN3:
            p->spd_roty += 2.0f * dtf;
            if (p->spd_roty > 1.5707964f) {
                p->spd_roty = 1.5707964f;
            }
            break;
        default:
            p->spd_roty = 1.5707964f;
            break;
        }
    }
    if (!sh2jms.lock_on) {
        if (sh2jms.weapon == 8) {
            if (p->spd > 0.5f) {
                p->rot.y = ANGLE_WRAP(p->rot.y + sh2jms.pad[0].pad3d.round_way * (0.875f * dtf));
            }
        } else {
            switch (sh2jms.ctrl_unit) {
            case 0:
                p->rot.y = ANGLE_WRAP(p->rot.y + sh2jms.pad[0].pad3d.round_way * (1.9f * dtf));
                break;
            case 1:
                p->rot.y = ANGLE_WRAP(p->rot.y + PlayerRotSpdByStick(1.9f));
                break;
            }
        }
    }
    PlayerSetAttackWithWalkIsOk();
}

static void upper_rswalk_3d(void) {
}

static void lower_run1_3d(struct SubCharacter *p) {
    static float lstickY_tmp = 0.0f;
    float target_speed;

    lstickY_tmp = sh2jms.lstick_y;
    switch (sh2jms.ctrl_unit) {
    case 0:
        if (p->spd > 3.5f) {
            p->spd -= 3.0f * dtf;
            p->spd = (p->spd > 3.5f) ? p->spd : 3.5f;
        } else {
            p->spd += 3.0f * dtf;
            p->spd = (p->spd > 3.5f) ? 3.5f : p->spd;
        }
        if (!sh2jms.lock_on) {
            p->rot.y = ANGLE_WRAP(p->rot.y + sh2jms.pad[0].pad3d.round_way * (1.7f * dtf));
        }
        break;
    case 1:
        target_speed = 3.5f * fabsf(lstickY_tmp);
        if (p->spd < target_speed) {
            p->spd += 3.0f * dtf;
        } else {
            p->spd -= 3.0f * dtf;
        }
        p->spd = (p->spd > 2.0f) ? p->spd : 2.0f;
        p->spd = (p->spd > target_speed) ? target_speed : p->spd;
        if (!sh2jms.lock_on) {
            p->rot.y = ANGLE_WRAP(p->rot.y + PlayerRotSpdByStick(1.7f));
        }
        break;
    }
    p->spd_org = p->spd;
    if (p->spd_roty != 0.0f) {
        switch (sh2jms.lower_prev) {
        case JMS_ST_L_WALK:
        case JMS_ST_L_LSWALK:
        case JMS_ST_L_RSWALK:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_RUN3:
        case JMS_ST_L_LSRUN:
        case JMS_ST_L_RSRUN:
            close_to_value(&p->spd_roty, 0.0f, 4.0f * dtf);
            break;
        default:
            p->spd_roty = 0.0f;
            break;
        }
    }
    if (p->spd >= 3.5f && !sh2jms.l_anime_st_flg) {
        player_flg_on(&sh2jms.lower_st_flg, 0x2000);
        player_flg_off(&sh2jms.lower_st_flg, 0x1000);
    }
    PlayerSetAttackWithRunIsOk();
}

static void upper_run1_3d(struct SubCharacter *p) {
    upper_run1(p);
}

static void lower_run2_3d(struct SubCharacter *p) {
    static float lstickY_tmp = 0.0f;
    float target_speed;

    lstickY_tmp = sh2jms.lstick_y;
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
        if (!sh2jms.lock_on) {
            p->rot.y = ANGLE_WRAP(p->rot.y + sh2jms.pad[0].pad3d.round_way * (1.7f * dtf));
        }
        if (p->spd == 4.0f && !sh2jms.l_anime_st_flg && sh2jms.tired < sh2jms.tired_max && !sh2jms.map_mode) {
            player_flg_on(&sh2jms.lower_st_flg, 0x4000);
            player_flg_off(&sh2jms.lower_st_flg, 0x2000);
        }
        break;
    case 1:
        target_speed = 4.0f * fabsf(sh2jms.lstick_y);
        if (p->spd < target_speed) {
            p->spd += 3.0f * dtf;
            p->spd = (p->spd > target_speed) ? target_speed : p->spd;
        } else {
            p->spd -= 3.0f * dtf;
            p->spd = (p->spd > target_speed) ? p->spd : target_speed;
        }
        if (!sh2jms.lock_on) {
            p->rot.y = ANGLE_WRAP(p->rot.y + PlayerRotSpdByStick(1.7f));
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
    if (p->spd_roty != 0.0f) {
        switch (sh2jms.lower_prev) {
        case JMS_ST_L_WALK:
        case JMS_ST_L_LSWALK:
        case JMS_ST_L_RSWALK:
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN3:
        case JMS_ST_L_LSRUN:
        case JMS_ST_L_RSRUN:
            close_to_value(&p->spd_roty, 0.0f, dtf * 4.0f);
            break;
        default:
            p->spd_roty = 0.0f;
            break;
        }
    }
    PlayerSetAttackWithRunIsOk();
    if (!sh2jms.cannot_run) {
        player_flg_on(&sh2jms.lower_st_flg, 0x20000);
    }
}

static void upper_run2_3d(struct SubCharacter *p) {
    upper_run2(p);
}

static void lower_run3_3d(struct SubCharacter *p) {
    static float lstickY_tmp = 0.0f;
    float target_speed;

    lstickY_tmp = sh2jms.lstick_y;
    if (ItemAmpolueEfficacy() == 0.0f) {
        sh2jms.tired += dt;
    }
    switch (sh2jms.ctrl_unit) {
    case 0:
        p->spd += 3.0f * dtf;
        p->spd = (p->spd > 4.8f) ? 4.8f : p->spd;
        if (!sh2jms.lock_on) {
            p->rot.y = ANGLE_WRAP(p->rot.y + sh2jms.pad[0].pad3d.round_way * (1.7f * dtf));
        }
        if (sh2jms.tired >= sh2jms.tired_max && !sh2jms.l_anime_st_flg) {
            player_flg_on(&sh2jms.lower_st_flg, 0x2000);
            player_flg_off(&sh2jms.lower_st_flg, 0x4000);
        }
        break;
    case 1:
        target_speed = 4.8f * fabsf(sh2jms.lstick_y);
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
        if (!sh2jms.lock_on) {
            p->rot.y = ANGLE_WRAP(p->rot.y + PlayerRotSpdByStick(1.7f));
        }
        if ((p->spd <= 4.0f || sh2jms.tired >= sh2jms.tired_max) && !sh2jms.l_anime_st_flg) {
            player_flg_on(&sh2jms.lower_st_flg, 0x2000);
            player_flg_off(&sh2jms.lower_st_flg, 0x4000);
        }
        break;
    }
    p->spd_org = p->spd;
    if (p->spd_roty != 0.0f) {
        switch (sh2jms.lower_prev) {
        case JMS_ST_L_WALK:
        case JMS_ST_L_LSWALK:
        case JMS_ST_L_RSWALK:
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_LSRUN:
        case JMS_ST_L_RSRUN:
            close_to_value(&p->spd_roty, 0.0f, 4.0f * dtf);
            break;
        default:
            p->spd_roty = 0.0f;
            break;
        }
    }
    PlayerSetAttackWithRunIsOk();
    if (!sh2jms.cannot_run) {
        player_flg_on(&sh2jms.lower_st_flg, 0x20000);
    }
}

static void upper_run3_3d(struct SubCharacter *p) {
    upper_run3(p);
}

static void lower_lsrun_3d(struct SubCharacter *p) {
    if (ItemAmpolueEfficacy() == 0.0f) {
        sh2jms.tired += dt;
    }
    p->spd += 6.0f * dtf;
    p->spd = (p->spd > 3.8f) ? 3.8f : p->spd;
    p->spd_org = p->spd;
    if (p->spd_roty != -1.5707964f) {
        switch (sh2jms.lower_prev) {
        case JMS_ST_L_WALK:
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_RUN3:
            p->spd_roty -= 2.0f * dtf;
            if (p->spd_roty < -1.5707964f) {
                p->spd_roty = -1.5707964f;
            }
            break;
        default:
            p->spd_roty = -1.5707964f;
            break;
        }
    }
    if (!sh2jms.lock_on) {
        switch (sh2jms.ctrl_unit) {
        case 0:
            p->rot.y = ANGLE_WRAP(p->rot.y + sh2jms.pad[0].pad3d.round_way * (1.7f * dtf));
            break;
        case 1:
            p->rot.y = ANGLE_WRAP(p->rot.y + PlayerRotSpdByStick(1.7f));
            break;
        }
    }
    PlayerSetAttackWithRunIsOk();
    if (!sh2jms.cannot_run) {
        player_flg_on(&sh2jms.lower_st_flg, 0x20000);
    }
}

static void upper_lsrun_3d(void) {
}

static void lower_rsrun_3d(struct SubCharacter *p) {
    if (ItemAmpolueEfficacy() == 0.0f) {
        sh2jms.tired += dt;
    }
    p->spd += 6.0f * dtf;
    p->spd = (p->spd > 3.8f) ? 3.8f : p->spd;
    p->spd_org = p->spd;
    if (p->spd_roty != 1.5707964f) {
        switch (sh2jms.lower_prev) {
        case JMS_ST_L_WALK:
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_RUN3:
            p->spd_roty += 2.0f * dtf;
            if (p->spd_roty > 1.5707964f) {
                p->spd_roty = 1.5707964f;
            }
            break;
        default:
            p->spd_roty = 1.5707964f;
            break;
        }
    }
    if (!sh2jms.lock_on) {
        switch (sh2jms.ctrl_unit) {
        case 0:
            p->rot.y = ANGLE_WRAP(p->rot.y + sh2jms.pad[0].pad3d.round_way * (1.7f * dtf));
            break;
        case 1:
            p->rot.y = ANGLE_WRAP(p->rot.y + PlayerRotSpdByStick(1.7f));
            break;
        }
    }
    PlayerSetAttackWithRunIsOk();
    if (!sh2jms.cannot_run) {
        player_flg_on(&sh2jms.lower_st_flg, 0x20000);
    }
}

static void upper_rsrun_3d(void) {
}

static void lower_lturn180_3d(struct SubCharacter *p) {
    float mov_angle;

    if (sh2jms.weapon == 8) {
        mov_angle = 3.1415927f * 1.5f * dtf;
        shCharacterAnimeSpeedAdd_(p, 2, -0x400);
        shCharacterAnimeSpeedAdd_(p, 1, -0x400);
    } else {
        mov_angle = 3.1415927f * 1.75f * dtf;
    }
    p->spd = 0.0f;
    p->spd_org = 0.0f;
    sh2jms.dist_rot.w += mov_angle;
    p->rot.y -= mov_angle;
    if (sh2jms.dist_rot.w >= 3.1415927f) {
        PlayerUpdateStatusStand3D(p);
        p->rot.y = sh2jms.dist_rot.y;
    }
    p->rot.y = ANGLE_WRAP(p->rot.y);
}

static void upper_lturn180_3d(void) {
}

static void lower_rturn180_3d(struct SubCharacter *p) {
    float mov_angle;

    if (sh2jms.weapon == 8) {
        mov_angle = 3.1415927f * 1.5f * dtf;
        shCharacterAnimeSpeedAdd_(p, 2, -0x400);
        shCharacterAnimeSpeedAdd_(p, 1, -0x400);
    } else {
        mov_angle = 3.1415927f * 1.75f * dtf;
    }
    p->spd = 0.0f;
    p->spd_org = 0.0f;
    sh2jms.dist_rot.w += mov_angle;
    p->rot.y += mov_angle;
    if (sh2jms.dist_rot.w >= 3.1415927f) {
        PlayerUpdateStatusStand3D(p);
        p->rot.y = sh2jms.dist_rot.y;
    }
    p->rot.y = ANGLE_WRAP(p->rot.y);
}

static void upper_rturn180_3d(void) {
}

static void lower_jump_3d(void) {
}

static void upper_jump_3d(void) {
}

static void lower_guard_3d(struct SubCharacter *p) {
    unsigned short frame;

    frame = shCharacterAnimeFrameGet_(p, 2);
    if (sh2jms.lower_prev != JMS_ST_L_GUARD) {
        sh2jms.lower_prev = JMS_ST_L_GUARD;
        p->spd_y = 0.0f;
        p->spd = 0.0f;
        p->spd_org = 0.0f;
        p->spd_roty = -3.1415927f;
    } else {
        if (frame <= 9) {
            p->spd_org = p->spd = sh2jms.shock * dtf;
        } else {
            p->spd = 0.0f;
            p->spd_org = 0.0f;
        }
        if (sh2jms.anime_pause & 1) {
            sh2jms.no_damage = 0;
            player_flg_on(&sh2jms.lower_st_flg, 1);
            player_flg_off(&sh2jms.lower_st_flg, 0x200000);
            player_flg_off(&sh2jms.l_anime_st_flg, 1);
        }
    }
}

static void upper_guard_3d(void) {
    if (sh2jms.anime_pause & 2) {
        player_flg_on(&sh2jms.upper_st_flg, 1);
        player_flg_off(&sh2jms.upper_st_flg, 0x200000);
        player_flg_off(&sh2jms.u_anime_st_flg, 1);
    }
}

static void lower_lstep_3d(void) {
}

static void upper_lstep_3d(void) {
}

static void lower_rstep_3d(void) {
}

static void upper_rstep_3d(void) {
}

static void lower_hold_3d(struct SubCharacter *p) {
    float spd;

    lower_hold(p);
    if (!sh2jms.lock_on) {
        switch (sh2jms.ctrl_unit) {
        case 0:
            if (sh2jms.pad[0].dash) {
                spd = 2.4f * (1.5f * sh2jms.pad[0].pad3d.round_way);
            } else {
                spd = 2.4f * sh2jms.pad[0].pad3d.round_way;
            }
            break;
        case 1:
            if (sh2jms.pad[0].dash) {
                spd = (1.5f * sh2jms.pad[0].pad3d.round_way) * (2.4f * fabsf(sh2jms.lstick_x));
            } else {
                spd = sh2jms.pad[0].pad3d.round_way * (2.4f * fabsf(sh2jms.lstick_x));
            }
            break;
        }
        p->rot.y = ANGLE_WRAP(p->rot.y + spd * dtf);
    }
}

static void upper_hold_3d(struct SubCharacter *p) {
    upper_hold(p);
}

static void lower_release_3d(struct SubCharacter *p) {
    float spd;

    lower_release(p);
    switch (sh2jms.ctrl_unit) {
    case 0:
        if (sh2jms.pad[0].dash) {
            spd = 2.4f * (1.5f * sh2jms.pad[0].pad3d.round_way);
        } else {
            spd = 2.4f * sh2jms.pad[0].pad3d.round_way;
        }
        break;
    case 1:
        if (sh2jms.pad[0].dash) {
            spd = (1.5f * sh2jms.pad[0].pad3d.round_way) * (2.4f * fabsf(sh2jms.lstick_x));
        } else {
            spd = sh2jms.pad[0].pad3d.round_way * (2.4f * fabsf(sh2jms.lstick_x));
        }
        break;
    }
    p->rot.y = ANGLE_WRAP(p->rot.y + spd * dtf);
}

static void upper_release_3d(struct SubCharacter *p) {
    upper_release(p);
}

static void lower_attack_3d(struct SubCharacter *p) {
    lower_attack(p);
}

static void upper_attack_3d(struct SubCharacter *p) {
    upper_attack(p);
}

static void lower_kick_3d(struct SubCharacter *p) {
    lower_kick(p);
}

static void upper_kick_3d(struct SubCharacter *p) {
    upper_kick(p);
}

static void lower_fall_3d(struct SubCharacter *p) {
    lower_fall(p);
}

static void upper_fall_3d(struct SubCharacter *p) {
    upper_fall(p);
}

static void lower_damage_3d(struct SubCharacter *p) {
    lower_damage(p);
}

static void upper_damage_3d(struct SubCharacter *p) {
    upper_damage(p);
}

static void lower_to_stand_3d(struct SubCharacter *p) {
    lower_to_stand(p);
}

static void upper_to_stand_3d(struct SubCharacter *p) {
    upper_to_stand(p);
}

static void lower_wall_f_3d(struct SubCharacter *p) {
    unsigned short frame;

    frame = shCharacterAnimeFrameGet_(p, 2);
    if (frame == 4 || frame > 9) {
        p->spd = 0.0f;
    } else if (frame > 4 && frame <= 9) {
        switch (sh2jms.lower_prev) {
        case JMS_ST_L_RUN1:
        case JMS_ST_L_RUN2:
        case JMS_ST_L_RUN3:
            p->spd_roty = -3.1415927f;
            break;
        case JMS_ST_L_RSRUN:
            p->spd_roty = -1.5707964f;
            break;
        case JMS_ST_L_LSRUN:
            p->spd_roty = 1.5707964f;
            break;
        }
        p->spd += 5.0f * dtf;
        p->spd = (p->spd > 1.2f) ? 1.2f : p->spd;
    }
    p->spd_org = p->spd;
    if (sh2jms.anime_pause & 1) {
        player_flg_on(&sh2jms.lower_st_flg, 1);
    }
}

static void upper_wall_f_3d(struct SubCharacter *p) {
    upper_wall_f(p);
}

static void lower_event_3d(struct SubCharacter *p) {
    lower_event(p);
}

static void upper_event_3d(struct SubCharacter *p) {
    upper_event(p);
}

static void PlayerUpdateStatus3D(struct SubCharacter *this) {
    dt = shGetDF();
    dtf = shGetDT();
    PlayerSetDT();
    PlayerUpdateStatus(this);
}

static void PlayerUpdateStatusStand3D() { /* Matching: K&R definition; see its declaration above. */
    struct shPlayerWork *w;

    w = &sh2jms;
    lower_st_set(JMS_ST_L_STAND, w);
    lower_flg_set(JMS_ST_L_STAND, w);
    upper_st_set(JMS_ST_U_STAND, w);
    upper_flg_set(JMS_ST_U_STAND, w);
    player_flg_on(&w->lower_st_flg, 1);
    player_flg_on(&w->upper_st_flg, 1);
}

/* Matching: the turn-180 targets use the ANGLE_WRAP macro (the inline loads rot.y before PI). */
static void PlayerUpdateStatusLower3D(struct SubCharacter *this) {
    struct shPlayerWork *w;
    struct PAD_INFO *p;
    struct PAD_3D *p3d;
    struct SubCharacterDisp *scp_d;

    w = &sh2jms;
    p = &sh2jms.pad[0];
    p3d = &sh2jms.pad[0].pad3d;
    /* Matching: scp_d is in the original's DWARF but never read (a dead store of the usual cast). */
    scp_d = (struct SubCharacterDisp *)this;
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
                if (actwithwep_flg_on(0x40) && (!u_anime_flg_on(0x40) || sh2jms.strike_splash_flg)) {
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
                if (!sh2jms.map_mode && actwithwep_flg_on(0x80) && (!u_anime_flg_on(0x40) || sh2jms.strike_splash_flg)) {
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
                if (!l_anime_flg_on(0x40) || sh2jms.strike_splash_flg) {
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
    switch (PlayerCheckTurn180()) {
    case -1:
        if (lower_flg_on(0x40000) && w->hold_type == -1) {
            if (w->lower_now != JMS_ST_L_LTURN) {
                lower_st_set(JMS_ST_L_LTURN, w);
                lower_flg_set(JMS_ST_L_LTURN, w);
                player_flg_on(&w->l_anime_st_flg, 0x40);
            }
            w->dist_rot.y = ANGLE_WRAP(this->rot.y - 3.1415927f);
            w->dist_rot.w = 0.0f;
            return;
        }
        break;
    case 1:
        if (lower_flg_on(0x80000) && w->hold_type == -1) {
            if (w->lower_now != JMS_ST_L_RTURN) {
                lower_st_set(JMS_ST_L_RTURN, w);
                lower_flg_set(JMS_ST_L_RTURN, w);
                player_flg_on(&w->l_anime_st_flg, 0x40);
            }
            w->dist_rot.y = ANGLE_WRAP(this->rot.y - 3.1415927f);
            w->dist_rot.w = 0.0f;
            return;
        }
        break;
    }
    if (p->forward || p->lstickY < 0) {
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
    if ((p->backward || p->lstickY > 0) && lower_flg_on(0x100) &&
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
    if (p3d->lslide && (!p3d->rslide || w->lower_now == JMS_ST_L_LSRUN)) {
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
    if (p3d->rslide && (!p3d->lslide || w->lower_now == JMS_ST_L_RSRUN)) {
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
    if ((p->lround || p->lstickX < 0) && lower_flg_on(0x40)) {
        if (w->lower_now != JMS_ST_L_LROUND) {
            lower_st_set(JMS_ST_L_LROUND, w);
            lower_flg_set(JMS_ST_L_LROUND, w);
        }
        return;
    }
    if ((p->rround || p->lstickX > 0) && lower_flg_on(0x80)) {
        if (w->lower_now != JMS_ST_L_RROUND) {
            lower_st_set(JMS_ST_L_RROUND, w);
            lower_flg_set(JMS_ST_L_RROUND, w);
        }
        return;
    }
    if (!w->l_anime_st_flg && shPadPress(0, key_config.dash) && sh2jms.weapon != 8 && lower_flg_on(0x10)) {
        if (w->lower_now != JMS_ST_L_READY) {
            lower_st_set(JMS_ST_L_READY, w);
            lower_flg_set(JMS_ST_L_READY, w);
        }
        return;
    }
    if (lower_flg_on(0x20)) {
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

static void PlayerUpdateStatusUpper3D(struct SubCharacter *this) {
    struct shPlayerWork *w;
    struct PAD_INFO *p;
    struct PAD_INFO *p_pre;
    struct PAD_3D *p3d;

    w = &sh2jms;
    p = &sh2jms.pad[0];
    p_pre = &sh2jms.pad[1];
    /* Matching: p3d is in the original's DWARF but never read (a dead store). */
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
                    if ((!p_pre->lstickX && p->lstickX > 0) || shPadTrigger(0, key_config.right_turn)) {
                        PlayerChangeTarget(1);
                    }
                    if ((!p_pre->lstickX && p->lstickX < 0) || shPadTrigger(0, key_config.left_turn)) {
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
    case JMS_ST_L_LROUND:
        if (upper_flg_on(0x40) && w->upper_now != JMS_ST_U_LROUND) {
            upper_st_set(JMS_ST_U_LROUND, w);
            upper_flg_set(JMS_ST_U_LROUND, w);
        }
        break;
    case JMS_ST_L_RROUND:
        if (upper_flg_on(0x80) && w->upper_now != JMS_ST_U_RROUND) {
            upper_st_set(JMS_ST_U_RROUND, w);
            upper_flg_set(JMS_ST_U_RROUND, w);
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
    case JMS_ST_L_LSWALK:
        if (upper_flg_on(0x400) && w->upper_now != JMS_ST_U_LSWALK) {
            upper_st_set(JMS_ST_U_LSWALK, w);
            upper_flg_set(JMS_ST_U_LSWALK, w);
            if (sh2jms.act_with_wep & 1) {
                player_flg_on(&w->upper_st_flg, 0x4000000);
            }
        }
        break;
    case JMS_ST_L_RSWALK:
        if (upper_flg_on(0x800) && w->upper_now != JMS_ST_U_RSWALK) {
            upper_st_set(JMS_ST_U_RSWALK, w);
            upper_flg_set(JMS_ST_U_RSWALK, w);
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
    case JMS_ST_L_LSRUN:
        if (upper_flg_on(0x8000) && w->upper_now != JMS_ST_U_LSRUN) {
            upper_st_set(JMS_ST_U_LSRUN, w);
            upper_flg_set(JMS_ST_U_LSRUN, w);
            if (sh2jms.act_with_wep & 2) {
                player_flg_on(&w->upper_st_flg, 0x4000000);
            }
        }
        break;
    case JMS_ST_L_RSRUN:
        if (upper_flg_on(0x10000) && w->upper_now != JMS_ST_U_RSRUN) {
            upper_st_set(JMS_ST_U_RSRUN, w);
            upper_flg_set(JMS_ST_U_RSRUN, w);
            if (sh2jms.act_with_wep & 2) {
                player_flg_on(&w->upper_st_flg, 0x4000000);
            }
        }
        break;
    case JMS_ST_L_LTURN:
        if (w->upper_now != JMS_ST_U_LTURN) {
            upper_st_set(JMS_ST_U_LTURN, w);
            upper_flg_set(JMS_ST_U_LTURN, w);
            player_flg_on(&w->u_anime_st_flg, 0x40);
        }
        break;
    case JMS_ST_L_RTURN:
        if (w->upper_now != JMS_ST_U_RTURN) {
            upper_st_set(JMS_ST_U_RTURN, w);
            upper_flg_set(JMS_ST_U_RTURN, w);
            player_flg_on(&w->u_anime_st_flg, 0x40);
        }
        break;
    case JMS_ST_L_JUMP:
        if (w->upper_now != JMS_ST_U_JUMP) {
            upper_st_set(JMS_ST_U_JUMP, w);
            upper_flg_set(JMS_ST_U_JUMP, w);
        }
        break;
    case JMS_ST_L_ATTACK:
    case JMS_ST_L_KICK:
    case JMS_ST_L_FALL:
    case JMS_ST_L_TO_STAND:
        break;
    }
}

static void (*func_list_lower[32])(struct SubCharacter *) = {
    lower_stand_3d,    lower_relax_3d,    lower_alert_3d,    lower_tired_3d,    lower_ready_3d,
    lower_readyoff_3d, lower_lround_3d,   lower_rround_3d,   lower_back_3d,     lower_walk_3d,
    lower_lswalk_3d,   lower_rswalk_3d,   lower_run1_3d,     lower_run2_3d,     lower_run3_3d,
    lower_lsrun_3d,    lower_rsrun_3d,    lower_wall_f_3d,   lower_lturn180_3d, lower_rturn180_3d,
    (void (*)(struct SubCharacter *))lower_jump_3d, lower_guard_3d,
    (void (*)(struct SubCharacter *))lower_lstep_3d, (void (*)(struct SubCharacter *))lower_rstep_3d,
    lower_fall_3d,     lower_damage_3d,   lower_hold_3d,     lower_release_3d,  lower_attack_3d,
    lower_kick_3d,     lower_to_stand_3d, lower_event_3d,
};

static void (*func_list_upper[32])(struct SubCharacter *) = {
    upper_stand_3d,    upper_relax_3d,    upper_alert_3d,    upper_tired_3d,    upper_ready_3d,
    upper_readyoff_3d, (void (*)(struct SubCharacter *))upper_lround_3d,
    (void (*)(struct SubCharacter *))upper_rround_3d, upper_back_3d, upper_walk_3d,
    (void (*)(struct SubCharacter *))upper_lswalk_3d, (void (*)(struct SubCharacter *))upper_rswalk_3d,
    upper_run1_3d,     upper_run2_3d,     upper_run3_3d,
    (void (*)(struct SubCharacter *))upper_lsrun_3d, (void (*)(struct SubCharacter *))upper_rsrun_3d,
    upper_wall_f_3d,
    (void (*)(struct SubCharacter *))upper_lturn180_3d, (void (*)(struct SubCharacter *))upper_rturn180_3d,
    (void (*)(struct SubCharacter *))upper_jump_3d, (void (*)(struct SubCharacter *))upper_guard_3d,
    (void (*)(struct SubCharacter *))upper_lstep_3d, (void (*)(struct SubCharacter *))upper_rstep_3d,
    upper_fall_3d,     upper_damage_3d,   upper_hold_3d,     upper_release_3d,  upper_attack_3d,
    upper_kick_3d,     upper_to_stand_3d, upper_event_3d,
};

/** Runs James's lower- and upper-body state functions and computes his movement for this frame (3D control). */
void PlayerUpdatePosition3D(struct SubCharacter *this) {
    void (*lower_func)(struct SubCharacter *);
    void (*upper_func)(struct SubCharacter *);
    float cos_x;
    float cos_z;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 3579
    assert(sh2jms.lower_now >= 0 && sh2jms.lower_now < 32);
    assert(sh2jms.upper_now >= 0 && sh2jms.upper_now < 32);
    lower_func = func_list_lower[sh2jms.lower_now];
    lower_func(this);
    upper_func = func_list_upper[sh2jms.upper_now];
    upper_func(this);
    if (PlayerWaterRoadIsOn()) {
        this->spd_org *= 0.65f;
    }
    this->pos_spd.x = this->spd_org * dtf * shSinF(PlayerAngleWrap(this->rot.y + this->spd_roty));
    this->pos_spd.z = this->spd_org * dtf * shCosF(PlayerAngleWrap(this->rot.y + this->spd_roty));
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

/* Matching: K&R definition; called with arguments it ignores (the DWARF shows none). */
static void PlayerUpdateStatusLower2nd3D() {
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

static void PlayerCheckAttack3D(struct SubCharacter *this) {
    PlayerCheckAttack(this);
}

/** Updates James's lower- and upper-body states from the input (3D control) and checks attacks. */
void PlayerCheckControl3D(struct SubCharacter *this) {
    PlayerUpdateStatus3D(this);
    PlayerUpdateStatusLower3D(this);
    PlayerUpdateStatusUpper3D(this);
    PlayerCheckAttack3D(this);
    PlayerUpdateStatusLower2nd3D(this);
}
