/*
 * en_pap.c: AI of the PAP enemy: sleeps, wanders, walks straight, chases, rushes and attacks,
 * takes damage, goes down or confused, with a last move before dying. Shared enemy code is in
 * en_common.c.
 * Abstract Daddy (verified; docs/characters.md).
 */
#include "enemy.h"
#include "sdk/libvu0.h"

static void enPAPCtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enPAPCtrlSleep(struct EnLOCAL_DATA *dp);
static void enPAPCtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enPAPCtrlEvent(struct EnLOCAL_DATA *dp);
static void enPAPCtrlHand(struct EnLOCAL_DATA *dp);
static void enPAPCtrlWander(struct EnLOCAL_DATA *dp);
static void enPAPCtrlStraight(struct EnLOCAL_DATA *dp);
static void enPAPCtrlChase(struct EnLOCAL_DATA *dp);
static void enPAPCtrlRush(struct EnLOCAL_DATA *dp);
static void enPAPCtrlAttack(struct EnLOCAL_DATA *dp);
static void enPAPCtrlDamage(struct EnLOCAL_DATA *dp);
static void enPAPCtrlConfuse(struct EnLOCAL_DATA *dp);
static void enPAPCtrlDown(struct EnLOCAL_DATA *dp);
static void enPAPCtrlLastMove(struct EnLOCAL_DATA *dp);
static void enPAPMoveAngle(struct EnLOCAL_DATA *dp);
static int enPAPCanSeePlayer(struct EnLOCAL_DATA *dp);
static void enPAPAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enPAPAnimeExec(struct EnLOCAL_DATA *dp);
static void enPAPAutoRecovery(struct EnLOCAL_DATA *dp);
static float enPAPGetWalkSpeed(struct EnLOCAL_DATA *dp);
static float enPAPGetRotSpeed(void);
static float enPAPGetTurnSpeed(struct EnLOCAL_DATA *dp);
static void enPAPSetDownTime(struct EnLOCAL_DATA *dp);
static void enPAPSetMoveCount(struct EnLOCAL_DATA *dp);
static void enPAPSetDc(struct EnLOCAL_DATA *dp);
static void enPAPDrawBoad(struct EnLOCAL_DATA *dp);
static void enPAPSoundSigns(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnPAPAnime[33] = {
    { 0x1645, 0 }, { 0x1646, 1 }, { 0x1645, 1 }, { 0x1648, 1 }, { 0x165F, 1 }, { 0x164A, 0 }, { 0x164B, 0 },
    { 0x164C, 0 }, { 0x164D, 0 }, { 0x164E, 0 }, { 0x164F, 0 }, { 0x1650, 0 }, { 0x1651, 0 }, { 0x164D, 0 },
    { 0x1652, 0 }, { 0x1653, 0 }, { 0x1654, 0 }, { 0x1655, 0 }, { 0x1656, 0 }, { 0x1657, 0 }, { 0x1658, 0 },
    { 0x1659, 0 }, { 0x1645, 0 }, { 0x165A, 0 }, { 0x1645, 1 }, { 0x165B, 1 }, { 0x1645, 0 }, { 0x165D, 0 },
    { 0x1647, 0 }, { 0x1649, 0 }, { 0x165C, 0 }, { 0x165E, 0 }, { 0x1660, 0 },
};

/* Matching: enPAPInitData: the stand-in before it sets its float-constant argument order. */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f; }
/** Sets up a new PAP: HP by difficulty mode, size, starting level and type from the spawn status.
 * @param dp enemy work */
