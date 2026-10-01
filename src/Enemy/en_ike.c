/*
 * en_ike.c: AI of the IKE enemy: appears at fixed points, waits, moves, chases, swings, kicks
 * and attacks, and can take the player (the hand position follows its skeleton, see enIKETrans).
 * Shared enemy code is in en_common.c.
 * The Flesh Lip (verified; docs/characters.md).
 */
#include "enemy.h"

/* Matching: level changes use EN_SET_LEVEL where the original's line table has both assignments on one
 * line; how neighbouring functions are spelled sets the enSetSize/enSetNewSize calls' float-constant order
 * (docs/matching-notes.md#en_ike-file). */

static void enIKECtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enIKECtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enIKECtrlHand(struct EnLOCAL_DATA *dp);
static void enIKECtrlWait(struct EnLOCAL_DATA *dp);
static void enIKECtrlAppearance(struct EnLOCAL_DATA *dp);
static void enIKECtrlMove(struct EnLOCAL_DATA *dp);
static void enIKECtrlChase(struct EnLOCAL_DATA *dp);
static void enIKECtrlKick(struct EnLOCAL_DATA *dp);
static void enIKECtrlSwing(struct EnLOCAL_DATA *dp);
static void enIKECtrlAttack(struct EnLOCAL_DATA *dp);
static void enIKECtrlAttack2(struct EnLOCAL_DATA *dp);
static void enIKECtrlTakePlayer(struct EnLOCAL_DATA *dp);
static void enIKECtrlDamage(struct EnLOCAL_DATA *dp);
static void enIKECtrlDead(struct EnLOCAL_DATA *dp);
static int enIKECountActive(struct EnLOCAL_DATA *dp);
static int enIKECheckOther(struct EnLOCAL_DATA *dp);
static int enIKECheckOther2(struct EnLOCAL_DATA *dp);
static float *enIKECheckNearOther(struct EnLOCAL_DATA *dp);
static void enIKECheckNearPlayer(struct EnLOCAL_DATA *dp);
static int enIKEGetDamageMotion(struct EnLOCAL_DATA *dp);
static int enIKECanSeePlayer(struct EnLOCAL_DATA *dp);
static int enIKECanSeePlayer2(struct EnLOCAL_DATA *dp);
static void enIKEAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enIKEAnimeExec(struct EnLOCAL_DATA *dp);
static float enIKEGetMoveSpeed(struct EnLOCAL_DATA *dp);
static void enIKESoundSwing(struct EnLOCAL_DATA *dp);
static void enIKESoundSigns(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnIKEAnime[21] = {
    { 0x15E1, 0 }, { 0x15E1, 1 }, { 0x15E2, 0 }, { 0x15E3, 0 }, { 0x15E5, 1 }, { 0x15E6, 0 }, { 0x15E4, 0 },
    { 0x15F3, 0 }, { 0x15E7, 0 }, { 0x15E8, 0 }, { 0x15E9, 0 }, { 0x15EA, 0 }, { 0x15EB, 0 }, { 0x15EC, 0 },
    { 0x15ED, 0 }, { 0x15EE, 0 }, { 0x15EF, 0 }, { 0x15F0, 0 }, { 0x15F1, 0 }, { 0x15F2, 0 }, { 0x15F4, 0 },
};

float appearance_point[7][4] = {
    { -141100.0f, -3000.0f, -20400.0f, 1.5707964f },
    { -141100.0f, -3000.0f, -19800.0f, 1.5707964f },
    { -140200.0f, -3000.0f, -18500.0f, 3.1415927f },
    { -139600.0f, -3000.0f, -18500.0f, 3.1415927f },
    { -138900.0f, -3000.0f, -19600.0f, -1.5707964f },
    { -138900.0f, -3000.0f, -20400.0f, -1.5707964f },
    { -140000.0f, -3000.0f, -21500.0f, 0.0f },
};

/* Matching: enIKEInitData: the stand-in before it sets its float-constant argument order. */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f; }
/** Sets up a new IKE: HP by difficulty mode, size; it starts in sub level 1, or 0 below y = -2140.
 * @param dp enemy work */
