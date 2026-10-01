/*
 * ef_smoke.c: the smoke effect. Each smoke object is 16 camera-facing puffs that swell, drift
 * upwards and fade until all of them are gone.
 */
#include "sh2.h"
#include "asm_helpers.h"

#include "math_const.h"

/* random value in [center - range, center + range] (the same macro as in ef_broken_glass.c) */
#define SWAY(center, range) shSway1f((center) - (range), (center) + (range))

struct EFCTSmokeObject smoke_obj[32];

static struct EFCTSmokeObject *CreateSmokeObject(struct EFCTObject *pObj, unsigned char kind);
static void SetSmokeDiffusionParam(struct EFCTSmokePlane *plane);
static void SetSmokeSpeed(struct EFCTSmokePlane *plane);
static unsigned int InitEffectVertexSmoke(struct EFCTVertexData **pVertex);
static void SetSmokeSTValue(struct EFCTObject *pObj);
static struct EFCTSmokeObject *GetSmokeObject(struct EFCTObject *pObj);
static void CountSmokeDiffusionTimer(struct EFCTSmokeObject *smoke);
static void RenewSmokeRGBA(struct EFCTObject *pObj, struct EFCTSmokeObject *smoke);
static void DiffuseSmoke(struct EFCTObject *pObj, struct EFCTSmokeObject *smoke);
static void SetSmokeVertex(float *pos, float width, float height, float *trans, struct EFCTVertexData *VertexData);
static void RenewSmokePos(struct EFCTSmokePlane *smoke);
static int IsFinishSmoke(struct EFCTSmokeObject *smoke);

/**
 * Sets up a smoke object: claims a smoke slot and allocates its 16 puffs.
 * @param pObj   effect object to fill
 * @param nIndex object index
 * @param pos    origin of the smoke
 * @param kind   0 or 1 shift the origin 125 units along +x or -x; other kinds leave it
 * @return 1 on success, 0 if no slot or vertex buffer was free
 */
int InitEffectObjectSmoke(struct EFCTObject *pObj, int nIndex, float *pos, unsigned char kind) {
    pObj->Index = nIndex;
    vcopy_dst_first(pObj->Pos, pos);
    if (kind == 0) {
        pObj->Pos[0] += 125.0f;
    } else if (kind == 1) {
        pObj->Pos[0] -= 125.0f;
    }
    if (CreateSmokeObject(pObj, kind) == NULL) {
        return 0;
    }
    vzero(pObj->trans);
    vzero(pObj->rot);
    pObj->height = pObj->width = 300.0f;
    pObj->VertexNum = InitEffectVertexSmoke(&pObj->pVertex);
    if (pObj->VertexNum == 0) {
        return 0;
    }
    SetSmokeSTValue(pObj);
    return 1;
}

static struct EFCTSmokeObject *CreateSmokeObject(struct EFCTObject *pObj, unsigned char kind) {
    struct EFCTSmokeObject *smoke;
    int i;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 84
    smoke = NULL;
    for (i = 0; i < 32; i++) {
        if (smoke_obj[i].use == 0) {
            smoke_obj[i].use = 1;
            smoke_obj[i].base_obj = pObj;
            smoke = &smoke_obj[i];
            break;
        }
    }
    if (smoke != NULL) {
        smoke->index = i;
        smoke->kind = kind;
        for (i = 0; i < 16; i++) {
            smoke->plane[i].valid = 1;
            SetSmokeDiffusionParam(&smoke->plane[i]);
            smoke->plane[i].timer = 0.0f;
            vzero(smoke->plane[i].trans);
            smoke->plane[i].trans[0] = shSway1f(-300.0f, 300.0f);
            smoke->plane[i].trans[1] = shSway1f(-80.0f, 80.0f);
            /*
             * Matching: trans[2] goes through SWAY() while trans[0] and [1] pass their bounds directly
             * (-300.0f loads the arguments in the other order), the same mix as in ef_broken_glass.c.
             */
            smoke->plane[i].trans[2] = SWAY(0.0f, 300.0f);
            SetSmokeSpeed(&smoke->plane[i]);
        }
    } else {
        assert_dw(0);
    }
    return smoke;
}

static void SetSmokeDiffusionParam(struct EFCTSmokePlane *plane) {
    float rate;

    plane->cycle = 5.95f + shSway1f(1.5f, 1.5f);
    rate = 1.0f + 1.05f * shRandF();
    plane->width_cycle_param = 300.0f * rate;
    rate = 1.0f;
    plane->height_cycle_param = 300.0f * (1.05f + rate * shRandF());
}

