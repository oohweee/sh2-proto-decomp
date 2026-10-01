/*
 * en_edb.c: AI of Eddie (EDB), a human enemy who approaches, hides, escapes, shoots and punches;
 * also NIK, a hanging object that swings when hit. Shared enemy code is in en_common.c.
 * Eddie's boss fight; NIK is the hanging meat in it (verified; docs/characters.md).
 */
#include "enemy.h"

static void enEDBCtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enEDBCtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enEDBCtrlHand(); /* No parameters, as in the DWARF; K&R so it still fits the handler table. */
static void enEDBCtrlApproach(struct EnLOCAL_DATA *dp);
static void enEDBCtrlAway(struct EnLOCAL_DATA *dp);
static void enEDBCtrlHidden(struct EnLOCAL_DATA *dp);
static void enEDBCtrlEscape(struct EnLOCAL_DATA *dp);
static void enEDBCtrlSearch(struct EnLOCAL_DATA *dp);
static void enEDBCtrlShoot(struct EnLOCAL_DATA *dp);
static void enEDBCtrlPunch(struct EnLOCAL_DATA *dp);
static void enEDBCtrlDamage(struct EnLOCAL_DATA *dp);
static void enEDBCtrlEscape2(struct EnLOCAL_DATA *dp);
static void enEDBCtrlDead(struct EnLOCAL_DATA *dp);
static int enEDBCheckArriveCorner(struct EnLOCAL_DATA *dp);
static int enEDBGetNearMeat(struct EnLOCAL_DATA *dp);
static void enEDBClearMark(struct EnLOCAL_DATA *dp);
static void enEDBSetMark(struct EnLOCAL_DATA *dp, int mark);
static int enEDBCheckMark(struct EnLOCAL_DATA *dp, int mark);
static int enEDBGetDamageMotion(struct EnLOCAL_DATA *dp);
static int enEDBSetDamage(struct EnLOCAL_DATA *dp);
static int enEDBCanSeePlayer(struct EnLOCAL_DATA *dp, float angle);
static void enEDBAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enEDBAnimeReset(struct EnLOCAL_DATA *dp, int anim);
static void enEDBAnimeExec(struct EnLOCAL_DATA *dp);
static void enEDBSetCartridge(struct EnLOCAL_DATA *dp);
static void enEDBAutoRecovery(struct EnLOCAL_DATA *dp);
static float enEDBGetSpeed(struct EnLOCAL_DATA *dp);
static float enEDBGetWalkSpeed(struct EnLOCAL_DATA *dp);
static float enEDBGetRunSpeed(struct EnLOCAL_DATA *dp);
static float enEDBGetHoldSpeed(struct EnLOCAL_DATA *dp);
/* Matching: K&R: the callers pass dp or nothing; it takes no parameters (the DWARF has none). */
static float enEDBGetAimingSpeed();
static int enEDBGetDefAfford(void);
static int enEDBGetPunchLimit(void);
static void enEDBAddAfford(struct EnLOCAL_DATA *dp);

float door_pos[4] = { 100000.0f, 0.0f, -99100.0f, 0.0f };

float room_corner[6][4] = {
    { 142500.0f, 0.0f, -103750.0f, 0.0f }, { 142500.0f, 0.0f, -96250.0f, 0.0f },
    { 138250.0f, 0.0f, -103750.0f, 0.0f }, { 138250.0f, 0.0f, -97500.0f, 0.0f },
    { 139500.0f, 0.0f, -96250.0f, 0.0f },  { 142500.0f, 0.0f, -97000.0f, 0.0f },
};

struct shStayObjectSettingData nik_pos_data[11] = {
    { 0x421, 0, 0, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, 0.378405f, 0.0f, 1.0f }, { 138900.0f, -2000.0f, -97760.61f, 1.0f } },
    { 0x421, 0, 1, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, 0.442f, 0.0f, 1.0f }, { 140200.0f, -2000.0f, -97561.61f, 1.0f } },
    { 0x421, 0, 2, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, -0.534405f, 0.0f, 1.0f }, { 141500.0f, -2000.0f, -96916.05f, 1.0f } },
    { 0x421, 0, 3, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, -0.389594f, 0.0f, 1.0f }, { 138900.0f, -2000.0f, -98986.56f, 1.0f } },
    { 0x421, 0, 4, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, -0.534405f, 0.0f, 1.0f }, { 141500.0f, -2000.0f, -98202.31f, 1.0f } },
    { 0x421, 0, 5, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, -0.389594f, 0.0f, 1.0f }, { 138900.0f, -2000.0f, -100236.04f, 1.0f } },
    { 0x421, 0, 6, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, -1.073996f, 0.0f, 1.0f }, { 140200.0f, -2000.0f, -99800.35f, 1.0f } },
    { 0x421, 0, 7, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, 0.80919f, 0.0f, 1.0f }, { 141500.0f, -2000.0f, -101270.47f, 1.0f } },
    { 0x421, 0, 8, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, -0.357592f, 0.0f, 1.0f }, { 138900.0f, -2000.0f, -102965.664f, 1.0f } },
    { 0x421, 0, 9, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, -0.0404f, 0.0f, 1.0f }, { 140200.0f, -2000.0f, -102423.51f, 1.0f } },
    { 0x421, 0, 10, { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, 1.325595f, 0.0f, 1.0f }, { 141500.0f, -2000.0f, -102587.93f, 1.0f } },
};

struct EnANIME_DATA EnEDBAnime[22] = {
    { 0x170D, 0 }, { 0x170D, 1 }, { 0x170E, 1 }, { 0x170F, 1 }, { 0x1710, 0 }, { 0x1711, 0 },
    { 0x1712, 0 }, { 0x1713, 0 }, { 0x1714, 0 }, { 0x1715, 0 }, { 0x1716, 0 }, { 0x1717, 0 },
    { 0x1718, 0 }, { 0x1719, 0 }, { 0x171A, 0 }, { 0x171B, 0 }, { 0x171C, 0 }, { 0x171D, 0 },
    { 0x171E, 0 }, { 0x171F, 0 }, { 0x1720, 0 }, { 0x1721, 0 },
};
/* Matching: fitted stand-in (not recovered code); it sets the next function's float-constant
 * order. Whether the original had code here: docs/stand-ins.md. */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f; }
/** Sets up a new Eddie: HP by difficulty mode and spawn status, size, bullets, first animation.
 * @param dp enemy work */
