/*
 * ef_flame.c: the flame effect. Each flame is a camera-facing object of up to 16 textured planes
 * whose size, colour and texture frame cycle over time. Which flames show depends on the current
 * camera cut of the demo they belong to; the flames behind Angela are Z-sorted among themselves.
 *
 * Matching: each `#line` puts the assert after it on its line in the original file (the asserts
 * bake "<file>:<line>" into their strings).
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "fi_libvu0_inline.h"
#include "sdk/libvu0.h"

/* Matching: #line keeps the original line numbers (declarations moved to headers). */
#line 18
#include "math_const.h"

static unsigned short camera_cut = 0;
static unsigned short behind_angela_flame_num = 0;
static unsigned short behind_angela_flame_start_index = 0;
struct EFCTFlameObject flame_obj[36];
float behind_angela_flame_pos_buf[12][4];
float behind_angela_flame_life_time_buf[12];

static void SetFlameCycleData(unsigned char kind, struct EFCTFlamePlane *plane);
static void SetFlameStartPos(unsigned char kind, float width, float height, float *start_pos);
static void SetFlameVertex(unsigned char kind, float width, float height, float *trans, float *start_pos, float cycle, float w_right_ratio, struct EFCTVertexData *VertexData);
static void FlameLocalRot(struct EFCTVertexData *pVertex, float *rot);
static void SetFlameTexKind(unsigned char kind, struct EFCTFlamePlane *plane);
static void SetFlameTransParam(unsigned char kind, float width, float height, float *trans);
static void SetFlameRotParam(unsigned char kind, float *rot);
static struct EFCTFlameObject *GetFlameObject(struct EFCTObject *pObj);
static void RenewFlameSize(struct EFCTObject *pObj, struct EFCTFlameObject *flame);
static void EnlargeFlameSize(float time, float w_speed, float h_speed, float w_enlarge_time, float h_enlarge_time, float *width, float *height);
static void RenewFlameRGBA(float cycle, int alpha, struct EFCTVertexData *pVertex);
static void EFCTTinyThreeDWorkFlame(unsigned int plane_num, struct EFCTObject *pObj);
static void SetFlameSTValue(struct EFCTObject *pObj, struct EFCTFlameObject *flame);
static void NextFrame(struct EFCTObject *pObj, float *wait_time, short *current, float *drawing_time);
static void CountFlameCycleTimer(struct EFCTFlameObject *flame);
static void ClipFlameEffectObject(struct EFCTVertexData *pVertex);
static void CalibrationZVal(int num, struct EFCTVertexData *pVertex);
static void TinyFlameZSort(unsigned int index, int num, struct EFCTVertexData *pVertex);
static int GetBehindAngelaFlameIndex(unsigned int index);
static int GetBehindAngelaFlameSortIndex(unsigned int index);
static void TinyFlameZSort2(unsigned int index, float *new_pos);
static float GetBehindAngelaFlameLifeTimer(unsigned int index);
static int IsFlameValid(struct EFCTObject *pObj, struct EFCTFlameObject *flame);

/*
 * Matching: a dead-stripped function; nothing in this build creates flames. Only the "0" of its
 * assert survives in .rodata (DrawFlame shares it). Name unknown; the body is a guess (a flame allocator).
 */
struct EFCTFlameObject *__stripped_ef_flame_code(struct EFCTObject *pObj, unsigned char kind) {
    int i;

    for (i = 0; i < 36; i++) {
        if (flame_obj[i].use == 0) {
            flame_obj[i].use = 1;
            flame_obj[i].kind = kind;
            flame_obj[i].base_obj = pObj;
            return &flame_obj[i];
        }
    }
    assert_dw(0);
    return NULL;
}

