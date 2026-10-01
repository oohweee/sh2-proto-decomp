/*
 * hh_class_blood_00.c: a pool of blood spreading on the floor, then fading out.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Blood_00 runs once a
 * frame for each instance, between the class's prefix and suffix.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

static float _square_00_vertex[4][4] = {
    { 1.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 0.0f, -1.0f, 1.0f },
    { -1.0f, 0.0f, 1.0f, 1.0f },
    { -1.0f, 0.0f, -1.0f, 1.0f },
};

static float _square_01_vertex[4][4] = {
    { 1.5f, 0.0f, 1.5f, 1.0f },
    { 1.0f, 0.0f, -1.0f, 1.0f },
    { -1.0f, 0.0f, 1.0f, 1.0f },
    { -1.0f, 0.0f, -1.0f, 1.0f },
};

static float _square_02_vertex[4][4] = {
    { 1.0f, 0.0f, 1.0f, 1.0f },
    { 1.5f, 0.0f, -1.5f, 1.0f },
    { -1.0f, 0.0f, 1.0f, 1.0f },
    { -1.0f, 0.0f, -1.0f, 1.0f },
};

static float _square_03_vertex[4][4] = {
    { 1.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 0.0f, -1.0f, 1.0f },
    { -1.5f, 0.0f, 1.5f, 1.0f },
    { -1.0f, 0.0f, -1.0f, 1.0f },
};

static float _square_04_vertex[4][4] = {
    { 1.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 0.0f, -1.0f, 1.0f },
    { -1.0f, 0.0f, 1.0f, 1.0f },
    { -1.5f, 0.0f, -1.5f, 1.0f },
};

static float (*_square_vertex_list[5])[4] = {
    _square_00_vertex, _square_01_vertex, _square_02_vertex, _square_03_vertex, _square_04_vertex,
};

static float _square_00_stq[4][4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 1.0f, 0.0f },
    { 1.0f, 0.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 1.0f, 0.0f },
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
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

static float _scale_end_list[8][4] = {
    { 40.0f, 1.0f, 40.0f, 0.0f },
    { 30.0f, 1.0f, 35.0f, 0.0f },
    { 35.0f, 1.0f, 30.0f, 0.0f },
    { 35.0f, 1.0f, 32.0f, 0.0f },
    { 30.0f, 1.0f, 30.0f, 0.0f },
    { 20.0f, 1.0f, 25.0f, 0.0f },
    { 25.0f, 1.0f, 20.0f, 0.0f },
    { 20.0f, 1.0f, 20.0f, 0.0f },
};

static struct Motion_Table_Infomeation _motion_info[4] = {
    { 0.0f, 0.7f, 0.0f },
    { 1.0f, 0.15f, 0.0f },
    { 2.0f, 0.1f, 0.35f },
    { 4.0f, 0.05f, 0.65f },
};

static unsigned int Object_Initialize(struct HH_Object_Blood_00 *pThis) {
    unsigned int result;
    float radian;

    pThis->Vertex_Kind = rand() % 5U;
    pThis->Scale_Kind = rand() % 5U;
    pThis->Rgba_Kind = rand() % 4U;
    pThis->Timer = 0.0f;
    pThis->Motion_Step = 0;
    radian = (rand() % 360) * 0.017453292f;
    pThis->Rotate_Y = Radian_Normalize(radian);
    result = 1;
    return result;
}

static unsigned int Object_Motion_00(struct HH_Object_Blood_00 *pThis) {
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
    /*
     * Matching: the DWARF has motion_num and the line table a statement here that left no code;
     * its value is not known (the table's length is a guess).
     */
    motion_num = ARRAY_COUNT(_motion_info);
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

static unsigned int Object_Draw(struct HH_Object_Blood_00 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    sceVif1Packet *pPk;
    float (*pVertex)[4];
    float (*pStq)[4];
    unsigned int i;
    float lwm[4][4];
    float lsm[4][4];
    float clip_mat[4][4];
    float Rgba[4];
    int rgba[4];
    float stq[4];
    int xyzf[4];
    float ratio;

    pPk = HH_Vif1Packet_Current_Get();
    pVertex = _square_vertex_list[pThis->Vertex_Kind];
    pStq = _square_00_stq;
    HH_ClassWrapper_WorldScreenMatrix_Get(lsm);
    sceVu0UnitMatrix(lwm);
    lwm[0][0] *= pThis->Scale[0];
    lwm[2][2] *= pThis->Scale[2];
    sceVu0TransMatrix(lwm, lwm, pElement->Option.Vector[0]);
    sceVu0MulMatrix(lsm, lsm, lwm);
    HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(clip_mat);
    sceVu0MulMatrix(clip_mat, clip_mat, lwm);
    HH_Vif1Packet_GeneralGifTag_TriangleStrip_Open();
    for (i = 0; i < 4; i++) {
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
unsigned int HH_Class_Prefix_Blood_00(void) {
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
unsigned int HH_Class_Suffix_Blood_00(void) {
    unsigned int result;

    HH_Vif1Packet_GeneralGifTag_GS_AD_Open();
    result = 1;
    return result;
}

/**
 * Class main: sets the pool up, then grows and draws it for 30 seconds, then fades it out and
 * ends the instance.
 * @param pBlock   the instance's data block (struct HH_Object_Blood_00)
 * @param pElement the element that created it; Option.Vector[0] is the pool's position
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Blood_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Blood_00 *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis);
        pThis->Step = 1;
        break;
    case 1:
        Object_Motion_00(pThis);
        Object_Draw(pThis, pElement);
        pThis->Timer += 1.0f / 30.0f;
        if (pThis->Timer > 30.0f) {
            pThis->Step = 2;
        }
        break;
    case 2:
        Object_Draw(pThis, pElement);
        pThis->Rgba[3] -= 1.0f;
        if (pThis->Rgba[3] <= 0.0f) {
            pThis->Rgba[3] = 0.0f;
            pThis->Step = 3;
        }
        break;
    case 3:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    return result;
}
