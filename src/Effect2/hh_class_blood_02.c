/*
 * hh_class_blood_02.c: a pool of blood spreading on the floor (square or circle mesh).
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Blood_02 runs once a
 * frame for each instance, between the class's prefix and suffix.
 *
 * Vertex_Infomeation_List is defined differently in several files (the DWARF has a struct of
 * that name in each, not all alike), so sh2/types.h only declares it and each file defines its
 * own version, this one below.
 * Here its second list holds STQs where the generated one has normals.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

struct Vertex_Infomeation_List {
    float (*pVertex_List)[4];
    float (*pStq_List)[4];
    unsigned int Vertex_Max;
    unsigned int Primitive_Type;
};

static float _square_00_vertex[4][4] = {
    { 1.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 0.0f, -1.0f, 1.0f },
    { -1.0f, 0.0f, 1.0f, 1.0f },
    { -1.0f, 0.0f, -1.0f, 1.0f },
};

static float _square_00_stq[4][4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 1.0f, 0.0f },
    { 1.0f, 0.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 1.0f, 0.0f },
};

static float _circle_00_vertex[10][4] = {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 1.0f, 0.0f, 0.0f, 1.0f },
    { 0.707107f, 0.0f, 0.707107f, 1.0f },
    { -0.0f, 0.0f, 1.0f, 1.0f },
    { -0.707107f, 0.0f, 0.707107f, 1.0f },
    { -1.0f, 0.0f, -0.0f, 1.0f },
    { -0.707107f, 0.0f, -0.707107f, 1.0f },
    { 0.0f, 0.0f, -1.0f, 1.0f },
    { 0.707107f, 0.0f, -0.707107f, 1.0f },
    { 1.0f, 0.0f, 0.0f, 1.0f },
};

static float _circle_00_stq[10][4] = {
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 1.0f, 0.5f, 1.0f, 0.0f },
    { 0.853553f, 0.853553f, 1.0f, 0.0f },
    { 0.5f, 1.0f, 1.0f, 0.0f },
    { 0.146447f, 0.853553f, 1.0f, 0.0f },
    { 0.0f, 0.5f, 1.0f, 0.0f },
    { 0.146447f, 0.146447f, 1.0f, 0.0f },
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.853554f, 0.146447f, 1.0f, 0.0f },
    { 1.0f, 0.5f, 1.0f, 0.0f },
};

static float _circle_01_vertex[10][4] = {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.8f, 0.0f, 0.0f, 1.0f },
    { 1.007627f, 0.0f, 1.007627f, 1.0f },
    { -0.0f, 0.0f, 1.675f, 1.0f },
    { -1.096016f, 0.0f, 1.096016f, 1.0f },
    { -0.925f, 0.0f, -0.0f, 1.0f },
    { -0.919239f, 0.0f, -0.919239f, 1.0f },
    { 0.0f, 0.0f, -1.3f, 1.0f },
    { 0.565686f, 0.0f, -0.565685f, 1.0f },
    { 0.8f, 0.0f, 0.0f, 1.0f },
};

static float _circle_02_vertex[10][4] = {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.4f, 0.0f, 0.0f, 1.0f },
    { 0.724784f, 0.0f, 0.724784f, 1.0f },
    { -0.0f, 0.0f, 1.275f, 1.0f },
    { -0.813173f, 0.0f, 0.813173f, 1.0f },
    { -0.525f, 0.0f, -0.0f, 1.0f },
    { -0.636396f, 0.0f, -0.636396f, 1.0f },
    { 0.0f, 0.0f, -0.9f, 1.0f },
    { 0.282843f, 0.0f, -0.282843f, 1.0f },
    { 0.4f, 0.0f, 0.0f, 1.0f },
};

static struct Vertex_Infomeation_List _vertex_info_list[4] = {
    { _square_00_vertex, _square_00_stq, 4, 4 },
    { _circle_00_vertex, _circle_00_stq, 10, 5 },
    { _circle_01_vertex, _circle_00_stq, 10, 5 },
    { _circle_02_vertex, _circle_00_stq, 10, 5 },
};

static float _rgba_start_list[4][4] = {
    { 64.0f, 180.0f, 150.0f, 128.0f },
    { 64.0f, 180.0f, 150.0f, 128.0f },
    { 64.0f, 180.0f, 150.0f, 128.0f },
    { 64.0f, 180.0f, 150.0f, 128.0f },
};

static float _rgba_end_list[4][4] = {
    { 96.0f, 255.0f, 230.0f, 128.0f },
    { 96.0f, 255.0f, 230.0f, 128.0f },
    { 96.0f, 255.0f, 230.0f, 128.0f },
    { 96.0f, 255.0f, 230.0f, 128.0f },
};

static float _scale_start_list[5][4] = {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
};

static float _scale_end_list[5][4] = {
    { 400.0f, 1.0f, 400.0f, 1.0f },
    { 350.0f, 1.0f, 400.0f, 1.0f },
    { 400.0f, 1.0f, 350.0f, 1.0f },
    { 300.0f, 1.0f, 400.0f, 1.0f },
    { 400.0f, 1.0f, 300.0f, 1.0f },
};

static struct Motion_Table_Infomeation _motion_info[4] = {
    { 0.0f, 0.0f, 0.0f },
    { 2.0f, 0.3f, 0.0f },
    { 6.0f, 0.5f, 0.35f },
    { 8.0f, 0.2f, 0.65f },
};

static unsigned int Object_Initialize(struct HH_Object_Blood_02 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    float radian;

    if (pElement->Option.Int_Value[1] == 0) {
        pThis->Vertex_Kind = rand() % 4U;
        pThis->Scale_Kind = rand() % 5U;
        pThis->Rgba_Kind = rand() % 4U;
        pThis->Timer = 0.0f;
        radian = (rand() % 360) * 0.017453292f;
        pThis->Rotate_Y = Radian_Normalize(radian);
    }
    pThis->Motion_Step = 0;
    result = 1;
    return result;
}

static unsigned int Object_Motion_00(struct HH_Object_Blood_02 *pThis) {
    unsigned int result;
    float *pRgba_s;
    float *pRgba_e;
    float *pScale_s;
    float *pScale_e;
    float local_time_start;
    float local_time_end;
    float local_scale_start;
    float local_scale_end;
    float local_rgba_start;
    float local_rgba_end;
    struct Motion_Table_Infomeation *pMotion_Info;
    unsigned int motion_num;
    unsigned int i;
    unsigned int current_step;
    float local_diff_time_current;
    float local_diff_time_max;
    float local_diff_time_ratio;
    float scale_ratio;
    float rgba_ratio;

    pRgba_s = _rgba_start_list[pThis->Rgba_Kind];
    pRgba_e = _rgba_end_list[pThis->Rgba_Kind];
    pScale_s = _scale_start_list[pThis->Scale_Kind];
    pScale_e = _scale_end_list[pThis->Scale_Kind];
    local_time_start = 0.0f;
    local_scale_start = 0.0f;
    local_rgba_start = 0.0f;
    pMotion_Info = _motion_info;
    current_step = pThis->Motion_Step;
    if (current_step > 2) {
        current_step = 2;
    }
    for (i = 0; i < current_step + 1; i++) {
        local_time_start += pMotion_Info[i].diff_time;
        local_scale_start += pMotion_Info[i].diff_scale_ratio;
        local_rgba_start += pMotion_Info[i].diff_rgba_ratio;
    }
    local_time_end = local_time_start + pMotion_Info[i].diff_time;
    local_scale_end = local_scale_start + pMotion_Info[i].diff_scale_ratio;
    local_rgba_end = local_rgba_start + pMotion_Info[i].diff_rgba_ratio;
    switch (pThis->Motion_Step) {
    case 0:
        if (pThis->Timer > local_time_end) {
            pThis->Motion_Step++;
        }
        break;
    case 1:
        if (pThis->Timer > local_time_end) {
            pThis->Motion_Step++;
        }
        break;
    case 2:
        if (pThis->Timer > local_time_end) {
            pThis->Motion_Step++;
        }
        break;
    }
    local_diff_time_current = pThis->Timer - local_time_start;
    local_diff_time_max = local_time_end - local_time_start;
    local_diff_time_ratio = local_diff_time_current / local_diff_time_max;
    if (local_diff_time_ratio > 1.0f) {
        local_diff_time_ratio = 1.0f;
    }
    scale_ratio = local_scale_start + local_diff_time_ratio * (local_scale_end - local_scale_start);
    rgba_ratio = local_rgba_start + local_diff_time_ratio * (local_rgba_end - local_rgba_start);
    sceVu0InterVectorXYZ(pThis->Scale, pScale_e, pScale_s, scale_ratio);
    sceVu0InterVectorXYZ(pThis->Rgba, pRgba_e, pRgba_s, rgba_ratio);
    result = 0;
    return result;
}

static unsigned int Object_Draw(struct HH_Object_Blood_02 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    sceVif1Packet *pPk;
    struct Vertex_Infomeation_List *pInfo;
    float (*pVertex)[4];
    float (*pStq)[4];
    unsigned int vertex_num;
    unsigned int i;
    float lwm[4][4];
    float lsm[4][4];
    float clip_mat[4][4];
    float Rgba[4];
    float stq[4];
    int xyzf[4];
    int rgba[4];
    float ratio;

    pPk = HH_Vif1Packet_Current_Get();
    pInfo = &_vertex_info_list[pThis->Vertex_Kind];
    pVertex = pInfo->pVertex_List;
    pStq = pInfo->pStq_List;
    vertex_num = pInfo->Vertex_Max;
    HH_ClassWrapper_WorldScreenMatrix_Get(lsm);
    sceVu0UnitMatrix(lwm);
    lwm[0][0] *= pThis->Scale[0];
    lwm[2][2] *= pThis->Scale[2];
    sceVu0RotMatrixY(lwm, lwm, pThis->Rotate_Y);
    sceVu0TransMatrix(lwm, lwm, pElement->Option.Vector[0]);
    sceVu0MulMatrix(lsm, lsm, lwm);
    HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(clip_mat);
    sceVu0MulMatrix(clip_mat, clip_mat, lwm);
    HH_Vif1PacketBuffer_GifTag_Open();
    switch (pInfo->Primitive_Type) {
    case 4:
        HH_Vif1Packet_GeneralGifTag_TriangleStrip_Open();
        break;
    case 5:
        HH_Vif1Packet_GeneralGifTag_TriangleFan_Open();
        break;
    }
    for (i = 0; i < vertex_num; i++) {
        sceVu0CopyVector(stq, pStq[i]);
        HH_ClassWrapper_Transform_PerspectiveProjection_Clip_forTriangleStrip(xyzf, stq, lsm, clip_mat, pVertex[i]);
        if (i == 0) {
            ratio = HH_ClassWrapper_FogParameter_A_Get() + stq[2] * HH_ClassWrapper_FogParameter_B_Get();
            ratio = HH_ClassWrapper_Float_Clamp(ratio, 0.0f, 255.0f) / 255.0f;
            sceVu0ScaleVector(Rgba, pThis->Rgba, ratio);
            sceVu0FTOI0Vector(rgba, Rgba);
        }
        xyzf[2] += 0xA0;
        ((u_long128 *)pPk->pCurrent)[0] = *(u_long128 *)stq;
        ((u_long128 *)pPk->pCurrent)[1] = *(u_long128 *)rgba;
        ((u_long128 *)pPk->pCurrent)[2] = *(u_long128 *)xyzf;
        pPk->pCurrent += 12;
    }
    sceVif1PkCloseGifTag(pPk);
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 0;
    return result;
}

/**
 * Class prefix, run once a frame before this class's instances: sets the GS TEX0 and ALPHA
 * registers they draw with (TEX0: the specular-map texture). Returns 1.
 */
