/*
 * What characters perceive: each character's list of nearby targets (sorted by distance) seen,
 * felt or lit within its look/feel areas, target selection for James and the enemies, and
 * queries about James's and Maria's state used by the enemy AI.
 */

#include "sh2.h"
#include "asm_helpers.h"

void *memcpy(); /* Matching: called without a prototype in the original (arguments passed unconverted). */

struct shInAreaTgtInfo sh2_target_info[20];
struct shInAreaTgtInfo sh2_target_info_buf[20];
static int rest_tgt;
static int rest_tgt_buf;

/*
 * Matching: sh_vu0.h's _shLength, declared here and defined after its first caller: sh_vu0.h is
 * included after shBattleCheckTargetMyArea. MWCC then calls an out-of-line copy placed there
 * instead of inlining it, which is what the original has. Including sh_vu0.h at the top would
 * define it first and inline it.
 */
static inline float _shLength(float *v0, float *v1);

static void shBattleCheckHitEyes(struct _CL_VHIT_RESULT *eye, struct SubCharacter *scp, int i, int net) {
    float sp[4];
    float ep[4];

    sp[0] = scp->pos.x;
    sp[1] = scp->eye_y;
    sp[2] = scp->pos.z;
    if (sh2_target_info[i].adr.scp->kind >> 8 == 7) {
        ep[0] = sh2_target_info[i].adr.scp->pos_spd.x;
        ep[1] = sh2_target_info[i].adr.scp->center_y;
        ep[2] = sh2_target_info[i].adr.scp->pos_spd.z;
    } else {
        ep[0] = sh2_target_info[i].adr.scp->pos.x;
        ep[1] = sh2_target_info[i].adr.scp->center_y;
        ep[2] = sh2_target_info[i].adr.scp->pos.z;
    }
    clCheckHitEyes(eye, (unsigned int)scp, sp, ep, net);
}

/**
 * Tests whether @p tgt is inside @p scp's look and feel areas, and lit and near enough to be
 * seen by its light. @param in_area out: the three flags. @param look, feel the areas' centres.
 */
void shBattleCheckTargetMyArea(struct shInArea *in_area, struct SubCharacter *scp, struct SubCharacter *tgt, float *look, float *feel) {
    float tgt_pos[4];
    float tgt_to_look;
    float tgt_to_feel;

    if (tgt->kind >> 8 == 7) {
        tgt_pos[0] = tgt->pos_spd.x;
        tgt_pos[1] = tgt->center_y;
        tgt_pos[2] = tgt->pos_spd.z;
    } else {
        tgt_pos[0] = tgt->pos.x;
        tgt_pos[1] = tgt->center_y;
        tgt_pos[2] = tgt->pos.z;
    }
    tgt_to_look = _shLength(look, tgt_pos);
    tgt_to_feel = _shLength(feel, tgt_pos);
    in_area->light_on = (sh2gfw_Check_CharaDarkOrBright(tgt) && tgt_to_feel <= 1.5f * scp->battle.look.radius) ? 1 : 0;
    in_area->look_on = (tgt_to_look <= scp->battle.look.radius) ? 1 : 0;
    in_area->feel_on = (tgt_to_feel <= scp->battle.feel.radius) ? 1 : 0;
    switch (tgt->kind >> 8) {
    case 7:
        if (!(scp->status & 0x200)) {
            in_area->look_on = in_area->feel_on = 0;
        }
        if (tgt_pos[1] < scp->eye_y - 600.0f || tgt_pos[1] > scp->eye_y + 1000.0f) {
            in_area->light_on = in_area->look_on = in_area->feel_on = 0;
        }
        break;
    case 2:
        if (tgt->battle.dead_timer > 1.5f) {
            in_area->light_on = in_area->look_on = in_area->feel_on = 0;
        }
        break;
    case 1:
        in_area->light_on = in_area->feel_on = 0;
        break;
    }
}

/* Matching: the deferred definition of _shLength (see the declaration above). */
#include "sh_vu0.h"

/** Returns 1 if any active, living enemy exists. */
int shBattleAroundTargetEnemy(void) {
    struct SubCharacter *tgt;

    for (tgt = sh2chara.head; tgt != NULL; tgt = tgt->next) {
        if (tgt->kind >> 8 == 2 && (tgt->battle.status & 0x400) && !(tgt->battle.status & 2)) {
            return 1;
        }
    }
    return 0;
}

