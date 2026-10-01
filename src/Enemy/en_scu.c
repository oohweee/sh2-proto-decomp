/*
 * en_scu.c: AI of the SCU enemy: ambushes (also from under cars), wanders, crawls, chases,
 * backs off and attacks. Shared enemy code is in en_common.c.
 * The Lying Figure (verified; docs/characters.md).
 */
#include "enemy.h"

static void enSCUCtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enSCUCtrlSleep(struct EnLOCAL_DATA *dp);
static void enSCUCtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enSCUCtrlEvent(struct EnLOCAL_DATA *dp);
static void enSCUCtrlHand(); /* No parameters, as in the DWARF; K&R so it still fits the handler table. */
static void enSCUCtrlWander(struct EnLOCAL_DATA *dp);
static void enSCUCtrlPrecaution(struct EnLOCAL_DATA *dp);
static void enSCUCtrlChase(struct EnLOCAL_DATA *dp);
static void enSCUCtrlBackward(struct EnLOCAL_DATA *dp);
static void enSCUCtrlAmbush(struct EnLOCAL_DATA *dp);
static void enSCUCtrlWaitCar(struct EnLOCAL_DATA *dp);
static void enSCUCtrlCrawl(struct EnLOCAL_DATA *dp);
static void enSCUCtrlAttack(struct EnLOCAL_DATA *dp);
static void enSCUCtrlDamage(struct EnLOCAL_DATA *dp);
static void enSCUCtrlConfuse(struct EnLOCAL_DATA *dp);
static void enSCUCtrlDown(struct EnLOCAL_DATA *dp);
static int enSCUCanSeePlayer(struct EnLOCAL_DATA *dp);
static int enSCUCheckAmbush(struct EnLOCAL_DATA *dp);
static int enSCUCheckUpper(struct EnLOCAL_DATA *dp);
static void enSCUAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enSCUAnimeReset(struct EnLOCAL_DATA *dp, int anim);
static void enSCUAnimeExec(struct EnLOCAL_DATA *dp);
static void enSCUAutoRecovery(struct EnLOCAL_DATA *dp);
static float enSCUGetWalkSpeed(struct EnLOCAL_DATA *dp);
static float enSCUGetCrawlSpeed(struct EnLOCAL_DATA *dp);
static float enSCUGetFeelRange(void);
static float enSCUGetAttackRange(void);
static float enSCUGetAttackAngle(void);
static float enSCUGetAttackProbability(struct EnLOCAL_DATA *dp);
static float enSCUGetRepertAttackProbability(void);
static float enSCUGetAttackSpeed(struct EnLOCAL_DATA *dp);
static float enSCUGetRotSpeed(void);
static float enSCUGetAimingSpeed(void);
static void enSCUSetDownTime(struct EnLOCAL_DATA *dp);
static float enSCUGetCrawlProbability(void);
static void enSCUSetMoveCount(struct EnLOCAL_DATA *dp);
static void enSCUSetDc(struct EnLOCAL_DATA *dp);
static void enSCUSoundLife(struct EnLOCAL_DATA *dp);
static void enSCUSoundCrawlInit(struct EnLOCAL_DATA *dp);
static void enSCUSoundCrawl(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnSCUAnime[33] = {
    { 0x138A, 0 }, { 0x138A, 1 }, { 0x1389, 1 }, { 0x138D, 0 }, { 0x138E, 0 }, { 0x138F, 0 }, { 0x1390, 0 },
    { 0x1391, 0 }, { 0x1392, 0 }, { 0x1393, 0 }, { 0x1394, 0 }, { 0x1395, 0 }, { 0x1396, 0 }, { 0x138F, 0 },
    { 0x1397, 0 }, { 0x1398, 0 }, { 0x1399, 0 }, { 0x139A, 0 }, { 0x139B, 0 }, { 0x139C, 0 }, { 0x139D, 0 },
    { 0x139E, 0 }, { 0x13A0, 0 }, { 0x139F, 0 }, { 0x13A4, 1 }, { 0x13A3, 1 }, { 0x13A9, 0 }, { 0x13A8, 0 },
    { 0x138B, 1 }, { 0x138C, 0 }, { 0x13A7, 0 }, { 0x13AB, 1 }, { 0x13AA, 0 },
};

struct EnAMBUSH_DATA ambush_apart[9] = {
    { 56200.0f, 10800.0f, 64000.0f, 12000.0f, 60400.0f, 12600.0f, 3.1415927f },
    { 56200.0f, 10800.0f, 60000.0f, 12000.0f, 56800.0f, 10000.0f, 0.0f },
    { 99600.0f, -20000.0f, 100800.0f, -15000.0f, 98900.0f, -17700.0f, 1.5707964f },
    { -106000.0f, 10800.0f, -100000.0f, 12000.0f, -103000.0f, 10300.0f, 0.0f },
    { -102500.0f, 10800.0f, -96000.0f, 12000.0f, -99400.0f, 12800.0f, 3.1415927f },
    { -84000.0f, 10800.0f, -80200.0f, 12000.0f, -80600.0f, 13000.0f, 3.1415927f },
    { 99600.0f, -20000.0f, 100800.0f, -15000.0f, 99000.0f, -17600.0f, 1.5707964f },
    { 99600.0f, -61500.0f, 100800.0f, -56000.0f, 99000.0f, -58800.0f, 1.5707964f },
    { -50600.0f, -65000.0f, -49400.0f, -57000.0f, -48700.0f, -60800.0f, -1.5707964f },
};

struct EnAMBUSH_DATA ambush_hospital[9] = {
    { -18000.0f, -24000.0f, -16800.0f, -16800.0f, -18500.0f, -22250.0f, 1.5707964f },
    { -25500.0f, -24000.0f, -16800.0f, -22800.0f, -18500.0f, -22250.0f, 3.1415927f },
    { 54000.0f, 59600.0f, 63000.0f, 60800.0f, 58500.0f, 61300.0f, 3.1415927f },
    { -66000.0f, 99600.0f, -56000.0f, 100800.0f, -61600.0f, 101300.0f, 3.1415927f },
    { -64000.0f, -140800.0f, -56000.0f, -139600.0f, -60000.0f, -139100.0f, 3.1415927f },
    { 18500.0f, -181200.0f, 23200.0f, -178800.0f, 22300.0f, -181750.0f, 0.0f },
    { 56000.0f, -180800.0f, 65000.0f, -179600.0f, 60000.0f, -179200.0f, 3.1415927f },
    { -24000.0f, -224000.0f, -16800.0f, -222800.0f, -18800.0f, -222200.0f, 3.1415927f },
    { -18000.0f, -224000.0f, -16800.0f, -216800.0f, -18500.0f, -222000.0f, 1.5707964f },
};

struct EnAMBUSH_DATA ambush_delusion[9] = {
    { 58400.0f, 107200.0f, 108400.0f, 65200.0f, 62150.0f, 106500.0f, 0.0f },
    { 61600.0f, 99200.0f, 62800.0f, 108400.0f, 63500.0f, 103800.0f, -1.5707964f },
    { 139600.0f, 97200.0f, 140800.0f, 103600.0f, 141300.0f, 98000.0f, -1.5707964f },
    { 56000.0f, 60400.0f, 63600.0f, 61600.0f, 62850.0f, 59900.0f, 0.0f },
    { 142000.0f, 54200.0f, 143200.0f, 66200.0f, 141500.0f, 60400.0f, 1.5707964f },
    { 138000.0f, 65000.0f, 143200.0f, 66200.0f, 140200.0f, 66750.0f, 3.1415927f },
    { 59200.0f, 17200.0f, 60400.0f, 28000.0f, 61000.0f, 21800.0f, -1.5707964f },
    { 137600.0f, 16800.0f, 142400.0f, 19200.0f, 139200.0f, 18600.0f, -1.5707964f },
    { 137600.0f, 18150.0f, 144400.0f, 20400.0f, 138300.0f, 19600.0f, 3.1415927f },
};

struct EnAMBUSH_DATA ambush_hotel_f[4] = {
    { -18800.0f, 98800.0f, -16800.0f, 107600.0f, -16250.0f, 102400.0f, -1.5707964f },
    { -18800.0f, 98800.0f, -16800.0f, 107600.0f, -16250.0f, 99600.0f, -1.5707964f },
    { -60400.0f, -64800.0f, -59600.0f, -59600.0f, -60700.0f, -64400.0f, 1.5707964f },
    { -60400.0f, -64800.0f, -59600.0f, -59600.0f, -59300.0f, -63200.0f, -1.5707964f },
};

struct EnAMBUSH_DATA ambush_hotel_b[5] = {
    { -22400.0f, 60400.0f, -12400.0f, 61200.0f, -12800.0f, 60000.0f, 0.0f },
    { -17000.0f, 58800.0f, -22400.0f, 61200.0f, -22900.0f, 59300.0f, 1.5707964f },
    { -102350.0f, 20650.0f, -98750.0f, 23850.0f, -101900.0f, 22300.0f, 3.1415927f },
    { -102350.0f, 17450.0f, -98750.0f, 23050.0f, -101150.0f, 21400.0f, -1.5707964f },
    { -63200.0f, -64800.0f, -59600.0f, -59600.0f, -59150.0f, -63150.0f, -1.5707964f },
};

struct EnAMBUSH_DATA *ambush_data[6] = {
    NULL, ambush_apart, ambush_hospital, ambush_delusion, ambush_hotel_f, ambush_hotel_b,
};

int ambush_num[6] = { 0, 9, 9, 9, 4, 5 };

/* Matching: enSCUInitData: the stand-in before it sets its float-constant argument order
 * (fitted, not recovered; whether the original had code here: docs/stand-ins.md).
 */
static float __stripped_float_code_101(float x0, float x1) { int i0 = (int)x0 * 45; int i1 = (int)x0 * 70; int i2 = (int)x0 * 74; i0 = i0 * 973; x0 += 451.0f; x1 += 3973.0f; x0 += 2804.0f; i2 = i1 * 169; x0 += 286.0f; i1 = i1 * 56; x0 += 3078.0f; x1 += 3491.0f; x0 += 3603.0f; i1 = i0 * 869; x0 += 3300.0f; x1 += 2880.0f * x1; x1 += 244.0f * x1; x0 += 2645.0f; x1 += 3912.0f; x1 += 766.0f * x1; x1 += 835.0f * x1; i1 = i0 * 521; x0 += 825.0f; x1 += 3640.0f; x0 += 3059.0f * x1; x1 += 180.0f; x0 += 368.0f * x0; i0 = i1 * 126; x1 += 3736.0f; return x0 + (float)i0 + (float)i1 + (float)i2; } /* fitted, not recovered: 20 constants */
/** Sets up a new SCU: HP by difficulty mode, size, starting level from the spawn status.
 * @param dp enemy work */
void enSCUInitData(struct EnLOCAL_DATA *dp) {
    float vitarity[5] = { 120.0f, 400.0f, 700.0f, 1500.0f, 9999.0f };
    float endurance[5] = { 10.0f, 190.0f, 285.0f, 380.0f, 450.0f };
    int mode;
    float rate;

    mode = enGetMode();
    dp->mlv = 1;
    dp->slv = 0;
    dp->sslv = 0;
    enSetBattleTarget(dp, 0);
    vcopy(&dp->scp->pos, dp->scu.stpos);
    switch (enGetPlace()) {
    case 0:
        dp->type = 3;
        break;
    default:
        dp->type = 6;
        break;
    }
    enSetSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
    dp->weight = 2;
    enSCUAnimeSet(dp, 1);
    switch (dp->scp->en_first_status) {
    case 1:
        rate = 1.2f;
        break;
    case 2:
        rate = 0.5f;
        break;
    case 4:
        dp->type = -1;
    case 3:
        rate = 0.3f;
        EN_SET_LEVEL(dp, 10);
        enSCUAnimeSet(dp, 26);
        enSetSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
        break;
    case 5:
        EN_SET_LEVEL(dp, 11);
        enSCUAnimeSet(dp, dp->lie + 26);
        return;
    case 8:
        rate = 1.0f;
        dp->type |= 8;
        EN_SET_LEVEL(dp, 5);
        break;
    case 7:
        rate = 1.0f;
        dp->type |= 0x10;
        EN_SET_LEVEL(dp, 6);
        break;
    case 13:
        enSCUAnimeSet(dp, 30);
        EN_SET_LEVEL(dp, 11);
        enSetHP(dp, 0.0f, 0.0f);
        return;
    case 15:
        enSCUAnimeSet(dp, 31);
        dp->mlv = 4;
        enFlagSetMoved(dp);
        enSetHP(dp, 1.0f, 1.0f);
        return;
    case 16:
        dp->type = 0x20;
        rate = 0.8f;
        break;
    case 17:
        dp->type = 0x21;
        dp->mlv = 4;
        return;
    case 14:
        enSCUAnimeSet(dp, 32);
        EN_SET_LEVEL(dp, 11);
        enSetHP(dp, 0.0f, 0.0f);
        return;
    case 0:
    case 6:
    case 9:
    case 10:
    case 11:
    case 12:
    default:
        rate = 1.0f;
        break;
    }
    enSetHP(dp, rate * vitarity[mode], rate * endurance[mode]);
    enSetSeeLightStatus(dp, 1000.0f, 1500.0f);
    enFlagSetMoved(dp);
    if (enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

/** Per-frame control of a SCU: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enSCUCtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlSCUFunc[7])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enSCUCtrlAutomatic, enSCUCtrlSleep, enSCUCtrlGoPlayable, enSCUCtrlEvent, enSCUCtrlHand,
        enWaitRegenerate,
    };

    enCtrlSCUFunc[dp->mlv](dp);
}

static void enSCUCtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlSCUSubFunc[12])(struct EnLOCAL_DATA *) = {
        enSCUCtrlWander,  enSCUCtrlPrecaution, enSCUCtrlChase,  enSCUCtrlBackward,
        enSCUCtrlAmbush,  enSCUCtrlWaitCar,    enSCUCtrlCrawl,  enSCUCtrlAttack,
        enSCUCtrlDamage,  enSCUCtrlConfuse,    enSCUCtrlDown,   enDyingExec,
    };

    enSetBattleTarget(dp, 0);
    enCtrlSCUSubFunc[dp->slv](dp);
    enSCUAnimeExec(dp);
    enMoveExec(dp);
    if (dp->slv <= 10 && enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

static void enSCUCtrlSleep(struct EnLOCAL_DATA *dp) {
    if (enCheckSleepOut(dp)) {
        enSleepOut(dp);
        enFlagResetLieDown(dp);
        dp->mlv = 3;
        if (dp->type & 8) {
            enFlagResetMoved(dp);
        }
    }
}

static void enSCUCtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    if (dp->type != -1) {
        if (dp->type & 0x10) {
            /* Matching: the line table has each level change here on one line (the macro or the
             * two stores fit equally); enSCUCtrlWander's float-constant order needs exactly one
             * of the three as EN_SET_LEVEL (with none, a stand-in was needed before it). Which
             * one isn't determined; the first is used. */
            EN_SET_LEVEL(dp, 6);
        } else if (dp->type & 8) {
            dp->slv = 5;
            dp->sslv = 0;
        } else {
            dp->slv = 0;
            dp->sslv = 0;
        }
    } else {
        EN_SET_LEVEL(dp, 11);
    }
}

