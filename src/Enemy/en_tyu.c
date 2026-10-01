/*
 * en_tyu.c: AI of the "tyu" enemy family (TYU, TY2, TY3): crawls on the floor, walls and
 * ceiling, chases, gathers, attacks and escapes. Shared enemy code is in en_common.c.
 * TYU is suspected to be the Creeper, TY2 and TY3 static variants of it (docs/characters.md).
 */
#include "enemy.h"
#include "fi_libvu0_inline.h"

/** m = m rotated by rot.z, then rot.y, then rot.x. */
inline void shRotMatrixZYX(float (*m)[4], float *rot) {
    shRotMatrixZ(m, m, rot[2]);
    shRotMatrixY(m, m, rot[1]);
    shRotMatrixX(m, m, rot[0]);
}

static void enTYUCtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enTYUCtrlSleep(struct EnLOCAL_DATA *dp);
static void enTYUCtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enTYUCtrlEvent(struct EnLOCAL_DATA *dp);
static void enTYUCtrlHand(struct EnLOCAL_DATA *dp);
static void enTYUCtrlStraight(struct EnLOCAL_DATA *dp);
static void enTYUCtrlChase(struct EnLOCAL_DATA *dp);
static void enTYUCtrlGathering(struct EnLOCAL_DATA *dp);
static void enTYUCtrlEscape(struct EnLOCAL_DATA *dp);
static void enTYUCtrlOnWall(struct EnLOCAL_DATA *dp);
static void enTYUCtrlOnCeiling(struct EnLOCAL_DATA *dp);
static void enTYUCtrlAttack(struct EnLOCAL_DATA *dp);
static void enTYUCtrlDown(struct EnLOCAL_DATA *dp);
static void enTYUCtrlDead(struct EnLOCAL_DATA *dp);
static int enTYUSetPathV(struct EnLOCAL_DATA *dp);
static int enTYUCheckNormal(float *normal);
static int enTYUCheckNearWall(struct EnLOCAL_DATA *dp);
static void enTYUCheckWall(struct EnLOCAL_DATA *dp);
static void enTYUCheckUnderWall(struct EnLOCAL_DATA *dp);
static int enTYUCanSeePlayer(struct EnLOCAL_DATA *dp);
static void enTYUCheckNearPlayer(struct EnLOCAL_DATA *dp);
static void enTYUAnimeSet(struct EnLOCAL_DATA *dp, int anim);
static void enTYUAnimeExec(struct EnLOCAL_DATA *dp);
static float enTYUGetSpeed(void);
static void enTYUSetAttackTime(struct EnLOCAL_DATA *dp);
static void enTYUSetStopTime(struct EnLOCAL_DATA *dp);
static void enTYUSetDownTime(struct EnLOCAL_DATA *dp);
static void enTYUSoundMove(struct EnLOCAL_DATA *dp);
static void enTY2CtrlFloor(struct EnLOCAL_DATA *dp);
static void enTY2CtrlWall(struct EnLOCAL_DATA *dp);
static void enTY2CtrlCeiling(struct EnLOCAL_DATA *dp);
static void enTY2CtrlDead(struct EnLOCAL_DATA *dp);
static void enTY2CheckCreateCharacter(struct EnLOCAL_DATA *dp);
static void enTY3CtrlFloor(struct EnLOCAL_DATA *dp);
static void enTY3CtrlWall(struct EnLOCAL_DATA *dp);
static void enTY3CtrlCeiling(struct EnLOCAL_DATA *dp);
static void enTY3CheckCreateCharacter(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnTYUAnime[2] = {
    { 0x1451, 0 },
    { 0x1452, 1 },
};
/* Matching: fitted stand-in (not recovered code); it sets the next function's float-constant
 * order. Whether the original had code here: docs/stand-ins.md. */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f; }
/** Sets up a new TYU: size, HP by difficulty mode, starting level from the spawn status.
 * @param dp enemy work */