void enPAPInitData(struct EnLOCAL_DATA *dp) {
    float vitarity[5] = { 500.0f, 750.0f, 1000.0f, 2000.0f, 4000.0f };
    float endurance[5] = { 150.0f, 285.0f, 380.0f, 500.0f, 600.0f };
    float vitarity2[5] = { 1000.0f, 4000.0f, 5000.0f, 7000.0f, 10000.0f };
    float endurance2[5] = { 300.0f, 400.0f, 600.0f, 800.0f, 1000.0f };
    int mode;

    mode = enGetMode();
    dp->mlv = 1;
    switch (dp->scp->en_first_status) {
    case 6:
        EN_SET_LEVEL(dp, 2);
        enSetHP(dp, vitarity2[mode], endurance2[mode]);
        dp->type = 1;
        break;
    case 13:
        dp->type = 1;
        dp->flag = 0x8000;
        enPAPAnimeSet(dp, 25);
        enSetSize(dp, 300.0f, 250.0f, 125.0f, 125.0f);
        dp->weight = 4;
        enFlagSetNoDamage(dp);
        dp->mlv = 4;
        return;
    case 5:
        enPAPAnimeSet(dp, 27);
        EN_SET_LEVEL(dp, 9);
        return;
    default:
        EN_SET_LEVEL(dp, 1);
        enSetHP(dp, vitarity[mode], endurance[mode]);
        dp->type = 0;
        break;
    }
    enSetSize(dp, 400.0f, 650.0f, 600.0f, 650.0f);
    dp->weight = 3;
    enPAPAnimeSet(dp, 2);
    enSetBattleTarget(dp, 1);
    if (enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

/** Per-frame control of a PAP: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enPAPCtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlPAPFunc[6])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enPAPCtrlAutomatic, enPAPCtrlSleep, enPAPCtrlGoPlayable, enPAPCtrlEvent, enPAPCtrlHand,
    };

    enCtrlPAPFunc[dp->mlv](dp);
}

static void enPAPCtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlPAPSubFunc[10])(struct EnLOCAL_DATA *) = {
        enPAPCtrlWander,  enPAPCtrlStraight, enPAPCtrlChase, enPAPCtrlRush,     enPAPCtrlAttack,
        enPAPCtrlDamage,  enPAPCtrlConfuse,  enPAPCtrlDown,  enPAPCtrlLastMove, enDyingExec,
    };

    enSetBattleTarget(dp, 1);
    enCtrlPAPSubFunc[dp->slv](dp);
    enPAPAnimeExec(dp);
    enMoveExec(dp);
    if (dp->type != 1 && dp->slv != 9 && enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

static void enPAPCtrlSleep(struct EnLOCAL_DATA *dp) {
    if (enCheckSleepOut(dp)) {
        enSleepOut(dp);
        dp->slv = 1;
        dp->sslv = 0;
        enFlagResetLieDown(dp);
    }
}

static void enPAPCtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    EN_SET_LEVEL(dp, 0);
}

static void enPAPCtrlEvent(struct EnLOCAL_DATA *dp) {
    if (++dp->sslv >= 10) {
        enAnimePause(dp);
        dp->mlv = 1;
        /* Matching: one line in the original; EN_SET_LEVEL (not the two stores) sets
         * enPAPCtrlWander's float-constant order. */
        EN_SET_LEVEL(dp, 9);
    }
}

static void enPAPCtrlHand(struct EnLOCAL_DATA *dp) {
}

static void enPAPCtrlWander(struct EnLOCAL_DATA *dp) {
    float vec[4];
    float a;
    int t;

    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            enReduceHP(dp);
            if (enGetSprayPower()) {
                EN_SET_LEVEL(dp, 6);
                return;
            }
        } else {
            EN_SET_LEVEL(dp, 5);
            return;
        }
    }
    t = enPAPCanSeePlayer(dp);
    if (t == 2) {
        EN_SET_LEVEL(dp, 4);
        return;
    }
    if (t == 1 || dp->p_dist < 1000.0f) {
        EN_SET_LEVEL(dp, 2);
        return;
    }
    switch (dp->sslv) {
    case 0:
        enPAPAnimeSet(dp, 1);
        enPAPGetWalkSpeed(dp);
        enInitPath(&dp->path, dp->scp->rot.y);
        shSinCosV_Scale(vec, dp->scp->rot.y, 1500.0f);
        _shAddVector(dp->pap.target, (float *)&dp->scp->pos, vec);
        dp->sslv++;
        break;
    case 1:
        if (shRandF() < 0.01f) {
            enPAPAnimeSet(dp, 2);
            dp->sslv = 2;
            break;
        }
        if (enDistXZ(dp->pap.target, (float *)&dp->scp->pos) < 750.0f || shRandF() < 0.01f) {
            a = dp->scp->rot.y + PI * (shRandF() - 0.5f) / 2.0f;
            shSinCosV_Scale(vec, a, 1500.0f);
            _shAddVector(dp->pap.target, (float *)&dp->scp->pos, vec);
        }
        if (enSetPath(dp, dp->pap.target, (float *)&dp->scp->pos)) {
            shSinCosV_Scale(vec, dp->path.markangle, 1500.0f);
            _shAddVector(dp->pap.target, (float *)&dp->scp->pos, vec);
            dp->path.timer = 0;
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            enPAPAnimeSet(dp, 1);
            dp->sslv = 1;
        }
        break;
    case 3:
        if (dp->anim_n == -1) {
            enPAPAnimeSet(dp, 1);
            dp->sslv = 1;
        }
        break;
    }
    enPAPMoveAngle(dp);
    if (enCalcAngleDifference(dp->path.markangle, dp->path.angle) > PI / 2) {
        enPAPAnimeSet(dp, 30);
        dp->sslv = 3;
    }
    enPAPAutoRecovery(dp);
    enPAPSoundSigns(dp);
}

