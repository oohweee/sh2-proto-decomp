/*
 * hh_class_glass_piece_00.c: a spinning piece of glass thrown along a parabola, bouncing off the
 * floor.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_GlassPiece_00 runs once a
 * frame for each instance, between the class's prefix and suffix.
 *
 * Vertex_Infomeation_List is defined differently in several files (the DWARF has a struct of
 * that name in each, not all alike), so sh2/types.h only declares it and each file defines its
 * own version, this one below.
 * Here its vertex and normal pointers are to the 16-byte aligned vector typedef (common.h's
 * sceVu0FVECTOR), which makes MWCC align the struct (and arrays of it) to 16.
 */
#include "sh2.h"
#include "hh_math.h"
#include "hh_vector.h"
#include "libc/math.h"
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

static float _elastic_vec[4] = { 1.0f, -0.8f, 1.0f, 1.0f };
static float _friction_vec[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
static float _square_00_vertex[4][4] = {
    { 20.0f, 20.0f, 0.0f, 1.0f },
    { 20.0f, -20.0f, 0.0f, 1.0f },
    { -20.0f, 20.0f, 0.0f, 1.0f },
    { -20.0f, -20.0f, 0.0f, 1.0f },
};

static float _square_01_vertex[4][4] = {
    { 20.0f, 0.0f, 20.0f, 1.0f },
    { 20.0f, 0.0f, -20.0f, 1.0f },
    { -20.0f, 0.0f, 20.0f, 1.0f },
    { -20.0f, 0.0f, -20.0f, 1.0f },
};

static float _square_00_normal[4][4] = {
    { 0.0f, 0.0f, 1.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 1.0f },
};

static float _square_01_normal[4][4] = {
    { 0.0f, -1.0f, 0.0f, 1.0f },
    { 0.0f, -1.0f, 0.0f, 1.0f },
    { 0.0f, -1.0f, 0.0f, 1.0f },
    { 0.0f, -1.0f, 0.0f, 1.0f },
};

static float _square_stq_00[4][4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.060546875f, 1.0f, 0.0f },
    { 0.060546875f, 0.0f, 1.0f, 0.0f },
    { 0.060546875f, 0.060546875f, 1.0f, 0.0f },
};

static float _square_stq_01[4][4] = {
    { 0.0f, 0.0625f, 1.0f, 0.0f },
    { 0.0f, 0.123046875f, 1.0f, 0.0f },
    { 0.060546875f, 0.0625f, 1.0f, 0.0f },
    { 0.060546875f, 0.123046875f, 1.0f, 0.0f },
};

static float _square_stq_02[4][4] = {
    { 0.0f, 0.125f, 1.0f, 0.0f },
    { 0.0f, 0.18554688f, 1.0f, 0.0f },
    { 0.060546875f, 0.125f, 1.0f, 0.0f },
    { 0.060546875f, 0.18554688f, 1.0f, 0.0f },
};

static float _square_stq_10[4][4] = {
    { 0.0625f, 0.0f, 1.0f, 0.0f },
    { 0.0625f, 0.060546875f, 1.0f, 0.0f },
    { 0.123046875f, 0.0f, 1.0f, 0.0f },
    { 0.123046875f, 0.060546875f, 1.0f, 0.0f },
};

static float _square_stq_11[4][4] = {
    { 0.0625f, 0.0625f, 1.0f, 0.0f },
    { 0.0625f, 0.123046875f, 1.0f, 0.0f },
    { 0.123046875f, 0.0625f, 1.0f, 0.0f },
    { 0.123046875f, 0.123046875f, 1.0f, 0.0f },
};

static float _square_stq_12[4][4] = {
    { 0.0625f, 0.125f, 1.0f, 0.0f },
    { 0.0625f, 0.18554688f, 1.0f, 0.0f },
    { 0.123046875f, 0.125f, 1.0f, 0.0f },
    { 0.123046875f, 0.18554688f, 1.0f, 0.0f },
};

