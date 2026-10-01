/*
 * en_nse.c: AI of the NSE enemy: sleeps, wanders, is wary of the player, chases and attacks,
 * takes damage, goes down or confused. Shared enemy code is in en_common.c.
 * NSE is the Bubble Head Nurse (suspected); XOO, which shares this AI, is unknown (docs/characters.md).
 */
#include "sh2.h"

typedef struct EnANIME_DATA EnANIME_DATA;

/* Set a new sub level (slv) and restart its step (sslv). Matching: the do/while leaves the
 * original's extra nop where a use ends a body that falls into a join. */
#define EN_SET_LEVEL(dp, lv) do { (dp)->slv = (lv); (dp)->sslv = 0; } while (0)

/* The asserts are fjAssert (common.h). */

#include "math_const.h"

/* Matching: local copies of asm_helpers.h/sh_vu0.h helpers, renamed `*_local`; with those headers
 * (or enemy.h) included instead, enNSEInitData's float constants load in another order. */
/* float to int (truncating) on the FPU */
inline int ftoi_local(float f) {
    int r;

    asm {
        cvt.w.s f, f
        mfc1    r, f
    }
    return r;
}

/* int to float on the FPU */
inline float itof_local(int i) {
    float f;

    asm {
        mtc1    i, f
        cvt.s.w f, f
    }
    return f;
}

static inline void _shAddVector_local(float *v0, float *v1, float *v2) {
    __asm__ __volatile__("
    lqc2      vf4, 0x0(%1)
    lqc2      vf5, 0x0(%2)
    vadd.xyzw vf4, vf4, vf5
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "r"(v2));
}