static void enPAPCtrlStraight(struct EnLOCAL_DATA *dp) {
    float vec[4];

    if (enCheckDamage(dp)) {
        enFlagSetMoved(dp);
        if (enCheckSpray(dp)) {
            enReduceHP(dp);
            if (enGetSprayPower()) {
                EN_SET_LEVEL(dp, 6);
                return;
            }
        } else {
            EN_SET_LEVEL(dp, 5);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        enPAPAnimeSet(dp, 1);
        enPAPGetWalkSpeed(dp);
        enInitPath(&dp->path, dp->scp->rot.y);
        shSinCosV_Scale(vec, dp->scp->rot.y, 1500.0f);
        _shAddVector(dp->pap.target, (float *)&dp->scp->pos, vec);
        dp->sslv++;
        break;
    case 1:
        if (shRandF() < 0.001f) {
            enPAPAnimeSet(dp, 2);
            dp->sslv = 2;
            return;
        }
        if (enPAPCanSeePlayer(dp) == 2) {
            enFlagSetMoved(dp);
            EN_SET_LEVEL(dp, 4);
            return;
        }
        if (enSetPath(dp, dp->pap.target, (float *)&dp->scp->pos)) {
            if (shRandF() < 0.01f) {
                enPAPAnimeSet(dp, 2);
                dp->sslv = 2;
                return;
            }
            dp->path.timer = 0;
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            enPAPAnimeSet(dp, 1);
            dp->sslv = 1;
        } else {
            return;
        }
    case 3:
        if (dp->anim_n == -1) {
            enPAPAnimeSet(dp, 1);
            dp->sslv = 1;
        }
        break;
    }
    enPAPMoveAngle(dp);
    if (enCalcAngleDifference(dp->path.markangle, dp->path.angle) > PI / 2) {
        enPAPAnimeSet(dp, 30);
        dp->sslv = 3;
    }
    shSinCosV_Scale(vec, dp->path.markangle, 1500.0f);
    _shAddVector(dp->pap.target, (float *)&dp->scp->pos, vec);
    enPAPAutoRecovery(dp);
    enPAPSoundSigns(dp);
}

static void enPAPCtrlChase(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            enReduceHP(dp);
            if (enGetSprayPower()) {
                EN_SET_LEVEL(dp, 6);
                return;
            }
        } else {
            EN_SET_LEVEL(dp, 5);
            return;
        }
    }
    if (!dp->sslv) {
        enPAPAnimeSet(dp, 1);
        dp->sslv++;
    } else if (dp->sslv == 2 && dp->anim_n == -1) {
        enPAPAnimeSet(dp, 1);
        dp->sslv = 1;
    }
    dp->path.markangle = enGetPlayerDirection(dp);
    dp->path.angle = dp->scp->rot.y;
    enPAPMoveAngle(dp);
    if (dp->sslv != 2) {
        dp->anim_s = ftoi(4096.0f * enPAPGetWalkSpeed(dp) *
                          enCalcSpeedRate(dp->scp->rot.y, (float *)&dp->scp->pos, enGetPlayerPos(dp)));
        t = enPAPCanSeePlayer(dp);
        if (t == 2) {
            EN_SET_LEVEL(dp, 4);
        } else if (!t && !dp->type && (dp->p_dist > 2500.0f || (enLocalWork.Status & 1))) {
            EN_SET_LEVEL(dp, 0);
        } else if (enCalcAngleDifference(dp->path.markangle, dp->path.angle) > PI / 2) {
            enPAPAnimeSet(dp, 30);
            dp->sslv = 2;
        }
    }
    enPAPAutoRecovery(dp);
    enPAPSoundSigns(dp);
}