void enIKEInitData(struct EnLOCAL_DATA *dp) {
    float vitarity[5] = { 1000.0f, 1500.0f, 2000.0f, 4000.0f, 5000.0f };
    int mode;

    mode = enGetMode();
    dp->mlv = 1;
    dp->slv = 1;
    dp->sslv = 0;
    enSetBattleTarget(dp, 1);
    if (dp->scp->pos.y < -2140.0f) {
        dp->slv = 0;
        dp->sslv = 0;
    }
    enSetSize(dp, 150.0f, 650.0f, 500.0f, 600.0f);
    dp->weight = 3;
    enSetHP(dp, vitarity[mode], vitarity[mode]);
    dp->flag |= 0x42;
    enIKEAnimeSet(dp, 1);
    enLocalWork.ActiveEnemy++;
}

/** Per-frame control of an IKE: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enIKECtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlIKEFunc[6])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enIKECtrlAutomatic, enDummyCtrl, enIKECtrlGoPlayable, enDummyCtrl, enIKECtrlHand,
    };

    enCtrlIKEFunc[dp->mlv](dp);
}

static void enIKECtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlIKESubFunc[11])(struct EnLOCAL_DATA *) = {
        enIKECtrlWait,   enIKECtrlAppearance, enIKECtrlMove,       enIKECtrlChase,  enIKECtrlKick, enIKECtrlSwing,
        enIKECtrlAttack, enIKECtrlAttack2,    enIKECtrlTakePlayer, enIKECtrlDamage, enIKECtrlDead,
    };

    enSetBattleTarget(dp, 1);
    enCtrlIKESubFunc[dp->slv](dp);
    enIKEAnimeExec(dp);
    enMoveExec(dp);
}

static void enIKECtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    dp->slv = 2;
    dp->sslv = 0;
}

static void enIKECtrlHand(struct EnLOCAL_DATA *dp) {
}

static void enIKECtrlWait(struct EnLOCAL_DATA *dp) {
    int i;
    int n;
    float dm;
    float d;
    int j;
    float *ppos;
    struct EnLOCAL_DATA *tp;

    switch (dp->sslv) {
    case 0:
        enIKEAnimeSet(dp, 1);
        enSetTimer(dp, 0x2A30);
        dp->flag |= 8;
        enFlagResetDisplay(dp);
        dp->sslv++;
        break;
    case 1:
        n = enIKECountActive(dp);
        if (n && enReduceTimer(dp) > 0) {
            break;
        }
        ppos = enGetPlayerPos(dp);
        dm = 0.0f;
        for (i = 0; i < ARRAY_COUNT(appearance_point); i++) {
            tp = enLocalWork.Data;
            for (j = 0; j < 32; j++, tp++) {
                if (tp != dp && tp->kind == 7 && enDistXZ(appearance_point[i], (float *)&tp->scp->pos) < 1000.0f) {
                    j = -1;
                    break;
                }
            }
            if (j == -1) {
                break;
            }
            d = enDistXZ(appearance_point[i], ppos);
            if (d > dm) {
                dm = d;
                n = i;
            }
        }
        vcopy3(appearance_point[n], &dp->scp->pos);
        vcopy(&dp->scp->pos, &dp->scp->b_pos);
        dp->scp->rot.y = appearance_point[n][3];
        EN_SET_LEVEL(dp, 1);
        enFlagSetDisplay(dp);
        break;
    }
}

static void enIKECtrlAppearance(struct EnLOCAL_DATA *dp) {
    switch (dp->sslv) {
    case 0:
        if (dp->scp->pos.y >= -1640.0f) {
            EN_SET_LEVEL(dp, 2);
            return;
        }
        enIKEAnimeSet(dp, 1);
        dp->flag |= 8;
        dp->sslv++;
    case 1:
        if (((float *)&dp->scp->pos)[1] >= -1640.0f) {
            ((float *)&dp->scp->pos)[1] = -1640.0f;
            enSetTimer(dp, 60);
            enInitPath(&dp->path, dp->scp->rot.y);
            dp->sslv++;
        } else {
            dp->vec[1] = 250.0f;
        }
        break;
    case 2:
        shSinCosV_Scale(dp->vec, dp->scp->rot.y, enIKEGetMoveSpeed(dp));
        if (enReduceTimer(dp) <= 0) {
            dp->flag &= ~8;
            EN_SET_LEVEL(dp, 2);
        }
        break;
    }
    enIKESoundSigns(dp);
}

static void enIKECtrlMove(struct EnLOCAL_DATA *dp) {
    float vec[4];
    int t;
    float a;
    float *tpos;
    struct EnLOCAL_DATA *tp;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        return;
    }
    if (!dp->sslv) {
        enIKEAnimeSet(dp, 1);
        enSetTimer(dp, 0);
        dp->sslv++;
    }
    if (shRandF() < 0.001f && (tp = enGetNearOtherEnemy(dp))) {
        if (dp->scp->pos.x > -140200.0f && dp->scp->pos.x < -139800.0f && dp->scp->pos.z > -20600.0f &&
            dp->scp->pos.z < -19400.0f && enDist((float *)&tp->scp->pos, (float *)&dp->scp->pos) > 1000.0f) {
            EN_SET_LEVEL(dp, 5);
            return;
        }
    }
    if ((tpos = enIKECheckNearOther(dp))) {
        a = enCalcDirection((float *)&dp->scp->pos, tpos);
        a = shAngleRegulate(a - dp->scp->rot.y);
        dp->ike.direc = ftoi(2.0f * (a / PI) + _shSignIP(a) / 2.0f) & 3;
        enInitPath(&dp->path, dp->scp->rot.y + PI * dp->ike.direc / 2.0f);
        enSetTimer(dp, 15);
    }
    shSinCosV_Scale(vec, dp->path.angle, 1000.0f);
    _shAddVector(vec, (float *)&dp->scp->pos, vec);
    t = enSetPath(dp, vec, (float *)&dp->scp->pos);
    if (dp->timer) {
        enReduceTimer(dp);
    } else if (t) {
        a = shAngleRegulate(dp->path.markangle - dp->path.angle);
        if (fabsf(a) > PI / 4) {
            dp->ike.direc = (dp->ike.direc + ftoi(_shSignIP(a))) & 3;
            enInitPath(&dp->path, dp->scp->rot.y + PI * dp->ike.direc / 2.0f);
            enSetTimer(dp, 15);
        }
    }
    enMoveAngle(&dp->path, 0.008726646f);
    dp->scp->rot.y = shAngleRegulate(dp->path.angle - PI * dp->ike.direc / 2.0f);
    shSinCosV_Scale(dp->vec, dp->path.angle, enIKEGetMoveSpeed(dp));
    t = enIKECanSeePlayer(dp);
    if (t == 3) {
        EN_SET_LEVEL(dp, 7);
    } else if (t == 2) {
        EN_SET_LEVEL(dp, 6);
    } else if (t == 1 && !enIKECheckOther(dp)) {
        EN_SET_LEVEL(dp, 3);
    } else if (enGetMode() >= 2 && !enIKECountActive(dp)) {
        EN_SET_LEVEL(dp, 3);
    }
    enIKECheckNearPlayer(dp);
    enIKESoundSigns(dp);
}

static void enIKECtrlChase(struct EnLOCAL_DATA *dp) {
    float a1;
    float a2;
    int t;
    struct _CL_VHIT_RESULT *res;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        return;
    }
    if (!dp->sslv) {
        enIKEAnimeSet(dp, 1);
        a1 = shAngleRegulate(enGetPlayerDirection(dp) - dp->scp->rot.y);
        dp->ike.direc = ftoi(2.0f * (a1 / PI) + _shSignIP(a1) / 2.0f) & 3;
        enInitPath(&dp->path, dp->scp->rot.y + PI * dp->ike.direc / 2.0f);
        enSetTimer(dp, 0);
        dp->sslv++;
    }
    a1 = shAngleRegulate(enGetPlayerDirection(dp) - dp->scp->rot.y);
    a2 = PI * dp->ike.direc / 2.0f;
    if (dp->timer) {
        enReduceTimer(dp);
    } else if (fabsf(a1) < 0.87266463f) {
        if (dp->ike.direc) {
            dp->ike.direc = 0;
            enInitPath(&dp->path, dp->scp->rot.y);
            enSetTimer(dp, 30);
        }
    } else if (enCalcAngleDifference(a1, a2) > PI / 4) {
        dp->ike.direc = ftoi(2.0f * (a1 / PI) + _shSignIP(a1) / 2.0f) & 3;
        enInitPath(&dp->path, dp->scp->rot.y + PI * dp->ike.direc / 2.0f);
        enSetTimer(dp, 30);
    }
    if (enSetPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos)) {
        res = &enLocalWork.HitResult;
        if (enGetPlayerDistance(dp) < 1000.0f && res->kind == 3 && enTransID(res->hobj.chara.sc->kind) == 7 &&
            res->hobj.chara.sc->battle.hp <= 0.0f) {
            EN_SET_LEVEL(dp, 4);
        }
    }
    enMoveAngle(&dp->path, 0.017453292f);
    dp->scp->rot.y = shAngleRegulate(dp->path.angle - PI * dp->ike.direc / 2.0f);
    shSinCosV_Scale(dp->vec, dp->path.angle, enIKEGetMoveSpeed(dp));
    t = enIKECanSeePlayer2(dp);
    if (t == 3) {
        EN_SET_LEVEL(dp, 7);
    } else if (t == 2) {
        EN_SET_LEVEL(dp, 6);
    } else if (!t || enIKECheckOther(dp)) {
        if (enGetMode() < 2 || enIKECountActive(dp)) {
            EN_SET_LEVEL(dp, 2);
        }
    }
    enIKECheckNearPlayer(dp);
    enIKESoundSigns(dp);
}

static void enIKECtrlKick(struct EnLOCAL_DATA *dp) {
    int t;
    float d;
    float d2;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        return;
    }
    if (!dp->sslv) {
        enIKEAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->ike.direc = 0;
        dp->sslv++;
    }
    dp->path.markangle = enGetPlayerDirection(dp);
    enMoveAngle(&dp->path, 0.017453292f);
    d = enGetPlayerDistance(dp);
    d2 = 750.0f - d;
    shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, d2);
    t = enIKECanSeePlayer2(dp);
    if (t == 3) {
        EN_SET_LEVEL(dp, 7);
    } else if (t == 2) {
        EN_SET_LEVEL(dp, 6);
    } else if (d > 1000.0f) {
        EN_SET_LEVEL(dp, 2);
    }
    enIKECheckNearPlayer(dp);
    enIKESoundSigns(dp);
}

static void enIKECtrlSwing(struct EnLOCAL_DATA *dp) {
    int d;
    float a;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        return;
    }
    if (!dp->sslv) {
        enIKEAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->ike.direc = (shRandI() >> 10) & 1;
        dp->ike.swing = 0.0f;
        dp->sslv++;
    }
    d = dp->ike.direc;
    if (!(dp->sslv & 1)) {
        d ^= 1;
    }
    a = dp->ike.swing += PI * shGetDT();
    switch (dp->sslv) {
    case 1:
    case 4:
        if (a > PI / 2) {
            dp->ike.swing -= PI / 2;
            dp->sslv++;
            if (dp->sslv == 2) {
                enIKESoundSwing(dp);
            } else {
                a = 0.0f;
                EN_SET_LEVEL(dp, 2);
            }
        }
        a = 0.17453292f * shSinF(2.0f * a);
        break;
    case 2:
    case 3:
        if (a > PI) {
            dp->ike.swing -= PI;
            dp->sslv++;
            if (dp->sslv == 3) {
                enIKESoundSwing(dp);
            }
        }
        a = 0.6981317f * shSinF(a);
        break;
    }
    if (d) {
        a = -a;
    }
    dp->trx = dp->scp->rot.x = a;
    enIKECheckNearPlayer(dp);
}

/* FAKEMATCH (for enIKECtrlAttack2's float-constant order): `-1 == dp->anim_n` in case 1 is a fitted spelling, not
 * evidence; the code is the same either way. */
