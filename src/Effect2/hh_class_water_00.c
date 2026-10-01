/*
 * hh_class_water_00.c: a rippling 48x48 water surface driven by a two-buffer wave equation, lit by
 * the spotlight.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Water_00 runs once a
 * frame for each instance, between the class's prefix and suffix.
 */
#include "sh2.h"
#include "libc/string.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

#define GRID_X_MAX 48
#define GRID_Z_MAX 48
#define GRID_WH 500.0f

static struct Wave_Element *Free_WaveElement_Search(struct HH_Object_Water_00 *pThis) {
    struct Wave_Element *result;
    unsigned int i;
    struct Wave_Element *pElement;

    result = NULL;
    for (i = 0; i < 4; i++) {
        pElement = &pThis->Wave_Info[i];
        if (!pElement->Enable) {
            result = pElement;
            break;
        }
    }
    return result;
}

static struct Wave_Element *Oldest_WaveElement_Search(struct HH_Object_Water_00 *pThis) {
    struct Wave_Element *result;
    unsigned int i;
    float time;
    struct Wave_Element *pElement;

    result = NULL;
    time = 0.0f;
    for (i = 0; i < 4; i++) {
        pElement = &pThis->Wave_Info[i];
        if (pElement->Enable) {
            if (time < pElement->Timer) {
                time = pElement->Timer;
                result = pElement;
            }
        }
    }
    return result;
}

static unsigned int WaveElement_Addition(struct HH_Object_Water_00 *pThis, struct Wave_Element *pElement) {
    unsigned int result;
    struct Wave_Element *pFree_Element;

    pFree_Element = Free_WaveElement_Search(pThis);
    if (pFree_Element == NULL) {
        pFree_Element = Oldest_WaveElement_Search(pThis);
    }
    *pFree_Element = *pElement;
    result = 1;
    return result;
}

static void Grid_Work_Initialize(struct HH_Object_Water_00 *pThis) {
    memset(pThis->Grid_Y_Value, 0, sizeof(pThis->Grid_Y_Value));
}

static unsigned int Object_Initialize(struct HH_Object_Water_00 *pThis) {
    unsigned int result;

    pThis->Timer = 0.0f;
    pThis->Motion_Step = 0;
    {
        struct Wave_Element wave_element;

        wave_element.Enable = 1;
        wave_element.Impact_Grid_Index[0] = 24;
        wave_element.Impact_Grid_Index[1] = 24;
        wave_element.Max_Distance0 = 1800.0f;
        wave_element.Lost_Time = 15.0f;
        wave_element.Arrival = 50.0f;
        wave_element.Omega = 540.0f * 0.017453292f;
        wave_element.Verocity = 400.0f;
        WaveElement_Addition(pThis, &wave_element);
    }
    {
        struct Wave_Element wave_element;

        wave_element.Enable = 1;
        wave_element.Impact_Grid_Index[0] = 38;
        wave_element.Impact_Grid_Index[1] = 38;
        wave_element.Max_Distance0 = 1200.0f;
        wave_element.Lost_Time = 12.0f;
        wave_element.Arrival = 40.0f;
        wave_element.Omega = 612.0f * 0.017453292f;
        wave_element.Verocity = 500.0f;
        WaveElement_Addition(pThis, &wave_element);
    }
    {
        struct Wave_Element wave_element;

        wave_element.Enable = 1;
        wave_element.Impact_Grid_Index[0] = 10;
        wave_element.Impact_Grid_Index[1] = 38;
        wave_element.Max_Distance0 = 1500.0f;
        wave_element.Lost_Time = 12.0f;
        wave_element.Arrival = 50.0f;
        wave_element.Omega = 900.0f * 0.017453292f;
        wave_element.Verocity = 300.0f;
        WaveElement_Addition(pThis, &wave_element);
    }
    result = 1;
    return result;
}

/*
 * pCurr_Grid_Y_C and pPrev_Grid_Y_C are set but unused: the grids go through pCurr_Grid_Y and
 * pPrev_Grid_Y (Matching: the original's addu offset, base, and its DWARF's v0 for both).
 */