/* Matching: enPAPCtrlRush: the stand-in before it sets its float-constant argument order. */
static float __stripped_float_code_101(float x0, float x1, float x2, float x3) { int i0 = (int)x0 * 65; i0 = i0 * 666; i0 = i0 * 729; i0 = i0 * 740; i0 = i0 * 740; x2 += 60.0f; i0 = i0 * 3; x1 += 1630.0f; x1 += 1340.0f; x0 += 4537.0f * x1; i0 = i0 * 647; i0 = i0 * 641; i0 = i0 * 664; x0 += 2529.0f; i0 = i0 * 886; x0 += 729.0f; i0 = i0 * 780; return x0 + (float)i0; } /* fitted, not recovered: 6 constants */
static void enPAPCtrlRush(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            enReduceHP(dp);
            if (enGetSprayPower()) {
                EN_SET_LEVEL(dp, 6);
                return;
            }
        } else if (dp->last_atk == 2 || dp->last_atk == 1) {
            dp->scp->battle.damage *= 0.4f;
            enReduceHP(dp);
            if (dp->scp->battle.hp <= 0.0f) {
                EN_SET_LEVEL(dp, 5);
                return;
            }
        } else {
            EN_SET_LEVEL(dp, 5);
            return;
        }
    }
    if (!dp->sslv) {
        enPAPAnimeSet(dp, 1);
        enSetNewSize(dp, 200.0f, 650.0f, 600.0f, 650.0f);
        enSetTimer(dp, 300);
        dp->sslv++;
    } else if (dp->sslv == 2 && dp->anim_n == -1) {
        enPAPAnimeSet(dp, 1);
        dp->slv = 2;
        dp->sslv = 0;
    }
    dp->path.markangle = enGetPlayerDirection(dp);
    dp->path.angle = dp->scp->rot.y;
    enPAPMoveAngle(dp);
    if (enReduceTimer(dp) <= 0) {
        EN_SET_LEVEL(dp, 2);
        enSetNewSize(dp, 400.0f, 650.0f, 600.0f, 650.0f);
        return;
    }
    if (dp->sslv != 2) {
        dp->anim_s = ftoi(5120.0f * enPAPGetWalkSpeed(dp) *
                          enCalcSpeedRate(dp->scp->rot.y, (float *)&dp->scp->pos, enGetPlayerPos(dp)));
        if (enPAPCanSeePlayer(dp) == 2) {
            enFlagSetNoDamage(dp);
            EN_SET_LEVEL(dp, 4);
        }
    }
    enPAPAutoRecovery(dp);
    enPAPSoundSigns(dp);
}

/* FAKEMATCH: fitted to the float-constant order, not recovered: both `enCheckHuggedPlayer() != 0`, level changes
 * 6 and 5 as `dp->slv = N; dp->sslv = 0;`, and the written-out `dp->flag` updates
 * (docs/matching-notes.md#en_pap-enpapctrlattack). */
static void enPAPCtrlAttack(struct EnLOCAL_DATA *dp) {
    float vec[4];

    if (enCheckDamage(dp)) {
        if (enCheckHuggedPlayer() != 0) {
            enResetDamage(dp);
        } else {
            if (enCheckSpray(dp)) {
                enReduceHP(dp);
                if (enGetSprayPower()) {
                    dp->slv = 6; dp->sslv = 0;
                    return;
                }
            } else {
                dp->slv = 5; dp->sslv = 0;
                return;
            }
        }
    }
    switch (dp->sslv) {
    case 0:
        if (enCalcAngleDifference(enGetPlayerDirection(dp), enGetPlayerAngle(dp)) > PI / 2) {
            enPAPAnimeSet(dp, 28);
        } else {
            enPAPAnimeSet(dp, 31);
        }
        enSetNewSize(dp, 150.0f, 650.0f, 600.0f, 650.0f);
        enFlagSetCritical(dp);
        enAttackStart(dp);
        dp->sslv++;
        break;
    case 1:
        enAttackCheck(dp, 0x39);
        dp->path.markangle = enGetPlayerDirection(dp);
        dp->path.angle = dp->scp->rot.y;
        enMoveAngle(&dp->path, 0.05235988f);
        dp->scp->rot.y = dp->path.angle;
        if (enCheckHuggedPlayer() != 0) {
            dp->flag |= 8;
            shSinCosV_Scale(vec, enGetPlayerDirection(dp), 150.0f);
            _shSubVector(vec, enGetPlayerPos(dp), vec);
            _shSubVector(vec, vec, (float *)&dp->scp->pos);
            vec[1] = vec[3] = 0.0f;
            _shScaleVector(dp->vec, vec, 4.0f);
            enSetNewSize(dp, 0.0f, 650.0f, 600.0f, 650.0f);
        }
        if (dp->anim_n == -1) {
            dp->flag = dp->flag | 0x8000;
            if (!enCheckHuggedPlayer()) {
                enPAPAnimeSet(dp, dp->anim == 28 ? (unsigned char)29 : (unsigned char)32);
                dp->sslv = 4;
            } else {
                enPAPAnimeSet(dp, dp->anim == 28 ? (unsigned char)3 : (unsigned char)4);
                enAttackStart(dp);
                dp->sslv = 2;
            }
            dp->flag = dp->flag & ~0x8000;
        }
        break;
    case 2:
        enAttackCheckHug(dp, 0x3A);
        if (!enCheckHuggedPlayer()) {
            enPAPAnimeSet(dp, dp->anim == 3 ? (unsigned char)29 : (unsigned char)32);
            dp->sslv++;
        }
        break;
    case 3:
        shSinCosV_Scale(dp->vec, dp->scp->rot.y, -500.0f);
    case 4:
        if (dp->anim_n == -1) {
            enSetNewSize(dp, 400.0f, 650.0f, 600.0f, 650.0f);
            dp->flag &= ~8;
            enFlagResetCritical(dp);
            enFlagResetNoDamage(dp);
            if (!dp->type) {
                EN_SET_LEVEL(dp, 0);
            } else {
                EN_SET_LEVEL(dp, 2);
            }
        }
        break;
    }
    enPAPAutoRecovery(dp);
    enPAPSoundSigns(dp);
}