static void enIKECtrlAttack(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        if (enCheckHuggedPlayer()) {
            enResetDamage(dp);
        } else {
            EN_SET_LEVEL(dp, 9);
            enSetNewSize(dp, 150.0f, 650.0f, 500.0f, 600.0f);
            dp->flag &= ~0x8000;
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        enIKECheckNearPlayer(dp);
        enIKEAnimeSet(dp, 2);
        enSetNewSize(dp, 150.0f, 600.0f, 400.0f, 600.0f);
        enAttackStart(dp);
        dp->flag |= 0x8000;
        enFlagSetCritical(dp);
        dp->sslv++;
        break;
    case 1:
        enIKECheckNearPlayer(dp);
        enAttackCheck(dp, 0x36);
        if (-1 == dp->anim_n) {
            dp->ike.pipe_count = 0;
            if (dp->flag & 4) {
                enIKEAnimeSet(dp, 6);
                dp->sslv = 4;
            } else {
                enIKEAnimeSet(dp, 3);
                dp->sslv++;
            }
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            enIKEAnimeSet(dp, 4);
            enAttackStart(dp);
            enSetTimer(dp, 120);
            dp->flag &= ~0x8000;
            dp->sslv++;
        }
        break;
    case 3:
        if (enCheckDeadPlayer() == 3) {
            EN_SET_LEVEL(dp, 8);
            break;
        }
        enAttackCheckHug(dp, 0x37);
        if (enReduceTimer(dp) <= 0) {
            enSoundCall(((shRandI() >> 10) & 1) + 0x4848, 1.0f, (float *)&dp->scp->pos);
            enSetTimer(dp, 120);
        }
        if (!enCheckHuggedPlayer()) {
            enIKEAnimeSet(dp, 5);
            dp->sslv++;
        }
        break;
    case 4:
        if (dp->anim_n == -1) {
            EN_SET_LEVEL(dp, 2);
            dp->flag &= ~0x8000;
            enFlagResetCritical(dp);
            enSetNewSize(dp, 150.0f, 650.0f, 500.0f, 600.0f);
        }
        break;
    }
    enIKESoundSigns(dp);
}

