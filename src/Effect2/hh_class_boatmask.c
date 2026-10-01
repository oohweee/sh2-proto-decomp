/*
 * hh_class_boatmask.c: a grid mesh drawn along the boat to mask the water surface inside it.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Boat_Mask runs once a
 * frame for each instance, between the class's prefix and suffix.
 */
#include "sh2.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

static float _square_00_vertex[4][4] = {
    { 275.0f, -392.0f, -2005.0f, 1.0f },
    { 245.0f, -392.0f, 1990.0f, 1.0f },
    { 275.0f, 392.0f, -2005.0f, 1.0f },
    { 245.0f, 392.0f, 1990.0f, 1.0f },
};

static float _square_stq[4][4] = {
    { 0.4f, 1.0f, 1.0f, 0.0f },
    { 0.4f, 0.0f, 1.0f, 0.0f },
    { 0.6f, 1.0f, 1.0f, 0.0f },
    { 0.6f, 0.0f, 1.0f, 0.0f },
};

static unsigned int Object_Initialize(struct HH_Object_Boat_Mask *pThis) {
    unsigned int result;

    pThis->Timer = 0.0f;
    result = 1;
    return result;
}

static unsigned int Object_Draw(void) {
    unsigned int result;
    sceVif1Packet *pPk = HH_Vif1Packet_Current_Get();
    float (*pVertex)[4] = _square_00_vertex;
    float Rgba[4] = { 128.0f, 128.0f, 128.0f, 255.0f };
    float (*pStq)[4] = _square_stq;
    unsigned int vertex_num;
    unsigned int v;
    unsigned int i;
    unsigned int j;
    float lwm[4][4];
    float lsm[4][4];
    float clip_mat[4][4];
    float stq[4];
    int xyzf[4];
    int rgba[4];
    float x_w;
    float z_w;
    float s_w;
    float t_w;
    struct SubCharacter *pSub_Char;
    float _lwm[4][4];
    static unsigned int div_i = 4;
    static unsigned int div_j = 8;

    x_w = (pVertex[3][1] - pVertex[0][1]) / div_i;
    z_w = (pVertex[0][2] - pVertex[3][2]) / div_j;
    s_w = (pStq[2][0] - pStq[1][0]) / div_i;
    t_w = (pStq[2][1] - pStq[1][1]) / div_j;
    pSub_Char = shCharacterGetSubCharacter(0x10B, -1);
    if (pSub_Char == NULL) {
        result = 0;
        return result;
    }
    HH_ClassWrapper_WorldScreenMatrix_Get(lsm);
    sceVu0UnitMatrix(lwm);
    shCharacterGetPartsMatrixForShadow(_lwm, 0x10B, 0, 0);
    sceVu0MulMatrix(lwm, lwm, _lwm);
    sceVu0MulMatrix(lsm, lsm, lwm);
    HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(clip_mat);
    sceVu0MulMatrix(clip_mat, clip_mat, lwm);
    HH_Vif1PacketBuffer_GifTag_Open();
    sceVu0FTOI0Vector(rgba, Rgba);
    for (j = 0; j < div_j; j++) {
        float vec[2][4] = {
            { 0.0f, 0.0f, z_w * j, 1.0f },
            { 0.0f, 0.0f, z_w * (j + 1), 1.0f },
        };
        float stq_list[2][4] = {
            { pStq[1][0], t_w * j, 1.0f, 0.0f },
            { pStq[1][0], t_w * (j + 1), 1.0f, 0.0f },
        };

        sceVu0AddVector(vec[0], vec[0], pVertex[1]);
        sceVu0AddVector(vec[1], vec[1], pVertex[1]);
        HH_Vif1Packet_GeneralGifTag_TriangleStrip_Open();
        for (i = 0; i <= div_i; i++) {
            float now_x;
            float now_s;

            now_x = x_w * i;
            now_s = s_w * i;
            vec[0][1] += now_x;
            vec[1][1] += now_x;
            stq_list[0][0] += now_s;
            stq_list[1][0] += now_s;
            for (v = 0; v < 2; v++) {
                sceVu0CopyVector(stq, stq_list[v]);
                HH_ClassWrapper_Transform_PerspectiveProjection_Clip_forTriangleStrip(xyzf, stq, lsm, clip_mat, vec[v]);
                ((u_long128 *)pPk->pCurrent)[0] = *(u_long128 *)stq;
                ((u_long128 *)pPk->pCurrent)[1] = *(u_long128 *)rgba;
                ((u_long128 *)pPk->pCurrent)[2] = *(u_long128 *)xyzf;
                pPk->pCurrent += 12;
            }
        }
        sceVif1PkCloseGifTag(pPk);
    }
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 0;
    return result;
}

/**
 * Class prefix, run once a frame before this class's instances: sets the GS FRAME, TEX0, ALPHA and
 * TEST registers they draw with. Returns 1.
 */
unsigned int HH_Class_Prefix_Boat_Mask(void) {
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned long tex0;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    *(u_long128 *)pPk->pCurrent = *HH_ClassWrapper_GS_EnvironmentRegister_Frame_AlphaMask_Get();
    pPk->pCurrent[1] = 0;
    pPk->pCurrent += 4;
    tex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(20, 0);
    sceVif1PkAddGsAD(pPk, GS_REG_TEX0_1, tex0 & 0xFFFFFFFFFFFFFFFF);
    sceVif1PkAddGsAD(pPk, GS_REG_ALPHA_1, GS_SET_ALPHA(0, 2, 0, 1, 0x80));
    sceVif1PkAddGsAD(pPk, GS_REG_TEST_1, 0x30002);
    sceVif1PkCloseGifTag(pPk);
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class suffix, run once a frame after this class's instances: sets the GS FRAME and TEST registers
 * back. Returns 1.
 */
unsigned int HH_Class_Suffix_Boat_Mask(void) {
    unsigned int result;
    sceVif1Packet *pPk;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    HH_Vif1Packet_GeneralGifTag_GS_AD_Open();
    *(u_long128 *)pPk->pCurrent = *HH_ClassWrapper_GS_EnvironmentRegister_Frame_AlphaMask_Get();
    pPk->pCurrent += 4;
    sceVif1PkAddGsAD(pPk, GS_REG_TEST_1, 0x50002);
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class main: draws the boat mask every frame until the instance is disabled.
 * @param pBlock   the instance's data block (struct HH_Object_Boat_Mask)
 * @param pElement the element that created it (unused)
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Boat_Mask(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Boat_Mask *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis);
        pThis->Step = 1;
        break;
    case 1:
        Object_Draw();
        break;
    case 2:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    return result;
}