void enEDBInitData(struct EnLOCAL_DATA *dp) {
    float vitarity1[5] = { 500.0f, 1800.0f, 2500.0f, 4000.0f, 5000.0f };
    float vitarity2[5] = { 2000.0f, 4000.0f, 5000.0f, 7000.0f, 10000.0f };
    float endurance[5] = { 200.0f, 300.0f, 350.0f, 500.0f, 600.0f };
    int mode;
    float hp;

    mode = enGetMode();
    dp->mlv = 1;
    enSetBattleTarget(dp, 1);
    dp->edb.bullet = 6;
    switch (dp->scp->en_first_status) {
    case 6:
        EN_SET_LEVEL(dp, 4);
        dp->type = 1;
        hp = vitarity2[mode];
        dp->flag |= 0x100;
        break;
    default:
        EN_SET_LEVEL(dp, 0);
        dp->type = 0;
        dp->edb.bullet--;
        hp = vitarity1[mode];
        break;
    }
    enSetHP(dp, hp, endurance[mode]);
    enSetSize(dp, 200.0f, 850.0f, 750.0f, 750.0f);
    dp->weight = 3;
    enEDBClearMark(dp);
    dp->edb.afford = enEDBGetDefAfford();
    dp->edb.speed = 1.0f;
    enEDBAnimeSet(dp, 1);
    dp->flag |= 0x400;
}

/** Per-frame control of Eddie: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enEDBCtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlEDBFunc[6])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enEDBCtrlAutomatic, enDummyCtrl, enEDBCtrlGoPlayable, enDummyCtrl, enEDBCtrlHand,
    };

    enCtrlEDBFunc[dp->mlv](dp);
}

static void enEDBCtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlEDBSubFunc[10])(struct EnLOCAL_DATA *) = {
        enEDBCtrlApproach, enEDBCtrlAway,   enEDBCtrlHidden, enEDBCtrlEscape,   enEDBCtrlSearch,
        enEDBCtrlShoot,    enEDBCtrlPunch,  enEDBCtrlDamage, enEDBCtrlEscape2,  enEDBCtrlDead,
    };

    enSetBattleTarget(dp, 1);
    enCtrlEDBSubFunc[dp->slv](dp);
    dp->edb.speed += 0.5 * shGetDT();
    if (dp->edb.speed > 1.0f) {
        dp->edb.speed = 1.0f;
    }
    enEDBAnimeExec(dp);
    enMoveExec(dp);
}

static void enEDBCtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    EN_SET_LEVEL(dp, 4);
}

/* K&R definition, see the declaration above. */
static void enEDBCtrlHand() {
}

