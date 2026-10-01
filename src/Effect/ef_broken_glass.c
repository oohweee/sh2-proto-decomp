/*
 * ef_broken_glass.c: the broken-glass effect. 300 glass shards fly off from a point, tumble,
 * fall to the floor and are lit by their angle to the light.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "fi_libvu0_inline.h"

#include "math_const.h"

/* random value in [center - range, center + range] */
#define SWAY(center, range) shSway1f((center) - (range), (center) + (range))

struct EfctBrokenGlassParticle particle[300];
float chara_pos[2][4];
float direction[4];
float total_time = 0.0f;

static unsigned int InitEffectVertexBrokenGlass(float *parent_pos, float *parent_rot, float base_width, float base_height, struct EFCTVertexData **pVertex, unsigned char *rgba);
/*
 * Matching: pos[2] passes its bounds directly (shSway1f(-5, 5)) while the other ranges go through
 * SWAY(): that is what makes the argument load order match.
 */
static void InitParticleParam(struct EfctBrokenGlassParticle *part);
static void SetBrokenGlassSTValue(struct EFCTObject *pObj);
static void ConvertLocalToWorld(struct EFCTVertexData *pVertex, int vertex_num, float *parent_pos, float *parent_rot, float *part_pos, float *part_rot, float *part_trans, float (*vertex_start_pos)[4]);
static void SetBrokenGlassParticalPos(float *pos, struct EFCTVertexData *pVertex);
static void ReplaceParticlePos(struct EFCTVertexData *pVertex, float *parent_pos, float *parent_rot, float *part_pos, float *part_rot, float *part_trans, float (*vertex_start_pos)[4]);
static void GetParticleTrans(struct EfctBrokenGlassParticle *part, float time);
static short ClipParticle(struct EFCTVertexData *pVertex);
static void CalcLightParam(struct EfctBrokenGlassParticle *pParticle, float *light_dir, struct EFCTVertexData *pVertex);
static short IsBrokenGlassEffectFinished(struct EFCTVertexData *pVertex, unsigned short num);
static void GetAngleFromVec(float *rot, float *direction);
static void SetGlassVertex(float width, float height, struct EFCTVertexData *VertexData);

/**
 * Sets up the broken-glass object: 300 shards around parent_pos, facing parent_direction.
 * @param pObj             effect object to fill
 * @param nIndex           object index
 * @param parent_pos       origin of the effect
 * @param parent_direction direction the shards fly off in
 * @param chara_info       two positions copied to chara_pos
 * @return 1 on success, 0 if the vertex buffer could not be allocated
 */
int InitEffectObjectBrokenGlass(struct EFCTObject *pObj, int nIndex, float *parent_pos, float *parent_direction, float (*chara_info)[4]) {
    unsigned char rgba[4] = {0x62, 0x60, 0x58, 0x70};
    int i;

    total_time = 0.0f;
    shSrand(1);
    pObj->Index = nIndex;
    vcopy_dst_first(pObj->Pos, parent_pos);
    pObj->height = pObj->width = 5.0f;
    for (i = 0; i < 4; i++) {
        pObj->trans[i] = 0.0f;
        pObj->rot[i] = 0.0f;
    }
    GetAngleFromVec(pObj->rot, parent_direction);
    vcopy_dst_first(direction, parent_direction);
    pObj->VertexNum = InitEffectVertexBrokenGlass(pObj->Pos, pObj->rot, pObj->width, pObj->height, &pObj->pVertex, rgba);
    if (pObj->VertexNum == 0) {
        return 0;
    }
    vcopy_dst_first(chara_pos[0], chara_info[0]);
    vcopy_dst_first(chara_pos[1], chara_info[1]);
    SetBrokenGlassSTValue(pObj);
    return 1;
}

