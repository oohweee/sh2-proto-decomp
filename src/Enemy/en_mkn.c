/*
 * en_mkn.c: AI of the MKN enemy: can stand still as a mannequin, falls or jumps down,
 * wanders, chases, circles and attacks. Shared enemy code is in en_common.c.
 * The Mannequin (verified; docs/characters.md).
 */
#include "enemy.h"

/* |a - b|. Matching: an inline function (name ours); written in place, enMKNCtrlConfuse and
 * enMKNCtrlDown don't match. */
inline float enDiff(float a, float b) {
    return fabsf(a - b);
}

static void enMKNCtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enMKNCtrlSleep(struct EnLOCAL_DATA *dp);
static void enMKNCtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enMKNCtrlEvent(struct EnLOCAL_DATA *dp);
static void enMKNCtrlHand(); /* No parameters, as in the DWARF; K&R so it still fits the handler table. */
static void enMKNCtrlMannequin(struct EnLOCAL_DATA *dp);
static void enMKNCtrlMannequin2(struct EnLOCAL_DATA *dp);
static void enMKNCtrlWaitFall(struct EnLOCAL_DATA *dp);
static void enMKNCtrlWaitJump(struct EnLOCAL_DATA *dp);
static void enMKNCtrlWander(struct EnLOCAL_DATA *dp);
static void enMKNCtrlPrecaution(struct EnLOCAL_DATA *dp);
static void enMKNCtrlChase(struct EnLOCAL_DATA *dp);
static void enMKNCtrlAround(struct EnLOCAL_DATA *dp);
static void enMKNCtrlAttack(struct EnLOCAL_DATA *dp);
static void enMKNCtrlDamage(struct EnLOCAL_DATA *dp);
static void enMKNCtrlConfuse(struct EnLOCAL_DATA *dp);
static void enMKNCtrlDown(struct EnLOCAL_DATA *dp);
static int enMKNCanSeePlayer(struct EnLOCAL_DATA *dp);
static int enMKNCanSeePlayer2(struct EnLOCAL_DATA *dp);
static void enMKNAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enMKNAnimeReset(struct EnLOCAL_DATA *dp, int anim);
static void enMKNAnimeExec(struct EnLOCAL_DATA *dp);
static void enMKNAutoRecovery(struct EnLOCAL_DATA *dp);
static float enMKNGetWalkSpeed(struct EnLOCAL_DATA *dp);
static float enMKNGetFeelRange(void);
static float enMKNGetAttackProbability(void);
static float enMKNGetRepertAttackProbability(void);
static float enMKNGetAttackSpeed(struct EnLOCAL_DATA *dp);
static float enMKNGetRotSpeed(void);
static void enMKNSetDownTime(struct EnLOCAL_DATA *dp);
static void enMKNSetMoveCount(struct EnLOCAL_DATA *dp);
static void enMKNSetDc(struct EnLOCAL_DATA *dp);
static void enMKNSoundLife(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnMKNAnime[32] = {
    { 0x13ED, 0 }, { 0x13EE, 1 }, { 0x13ED, 1 }, { 0x13EF, 0 }, { 0x13F0, 0 }, { 0x13F1, 0 }, { 0x13F2, 0 },
    { 0x13F3, 0 }, { 0x13F4, 0 }, { 0x13F5, 0 }, { 0x13F6, 0 }, { 0x13F7, 0 }, { 0x13F8, 0 }, { 0x13F1, 0 },
    { 0x13F9, 0 }, { 0x13FA, 0 }, { 0x13FB, 0 }, { 0x13FC, 0 }, { 0x13FD, 0 }, { 0x13FE, 0 }, { 0x13FF, 0 },
    { 0x1400, 0 }, { 0x1402, 0 }, { 0x1401, 0 }, { 0x1406, 1 }, { 0x1405, 1 }, { 0x140E, 0 }, { 0x140D, 0 },
    { 0x1409, 0 }, { 0x140A, 0 }, { 0x140B, 0 }, { 0x140C, 0 },
};
/* Matching: fitted stand-in (not recovered code); it sets the next function's float-constant
 * order. Whether the original had code here: docs/stand-ins.md. */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f + 53.0f + 55.0f + 57.0f + 59.0f + 61.0f + 63.0f + 65.0f; } /* fitted, not recovered: 32 constants */
/** Sets up a new MKN: HP by difficulty mode, size, starting level from the spawn status.
 * @param dp enemy work */