static void enEDBCtrlApproach(struct EnLOCAL_DATA *dp) {
    int t;
    int c;

    if (enCheckDamage(dp)) {
        if (enEDBSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
    }
    t = enEDBCanSeePlayer(dp, dp->path.angle);
    c = enCheckPlayerCondition(dp);
    if (t == 2 && (!(c & 1) || (c & 3) == 3) && enCheckPlayerWeapon()) {
        EN_SET_LEVEL(dp, 5);
        return;
    }
    if (dp->p_dist < 1000.0f) {
        EN_SET_LEVEL(dp, 6);
        return;
    }
    if (!dp->sslv) {
        enInitPath(&dp->path, dp->scp->rot.y);
        enEDBAnimeSet(dp, 2);
        dp->sslv++;
    }
    enMoveAngleToPlayer(dp, 0.05235988f);
    shSinCosV_Scale(dp->vec, dp->scp->rot.y, enEDBGetWalkSpeed(dp));
    enEDBAutoRecovery(dp);
}

static void enEDBCtrlAway(struct EnLOCAL_DATA *dp) {
    float vec[4];
    float d1;
    float d2;
    float vec2[4];
    int dir;

    if (enCheckDamage(dp)) {
        if (enEDBSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (dp->p_dist > 750.0f && enGetMode() >= 3) {
            EN_SET_LEVEL(dp, 5);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        enEDBAnimeSet(dp, 2);
        enInitPath(&dp->path, dp->scp->rot.y);
        enSetTimer(dp, 120);
        dp->sslv++;
    case 1:
        shSinCosV_Scale(vec, enGetPlayerDirection(dp), 1000.0f);
        _shSubVector(vec, (float *)&dp->scp->pos, vec);
        enReduceTimer(dp);
        if (enSetPath(dp, vec, (float *)&dp->scp->pos) && dp->timer <= 0) {
            if (dp->p_dist > 600.0f) {
                if (enCheckPlayerWeapon() || dp->p_dist > 1500.0f) {
                    if (enCalcAngleDifference(dp->scp->rot.y, enGetPlayerDirection(dp)) < 1.3962634f) {
                        dp->slv = 5;
                        dp->sslv = 0;
                    } else {
                        dp->path.dist = 0.0f;
                    }
                } else {
                    dp->slv = 0;
                    dp->sslv = 0;
                }
            } else {
                if (enGetPlayerWeapon() != 8) {
                    EN_SET_LEVEL(dp, 6);
                }
            }
        }
        if (dp->path.dist < 500.0f) {
            shSinCosV_Scale(vec2, dp->scp->rot.y - 1.5707964f, 400.0f);
            _shAddVector(vec, (float *)&dp->scp->pos, vec2);
            d1 = enCheckPath(dp, vec, (float *)&dp->scp->pos);
            _shSubVector(vec, (float *)&dp->scp->pos, vec2);
            d2 = enCheckPath(dp, vec, (float *)&dp->scp->pos);
            if (d1 < 0.0f) {
                if (d2 < d1) {
                    dir = 0;
                } else {
                    dir = 1;
                }
            } else {
                if (d2 < 0.0f || d2 > d1) {
                    dir = 0;
                } else {
                    dir = 1;
                }
            }
            enEDBAnimeReset(dp, dir + 4);
            dp->sslv = 2;
        } else {
            enMoveAngle(&dp->path, 0.05235988f);
            shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enEDBGetWalkSpeed(dp));
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            shSinCosV_Scale(vec, dp->scp->rot.y, 500.0f);
            if (enCheckPath(dp, vec, (float *)&dp->scp->pos) < 0.0f) {
                enEDBAnimeSet(dp, 2);
                enInitPath(&dp->path, dp->scp->rot.y);
                dp->sslv = 1;
            } else {
                EN_SET_LEVEL(dp, 0);
            }
        }
        break;
    }
    enEDBAutoRecovery(dp);
}

static void enEDBCtrlHidden(struct EnLOCAL_DATA *dp) {
    float a;
    float *npos;

    if (enCheckDamage(dp)) {
        if (enEDBSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        enEDBAnimeSet(dp, 3);
        if (enEDBGetNearMeat(dp) == -1) {
            EN_SET_LEVEL(dp, 3);
            break;
        }
        enSetTimer(dp, 60);
        dp->sslv++;
    case 1:
        if (dp->p_dist < 1000.0f) {
            EN_SET_LEVEL(dp, 6);
            break;
        }
        if (enDistXZ(dp->edb.target, (float *)&dp->scp->pos) < 150.0f) {
            dp->sslv = 3;
            break;
        }
        enSetPath(dp, dp->edb.target, (float *)&dp->scp->pos);
        if (enReduceTimer(dp) <= 0) {
            a = shAngleRegulate(dp->path.markangle - dp->scp->rot.y);
            if (a > 0.87266463f) {
                enEDBAnimeSet(dp, 4);
                dp->sslv = 2;
                break;
            }
            if (a < -0.87266463f) {
                enEDBAnimeSet(dp, 5);
                dp->sslv = 2;
                break;
            }
        }
        enMoveAngle(&dp->path, 0.05235988f);
        shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enEDBGetRunSpeed(dp));
        break;
    case 2:
        enEDBGetSpeed(dp);
        if (dp->anim_n == -1) {
            enEDBAnimeSet(dp, 3);
            enInitPath(&dp->path, dp->scp->rot.y);
            enSetTimer(dp, 60);
            dp->sslv = 1;
        }
        break;
    case 3:
        enEDBAddAfford(dp);
        if (!dp->edb.bullet) {
            enEDBAnimeSet(dp, 9);
            dp->sslv = 4;
            break;
        }
        enEDBAnimeSet(dp, 1);
        enSetTimer(dp, 600);
        dp->sslv = 5;
        break;
    case 4:
        enEDBGetSpeed(dp);
        if (dp->anim_n == -1) {
            if (!dp->edb.bullet) {
                enEDBAnimeSet(dp, 9);
                dp->sslv = 4;
                break;
            }
            enEDBAnimeSet(dp, 1);
            enSetTimer(dp, 600);
            dp->sslv = 5;
        }
        break;
    case 5:
        enEDBGetSpeed(dp);
        if (enReduceTimer(dp) <= 0) {
            if (enEDBGetNearMeat(dp) == -1) {
                EN_SET_LEVEL(dp, 4);
                break;
            }
            enEDBAnimeSet(dp, 3);
            enSetTimer(dp, 60);
            dp->sslv = 1;
            break;
        }
        switch (enEDBCanSeePlayer(dp, dp->scp->rot.y)) {
        case 3:
            EN_SET_LEVEL(dp, 6);
            break;
        case 2:
            EN_SET_LEVEL(dp, 5);
            break;
        case 0:
            if (dp->p_dist < 1500.0f) {
                if (enEDBGetNearMeat(dp) == -1) {
                    EN_SET_LEVEL(dp, 3);
                    break;
                }
                enEDBAnimeSet(dp, 3);
                enSetTimer(dp, 60);
                dp->sslv = 1;
            } else if ((enCheckPlayerCondition(dp) & 3) == 3 &&
                       enCheckPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos) < 0.0f) {
                dp->sslv = 0;
            } else {
                npos = (float *)&nik_pos_data[dp->edb.mark[dp->edb.mark_n] - 6].trans;
                a = enCalcAngleDifference(enCalcDirection((float *)&dp->scp->pos, npos),
                                          enCalcDirection(enGetPlayerPos(dp), npos));
                if (a < 2.0943952f) {
                    dp->sslv = 0;
                }
            }
            break;
        case 1:
            dp->path.markangle = enGetPlayerDirection(dp);
            dp->path.angle = dp->scp->rot.y;
            enMoveAngle(&dp->path, 0.05235988f);
            dp->scp->rot.y = dp->path.angle;
            break;
        }
        break;
    }
    enEDBAutoRecovery(dp);
}

static void enEDBCtrlEscape(struct EnLOCAL_DATA *dp) {
    int t;
    float a;
    float vec[4];

    if (enCheckDamage(dp)) {
        if (enEDBSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        a = shAngleRegulate(enGetPlayerDirection(dp) - dp->scp->rot.y);
        if (a >= 0.0f && a < 1.5707964f) {
            enEDBAnimeSet(dp, 5);
            dp->sslv = 1;
            break;
        }
        if (a <= 0.0f && a > -1.5707964f) {
            enEDBAnimeSet(dp, 4);
            dp->sslv = 1;
            break;
        }
        enEDBAnimeSet(dp, 3);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->edb.ccount = 0;
        enSetTimer(dp, 60);
        dp->sslv = 2;
        break;
    case 1:
        enEDBGetSpeed(dp);
        if (dp->anim_n == -1) {
            enEDBAnimeSet(dp, 3);
            enInitPath(&dp->path, dp->scp->rot.y);
            dp->edb.ccount = 0;
            enSetTimer(dp, 60);
            dp->sslv = 2;
        }
        break;
    case 2:
        if ((t = enEDBCheckArriveCorner(dp)) != -1 && !enEDBCheckMark(dp, t)) {
            enEDBSetMark(dp, t);
            enEDBAddAfford(dp);
            if (!dp->edb.bullet) {
                enEDBAnimeSet(dp, 9);
                dp->sslv = 3;
                break;
            }
            dp->sslv = 4;
            break;
        }
        shSinCosV_Scale(vec, dp->scp->rot.y, 1000.0f);
        _shAddVector(vec, (float *)&dp->scp->pos, vec);
        if (enSetPath(dp, vec, (float *)&dp->scp->pos)) {
            if (++dp->timer > 10 && dp->path.dist < 500.0f) {
                if (fabsf(dp->scp->rot.y) < 1.5707964f) {
                    enEDBAnimeSet(dp, 5);
                } else {
                    enEDBAnimeSet(dp, 4);
                }
                dp->sslv = 1;
                break;
            }
            dp->path.timer = 0;
        } else {
            dp->edb.ccount = 0;
        }
        if (dp->p_dist < 1000.0f) {
            if (enReduceTimer(dp) <= 0) {
                EN_SET_LEVEL(dp, 6);
                break;
            }
        } else {
            enSetTimer(dp, 60);
        }
        enMoveAngle(&dp->path, 0.05235988f);
        shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enEDBGetRunSpeed(dp));
        if (enEDBCanSeePlayer(dp, dp->scp->rot.y) >= 2) {
            EN_SET_LEVEL(dp, 6);
        }
        break;
    case 3:
        enEDBGetSpeed(dp);
        if (dp->anim_n == -1) {
            dp->sslv++;
        }
        break;
    case 4:
        enEDBGetSpeed(dp);
        if (enGetMode() <= 1) {
            EN_SET_LEVEL(dp, 2);
            break;
        }
        if (shAngleRegulate(enGetPlayerDirection(dp) - dp->scp->rot.y) > 0.0f) {
            enEDBAnimeSet(dp, 4);
        } else {
            enEDBAnimeSet(dp, 5);
        }
        dp->edb.turn = 0;
        dp->sslv++;
        break;
    case 5:
        enEDBGetSpeed(dp);
        if (dp->anim_n == -1) {
            if ((enCheckPlayerCondition(dp) & 7) == 3) {
                dp->sslv = 0;
                break;
            }
            if (++dp->edb.turn <= 2) {
                a = shAngleRegulate(enGetPlayerDirection(dp) - dp->scp->rot.y);
                if (a > 0.87266463f) {
                    enEDBAnimeReset(dp, 4);
                    break;
                }
                if (a < -0.87266463f) {
                    enEDBAnimeReset(dp, 5);
                    break;
                }
            }
            enEDBAnimeSet(dp, 6);
            enInitPath(&dp->path, dp->scp->rot.y);
            enSetTimer(dp, enGetMode() * 60 + 180);
            dp->sslv++;
        }
        break;
    case 6:
        enEDBGetHoldSpeed(dp);
        t = enEDBCanSeePlayer(dp, dp->scp->rot.y);
        if (t == 3) {
            EN_SET_LEVEL(dp, 6);
            break;
        }
        if (enReduceTimer(dp) <= 0 || dp->p_dist < 1500.0f) {
            dp->edb.afford = enEDBGetDefAfford();
            if (t > 0) {
                EN_SET_LEVEL(dp, 6);
                break;
            }
            EN_SET_LEVEL(dp, 4);
            break;
        }
        if (dp->anim_n == -1 && (t > 1 || (enCheckPlayerCondition(dp) & 3) == 3)) {
            EN_SET_LEVEL(dp, 5);
            break;
        }
        dp->path.markangle = enGetPlayerDirection(dp);
        enMoveAngle(&dp->path, enEDBGetAimingSpeed());
        dp->scp->rot.y = dp->path.angle;
        break;
    }
    enEDBAutoRecovery(dp);
}

static void enEDBCtrlSearch(struct EnLOCAL_DATA *dp) {
    float vec[4];

    if (enCheckDamage(dp)) {
        if (enEDBSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
    }
    if (!dp->sslv) {
        enEDBAnimeSet(dp, 2);
        enInitPath(&dp->path, dp->scp->rot.y);
        enEDBClearMark(dp);
        dp->edb.rot = dp->edb.arot = 0.0f;
        dp->sslv++;
    }
    dp->edb.arot = 0.9f * (dp->edb.arot + shSway1f(-0.017453292f, 0.02f) - 0.0001f * dp->edb.rot);
    dp->edb.rot += dp->edb.arot;
    switch (enEDBCanSeePlayer(dp, dp->scp->rot.y)) {
    case 3:
        EN_SET_LEVEL(dp, 6);
        return;
    case 2:
        EN_SET_LEVEL(dp, 5);
        return;
    case 1:
        if (dp->edb.pcount < enEDBGetPunchLimit()) {
            EN_SET_LEVEL(dp, 6);
            return;
        }
        break;
    default:
        if (shRandF() < 0.001f) {
            EN_SET_LEVEL(dp, 5);
            return;
        }
        if (dp->p_dist < 1500.0f) {
            EN_SET_LEVEL(dp, 6);
            return;
        }
        break;
    }
    shSinCosV_Scale(vec, dp->scp->rot.y + dp->edb.rot, 1000.0f);
    _shAddVector(vec, (float *)&dp->scp->pos, vec);
    if (enSetPath(dp, vec, (float *)&dp->scp->pos)) {
        dp->path.timer = 0;
        dp->edb.rot = dp->edb.arot = 0.0f;
    }
    enMoveAngle(&dp->path, 0.05235988f);
    shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, 1000.0f);
    enEDBAutoRecovery(dp);
}

static void enEDBCtrlShoot(struct EnLOCAL_DATA *dp) {
    int t;
    float d;

    if (enCheckDamage(dp)) {
        if (enEDBSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        if (!dp->edb.bullet) {
            enEDBAnimeSet(dp, 9);
            dp->sslv = 5;
            break;
        }
        if (!enCheckIntoScreen(dp)) {
            enEDBAnimeSet(dp, 1);
            enSetTimer(dp, 150);
            dp->sslv = 6;
            break;
        }
        if (enCalcAngleDifference(dp->scp->rot.y, enGetPlayerDirection(dp)) > 1.5707964f && dp->type == 1) {
            EN_SET_LEVEL(dp, 4);
            break;
        }
        dp->sslv++;
    case 1:
        d = shAngleRegulate(enGetPlayerDirection(dp) - dp->scp->rot.y);
        if (d < -1.5707964f) {
            enEDBAnimeSet(dp, 5);
            dp->sslv = 7;
            break;
        }
        if (d > 1.5707964f) {
            enEDBAnimeSet(dp, 4);
            dp->sslv = 7;
            break;
        }
        enEDBAddAfford(dp);
        enEDBAnimeSet(dp, 6);
        enInitPath(&dp->path, dp->scp->rot.y);
        enFlagSetCritical(dp);
        dp->sslv++;
        break;
    case 2:
        enEDBGetHoldSpeed(dp);
        enMoveAngleToPlayer(dp, enEDBGetAimingSpeed(dp));
        if (enEDBCanSeePlayer(dp, dp->scp->rot.y) != 2 && dp->type) {
            enFlagResetCritical(dp);
            if (dp->edb.pcount <= 2) {
                dp->slv = 6;
                dp->sslv = 0;
            } else {
                dp->slv = 4;
                dp->sslv = 0;
            }
            return;
        }
        if (dp->anim_n == -1) {
            enEDBAnimeSet(dp, 7);
            enAttackStart(dp);
            dp->edb.pcount = 0;
            dp->edb.bullet--;
            dp->sslv++;
        }
        break;
    case 3:
        enEDBGetSpeed(dp);
        enAttackCheck(dp, 0x34);
        if (dp->anim_n == -1) {
            enEDBAddAfford(dp);
            enFlagResetCritical(dp);
            if (!dp->type) {
                EN_SET_LEVEL(dp, 0);
                break;
            }
            if (dp->edb.afford > 0 && dp->edb.bullet > 0 && enCheckIntoScreen(dp) &&
                enEDBCanSeePlayer(dp, dp->scp->rot.y) >= 2 && !(enCheckPlayerCondition(dp) & 1)) {
                enEDBAnimeSet(dp, 6);
                enFlagSetCritical(dp);
                enSetTimer(dp, 300);
                dp->sslv++;
                break;
            }
            if (dp->scp->battle.hp_rate < 20.0f) {
                EN_SET_LEVEL(dp, 2);
                break;
            }
            EN_SET_LEVEL(dp, 4);
            break;
        }
        break;
    case 4:
        enEDBGetHoldSpeed(dp);
        enMoveAngleToPlayer(dp, enEDBGetAimingSpeed(dp));
        enReduceTimer(dp);
        t = enEDBCanSeePlayer(dp, dp->scp->rot.y);
        if (dp->anim_n == -1) {
            if (dp->timer <= 0 || (enCheckPlayerCondition(dp) & 2) || t == 3) {
                if (!enCheckIntoScreen(dp)) {
                    EN_SET_LEVEL(dp, 6);
                    break;
                }
                enEDBAnimeSet(dp, 7);
                enAttackStart(dp);
                dp->edb.pcount = 0;
                dp->edb.bullet--;
                dp->sslv = 3;
            }
        } else {
            if (!t) {
                if (dp->scp->battle.hp_rate < 20.0f) {
                    dp->slv = 2;
                    dp->sslv = 0;
                } else {
                    EN_SET_LEVEL(dp, 4);
                }
                enFlagResetCritical(dp);
            } else if (t == 3) {
                enFlagResetCritical(dp);
                EN_SET_LEVEL(dp, 6);
            }
        }
        break;
    case 5:
        enEDBGetSpeed(dp);
        if (dp->anim_n == -1) {
            dp->sslv = 1;
        }
        break;
    case 6:
        enMoveAngleToPlayer(dp, 0.05235988f);
        switch (enEDBCanSeePlayer(dp, dp->scp->rot.y)) {
        case 2:
            dp->sslv = 1;
            break;
        case 3:
            EN_SET_LEVEL(dp, 6);
            break;
        default:
            if (enCheckIntoScreen(dp)) {
                if (!dp->type) {
                    dp->sslv = 1;
                } else {
                    EN_SET_LEVEL(dp, 4);
                }
            } else if (enReduceTimer(dp) <= 0) {
                dp->sslv = 1;
            }
            break;
        }
        break;
    case 7:
        enEDBGetSpeed(dp);
        if (dp->anim_n == -1) {
            dp->sslv = 1;
        }
        break;
    }
    enEDBAutoRecovery(dp);
}

static void enEDBCtrlPunch(struct EnLOCAL_DATA *dp) {
    int t;
    float d1;
    float d2;
    float vec[4];

    if (enCheckDamage(dp)) {
        if (enEDBSetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (dp->sslv == 6 && dp->p_dist > 750.0f && enGetPlayerWeapon()) {
            EN_SET_LEVEL(dp, 5);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        if (dp->edb.pcount >= enEDBGetPunchLimit()) {
            EN_SET_LEVEL(dp, 5);
            return;
        }
        t = enEDBCanSeePlayer(dp, dp->scp->rot.y);
        if (t == 3) {
            enEDBAnimeSet(dp, 8);
            enAttackStart(dp);
            dp->sslv = 3;
            break;
        }
        enInitPath(&dp->path, dp->scp->rot.y);
        enSetTimer(dp, 90);
        if (enGetPlayerWeapon() != 8) {
            enEDBAnimeSet(dp, 3);
            dp->sslv++;
            break;
        }
        enEDBAnimeSet(dp, 2);
        dp->sslv = 2;
        break;
    case 1:
    case 2:
        if (enEDBCanSeePlayer(dp, dp->scp->rot.y) == 3) {
            if (enGetPlayerWeapon() == 8 && shRandF() > 0.2f * enGetMode()) {
                EN_SET_LEVEL(dp, 1);
                break;
            }
            enEDBAnimeSet(dp, 8);
            enAttackStart(dp);
            enFlagSetCritical(dp);
            dp->sslv = 3;
        } else {
            enSetPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos);
            enMoveAngle(&dp->path, 0.05235988f);
            if (dp->sslv == 1) {
                shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enEDBGetRunSpeed(dp));
            } else {
                shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enEDBGetWalkSpeed(dp));
            }
        }
        if (enCalcAngleDifference(dp->scp->rot.y, enGetPlayerAngle(dp)) < 0.5235988f && dp->p_dist < 1000.0f) {
            if (!dp->type) {
                dp->slv = 0;
                dp->sslv = 0;
            } else {
                if (dp->scp->battle.hp_rate < 20.0f) {
                    EN_SET_LEVEL(dp, 2);
                }
            }
        }
        if (!dp->type && enReduceTimer(dp) <= 0) {
            EN_SET_LEVEL(dp, 1);
        }
        break;
    case 3:
        enEDBGetSpeed(dp);
        enMoveAngleToPlayer(dp, 0.05235988f);
        enAttackCheck(dp, 0x35);
        if (dp->anim_n == -1) {
            dp->edb.pcount++;
            enEDBAddAfford(dp);
            enFlagResetCritical(dp);
            if (!dp->edb.bullet) {
                enEDBAnimeSet(dp, 9);
                enFlagSetCritical(dp);
                dp->sslv = 4;
                break;
            }
            if (!dp->type) {
                EN_SET_LEVEL(dp, 1);
                break;
            }
            if (dp->edb.afford <= 0) {
                EN_SET_LEVEL(dp, 3);
                break;
            }
            dp->sslv = 5;
        }
        break;
    case 4:
        enEDBGetSpeed(dp);
        if (dp->anim_n == -1) {
            enFlagResetCritical(dp);
            if (!dp->type) {
                EN_SET_LEVEL(dp, 1);
                break;
            }
            if (dp->edb.afford <= 0) {
                EN_SET_LEVEL(dp, 3);
                break;
            }
            dp->sslv = 5;
            if (enEDBCanSeePlayer(dp, dp->scp->rot.y) >= 2) {
                EN_SET_LEVEL(dp, 5);
            }
        }
        break;
    case 5:
        enEDBGetSpeed(dp);
        shSinCosV_Scale(vec, dp->scp->rot.y + 0.87266463f, 1000.0f);
        _shAddVector(vec, (float *)&dp->scp->pos, vec);
        d1 = enCheckPath(dp, vec, (float *)&dp->scp->pos);
        shSinCosV_Scale(vec, dp->scp->rot.y - 0.87266463f, 1000.0f);
        d2 = enCheckPath(dp, vec, (float *)&dp->scp->pos);
        if (d1 < 0.0f && d2 < 0.0f) {
            if (shAngleRegulate(enGetPlayerDirection(dp) - dp->scp->rot.y) < 0.0f) {
                enEDBAnimeSet(dp, 4);
            } else {
                enEDBAnimeSet(dp, 5);
            }
        } else if (d1 < 0.0f) {
            enEDBAnimeSet(dp, 4);
        } else if (d2 < 0.0f || d2 > d1) {
            enEDBAnimeSet(dp, 5);
        } else {
            enEDBAnimeSet(dp, 4);
        }
        dp->sslv++;
        break;
    case 6:
        enEDBGetSpeed(dp);
        if (dp->anim_n == -1) {
            enInitPath(&dp->path, dp->scp->rot.y);
            enEDBAnimeSet(dp, 3);
            enSetTimer(dp, 300);
            dp->sslv++;
        }
        break;
    case 7:
        shSinCosV_Scale(vec, dp->scp->rot.y, 1000.0f);
        _shAddVector(vec, (float *)&dp->scp->pos, vec);
        enSetPath(dp, vec, (float *)&dp->scp->pos);
        enMoveAngle(&dp->path, 0.05235988f);
        shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enEDBGetRunSpeed(dp));
        if (dp->scp->battle.hp_rate < 20.0f) {
            if (dp->p_dist > 1500.0f) {
                EN_SET_LEVEL(dp, 2);
            } else if (enReduceTimer(dp) <= 0) {
                EN_SET_LEVEL(dp, 3);
            }
        } else {
            EN_SET_LEVEL(dp, 5);
        }
        break;
    }
    enEDBAutoRecovery(dp);
}

