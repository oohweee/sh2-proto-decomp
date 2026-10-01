/*
 * en_oni.c: AI of the ONI enemy: wanders, chases, attacks (alone or with another ONI),
 * goes berserk or confused, dies or kills itself. Shared enemy code is in en_common.c.
 * Pyramid Head, the spear pair at the end of the hotel (verified; the hospital chase is suspected; docs/characters.md).
 */
#include "enemy.h"

static void enONICtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enONICtrlSleep(struct EnLOCAL_DATA *dp);
static void enONICtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enONICtrlEvent(struct EnLOCAL_DATA *dp);
static void enONICtrlEventChase(struct EnLOCAL_DATA *dp);
static void enONICtrlHand(struct EnLOCAL_DATA *dp);
static void enONICtrlWander(struct EnLOCAL_DATA *dp);
static void enONICtrlChase(struct EnLOCAL_DATA *dp);
static void enONICtrlBothAttack(struct EnLOCAL_DATA *dp);
static void enONICtrlWait(struct EnLOCAL_DATA *dp);
static void enONICtrlBerserk(struct EnLOCAL_DATA *dp);
static void enONICtrlAttack(struct EnLOCAL_DATA *dp);
static void enONICtrlAttack3(struct EnLOCAL_DATA *dp);
static void enONICtrlDamage(struct EnLOCAL_DATA *dp);
static void enONICtrlConfuse(struct EnLOCAL_DATA *dp);
static void enONICtrlSuicide(struct EnLOCAL_DATA *dp);
static void enONICtrlDead(struct EnLOCAL_DATA *dp);
static void enONICheckPlayerWeapon(struct EnLOCAL_DATA *dp);
static int enONIGetDamageMotion(struct EnLOCAL_DATA *dp);
static int enONISetDamage(struct EnLOCAL_DATA *dp);
static int enONICanSeePlayer(struct EnLOCAL_DATA *dp);
static int enONICanSeeCharacter(struct EnLOCAL_DATA *dp, struct SubCharacter *scp);
static void enONIAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enONIAnimeReset(struct EnLOCAL_DATA *dp, int anim);
static void enONIAnimeExec(struct EnLOCAL_DATA *dp);
static float enONIGetSpeed(struct EnLOCAL_DATA *dp);
static float enONIGetWalkSpeed(struct EnLOCAL_DATA *dp);
static float enONIGetAttackSpeed(struct EnLOCAL_DATA *dp);
static float enONIGetFeelRange(void);
static float enONIGetRotSpeed(void);
static void enONISetSlowTime(struct EnLOCAL_DATA *dp);
static void enONISoundLife(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnONIAnime[16] = {
    { 0x1519, 0 }, { 0x151A, 1 }, { 0x1519, 1 }, { 0x151B, 0 }, { 0x1522, 1 }, { 0x1521, 0 },
    { 0x1523, 0 }, { 0x151C, 0 }, { 0x151D, 0 }, { 0x151E, 0 }, { 0x151F, 0 }, { 0x1520, 0 },
    { 0x1524, 0 }, { 0x1525, 0 }, { 0x1526, 0 }, { 0x1527, 0 },
};

float check_point[13][4] = {
    { 16100.0f, 0.0f, -260300.0f, 0.0f },
    { 16000.0f, 0.0f, -258800.0f, 0.0f },
    { 16100.0f, 0.0f, -258100.0f, 0.0f },
    { 19200.0f, 0.0f, -258000.0f, PI / 2 },
    { 19900.0f, 0.0f, -257900.0f, 0.0f },
    { 19900.0f, 0.0f, -255700.0f, 0.0f },
    { 19200.0f, 0.0f, -255600.0f, -PI / 2 },
    { 16100.0f, 0.0f, -255500.0f, 0.0f },
    { 16100.0f, 0.0f, -254500.0f, 0.0f },
    { 18300.0f, 0.0f, -254300.0f, 0.0f },
    { 18400.0f, 0.0f, -252400.0f, 0.0f },
    { 18500.0f, 0.0f, -251700.0f, 0.0f },
    { 34000.0f, 0.0f, -251600.0f, 0.0f },
};

char warp_check[4] = { 1, 3, 6, 10 };

float suicide_point[2][4] = {
    { -19344.45f, 0.0f, 21723.15f, -0.5235988f },
    { -19622.8f, 0.0f, 22746.35f, 0.7853982f },
};
/* Matching: fitted stand-in (not recovered code); it sets the next function's float-constant
 * order. Whether the original had code here: docs/stand-ins.md. */
static float __stripped_float_code_0(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f + 53.0f + 55.0f + 57.0f + 59.0f + 61.0f + 63.0f + 65.0f + 67.0f + 69.0f + 71.0f + 73.0f + 75.0f + 77.0f + 79.0f + 81.0f + 83.0f + 85.0f + 87.0f + 89.0f + 91.0f + 93.0f + 95.0f + 97.0f + 99.0f + 101.0f + 103.0f + 105.0f + 107.0f + 109.0f + 111.0f + 113.0f + 115.0f + 117.0f + 119.0f + 121.0f + 123.0f + 125.0f + 127.0f + 129.0f + 131.0f + 133.0f + 135.0f + 137.0f + 139.0f + 141.0f + 143.0f + 145.0f + 147.0f + 149.0f + 151.0f + 153.0f + 155.0f + 157.0f + 159.0f + 161.0f + 163.0f + 165.0f + 167.0f + 169.0f + 171.0f + 173.0f + 175.0f + 177.0f + 179.0f + 181.0f + 183.0f + 185.0f + 187.0f + 189.0f + 191.0f + 193.0f + 195.0f + 197.0f + 199.0f + 201.0f + 203.0f + 205.0f + 207.0f + 209.0f + 211.0f + 213.0f + 215.0f + 217.0f + 219.0f + 221.0f + 223.0f + 225.0f + 227.0f + 229.0f + 231.0f + 233.0f + 235.0f + 237.0f + 239.0f + 241.0f + 243.0f + 245.0f + 247.0f + 249.0f + 251.0f + 253.0f + 255.0f + 257.0f + 259.0f + 261.0f + 263.0f + 265.0f + 267.0f + 269.0f + 271.0f + 273.0f + 275.0f + 277.0f + 279.0f + 281.0f + 283.0f + 285.0f + 287.0f + 289.0f + 291.0f + 293.0f + 295.0f + 297.0f + 299.0f + 301.0f + 303.0f + 305.0f + 307.0f + 309.0f + 311.0f + 313.0f + 315.0f + 317.0f + 319.0f + 321.0f + 323.0f + 325.0f + 327.0f + 329.0f + 331.0f + 333.0f + 335.0f + 337.0f + 339.0f + 341.0f + 343.0f + 345.0f + 347.0f + 349.0f + 351.0f + 353.0f + 355.0f + 357.0f + 359.0f + 361.0f + 363.0f + 365.0f + 367.0f + 369.0f + 371.0f + 373.0f + 375.0f + 377.0f + 379.0f + 381.0f + 383.0f + 385.0f + 387.0f + 389.0f + 391.0f + 393.0f + 395.0f + 397.0f + 399.0f; } /* fitted, not recovered: 199 constants */
/** Sets up a new ONI: HP by difficulty mode, size, starting level from the spawn status.
 * @param dp enemy work */
void enONIInitData(struct EnLOCAL_DATA *dp) {
    float vitarity[5] = { 3000.0f, 7500.0f, 10000.0f, 20000.0f, 40000.0f };
    float endurance[5] = { 500.0f, 700.0f, 1000.0f, 2000.0f, 5000.0f };
    int mode;
    float hp;
    int i;
    int n;
    struct EnLOCAL_DATA *tp;

    mode = enGetMode();
    dp->oni.other = NULL;
    dp->mlv = 1;
    dp->weight = 3;
    enSetSize(dp, 250.0f, 900.0f, 800.0f, 850.0f);
    switch (dp->scp->en_first_status) {
    case 15:
        enSetSize(dp, 350.0f, 900.0f, 800.0f, 850.0f);
        dp->mlv = 4;
        dp->slv = 0;
        dp->weight = 4;
        hp = 1000000.0f;
        enFlagSetMoved(dp);
        enSoundCall(0x3EF7, 1.0f, (float *)&dp->scp->pos);
        break;
    case 6:
        dp->slv = 1;
        dp->sslv = 0;
        dp->type = 2;
        hp = vitarity[mode];
        n = 0;
        tp = enLocalWork.Data;
        for (i = 0; i < 32; i++, tp++) {
            if (tp->kind == 5 && tp != dp) {
                tp->oni.other = dp;
                dp->oni.other = tp;
                n++;
            }
        }
        dp->oni.id = n;
        enSetBattleTarget(dp, 1);
        break;
    case 13:
        EN_SET_LEVEL(dp, 10);
        hp = 0.0f;
        break;
    default:
        dp->slv = 0;
        dp->sslv = 0;
        dp->type = 0;
        hp = 1000000.0f;
        enFlagSetMoved(dp);
        break;
    }
    enSetHP(dp, hp, endurance[mode]);
    if (enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

/** Per-frame control of a ONI: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enONICtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlONIFunc[6])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enONICtrlAutomatic, enONICtrlSleep, enONICtrlGoPlayable, enONICtrlEvent, enONICtrlHand,
    };

    enCtrlONIFunc[dp->mlv](dp);
}

static void enONICtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlONISubFunc[11])(struct EnLOCAL_DATA *) = {
        enONICtrlWander, enONICtrlChase,  enONICtrlBothAttack, enONICtrlWait,    enONICtrlBerserk, enONICtrlAttack,
        enONICtrlAttack3, enONICtrlDamage, enONICtrlConfuse,    enONICtrlSuicide, enONICtrlDead,
    };

    enSetBattleTarget(dp, 0);
    enONICheckPlayerWeapon(dp);
    enCtrlONISubFunc[dp->slv](dp);
    if (enReduceTimer(dp) <= 0) {
        dp->endurance += 200.0f * shGetDT();
        if (dp->endurance > dp->endurance_max) {
            dp->endurance = dp->endurance_max;
        }
    }
    enONIAnimeExec(dp);
    enMoveExec(dp);
    if (dp->type != 2 && enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

static void enONICtrlSleep(struct EnLOCAL_DATA *dp) {
    if (enCheckSleepOut(dp)) {
        enSleepOut(dp);
        dp->slv = 0;
        dp->sslv = 0;
        dp->type = 0;
    }
}

static void enONICtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    EN_SET_LEVEL(dp, 1);
}

static void enONICtrlEvent(struct EnLOCAL_DATA *dp) {
    int n;

    enSetBattleTarget(dp, 0);
    switch (dp->slv) {
    case 0:
        enONICtrlEventChase(dp);
        break;
    case 4:
        n = dp->sslv;
        if (dp->oni.check <= warp_check[n]) {
            dp->oni.warp = n;
        }
        dp->slv = 0;
        dp->sslv = 0;
        break;
    }
    enONIAnimeExec(dp);
    enMoveExec(dp);
}

static void enONICtrlEventChase(struct EnLOCAL_DATA *dp) {
    float *tpos;
    float d1;
    float d2;

    if (enCheckDamage(dp)) {
        switch (dp->last_atk) {
        case 2:
        case 1:
        case 12:
        case 13:
        case 14:
            break;
        default:
            enSetHitBack(dp);
            break;
        }
        enResetDamage(dp);
    }
    switch (dp->sslv) {
    case 0:
        enONIAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->oni.warp = -1;
        dp->anim_s = 0x1800;
        dp->sslv++;
        break;
    case 1:
        tpos = check_point[dp->oni.check];
        dp->path.markangle = enCalcDirection(tpos, (float *)&dp->scp->pos);
        enMoveAngle(&dp->path, enONIGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        d1 = enGetPlayerDistance(dp);
        d2 = enDistXZ(tpos, (float *)&dp->scp->pos);
        if (d2 < 150.0f) {
            dp->oni.check++;
            if (dp->oni.warp != -1 && dp->oni.check > warp_check[dp->oni.warp]) {
                dp->oni.warp = -1;
            }
        }
        if (!enCheckIntoScreen(dp)) {
            if (d1 > 2500.0f || dp->oni.warp != -1) {
                shSinCosV_Scale(dp->vec, dp->scp->rot.y, 1500.0f);
            }
        }
        if (d1 < 625.0f && enCheckPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos) < 0.0f) {
            enONIAnimeSet(dp, 3);
            enONIGetAttackSpeed(dp);
            dp->anim_s /= 2;
            enAttackStart(dp);
            dp->sslv++;
        }
        break;
    case 2:
        enAttackCheck(dp, 0x31);
        enMoveAngleToPlayer(dp, enONIGetRotSpeed());
        if (dp->anim_n == -1) {
            dp->sslv = 0;
        }
        break;
    }
}

static void enONICtrlHand(struct EnLOCAL_DATA *dp) {
}

static void enONICtrlWander(struct EnLOCAL_DATA *dp) {
    float vec[4];
    int t;

    if (enCheckDamage(dp)) {
        if (enONISetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 8);
            return;
        }
    }
    if (!dp->sslv) {
        enONIAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    enONIGetWalkSpeed(dp);
    shSinCosV_Scale(vec, dp->scp->rot.y, 1500.0f);
    _shAddVector(vec, (float *)&dp->scp->pos, vec);
    enSetPath(dp, vec, (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, enONIGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    t = enONICanSeePlayer(dp);
    if (t >= 2) {
        EN_SET_LEVEL(dp, 5);
    } else if (t == 1) {
        EN_SET_LEVEL(dp, 1);
    }
}

static void enONICtrlChase(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        if (enONISetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 8);
            return;
        }
    }
    if (!dp->sslv) {
        enONIAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    enSetPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, enONIGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    dp->anim_s = ftoi(4096.0f * enONIGetWalkSpeed(dp) *
                      enCalcSpeedRate(dp->scp->rot.y, (float *)&dp->scp->pos, enGetPlayerPos(dp)));
    if (dp->type == 2 && enGetMode() >= 2 &&
        enCalcAngleDifference(dp->scp->rot.y, enGetPlayerDirection(dp)) < 0.08726646f) {
        dp->anim_s = dp->anim_s * 3 / 2;
    }
    t = enONICanSeePlayer(dp);
    if (t == 2) {
        EN_SET_LEVEL(dp, 5);
    } else if (t == 4) {
        EN_SET_LEVEL(dp, 6);
    } else if (t == 0 && dp->type != 2) {
        EN_SET_LEVEL(dp, 0);
    } else if (t != 0 && dp->oni.other && ((struct EnLOCAL_DATA *)dp->oni.other)->slv == 1 &&
               dp->p_dist > ((struct EnLOCAL_DATA *)dp->oni.other)->p_dist) {
        dp->slv = 2;
        dp->sslv = 0;
    }
}

static void enONICtrlBothAttack(struct EnLOCAL_DATA *dp) {
    float vec[4];
    float a;
    float a1;
    float a2;
    int t;

    if (enCheckDamage(dp)) {
        if (enONISetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 8);
            return;
        }
    }
    if (dp->anim == 1) {
        enONIGetWalkSpeed(dp);
        dp->anim_s = dp->anim_s * 6 / 5;
    }
    t = enONICanSeePlayer(dp);
    if (t == 2) {
        if (enDistXZ((float *)&dp->scp->pos, (float *)&((struct EnLOCAL_DATA *)dp->oni.other)->scp->pos) > 750.0f) {
            EN_SET_LEVEL(dp, 6);
        } else {
            EN_SET_LEVEL(dp, 5);
        }
        return;
    }
    if (t == 4) {
        EN_SET_LEVEL(dp, 6);
        return;
    }
    if (dp->p_dist > 4000.0f) {
        EN_SET_LEVEL(dp, 1);
        return;
    }
    if (dp->p_dist < ((struct EnLOCAL_DATA *)dp->oni.other)->p_dist) {
        EN_SET_LEVEL(dp, 1);
        return;
    }
    a = enGetPlayerDirection(dp);
    a1 = enGetPlayerDirection(dp->oni.other);
    a2 = shAngleRegulate(a - a1);
    switch (dp->sslv) {
    case 0:
        if (dp->p_dist < 500.0f) {
            enONIAnimeSet(dp, 2);
            dp->sslv = 4;
            break;
        }
        enONIAnimeSet(dp, 1);
        enONIGetWalkSpeed(dp);
        dp->sslv++;
    case 1:
        shSinCosV_Scale(vec, a1 - 1.0471976f * _shSign(a2), 2500.0f);
        _shAddVector(vec, enGetPlayerPos(dp), vec);
        enSetPath(dp, vec, (float *)&dp->scp->pos);
        enMoveAngle(&dp->path, enONIGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        if (fabsf(a2) > 1.7453293f) {
            dp->sslv = 2;
        }
        break;
    case 2:
        shSinCosV_Scale(vec, a1, 500.0f);
        _shAddVector(vec, enGetPlayerPos(dp), vec);
        if (enSetPath(dp, vec, (float *)&dp->scp->pos)) {
            dp->sslv = 3;
        }
        if (dp->p_dist < 500.0f) {
            dp->sslv = 3;
        }
        enMoveAngle(&dp->path, enONIGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        break;
    case 3:
        enMoveAngleToPlayer(dp, enONIGetRotSpeed());
        if (dp->p_dist > 2500.0f) {
            dp->sslv = 1;
        } else if (dp->p_dist < 500.0f) {
            enONIAnimeSet(dp, 2);
            dp->sslv = 4;
        }
        break;
    case 4:
        enMoveAngleToPlayer(dp, enONIGetRotSpeed());
        if (dp->p_dist > 500.0f && dp->anim_loop > 1) {
            enONIAnimeSet(dp, 1);
            dp->sslv = 1;
        }
        break;
    }
}

static void enONICtrlWait(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        if (enONISetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 8);
            return;
        }
    }
    enONIGetWalkSpeed(dp);
    t = enONICanSeePlayer(dp);
    if (t == 2) {
        if (enDistXZ((float *)&dp->scp->pos, (float *)&((struct EnLOCAL_DATA *)dp->oni.other)->scp->pos) > 750.0f) {
            EN_SET_LEVEL(dp, 6);
        } else {
            EN_SET_LEVEL(dp, 5);
        }
        return;
    }
    if (t == 4) {
        EN_SET_LEVEL(dp, 6);
        return;
    }
    if (dp->p_dist > 2500.0f) {
        EN_SET_LEVEL(dp, 1);
        return;
    }
    if (enCalcAngleDifference(enGetPlayerDirection(dp), enGetPlayerDirection(dp->oni.other)) < 1.7453293f) {
        EN_SET_LEVEL(dp, 2);
        return;
    }
    if (!dp->sslv) {
        enONIAnimeSet(dp, 2);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    enMoveAngleToPlayer(dp, enONIGetRotSpeed());
}

static void enONICtrlBerserk(struct EnLOCAL_DATA *dp) {
    float vec[4];
    float d;
    int t;
    struct SubCharacter *scp;
    float *pos;

    if (enCheckDamage(dp)) {
        if (enONISetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 8);
            return;
        }
    }
    if (!dp->sslv) {
        enONIAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    enONIGetWalkSpeed(dp);
    scp = enGetNearCharacter(dp);
    t = enONICanSeeCharacter(dp, scp);
    if (t == 2) {
        EN_SET_LEVEL(dp, 5);
    }
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
    enMoveAngle(&dp->path, enONIGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
}

static void enONICtrlAttack(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        if (enONISetDamage(dp)) {
            EN_SET_LEVEL(dp, 7);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        enONIAnimeSet(dp, 3);
        enInitPath(&dp->path, dp->scp->rot.y);
        enFlagSetCritical(dp);
        enAttackStart(dp);
        dp->sslv++;
        break;
    case 1:
        enAttackCheck(dp, 0x31);
        if (enGetMode() >= 3) {
            enMoveAngleToPlayer(dp, enONIGetRotSpeed());
        }
        if (dp->anim_n == -1) {
            if (!(dp->flag & 4) && dp->type == 1) {
                dp->type = 0;
            }
            enFlagResetCritical(dp);
            if (dp->type == 1) {
                EN_SET_LEVEL(dp, 4);
            } else if (dp->type == 0) {
                EN_SET_LEVEL(dp, 0);
            } else {
                EN_SET_LEVEL(dp, 1);
                if (dp->oni.other && enONICanSeePlayer(dp->oni.other) >= 2) {
                    EN_SET_LEVEL(dp, 3);
                }
            }
        }
        break;
    }
    enONIGetAttackSpeed(dp);
}

static void enONICtrlAttack3(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        enONISetDamage(dp);
        enSetTimer(dp, 0);
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 8);
            return;
        }
    }
    switch (dp->sslv) {
    case 0:
        enONIAnimeSet(dp, 5);
        enFlagSetCritical(dp);
        enAttackStart(dp);
        dp->sslv++;
        break;
    case 1:
        if (dp->anim_n == -1) {
            enONIAnimeSet(dp, 4);
            dp->oni.timer2 = ((shRandI() >> 12) & 3) + 2;
            dp->sslv++;
        }
        break;
    case 2:
        enAttackCheck(dp, 0x32);
        if (dp->anim_loop >= dp->oni.timer2 || !(dp->flag & 4) ||
            (dp->p_dist > 2500.0f && dp->anim_loop > 0 && dp->anim_n < 48000) || dp->anim_step > 0) {
            enONIAnimeSet(dp, 6);
            dp->sslv++;
        }
        break;
    case 3:
        if (dp->anim_n == -1) {
            enFlagResetCritical(dp);
            dp->slv = 1;
            dp->sslv = 0;
        }
        break;
    }
    enONIGetAttackSpeed(dp);
}

static void enONICtrlDamage(struct EnLOCAL_DATA *dp) {
    int m;
    float d0;
    float d1;
    float d2;
    float d3;
    float d4;
    int id;
    int n1;
    int n2;

    switch (dp->sslv) {
    case 0:
        if (dp->type == 2) {
            dp->endurance = dp->endurance_max;
        } else {
            enSetHP(dp, 1000000.0f, 1000000.0f);
        }
        m = enONIGetDamageMotion(dp);
        /* Matching: the assert bakes its original line number into the object. */
#line 826
        fjAssert(m != 0);
        enONIAnimeSet(dp, m);
        enFlagResetCritical(dp);
        dp->sslv++;
        break;
    case 1:
        if (enCheckDamage(dp)) {
            if (enONISetDamage(dp)) {
                dp->sslv = 0;
                break;
            }
        }
        if (dp->anim_n == -1) {
            if (dp->type == 2 && enCheckDeath(dp) && enCheckDeath(dp->oni.other)) {
                EN_SET_LEVEL(dp, 9);
                ((struct EnLOCAL_DATA *)dp->oni.other)->slv = 9;
                ((struct EnLOCAL_DATA *)dp->oni.other)->sslv = 0;
                d0 = enCalcDirection((float *)&((struct EnLOCAL_DATA *)dp->oni.other)->scp->pos, (float *)&dp->scp->pos);
                d1 = enCalcDirection(suicide_point[0], (float *)&dp->scp->pos);
                d2 = enCalcDirection(suicide_point[1], (float *)&dp->scp->pos);
                d3 = shAngleRegulate(d1 - d0);
                d4 = shAngleRegulate(d2 - d0);
                if (d3 * d4 < 0.0f) {
                    d0 = enDistXZ((float *)&((struct EnLOCAL_DATA *)dp->oni.other)->scp->pos, (float *)&dp->scp->pos);
                    d1 = enDistXZ(suicide_point[0], (float *)&dp->scp->pos);
                    d2 = enDistXZ(suicide_point[1], (float *)&dp->scp->pos);
                    d3 = enDistXZ(suicide_point[0], (float *)&((struct EnLOCAL_DATA *)dp->oni.other)->scp->pos);
                    d4 = enDistXZ(suicide_point[1], (float *)&((struct EnLOCAL_DATA *)dp->oni.other)->scp->pos);
                    if (d2 > d0 && d1 < d0) {
                        id = 0;
                    } else if (d1 > d0 && d2 < d0) {
                        id = 1;
                    } else {
                        n1 = 0;
                        if (d2 > d1) {
                            d1 = d2;
                            n1 = 1;
                        }
                        n2 = 0;
                        if (d4 > d3) {
                            d3 = d4;
                            n2 = 1;
                        }
                        if (n1 != n2 || d1 < d3) {
                            id = n1;
                        } else {
                            id = n1 ^ 1;
                        }
                    }
                } else {
                    if (fabsf(d3) > fabsf(d4)) {
                        id = 0;
                    } else {
                        id = 1;
                    }
                }
                dp->oni.id = id;
                ((struct EnLOCAL_DATA *)dp->oni.other)->oni.id = id ^ 1;
            } else {
                EN_SET_LEVEL(dp, 1);
            }
        }
        break;
    }
}

static void enONICtrlConfuse(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;

    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            if (dp->sslv != 4) {
                dp->sslv = 0;
            }
        } else {
            if (enONISetDamage(dp)) {
                EN_SET_LEVEL(dp, 7);
            } else {
                EN_SET_LEVEL(dp, 0);
            }
            return;
        }
    }
    if (!dp->sslv) {
        enReduceHP(dp);
        enONIAnimeSet(dp, 2);
        dp->sslv++;
    }
    if (dp->sslv >= 4) {
        tp = dp->oni.tp;
        dp->path.markangle = enCalcDirection((float *)&tp->scp->pos, (float *)&dp->scp->pos);
        enMoveAngle(&dp->path, enONIGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        enAttackCheck(dp, 0x31);
        if (dp->anim_n == -1) {
            if (++dp->sslv <= 5 && enDistXZ((float *)&tp->scp->pos, (float *)&dp->scp->pos) < 1250.0f) {
                enAttackStart(dp);
                enONIAnimeReset(dp, 3);
            } else {
                dp->slv = 0;
                dp->sslv = 0;
            }
        }
        return;
    }
    if (dp->sslv == 1 && dp->anim_n == -1) {
        tp = enGetNearOtherEnemy(dp);
        if (tp && enGetSprayPower() && fabsf(tp->scp->pos.y - ((float *)&dp->scp->pos)[1]) < 250.0f && enDistXZ((float *)&tp->scp->pos, (float *)&dp->scp->pos) < 1250.0f) {
            dp->oni.tp = tp;
            enAttackStart(dp);
            enONIAnimeSet(dp, 3);
            enInitPath(&dp->path, dp->scp->rot.y);
            dp->sslv = 4;
        } else {
            dp->sslv++;
            enSetTimer(dp, 600 - enGetMode() * 90);
        }
    } else if (dp->sslv == 2 &&
               enReduceTimer(dp) <= 0) {
        dp->slv = 0;
        dp->sslv = 0;
    }
    enONISoundLife(dp);
}

