/*
 * en_bos.c: AI of the BOS enemy: approach, distance, two attacks, generate and dead states,
 * with a time limit (bos.end_count) set by difficulty mode. Shared enemy code is in en_common.c.
 * Mary, final boss (verified; docs/characters.md).
 */
#include "enemy.h"

static void enBOSCtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enBOSCtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enBOSCtrlHand(struct EnLOCAL_DATA *dp);
static void enBOSCtrlApproach(struct EnLOCAL_DATA *dp);
static void enBOSCtrlDistance(struct EnLOCAL_DATA *dp);
static void enBOSCtrlGenerate(struct EnLOCAL_DATA *dp);
static void enBOSCtrlAttack(struct EnLOCAL_DATA *dp);
static void enBOSCtrlAttack2(struct EnLOCAL_DATA *dp);
static void enBOSCtrlDead(struct EnLOCAL_DATA *dp);
static int enBOSCheckPlayerLastBullet(struct EnLOCAL_DATA *dp);
static int enBOSCanAttackPlayer(struct EnLOCAL_DATA *dp);
static void enBOSResetSpeed(struct EnLOCAL_DATA *dp);
static int enBOSCheckDamage(struct EnLOCAL_DATA *dp);
static int enBOSCheckFloor(struct EnLOCAL_DATA *dp);
static void enBOSCheckNearPlayer(struct EnLOCAL_DATA *dp);
static void enBOSMoveExec(struct EnLOCAL_DATA *dp);
static void enBOSAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enBOSAnimeExec(struct EnLOCAL_DATA *dp);
static float enBOSGetMoveSpeed(void);
static void enBOSSoundSigns(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnBOSAnime[10] = {
    { 0x16A9, 0 }, { 0x16A9, 1 }, { 0x16AA, 0 }, { 0x16AB, 0 }, { 0x16AC, 1 },
    { 0x16AD, 0 }, { 0x16AE, 0 }, { 0x16B0, 0 }, { 0x16AF, 0 }, { 0x16B1, 1 },
};

/** Sets up a new BOS: HP by difficulty mode, size, time limit (longer above mode 2), starting
 * level from the spawn status.
 * @param dp enemy work */
void enBOSInitData(struct EnLOCAL_DATA *dp) {
    float vitarity[5] = { 5000.0f, 7500.0f, 10000.0f, 20000.0f, 40000.0f };
    int mode;

    mode = enGetMode();
    dp->mlv = 1;
    enSetHP(dp, vitarity[mode], vitarity[mode]);
    enSetSize(dp, 250.0f, 800.0f, 100.0f, -150.0f);
    dp->weight = 2;
    dp->flag |= 0x402;
    if (mode <= 2) {
        dp->bos.end_count = 1800;
    } else {
        dp->bos.end_count = 18000;
    }
    if (dp->scp->en_first_status == 13) {
        enBOSAnimeSet(dp, 9);
        enSetSize(dp, 350.0f, 250.0f, 125.0f, 250.0f);
        EN_SET_LEVEL(dp, 5);
    } else {
        EN_SET_LEVEL(dp, 0);
    }
}

/** Per-frame control of a BOS: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enBOSCtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlBOSFunc[6])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enBOSCtrlAutomatic, enDummyCtrl, enBOSCtrlGoPlayable, enDummyCtrl, enBOSCtrlHand,
    };

    enCtrlBOSFunc[dp->mlv](dp);
}

static void enBOSCtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlBOSSubFunc[6])(struct EnLOCAL_DATA *) = {
        enBOSCtrlApproach, enBOSCtrlDistance, enBOSCtrlGenerate, enBOSCtrlAttack, enBOSCtrlAttack2, enBOSCtrlDead,
    };

    enSetBattleTarget(dp, 1);
    enCtrlBOSSubFunc[dp->slv](dp);
    enBOSAnimeExec(dp);
    if (dp->slv <= 4) {
        enBOSMoveExec(dp);
        enBOSSoundSigns(dp);
        fogSetStayPoint(&dp->scp->pos);
    } else {
        enMoveExec(dp);
        fogResetStayPoint();
    }
}

static void enBOSCtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    EN_SET_LEVEL(dp, 0);
}

static void enBOSCtrlHand(struct EnLOCAL_DATA *dp) {
}

static void enBOSCtrlApproach(struct EnLOCAL_DATA *dp) {
    float pos[4];
    float vec[4];
    float a;

    if (enBOSCheckPlayerLastBullet(dp)) {
        return;
    }
    if (enBOSCheckDamage(dp) == 2) {
        return;
    }
    switch (dp->sslv) {
    case 0:
        if (dp->anim != 8 && dp->anim != 1) {
            enBOSAnimeSet(dp, 1);
        }
        dp->sslv++;
    }
    shSinCosV_Scale(vec, enGetPlayerAngle(dp), 600.0f);
    _shAddVector(pos, enGetPlayerPos(dp), vec);
    if (enCheckPlayerHitEyes(dp, pos) >= 0.0f) {
        _shSubVector(pos, enGetPlayerPos(dp), vec);
    }
    a = enCalcDirection(pos, (float *)&dp->scp->pos);
    a = shAngleRegulate(a - dp->path.angle);
    dp->bos.rot_add = 0.05f * a;
    switch (enBOSCanAttackPlayer(dp)) {
    case 1:
        EN_SET_LEVEL(dp, 4);
        break;
    case 2:
        if (enBOSCheckFloor(dp)) {
            EN_SET_LEVEL(dp, 4);
        } else {
            EN_SET_LEVEL(dp, 3);
        }
        break;
    default:
        if (dp->p_dist >= 1500.0f && enCheckIntoScreen(dp)) {
            if (dp->bos.insect_dp) {
                if (((struct EnLOCAL_DATA *)dp->bos.insect_dp)->kind) {
                    break;
                }
                dp->bos.insect_dp = NULL;
            }
            EN_SET_LEVEL(dp, 2);
        }
        break;
    }
    enBOSCheckNearPlayer(dp);
}

static void enBOSCtrlDistance(struct EnLOCAL_DATA *dp) {
    float pos[4];
    float vec[4];
    float a;

    if (enBOSCheckPlayerLastBullet(dp)) {
        return;
    }
    switch (enBOSCheckDamage(dp)) {
    case 2:
        return;
    case 1:
        EN_SET_LEVEL(dp, 0);
        return;
    }
    switch (dp->sslv) {
    case 0:
        if (dp->anim != 8 && dp->anim != 1) {
            enBOSAnimeSet(dp, 1);
        }
        enSetTimer(dp, 120);
        dp->sslv++;
    }
    shSinCosV_Scale(vec, enGetPlayerDirection(dp), 1500.0f);
    _shSubVector(pos, enGetPlayerPos(dp), vec);
    a = enCalcDirection(pos, (float *)&dp->scp->pos);
    a = shAngleRegulate(a - dp->path.angle);
    dp->bos.rot_add = 0.05f * a;
    if (enReduceTimer(dp) <= 0 || enBOSCanAttackPlayer(dp)) {
        EN_SET_LEVEL(dp, 0);
    }
    enBOSCheckNearPlayer(dp);
}

static void enBOSCtrlGenerate(struct EnLOCAL_DATA *dp) {
    int i;
    struct EnLOCAL_DATA *tp;
    float pos[4];
    float vec[4];
    float a;
    struct SubCharacter *tscp;

    if (enBOSCheckDamage(dp) == 2) {
        return;
    }
    tp = dp->bos.insect_dp;
    if (enBOSCheckPlayerLastBullet(dp)) {
        return;
    }
    switch (dp->sslv) {
    case 0:
        if (dp->anim != 8 && dp->anim != 1) {
            enBOSAnimeSet(dp, 1);
        }
        if (!(tp = enEntryEnemy(15))) {
            EN_SET_LEVEL(dp, 0);
            return;
        }
        tscp = enINSGetSubCharacter(tp);
        vcopy(&dp->scp->pos, &tscp->pos);
        vzero(&tscp->rot);
        tscp->rot.y = a = enGetPlayerDirection(dp);
        enInitData(tp, tscp);
        dp->bos.insect_dp = tp;
        for (i = 0; i < 32; i++) {
            if (!(tp = enEntryEnemy(15))) {
                break;
            }
            tscp = enINSGetSubCharacter(tp);
            vcopy(&dp->scp->pos, &tscp->pos);
            tscp->rot.y = a;
            enInitData(tp, tscp);
        }
        tp = dp->bos.insect_dp;
        dp->bos.mode = 1;
        enAttackStart(dp);
        dp->sslv++;
        break;
    case 1:
        if (tp->slv == 1) {
            enAttackCheck(dp, 0x3E);
            if (!(dp->flag & 4)) {
                enAttackStart(dp);
                dp->sslv++;
            }
        } else if (tp->slv == 2) {
            EN_SET_LEVEL(dp, 1);
        }
        break;
    case 2:
        enAttackCheckHug(dp, 0x3F);
        if (!enCheckHuggedPlayer()) {
            tp->slv = 2;
            dp->sslv++;
        }
        break;
    case 3:
        if (enBOSCanAttackPlayer(dp) == 2) {
            if (!enBOSCheckFloor(dp)) {
                EN_SET_LEVEL(dp, 3);
                return;
            } else {
                EN_SET_LEVEL(dp, 4);
                return;
            }
        }
        break;
    }
    shSinCosV_Scale(vec, enGetPlayerAngle(dp), 600.0f);
    _shAddVector(pos, enGetPlayerPos(dp), vec);
    if (enCheckPlayerHitEyes(dp, pos) >= 0.0f) {
        _shSubVector(pos, enGetPlayerPos(dp), vec);
    }
    a = enCalcDirection(pos, (float *)&dp->scp->pos);
    a = shAngleRegulate(a - dp->path.angle);
    dp->bos.rot_add = 0.05f * a;
    if (!tp->kind) {
        EN_SET_LEVEL(dp, 0);
        dp->bos.mode = 0;
        dp->bos.insect_dp = NULL;
    }
    enBOSCheckNearPlayer(dp);
}

static void enBOSCtrlAttack(struct EnLOCAL_DATA *dp) {
    float vec[4];

    if (enCheckDamage(dp)) {
        if (enCheckHuggedPlayer()) {
            enResetDamage(dp);
        } else {
            if (enBOSCheckDamage(dp) == 2) {
                return;
            }
            EN_SET_LEVEL(dp, 1);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        enBOSAnimeSet(dp, 2);
        enAttackStart(dp);
        dp->flag |= 0x8000;
        enFlagSetCritical(dp);
        dp->bos.mode = 2;
        dp->sslv++;
        break;
    case 1:
        shSinCosV_Scale(vec, enGetPlayerAngle(dp), 600.0f);
        _shAddVector(vec, enGetPlayerPos(dp), vec);
        _shSubVector(dp->vec, vec, (float *)&dp->scp->pos);
        dp->vec[1] = dp->vec[3] = 0.0f;
        enAttackCheck(dp, 0x3B);
        if (dp->anim_n == -1) {
            if (dp->flag & 4) {
                enBOSAnimeSet(dp, 6);
                dp->sslv = 4;
            } else {
                enBOSAnimeSet(dp, 3);
                enAttackStart(dp);
                dp->sslv++;
            }
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            enBOSAnimeSet(dp, 4);
            dp->flag &= ~0x8000;
            dp->sslv++;
        }
        break;
    case 3:
        enAttackCheckHug(dp, 0x3C);
        if (!enCheckHuggedPlayer()) {
            enBOSAnimeSet(dp, 5);
            dp->sslv++;
        }
        break;
    case 4:
        if (dp->anim_n == -1) {
            EN_SET_LEVEL(dp, 1);
            dp->flag &= ~0x8000;
            enFlagResetCritical(dp);
            enBOSResetSpeed(dp);
        }
        break;
    }
}

static void enBOSCtrlAttack2(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        if (enBOSCheckDamage(dp) == 2) {
            return;
        }
        EN_SET_LEVEL(dp, 1);
        return;
    }
    switch (dp->sslv) {
    case 0:
        enBOSAnimeSet(dp, 7);
        enAttackStart(dp);
        enFlagSetCritical(dp);
        dp->sslv++;
    case 1:
        enAttackCheck(dp, 0x3D);
        if (dp->anim_n == -1) {
            EN_SET_LEVEL(dp, 0);
            enFlagResetCritical(dp);
        }
    }
    enBOSCheckNearPlayer(dp);
}

/* The `return` after the timer test gives the original's branch to the `b` over the else part. */

static void enBOSCtrlDead(struct EnLOCAL_DATA *dp) {
    if (dp->scp->en_first_status == 13) {
        if (dp->sslv == 1) {
            if (enReduceTimer(dp) <= 0) {
                enResetFilter();
                game_flag.flag[15] |= 0x10000000;
            }
            return;
        } else if (enCheckDamage(dp)) {
            enKillCountUp(dp);
            enSetFadeOut();
            enSetTimer(dp, 120);
            dp->flag |= 8;
            dp->sslv++;
        }
    } else {
        if (dp->sslv == 0) {
            enSetTimer(dp, 60);
            enSetBlur();
            dp->flag |= 8;
            dp->sslv++;
        }
        if (enReduceTimer(dp) <= 0) {
            enResetFilter();
            game_flag.flag[15] |= 0x4000000;
        }
    }
}

static int enBOSCheckPlayerLastBullet(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;

    if (!enCheckPlayerBulletEmpty()) {
        return 0;
    }
    if ((dp->bos.end_count -= shGetDT()) > 0) {
        return 0;
    }
    tp = dp->bos.insect_dp;
    if (tp && tp->kind && tp->slv <= 1) {
        tp->slv = 2;
    }
    game_flag.flag[15] |= 0x4000000;
    return 1;
}

static int enBOSCanAttackPlayer(struct EnLOCAL_DATA *dp) {
    float dist;
    float a;

    dist = dp->p_dist = enGetPlayerDistance(dp);
    a = enCalcAngleDifference(enGetPlayerDirection(dp), dp->scp->rot.y);
    if (dist < 700.0f && dist > 500.0f && a < 0.08726646f &&
        enCalcAngleDifference(enGetPlayerAngle(dp), dp->scp->rot.y) > 1.3962634f && !enCheckNoDamageHuman(dp)) {
        return 2;
    }
    if (dist < 600.0f && a < 0.08726646f && !enCheckNoDamageHuman(dp)) {
        return 1;
    }
    return 0;
}

static void enBOSResetSpeed(struct EnLOCAL_DATA *dp) {
    dp->bos.move_speed = 0.0f;
    dp->bos.y_speed = 0.0f;
    dp->bos.rot_add = 0.0f;
    dp->bos.rot_speed = 0.0f;
    dp->bos.mode = 0;
}

static int enBOSCheckDamage(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;

    if (!enCheckDamage(dp)) {
        return 0;
    }
    if (shPadGetPort() == 6 && shPadPress(0, 0x40000)) {
        dp->scp->battle.hp = dp->scp->battle.hp_rate = dp->endurance = 0.0f;
    }
    if (dp->last_atk >= 21 && dp->last_atk <= 22) {
        dp->scp->battle.damage *= 10.0f;
    }
    enReduceHP(dp);
    enSetHitBack(dp);
    dp->flag &= ~0x8000;
    if (enCheckDeath(dp)) {
        tp = dp->bos.insect_dp;
        if (tp && tp->kind && tp->slv <= 1) {
            tp->slv = 2;
        }
        EN_SET_LEVEL(dp, 5);
        return 2;
    }
    enBOSAnimeSet(dp, 8);
    return 1;
}

float bed_pos[4] = { 58900.0f, -16000.0f, 58800.0f, 800.0f };
float stair_pos[4] = { 60900.0f, -16000.0f, 55600.0f, 900.0f };

static int enBOSCheckFloor(struct EnLOCAL_DATA *dp) {
    if (distXZ(bed_pos, (float *)&dp->scp->pos) < bed_pos[3] ||
        distXZ(stair_pos, (float *)&dp->scp->pos) < stair_pos[3]) {
        return 1;
    }
    return 0;
}

static void enBOSCheckNearPlayer(struct EnLOCAL_DATA *dp) {
    enCheckNearPlayer(dp, &dp->bos.near_count, &dp->bos.dist, 200.0f);
}

/* Matching: fitted stand-in for float code (docs/stand-ins.md); it makes enBOSMoveExec materialize
 * 0.1f (f13) before -250.0f (f12) for shSway1f, as the original does (as in flyMove). */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f; }
