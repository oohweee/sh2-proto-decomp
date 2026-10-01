/*
 * ef_sparks.c: sparks: two small quads that fly off sideways in an arc, fade and respawn with
 * a new random size, direction and colour.
 */
#include "sh2.h"

#include "math_const.h"

static void CountSparksLifeTimer(struct EFCTSparksObject *sparks);
static void RenewSparksRGBA(struct EFCTSparksObject *sparks);
static void MoveSparks(struct EFCTSparksObject *sparks);
static void SetSparksVertex(float *pos, float width, float height, float *trans, struct EFCTVertexData *VertexData);

static void SetSparksSize(float *width, float *height) {
    *width = 20.0f + 5.0f * shRandF();
    *height = 20.0f + 5.0f * shRandF();
}

static void SetSparksKind(int *kind) {
    if (shRandF() > 0.5f) {
        *kind = 0;
    } else {
        *kind = 1;
    }
}

static void SetSparksStartPos(int kind, float *trans) {
    float rate;

    trans[0] = 150.0f * shRandF();
    if (kind == 0) {
        trans[0] = -trans[0];
    }
    trans[1] = 375.0f * (-1.0f * shRandF());
    rate = 50.0f;
    trans[2] = shSway1f(-rate, rate);
}

static void SetSparksSpeed(int kind, float *speed) {
    float min;
    float max;

    speed[0] = 50.0f + 100.0f * shRandF();
    if (kind == 0) {
        speed[0] = -speed[0];
    }
    speed[1] = -625.0f + -150.0f * shRandF();
    min = -100.0f;
    max = 100.0f;
    speed[2] = shSway1f(min, max);
}

static void SetSparksRGBA(int *rgba) {
    float rate;

    rate = 0.3f + 0.7f * shRandF();
    /* Matching: converted through unsigned short, as in the original. */
    rgba[0] = (unsigned short)(128.0f * rate);
    rgba[2] = rgba[1] = (unsigned short)(120.0f * rate);
    rgba[3] = 0x62;
}

static void RenewSparksPos(struct EFCTSparksPlane *sparks) {
    float ratio;

    ratio = shSinF(PI * (sparks->timer / sparks->life_span));
    sparks->pos[0] = sparks->speed[0] * ratio;
    sparks->pos[2] = sparks->speed[2] * ratio;
    ratio = sparks->timer / sparks->life_span;
    sparks->pos[1] = sparks->speed[1] * ratio;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 209
/**
 * Per-frame update of a sparks object: advances its timers, positions and colour, then transforms,
 * clips and draws it.
 * @param sparks the sparks object
 */
void DrawSparks(struct EFCTSparksObject *sparks) {
    if (sparks == NULL) {
        assert(0);
    }
    CountSparksLifeTimer(sparks);
    MoveSparks(sparks);
    RenewSparksRGBA(sparks);
    EFCTThreeDWork(&sparks->base_obj);
    ClipEffectObject2(&sparks->base_obj);
    DrawPrimitive(&sparks->base_obj);
}

static void CountSparksLifeTimer(struct EFCTSparksObject *sparks) {
    int i;

    if (sparks != NULL) {
        for (i = 0; i < 2; i++) {
            sparks->plane[i].timer += EFCTGetPassingTimePerFrame();
            if (sparks->plane[i].timer > sparks->plane[i].life_span) {
                sparks->plane[i].timer = 0.0f;
            }
        }
    }
}

static void RenewSparksRGBA(struct EFCTSparksObject *sparks) {
    float ratio;
    int rgba[4];
    int i;

    for (i = 0; i < 2; i++) {
        ratio = shSinF(PI * (0.5f * (1.0f + sparks->plane[i].timer / sparks->plane[i].life_span)));
        /* Matching: converted through unsigned short, as in the original. */
        rgba[0] = (unsigned short)(sparks->plane[i].rgba[0] * ratio);
        rgba[1] = (unsigned short)(sparks->plane[i].rgba[1] * ratio);
        rgba[2] = (unsigned short)(sparks->plane[i].rgba[2] * ratio);
        ratio = shSinF(PI * (sparks->plane[i].timer / sparks->plane[i].life_span));
        rgba[3] = (unsigned short)(sparks->plane[i].rgba[3] * ratio);
        EFCTResetRGBA(rgba, &sparks->base_obj.pVertex[i * 4]);
    }
}

static void MoveSparks(struct EFCTSparksObject *sparks) {
    int i;
    float width;
    float height;

    for (i = 0; i < 2; i++) {
        if (sparks->plane[i].timer == 0.0f) {
            SetSparksSize(&sparks->plane[i].width, &sparks->plane[i].height);
            SetSparksKind(&sparks->plane[i].kind);
            SetSparksStartPos(sparks->plane[i].kind, sparks->plane[i].trans);
            SetSparksSpeed(sparks->plane[i].kind, sparks->plane[i].speed);
            SetSparksRGBA(sparks->plane[i].rgba);
        }
        width = sparks->plane[i].width;
        height = sparks->plane[i].height;
        RenewSparksPos(&sparks->plane[i]);
        SetSparksVertex(sparks->plane[i].pos, width, height, sparks->plane[i].trans, &sparks->base_obj.pVertex[i * 4]);
    }
}

static void SetSparksVertex(float *pos, float width, float height, float *trans, struct EFCTVertexData *VertexData) {
    VertexData[0].LocalPos[0] = trans[0] + (pos[0] + 0.5f * -width);
    VertexData[0].LocalPos[1] = trans[1] + (pos[1] + 0.5f * -height);
    VertexData[0].LocalPos[2] = pos[2] + trans[2];
    VertexData[0].LocalPos[3] = 1.0f;
    VertexData[1].LocalPos[0] = trans[0] + (pos[0] + 0.5f * width);
    VertexData[1].LocalPos[1] = trans[1] + (pos[1] + 0.5f * -height);
    VertexData[1].LocalPos[2] = pos[2] + trans[2];
    VertexData[1].LocalPos[3] = 1.0f;
    VertexData[2].LocalPos[0] = trans[0] + (pos[0] + 0.5f * -width);
    VertexData[2].LocalPos[1] = trans[1] + (pos[1] + 0.5f * height);
    VertexData[2].LocalPos[2] = pos[2] + trans[2];
    VertexData[2].LocalPos[3] = 1.0f;
    VertexData[3].LocalPos[0] = trans[0] + (pos[0] + 0.5f * width);
    VertexData[3].LocalPos[1] = trans[1] + (pos[1] + 0.5f * height);
    VertexData[3].LocalPos[2] = pos[2] + trans[2];
    VertexData[3].LocalPos[3] = 1.0f;
}
