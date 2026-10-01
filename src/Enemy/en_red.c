/*
 * en_red.c: AI of the RED enemy: wanders, chases, seizes and attacks the player, walks
 * stairs, and has a battle-end state. Shared enemy code is in en_common.c.
 * Pyramid Head, with the Great Knife (verified; docs/characters.md).
 */
#include "enemy.h"

static void enREDCtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enREDCtrlSleep(struct EnLOCAL_DATA *dp);
static void enREDCtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enREDCtrlEvent(struct EnLOCAL_DATA *dp);
static void enREDCtrlHand(struct EnLOCAL_DATA *dp);
static void enREDCtrlWander(struct EnLOCAL_DATA *dp);
static void enREDCtrlChase(struct EnLOCAL_DATA *dp);
static void enREDCtrlBerserk(struct EnLOCAL_DATA *dp);
static void enREDCtrlStair(struct EnLOCAL_DATA *dp);
static void enREDCtrlAttack(struct EnLOCAL_DATA *dp);
static void enREDCtrlSeize(struct EnLOCAL_DATA *dp);
static void enREDCtrlConfuse(struct EnLOCAL_DATA *dp);
static void enREDCtrlBattleEnd(struct EnLOCAL_DATA *dp);
static void enREDCtrlOnlyWalk(struct EnLOCAL_DATA *dp);
static void enREDCheckPlayerWeapon(struct EnLOCAL_DATA *dp);
static int enREDSetDamage(struct EnLOCAL_DATA *dp);
static int enREDCanSeePlayer(struct EnLOCAL_DATA *dp);
static int enREDCanSeeCharacter(struct EnLOCAL_DATA *dp, struct SubCharacter *scp);
static void enREDAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enREDAnimeReset(struct EnLOCAL_DATA *dp, int anim);
static void enREDAnimeExec(struct EnLOCAL_DATA *dp);
static float enREDGetSpeed(struct EnLOCAL_DATA *dp);
static float enREDGetWalkSpeed(struct EnLOCAL_DATA *dp);
static float enREDGetAttackSpeed(struct EnLOCAL_DATA *dp);
static float enREDGetFeelRange(void);
static float enREDGetRotSpeed(void);
static void enREDSetSlowTime(struct EnLOCAL_DATA *dp);
static void enREDSetMoveCount(struct EnLOCAL_DATA *dp);
static void enREDSoundLife(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnREDAnime[11] = {
    { 0x14B5, 0 }, { 0x14B6, 1 }, { 0x14BE, 1 }, { 0x14B7, 0 }, { 0x14B8, 0 }, { 0x14B9, 0 },
    { 0x14BA, 0 }, { 0x14BB, 0 }, { 0x14BC, 0 }, { 0x14BD, 0 }, { 0x14BF, 1 },
};

float ap_boss_point[3][4] = {
    { 100000.0f, -1700.0f, -101000.0f, 1.0f },
    { 98850.0f, -1700.0f, -99400.0f, 1.0f },
    { 99200.0f, -1700.0f, -99000.0f, 1.0f },
};

float ap_boss_end_point[6][4] = {
    { 98850.0f, -1700.0f, -99400.0f, 1.0f },
    { 98800.0f, -1700.0f, -99000.0f, 1.0f },
    { 101136.0f, -850.0f, -98992.0f, 1.0f },
    { 101136.0f, -850.0f, -99820.0f, 1.0f },
    { 98800.0f, 0.0f, -99800.0f, 1.0f },
    { 98800.0f, 0.0f, -98620.0f, 1.0f },
};

float hp_roof_pos[4] = { 17800.0f, 0.0f, -23200.0f, 0.0f };

/** Sets up a new RED: HP by difficulty mode, size, starting level from the spawn status.
 * @param dp enemy work */
void enREDInitData(struct EnLOCAL_DATA *dp) {
    float vitarity[5] = { 1200.0f, 3000.0f, 4000.0f, 5000.0f, 7000.0f };
    float endurance[5] = { 500.0f, 700.0f, 1000.0f, 2000.0f, 5000.0f };
    int mode;
    float hp;
    float ep;

    mode = enGetMode();
    enSetSize(dp, 150.0f, 900.0f, 800.0f, 850.0f);
    dp->weight = 3;
    shCharacterSetWeaponRED(dp->scp, 1);
    switch (dp->scp->en_first_status) {
    case 6:
        dp->slv = 1;
        dp->sslv = 0;
        dp->type = 2;
        hp = vitarity[mode];
        ep = 0.3f * endurance[mode];
        if (mode <= 2) {
            dp->red.boss_timer = 10800;
        } else {
            dp->red.boss_timer = 18000;
        }
        break;
    case 12:
        dp->mlv = 4;
        dp->type = 0;
        enSetHP(dp, 1000000.0f, 1000000.0f);
        shCharacterSetWeaponRED(dp->scp, 0);
        enFlagSetMoved(dp);
        return;
    case 15:
        dp->mlv = 4;
        dp->type = 1;
        enSetHP(dp, 1000000.0f, 1000000.0f);
        return;
    default:
        dp->slv = 0;
        dp->sslv = 0;
        dp->type = 0;
        hp = 1000000.0f;
        ep = endurance[mode];
        enFlagSetMoved(dp);
        dp->red.attack_count = 2;
        break;
    }
    dp->mlv = 1;
    enSetHP(dp, hp, ep);
    enREDAnimeSet(dp, 1);
    if (enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

/** Per-frame control of a RED: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enREDCtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlREDFunc[6])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enREDCtrlAutomatic, enREDCtrlSleep, enREDCtrlGoPlayable, enREDCtrlEvent, enREDCtrlHand,
    };

    enCtrlREDFunc[dp->mlv](dp);
}

static void enREDCtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlREDSubFunc[9])(struct EnLOCAL_DATA *) = {
        enREDCtrlWander, enREDCtrlChase,   enREDCtrlBerserk,   enREDCtrlStair,    enREDCtrlAttack,
        enREDCtrlSeize,  enREDCtrlConfuse, enREDCtrlBattleEnd, enREDCtrlOnlyWalk,
    };

    enSetBattleTarget(dp, 0);
    enREDCheckPlayerWeapon(dp);
    enCtrlREDSubFunc[dp->slv](dp);
    if (enReduceTimer(dp) <= 0) {
        dp->endurance += 200.0f * shGetDT();
        if (dp->endurance > dp->endurance_max) {
            dp->endurance = dp->endurance_max;
        }
    }
    if (dp->type == 2 && dp->slv != 7) {
        if ((dp->red.boss_timer -= shGetDF()) <= 0) {
            EN_SET_LEVEL(dp, 7);
            dp->endurance_max *= 1.5f;
        }
    }
    enREDAnimeExec(dp);
    enMoveExec(dp);
    if (dp->type != 2 && enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

static void enREDCtrlSleep(struct EnLOCAL_DATA *dp) {
    if (enCheckSleepOut(dp)) {
        enSleepOut(dp);
        dp->slv = 0;
        dp->sslv = 0;
        dp->type = 0;
    }
}

static void enREDCtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    dp->slv = 1;
    dp->sslv = 0;
}

static void enREDCtrlEvent(struct EnLOCAL_DATA *dp) {
    switch (dp->type) {
    case 0:
        if (!dp->sslv) {
            enREDAnimeSet(dp, 10);
            dp->sslv++;
        }
        break;
    case 1:
        if (!dp->sslv) {
            enREDAnimeSet(dp, 1);
            enInitPath(&dp->path, -PI / 2);
            dp->sslv++;
        }
        dp->path.markangle = enCalcDirection(hp_roof_pos, (float *)&dp->scp->pos);
        enMoveAngle(&dp->path, enREDGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        if (enDistXZ(hp_roof_pos, (float *)&dp->scp->pos) < 50.0f) {
            game_flag.flag[6] |= 0x400;
        }
        break;
    }
    enREDAnimeExec(dp);
    enMoveExec(dp);
}

static void enREDCtrlHand(struct EnLOCAL_DATA *dp) {
}

static void enREDCtrlWander(struct EnLOCAL_DATA *dp) {
    float vec[4];
    int t;

    if (enCheckDamage(dp)) {
        if (enREDSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 6);
            return;
        }
    }
    if (!dp->sslv) {
        enREDAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    enREDGetWalkSpeed(dp);
    shSinCosV_Scale(vec, dp->scp->rot.y, 1500.0f);
    _shAddVector(vec, (float *)&dp->scp->pos, vec);
    enSetPath(dp, vec, (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, enREDGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    t = enREDCanSeePlayer(dp);
    if (t >= 2) {
        EN_SET_LEVEL(dp, 4);
    } else if (t == 1) {
        EN_SET_LEVEL(dp, 1);
    }
    enREDSoundLife(dp);
}

static void enREDCtrlChase(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        if (enREDSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 6);
            return;
        }
    }
    if (!dp->sslv) {
        enREDAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    enSetPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, enREDGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    dp->anim_s = ftoi(4096.0f * enREDGetWalkSpeed(dp) *
                      enCalcSpeedRate(dp->scp->rot.y, (float *)&dp->scp->pos, enGetPlayerPos(dp)));
    t = enREDCanSeePlayer(dp);
    if (t == 5) {
        EN_SET_LEVEL(dp, 5);
    } else if (t >= 2) {
        EN_SET_LEVEL(dp, 4);
    } else if (t == 0 && dp->type != 2) {
        EN_SET_LEVEL(dp, 0);
    } else if (dp->type == 2 && enGetPlayerPos(dp)[2] > -99300.0f) {
        EN_SET_LEVEL(dp, 3);
    } else if (dp->type == 2 && dp->scp->pos.z > -99300.0f && enGetPlayerPos(dp)[2] < -99300.0f) {
        EN_SET_LEVEL(dp, 3);
        if (dp->scp->pos.x > 99200.0f) {
            dp->sslv = 5;
        } else {
            dp->sslv = 6;
        }
    }
    enREDSoundLife(dp);
}

static void enREDCtrlBerserk(struct EnLOCAL_DATA *dp) {
    float vec[4];
    float d;
    int t;
    struct SubCharacter *scp;
    float *pos;

    if (enCheckDamage(dp)) {
        if (enREDSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 6);
            return;
        }
    }
    if (!dp->sslv) {
        enREDAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    enREDGetWalkSpeed(dp);
    scp = enGetNearCharacter(dp);
    pos = (float *)&scp->pos;
    _shSubVector(vec, (float *)&dp->scp->pos, pos);
    d = _shVectorLength(vec);
    if (d < 250.0f) {
        _shCopyVector(vec, pos);
    } else {
        _shScaleVector(vec, vec, 250.0f / d);
        _shAddVector(vec, pos, vec);
    }
    enSetPath(dp, vec, (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, enREDGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    t = enREDCanSeeCharacter(dp, scp);
    if (t >= 2) {
        EN_SET_LEVEL(dp, 4);
    }
    enREDSoundLife(dp);
}

static void enREDCtrlStair(struct EnLOCAL_DATA *dp) {
    float *target;

    target = NULL;
    if (enCheckDamage(dp)) {
        if (enREDSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 6);
            return;
        }
    }
    if (enGetPlayerPos(dp)[2] < -99400.0f) {
        EN_SET_LEVEL(dp, 1);
        return;
    }
    switch (dp->sslv) {
    case 0:
        enREDAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    case 1:
        target = ap_boss_point[1];
        if (enDistXZ((float *)&dp->scp->pos, target) < 50.0f) {
            dp->sslv++;
        }
        break;
    case 2:
        target = ap_boss_point[2];
        if (enDistXZ((float *)&dp->scp->pos, target) < 50.0f) {
            dp->sslv++;
        }
        break;
    case 3:
        target = enGetPlayerPos(dp);
        break;
    case 4:
        enAttackCheck(dp, (dp->anim == 3) ? (unsigned char)0x2C : (unsigned char)0x2D);
        if (dp->anim_n == -1) {
            enREDAnimeSet(dp, 1);
            enREDGetWalkSpeed(dp);
            enInitPath(&dp->path, dp->scp->rot.y);
            if (dp->scp->pos.z < -99300.0f) {
                dp->sslv = 6;
            } else {
                dp->sslv = 5;
            }
        }
        return;
    case 5:
        target = ap_boss_point[1];
        if (enDistXZ((float *)&dp->scp->pos, target) < 50.0f) {
            dp->sslv++;
        }
        break;
    case 6:
        target = ap_boss_point[0];
        if (enDistXZ((float *)&dp->scp->pos, target) < 50.0f) {
            EN_SET_LEVEL(dp, 1);
        }
        break;
    }
    if (dp->sslv <= 3 && dp->p_dist < 1500.0f &&
        enCalcAngleDifference(dp->scp->rot.y, enGetPlayerDirection(dp)) < 0.12217305f && dp->anim_n >= 48000) {
        if (dp->red.attack_count <= 2) {
            enREDAnimeSet(dp, 3);
        } else {
            enREDAnimeSet(dp, 4);
        }
        enAttackStart(dp);
        dp->red.attack_count++;
        dp->sslv = 4;
    }
    enREDGetWalkSpeed(dp);
    dp->path.markangle = enCalcDirection(target, (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, enREDGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    if (dp->sslv >= 5 && dp->p_dist < 500.0f) {
        EN_SET_LEVEL(dp, 1);
    }
    enREDSoundLife(dp);
}

static void enREDCtrlAttack(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp) != 0) {
        enREDSetDamage(dp);
    }
    switch (dp->sslv) {
    case 0:
        t = enREDCanSeePlayer(dp);
        if (t == 3) {
            enREDAnimeReset(dp, 4);
        } else if (t == 4) {
            enREDAnimeReset(dp, 5);
        } else {
            enREDAnimeReset(dp, 3);
        }
        enFlagSetCritical(dp);
        enAttackStart(dp);
        dp->red.attack_count++;
        dp->sslv += 1;
        break;
    case 1:
        if (dp->anim != 4) {
            enMoveAngleToPlayer(dp, enREDGetRotSpeed());
        }
        enAttackCheck(dp, dp->anim + 0x29);
        if (dp->anim_n == -1) {
            enFlagResetCritical(dp);
            if (dp->type == 2 && enCheckDeath(dp)) {
                dp->slv = 7; dp->sslv = 0;
            } else {
                if (enREDCanSeePlayer(dp) >= 2) {
                    dp->sslv = 0;
                } else if (dp->type == 2) {
                    EN_SET_LEVEL(dp, 1);
                } else if (dp->type == 1) {
                    if (dp->anim == 4 && enREDCanSeePlayer(dp) <= 1) {
                        dp->type = 0;
                        EN_SET_LEVEL(dp, 0);
                    } else {
                        EN_SET_LEVEL(dp, 2);
                    }
                } else {
                    EN_SET_LEVEL(dp, 0);
                }
            }
        }
        break;
    }
    enREDGetAttackSpeed(dp);
}

/* FAKEMATCH: fitted to the enSetNewSize calls' float-constant order, not recovered: the `!= 0` tests, `(void)0;`,
 * `attack_count = attack_count + 1` and `flag = flag | 8` here, and spellings in enREDCtrlAttack
 * (docs/matching-notes.md#en_red-enredctrlseize). */
static void enREDCtrlSeize(struct EnLOCAL_DATA *dp) {
    int t;
    if (enCheckDamage(dp) != 0) {
        if (enCheckHuggedPlayer() != 0) {
            (void)0;
            enResetDamage(dp);
        } else if (enREDSetDamage(dp) != 0) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        t = enREDCanSeePlayer(dp);
        enREDAnimeSet(dp, 6);
        enSetNewSize(dp, 250.0f, 900.0f, 800.0f, 850.0f);
        enFlagSetCritical(dp);
        enAttackStart(dp);
        dp->red.attack_count = dp->red.attack_count + 1;
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
        break;
    case 1:
        dp->path.markangle = enGetPlayerDirection(dp);
        enMoveAngle(&dp->path, 0.17453292f);
        dp->scp->rot.y = dp->path.angle;
        enAttackCheck(dp, 0x2F);
        if (!(dp->flag & 4)) {
            dp->flag = dp->flag | 8;
        }
        if (dp->anim_n == -1) {
            enSetNewSize(dp, 150.0f, 900.0f, 800.0f, 850.0f);
            enFlagResetCritical(dp);
            dp->flag |= 0x8000;
            if (dp->flag & 4) {
                enREDAnimeSet(dp, 9);
                dp->sslv = 4;
            } else {
                enREDAnimeSet(dp, 7);
                enAttackStart(dp);
                dp->sslv = 2;
            }
            dp->flag &= ~0x8000;
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            enREDAnimeSet(dp, 8);
            dp->sslv++;
        }
        break;
    case 3:
        enAttackCheckHug(dp, 0x30);
        if (!enCheckHuggedPlayer()) {
            enREDAnimeSet(dp, 9);
            dp->sslv++;
        }
        break;
    case 4:
        if (dp->anim_n == -1) {
            dp->flag &= ~8;
            if (dp->type == 2) {
                EN_SET_LEVEL(dp, 1);
            } else if (dp->type == 1) {
                dp->type = 0;
                EN_SET_LEVEL(dp, 0);
            } else {
                EN_SET_LEVEL(dp, 0);
            }
        }
        break;
    }
}

static void enREDCtrlConfuse(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;

    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            if (dp->sslv != 4) {
                dp->sslv = 0;
            }
        } else {
            if (enREDSetDamage(dp)) {
                EN_SET_LEVEL(dp, 7);
            } else {
                EN_SET_LEVEL(dp, 0);
            }
            return;
        }
    }
    if (!dp->sslv) {
        enReduceHP(dp);
        enREDAnimeSet(dp, 2);
        dp->sslv++;
    }
    if (dp->sslv >= 4) {
        tp = dp->red.tp;
        dp->path.markangle = enCalcDirection((float *)&tp->scp->pos, (float *)&dp->scp->pos);
        enMoveAngle(&dp->path, enREDGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        enAttackCheck(dp, 0x2C);
        if (dp->anim_n == -1) {
            if (++dp->sslv <= 5 && enDistXZ((float *)&tp->scp->pos, (float *)&dp->scp->pos) < 900.0f) {
                enAttackStart(dp);
                enREDAnimeReset(dp, 3);
            } else {
                EN_SET_LEVEL(dp, 0);
            }
        }
        return;
    }
    if (dp->sslv == 1 && dp->anim_n == -1) {
        tp = enGetNearOtherEnemy(dp);
        if (tp && enGetSprayPower() && fabsf(tp->scp->pos.y - ((float *)&dp->scp->pos)[1]) < 250.0f &&
            enDistXZ((float *)&tp->scp->pos, (float *)&dp->scp->pos) < 900.0f) {
            dp->red.tp = tp;
            enAttackStart(dp);
            enREDAnimeSet(dp, 3);
            enInitPath(&dp->path, dp->scp->rot.y);
            dp->sslv = 4;
        } else {
            dp->sslv++;
            enSetTimer(dp, 600 - enGetMode() * 90);
        }
    } else if (dp->sslv == 2 && enReduceTimer(dp) <= 0) {
        EN_SET_LEVEL(dp, 0);
    }
    enREDSoundLife(dp);
}

static void enREDCtrlBattleEnd(struct EnLOCAL_DATA *dp) {
    float *target;

    if (enCheckDamage(dp)) {
        enResetDamage(dp);
    }
    switch (dp->sslv) {
    case 0:
        enREDAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        game_flag.flag[4] |= 0x10000;
        enSetTimer(dp, 300);
        dp->sslv++;
        break;
    case 1:
        enMoveAngleToPlayer(dp, enREDGetRotSpeed());
        if (enGetPlayerPos(dp)[2] > -99400.0f && enREDCanSeePlayer(dp) >= 2) {
            enREDAnimeSet(dp, 3);
            enAttackStart(dp);
            dp->sslv = 2;
        } else if (enReduceTimer(dp) <= 0 && enGetPlayerPos(dp)[2] < -99400.0f) {
            if (dp->scp->pos.z < -99400.0f) {
                dp->sslv = 4;
            } else if (dp->scp->pos.z < -99150.0f) {
                dp->sslv = 5;
            } else {
                dp->sslv = 6;
                dp->flag |= 0x10;
            }
        }
        break;
    case 2:
        enReduceTimer(dp);
        enMoveAngleToPlayer(dp, enREDGetRotSpeed());
        enAttackCheck(dp, 0x2C);
        if (dp->anim_n == -1) {
            if (enGetPlayerPos(dp)[2] > -99400.0f && enREDCanSeePlayer(dp) >= 2) {
                enREDAnimeSet(dp, 4);
                enAttackStart(dp);
                dp->sslv = 3;
            } else {
                enREDAnimeSet(dp, 1);
                dp->sslv = 1;
            }
        }
        break;
    case 3:
        enReduceTimer(dp);
        enMoveAngleToPlayer(dp, enREDGetRotSpeed());
        enAttackCheck(dp, 0x2D);
        if (dp->anim_n == -1) {
            if (enGetPlayerPos(dp)[2] > -99400.0f && enREDCanSeePlayer(dp) >= 2) {
                enREDAnimeReset(dp, 4);
                enAttackStart(dp);
            } else {
                enREDAnimeSet(dp, 1);
                dp->sslv = 1;
            }
        }
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        enREDGetWalkSpeed(dp);
        target = ap_boss_end_point[dp->sslv - 4];
        dp->path.markangle = enCalcDirection(target, (float *)&dp->scp->pos);
        enMoveAngle(&dp->path, enREDGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        if (dp->sslv >= 9) {
            dp->vec[1] = 500.0f;
        }
        if (enDistXZ((float *)&dp->scp->pos, target) < 50.0f) {
            if (dp->sslv <= 8) {
                if (++dp->sslv == 6) {
                    dp->flag |= 0x10;
                }
                if (dp->sslv == 9) {
                    dp->flag |= 2;
                }
            } else {
                game_flag.flag[4] |= 0x40000;
            }
        }
        break;
    }
}

static void enREDCtrlOnlyWalk(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        enREDAnimeSet(dp, 1);
        enREDGetWalkSpeed(dp);
    }
}

static void enREDCheckPlayerWeapon(struct EnLOCAL_DATA *dp) {
    if (dp->type != 2) {
        switch (enGetPlayerWeapon()) {
        case 0:
        case 1:
        case 5:
        case 4:
            enFlagSetNoDamage(dp);
            break;
        default:
            enFlagResetNoDamage(dp);
            break;
        }
    }
}

static int enREDSetDamage(struct EnLOCAL_DATA *dp) {
    enREDSetSlowTime(dp);
    if (dp->type != 2) {
        dp->endurance -= dp->scp->battle.damage;
        if (dp->endurance < 0.0f) {
            dp->endurance = 0.0f;
        }
        dp->scp->battle.damage = 0.0f;
    } else {
        enReduceHP(dp);
        if (enCheckDeath(dp)) {
            return 1;
        }
    }
    return 0;
}

static int enREDCanSeePlayer(struct EnLOCAL_DATA *dp) {
    float *ppos;
    float dist;
    float a;
    float a1;
    int wcd;

    ppos = enGetPlayerPos(dp);
    wcd = enGetWorldCondition();
    dist = enCheckPath2(dp, ppos, (float *)&dp->scp->pos);
    if (dist >= 0.0f) {
        if (wcd >= 2 && enCheckSeeLight(dp)) {
            return 1;
        }
        return 0;
    }
    if (dp->p_dist > enREDGetFeelRange()) {
        return 0;
    }
    if (enCheckNoDamageHuman(dp)) {
        return 1;
    }
    if (!enCheckIntoScreen(dp)) {
        return 1;
    }
    if (dp->anim == 1 && dp->anim_n < 48000) {
        return 1;
    }
    dist = enDistXZ(ppos, (float *)&dp->scp->pos);
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a1 = a = shAngleRegulate(a - dp->scp->rot.y);
    a = fabsf(a);
    if (dp->type == 2 && dist < 500.0f && a < 0.5235988f) {
        return 5;
    }
    if (dp->red.attack_count >= 3 && dist < 1500.0f && a < 0.12217305f) {
        return 3;
    }
    if (dist < 900.0f && a < 0.17453292f) {
        return 2;
    }
    if (dist < 500.0f && a1 < 0.5235988f && a1 > -0.6981317f && shRandF() < 0.05f) {
        return 4;
    }
    if (enLocalWork.Status & 1) {
        return 0;
    }
    return 1;
}

static int enREDCanSeeCharacter(struct EnLOCAL_DATA *dp, struct SubCharacter *scp) {
    float *ppos;
    float dist;
    float a;
    float a1;
    int wcd;
    int i;
    struct EnFORBIDDENAREA *fa;

    ppos = (float *)&scp->pos;
    wcd = enGetWorldCondition();
    dist = enCheckPath2(dp, ppos, (float *)&dp->scp->pos);
    if (dist >= 0.0f) {
        return 0;
    }
    dist = enDistXZ(ppos, (float *)&dp->scp->pos);
    if (dist > enREDGetFeelRange()) {
        return 0;
    }
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a1 = a = shAngleRegulate(a - dp->scp->rot.y);
    a = fabsf(a);
    if (dist < 900.0f && a < 0.17453292f) {
        return 2;
    }
    if (dist < 1500.0f && a < 0.12217305f) {
        return 3;
    }
    if (dist < 500.0f && a1 < 0.5235988f && a1 > -0.6981317f && shRandF() < 0.05f) {
        return 4;
    }
    fa = enLocalWork.ForbiddenArea;
    for (i = 0; i < enLocalWork.ForbiddenNum; i++, fa++) {
        if (ppos[0] >= fa->x0 && ppos[0] <= fa->x1 && ppos[2] >= fa->z0 && ppos[2] <= fa->z1) {
            return 0;
        }
    }
    return 1;
}

static void enREDAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        enAnimeRestart(dp);
        if (anim == 2) {
            enREDSetMoveCount(dp);
        }
        return;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 1003
    fjAssert(anim >= 0 && anim < sizeof(EnREDAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnREDAnime[anim].Anime);
    if (anim == 2) {
        enREDSetMoveCount(dp);
    }
}

static void enREDAnimeReset(struct EnLOCAL_DATA *dp, int anim) {

    /* Matching: the assert bakes its original line number into the object. */
#line 1014
    fjAssert(anim >= 0 && anim < sizeof(EnREDAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnREDAnime[anim].Anime);
    if (anim == 2) {
        enREDSetMoveCount(dp);
    }
}

static void enREDAnimeExec(struct EnLOCAL_DATA *dp) {
    int of;

    of = dp->anim_n;
    enAnimeExec(dp, EnREDAnime, 0x14B5);
    if (dp->anim_n == -1 || (dp->flag & 1)) {
        return;
    }
    if (dp->anim == 1) {
        enSetTransWalk(dp);
        if ((dp->anim_s > 0 && of < 0x12C0 && dp->anim_n >= 0x12C0) ||
            (dp->anim_s < 0 && of >= 0x12C0 && dp->anim_n < 0x12C0)) {
            if (dp->type == 2 && dp->scp->pos.z > -1600.0f) {
                enSoundCall(0x3E94, 1.0f, (float *)&dp->scp->pos);
            } else {
                if (enCheckWater(dp)) {
                    enSoundCall(0x4978, 1.0f, (float *)&dp->scp->pos);
                }
            }
            if (dp->p_dist > 5000.0f) {
                enSoundCall3D(ftoi(3.0f * shRandF()) * 2 + 0x3EE4, 1.0f, (float *)&dp->scp->pos);
            } else {
                enSoundCall(ftoi(6.0f * shRandF()) + 0x3EE4, 1.0f, (float *)&dp->scp->pos);
            }
        }
    } else if (dp->anim == 3 || dp->anim == 4) {
        if (dp->scp->battle.atk_result == 2 && dp->anim_step == 0) {
            dp->anim_step++;
            enSoundCall(0x3EEB, 1.0f, (float *)&dp->scp->pos);
        }
    }
}

static float enREDGetSpeed(struct EnLOCAL_DATA *dp) {
    return 0.25f + 0.75f * (dp->endurance / dp->endurance_max);
}

static float enREDGetWalkSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.3f, 0.5f, 0.8f, 1.0f, 1.2f };
    float r;

    r = enREDGetSpeed(dp) * speed_rate[enGetMode()];
    if (dp->type == 1) {
        r *= 1.2f;
    }
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

static float enREDGetAttackSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.75f, 0.8f, 1.0f, 1.1f, 1.2f };
    float r;

    r = enREDGetSpeed(dp) * speed_rate[enGetMode()];
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

static float enREDGetFeelRange(void) {
    float feel_range[5] = { 1000.0f, 1500.0f, 2000.0f, 2500.0f, 4000.0f };
    float r;

    r = feel_range[enGetMode()];
    if (enGetWorldCondition() == 4) {
        r *= 0.75f;
    }
    return r;
}

static float enREDGetRotSpeed(void) {
    float rot_rate[5] = { 0.5f, 0.7f, 1.0f, 1.5f, 2.0f };

    return 0.05235988f * rot_rate[enGetMode()];
}

static void enREDSetSlowTime(struct EnLOCAL_DATA *dp) {
    int timer[5] = { 180, 90, 60, 30, 1 };

    enSetTimer(dp, timer[enGetMode()] * 2);
}

static void enREDSetMoveCount(struct EnLOCAL_DATA *dp) {
    int n;

    n = ftoi(6.0f - 0.2f * enGetMode() + shSway1f(-2.0f, 0.5f));
    enSetAnimeCount(dp, n << 11);
}

/* Matching: `dp->sound_wait += 1` (the same code as `++`) sets this function's float-constant order
 * (1.0f after the sound number). */
static void enREDSoundLife(struct EnLOCAL_DATA *dp) {
    if (dp->sound_wait < 300) {
        dp->sound_wait += 1;
    } else if (dp->anim == 2 || shRandF() < 0.2f * shGetDT()) {
        enSoundCall(0x3EF6 + ftoi(2.0f * shRandF()), 1.0f, (float *)&dp->scp->pos);
        dp->sound_wait = 0;
    }
}
