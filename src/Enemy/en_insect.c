/*
 * en_insect.c: the insect swarms (INS): each insect is a small enemy drawn as a sprite
 * (en_effect.c) that approaches, attacks, chases, escapes to fixed points or stays.
 * The insects summoned by Mary's boss form (suspected; docs/characters.md).
 */
#include "enemy.h"

static void enINSCtrlApproach(struct EnLOCAL_DATA *dp);
static void enINSCtrlAttack(struct EnLOCAL_DATA *dp);
static void enINSCtrlEscape(struct EnLOCAL_DATA *dp);
static void enINSCtrlChase(struct EnLOCAL_DATA *dp);
static void enINSCtrlStay(struct EnLOCAL_DATA *dp);
static void enINSMove(struct EnLOCAL_DATA *dp, float *target, float rot_rate, float rot_delta, float speed);
static void enINSCheckWall(struct EnLOCAL_DATA *dp);
static void enINSAnimeExec(struct EnLOCAL_DATA *dp);

float escape_point[3][4] = {
    { 58600.0f, -16500.0f, 63600.0f, 0.0f },
    { 61400.0f, -16500.0f, 63600.0f, 0.0f },
    { 58600.0f, -16500.0f, 56400.0f, 0.0f },
};
float stay_pos[4];

/** Sets up an insect: follows another insect as leader if there is one (sub level 3), else
 * approaches (0); random view rotation and spin.
 * @param dp enemy work
 * Matching: `&enLocalWork.Data[0]` rather than `enLocalWork.Data` (the same code) sets
 * enSetInsect's float-constant order (enSetSize); EN_SET_LEVEL is the line table's (one line each). */
void enINSInitData(struct EnLOCAL_DATA *dp) {
    struct EnLOCAL_DATA *tp;
    int i;

    tp = &enLocalWork.Data[0];
    enSetHP(dp, 1.0f, 1.0f);
    for (i = 0; i < 32; i++, tp++) {
        if (tp == dp) {
            continue;
        }
        if (tp->kind == 15) {
            dp->ins.leader = tp;
            break;
        }
    }
    dp->mlv = 1;
    if (i == 32) {
        EN_SET_LEVEL(dp, 0);
    } else {
        EN_SET_LEVEL(dp, 3);
    }
    dp->flag = 10;
    enSetSize(dp, 50.0f, 0.0f, 0.0f, 0.0f);
    dp->anim_n = (shRandI() >> 4) % 9600;
    dp->anim_s = 0x400;
    shRandV_Scale(dp->ins.view_rot, 3.1415927f);
    shRandV_Scale(dp->ins.rot_add, 12.566371f);
    dp->scp->battle.status = 0;
    dp->ins.twin_dist = 25.0f;
}

/** Spawns a swarm of insects around pos that stay there.
 * @param pos swarm centre (also the stay position)
 * @param num number of insects
 * Matching: the level change is on one line, as in the line table; written out, not EN_SET_LEVEL (the
 * same code), which would move this function's float-constant order (enSetSize). */
void enSetInsect(float *pos, int num) {
    struct EnLOCAL_DATA *dp;
    struct SubCharacter *scp;
    float vec[4];
    int i;

    _shCopyVector(stay_pos, pos);
    for (i = 0; i < num; i++) {
        if (!(dp = enEntryEnemy(15))) {
            return;
        }
        scp = enINSGetSubCharacter(dp);
        shRandV_Scale(vec, 500.0f);
        _shAddVector((float *)&scp->pos, pos, vec);
        _shCopyVector((float *)&scp->b_pos, pos);
        dp->scp = scp;
        scp->battle.status = 0;
        dp->type = 0;
        enSetHP(dp, 1.0f, 1.0f);
        dp->mlv = 1;
        dp->slv = 4; dp->sslv = 0;
        dp->flag = 10;
        enSetSize(dp, 50.0f, 0.0f, 0.0f, 0.0f);
        dp->anim_n = (shRandI() >> 4) % 9600;
        dp->anim_s = 0x400;
        shRandV_Scale(dp->ins.view_rot, 3.1415927f);
        shRandV_Scale(dp->ins.rot_add, 12.566371f);
        dp->ins.twin_dist = 25.0f;
        dp->randseed = shRandI() ^ shRandI();
    }
}

/** Removes every insect from the enemy table. */
void enKillAllInsect(void) {
    struct EnLOCAL_DATA *dp;
    int i;

    dp = enLocalWork.Data;
    for (i = 0; i < 32; i++, dp++) {
        if (dp->kind == 15) {
            dp->kind = 0;
        }
    }
}