static void SetFlameCycleData(unsigned char kind, struct EFCTFlamePlane *plane) {
    float rate;
    float width;
    float height;
    float cycle_max;
    float cycle_min;
    float w_cycle_param_max;
    float w_cycle_param_min;
    float h_cycle_param_max;
    float h_cycle_param_min;

    if (kind == 0 || kind == 1) {
        width = 160.0f;
        height = 200.0f;
        cycle_max = 1.25f;
        cycle_min = 0.85f;
        w_cycle_param_max = 4.25f;
        w_cycle_param_min = 2.4f;
        h_cycle_param_max = 3.85f;
        h_cycle_param_min = 1.85f;
        plane->w_right_ratio = 0.5f;
    } else if (kind == 3) {
        width = 150.0f;
        height = 50.0f;
        cycle_max = 2.35f;
        cycle_min = 0.75f;
        w_cycle_param_max = 7.25f;
        w_cycle_param_min = 0.7f;
        h_cycle_param_max = 15.05f;
        h_cycle_param_min = 0.75f;
        plane->w_right_ratio = 0.5f;
    } else {
        width = 130.0f;
        height = 100.0f;
        cycle_max = 2.35f;
        cycle_min = 0.75f;
        w_cycle_param_max = 3.25f;
        w_cycle_param_min = 0.7f;
        h_cycle_param_max = 4.05f;
        h_cycle_param_min = 0.75f;
        plane->w_right_ratio = 0.5f;
    }
    rate = cycle_max - cycle_min;
    plane->cycle = cycle_min + rate * shRandF();
    plane->width_cycle_param = width * (w_cycle_param_min + (w_cycle_param_max - w_cycle_param_min) * shRandF());
    plane->height_cycle_param = height * (h_cycle_param_min + (h_cycle_param_max - h_cycle_param_min) * shRandF());
}

static void SetFlameStartPos(unsigned char kind, float width, float height, float *start_pos) {
    if (kind == 0 || kind == 1) {
        if (camera_cut == 5) {
            start_pos[0] = shSway1f(10.0f * -width, 10.0f * width);
        } else if (camera_cut == 0) {
            start_pos[0] = 20.0f * (width * -shRandF());
        } else if (camera_cut == 3) {
            start_pos[0] = shSway1f(10.0f * -width, 10.0f * width);
        }
    } else {
        start_pos[0] = shSway1f(0.2f * -width, 0.2f * width);
    }
}

static void SetFlameVertex(unsigned char kind, float width, float height, float *trans, float *start_pos, float cycle, float w_right_ratio, struct EFCTVertexData *VertexData) {
    float adjustment[4][4];
    float ratio;

    vzero(adjustment[0]);
    vzero(adjustment[1]);
    vzero(adjustment[2]);
    vzero(adjustment[3]);
    if (kind == 0 || kind == 1) {
        if (camera_cut == 5) {
            adjustment[2][0] = 0.15f * width;
            adjustment[3][0] = 0.15f * -width;
        } else if (camera_cut == 1 || camera_cut == 2 || camera_cut == 4) {
            if (kind == 0) {
                width *= 0.8f;
            }
            ratio = shSinF(PI * cycle);
            adjustment[0][0] = 0.4f * -width * ratio;
            adjustment[0][1] = 0.5f * -height * ratio;
            ratio = shSinF(PI * (0.5f + cycle));
            adjustment[2][0] = 0.25f * -width * ratio;
            adjustment[3][0] = 0.35f * -width * ratio;
        } else if (camera_cut == 3) {
            height *= 0.5f;
            width *= 0.5f;
        } else {
            width *= 0.55f;
            adjustment[0][0] = 0.4f * -width;
            adjustment[1][0] = 0.4f * width;
        }
    } else {
        adjustment[0][0] = trans[0] * cycle - trans[0];
        adjustment[1][0] = trans[0] * cycle - trans[0];
        adjustment[2][0] = -trans[0];
        adjustment[3][0] = -trans[0];
    }
    VertexData[0].LocalPos[0] = adjustment[0][0] + (start_pos[0] + (trans[0] + -width * (1.0f - w_right_ratio)));
    VertexData[0].LocalPos[1] = adjustment[0][1] + (start_pos[1] + (-height + trans[1]));
    VertexData[0].LocalPos[2] = adjustment[0][2] + (trans[2] + start_pos[2]);
    VertexData[0].LocalPos[3] = 1.0f;
    VertexData[1].LocalPos[0] = adjustment[1][0] + (start_pos[0] + (trans[0] + width * w_right_ratio));
    VertexData[1].LocalPos[1] = adjustment[1][1] + (start_pos[1] + (-height + trans[1]));
    VertexData[1].LocalPos[2] = adjustment[1][2] + (trans[2] + start_pos[2]);
    VertexData[1].LocalPos[3] = 1.0f;
    VertexData[2].LocalPos[0] = adjustment[2][0] + (start_pos[0] + (trans[0] + -width * (1.0f - w_right_ratio)));
    VertexData[2].LocalPos[1] = adjustment[2][1] + (start_pos[1] + (0.15 * height + trans[1]));
    VertexData[2].LocalPos[2] = adjustment[2][2] + (trans[2] + start_pos[2]);
    VertexData[2].LocalPos[3] = 1.0f;
    VertexData[3].LocalPos[0] = adjustment[3][0] + (start_pos[0] + (trans[0] + width * w_right_ratio));
    VertexData[3].LocalPos[1] = adjustment[3][1] + (start_pos[1] + (0.15 * height + trans[1]));
    VertexData[3].LocalPos[2] = adjustment[3][2] + (trans[2] + start_pos[2]);
    VertexData[3].LocalPos[3] = 1.0f;
}