static void enNSECtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enNSECtrlSleep(struct EnLOCAL_DATA *dp);
static void enNSECtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enNSECtrlEvent(struct EnLOCAL_DATA *dp);
static void enNSECtrlHand(struct EnLOCAL_DATA *dp);
static void enNSECtrlWander(struct EnLOCAL_DATA *dp);
static void enNSECtrlPrecaution(struct EnLOCAL_DATA *dp);
static void enNSECtrlChase(struct EnLOCAL_DATA *dp);
static void enNSECtrlAttack(struct EnLOCAL_DATA *dp);
static void enNSECtrlDamage(struct EnLOCAL_DATA *dp);
static void enNSECtrlConfuse(struct EnLOCAL_DATA *dp);
static void enNSECtrlDown(struct EnLOCAL_DATA *dp);
static int enNSECanSeePlayer(struct EnLOCAL_DATA *dp);
static void enNSEAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enNSEAnimeReset(struct EnLOCAL_DATA *dp, int anim);
static void enNSEAnimeExec(struct EnLOCAL_DATA *dp);
static void enNSEAutoRecovery(struct EnLOCAL_DATA *dp);
static float enNSEGetWalkSpeed(struct EnLOCAL_DATA *dp);
static float enNSEGetFeelRange(void);
static float enNSEGetAttackProbability(void);
static float enNSEGetAttackSpeed(struct EnLOCAL_DATA *dp);
static float enNSEGetRotSpeed(void);
static void enNSESetDownTime(struct EnLOCAL_DATA *dp);
static void enNSESetMoveCount(struct EnLOCAL_DATA *dp);
static void enNSESetDc(struct EnLOCAL_DATA *dp);
static void enNSESoundSigns(struct EnLOCAL_DATA *dp);
static void enNSESoundVoice(struct EnLOCAL_DATA *dp);
static void enNSESoundAttackVoice(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnNSEAnime[28] = {
    { 0x157D, 0 }, { 0x157E, 1 }, { 0x157D, 1 }, { 0x157F, 0 }, { 0x1580, 0 }, { 0x1581, 0 }, { 0x1582, 0 },
    { 0x1583, 0 }, { 0x1584, 0 }, { 0x1585, 0 }, { 0x1586, 0 }, { 0x1587, 0 }, { 0x1588, 0 }, { 0x1581, 0 },
    { 0x1589, 0 }, { 0x158A, 0 }, { 0x158B, 0 }, { 0x158C, 0 }, { 0x158D, 0 }, { 0x158E, 0 }, { 0x158F, 0 },
    { 0x1590, 0 }, { 0x1592, 0 }, { 0x1591, 0 }, { 0x1594, 1 }, { 0x1593, 1 }, { 0x1596, 0 }, { 0x1595, 0 },
};

/** Sets up a new NSE: size, HP by difficulty mode scaled by the spawn status, which also
 * picks the starting level (e.g. lying down).
 * @param dp enemy work */
void enNSEInitData(struct EnLOCAL_DATA *dp) {
    short vitarity[5] = { 400, 500, 800, 1500, 9999 };
    float endurance[5] = { 10.0f, 190.0f, 285.0f, 380.0f, 450.0f };
    int mode;
    float rate;

    mode = enGetMode();
    dp->mlv = 1;
    dp->slv = 0;
    dp->sslv = 0;
    enSetBattleTarget(dp, 0);
    enSetSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
    dp->weight = 2;
    switch (dp->scp->en_first_status) {
    case 1:
        rate = 1.2f;
        break;
    case 2:
        rate = 0.5f;
        break;
    case 4:
        dp->type = 1;
    case 3:
        rate = 0.3f;
        dp->slv = 6;
        dp->sslv = 0;
        enNSEAnimeSet(dp, 26);
        enSetSize(dp, 200.0f, 200.0f, 100.0f, 200.0f);
        break;
    case 5:
        dp->slv = 7;
        dp->sslv = 0;
        enNSEAnimeSet(dp, dp->lie + 26);
        return;
    default:
        rate = 1.0f;
        break;
    }
    if (dp->scp->kind == 0x20B) {
        rate *= 1.2f;
    }
    enSetHP(dp, rate * vitarity[mode], rate * endurance[mode]);
    enSetSeeLightStatus(dp, 1000.0f, 1500.0f);
    enNSEAnimeSet(dp, 1);
    enFlagSetMoved(dp);
    if (enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

/** Per-frame control of an NSE: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enNSECtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlNSEFunc[7])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enNSECtrlAutomatic, enNSECtrlSleep, enNSECtrlGoPlayable, enNSECtrlEvent, enNSECtrlHand,
        enWaitRegenerate,
    };

    enCtrlNSEFunc[dp->mlv](dp);
}

static void enNSECtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlNSESubFunc[8])(struct EnLOCAL_DATA *) = {
        enNSECtrlWander, enNSECtrlPrecaution, enNSECtrlChase, enNSECtrlAttack,
        enNSECtrlDamage, enNSECtrlConfuse,    enNSECtrlDown,  enDyingExec,
    };

    enSetBattleTarget(dp, 0);
    enCtrlNSESubFunc[dp->slv](dp);
    enNSEAnimeExec(dp);
    enMoveExec(dp);
    if (dp->slv <= 6) {
        if (enCheckSleepIn(dp)) {
            enSleepIn(dp);
        } else {
            enNSESoundSigns(dp);
        }
    }
}

static void enNSECtrlSleep(struct EnLOCAL_DATA *dp) {
    if (enCheckSleepOut(dp)) {
        enSleepOut(dp);
    }
}

static void enNSECtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    dp->slv = 0;
    dp->sslv = 0;
}

static void enNSECtrlEvent(struct EnLOCAL_DATA *dp) {
    void *tmp;

    tmp = dp; /* Matching: reconstructed dead store: the DWARF lists dp and tmp (both referenced), the code is empty. */
}

static void enNSECtrlHand(struct EnLOCAL_DATA *dp) {
}

static void enNSECtrlWander(struct EnLOCAL_DATA *dp) {
    float d;
    float a;
    int t;
    float vec[4];

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 4);
        return;
    }
    t = enNSECanSeePlayer(dp);
    if (t >= 2) {
        EN_SET_LEVEL(dp, 3);
        return;
    }
    if (t == 1) {
        EN_SET_LEVEL(dp, 2);
        return;
    }
    if (dp->p_dist < enNSEGetFeelRange() || enCheckPlayerSound(dp)) {
        EN_SET_LEVEL(dp, 1);
        return;
    }
    a = dp->scp->rot.y;
    switch (dp->sslv) {
    case 0:
        enNSEAnimeSet(dp, 1);
        enInitPath(&dp->path, a);
        dp->nse.speed = dp->nse.tspeed = 1.0f + 0.5f * shRandF();
        dp->sslv++;
        break;
    case 1:
        d = shRandF();
        if (d < 0.002f) {
            enNSEAnimeSet(dp, 2);
            enNSESoundVoice(dp);
            dp->sslv = 2;
        } else if (!dp->path.step && d < 0.02f) {
            dp->nse.tspeed = 1.0f + 0.5f * shRandF();
            a += 0.05235988f * (4.0f * (shRandF() - 0.5f));
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            enNSEAnimeSet(dp, 1);
            dp->nse.tspeed = 1.0f + 0.5f * shRandF();
            dp->sslv = 1;
        }
        break;
    }
    if (dp->anim == 1) {
        d = dp->nse.tspeed - dp->nse.speed;
        if (d < -0.01f) {
            d = -0.01f;
        }
        if (d > 0.01f) {
            d = 0.01f;
        }
        dp->nse.speed += d;
        dp->anim_s = ftoi_local(4096.0f * dp->nse.speed * enNSEGetWalkSpeed(dp));
    }
    shSinCosV_Scale(vec, a, 1000.0f);
    _shAddVector_local(vec, (float *)&dp->scp->pos, vec);
    if (enSetPath(dp, vec, (float *)&dp->scp->pos)) {
        dp->path.timer = 0;
    }
    enMoveAngle(&dp->path, enNSEGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    enNSEAutoRecovery(dp);
}

/* FAKEMATCH (for enNSECtrlAttack's float-constant order): the first level change as a comma expression is a
 * fitted spelling, not evidence; the code is the same either way. */
static void enNSECtrlPrecaution(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        enSetSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
        dp->slv = 4, dp->sslv = 0;
        return;
    }
    t = enNSECanSeePlayer(dp);
    if (t >= 2) {
        EN_SET_LEVEL(dp, 3);
        return;
    }
    if (t == 1) {
        EN_SET_LEVEL(dp, 2);
        return;
    }
    if (dp->p_dist > enNSEGetFeelRange()) {
        EN_SET_LEVEL(dp, 0);
        return;
    }
    switch (dp->sslv) {
    case 0:
        enNSEAnimeSet(dp, 2);
        dp->sslv++;
    case 1:
        if (dp->anim_n == -1) {
            enNSEAnimeReset(dp, 2);
        }
    }
    enMoveAngleToPlayer(dp, enNSEGetRotSpeed());
    enNSEAutoRecovery(dp);
}

