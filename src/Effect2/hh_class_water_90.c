/*
 * hh_class_water_90.c: a rippling water surface on a grid, lit by a directional light and fogged.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Water_90 runs once a
 * frame for each instance, between the class's prefix and suffix.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "libc/string.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

#define GRID_WH 300.0f


static unsigned int _area00_view_list[1] = { 0 };

static struct WaveArea_Infomeation _Area_Info_List[1] = {
    {
        { -16800.0f, 3500.0f, 2100.0f, 1.0f },
        { 15600.0f, 0.0f, 10200.0f, 300.0f },
        { 52, 34 },
        _area00_view_list,
        1,
        NULL,
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        NULL,
    },
};

static void Grid_Work_Initialize(struct HH_Object_Water_90 *pThis) {
    memset(pThis->Area00_Grid_Y_Value, 0, sizeof(pThis->Area00_Grid_Y_Value));
}

static unsigned int Object_Initialize(struct HH_Object_Water_90 *pThis) {
    unsigned int result;

    pThis->Timer = 0.0f;
    pThis->Motion_Step = 0;
    sceVu0CopyVector(pThis->ST_Defference, (float[4]){ 0.0f, 0.0f, 0.0f, 0.0f });
    sceVu0CopyVector(pThis->Location_Defference, (float[4]){ 0.0f, 0.0f, 0.0f, 0.0f });
    pThis->Area_WavePostTime[0] = 0.0f;
    pThis->pArea_Grid_Y_Value_Table[0] = pThis->Area00_Grid_Y_Value;
    _Area_Info_List[0].pGrid_Y_Value = pThis->Area00_Grid_Y_Value;
    result = 1;
    return result;
}

/*
 * Matching: the float-constant order for the first sceVu0ClampVector (255.0f before 0.0f) needs
 * the stripped float-code stand-in below (fitted by tools/constcount.py, not recovered).
 * The #line after it keeps the following line numbers as if it were absent.
 */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f + 53.0f + 55.0f + 57.0f + 59.0f + 61.0f + 63.0f + 65.0f + 67.0f + 69.0f + 71.0f + 73.0f + 75.0f + 77.0f + 79.0f + 81.0f + 83.0f + 85.0f + 87.0f + 89.0f + 91.0f + 93.0f + 95.0f + 97.0f + 99.0f + 101.0f + 103.0f + 105.0f + 107.0f + 109.0f + 111.0f + 113.0f + 115.0f + 117.0f + 119.0f + 121.0f + 123.0f + 125.0f + 127.0f; } /* fitted, not recovered: 63 constants */
