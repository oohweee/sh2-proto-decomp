/*
 * Player (James) control from events and scripts: event-mode queries, forcing event
 * animations, walking/running James to a target point, his drama animations, and the
 * current weapon.
 */

#include "sh2.h"

/* Matching: no prototype; it sits in func_list_event and is called with a target it ignores
 * (the DWARF shows no parameters). */
static void event_jms_stand();
static void event_jms_walk(float *target);
static void event_jms_run(float *target);

void (*func_list_event[3])(float *) = { event_jms_stand, event_jms_walk, event_jms_run };

int pjames_anime_adr_list[30] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0x11CD0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

static const struct _AnimeInfo pjames_demo_anim[30] = {
    { 0, 0, 0, 0, 0, 0 },
    { 0x3B7, 0xB, 0x100, 1, 0xA, 1 },
    { 0x3B8, 0x191, 0x800, 0, 0x190, 0 },
    { 0x3B9, 0xF0, 0x800, 0, 0xEF, 0 },
    { 0x3BA, 0x4A, 0x800, 0, 0x49, 0 },
    { 0x3BB, 0x4A, 0x800, 0, 0x49, 0 },
    { 0x3BC, 0xEC, 0x800, 0, 0xEB, 0 },
    { 0x3BD, 0x12C, 0x800, 0, 0x12B, 0 },
    { 0x3BE, 0x137, 0x800, 0, 0x136, 0 },
    { 0x3BF, 0x214, 0x800, 0, 0x213, 0 },
    { 0x3C0, 0x105, 0x800, 0, 0x104, 0 },
    { 0x3C1, 0xDE, 0x800, 0, 0xDD, 0 },
    { 0x3C2, 0xFA, 0x800, 0, 0xF9, 0 },
    { 0x3C3, 0x13C, 0x800, 0, 0x13B, 0 },
    { 0x3C4, 0x3E, 0x800, 0, 0x3D, 0 },
    { 0x3C5, 0x65, 0x800, 0, 0x64, 0 },
    { 0x3C6, 0x107, 0x800, 0, 0x106, 0 },
    { 0x3C7, 0xEA, 0x800, 0, 0xE9, 0 },
    { 0x3C8, 0x87, 0x800, 0, 0x86, 0 },
    { 0x3C9, 0x87, 0x800, 0, 0x86, 0 },
    { 0x3CA, 0x9C, 0x800, 0, 0x9B, 0 },
    { 0x3CB, 0x398, 0x800, 0, 0x397, 0 },
    { 0x3CC, 0x120, 0x800, 0, 0x11F, 0 },
    { 0x3CD, 0x14F, 0x800, 0, 0x14E, 0 },
    { 0x3CE, 0x137, 0x800, 0, 0x136, 0 },
    { 0x3CF, 0x534, 0x800, 0, 0x533, 0 },
    { 0x3D0, 0x1C0, 0x800, 0, 0x1BF, 0 },
    { 0x3D1, 0x1EA, 0x800, 0, 0x1E9, 0 },
    { 0x3D2, 0xBF, 0x800, 0, 0xBF, 0 },
    { 0x3D3, 0xBA, 0x800, 0, 0xB9, 0 },
};

/* XZ distance from a to p's position on the FPU (inline asm in the original). Local: reads the
 * SubCharacter's pos by offset (0x20/0x28). */
inline float PlayerEventDistXZ(float *a, struct SubCharacter *p) {
    float d;

    asm {
        lwc1 d, 0(a)
        lwc1 $f8, 0x20(p)
        lwc1 $f9, 8(a)
        lwc1 $f10, 0x28(p)
        sub.s d, d, $f8
        sub.s $f9, $f9, $f10
        mula.s d, d
        madd.s d, $f9, $f9
        sqrt.s d, d
    }
    return d;
}

/** Returns 1 if the player is in demo (event) mode (status 0x2000), else 0. */
int PlayerNowDemoEventMode(void) {
    return (sh2jms.player->status & 0x2000) ? 1 : 0;
}