void enTYUInitData(struct EnLOCAL_DATA *dp) {
    float vitarity[5] = { 1.0f, 250.0f, 400.0f, 600.0f, 1000.0f };
    float endurance[5] = { 1.0f, 100.0f, 150.0f, 240.0f, 300.0f };
    int mode;

    mode = enGetMode();
    dp->mlv = 1;
    switch (dp->type = dp->scp->en_first_status) {
    case 1:
    case 3:
        EN_SET_LEVEL(dp, 4);
        break;
    case 4:
        EN_SET_LEVEL(dp, 5);
        break;
    case 5:
        EN_SET_LEVEL(dp, 8);
        return;
    default:
        dp->slv = 0;
        dp->sslv = 0;
        break;
    }
    dp->tyu.tcomm = NULL;
    enSetSize(dp, 100.0f, 150.0f, 75.0f, 100.0f);
    dp->weight = 1;
    enSetHP(dp, vitarity[mode], endurance[mode]);
    enTYUAnimeSet(dp, 1);
    enFlagSetRotFloor(dp);
    enFlagSetLieDown(dp);
    enFlagSetMoved(dp);
    dp->tyu.moves = -1;
    enLocalWork.ActiveEnemy++;
    if (enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
    dp->flag = 0x10;
}

/** Per-frame control of a TYU: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enTYUCtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlTYUFunc[7])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enTYUCtrlAutomatic, enTYUCtrlSleep, enTYUCtrlGoPlayable, enTYUCtrlEvent, enTYUCtrlHand,
        enWaitRegenerate,
    };

    enCtrlTYUFunc[dp->mlv](dp);
}

static void enTYUCtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlTYUSubFunc[9])(struct EnLOCAL_DATA *) = {
        enTYUCtrlStraight, enTYUCtrlChase,  enTYUCtrlGathering, enTYUCtrlEscape, enTYUCtrlOnWall,
        enTYUCtrlOnCeiling, enTYUCtrlAttack, enTYUCtrlDown,      enTYUCtrlDead,
    };

    enSetBattleTarget(dp, 0);
    enCtrlTYUSubFunc[dp->slv](dp);
    enTYUAnimeExec(dp);
    enMoveExec(dp);
    if (dp->slv <= 7 && enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

static void enTYUCtrlSleep(struct EnLOCAL_DATA *dp) {
    if (enCheckSleepOut(dp)) {
        enSleepOut(dp);
        enFlagSetLieDown(dp);
        enLocalWork.ActiveEnemy++;
        switch (dp->type) {
        case 1:
        case 3:
            EN_SET_LEVEL(dp, 4);
            break;
        case 4:
            EN_SET_LEVEL(dp, 5);
            break;
        default:
            dp->slv = 0;
            dp->sslv = 0;
            enFlagSetRotFloor(dp);
            break;
        }
    }
}

static void enTYUCtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    EN_SET_LEVEL(dp, 0);
}

static void enTYUCtrlEvent(struct EnLOCAL_DATA *dp) {
    void *tmp;

    tmp = dp; /* Matching: reconstructed dead store: the DWARF lists dp and tmp (both referenced), the code is empty. */
}

static void enTYUCtrlHand(struct EnLOCAL_DATA *dp) {
}

static void enTYUCtrlStraight(struct EnLOCAL_DATA *dp) {
    float vec[4];
    int t;
    struct EnCOMMUNICATION *comm;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 7);
        return;
    }
    if (!dp->sslv) {
        enTYUAnimeSet(dp, 1);
        dp->flag = 0x10;
        enFlagSetRotFloor(dp);
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    if (dp->sslv == 2) {
        dp->scp->rot.y = enCalcDirection(dp->tyu.point, (float *)&dp->scp->pos);
        shSinCosV_Scale(dp->vec, dp->scp->rot.y, enTYUGetSpeed());
        if (enDistXZ((float *)&dp->scp->pos, dp->tyu.point) <= 100.0f) {
            dp->scp->pos.y -= 100.0f;
            dp->flag |= 2;
            dp->type = 3;
            EN_SET_LEVEL(dp, 4);
        }
        return;
    }
    if ((comm = enCommunicateTribe(3, (float *)&dp->scp->pos))) {
        dp->tyu.tcomm = comm;
        EN_SET_LEVEL(dp, 2);
        return;
    }
    if (shRandF() < 0.003f) {
        dp->path.angle = shAngleRegulate(dp->path.angle + PI * shRandF() / 3.0f);
    }
    shSinCosV_Scale(vec, dp->scp->rot.y, 500.0f);
    _shAddVector(vec, (float *)&dp->scp->pos, vec);
    if (enSetPath(dp, vec, (float *)&dp->scp->pos) && dp->type == 2) {
        struct _CL_VHIT_RESULT *res;
        res = &enLocalWork.HitResult;
        if (res->kind == 1) {
            vcopy(res->hobj.wall.cp, dp->tyu.point);
            dp->sslv = 2;
            return;
        }
    }
    enMoveAngle(&dp->path, 0.08726646f);
    shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enTYUGetSpeed());
    t = enTYUCanSeePlayer(dp);
    if (t == 2) {
        EN_SET_LEVEL(dp, 6);
    } else if (t == 1) {
        EN_SET_LEVEL(dp, 1);
    }
    if (enCheckFinishedByHuman(dp) && dp->p_dist < 200.0f) {
        vzero(dp->vec);
    }
    enTYUCheckNearPlayer(dp);
    enTYUSoundMove(dp);
    enTYUCheckWall(dp);
}

static void enTYUCtrlChase(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 7);
        return;
    }
    if (!dp->sslv) {
        enTYUAnimeSet(dp, 1);
        dp->flag = 0x410;
        enInitPath(&dp->path, enGetPlayerDirection(dp));
        enFlagSetRotFloor(dp);
        dp->sslv++;
    }
    enSetPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, 0.08726646f);
    shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle,
                    enTYUGetSpeed() * enCalcSpeedRate(dp->path.angle, (float *)&dp->scp->pos, enGetPlayerPos(dp)));
    t = enTYUCanSeePlayer(dp);
    if (t == 2) {
        EN_SET_LEVEL(dp, 6);
    } else if (t == 0) {
        EN_SET_LEVEL(dp, 0);
    }
    if (enCheckFinishedByHuman(dp) && dp->p_dist < 200.0f) {
        vzero(dp->vec);
    }
    enTYUCheckNearPlayer(dp);
    enTYUSoundMove(dp);
    enTYUCheckWall(dp);
}