/* FAKEMATCH: the float-constant order of the enSetNewSize calls comes from compiler leftovers (docs/toolchain.md,
 * "Root cause"); fitted, not recovered: the braced case bodies, `dp->sslv = dp->sslv + 1`, the last `break;` and
 * enIKECtrlAttack's `-1 == dp->anim_n`. The statements are the original's (line table). */
static void enIKECtrlAttack2(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        enSetNewSize(dp, 150.0f, 650.0f, 500.0f, 600.0f);
        return;
    }
    switch (dp->sslv) {
    case 0: {
        enIKEAnimeSet(dp, 7);
        enAttackStart(dp);
        enSetNewSize(dp, 150.0f, 600.0f, 400.0f, 600.0f);
        enFlagSetCritical(dp);
        dp->sslv = dp->sslv + 1;
    }
    case 1: {
        enAttackCheck(dp, 0x38);
        if (dp->anim_n == -1) {
            EN_SET_LEVEL(dp, 2);
            enSetNewSize(dp, 150.0f, 650.0f, 400.0f, 600.0f);
            enFlagResetCritical(dp);
            dp->ike.pipe_count = 0;
        }
        break;
    }
    }
    enIKECheckNearPlayer(dp);
    enIKESoundSigns(dp);
}

static void enIKECtrlTakePlayer(struct EnLOCAL_DATA *dp) {
    int i;
    struct EnLOCAL_DATA *tp;

    if (!dp->sslv) {
        enInitPath(&dp->path, dp->scp->rot.y);
        if (fabsf(dp->scp->rot.y) <= PI / 2) {
            dp->path.markangle = 0.0f;
        } else {
            dp->path.markangle = PI;
        }
        tp = enLocalWork.Data;
        for (i = 0; i < 32; i++, tp++) {
            if (tp->kind == 7) {
                tp->weight = 0;
            }
        }
        dp->weight = 4;
        enAnimePause(dp);
        dp->sslv++;
    }
    enAttackCheckHug(dp, 0x37);
    enMoveAngle(&dp->path, 0.008726646f);
    dp->scp->rot.y = dp->path.angle;
    if (dp->sslv == 2) {
        dp->vec[1] = -500.0f;
        if (dp->scp->pos.y < -4000.0f) {
            enSetGameOver();
        }
    } else if (dp->scp->pos.z < -21450.0f) {
        dp->scp->pos.z = -21500.0f;
        dp->sslv++;
    } else {
        dp->vec[2] = -500.0f;
        if (dp->scp->pos.z < -21300.0f) {
            dp->flag |= 8;
        }
    }
}