void enMKNInitData(struct EnLOCAL_DATA *dp) {
    float vitarity[5] = { 400.0f, 400.0f, 700.0f, 1500.0f, 9999.0f };
    float endurance[5] = { 10.0f, 190.0f, 285.0f, 380.0f, 450.0f };
    int mode;
    float rate;

    mode = enGetMode();
    dp->mlv = 1;
    dp->slv = 0;
    dp->sslv = 0;
    enSetBattleTarget(dp, 0);
    vcopy(&dp->scp->pos, dp->mkn.stpos);
    switch (enGetPlace()) {
    case 0:
        dp->type = 0;
        break;
    default:
        dp->type = 4;
        break;
    }
    enSetSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
    dp->weight = 2;
    enMKNAnimeSet(dp, 1);
    switch (dp->scp->en_first_status) {
    case 1:
        rate = 1.2f;
        break;
    case 2:
        rate = 0.5f;
        break;
    case 4:
        dp->type = 5;
    case 3:
        rate = 0.3f;
        EN_SET_LEVEL(dp, 11);
        enMKNAnimeSet(dp, 26);
        enSetSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
        break;
    case 5:
        EN_SET_LEVEL(dp, 12);
        enMKNAnimeSet(dp, dp->lie + 26);
        return;
    case 13:
        enMKNAnimeSet(dp, 30);
        EN_SET_LEVEL(dp, 12);
        rate = 0.0f;
        break;
    case 14:
        enMKNAnimeSet(dp, 31);
        EN_SET_LEVEL(dp, 12);
        rate = 0.0f;
        break;
    case 10:
        dp->type = 2;
        EN_SET_LEVEL(dp, 2);
        enMKNAnimeSet(dp, 28);
        dp->flag |= 2;
        dp->lie = 1;
        rate = 1.0f;
        break;
    case 11:
        dp->type = 3;
        EN_SET_LEVEL(dp, 3);
        enMKNAnimeSet(dp, 28);
        dp->flag |= 2;
        dp->lie = 1;
        rate = 1.0f;
        break;
    case 9:
        dp->type = 6;
        EN_SET_LEVEL(dp, 1);
        rate = 0.7f;
        break;
    case 0:
    case 6:
    case 7:
    case 8:
    case 12:
    default:
        rate = 1.0f;
        break;
    }
    enSetHP(dp, rate * vitarity[mode], rate * endurance[mode]);
    enSetSeeLightStatus(dp, 1500.0f, 2000.0f);
    if (enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

/** Per-frame control of a MKN: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enMKNCtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlMKNFunc[7])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enMKNCtrlAutomatic, enMKNCtrlSleep, enMKNCtrlGoPlayable, enMKNCtrlEvent, enMKNCtrlHand,
        enWaitRegenerate,
    };

    enCtrlMKNFunc[dp->mlv](dp);
}

static void enMKNCtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlMKNSubFunc[13])(struct EnLOCAL_DATA *) = {
        enMKNCtrlMannequin, enMKNCtrlMannequin2, enMKNCtrlWaitFall, enMKNCtrlWaitJump, enMKNCtrlWander,
        enMKNCtrlPrecaution, enMKNCtrlChase, enMKNCtrlAround, enMKNCtrlAttack, enMKNCtrlDamage,
        enMKNCtrlConfuse, enMKNCtrlDown, enDyingExec,
    };

    enSetBattleTarget(dp, 0);
    enCtrlMKNSubFunc[dp->slv](dp);
    enMKNAnimeExec(dp);
    enMoveExec(dp);
    if (dp->slv <= 11 &&
        enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

static void enMKNCtrlSleep(struct EnLOCAL_DATA *dp) {
    if (enCheckSleepOut(dp)) {
        enSleepOut(dp);
        switch (dp->type) {
        case 2:
            enFlagResetMoved(dp);
            EN_SET_LEVEL(dp, 2);
            break;
        case 3:
            enFlagResetMoved(dp);
            EN_SET_LEVEL(dp, 3);
            break;
        case 1:
            EN_SET_LEVEL(dp, 4);
            break;
        case 6:
            enFlagResetMoved(dp);
            EN_SET_LEVEL(dp, 1);
            break;
        default:
            enFlagResetMoved(dp);
            EN_SET_LEVEL(dp, 0);
            break;
        }
    }
}

static void enMKNCtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    dp->slv = 4; dp->sslv = 0;
}

static void enMKNCtrlEvent(struct EnLOCAL_DATA *dp) {
    void *tmp;
}

/* Matching: enMKNCtrlMannequin: this stand-in sets its float-constant argument order (fitted,
 * not recovered; docs/stand-ins.md). It sits in the gap before enMKNCtrlHand, where the
 * original's line table leaves room for code (117 lines; the file's usual gap between functions
 * is 3). Its body is still fitted. */
static float __stripped_float_code_104(float x0, float x1, float x2, float x3) { int i0 = (int)x0 * 72; int i1 = (int)x0 * 39; int i2 = (int)x0 * 56; int i3 = (int)x0 * 45; x0 += 3385.0f; x0 += 4636.0f * x3; x3 += 138.0f; x3 += 2363.0f; i2 = i1 * 80; x0 += 3657.0f; x0 += 2301.0f * x0; x1 += 1846.0f * x3; x1 += 1890.0f; x3 += 80.0f; x0 += 4480.0f; i1 = i0 * 98; x0 += 1173.0f; x1 += 2457.0f * x2; x1 += 2546.0f; x3 += 4127.0f * x2; i1 = i0 * 174; x0 += 3121.0f; return x0 + (float)i0 + (float)i1 + (float)i2 + (float)i3; } /* fitted, not recovered: 15 constants */
/* K&R definition, see the declaration above. */
static void enMKNCtrlHand() {
}

static void enMKNCtrlMannequin(struct EnLOCAL_DATA *dp) {
    int t;
    struct EnANIME_RANGE pose[2] = { { 3, 0, 17 }, { 4, 0, 17 } };

    if (enCheckDamage(dp)) {
        enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
        EN_SET_LEVEL(dp, 9);
        enFlagSetMoved(dp);
        return;
    }
    switch (dp->sslv) {
    case 0:
        t = ftoi(2.0f * shRandF());
        enMKNAnimeSet(dp, pose[t].Anime);
        dp->mkn.frame = ftoi(pose[t].Start + (pose[t].End - pose[t].Start + 1) * shRandF());
        enAnimeSetDirectFrame(dp, dp->anim, EnMKNAnime[dp->anim].Anime, dp->mkn.frame);
        dp->timer = 5;
        dp->sslv++;
        break;
    case 1:
        enAnimeFrameSet(dp, dp->mkn.frame);
        if (--dp->timer <= 0) {
            enAnimePause(dp);
            dp->sslv++;
        }
        break;
    case 3:
        if (enReduceTimer(dp) <= 0) {
            enSetCommunication(2, 1, (float *)&dp->scp->pos, 1000.0f, 2);
            enFlagSetMoved(dp);
            enAnimeRestart(dp);
            enInitPath(&dp->path, dp->scp->rot.y);
            enAttackStart(dp);
            enSetNewSize(dp, 350.0f, 800.0f, 750.0f, 750.0f);
            dp->sslv++;
        }
        return;
    case 4:
        enAttackCheck(dp, dp->anim == 3 ? (unsigned char)0x26 : (unsigned char)0x27);
        enMoveAngleToPlayer(dp, 0.017453292f);
        if (dp->anim_n == -1) {
            enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
            EN_SET_LEVEL(dp, 5);
            if (!dp->type) {
                dp->type = 1;
            }
        }
        return;
    }
    t = enMKNCanSeePlayer2(dp);
    if (t == 2) {
        enSetCommunication(2, 1, (float *)&dp->scp->pos, 1000.0f, 2);
        EN_SET_LEVEL(dp, 8);
        enFlagSetMoved(dp);
        if (!dp->type) {
            dp->type = 1;
        }
    } else if (t == 1) {
        enSetTimer(dp, 0);
        dp->sslv = 3;
    } else if (enCommunicateTribe(2, (float *)&dp->scp->pos) && dp->sslv != 3) {
        enSetTimer(dp, 90);
        dp->sslv = 3;
    }
}

/* Matching: enMKNCtrlMannequin2: the stand-in before it sets its float-constant argument order
 * (fitted, not recovered; whether the original had code here: docs/stand-ins.md). Refitted for the
 * FAKEMATCH spelling in enMKNCtrlMannequin2's case 0.
 */
static float __stripped_float_code_101(float x0, float x1) { int i0 = (int)x0 * 152; i0 = i0 * 803; x0 += 125.0f; i0 = i0 * 254; x0 += 3572.0f * x0; return x0 + (float)i0; } /* fitted, not recovered: 2 constants */
/* FAKEMATCH (for enMKNCtrlWaitFall's float-constant order): `dp->sslv = dp->sslv + 1` in case 0 is a fitted
 * spelling, not evidence; the code is the same either way. */
static void enMKNCtrlMannequin2(struct EnLOCAL_DATA *dp) {
    switch (dp->sslv) {
    case 0:
        enMKNAnimeSet(dp, 27);
        enSetSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
        enFlagSetDead(dp);
        dp->sslv = dp->sslv + 1;
        break;
    case 1:
        if (dp->type == 7) {
            enMKNAnimeSet(dp, 23);
            enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
            enFlagSetMoved(dp);
            enFlagSetNoDamage(dp);
            enFlagResetDead(dp);
            enSoundCall(ftoi(4.0f * shRandF()) + 0x30D9, 1.0f, (float *)&dp->scp->pos);
            switch (enGetPlace()) {
            case 0:
                dp->type = 1;
                break;
            default:
                dp->type = 4;
                break;
            }
            dp->sslv++;
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            enResetDamage(dp);
            enFlagResetNoDamage(dp);
            EN_SET_LEVEL(dp, 5);
        }
        break;
    }
}

/* FAKEMATCH: the float-constant order of the enSetNewSize calls comes from compiler leftovers (docs/toolchain.md,
 * "Root cause"); fitted, not recovered: the braced case 0, `dp->flag = dp->flag | 0x8000`,
 * `enCheckFloor(pos) != 0`, case 2's `dp->sslv += 1`, enMKNCtrlMannequin2's case 0, and the refitted stand-ins
 * before it and enMKNCtrlWaitJump. */
static void enMKNCtrlWaitFall(struct EnLOCAL_DATA *dp) {
    float d;
    float *ppos;
    float pos[4];
    float vec[4];

    if (enCheckDamage(dp)) {
        enReduceHP(dp);
        enSetHitBack(dp);
    }
    switch (dp->sslv) {
    case 0: {
        if (enGetPlayerDistance(dp) < 3000.0f) {
            dp->flag = dp->flag | 0x8000;
            enMKNAnimeReset(dp, 28);
            enSetNewSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
            d = enGetPlayerPos(dp)[1] - dp->scp->pos.y;
            d = fsqrt(0.25f + 2.0f * (30.0f * (30.0f * d)) / 9800.0f) - 1.0f;
            dp->anim_s = ftoi(30720.0f / d);
            dp->mkn.fall = 0.0f;
            dp->sslv++;
        }
        break;
    }
    case 1:
        ppos = enGetPlayerPos(dp);
        shSinCosV_Scale(vec, enGetPlayerAngle(dp), 1000.0f);
        _shAddVector(pos, ppos, vec);
        if (enCheckPlayerHitEyes(dp, pos) >= 0.0f) {
            _shSubVector(pos, ppos, vec);
        }
        _shSubVector(vec, pos, (float *)&dp->scp->pos);
        if (lengthXYZ(vec) < 2500.0f) {
            vcopy(vec, dp->vec);
        } else {
            shSinCosV_Scale(dp->vec, shAtanV(vec), 2500.0f);
        }
        dp->mkn.fall += 9800.0f * shGetDT();
        dp->vec[1] = dp->mkn.fall;
        vcopy_dst_first(pos, &dp->scp->pos);
        if (pos[1] > ppos[1]) {
            pos[1] = ppos[1];
        }
        if (ppos[1] - dp->scp->pos.y < 50.0f && enCheckFloor(pos) != 0) {
            d = enLocalWork.HitResult.hobj.wall.cp[1];
            if (d <= dp->scp->pos.y) {
                switch (enGetStage()) {
                case 2:
                    enSoundCall3D(0x9D23, 1.0f, (float *)&dp->scp->pos);
                    break;
                case 3:
                    enSoundCall3D(0x9D2E, 1.0f, (float *)&dp->scp->pos);
                    break;
                }
                dp->scp->pos.y = d;
                dp->vec[1] = 0.0f;
                enMKNAnimeSet(dp, 29);
                dp->flag &= ~0x8002;
                dp->type = 1;
                enFlagSetRotFloor(dp);
                enSetTimer(dp, 30);
                dp->sslv++;
            }
        } else if (enGetPlayerDistance(dp) < 550.0f) {
            dp->scp->pos.x = pos[0];
            dp->scp->pos.z = pos[2];
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            if (enReduceTimer(dp) <= 0) {
                enMKNAnimeSet(dp, 23);
                enFlagResetRotFloor(dp);
                enFlagSetMoved(dp);
                enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
                enAddHP(dp, 100.0f);
                enFlagSetNoDamage(dp);
                dp->sslv += 1;
            }
        }
        break;
    case 3:
        if (dp->anim_n == -1) {
            enFlagResetNoDamage(dp);
            EN_SET_LEVEL(dp, 6);
        }
        break;
    }
}

/* Matching: enMKNCtrlWaitJump: the stand-in before it sets its float-constant argument order (fitted, not
 * recovered; docs/stand-ins.md), refitted for enMKNCtrlWaitFall's FAKEMATCH spellings. */
static float __stripped_float_code_102(float x0, float x1) { int i0 = (int)x0 * 16; int i1 = (int)x0 * 95; x1 += 3607.0f; x1 += 4860.0f * x0; x0 += 1360.0f; x0 += 2556.0f; i1 = i1 * 836; x1 += 3893.0f * x1; i1 = i0 * 798; x1 += 2208.0f * x0; x1 += 4197.0f * x1; return x0 + (float)i0 + (float)i1; } /* fitted, not recovered: 7 constants */
static void enMKNCtrlWaitJump(struct EnLOCAL_DATA *dp) {
    float d;
    float *ppos;
    float pos[4];

    if (enCheckDamage(dp)) {
        enReduceHP(dp);
        enSetHitBack(dp);
    }
    switch (dp->sslv) {
    case 0:
        if (enGetPlayerDistance(dp) < 3000.0f) {
            dp->flag |= 0x8000;
            enMKNAnimeReset(dp, 28);
            enSetNewSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
            dp->mkn.fall = -7500.0f;
            dp->sslv++;
        }
        break;
    case 1:
        ppos = enGetPlayerPos(dp);
        shSinCosV_Scale(dp->vec, dp->scp->rot.y, 2000.0f);
        d = dp->mkn.fall;
        dp->mkn.fall += 9800.0f * shGetDT();
        if (d < 0.0f) {
            dp->anim_s = 0;
            if (dp->mkn.fall >= 0.0f) {
                d = enGetPlayerPos(dp)[1] - dp->scp->pos.y;
                d = fsqrt(0.25f + 2.0f * (30.0f * (30.0f * d)) / 9800.0f) - 1.0f;
                dp->anim_s = ftoi(30720.0f / d);
            }
        }
        dp->vec[1] = dp->mkn.fall;
        vcopy_dst_first(pos, &dp->scp->pos);
        if (pos[1] > ppos[1]) {
            pos[1] = ppos[1];
        }
        if (ppos[1] - dp->scp->pos.y < 50.0f && enCheckFloor(pos)) {
            d = enLocalWork.HitResult.hobj.wall.cp[1];
            if (d <= dp->scp->pos.y) {
                switch (enGetStage()) {
                case 2:
                    enSoundCall3D(0x9D23, 1.0f, (float *)&dp->scp->pos);
                    break;
                case 3:
                    enSoundCall3D(0x9D2E, 1.0f, (float *)&dp->scp->pos);
                    break;
                }
                dp->scp->pos.y = d;
                dp->vec[1] = 0.0f;
                enMKNAnimeSet(dp, 29);
                dp->flag &= ~0x8002;
                dp->type = 1;
                enFlagSetRotFloor(dp);
                enSetTimer(dp, 30);
                dp->sslv++;
            }
        }
        break;
    case 2:
        if (dp->anim_n == -1 && enReduceTimer(dp) <= 0) {
            enMKNAnimeSet(dp, 23);
            enFlagResetRotFloor(dp);
            enFlagSetMoved(dp);
            enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
            enAddHP(dp, 100.0f);
            enFlagSetNoDamage(dp);
            dp->sslv++;
        }
        break;
    case 3:
        if (dp->anim_n == -1) {
            enFlagResetNoDamage(dp);
            EN_SET_LEVEL(dp, 6);
        }
        break;
    }
}

static void enMKNCtrlWander(struct EnLOCAL_DATA *dp) {
    float d;
    float r;
    int t;
    float vec[4];

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        return;
    }
    switch (dp->sslv) {
    case 0:
        vcopy(&dp->scp->pos, dp->mkn.stpos);
        enMKNAnimeSet(dp, 1);
        enMKNGetWalkSpeed(dp);
        enInitPath(&dp->path, dp->scp->rot.y);
        shSinCosV_Scale(vec, dp->path.angle, 1500.0f);
        _shAddVector(dp->mkn.target, (float *)&dp->scp->pos, vec);
        dp->sslv++;
    case 1:
        d = enDistXZ(dp->mkn.stpos, (float *)&dp->scp->pos);
        if (dp->type < 2 && d > 5000.0f) {
            vcopy(dp->mkn.stpos, dp->mkn.target);
            dp->sslv = 2;
            enSetTimer(dp, 300);
            break;
        }
        r = shRandF();
        if (enDistXZ(dp->mkn.target, (float *)&dp->scp->pos) < 600.0f || r < 0.02f) {
            d = dp->scp->rot.y + PI * (shRandF() - 0.5f) / 2.0f;
            shSinCosV_Scale(vec, d, 1500.0f);
            _shAddVector(dp->mkn.target, (float *)&dp->scp->pos, vec);
            if (dp->type < 2 && enDistXZ(dp->mkn.target, dp->mkn.stpos) > 5000.0f) {
                d = dp->scp->rot.y + PI * ((shRandI() & 0x8000) ? 1 : -1) / 2.0f;
                shSinCosV_Scale(vec, d, 1500.0f);
                _shAddVector(dp->mkn.target, (float *)&dp->scp->pos, vec);
            }
        } else if (r < 0.021f) {
            enMKNAnimeSet(dp, 2);
            dp->sslv = 3;
        }
        break;
    case 2:
        dp->anim_s = ftoi(4096.0f * enMKNGetWalkSpeed(dp) *
                          enCalcSpeedRate(dp->path.angle, (float *)&dp->scp->pos, dp->mkn.stpos));
        d = enDistXZ(dp->mkn.stpos, (float *)&dp->scp->pos);
        if (d < 5000.0f) {
            dp->anim_s = 0x1000;
            dp->sslv = 1;
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            dp->anim_s = 0x1000;
            vcopy(&dp->scp->pos, dp->mkn.stpos);
            dp->sslv = 1;
            break;
        }
        if (shRandF() < 0.01f) {
            enMKNAnimeSet(dp, 2);
            dp->sslv = 3;
        }
        break;
    case 3:
        if (dp->anim_n == -1) {
            enMKNAnimeSet(dp, 1);
            enMKNGetWalkSpeed(dp);
            dp->sslv = 1;
        }
        break;
    }
    if (enSetPath(dp, dp->mkn.target, (float *)&dp->scp->pos)) {
        shSinCosV_Scale(vec, dp->path.markangle, 1500.0f);
        _shAddVector(dp->mkn.target, (float *)&dp->scp->pos, vec);
    }
    enMoveAngle(&dp->path, enMKNGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    enMKNAutoRecovery(dp);
    t = enMKNCanSeePlayer(dp);
    if (t == 2 && dp->anim_loop) {
        EN_SET_LEVEL(dp, 8);
    } else if (t == 1) {
        EN_SET_LEVEL(dp, 6);
    } else if (dp->anim_loop && dp->p_dist < enMKNGetFeelRange()) {
        dp->slv = 5;
        dp->sslv = 0;
    }
    enMKNSoundLife(dp);
}

static void enMKNCtrlPrecaution(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        return;
    }
    if (!dp->sslv) {
        enMKNAnimeSet(dp, 2);
        dp->sslv++;
    }
    if (dp->anim_n == -1) {
        t = enMKNCanSeePlayer(dp);
        if (t == 2) {
            EN_SET_LEVEL(dp, 8);
        } else if (t == 1 && dp->p_dist > 350.0f) {
            EN_SET_LEVEL(dp, 6);
        } else if (dp->p_dist > enMKNGetFeelRange()) {
            EN_SET_LEVEL(dp, 4);
        } else {
            enMKNAnimeReset(dp, 2);
        }
    }
    enMoveAngleToPlayer(dp, enMKNGetRotSpeed());
    enMKNAutoRecovery(dp);
    enMKNSoundLife(dp);
}

static void enMKNCtrlChase(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        return;
    }
    if (!dp->sslv) {
        enMKNAnimeSet(dp, 1);
        dp->sslv++;
    }
    enSetPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, enMKNGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    dp->anim_s = ftoi(4096.0f * enMKNGetWalkSpeed(dp) *
                      enCalcSpeedRate(dp->scp->rot.y, (float *)&dp->scp->pos, enGetPlayerPos(dp)));
    enMKNAutoRecovery(dp);
    t = enMKNCanSeePlayer(dp);
    if (t == 2 && dp->anim_loop) {
        EN_SET_LEVEL(dp, 8);
    } else if (!t && (dp->p_dist > enMKNGetFeelRange() || (enLocalWork.Status & 1))) {
        if (dp->type > 1 || enGetMode() <= 1) {
            EN_SET_LEVEL(dp, 4);
        } else {
            EN_SET_LEVEL(dp, 7);
        }
    } else if (dp->p_dist < 350.0f && dp->anim_loop) {
        dp->slv = 5;
        dp->sslv = 0;
    }
    enMKNSoundLife(dp);
}

