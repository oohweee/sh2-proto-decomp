/*
 * ef_stage.c: the stage effects: gun fire (muzzle flash) and gun smoke, and blood (spray,
 * drops and pools). Each is a set of textured sprites animated through the EFCT system
 * (ef_common.c).
 *
 * Matching: the `#line` directives after the two constant stand-ins below restore the original
 * line numbering, which the assert strings bake in.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "fi_libvu0_inline.h"

#include "math_const.h"

static void GetGunFireObjectSize(int weapon_kind, float *width, float *height, float *depth);
static unsigned int InitEffectVertexGunFire(float *Pos, float width, float height, float depth, unsigned char *rgba, struct EFCTVertexData **pVertex);
static void SetEffectVertexGunFire(float *Pos, float width, float height, float depth, struct EFCTVertexData *VertexData);
static void GunFireLocalRotV(struct EFCTVertexData *pVertex, float *Direction, float Angle);
static void GetGunSmokeObjectSize(int weapon_kind, float *width, float *height);
static void GetGunFireColor(unsigned char light, unsigned char *rgba);
static void GetGunSmokeColor(unsigned char light, unsigned char *rgba);
static void SetBloodSprayGravityParam(struct EFCTObject *pObj);
static void SetBloodSprayObjectPos(struct EFCTObject *pObj);
static void SetBloodDropObjectPos(struct EFCTObject *pObj);
static void SetBloodPoolObjectPos(struct EFCTObject *pObj);
static void SetGunSmokeObjectPos(struct EFCTObject *pObj);
static void SetBloodSpraySTValue(struct EFCTObject *pObj);
static void SetGunFireSTValue(struct EFCTObject *pObj);
static void SetGunSmokeSTValue(struct EFCTObject *pObj);
static void EFCTDecreaseAlpha(struct EFCTObject *pObj);

/**
 * Sets up a muzzle-flash object: size and colour by weapon and light, animation and vertices.
 * @param pObj         effect object to fill
 * @param nIndex       object (layer) index
 * @param Pos          muzzle position
 * @param vec          firing direction
 * @param wep_kind     weapon kind
 * @param light_status whether James's flashlight is on
 * @return 1 on success, 0 if an allocation failed
 */
int InitEffectObjectGunFire(struct EFCTObject *pObj, int nIndex, float *Pos, float *vec, int wep_kind, unsigned char light_status) {
    float ZAng;
    int i;
    float width;
    float height;
    float depth;
    unsigned char rgba[4];

    pObj->Index = nIndex;
    vcopy_dst_first(pObj->Pos, Pos);
    if (!InitEffectAnimData(1, 0.025f, 0, &pObj->pAnimData)) {
        return 0;
    }
    GetGunFireObjectSize(wep_kind, &width, &height, &depth);
    pObj->width = width;
    pObj->height = height;
    GetGunFireColor(light_status, rgba);
    pObj->VertexNum = InitEffectVertexGunFire(pObj->Pos, width, height, depth, rgba, &pObj->pVertex);
    if (pObj->VertexNum == 0) {
        return 0;
    }
    for (i = 0; i < 12; i++) {
        ZAng = GetSpriteRotAngle(2.0f * PI, 12, i, 30);
        GunFireLocalRotV(&pObj->pVertex[i * 4], vec, ZAng);
    }
    *(u_long128 *)pObj->trans = 0;
    *(u_long128 *)pObj->rot = 0;
    SetGunFireSTValue(pObj);
    return 1;
}

/*
 * Matching: a fitted stand-in for float code (docs/stand-ins.md; found by tools/constcount.py, not
 * recovered). The float constants compiled earlier in the file decide the order in which a
 * call's float-constant arguments load; with it, both shSway1f calls in
 * InitEffectObjectGunSmoke load theirs in the original's order (max first).
 */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f; }
/**
 * Sets up a gun-smoke object: size and colour by weapon and light, animation, vertices and a
 * random offset.
 * @param pObj         effect object to fill
 * @param nIndex       object (layer) index
 * @param Pos          muzzle position
 * @param wep_kind     weapon kind
 * @param light_status whether James's flashlight is on
 * @return 1 on success, 0 if an allocation failed
 */