static unsigned int Object_Motion_00(struct HH_Object_Water_00 *pThis) {
    static float dev_0 = 2.0f;
    static float dev_1 = 16.0f;
    unsigned int result;
    float *pCurr_Grid_Y;
    float *pPrev_Grid_Y;
    unsigned int x_index;
    unsigned int z_index;
    float *pCurr_Grid_Y_C;
    float *pPrev_Grid_Y_C;
    unsigned int table_index;
    float depth;

    pCurr_Grid_Y = pThis->Grid_Y_Value[pThis->CurrentBuffer];
    pPrev_Grid_Y = pThis->Grid_Y_Value[pThis->CurrentBuffer ^ 1];
    for (z_index = 1; z_index < GRID_Z_MAX - 1; z_index++) {
        for (x_index = 1; x_index < GRID_X_MAX - 1; x_index++) {
            table_index = x_index + z_index * GRID_X_MAX;
            pCurr_Grid_Y_C = &pCurr_Grid_Y[table_index];
            depth = pCurr_Grid_Y[table_index - 1];
            depth += pCurr_Grid_Y[table_index + 1];
            depth += pCurr_Grid_Y[x_index + (z_index - 1) * GRID_X_MAX];
            depth += pCurr_Grid_Y[x_index + (z_index + 1) * GRID_X_MAX];
            depth /= dev_0;
            pPrev_Grid_Y_C = &pPrev_Grid_Y[table_index];
            depth -= pPrev_Grid_Y[table_index];
            depth -= depth / dev_1;
            pPrev_Grid_Y[table_index] = depth;
        }
    }
    result = 0;
    return result;
}

static float Specular_Calculator(float *View_Direction, float *Light_Direction, float *Normal_Vector) {
    float result;
    float specular_coefficient;
    float input_light_power;
    float reverse_light_dir[4];
    float tmp_vec[4];
    float cos_theta;
    float cos_beta;
    float cos_beta_min;

    cos_beta_min = 0.939692f;
    cos_theta = sceVu0InnerProduct(Light_Direction, Normal_Vector);
    if (cos_theta < 0.0f) {
        cos_theta = -cos_theta;
    }
    sceVu0ScaleVectorXYZ(tmp_vec, Normal_Vector, cos_theta * 2.0f);
    sceVu0SubVector(reverse_light_dir, tmp_vec, Light_Direction);
    sceVu0Normalize(reverse_light_dir, reverse_light_dir);
    cos_beta = sceVu0InnerProduct(reverse_light_dir, View_Direction);
    if (cos_beta < 0.0f) {
        cos_beta = -cos_beta;
    }
    input_light_power = 1.0f;
    specular_coefficient = (cos_beta < cos_beta_min) ? 0.0f : 1.0f - (1.0f - cos_beta) / (1.0f - cos_beta_min);
    result = specular_coefficient * input_light_power;
    return result;
}