static void enSCUCtrlEvent(struct EnLOCAL_DATA *dp) {
    enSCUAnimeExec(dp);
    enMoveExec(dp);
}

/* K&R definition, see the declaration above. */
static void enSCUCtrlHand() {
}

static void enSCUCtrlWander(struct EnLOCAL_DATA *dp) {
    float d;
    int t;
    float vec[4];

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 8);
        return;
    }
    if (enSCUCheckAmbush(dp)) {
        EN_SET_LEVEL(dp, 4);
        return;
    }
    if (enCommunicateTribe(1, (float *)&dp->scp->pos) || enCheckPlayerSound(dp)) {
        EN_SET_LEVEL(dp, 1);
        return;
    }
    t = enSCUCanSeePlayer(dp);
    if (dp->p_dist < enSCUGetFeelRange() && enGetMode() > 0 && !(enLocalWork.Status & 1)) {
        EN_SET_LEVEL(dp, 1);
        return;
    }
    switch (dp->sslv) {
    case 0:
        vcopy(&dp->scp->pos, dp->scu.stpos);
        enSCUAnimeSet(dp, 1);
        enSCUGetWalkSpeed(dp);
        enInitPath(&dp->path, dp->scp->rot.y);
        shSinCosV_Scale(vec, dp->scp->rot.y, 1000.0f);
        _shAddVector(dp->scu.target, (float *)&dp->scp->pos, vec);
        dp->sslv++;
    case 1:
        d = enDistXZ(dp->scu.stpos, (float *)&dp->scp->pos);
        if ((dp->type & 1) && d > 3500.0f) {
            vcopy(dp->scu.stpos, dp->scu.target);
            dp->sslv = 2;
            enSetTimer(dp, 300);
            break;
        }
        if (enDistXZ(dp->scu.target, (float *)&dp->scp->pos) < 400.0f || shRandF() < 0.002f) {
            d = dp->scp->rot.y + PI * (shRandF() - 0.5f) / 2.0f;
            shSinCosV_Scale(vec, d, 1000.0f);
            _shAddVector(dp->scu.target, (float *)&dp->scp->pos, vec);
            if ((dp->type & 1) && enDistXZ(dp->scu.target, dp->scu.stpos) > 3500.0f) {
                d = dp->scp->rot.y + PI * ((shRandI() & 0x8000) ? 1 : -1) / 2.0f;
                shSinCosV_Scale(vec, d, 1000.0f);
                _shAddVector(dp->scu.target, (float *)&dp->scp->pos, vec);
            }
        } else if (shRandF() < 0.0001f) {
            enSCUAnimeSet(dp, 2);
            dp->sslv = 3;
        }
        break;
    case 2:
        d = enDistXZ(dp->scu.stpos, (float *)&dp->scp->pos);
        if (d < 3500.0f) {
            dp->sslv = 1;
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            vcopy(&dp->scp->pos, dp->scu.stpos);
            dp->sslv = 1;
        }
        break;
    case 3:
        if (dp->anim_n == -1) {
            enSCUAnimeSet(dp, 1);
            enSCUGetWalkSpeed(dp);
            dp->sslv = 1;
        }
        break;
    }
    if (enSetPath(dp, dp->scu.target, (float *)&dp->scp->pos)) {
        shSinCosV_Scale(vec, dp->path.markangle, 1000.0f);
        _shAddVector(dp->scu.target, (float *)&dp->scp->pos, vec);
    }
    enMoveAngle(&dp->path, enSCUGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    enSCUAutoRecovery(dp);
    if (t == 2 && dp->anim_loop) {
        EN_SET_LEVEL(dp, 7);
    } else if (t == 1) {
        dp->slv = 2;
        dp->sslv = 0;
    }
    enSCUSoundLife(dp);
}