/** Returns 1 if event button @p button (0 action, 1 menu, 2 light, 3 map) is pressed while James's
 * upper body is in a state below 0x11. */
int PlayerEventButtonCheck(int button) {
    int pad;

    switch (button) {
    case 0:
        pad = sh2jms.pad[0].action;
        break;
    case 1:
        pad = sh2jms.pad[0].menu;
        break;
    case 2:
        pad = sh2jms.pad[0].light;
        break;
    case 3:
        pad = sh2jms.pad[0].map;
        break;
    default:
        return 0;
    }
    if (pad && sh2jms.upper_now < 0x11) {
        return 1;
    }
    return 0;
}

/** Returns whether James's death animation has finished (dead == 2). */
int PlayerEventDeadAnimeFinish(void) {
    return sh2jms.dead == 2;
}

/** Returns 1 if James exists and is dead or dying. */
int PlayerEventJamesDeadly(void) {
    if (sh2jms.player && sh2jms.dead) {
        return 1;
    }
    return 0;
}

/** Returns 1 if Maria exists and is dead or dying. */
int PlayerEventMariaDeadly(void) {
    if (sh2mar.mar_p && sh2mar.dead) {
        return 1;
    }
    return 0;
}

/** Returns 1 once James's lower-body animation has reached its info's `pad` frame (if set). */
int PlayerEventAnimeSuccessFrame(void) {
    struct _AnimeInfo *a_info;
    short frame;

    a_info = shCharacterAnimeGetInfo_(sh2jms.player, 1);
    frame = shCharacterAnimeFrameGet_(sh2jms.player, 1);
    if (a_info->pad != 0 && !(frame < a_info->pad)) {
        return 1;
    }
    return 0;
}

/** Makes James play event animation @p anime. */
void PlayerEventAnimeSet(int anime) {
    player_flg_on(&sh2jms.lower_st_flg, 0x80000000);
    player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
    player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
    sh2jms.event_anime = anime;
}

/** Makes James play event animation @p anime, flagged direct (bit 31). */
void PlayerEventAnimeSetDirect(int anime) {
    player_flg_on(&sh2jms.lower_st_flg, 0x80000000);
    player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
    player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
    sh2jms.event_anime = anime;
    sh2jms.event_anime |= 0x80000000;
}

static void event_jms_stand() { /* Matching: K&R definition; see its declaration above. */
    PlayerSpeedDownToStand(sh2jms.player);
}

static void event_jms_walk(float *target) {
    struct SubCharacter *p;
    float to_target;

    p = sh2jms.player;
    if (p->spd > 1.5f) {
        p->spd -= 5.0f * shGetDT();
        p->spd = (p->spd > 1.5f) ? p->spd : 1.5f;
    } else {
        p->spd += 2.5f * shGetDT();
        p->spd = (p->spd > 1.5f) ? 1.5f : p->spd;
    }
    p->spd_org = p->spd;
    to_target = shAtan2(target[2] - p->pos.z, target[0] - p->pos.x);
    close_to_angle_target(&p->rot.y, to_target, -3.1415927f, 3.1415927f, 12.0f);
    switch (playing.control_type) {
    case 0:
        p->spd_roty = 0.0f;
        break;
    case 1:
        p->spd_roty = to_target;
        break;
    }
}

/* Matching: a const variable makes MWCC build the 8.0f argument before -PI/PI. Unverifiable: it is
 * propagated and never emitted, and MWCC leaves no DWARF for such a static const. */
static const float run_spd = 8.0f;

static void event_jms_run(float *target) {
    struct SubCharacter *p;
    float to_target;

    p = sh2jms.player;
    p->spd += 3.0f * shGetDT();
    p->spd = (p->spd > 4.0f) ? 4.0f : p->spd;
    p->spd_org = p->spd;
    to_target = shAtan2(target[2] - p->pos.z, target[0] - p->pos.x);
    close_to_angle_target(&p->rot.y, to_target, -3.1415927f, 3.1415927f, run_spd);
    switch (playing.control_type) {
    case 0:
        p->spd_roty = 0.0f;
        break;
    case 1:
        p->spd_roty = to_target;
        break;
    }
}