/* iRGBA = ftoi0(clamp(RGBA_Base + RGBA_Specular_Base * Specular_Ratio, 0, 255)) on VU0. */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
static void SpecularRGBA_Calculator(int *iRGBA, float *RGBA_Base, float *RGBA_Specular_Base, float Specular_Ratio) {
    __asm__ __volatile__("
    lqc2        vf30, 0x0(%1)
    lqc2        vf31, 0x0(%2)
    mfc1        t0, %3
    mfc1        t1, %4
    qmtc2.ni    t0, vf29
    ctc2.ni     t1, vi21
    vmulx.xyzw  vf31, vf31, vf29x
    vadd.xyzw   vf31, vf30, vf31
    vmaxx.xyzw  vf31, vf31, vf0x
    vminii.xyzw vf31, vf31, I
    vftoi0.xyzw vf31, vf31
    sqc2        vf31, 0x0(%0)
    " : : "r"(iRGBA), "r"(RGBA_Base), "r"(RGBA_Specular_Base), "f"(Specular_Ratio), "f"(255.0f));
}

/*
 * Matching: a fitted stand-in for float code (docs/stand-ins.md) before Object_Draw (found by
 * tools/constcount.py, not recovered); it sets the order of Object_Draw's float-constant loads.
 * The #line after it keeps the following line numbers as if it were absent.
 */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f + 53.0f + 55.0f + 57.0f + 59.0f + 61.0f + 63.0f + 65.0f + 67.0f + 69.0f + 71.0f + 73.0f + 75.0f + 77.0f + 79.0f + 81.0f + 83.0f + 85.0f + 87.0f + 89.0f + 91.0f + 93.0f + 95.0f + 97.0f + 99.0f + 101.0f + 103.0f + 105.0f + 107.0f + 109.0f + 111.0f + 113.0f + 115.0f + 117.0f + 119.0f + 121.0f + 123.0f + 125.0f + 127.0f + 129.0f + 131.0f + 133.0f + 135.0f + 137.0f + 139.0f + 141.0f + 143.0f + 145.0f + 147.0f + 149.0f + 151.0f + 153.0f + 155.0f + 157.0f + 159.0f + 161.0f + 163.0f + 165.0f + 167.0f + 169.0f + 171.0f + 173.0f + 175.0f + 177.0f + 179.0f + 181.0f + 183.0f + 185.0f + 187.0f + 189.0f + 191.0f + 193.0f + 195.0f + 197.0f + 199.0f + 201.0f + 203.0f + 205.0f + 207.0f + 209.0f + 211.0f + 213.0f + 215.0f + 217.0f + 219.0f + 221.0f + 223.0f + 225.0f + 227.0f + 229.0f + 231.0f + 233.0f + 235.0f + 237.0f + 239.0f + 241.0f + 243.0f + 245.0f + 247.0f + 249.0f + 251.0f + 253.0f + 255.0f + 257.0f + 259.0f + 261.0f + 263.0f + 265.0f + 267.0f + 269.0f + 271.0f + 273.0f + 275.0f + 277.0f + 279.0f + 281.0f + 283.0f + 285.0f + 287.0f + 289.0f + 291.0f + 293.0f + 295.0f + 297.0f + 299.0f + 301.0f + 303.0f + 305.0f + 307.0f + 309.0f + 311.0f + 313.0f + 315.0f + 317.0f + 319.0f + 321.0f + 323.0f + 325.0f + 327.0f + 329.0f + 331.0f + 333.0f + 335.0f + 337.0f + 339.0f + 341.0f + 343.0f + 345.0f + 347.0f + 349.0f + 351.0f + 353.0f + 355.0f + 357.0f + 359.0f + 361.0f + 363.0f + 365.0f + 367.0f + 369.0f + 371.0f + 373.0f; } /* fitted, not recovered: 186 constants */