static unsigned int Object_Draw(struct HH_Object_Water_90 *pThis, float *pGrid_Y_Value, float *WorldLocation, unsigned int Grid_X_Max, unsigned int Grid_Z_Max) {
    static unsigned long _GifTag[2] = { 0x1000000000000000, 14 };
    static unsigned long _GifTag_Tri[2] = { 0x3036400000000000, 0x412 };
    static float Light_Base[4] = { 18.0f, 18.0f, 18.0f, 0.0f };
    static float Ambient_Color[4] = { 50.0f, 50.0f, 50.0f, 70.0f };
    static float amb_alpha = 40.0f;
    static float light_alpha = 50.0f;
    static float fog_ratio_f = 1.0f;
    static unsigned int flag = 0;
    static float x = 0.7768225f;
    static float y = 0.2888876f;
    static float z = 0.559206f;
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
    float view_dir[4];
    float pos[4];
    float dir[4];
    float Light_Color[4];
    float Parameter[4];
    float far_z;
    float cos_theta;
    unsigned int *pPk_Current;
    unsigned int *pPk_End;

    pPk = HH_Vif1Packet_Current_Get();
    x_grid_max = Grid_X_Max;
    z_grid_max = Grid_Z_Max - 1;
    pGrid_Y = pGrid_Y_Value;
    pPk_Current = NULL;
    HH_ClassWrapper_ViewDirection_Get(view_dir);
    HH_ClassWrapper_SpotLight_EnvironmentParameter_Get(pos, dir, Light_Color, Parameter);
    sceVu0MulVector(Light_Color, Light_Color, Light_Base);
    Ambient_Color[3] = amb_alpha;
    Light_Color[3] = light_alpha;
    sceVu0Normalize(dir, dir);
    if (flag) {
        dir[0] = x;
        dir[1] = y;
        dir[2] = z;
    }
    sceVu0ScaleVectorXYZ(dir, dir, -1.0f);
    sceVu0SubVector(pos, pos, WorldLocation);
    HH_ClassWrapper_WorldScreenMatrix_Get(lsm);
    sceVu0UnitMatrix(lwm);
    sceVu0TransMatrix(lwm, lwm, WorldLocation);
    sceVu0MulMatrix(lsm, lsm, lwm);
    HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(clip_mat);
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
            static float sx = 7.0f;
            static float ty = 7.0f;
            float vec0[4];
            float vec1[4];
            float n0[4];
            float specular_ratio;
            float stq0[4];
            float stq1[4];
            float base;

            Grid_Vertex0[0] = Grid_Vertex1[0] = x_index * GRID_WH;
            Grid_Vertex2[0] = ((x_index + 1) % x_grid_max) * GRID_WH;
            Grid_Vertex0[1] = pGrid_Y[x_index + x_grid_max * z_index];
            Grid_Vertex1[1] = pGrid_Y[x_index + x_grid_max * (z_index + 1)];
            Grid_Vertex2[1] = pGrid_Y[(x_index + 1) % x_grid_max + x_grid_max * z_index];
            base = (1.0f / (GRID_WH * ty)) * 0.25f;
            stq0[0] = stq1[0] = (Grid_Vertex0[0] / (GRID_WH * sx)) * 0.25f;
            stq0[1] = Grid_Vertex0[2] * base;
            stq1[1] = Grid_Vertex1[2] * base;
            sceVu0AddVector(stq0, stq0, pThis->ST_Defference);
            sceVu0AddVector(stq1, stq1, pThis->ST_Defference);
            sceVu0SubVector(vec0, Grid_Vertex2, Grid_Vertex0);
            sceVu0SubVector(vec1, Grid_Vertex1, Grid_Vertex0);
            sceVu0Normalize(vec0, vec0);
            sceVu0Normalize(vec1, vec1);
            vec0[3] = 1.0f;
            vec1[3] = 1.0f;
            sceVu0OuterProduct(n0, vec0, vec1);
            if (z_index == 0) {
                float color_scale;
                float fog_col_f;
                unsigned int fog_col_i;

                HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq0, lsm, clip_mat, Grid_Vertex0, 0x3F);
                color_scale = sceVu0InnerProduct(dir, n0);
                color_scale = HH_ClassWrapper_Float_Clamp(color_scale, 0.0f, 1.0f);
                if (color_scale == 0.0f) {
                    sceVu0FTOI0Vector(rgba, Ambient_Color);
                } else {
                    sceVu0ScaleVectorXYZ(Rgba, Light_Color, color_scale);
                    sceVu0AddVector(Rgba, Rgba, Ambient_Color);
                    sceVu0ClampVector(Rgba, Rgba, 0.0f, 255.0f);
                    sceVu0FTOI0Vector(rgba, Rgba);
                }
                fog_col_f = HH_ClassWrapper_FogParameter_A_Get() + stq0[2] * HH_ClassWrapper_FogParameter_B_Get();
                fog_col_i = HH_ClassWrapper_Float_Clamp(fog_col_f * fog_ratio_f, 0.0f, 255.0f);
                xyzf[3] &= ~0xFF0;
                xyzf[3] |= fog_col_i << 4;
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
                float fog_col_f;
                unsigned int fog_col_i;

                HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq1, lsm, clip_mat, Grid_Vertex1, 0x3F);
                color_scale = sceVu0InnerProduct(dir, n0);
                color_scale = HH_ClassWrapper_Float_Clamp(color_scale, 0.0f, 1.0f);
                if (color_scale == 0.0f) {
                    sceVu0FTOI0Vector(rgba, Ambient_Color);
                } else {
                    sceVu0ScaleVectorXYZ(Rgba, Light_Color, color_scale);
                    sceVu0AddVector(Rgba, Rgba, Ambient_Color);
                    sceVu0ClampVector(Rgba, Rgba, 0.0f, 255.0f);
                    sceVu0FTOI0Vector(rgba, Rgba);
                }
                fog_col_f = HH_ClassWrapper_FogParameter_A_Get() + stq1[2] * HH_ClassWrapper_FogParameter_B_Get();
                fog_col_i = HH_ClassWrapper_Float_Clamp(fog_col_f * fog_ratio_f, 0.0f, 255.0f);
                xyzf[3] &= ~0xFF0;
                xyzf[3] |= fog_col_i << 4;
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
 * Class prefix, run once a frame before this class's instances: sets the GS ZBUF, TEX0, CLAMP and
 * ALPHA registers they draw with. A debug key combination steps the CLAMP region through the
 * texture's tiles. Returns 1.
 */
