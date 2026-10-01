/*
 * hh_class_blood_footmark_00.c: a bloody footprint on the ground that fades out.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Blood_FootMark_00 runs
 * once a frame for each instance, between the class's prefix and suffix.
 *
 * Vertex_Infomeation_List is defined differently in several files (the DWARF has a struct of
 * that name in each, not all alike), so sh2/types.h only declares it and each file defines its
 * own version, this one below.
 * Here its lists are vertices, RGBAs and a table of STQ lists (five members).
 */
#include "sh2.h"
#include "libc/stdlib.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

struct Vertex_Infomeation_List {
    float (*pVertex_List)[4];
    float (*pRgba_List)[4];
    float (**pStq_List)[4];
    unsigned int Vertex_Max;
    unsigned int Primitive_Type;
};

static float _square_00_vertex[4][4] = {
    { -110.0f, 0.0f, 120.0f, 1.0f },
    { -110.0f, 0.0f, -120.0f, 1.0f },
    { 110.0f, 0.0f, 120.0f, 1.0f },
    { 110.0f, 0.0f, -120.0f, 1.0f },
};

static float _square_00_stq[4][4] = {
    { 0.125f, 0.75f, 1.0f, 0.0f },
    { 0.125f, 0.875f, 1.0f, 0.0f },
    { 0.25f, 0.75f, 1.0f, 0.0f },
    { 0.25f, 0.875f, 1.0f, 0.0f },
};

static float _square_01_stq[4][4] = {
    { 0.125f, 0.875f, 1.0f, 0.0f },
    { 0.125f, 1.0f, 1.0f, 0.0f },
    { 0.25f, 0.875f, 1.0f, 0.0f },
    { 0.25f, 1.0f, 1.0f, 0.0f },
};

static float _square_10_stq[4][4] = {
    { 0.0f, 0.75f, 1.0f, 0.0f },
    { 0.0f, 0.875f, 1.0f, 0.0f },
    { 0.125f, 0.75f, 1.0f, 0.0f },
    { 0.125f, 0.875f, 1.0f, 0.0f },
};

static float _square_11_stq[4][4] = {
    { 0.0f, 0.875f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 1.0f, 0.0f },
    { 0.125f, 0.875f, 1.0f, 0.0f },
    { 0.125f, 1.0f, 1.0f, 0.0f },
};

static sceVu0FVECTOR *_square_0x_stq_list[2] = { _square_00_stq, _square_01_stq };
static sceVu0FVECTOR *_square_1x_stq_list[2] = { _square_10_stq, _square_11_stq };

static float _square_00_rgba[4][4] = {
    { 64.0f, 150.0f, 120.0f, 128.0f },
    { 64.0f, 150.0f, 120.0f, 128.0f },
    { 64.0f, 150.0f, 120.0f, 128.0f },
    { 64.0f, 150.0f, 120.0f, 128.0f },
};

static struct Vertex_Infomeation_List _vertex_info_list[2] = {
    { _square_00_vertex, _square_00_rgba, _square_0x_stq_list, 4, 4 },
    { _square_00_vertex, _square_00_rgba, _square_1x_stq_list, 4, 4 },
};

static unsigned int Object_Initialize(struct HH_Object_Blood_FootMark_00 *pThis) {
    unsigned int result;

    pThis->Timer = 0.0f;
    pThis->Vertex_Kind = rand() % 2U;
    result = 1;
    return result;
}

static unsigned int Object_Draw(struct HH_Object_Blood_FootMark_00 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    sceVif1Packet *pPk;
    struct Vertex_Infomeation_List *pInfo;
    float (*pVertex)[4];
    float (*pRgba)[4];
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
    pInfo = &_vertex_info_list[pElement->Option.Int_Value[0]];
    pVertex = pInfo->pVertex_List;
    pRgba = pInfo->pRgba_List;
    pStq = pInfo->pStq_List[pThis->Vertex_Kind];
    vertex_num = pInfo->Vertex_Max;
    sceVu0CopyVector(Rgba, pRgba[0]);
    Rgba[3] *= pElement->Option.Float_Value[1];
    HH_ClassWrapper_WorldScreenMatrix_Get(lsm);
    sceVu0UnitMatrix(lwm);
    sceVu0RotMatrixY(lwm, lwm, pElement->Option.Float_Value[0]);
    sceVu0TransMatrix(lwm, lwm, pElement->Option.Vector[0]);
    sceVu0MulMatrix(lsm, lsm, lwm);
    HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(clip_mat);
    sceVu0MulMatrix(clip_mat, clip_mat, lwm);
    HH_Vif1PacketBuffer_GifTag_Open();
    HH_Vif1Packet_GeneralGifTag_TriangleStrip_Open();
    for (i = 0; i < vertex_num; i++) {
        sceVu0CopyVector(stq, pStq[i]);
        HH_ClassWrapper_Transform_PerspectiveProjection_Clip_forTriangleStrip(xyzf, stq, lsm, clip_mat, pVertex[i]);
        if (i == 0) {
            ratio = HH_ClassWrapper_FogParameter_A_Get() + stq[2] * HH_ClassWrapper_FogParameter_B_Get();
            ratio = HH_ClassWrapper_Float_Clamp(ratio, 0.0f, 255.0f) / 255.0f;
            sceVu0ScaleVector(Rgba, Rgba, ratio);
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
 * registers they draw with. Returns 1.
 */
unsigned int HH_Class_Prefix_Blood_FootMark_00(void) {
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
unsigned int HH_Class_Suffix_Blood_FootMark_00(void) {
    unsigned int result;

    HH_Vif1Packet_GeneralGifTag_GS_AD_Open();
    result = 1;
    return result;
}

/**
 * Class main: draws the footprint; after 10 seconds fades it out and ends the instance.
 * @param pBlock   the instance's data block (struct HH_Object_Blood_FootMark_00)
 * @param pElement the element that created it; Option.Vector[0] is the position,
 *                 Float_Value[0] the Y rotation, Float_Value[1] the opacity (lowered as it
 *                 fades) and Int_Value[0] the footprint shape
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Blood_FootMark_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Blood_FootMark_00 *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis);
        pThis->Step = 1;
        break;
    case 1:
        Object_Draw(pThis, pElement);
        pThis->Timer += 1.0f / 30.0f;
        if (pThis->Timer > 10.0f) {
            pElement->Option.Float_Value[1] -= 0.01f;
            if (pElement->Option.Float_Value[1] <= 0.0f) {
                pElement->Option.Float_Value[1] = 0.0f;
                pThis->Step = 2;
            }
        }
        break;
    case 2:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    return result;
}
