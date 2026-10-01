/*
 * cl_main.c: character/BG collision, battle hit queue and eye/weapon ray tests.
 */

#include "sh2.h"

#include "asm_libm.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "libc/string.h"

static void clCollectCharaALL(void);
static void clAddCollectVector(float *v0, float *v1);
static void clCheckBg2Chara(int no);
static void clCheckHitWallCollision(struct _CL_HITPOLY_COLUMN *col, int *whnum, struct _CL_HITPOLY_PLANE *pl, int *ptr);
static void clCheckHitDynamicWallCollision(struct _CL_HITPOLY_COLUMN *col, int *whnum);
static int clMakeWallHitCollectVector(struct SubCharacter *sc, float *wcv, float mang, int *flg, int num);
/* Matching: called without a prototype in the original (arguments passed unconverted). */
static void clAddWallCollectVector();
static void clCheckColumn2WallHit(struct _CL_HITRESULT *cres, struct _CL_HITPOLY_PLANE *pl, struct _CL_HITPOLY_COLUMN *col);
static void clCheckColumn2ColumnHit(struct _CL_HITPOLY_COLUMN *col, int *whnum, struct _CL_HITPOLY_COLUMN *cl, int *ptr);
static void clCollectCharaHeightNormal(struct SubCharacter *sc);
static void clModifiedBattleData(void);
static void clSetOneBattleResult(struct _CL_BATTLE_QUE *que, struct _CL_VHIT_RESULT *vres, float *vec);
static void clSetThrustBattleResult(struct _CL_BATTLE_QUE *que, float *vec);
static void clCheckHitSwordWeapon(struct _CL_VHIT_RESULT *res, unsigned int id, float *svs, float *sve, float *evs, float *eve);
static void clCheckHitGunWeapon(struct _CL_VHIT_RESULT *res, unsigned int id, float *st, float *ed);
static void clCheckHitSwordVector(struct _CL_VHIT_RESULT *res, unsigned int id, float *sp, float *ep);
static void clCheckHitSwordVectorWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, struct _CL_HITPOLY_PLANE *pl, int *ptr);
static void clCheckHitNoThruVectorWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, struct _CL_HITPOLY_PLANE *pl, int *ptr);
static void clCheckHitSwordVectorDynamicWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min);
static void clCheckHitSwordVectorDynamicWallNoThru(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min);
static void clCheckHitSwordVectorDynamicFloor(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min);
static void clCheckHitSwordVectorDynamicFloorNoThru(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min);
static void clCheckHitSwordWeaponThrust(unsigned int id, float *svs, float *sve, float *evs, float *eve);
static int clCheckHitThrustSwordVector(unsigned int id, float *sp, float *ep);
static void clCheckHitGunWeaponThrust(unsigned int id, float *st, float *ed);
static void clCheckHitThrustGunVector(unsigned int id, float *sp, float *ep);
static void clCheckHitThrustGunVectorCharacter(float *sp, float *ep, float min, unsigned int id);
static struct _CL_SELECT_MAP *clGetHitSectListVECHITOutDoor(float *st, float *ed);
/* Matching: no prototype; called with arguments it ignores. */
static struct _CL_SELECT_MAP *clGetHitSectListVECHITInDoor();
static int Line2PlaneBoundaryCheckXZ(float (*l0)[4], float (*l1)[4], float (*p0)[4], float (*p1)[4], float (*p2)[4], float (*p3)[4]);
static struct _CL_SELECT_MAP *clGetHitSectListMOVEOutDoor(float *bpos);
static struct _CL_SELECT_MAP *clGetHitSectListMOVEInDoor(); /* Matching: likewise. */
static void clCheckHitEyeVector(struct _CL_VHIT_RESULT *res, unsigned int id, float *sp, float *ep);
static void clCheckHitEyeVectorNoThru(struct _CL_VHIT_RESULT *res, unsigned int id, float *sp, float *ep);
static void clCheckHitEyeVectorAllNoThru(struct _CL_VHIT_RESULT *res, unsigned int id, float *sp, float *ep);
static void clCheckHitEyeVectorWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, struct _CL_HITPOLY_PLANE *pl, int *ptr);
static void clCheckHitEyeVectorBGColumn(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, struct _CL_HITPOLY_COLUMN *cl, int *ptr);
static void clCheckHitEyeVectorDynamicWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min);
static void clCheckHitEyeVectorDynamicFloor(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min);
static void clCheckHitEyeVectorCharacter(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, unsigned int id);

/*
 * Rounds v1 to 1/16 steps (ftoi4 then itof4) into v0. An inline VU0 helper in the
 * original; name not recovered.
 */