static void enMKNCtrlAround(struct EnLOCAL_DATA *dp) {
    float a;
    int t;
    float vec[4];

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        return;
    }
    if (!dp->sslv) {
        enMKNAnimeSet(dp, 1);
        enMKNGetWalkSpeed(dp);
        dp->sslv++;
    }
    a = enGetPlayerAngle(dp);
    if (shAngleRegulate(enGetPlayerDirection(dp) - a) < 0.0f) {
        a += 2.0943952f;
    } else {
        a -= 2.0943952f;
    }
    shSinCosV_Scale(vec, a, 1000.0f);
    _shAddVector(vec, enGetPlayerPos(dp), vec);
    enSetPath(dp, vec, (float *)&dp->scp->pos);
    if (fabsf(dp->path.dist) < 500.0f) {
        dp->slv = 6;
        dp->sslv = 0;
    }
    enMoveAngle(&dp->path, enMKNGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    enMKNAutoRecovery(dp);
    t = enMKNCanSeePlayer(dp);
    if (t == 2 && dp->anim_loop) {
        EN_SET_LEVEL(dp, 8);
    } else if (dp->p_dist < enMKNGetFeelRange()) {
        EN_SET_LEVEL(dp, 6);
    } else if (dp->p_dist > 3500.0f) {
        EN_SET_LEVEL(dp, 4);
    }
    enMKNSoundLife(dp);
}