/* FAKEMATCH (for enNSECtrlAttack's float-constant order): the first level change written out on one line and
 * `(enLocalWork.Status & 1) != 0` are fitted spellings, not evidence; the code is the same either way. */
static void enNSECtrlChase(struct EnLOCAL_DATA *dp) {
    float d;
    int t;

    if (enCheckDamage(dp)) {
        enSetSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
        dp->slv = 4; dp->sslv = 0;
        return;
    }
    t = enNSECanSeePlayer(dp);
    if (t >= 2) {
        EN_SET_LEVEL(dp, 3);
        return;
    }
    if (!t && (dp->p_dist > enNSEGetFeelRange() || (enLocalWork.Status & 1) != 0)) {
        EN_SET_LEVEL(dp, 0);
        return;
    }
    switch (dp->sslv) {
    case 0:
        enNSEAnimeSet(dp, 1);
        enSetTimer(dp, 300);
        dp->nse.speed = dp->nse.tspeed = 1.0f;
        dp->sslv++;
    case 1:
        enSetPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos);
        dp->nse.tspeed = enCalcSpeedRate(dp->path.angle, (float *)&dp->scp->pos, enGetPlayerPos(dp));
    }
    if (dp->anim == 1) {
        d = dp->nse.tspeed - dp->nse.speed;
        if (d < -0.01f) {
            d = -0.01f;
        }
        if (d > 0.01f) {
            d = 0.01f;
        }
        dp->nse.speed += d;
        dp->anim_s = ftoi_local(4096.0f * dp->nse.speed * enNSEGetWalkSpeed(dp));
    }
    enMoveAngle(&dp->path, enNSEGetRotSpeed());
    dp->scp->rot.y = dp->path.angle;
    enNSEAutoRecovery(dp);
}

/* FAKEMATCH: `enCheckDamage(dp) != 0` and the written-out `dp->flag = dp->flag & ~0x400` are chosen for the
 * enSetNewSize calls' float-constant order, not evidenced (three more in enNSECtrlChase and enNSECtrlPrecaution)
 * (docs/matching-notes.md#en_nse-ennsectrlattack). */