static inline void _clRoundVector4(float *v0, float *v1) {
    __asm__ __volatile__("
    lqc2        vf4, 0x0(%1)
    vftoi4.xyzw vf5, vf4
    vitof4.xyzw vf4, vf5
    sqc2        vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}

/* 3-component inner product on the FPU. Matching: local: loads p1 first, in native asm (unlike
 * _shInnerProduct). */
inline float vinner(float *p0, float *p1) {
    float ret;

    asm {
        lwc1 ret, 0(p1)
        lwc1 $f8, 0(p0)
        lwc1 $f9, 4(p1)
        lwc1 $f10, 4(p0)
        mula.s ret, $f8
        lwc1 ret, 8(p1)
        lwc1 $f8, 8(p0)
        madda.s $f9, $f10
        madd.s ret, ret, $f8
    }
    return ret;
}

/*
 * *d = squared distance between v0 and v1 (hand-written VU0 helper; the result is
 * also left in $f12). Name not recovered.
 */
static inline void clSquareDistance(float *v0, float *v1, float *d) {
    __asm__ __volatile__("
    lqc2     vf1, 0x0(%0)
    lqc2     vf2, 0x0(%1)
    vsub.xyz vf3, vf1, vf2
    vmul.xyz vf3, vf3, vf3
    vaddz.x  vf3, vf3, vf3z
    vaddy.x  vf3, vf3, vf3y
    qmfc2.ni t0, vf3
    mtc1     t0, $f12
    sw       t0, 0x0(%2)
    " : : "r"(v0), "r"(v1), "r"(d));
}


unsigned char clPermColExpFlg[210] = {
    1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
    1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0,
    1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0,
    1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1,
    1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
};

int clCollisionEnable;
int clCharaListAct;
int clCharaListUse[2];
struct _CL_CHARA_LIST clCharaList[2][32];
struct _CL_WALLHITDAT clWallHitData[32];
struct _CL_SELECT_MAP clSelectMap[128];
int clDynamicWallListAct;
struct _CL_DYNAMICWALL_LIST clDynamicWallList[2];
int clDynamicFloorListAct;
struct _CL_DYNAMICFLOOR_LIST clDynamicFloorList[2];
int clVHitListUse;
struct _CL_VHIT_RESULT clVHitResult[64];
int clUseBattleQue;
struct _CL_BATTLE_QUE clBattleQue[64];
int clUseBattleResult;
struct _CL_BATTLE_RESULT clBattleResult[65];

/** Resets all collision lists and the battle results, and enables collision. */
void clAllInitCollisionData(void) {
    clCharaListAct = 0;
    clDynamicWallListAct = 0;
    clDynamicFloorListAct = 0;
    clCharaListUse[0] = 0;
    clCharaListUse[1] = 0;
    clDynamicWallList[0].use = 0;
    clDynamicWallList[1].use = 0;
    clDynamicFloorList[0].use = 0;
    clDynamicFloorList[1].use = 0;
    clBattleResult[64].atr = 0;
    clUseBattleResult = 0;
    clCollisionEnable = 1;
}

/** Starts a frame: flips the double-buffered character/dynamic wall/floor lists and clears the new
 * ones and the battle queue. */
void clFrameInitCollisionData(void) {
    clCharaListAct = clCharaListAct ? 0 : 1;
    clCharaListUse[clCharaListAct] = 0;
    clUseBattleQue = 0;
    clDynamicWallListAct = clDynamicWallListAct ? 0 : 1;
    clDynamicWallList[clDynamicWallListAct].use = 0;
    clDynamicFloorListAct = clDynamicFloorListAct ? 0 : 1;
    clDynamicFloorList[clDynamicFloorListAct].use = 0;
}

/** Resolves character collisions and applies the push-out to each listed character's position, matrix and columns. */
void clCollectCharaPosition(void) {
    int i;
    float dif[4];

    clCollectCharaALL();
    for (i = 0; i < clCharaListUse[clCharaListAct]; i++) {
        clCharaList[clCharaListAct][i].col.p[0][3] = 1.0f;
        _shSubVector(dif, clCharaList[clCharaListAct][i].col.p[0], clCharaList[clCharaListAct][i].opos);
        clCharaList[clCharaListAct][i].sc->pos.x += dif[0];
        clCharaList[clCharaListAct][i].sc->pos.z += dif[2];
        clCharaList[clCharaListAct][i].sc->mat.d[3][0] += dif[0];
        clCharaList[clCharaListAct][i].sc->mat.d[3][2] += dif[2];
        _shAddVector(clCharaList[clCharaListAct][i].wcol.p[0], clCharaList[clCharaListAct][i].wcol.p[0], dif);
        clCharaList[clCharaListAct][i].wcol.p[0][3] = 1.0f;
        clCharaList[clCharaListAct][i].heightfunc((float *)clCharaList[clCharaListAct][i].sc);
        dif[0] = clCharaList[clCharaListAct][i].sc->b_pos.y - clCharaList[clCharaListAct][i].sc->pos.y;
        if (fabsf(dif[0]) > 600.0f / shGetFPS() && !(clCharaList[clCharaListAct][i].sc->status & 0x10000)) {
            dif[0] /= 2.0f;
            clCharaList[clCharaListAct][i].sc->pos.y += dif[0];
            clCharaList[clCharaListAct][i].sc->mat.d[3][1] += dif[0];
            clCharaList[clCharaListAct][i].wcol.p[0][1] += dif[0];
        }
    }
}

#define CL_NEW clCharaList[clCharaListAct][clCharaListUse[clCharaListAct]]

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 637
/**
 * Adds a character to this frame's collision list.
 * @param col column for movement collision. @param wcol column for battle hits. @param sc the character.
 * @param func height/ground function, or NULL for the default.
 */
void clSetCharaHitColumn(struct _CL_HITPOLY_COLUMN *col, struct _CL_HITPOLY_COLUMN *wcol, struct SubCharacter *sc, void (*func)()) {
    float dif[4];

    assert(clCharaListUse[clCharaListAct] < 32);
    if (col->p[1][3] == 0.0f) {
        CL_NEW.movflg = 0;
    } else {
        CL_NEW.movflg = 1;
    }
    if (wcol->p[1][3] == 0.0f) {
        CL_NEW.batflg = 0;
    } else {
        CL_NEW.batflg = 1;
    }
    memcpy(&CL_NEW.col, col, sizeof(struct _CL_HITPOLY_COLUMN));
    memcpy(&CL_NEW.wcol, wcol, sizeof(struct _CL_HITPOLY_COLUMN));
    _shSubVector(dif, (float *)&sc->pos, (float *)&sc->b_pos);
    _shSubVector(CL_NEW.pos, col->p[0], dif);
    _shSubVector(CL_NEW.col.p[0], col->p[0], dif);
    vcopy(col->p[0], CL_NEW.opos);
    CL_NEW.sc = sc;
    _shSubVector(CL_NEW.mvec, (float *)&sc->pos, (float *)&sc->b_pos);
    if (CL_NEW.mvec[0] == 0.0f && CL_NEW.mvec[2] == 0.0f) {
        CL_NEW.mang = 3.4028235e38f;
    } else {
        CL_NEW.mang = shAtan2(CL_NEW.mvec[0], CL_NEW.mvec[2]);
    }
    *(u_long128 *)CL_NEW.ccvec = 0;
    if (func == NULL) {
        CL_NEW.heightfunc = (void (*)(float *))clCollectCharaHeightNormal;
    } else {
        CL_NEW.heightfunc = (void (*)(float *))func;
    }
    *(u_long128 *)CL_NEW.wallcv = 0;
    CL_NEW.wflg = 0;
    clCharaListUse[clCharaListAct]++;
}
/** Adds a moving wall polygon to this frame's dynamic wall list. */
void clAddDynamicWall(struct _CL_HITPOLY_PLANE *pl) {
    clDynamicWallList[clDynamicWallListAct].dw[clDynamicWallList[clDynamicWallListAct].use] = pl;
    clDynamicWallList[clDynamicWallListAct].use++;
}
/** Adds a moving floor polygon to this frame's dynamic floor list. */
void clAddDynamicFloor(struct _CL_HITPOLY_PLANE *pl) {
    clDynamicFloorList[clDynamicFloorListAct].dw[clDynamicFloorList[clDynamicFloorListAct].use] = pl;
    clDynamicFloorList[clDynamicFloorListAct].use++;
}

#define CL(n) clCharaList[clCharaListAct][n]

static void clCollectCharaALL(void) {
    int i;
    int j;
    int hit;
    int limit;
    struct _CL_HITPOLY_COLUMN *col0;
    struct _CL_HITPOLY_COLUMN *col1;
    struct _CL_HITRESULT cres;
    float dist;

    for (limit = 0; limit < 5; limit++) {
        for (i = 0; i < clCharaListUse[clCharaListAct]; i++) {
            col0 = &CL(i).col;
            vcopy(col0->p[0], CL(i).pos);
            if (CL(i).ccvec[0] == 0.0f) {
                col0->p[0][0] += CL(i).mvec[0];
            } else if (CL(i).ccvec[0] * CL(i).mvec[0] < 0.0f) {
                col0->p[0][0] += CL(i).ccvec[0];
            } else if (fabsf(CL(i).ccvec[0]) > fabsf(CL(i).mvec[0])) {
                col0->p[0][0] += CL(i).ccvec[0];
            } else {
                col0->p[0][0] += CL(i).mvec[0];
            }
            if (CL(i).ccvec[2] == 0.0f) {
                col0->p[0][2] += CL(i).mvec[2];
            } else if (CL(i).ccvec[2] * CL(i).mvec[2] < 0.0f) {
                col0->p[0][2] += CL(i).ccvec[2];
            } else if (fabsf(CL(i).ccvec[2]) > fabsf(CL(i).mvec[2])) {
                col0->p[0][2] += CL(i).ccvec[2];
            } else {
                col0->p[0][2] += CL(i).mvec[2];
            }
            _shSubVector(CL(i).mvec, col0->p[0], CL(i).pos);
            dist = col0->p[1][3] / lengthXZ(CL(i).mvec);
            if (dist < 1.0f) {
                _shScaleVector(CL(i).mvec, CL(i).mvec, dist);
            }
            if (CL(i).mvec[0] == 0.0f && CL(i).mvec[2] == 0.0f) {
                CL(i).mang = 3.4028235e38f;
            } else {
                CL(i).mang = shAtan2(CL(i).mvec[0], CL(i).mvec[2]);
            }
            if (CL(i).movflg) {
                clCheckBg2Chara(i);
                if (CL(i).wflg) {
                    _shAddVector(col0->p[0], col0->p[0], CL(i).wallcv);
                }
            }
            *(u_long128 *)CL(i).ccvec = 0;
            for (j = 0; j < clCharaListUse[clCharaListAct]; j++) {
                if (i != j && CL(i).movflg && CL(j).movflg) {
                    col1 = &CL(j).col;
                    if (clCheckSubColumnToColumn(&cres, col1->p, col0->p)) {
                        cres.cv[1] = 0.0f;
                        _shAddVector(col0->p[0], col0->p[0], cres.cv);
                        {
                        float vec1[4];
                        int wt;
                        float div[5] = {1.0f, 0.98f, 0.94f, 0.9f, 0.86f};

                        wt = col0->weight - col1->weight;
                        if (wt < 0) {
                            wt = 0;
                        } else {
                            wt++;
                        }
                        _shScaleVector(vec1, cres.cv, 1.0f - div[wt]);
                        vec1[0] *= -1.0f;
                        vec1[2] *= -1.0f;
                        dist = col0->p[1][3] / lengthXZ(vec1);
                        if (dist < 1.0f) {
                            _shScaleVector(vec1, vec1, dist);
                        }
                        clAddCollectVector(CL(j).ccvec, vec1);
                        }
                        hit++;
                    }
                }
            }
            *(u_long128 *)CL(i).mvec = 0;
            if (CL(i).movflg) {
                clCheckBg2Chara(i);
                if (CL(i).wflg) {
                    _shAddVector(col0->p[0], col0->p[0], CL(i).wallcv);
                }
            }
        }
        if (!hit) {
            break;
        }
        hit = 0;
    }
}

#define CL_CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

static void clAddCollectVector(float *v0, float *v1) {
    float tv[4];

    _shAddVector(tv, v0, v1);
    if (v0[0] > v1[0]) {
        tv[0] = CL_CLAMP(tv[0], v1[0], v0[0]);
    } else {
        tv[0] = CL_CLAMP(tv[0], v0[0], v1[0]);
    }
    if (v0[2] > v1[2]) {
        tv[2] = CL_CLAMP(tv[2], v1[2], v0[2]);
    } else {
        tv[2] = CL_CLAMP(tv[2], v0[2], v1[2]);
    }
    vcopy(tv, v0);
}

#define CLD(base) ((struct _CL_CLDHEADER *)(base))

static void clCheckBg2Chara(int no) {
    int whnum;
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wl;
    struct _CL_HITPOLY_PLANE *sw;
    struct _CL_HITPOLY_COLUMN *cl;
    int *ptr;
    float tmp[4];
    float mang;

    whnum = 0;
    CL(no).wflg = 0;
    *(u_long128 *)CL(no).wallcv = 0;
    clCheckHitDynamicWallCollision(&CL(no).col, &whnum);
    smap = clGetHitSectListMOVE(CL(no).col.p[0]);
    smapsv = smap;
    if (smap->base) {
        while (smap->base) {
            wl = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->wldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b1ofs[smap->sect]);
            clCheckHitWallCollision(&CL(no).col, &whnum, wl, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            sw = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->swdofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b3ofs[smap->sect]);
            clCheckHitWallCollision(&CL(no).col, &whnum, sw, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            cl = (struct _CL_HITPOLY_COLUMN *)(smap->base + CLD(smap->base)->cldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->clofs[smap->sect]);
            clCheckColumn2ColumnHit(&CL(no).col, &whnum, cl, ptr);
            smap++;
        }
    }
    _shSubVector(tmp, CL(no).col.p[0], CL(no).opos);
    _shAddVector(tmp, (float *)&CL(no).sc->pos, tmp);
    _shSubVector(tmp, tmp, (float *)&CL(no).sc->b_pos);
    if (tmp[0] == 0.0f && tmp[2] == 0.0f) {
        mang = 3.4028235e38f;
    } else {
        mang = shAtan2(tmp[0], tmp[2]);
    }
    if (!clMakeWallHitCollectVector(CL(no).sc, CL(no).wallcv, mang, &CL(no).wflg, whnum)) {
        _shAddVector(CL(no).col.p[0], CL(no).col.p[0], CL(no).wallcv);
        _shSubVector(CL(no).wallcv, (float *)&CL(no).sc->b_pos, CL(no).col.p[0]);
    }
    _clRoundVector4(CL(no).wallcv, CL(no).wallcv);
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1188
static void clCheckHitWallCollision(struct _CL_HITPOLY_COLUMN *col, int *whnum, struct _CL_HITPOLY_PLANE *pl, int *ptr) {
    struct _CL_HITRESULT cres;

    for (; *ptr != -1; ptr++) {
        clCheckColumn2WallHit(&cres, &pl[*ptr], col);
        if (cres.chk) {
            assert(*whnum < 32);
            clWallHitData[*whnum].kind = cres.chk;
            clWallHitData[*whnum].pl = (struct _CL_HITPOLY_PLANE *)cres.pd;
            cres.cv[1] = 0.0f;
            vcopy(cres.cv, clWallHitData[*whnum].cv);
            (*whnum)++;
        }
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1226
static void clCheckHitDynamicWallCollision(struct _CL_HITPOLY_COLUMN *col, int *whnum) {
    int i;
    int j;
    struct _CL_HITRESULT cres;
    int ac;

    ac = clDynamicWallListAct ? 0 : 1;
    for (i = 0; i < clDynamicWallList[ac].use; i++) {
        for (j = 0; clDynamicWallList[ac].dw[i][j].kind; j++) {
            clCheckColumn2WallHit(&cres, &clDynamicWallList[ac].dw[i][j], col);
            if (cres.chk) {
                assert_dw(*whnum < 32); /* Matching: do/while(0) form (its nop) */
                clWallHitData[*whnum].kind = cres.chk;
                clWallHitData[*whnum].pl = (struct _CL_HITPOLY_PLANE *)cres.pd;
                cres.cv[1] = 0.0f;
                vcopy(cres.cv, clWallHitData[*whnum].cv);
                (*whnum)++;
            }
        }
    }
}

static int clMakeWallHitCollectVector(struct SubCharacter *sc, float *wcv, float mang, int *flg, int num) {
    int i;
    int j;

    switch (num) {
    case 1:
        clAddWallCollectVector(wcv, clWallHitData[0].cv);
        break;
    case 0:
        break;
    default: {
        float ang0;
        float ang1;
        float newpos[4];
        float dif;

        for (i = 0; i < num; i++) {
            if (clWallHitData[i].kind != 3) {
                clCalcPlaneEquation(clWallHitData[i].pl, clWallHitData[i].normal);
                _shNormalize(clWallHitData[i].normal, clWallHitData[i].normal);
                clWallHitData[i].nang = shAtan2(clWallHitData[i].normal[0], clWallHitData[i].normal[2]);
                if (mang != 3.4028235e38f) {
                    if (clWallHitData[i].kind == 2) {
                        for (j = 0; j < num; j++) {
                            if (i != j && clWallHitData[j].kind == 1) {
                                _shAddVector(newpos, clWallHitData[j].cv, (float *)&sc->pos);
                                _shSubVector(newpos, newpos, (float *)&sc->b_pos);
                                ang1 = shAtan2(newpos[0], newpos[2]);
                                ang0 = clWallHitData[i].nang - ang1;
                                ang0 = shAngleRegulate(ang0);
                                if (ang0 < 1.55334306f && ang0 > -1.55334306f) {
                                    clWallHitData[i].kind = -1;
                                    break;
                                }
                            }
                        }
                    }
                    if (clWallHitData[i].kind != -1) {
                        ang0 = clWallHitData[i].nang - mang;
                        ang0 = shAngleRegulate(ang0);
                        if (ang0 < 1.30899692f && ang0 > -1.30899692f) {
                            clWallHitData[i].kind = -1;
                        }
                    }
                }
            }
        }
        for (i = 0; i < num; i++) {
            if (clWallHitData[i].kind == 2) {
                for (j = 0; j < num; j++) {
                    if (i != j && clWallHitData[j].kind == 1) {
                        ang1 = clWallHitData[j].nang - clWallHitData[i].nang;
                        ang1 = shAngleRegulate(ang1);
                        if (ang1 < 0.0349065848f && ang1 > -0.0349065848f) {
                            clWallHitData[i].kind = -1;
                            break;
                        }
                    }
                }
            }
        }
        if (sc->status & 0x10000) {
            sc->colis_fall_timer = (double)shGetFPS();
        }
        if (sc->colis_fall_timer) {
            for (i = 0; i < num; i++) {
                if (clWallHitData[i].kind != -1) {
                    ang0 = clWallHitData[i].nang - mang;
                    ang0 = shAngleRegulate(ang0);
                    if (ang0 < 1.57079637f && ang0 > -1.57079637f) {
                        dif = lengthXZ(clWallHitData[i].cv);
                        dif = 500.0f / shGetFPS() / dif;
                        if (dif < 1.0f) {
                            _shScaleVector(clWallHitData[i].cv, clWallHitData[i].cv, dif);
                        }
                    }
                }
            }
            sc->colis_fall_timer--;
        }
        if (sc->battle.status == 1 || sc->battle.status == 2) {
            for (i = 0; i < num - 1; i++) {
                if (clWallHitData[i].kind != -1) {
                    for (j = i + 1; j < num; j++) {
                        if (clWallHitData[j].kind != -1) {
                            if ((clWallHitData[i].cv[0] != 0.0f || clWallHitData[i].cv[2] != 0.0f) &&
                                (clWallHitData[j].cv[0] != 0.0f || clWallHitData[j].cv[2] != 0.0f)) {
                                ang0 = shAtan2(clWallHitData[i].cv[0], clWallHitData[i].cv[2]);
                                ang0 -= shAtan2(clWallHitData[j].cv[0], clWallHitData[j].cv[2]);
                                ang0 = shAngleRegulate(ang0);
                                if (ang0 > 2.09439516f || ang0 < -2.09439516f) {
                                    clWallHitData[i].kind = -2;
                                    clWallHitData[j].kind = -2;
                                }
                            }
                        }
                    }
                }
            }
        }
        for (i = 0; i < num; i++) {
            if (clWallHitData[i].kind >= 0) {
                clAddWallCollectVector(wcv, clWallHitData[i].cv, flg);
            }
        }
        break;
    }
    }
    return 1;
}

static void clAddWallCollectVector(float *v0, float *v1, int *flg) {
    float tv[4];

    if (*flg == 0) {
        vcopy(v1, v0);
    } else {
        _shAddVector(tv, v0, v1);
        if (v0[0] > v1[0]) {
            tv[0] = CL_CLAMP(tv[0], v1[0], v0[0]);
        } else {
            tv[0] = CL_CLAMP(tv[0], v0[0], v1[0]);
        }
        if (v0[2] > v1[2]) {
            tv[2] = CL_CLAMP(tv[2], v1[2], v0[2]);
        } else {
            tv[2] = CL_CLAMP(tv[2], v0[2], v1[2]);
        }
        vcopy(tv, v0);
    }
    (*flg)++;
}

/*
 * Matching: the VU0 edge-vector code is asm in the body: the original's line table has an entry per
 * instruction here. Its operands are bound in the order cp, vp, cv (the one order of six that matches).
 */
/* The asm below is the original's, in a C function's body: its line table has an entry per asm
 * instruction and the return on the closing brace. */
static void clCheckColumn2WallHit(struct _CL_HITRESULT *cres, struct _CL_HITPOLY_PLANE *pl, struct _CL_HITPOLY_COLUMN *col) {
    struct _CL_HITRESULT tmp;
    float normal[4];
    float pos[4];
    int hitchk;
    float vec[4];
    float iv0;
    float iv1;
    float vp[2][4];

    cres->chk = 0;
    _shSubVector(pos, col->p[0], pl->p[2]);
    clCalcPlaneEquation(pl, normal);
    if (vinner(pos, normal) < 0.0f) {
        return;
    }
    hitchk = clCheckSubWallToColumn(&tmp, (float (*)[4])pl->p[0], (float (*)[4])pl->p[2], (float (*)[4])col->p[0]);
    if (hitchk != 1) {
        return;
    }
    vcopy(pl->p[0], vp[0]);
    vcopy(pl->p[2], vp[1]);
    _shSubVector(vec, vp[0], vp[1]);
    vec[1] = 0.0f;
    pos[1] = 0.0f;
    iv0 = vinner(pos, vec);
    _shSubVector(vec, vp[1], vp[0]);
    _shSubVector(pos, col->p[0], vp[0]);
    vec[1] = 0.0f;
    pos[1] = 0.0f;
    iv1 = vinner(pos, vec);
    if ((iv0 < 0.0f && !(iv1 < 0.0f)) || (!(iv0 < 0.0f) && iv1 < 0.0f)) {
        __asm__ __volatile__("
        .set noreorder
        lqc2      vf4, 0x0(%0)
        lqc2      vf5, 0x0(%1)
        vsub.xyzw vf5, vf4, vf5
        vmul.xyzw vf6, vf5, vf5
        vaddz.x   vf6, vf6, vf6z
        qmfc2.ni  t0, vf6
        mtc1      t0, $f0
        lqc2      vf7, 0x10(%1)
        vsub.xyzw vf7, vf4, vf7
        vmul.xyzw vf8, vf7, vf7
        vaddz.x   vf8, vf8, vf8z
        qmfc2.ni  t0, vf8
        mtc1      t0, $f1
        c.lt.s    $f0, $f1
        nop
        bc1t      _cl_edge0
        nop
        vadd.xz   vf5, vf0, vf7
        vsqrt     Q, vf8x
        b         _cl_edge1
        nop
    _cl_edge0:
        nop
        nop
        vsqrt     Q, vf6x
    _cl_edge1:
        lqc2      vf6, 0x10(%0)
        vaddw.w   vf6, vf0, vf6w
        vaddw.x   vf6, vf0, vf6w
        vwaitq
        vsubq.x   vf6, vf6, Q
        vaddq.x   vf4, vf0, Q
        vdiv      Q, vf6x, vf4x
        vmove.yw  vf5, vf0
        vwaitq
        vmulq.xz  vf5, vf5, Q
        vnop
        sqc2      vf5, 0x0(%2)
        .set reorder
        " : : "r"(col->p[0]), "r"(vp[0]), "r"(tmp.cv));
        cres->chk = 2;
    } else {
        cres->chk = 1;
    }
    cres->pd = (struct _CL_HITPOLY_HEAD *)pl;
    vcopy(tmp.cv, cres->cv);
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1651
static void clCheckColumn2ColumnHit(struct _CL_HITPOLY_COLUMN *col, int *whnum, struct _CL_HITPOLY_COLUMN *cl, int *ptr) {
    struct _CL_HITRESULT cres;
    int hitchk;

    for (; *ptr != -1; ptr++) {
        hitchk = clCheckSubColumnToColumn(&cres, cl[*ptr].p, col->p);
        if (hitchk == 1) {
            assert_dw(*whnum < 32); /* Matching: do/while(0) form (its nop) */
            clWallHitData[*whnum].kind = 3;
            cres.cv[1] = 0.0f;
            vcopy(cres.cv, clWallHitData[*whnum].cv);
            (*whnum)++;
        }
    }
}

static void clCollectCharaHeightNormal(struct SubCharacter *sc) {
    float st[4];
    float ed[4];
    float *pos;
    float *matt;
    struct _CL_VHIT_RESULT res;

    matt = (float *)&sc->pos;
    st[0] = sc->pos.x;
    st[1] = sc->pos.y + -500.0f;
    st[2] = sc->pos.z;
    st[3] = 1.0f;
    ed[0] = sc->pos.x;
    ed[1] = sc->pos.y + 1500.0f;
    ed[2] = sc->pos.z;
    ed[3] = 1.0f;
    clCheckHitEyesOnlyFloor(&res, NULL, st, ed);
    if (res.kind == 1) {
        pos = matt;
        pos[1] = res.hobj.wall.cp[1];
        matt[1] = res.hobj.wall.cp[1];
        vcopy(res.hobj.wall.nl, sc->grnd_normal);
        sc->grnd_height = res.hobj.wall.cp[1];
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1727
/** Queues an attack check for clBattleCheckExec. */
void clBattleAddQue(struct _CL_BATTLE_QUE *que) {
    assert(clUseBattleQue < 64);
    memcpy(&clBattleQue[clUseBattleQue], que, sizeof(struct _CL_BATTLE_QUE));
    clUseBattleQue++;
}

/**
 * Returns the next unread battle result for attacker @p id (marking it read), or the empty
 * sentinel clBattleResult[64]. @param before previous result, or NULL to start over.
 */
struct _CL_BATTLE_RESULT *clBattleGetResult(unsigned int id, struct _CL_BATTLE_RESULT *before) {
    int i;

    if (before == NULL) {
        i = 0;
    } else {
        i = 1 - before->enable;
    }
    while (i < clUseBattleResult) {
        if (id == clBattleResult[i].id && clBattleResult[i].enable > 0) {
            clBattleResult[i].enable = -i;
            return &clBattleResult[i];
        }
        i++;
    }
    return &clBattleResult[64];
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1823
/** Runs the queued attack checks (sword/gun, swing/thrust) and records the results. */
void clBattleCheckExec(void) {
    int i;
    struct _CL_VHIT_RESULT vres;
    float dirc[4];

    clUseBattleResult = 0;
    for (i = 0; i < clUseBattleQue; i++) {
        switch (clBattleQue[i].kind) {
        case 2:
            clCheckHitSwordWeapon(&vres, (unsigned int)clBattleQue[i].sc, clBattleQue[i].svs, clBattleQue[i].sve, clBattleQue[i].evs, clBattleQue[i].eve);
            if (vres.kind) {
                _shSubVector(dirc, clBattleQue[i].eve, clBattleQue[i].sve);
                clSetOneBattleResult(&clBattleQue[i], &vres, dirc);
            }
            break;
        case 3:
        case 4:
            clCheckHitGunWeapon(&vres, (unsigned int)clBattleQue[i].sc, clBattleQue[i].svs, clBattleQue[i].sve);
            if (vres.kind) {
                _shSubVector(dirc, clBattleQue[i].sve, clBattleQue[i].svs);
                clSetOneBattleResult(&clBattleQue[i], &vres, dirc);
            }
            break;
        case 5:
        case 6:
            clCheckHitGunWeapon(&vres, (unsigned int)clBattleQue[i].sc, clBattleQue[i].svs, clBattleQue[i].sve);
            if (vres.kind == 3 && vres.hobj.chara.sc->kind != 0x100 && vres.hobj.chara.sc->kind != 0x101) {
                break;
            }
            if (vres.kind) {
                _shSubVector(dirc, clBattleQue[i].sve, clBattleQue[i].svs);
                clSetOneBattleResult(&clBattleQue[i], &vres, dirc);
            }
            break;
        case 1:
            clCheckHitSwordWeaponThrust((unsigned int)clBattleQue[i].sc, clBattleQue[i].svs, clBattleQue[i].sve, clBattleQue[i].evs, clBattleQue[i].eve);
            _shSubVector(dirc, clBattleQue[i].eve, clBattleQue[i].sve);
            clSetThrustBattleResult(&clBattleQue[i], dirc);
            break;
        case 7:
            clCheckHitGunWeaponThrust((unsigned int)clBattleQue[i].sc, clBattleQue[i].svs, clBattleQue[i].sve);
            _shSubVector(dirc, clBattleQue[i].sve, clBattleQue[i].svs);
            clSetThrustBattleResult(&clBattleQue[i], dirc);
            break;
        default:
            assert_dw(0); /* Matching: do/while(0) form (its nop) */
        }
    }
    clModifiedBattleData();
}

#define BR clBattleResult

static void clModifiedBattleData(void) {
    int i;
    int j;
    int k;

    for (i = 0; i < clUseBattleResult; i++) {
        if (BR[i].enable > 0) {
            if (BR[i].kind == 6 && BR[i].atr == 1) {
                for (j = 0; j < clUseBattleResult; j++) {
                    if (BR[j].enable > 0 && i != j && BR[j].atr == 4 && BR[i].id == BR[j].id) {
                        BR[i].enable = -1;
                        break;
                    }
                }
                if (BR[i].enable == -1) {
                    for (j = 0; j < clUseBattleResult; j++) {
                        if (BR[j].enable > 0 && i != j && BR[j].atr == 4 && BR[i].id == (unsigned int)BR[j].obj.en) {
                            BR[j].enable = -1;
                        }
                    }
                }
            }
            if (BR[i].atr == 4 && BR[i].kind == 6 && BR[i].enable > 0) {
                for (j = 0; j < clUseBattleResult; j++) {
                    if (BR[j].enable > 0 && i != j && BR[j].atr == 4 && BR[i].id == BR[j].id) {
                        BR[i].enable = -1;
                        for (k = 0; k < clUseBattleResult; k++) {
                            if (BR[k].enable > 0 && i != k && BR[k].kind == 6 && BR[k].atr == 1 && (unsigned int)BR[k].obj.en == BR[i].id) {
                                BR[k].enable = -1;
                            }
                        }
                    }
                }
            }
        }
    }
}

#define BRU clBattleResult[clUseBattleResult]

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1993
static void clSetOneBattleResult(struct _CL_BATTLE_QUE *que, struct _CL_VHIT_RESULT *vres, float *vec) {
    assert_dw(clUseBattleResult < 64); /* Matching: do/while(0) form (its nop) */
    switch (vres->kind) {
    case 1:
        BRU.enable = 1;
        BRU.id = (unsigned int)que->sc;
        BRU.atr = 2;
        BRU.kind = que->kind;
        BRU.btlid = que->btlid;
        vcopy(vres->hobj.wall.cp, BRU.pos);
        BRU.obj.pl = (struct _CL_HITPOLY_PLANE *)vres->hobj.wall.pd;
        clUseBattleResult++;
        break;
    case 2:
        BRU.enable = 1;
        BRU.id = (unsigned int)que->sc;
        BRU.atr = 3;
        BRU.kind = que->kind;
        BRU.btlid = que->btlid;
        vcopy(vres->hobj.wall.cp, BRU.pos);
        BRU.obj.pl = (struct _CL_HITPOLY_PLANE *)vres->hobj.wall.pd;
        clUseBattleResult++;
        break;
    case 3:
        BRU.enable = 1;
        BRU.id = (unsigned int)que->sc;
        BRU.atr = 1;
        vcopy(vres->hobj.chara.cp, BRU.pos);
        BRU.kind = que->kind;
        BRU.btlid = que->btlid;
        BRU.obj.en = vres->hobj.chara.sc;
        clUseBattleResult++;
        BRU.enable = 1;
        BRU.id = (unsigned int)vres->hobj.chara.sc;
        BRU.atr = 4;
        vcopy(vres->hobj.chara.cp, BRU.pos);
        vcopy(vec, BRU.vec);
        BRU.kind = que->kind;
        BRU.btlid = que->btlid;
        BRU.obj.en = que->sc;
        clUseBattleResult++;
        break;
    }
}

static void clSetThrustBattleResult(struct _CL_BATTLE_QUE *que, float *vec) {
    int i;
    int j;

    for (i = 0; i < clVHitListUse; i++) {
        switch (clVHitResult[i].kind) {
        case 1:
            BRU.enable = 1;
            BRU.id = (unsigned int)que->sc;
            BRU.atr = 2;
            BRU.kind = que->kind;
            BRU.btlid = que->btlid;
            vcopy(clVHitResult[i].hobj.wall.cp, BRU.pos);
            BRU.obj.pl = (struct _CL_HITPOLY_PLANE *)clVHitResult[i].hobj.wall.pd;
            clUseBattleResult++;
            break;
        case 2:
            BRU.enable = 1;
            BRU.id = (unsigned int)que->sc;
            BRU.atr = 3;
            BRU.kind = que->kind;
            BRU.btlid = que->btlid;
            vcopy(clVHitResult[i].hobj.wall.cp, BRU.pos);
            BRU.obj.pl = (struct _CL_HITPOLY_PLANE *)clVHitResult[i].hobj.wall.pd;
            clUseBattleResult++;
            break;
        case 3:
            for (j = 0; j < i; j++) {
                if (clVHitResult[i].hobj.chara.sc == clVHitResult[j].hobj.chara.sc) {
                    j = -1;
                    break;
                }
            }
            if (j != -1) {
                BRU.enable = 1;
                BRU.id = (unsigned int)que->sc;
                BRU.atr = 1;
                vcopy(clVHitResult[i].hobj.chara.cp, BRU.pos);
                BRU.obj.en = clVHitResult[i].hobj.chara.sc;
                BRU.kind = que->kind;
                BRU.btlid = que->btlid;
                clUseBattleResult++;
                BRU.enable = 1;
                BRU.id = (unsigned int)clVHitResult[i].hobj.chara.sc;
                BRU.atr = 4;
                vcopy(clVHitResult[i].hobj.chara.cp, BRU.pos);
                vcopy(vec, BRU.vec);
                BRU.kind = que->kind;
                BRU.btlid = que->btlid;
                BRU.obj.en = que->sc;
                clUseBattleResult++;
            }
            break;
        }
    }
}

float clswPerc[5] = { 1.0f, 0.7f, 0.5f, 0.3f, 0.0f };

static void clCheckHitSwordWeapon(struct _CL_VHIT_RESULT *res, unsigned int id, float *svs, float *sve, float *evs, float *eve) {
    int i;
    float st[4];
    float ed[4];
    float tmp[4];

    for (i = 0; i < 5; i++) {
        _shScaleVector(st, svs, clswPerc[i]);
        _shScaleVector(tmp, evs, clswPerc[4 - i]);
        _shAddVector(st, st, tmp);
        _shScaleVector(ed, sve, clswPerc[i]);
        _shScaleVector(tmp, eve, clswPerc[4 - i]);
        _shAddVector(ed, ed, tmp);
        clCheckHitSwordVector(res, id, st, ed);
        if (res->kind) {
            break;
        }
    }
}

static void clCheckHitGunWeapon(struct _CL_VHIT_RESULT *res, unsigned int id, float *st, float *ed) {
    clCheckHitEyeVector(res, id, st, ed);
}

static void clCheckHitSwordVector(struct _CL_VHIT_RESULT *res, unsigned int id, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    struct _CL_HITPOLY_COLUMN *cl;
    int *ptr;
    float min;

    clSquareDistance(sp, ep, &min);
    res->kind = 0;
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    clCheckHitSwordVectorDynamicWall(res, sp, ep, &min);
    clCheckHitSwordVectorDynamicFloor(res, sp, ep, &min);
    if (smap->base) {
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->fldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b0ofs[smap->sect]);
            clCheckHitSwordVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->wldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b1ofs[smap->sect]);
            clCheckHitSwordVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->cedofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b2ofs[smap->sect]);
            clCheckHitSwordVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            cl = (struct _CL_HITPOLY_COLUMN *)(smap->base + CLD(smap->base)->cldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->clofs[smap->sect]);
            clCheckHitEyeVectorBGColumn(res, sp, ep, &min, cl, ptr);
            smap++;
        }
    }
    clCheckHitEyeVectorCharacter(res, sp, ep, &min, id);
}

static void clCheckHitSwordVectorWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, struct _CL_HITPOLY_PLANE *pl, int *ptr) {
    int ret;
    struct _CL_HITRESULT cres;
    float dist;

    for (; *ptr != -1; ptr++) {
        if (pl[*ptr].material == 12) {
            continue;
        }
        if (pl[*ptr].shape == 0) {
            ret = clCheckSubLineToPlane3(&cres, (float (*)[4])sp, (float (*)[4])ep, &pl[*ptr].p[0], &pl[*ptr].p[1], &pl[*ptr].p[2]);
        } else {
            ret = clCheckSubLineToPlane(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])pl[*ptr].p[0], (float (*)[4])pl[*ptr].p[1], (float (*)[4])pl[*ptr].p[2], (float (*)[4])pl[*ptr].p[3]);
        }
        if (ret) {
            clSquareDistance(sp, cres.cp, &dist);
            if (dist < *min) {
                *min = dist;
                res->kind = 1;
                vcopy(cres.cp, res->hobj.wall.cp);
                clCalcPlaneEquation(&pl[*ptr], res->hobj.wall.nl);
                res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&pl[*ptr];
            }
        }
    }
}

static void clCheckHitNoThruVectorWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, struct _CL_HITPOLY_PLANE *pl, int *ptr) {
    int ret;
    struct _CL_HITRESULT cres;
    float dist;

    for (; *ptr != -1; ptr++) {
        if (pl[*ptr].shape == 0) {
            ret = clCheckSubLineToPlane3(&cres, (float (*)[4])sp, (float (*)[4])ep, &pl[*ptr].p[0], &pl[*ptr].p[1], &pl[*ptr].p[2]);
        } else {
            ret = clCheckSubLineToPlane(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])pl[*ptr].p[0], (float (*)[4])pl[*ptr].p[1], (float (*)[4])pl[*ptr].p[2], (float (*)[4])pl[*ptr].p[3]);
        }
        if (ret) {
            clSquareDistance(sp, cres.cp, &dist);
            if (dist < *min) {
                *min = dist;
                res->kind = 1;
                vcopy(cres.cp, res->hobj.wall.cp);
                clCalcPlaneEquation(&pl[*ptr], res->hobj.wall.nl);
                res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&pl[*ptr];
            }
        }
    }
}

static void clCheckHitSwordVectorDynamicWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min) {
    int i;
    int j;
    int ret;
    struct _CL_HITRESULT cres;
    float dist;
    int ac;

    ac = clDynamicWallListAct ? 0 : 1;
    for (i = 0; i < clDynamicWallList[ac].use; i++) {
        for (j = 0; clDynamicWallList[ac].dw[i][j].kind; j++) {
            if (clDynamicWallList[ac].dw[i][j].material == 12) {
                continue;
            }
            if (clDynamicWallList[ac].dw[i][j].shape == 0) {
                ret = clCheckSubLineToPlane3(&cres, (float (*)[4])sp, (float (*)[4])ep, &clDynamicWallList[ac].dw[i][j].p[0], &clDynamicWallList[ac].dw[i][j].p[1], &clDynamicWallList[ac].dw[i][j].p[2]);
            } else {
                ret = clCheckSubLineToPlane(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])clDynamicWallList[ac].dw[i][j].p[0], (float (*)[4])clDynamicWallList[ac].dw[i][j].p[1], (float (*)[4])clDynamicWallList[ac].dw[i][j].p[2], (float (*)[4])clDynamicWallList[ac].dw[i][j].p[3]);
            }
            if (ret) {
                clSquareDistance(sp, cres.cp, &dist);
                if (dist < *min) {
                    *min = dist;
                    res->kind = 1;
                    vcopy(cres.cp, res->hobj.wall.cp);
                    clCalcPlaneEquation(&clDynamicWallList[ac].dw[i][j], res->hobj.wall.nl);
                    res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&clDynamicWallList[ac].dw[i][j];
                }
            }
        }
    }
}