/* Matching: enIKECtrlDamage: the stand-in before it sets its float-constant argument order. */
static float __stripped_float_code_101(float x0) { int i0 = (int)x0 * 77; int i1 = (int)x0 * 72; x0 += 1841.0f * x0; x0 += 1336.0f * x0; x0 += 4772.0f * x0; x0 += 142.0f * x0; x0 += 3331.0f; x0 += 1463.0f * x0; i0 = i0 * 125; x0 += 3221.0f * x0; x0 += 639.0f; x0 += 884.0f * x0; i1 = i1 * 135; x0 += 1649.0f * x0; x0 += 169.0f; x0 += 1017.0f; x0 += 121.0f; x0 += 2222.0f; i1 = i1 * 256; i1 = i1 * 735; x0 += 4945.0f; x0 += 4412.0f; x0 += 3409.0f * x0; x0 += 4258.0f * x0; x0 += 1294.0f * x0; return x0 + (float)i0 + (float)i1; } /* fitted, not recovered: 19 constants */
static void enIKECtrlDamage(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        if (dp->last_atk == 17) {
            dp->scp->battle.damage /= ++dp->ike.pipe_count;
            if (dp->ike.pipe_count > 4) {
                if (dp->ike.pipe_count >= 10) {
                    dp->ike.pipe_count = 0;
                }
                enReduceHP(dp);
                if (!enCheckDeath(dp)) {
                    EN_SET_LEVEL(dp, 4);
                    return;
                }
            }
        } else {
            dp->ike.pipe_count = 0;
        }
        enReduceHP(dp);
        if (enCheckSpray(dp)) {
            dp->scp->battle.hp_rate = 0.0f;
            dp->scp->battle.hp = 0.0f;
            EN_SET_LEVEL(dp, 10);
            return;
        }
        enIKEAnimeSet(dp, enIKEGetDamageMotion(dp));
        enSetHitBack(dp);
        dp->hb_s *= 0.5f;
        enSetNewSize(dp, 150.0f, 500.0f, 400.0f, 600.0f);
        dp->trz = 0.0f;
        dp->sslv++;
    }
    if (enCheckDamage(dp)) {
        enReduceHP(dp);
        enSetHitBack(dp);
        dp->hb_s *= 0.5f;
    }
    if (dp->anim_n == -1) {
        dp->scp->pos.y = -1640.0f;
        enSetNewSize(dp, 150.0f, 650.0f, 500.0f, 600.0f);
        if (enCheckDeath(dp)) {
            enKillCountUp(dp);
            EN_SET_LEVEL(dp, 10);
        } else {
            EN_SET_LEVEL(dp, 3);
        }
    }
    enIKECheckNearPlayer(dp);
}