static void enMKNCtrlAttack(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 9);
        return;
    }
    switch (dp->sslv) {
    case 0:
        if (shRandF() < enMKNGetAttackProbability()) {
            if (dp->anim == 3) {
                enMKNAnimeSet(dp, 4);
            } else if (dp->anim == 4 || (shRandI() & 0x8000)) {
                enMKNAnimeSet(dp, 3);
            } else {
                enMKNAnimeSet(dp, 4);
            }
            enMKNGetAttackSpeed(dp);
        } else {
            enMKNAnimeSet(dp, 2);
            dp->sslv = 2;
            break;
        }
        enAttackStart(dp);
        dp->sslv++;
        break;
    case 1:
        enAttackCheck(dp, dp->anim == 3 ? (unsigned char)0x26 : (unsigned char)0x27);
        if (dp->anim_n == -1) {
            if (shRandF() < enMKNGetRepertAttackProbability() && enMKNCanSeePlayer(dp) == 2) {
                dp->sslv = 0;
            } else {
                dp->slv = 6;
                dp->sslv = 0;
            }
            return;
        }
        break;
    case 2:
        enMKNSoundLife(dp);
        if (dp->anim_n == -1) {
            if (enMKNCanSeePlayer(dp) == 2) {
                dp->sslv = 0;
            } else {
                EN_SET_LEVEL(dp, 6);
            }
        }
        break;
    }
    enMoveAngleToPlayer(dp, enMKNGetRotSpeed());
    enMKNAutoRecovery(dp);
}