static void clCheckHitSwordVectorDynamicWallNoThru(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min) {
    int i;
    int j;
    int ret;
    struct _CL_HITRESULT cres;
    float dist;
    int ac;

    ac = clDynamicWallListAct ? 0 : 1;
    for (i = 0; i < clDynamicWallList[ac].use; i++) {
        for (j = 0; clDynamicWallList[ac].dw[i][j].kind; j++) {
            if (clDynamicWallList[ac].dw[i][j].shape == 0) {
                ret = clCheckSubLineToPlane3(&cres, (float (*)[4])sp, (float (*)[4])ep, &clDynamicWallList[ac].dw[i][j].p[0], &clDynamicWallList[ac].dw[i][j].p[1], &clDynamicWallList[ac].dw[i][j].p[2]);
            } else {
                ret = clCheckSubLineToPlane(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])clDynamicWallList[ac].dw[i][j].p[0], (float (*)[4])clDynamicWallList[ac].dw[i][j].p[1], (float (*)[4])clDynamicWallList[ac].dw[i][j].p[2], (float (*)[4])clDynamicWallList[ac].dw[i][j].p[3]);
            }
            if (ret) {
                clSquareDistance(sp, cres.cp, &dist);
                if (dist < *min) {
                    *min = dist;
                    res->kind = 1;
                    vcopy(cres.cp, res->hobj.wall.cp);
                    clCalcPlaneEquation(&clDynamicWallList[ac].dw[i][j], res->hobj.wall.nl);
                    res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&clDynamicWallList[ac].dw[i][j];
                }
            }
        }
    }
}

