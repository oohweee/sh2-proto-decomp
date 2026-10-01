/*
 * en_arm.c: AI of the ARM enemy: sleeps, wanders, turns toward and grabs at the player, takes
 * damage and dies. Shared enemy code is in en_common.c.
 * The Mandarin (suspected; docs/characters.md).
 */
#include "enemy.h"
#include "fi_libvu0_inline.h"

static void enARMCtrlAutomatic(struct EnLOCAL_DATA *dp);
static void enARMCtrlSleep(struct EnLOCAL_DATA *dp);
static void enARMCtrlGoPlayable(struct EnLOCAL_DATA *dp);
static void enARMCtrlEvent(struct EnLOCAL_DATA *dp);
static void enARMCtrlHand(struct EnLOCAL_DATA *dp);
static void enARMCtrlWander(struct EnLOCAL_DATA *dp);
static void enARMCtrlTurn(struct EnLOCAL_DATA *dp);
static void enARMCtrlDamage(struct EnLOCAL_DATA *dp);
static void enARMCtrlDead(struct EnLOCAL_DATA *dp);
static void enARMCheckNearPlayer(struct EnLOCAL_DATA *dp);
static int enARMGetDamageMotion(struct EnLOCAL_DATA *dp);
static int enARMCanSeePlayer(struct EnLOCAL_DATA *dp);
static int enARMCheckNearOther(struct EnLOCAL_DATA *dp);

static void enARMAnimeSet(struct EnLOCAL_DATA *dp, int anim);

static void enARMAnimeReset(struct EnLOCAL_DATA *dp, int anim);
static void enARMAnimeExec(struct EnLOCAL_DATA *dp);
static void enARMSetHandPos(struct EnLOCAL_DATA *dp, int d);
static int enARMCheckFloor(struct EnLOCAL_DATA *dp, int d);
static float enARMGetMoveSpeed(struct EnLOCAL_DATA *dp);
static void enARMSoundSigns(struct EnLOCAL_DATA *dp);

struct EnANIME_DATA EnARMAnime[14] = {
    { 0x1483, 0 }, { 0x1483, 1 }, { 0x1484, 0 }, { 0x1485, 0 }, { 0x1486, 0 },
    { 0x1487, 0 }, { 0x1488, 0 }, { 0x1489, 0 }, { 0x148A, 0 }, { 0x148B, 0 },
    { 0x148C, 0 }, { 0x148D, 0 }, { 0x148E, 0 }, { 0x148F, 0 },
};

/** Sets up a new ARM: HP by difficulty mode, size, first animation; asleep if out of play.
 * @param dp enemy work */
void enARMInitData(struct EnLOCAL_DATA *dp) {
    float vitarity[5] = { 500.0f, 800.0f, 1200.0f, 2500.0f, 4000.0f };
    int mode;

    mode = enGetMode();
    dp->mlv = 1;
    dp->slv = 0;
    dp->sslv = 0;
    enSetBattleTarget(dp, 0);
    enSetSize(dp, 300.0f, 500.0f, 600.0f, 600.0f);
    dp->weight = 3;
    enSetHP(dp, vitarity[mode], vitarity[mode]);
    enFlagSetMoved(dp);
    dp->flag |= 0x8442;
    enARMAnimeSet(dp, 1);
    dp->arm.arm = -1;
    if (enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

/** Per-frame control of an ARM: runs the handler for its main level (dp->mlv).
 * @param dp enemy work */
void enARMCtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlARMFunc[6])(struct EnLOCAL_DATA *) = {
        enDummyCtrl, enARMCtrlAutomatic, enARMCtrlSleep, enARMCtrlGoPlayable, enARMCtrlEvent, enARMCtrlHand,
    };

    enCtrlARMFunc[dp->mlv](dp);
}

static void enARMCtrlAutomatic(struct EnLOCAL_DATA *dp) {
    void (*enCtrlARMSubFunc[4])(struct EnLOCAL_DATA *) = {
        enARMCtrlWander, enARMCtrlTurn, enARMCtrlDamage, enARMCtrlDead,
    };

    enSetBattleTarget(dp, 0);
    switch (dp->anim) {
    case 2:
    case 4:
        enARMSetHandPos(dp, 0);
        dp->arm.arm = 0;
        break;
    case 3:
    case 5:
        enARMSetHandPos(dp, 1);
        dp->arm.arm = 1;
        break;
    default:
        dp->arm.arm = -1;
        break;
    }
    enCtrlARMSubFunc[dp->slv](dp);
    enARMAnimeExec(dp);
    enMoveExec(dp);
    _shCopyVector(dp->arm.old_pos, (float *)&dp->scp->pos);
    if (dp->slv <= 1) {
        enARMSoundSigns(dp);
    }
    if (enCheckSleepIn(dp)) {
        enSleepIn(dp);
    }
}