/* Matching: enMKNCtrlDamage: the stand-in before it sets its float-constant argument order
 * (fitted, not recovered; whether the original had code here: docs/stand-ins.md).
 */
static float __stripped_float_code_103(float x0) { x0 += 3720.0f; x0 += 906.0f; x0 += 580.0f; x0 += 1839.0f; x0 += 1147.0f * x0; x0 += 2668.0f; x0 += 3585.0f; return x0; } /* fitted, not recovered: 7 constants */
static void enMKNCtrlDamage(struct EnLOCAL_DATA *dp) {
    float d;

    switch (dp->sslv) {
    case 0:
        enSetCommunication(2, 1, (float *)&dp->scp->pos, 1000.0f, 2);
        d = enReduceHP(dp);
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 10);
            break;
        }
        if (d <= 0.0f || (d < 100.0f && shRandF() > 0.8f + 0.002f * d)) {
            enMKNAnimeSet(dp, enGetDownMotion(dp));
        } else {
            enMKNAnimeReset(dp, enGetDamageMotion(dp));
        }
        if (dp->anim >= 14 && dp->anim <= 21) {
            dp->lie = enGetLieDirection(dp->anim);
            enSetNewSize(dp, 400.0f, 200.0f, 100.0f, 200.0f);
            dp->sslv = 2;
        } else {
            dp->sslv = 1;
        }
        enSetHitBack(dp);
        break;
    case 1:
        if (dp->anim_n == -1 && 0.0f == dp->hb_s) {
            EN_SET_LEVEL(dp, 5);
        } else if (enCheckDamage(dp)) {
            dp->sslv = 0;
        }
        break;
    case 2:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
        }
        if (dp->anim_n == -1) {
            EN_SET_LEVEL(dp, 11);
            enFlagResetNoDamage(dp);
        }
        break;
    }
}