static void clCheckHitSwordVectorDynamicFloor(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min) {
    int i;
    int j;
    int ret;
    struct _CL_HITRESULT cres;
    float dist;
    int ac;

    ac = clDynamicFloorListAct ? 0 : 1;
    for (i = 0; i < clDynamicFloorList[ac].use; i++) {
        for (j = 0; clDynamicFloorList[ac].dw[i][j].kind; j++) {
            if (clDynamicFloorList[ac].dw[i][j].material == 12) {
                continue;
            }
            if (clDynamicFloorList[ac].dw[i][j].shape == 0) {
                ret = clCheckSubLineToPlane3(&cres, (float (*)[4])sp, (float (*)[4])ep, &clDynamicFloorList[ac].dw[i][j].p[0], &clDynamicFloorList[ac].dw[i][j].p[1], &clDynamicFloorList[ac].dw[i][j].p[2]);
            } else {
                ret = clCheckSubLineToPlane(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[0], (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[1], (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[2], (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[3]);
            }
            if (ret) {
                clSquareDistance(sp, cres.cp, &dist);
                if (dist < *min) {
                    *min = dist;
                    res->kind = 1;
                    vcopy(cres.cp, res->hobj.wall.cp);
                    clCalcPlaneEquation(&clDynamicFloorList[ac].dw[i][j], res->hobj.wall.nl);
                    res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&clDynamicFloorList[ac].dw[i][j];
                }
            }
        }
    }
}

static void clCheckHitSwordVectorDynamicFloorNoThru(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min) {
    int i;
    int j;
    int ret;
    struct _CL_HITRESULT cres;
    float dist;
    int ac;

    ac = clDynamicFloorListAct ? 0 : 1;
    for (i = 0; i < clDynamicFloorList[ac].use; i++) {
        for (j = 0; clDynamicFloorList[ac].dw[i][j].kind; j++) {
            if (clDynamicFloorList[ac].dw[i][j].shape == 0) {
                ret = clCheckSubLineToPlane3(&cres, (float (*)[4])sp, (float (*)[4])ep, &clDynamicFloorList[ac].dw[i][j].p[0], &clDynamicFloorList[ac].dw[i][j].p[1], &clDynamicFloorList[ac].dw[i][j].p[2]);
            } else {
                ret = clCheckSubLineToPlane(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[0], (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[1], (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[2], (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[3]);
            }
            if (ret) {
                clSquareDistance(sp, cres.cp, &dist);
                if (dist < *min) {
                    *min = dist;
                    res->kind = 1;
                    vcopy(cres.cp, res->hobj.wall.cp);
                    clCalcPlaneEquation(&clDynamicFloorList[ac].dw[i][j], res->hobj.wall.nl);
                    res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&clDynamicFloorList[ac].dw[i][j];
                }
            }
        }
    }
}

static void clCheckHitSwordWeaponThrust(unsigned int id, float *svs, float *sve, float *evs, float *eve) {
    int i;
    float st[4];
    float ed[4];
    float tmp[4];

    clVHitListUse = 0;
    for (i = 0; i < 5; i++) {
        _shScaleVector(st, svs, clswPerc[i]);
        _shScaleVector(tmp, evs, clswPerc[4 - i]);
        _shAddVector(st, st, tmp);
        _shScaleVector(ed, sve, clswPerc[i]);
        _shScaleVector(tmp, eve, clswPerc[4 - i]);
        _shAddVector(ed, ed, tmp);
        if (clCheckHitThrustSwordVector(id, st, ed)) {
            break;
        }
    }
}

static int clCheckHitThrustSwordVector(unsigned int id, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    struct _CL_HITPOLY_COLUMN *cl;
    int *ptr;
    float min;
    int whflg;

    whflg = 0;
    clSquareDistance(sp, ep, &min);
    clVHitResult[clVHitListUse].kind = 0;
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    clCheckHitSwordVectorDynamicWall(&clVHitResult[clVHitListUse], sp, ep, &min);
    clCheckHitSwordVectorDynamicFloor(&clVHitResult[clVHitListUse], sp, ep, &min);
    if (smap->base) {
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->fldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b0ofs[smap->sect]);
            clCheckHitSwordVectorWall(&clVHitResult[clVHitListUse], sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->wldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b1ofs[smap->sect]);
            clCheckHitSwordVectorWall(&clVHitResult[clVHitListUse], sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->cedofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b2ofs[smap->sect]);
            clCheckHitSwordVectorWall(&clVHitResult[clVHitListUse], sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            cl = (struct _CL_HITPOLY_COLUMN *)(smap->base + CLD(smap->base)->cldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->clofs[smap->sect]);
            clCheckHitEyeVectorBGColumn(&clVHitResult[clVHitListUse], sp, ep, &min, cl, ptr);
            smap++;
        }
    }
    if (clVHitResult[clVHitListUse].kind) {
        clVHitListUse++;
        whflg = 1;
    }
    clCheckHitThrustGunVectorCharacter(sp, ep, min, id);
    return whflg;
}

static void clCheckHitGunWeaponThrust(unsigned int id, float *st, float *ed) {
    clVHitListUse = 0;
    clCheckHitThrustGunVector(id, st, ed);
}


static void clCheckHitThrustGunVector(unsigned int id, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    struct _CL_HITPOLY_COLUMN *cl;
    int *ptr;
    float min;

    clSquareDistance(sp, ep, &min);
    clVHitResult[clVHitListUse].kind = 0;
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    clCheckHitEyeVectorDynamicWall(&clVHitResult[clVHitListUse], sp, ep, &min);
    clCheckHitEyeVectorDynamicFloor(&clVHitResult[clVHitListUse], sp, ep, &min);
    if (smap->base) {
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->fldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b0ofs[smap->sect]);
            clCheckHitEyeVectorWall(&clVHitResult[clVHitListUse], sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->wldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b1ofs[smap->sect]);
            clCheckHitEyeVectorWall(&clVHitResult[clVHitListUse], sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->cedofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b2ofs[smap->sect]);
            clCheckHitEyeVectorWall(&clVHitResult[clVHitListUse], sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            cl = (struct _CL_HITPOLY_COLUMN *)(smap->base + CLD(smap->base)->cldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->clofs[smap->sect]);
            clCheckHitEyeVectorBGColumn(&clVHitResult[clVHitListUse], sp, ep, &min, cl, ptr);
            smap++;
        }
    }
    if (clVHitResult[clVHitListUse].kind) {
        clVHitListUse++;
    }
    clCheckHitThrustGunVectorCharacter(sp, ep, min, id);
}

static void clCheckHitThrustGunVectorCharacter(float *sp, float *ep, float min, unsigned int id) {
    int i;
    int ret;
    struct _CL_HITRESULT cres;
    float dist;
    int ac;

    ac = clCharaListAct ? 0 : 1;
    for (i = 0; i < clCharaListUse[ac]; i++) {
        if (id != (unsigned int)clCharaList[ac][i].sc && clCharaList[ac][i].batflg) {
            ret = clCheckSubLineToColumnPlus(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])clCharaList[ac][i].wcol.p[0]);
            if (ret) {
                clSquareDistance(sp, cres.cp, &dist);
                if (dist < min) {
                    clVHitResult[clVHitListUse].kind = 3;
                    vcopy(cres.cp, clVHitResult[clVHitListUse].hobj.chara.cp);
                    clVHitResult[clVHitListUse].hobj.chara.sc = clCharaList[ac][i].sc;
                    clVHitListUse++;
                }
            }
        }
    }
}

/** Returns the map sections a ray from @p st to @p ed may hit (outdoor or indoor lookup). */
struct _CL_SELECT_MAP *clGetHitSectListVECHIT(float *st, float *ed) {
    if (BgIsOut(0)) {
        return clGetHitSectListVECHITOutDoor(st, ed);
    } else {
        return clGetHitSectListVECHITInDoor(st, ed);
    }
}

/* Matching: an inline helper, so sx/sz are not in the DWARF; without them ch->sx/ch->sz are reloaded
 * instead of being kept in $f20/$f21 across the calls. */
static inline void clMakeBoxXZ(float (*box)[4], float x, float z, float w) {
    box[0][0] = x;
    box[0][2] = z;
    box[1][0] = x;
    box[1][2] = w + z;
    box[2][0] = w + x;
    box[2][2] = w + z;
    box[3][0] = w + x;
    box[3][2] = z;
}

static struct _CL_SELECT_MAP *clGetHitSectListVECHITOutDoor(float *st, float *ed) {
    float sx;
    float sz;
    int j;
    int k;
    int use;
    struct _CL_CLDHEADER *ch;
    float box[4][4];
    void **list;

    use = 0;
    if (clCollisionEnable) {
        for (list = loadBgCLD_GetLoadedDataAddrList(); (ch = *list) != NULL; list++) {
            if (ch->disable) {
                continue;
            }
            sx = ch->sx;
            sz = ch->sz;
            clMakeBoxXZ(box, sx, sz, 20000.0f);
            if (Line2PlaneBoundaryCheckXZ((float (*)[4])st, (float (*)[4])ed, (float (*)[4])box[0], &box[1], &box[2], &box[3])) {
                continue;
            }
            if (!clCheckCrossLine2BoxXZ(box, st, ed)) {
                continue;
            }
            for (j = 0; j < 4; j++) {
                for (k = 0; k < 4; k++) {
                    box[0][0] = sx + 5000.0f * k;
                    box[0][2] = sz + 5000.0f * j;
                    box[1][0] = box[0][0];
                    box[1][2] = box[0][2] + 5000.0f;
                    box[2][0] = box[0][0] + 5000.0f;
                    box[2][2] = box[0][2] + 5000.0f;
                    box[3][0] = box[0][0] + 5000.0f;
                    box[3][2] = box[0][2];
                    if (!Line2PlaneBoundaryCheckXZ((float (*)[4])st, (float (*)[4])ed, (float (*)[4])box[0], &box[1], &box[2], &box[3]) && clCheckCrossLine2BoxXZ(box, st, ed)) {
                        clSelectMap[use].base = (unsigned char *)ch;
                        clSelectMap[use].sect = j * 4 + k;
                        use++;
                    }
                }
            }
        }
    }
    clSelectMap[use].base = NULL;
    return clSelectMap;
}

/* Matching: K&R definition; see its declaration above. */
static struct _CL_SELECT_MAP *clGetHitSectListVECHITInDoor() {
    int use;
    struct _CL_CLDHEADER *ch;
    void **list;

    use = 0;
    if (clCollisionEnable) {
        for (list = loadBgCLD_GetLoadedDataAddrList(); (ch = *list) != NULL; list++) {
            if (!ch->disable) {
                clSelectMap[use].base = (unsigned char *)ch;
                clSelectMap[use].sect = 0;
                use++;
            }
        }
    }
    clSelectMap[use].base = NULL;
    return clSelectMap;
}

/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
static int Line2PlaneBoundaryCheckXZ(float (*l0)[4], float (*l1)[4], float (*p0)[4], float (*p1)[4], float (*p2)[4], float (*p3)[4]) {
    int ret;

    __asm__ __volatile__("
    lqc2     vf1, 0x0(%1)
    lqc2     vf2, 0x0(%2)
    lqc2     vf3, 0x0(%3)
    lqc2     vf4, 0x0(%4)
    lqc2     vf5, 0x0(%5)
    lqc2     vf6, 0x0(%6)
    vmax.xz  vf7, vf3, vf4
    vmax.xz  vf8, vf5, vf6
    vmini.xz vf9, vf3, vf4
    vmini.xz vf10, vf5, vf6
    vmax.xz  vf7, vf7, vf8
    vmini.xz vf8, vf1, vf2
    vmini.xz vf9, vf9, vf10
    vmax.xz  vf10, vf1, vf2
    vnop
    ctc2.ni  zero, vi16
    vsub.xz  vf7, vf7, vf8
    vsub.xz  vf8, vf10, vf9
    vnop
    vnop
    vnop
    vnop
    vnop
    cfc2.ni  %0, vi16
    andi     %0, %0, 0x80
    " : "=r"(ret) : "r"(l0), "r"(l1), "r"(p0), "r"(p1), "r"(p2), "r"(p3));
    return ret;
}

/** Returns non-zero if the segment @p st-@p ed crosses the XZ quad @p box. */
int clCheckCrossLine2BoxXZ(float (*box)[4], float *st, float *ed) {
    int i;
    float va[4];
    float vb[4];
    float outer;
    int jyun[5] = { 0, 1, 2, 3, 0 };

    for (i = 0; i < 4; i++) {
        _shSubVector(va, box[jyun[i + 1]], box[jyun[i]]);
        _shSubVector(vb, st, box[i]);
        outer = va[0] * vb[2] - va[2] * vb[0];
        if (outer > 0.0f) {
            break;
        }
    }
    if (i == 4) {
        return 1;
    }
    for (i = 0; i < 4; i++) {
        _shSubVector(va, box[jyun[i + 1]], box[jyun[i]]);
        _shSubVector(vb, ed, box[i]);
        outer = va[0] * vb[2] - va[2] * vb[0];
        if (outer > 0.0f) {
            break;
        }
    }
    if (i == 4) {
        return 1;
    }
    for (i = 0; i < 4; i++) {
        if (clCheckCrossLine2LineXZ(st, ed, box[jyun[i]], box[jyun[i + 1]])) {
            return 1;
        }
    }
    return 0;
}

/** Returns non-zero if the segments @p va0-@p va1 and @p vb0-@p vb1 cross in XZ. */
int clCheckCrossLine2LineXZ(float *va0, float *va1, float *vb0, float *vb1) {
    float bp[4];
    float p0[4];
    float p1[4];
    float outer0;
    float outer1;

    _shSubVector(bp, va1, va0);
    _shSubVector(p0, vb0, va0);
    _shSubVector(p1, vb1, va0);
    outer0 = bp[0] * p0[2] - bp[2] * p0[0];
    outer1 = bp[0] * p1[2] - bp[2] * p1[0];
    if (outer0 == 0.0f && outer1 == 0.0f) {
        if ((va0[0] <= fmaxf(vb0[0], vb1[0]) && va0[0] >= fminf(vb0[0], vb1[0]) && va0[2] <= fmaxf(vb0[2], vb1[2]) && va0[2] >= fminf(vb0[2], vb1[2]))
            || (va1[0] <= fmaxf(vb0[0], vb1[0]) && va1[0] >= fminf(vb0[0], vb1[0]) && va1[2] <= fmaxf(vb0[2], vb1[2]) && va1[2] >= fminf(vb0[2], vb1[2]))
            || (vb0[0] <= fmaxf(va0[0], va1[0]) && vb0[0] >= fminf(va0[0], va1[0]) && vb0[2] <= fmaxf(va0[2], va1[2]) && vb0[2] >= fminf(va0[2], va1[2]))
            || (vb1[0] <= fmaxf(va0[0], va1[0]) && vb1[0] >= fminf(va0[0], va1[0]) && vb1[2] <= fmaxf(va0[2], va1[2]) && vb1[2] >= fminf(va0[2], va1[2]))) {
            return 1;
        }
        return 0;
    }
    if (outer0 * outer1 >= 0.0f) {
        return 0;
    }
    _shSubVector(bp, vb1, vb0);
    _shSubVector(p0, va0, vb0);
    _shSubVector(p1, va1, vb0);
    outer0 = bp[0] * p0[2] - bp[2] * p0[0];
    outer1 = bp[0] * p1[2] - bp[2] * p1[0];
    if (outer0 * outer1 >= 0.0f) {
        return 0;
    }
    return 1;
}

/** Returns the map sections to test for a character at @p bpos (outdoor or indoor lookup). */
struct _CL_SELECT_MAP *clGetHitSectListMOVE(float *bpos) {
    if (BgIsOut(0)) {
        return clGetHitSectListMOVEOutDoor(bpos);
    } else {
        return clGetHitSectListMOVEInDoor(bpos);
    }
}

/*
 * Matching: the squared-distance code is asm in the body, one instruction per line with blank lines
 * where the original's line table has gaps; operands in the order a, cpos, &dist (the one that matches).
 */
static struct _CL_SELECT_MAP *clGetHitSectListMOVEOutDoor(float *bpos) {
    int j;
    int k;
    int use;
    struct _CL_CLDHEADER *ch;
    float pos[4];
    float cpos[4];
    float bcpos[4];
    float dist;
    void **list;

    use = 0;
    vcopy(bpos, pos);
    pos[1] = 0.0f;
    pos[3] = 1.0f;
    *(u_long128 *)cpos = 0;
    if (clCollisionEnable) {
        for (list = loadBgCLD_GetLoadedDataAddrList(); (ch = *list) != NULL; list++) {
            if (ch->disable) {
                continue;
            }
            cpos[0] = ch->sx;
            cpos[2] = ch->sz;
            vcopy(cpos, bcpos);
            __asm__ __volatile__("
                lqc2     vf1, 0x0(%0)
                lqc2     vf2, 0x0(%1)

                vsub.xz  vf3, vf1, vf2
                vmul.xz  vf3, vf3, vf3
                vaddz.x  vf3, vf3, vf3z

                qmfc2.ni t0, vf3
                mtc1     t0, $f12
                sw       t0, 0x0(%2)
            " : : "r"(bpos), "r"(cpos), "r"(&dist));
            if (dist <= 9e8f) {
                for (j = 0; j < 4; j++) {
                    for (k = 0; k < 4; k++) {
                        cpos[2] += 5000.0f * j;
                        cpos[0] += 5000.0f * k;
                        __asm__ __volatile__("
                            lqc2     vf1, 0x0(%0)
                            lqc2     vf2, 0x0(%1)

                            vsub.xz  vf3, vf1, vf2
                            vmul.xz  vf3, vf3, vf3
                            vaddz.x  vf3, vf3, vf3z

                            qmfc2.ni t0, vf3
                            mtc1     t0, $f12
                            sw       t0, 0x0(%2)
                        " : : "r"(pos), "r"(cpos), "r"(&dist));
                        if (dist < 6.4e7f) {
                            clSelectMap[use].base = (unsigned char *)ch;
                            clSelectMap[use].sect = j * 4 + k;
                            use++;
                        }
                        vcopy(bcpos, cpos);
                    }
                }
            }
        }
    }
    clSelectMap[use].base = NULL;
    return clSelectMap;
}

static struct _CL_SELECT_MAP *clGetHitSectListMOVEInDoor() { /* Matching: K&R definition; see its declaration above. */
    int use;
    struct _CL_CLDHEADER *ch;
    void **list;

    use = 0;
    if (clCollisionEnable) {
        for (list = loadBgCLD_GetLoadedDataAddrList(); (ch = *list) != NULL; list++) {
            if (!ch->disable) {
                clSelectMap[use].base = (unsigned char *)ch;
                clSelectMap[use].sect = 0;
                use++;
            }
        }
    }
    clSelectMap[use].base = NULL;
    return clSelectMap;
}

/**
 * Casts a sight ray from @p st to @p ed. @param res out: nearest hit. @param id the looking
 * character. @param thru 1: clCheckHitEyeVector, 0: its NoThru variant, 2: its AllNoThru variant.
 */
void clCheckHitEyes(struct _CL_VHIT_RESULT *res, unsigned int id, float *st, float *ed, int thru) {
    switch (thru) {
    case 1:
        clCheckHitEyeVector(res, id, st, ed);
        break;
    case 0:
        clCheckHitEyeVectorNoThru(res, id, st, ed);
        break;
    case 2:
        clCheckHitEyeVectorAllNoThru(res, id, st, ed);
        break;
    }
}

/** Casts a ray from @p sp to @p ep against floors only (dynamic and map). @param res out: nearest
 * hit. @param scp unused. */
void clCheckHitEyesOnlyFloor(struct _CL_VHIT_RESULT *res, struct SubCharacter *scp, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    int *ptr;
    float min;

    clSquareDistance(sp, ep, &min);
    res->kind = 0;
    clCheckHitEyeVectorDynamicFloor(res, sp, ep, &min);
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    if (smap->base) {
        while (smapsv->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smapsv->base + CLD(smapsv->base)->fldofs);
            ptr = (int *)(smapsv->base + CLD(smapsv->base)->b0ofs[smapsv->sect]);
            clCheckHitSwordVectorWall(res, sp, ep, &min, wall, ptr);
            smapsv++;
        }
    }
}

/** Like clCheckHitEyesOnlyFloor, but tests map floors with clCheckHitEyeVectorWall instead of the sword-ray test. */
void clCheckHitEyesOnlyFloorThru(struct _CL_VHIT_RESULT *res, struct SubCharacter *scp, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    int *ptr;
    float min;

    clSquareDistance(sp, ep, &min);
    res->kind = 0;
    clCheckHitEyeVectorDynamicFloor(res, sp, ep, &min);
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    if (smap->base) {
        while (smapsv->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smapsv->base + CLD(smapsv->base)->fldofs);
            ptr = (int *)(smapsv->base + CLD(smapsv->base)->b0ofs[smapsv->sect]);
            clCheckHitEyeVectorWall(res, sp, ep, &min, wall, ptr);
            smapsv++;
        }
    }
}

/** Casts a ray from @p sp to @p ep against walls, ceilings and columns (dynamic walls and map).
 * @param res out: nearest hit. */
void clCheckHitEyesOnlyWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    struct _CL_HITPOLY_COLUMN *cl;
    int *ptr;
    float min;

    clSquareDistance(sp, ep, &min);
    res->kind = 0;
    clCheckHitSwordVectorDynamicWall(res, sp, ep, &min);
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    if (smap->base) {
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->wldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b1ofs[smap->sect]);
            clCheckHitSwordVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->cedofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b2ofs[smap->sect]);
            clCheckHitEyeVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            cl = (struct _CL_HITPOLY_COLUMN *)(smap->base + CLD(smap->base)->cldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->clofs[smap->sect]);
            clCheckHitEyeVectorBGColumn(res, sp, ep, &min, cl, ptr);
            smap++;
        }
    }
}