static float _square_stq_20[4][4] = {
    { 0.125f, 0.0f, 1.0f, 0.0f },
    { 0.125f, 0.060546875f, 1.0f, 0.0f },
    { 0.18554688f, 0.0f, 1.0f, 0.0f },
    { 0.18554688f, 0.060546875f, 1.0f, 0.0f },
};

static float _square_stq_21[4][4] = {
    { 0.125f, 0.0625f, 1.0f, 0.0f },
    { 0.125f, 0.123046875f, 1.0f, 0.0f },
    { 0.18554688f, 0.0625f, 1.0f, 0.0f },
    { 0.18554688f, 0.123046875f, 1.0f, 0.0f },
};

static float _square_stq_22[4][4] = {
    { 0.125f, 0.125f, 1.0f, 0.0f },
    { 0.125f, 0.18554688f, 1.0f, 0.0f },
    { 0.18554688f, 0.125f, 1.0f, 0.0f },
    { 0.18554688f, 0.18554688f, 1.0f, 0.0f },
};

static sceVu0FVECTOR *_stq_list[9] = {
    _square_stq_00, _square_stq_01, _square_stq_02, _square_stq_10,
    _square_stq_11, _square_stq_12, _square_stq_20, _square_stq_21,
    _square_stq_22,
};

static struct Vertex_Infomeation_List _vertex_info_list[2] = {
    { _square_00_vertex, _square_00_normal, 4, 4 },
    { _square_01_vertex, _square_01_normal, 4, 4 },
};


static unsigned int Object_Initialize(struct HH_Object_GlassPiece_00 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    float *src_direction;
    float theta;
    float phai;

    pThis->Timer = 0.0f;
    src_direction = pElement->Option.Vector[1];
    pThis->Verocity_0[0] = src_direction[0];
    pThis->Verocity_0[1] = src_direction[1];
    pThis->Verocity_0[2] = src_direction[2];
    pThis->Verocity_0[3] = 2450.0f;
    if (pElement->Option.Int_Value[0] == 0) {
        theta = (rand() % 360) * 0.017453292f - 3.1415927f;
        phai = (rand() % 360) * 0.017453292f - 3.1415927f;
        theta *= 15.0f;
        phai *= 15.0f;
        pThis->Rotate[0] = theta;
        pThis->Rotate[1] = phai;
        pThis->Vertex_Kind = rand() % 2U;
        pThis->Texture_Kind = rand() % 9U;
    }
    result = 0;
    return result;
}

static void ParabolaMotion_Calculator(struct HH_Object_GlassPiece_00 *pThis, struct ImpactQueue_Element *pElement, float Time, float *Position) {
    float time_mat[4][4];

    sceVu0UnitMatrix(time_mat);
    time_mat[0][0] = Time;
    time_mat[1][1] = Time;
    time_mat[2][2] = Time;
    time_mat[3][1] = Time * Time;
    time_mat[3][3] = 0.0f;
    sceVu0ApplyMatrix(Position, time_mat, pThis->Verocity_0);
}

static void SpecularRGBA_Calculator(int *iRGBA, float *RGBA_Base, float *RGBA_Specular_Base, float *Normal_Vector);