/* Matching: enIKECtrlDead: the stand-in before it sets its float-constant argument order. */
static float __stripped_float_code_3(float x) { return x + 3.0f + 5.0f; }
static void enIKECtrlDead(struct EnLOCAL_DATA *dp) {
    float *pos;

    if (enCheckDamage(dp)) {
        enResetDamage(dp);
    }
    if (!dp->sslv) {
        enIKEAnimeSet(dp, 1);
        enFlagSetDead(dp);
        enFlagSetNoDamage(dp);
        enInitPath(&dp->path, dp->scp->rot.y + PI * dp->ike.direc / 2.0f);
        enSetSize(dp, 150.0f, 100.0f, 500.0f, 600.0f);
        if (--enLocalWork.ActiveEnemy <= 0) {
            enSetBlur();
            dp->sslv++;
        }
        dp->weight = 4;
        dp->sslv++;
    }
    if (dp->sslv <= 2) {
        pos = (float *)&dp->scp->pos;
        if (pos[0] < -140500.0f || pos[0] > -139300.0f || pos[2] < -20500.0f || pos[2] > -19300.0f) {
            enIKEAnimeSet(dp, 20);
            dp->sslv += 2;
            return;
        }
        shSinCosV_Scale(dp->vec, dp->path.angle, enIKEGetMoveSpeed(dp));
    }
    if (dp->sslv == 4) {
        if (dp->anim_n == -1) {
            game_flag.flag[6] |= 0x40000;
        }
    }
}

static int enIKECountActive(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;
    int i;
    int n;

    tp = enLocalWork.Data;
    n = 0;
    for (i = 0; i < 32; i++, tp++) {
        if (tp != dp) {
            if (tp->kind == 7 && tp->slv != 0 && tp->slv != 10) {
                n++;
            }
        }
    }
    return n;
}