int InitEffectObjectGunSmoke(struct EFCTObject *pObj, int nIndex, float *Pos, int wep_kind, unsigned char light_status) {
    unsigned char rgba[4];
    float ang;
    float width;
    float height;

    pObj->Index = nIndex;
    vcopy_dst_first(pObj->Pos, Pos);
    if (!InitEffectAnimData(8, 0.1f, 0, &pObj->pAnimData)) {
        return 0;
    }
    GetGunSmokeObjectSize(wep_kind, &width, &height);
    pObj->width = width;
    pObj->height = height;
    *(u_long128 *)pObj->trans = 0;
    ang = 0.1f * width;
    pObj->trans[0] = shSway1f(-ang, ang / 2.0f);
    ang = 0.1f * height;
    pObj->trans[1] = shSway1f(-ang, ang / 2.0f);
    GetGunSmokeColor(light_status, rgba);
    *(u_long128 *)pObj->rot = 0;
    ang = shRandI() % 361 - 180;
    pObj->rot[2] = (PI / 180.0f) * ang;
    pObj->VertexNum = InitEffectVertexSprite(pObj->Pos, pObj->width, pObj->height, 0, &pObj->pVertex, pObj->rot, rgba);
    if (pObj->VertexNum == 0) {
        return 0;
    }
    SetGunSmokeSTValue(pObj);
    return 1;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 272
static void GetGunFireObjectSize(int weapon_kind, float *width, float *height, float *depth) {
    switch (weapon_kind) {
    case 1:
        *width = 96.0f;
        *height = *width;
        *depth = 144.0f;
        break;
    case 2:
        *width = 288.0f;
        *height = *width;
        *depth = 96.0f;
        break;
    case 3:
        *width = 72.0f;
        *height = *width;
        *depth = 180.0f;
        break;
    default:
        assert(0);
    }
}

static unsigned int InitEffectVertexGunFire(float *Pos, float width, float height, float depth, unsigned char *rgba, struct EFCTVertexData **pVertex) {
    int i;

    if (*pVertex != NULL) {
        EfctFree(*pVertex);
        *pVertex = NULL;
    }
    *pVertex = EfctMalloc(sizeof(struct EFCTVertexData) * 48);
    if (*pVertex == NULL) {
        return 0;
    }
    SetEffectVertexGunFire(Pos, width, height, depth, *pVertex);
    for (i = 0; i < 48; i++) {
        (*pVertex)[i].rgba[0] = rgba[0];
        (*pVertex)[i].rgba[1] = rgba[1];
        (*pVertex)[i].rgba[2] = rgba[2];
        (*pVertex)[i].rgba[3] = rgba[3];
        (*pVertex)[i].is_valid = 1;
    }
    return 48;
}

static void SetEffectVertexGunFire(float *Pos, float width, float height, float depth, struct EFCTVertexData *VertexData) {
    int i;

    for (i = 0; i < 12; i++) {
        VertexData[i * 4 + 0].LocalPos[0] = -width / 2.0f;
        VertexData[i * 4 + 0].LocalPos[1] = 0.0f;
        VertexData[i * 4 + 0].LocalPos[2] = 0.0f;
        VertexData[i * 4 + 0].LocalPos[3] = 1.0f;
        VertexData[i * 4 + 1].LocalPos[0] = width / 2.0f;
        VertexData[i * 4 + 1].LocalPos[1] = 0.0f;
        VertexData[i * 4 + 1].LocalPos[2] = 0.0f;
        VertexData[i * 4 + 1].LocalPos[3] = 1.0f;
        VertexData[i * 4 + 2].LocalPos[0] = -width / 2.0f;
        VertexData[i * 4 + 2].LocalPos[1] = height / 2.0f;
        VertexData[i * 4 + 2].LocalPos[2] = depth;
        VertexData[i * 4 + 2].LocalPos[3] = 1.0f;
        VertexData[i * 4 + 3].LocalPos[0] = width / 2.0f;
        VertexData[i * 4 + 3].LocalPos[1] = height / 2.0f;
        VertexData[i * 4 + 3].LocalPos[2] = depth;
        VertexData[i * 4 + 3].LocalPos[3] = 1.0f;
    }
}

static void GunFireLocalRotV(struct EFCTVertexData *pVertex, float *Direction, float Angle) {
    float mtx[4][4];
    float rot[4];
    int i;

    vwVectorToAngle(rot, Direction);
    rot[2] = Angle;
    _sceVu0UnitMatrix(mtx);
    shRotMatrixZ(mtx, mtx, rot[2]);
    shRotMatrixX(mtx, mtx, rot[0]);
    shRotMatrixY(mtx, mtx, rot[1]);
    for (i = 0; i < 4; i++) {
        _sceVu0ApplyMatrix(pVertex[i].LocalPos, mtx, pVertex[i].LocalPos);
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 418
static void GetGunSmokeObjectSize(int weapon_kind, float *width, float *height) {
    switch (weapon_kind) {
    case 1:
        *width = 130.0f;
        *height = *width;
        break;
    case 2:
        *width = 180.0f;
        *height = *width;
        break;
    case 3:
        *width = 160.0f;
        *height = *width;
        break;
    default:
        assert(0);
    }
}

static void GetGunFireColor(unsigned char light, unsigned char *rgba) {
    if (BgIsOut(0) || light) {
        rgba[0] = 0x28;
        rgba[1] = 0x28;
        rgba[2] = 0x28;
    } else {
        rgba[0] = 0x24;
        rgba[1] = 0x20;
        rgba[2] = 0x20;
    }
    rgba[3] = 0x26;
}

static void GetGunSmokeColor(unsigned char light, unsigned char *rgba) {
    if (BgIsOut(0) || light) {
        rgba[0] = 0x2A;
        rgba[1] = 0x2A;
        rgba[2] = 0x2A;
    } else {
        rgba[0] = 0x1A;
        rgba[1] = 0x1A;
        rgba[2] = 0x1F;
    }
    rgba[3] = 0x60;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 476
/**
 * Per-frame task of a blood-spray object: animates, moves, clips and draws it; ends with its
 * animation.
 */
void DrawBloodSpray(void *task) {
    struct EFCTTask *pTask;
    int IsNext;

    pTask = task;
    if (pTask == NULL) {
        assert(0);
    }
    IsNext = EFCTNextFrame(pTask->pObj, 0);
    if (IsNext == 1) {
        SetBloodSprayObjectPos(pTask->pObj);
    }
    SetBloodSprayGravityParam(pTask->pObj);
    EFCTTinyThreeDWork(pTask->pObj);
    ClipEffectObject(pTask->pObj);
    SetBloodSpraySTValue(pTask->pObj);
    DrawPrimitive(pTask->pObj);
    if (pTask->pObj->pAnimData->Status == 2) {
        EFCTCutEffectTask(pTask);
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 548
/**
 * Per-frame task of a blood-drop object: animates, moves, clips and draws it; ends with its
 * animation.
 */
void DrawBloodDrop(void *task) {
    struct EFCTTask *pTask;
    int IsNext;

    pTask = task;
    if (pTask == NULL) {
        assert(0);
    }
    IsNext = EFCTNextFrame(pTask->pObj, 0);
    if (IsNext == 1) {
        SetBloodDropObjectPos(pTask->pObj);
    }
    EFCTThreeDWork(pTask->pObj);
    ClipEffectObject(pTask->pObj);
    DrawPrimitive(pTask->pObj);
    if (shCharacterGetSubCharacter(pTask->pObj->chara_kind, pTask->pObj->chara_id) == NULL) {
        EFCTCutEffectTask(pTask);
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 598
/** Per-frame task of a blood-pool object: animates, clips and draws it; ends with its animation. */
void DrawBloodPool(void *task) {
    struct EFCTTask *pTask;
    int IsNext;

    pTask = task;
    if (pTask == NULL) {
        assert(0);
    }
    IsNext = EFCTNextFrame(pTask->pObj, 0);
    if (IsNext == 1) {
        SetBloodPoolObjectPos(pTask->pObj);
    }
    EFCTThreeDWork(pTask->pObj);
    ClipEffectObject(pTask->pObj);
    DrawPrimitive(pTask->pObj);
    if (shCharacterGetSubCharacter(pTask->pObj->chara_kind, pTask->pObj->chara_id) == NULL) {
        EFCTCutEffectTask(pTask);
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 642
/**
 * Per-frame task of a muzzle-flash object: animates, transforms, clips and draws it; ends with its
 * animation.
 */
void DrawGunFire(void *task) {
    struct EFCTTask *pTask;
    int IsNext;

    pTask = task;
    if (pTask == NULL) {
        assert(0);
    }
    IsNext = EFCTNextFrame(pTask->pObj, 0);
    EFCTThreeDWork(pTask->pObj);
    ClipEffectObject(pTask->pObj);
    DrawPrimitive(pTask->pObj);
    if (pTask->pObj->pAnimData->Status == 2) {
        EFCTCutEffectTask(pTask);
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 694
/**
 * Per-frame task of a gun-smoke object: animates, moves, fades, clips and draws it; ends with its
 * animation.
 */
void DrawGunSmoke(void *task) {
    struct EFCTTask *pTask;
    int IsNext;

    pTask = task;
    if (pTask == NULL) {
        assert(0);
    }
    IsNext = EFCTNextFrame(pTask->pObj, 0);
    SetGunSmokeObjectPos(pTask->pObj);
    EFCTTinyThreeDWork(pTask->pObj);
    EFCTDecreaseAlpha(pTask->pObj);
    ClipEffectObject(pTask->pObj);
    DrawPrimitive(pTask->pObj);
    if (pTask->pObj->pAnimData->Status == 2) {
        EFCTCutEffectTask(pTask);
    }
}

static void SetBloodSprayGravityParam(struct EFCTObject *pObj) {
    float t;

    t = (pObj->pAnimData->CurrentFrameNo + 1) * EFCTGetPassingTimePerFrame();
    pObj->trans[1] += 35.0f * t * t;
}

/*
 * Matching: a second fitted stand-in (see above). With it, the shSway1f calls after this point
 * load their arguments max first (f13, then f12) where the original does.
 */
static float __stripped_float_code_2(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f; }
static void SetBloodSprayObjectPos(struct EFCTObject *pObj) {
    if (pObj->pAnimData->CurrentFrameNo == pObj->pAnimData->StartFrameNo) {
        switch (pObj->Index) {
        case 0:
            pObj->trans[0] = 0.0f;
            pObj->trans[1] = 0.0f;
            break;
        case 1:
            pObj->trans[0] = shSway1f(-20.0f, 20.0f);
            pObj->trans[1] = shSway1f(20.0f, 20.0f);
            break;
        case 2:
            pObj->trans[0] = shSway1f(20.0f, 20.0f);
            pObj->trans[1] = shSway1f(20.0f, 20.0f);
            break;
        case 3:
            pObj->trans[0] = shSway1f(-10.0f, 10.0f);
            pObj->trans[1] = shSway1f(20.0f, 20.0f);
            break;
        case 4:
            pObj->trans[0] = shSway1f(10.0f, 10.0f);
            pObj->trans[1] = shSway1f(20.0f, 20.0f);
            break;
        }
    }
}

static void SetBloodDropObjectPos(struct EFCTObject *pObj) {
    if (pObj->pAnimData->CurrentFrameNo == pObj->pAnimData->StartFrameNo) {
        pObj->width = (32.0f + 32.0f * shRandF()) - 16.0f;
        pObj->height = pObj->width;
    } else {
        pObj->width *= 33.8f * EFCTGetPassingTimePerFrame();
        pObj->height *= 33.8f * EFCTGetPassingTimePerFrame();
    }
    if (pObj->pAnimData->CurrentFrameNo == pObj->pAnimData->StartFrameNo) {
        pObj->trans[0] = shSway1f(-96.0f, 96.0f);
        pObj->trans[2] = shSway1f(-96.0f, 96.0f);
    }
    SetEffectVertexSpriteXZ(pObj->width, pObj->height, pObj->pVertex);
    SpriteLocalRot(pObj->pVertex, pObj->VertexNum, pObj->rot);
}

static void SetBloodPoolObjectPos(struct EFCTObject *pObj) {
    if (pObj->pAnimData->CurrentFrameNo == pObj->pAnimData->StartFrameNo) {
        pObj->width = 150.0f;
        pObj->height = pObj->width;
    } else {
        pObj->width *= 30.88f * EFCTGetPassingTimePerFrame();
        pObj->height *= 30.88f * EFCTGetPassingTimePerFrame();
    }
    if (pObj->pAnimData->CurrentFrameNo == pObj->pAnimData->StartFrameNo) {
        if (pObj->Index == 0) {
            pObj->trans[0] = -1.0f * shSway1f(112.5f, 112.5f);
            pObj->trans[2] = -1.0f * shSway1f(112.5f, 112.5f);
        } else if (pObj->Index == 1) {
            pObj->trans[0] = shSway1f(112.5f, 112.5f);
            pObj->trans[2] = -1.0f * shSway1f(112.5f, 112.5f);
        } else if (pObj->Index == 2) {
            pObj->trans[0] = -1.0f * shSway1f(112.5f, 112.5f);
            pObj->trans[2] = shSway1f(112.5f, 112.5f);
        } else if (pObj->Index == 3) {
            pObj->trans[0] = shSway1f(112.5f, 112.5f);
            pObj->trans[2] = shSway1f(112.5f, 112.5f);
        }
    }
    SetEffectVertexSpriteXZ(pObj->width, pObj->height, pObj->pVertex);
    SpriteLocalRot(pObj->pVertex, pObj->VertexNum, pObj->rot);
}

static void SetGunSmokeObjectPos(struct EFCTObject *pObj) {
    float ratio;

    ratio = EFCTGetPassingTimePerFrame() / 0.8f;
    pObj->width += 130.0f * ratio;
    pObj->height += 130.0f * ratio;
    SetEffectVertexSpriteXY(pObj->width, pObj->height, pObj->pVertex);
    SpriteLocalRot(pObj->pVertex, pObj->VertexNum, pObj->rot);
}

static void SetBloodSpraySTValue(struct EFCTObject *pObj) {
    unsigned int FrameNumber;
    int TopLeftS;
    int i;

    FrameNumber = pObj->pAnimData->CurrentFrameNo;
    if (pObj->pAnimData->Status != 1) {
        for (i = 0; i < pObj->VertexNum; i++) {
            pObj->pVertex[i].stq[0] = 0.0f;
            pObj->pVertex[i].stq[1] = 0.0f;
        }
    } else {
        switch (pObj->Index) {
        case 0:
            TopLeftS = 6;
            break;
        case 1:
            TopLeftS = 3;
            break;
        case 2:
            TopLeftS = 4;
            break;
        case 3:
            TopLeftS = 2;
            break;
        case 4:
            TopLeftS = 5;
            break;
        }
        pObj->pVertex[0].stq[0] = 0.125f * TopLeftS;
        pObj->pVertex[0].stq[1] = 0.125f * FrameNumber;
        pObj->pVertex[1].stq[0] = 0.125f * (TopLeftS + 1);
        pObj->pVertex[1].stq[1] = 0.125f * FrameNumber;
        pObj->pVertex[2].stq[0] = 0.125f * TopLeftS;
        pObj->pVertex[2].stq[1] = 0.125f * (FrameNumber + 1);
        pObj->pVertex[3].stq[0] = 0.125f * (TopLeftS + 1);
        pObj->pVertex[3].stq[1] = 0.125f * (FrameNumber + 1);
    }
}

static void SetGunFireSTValue(struct EFCTObject *pObj) {
    int i;

    for (i = 0; i < 12; i++) {
        pObj->pVertex[i * 4 + 0].stq[0] = 0.25f;
        pObj->pVertex[i * 4 + 0].stq[1] = 0.875f;
        pObj->pVertex[i * 4 + 1].stq[0] = 0.375f;
        pObj->pVertex[i * 4 + 1].stq[1] = 0.875f;
        pObj->pVertex[i * 4 + 2].stq[0] = 0.25f;
        pObj->pVertex[i * 4 + 2].stq[1] = 0.75f;
        pObj->pVertex[i * 4 + 3].stq[0] = 0.375f;
        pObj->pVertex[i * 4 + 3].stq[1] = 0.75f;
    }
}

static void SetGunSmokeSTValue(struct EFCTObject *pObj) {
    int S;
    int T;

    switch (shRandI() % 3) {
    case 0:
        S = 2;
        T = 7;
        break;
    case 1:
        S = 3;
        T = 6;
        break;
    case 2:
        S = 3;
        T = 7;
        break;
    }
    pObj->pVertex[0].stq[0] = 0.125f * S;
    pObj->pVertex[0].stq[1] = 0.125f * T;
    pObj->pVertex[1].stq[0] = 0.125f * (S + 1);
    pObj->pVertex[1].stq[1] = 0.125f * T;
    pObj->pVertex[2].stq[0] = 0.125f * S;
    pObj->pVertex[2].stq[1] = 0.125f * (T + 1);
    pObj->pVertex[3].stq[0] = 0.125f * (S + 1);
    pObj->pVertex[3].stq[1] = 0.125f * (T + 1);
}

/**
 * Returns how many objects (layers) a stage effect is made of.
 * @param EffectKind 1-3 blood spray/drop/pool, 4 gun fire, 5 gun smoke
 */
unsigned short GetStageEffectLayerNum(short EffectKind) {
    unsigned short Ret;

    switch (EffectKind) {
    case 1:
        Ret = 5;
        break;
    case 2:
        Ret = 3;
        break;
    case 3:
        Ret = 3;
        break;
    case 4:
        Ret = 1;
        break;
    case 5:
        Ret = 3;
        break;
    }
    return Ret;
}

/** Sets the texture environment for blood (tfx 0, transparency 0, CLUT 2). */
void InitBloodTexEnv(struct EFCTTexEnvInfo *pTexInfo) {
    pTexInfo->transparency = 0;
    pTexInfo->tfx = 0;
    pTexInfo->clut_id = 2;
}

/** Sets the texture environment for gun fire (tfx 0, transparency 1, CLUT 0). */
void InitGunFireTexEnv(struct EFCTTexEnvInfo *pTexInfo) {
    pTexInfo->tfx = 0;
    pTexInfo->transparency = 1;
    pTexInfo->clut_id = 0;
}

/** Sets the texture environment for gun smoke (tfx 0, transparency 1, CLUT 0). */
void InitGunSmokeTexEnv(struct EFCTTexEnvInfo *pTexInfo) {
    pTexInfo->tfx = 0;
    pTexInfo->transparency = 1;
    pTexInfo->clut_id = 0;
}

static void EFCTDecreaseAlpha(struct EFCTObject *pObj) {
    int i;
    float val;

    if (pObj->pVertex[0].rgba[3] <= 0) {
        val = 0.0f;
    } else if (pObj->pVertex[0].rgba[3] <= 9) {
        val = 30.0f * EFCTGetPassingTimePerFrame();
    } else if (pObj->pVertex[0].rgba[3] <= 31) {
        val = 60.0f * EFCTGetPassingTimePerFrame();
    } else if (pObj->pVertex[0].rgba[3] <= 47) {
        val = 120.0f * EFCTGetPassingTimePerFrame();
    } else {
        val = 150.0f * EFCTGetPassingTimePerFrame();
    }
    for (i = 0; i < pObj->VertexNum; i++) {
        /* Matching: converted through int; a direct float-to-unsigned-char cast compiles differently. */
        pObj->pVertex[i].rgba[3] -= (unsigned char)(int)val;
    }
}