static void FlameLocalRot(struct EFCTVertexData *pVertex, float *rot) {
    float mtx[4][4];
    int i;

    _sceVu0UnitMatrix(mtx);
    shRotMatrixZ(mtx, mtx, rot[2]);
    shRotMatrixY(mtx, mtx, rot[1]);
    shRotMatrixX(mtx, mtx, rot[0]);
    for (i = 0; i < 4; i++) {
        _sceVu0ApplyMatrix(pVertex[i].LocalPos, mtx, pVertex[i].LocalPos);
    }
}

static void SetFlameTexKind(unsigned char kind, struct EFCTFlamePlane *plane) {
    int i;

    if (kind == 0 || kind == 1) {
        do {
            i = shRandI() % 5;
        } while (i == 1);
        plane->s_index = i + 2;
    } else {
        plane->s_index = shRandI() % 4 + 4;
    }
}

static void SetFlameTransParam(unsigned char kind, float width, float height, float *trans) {
    if (kind == 0 || kind == 1) {
        if (camera_cut == 5) {
            trans[0] = shSway1f(15.5f * -width, 15.5f * width);
        } else if (camera_cut == 0) {
            trans[0] = 30.5f * (-width * shRandF());
        } else if (camera_cut == 4) {
            trans[0] = 30.5f * (-width * shRandF());
        } else if (camera_cut == 3) {
            trans[0] = shSway1f(15.5f * -width, 15.5f * width);
        }
        trans[1] = -(100.5f * height) * shRandF();
        trans[2] = shSway1f(-25.0f, 25.0f);
    } else {
        trans[0] = shSway1f(0.4f * -width, 0.4f * width);
    }
}

static void SetFlameRotParam(unsigned char kind, float *rot) {
    vzero(rot);
    if (camera_cut == 4) {
        if (kind == 0) {
            rot[0] = 0.2617994f;
            rot[1] = -0.2617994f;
        } else if (kind == 1) {
            rot[0] = 0.5235988f;
            rot[1] = -0.5235988f;
        }
    } else if (camera_cut == 1 || camera_cut == 2) {
        rot[1] = 0.2617994f;
    } else if (camera_cut == 3) {
        if (kind == 1) {
            rot[0] = 1.0471976f;
            rot[1] = -0.5235988f;
        }
    }
    if (kind == 1) {
        rot[1] += PI;
    }
}

/**
 * Per-frame task of one flame: checks it against the demo's camera cut, advances its planes'
 * animation, size and colour, and transforms and clips it.
 * @param task the EFCTTask of this flame
 */