static void enSCUCtrlPrecaution(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 8);
        return;
    }
    if (!dp->sslv) {
        enSCUAnimeReset(dp, 2);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    if (enSCUCheckAmbush(dp)) {
        EN_SET_LEVEL(dp, 4);
        return;
    }
    if (dp->anim_n == -1) {
        t = enSCUCanSeePlayer(dp);
        if (t == 2) {
            EN_SET_LEVEL(dp, 7);
        } else if (t == 1 && dp->p_dist > 500.0f) {
            EN_SET_LEVEL(dp, 2);
        } else if (!enCommunicateTribe(1, (float *)&dp->scp->pos)) {
            if (dp->p_dist > enSCUGetAttackRange()) {
                EN_SET_LEVEL(dp, 0);
            }
        } else {
            enSCUAnimeReset(dp, 2);
        }
    }
    enMoveAngleToPlayer(dp, enSCUGetRotSpeed());
    enSCUAutoRecovery(dp);
    enSCUSoundLife(dp);
}

static void enSCUCtrlChase(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 8);
        return;
    }
    if (!dp->sslv) {
        enSCUAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        if (enGetMode() > 0) {
            enSetCommunication(1, 1, enGetPlayerPos(dp), 4000.0f, 120);
        }
        dp->sslv++;
    }
    enSetPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, enSCUGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    dp->anim_s = ftoi(4096.0f * enSCUGetWalkSpeed(dp) *
                      enCalcSpeedRate(dp->scp->rot.y, (float *)&dp->scp->pos, enGetPlayerPos(dp)));
    enSCUAutoRecovery(dp);
    t = enSCUCanSeePlayer(dp);
    if (t == 2 && dp->anim_loop) {
        EN_SET_LEVEL(dp, 7);
    } else if (!t && (dp->p_dist > enSCUGetFeelRange() || (enLocalWork.Status & 1))) {
        if (!enCommunicateTribe(1, (float *)&dp->scp->pos)) {
            EN_SET_LEVEL(dp, 0);
        } else {
            EN_SET_LEVEL(dp, 1);
        }
    } else if ((dp->type & 2) && dp->p_dist < 750.0f && dp->anim_loop && enGetMode() > 0) {
        dp->slv = 3;
        dp->sslv = 0;
    }
    enSCUSoundLife(dp);
}