unsigned int HH_Class_Prefix_Blood_02(void) {
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned long tex0;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    tex0 = sh2_SpecularMappingTEX0();
    sceVif1PkAddGsAD(pPk, GS_REG_TEX0_1, tex0);
    sceVif1PkAddGsAD(pPk, GS_REG_ALPHA_1, GS_SET_ALPHA(2, 0, 0, 1, 0x80));
    sceVif1PkCloseGifTag(pPk);
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class suffix, run once a frame after this class's instances: opens a general A+D GIF tag in the
 * packet. Returns 1.
 */
unsigned int HH_Class_Suffix_Blood_02(void) {
    unsigned int result;

    HH_Vif1Packet_GeneralGifTag_GS_AD_Open();
    result = 1;
    return result;
}

/**
 * Class main: sets the pool up (random mesh, size, colour and rotation unless
 * Option.Int_Value[1] is set), then grows and draws it until something disables it.
 * @param pBlock   the instance's data block (struct HH_Object_Blood_02)
 * @param pElement the element that created it; Option.Vector[0] is the pool's position
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Blood_02(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Blood_02 *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis, pElement);
        pThis->Step = 1;
        break;
    case 1:
        Object_Motion_00(pThis);
        Object_Draw(pThis, pElement);
        pThis->Timer += 1.0f / 30.0f;
        break;
    case 2:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    return result;
}

/**
 * Tests whether a point lies inside a blood pool: its squared distance from the pool's centre
 * is below 95% of the squared distance to the nearest mesh vertex.
 * @param pBlock          the pool's data block (struct HH_Object_Blood_02)
 * @param pElement        the pool's element (Option.Vector[0] is its centre)
 * @param Target_Location the point to test
 * @return 1 if the point is inside, else 0
 */
unsigned int HH_Class_Blood_02_DesignateLocation_CollisionCheck(void *pBlock, struct ImpactQueue_Element *pElement, float *Target_Location) {
    unsigned int result;
    struct HH_Object_Blood_02 *pThis;
    struct Vertex_Infomeation_List *pInfo;
    float (*pVertex)[4];
    unsigned int vertex_num;
    float max;
    float min;
    float volume;
    unsigned int i;
    float vertex[4];
    float target_dir[4];
    float target_volume;

    result = 0;
    pThis = pBlock;
    pInfo = &_vertex_info_list[pThis->Vertex_Kind];
    pVertex = pInfo->pVertex_List;
    vertex_num = pInfo->Vertex_Max;
    for (i = 1; i < vertex_num; i++) {
        sceVu0MulVector(vertex, pThis->Scale, pVertex[i]);
        volume = sceVu0InnerProduct(vertex, vertex);
        if (i == 1) {
            min = volume;
            max = volume;
        }
        if (volume < min) {
            min = volume;
        }
        if (volume > max) {
            max = volume;
        }
    }
    sceVu0SubVector(target_dir, Target_Location, pElement->Option.Vector[0]);
    target_volume = sceVu0InnerProduct(target_dir, target_dir);
    min *= 0.95f;
    if (target_volume < min) {
        result = 1;
    }
    return result;
}