static void enARMCtrlSleep(struct EnLOCAL_DATA *dp) {
    if (enCheckSleepOut(dp)) {
        enSleepOut(dp);
        dp->mlv = 3;
        EN_SET_LEVEL(dp, 0);

    }
    dp->arm.arm = -1;
}

static void enARMCtrlGoPlayable(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    EN_SET_LEVEL(dp, 0);

    dp->arm.arm = -1;
}

static void enARMCtrlEvent(struct EnLOCAL_DATA *dp) {
    void *tmp;

    tmp = dp; /* Matching: reconstructed dead store: the DWARF lists dp and tmp (both referenced), the code is empty. */
}

static void enARMCtrlHand(struct EnLOCAL_DATA *dp) {
}

/* if/else (not ?:) so s is set with movz straight into s0. */
static void enARMCtrlWander(struct EnLOCAL_DATA *dp) {
    float vec[4];
    signed char s;

    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 2);

        return;
    }
    if (dp->sslv == 0) {
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->anim_n = -1;
    }
    enReduceTimer(dp);
    if (dp->anim_n == -1) {
        if (enARMCheckNearOther(dp)) {
            EN_SET_LEVEL(dp, 1);

            enSetTimer(dp, 0);
            dp->arm.attack = 0;
            return;
        }
        switch (enARMCanSeePlayer(dp)) {
        case 0:
            s = 1;
            dp->arm.attack = 0;
            break;
        case 1:
            if (dp->timer == 0) {
                s = 2;
            } else {
                s = 1;
            }
            dp->arm.attack = 0;
            break;
        case 2:
            s = 3;
            dp->arm.attack++;
            break;
        }
        if ((dp->sslv >= 2 && ((dp->sslv != s && s < 3) || !(dp->flag & 4))) || dp->arm.attack > 4) {
            enSetTimer(dp, 180);
            dp->arm.attack = 0;
            s = 1;
        }
        dp->sslv = s;
        if (dp->arm.arm == -1) {
            if (dp->anim == 7 || dp->anim == 10 || dp->anim == 11) {
                dp->arm.arm = 0;
            } else {
                dp->arm.arm = 1;
            }
        }
        if (s <= 2) {
            enARMAnimeSet(dp, 3 - dp->arm.arm);
        } else {
            enARMAnimeSet(dp, 5 - dp->arm.arm);
            enAttackStart(dp);
        }
    }
    enARMGetMoveSpeed(dp);
    if (dp->sslv == 1) {
        shSinCosV_Scale(vec, dp->scp->rot.y, 1000.0f);
        _shAddVector(vec, (float *)&dp->scp->pos, vec);
        enSetPath(dp, vec, (float *)&dp->scp->pos);
    } else {
        if (dp->sslv == 3) {
            enAttackCheck(dp, dp->anim == 4 ? (unsigned char)0x40 : (unsigned char)0x41);
        }
        enSetPath(dp, enGetPlayerPos(dp), (float *)&dp->scp->pos);
    }
    enMoveAngle(&dp->path, 0.05235988f);
    dp->scp->rot.y = dp->path.angle;
    enARMCheckNearPlayer(dp);
}

/* Matching: EN_SET_LEVEL's do { } while (0) gives the extra nop before the epilogue. */
static void enARMCtrlTurn(struct EnLOCAL_DATA *dp) {
    if (enCheckDamage(dp)) {
        EN_SET_LEVEL(dp, 2);
        return;
    }
    if (dp->sslv == 0) {
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->anim_n = -1;
        if (dp->arm.arm == -1) {
            if (dp->anim == 7 || dp->anim == 10 || dp->anim == 11) {
                dp->arm.arm = 0;
            } else {
                dp->arm.arm = 1;
            }
        }
        if (enARMCanSeePlayer(dp) == 2) {
            enARMAnimeSet(dp, 5 - dp->arm.arm);
            enAttackStart(dp);
        } else {
            enARMAnimeSet(dp, 3 - dp->arm.arm);
        }
        dp->sslv++;
    }
    if (dp->anim == 4) {
        enAttackCheck(dp, 0x40);
    } else if (dp->anim == 5) {
        enAttackCheck(dp, 0x41);
    }
    enARMGetMoveSpeed(dp);
    if (dp->p_dist > 5000.0f) {
        dp->anim_s /= 4;
        dp->arm.scount = 4;
    }
    dp->path.markangle = dp->path.angle + 0.05235988f * dp->arm.dir;
    enMoveAngle(&dp->path, 0.05235988f);
    dp->scp->rot.y = dp->path.angle;
    enARMCheckNearPlayer(dp);
    if (dp->anim_n == -1) {
        if (++dp->sslv <= 3) {
            if (enARMCanSeePlayer(dp) == 2) {
                enARMAnimeSet(dp, 5 - dp->arm.arm);
                enAttackStart(dp);
            } else {
                enARMAnimeSet(dp, 3 - dp->arm.arm);
            }
        } else {
            EN_SET_LEVEL(dp, 0);
        }
    }
}