static void enPAPCtrlDamage(struct EnLOCAL_DATA *dp) {
    float d;

    switch (dp->sslv) {
    case 0:
        if (dp->endurance == dp->endurance_max && !enCheckCritical(dp)) {
            enReduceHP(dp);
            enAnimePause(dp);
            enSetHitBack(dp);
            enSetTimer(dp, 10);
            dp->sslv = 3;
            break;
        }
        enFlagResetCritical(dp);
        d = enReduceHP(dp);
        if (d <= 0.0f || (d < 100.0f && shRandF() > 0.8f + 0.002f * d)) {
            enPAPAnimeSet(dp, enGetDownMotion(dp));
            if (dp->type == 1 && dp->scp->battle.hp <= 0.0f) {
                enSetBlur();
                dp->flag |= 8;
                dp->anim_s = 0x200;
            }
        } else {
            enPAPAnimeSet(dp, enGetDamageMotion(dp));
        }
        if (dp->anim >= 14 && dp->anim <= 21) {
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 300.0f, 250.0f, 200.0f, 250.0f);
            dp->sslv = 2;
        } else {
            dp->sslv = 1;
        }
        enSetHitBack(dp);
        break;
    case 1:
        if (dp->anim_n == -1 && 0.0f == dp->hb_s) {
            if ((dp->type == 1 || enGetMode() >= 3) && (dp->last_atk == 2 || dp->last_atk == 1)) {
                EN_SET_LEVEL(dp, 3);
            } else {
                EN_SET_LEVEL(dp, 2);
            }
        } else if (enCheckDamage(dp)) {
            dp->sslv = 0;
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            EN_SET_LEVEL(dp, 7);
            enFlagResetNoDamage(dp);
        }
        break;
    case 3:
        if (enReduceTimer(dp) <= 0) {
            EN_SET_LEVEL(dp, 2);
        } else if (enCheckDamage(dp)) {
            dp->sslv = 0;
        }
        break;
    }
}

static void enPAPCtrlConfuse(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            dp->sslv = 0;
        } else {
            EN_SET_LEVEL(dp, 5);
            return;
        }
    }
    if (!dp->sslv) {
        enReduceHP(dp);
        enPAPAnimeSet(dp, 2);
        dp->sslv++;
    }
    if (dp->sslv == 1 && dp->anim_n == -1) {
        dp->sslv++;
        enSetTimer(dp, 600 - enGetMode() * 90);
    } else if (dp->sslv == 2 && enReduceTimer(dp) <= 0) {
        EN_SET_LEVEL(dp, 0);
    }
}