/* FAKEMATCH (for enMKNCtrlDown): the first `dp->sslv = dp->sslv + 1`, `tp != NULL`, `!= 0` are fitted. */
static void enMKNCtrlConfuse(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;

    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            if (dp->sslv != 4) {
                dp->sslv = 0;
            }
        } else {
            EN_SET_LEVEL(dp, 9);
            return;
        }
    }
    if (!dp->sslv) {
        enReduceHP(dp);
        enMKNAnimeSet(dp, 2);
        dp->sslv = dp->sslv + 1;
    }
    if (dp->sslv >= 4) {
        tp = dp->mkn.tp;
        dp->path.markangle = enCalcDirection((float *)&tp->scp->pos, (float *)&dp->scp->pos);
        enMoveAngle(&dp->path, enMKNGetRotSpeed());
        dp->scp->rot.y = dp->path.angle;
        enAttackCheck(dp, dp->anim == 3 ? (unsigned char)0x26 : (unsigned char)0x27);
        if (dp->anim_n == -1) {
            if (++dp->sslv < 6) {
                enAttackStart(dp);
                enMKNAnimeSet(dp, dp->anim == 3 ? (unsigned char)4 : (unsigned char)3);
            } else {
                EN_SET_LEVEL(dp, 5);
            }
        }
        return;
    }
    if (dp->sslv == 1 && dp->anim_n == -1) {
        tp = enGetNearOtherEnemy(dp);
        if (tp != NULL && enGetSprayPower() != 0 && enDiff(tp->scp->pos.y, dp->scp->pos.y) < 250.0f &&
            enDistXZ((float *)&tp->scp->pos, (float *)&dp->scp->pos) < 1100.0f) {
            dp->mkn.tp = tp;
            enAttackStart(dp);
            enMKNAnimeSet(dp, ((shRandI() >> 20) & 1) + 3);
            enInitPath(&dp->path, dp->scp->rot.y);
            dp->sslv = 4;
            return;
        }
        dp->sslv++;
        enSetTimer(dp, 600 - enGetMode() * 90);
    } else if (dp->sslv == 2 && enReduceTimer(dp) <= 0) {
        EN_SET_LEVEL(dp, 5);
    }
}

/* FAKEMATCH: the float-constant order of the enSetNewSize calls comes from compiler leftovers (docs/toolchain.md,
 * "Root cause"); fitted, not recovered: the three (void) casts and `dp->sslv = dp->sslv + 1` in case 0, the comma
 * level change in case 3, case 5's `== 0`, enMKNCtrlConfuse's (see there) and its one-line EN_SET_LEVEL(dp, 5). */