static void enNSECtrlAttack(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp) != 0) {
        EN_SET_LEVEL(dp, 4);
        enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
        dp->flag = dp->flag & ~0x400;
        return;
    }
    switch (dp->sslv) {
    case 0:
        if (shRandF() < enNSEGetAttackProbability()) {
            if (enNSECanSeePlayer(dp) == 3 && (dp->p_dist > 900.0f || shRandF() < 0.5f)) {
                enNSEAnimeReset(dp, 4);
            } else {
                enNSEAnimeReset(dp, 3);
            }
        } else {
            enNSEAnimeSet(dp, 2);
            dp->sslv = 2;
            break;
        }
        enNSEGetAttackSpeed(dp);
        enSetNewSize(dp, 300.0f, 800.0f, 750.0f, 750.0f);
        dp->flag |= 0x400;
        enAttackStart(dp);
        enInitPath(&dp->path, dp->scp->rot.y);
        enNSESoundAttackVoice(dp);
        dp->sslv++;
        break;
    case 1:
        if (dp->scp->kind == 0x207) {
            enAttackCheck(dp, (unsigned char)(dp->anim == 3 ? 0x28 : 0x29));
        } else {
            enAttackCheck(dp, (unsigned char)(dp->anim == 3 ? 0x2A : 0x2B));
        }
        if (enGetMode() > 2 || dp->anim == 3) {
            enMoveAngleToPlayer(dp, enNSEGetRotSpeed());
        }
        if (dp->anim_n == -1) {
            if (enNSECanSeePlayer(dp) >= 2) {
                dp->sslv = 0;
            } else {
                dp->slv = 2; dp->sslv = 0;
                enSetNewSize(dp, 150.0f, 800.0f, 750.0f, 750.0f);
                dp->flag &= ~0x400;
            }
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            if (enNSECanSeePlayer(dp) >= 2) {
                dp->sslv = 0;
            } else {
                EN_SET_LEVEL(dp, 2);
            }
        }
        break;
    }
    enNSEAutoRecovery(dp);
}

/* Matching: enNSECtrlDamage: the stand-in before it sets its float-constant argument order. */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f; }
static void enNSECtrlDamage(struct EnLOCAL_DATA *dp) {
    float d;

    switch (dp->sslv) {
    case 0:
        d = enReduceHP(dp);
        if (enCheckSpray(dp)) {
            EN_SET_LEVEL(dp, 5);
            break;
        }
        if (d <= 0.0f || (d < 100.0f && shRandF() > 0.8f + 0.002f * d)) {
            enNSEAnimeSet(dp, enGetDownMotion(dp));
        } else {
            enNSEAnimeReset(dp, enGetDamageMotion(dp));
        }
        if (dp->anim >= 14 && dp->anim <= 21) {
            dp->lie = enGetLieDirection(dp->anim);
            enFlagSetNoDamage(dp);
            enSetNewSize(dp, 400.0f, 200.0f, 100.0f, 200.0f);
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
        if (dp->anim_n == -1) {
            EN_SET_LEVEL(dp, 6);
            enFlagResetNoDamage(dp);
        }
        break;
    }
}

static void enNSECtrlConfuse(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        if (enCheckSpray(dp)) {
            dp->sslv = 0;
        } else {
            EN_SET_LEVEL(dp, 4);
            return;
        }
    }
    if (!dp->sslv) {
        enReduceHP(dp);
        enNSEAnimeSet(dp, 2);
        dp->sslv++;
    }
    if (dp->sslv == 1 && dp->anim_n == -1) {
        dp->sslv++;
        enSetTimer(dp, 600 - enGetMode() * 90);
    } else if (dp->sslv == 2 && enReduceTimer(dp) <= 0) {
        EN_SET_LEVEL(dp, 1);
    }
}