/* Matching: enPAPCtrlDown: the stand-in before it sets its float-constant argument order. */
static float __stripped_float_code_102(float x0, float x1, float x2, float x3) { int i0 = (int)x0 * 69; int i1 = (int)x0 * 35; x1 += 1897.0f; x1 += 2352.0f; x3 += 2696.0f; x1 += 1288.0f; x1 += 4097.0f; i0 = i0 * 375; x1 += 538.0f * x2; x2 += 392.0f * x0; x1 += 4128.0f; x3 += 582.0f; x2 += 4526.0f * x2; x2 += 281.0f; x3 += 2713.0f; x1 += 4032.0f; x1 += 253.0f; x3 += 1270.0f * x3; i1 = i0 * 117; x2 += 2757.0f * x3; i0 = i0 * 361; x2 += 2516.0f; x1 += 3365.0f * x0; x3 += 2575.0f; x0 += 1580.0f; i1 = i0 * 739; x3 += 2392.0f; x0 += 4840.0f; x2 += 3772.0f * x0; x3 += 3094.0f; x1 += 2513.0f; x0 += 916.0f; x1 += 4182.0f * x2; x0 += 4044.0f; x2 += 2769.0f; x2 += 3874.0f; x2 += 1675.0f * x1; x3 += 3633.0f; x2 += 3563.0f; i1 = i1 * 695; x1 += 1217.0f * x0; x0 += 4367.0f; return x0 + (float)i0 + (float)i1; } /* fitted, not recovered: 35 constants */
static void enPAPCtrlDown(struct EnLOCAL_DATA *dp) {
    switch (dp->sslv) {
    case 0:
        enFlagSetRotFloor(dp);
        enResetDamage(dp);
        if (!dp->type) {
            enFlagSetLieDown(dp);
        }
        enSetNewSize(dp, 300.0f, 250.0f, 200.0f, 250.0f);
        if (enCheckDeath(dp)) {
            if (dp->type == 1) {
                enSetBlur();
                dp->flag |= 8;
                EN_SET_LEVEL(dp, 8);
                break;
            }
            enFlagResetMoved(dp);
            if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                enPAPAnimeSet(dp, 27);
                EN_SET_LEVEL(dp, 9);
                break;
            }
            enPAPSetDc(dp);
            dp->sslv = 4;
            break;
        }
        if (dp->type != 1) {
            enPAPSetDownTime(dp);
        } else {
            enSetTimer(dp, 0);
        }
        dp->sslv++;
        break;
    case 1:
        if (enCheckDamage(dp)) {
            enAnimePause(dp);
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                enPAPAnimeSet(dp, 27);
                EN_SET_LEVEL(dp, 9);
                break;
            }
            dp->sslv = 3;
            enSetTimer(dp, 60);
            break;
        }
        if (enReduceTimer(dp) <= 0 && !enCheckFinishedByHuman(dp)) {
            if (!enGetMode() && dp->type != 1) {
                dp->scp->battle.hp_rate = 0.0f;
                dp->scp->battle.hp = 0.0f;
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 9);
                break;
            }
            enPAPAnimeSet(dp, 23);
            enFlagResetRotFloor(dp);
            enFlagResetLieDown(dp);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 400.0f, 650.0f, 600.0f, 650.0f);
            enAddHP(dp, 100.0f);
            dp->sslv = 2;
            break;
        }
        if (dp->flag & 0x2000) {
            if (dp->anim == 25) {
                if (200.0f * shRandF() < 100.0f - dp->scp->battle.hp_rate) {
                    enAnimePause(dp);
                    enAddHP(dp, 10.0f);
                }
            } else {
                if (300.0f * shRandF() < 200.0f + dp->scp->battle.hp_rate) {
                    enPAPAnimeSet(dp, 25);
                } else {
                    enAddHP(dp, 10.0f);
                }
            }
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            enResetDamage(dp);
            enFlagResetNoDamage(dp);
            EN_SET_LEVEL(dp, 2);
        }
        break;
    case 3:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckDeath(dp)) {
                enFlagResetMoved(dp);
                if (dp->type == 1) {
                    enSetBlur();
                    dp->flag |= 8;
                    EN_SET_LEVEL(dp, 8);
                    break;
                }
                if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                    enKillCountUp(dp);
                    enPAPAnimeSet(dp, 27);
                    EN_SET_LEVEL(dp, 9);
                    break;
                }
                enPAPSetDc(dp);
                dp->sslv = 4;
                break;
            }
            if (enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                enPAPAnimeSet(dp, 27);
                EN_SET_LEVEL(dp, 9);
                break;
            }
            enSetTimer(dp, 60);
            break;
        }
        if (dp->hb_s == 0.0f && enReduceTimer(dp) <= 0) {
            if (!enGetMode() && dp->type != 1) {
                dp->scp->battle.hp_rate = 0.0f;
                dp->scp->battle.hp = 0.0f;
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 11);
                break;
            }
            if (enCheckFinishedByHuman(dp)) {
                dp->sslv = 5;
                break;
            }
            enPAPAnimeSet(dp, 23);
            enAddHP(dp, 50.0f);
            enFlagResetRotFloor(dp);
            enFlagResetLieDown(dp);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 400.0f, 650.0f, 600.0f, 650.0f);
            dp->sslv = 2;
        }
        break;
    case 4:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            dp->hb_s *= itof(dp->pap.dcm - dp->pap.dc) / (dp->pap.dcm + 1);
            if (++dp->pap.dc >= dp->pap.dcm || enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                enPAPAnimeSet(dp, 27);
                EN_SET_LEVEL(dp, 9);
                break;
            }
            enSetTimer(dp, 200);
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            enKillCountUp(dp);
            enPAPAnimeSet(dp, 27);
            EN_SET_LEVEL(dp, 9);
        }
        break;
    case 5:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                enPAPAnimeSet(dp, 27);
                EN_SET_LEVEL(dp, 9);
                break;
            }
            enSetTimer(dp, 60);
            dp->sslv = 3;
            break;
        }
        if (!enCheckFinishedByHuman(dp)) {
            enPAPAnimeSet(dp, 23);
            enAddHP(dp, 50.0f);
            enFlagResetRotFloor(dp);
            enFlagResetLieDown(dp);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 400.0f, 650.0f, 600.0f, 650.0f);
            dp->sslv = 2;
        }
        break;
    }
}