static void enSCUCtrlBackward(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 8);
        return;
    }
    if (!dp->sslv) {
        enSCUAnimeSet(dp, 1);
        enSCUGetWalkSpeed(dp);
        enAnimeReverse(dp);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    enMoveAngleToPlayer(dp, enSCUGetRotSpeed());
    enSCUAutoRecovery(dp);
    t = enSCUCanSeePlayer(dp);
    if (t == 2 && (dp->anim_loop > 1 || enGetMode() >= 4)) {
        EN_SET_LEVEL(dp, 7);
    } else if (!t && dp->anim_loop >= 2 && dp->p_dist > enSCUGetFeelRange()) {
        if (!enCommunicateTribe(1, (float *)&dp->scp->pos)) {
            EN_SET_LEVEL(dp, 0);
        } else {
            EN_SET_LEVEL(dp, 1);
        }
    } else if (dp->anim_loop >= 3 && dp->p_dist > 750.0f) {
        dp->slv = 1;
        dp->sslv = 0;
    }
    enSCUSoundLife(dp);
}

static void enSCUCtrlAmbush(struct EnLOCAL_DATA *dp) {
    int t;
    float vec[4];
    short px;
    short pz;
    float a;
    float d;
    /* Matching: an initializer (not a first statement; the line table fits both) sets
     * enSCUCtrlWaitCar's float-constant order. */
    struct EnAMBUSH_DATA *pa = dp->scu.ambush;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 8);
        return;
    }
    switch (dp->sslv) {
    case 0:
        enSCUAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
        break;
    case 1:
    case 2:
        vec[0] = pa->pos_x;
        vec[2] = pa->pos_z;
        vec[1] = dp->scp->pos.y;
        enSetPath(dp, vec, (float *)&dp->scp->pos);
        if (dp->type & 2) {
            if (dp->sslv == 1 && enCalcAngleDifference(dp->path.markangle, dp->scp->rot.y) > 1.5707964f) {
                enInitPath(&dp->path, PI + dp->scp->rot.y);
                dp->sslv = 2;
                return;
            }
            if (dp->sslv == 2 && enCalcAngleDifference(dp->path.markangle, dp->scp->rot.y) < 1.5707964f) {
                enInitPath(&dp->path, dp->scp->rot.y);
                dp->sslv = 1;
                return;
            }
        }
        enMoveAngle(&dp->path, enSCUGetRotSpeed());
        a = dp->path.angle;
        if (dp->sslv == 1) {
            dp->scp->rot.y = a;
        } else {
            dp->scp->rot.y = shAngleRegulate(PI + a);
        }
        enSCUGetWalkSpeed(dp);
        if (dp->sslv == 2) {
            dp->anim_s = -dp->anim_s;
        }
        d = enDistXZ(vec, (float *)&dp->scp->pos);
        if (d < 50.0f) {
            enSCUAnimeSet(dp, 2);
            vcopy3(vec, &dp->scp->pos);
            enInitPath(&dp->path, dp->scp->rot.y);
            dp->sslv = 3;
        } else if (d > 1250.0f) {
            EN_SET_LEVEL(dp, 0);
        }
        break;
    case 3:
        a = 0.5f * shAngleRegulate(enGetPlayerDirection(dp) - pa->dir);
        dp->path.markangle = shAngleRegulate(pa->dir + a);
        enMoveAngle(&dp->path, enSCUGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        _shScaleVector(vec, enGetPlayerPos(dp), 0.02f);
        px = ftoi(vec[0]);
        pz = ftoi(vec[2]);
        if (px < pa->pl_x_min || pz < pa->pl_z_min || px > pa->pl_x_max || pz > pa->pl_z_max) {
            EN_SET_LEVEL(dp, 0);
            return;
        }
        if (dp->anim_n == -1) {
            enSCUAnimeReset(dp, 2);
            enAddHP(dp, 10.0f);
            dp->anim_loop++;
        }
        break;
    }
    enSCUAutoRecovery(dp);
    t = enSCUCanSeePlayer(dp);
    if (t == 2 && dp->anim_loop) {
        EN_SET_LEVEL(dp, 7);
    } else if (t == 1 && dp->sslv <= 2) {
        EN_SET_LEVEL(dp, 2);
    }
}

static void enSCUCtrlWaitCar(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        enSetSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
        enSCUAnimeSet(dp, 24);
        enFlagSetRotFloor(dp);
        enFlagResetMoved(dp);
        dp->flag |= 8;
        dp->lie = 0;
        dp->sslv++;
    }
    if (dp->p_dist < 2000.0f) {
        dp->scp->rot.y = enGetPlayerDirection(dp);
        enFlagSetMoved(dp);
        dp->flag &= ~8;
        dp->type &= ~8;
        dp->slv = 6; dp->sslv = 0;
    }
}

/* FAKEMATCH: fitted to the float-constant order, not recovered: `enCheckInstantDeath(dp) != 0`, the written-out
 * level changes to 11, and case 3's `enCheckFinishedByHuman(dp) == 0`
 * (docs/matching-notes.md#en_scu-enscuctrlcrawl). */