/* Matching: EN_SET_LEVEL's do { } while (0) gives the extra nop at the end of case 2. */
static void enARMCtrlDamage(struct EnLOCAL_DATA *dp) {
    switch (dp->sslv) {
    case 0:
        enReduceHP(dp);
        dp->flag &= ~0x8000;
        enARMAnimeReset(dp, enARMGetDamageMotion(dp));
        dp->sslv++;
        break;
    case 1:
        if (enCheckDamage(dp)) {
            enReduceHP(dp);
        }
        if (dp->anim_n == -1) {
            if (enCheckDeath(dp)) {
                if (dp->anim == 6 || dp->anim == 8 || dp->anim == 9) {
                    enARMAnimeSet(dp, 12);
                } else {
                    enARMAnimeSet(dp, 13);
                }
                fogEraseObj(dp - enLocalWork.Data + 10);
                enFlagSetNoDamage(dp);
                enFlagResetMoved(dp);
                enFlagSetDead(dp);
                enKillCountUp(dp);
                dp->sslv++;
            } else {
                EN_SET_LEVEL(dp, 0);
                enSetTimer(dp, 0);
                dp->arm.attack = 0;
                dp->flag |= 0x8000;
            }
        }
        break;
    case 2:
        if (dp->anim_n == -1) {
            EN_SET_LEVEL(dp, 3);
        }
        break;
    }
    enARMCheckNearPlayer(dp);
}

static void enARMCtrlDead(struct EnLOCAL_DATA *dp) {
    if (dp->sslv == 0) {
        enSetTimer(dp, 60);
        dp->sslv++;
    }
    if (enReduceTimer(dp) <= 0) {
        enDeleteCharacter(dp);
    }
}

static void enARMCheckNearPlayer(struct EnLOCAL_DATA *dp) {
    enCheckNearPlayer(dp, &dp->arm.near_count, &dp->arm.dist, 200.0f);
}

static int enARMGetDamageMotion(struct EnLOCAL_DATA *dp) {
    int m;
    int id;
    int dd;
    int arm;
    float a;

    a = shAngleRegulate(shAtanV(dp->scp->battle.vec) - dp->scp->rot.y);
    if (fabsf(a) > 1.5707964f) {
        dd = 0;
    } else {
        dd = 1;
    }
    switch (dp->anim) {
    case 2:
    case 3:
    case 4:
    case 5:
        arm = (dp->arm.arm ? 1 : 0) ^ (dp->anim_n > 19200);
        break;
    case 1:
    case 6:
    case 8:
    case 9:
        arm = 0;
        break;
    default:
        arm = 1;
        break;
    }
    id = dp->last_atk;
    switch (id) {
    case 2:
    case 1:
        m = arm + 6;
        break;
    case 4:
    case 6:
        m = dd + (arm * 2 + 6);
        break;
    default:
        m = 0;
        printf("Illegal damage type!(%d)\n", id);
        break;
    }
    return m;
}

/* Matching: fitted stand-in for float code (docs/stand-ins.md); it makes enARMCanSeePlayer materialize
 * 1000.0f before loading dp->scp->rot.y for shSinCosV_Scale, as the original does. */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f; }
static int enARMCanSeePlayer(struct EnLOCAL_DATA *dp) {
    float a;
    float vec[4];

    a = enCalcAngleDifference(enGetPlayerDirection(dp), dp->scp->rot.y);
    if (enLocalWork.Status & 1) {
        return 0;
    }
    if ((dp->p_dist > 2500.0f || a > 2.0943952f) && dp->anim <= 5) {
        return 0;
    }
    if (dp->p_dist < 1000.0f) {
        if (enARMCheckFloor(dp, 0) != 1 && enARMCheckFloor(dp, 1) != 1) {
            shSinCosV_Scale(vec, dp->scp->rot.y, 1000.0f);
            _shAddVector(vec, (float *)&dp->scp->pos, vec);
            if (enCheckFloor(vec) != 1) {
                return 2;
            }
        }
    }
    return 1;
}