/**
 * Moves James towards @p target for one frame, standing, walking or running by distance.
 * @return the XZ distance to the target.
 */
float PlayerEventMove(float *target) {
    void (*event_jms_func)(float *);
    static float distance;

    sh2jms.event_move_mode = 1;
    distance = PlayerEventDistXZ(target, sh2jms.player);
    if (sh2jms.event_status_now) {
        if (distance > 3500.0f) {
            if (sh2jms.event_status_now != 2) {
                sh2jms.event_status_prev = sh2jms.event_status_now;
                sh2jms.event_status_now = 2;
            }
        } else if (distance > 0.7f * 125.0) { /* Matching: a double compare, as in the original */
            if (sh2jms.event_status_now != 1) {
                sh2jms.event_status_prev = sh2jms.event_status_now;
                sh2jms.event_status_now = 1;
            }
        } else {
            if (sh2jms.event_status_now != 0) {
                sh2jms.event_status_prev = sh2jms.event_status_now;
                sh2jms.event_status_now = 0;
            }
        }
    }
    if (sh2jms.event_status_prev != sh2jms.event_status_now) {
        player_flg_on(&sh2jms.u_anime_st_flg, 0x40);
        player_flg_on(&sh2jms.l_anime_st_flg, 0x40);
        sh2jms.event_status_prev = sh2jms.event_status_now;
    }
    event_jms_func = func_list_event[sh2jms.event_status_now];
    event_jms_func(target);
    return distance;
}

/** Returns 1 (and ends the move) once James has stopped at the target or the move was cancelled. */
int PlayerEventMoveIsEnd(void) {
    if ((!sh2jms.event_status_now && !l_anime_flg_on(2) && !l_anime_flg_on(0x40)) ||
        !sh2jms.event_move_mode) {
        PlayerEventMoveCancel();
        return 1;
    }
    return 0;
}

/** Ends an event move. @return 1. */
int PlayerEventMoveCancel(void) {
    sh2jms.event_move_mode = 0;
    sh2jms.event_status_prev = sh2jms.event_status_now = 0xFF;
    sh2jms.tired = 0;
    return 1;
}

/**
 * Plays drama animation @p anime_id on a player James model (0x100/0x101).
 * @return 0, or -1 if @p scp isn't a player James model.
 */
int shCharacterHumanPJAMESAnimeSet(struct SubCharacter *scp, int anime_id) {
    short id;
    struct _AnimeInfo *aip;

    id = shCharacterGetModelID(scp);
    if (id == 0x100 || id == 0x101) {
        aip = (struct _AnimeInfo *)&pjames_demo_anim[anime_id - 0x3B6];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            pjames_anime_adr_list[anime_id - 0x3B6] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x3B6));
        return 0;
    }
    return -1;
}

/** Gives James weapon @p wep and sets the matching motion set. */
void JamesWeaponSet(int wep) {
    sh2jms.weapon = wep;
    switch (wep) {
    case 0:
    case 1:
    case 4:
        sh2jms.motion_no = 0;
        break;
    case 2:
    case 3:
        sh2jms.motion_no = 1;
        break;
    case 5:
    case 6:
        sh2jms.motion_no = 2;
        break;
    case 8:
        sh2jms.motion_no = 3;
        break;
    case 7:
        sh2jms.motion_no = 4;
        break;
    }
    sh2jms.hold_type = -1;
    actwithwep_flg_set(0, &sh2jms);
    PlayerCheckInit(sh2jms.player);
}

/** Returns James's current weapon. */
int PlayerGetJamesWeapon(void) {
    return sh2jms.weapon;
}