static void enSCUCtrlCrawl(struct EnLOCAL_DATA *dp) {
    float vec[4];
    int t;

    if (enCheckDamage(dp)) {
        if (dp->sslv == 2 || dp->sslv <= 0) {
            enResetDamage(dp);
        } else if (enCheckSpray(dp)) {
            enReduceHP(dp);
            enSCUAnimeSet(dp, 24);
            enSetTimer(dp, 180);
            dp->sslv = 6;
        } else {
            enReduceHP(dp);
            enSCUAnimeSet(dp, 26);
            enSetHitBack(dp);
            if (enCheckInstantDeath(dp) != 0) {
                enKillCountUp(dp);
                dp->slv = 11; dp->sslv = 0;
                return;
            }
            dp->sslv = 5;
            enSetTimer(dp, 10);
        }
    }
    switch (dp->sslv) {
    case -1:
        enFlagSetNoDamage(dp);
        enSCUAnimeSet(dp, 29);
        enSetNewSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
        dp->sslv--;
        break;
    case -2:
        if (dp->anim_n == -1) {
            dp->sslv = 0;
        }
        break;
    case 0:
        enSCUAnimeSet(dp, 28);
        if (!(dp->type & 0x10) && dp->endurance < dp->endurance_max) {
            enFlagSetLieDown(dp);
        } else {
            enFlagResetLieDown(dp);
        }
        enFlagResetNoDamage(dp);
        enFlagSetRotFloor(dp);
        enInitPath(&dp->path, dp->scp->rot.y);
        shSinCosV_Scale(vec, dp->scp->rot.y + shSway1f(-1.5707964f, 0.5f), 1000.0f);
        _shAddVector(dp->scu.target, (float *)&dp->scp->pos, vec);
        dp->lie = 0;
        enSCUSoundCrawlInit(dp);
        enSetTimer(dp, ftoi(180.0f * (1.0f + shSway1f(-0.5f, 0.5f))));
        dp->flag |= 0x200;
        dp->sslv++;
    case 1:
        enSCUSoundCrawl(dp);
        if (enDistXZ(dp->scu.target, (float *)&dp->scp->pos) < 400.0f || shRandF() < 0.002f) {
            shSinCosV_Scale(vec, dp->scp->rot.y + shSway1f(-1.5707964f, 0.5f), 1000.0f);
            _shAddVector(dp->scu.target, (float *)&dp->scp->pos, vec);
        }
        t = enSCUCanSeePlayer(dp);
        if (enCheckFinishedByHuman(dp) && dp->p_dist < 1000.0f) {
            dp->anim_loop = 0;
            enAnimePause(dp);
            enSetTimer(dp, ftoi(180.0f * (1.0f + shSway1f(-0.5f, 0.5f))));
            dp->sslv = 3;
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            if ((t == 2 || shRandF() < 0.02f || enSCUCheckAmbush(dp)) && !(dp->type & 0x10) && dp->type != -1 &&
                !enSCUCheckUpper(dp)) {
                if (!enGetMode()) {
                    dp->scp->battle.hp_rate = 0.0f; dp->scp->battle.hp = 0.0f;
                    enKillCountUp(dp);
                    EN_SET_LEVEL(dp, 11);
                    break;
                }
                enSCUAnimeSet(dp, 22);
                enFlagResetRotFloor(dp);
                enFlagResetLieDown(dp);
                enFlagSetNoDamage(dp);
                enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
                enAddHP(dp, 10.0f);
                dp->sslv = 2;
            } else if (shRandF() < 0.1f) {
                enSCUAnimeSet(dp, 24);
                enSetTimer(dp, ftoi(180.0f * (1.0f + shSway1f(-0.5f, 0.5f))));
                dp->sslv = 4;
            } else {
                dp->anim_loop = 0;
                enAnimePause(dp);
                enSetTimer(dp, ftoi(180.0f * (1.0f + shSway1f(-0.5f, 0.5f))));
                dp->sslv = 3;
                enAddHP(dp, 10.0f);
            }
            break;
        }
        if ((dp->type & 0x10) && dp->p_dist < 2000.0f && !enSCUCheckUpper(dp)) {
            enSCUAnimeSet(dp, 22);
            dp->type &= ~0x10;
            enFlagResetRotFloor(dp);
            enFlagResetLieDown(dp);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
            dp->sslv = 2;
            break;
        }
        if (enSetPath(dp, dp->scu.target, (float *)&dp->scp->pos)) {
            shSinCosV_Scale(vec, dp->path.markangle, 1000.0f);
            _shAddVector(dp->scu.target, (float *)&dp->scp->pos, vec);
            dp->path.timer = 0;
        }
        enMoveAngle(&dp->path, enSCUGetRotSpeed());
        shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enSCUGetCrawlSpeed(dp));
        break;
    case 2:
        if (dp->anim_n == -1) {
            enResetDamage(dp);
            enFlagResetNoDamage(dp);
            if (enSCUCanSeePlayer(dp) <= 0) {
                dp->slv = 1; dp->sslv = 0;
            } else {
                dp->slv = 2; dp->sslv = 0;
            }
            dp->flag &= ~0x200;
        }
        break;
    case 3:
        if (enReduceTimer(dp) <= 0 && enCheckFinishedByHuman(dp) == 0) {
            t = enSCUCanSeePlayer(dp);
            if ((t == 2 || dp->p_dist < 1000.0f || dp->scp->battle.hp == dp->scp->battle.hp_max) && dp->type != -1 &&
                !(dp->type & 0x10) && !enSCUCheckUpper(dp)) {
                if (enCheckFinishedByHuman(dp)) {
                    break;
                }
                if (!enGetMode()) {
                    enSCUAnimeSet(dp, 26);
                    dp->scp->battle.hp_rate = 0.0f; dp->scp->battle.hp = 0.0f;
                    enKillCountUp(dp);
                    dp->slv = 11; dp->sslv = 0;
                    break;
                }
                enSCUAnimeSet(dp, 22);
                enFlagResetRotFloor(dp);
                enFlagSetNoDamage(dp);
                enFlagResetLieDown(dp);
                enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
                enAddHP(dp, 10.0f);
                dp->sslv = 2;
            } else if (shRandF() < 0.1f) {
                enSCUAnimeSet(dp, 24);
                enSetTimer(dp, ftoi(120.0f * (1.0f + shSway1f(-0.5f, 0.5f))));
                dp->sslv = 4;
            } else {
                dp->sslv = 0;
            }
        }
        break;
    case 4:
        if (enReduceTimer(dp) <= 0) {
            enAddHP(dp, 10.0f);
            if (100.0f * shRandF() < dp->scp->battle.hp_rate && !enCheckFinishedByHuman(dp)) {
                dp->sslv = 0;
                break;
            }
            enSCUAnimeSet(dp, 26);
            enSetTimer(dp, ftoi(180.0f * (1.0f + shSway1f(-0.5f, 0.5f))));
            dp->sslv = 3;
        }
        break;
    case 5:
        if (0.0f == dp->hb_s && enReduceTimer(dp) <= 0) {
            dp->slv = 10;
            dp->sslv = 0;
            dp->flag &= ~0x200;
        }
        break;
    case 6:
        if (enReduceTimer(dp) <= 0) {
            enSCUAnimeSet(dp, 26);
            enSetTimer(dp, ftoi(180.0f * (1.0f + shSway1f(-0.5f, 0.5f))));
            dp->sslv = 3;
        }
        break;
    }
    enSCUAutoRecovery(dp);
}

static void enSCUCtrlAttack(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 8);
        return;
    }
    switch (dp->sslv) {
    case 0:
        if (shRandF() < enSCUGetAttackProbability(dp)) {
            enSCUAnimeReset(dp, 3);
            enSCUGetAttackSpeed(dp);
            enFlagSetCritical(dp);
            enAttackStart(dp);
            dp->sslv++;
            break;
        }
        if (dp->anim == 2) {
            enSCUSetMoveCount(dp);
            dp->anim_n = 0;
        } else {
            enSCUAnimeReset(dp, 2);
        }
        dp->sslv = 2;
        break;
    case 1:
        enMoveAngleToPlayer(dp, enSCUGetAimingSpeed());
        enAttackCheck(dp, 0x24);
        if (dp->anim_n == -1) {
            enFlagResetCritical(dp);
            if (shRandF() < enSCUGetRepertAttackProbability() && enSCUCanSeePlayer(dp) == 2) {
                dp->sslv = 0;
                break;
            }
            if (dp->type & 2) {
                if (enGetPlayerDistance(dp) < 750.0f && enGetMode() > 0) {
                    EN_SET_LEVEL(dp, 3);
                    break;
                }
                EN_SET_LEVEL(dp, 2);
                break;
            }
            EN_SET_LEVEL(dp, 2);
            break;
        }
        break;
    case 2:
        enSCUSoundLife(dp);
        if (dp->anim_n == -1) {
            if (dp->type & 2) {
                if (enGetPlayerDistance(dp) < 750.0f) {
                    dp->sslv = 0;
                    break;
                }
                EN_SET_LEVEL(dp, 2);
                break;
            }
            dp->slv = 2;
            dp->sslv = 0;
        }
        break;
    }
    enSCUAutoRecovery(dp);
}
/* Matching: fitted stand-in (not recovered code); it sets the next function's float-constant
 * order. Whether the original had code here: docs/stand-ins.md. */