/** Makes every insect wait: they are reset to the stay position (the first insect's position
 * while status bit 1 is set) on their next update. */
void enWaitAllInsect(void) {
    struct EnLOCAL_DATA *dp;
    int i;

    dp = enLocalWork.Data;
    if (enLocalWork.Status & 2) {
        for (i = 0; i < 32; i++, dp++) {
            if (dp->kind == 15 && dp->type <= 2) {
                _shCopyVector(stay_pos, (float *)&dp->scp->pos);
                break;
            }
        }
    }
    for (i = 0; i < 32; i++, dp++) {
        if (dp->kind == 15) {
            dp->type = -1;
        }
    }
}

/** Returns the character of an insect: insects use a character array in shared memory
 * (MemShare_gp_data_buf + 0x140000), one per enemy slot.
 * @param dp enemy work */
struct SubCharacter *enINSGetSubCharacter(struct EnLOCAL_DATA *dp) {
    return (struct SubCharacter *)(MemShare_gp_data_buf + 0x140000) + (dp - enLocalWork.Data);
}

/** Per-frame control of an insect: resets a waiting insect, runs the handler for its sub level
 * (dp->slv), animates, moves and draws it.
 * @param dp enemy work
 * Matching: the call through `(*enCtrlINSSubFunc[dp->slv])` (the same code) sets
 * enINSCtrlApproach's float-constant order (enINSMove). */
void enINSCtrlMain(struct EnLOCAL_DATA *dp) {
    void (*enCtrlINSSubFunc[5])(struct EnLOCAL_DATA *) = {
        enINSCtrlApproach, enINSCtrlAttack, enINSCtrlEscape, enINSCtrlChase, enINSCtrlStay,
    };
    struct SubCharacter *scp;

    enSetBattleTarget(dp, 1);
    if (dp->type == -1) {
        scp = enINSGetSubCharacter(dp);
        _shCopyVector((float *)&scp->pos, stay_pos);
        _shCopyVector((float *)&scp->b_pos, stay_pos);
        dp->scp = scp;
        scp->battle.status = 0;
        dp->type = 0;
        enSetHP(dp, 1.0f, 1.0f);
        dp->sslv = 0;
    }
    (*enCtrlINSSubFunc[dp->slv])(dp);
    enINSAnimeExec(dp);
    enMoveExec(dp);
    enEfctDrawInsect(dp);
}

/* Matching: `!dp->sslv` (the same code as `== 0`) sets enINSCtrlAttack's float-constant order, and
 * def_speed's `x = x + ...` (the same code as `+=`) this function's (enINSMove). */
static void enINSCtrlApproach(struct EnLOCAL_DATA *dp) {
    float speed;
    float d;
    float tpos[4];

    if (!dp->sslv) {
        enInitPath(&dp->path, dp->scp->rot.y);
        dp->ins.def_speed = dp->ins.move_speed = 500.0f;
        enSetTimer(dp, 15);
        dp->sslv++;
        enSoundCall(0x49B2, 0.6f, (float *)&dp->scp->pos);
    }
    _shCopyVector(tpos, enGetPlayerPos(dp));
    d = enDistXZ(tpos, (float *)&dp->scp->pos);
    dp->ins.def_speed = dp->ins.def_speed + 250.0f * shGetDT();
    speed = dp->ins.def_speed * fsqrt(d / 500.0f);
    tpos[1] = -16800.0f;
    enINSMove(dp, tpos, 0.8f, PI / 180.0f * 5.0f, speed);
    if (d < 250.0f) {
        if (enCheckPlayerSprayNow()) {
            EN_SET_LEVEL(dp, 2);
        } else if (enReduceTimer(dp) <= 0) {
            EN_SET_LEVEL(dp, 1);
        }
    }
}

static void enINSCtrlAttack(struct EnLOCAL_DATA *dp) {
    float tpos[4];

    _shCopyVector(tpos, enGetPlayerPos(dp));
    tpos[1] = -16800.0f;
    enINSMove(dp, tpos, 0.9f, PI / 180.0f * 5.0f, 1500.0f);
    SeCallPosChange(0x49B2, 0.6f, (float *)&dp->scp->pos, 0);
}