static void enPAPCtrlLastMove(struct EnLOCAL_DATA *dp) {
    if (dp->anim_n == -1) {
        enResetFilter();
        game_flag.flag[10] |= 0x4000;
        enPAPAnimeSet(dp, 27);
        EN_SET_LEVEL(dp, 9);
    }
}

static void enPAPMoveAngle(struct EnLOCAL_DATA *dp) {
    float vec[4];
    float vec2[4];
    float tpos[4];
    float a;
    float d;

    if (dp->anim != 30) {
        d = enPAPGetRotSpeed();
        a = dp->path.markangle;
        shSinCosV_Scale(vec, a, 300.0f + dp->size);
        _shAddVector(vec, (float *)&dp->scp->pos, vec);
        shSinCosV_Scale(vec2, PI / 2 + a, dp->size);
        _shAddVector(tpos, vec, vec2);
        if (enCheckPath(dp, tpos, (float *)&dp->scp->pos) >= 0.0f) {
            a -= d;
        }
        _shSubVector(tpos, vec, vec2);
        if (enCheckPath(dp, tpos, (float *)&dp->scp->pos) >= 0.0f) {
            a += d;
        }
        dp->path.markangle = shAngleRegulate(a);
    } else {
        d = enPAPGetTurnSpeed(dp) * shSinF(PI * dp->anim_n / 2400.0f / 30.0f);
    }
    enMoveAngle(&dp->path, d);
    dp->scp->rot.y = dp->path.angle;
}

static int enPAPCanSeePlayer(struct EnLOCAL_DATA *dp) {
    float *ppos;
    float dist;
    float a;

    ppos = enGetPlayerPos(dp);
    dist = enCheckPath2(dp, ppos, (float *)&dp->scp->pos);
    dp->p_dist = enDistXZ(ppos, (float *)&dp->scp->pos);
    if (dist >= 0.0f) {
        return 0;
    }
    dist = dp->p_dist;
    if (dist > 2500.0f) {
        return 0;
    }
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a = enCalcAngleDifference(a, dp->scp->rot.y);
    if (!enCheckNoDamageHuman(dp) && a < 0.17453292f &&
        dist < 405.0f + enGetPlayerSize() - (dp->slv == 3 ? 100.0f : 0.0f)) {
        return 2;
    }
    if (enLocalWork.Status & 1) {
        return 0;
    }
    if (a > 0.17453292f) {
        return 0;
    }
    return 1;
}

static void enPAPAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        enAnimeRestart(dp);
        if (anim == 2) {
            enPAPSetMoveCount(dp);
        }
        return;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 1097
    fjAssert(anim >= 0 && anim < sizeof(EnPAPAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnPAPAnime[anim].Anime);
    if (anim == 2) {
        enPAPSetMoveCount(dp);
    }
    if (anim >= 14 && anim <= 27) {
        dp->flag &= ~0x400;
    } else {
        dp->flag |= 0x400;
    }
}

static void enPAPAnimeExec(struct EnLOCAL_DATA *dp) {
    int of;

    of = dp->anim_n;
    enAnimeExec(dp, EnPAPAnime, 0x1645);
    if (dp->anim == 27) {
        enPAPDrawBoad(dp);
    }
    if (dp->anim_n == -1 || (dp->flag & 1)) {
        return;
    }
    if (dp->anim == 11 || dp->anim == 12 || dp->anim == 20 || dp->anim == 21) {
        enSetTransWalk(dp);
    } else if (dp->anim == 1) {
        enSetTransWalk(dp);
        if ((dp->anim_s > 0 && ((of < 0x12C0 && dp->anim_n >= 0x12C0) || (of < 0x9600 && dp->anim_n >= 0x9600))) ||
            (dp->anim_s < 0 && ((of > 0x5460 && dp->anim_n <= 0x5460) || (of > 0xD7A0 && dp->anim_n <= 0xD7A0)))) {
            if (enCheckWater(dp)) {
                enSoundCall(0x4978, 1.0f, (float *)&dp->scp->pos);
            } else {
                if (dp->flag & 0x1000) {
                    enSoundCall3D(0x477C, 1.0f, (float *)&dp->scp->pos);
                } else {
                    enSoundCall(0x477C, 1.0f, (float *)&dp->scp->pos);
                }
            }
        }
    } else if (dp->anim == 30) {
        if (dp->anim_n <= 0xCE40) {
            enFlagSetNoDamage(dp);
        } else {
            enFlagResetNoDamage(dp);
        }
    }
}