static void enTYUCtrlGathering(struct EnLOCAL_DATA *dp) {
    int t;
    struct EnCOMMUNICATION *comm;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 7);
        return;
    }
    if ((comm = enCommunicateTribe(3, (float *)&dp->scp->pos)) != dp->tyu.tcomm) {
        dp->tyu.tcomm = comm;
        if (!comm) {
            EN_SET_LEVEL(dp, 0);
            return;
        }
    }
    if (!dp->sslv) {
        enTYUAnimeSet(dp, 1);
        enFlagSetRotFloor(dp);
        dp->sslv++;
    }
    enSetPath(dp, comm->pos, (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, 0.08726646f);
    shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle,
                    enTYUGetSpeed() * enCalcSpeedRate(dp->path.angle, (float *)&dp->scp->pos, comm->pos));
    t = enTYUCanSeePlayer(dp);
    if (t == 2) {
        EN_SET_LEVEL(dp, 6);
    } else if (t == 1) {
        dp->slv = 1;
        dp->sslv = 0;
    }
    if (enCheckFinishedByHuman(dp) && dp->p_dist < 200.0f) {
        vzero(dp->vec);
    }
    enTYUCheckNearPlayer(dp);
    enTYUSoundMove(dp);
    enTYUCheckWall(dp);
}

static void enTYUCtrlEscape(struct EnLOCAL_DATA *dp) {
    int t;
    float vec[4];

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 7);
        return;
    }
    if (!dp->sslv) {
        enTYUAnimeSet(dp, 1);
        enInitPath(&dp->path, dp->scp->rot.y);
        enFlagSetRotFloor(dp);
        dp->sslv++;
    }
    if (dp->p_dist > 500.0f) {
        dp->flag &= ~0x10;
    }
    shSinCosV_Scale(vec, PI / 2 + enGetPlayerDirection(dp), 500.0f);
    _shAddVector(vec, (float *)&dp->scp->pos, vec);
    if (enSetPath(dp, vec, (float *)&dp->scp->pos) && dp->type == 2) {
        struct _CL_VHIT_RESULT *res;
        res = &enLocalWork.HitResult;
        if (res->kind == 1) {
            vcopy(res->hobj.wall.cp, dp->tyu.point);
            EN_SET_LEVEL(dp, 0);
            dp->sslv = 2;
            return;
        }
    }
    enMoveAngle(&dp->path, 0.08726646f);
    shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enTYUGetSpeed());
    if (dp->path.dist < 250.0f && enLocalWork.HitResult.kind == 1) {
        EN_SET_LEVEL(dp, 0);
        return;
    }
    t = enTYUCanSeePlayer(dp);
    if (!t) {
        EN_SET_LEVEL(dp, 0);
    }
    if (enCheckFinishedByHuman(dp) && dp->p_dist < 200.0f) {
        vzero(dp->vec);
        EN_SET_LEVEL(dp, 0);
    }
    enTYUCheckNearPlayer(dp);
    enTYUSoundMove(dp);
    enTYUCheckWall(dp);
}

static void enTYUCtrlOnWall(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 7);
        dp->sslv = -1;
        return;
    }
    if (!dp->sslv) {
        enTYUAnimeSet(dp, 1);
        if (!enTYUCheckNearWall(dp)) {
            if (dp->type == 3) {
                dp->type = 2;
                /* Matching: one line in the original; EN_SET_LEVEL here and below (not the two stores)
                 * sets this function's float-constant order. */
                EN_SET_LEVEL(dp, 0);
            }
            return;
        }
        dp->sslv++;
    }
    enTYUSetPathV(dp);
    if (dp->path.dist < 250.0f) {
        if (dp->type == 3 && enTYUCheckNormal(enLocalWork.HitResult.hobj.wall.nl)) {
            if (dp->path.dist < 100.0f) {
                float vec[4];

                dp->scp->rot.y = shAngleRegulate(PI - dp->scp->rot.z);
                dp->scp->rot.x = dp->trx = 0.0f;
                dp->scp->rot.z = dp->trz = 0.0f;
                dp->flag = 0x410;
                enFlagSetRotFloor(dp);
                dp->type = 2;
                EN_SET_LEVEL(dp, 0);
                shSinCosV_Scale(vec, dp->scp->rot.y, 100.0f);
                _shAddVector((float *)&dp->scp->pos, enLocalWork.HitResult.hobj.wall.cp, vec);
                dp->scp->pos.w = 1.0f;
                return;
            }
        } else {
            dp->path.angle = dp->path.markangle;
            if (dp->path.dist < 100.0f) {
                dp->scp->rot.y = dp->path.angle = dp->path.markangle = PI;
            }
        }
    } else {
        enMoveAngle(&dp->path, 0.08726646f);
    }
    if (dp->scp->pos.y < enGetPlayerPos(dp)[1] - 2000.0f) {
        dp->path.angle = dp->path.markangle = PI;
    }
    dp->scp->rot.y = dp->path.angle;
    enMakeRotVector(dp->vec, (float *)&dp->scp->rot, enTYUGetSpeed());
    enTYUSoundMove(dp);
    enTYUCheckUnderWall(dp);
}