static unsigned int Object_Draw(struct HH_Object_GlassPiece_00 *pThis, struct ImpactQueue_Element *pElement, float *Current_Position) {
    static float Specular_Rgba[4] = { 128.0f, 128.0f, 128.0f, 128.0f };
    static float Base_Rgba[4] = { 0.0f, 0.0f, 0.0f, 128.0f };
    unsigned int result;
    sceVif1Packet *pPk;
    struct Vertex_Infomeation_List *pInfo;
    float (*pVertex)[4];
    float (*pNormal)[4];
    unsigned int vertex_num;
    unsigned int prim_type;
    unsigned int i;
    float lwm[4][4];
    float lsm[4][4];
    float clip_mat[4][4];
    int rgba[4];
    float normal[4];
    float (*pStq)[4];
    float time;
    float rot_x;
    float rot_y;
    int xyzf[4];
    float q;
    unsigned int addr;

    pPk = HH_Vif1Packet_Current_Get();
    pInfo = &_vertex_info_list[pThis->Vertex_Kind];
    pVertex = pInfo->pVertex_List;
    pNormal = pInfo->pNormal_List;
    vertex_num = pInfo->Vertex_Max;
    prim_type = pInfo->Primitive_Type;
    pStq = _stq_list[pThis->Texture_Kind];
    HH_ClassWrapper_WorldScreenMatrix_Get(lsm);
    sceVu0UnitMatrix(lwm);
    time = pThis->Timer + pElement->Option.Float_Value[0];
    rot_x = Radian_Normalize(pThis->Rotate[0] * time);
    rot_y = Radian_Normalize(pThis->Rotate[1] * time);
    sceVu0RotMatrixY(lwm, lwm, rot_y);
    sceVu0RotMatrixX(lwm, lwm, rot_x);
    sceVu0TransMatrix(lwm, lwm, pElement->Option.Vector[0]);
    sceVu0TransMatrix(lwm, lwm, Current_Position);
    sceVu0MulMatrix(lsm, lsm, lwm);
    HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(clip_mat);
    sceVu0MulMatrix(clip_mat, clip_mat, lwm);
    HH_Vif1PacketBuffer_GifTag_Open();
    sceVif1PkAddGsAD(pPk, GS_REG_PRIM, GS_SET_PRIM(prim_type, 1, 1, 0, 1, 0, 0, 0, 0));
    lwm[3][0] = 0.0f;
    lwm[3][1] = 0.0f;
    lwm[3][2] = 0.0f;
    sceVu0ApplyMatrix(normal, lwm, pNormal[0]);
    SpecularRGBA_Calculator(rgba, Base_Rgba, Specular_Rgba, normal);
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

static float Specular_Calculator(float *Normal_Vector) {
    static float cos_beta_min = 0.996194f;
    float result;
    float specular_coefficient;
    float input_light_power;
    float revers_light_dir[4];
    float light_dir[4];
    float view_dir[4];
    float tmp_vec[4];
    float cos_theta;
    float cos_beta;

    HH_ClassWrapper_LightDirection_Get(light_dir);
    sceVu0Normalize(light_dir, light_dir);
    HH_ClassWrapper_ViewDirection_Get(view_dir);
    sceVu0Normalize(view_dir, view_dir);
    cos_theta = sceVu0InnerProduct(light_dir, Normal_Vector);
    if (cos_theta < 0.0f) {
        cos_theta = -cos_theta;
    }
    sceVu0ScaleVectorXYZ(tmp_vec, Normal_Vector, 2.0f * cos_theta);
    sceVu0SubVector(revers_light_dir, tmp_vec, light_dir);
    sceVu0Normalize(revers_light_dir, revers_light_dir);
    cos_beta = sceVu0InnerProduct(revers_light_dir, view_dir);
    if (cos_beta < 0.0f) {
        cos_beta = -cos_beta;
    }
    input_light_power = 1.0f;
    specular_coefficient = (cos_beta < cos_beta_min) ? 0.0f : 1.0f - (1.0f - cos_beta) / (1.0f - cos_beta_min);
    result = specular_coefficient * input_light_power;
    return result;
}

static void SpecularRGBA_Calculator(int *iRGBA, float *RGBA_Base, float *RGBA_Specular_Base, float *Normal_Vector) {
    float specular;

    specular = Specular_Calculator(Normal_Vector);
    __asm__ __volatile__("
    lqc2        vf30, 0x0(%1)
    lqc2        vf31, 0x0(%2)
    mfc1        t0, %3
    qmtc2.ni    t0, vf29
    vmulx.w     vf31, vf31, vf29x
    vadd.xyzw   vf31, vf30, vf31
    vftoi0.xyzw vf29, vf31
    sqc2        vf29, 0x0(%0)
    " : : "r"(iRGBA), "r"(RGBA_Base), "r"(RGBA_Specular_Base), "f"(specular));
}

/**
 * Class prefix, run once a frame before this class's instances: sets the GS TEX0, CLAMP and ALPHA
 * registers they draw with. Returns 1.
 */
unsigned int HH_Class_Prefix_GlassPiece_00(void) {
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned long tex0;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    tex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(0x11, 0);
    sceVif1PkAddGsAD(pPk, GS_REG_TEX0_1, tex0);
    sceVif1PkAddGsAD(pPk, GS_REG_CLAMP_1, 0x1FF001FFF);
    sceVif1PkAddGsAD(pPk, GS_REG_ALPHA_1, GS_SET_ALPHA(0, 1, 0, 1, 0x80));
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class suffix, run once a frame after this class's instances: does nothing. Returns 1.
 */
unsigned int HH_Class_Suffix_GlassPiece_00(void) {
    unsigned int result;

    result = 1;
    return result;
}

/**
 * Class main: throws the piece along a parabola, spinning. When it hits the floor fast enough
 * it bounces (a new parabola from the hit point, with elasticity and friction applied);
 * otherwise it comes to rest there and is drawn lying on the floor.
 * @param pBlock   the instance's data block (struct HH_Object_GlassPiece_00)
 * @param pElement the element that created it; Option.Vector[0] is the start position,
 *                 Vector[1] the initial velocity (both updated on a bounce), Int_Value[0] is
 *                 set after the first bounce and Float_Value[0] accumulates the flight time
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_GlassPiece_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_GlassPiece_00 *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis, pElement);
        pThis->Step++;
        break;
    case 1: {
        float position[4];
        float *src_location;
        float *src_direction;
        float dir_vec[4];
        float volume;
        float reflection_vec[4];
        float ref_normalize[4];
        float ref_xz_volume;
        float tan_value;
        float y;
        float verocity_xz[4];
        float v_xz_volume;

        src_location = pElement->Option.Vector[0];
        src_direction = pElement->Option.Vector[1];
        ParabolaMotion_Calculator(pThis, pElement, pThis->Timer, position);
        if (position[1] + src_location[1] > 0.0f) {
            ParabolaMotion_Calculator(pThis, pElement, pThis->Timer - 1.0f / 30.0f, dir_vec);
            sceVu0SubVector(dir_vec, position, dir_vec);
            volume = sceVu0InnerProduct(dir_vec, dir_vec);
            if (volume > 625.0f) {
                sceVu0AddVector(src_location, src_location, position);
                src_location[1] = 0.0f;
                sceVu0MulVector(reflection_vec, dir_vec, _elastic_vec);
                sceVu0Normalize(ref_normalize, reflection_vec);
                y = ref_normalize[1];
                ref_normalize[1] = 0.0f;
                ref_xz_volume = sceVu0InnerProduct(ref_normalize, ref_normalize);
                tan_value = y / HH_MathWrapper_Sqrtf(ref_xz_volume);
                sceVu0CopyVector(verocity_xz, pThis->Verocity_0);
                verocity_xz[1] = 0.0f;
                verocity_xz[3] = 0.0f;
                v_xz_volume = sceVu0InnerProduct(verocity_xz, verocity_xz);
                v_xz_volume = HH_MathWrapper_Sqrtf(v_xz_volume);
                src_direction[1] = tan_value * v_xz_volume;
                sceVu0MulVector(src_direction, src_direction, _friction_vec);
                Vector_Zero(position);
                pElement->Option.Int_Value[0] = 1;
                pElement->Option.Float_Value[0] += pThis->Timer;
                pThis->Step = 0;
            } else {
                position[1] = -src_location[1];
                pThis->Step = 2;
            }
        }
        Object_Draw(pThis, pElement, position);
        pThis->Timer += 1.0f / 30.0f;
        break;
    }
    case 2: {
        float position[4];
        float *src_location;

        src_location = pElement->Option.Vector[0];
        ParabolaMotion_Calculator(pThis, pElement, pThis->Timer, position);
        position[1] = -src_location[1];
        Object_Draw(pThis, pElement, position);
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