static void enMKNCtrlDown(struct EnLOCAL_DATA *dp) {
    switch (dp->sslv) {
    case 0:
        (void)enResetDamage(dp);
        (void)enFlagSetRotFloor(dp);
        (void)enFlagSetLieDown(dp);
        enSetNewSize(dp, 250.0f, 200.0f, 100.0f, 200.0f);
        if (enCheckDeath(dp)) {
            enFlagResetMoved(dp);
            if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 12);
                break;
            }
            enMKNSetDc(dp);
            dp->sslv = 4;
            break;
        }
        enMKNSetDownTime(dp);
        if (dp->scp->en_first_status == 3 || dp->scp->en_first_status == 4) {
            dp->timer += enCalcTimer(600);
        }
        dp->sslv = dp->sslv + 1;
        break;
    case 1:
        if (enCheckDamage(dp)) {
            enMKNAnimeReset(dp, dp->lie + 26);
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckDeath(dp)) {
                enFlagResetMoved(dp);
                if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                    enKillCountUp(dp);
                    EN_SET_LEVEL(dp, 12);
                    break;
                }
                enMKNSetDc(dp);
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
                EN_SET_LEVEL(dp, 12);
                break;
            }
            if (dp->type == 5) {
                dp->timer += enCalcTimer(600);
                break;
            }
            enMKNAnimeSet(dp, dp->lie + 22);
            enAddHP(dp, 50.0f);
            enFlagResetRotFloor(dp);
            enFlagResetLieDown(dp);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
            dp->sslv = 2;
            break;
        }
        if (dp->flag & 0x2000) {
            if (dp->anim >= 24 && dp->anim <= 25) {
                if (100.0f * shRandF() > dp->scp->battle.hp_rate) {
                    enMKNAnimeReset(dp, dp->lie + 26);
                    enAddHP(dp, 10.0f);
                }
            } else if (200.0f * shRandF() < 100.0f + dp->scp->battle.hp_rate) {
                enMKNAnimeSet(dp, dp->lie + 24);
            } else {
                enAddHP(dp, 10.0f);
            }
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            enResetDamage(dp);
            enFlagResetNoDamage(dp);
            if (enMKNCanSeePlayer(dp) <= 0) {
                EN_SET_LEVEL(dp, 5);
            } else {
                EN_SET_LEVEL(dp, 6);
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
                    dp->slv = 12, dp->sslv = 0;
                    break;
                }
                enMKNSetDc(dp);
                dp->sslv = 4;
                break;
            }
            if (enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 12);
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
                EN_SET_LEVEL(dp, 12);
                break;
            }
            if (dp->type == 5) {
                enSetTimer(dp, 600);
                dp->sslv = 1;
                break;
            }
            if (enCheckFinishedByHuman(dp)) {
                dp->sslv = 5;
                break;
            }
            enMKNAnimeSet(dp, dp->lie + 22);
            enAddHP(dp, 50.0f);
            enFlagResetRotFloor(dp);
            enFlagResetLieDown(dp);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
            dp->sslv = 2;
        }
        break;
    case 4:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            dp->hb_s *= itof(dp->mkn.dcm - dp->mkn.dc) / (dp->mkn.dcm + 1);
            if (++dp->mkn.dc >= dp->mkn.dcm || enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 12);
                break;
            }
            enSetTimer(dp, 200);
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            enKillCountUp(dp);
            EN_SET_LEVEL(dp, 12);
        }
        break;
    case 5:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckDeath(dp)) {
                enFlagResetMoved(dp);
                if (enCheckAimedByHuman(dp) == 0 || enCheckInstantDeath(dp)) {
                    enKillCountUp(dp);
                    EN_SET_LEVEL(dp, 12);
                    break;
                }
                enMKNSetDc(dp);
                dp->sslv = 4;
                break;
            }
            enSetTimer(dp, 90);
            dp->sslv = 3;
            break;
        }
        if (!enCheckFinishedByHuman(dp)) {
            enMKNAnimeSet(dp, dp->lie + 22);
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

static int enMKNCanSeePlayer(struct EnLOCAL_DATA *dp) {
    float *ppos;
    float dist;
    float a;
    int wcd;

    ppos = enGetPlayerPos(dp);
    wcd = enGetWorldCondition();
    dist = enCheckPath2(dp, ppos, (float *)&dp->scp->pos);
    if (dist >= 0.0f) {
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
        if (dist > 2000.0f) {
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
        if (dist > 1100.0f) {
            return 0;
        }
        break;
    }
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a = enCalcAngleDifference(a, dp->scp->rot.y);
    if (dist < 1100.0f && a < 0.2617994f && !enCheckNoDamageHuman(dp)) {
        return 2;
    }
    if (enLocalWork.Status & 1) {
        return 0;
    }
    if (wcd >= 2 && enCheckSeeLight(dp)) {
        return 1;
    }
    if (a > 1.0471976f) {
        return 0;
    }
    return 1;
}

static int enMKNCanSeePlayer2(struct EnLOCAL_DATA *dp) {
    float *ppos;
    float dist;
    float a;
    float a1;
    float a2;
    int wcd;

    ppos = enGetPlayerPos(dp);
    wcd = enGetWorldCondition();
    if (fabsf(ppos[1] - dp->scp->pos.y) > 500.0f) {
        return 0;
    }
    dist = enCheckPath2(dp, ppos, (float *)&dp->scp->pos);
    if (dist >= 0.0f) {
        return 0;
    }
    dist = dp->p_dist;
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a1 = enCalcAngleDifference(a, dp->scp->rot.y);
    if (wcd == 1) {
        if (dist > 3500.0f) {
            return 0;
        }
    } else if (enCheckPlayerLight()) {
        a2 = enCalcAngleDifference(PI + enGetPlayerAngle(dp), a);
        if (dist > 2250.0f || a2 > 0.6981317f) {
            if (dist < 900.0f) {
                return 1;
            }
            return 0;
        }
    } else {
        if (dist > 1250.0f || a1 > 1.5707964f) {
            if (dist < 900.0f) {
                return 1;
            }
            return 0;
        }
    }
    if (dist < 1100.0f && a1 < 0.2617994f && !enCheckNoDamageHuman(dp)) {
        return 2;
    }
    return 1;
}

static void enMKNAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        enAnimeRestart(dp);
        dp->anim_s = 0x1000;
        if (anim == 2) {
            enMKNSetMoveCount(dp);
        }
        return;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 1440
    fjAssert(anim >= 0 && anim < sizeof(EnMKNAnime) / sizeof(EnANIME_DATA));
    if (anim == 28) {
        enAnimeSetDirectFrame(dp, anim, EnMKNAnime[anim].Anime, 0);
        return;
    }
    enAnimeSet(dp, anim, EnMKNAnime[anim].Anime);
    if (anim == 2) {
        enMKNSetMoveCount(dp);
    }
}

static void enMKNAnimeReset(struct EnLOCAL_DATA *dp, int anim) {
    /* Matching: the assert bakes its original line number into the object. */
#line 1455
    fjAssert(anim >= 0 && anim < sizeof(EnMKNAnime) / sizeof(EnANIME_DATA));
    if (anim == 28) {
        enAnimeSetDirectFrame(dp, anim, EnMKNAnime[anim].Anime, 0);
        return;
    }
    enAnimeSet(dp, anim, EnMKNAnime[anim].Anime);
    if (anim == 2) {
        enMKNSetMoveCount(dp);
    }
}

static void enMKNAnimeExec(struct EnLOCAL_DATA *dp) {
    int of;

    of = dp->anim_n;
    enAnimeExec(dp, EnMKNAnime, 0x13ED);
    if (dp->anim_n == -1 || (dp->flag & 1)) {
        return;
    }
    if (dp->anim >= 14 && dp->anim <= 21) {
        enSetTrans(dp);
    } else if (dp->anim >= 3 && dp->anim <= 4) {
        enSetTransWalk(dp);
    } else if (dp->anim >= 11 && dp->anim <= 12) {
        enSetTransWalk(dp);
    } else if (dp->anim == 1) {
        enSetTransWalk(dp);
        if ((dp->anim_s > 0 && ((of < 0x12C0 && dp->anim_n >= 0x12C0) || (of < 0x9F60 && dp->anim_n >= 0x9F60))) ||
            (dp->anim_s < 0 && ((of > 0x7080 && dp->anim_n <= 0x7080) || (of > 0x10680 && dp->anim_n <= 0x10680)))) {
            if (enCheckWater(dp)) {
                enSoundCall(0x4978, 1.0f, (float *)&dp->scp->pos);
            } else {
                if (dp->flag & 0x1000) {
                    enSoundCall3D(0x30D4, 1.0f, (float *)&dp->scp->pos);
                } else {
                    enSoundCall(0x30D4, 1.0f, (float *)&dp->scp->pos);
                }
            }
            shEnemyMKN_EffectFoot(dp->scp, dp->anim_n < 0x9600 ? 1 : 0);
        }
    }
}

static void enMKNAutoRecovery(struct EnLOCAL_DATA *dp) {
    short recover_rate[5] = { 0, 10, 30, 60, 100 };

    enAddEnduranceDT(dp, itof(recover_rate[enGetMode()]));
}

static float enMKNGetWalkSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.7f, 0.8f, 1.0f, 1.2f, 1.5f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.5f + dp->scp->battle.hp_rate / 200.0f;
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

static float enMKNGetFeelRange(void) {
    float feel_range[5] = { 400.0f, 750.0f, 1000.0f, 1500.0f, 1500.0f };

    if (enGetWorldCondition() == 4) {
        return 500.0f;
    }
    return feel_range[enGetMode()];
}

static float enMKNGetAttackProbability(void) {
    float attack_rate[5] = { 0.02f, 0.5f, 0.8f, 0.9f, 1.0f };

    if (BgIsOut(0)) {
        return 1.0f;
    }
    return attack_rate[enGetMode()];
}

static float enMKNGetRepertAttackProbability(void) {
    float attack_rate[5] = { 0.0f, 0.0f, 0.2f, 0.6f, 1.0f };

    return attack_rate[enGetMode()];
}

static float enMKNGetAttackSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.5f, 0.8f, 1.0f, 1.2f, 1.5f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.75f + dp->scp->battle.hp_rate / 400.0f;
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

static float enMKNGetRotSpeed(void) {
    float rot_rate[5] = { 0.5f, 0.8f, 1.0f, 1.2f, 1.5f };

    return 0.05235988f * rot_rate[enGetMode()];
}

static void enMKNSetDownTime(struct EnLOCAL_DATA *dp) {
    short down_time[5] = { 900, 500, 400, 300, 240 };

    enSetTimer(dp, down_time[enGetMode()]);
}

static void enMKNSetMoveCount(struct EnLOCAL_DATA *dp) {
    int n;

    n = ftoi(6.0f - 0.2f * enGetMode() + shSway1f(-2.0f, 0.5f));
    enSetAnimeCount(dp, n << 11);
}

static void enMKNSetDc(struct EnLOCAL_DATA *dp) {
    enSetTimer(dp, 200);
    dp->mkn.dc = 0;
    dp->mkn.dcm = (shRandI() >> 10) % (enGetMode() + 2) + 4;
}

static void enMKNSoundLife(struct EnLOCAL_DATA *dp) {
    if (dp->sound_wait < 300) {
        dp->sound_wait++;
        return;
    }
    if (dp->anim != 2 && shRandF() >= 0.2f * shGetDT()) {
        return;
    }
    enSoundCall(ftoi(4.0f * shRandF()) + 0x30D9, 1.0f, (float *)&dp->scp->pos);
    dp->sound_wait = 0;
}
