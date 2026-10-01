/*
 * hh_class_water_pool_00.c: a puddle of water spreading on the floor, lit by the spotlight.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Water_Pool_00 runs once a
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

static float _square_00_stq[4][4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.24804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.0f, 1.0f, 0.0f },
    { 0.24804688f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_01_stq[4][4] = {
    { 0.25f, 0.0f, 1.0f, 0.0f },
    { 0.25f, 0.24804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.0f, 1.0f, 0.0f },
    { 0.49804688f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_02_stq[4][4] = {
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.24804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.0f, 1.0f, 0.0f },
    { 0.7480469f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_03_stq[4][4] = {
    { 0.75f, 0.0f, 1.0f, 0.0f },
    { 0.75f, 0.24804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.0f, 1.0f, 0.0f },
    { 0.9980469f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_10_stq[4][4] = {
    { 0.0f, 0.25f, 1.0f, 0.0f },
    { 0.0f, 0.49804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.25f, 1.0f, 0.0f },
    { 0.24804688f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_11_stq[4][4] = {
    { 0.25f, 0.25f, 1.0f, 0.0f },
    { 0.25f, 0.49804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.25f, 1.0f, 0.0f },
    { 0.49804688f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_12_stq[4][4] = {
    { 0.5f, 0.25f, 1.0f, 0.0f },
    { 0.5f, 0.49804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.25f, 1.0f, 0.0f },
    { 0.7480469f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_13_stq[4][4] = {
    { 0.75f, 0.25f, 1.0f, 0.0f },
    { 0.75f, 0.49804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.25f, 1.0f, 0.0f },
    { 0.9980469f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_20_stq[4][4] = {
    { 0.0f, 0.5f, 1.0f, 0.0f },
    { 0.0f, 0.7480469f, 1.0f, 0.0f },
    { 0.24804688f, 0.5f, 1.0f, 0.0f },
    { 0.24804688f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_21_stq[4][4] = {
    { 0.25f, 0.5f, 1.0f, 0.0f },
    { 0.25f, 0.7480469f, 1.0f, 0.0f },
    { 0.49804688f, 0.5f, 1.0f, 0.0f },
    { 0.49804688f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_22_stq[4][4] = {
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.7480469f, 1.0f, 0.0f },
    { 0.7480469f, 0.5f, 1.0f, 0.0f },
    { 0.7480469f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_23_stq[4][4] = {
    { 0.75f, 0.5f, 1.0f, 0.0f },
    { 0.75f, 0.7480469f, 1.0f, 0.0f },
    { 0.9980469f, 0.5f, 1.0f, 0.0f },
    { 0.9980469f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_30_stq[4][4] = {
    { 0.0f, 0.75f, 1.0f, 0.0f },
    { 0.0f, 0.9980469f, 1.0f, 0.0f },
    { 0.24804688f, 0.75f, 1.0f, 0.0f },
    { 0.24804688f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_31_stq[4][4] = {
    { 0.25f, 0.75f, 1.0f, 0.0f },
    { 0.25f, 0.9980469f, 1.0f, 0.0f },
    { 0.49804688f, 0.75f, 1.0f, 0.0f },
    { 0.49804688f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_32_stq[4][4] = {
    { 0.5f, 0.75f, 1.0f, 0.0f },
    { 0.5f, 0.9980469f, 1.0f, 0.0f },
    { 0.7480469f, 0.75f, 1.0f, 0.0f },
    { 0.7480469f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_33_stq[4][4] = {
    { 0.75f, 0.75f, 1.0f, 0.0f },
    { 0.75f, 0.9980469f, 1.0f, 0.0f },
    { 0.9980469f, 0.75f, 1.0f, 0.0f },
    { 0.9980469f, 0.9980469f, 1.0f, 0.0f },
};

static float (*_square_stq_list[16])[4] = {
    _square_00_stq, _square_01_stq, _square_02_stq, _square_03_stq,
    _square_10_stq, _square_11_stq, _square_12_stq, _square_13_stq,
    _square_20_stq, _square_21_stq, _square_22_stq, _square_23_stq,
    _square_30_stq, _square_31_stq, _square_32_stq, _square_33_stq,
};

static float _rgba_start_list[1][4] = {
    { 255.0f, 255.0f, 255.0f, 255.0f },
};

static float _rgba_end_list[1][4] = {
    { 188.0f, 188.0f, 218.0f, 50.0f },
};

static float _scale_start_list[3][4] = {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
};

static float _scale_end_list[4][4] = {
    { 200.0f, 1.0f, 200.0f, 1.0f },
    { 250.0f, 1.0f, 250.0f, 1.0f },
    { 275.0f, 1.0f, 275.0f, 1.0f },
    { 300.0f, 1.0f, 300.0f, 1.0f },
};

static struct Motion_Table_Infomeation _motion_info[4] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.4f, 0.5f, 0.3f },
    { 0.8f, 0.5f, 0.5f },
    { 1.2f, 0.5f, 0.2f },
};


/* Scales the matrix by a per-axis factor. */
static inline void Matrix_Scale(float m[4][4], float *scale) {
    float sm[4][4];
    int i;

    sceVu0UnitMatrix(sm);
    for (i = 0; i < 3; i++) {
        sm[i][i] *= scale[i];
    }
    sceVu0MulMatrix(m, m, sm);
}