static float __stripped_float_code_3(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f; }
static void enSCUCtrlDamage(struct EnLOCAL_DATA *dp) {
    float d;

    switch (dp->sslv) {
    case 0:
        d = enReduceHP(dp);
        if (enCheckSpray(dp)) {
            if (enGetSprayPower()) {
                EN_SET_LEVEL(dp, 6);
                dp->sslv = -1;
                break;
            }
            EN_SET_LEVEL(dp, 9);
            break;
        }
        if (d <= 0.0f || (d < 100.0f && shRandF() > 0.8f + 0.002f * d)) {
            enSCUAnimeReset(dp, enGetDownMotion(dp));
        } else {
            enSCUAnimeReset(dp, enGetDamageMotion(dp));
        }
        if (dp->anim >= 14 && dp->anim <= 21) {
            dp->lie = enGetLieDirection(dp->anim);
            enSetNewSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
            dp->sslv = 2;
        } else {
            dp->sslv = 1;
        }
        enSetHitBack(dp);
        break;
    case 1:
        if (dp->anim_n == -1 && 0.0f == dp->hb_s) {
            EN_SET_LEVEL(dp, 1);
        } else if (enCheckDamage(dp)) {
            dp->sslv = 0;
        }
        break;
    case 2:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
        }
        if (dp->anim_n == -1) {
            if (dp->type == 0x20) {
                dp->scp->battle.hp = 0.0f;
                dp->scp->battle.hp_rate = 0.0f;
                dp->endurance = 0.0f;
                enKillCountUp(dp);
                game_flag.flag[1] |= 0x200;
                enSCUAnimeSet(dp, 32);
                EN_SET_LEVEL(dp, 11);
                break;
            }
            EN_SET_LEVEL(dp, 10);
        }
        break;
    }
}

static void enSCUCtrlConfuse(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            dp->sslv = 0;
        } else {
            EN_SET_LEVEL(dp, 8);
            return;
        }
    }
    if (!dp->sslv) {
        enReduceHP(dp);
        enSCUAnimeSet(dp, 2);
        dp->sslv++;
    }
    if (dp->sslv == 1 && dp->anim_n == -1) {
        dp->sslv++;
        enSetTimer(dp, 600 - enGetMode() * 90);
    } else if (dp->sslv == 2 && enReduceTimer(dp) <= 0) {
        EN_SET_LEVEL(dp, 1);
    }
}

/* Matching: enSCUCtrlDown: the stand-in before it sets its float-constant argument order
 * (fitted, not recovered; whether the original had code here: docs/stand-ins.md).
 */
static float __stripped_float_code_102(float x0, float x1) { int i0 = (int)x0 * 65; x1 += 107.0f; x0 += 118.0f; x0 += 1358.0f; x0 += 1943.0f; x1 += 2838.0f; x1 += 812.0f; i0 = i0 * 986; x0 += 4112.0f; i0 = i0 * 385; x1 += 4904.0f * x0; x0 += 2579.0f; x0 += 2011.0f; x1 += 1396.0f * x1; x0 += 1440.0f * x1; i0 = i0 * 689; return x0 + (float)i0; } /* fitted, not recovered: 12 constants */
static void enSCUCtrlDown(struct EnLOCAL_DATA *dp) {
    switch (dp->sslv) {
    case 0:
        enFlagSetRotFloor(dp);
        enFlagSetLieDown(dp);
        enSetNewSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
        if (enCheckDeath(dp)) {
            enFlagResetMoved(dp);
            if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 11);
                break;
            }
            enSCUSetDc(dp);
            dp->sslv = 4;
            break;
        }
        enSCUSetDownTime(dp);
        if (dp->scp->en_first_status == 3 || dp->scp->en_first_status == 4) {
            dp->timer += enCalcTimer(600);
        }
        dp->sslv++;
        break;
    case 1:
        if (enCheckDamage(dp)) {
            enSCUAnimeReset(dp, dp->lie + 26);
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckDeath(dp)) {
                enFlagResetMoved(dp);
                if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                    enKillCountUp(dp);
                    EN_SET_LEVEL(dp, 11);
                    break;
                }
                enSCUSetDc(dp);
                dp->sslv = 4;
                break;
            }
            dp->sslv = 3;
            enSetTimer(dp, 90);
            break;
        }
        if (enReduceTimer(dp) <= 0 && !enCheckFinishedByHuman(dp)) {
            if (!enGetMode()) {
                dp->scp->battle.hp_rate = 0.0f;
                dp->scp->battle.hp = 0.0f;
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 11);
                break;
            }
            enAddHP(dp, 50.0f);
            if ((!dp->lie && (shRandF() < enSCUGetCrawlProbability() || dp->type == -1)) || enSCUCheckUpper(dp)) {
                EN_SET_LEVEL(dp, 6);
                break;
            }
            enSCUAnimeSet(dp, dp->lie + 22);
            enFlagResetRotFloor(dp);
            enFlagResetLieDown(dp);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
            dp->sslv = 2;
            break;
        }
        if ((dp->flag & 0x2000) && !enCheckFinishedByHuman(dp)) {
            if (!dp->lie && shRandF() < enSCUGetCrawlProbability()) {
                if (dp->anim == 24) {
                    enSCUAnimeReset(dp, 26);
                    enSetTimer(dp, 60);
                    dp->sslv = 5;
                    break;
                }
                EN_SET_LEVEL(dp, 6);
                break;
            }
            if (dp->anim >= 24 && dp->anim <= 25) {
                if (100.0f * shRandF() > dp->scp->battle.hp_rate) {
                    enSCUAnimeReset(dp, dp->lie + 26);
                    enAddHP(dp, 10.0f);
                }
            } else {
                if (200.0f * shRandF() < 100.0f + dp->scp->battle.hp_rate) {
                    enSCUAnimeSet(dp, dp->lie + 24);
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
            if (enSCUCanSeePlayer(dp) <= 0) {
                EN_SET_LEVEL(dp, 1);
            } else {
                EN_SET_LEVEL(dp, 2);
            }
        }
        break;
    case 3:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckDeath(dp)) {
                enFlagResetMoved(dp);
                if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                    enKillCountUp(dp);
                    EN_SET_LEVEL(dp, 11);
                    break;
                }
                enSCUSetDc(dp);
                dp->sslv = 4;
                break;
            }
            if (enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 11);
                break;
            }
            enSetTimer(dp, 90);
            break;
        }
        if (dp->hb_s == 0.0f && enReduceTimer(dp) <= 0) {
            if (!enGetMode()) {
                dp->scp->battle.hp_rate = 0.0f;
                dp->scp->battle.hp = 0.0f;
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 11);
                break;
            }
            if (dp->type == -1) {
                dp->sslv = 1;
                break;
            }
            if (enCheckFinishedByHuman(dp)) {
                dp->sslv = 6;
                break;
            }
            if (enSCUCheckUpper(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 11);
                break;
            }
            enSCUAnimeSet(dp, dp->lie + 22);
            enAddHP(dp, 50.0f);
            enFlagResetRotFloor(dp);
            enFlagResetLieDown(dp);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
            dp->sslv = 2;
            break;
        }
        break;
    case 4:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            dp->hb_s *= itof(dp->scu.dcm - dp->scu.dc) / (dp->scu.dcm + 1);
            if (++dp->scu.dc >= dp->scu.dcm || enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 11);
                break;
            }
            enSetTimer(dp, 200);
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            enKillCountUp(dp);
            EN_SET_LEVEL(dp, 11);
        }
        break;
    case 5:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckDeath(dp)) {
                enFlagResetMoved(dp);
                if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                    enKillCountUp(dp);
                    EN_SET_LEVEL(dp, 11);
                    break;
                }
                enSCUSetDc(dp);
                dp->sslv = 4;
                break;
            }
            enSetTimer(dp, 90);
            dp->sslv = 3;
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            EN_SET_LEVEL(dp, 6);
        }
        break;
    case 6:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckDeath(dp)) {
                enFlagResetMoved(dp);
                if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                    enKillCountUp(dp);
                    EN_SET_LEVEL(dp, 11);
                    break;
                }
                enSCUSetDc(dp);
                dp->sslv = 4;
                break;
            }
            enSetTimer(dp, 90);
            dp->sslv = 3;
            break;
        }
        if (!enCheckFinishedByHuman(dp)) {
            enSCUAnimeSet(dp, dp->lie + 22);
            enAddHP(dp, 50.0f);
            enFlagResetRotFloor(dp);
            enFlagResetLieDown(dp);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
            dp->sslv = 2;
        }
        break;
    }
}