/* Matching: enNSECtrlDown: the stand-in before it sets its float-constant argument order. */
static float __stripped_float_code_103(float x0) { int i0 = (int)x0 * 4; int i1 = (int)x0 * 87; x0 += 3453.0f * x0; i1 = i1 * 380; i1 = i0 * 74; i0 = i0 * 878; i0 = i0 * 529; i1 = i1 * 182; i1 = i0 * 190; x0 += 3094.0f * x0; i0 = i0 * 923; return x0 + (float)i0 + (float)i1; } /* fitted, not recovered: 2 constants */
static void enNSECtrlDown(struct EnLOCAL_DATA *dp) {
    switch (dp->sslv) {
    case 0:
        enFlagSetRotFloor(dp);
        enResetDamage(dp);
        enFlagSetLieDown(dp);
        enSetNewSize(dp, 200.0f, 200.0f, 100.0f, 200.0f);
        if (enCheckDeath(dp)) {
            enFlagResetMoved(dp);
            if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 7);
                break;
            }
            enNSESetDc(dp);
            dp->sslv = 4;
            break;
        }
        enNSESetDownTime(dp);
        if (dp->scp->en_first_status == 3 || dp->scp->en_first_status == 4) {
            dp->timer += enCalcTimer(600);
        }
        dp->sslv++;
        break;
    case 1:
        if (enCheckDamage(dp)) {
            enAnimePause(dp);
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckDeath(dp)) {
                enFlagResetMoved(dp);
                if (!enCheckAimedByHuman(dp) || enCheckInstantDeath(dp)) {
                    enKillCountUp(dp);
                    EN_SET_LEVEL(dp, 7);
                    break;
                }
                enNSESetDc(dp);
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
                EN_SET_LEVEL(dp, 7);
                break;
            }
            if (dp->type == 1) {
                dp->timer += enCalcTimer(600);
                break;
            }
            enNSEAnimeSet(dp, dp->lie + 22);
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
                if (200.0f * shRandF() < 100.0f - dp->scp->battle.hp_rate) {
                    enAnimePause(dp);
                    enAddHP(dp, 10.0f);
                }
            } else {
                if (300.0f * shRandF() < 200.0f + dp->scp->battle.hp_rate) {
                    enNSEAnimeSet(dp, dp->lie + 24);
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
            EN_SET_LEVEL(dp, 1);
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
                    EN_SET_LEVEL(dp, 7);
                    break;
                }
                enNSESetDc(dp);
                dp->sslv = 4;
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
                EN_SET_LEVEL(dp, 7);
                break;
            }
            if (dp->type == 1) {
                enSetTimer(dp, 600);
                dp->sslv = 1;
                break;
            }
            if (enCheckFinishedByHuman(dp)) {
                dp->sslv = 5;
                break;
            }
            enNSEAnimeSet(dp, dp->lie + 22);
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
            dp->hb_s *= itof_local(dp->nse.dcm - dp->nse.dc) / (dp->nse.dcm + 1);
            if (++dp->nse.dc >= dp->nse.dcm || enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 7);
                break;
            }
            enSetTimer(dp, 200);
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            enKillCountUp(dp);
            EN_SET_LEVEL(dp, 7);
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
                    EN_SET_LEVEL(dp, 7);
                    break;
                }
                enNSESetDc(dp);
                dp->sslv = 4;
                break;
            }
            enSetTimer(dp, 90);
            dp->sslv = 3;
            break;
        }
        if (!enCheckFinishedByHuman(dp)) {
            enNSEAnimeSet(dp, dp->lie + 22);
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

static int enNSECanSeePlayer(struct EnLOCAL_DATA *dp) {
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
    case 4:
        if (dist > 1000.0f) {
            return 0;
        }
        break;
    default:
        if (dist > 4000.0f) {
            return 0;
        }
        break;
    }
    a = enCalcDirection(ppos, (float *)&dp->scp->pos);
    a = enCalcAngleDifference(a, dp->scp->rot.y);
    if (dist < 1500.0f && a < 0.08726646f && wcd != 4 && !enCheckNoDamageHuman(dp)) {
        return 3;
    }
    if (dist < 900.0f && a < 0.5235988f && !enCheckNoDamageHuman(dp)) {
        return 2;
    }
    if (enLocalWork.Status & 1) {
        return 0;
    }
    if (wcd >= 2 && enCheckSeeLight(dp)) {
        return 1;
    }
    if (a > 1.3962634f) {
        return 0;
    }
    return 1;
}

static void enNSEAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        enAnimeRestart(dp);
        if (anim == 2) {
            enNSESetMoveCount(dp);
        }
        return;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 914
    fjAssert(anim >= 0 && anim < sizeof(EnNSEAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnNSEAnime[anim].Anime);
    if (anim == 2) {
        enNSESetMoveCount(dp);
    }
}