void DrawFlame(void *task) {
    struct EFCTTask *pTask;
    struct EFCTFlameObject *flame;
    int i;

    pTask = task;
    if (pTask == NULL) {
#line 705
        assert_dw(0);
    }
    flame = GetFlameObject(pTask->pObj);
    if (flame == NULL) {
#line 712
        assert_dw(0);
    }
    if (flame->index == 0 && (demo_status & 1)) {
        EFCTFlameChangeCamera();
    }
    if (!IsFlameValid(pTask->pObj, flame)) {
        SetFlameSmokeValid(flame->index, 0);
        return;
    }
    SetFlameSmokeValid(flame->index, 1);
    for (i = 0; i < flame->plane_num; i++) {
        NextFrame(pTask->pObj, &flame->plane[i].draw_wait, &flame->plane[i].current_frame, &flame->plane[i].drawing_time);
    }
    CountFlameCycleTimer(flame);
    if (flame->kind == 3 && camera_cut == 6) {
        behind_angela_flame_life_time_buf[GetBehindAngelaFlameIndex(flame->index)] = flame->life_timer += EFCTGetPassingTimePerFrame();
    }
    RenewFlameSize(pTask->pObj, flame);
    if (flame->kind == 3) {
        TinyFlameZSort2(flame->index, pTask->pObj->Pos);
    }
    if ((camera_cut == 4 && flame->kind == 0) || flame->kind == 3 || flame->kind == 2) {
        EFCTThreeDWork(pTask->pObj);
        for (i = 0; i < flame->plane_num; i++) {
            ClipFlameEffectObject(&pTask->pObj->pVertex[i * 4]);
        }
    } else {
        EFCTTinyThreeDWorkFlame(flame->plane_num, pTask->pObj);
    }
    if (flame->kind == 0 || flame->kind == 1) {
        CalibrationZVal(pTask->pObj->VertexNum, pTask->pObj->pVertex);
    }
    if (flame->kind == 3) {
        TinyFlameZSort(flame->index, pTask->pObj->VertexNum, pTask->pObj->pVertex);
    }
    SetFlameSTValue(pTask->pObj, flame);
    DrawPrimitive(pTask->pObj);
    DrawSparks(&flame->sparks);
}

static struct EFCTFlameObject *GetFlameObject(struct EFCTObject *pObj) {
    int i;

    for (i = 0; i < 36; i++) {
        if (flame_obj[i].use == 1 && flame_obj[i].base_obj == pObj) {
            return &flame_obj[i];
        }
    }
    return NULL;
}