static int enSCUCanSeePlayer(struct EnLOCAL_DATA *dp) {
    float *ppos;
    float dist;
    float a;
    int wcd;

    ppos = enGetPlayerPos(dp);
    wcd = enGetWorldCondition();
    dist = enCheckPath2(dp, ppos, (float *)&dp->scp->pos);
    if (dist >= 0.0f) {
        if (enGetMode() >= 2 && dp->p_dist < enSCUGetAttackRange()) {
            a = enCalcDirection(ppos, (float *)&dp->scp->pos);
            a = enCalcAngleDifference(a, dp->scp->rot.y);
            if (a < enSCUGetAttackAngle()) {
                return 2;
            }
        }
        if (wcd >= 2 && enCheckSeeLight(dp) && !(enLocalWork.Status & 1)) {
            return 1;
        }
        return 0;
    }
    dist = dp->p_dist;
    switch (wcd) {
    case 0:
        if (dist > 5000.0f) {
            return 0;
        }
        break;
    case 1:
        if (dist > 3000.0f) {
            return 0;
        }
        break;
    case 2:
    case 3:
        if (dist > 4000.0f) {
            return 0;
        }
        break;
    case 4:
        if (dist > 1000.0f) {
            return 0;
        }
        break;
    }
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a = enCalcAngleDifference(a, dp->scp->rot.y);
    if (a < enSCUGetAttackAngle() && dist < enSCUGetAttackRange() && !enCheckNoDamageHuman(dp)) {
        return 2;
    }
    if (wcd >= 2 && enCheckSeeLight(dp)) {
        return 1;
    }
    if (a > 1.2217305f) {
        return 0;
    }
    return !(enLocalWork.Status & 1);
}

static int enSCUCheckAmbush(struct EnLOCAL_DATA *dp) {
    struct EnAMBUSH_DATA *pa;
    int num_max;
    int i;
    float px;
    float pz;
    float *ppos;
    float pos[4];

    pa = ambush_data[enGetPlace()];
    num_max = ambush_num[enGetPlace()];
    if (!(dp->type & 4) || enGetMode() < 3 || !num_max || !pa) {
        return 0;
    }
    ppos = enGetPlayerPos(dp);
    px = ppos[0];
    pz = ppos[2];
    for (i = 0; i < num_max; i++, pa++) {
        if (px >= pa->pl_x_min && pz >= pa->pl_z_min && px <= pa->pl_x_max && pz <= pa->pl_z_max) {
            pos[0] = pa->pos_x;
            pos[2] = pa->pos_z;
            if (enDistXZ(pos, (float *)&dp->scp->pos) <= 1250.0f) {
                dp->scu.ambush = pa;
                return 1;
            }
        }
    }
    return 0;
}

static int enSCUCheckUpper(struct EnLOCAL_DATA *dp) {
    float pos1[4];
    float pos2[4];

    vcopy_dst_first(pos1, &dp->scp->pos);
    vcopy_dst_first(pos2, &dp->scp->pos);
    pos1[1] -= 1300.0f;
    pos2[1] -= 50.0f;
    if (enCheckHitEyes(dp, pos1, pos2) >= 0.0f) {
        return 1;
    }
    return 0;
}

static void enSCUAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        if (anim == 26) {
            return;
        }
        enAnimeRestart(dp);
        dp->anim_s = 0x1000;
        dp->anim_loop = 0;
        if (anim == 2) {
            enSCUSetMoveCount(dp);
        }
        return;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 1580
    fjAssert(anim >= 0 && anim < sizeof(EnSCUAnime) / sizeof(EnANIME_DATA));
    if (anim == 28) {
        dp->flag |= 0x8000;
    }
    enAnimeSet(dp, anim, EnSCUAnime[anim].Anime);
    if (anim == 28) {
        dp->flag &= ~0x8000;
    }
    if (anim == 2) {
        enSCUSetMoveCount(dp);
    }
}