/* Matching: `dp->sslv += 1` (the same code as `++`) sets enINSCtrlChase's float-constant order. */
static void enINSCtrlEscape(struct EnLOCAL_DATA *dp) {
    float *ppos;
    float vec[4];

    ppos = enGetPlayerPos(dp);
    if (dp->sslv == 0) {
        int i;
        float d;
        float dm;

        dm = 0.0f;
        for (i = 0; i < ARRAY_COUNT(escape_point); i++) {
            d = enDist(escape_point[i], (float *)&dp->scp->pos);
            if (d > dm) {
                dm = d;
                dp->type = i;
            }
        }
        dp->sslv += 1;
    }
    _shCopyVector(vec, escape_point[dp->type]);
    if (dp->sslv == 2) {
        vec[1] = -15000.0f;
    }
    enINSMove(dp, vec, 0.8f, PI / 180.0f * 5.0f, 2500.0f);
    SeCallPosChange(0x49B2, 0.6f, (float *)&dp->scp->pos, 0);
    if (dp->sslv == 1) {
        if (enDist(vec, (float *)&dp->scp->pos) < 1500.0f) {
            dp->sslv += 1;
        }
    } else {
        if (dp->scp->pos.y > -15500.0f) {
            int i;
            struct EnLOCAL_DATA *tp;

            tp = enLocalWork.Data;
            for (i = 0; i < 32; i++, tp++) {
                if (tp->kind == 15) {
                    enDeleteEnemy(tp);
                }
            }
            enSoundStop(0x49B2);
        }
    }
}

/* Matching: 10 degrees written as `PI / 180.0f * 10.0f`, like the file's `PI / 180.0f * 5.0f` (the same
 * constant as 0.17453292f), sets this function's float-constant order (enINSMove). */
static void enINSCtrlChase(struct EnLOCAL_DATA *dp) {
    float tpos[4];

    if (dp->sslv == 0) {
        enInitPath(&dp->path, dp->scp->rot.y + shSway1f(-0.52359879f, 0.1f));
        dp->ins.move_speed = 1500.0f;
    }
    if (((struct EnLOCAL_DATA *)dp->ins.leader)->slv == 1) {
        _shCopyVector(tpos, enGetPlayerPos(dp));
        tpos[1] = -16800.0f;
        enINSMove(dp, tpos, 0.8f, PI / 180.0f * 5.0f, 1500.0f);
    } else {
        enINSMove(dp, (float *)&((struct EnLOCAL_DATA *)dp->ins.leader)->scp->pos, 0.8f, PI / 180.0f * 10.0f,
                  1500.0f * (enDist((float *)&((struct EnLOCAL_DATA *)dp->ins.leader)->scp->pos,
                                    (float *)&dp->scp->pos) / 500.0f));
    }
}

/* Matching: enINSCtrlStay: the stand-in before it sets its float-constant argument order
 * (fitted, not recovered). */
static float __stripped_float_code_101(float x0) { int i0 = (int)x0 * 20; int i1 = (int)x0 * 70; x0 += 2490.0f * x0; x0 += 823.0f * x0; x0 += 3752.0f; x0 += 1820.0f; x0 += 2761.0f; i0 = i1 * 595; x0 += 283.0f; i0 = i0 * 598; i0 = i1 * 770; x0 += 3432.0f * x0; i0 = i0 * 960; x0 += 3183.0f; x0 += 3531.0f; x0 += 4390.0f * x0; i1 = i1 * 471; x0 += 2209.0f; x0 += 3164.0f; i1 = i0 * 934; i1 = i1 * 868; i0 = i1 * 760; x0 += 1760.0f; x0 += 2250.0f; x0 += 4013.0f; x0 += 540.0f; x0 += 460.0f * x0; x0 += 1315.0f * x0; x0 += 3724.0f * x0; i0 = i0 * 386; x0 += 3593.0f; x0 += 3674.0f * x0; x0 += 2420.0f * x0; x0 += 1108.0f; return x0 + (float)i0 + (float)i1; } /* fitted, not recovered: 23 constants */
/* Matching: `!dp->sslv` and `PI / 180.0f * 10.0f` (the same code as `== 0` and 0.17453292f) set
 * enINSMove's float-constant order (its last shSway1f call). */