static unsigned int Object_Draw(struct HH_Object_Water_00 *pThis, struct ImpactQueue_Element *pElement) {
    static unsigned long _GifTag[2] = { 0x1000000000000000, 14 };
    static unsigned long _GifTag_Tri[2] = { 0x3025400000000000, 0x412 };
    static float Light_Base[4] = { 16.0f, 16.0f, 16.0f, 0.0f };
    static float Amb_Base[4] = { 64.0f, 64.0f, 64.0f, 0.0f };
    static float Ambient_Color2[4] = { 19.0f, 19.0f, 19.0f, 255.0f };
    static float SpecularRgba[4] = { 255.0f, 255.0f, 255.0f, 64.0f };
    static float amb_alpha = 32.0f;
    static float light_alpha = 128.0f;
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned int vertex_num;
    unsigned int x_grid_max;
    unsigned int z_grid_max;
    unsigned int x_index;
    unsigned int z_index;
    unsigned int prim_type;
    float lwm[4][4];
    float lsm[4][4];
    float clip_mat[4][4];
    float *pGrid_Y;
    float Ambient_Color[4];
    float view_dir[4];
    float pos[4];
    float dir[4];
    float Light_Color[4];
    float Parameter[4];
    float far_z;
    float cos_theta;
    unsigned int *pPk_Current;
    unsigned int *pPk_End;
    unsigned int Clip_Mask;

    pPk = HH_Vif1Packet_Current_Get();
    x_grid_max = GRID_X_MAX;
    z_grid_max = GRID_Z_MAX - 1;
    pGrid_Y = pThis->Grid_Y_Value[pThis->CurrentBuffer ^ 1];
    pPk_Current = NULL;
    HH_ClassWrapper_ViewDirection_Get(view_dir);
    HH_ClassWrapper_AmbientColor_Get(Ambient_Color);
    HH_ClassWrapper_SpotLight_EnvironmentParameter_Get(pos, dir, Light_Color, Parameter);
    sceVu0MulVector(Light_Color, Light_Color, Light_Base);
    sceVu0MulVector(Ambient_Color, Ambient_Color, Amb_Base);
    Ambient_Color[3] = amb_alpha;
    Light_Color[3] = light_alpha;
    cos_theta = Parameter[0];
    far_z = Parameter[2];
    sceVu0Normalize(dir, dir);
    sceVu0SubVector(pos, pos, pElement->Option.Vector[0]);
    HH_ClassWrapper_WorldScreenMatrix_Get(lsm);
    sceVu0UnitMatrix(lwm);
    sceVu0TransMatrix(lwm, lwm, pElement->Option.Vector[0]);
    sceVu0MulMatrix(lsm, lsm, lwm);
    HH_ClassWrapper_ViewFrustum_ClipMatrix_Get(clip_mat);
    sceVu0MulMatrix(clip_mat, clip_mat, lwm);
    HH_Vif1PacketBuffer_GifTag_Open();
    sceVif1PkCloseGifTag(pPk);
    for (z_index = 0; z_index < z_grid_max; z_index++) {
        float Grid_Vertex0[4] = { GRID_WH, 0.0f, z_index * GRID_WH, 1.0f };
        float Grid_Vertex1[4] = { GRID_WH, 0.0f, (z_index + 1) * GRID_WH, 1.0f };
        float Grid_Vertex2[4] = { GRID_WH, 0.0f, z_index * GRID_WH, 1.0f };
        float Rgba[4];
        int xyzf[4];
        int rgba[4];
        unsigned int addr;

        sceVif1PkOpenGifTag(pPk, *(u_long128 *)_GifTag_Tri);
        if (pPk_Current != NULL) {
            HH_ClassWrapper_MemoryCopy128Align_DesignateCycle(pPk->pCurrent, pPk_Current, x_grid_max, 3, 3);
        }
        for (x_index = 0; x_index < x_grid_max; x_index++) {
            float vec0[4];
            float vec1[4];
            float n0[4];
            float specular_ratio;
            float stq0[4];
            float stq1[4];

            Grid_Vertex0[0] = Grid_Vertex1[0] = x_index * GRID_WH;
            Grid_Vertex2[0] = ((x_index + 1) % x_grid_max) * GRID_WH;
            Grid_Vertex0[1] = pGrid_Y[x_index + x_grid_max * z_index];
            Grid_Vertex1[1] = pGrid_Y[x_index + x_grid_max * (z_index + 1)];
            Grid_Vertex2[1] = pGrid_Y[(x_index + 1) % x_grid_max + x_grid_max * z_index];
            stq0[0] = stq1[0] = Grid_Vertex0[0] / 12000.0f;
            stq0[1] = Grid_Vertex0[2] / 11750.0f;
            stq1[1] = Grid_Vertex1[2] / 11750.0f;
            sceVu0SubVector(vec0, Grid_Vertex2, Grid_Vertex0);
            sceVu0SubVector(vec1, Grid_Vertex1, Grid_Vertex0);
            sceVu0Normalize(vec0, vec0);
            sceVu0Normalize(vec1, vec1);
            vec1[3] = vec0[3] = 1.0f;
            sceVu0OuterProduct(n0, vec0, vec1);
            specular_ratio = Specular_Calculator(view_dir, dir, n0);
            if (z_index == 0) {
                float color_scale;

                HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq0, lsm, clip_mat, Grid_Vertex0, 0x3F);
                color_scale = HH_ClassWrapper_SpotLight_ColorRatio_Calculator(pos, dir, Grid_Vertex0, cos_theta, far_z);
                color_scale = HH_ClassWrapper_Float_Clamp(color_scale, 0.0f, 1.0f);
                if (color_scale == 0.0f) {
                    sceVu0FTOI0Vector(rgba, Ambient_Color2);
                } else {
                    sceVu0ScaleVectorXYZ(Rgba, Light_Color, color_scale);
                    sceVu0AddVector(Rgba, Rgba, Ambient_Color);
                    sceVu0ClampVector(Rgba, Rgba, 0.0f, 255.0f);
                    if (Grid_Vertex0[1] != 0.0f) {
                        SpecularRGBA_Calculator(rgba, Rgba, SpecularRgba, specular_ratio);
                    } else {
                        sceVu0FTOI0Vector(rgba, Rgba);
                    }
                }
                *(u_long128 *)pPk->pCurrent = *(u_long128 *)stq0;
                pPk->pCurrent += 4;
                *(u_long128 *)pPk->pCurrent = *(u_long128 *)rgba;
                pPk->pCurrent += 4;
                *(u_long128 *)pPk->pCurrent = *(u_long128 *)xyzf;
                pPk->pCurrent += 4;
            } else {
                pPk->pCurrent += 12;
            }
            {
                float color_scale;

                HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq1, lsm, clip_mat, Grid_Vertex1, 0x3F);
                color_scale = HH_ClassWrapper_SpotLight_ColorRatio_Calculator(pos, dir, Grid_Vertex1, cos_theta, far_z);
                color_scale = HH_ClassWrapper_Float_Clamp(color_scale, 0.0f, 1.0f);
                if (color_scale == 0.0f) {
                    sceVu0FTOI0Vector(rgba, Ambient_Color2);
                } else {
                    sceVu0ScaleVectorXYZ(Rgba, Light_Color, color_scale);
                    sceVu0AddVector(Rgba, Rgba, Ambient_Color);
                    sceVu0ClampVector(Rgba, Rgba, 0.0f, 255.0f);
                    if (Grid_Vertex1[1] != 0.0f) {
                        SpecularRGBA_Calculator(rgba, Rgba, SpecularRgba, specular_ratio);
                    } else {
                        sceVu0FTOI0Vector(rgba, Rgba);
                    }
                }
                if (x_index == 0) {
                    pPk_Current = pPk->pCurrent;
                }
                *(u_long128 *)pPk->pCurrent = *(u_long128 *)stq1;
                pPk->pCurrent += 4;
                *(u_long128 *)pPk->pCurrent = *(u_long128 *)rgba;
                pPk->pCurrent += 4;
                *(u_long128 *)pPk->pCurrent = *(u_long128 *)xyzf;
                pPk->pCurrent += 4;
            }
        }
        sceVif1PkCloseGifTag(pPk);
    }
    pPk_End = pPk->pCurrent;
    HH_ClassWrapper_Packet_ADC_Flag_OnceMore_Set(pPk_End, x_grid_max, z_grid_max);
    sceVif1PkOpenGifTag(pPk, *(u_long128 *)_GifTag);
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 0;
    return result;
}