static void enTYUCtrlOnCeiling(struct EnLOCAL_DATA *dp) {
    float vec[4];

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 7);
        dp->sslv = -1;
        return;
    }
    if (!dp->sslv) {
        dp->flag = 0x452;
        dp->scp->rot.x = dp->trx = 0.0f;
        dp->scp->rot.z = dp->trz = PI;
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    if (shRandF() < 0.01f) {
        dp->path.angle = shAngleRegulate(dp->path.angle + PI * shRandF() / 3.0f);
    }
    shSinCosV_Scale(vec, dp->path.angle, 500.0f);
    _shAddVector(vec, (float *)&dp->scp->pos, vec);
    enSetPath(dp, vec, (float *)&dp->scp->pos);
    enMoveAngle(&dp->path, 0.08726646f);
    shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, enTYUGetSpeed());
    enTYUSoundMove(dp);
    enTYUCheckWall(dp);
}

static void enTYUCtrlAttack(struct EnLOCAL_DATA *dp) {
    int t;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 7);
        return;
    }
    if (!dp->sslv) {
        enTYUSetAttackTime(dp);
        enAttackStart(dp);
        dp->sslv++;
    }
    enAttackCheck(dp, 0x33);
    if (enReduceTimer(dp) <= 0) {
        t = enTYUCanSeePlayer(dp);
        if (t == 2) {
            if (shRandF() < 0.1f) {
                dp->sslv = 0;
            } else {
                EN_SET_LEVEL(dp, 3);
            }
        } else if (t == 1) {
            EN_SET_LEVEL(dp, 1);
        } else {
            EN_SET_LEVEL(dp, 0);
        }
    }
    enTYUCheckNearPlayer(dp);
}

static void enTYUCtrlDown(struct EnLOCAL_DATA *dp) {
    struct _CL_VHIT_RESULT *res;

    switch (dp->sslv) {
    case 0:
        if (enCheckSpray(dp)) {
            dp->scp->battle.hp = dp->scp->battle.hp_rate = 0.0f;
        }
        if (enReduceHP(dp) <= 0.0f) {
            enTYUSetDownTime(dp);
        } else {
            enTYUSetStopTime(dp);
        }
        enSetHitBack(dp);
        if (enGetMode() > 0 && !(enLocalWork.Status & 2)) {
            enSetCommunication(3, 1, (float *)&dp->scp->pos, 5000.0f, 180);
        }
        enAnimePause(dp);
        if (enCheckDeath(dp)) {
            if (enGetMode() > 0) {
                enSetCommunication(3, 1, (float *)&dp->scp->pos, 5000.0f, 300);
            }
            if (enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 8);
                break;
            }
            enSetTimer(dp, 300);
            dp->sslv = 2;
            break;
        }
        dp->sslv++;
        break;
    case 1:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            if (enCheckDeath(dp)) {
                if (enGetMode() > 0) {
                    enSetCommunication(3, 1, (float *)&dp->scp->pos, 5000.0f, 300);
                }
                if (enCheckInstantDeath(dp)) {
                    enKillCountUp(dp);
                    EN_SET_LEVEL(dp, 8);
                    break;
                }
                enSetTimer(dp, 300);
                dp->sslv = 2;
                break;
            }
            if (dp->endurance <= 0.0f) {
                enTYUSetDownTime(dp);
            } else {
                enTYUSetStopTime(dp);
            }
            enSetHitBack(dp);
        }
        if (enReduceTimer(dp) <= 0 && !enCheckFinishedByHuman(dp)) {
            enAnimeRestart(dp);
            if (dp->endurance <= 0.0f) {
                enAddHP(dp, 100.0f);
                EN_SET_LEVEL(dp, 3);
            } else {
                EN_SET_LEVEL(dp, 1);
            }
        }
        break;
    case 2:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
            enSetHitBack(dp);
            if (enCheckInstantDeath(dp)) {
                enKillCountUp(dp);
                EN_SET_LEVEL(dp, 8);
                break;
            }
            enSetTimer(dp, 300);
            break;
        }
        if (enReduceTimer(dp) <= 0) {
            enKillCountUp(dp);
            EN_SET_LEVEL(dp, 8);
        }
        break;
    case -1:
        enReduceHP(dp);
        dp->flag = 0x432;
        dp->trx = PI / 2;
        dp->trz = PI;
        dp->sslv--;
    case -2:
        if (enCheckFloor((float *)&dp->scp->pos)) {
            res = &enLocalWork.HitResult;
            if (res->hobj.wall.cp[1] > dp->scp->pos.y - 150.0f) {
                dp->flag = 0x410;
                enFlagSetRotFloor(dp);
                dp->scp->rot.x = dp->trx = 0.0f;
                dp->scp->rot.z = dp->trz = 0.0f;
                dp->scp->pos.y = res->hobj.wall.cp[1];
                if (dp->type == 3 || (enLocalWork.Status & 2)) {
                    dp->type = 2;
                } else {
                    dp->type = 0;
                }
                dp->sslv = 0;
                break;
            }
        }
        dp->vec[1] = 1500.0f;
        break;
    }
    enTYUCheckNearPlayer(dp);
}