static void enONICtrlSuicide(struct EnLOCAL_DATA *dp) {
    float d;
    float a;
    int t;
    int td;
    int ts;

    if (enCheckDamage(dp)) {
        enResetDamage(dp);
    }
    switch (dp->sslv) {
    case 0:
        enONIAnimeSet(dp, 1);
        dp->weight = 4;
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
        break;
    case 2:
        enAttackCheck(dp, 0x31);
        enMoveAngleToPlayer(dp, enONIGetRotSpeed());
        if (dp->anim_n == -1) {
            dp->sslv = 0;
        }
        return;
    case 3:
    case 4:
        dp->path.markangle = suicide_point[dp->oni.id][3];
        enMoveAngle(&dp->path, enONIGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        _shSubVector(dp->vec, suicide_point[dp->oni.id], (float *)&dp->scp->pos);
        dp->vec[1] = dp->vec[3] = 0.0f;
        if (dp->scp->rot.y == dp->path.markangle) {
            dp->sslv = 4;
            if (((struct EnLOCAL_DATA *)dp->oni.other)->sslv == 4) {
                game_flag.flag[15] |= 8;
            }
        }
        return;
    }
    if ((d = enDistXZ(suicide_point[dp->oni.id], (float *)&dp->scp->pos)) < 50.0f) {
        enONIAnimeSet(dp, 2);
        dp->sslv = 3;
        return;
    }
    if (d > enDistXZ(suicide_point[((struct EnLOCAL_DATA *)dp->oni.other)->oni.id], (float *)&((struct EnLOCAL_DATA *)dp->oni.other)->scp->pos) + 100.0f) {
        t = 5000;
    } else {
        t = 4096;
    }
    ts = ftoi(4096.0f * shGetDT());
    td = t - dp->anim_s;
    if (td < -ts) {
        dp->anim_s -= ts;
    } else if (td > ts) {
        dp->anim_s += ts;
    } else {
        dp->anim_s = t;
    }
    a = enCalcDirection(suicide_point[dp->oni.id], (float *)&dp->scp->pos);
    dp->path.markangle = a;
    enMoveAngle(&dp->path, enONIGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    if (enCalcAngleDifference(a, enGetPlayerDirection(dp)) < 0.17453292f && dp->p_dist < 1250.0f) {
        enONIAnimeReset(dp, 3);
        enAttackStart(dp);
        dp->sslv = 2;
    }
}

static void enONICtrlDead(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        dp->weight = 4;
        enONIAnimeSet(dp, 15);
        enFlagSetNoDamage(dp);
        enFlagSetDead(dp);
        dp->sslv++;
    }
}

static void enONICheckPlayerWeapon(struct EnLOCAL_DATA *dp) {
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

static int enONIGetDamageMotion(struct EnLOCAL_DATA *dp) {
    int m;
    int id;
    int dd;
    float a;

    a = shAngleRegulate(shAtanV(dp->scp->battle.vec) - dp->scp->rot.y);
    if (fabsf(a) > PI / 2) {
        dd = 0;
    } else {
        dd = 1;
    }
    id = dp->last_atk;
    switch (id) {
    case 2:
    case 1:
    case 12:
    case 13:
    case 15:
    case 16:
    case 23:
    case 24:
    case 36:
    case 38:
    case 39:
        m = 0;
        break;
    case 4:
    case 6:
        m = dd + 7;
        break;
    case 14:
    case 17:
        m = 9;
        break;
    case 18:
        m = dd + 10;
        break;
    case 19:
    case 20:
        if (a < 0.0f) {
            m = 12;
        } else {
            m = 13;
        }
        break;
    case 21:
    case 22:
        m = 14;
        break;
    default:
        m = 0;
        printf("Illegal damage type!(%d)\n", id);
        break;
    }
    return m;
}

static int enONISetDamage(struct EnLOCAL_DATA *dp) {
    enONISetSlowTime(dp);
    if (dp->type != 2) {
        dp->endurance -= dp->scp->battle.damage;
        if (dp->endurance < 0.0f) {
            dp->endurance = 0.0f;
        }
        dp->scp->battle.damage = 0.0f;
    } else {
        if ((dp->scp->battle.hp -= dp->scp->battle.damage) < 0.0f) {
            dp->endurance = 0.0f;
            if ((((struct EnLOCAL_DATA *)dp->oni.other)->scp->battle.hp += 0.5f * dp->scp->battle.hp) < 0.0f) {
                ((struct EnLOCAL_DATA *)dp->oni.other)->scp->battle.hp = 0.0f;
            }
            ((struct EnLOCAL_DATA *)dp->oni.other)->scp->battle.hp_rate = 100.0f * (((struct EnLOCAL_DATA *)dp->oni.other)->scp->battle.hp / ((struct EnLOCAL_DATA *)dp->oni.other)->scp->battle.hp_max);
            dp->scp->battle.hp = 0.0f;
        }
        dp->endurance -= dp->scp->battle.damage;
        if (dp->endurance < 0.0f) {
            dp->endurance = 0.0f;
        }
        dp->scp->battle.damage = 0.0f;
        dp->scp->battle.hp_rate = 100.0f * (dp->scp->battle.hp / dp->scp->battle.hp_max);
        if (dp->endurance <= 0.0f) {
            return 1;
        }
    }
    return 0;
}

static int enONICanSeePlayer(struct EnLOCAL_DATA *dp) {
    float *ppos;
    float dist;
    float a;
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
    dist = dp->p_dist;
    if (dist > enONIGetFeelRange()) {
        return 0;
    }
    if (enCheckNoDamageHuman(dp)) {
        return 1;
    }
    if (dp->oni.other && (((struct EnLOCAL_DATA *)dp->oni.other)->slv == 5 || ((struct EnLOCAL_DATA *)dp->oni.other)->slv == 6)) {
        return 1;
    }
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a = enCalcAngleDifference(a, dp->scp->rot.y);
    if (dp->type == 2 && dist < 1100.0f &&
        (!dp->oni.other ||
         enDistXZ((float *)&dp->scp->pos, (float *)&((struct EnLOCAL_DATA *)dp->oni.other)->scp->pos) > 750.0f)) {
        if (dp->scp->pos.x > -22100.0f && dp->scp->pos.x < -17900.0f && dp->scp->pos.z > 19500.0f &&
            dp->scp->pos.z < 24500.0f) {
            return 4;
        }
    }
    if (dist < 1250.0f && a < 0.17453292f) {
        return 2;
    }
    return 1;
}

static int enONICanSeeCharacter(struct EnLOCAL_DATA *dp, struct SubCharacter *scp) {
    float *ppos;
    float dist;
    float a;
    int wcd;

    ppos = (float *)&scp->pos;
    wcd = enGetWorldCondition();
    dist = enCheckPath2(dp, ppos, (float *)&dp->scp->pos);
    if (dist >= 0.0f) {
        return 0;
    }
    dist = enDistXZ(ppos, (float *)&dp->scp->pos);
    if (dist > enONIGetFeelRange()) {
        return 0;
    }
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a = enCalcAngleDifference(a, dp->scp->rot.y);
    if (dist < 1250.0f && a < 0.17453292f) {
        return 2;
    }
    if (enLocalWork.Status & 1) {
        return 0;
    }
    return 1;
}

static void enONIAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        enAnimeRestart(dp);
        return;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 1199
    fjAssert(anim >= 0 && anim < sizeof(EnONIAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnONIAnime[anim].Anime);
}

static void enONIAnimeReset(struct EnLOCAL_DATA *dp, int anim) {

    /* Matching: the assert bakes its original line number into the object. */
#line 1207
    fjAssert(anim >= 0 && anim < sizeof(EnONIAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnONIAnime[anim].Anime);
}

static void enONIAnimeExec(struct EnLOCAL_DATA *dp) {
    int of;

    of = dp->anim_n;
    enAnimeExec(dp, EnONIAnime, 0x1519);
    if (dp->anim_n == -1 || (dp->flag & 1)) {
        return;
    }
    if (dp->anim == 1) {
        enSetTransWalk(dp);
        if ((dp->anim_s > 0 && ((of < 0xCE40 && dp->anim_n >= 0xCE40) || (of < 0x19C80 && dp->anim_n >= 0x19C80))) ||
            (dp->anim_s < 0 && ((of > 0xA8C0 && dp->anim_n <= 0xA8C0) || (of > 0x17700 && dp->anim_n <= 0x17700)))) {
            if (enCheckWater(dp)) {
                enSoundCall(0x4978, 1.0f, (float *)&dp->scp->pos);
            } else {
                if (dp->flag & 0x1000) {
                    enSoundCall3D(0x3EF1, 1.0f, (float *)&dp->scp->pos);
                } else {
                    enSoundCall(0x3EF1, 1.0f, (float *)&dp->scp->pos);
                }
            }
        }
    }
}

static float enONIGetSpeed(struct EnLOCAL_DATA *dp) {
    return 0.25f + 0.75f * (dp->endurance / dp->endurance_max);
}

static float enONIGetWalkSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.5f, 0.8f, 1.0f, 1.1f, 1.2f };
    float r;

    r = enONIGetSpeed(dp) * speed_rate[enGetMode()];
    if (dp->type == 1) {
        r *= 1.5f;
    }
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

static float enONIGetAttackSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.75f, 0.8f, 1.0f, 1.1f, 1.2f };
    float r;

    r = enONIGetSpeed(dp) * speed_rate[enGetMode()];
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

static float enONIGetFeelRange(void) {
    float feel_range[5] = { 1000.0f, 1500.0f, 2000.0f, 2500.0f, 4000.0f };
    float r;

    r = feel_range[enGetMode()];
    if (enGetWorldCondition() == 4) {
        r *= 0.75f;
    }
    return r;
}

static float enONIGetRotSpeed(void) {
    float rot_rate[5] = { 0.5f, 0.7f, 1.0f, 1.5f, 2.0f };

    return 0.05235988f * rot_rate[enGetMode()];
}

static void enONISetSlowTime(struct EnLOCAL_DATA *dp) {
    int timer[5] = { 180, 90, 60, 30, 1 };

    enSetTimer(dp, timer[enGetMode()]);
}

static void enONISoundLife(struct EnLOCAL_DATA *dp) {
    if (dp->sound_wait < 300) {
        dp->sound_wait++;
    } else if (dp->anim == 2 || shRandF() < 0.2f * shGetDT()) {
        enSoundCall(ftoi(2.0f * shRandF()) + 0x3EF6, 1.0f, (float *)&dp->scp->pos);
        dp->sound_wait = 0;
    }
}