static void RenewFlameSize(struct EFCTObject *pObj, struct EFCTFlameObject *flame) {
    int i;
    float life_timer;
    float delta_width;
    float delta_height;
    float cycle;
    float ratio;
    float ratio2;
    float width;
    float height;
    float trans[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    for (i = 0; i < flame->plane_num; i++) {
        cycle = flame->plane[i].timer / flame->plane[i].cycle;
        ratio = shSinF(PI * cycle);
        width = pObj->width;
        height = pObj->height;
        if (flame->kind == 3) {
            life_timer = GetBehindAngelaFlameLifeTimer(flame->index);
        } else {
            life_timer = flame->life_timer;
        }
        EnlargeFlameSize(life_timer, flame->plane[i].w_enlarge_speed, flame->plane[i].h_enlarge_speed, flame->plane[i].w_enlarge_time, flame->plane[i].h_enlarge_time, &width, &height);
        if (flame->plane[i].timer == 0.0f) {
            SetFlameCycleData(flame->kind, &flame->plane[i]);
            SetFlameStartPos(flame->kind, width, height, flame->plane[i].start_pos);
            SetFlameTransParam(flame->kind, width, height, flame->plane[i].trans);
            SetFlameTexKind(flame->kind, &flame->plane[i]);
            if (flame->kind == 0 || flame->kind == 1) {
                SetFlameRotParam(flame->kind, flame->plane[i].rot);
            }
        }
        if (flame->kind == 0 || flame->kind == 1) {
            trans[0] = flame->plane[i].trans[0] * ratio;
            trans[1] = flame->plane[i].trans[1] * ratio;
            trans[2] = flame->plane[i].trans[2] * ratio;
        } else {
            trans[0] = flame->plane[i].trans[0] * ratio;
            trans[1] = flame->plane[i].trans[1] * ratio;
            ratio2 = shSinF(PI * (4.0f * flame->plane[i].timer / flame->plane[i].cycle));
            trans[2] = flame->plane[i].trans[2] * fabsf(ratio2);
        }
        delta_width = flame->plane[i].width_cycle_param * ratio;
        delta_height = flame->plane[i].height_cycle_param * ratio;
        width += delta_width;
        height += delta_height;
        SetFlameVertex(flame->kind, width, height, trans, flame->plane[i].start_pos, cycle, flame->plane[i].w_right_ratio, &pObj->pVertex[i * 4]);
        FlameLocalRot(&pObj->pVertex[i * 4], flame->plane[i].rot);
        if (flame->kind == 3) {
            RenewFlameRGBA(cycle, 0x80, &pObj->pVertex[i * 4]);
        } else {
            RenewFlameRGBA(cycle, 100, &pObj->pVertex[i * 4]);
        }
    }
}

static void EnlargeFlameSize(float time, float w_speed, float h_speed, float w_enlarge_time, float h_enlarge_time, float *width, float *height) {
    if (time <= w_enlarge_time) {
        *width *= 1.0f + (w_speed - 1.0f) * time;
    } else {
        *width *= 1.0f + (w_speed - 1.0f) * w_enlarge_time;
    }
    if (time <= h_enlarge_time) {
        *height *= 1.0f + (h_speed - 1.0f) * time;
    } else {
        *height *= 1.0f + (h_speed - 1.0f) * h_enlarge_time;
    }
}

static void RenewFlameRGBA(float cycle, int alpha, struct EFCTVertexData *pVertex) {
    float ratio;
    int rgba[4];

    ratio = shSinF(PI / 2.0f);
    /* Matching: converted through unsigned short, as in the original. */
    rgba[0] = (unsigned short)(128.0f * ratio);
    rgba[2] = rgba[1] = (unsigned short)(108.0f * ratio);
    ratio = shSinF(PI * (2.0f * cycle));
    rgba[3] = (unsigned short)(alpha * ratio);
    EFCTResetRGBA(rgba, pVertex);
}

static void EFCTTinyThreeDWorkFlame(unsigned int plane_num, struct EFCTObject *pObj) {
    float lw_mtx[4][4];
    float ls_mtx[4][4];
    float wv_mtx[4][4];
    float ws_mtx[4][4];
    int i;
    float q;
    float lcm[4][4];
    float vcm[4][4];
    float work[4][4];
    int j;
    int is_valid;

    sceVu0CopyMatrix(wv_mtx, VbWvsMatrix.wvm);
    wv_mtx[3][0] = wv_mtx[3][1] = wv_mtx[3][2] = 0.0f;
    wv_mtx[3][3] = 1.0f;
    wv_mtx[0][1] = wv_mtx[1][0] = 0.0f;
    wv_mtx[1][1] = 1.0f;
    wv_mtx[1][2] = wv_mtx[2][1] = 0.0f;
    sceVu0InversMatrix(lw_mtx, wv_mtx);
    _sceVu0TransMatrix(lw_mtx, lw_mtx, pObj->Pos);
    _sceVu0TransMatrix(lw_mtx, lw_mtx, pObj->trans);
    sceVu0CopyMatrix(ws_mtx, cam0.world_screen);
    shMulMatrix(ls_mtx, ws_mtx, lw_mtx);
    sceVu0CopyMatrix(vcm, cam0.view_clip);
    shMulMatrix(work, VbWvsMatrix.wvm, lw_mtx);
    shMulMatrix(lcm, vcm, work);
    for (i = 0; i < plane_num; i++) {
        is_valid = 1;
        for (j = 0; j < 4; j++) {
            if (HH_ClassWrapper_RotTrans_PerspectiveProjection_Clip(pObj->pVertex[i * 4 + j].ScreenPos, &q, ls_mtx, lcm, pObj->pVertex[i * 4 + j].LocalPos)) {
                is_valid = 0;
            }
            pObj->pVertex[i * 4 + j].stq[2] = q;
        }
        for (j = 0; j < 4; j++) {
            pObj->pVertex[i * 4 + j].is_valid = is_valid;
        }
    }
}

static void SetFlameSTValue(struct EFCTObject *pObj, struct EFCTFlameObject *flame) {
    int i;
    float s_index;
    float t_index;

    for (i = 0; i < flame->plane_num; i++) {
        s_index = flame->plane[i].s_index;
        t_index = flame->plane[i].current_frame;
        pObj->pVertex[i * 4].stq[0] = 0.125f * s_index;
        pObj->pVertex[i * 4].stq[1] = 0.125f * t_index;
        pObj->pVertex[i * 4 + 1].stq[0] = 0.125f * (1.0f + s_index);
        pObj->pVertex[i * 4 + 1].stq[1] = 0.125f * t_index;
        pObj->pVertex[i * 4 + 2].stq[0] = 0.125f * s_index;
        pObj->pVertex[i * 4 + 2].stq[1] = 0.125f * (1.0f + t_index);
        pObj->pVertex[i * 4 + 3].stq[0] = 0.125f * (1.0f + s_index);
        pObj->pVertex[i * 4 + 3].stq[1] = 0.125f * (1.0f + t_index);
    }
}

/** Sets the texture environment for flames (tfx 0, transparency 2, CLUT 0). */
void InitFlameTexEnv(struct EFCTTexEnvInfo *pTexInfo) {
    pTexInfo->tfx = 0;
    pTexInfo->transparency = 2;
    pTexInfo->clut_id = 0;
}

/** Returns the number of layers the flame effect uses (1). */
unsigned short GetFlameEffectLayerNum(void) {
    return 1;
}

static void NextFrame(struct EFCTObject *pObj, float *wait_time, short *current, float *drawing_time) {
    struct EFCTAnimationData *pAnim;

    pAnim = pObj->pAnimData;
    if (*drawing_time < *wait_time) {
        *drawing_time += EFCTGetPassingTimePerFrame();
    } else {
        *drawing_time = 0.0f;
        if (*current == pAnim->FinishFrameNo) {
            *current = pAnim->StartFrameNo;
        } else if (*current < pAnim->TotalFrame - 1) {
            *current = *current + 1;
        } else {
            *current = 0;
        }
    }
    if (*current > 7) {
#line 1198
        assert_dw(0);
    }
}

static void CountFlameCycleTimer(struct EFCTFlameObject *flame) {
    int i;

    if (flame != NULL) {
        for (i = 0; i < flame->plane_num; i++) {
            flame->plane[i].timer += EFCTGetPassingTimePerFrame();
            if (flame->plane[i].timer / flame->plane[i].cycle > 0.5f) {
                flame->plane[i].timer = 0.0f;
            }
        }
    }
}

static void ClipFlameEffectObject(struct EFCTVertexData *pVertex) {
    int i;
    int valid;

    valid = 0;
    for (i = 0; i < 4; i++) {
        if (!EFCTClipVertex(pVertex[i].ScreenPos)) {
            valid = 1;
            break;
        }
    }
    for (i = 0; i < 4; i++) {
        pVertex[i].is_valid = valid;
    }
}

static void CalibrationZVal(int num, struct EFCTVertexData *pVertex) {
    int z;

    if (camera_cut == 4) {
        z = 5000;
    } else if (camera_cut == 1) {
        z = 3000;
    } else if (camera_cut == 6) {
        z = 250;
    } else {
        z = 2000;
    }
    CalibrationZValue(num, z, pVertex);
}

static void TinyFlameZSort(unsigned int index, int num, struct EFCTVertexData *pVertex) {
}

static int GetBehindAngelaFlameIndex(unsigned int index) {
#line 1340
    assert_dw(index >= behind_angela_flame_start_index);
    return index - behind_angela_flame_start_index;
}

static int GetBehindAngelaFlameSortIndex(unsigned int index) {
#line 1349
    assert_dw(index >= behind_angela_flame_start_index);
    return behind_angela_flame_num - GetBehindAngelaFlameIndex(index) - 1;
}

static void TinyFlameZSort2(unsigned int index, float *new_pos) {
    int new_index;

    new_index = GetBehindAngelaFlameSortIndex(index);
#line 1365
    assert(new_index >= 0);
    vcopy_dst_first(new_pos, behind_angela_flame_pos_buf[new_index]);
}

static float GetBehindAngelaFlameLifeTimer(unsigned int index) {
    int new_index;

    new_index = GetBehindAngelaFlameSortIndex(index);
#line 1378
    assert(new_index >= 0);
    return behind_angela_flame_life_time_buf[new_index];
}

/** Advances the flames' camera cut counter (called on each camera change of the demo). */
void EFCTFlameChangeCamera(void) {
    camera_cut++;
}

static int IsFlameValid(struct EFCTObject *pObj, struct EFCTFlameObject *flame) {
    if (camera_cut == 4) {
        if (pObj->Pos[2] <= -76500.0f) {
            return 0;
        }
    } else if (camera_cut == 6) {
        if (pObj->Pos[2] <= -78000.0f) {
            return 0;
        }
    } else if (camera_cut == 3) {
        if (pObj->Pos[2] > -77000.0f) {
            return 0;
        }
    }
    if (flame->kind == 1) {
        switch (camera_cut) {
        case 1:
        case 2:
        case 5:
            return 0;
        case 0:
            if (pObj->Pos[2] > -77500.0f) {
                return 0;
            }
        case 7:
            if (pObj->Pos[2] > -77000.0f) {
                return 0;
            }
        case 6:
            if (pObj->Pos[2] > -77500.0f) {
                return 0;
            }
        }
    }
    return 1;
}
