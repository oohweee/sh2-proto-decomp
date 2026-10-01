/*
 * hh_class_blood_01.c: a spurt of blood sprites thrown along a parabola; spawns a stain where it
 * lands.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Blood_01 runs once a
 * frame for each instance, between the class's prefix and suffix.
 *
 * Vertex_Infomeation_List is defined differently in several files (the DWARF has a struct of
 * that name in each, not all alike), so sh2/types.h only declares it and each file defines its
 * own version, this one below.
 * Here its vertex and normal pointers are to the 16-byte aligned vector typedef (common.h's
 * sceVu0FVECTOR), which makes MWCC align the struct (and arrays of it) to 16.
 */
#include "sh2.h"
#include "libc/stdlib.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

struct Vertex_Infomeation_List {
    sceVu0FVECTOR *pVertex_List;
    sceVu0FVECTOR *pNormal_List;
    unsigned int Vertex_Max;
    unsigned int Primitive_Type;
};

static float _suppress_coff = 2.0f;

static float _square_00_vertex[4][4] = {
    { -25.0f, -25.0f, 0.0f, 1.0f },
    { -25.0f, 25.0f, 0.0f, 1.0f },
    { 25.0f, -25.0f, 0.0f, 1.0f },
    { 25.0f, 25.0f, 0.0f, 1.0f },
};

static float _square_00_normal[4][4] = {
    { 0.0f, 0.0f, -1.0f, 1.0f },
    { 0.0f, 0.0f, -1.0f, 1.0f },
    { 0.0f, 0.0f, -1.0f, 1.0f },
    { 0.0f, 0.0f, -1.0f, 1.0f },
};

static float _square_00_stq[4][4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 1.0f, 0.0f },
};

static float _square_01_stq[4][4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 1.0f, 0.0f },
};

static float _square_02_stq[4][4] = {
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.5f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
};

static float _square_03_stq[4][4] = {
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 0.0f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
};

static float _square_10_stq[4][4] = {
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 1.0f, 0.0f, 1.0f, 0.0f },
    { 1.0f, 0.5f, 1.0f, 0.0f },
};

static float _square_11_stq[4][4] = {
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 1.0f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 1.0f, 0.5f, 1.0f, 0.0f },
};

static float _square_12_stq[4][4] = {
    { 1.0f, 0.5f, 1.0f, 0.0f },
    { 1.0f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.0f, 1.0f, 0.0f },
};

static float _square_13_stq[4][4] = {
    { 1.0f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 1.0f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.0f, 1.0f, 0.0f },
};

static struct Vertex_Infomeation_List _vertex_info_list[1] = {
    { _square_00_vertex, _square_00_normal, 4, 4 },
};

static sceVu0FVECTOR *_stq_list[8] = {
    _square_00_stq, _square_01_stq, _square_02_stq, _square_03_stq,
    _square_10_stq, _square_11_stq, _square_12_stq, _square_13_stq,
};

static float _rgba_start_list[2] = { 128.0f, 128.0f };
static float _rgba_end_list[2] = { 0.0f, 128.0f };
static float _scale_start_list[2] = { 0.8f, 0.8f };
static float _scale_end_list[2] = { 7.0f, 2.0f };

static struct Motion_Table_Infomeation _motion_info_0[2] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.6f, 1.0f, 1.0f },
};

static struct Motion_Table_Infomeation _motion_info_1[2] = {
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 1.0f },
};

static struct Motion_Table_Infomeation *_motion_info_list[2] = { _motion_info_0, _motion_info_1 };

static unsigned int Object_Initialize(struct HH_Object_Blood_01 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    float *src_direction;
    float resist_xz;
    float resist_y0;
    float resist_ya;

    pThis->Timer = 0.0f;
    src_direction = pElement->Option.Vector[1];
    resist_xz = 0.5494505f;
    resist_y0 = 1.098901f;
    resist_ya = 1.098901f;
    pThis->Verocity_0[0] = src_direction[0] * resist_xz;
    pThis->Verocity_0[1] = src_direction[1] * resist_y0;
    pThis->Verocity_0[2] = src_direction[2] * resist_xz;
    pThis->Verocity_0[3] = 2450.0f * _suppress_coff * resist_ya;
    pElement->Option.Int_Value[1] = rand() % 8U;
    result = 0;
    return result;
}