static void enTYUCtrlDead(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        enAnimePause(dp);
        enFlagSetDead(dp);
        enFlagResetLieDown(dp);
        enFlagResetMoved(dp);
        fogEraseObj(dp - enLocalWork.Data + 10);
        dp->sslv++;
        if (enLocalWork.Status & 2) {
            short id;
            float pos[4];
            float rot[4];
            enLocalWork.ActiveEnemy--;
            _shCopyVector(pos, (float *)&dp->scp->pos);
            _shCopyVector(rot, (float *)&dp->scp->rot);
            enEfctSetTYU2D((float *)&dp->scp->pos, (float *)&dp->scp->rot);
            enDeleteCharacter(dp);
        }
    }
}

static int enTYUSetPathV(struct EnLOCAL_DATA *dp) {
    struct EnPATH_DATA *p = &dp->path;
    float dist;
    float ma;
    float a;
    float a1;
    float a2;
    float d1;
    float d2;
    float rmat[4][4];
    float pos[4];
    float rot[4];
    int k;

    _sceVu0UnitMatrix(rmat);
    shRotMatrixZYX(rmat, (float *)&dp->scp->rot);
    vzero(rot);
    rot[1] = 60.0f;
    _sceVu0ApplyMatrix(rot, rmat, rot);
    _shSubVector(pos, (float *)&dp->scp->pos, rot);
    vcopy(&dp->scp->rot, rot);
    dist = enCheckForward(dp, pos, rot, 500.0f);
    ma = p->markangle;
    if (dist < 0.0f) {
        p->dist = -dist;
        p->step = 0;
        p->deadend = 0;
        return 0;
    }
    p->dist = dist;
    dist += 50.0f;
    switch (k = p->step) {
    case 0:
        p->step = 1;
        rot[1] = a1 = ma + 0.2617994f;
        d1 = enCheckForward(dp, pos, rot, dist);
        rot[1] = a2 = ma - 0.2617994f;
        d2 = enCheckForward(dp, pos, rot, dist);
        if (d1 < 0.0f && d2 >= 0.0f) {
            ma = a1;
        } else if (d1 >= 0.0f && d2 < 0.0f) {
            ma = a2;
        } else {
            if (d1 >= 0.0f && d2 >= 0.0f && ++p->deadend >= 3) {
                ma = PI + ma;
                p->step = -1;
            } else {
                ma -= 0.0017453292f;
                p->step = 2;
            }
        }
        break;
    case 1:
        a = shAngleRegulate(ma - p->angle);
        if (a == 0.0f) {
            a = 0.017453292f;
        }
        if (fabsf(a) < 0.2617994f) {
            a1 = ma;
        } else {
            a1 = p->angle + 0.2617994f * _shSign(a);
        }
        rot[1] = a1;
        d1 = enCheckForward(dp, pos, rot, dist);
        if (d1 < 0.0f) {
            ma = a1;
            break;
        }
        rot[1] = ma;
        d2 = enCheckForward(dp, pos, rot, dist);
        if (d2 < 0.0f) {
            break;
        }
        if (d1 < d2) {
            a1 = ma;
            d1 = d2;
        }
        a2 = p->angle - 0.2617994f * _shSignIP(a);
        rot[1] = a2;
        d2 = enCheckForward(dp, pos, rot, dist);
        if (d2 < 0.0f) {
            ma = a2;
            break;
        }
        if (d1 < 0.0f) {
            ma = a1;
            break;
        }
        ma = a1;
        p->step = 2;
        break;
    case -1:
        ma = p->markangle;
        if (enCalcAngleDifference(ma, p->angle) < 0.2617994f) {
            p->deadend = 0;
            p->step = 1;
        }
        break;
    default:
        a1 = ma + 0.2617994f * k;
        rot[1] = a1;
        d1 = enCheckForward(dp, pos, rot, dist);
        a2 = ma - 0.2617994f * k;
        rot[1] = a2;
        d2 = enCheckForward(dp, pos, rot, dist);
        if (d1 < 0.0f) {
            if (d2 < 0.0f) {
                if (++k >= 6) {
                    ma = a1;
                    k = 1;
                }
            } else {
                ma = a1;
                k = 1;
            }
        } else if (d2 < 0.0f) {
            ma = a2;
            k = 1;
        } else {
            if (d1 > d2) {
                ma = a1;
            } else {
                ma = a2;
            }
            if (++k >= 6) {
                ma = PI + ma;
                k = -1;
            }
        }
        p->step = k;
        break;
    }
    p->markangle = shAngleRegulate(ma);
    return 1;
}

static int enTYUCheckNormal(float *normal) {
    float dx;
    float dy;
    float dz;

    dy = normal[1];
    if (dy >= 0.0f) {
        return 0;
    }
    dx = normal[0];
    dz = normal[2];
    return dy * dy > dx * dx + dz * dz;
}