/** Casts a ray from @p sp to @p ep against floors and ceilings. @param res out: nearest hit. */
void clCheckHitEyesOnlyFloorCeil(struct _CL_VHIT_RESULT *res, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    int *ptr;
    float min;

    clSquareDistance(sp, ep, &min);
    res->kind = 0;
    clCheckHitEyeVectorDynamicFloor(res, sp, ep, &min);
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    if (smap->base) {
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->fldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b0ofs[smap->sect]);
            clCheckHitEyeVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->cedofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b2ofs[smap->sect]);
            clCheckHitEyeVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
    }
}
static void clCheckHitEyeVector(struct _CL_VHIT_RESULT *res, unsigned int id, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    struct _CL_HITPOLY_COLUMN *cl;
    int *ptr;
    float min;

    clSquareDistance(sp, ep, &min);
    res->kind = 0;
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    clCheckHitEyeVectorDynamicWall(res, sp, ep, &min);
    clCheckHitEyeVectorDynamicFloor(res, sp, ep, &min);
    if (smap->base) {
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->fldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b0ofs[smap->sect]);
            clCheckHitEyeVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->wldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b1ofs[smap->sect]);
            clCheckHitEyeVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->cedofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b2ofs[smap->sect]);
            clCheckHitEyeVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            cl = (struct _CL_HITPOLY_COLUMN *)(smap->base + CLD(smap->base)->cldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->clofs[smap->sect]);
            clCheckHitEyeVectorBGColumn(res, sp, ep, &min, cl, ptr);
            smap++;
        }
    }
    clCheckHitEyeVectorCharacter(res, sp, ep, &min, id);
}