static void enBOSMoveExec(struct EnLOCAL_DATA *dp) {
    float add;
    float d;
    float s;
    int mode;
    struct EnPATH_DATA tpath;

    mode = dp->bos.mode;
    if (mode <= 1) {
        dp->bos.rot_speed = 0.95f * dp->bos.rot_speed + dp->bos.rot_add;
        dp->path.markangle = dp->path.angle + dp->bos.rot_speed;
        enMoveAngle(&dp->path, 0.034906585f);
        add = 250.0f * shGetDT();
        d = dp->bos.move_speed;
        if (dp->anim == 8) {
            if ((d -= add) < 0.0f) {
                d = 0.0f;
            }
            mode = 1;
            if (dp->anim_n == -1) {
                enBOSAnimeSet(dp, mode);
            }
        } else {
            s = enBOSGetMoveSpeed();
            d += add;
            if (d > s) {
                d = s;
            }
        }
        dp->bos.move_speed = d;
        shSinCosV_Scale(dp->vec, dp->path.angle, d);
    }
    tpath.markangle = enGetPlayerDirection(dp);
    tpath.angle = dp->scp->rot.y;
    enMoveAngle(&tpath, 0.017453292f);
    dp->scp->rot.y = tpath.angle;
    dp->bos.y_speed = 0.98f * (dp->bos.y_speed + (mode ? 0.2f * (-17500.0f - dp->scp->pos.y) : shSway1f(-250.0f, 0.1f)) -
                               0.01f * (dp->scp->pos.y - -17500.0f));
    if ((dp->scp->pos.y < -17700.0f && dp->bos.y_speed < 0.0f) ||
        (dp->scp->pos.y >= -17300.0f && dp->bos.y_speed > 0.0f)) {
        dp->bos.y_speed *= 0.5f;
    }
    if (dp->scp->pos.y <= -17300.0f || dp->bos.y_speed < 0.0f) {
        dp->vec[1] = dp->bos.y_speed;
    }
    enMoveExec(dp);
}