static void enINSCtrlStay(struct EnLOCAL_DATA *dp) {
    float vec[4];

    if (!dp->sslv) {
        enInitPath(&dp->path, dp->scp->rot.y + shSway1f(-0.52359879f, 0.1f));
        dp->ins.move_speed = 500.0f;
    }
    if (enCheckPlayerLight() && enCheckHitEyes(dp, (float *)&dp->scp->pos, enGetPlayerPos(dp)) < 0.0f) {
        shSinCosV_Scale(vec, enGetPlayerAngle(dp), 600.0f);
        _shAddVector(vec, enGetPlayerPos(dp), vec);
        vec[1] = dp->scp->b_pos.y;
        enINSMove(dp, vec, 0.9f, PI / 180.0f * 10.0f, 1500.0f);
    } else {
        enINSMove(dp, (float *)&dp->scp->b_pos, 0.9f, PI / 180.0f * 5.0f, 500.0f);
    }
    dp->vec[1] *= 3.0f;
}

static void enINSMove(struct EnLOCAL_DATA *dp, float *target, float rot_rate, float rot_delta, float speed) {
    float a;
    float vec[4];

    dp->ins.y_speed = 0.95f * (dp->ins.y_speed + shSway1f(-250.0f, 0.1f) - 0.05f * (dp->scp->pos.y - target[1]));
    a = shAngleRegulate(enCalcDirection(target, (float *)&dp->scp->pos) - dp->path.angle);
    dp->ins.rot_speed = dp->ins.rot_speed * rot_rate + 0.1f * a;
    dp->path.markangle = dp->path.angle + dp->ins.rot_speed;
    enMoveAngle(&dp->path, rot_delta);
    dp->ins.speed_add = 0.98f * (dp->ins.speed_add + shSway1f(-250.0f, 0.1f) - 0.1f * (dp->ins.move_speed - speed));
    dp->ins.move_speed += dp->ins.speed_add;
    if (dp->ins.move_speed < 0.5f * speed) {
        dp->ins.move_speed = 0.5f * speed;
        dp->ins.speed_add = 0.0f;
    } else if (dp->ins.move_speed > 2.0f * speed) {
        dp->ins.move_speed = 2.0f * speed;
        dp->ins.speed_add = 0.0f;
    }
    shSinCosV_Scale(dp->vec, dp->scp->rot.y = dp->path.angle, dp->ins.move_speed);
    dp->vec[1] = dp->ins.y_speed;
    _shScaleVector(vec, dp->ins.rot_add, shGetDT());
    _shAddVector(vec, dp->ins.view_rot, vec);
    dp->ins.view_rot[0] = shAngleRegulate(vec[0]);
    dp->ins.view_rot[1] = shAngleRegulate(vec[1]);
    dp->ins.view_rot[2] = shAngleRegulate(vec[2]);
    dp->ins.dist_add = 0.95f * (dp->ins.dist_add + shSway1f(-5.0f, 0.1f) - 0.05f * (dp->ins.twin_dist - 25.0f));
    dp->ins.twin_dist += dp->ins.dist_add;
    enINSCheckWall(dp);
}

/* Matching: (stand-in: 500.0f before dp->scp->rot.y for shSinCosV_Scale) */
static float __stripped_float_code_5(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f; }
static void enINSCheckWall(struct EnLOCAL_DATA *dp) {
    struct _CL_VHIT_RESULT *res;
    float sp[4];
    float ep[4];
    float vec[4];
    float d;
    float tpos[4];

    res = &enLocalWork.HitResult;
    shSinCosV_Scale(vec, dp->scp->rot.y, 500.0f);
    _shSubVector(sp, (float *)&dp->scp->pos, vec);
    _shAddVector(ep, (float *)&dp->scp->pos, vec);
    clCheckHitEyesOnlyWall(res, sp, ep);
    if (res->kind == 1) {
        d = enDistXZ(sp, res->hobj.wall.cp) - 50.0f;
        if (d < 500.0f) {
            _shNormalize(vec, res->hobj.wall.nl);
            _shScaleVector(vec, vec, 50.0f);
            _shAddVector(tpos, res->hobj.wall.cp, vec);
            clCheckHitEyesOnlyWall(res, (float *)&dp->scp->pos, tpos);
            if (res->kind != 1) {
                vcopy3(tpos, &dp->scp->pos);
                vzero(dp->vec);
            }
        }
    }
}

static void enINSAnimeExec(struct EnLOCAL_DATA *dp) {
    int n;

    n = dp->anim_n;
    n += (dp->anim_s * ftoi(144000.0f * shGetDT())) >> 12;
    if (n >= 9600) {
        n -= 9600;
    }
    dp->anim_n = n;
    dp->anim = n / 2400;
}