static void enEDBCtrlDamage(struct EnLOCAL_DATA *dp) {
    int w;

    switch (dp->sslv) {
    case 0:
        if (!dp->type && enCheckSpray(dp) && enGetSprayPower() >= 2) {
            EN_SET_LEVEL(dp, 8);
            break;
        }
        enEDBAnimeReset(dp, enEDBGetDamageMotion(dp));
        if (dp->endurance <= 0.0f) {
            dp->endurance = dp->endurance_max;
        }
        enFlagResetCritical(dp);
        dp->edb.speed = 1.0f;
        dp->sslv++;
        break;
    case 1:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
        }
        if (dp->type == 1 && enCheckDeath(dp)) {
            enSoundStop(0x471B);
            enSoundStop(0x471C);
            enSoundStop(0x471D);
            enSoundStop(0x471E);
            enSoundCall(0x471F, 1.0f, (float *)&dp->scp->pos);
            enKillCountUp(dp);
            EN_SET_LEVEL(dp, 9);
            break;
        }
        if (dp->anim_n == -1) {
            if (!dp->type) {
                if (enCheckDeath(dp)) {
                    EN_SET_LEVEL(dp, 8);
                    break;
                }
                w = enGetPlayerWeapon();
                if (w == 2 && enGetMode() > 1) {
                    EN_SET_LEVEL(dp, 5);
                    break;
                }
                if (w == 6 && enGetMode() > 1) {
                    EN_SET_LEVEL(dp, 6);
                    break;
                }
                EN_SET_LEVEL(dp, 1);
                break;
            }
            if (dp->edb.afford <= 0) {
                EN_SET_LEVEL(dp, 2);
                break;
            }
            if (dp->p_dist < 1500.0f) {
                EN_SET_LEVEL(dp, 6);
                break;
            }
            EN_SET_LEVEL(dp, 5);
        }
        break;
    }
}