unsigned int HH_Class_Prefix_Water_90(void) {
    static unsigned int interval = 1;
    static unsigned int j;
    static int i;
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned long tex0;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    sceVif1PkAddGsAD(pPk, GS_REG_ZBUF_1, GS_SET_ZBUF(0x1C0, 0xA, 0));
    tex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(0xD, 0);
    sceVif1PkAddGsAD(pPk, GS_REG_TEX0_1, tex0);
    sceVif1PkAddGsAD(pPk, GS_REG_CLAMP_1, GS_SET_CLAMP(3, 3, 0x7F, (i / 4) * 128, 0x7F, (i % 4) * 128));
    if (HH_DBG_Wrapper_Controller_KeyAssign_Check(1, 0, 0x2000)) {
        if (j % interval == 0) {
            i++;
            i %= 16;
        }
    }
    j++;
    sceVif1PkAddGsAD(pPk, GS_REG_ALPHA_1, GS_SET_ALPHA(0, 1, 0, 1, 0x80));
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class suffix, run once a frame after this class's instances: sets the GS CLAMP and ZBUF registers
 * back. Returns 1.
 */
unsigned int HH_Class_Suffix_Water_90(void) {
    unsigned int result;
    sceVif1Packet *pPk;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    sceVif1PkAddGsAD(pPk, GS_REG_CLAMP_1, 5);
    sceVif1PkAddGsAD(pPk, GS_REG_ZBUF_1, GS_SET_ZBUF(0x1C0, 0xA, 1));
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class main: each frame, scrolls the texture, adds waves along the grid's edges at intervals,
 * runs the wave simulation and draws the surface. Runs until the instance is disabled from
 * outside.
 * @param pBlock   the instance's data block (struct HH_Object_Water_90)
 * @param pElement the element that created it; Option.Vector[0] selects the grid area
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Water_90(void *pBlock, struct ImpactQueue_Element *pElement) {
    static unsigned int run = 1;
    static float degree = 10.0f;
    static float base_move = 128.0f;
    static float _interval = 10.0f;
    static float __distance = 28800.0f;
    static float __lost_time = 20.0f;
    static float __arri = 200.0f;
    static float __omega = 200.0f;
    static float __v = 500.0f;
    unsigned int result;
    struct HH_Object_Water_90 *pThis;
    float rad_omega;
    float rad;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis);
        pThis->Step = 1;
        break;
    case 1: {
        int area;
        float temp[4];

        if (run) {
            Grid_Work_Initialize(pThis);
        }
        rad = degree * 0.017453292f;
        rad_omega = Radian_Normalize(rad * pThis->Timer);
        pThis->ST_Defference[0] = (base_move + base_move * HH_MathWrapper_Cosf(rad_omega)) / 512.0f;
        pThis->ST_Defference[1] = (base_move + base_move * HH_MathWrapper_Sinf(rad_omega)) / 512.0f;
        area = HH_Class_WaterCommon_WaveArea_CurrentArea_Search(pElement->Option.Vector[0], _Area_Info_List, 1);
        HH_ClassWrapper_JMS_WorldPosition_Get(temp);
        HH_Class_WaterCommon_Area_Enable_Table_Clear(pThis->Area_Enable_Table, 1);
        if (area != -1) {
            HH_Class_WaterCommon_Area_Enable_Manager(pThis->Area_Enable_Table, _Area_Info_List, 1, area);
        } else {
            pThis->Area_Enable_Table[0] = 1;
        }
        if ((Sh2sys.main_status >> 6) & 1) {
            pThis->Area_Enable_Table[0] = 0;
        }
        if (temp[2] < -4000.0f) {
            pThis->Area_Enable_Table[0] = 0;
        }
        if (temp[0] < -20200.0f) {
            pThis->Area_Enable_Table[0] = 0;
        }
        if (run) {
            unsigned int i;
            struct WaveArea_Infomeation *pInfo;
            struct Wave_Element wave_element;
            unsigned int x;
            unsigned int z;

            for (i = 0; i < 1; i++) {
                if (pThis->Area_Enable_Table[i]) {
                    if (pThis->Timer - pThis->Area_WavePostTime[i] > _interval) {
                        pInfo = &_Area_Info_List[i];
                        x = rand() % pInfo->Grid_Index[0];
                        z = rand() % pInfo->Grid_Index[1];
                        x = pInfo->Grid_Index[0] / 2;
                        z = pInfo->Grid_Index[1] - 1;
                        wave_element.Enable = 1;
                        wave_element.Area = i;
                        wave_element.Impact_Grid_Index[0] = x;
                        wave_element.Impact_Grid_Index[1] = z;
                        wave_element.Max_Distance0 = __distance;
                        wave_element.Lost_Time = __lost_time;
                        wave_element.Arrival = __arri;
                        wave_element.Omega = __omega * 0.017453292f;
                        wave_element.Verocity = __v;
                        wave_element.Timer = 0.0f;
                        HH_Class_WaterCommon_WaveElement_Addition(pThis->Wave_Info, 20, &wave_element);
                        pThis->Area_WavePostTime[i] = pThis->Timer;
                    }
                }
            }
        }
        {
            unsigned int i;
            struct WaveArea_Infomeation *pArea_Info;
            float world_location[4];

            for (i = 0; i < 1; i++) {
                if (pThis->Area_Enable_Table[i]) {
                    pArea_Info = &_Area_Info_List[i];
                    sceVu0AddVector(world_location, pArea_Info->World_Location, pThis->Location_Defference);
                    if (run) {
                        HH_Class_WaterCommon_WaveArea_Calculator(pArea_Info, pThis->Wave_Info, 20, i);
                    }
                    Object_Draw(pThis, pThis->pArea_Grid_Y_Value_Table[i], world_location, pArea_Info->Grid_Index[0], pArea_Info->Grid_Index[1]);
                }
            }
        }
        HH_Class_WaterCommon_WaveElement_Time_Count(pThis->Wave_Info, 20);
        pThis->Timer += 1.0f / 30.0f;
        break;
    }
    case 2:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    return result;
}