static void enPAPAutoRecovery(struct EnLOCAL_DATA *dp) {
    short recover_rate[5] = { 0, 10, 30, 60, 100 };

    enAddEnduranceDT(dp, itof(recover_rate[enGetMode()]));
}

static float enPAPGetWalkSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.7f, 0.8f, 1.0f, 1.2f, 1.5f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.5f + dp->scp->battle.hp_rate / 200.0f;
    if (dp->type == 1) {
        r *= 0.5f;
    }
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

static float enPAPGetRotSpeed(void) {
    float rot_rate[5] = { 0.5f, 0.8f, 1.0f, 1.2f, 1.5f };

    return 0.05235988f * rot_rate[enGetMode()];
}

static float enPAPGetTurnSpeed(struct EnLOCAL_DATA *dp) {
    float rot_rate[5] = { 1.0f, 1.0f, 1.0f, 1.5f, 2.0f };
    float r;

    r = rot_rate[enGetMode()];
    dp->anim_s = ftoi(4096.0f * r);
    return 0.10471976f * r;
}

static void enPAPSetDownTime(struct EnLOCAL_DATA *dp) {
    short down_time[5] = { 900, 480, 360, 300, 240 };
    short down_time2[5] = { 180, 180, 120, 60, 60 };

    if (!dp->type) {
        enSetTimer(dp, down_time[enGetMode()]);
    } else {
        enSetTimer(dp, down_time2[enGetMode()]);
    }
}

static void enPAPSetMoveCount(struct EnLOCAL_DATA *dp) {
    int n;

    n = ftoi(10.0f - 0.4f * enGetMode() + shSway1f(-4.0f, 0.5f));
    enSetAnimeCount(dp, n << 11);
}

static void enPAPSetDc(struct EnLOCAL_DATA *dp) {
    enSetTimer(dp, 200);
    dp->pap.dc = 0;
    dp->pap.dcm = (shRandI() >> 10) % (enGetMode() + 5) + 4;
}

/* Matching: 1.5707964f as a literal (not PI / 2) and the stand-in: float-constant order of the
 * shSinCosV_Scale calls. */
static float __stripped_float_code_3(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f; }
static void enPAPDrawBoad(struct EnLOCAL_DATA *dp) {
    float pos[4][4];
    float vec1[4];
    float vec2[4];
    float wsm[4][4];
    int iv[4];
    unsigned long xyz[4];
    int i;
    int cn;
    unsigned int w;

    sceVu0CopyMatrix(wsm, cam0.world_screen);
    shSinCosV_Scale(vec1, dp->scp->rot.y, 495.0f);
    shSinCosV_Scale(vec2, 1.5707964f + dp->scp->rot.y, 195.0f);
    _shAddVector(pos[0], (float *)&dp->scp->pos, vec1);
    _shAddVector(pos[0], pos[0], vec2);
    _shAddVector(pos[1], (float *)&dp->scp->pos, vec1);
    _shSubVector(pos[1], pos[1], vec2);
    shSinCosV_Scale(vec1, dp->scp->rot.y, 315.0f);
    _shSubVector(pos[2], (float *)&dp->scp->pos, vec1);
    _shAddVector(pos[2], pos[2], vec2);
    _shSubVector(pos[3], (float *)&dp->scp->pos, vec1);
    _shSubVector(pos[3], pos[3], vec2);
    w = 0;
    cn = 0;
    for (i = 0; i < 4; i++) {
        pos[i][1] -= 10.0f;
        pos[i][3] = 1.0f;
        xyz[i] = _shRotTransPersXYZ(iv, wsm, pos[i]);
        if (iv[3] < 0 || iv[2] < 0 || iv[0] < 0 || iv[1] < 0 || iv[0] > 0xFFFF || iv[1] > 0xFFFF) {
            return;
        }
        cn += shScreenClipI(iv);
        if (w < iv[3]) {
            w = iv[3];
        }
    }
    if (cn <= 3) {
        spkOpenDGiftag(0x6400000000008000, 0x55DD10, w, 1);
        PK_ADD(4);
        PK_ADD(0x80000000);
        PK_ADD(xyz[0]);
        PK_ADD(xyz[1]);
        PK_ADD(xyz[2]);
        PK_ADD(xyz[3]);
        spkCloseGiftag();
    }
}

static void enPAPSoundSigns(struct EnLOCAL_DATA *dp) {
    int signs;

    if ((dp->pap.count -= shGetDF()) <= 0) {
        dp->pap.count = enCalcTimer(120);
        do {
            signs = (shRandI() >> 20) & 7;
        } while (signs == dp->pap.signs);
        dp->pap.signs = signs;
        enSoundCall(signs + 0x4781, 0.4f, (float *)&dp->scp->pos);
    }
}