static void enEDBCtrlEscape2(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            enReduceHP(dp);
        } else {
            EN_SET_LEVEL(dp, 7);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        enEDBAnimeSet(dp, 3);
        enInitPath(&dp->path, dp->scp->rot.y);
        enSetTimer(dp, 60);
        dp->sslv++;
        break;
    case 1:
        if (enReduceTimer(dp) <= 0) {
            int t;

            t = enEDBCanSeePlayer(dp, dp->scp->rot.y);
            if (t == 3) {
                enEDBAnimeSet(dp, 8);
                enAttackStart(dp);
                dp->sslv = 2;
                break;
            }
        }
        dp->path.markangle = enCalcDirection(door_pos, (float *)&dp->scp->pos);
        if (enDistXZ(door_pos, (float *)&dp->scp->pos) < 500.0f &&
            fabsf(enCalcAngleDifference(dp->scp->rot.y, dp->path.markangle)) < 1.5707964f) {
            game_flag.flag[11] |= 0x200000;
        }
        enMoveAngle(&dp->path, 0.05235988f);
        shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, 1400.0f);
        break;
    case 2:
        enEDBGetSpeed(dp);
        enMoveAngleToPlayer(dp, 0.05235988f);
        enAttackCheck(dp, 0x35);
        if (dp->anim_n == -1) {
            t = enEDBCanSeePlayer(dp, dp->scp->rot.y);
            if (t == 3) {
                dp->sslv = 0;
                break;
            }
            if (t == 2) {
                if (dp->edb.bullet) {
                    enEDBAnimeSet(dp, 6);
                    dp->sslv = 4;
                    break;
                }
                enEDBAnimeSet(dp, 9);
                dp->sslv = 3;
                break;
            }
            enEDBAnimeSet(dp, 3);
            dp->sslv = 1;
        }
        break;
    case 3:
        if (dp->anim_n == -1) {
            enEDBAnimeSet(dp, 3);
            dp->sslv = 1;
        }
        break;
    case 4:
        enEDBGetHoldSpeed(dp);
        enMoveAngleToPlayer(dp, enEDBGetAimingSpeed(dp));
        if (dp->anim_n == -1) {
            enSetTimer(dp, 60);
            dp->sslv++;
        }
        break;
    case 5:
        if (enCheckPath(dp, door_pos, (float *)&dp->scp->pos) < 0.0f) {
            enEDBAnimeSet(dp, 3);
            dp->sslv = 1;
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            enEDBAnimeSet(dp, 7);
            enAttackStart(dp);
            dp->edb.bullet--;
            dp->sslv++;
        }
        break;
    case 6:
        enEDBGetSpeed(dp);
        enAttackCheck(dp, 0x34);
        if (dp->anim_n == -1) {
            enEDBAnimeSet(dp, 3);
            dp->sslv = 1;
        }
        break;
    }
    enEDBAutoRecovery(dp);
}