static void SetSmokeSpeed(struct EFCTSmokePlane *plane) {
    float rate1;
    float rate2;

    rate1 = -110.0f;
    rate2 = 110.0f;
    plane->speed[0] = shSway1f(rate1, rate2);
    plane->speed[1] = 40.0f + 100.0f * shRandF();
    plane->speed[2] = shSway1f(rate1, rate2);
}

static unsigned int InitEffectVertexSmoke(struct EFCTVertexData **pVertex) {
    int i;

    if (*pVertex != NULL) {
        EfctFree(*pVertex);
        *pVertex = NULL;
    }
    *pVertex = EfctMalloc(sizeof(struct EFCTVertexData) * 64);
    if (*pVertex == NULL) {
        return 0;
    }
    for (i = 0; i < 64; i++) {
        (*pVertex)[i].rgba[0] = 0x70;
        (*pVertex)[i].rgba[1] = 0x68;
        (*pVertex)[i].rgba[2] = 0x68;
        (*pVertex)[i].rgba[3] = 0x14;
        (*pVertex)[i].is_valid = 1;
    }
    return 64;
}

static void SetSmokeSTValue(struct EFCTObject *pObj) {
    int i;
    int s_index;
    int t_index;

    for (i = 0; i < 16; i++) {
        switch (shRandI() % 3) {
        case 0:
            s_index = 2;
            t_index = 7;
            break;
        case 1:
            s_index = 3;
            t_index = 6;
            break;
        case 2:
            s_index = 3;
            t_index = 7;
            break;
        }
        pObj->pVertex[i * 4 + 0].stq[0] = 0.125f * s_index;
        pObj->pVertex[i * 4 + 0].stq[1] = 0.125f * t_index;
        pObj->pVertex[i * 4 + 1].stq[0] = 0.125f * (s_index + 1);
        pObj->pVertex[i * 4 + 1].stq[1] = 0.125f * t_index;
        pObj->pVertex[i * 4 + 2].stq[0] = 0.125f * s_index;
        pObj->pVertex[i * 4 + 2].stq[1] = 0.125f * (t_index + 1);
        pObj->pVertex[i * 4 + 3].stq[0] = 0.125f * (s_index + 1);
        pObj->pVertex[i * 4 + 3].stq[1] = 0.125f * (t_index + 1);
    }
}

/** Sets the texture environment for smoke (tfx 0, transparency 2, CLUT 1). */
void InitSmokeTexEnv(struct EFCTTexEnvInfo *pTexInfo) {
    pTexInfo->tfx = 0;
    pTexInfo->transparency = 2;
    pTexInfo->clut_id = 1;
}

/**
 * Per-frame task of one smoke object: updates its puffs' colour and spread, transforms and
 * clips them; ends the task when every puff has faded.
 * @param task the EFCTTask of this smoke
 */
void DrawSmoke(void *task) {
    struct EFCTTask *pTask;
    struct EFCTSmokeObject *smoke;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 226
    pTask = task;
    if (pTask == NULL) {
        assert_dw(0);
    }
    smoke = GetSmokeObject(pTask->pObj);
    if (smoke == NULL) {



        assert_dw(0);
    }
    RenewSmokeRGBA(pTask->pObj, smoke);
    DiffuseSmoke(pTask->pObj, smoke);
    EFCTTinyThreeDWork(pTask->pObj);
    ClipEffectObject(pTask->pObj);
    DrawPrimitive(pTask->pObj);
    CountSmokeDiffusionTimer(smoke);
    if (IsFinishSmoke(smoke) == 1) {
        smoke->use = 0;
        EFCTCutEffectTask(pTask);
    }
}

static struct EFCTSmokeObject *GetSmokeObject(struct EFCTObject *pObj) {
    int i;

    for (i = 0; i < 32; i++) {
        if (smoke_obj[i].use == 1 && smoke_obj[i].base_obj == pObj) {
            return &smoke_obj[i];
        }
    }
    return NULL;
}

static void CountSmokeDiffusionTimer(struct EFCTSmokeObject *smoke) {
    int i;

    if (smoke != NULL) {
        for (i = 0; i < 16; i++) {
            if (smoke->plane[i].valid == 1) {
                smoke->plane[i].timer += EFCTGetPassingTimePerFrame();
                if (smoke->plane[i].timer > smoke->plane[i].cycle) {
                    smoke->plane[i].timer = 0.0f;
                    smoke->plane[i].valid = 0;
                }
            }
        }
    }
}