static void enBOSAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        enAnimeRestart(dp);
    } else {
        /* Matching: the assert bakes its original line number into the object. */
#line 725
        fjAssert(anim >= 0 && anim < sizeof(EnBOSAnime) / sizeof(EnANIME_DATA));
        enAnimeSet(dp, anim, EnBOSAnime[anim].Anime);
    }
}

static void enBOSAnimeExec(struct EnLOCAL_DATA *dp) {
    enAnimeExec(dp, EnBOSAnime, 0x16A9);
    if (dp->anim_n == -1 || (dp->flag & 1)) {
        return;
    }
    if (dp->anim == 2) {
        if (dp->anim_n >= 0x7080 && dp->anim_n < 0x8340 && (dp->flag & 4)) {
            dp->anim_s = 0x400;
        } else {
            dp->anim_s = 0x1000;
        }
    }
}

static float enBOSGetMoveSpeed(void) {
    float speed_rate[5] = { 0.7f, 0.8f, 1.0f, 1.5f, 2.0f };

    return 500.0f * speed_rate[enGetMode()];
}

/* Matching: fitted stand-in for float code (docs/stand-ins.md); it makes enBOSSoundSigns
 * materialize 0.4f (f12) before signs + 0x49AE (a0), as the original does. */
static float __stripped_float_code_2(float x) { return x + 3.0f + 5.0f; }
static void enBOSSoundSigns(struct EnLOCAL_DATA *dp) {
    int signs;

    if ((dp->bos.count -= shGetDF()) <= 0) {
        dp->bos.count = enCalcTimer(60);
        do {
            signs = (shRandI() >> 20) & 3;
        } while (signs == dp->bos.signs);
        dp->bos.signs = signs;
        if (signs) {
            enSoundCall(signs + 0x49AE, 0.4f, (float *)&dp->scp->pos);
        } else {
            enSoundCall3D(0x49AE, 0.4f, (float *)&dp->scp->pos);
        }
    }
}