static int enTYUCheckNearWall(struct EnLOCAL_DATA *dp) {
    struct _CL_VHIT_RESULT *res = &enLocalWork.HitResult;
    float sp[4];
    float ep[4];
    float vec[4];
    float cp[4];
    float d;
    float r;
    float dm;

    dm = 3.4028235e38f;
    shSinCosV_Scale(vec, dp->path.angle, 500.0f);
    _shSubVector(sp, (float *)&dp->scp->pos, vec);
    _shAddVector(ep, (float *)&dp->scp->pos, vec);
    clCheckHitEyesOnlyWall(res, sp, ep);
    if (res->kind == 1) {
        dm = enDistXZ((float *)&dp->scp->pos, res->hobj.wall.cp);
        _shNormalize(vec, res->hobj.wall.nl);
        _shScaleVector(vec, vec, 50.0f);
        _shSubVector(cp, res->hobj.wall.cp, vec);
        r = shAtanV(res->hobj.wall.nl);
    }
    sp[0] = dp->scp->pos.x + vec[2];
    sp[2] = dp->scp->pos.z - vec[0];
    ep[0] = dp->scp->pos.x - vec[2];
    ep[2] = dp->scp->pos.z + vec[0];
    clCheckHitEyesOnlyWall(res, sp, ep);
    if (res->kind == 1) {
        d = enDistXZ((float *)&dp->scp->pos, res->hobj.wall.cp);
        if (d < dm) {
            dm = d;
            _shNormalize(vec, res->hobj.wall.nl);
            _shScaleVector(vec, vec, 50.0f);
            _shSubVector(cp, res->hobj.wall.cp, vec);
            r = shAtanV(res->hobj.wall.nl);
        }
    }
    sp[0] = dp->scp->pos.x - vec[2];
    sp[2] = dp->scp->pos.z + vec[0];
    ep[0] = dp->scp->pos.x + vec[2];
    ep[2] = dp->scp->pos.z - vec[0];
    clCheckHitEyesOnlyWall(res, sp, ep);
    if (res->kind == 1) {
        d = enDistXZ((float *)&dp->scp->pos, res->hobj.wall.cp);
        if (d < dm) {
            dm = d;
            _shNormalize(vec, res->hobj.wall.nl);
            _shScaleVector(vec, vec, 50.0f);
            _shSubVector(cp, res->hobj.wall.cp, vec);
            r = shAtanV(res->hobj.wall.nl);
        }
    }
    sp[0] = dp->scp->pos.x + vec[0];
    sp[2] = dp->scp->pos.z + vec[2];
    ep[0] = dp->scp->pos.x - vec[0];
    ep[2] = dp->scp->pos.z - vec[2];
    clCheckHitEyesOnlyWall(res, sp, ep);
    if (res->kind == 1) {
        d = enDistXZ((float *)&dp->scp->pos, res->hobj.wall.cp);
        if (d < dm) {
            dm = d;
            _shNormalize(vec, res->hobj.wall.nl);
            _shScaleVector(vec, vec, 50.0f);
            _shSubVector(cp, res->hobj.wall.cp, vec);
            r = shAtanV(res->hobj.wall.nl);
        }
    }
    if (dm == 3.4028235e38f) {
        return 0;
    }
    enFlagResetRotFloorJust(dp);
    dp->flag = 0x432;
    _sceVu0CopyVectorXYZ((float *)&dp->scp->pos, cp);
    dp->scp->rot.x = dp->trx = PI / 2;
    dp->scp->rot.y = 0.0f;
    dp->scp->rot.z = dp->trz = shAngleRegulate(PI - r);
    enInitPath(&dp->path, 0.0f);
    return 1;
}
/* Matching: fitted stand-in (not recovered code); it sets the next function's float-constant
 * order. Whether the original had code here: docs/stand-ins.md. */
static float __stripped_float_code_3(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f; }
static void enTYUCheckWall(struct EnLOCAL_DATA *dp) {
    struct _CL_VHIT_RESULT *res = &enLocalWork.HitResult;
    float sp[4];
    float ep[4];
    float vec[4];
    float d;
    float tpos[4];
    float r;

    shSinCosV_Scale(vec, dp->scp->rot.y, 500.0f);
    _shSubVector(sp, (float *)&dp->scp->pos, vec);
    _shAddVector(ep, (float *)&dp->scp->pos, vec);
    if (dp->flag & 0x40) {
        sp[1] += 50.0f;
        ep[1] += 50.0f;
    } else {
        sp[1] -= 50.0f;
        ep[1] -= 50.0f;
    }
    clCheckHitEyesOnlyWall(res, sp, ep);
    if (res->kind == 1) {
        d = enDistXZ(sp, res->hobj.wall.cp) - 100.0f;
        if (d < 500.0f) {
            _shNormalize(vec, res->hobj.wall.nl);
            _shScaleVector(vec, vec, 100.0f);
            _shAddVector(tpos, res->hobj.wall.cp, vec);
            r = shAtanV(res->hobj.wall.nl);
            _shCopyVector(sp, (float *)&dp->scp->pos);
            if (dp->flag & 0x40) {
                sp[1] += 50.0f;
            } else {
                sp[1] -= 50.0f;
            }
            clCheckHitEyesOnlyWall(res, sp, tpos);
            if (res->kind != 1) {
                vcopy3(tpos, &dp->scp->pos);
                vcopy(&dp->scp->pos, &dp->scp->b_pos);
                vzero(dp->vec);
                dp->scp->rot.y = r;
                enInitPath(&dp->path, r);
            }
        }
    }
}