/**
 * Rebuilds @p scp's target list: every human, enemy or item (kinds 1xx, 2xx, 7xx) in its look,
 * feel or light area, sorted by distance (James's list is also kept for the camera).
 * @return 1 if the list isn't empty.
 */
int shBattleCheckTargetChara(struct SubCharacter *scp) {
    int i;
    int j;
    struct SubCharacter *tgt;
    struct shInArea in_area;
    float look_center[4];
    float feel_center[4];
    struct shInAreaTgtInfo dummy;

    look_center[0] = scp->pos.x + scp->battle.look.center * shSinF(scp->rot.y);
    look_center[1] = scp->eye_y;
    look_center[2] = scp->pos.z + scp->battle.look.center * shCosF(scp->rot.y);
    feel_center[0] = scp->pos.x + scp->battle.feel.center * shSinF(scp->rot.y);
    feel_center[1] = scp->center_y;
    feel_center[2] = scp->pos.z + scp->battle.feel.center * shCosF(scp->rot.y);
    if (scp->kind == 0x105) {
        shQzero(sh2_target_info, sizeof(sh2_target_info));
        rest_tgt = 20;
    }
    for (tgt = sh2chara.head; tgt != NULL; tgt = tgt->next) {
        if (tgt == scp) {
            continue;
        }
        switch (tgt->kind >> 8) {
        case 1:
        case 2:
        case 7:
            break;
        default:
            continue;
        }
        if (tgt->kind >> 8 == 2 && !(tgt->battle.status & 0x400)) {
            continue;
        }
        shBattleCheckTargetMyArea(&in_area, scp, tgt, look_center, feel_center);
        if (in_area.look_on || in_area.feel_on || in_area.light_on) {
            if (rest_tgt != 0) {
                sh2_target_info[20 - rest_tgt].in_area = in_area;
                sh2_target_info[20 - rest_tgt].adr.scp = tgt;
                sh2_target_info[20 - rest_tgt].distance = _shLength((float *)&scp->pos, (float *)&tgt->pos);
                rest_tgt--;
            }
        }
    }
    for (i = 0; i < 19 - rest_tgt; i++) {
        for (j = 1; j < 20 - rest_tgt; j++) {
            if (sh2_target_info[i].distance > sh2_target_info[j].distance) {
                memcpy(&dummy, &sh2_target_info[i], sizeof(struct shInAreaTgtInfo));
                memcpy(&sh2_target_info[i], &sh2_target_info[j], sizeof(struct shInAreaTgtInfo));
                memcpy(&sh2_target_info[j], &dummy, sizeof(struct shInAreaTgtInfo));
            }
        }
    }
    if (scp == sh2jms.player) {
        memcpy(sh2_target_info_buf, sh2_target_info, sizeof(sh2_target_info));
        rest_tgt_buf = rest_tgt;
    }
    if (rest_tgt == 20) {
        return 0;
    }
    return 1;
}

/** Returns the nearest living enemy @p scp can see (preferring ones without status 4), or NULL. */
struct SubCharacter *shBattleGetTargetEnemy(struct SubCharacter *scp) {
    int i;
    struct SubCharacter *kari_target;
    struct _CL_VHIT_RESULT eye;

    kari_target = NULL;
    if (rest_tgt == 20) {
        return NULL;
    }
    for (i = 0; i < 20 - rest_tgt; i++) {
        if (sh2_target_info[i].adr.scp->kind >> 8 == 2 && !(sh2_target_info[i].adr.scp->battle.status & 2)) {
            shBattleCheckHitEyes(&eye, scp, i, 1);
            if (eye.kind == 3 && eye.hobj.chara.sc == sh2_target_info[i].adr.scp) {
                if (sh2_target_info[i].adr.scp->battle.status & 4) {
                    if (kari_target == NULL) {
                        kari_target = sh2_target_info[i].adr.scp;
                    }
                } else {
                    return sh2_target_info[i].adr.scp;
                }
            }
        }
    }
    return kari_target;
}

/**
 * Returns another visible living enemy on the given side of @p scp, or NULL.
 * @param key 1 for a target at a positive angle from @p scp's facing, -1 for a negative one.
 */
struct SubCharacter *shBattleChangeTargetEnemy(struct SubCharacter *scp, int key) {
    int i;
    float to_target;
    float rot_tmp;
    struct _CL_VHIT_RESULT eye;