static void enEDBCtrlDead(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        dp->anim_s = 0x200;
        enSetTimer(dp, 90);
        enSetBlur();
        dp->flag |= 8;
        dp->sslv++;
    }
    if (enReduceTimer(dp) <= 0) {
        enResetFilter();
        game_flag.flag[11] |= 0x400000;
    }
}

static int enEDBCheckArriveCorner(struct EnLOCAL_DATA *dp) {
    int i;
    float d;

    for (i = 0; i < ARRAY_COUNT(room_corner); i++) {
        d = enDistXZ(room_corner[i], (float *)&dp->scp->pos);
        if (d < 150.0f) { return i; }
    }
    return -1;
}

static int enEDBGetNearMeat(struct EnLOCAL_DATA *dp) {
    int i;
    int j;
    int m;
    float a;
    float d;
    float dm;
    float *ppos;
    float pos[4];
    float tpos[4];
    float vec[4];

    ppos = enGetPlayerPos(dp);
    dm = 3.4028235e38f;
    m = -1;
    for (i = 0; i < 11; i++) {
        j = i + 6;
        if (enEDBCheckMark(dp, j)) {
            continue;
        }
        vcopy(&nik_pos_data[i].trans, pos);
        a = enCalcDirection(pos, ppos);
        shSinCosV_Scale(vec, a, 500.0f);
        _shAddVector(pos, pos, vec);
        d = enCheckPath(dp, pos, (float *)&dp->scp->pos);
        if (d < 0.0f) {
            if (enDistXZ(pos, ppos) >= 2000.0f) {
                if (-d < dm) {
                    dm = -d;
                    m = j;
                    vcopy(pos, tpos);
                }
            }
        }
    }
    if (m != -1) {
        vcopy(tpos, dp->edb.target);
        enEDBSetMark(dp, m);
    }
    return m;
}

static void enEDBClearMark(struct EnLOCAL_DATA *dp) {
    int i;

    for (i = 0; i < 3; i++) {
        dp->edb.mark[i] = -1;
    }
    dp->edb.mark_n = 0;
}

static void enEDBSetMark(struct EnLOCAL_DATA *dp, int mark) {
    if (++dp->edb.mark_n >= 3) {
        dp->edb.mark_n = 0;
    }
    dp->edb.mark[dp->edb.mark_n] = mark;
}

static int enEDBCheckMark(struct EnLOCAL_DATA *dp, int mark) {
    int i;

    for (i = 0; i < 3; i++) {
        if (dp->edb.mark[i] == mark) {
            return 1;
        }
    }
    return 0;
}