static unsigned int InitEffectVertexBrokenGlass(float *parent_pos, float *parent_rot, float base_width, float base_height, struct EFCTVertexData **pVertex, unsigned char *rgba) {
    int i;
    int j;
    float width;
    float height;

    if (*pVertex != NULL) {
        EfctFree(*pVertex);
        *pVertex = NULL;
    }
    *pVertex = EfctMalloc(sizeof(struct EFCTVertexData) * 1200);
    if (*pVertex == NULL) {
        return 0;
    }
    for (i = 0; i < 300; i++) {
        width = base_width * (0.5f + 2.5f * shRandF());
        height = base_height * (0.5f + 2.5f * shRandF());
        SetGlassVertex(width, height, &(*pVertex)[i * 4]);
        for (j = 0; j < 4; j++) {
            vcopy_dst_first(particle[i].vertex_start_pos[j], (*pVertex)[i * 4 + j].LocalPos);
        }
        InitParticleParam(&particle[i]);
        ConvertLocalToWorld(&(*pVertex)[i * 4], 4, parent_pos, parent_rot, particle[i].pos, particle[i].rot,
                            particle[i].trans, particle[i].vertex_start_pos);
        SetBrokenGlassParticalPos(particle[i].dst_pos, &(*pVertex)[i * 4]);
        vcopy(particle[i].dst_pos, particle[i].src_pos);
        for (j = 0; j < 4; j++) {
            (*pVertex)[i * 4 + j].rgba[0] = rgba[0];
            (*pVertex)[i * 4 + j].rgba[1] = rgba[1];
            (*pVertex)[i * 4 + j].rgba[2] = rgba[2];
            (*pVertex)[i * 4 + j].rgba[3] = rgba[3];
            (*pVertex)[i * 4 + j].is_valid = 1;
        }
    }
    return 1200;
}

/** Returns deg converted to radians. Matching: an inline function (name ours); written in place,
 * InitParticleParam doesn't match. */
inline float deg2rad(float deg) {
    return (PI / 180.0f) * deg;
}

static void InitParticleParam(struct EfctBrokenGlassParticle *part) {
    part->rot[0] = deg2rad(SWAY(0.0f, 180.0f));
    part->rot[1] = deg2rad(SWAY(0.0f, 180.0f));
    part->rot[2] = deg2rad(SWAY(0.0f, 180.0f));
    part->rot[3] = 0.0f;
    part->pos[0] = SWAY(0.0f, 150.0f);
    part->pos[1] = SWAY(0.0f, 150.0f);
    part->pos[2] = shSway1f(-5.0f, 5.0f);
    part->pos[3] = 0.0f;
    part->spd[0] = SWAY(0.0f, 440.0f);
    part->spd[1] = -1.0f * (120.0f + 2880.0f * shRandF());
    /* Matching: double constants; the original computes this in (software) double precision. */
    part->spd[2] = 45.0 + 500.0 * shRandF();
    part->trans[0] = 0.0f;
    part->trans[1] = 0.0f;
    part->trans[2] = 0.0f;
    part->trans[3] = 0.0f;
    part->top = 0.0f;
    part->falling_param = 0.0f;
    part->top_time = 0.0f;
    part->status = 0;
}

static void SetBrokenGlassSTValue(struct EFCTObject *pObj) {
    int i;
    int s_index;
    int t_index;

    for (i = 0; i < 300; i++) {
        switch (shRandI() % 9) {
        case 0:
            s_index = 0;
            t_index = 0;
            break;
        case 1:
            s_index = 1;
            t_index = 0;
            break;
        case 2:
            s_index = 2;
            t_index = 0;
            break;
        case 3:
            s_index = 0;
            t_index = 1;
            break;
        case 4:
            s_index = 1;
            t_index = 1;
            break;
        case 5:
            s_index = 2;
            t_index = 1;
            break;
        case 6:
            s_index = 0;
            t_index = 2;
            break;
        case 7:
            s_index = 1;
            t_index = 2;
            break;
        case 8:
            s_index = 2;
            t_index = 2;
            break;
        default:
            s_index = 0;
            t_index = 0;
            break;
        }
        pObj->pVertex[i * 4 + 0].stq[0] = 0.0625f * s_index;
        pObj->pVertex[i * 4 + 0].stq[1] = 0.0625f * t_index;
        pObj->pVertex[i * 4 + 1].stq[0] = 0.0625f * (s_index + 1);
        pObj->pVertex[i * 4 + 1].stq[1] = 0.0625f * t_index;
        pObj->pVertex[i * 4 + 2].stq[0] = 0.0625f * s_index;
        pObj->pVertex[i * 4 + 2].stq[1] = 0.0625f * (t_index + 1);
        pObj->pVertex[i * 4 + 3].stq[0] = 0.0625f * (s_index + 1);
        pObj->pVertex[i * 4 + 3].stq[1] = 0.0625f * (t_index + 1);
    }
}