/**
 * Class prefix, run once a frame before this class's instances: sets the GS TEX0, CLAMP and ALPHA
 * registers they draw with. Returns 1.
 */
unsigned int HH_Class_Prefix_Water_00(void) {
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned long tex0;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    tex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(0xC, 0);
    sceVif1PkAddGsAD(pPk, GS_REG_TEX0_1, tex0);
    sceVif1PkAddGsAD(pPk, GS_REG_CLAMP_1, 0);
    sceVif1PkAddGsAD(pPk, GS_REG_ALPHA_1, GS_SET_ALPHA(0, 1, 0, 1, 0x80));
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class suffix, run once a frame after this class's instances: sets the GS CLAMP register back.
 * Returns 1.
 */
unsigned int HH_Class_Suffix_Water_00(void) {
    unsigned int result;
    sceVif1Packet *pPk;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    sceVif1PkAddGsAD(pPk, GS_REG_CLAMP_1, 5);
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class main: each frame, swaps the wave buffers, advances the wave equation and draws the
 * surface. Runs until the instance is disabled from outside.
 * @param pBlock   the instance's data block (struct HH_Object_Water_00)
 * @param pElement the element that created it; Option.Vector[0] is the surface's position
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Water_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Water_00 *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis);
        Grid_Work_Initialize(pThis);
        pThis->Step = 1;
        break;
    case 1:
        pThis->CurrentBuffer ^= 1;
        Object_Motion_00(pThis);
        HH_DBG_Wrapper_T0_COUNT_Get();
        Object_Draw(pThis, pElement);
        HH_DBG_Wrapper_T0_COUNT_Delta();
        pThis->Timer += 1.0f / 30.0f;
        break;
    case 2:
        Object_Draw(pThis, pElement);
        break;
    case 3:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    return result;
}