static int enEDBGetDamageMotion(struct EnLOCAL_DATA *dp) {
    int m;
    int id;
    int dd;
    float a;

    a = shAngleRegulate(shAtanV(dp->scp->battle.vec) - dp->scp->rot.y);
    if (fabsf(a) > 2.0943952f) {
        dd = 0;
    } else {
        dd = 1;
    }
    if (dp->scp->battle.damage <= 120.0f && dp->scp->battle.hp > 0.0f) {
        dd += 2;
    }
    id = dp->last_atk;
    switch (id) {
    case 1:
    case 2:
    case 4:
    case 6:
        m = dd + 10;
        break;
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 23:
        m = dd + 14;
        break;
    case 19:
    case 20:
    case 21:
    case 22:
        m = (dd & 1) + 14;
        break;
    case 18:
    case 24:
        m = dd + 18;
        dp->scp->battle.shock = 0.0f;
        dp->hb_s = 0.0f;
        break;
    default:
        m = 10;
        printf("Illegal damage type!(%d)\n", id);
        break;
    }
    return m;
}

static int enEDBSetDamage(struct EnLOCAL_DATA *dp) {
    if (enCheckSpray(dp)) {
        enReduceHP(dp);
        dp->edb.speed = 0.5f;
        if (!dp->type && enGetSprayPower() >= 2) {
            return 1;
        }
        return 0;
    }
    dp->edb.afford--;
    enSetHitBack(dp);
    if (dp->last_atk >= 19 && dp->last_atk <= 22) {
        dp->scp->battle.damage *= 2.0f;
    } else if (dp->last_atk == 23) {
        dp->scp->battle.damage *= 5.0f;
    } else if (enCheckCritical(dp) && (dp->last_atk <= 0 || dp->last_atk > 6)) {
        dp->scp->battle.damage *= 2.0f;
    }
    if (enReduceHP(dp) <= 0.0f || !dp->type) {
        return 1;
    }
    if (dp->anim == 9) {
        return 1;
    }
    switch (dp->last_atk) {
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
        return 1;
    }
    dp->edb.speed = 0.5f;
    return 0;
}

static int enEDBCanSeePlayer(struct EnLOCAL_DATA *dp, float angle) {
    float *ppos;
    float dist;
    float a;
    float vec[4];

    ppos = enGetPlayerPos(dp);
    dist = enCheckPath2(dp, ppos, (float *)&dp->scp->pos);
    if (dist >= 0.0f) {
        shSinCosV_Scale(vec, enCalcDirection(ppos, (float *)&dp->scp->pos), dist);
        _shAddVector(vec, (float *)&dp->scp->pos, vec);
        return 0;
    }
    dist = enDistXZ(ppos, (float *)&dp->scp->pos);
    if (dist > 5000.0f) {
        return 0;
    }
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a = enCalcAngleDifference(a, angle);
    if (dist < 550.0f && a < 0.5235988f) {
        return 3;
    }
    if (dist < 4000.0f && a < 0.5235988f && enCheckIntoScreen(dp)) {
        return 2;
    }
    if (a > 1.3962634f) {
        return 0;
    }
    return 1;
}

static void enEDBAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        enAnimeRestart(dp);
        return;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 1673
    fjAssert(anim >= 0 && anim < sizeof(EnEDBAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnEDBAnime[anim].Anime);
}

static void enEDBAnimeReset(struct EnLOCAL_DATA *dp, int anim) {
    /* Matching: the assert bakes its original line number into the object. */
#line 1681
    fjAssert(anim >= 0 && anim < sizeof(EnEDBAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnEDBAnime[anim].Anime);
}

static void enEDBAnimeExec(struct EnLOCAL_DATA *dp) {
    int of;
    int d;

    of = dp->anim_n;
    enAnimeExec(dp, EnEDBAnime, 0x170D);
    if (dp->anim_n == -1 || (dp->flag & 1)) {
        return;
    }
    d = -1;
    switch (dp->anim) {
    case 4:
        d = 1;
    case 5:
        if (dp->anim_n >= 0x41A0) {
            dp->scp->rot.y = shAngleRegulate(dp->scp->rot.y + d * (4.1887903f * shGetDT()));
        }
        if (dp->anim_n >= 0x9600) {
            enSetTrans(dp);
            if (!dp->anim_step) {
                dp->tx2 = dp->tx;
                dp->tz2 = dp->tz;
                dp->anim_step = 1;
            }
        }
        break;
    case 19:
        d++;
    case 18:
        d++;
    case 15:
        d++;
    case 14:
        d++;
    case 11:
        d++;
    case 10: {
        struct EDB_ANM_DATA1 edb_anm_data1[6] = {
            { 3, 6, -2250.0f, 3, -1000.0f }, { 3, 6, 1750.0f, 9, 1250.0f },  { 3, 6, -4000.0f, 3, -1250.0f },
            { 2, 5, 2250.0f, 9, 1000.0f },   { 2, 6, -2250.0f, 9, -1000.0f }, { 3, 6, 2250.0f, 11, 1250.0f },
        };

        d++;
        if (dp->anim_n >= edb_anm_data1[d].start1 * 2400 && dp->anim_n <= edb_anm_data1[d].end1 * 2400) {
            enSetTransForward(dp, edb_anm_data1[d].rate1);
        } else if (dp->anim_n <= edb_anm_data1[d].end2 * 2400) {
            enSetTransForward(dp, edb_anm_data1[d].rate2);
        }
        break;
    }
    case 21:
        d++;
    case 20:
        d++;
    case 17:
        d++;
    case 16:
        d++;
    case 13:
        d++;
    case 12: {
        struct EDB_ANM_DATA2 edb_anm_data2[6] = {
            { 7, -750.0f }, { 7, 750.0f }, { 7, -1250.0f }, { 7, 1250.0f }, { 7, -1250.0f }, { 7, 750.0f },
        };

        d++;
        if (dp->anim_n <= edb_anm_data2[d].end * 2400) {
            enSetTransForward(dp, edb_anm_data2[d].rate);
        }
        break;
    }
    case 9:
        if ((of < 0xC4E0 && dp->anim_n >= 0xC4E0) || (of < 0xFD20 && dp->anim_n >= 0xFD20) ||
            (of < 0x13560 && dp->anim_n >= 0x13560) || (of < 0x16DA0 && dp->anim_n >= 0x16DA0) ||
            (of < 0x1A5E0 && dp->anim_n >= 0x1A5E0) || (of < 0x1DE20 && dp->anim_n >= 0x1DE20)) {
            if (dp->edb.bullet <= 5) {
                dp->edb.bullet++;
                enSoundCall(0x471A, 0.5f, (float *)&dp->scp->pos);
            }
        }
        if (of < 0x4B00 && dp->anim_n >= 0x4B00) {
            enEDBSetCartridge(dp);
        }
        break;
    case 2:
        if ((dp->anim_s > 0 && ((of < 0x41A0 && dp->anim_n >= 0x41A0) || (of < 0xB220 && dp->anim_n >= 0xB220))) ||
            (dp->anim_s < 0 && ((of > 0x2EE0 && dp->anim_n <= 0x2EE0) || (of > 0x9F60 && dp->anim_n <= 0x9F60)))) {
            if (dp->flag & 0x1000) {
                enSoundCall3D(0x4718, 1.0f, (float *)&dp->scp->pos);
            } else {
                enSoundCall(0x4718, 1.0f, (float *)&dp->scp->pos);
            }
        }
        break;
    case 3:
        if ((dp->anim_s > 0 && ((of < 0x4B00 && dp->anim_n >= 0x4B00) || (of < 0xBB80 && dp->anim_n >= 0xBB80))) ||
            (dp->anim_s < 0 && ((of > 0x1C20 && dp->anim_n <= 0x1C20) || (of > 0x8CA0 && dp->anim_n <= 0x8CA0)))) {
            if (dp->flag & 0x1000) {
                enSoundCall3D(0x4718, 1.0f, (float *)&dp->scp->pos);
            } else {
                enSoundCall(0x4718, 1.0f, (float *)&dp->scp->pos);
            }
        }
        break;
    }
}

/** Starts the muzzle-flash effect at Eddie's gun, between skeleton points 30 and 31.
 * @param dp enemy work */
void enEDBSetGunFire(struct EnLOCAL_DATA *dp) {
    float vec1[4];
    float vec2[4];

    enGetSkeletonVector(vec1, dp, 30);
    enGetSkeletonVector(vec2, dp, 31);
    _shAddVector(vec1, vec1, vec2);
    _shScaleVector(vec1, vec1, 0.5f);
    vec1[2] += 225.0f;
    vec1[1] -= 75.0f;
    shRotVectorY(vec1, vec1, dp->scp->rot.y);
    vcopy(vec1, vec2);
    _shAddVectorXYZ(vec1, (float *)&dp->scp->pos, vec1);
    vec2[1] = 0.0f;
    _shNormalize(vec2, vec2);
    EFCTSetGunFireEddie(vec1, vec2);
}

static void enEDBSetCartridge(struct EnLOCAL_DATA *dp) {
    float vec[4];

    enGetSkeletonVector(vec, dp, 31);
    vec[2] += 60.0f;
    vec[1] -= 40.0f;
    shRotVectorY(vec, vec, dp->scp->rot.y);
    _shAddVectorXYZ(vec, (float *)&dp->scp->pos, vec);
    EFCTSetDischargeCartridgeEddie(vec);
}

static void enEDBAutoRecovery(struct EnLOCAL_DATA *dp) {
    short recover_rate[5] = { 10, 50, 70, 100, 150 };

    enAddEnduranceDT(dp, itof(recover_rate[enGetMode()]));
}

static float enEDBGetSpeed(struct EnLOCAL_DATA *dp) {
    dp->anim_s = ftoi(4096.0f * dp->edb.speed);
    return dp->edb.speed;
}

static float enEDBGetWalkSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.7f, 0.8f, 1.0f, 1.2f, 1.4f };
    float r;

    r = dp->edb.speed * speed_rate[enGetMode()];
    dp->anim_s = ftoi(4096.0f * r);
    return 1000.0f * r;
}