    if (rest_tgt == 20) {
        return NULL;
    }
    for (i = 0; i < 20 - rest_tgt; i++) {
        if (sh2jms.target != sh2_target_info[i].adr.scp) {
            if (sh2_target_info[i].adr.scp->kind >> 8 == 2) {
                if (!(sh2_target_info[i].adr.scp->battle.status & 2)) {
                    to_target = shAtan2(sh2_target_info[i].adr.scp->pos.z - scp->pos.z,
                                        sh2_target_info[i].adr.scp->pos.x - scp->pos.x);
                    rot_tmp = shAngleRegulate(to_target - scp->rot.y);
                    if ((key == 1 && rot_tmp > 0.0f) || (key == -1 && rot_tmp < 0.0f)) {
                        shBattleCheckHitEyes(&eye, scp, i, 1);
                        if (eye.kind == 3) {
                            return sh2_target_info[i].adr.scp;
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

/** For @p kind 0, returns the nearest listed target @p scp's sight ray reaches (as an address), else 0. */
unsigned int shBattleGetTargetChara(struct SubCharacter *scp, int kind) {
    int i;
    struct _CL_VHIT_RESULT eye;

    switch (kind) {
    case 0:
        if (rest_tgt == 20) {
            return 0;
        }
        for (i = 0; i < 20 - rest_tgt; i++) {
            shBattleCheckHitEyes(&eye, scp, i, 1);
            if (eye.kind != 1 && eye.kind != 2) {
                return (unsigned int)sh2_target_info[i].adr.scp;
            }
        }
        break;
    case 1:
        return 0;
    }
    return 0;
}

/** Returns entry @p i of James's target list if it is an enemy (@p type 0) or an item (@p type 1), else NULL. */
struct SubCharacter *shCameraGetNearTarget(int i, int type) {
    int kind;

    if (rest_tgt_buf == 20) {
        return NULL;
    }
    if (sh2_target_info_buf[i].adr.scp != NULL) {
        kind = sh2_target_info_buf[i].adr.scp->kind >> 8;
        if ((type == 0 && kind == 2) || (type == 1 && kind == 7)) {
            return sh2_target_info_buf[i].adr.scp;
        }
    }
    return NULL;
}

/** Returns a visible living enemy with status 4 within 600 units of @p scp, or NULL. */
struct SubCharacter *shBattleGetNearDeadlyTargetEnemy(struct SubCharacter *scp) {
    int i;
    struct _CL_VHIT_RESULT eye;

    if (rest_tgt == 20) {
        return NULL;
    }
    for (i = 0; i < 20 - rest_tgt; i++) {
        if (sh2_target_info[i].adr.scp->kind >> 8 == 2 && !(sh2_target_info[i].adr.scp->battle.status & 2) &&
            (sh2_target_info[i].adr.scp->battle.status & 4)) {
            shBattleCheckHitEyes(&eye, scp, i, 0);
            if (eye.kind == 3 && eye.hobj.chara.sc == sh2_target_info[i].adr.scp && sh2_target_info[i].distance <= 600.0f) {
                return sh2_target_info[i].adr.scp;
            }
        }
    }
    return NULL;
}

/**
 * Returns the human an enemy should target. @param type 1 James, 2 Maria, otherwise the
 * nearer of the two (in XZ).
 */
struct SubCharacter *shBattleGetTargetHuman(struct SubCharacter *scp, unsigned int type) {
    float pos1[4];
    float pos2[4];
    float scalar1;
    float scalar2;
    struct SubCharacter *p;
    struct SubCharacter *p1;
    struct SubCharacter *p2;

    switch (type) {
    case 1:
        p = shCharacterGetSubCharacter(0x100, -1);
        if (p == NULL) {
            p = shCharacterGetSubCharacter(0x101, -1);
        }
        break;
    case 2:
        return shCharacterGetSubCharacter(0x105, -1);
    default:
        p1 = shCharacterGetSubCharacter(0x100, -1);
        if (p1 == NULL) {
            p1 = shCharacterGetSubCharacter(0x101, -1);
        }
        p2 = shCharacterGetSubCharacter(0x105, -1);
        if (p1 != NULL) {
            if (p2 != NULL) {
                pos1[0] = p1->pos.x - scp->pos.x;
                pos1[2] = p1->pos.z - scp->pos.z;
                pos2[0] = p2->pos.x - scp->pos.x;
                pos2[2] = p2->pos.z - scp->pos.z;
                scalar1 = lengthXZ(pos1);
                scalar2 = lengthXZ(pos2);
                return (scalar1 <= scalar2) ? p1 : p2;
            }
            return p1;
        }
        if (p2 != NULL) {
            return p2;
        }
        return NULL;
    }
    return p;
}

/** Returns whether @p tgt is making a sound enemies hear (battle status 0x200). @param scp unused. */
int shBattleListenHumanSound(struct SubCharacter *scp, struct SubCharacter *tgt) {
    return (tgt->battle.status & 0x200) != 0;
}

/** Returns 1 if @p scp is inside James's flashlight beam, 0 if not, -1 if the light is off. */
int shBattleSeeHumanLight(struct SubCharacter *scp) {
    struct SPOT_LIGHT spot;
    float light_center[4];
    float scp_center[4];
    float angle;
    float light_radius;
    float dist;

    if (!item.light_switch) {
        return -1;
    }
    kari_sh2gde_getspotParams(spot.c, spot.zdir, spot.range);
    angle = shAtan2(spot.zdir[2], spot.zdir[0]);
    light_center[0] = spot.c[0] + 0.5f * spot.range[3] * spot.zdir[0];
    light_center[1] = spot.c[1] + 0.5f * spot.range[3] * spot.zdir[1];
    light_center[2] = spot.c[2] + 0.5f * spot.range[3] * spot.zdir[2];
    light_radius = 0.5f * spot.range[3] * shSinF(spot.range[1]);
    scp_center[0] = scp->pos.x;
    scp_center[2] = scp->pos.z;
    scp_center[1] = scp->center_y;
    dist = _shLength(light_center, scp_center);
    if (dist < light_radius) {
        return 1;
    }
    return 0;
}

/** Returns 1 if James has locked on to @p scp. */
int shBattleAimedByHuman(struct SubCharacter *scp) {
    if (sh2jms.target == scp && sh2jms.lock_on) {
        return 1;
    }
    return 0;
}

/** Returns 1 if James is finishing off @p scp while it lies down (lower-body state 0x1D). */
int shBattleFinishedByHuman(struct SubCharacter *scp) {
    if (sh2jms.enemy_liedown == scp && sh2jms.lower_now == 0x1D) {
        return 1;
    }
    return 0;
}

/** Returns whether James can't take damage now. */
int shBattleNoDamageHuman(void) {
    return shBattleNoDamageHumanJames();
}

/** Returns 1 if James is invulnerable (no_damage or muteki_time). */
int shBattleNoDamageHumanJames(void) {
    if (sh2jms.no_damage || sh2jms.muteki_time) {
        return 1;
    }
    return 0;
}

/** Returns 1 if Maria is invulnerable (no_damage or muteki_time). */
int shBattleNoDamageHumanMaria(void) {
    if (sh2mar.no_damage || sh2mar.muteki_time) {
        return 1;
    }
    return 0;
}

/** Returns 1 if James is being held (hugging_gauge non-zero). */
int shBattleHuggedHuman(void) {
    return !!sh2jms.hugging_gauge;
}

/** Sets @p scp's look area: centre offset ahead and radius, in units of 500. */
void shBattleSetLookArea(struct SubCharacter *scp, float center, float radius) {
    scp->battle.look.center = 500.0f * center;
    scp->battle.look.radius = 500.0f * radius;
}

/** Sets @p scp's feel area: centre offset ahead and radius, in units of 500. */
void shBattleSetFeelArea(struct SubCharacter *scp, float center, float radius) {
    scp->battle.feel.center = 500.0f * center;
    scp->battle.feel.radius = 500.0f * radius;
}

/** Clears the target lists. */
void shBattleInitEnemyCheckWork(void) {
    shQzero(sh2_target_info, sizeof(sh2_target_info));
    shQzero(sh2_target_info_buf, sizeof(sh2_target_info_buf));
    rest_tgt = rest_tgt_buf = 20;
}

/** Clears the target lists and the attack queue. */
void shBattleInit(void) {
    shBattleInitEnemyCheckWork();
    shBattleInitAttackQueue();
}

/** Runs the attack queue. */
void shBattleExec(void) {
    shBattleExecAttackQueue();
}