static int enIKECheckOther(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;
    int i;

    tp = enLocalWork.Data;
    for (i = 0; i < 32; i++, tp++) {
        if (tp != dp) {
            if (tp->kind == 7 && tp->slv != 10 && tp->slv != 9) {
                if (enGetPlayerDistance(tp) < 500.0f) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int enIKECheckOther2(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;
    int i;

    tp = enLocalWork.Data;
    for (i = 0; i < 32; i++, tp++) {
        if (tp != dp) {
            if (tp->kind == 7 && (tp->slv == 6 || tp->slv == 7)) {
                return 1;
            }
        }
    }
    return 0;
}

static float *enIKECheckNearOther(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;
    int i;
    float d;
    float md;
    float *pos;

    tp = enLocalWork.Data;
    md = 3.4028235e38f;
    pos = NULL;
    for (i = 0; i < 32; i++, tp++) {
        if (tp != dp) {
            if (tp->kind == 7 && tp->slv != 10) {
                d = enDistXZ((float *)&tp->scp->pos, (float *)&dp->scp->pos);
                if (d < 500.0f && d < md) {
                    pos = (float *)&tp->scp->pos;
                    md = d;
                }
            }
        }
    }
    return pos;
}

static void enIKECheckNearPlayer(struct EnLOCAL_DATA *dp) {
    enCheckNearPlayer(dp, &dp->ike.near_count, &dp->ike.dist, 200.0f);
}

static int enIKEGetDamageMotion(struct EnLOCAL_DATA *dp) {
    int m;
    int id;
    int dd;
    float a;
    float vec[4];

    a = shAngleRegulate(shAtanV(dp->scp->battle.vec) - dp->scp->rot.y);
    if (fabsf(a) > PI / 2) {
        dd = 0;
    } else {
        dd = 1;
    }
    enGetSkeletonVector(vec, dp, 12);
    if (vec[1] < -150.0f) {
        dd += 2;
    }
    id = dp->last_atk;
    switch (id) {
    case 2:
    case 1:
        m = dd + 8;
        break;
    case 4:
    case 6:
        m = dd + 12;
        break;
    case 14:
    case 17:
    case 21:
    case 22:
        m = dd + 16;
        break;
    default:
        m = 16;
        printf("Illegal damage type!(%d)\n", id);
        break;
    }
    return m;
}

static int enIKECanSeePlayer(struct EnLOCAL_DATA *dp) {
    float dist;

    dist = enGetPlayerDistance(dp);
    if (dist < 50.0f && !enIKECheckOther2(dp) && !enCheckNoDamageHuman(dp)) {
        return 2;
    }
    return !(dist > 1000.0f);
}

static int enIKECanSeePlayer2(struct EnLOCAL_DATA *dp) {
    float dist;
    struct _CL_VHIT_RESULT *res;
    float d;

    dist = enGetPlayerDistance(dp);
    if (dist < 50.0f && !enIKECheckOther2(dp) && !enCheckNoDamageHuman(dp)) {
        return 2;
    }
    if (dist > 2500.0f) {
        return 0;
    }
    d = enCheckPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos);
    if (d > 0.0f && dist > 500.0f && dist < 1000.0f &&
        enGetMode() >= 2 && !enCheckNoDamageHuman(dp) && !enIKECheckOther2(dp) &&
        enCalcAngleDifference(dp->scp->rot.y, enGetPlayerDirection(dp)) < 0.17453292f) {
        return 3;
    }
    return 1;
}

static void enIKEAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim != dp->anim) {
        /* Matching: the assert bakes its original line number into the object. */
#line 1044
        fjAssert(anim >= 0 && anim < sizeof(EnIKEAnime) / sizeof(EnANIME_DATA));
        enAnimeSet(dp, anim, EnIKEAnime[anim].Anime);
    }
}

static void enIKEAnimeExec(struct EnLOCAL_DATA *dp) {
    enAnimeExec(dp, EnIKEAnime, 0x15E1);
}

static float enIKEGetMoveSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.5f, 0.8f, 1.0f, 1.2f, 1.5f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.8f + dp->scp->battle.hp_rate / 500.0f;
    return 500.0f * r;
}

static void enIKESoundSwing(struct EnLOCAL_DATA *dp) {
    enSoundCall(((shRandI() >> 20) & 3) + 0x484C, 1.0f, (float *)&dp->scp->pos);
}

static void enIKESoundSigns(struct EnLOCAL_DATA *dp) {
    int signs;

    if ((dp->ike.count -= shGetDF()) <= 0) {
        dp->ike.count = enCalcTimer(60);
        do {
            signs = (shRandI() >> 20) % 6;
        } while (signs == dp->ike.signs);
        dp->ike.signs = signs;
        enSoundCall(signs + 0x4850, 1.0f, (float *)&dp->scp->pos);
    }
}