static float enEDBGetRunSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.7f, 0.8f, 1.0f, 1.2f, 1.4f };
    float r;

    r = dp->edb.speed * speed_rate[enGetMode()];
    dp->anim_s = ftoi(4096.0f * r);
    return 1400.0f * r;
}

static float enEDBGetHoldSpeed(struct EnLOCAL_DATA *dp) {
    float r;

    if (!dp->type) {
        r = 1.0f;
    } else {
        float speed_rate[5] = { 0.5f, 0.6f, 0.7f, 1.0f, 1.2f };

        r = speed_rate[enGetMode()];
        if (enGetPlayerWeapon() == 2 && r < 1.0f) {
            r = 1.0f;
        }
        r *= dp->edb.speed;
    }
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

/* Matching: K&R definition, see the declaration above. */
static float enEDBGetAimingSpeed() {
    float speed_rate[5] = { 0.6f, 0.7f, 0.9f, 1.2f, 1.5f };

    return 0.05235988f * speed_rate[enGetMode()];
}

static int enEDBGetDefAfford(void) {
    short afford[5] = { 7, 10, 15, 20, 30 };

    return afford[enGetMode()];
}

static int enEDBGetPunchLimit(void) {
    signed char limit[5] = { 2, 3, 3, 4, 5 };

    return limit[enGetMode()];
}

static void enEDBAddAfford(struct EnLOCAL_DATA *dp) {
    int max;

    max = enEDBGetDefAfford();
    if ((dp->edb.afford += 3) > max) {
        dp->edb.afford = max;
    }
}
/* Matching: fitted stand-in (not recovered code); it sets the next function's float-constant
 * order. Whether the original had code here: docs/stand-ins.md. */
static float __stripped_float_code_5(float x) { return x + 3.0f + 5.0f; }
/** Sets up a NIK: no HP, size and weight; it is flagged dead from the start (not a real enemy).
 * @param dp enemy work */
void enNIKInitData(struct EnLOCAL_DATA *dp) {
    enSetHP(dp, 0.0f, 0.0f);
    enSetSize(dp, 200.0f, 2000.0f, 1250.0f, 1250.0f);
    dp->weight = 4;
    enFlagSetRotFloor(dp);
    dp->flag = 0x40;
    enFlagSetDead(dp);
}

/** Per-frame control of a NIK: a hit pushes it, a damped spring swings it back, and its x/z
 * rotation follows the swing.
 * @param dp enemy work */
void enNIKCtrlMain(struct EnLOCAL_DATA *dp) {
    float vec[4];

    if (enCheckDamage(dp)) {
        _shNormalize(vec, dp->scp->battle.vec);
        _shScaleVector(vec, vec, dp->scp->battle.shock);
        vec[1] = 0.0f;
        _shAddVector(dp->nik.acc, dp->nik.acc, vec);
        enResetDamage(dp);
    }
    _shScaleVector(vec, dp->nik.swing, 2.0f * shGetDT());
    _shSubVector(dp->nik.acc, dp->nik.acc, vec);
    _shScaleVector(dp->nik.acc, dp->nik.acc, 1.0f - 3.0f * shGetDT());
    _shAddVector(dp->nik.swing, dp->nik.swing, dp->nik.acc);
    shRotVectorY(vec, dp->nik.swing, -dp->scp->rot.y);
    dp->scp->rot.x = dp->trx = shAtan2(2000.0f, vec[2]);
    dp->scp->rot.z = dp->trz = shAtan2(2000.0f, -vec[0]);
    _shSubVector(dp->vec, (float *)&nik_pos_data[dp->scp->id].trans, (float *)&dp->scp->pos);
    enMoveExec(dp);
    vwGetViewPosition(vec);
    if (enDistXZ(vec, (float *)&dp->scp->pos) < 300.0f) {
        dp->scp->status &= ~0x10;
    } else {
        dp->scp->status |= 0x10;
    }
}