static void enTYUCheckUnderWall(struct EnLOCAL_DATA *dp) {
    struct _CL_VHIT_RESULT *res;
    float sp[4];
    float ep[4];
    float vec[4];

    res = &enLocalWork.HitResult;
    shSinCosV_Scale(vec, -dp->scp->rot.z, 500.0f);
    _shSubVector(sp, (float *)&dp->scp->pos, vec);
    _shAddVector(ep, (float *)&dp->scp->pos, vec);
    clCheckHitEyesOnlyWall(res, sp, ep);
    if (res->kind == 1) {
        _shNormalize(vec, res->hobj.wall.nl);
        _shScaleVector(vec, vec, 50.0f);
        _shSubVector(vec, res->hobj.wall.cp, vec);
        dp->scp->pos.x = vec[0];
        dp->scp->pos.z = vec[2];
    } else {
        EN_SET_LEVEL(dp, 7);
        dp->sslv = -1;
    }
}

static int enTYUCanSeePlayer(struct EnLOCAL_DATA *dp) {
    float dist;
    float a;
    float feel_range[5] = { 0.0f, 500.0f, 600.0f, 750.0f, 1000.0f };

    if (enGetMode() <= 0) {
        return 0;
    }
    dist = dp->p_dist;
    a = enCalcAngleDifference(enGetPlayerDirection(dp), dp->scp->rot.y);
    if (dist < 150.0f && a < 1.0471976f) {
        return 2;
    }
    if (enLocalWork.Status & 1) {
        return 0;
    }
    return !(dist > feel_range[enGetMode()]);
}

static void enTYUCheckNearPlayer(struct EnLOCAL_DATA *dp) {
    enCheckNearPlayer(dp, &dp->tyu.near_count, &dp->tyu.dist, 200.0f);
}

static void enTYUAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        enAnimeRestart(dp);
        return;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 1089
    fjAssert(anim >= 0 && anim < sizeof(EnTYUAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnTYUAnime[anim].Anime);
}

static void enTYUAnimeExec(struct EnLOCAL_DATA *dp) {
    enAnimeExec(dp, EnTYUAnime, 0x1451);
}

static float enTYUGetSpeed(void) {
    float speed_rate[5] = { 0.8f, 0.9f, 1.0f, 1.1f, 1.2f };

    return 1250.0f * speed_rate[enGetMode()];
}

static void enTYUSetAttackTime(struct EnLOCAL_DATA *dp) {
    short attack_time[5] = { 60, 40, 30, 20, 10 };

    enSetTimer(dp, attack_time[enGetMode()]);
}

static void enTYUSetStopTime(struct EnLOCAL_DATA *dp) {
    short stop_time[5] = { 60, 40, 30, 20, 10 };

    enSetTimer(dp, stop_time[enGetMode()]);
}

static void enTYUSetDownTime(struct EnLOCAL_DATA *dp) {
    short down_time[5] = { 600, 500, 400, 300, 240 };

    enSetTimer(dp, down_time[enGetMode()]);
}

static void enTYUSoundMove(struct EnLOCAL_DATA *dp) {
    signed char r;

    if ((dp->tyu.count -= (short)shGetDF()) <= 0) {
        dp->tyu.count = enCalcTimer(20);
        if (dp->flag & 0x1000) {
            do {
                r = ftoi(3.0f * shRandF());
            } while (r == dp->tyu.moves);
            enSoundCall(r + 0x2FA8, 0.2f, (float *)&dp->scp->pos);
            enSoundCall3D(r + 0x2FA8, 0.5f, (float *)&dp->scp->pos);
        } else {
            do {
                r = ftoi(8.0f * shRandF());
            } while (r == dp->tyu.moves);
            enSoundCall(r + 0x2FA8, 0.2f, (float *)&dp->scp->pos);
        }
        dp->tyu.moves = r;
    }
}

/** Sets up a new TY2: size, fixed HP, starting level (floor, wall, ceiling, dead) from the spawn status.
 * @param dp enemy work */
void enTY2InitData(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    enSetSize(dp, 250.0f, 150.0f, 75.0f, 100.0f);
    dp->weight = 1;
    enSetHP(dp, 1.0f, 1.0f);
    dp->flag = 0x410;
    switch (dp->scp->en_first_status) {
    case 5:
        EN_SET_LEVEL(dp, 3);
        dp->flag = 0x408;
        dp->endurance = 0.0f;
        dp->scp->battle.hp = dp->scp->battle.hp_rate = 0.0f;
        break;
    case 1:
    case 3:
        EN_SET_LEVEL(dp, 1);
        break;
    case 4:
        EN_SET_LEVEL(dp, 2);
        break;
    default:
        EN_SET_LEVEL(dp, 0);
        break;
    }
    enFlagSetRotFloor(dp);
}

/** Per-frame control of a TY2: runs the handler for its sub level (dp->slv), then moves it.
 * @param dp enemy work */
void enTY2CtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlTY2SubFunc[4])(struct EnLOCAL_DATA *) = {
        enTY2CtrlFloor, enTY2CtrlWall, enTY2CtrlCeiling, enTY2CtrlDead,
    };

    enSetBattleTarget(dp, 0);
    enCtrlTY2SubFunc[dp->slv](dp);
    enMoveExec(dp);
}