static void enSCUAnimeReset(struct EnLOCAL_DATA *dp, int anim) {
    /* Matching: the assert bakes its original line number into the object. */
#line 1597
    fjAssert(anim >= 0 && anim < sizeof(EnSCUAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnSCUAnime[anim].Anime);
    if (anim == 2) {
        enSCUSetMoveCount(dp);
    }
}

static void enSCUAnimeExec(struct EnLOCAL_DATA *dp) {
    int of;
    float pos[4];
    float vec[4];

    of = dp->anim_n;
    enAnimeExec(dp, EnSCUAnime, 0x138A);
    if (dp->anim_n == -1 || (dp->flag & 1)) {
        return;
    }
    if ((dp->anim >= 14 && dp->anim <= 21) || dp->anim == 29) {
        enSetTrans(dp);
    } else if (dp->anim >= 11 && dp->anim <= 12) {
        enSetTransWalk(dp);
    } else if (dp->anim == 3) {
        if (dp->anim_step <= 1 && dp->anim_n > 0x5DC0) {
            vcopy_dst_first(pos, &dp->scp->pos);
            pos[1] -= 700.0f;
            shSinCosV_Scale(vec, dp->scp->rot.y, BgIsOut(0) ? 2500.0f : 1250.0f);
            vec[1] += 0.5f * (enGetPlayerPos(dp)[1] - dp->scp->pos.y);
            enEfctPoisonFog(pos, vec);
            if (dp->anim_n > 0xBB80) {
                dp->anim_step = 2;
            }
        }
    } else if (dp->anim == 1) {
        enSetTransWalk(dp);
        if ((dp->anim_s > 0 && ((of < 0x12C0 && dp->anim_n >= 0x12C0) || (of < 0xA8C0 && dp->anim_n >= 0xA8C0))) ||
            (dp->anim_s < 0 && ((of > 0x960 && dp->anim_n <= 0x960) || (of > 0x9600 && dp->anim_n <= 0x9600)))) {
            if (enCheckWater(dp)) {
                enSoundCall(0x4978, 1.0f, (float *)&dp->scp->pos);
            } else {
                if (dp->flag & 0x1000) {
                    enSoundCall3D(0x2EF3, 1.0f, (float *)&dp->scp->pos);
                } else {
                    enSoundCall(0x2EF3, 1.0f, (float *)&dp->scp->pos);
                }
            }
            shEnemySCU_EffectFoot(dp->scp, dp->anim_n < 0x9600 ? 0 : 1);
        }
    }
}

static void enSCUAutoRecovery(struct EnLOCAL_DATA *dp) {
    short recover_rate[5] = { 0, 10, 30, 60, 100 };

    enAddEnduranceDT(dp, itof(recover_rate[enGetMode()]));
}

static float enSCUGetWalkSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.7f, 0.8f, 1.0f, 1.2f, 1.5f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.6f + dp->scp->battle.hp_rate / 250.0f;
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

static float enSCUGetCrawlSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 1.0f, 1.5f, 2.0f, 2.0f, 2.0f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.75f + dp->scp->battle.hp_rate / 400.0f;
    dp->anim_s = ftoi(4096.0f * r);
    return 1500.0f * r;
}

static float enSCUGetFeelRange(void) {
    float feel_range[5] = { 400.0f, 750.0f, 1000.0f, 1500.0f, 1500.0f };

    if (enGetWorldCondition() == 4) {
        return 500.0f;
    }
    return feel_range[enGetMode()];
}

static float enSCUGetAttackRange(void) {
    float attack_range[5] = { 750.0f, 900.0f, 1200.0f, 1500.0f, 1500.0f };
    float r;

    r = attack_range[enGetMode()];
    if (BgIsOut(0)) {
        r *= 1.5f;
    }
    return r;
}

static float enSCUGetAttackAngle(void) {
    float attack_angle[5] = { 0.08726646f, 0.13962634f, 0.17453292f, 0.34906584f, 0.5235988f };

    return attack_angle[enGetMode()];
}

static float enSCUGetAttackProbability(struct EnLOCAL_DATA *dp) {
    float attack_rate[5] = { 0.2f, 0.5f, 0.7f, 0.8f, 1.0f };
    int m;

    m = enGetMode();
    if (BgIsOut(0) && dp->type != 0x20) {
        return 1.0f;
    }
    if (m <= 2 && enCheckPlayerBulletEmpty()) {
        return 0.2f * attack_rate[m];
    }
    return attack_rate[m];
}

static float enSCUGetRepertAttackProbability(void) {
    float attack_rate[5] = { 0.0f, 0.0f, 0.2f, 0.5f, 0.99f };

    return attack_rate[enGetMode()];
}

static float enSCUGetAttackSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.5f, 0.5f, 0.6f, 0.8f, 1.0f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.75f + dp->scp->battle.hp_rate / 400.0f;
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

static float enSCUGetRotSpeed(void) {
    float rot_rate[5] = { 0.5f, 0.7f, 1.0f, 1.2f, 1.5f };

    return 0.05235988f * rot_rate[enGetMode()];
}

static float enSCUGetAimingSpeed(void) {
    float speed_rate[5] = { 0.0f, 0.1f, 0.3f, 0.7f, 1.0f };
    float speed_rate2[5] = { 0.0f, 0.0f, 0.1f, 0.5f, 1.0f };

    if (BgIsOut(0)) {
        return 0.05235988f * speed_rate[enGetMode()];
    }
    return 0.05235988f * speed_rate2[enGetMode()];
}

static void enSCUSetDownTime(struct EnLOCAL_DATA *dp) {
    short down_time[5] = { 900, 400, 300, 240, 180 };

    enSetTimer(dp, down_time[enGetMode()]);
}

static float enSCUGetCrawlProbability(void) {
    float crawl_rate[5] = { 0.8f, 0.8f, 0.8f, 0.8f, 0.8f };

    return crawl_rate[enGetMode()];
}

static void enSCUSetMoveCount(struct EnLOCAL_DATA *dp) {
    int n;

    n = ftoi(6.0f - 0.2f * enGetMode() + shSway1f(-2.0f, 0.5f));
    enSetAnimeCount(dp, n << 11);
}

static void enSCUSetDc(struct EnLOCAL_DATA *dp) {
    enSetTimer(dp, 200);
    dp->scu.dc = 0;
    dp->scu.dcm = (shRandI() >> 10) % (enGetMode() + 2) + 4;
}

static void enSCUSoundLife(struct EnLOCAL_DATA *dp) {
    if (dp->sound_wait < 300) {
        dp->sound_wait++;
        return;
    }
    if (dp->anim != 2 && shRandF() >= 0.2f * shGetDT()) {
        return;
    }
    enSoundCall(ftoi(8.0f * shRandF()) + 0x2EE1, 1.0f, (float *)&dp->scp->pos);
    dp->sound_wait = 0;
}

static void enSCUSoundCrawlInit(struct EnLOCAL_DATA *dp) {
    dp->scu.count = 0;
}
/* Matching: fitted stand-in (not recovered code); it sets the next function's float-constant
 * order. Whether the original had code here: docs/stand-ins.md. */
static float __stripped_float_code_4(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f; }
static void enSCUSoundCrawl(struct EnLOCAL_DATA *dp) {
    if ((dp->scu.count -= shGetDF()) <= 0) {
        dp->scu.count = enCalcTimer(20);
        if (dp->flag & 0x1000) {
            signed char r = ftoi(8.0f * shRandF());
            if (r >= 6) {
                enSoundCall3D(0, 1.0f, (float *)&dp->scp->pos);
            } else {
                enSoundCall(r + 0x2EE9, 1.0f, (float *)&dp->scp->pos);
            }
        } else {
            enSoundCall(ftoi(6.0f * shRandF()) + 0x2EE9, 1.0f, (float *)&dp->scp->pos);
        }
    }
}