static void ConvertLocalToWorld(struct EFCTVertexData *pVertex, int vertex_num, float *parent_pos, float *parent_rot, float *part_pos, float *part_rot, float *part_trans, float (*vertex_start_pos)[4]) {
    float particle_rot_mtx[4][4];
    float parent_rot_mtx[4][4];
    float local_mtx[4][4];
    float lw_mtx[4][4];
    float trans_xz[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float trans_y[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    _shAddVector(trans_xz, part_pos, part_trans);
    trans_xz[1] = part_pos[1];
    trans_y[1] = part_trans[1];
    _sceVu0UnitMatrix(parent_rot_mtx);
    shRotMatrixZ(parent_rot_mtx, parent_rot_mtx, parent_rot[2]);
    shRotMatrixX(parent_rot_mtx, parent_rot_mtx, parent_rot[0]);
    shRotMatrixY(parent_rot_mtx, parent_rot_mtx, parent_rot[1]);
    _sceVu0UnitMatrix(particle_rot_mtx);
    shRotMatrixZ(particle_rot_mtx, particle_rot_mtx, part_rot[2]);
    shRotMatrixY(particle_rot_mtx, particle_rot_mtx, part_rot[1]);
    shRotMatrixX(particle_rot_mtx, particle_rot_mtx, part_rot[0]);
    _sceVu0TransMatrix(particle_rot_mtx, particle_rot_mtx, trans_xz);
    shMulMatrix(local_mtx, parent_rot_mtx, particle_rot_mtx);
    _sceVu0TransMatrix(local_mtx, local_mtx, trans_y);
    _sceVu0UnitMatrix(lw_mtx);
    _sceVu0TransMatrix(lw_mtx, lw_mtx, parent_pos);
    for (i = 0; i < vertex_num; i++) {
        _sceVu0ApplyMatrix(pVertex[i].LocalPos, local_mtx, vertex_start_pos[i]);
        _sceVu0ApplyMatrix(pVertex[i].WorldPos, lw_mtx, pVertex[i].LocalPos);
    }
}

static void SetBrokenGlassParticalPos(float *pos, struct EFCTVertexData *pVertex) {
    int i;

    *(u_long128 *)pos = 0;
    for (i = 0; i < 4; i++) {
        _shAddVectorXYZ(pos, pos, pVertex[i].WorldPos);
    }
    _shDivVectorXYZ(pos, pos, 4.0f);
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 392
/**
 * Per-frame task: moves, clips, lights and draws the shards; deletes the task once every
 * shard has reached the floor.
 * @param task the EFCTTask of this effect
 */
void DrawBrokenGlass(void *task) {
    struct EFCTTask *pTask;
    int i;
    int j;
    float l_pos_bakcup[4][4];

    pTask = task;
    if (pTask == NULL) {
        assert_dw(0);
    }
    total_time += EFCTGetPassingTimePerFrame();
    for (i = 0; i < 300; i++) {
        if (particle[i].status != 1) {
            if (particle[i].status != 2) {
                GetParticleTrans(&particle[i], total_time);
            }
            for (j = 0; j < 4; j++) {
                vcopy_dst_first(l_pos_bakcup[j], pTask->pObj->pVertex[i * 4 + j].LocalPos);
            }
            ReplaceParticlePos(&pTask->pObj->pVertex[i * 4], pTask->pObj->Pos, pTask->pObj->rot, particle[i].pos,
                               particle[i].rot, particle[i].trans, particle[i].vertex_start_pos);
        }
    }
    EFCTThreeDWork(pTask->pObj);
    for (i = 0; i < 300; i++) {
        if (particle[i].status != 1) {
            ClipParticle(&pTask->pObj->pVertex[i * 4]);
        }
    }
    for (i = 0; i < 300; i++) {
        if (particle[i].status != 1) {
            CalcLightParam(&particle[i], pTask->pObj->rot, &pTask->pObj->pVertex[i * 4]);
        }
    }
    DrawPrimitive(pTask->pObj);
    if (IsBrokenGlassEffectFinished(pTask->pObj->pVertex, pTask->pObj->VertexNum)) {
        EFCTDeleteTask(pTask->pObj->EffectKind);
        total_time = 0.0f;
    }
}

static void ReplaceParticlePos(struct EFCTVertexData *pVertex, float *parent_pos, float *parent_rot, float *part_pos, float *part_rot, float *part_trans, float (*vertex_start_pos)[4]) {
    part_rot[0] += (4.0f * PI) * shGetDT();
    part_rot[1] += 8.3775806f * shGetDT();
    if (part_rot[0] > PI) {
        part_rot[0] -= 2.0f * PI;
    }
    if (part_rot[1] > PI) {
        part_rot[1] -= 2.0f * PI;
    }
    ConvertLocalToWorld(pVertex, 4, parent_pos, parent_rot, part_pos, part_rot, part_trans, vertex_start_pos);
}

static void GetParticleTrans(struct EfctBrokenGlassParticle *part, float time) {
    float v;

    part->trans[0] = part->spd[0] * time;
    part->trans[2] = part->spd[2] * time;
    if (part->top == 0.0f) {
        part->trans[1] = part->spd[1] * time + 1225.0f * sqr(time);
        v = part->spd[1] + 2450.0f * time;
        if (v >= 0.0f) {
            part->top = part->trans[1];
            part->falling_param = 0.05f + 0.45f * shRandF();
            part->top_time = time;
        }
    } else {
        part->trans[1] = part->top + 4900.0f * (0.5f * part->falling_param) * sqr(time - part->top_time);
    }
}

static short ClipParticle(struct EFCTVertexData *pVertex) {
    int i;
    short valid;

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
    return valid;
}

static void CalcLightParam(struct EfctBrokenGlassParticle *pParticle, float *light_dir, struct EFCTVertexData *pVertex) {
    unsigned short alpha;
    unsigned short rgb;
    int rgba[4];

    alpha = 0x70;
    rgb = 0;
    if (pParticle->rot[0] > light_dir[0] - 0.2f && pParticle->rot[0] < 0.2f + light_dir[0]) {
        rgb += 0x34;
        alpha += 8;
    }
    if (pParticle->rot[1] > light_dir[1] - 0.2f && pParticle->rot[1] < 0.2f + light_dir[1]) {
        rgb += 0x34;
        alpha += 8;
    }
    rgba[0] = rgb + 0x62;
    rgba[1] = rgb + 0x60;
    rgba[2] = rgb + 0x58;
    rgba[3] = alpha;
    EFCTResetRGBA(rgba, pVertex);
}

static short IsBrokenGlassEffectFinished(struct EFCTVertexData *pVertex, unsigned short num) {
    int i;
    short ret;

    ret = 1;
    for (i = 0; i < num; i++) {
        if (pVertex[i].WorldPos[1] < 0.0f) {
            ret = 0;
            break;
        }
    }
    return ret;
}

/** Sets the texture environment for broken glass (tfx 0, transparency 2, CLUT 0). */
void InitBrokenGlassTexEnv(struct EFCTTexEnvInfo *pTexInfo) {
    pTexInfo->tfx = 0;
    pTexInfo->transparency = 2;
    pTexInfo->clut_id = 0;
}

/** Returns the number of layers the broken-glass effect uses (1). */
int GetBrokenGlassEffectLayerNum(void) {
    return 1;
}

static void GetAngleFromVec(float *rot, float *direction) {
    vwVectorToAngle(rot, direction);
}

static void SetGlassVertex(float width, float height, struct EFCTVertexData *VertexData) {
    VertexData[0].LocalPos[0] = -width * (0.5f + 2.0f * shRandF());
    VertexData[0].LocalPos[1] = -height * (0.5f + 2.0f * shRandF());
    VertexData[0].LocalPos[2] = 0.0f;
    VertexData[0].LocalPos[3] = 1.0f;
    VertexData[1].LocalPos[0] = width * (0.5f + 2.0f * shRandF());
    VertexData[1].LocalPos[1] = -height * (0.5f + 2.0f * shRandF());
    VertexData[1].LocalPos[2] = 0.0f;
    VertexData[1].LocalPos[3] = 1.0f;
    VertexData[2].LocalPos[0] = -width * (0.5f + 2.0f * shRandF());
    VertexData[2].LocalPos[1] = height * (0.5f + 2.0f * shRandF());
    VertexData[2].LocalPos[2] = 0.0f;
    VertexData[2].LocalPos[3] = 1.0f;
    VertexData[3].LocalPos[0] = width * (0.5f + 2.0f * shRandF());
    VertexData[3].LocalPos[1] = height * (0.5f + 2.0f * shRandF());
    VertexData[3].LocalPos[2] = 0.0f;
    VertexData[3].LocalPos[3] = 1.0f;
}