static void ParabolaMotion_Calculator(struct HH_Object_Blood_01 *pThis, struct ImpactQueue_Element *pElement, float Time, float *Position) {
    float time_mat[4][4];

    sceVu0UnitMatrix(time_mat);
    time_mat[0][0] = Time;
    time_mat[1][1] = Time;
    time_mat[2][2] = Time;
    time_mat[3][1] = Time * Time;
    time_mat[3][3] = 0.0f;
    sceVu0ApplyMatrix(Position, time_mat, pThis->Verocity_0);
}

static unsigned int Object_Motion_00(struct HH_Object_Blood_01 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    unsigned int kind;
    float Rgba_s;
    float Rgba_e;
    float Scale_s;
    float Scale_e;
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

    kind = pElement->Option.Int_Value[0];
    Rgba_s = _rgba_start_list[kind];
    Rgba_e = _rgba_end_list[kind];
    Scale_s = _scale_start_list[kind];
    Scale_e = _scale_end_list[kind];
    local_time_start = 0.0f;
    local_scale_start = 0.0f;
    local_rgba_start = 0.0f;
    pMotion_Info = _motion_info_list[kind];
    /*
     * Matching: the DWARF has motion_num and the line table a statement here that left no code;
     * its value is not known (the table's length is a guess).
     */
    motion_num = ARRAY_COUNT(_motion_info_0);
    current_step = pThis->Motion_Step;
    if (current_step > 1) {
        current_step = 1;
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
    }
    local_diff_time_current = pThis->Timer - local_time_start;
    local_diff_time_max = local_time_end - local_time_start;
    local_diff_time_ratio = local_diff_time_current / local_diff_time_max;
    if (local_diff_time_ratio > 1.0f) {
        local_diff_time_ratio = 1.0f;
    }
    scale_ratio = local_scale_start + local_diff_time_ratio * (local_scale_end - local_scale_start);
    rgba_ratio = local_rgba_start + local_diff_time_ratio * (local_rgba_end - local_rgba_start);
    pThis->Scale = Scale_e * scale_ratio + Scale_s * (1.0f - scale_ratio);
    pThis->Alpha = Rgba_e * rgba_ratio + Rgba_s * (1.0f - rgba_ratio);
    result = 0;
    return result;
}