static unsigned int Object_Initialize(struct HH_Object_Water_Pool_00 *pThis) {
    unsigned int result;
    float radian;

    pThis->Texture_Kind = (unsigned char)(rand() % 16U);
    pThis->Scale_Kind = rand() % 3U;
    pThis->Rgba_Kind = rand() % 1U;
    pThis->Timer = 0.0f;
    radian = (rand() % 360) * 0.017453292f;
    pThis->Rotate_Y = Radian_Normalize(radian);
    pThis->Motion_Step = 0;
    result = 1;
    return result;
}

static unsigned int Object_Motion_00(struct HH_Object_Water_Pool_00 *pThis) {
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
    sceVu0InterVector(pThis->Rgba, pRgba_e, pRgba_s, rgba_ratio);
    result = 0;
    return result;
}

static unsigned int Object_Draw(struct HH_Object_Water_Pool_00 *pThis, struct ImpactQueue_Element *pElement) {
    static float color_scale;
    unsigned int result;
    sceVif1Packet *pPk;
    float (*pVertex)[4];
    float (*pStq)[4];
    unsigned int vertex_num;
    unsigned int prim_type;
    unsigned int i;
    float lwm[4][4];
    float lsm[4][4];
    float clip_mat[4][4];
    float Rgba[4];
    float pos[4];
    float dir[4];
    int rgba[4];
    int xyzf[4];
    float q;
    unsigned int addr;
    float spot_param; /* @bug never set (not in the DWARF); the original passes whatever is in f20 */

    pPk = HH_Vif1Packet_Current_Get();
    pVertex = _square_00_vertex;
    pStq = _square_stq_list[pThis->Texture_Kind];
    if (HH_ClassWrapper_SpotLight_Enable_Check()) {
        color_scale = HH_ClassWrapper_SpotLight_ColorRatio_Calculator(pos, dir, lwm[3], spot_param, spot_param);
        color_scale = HH_ClassWrapper_Float_Clamp(color_scale, 0.2f, 1.0f);
    } else {
        color_scale = 0.2f;
    }
    HH_ClassWrapper_WorldScreenMatrix_Get(lsm);
    sceVu0UnitMatrix(lwm);
    Matrix_Scale(lwm, pThis->Scale);
    sceVu0RotMatrixY(lwm, lwm, pThis->Rotate_Y);
    sceVu0TransMatrix(lwm, lwm, pElement->Option.Vector[0]);
    sceVu0MulMatrix(lsm, lsm, lwm);
    HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(clip_mat);
    sceVu0MulMatrix(clip_mat, clip_mat, lwm);
    HH_Vif1PacketBuffer_GifTag_Open();
    prim_type = GS_PRIM_TRISTRIP;
    vertex_num = 4;
    sceVif1PkAddGsAD(pPk, GS_REG_PRIM, GS_SET_PRIM(prim_type, 1, 1, 0, 1, 0, 0, 0, 0));
    sceVu0CopyVector(Rgba, pThis->Rgba);
    Rgba[3] *= color_scale;
    sceVu0ClampVector(Rgba, Rgba, 0.0f, 255.0f);
    sceVu0FTOI0Vector(rgba, Rgba);
    for (i = 0; i < vertex_num; i++) {
        addr = GS_REG_XYZF2;
        if (HH_ClassWrapper_RotTrans_PerspectiveProjection_Clip(xyzf, &q, lsm, clip_mat, pVertex[i])) {
            addr = GS_REG_XYZF3;
        }
        sceVif1PkAddGsAD(pPk, GS_REG_RGBAQ, GS_SET_RGBAQ(rgba[0], rgba[1], rgba[2], rgba[3], Float_Bits(q)));
        sceVif1PkAddGsAD(pPk, GS_REG_ST, GS_SET_ST(Float_Bits(pStq[i][0] * q), Float_Bits(pStq[i][1] * q)));
        sceVif1PkAddGsAD(pPk, addr, GS_SET_XYZF(xyzf[0], xyzf[1], xyzf[2] + 10, xyzf[3]));
    }
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 0;
    return result;
}

/**
 * Class prefix, run once a frame before this class's instances: sets the GS TEX0 and ALPHA
 * registers they draw with. Returns 1.
 */
unsigned int HH_Class_Prefix_Water_Pool_00(void) {
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned long tex0;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    tex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(0x10, 0);
    sceVif1PkAddGsAD(pPk, GS_REG_TEX0_1, tex0);
    sceVif1PkAddGsAD(pPk, GS_REG_ALPHA_1, GS_SET_ALPHA(0, 1, 0, 1, 0x80));
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class suffix, run once a frame after this class's instances: does nothing. Returns 1.
 */
unsigned int HH_Class_Suffix_Water_Pool_00(void) {
    unsigned int result;

    result = 1;
    return result;
}

/**
 * Class main: runs the puddle's spread-and-fade motion table and draws it; ends when the
 * motion is over.
 * @param pBlock   the instance's data block (struct HH_Object_Water_Pool_00)
 * @param pElement the element that created it; Option.Vector[0] is the puddle's position
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Water_Pool_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Water_Pool_00 *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis);
        pThis->Step = 1;
        break;
    case 1:
        Object_Motion_00(pThis);
        if (pThis->Motion_Step > 2) {
            pThis->Rgba[3] = 0.0f;
            pThis->Step = 2;
        }
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