static void RenewSmokeRGBA(struct EFCTObject *pObj, struct EFCTSmokeObject *smoke) {
    float ratio;
    int rgba[4];
    int i;

    for (i = 0; i < 16; i++) {
        ratio = shSinF(PI * (0.5f * (1.0f + smoke->plane[i].timer / smoke->plane[i].cycle)));
        /* Matching: converted through unsigned short, as in the original. */
        rgba[0] = (unsigned short)(112.0f * ratio);
        rgba[2] = rgba[1] = (unsigned short)(104.0f * ratio);
        ratio = shSinF(PI * (smoke->plane[i].timer / smoke->plane[i].cycle));
        rgba[3] = (unsigned short)(20.0f * ratio);
        EFCTResetRGBA(rgba, &pObj->pVertex[i * 4]);
    }
}

static void DiffuseSmoke(struct EFCTObject *pObj, struct EFCTSmokeObject *smoke) {
    int i;
    float delta_width;
    float delta_height;
    float ratio;
    float width;
    float height;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 355
    for (i = 0; i < 16; i++) {
        if (smoke->plane[i].timer == 0.0f) {
            SetSmokeDiffusionParam(&smoke->plane[i]);
            smoke->plane[i].trans[0] = shSway1f(-300.0f, 300.0f);
            smoke->plane[i].trans[1] = -80.0f * shRandF();
            smoke->plane[i].trans[2] = shSway1f(-300.0f, 300.0f);
            vzero(smoke->plane[i].pos);
            SetSmokeSpeed(&smoke->plane[i]);
        }
        ratio = shSinF(PI * (0.5f * (smoke->plane[i].timer / smoke->plane[i].cycle)));
        delta_width = smoke->plane[i].width_cycle_param * ratio;
        delta_height = smoke->plane[i].height_cycle_param * ratio;
        if (delta_width < 0.0f || delta_height < 0.0f) {
            assert_dw(0);
        }
        width = pObj->width + delta_width;
        height = pObj->height + delta_height;
        RenewSmokePos(&smoke->plane[i]);
        SetSmokeVertex(smoke->plane[i].pos, width, height, smoke->plane[i].trans, &pObj->pVertex[i * 4]);
    }
}

static void SetSmokeVertex(float *pos, float width, float height, float *trans, struct EFCTVertexData *VertexData) {
    VertexData[0].LocalPos[0] = trans[0] + (pos[0] + 0.5f * -width);
    VertexData[0].LocalPos[1] = trans[1] + (pos[1] + 0.75f * -height);
    VertexData[0].LocalPos[2] = pos[2] + trans[2];
    VertexData[0].LocalPos[3] = 1.0f;
    VertexData[1].LocalPos[0] = trans[0] + (pos[0] + 0.5f * width);
    VertexData[1].LocalPos[1] = trans[1] + (pos[1] + 0.75f * -height);
    VertexData[1].LocalPos[2] = pos[2] + trans[2];
    VertexData[1].LocalPos[3] = 1.0f;
    VertexData[2].LocalPos[0] = trans[0] + (pos[0] + 0.5f * -width);
    VertexData[2].LocalPos[1] = trans[1] + (pos[1] + 0.25f * height);
    VertexData[2].LocalPos[2] = pos[2] + trans[2];
    VertexData[2].LocalPos[3] = 1.0f;
    VertexData[3].LocalPos[0] = trans[0] + (pos[0] + 0.5f * width);
    VertexData[3].LocalPos[1] = trans[1] + (pos[1] + 0.25f * height);
    VertexData[3].LocalPos[2] = pos[2] + trans[2];
    VertexData[3].LocalPos[3] = 1.0f;
}

static void RenewSmokePos(struct EFCTSmokePlane *smoke) {
    smoke->pos[0] = smoke->timer * smoke->speed[0];
    smoke->pos[1] = -1.0f * (smoke->timer * smoke->speed[1]);
    smoke->pos[2] = smoke->timer * smoke->speed[2];
}

/** Does nothing (empty in the original); ef_flame calls it to show or hide a flame's smoke. */
void SetFlameSmokeValid(int index, int valid) {
}

static int IsFinishSmoke(struct EFCTSmokeObject *smoke) {
    int i;
    int ret;

    ret = 1;
    for (i = 0; i < 16; i++) {
        if (smoke->plane[i].valid == 1) {
            ret = 0;
        }
    }
    return ret;
}