static void clCheckHitEyeVectorNoThru(struct _CL_VHIT_RESULT *res, unsigned int id, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    struct _CL_HITPOLY_COLUMN *cl;
    int *ptr;
    float min;

    clSquareDistance(sp, ep, &min);
    res->kind = 0;
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    clCheckHitSwordVectorDynamicWall(res, sp, ep, &min);
    clCheckHitSwordVectorDynamicFloor(res, sp, ep, &min);
    if (smap->base) {
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->fldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b0ofs[smap->sect]);
            clCheckHitSwordVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->wldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b1ofs[smap->sect]);
            clCheckHitSwordVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->cedofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b2ofs[smap->sect]);
            clCheckHitSwordVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            cl = (struct _CL_HITPOLY_COLUMN *)(smap->base + CLD(smap->base)->cldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->clofs[smap->sect]);
            clCheckHitEyeVectorBGColumn(res, sp, ep, &min, cl, ptr);
            smap++;
        }
    }
    clCheckHitEyeVectorCharacter(res, sp, ep, &min, id);
}

static void clCheckHitEyeVectorAllNoThru(struct _CL_VHIT_RESULT *res, unsigned int id, float *sp, float *ep) {
    struct _CL_SELECT_MAP *smap;
    struct _CL_SELECT_MAP *smapsv;
    struct _CL_HITPOLY_PLANE *wall;
    struct _CL_HITPOLY_COLUMN *cl;
    int *ptr;
    float min;

    clSquareDistance(sp, ep, &min);
    res->kind = 0;
    smap = clGetHitSectListVECHIT(sp, ep);
    smapsv = smap;
    clCheckHitSwordVectorDynamicWallNoThru(res, sp, ep, &min);
    clCheckHitSwordVectorDynamicFloorNoThru(res, sp, ep, &min);
    if (smap->base) {
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->fldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b0ofs[smap->sect]);
            clCheckHitNoThruVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->wldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b1ofs[smap->sect]);
            clCheckHitNoThruVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->swdofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b3ofs[smap->sect]);
            clCheckHitNoThruVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            wall = (struct _CL_HITPOLY_PLANE *)(smap->base + CLD(smap->base)->cedofs);
            ptr = (int *)(smap->base + CLD(smap->base)->b2ofs[smap->sect]);
            clCheckHitNoThruVectorWall(res, sp, ep, &min, wall, ptr);
            smap++;
        }
        smap = smapsv;
        while (smap->base) {
            cl = (struct _CL_HITPOLY_COLUMN *)(smap->base + CLD(smap->base)->cldofs);
            ptr = (int *)(smap->base + CLD(smap->base)->clofs[smap->sect]);
            clCheckHitEyeVectorBGColumn(res, sp, ep, &min, cl, ptr);
            smap++;
        }
    }
    clCheckHitEyeVectorCharacter(res, sp, ep, &min, id);
}

static void clCheckHitEyeVectorWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, struct _CL_HITPOLY_PLANE *pl, int *ptr) {
    int ret;
    struct _CL_HITRESULT cres;
    float dist;

    for (; *ptr != -1; ptr++) {
        if (pl[*ptr].material == 9 || pl[*ptr].material == 11 || pl[*ptr].material == 12) {
            continue;
        }
        if (pl[*ptr].shape == 0) {
            ret = clCheckSubLineToPlane3(&cres, (float (*)[4])sp, (float (*)[4])ep, &pl[*ptr].p[0], &pl[*ptr].p[1], &pl[*ptr].p[2]);
        } else {
            ret = clCheckSubLineToPlane(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])pl[*ptr].p[0], (float (*)[4])pl[*ptr].p[1], (float (*)[4])pl[*ptr].p[2], (float (*)[4])pl[*ptr].p[3]);
        }
        if (ret) {
            clSquareDistance(sp, cres.cp, &dist);
            if (dist < *min) {
                *min = dist;
                res->kind = 1;
                vcopy(cres.cp, res->hobj.wall.cp);
                clCalcPlaneEquation(&pl[*ptr], res->hobj.wall.nl);
                res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&pl[*ptr];
            }
        }
    }
}

static void clCheckHitEyeVectorBGColumn(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, struct _CL_HITPOLY_COLUMN *cl, int *ptr) {
    int ret;
    struct _CL_HITRESULT cres;
    float dist;

    for (; *ptr != -1; ptr++) {
        ret = clCheckSubLineToColumnPlus(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])cl[*ptr].p[0]);
        if (ret) {
            clSquareDistance(sp, cres.cp, &dist);
            if (dist < *min) {
                *min = dist;
                res->kind = 2;
                vcopy(cres.cp, res->hobj.wall.cp);
                *(u_long128 *)res->hobj.wall.nl = 0;
                res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&cl[*ptr];
            }
        }
    }
}

static void clCheckHitEyeVectorDynamicWall(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min) {
    int i;
    int j;
    int ret;
    struct _CL_HITRESULT cres;
    float dist;
    int ac;

    ac = clDynamicWallListAct ? 0 : 1;
    for (i = 0; i < clDynamicWallList[ac].use; i++) {
        for (j = 0; clDynamicWallList[ac].dw[i][j].kind; j++) {
            if (clDynamicWallList[ac].dw[i][j].material == 9 || clDynamicWallList[ac].dw[i][j].material == 11 || clDynamicWallList[ac].dw[i][j].material == 12) {
                continue;
            }
            if (clDynamicWallList[ac].dw[i][j].shape == 0) {
                ret = clCheckSubLineToPlane3(&cres, (float (*)[4])sp, (float (*)[4])ep, &clDynamicWallList[ac].dw[i][j].p[0], &clDynamicWallList[ac].dw[i][j].p[1], &clDynamicWallList[ac].dw[i][j].p[2]);
            } else {
                ret = clCheckSubLineToPlane(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])clDynamicWallList[ac].dw[i][j].p[0], (float (*)[4])clDynamicWallList[ac].dw[i][j].p[1], (float (*)[4])clDynamicWallList[ac].dw[i][j].p[2], (float (*)[4])clDynamicWallList[ac].dw[i][j].p[3]);
            }
            if (ret) {
                clSquareDistance(sp, cres.cp, &dist);
                if (dist < *min) {
                    *min = dist;
                    res->kind = 1;
                    vcopy(cres.cp, res->hobj.wall.cp);
                    clCalcPlaneEquation(&clDynamicWallList[ac].dw[i][j], res->hobj.wall.nl);
                    res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&clDynamicWallList[ac].dw[i][j];
                }
            }
        }
    }
}

static void clCheckHitEyeVectorDynamicFloor(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min) {
    int i;
    int j;
    int ret;
    struct _CL_HITRESULT cres;
    float dist;
    int ac;

    ac = clDynamicFloorListAct ? 0 : 1;
    for (i = 0; i < clDynamicFloorList[ac].use; i++) {
        for (j = 0; clDynamicFloorList[ac].dw[i][j].kind; j++) {
            if (clDynamicFloorList[ac].dw[i][j].material == 9 || clDynamicFloorList[ac].dw[i][j].material == 11 || clDynamicFloorList[ac].dw[i][j].material == 12) {
                continue;
            }
            if (clDynamicFloorList[ac].dw[i][j].shape == 0) {
                ret = clCheckSubLineToPlane3(&cres, (float (*)[4])sp, (float (*)[4])ep, &clDynamicFloorList[ac].dw[i][j].p[0], &clDynamicFloorList[ac].dw[i][j].p[1], &clDynamicFloorList[ac].dw[i][j].p[2]);
            } else {
                ret = clCheckSubLineToPlane(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[0], (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[1], (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[2], (float (*)[4])clDynamicFloorList[ac].dw[i][j].p[3]);
            }
            if (ret) {
                clSquareDistance(sp, cres.cp, &dist);
                if (dist < *min) {
                    *min = dist;
                    res->kind = 1;
                    vcopy(cres.cp, res->hobj.wall.cp);
                    clCalcPlaneEquation(&clDynamicFloorList[ac].dw[i][j], res->hobj.wall.nl);
                    res->hobj.wall.pd = (struct _CL_HITPOLY_HEAD *)&clDynamicFloorList[ac].dw[i][j];
                }
            }
        }
    }
}

static void clCheckHitEyeVectorCharacter(struct _CL_VHIT_RESULT *res, float *sp, float *ep, float *min, unsigned int id) {
    int i;
    int ret;
    struct _CL_HITRESULT cres;
    float dist;
    int ac;

    ac = clCharaListAct ? 0 : 1;
    for (i = 0; i < clCharaListUse[ac]; i++) {
        if (id != (unsigned int)clCharaList[ac][i].sc && clCharaList[ac][i].batflg) {
            ret = clCheckSubLineToColumnPlus(&cres, (float (*)[4])sp, (float (*)[4])ep, (float (*)[4])clCharaList[ac][i].wcol.p[0]);
            if (ret) {
                clSquareDistance(sp, cres.cp, &dist);
                if (dist < *min) {
                    *min = dist;
                    res->kind = 3;
                    vcopy(cres.cp, res->hobj.chara.cp);
                    res->hobj.chara.sc = clCharaList[ac][i].sc;
                }
            }
        }
    }
}

/** Returns whether the current room allows column expansion (clPermColExpFlg). */
int clPermitColumnExpansion(void) {
    return clPermColExpFlg[RoomNameJms()];
}