static unsigned int Object_Draw(struct HH_Object_Blood_01 *pThis, struct ImpactQueue_Element *pElement, float *Current_Position) {
    unsigned int result;
    sceVif1Packet *pPk;
    struct Vertex_Infomeation_List *pInfo;
    float (*pVertex)[4];
    float (*pStq)[4];
    float lwm[4][4];
    float lsm[4][4];
    float clip_mat[4][4];

    pPk = HH_Vif1Packet_Current_Get();
    pInfo = &_vertex_info_list[pThis->Vertex_Kind];
    pVertex = pInfo->pVertex_List;
    pStq = _stq_list[pElement->Option.Int_Value[1]];
    {
        float Base_Rgba[4] = { 40.0f, 255.0f, 230.0f, pThis->Alpha };
        int xyzf[4];
        int rgba[4];
        float stq_dummy[4];

        HH_ClassWrapper_WorldScreenMatrix_Get(lsm);
        HH_ClassWrapper_AlwaysFront_WorldView_Matrix_Get(lwm);
        sceVu0ScaleVector(lwm[0], lwm[0], pThis->Scale);
        sceVu0ScaleVector(lwm[1], lwm[1], pThis->Scale);
        sceVu0ScaleVector(lwm[2], lwm[2], pThis->Scale);
        sceVu0TransMatrix(lwm, lwm, pElement->Option.Vector[0]);
        sceVu0TransMatrix(lwm, lwm, Current_Position);
        sceVu0MulMatrix(lsm, lsm, lwm);
        HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(clip_mat);
        sceVu0MulMatrix(clip_mat, clip_mat, lwm);
        HH_Vif1PacketBuffer_GifTag_Open();
        HH_Vif1Packet_GeneralGifTag_Sprite_Open();
        sceVu0FTOI0Vector(rgba, Base_Rgba);
        ((u_long128 *)pPk->pCurrent)[0] = *(u_long128 *)rgba;
        HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq_dummy, lsm, clip_mat, pVertex[0], 0x3F);
        ((u_long128 *)pPk->pCurrent)[1] = *(u_long128 *)pStq[0];
        xyzf[2] += 0xA0;
        ((u_long128 *)pPk->pCurrent)[2] = *(u_long128 *)xyzf;
        HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq_dummy, lsm, clip_mat, pVertex[3], 0x3F);
        ((u_long128 *)pPk->pCurrent)[3] = *(u_long128 *)pStq[3];
        xyzf[2] += 0xA0;
        ((u_long128 *)pPk->pCurrent)[4] = *(u_long128 *)xyzf;
        pPk->pCurrent += 20;
    }
    sceVif1PkCloseGifTag(pPk);
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 0;
    return result;
}

/**
 * Class prefix, run once a frame before this class's instances: sets the GS TEX0 and ALPHA
 * registers they draw with. Returns 1.
 */
unsigned int HH_Class_Prefix_Blood_01(void) {
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned long tex0;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    tex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(0, 0);
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
unsigned int HH_Class_Suffix_Blood_01(void) {
    unsigned int result;

    HH_Vif1Packet_GeneralGifTag_GS_AD_Open();
    result = 1;
    return result;
}

/**
 * Class main: throws the sprites along a parabola, scaling and fading them. Kind 0 ends when
 * they have faded out; kind 1 ends when they reach the floor and posts a blood pool there
 * (class 10, HH_Class_Blood_00); other kinds end at once.
 * @param pBlock   the instance's data block (struct HH_Object_Blood_01)
 * @param pElement the element that created it; Option.Vector[0] is the start position,
 *                 Vector[1] the initial velocity, Int_Value[0] the kind; Int_Value[1] is set to a
 *                 random texture variant
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Blood_01(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Blood_01 *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis, pElement);
        switch (pElement->Option.Int_Value[0]) {
        case 0:
            pThis->Step = 1;
            break;
        case 1:
            pThis->Step = 2;
            break;
        default:
            pThis->Step = 3;
            break;
        }
        break;
    case 1: {
        float position[4];

        ParabolaMotion_Calculator(pThis, pElement, pThis->Timer, position);
        Object_Motion_00(pThis, pElement);
        if (pThis->Alpha <= 0.0f) {
            pThis->Step = 3;
        }
        Object_Draw(pThis, pElement, position);
        pThis->Timer += 1.0f / 60.0f;
        break;
    }
    case 2: {
        float position[4];

        ParabolaMotion_Calculator(pThis, pElement, pThis->Timer, position);
        Object_Motion_00(pThis, pElement);
        if (position[1] + pElement->Option.Vector[0][1] > 0.0f) {
            struct ImpactQueue_Element descriptor;

            descriptor.hInstance = 0;
            descriptor.pResultHandle_Address = NULL;
            sceVu0AddVector(descriptor.Option.Vector[0], pElement->Option.Vector[0], position);
            descriptor.Option.Vector[0][1] = 0.0f;
            descriptor.Class_Descriptor = 10;
            HH_Effect_Object_Impact_Post(&descriptor);
            pThis->Step = 3;
        }
        Object_Draw(pThis, pElement, position);
        pThis->Timer += 1.0f / 60.0f;
        break;
    }
    case 3:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    return result;
}