static int enARMCheckNearOther(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;
    int i;
    float *tpos;
    float d;

    tp = enLocalWork.Data;
    for (i = 0; i < 32; i++, tp++) {
        if (tp != dp && tp->kind == 10) {
            tpos = (float *)&tp->scp->pos;
            if (enDist((float *)&dp->scp->pos, tpos) < 1000.0f) {
                d = shAngleRegulate(dp->scp->rot.y - enCalcDirection(tpos, (float *)&dp->scp->pos));
                if (fabsf(d) < 1.5707964f) {
                    if (d >= 0.0f) {
                        dp->arm.dir = 1;
                    } else {
                        dp->arm.dir = -1;
                    }
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void enARMAnimeSet(struct EnLOCAL_DATA *dp, int anim) {
    if (anim == dp->anim) {
        enAnimeRestart(dp);
    } else {
        /* Matching: the assert bakes its original line number into the object. */
#line 579
        fjAssert(anim >= 0 && anim < sizeof(EnARMAnime) / sizeof(EnANIME_DATA));
        enAnimeSet(dp, anim, EnARMAnime[anim].Anime);
        dp->arm.arm = -1;
    }
}

static void enARMAnimeReset(struct EnLOCAL_DATA *dp, int anim) {
    /* Matching: the assert bakes its original line number into the object. */
#line 588
    fjAssert(anim >= 0 && anim < sizeof(EnARMAnime) / sizeof(EnANIME_DATA));
    enAnimeSet(dp, anim, EnARMAnime[anim].Anime);
    dp->arm.arm = -1;
}

static void enARMAnimeExec(struct EnLOCAL_DATA *dp) {
    int of;
    float pos[4];

    /* Matching: dead store as in the other AnimeExec functions; the DWARF has of, the line table a
     * statement without code here. */
    of = dp->anim_n;
    enAnimeExec(dp, EnARMAnime, 0x1483);
    if (dp->anim >= 2 && dp->anim <= 5 && dp->anim_step == 0 && dp->anim_n == -1) {
        dp->anim_step = 1;
        if (dp->p_dist < 5000.0f || ++dp->arm.scount >= 4) {
            dp->arm.scount = 0;
            enARMGetHandPos(pos, dp, dp->arm.arm ^ 1);
            _shAddVector(pos, (float *)&dp->scp->pos, pos);
            pos[1] = dp->scp->pos.y;
            switch (enCheckFloor(pos)) {
            case 2:
                enSoundCall(0x2FDA, 0.8f, pos);
                break;
            default:
                enSoundCall(0x2FDB, 0.8f, pos);
                break;
            }
        }
    }
    if (dp->anim_n == -1 || (dp->flag & 1)) {
        return;
    }
}

static void enARMSetHandPos(struct EnLOCAL_DATA *dp, int d) {
    float vec[4];

    enARMGetHandPos(vec, dp, d);
    _shAddVector(dp->arm.hand_pos, (float *)&dp->scp->pos, vec);
}

/** Gets the position of hand d, 550 along its skeleton node (11 + d), rotated by the ARM's heading.
 * @param vec result
 * @param dp enemy work
 * @param d hand index */
void enARMGetHandPos(float *vec, struct EnLOCAL_DATA *dp, int d) {
    float mat[4][4];

    enGetSkeletonMatrix(mat, dp, d + 11);
    _shUnitVector(vec);
    vec[0] = 550.0f;
    _sceVu0ApplyMatrix(vec, mat, vec);
    _sceVu0UnitMatrix(mat);
    shRotMatrixY(mat, mat, dp->scp->rot.y);
    _sceVu0ApplyMatrix(vec, mat, vec);
}

static int enARMCheckFloor(struct EnLOCAL_DATA *dp, int d) {
    float pos[4];

    enARMGetHandPos(pos, dp, d);
    return enCheckFloor(pos);
}

static float enARMGetMoveSpeed(struct EnLOCAL_DATA *dp) {
    float speed_rate[5] = { 0.7f, 0.8f, 1.0f, 1.2f, 1.5f };
    float r;

    r = speed_rate[enGetMode()];
    r *= 0.6f + dp->scp->battle.hp_rate / 250.0f;
    if (dp->p_dist > 5000.0f) {
        r *= 4.0f;
    }
    dp->anim_s = ftoi(4096.0f * r);
    return r;
}

/* Matching: fitted stand-in for float code (docs/stand-ins.md); it makes enARMSoundSigns materialize
 * 1.0f before signs + 0x2FE2 for enSoundCall, as the original does. */
static float __stripped_float_code_2(float x) { return x + 3.0f + 5.0f + 7.0f; }
static void enARMSoundSigns(struct EnLOCAL_DATA *dp) {
    int signs;

    if ((dp->arm.count -= shGetDF()) <= 0) {
        dp->arm.count = enCalcTimer(60);
        do {
            signs = (shRandI() >> 20) & 3;
        } while (signs == dp->arm.signs);
        dp->arm.signs = signs;
        enSoundCall(signs + 0x2FE2, 1.0f, (float *)&dp->scp->pos);
    }
}