static void enTY2CtrlFloor(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        enTY2CheckCreateCharacter(dp);
        enResetDamage(dp);
    }
}

static void enTY2CtrlWall(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        if (!enTYUCheckNearWall(dp)) {
            return;
        }
        dp->sslv++;
    }
    if (enCheckDamage(dp)) {
        enTY2CheckCreateCharacter(dp);
        enResetDamage(dp);
    }
}

static void enTY2CtrlCeiling(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        dp->flag = 0x452;
        dp->scp->rot.x = dp->trx = 0.0f;
        dp->scp->rot.z = dp->trz = PI;
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    if (enCheckDamage(dp)) {
        enTY2CheckCreateCharacter(dp);
        enResetDamage(dp);
    }
}

static void enTY2CtrlDead(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        enFlagSetDead(dp);
        enFlagResetLieDown(dp);
        enFlagResetMoved(dp);
        fogEraseObj(dp - enLocalWork.Data + 10);
        dp->sslv++;
    }
}

static void enTY2CheckCreateCharacter(struct EnLOCAL_DATA *dp) {
    struct SubCharacter *scp;
    struct EnLOCAL_DATA *tp;
    int i;

    if (enLocalWork.ActiveEnemy >= 12) {
        tp = enLocalWork.Data;
        for (i = 0; i < 32; i++) {
            if (tp->kind == 3) {
                tp->scp->battle.damage = dp->scp->battle.damage;
                tp->scp->battle.id = dp->scp->battle.id;
                tp->scp->battle.shock = dp->scp->battle.shock;
                _shCopyVector(tp->scp->battle.vec, dp->scp->battle.vec);
                return;
            }
        }
        return;
    }
    scp = CharaWorkCreate(0x202, -1, (float *)&dp->scp->pos, (float *)&dp->scp->rot, dp->scp->en_first_status);
    if (scp) {
        tp = scp->enemy_p;
        tp->slv = 7;
        switch (tp->type) {
        case 1:
        case 3:
        case 4:
            tp->sslv = -1;
            break;
        }
        enDeleteCharacter(dp);
    }
}
/* Matching: fitted stand-in (not recovered code); it sets the next function's float-constant
 * order. Whether the original had code here: docs/stand-ins.md. */
static float __stripped_float_code_4(float x) { return x + 3.0f; }
/** Sets up a new TY3: size, fixed HP, starting level (floor, wall, ceiling) from the spawn status.
 * @param dp enemy work */
void enTY3InitData(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    enSetSize(dp, 250.0f, 150.0f, 75.0f, 100.0f);
    dp->weight = 1;
    enSetHP(dp, 8.0f, 8.0f);
    switch (dp->scp->en_first_status) {
    case 1:
    case 3:
        EN_SET_LEVEL(dp, 1);
        break;
    case 4:
        EN_SET_LEVEL(dp, 2);
        break;
    default:
        EN_SET_LEVEL(dp, 0);
        break;
    }
    dp->flag = 0x410;
    enFlagSetRotFloor(dp);
}

/** Per-frame control of a TY3: runs the handler for its sub level (dp->slv), then moves it.
 * @param dp enemy work */
void enTY3CtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlTY3SubFunc[3])(struct EnLOCAL_DATA *) = {
        enTY3CtrlFloor,
        enTY3CtrlWall,
        enTY3CtrlCeiling,
    };

    enSetBattleTarget(dp, 0);
    enCtrlTY3SubFunc[dp->slv](dp);
    enMoveExec(dp);
}

static void enTY3CtrlFloor(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        enTY3CheckCreateCharacter(dp);
        enResetDamage(dp);
    }
}

static void enTY3CtrlWall(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        if (!enTYUCheckNearWall(dp)) {
            return;
        }
        dp->sslv++;
    }
    if (enCheckDamage(dp)) {
        enTY3CheckCreateCharacter(dp);
        enResetDamage(dp);
    }
}

static void enTY3CtrlCeiling(struct EnLOCAL_DATA *dp) {
    if (!dp->sslv) {
        dp->flag = 0x452;
        dp->scp->rot.x = dp->trx = 0.0f;
        dp->scp->rot.z = dp->trz = PI;
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->sslv++;
    }
    if (enCheckDamage(dp)) {
        enTY3CheckCreateCharacter(dp);
        enResetDamage(dp);
    }
}

static void enTY3CheckCreateCharacter(struct EnLOCAL_DATA *dp) {
    struct SubCharacter *scp;
    struct EnLOCAL_DATA *tp;

    while (dp->endurance) {
        if (enLocalWork.ActiveEnemy >= 12) {
            return;
        }
        scp = CharaWorkCreate(0x202, -1, (float *)&dp->scp->pos, (float *)&dp->scp->rot, dp->scp->en_first_status);
        if (!scp) {
            return;
        }
        tp = scp->enemy_p;
        tp->slv = 7;
        switch (tp->type) {
        case 1:
        case 3:
        case 4:
            tp->sslv = -1;
            break;
        }
        if ((dp->endurance -= 1.0f) == 0.0f) {
            enDeleteCharacter(dp);
        }
    }
}