static void enNSEAnimeReset(struct EnLOCAL_DATA *dp, int anim) {

    /* Matching: the assert bakes its original line number into the object. */
#line 925
    fjAssert(anim >= 0 && anim < sizeof(EnNSEAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnNSEAnime[anim].Anime);
    if (anim == 2) {
        enNSESetMoveCount(dp);
    }
}

static void enNSEAnimeExec(struct EnLOCAL_DATA *dp) {
    int of;

    of = dp->anim_n;
    enAnimeExec(dp, EnNSEAnime, 0x157D);
    if (dp->anim_n == -1 || (dp->flag & 1)) {
        return;
    }
    if (dp->anim >= 14 && dp->anim <= 21) {
        enSetTrans(dp);
    } else if (dp->anim >= 11 && dp->anim <= 12) {
        enSetTransWalk(dp);
    } else if (dp->anim >= 3 && dp->anim <= 4) {
        enSetTransWalk(dp);
    } else if (dp->anim == 1) {
        enSetTransWalk(dp);
        if ((dp->anim_s > 0 && ((of < 0x960 && dp->anim_n >= 0x960) || (of < 0xA8C0 && dp->anim_n >= 0xA8C0))) ||
            (dp->anim_s < 0 && ((of > 0x8CA0 && dp->anim_n <= 0x8CA0) || (of > 0x12C00 && dp->anim_n <= 0x12C00)))) {
            if (enCheckWater(dp)) {
                enSoundCall(0x4978, 1.0f, (float *)&dp->scp->pos);
            } else {
                if (dp->flag & 0x1000) {
                    enSoundCall3D(0x2F44, 1.0f, (float *)&dp->scp->pos);
                } else {
                    enSoundCall(0x2F44, 1.0f, (float *)&dp->scp->pos);
                }
            }
        }
    }
}

static void enNSEAutoRecovery(struct EnLOCAL_DATA *dp) {
    short recover_rate[5] = { 0, 10, 30, 60, 100 };

    enAddEnduranceDT(dp, itof_local(recover_rate[enGetMode()]));
}

static float enNSEGetWalkSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.7f, 0.8f, 1.0f, 1.2f, 1.5f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.5f + dp->scp->battle.hp_rate / 200.0f;
    dp->anim_s = ftoi_local(4096.0f * r);
    return r;
}

static float enNSEGetFeelRange(void) {
    float feel_range[5] = { 1000.0f, 1250.0f, 1500.0f, 1500.0f, 1500.0f };

    if (enGetWorldCondition() == 4) {
        return 750.0f;
    }
    return feel_range[enGetMode()];
}

static float enNSEGetAttackProbability(void) {
    float attack_rate[5] = { 0.1f, 0.5f, 0.7f, 0.9f, 1.0f };

    return attack_rate[enGetMode()];
}

static float enNSEGetAttackSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.5f, 0.8f, 1.0f, 1.1f, 1.2f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.5f + dp->scp->battle.hp_rate / 200.0f;
    dp->anim_s = ftoi_local(4096.0f * r);
    return r;
}

static float enNSEGetRotSpeed(void) {
    float rot_rate[5] = { 0.5f, 0.8f, 1.0f, 1.2f, 1.5f };

    return 0.05235988f * rot_rate[enGetMode()];
}

static void enNSESetDownTime(struct EnLOCAL_DATA *dp) {
    short down_time[5] = { 600, 480, 360, 300, 240 };

    enSetTimer(dp, down_time[enGetMode()]);
}

static void enNSESetMoveCount(struct EnLOCAL_DATA *dp) {
    int n;

    n = ftoi_local(8.0f - 0.4f * enGetMode() + shSway1f(-2.0f, 0.5f));
    enSetAnimeCount(dp, n << 11);
}

static void enNSESetDc(struct EnLOCAL_DATA *dp) {
    enSetTimer(dp, 200);
    dp->nse.dc = 0;
    dp->nse.dcm = (shRandI() >> 10) % (enGetMode() + 3) + 4;
}

static void enNSESoundSigns(struct EnLOCAL_DATA *dp) {
    int signs;

    if ((dp->nse.count -= shGetDF()) <= 0) {
        dp->nse.count = enCalcTimer(60);
        do {
            signs = (shRandI() >> 20) & 3;
        } while (signs == dp->nse.signs);
        dp->nse.signs = signs;
        enSoundCall(signs + 0x2F47, 0.6f, (float *)&dp->scp->pos);
    }
}

/* Matching: enNSESoundVoice: the stand-in before it sets its float-constant argument order. */
static float __stripped_float_code_101(float x0, float x1, float x2) { x0 += 1924.0f; return x0; } /* fitted, not recovered: 1 constant */
static void enNSESoundVoice(struct EnLOCAL_DATA *dp) {
    enSoundCall(ftoi_local(4.0f * shRandF()) + 0x2F4D, 0.7f, (float *)&dp->scp->pos);
}

static void enNSESoundAttackVoice(struct EnLOCAL_DATA *dp) {
    enSoundCall(ftoi_local(6.0f * shRandF()) + 0x2F4B, 1.0f, (float *)&dp->scp->pos);
}
