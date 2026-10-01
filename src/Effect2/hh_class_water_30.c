/*
 * hh_class_water_30.c: a rising water surface made of one grid area, lit by the spotlight.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Water_30 runs once a
 * frame for each instance, between the class's prefix and suffix.
 */
#include "sh2.h"
#include "hh_math.h"
#include "hh_vector.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "libc/string.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)
#define GAME_FLAG_ON(n) (game_flag.flag[(n) >> 5] |= 1 << ((n) & 31))

static unsigned int _area00_view_list[1] = { 0 };

static struct WaveArea_Infomeation _Area_Info_List[1] = {
    { { 98750.0f, -1450.0f, -100850.0f, 1.0f }, { 3000.0f, 0.0f, 2400.0f, 150.0f }, { 20, 16 }, _area00_view_list, 1 },
};

/* Puts the surface back at its starting height. */
static inline void Location_Initialize(float *v) {
    float init[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    sceVu0CopyVector(v, init);
}

static void Grid_Work_Initialize(struct HH_Object_Water_30 *pThis) {
    memset(pThis->Area00_Grid_Y_Value, 0, sizeof(pThis->Area00_Grid_Y_Value));
}

static unsigned int Object_Initialize(struct HH_Object_Water_30 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;

    pThis->Timer = 0.0f;
    pThis->Motion_Step = 0;
    Vector_Zero(pThis->ST_Defference);
    Location_Initialize(pThis->Location_Defference);
    pThis->Area_WavePostTime[0] = 0.0f;
    _Area_Info_List[0].pGrid_Y_Value = pThis->pArea_Grid_Y_Value_Table[0] = pThis->Area00_Grid_Y_Value;
    result = 1;
    return result;
}

static void CurrentPosition_AreaIndex_Calculator(struct ImpactQueue_Element *pElement, struct WaveArea_Infomeation *pInfo, unsigned int *pX_Index, unsigned int *pZ_Index) {
    float check_pos[4];

    sceVu0SubVector(check_pos, pElement->Option.Vector[0], pInfo->World_Location);
    *pX_Index = check_pos[0] / 150.0f;
    *pZ_Index = check_pos[2] / 150.0f;
}

static float Specular_Calculator(float *View_Direction, float *Light_Direction, float *Normal_Vector) {
    static float cos_beta_min = 0.819152f;
    float result;
    float specular_coefficient;
    float input_light_power;
    float reverse_light_dir[4];
    float tmp_vec[4];
    float cos_theta;
    float cos_beta;

    cos_theta = sceVu0InnerProduct(Light_Direction, Normal_Vector);
    if (cos_theta < 0.0f) {
        cos_theta = -cos_theta;
    }
    sceVu0ScaleVectorXYZ(tmp_vec, Normal_Vector, 2.0f * cos_theta);
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
    " : : "r"(iRGBA), "r"(RGBA_Base), "r"(RGBA_Specular_Base), "f"(Specular_Ratio), "f"(255.0f) : "t0", "t1");
}

static unsigned int Object_Draw(struct HH_Object_Water_30 *pThis, float *pGrid_Y_Value, float *WorldLocation, unsigned int Grid_X_Max, unsigned int Grid_Z_Max) {
    static unsigned long _GifTag[2] = { 0x1000000000000000, 0xE };
    static unsigned long _GifTag_Tri[2] = { 0x302E400000000000, 0x412 };
    static float Light_Base[4] = { 48.0f, 48.0f, 48.0f, 0.0f };
    static float Amb_Base[4] = { 64.0f, 64.0f, 64.0f, 0.0f };
    static float SpecularRgba[4] = { 128.0f, 128.0f, 128.0f, 64.0f };
    static float amb_alpha = 64.0f;
    static float light_alpha = 32.0f;
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
    float tex_s_w;
    float tex_t_w;
    unsigned int *pPk_Current;
    unsigned int *pPk_End;

    pPk = HH_Vif1Packet_Current_Get();
    x_grid_max = Grid_X_Max;
    z_grid_max = Grid_Z_Max - 1;
    pGrid_Y = pGrid_Y_Value;
    tex_s_w = 512.0f / (x_grid_max - 1);
    tex_t_w = 512.0f / z_grid_max;
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
        float Grid_Vertex0[4] = { 150.0f, 0.0f, z_index * 150.0f, 1.0f };
        float Grid_Vertex1[4] = { 150.0f, 0.0f, (z_index + 1) * 150.0f, 1.0f };
        float Grid_Vertex2[4] = { 150.0f, 0.0f, z_index * 150.0f, 1.0f };
        float Rgba[4];
        int xyzf[4];
        int rgba[4];
        unsigned int addr;
        float vec0[4];
        float vec1[4];
        float n0[4];
        float specular_ratio;

        sceVif1PkOpenGifTag(pPk, *(u_long128 *)_GifTag_Tri);
        if (pPk_Current) {
            HH_ClassWrapper_MemoryCopy128Align_DesignateCycle(pPk->pCurrent, pPk_Current, x_grid_max, 3, 3);
        }
        for (x_index = 0; x_index < x_grid_max; x_index++) {
            float now_s = tex_s_w * x_index / 512.0f;
            float stq0[4] = { now_s, tex_t_w * z_index / 512.0f };
            float stq1[4] = { now_s, tex_t_w * (z_index + 1) / 512.0f };
            float color_scale;

            Grid_Vertex0[0] = Grid_Vertex1[0] = x_index * 150.0f;
            Grid_Vertex2[0] = ((x_index + 1) % x_grid_max) * 150.0f;
            Grid_Vertex0[1] = pGrid_Y[x_index + x_grid_max * z_index];
            Grid_Vertex1[1] = pGrid_Y[x_index + x_grid_max * (z_index + 1)];
            Grid_Vertex2[1] = pGrid_Y[(x_index + 1) % x_grid_max + x_grid_max * z_index];
            sceVu0SubVector(vec0, Grid_Vertex2, Grid_Vertex0);
            sceVu0SubVector(vec1, Grid_Vertex1, Grid_Vertex0);
            sceVu0Normalize(vec0, vec0);
            sceVu0Normalize(vec1, vec1);
            vec0[3] = 1.0f;
            vec1[3] = 1.0f;
            sceVu0OuterProduct(n0, vec0, vec1);
            specular_ratio = Specular_Calculator(view_dir, dir, n0);
            if (z_index == 0) {
                float color_scale;

                HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq0, lsm, clip_mat, Grid_Vertex0, 0x3F);
                color_scale = HH_ClassWrapper_SpotLight_ColorRatio_Calculator(pos, dir, Grid_Vertex0, cos_theta, far_z);
                color_scale = HH_ClassWrapper_Float_Clamp(color_scale, 0.0f, 1.0f);
                sceVu0ScaleVectorXYZ(Rgba, Light_Color, color_scale);
                sceVu0AddVector(Rgba, Rgba, Ambient_Color);
                sceVu0ClampVector(Rgba, Rgba, 0.0f, 255.0f);
                SpecularRGBA_Calculator(rgba, Rgba, SpecularRgba, specular_ratio);
                *(u_long128 *)pPk->pCurrent = *(u_long128 *)stq0;
                pPk->pCurrent += 4;
                *(u_long128 *)pPk->pCurrent = *(u_long128 *)rgba;
                pPk->pCurrent += 4;
                *(u_long128 *)pPk->pCurrent = *(u_long128 *)xyzf;
                pPk->pCurrent += 4;
            } else {
                pPk->pCurrent += 12;
            }
            HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq1, lsm, clip_mat, Grid_Vertex1, 0x3F);
            color_scale = HH_ClassWrapper_SpotLight_ColorRatio_Calculator(pos, dir, Grid_Vertex1, cos_theta, far_z);
            color_scale = HH_ClassWrapper_Float_Clamp(color_scale, 0.0f, 1.0f);
            sceVu0ScaleVectorXYZ(Rgba, Light_Color, color_scale);
            sceVu0AddVector(Rgba, Rgba, Ambient_Color);
            sceVu0ClampVector(Rgba, Rgba, 0.0f, 255.0f);
            SpecularRGBA_Calculator(rgba, Rgba, SpecularRgba, specular_ratio);
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
 * Class prefix, run once a frame before this class's instances: sets the GS TEX0, CLAMP, ZBUF and
 * ALPHA registers they draw with. Returns 1.
 */
unsigned int HH_Class_Prefix_Water_30(void) {
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned long tex0;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    tex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(11, 0);
    sceVif1PkAddGsAD(pPk, GS_REG_TEX0_1, tex0);
    sceVif1PkAddGsAD(pPk, GS_REG_CLAMP_1, 0x1FF001FFF);
    sceVif1PkAddGsAD(pPk, GS_REG_ZBUF_1, GS_SET_ZBUF(0x1C0, 0xA, 0));
    sceVif1PkAddGsAD(pPk, GS_REG_ALPHA_1, GS_SET_ALPHA(0, 1, 0, 1, 0x80));
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class suffix, run once a frame after this class's instances: sets the GS CLAMP and ZBUF registers
 * back. Returns 1.
 */
unsigned int HH_Class_Suffix_Water_30(void) {
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
 * Class main: as the other grid water classes (texture scroll, impact and random waves, wave
 * simulation, drawing); while game flag 147 is set the surface rises, and at 1700 units it
 * sets flag 148 and the instance ends (at once, if flag 148 is already set).
 * @param pBlock   the instance's data block (struct HH_Object_Water_30)
 * @param pElement the element that created it; Option.Vector[0] selects the grid area; when
 *                 Option.Int_Value[0] is set, an impact wave is added at Option.Vector[0] (then
 *                 the flag is cleared)
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Water_30(void *pBlock, struct ImpactQueue_Element *pElement) {
    static float degree = 40.0f;
    static float base_move = 8.0f;
    static float add_move = 0.0f;
    static float center = 0.0f;
    static float _distance = 1200.0f;
    static float _lost_time = 12.0f;
    static float _arri = 40.0f;
    static float _omega = 540.0f;
    static float _v = 500.0f;
    static float _interval = 4.0f;
    static float __distance = 3200.0f;
    static float __lost_time = 10.0f;
    static float __arri = 40.0f;
    static float __omega = 612.0f;
    static float __v = 1200.0f;
    static float time = 10.0f;
    unsigned int result;
    struct HH_Object_Water_30 *pThis;
    float rad_omega;
    float rad;
    int area;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        if (GAME_FLAG(148)) {
            pThis->Step = 2;
        }
        Object_Initialize(pThis, pElement);
        pThis->Step = 1;
        break;
    case 1:
        Grid_Work_Initialize(pThis);
        rad_omega = 0.017453292f * degree;
        rad = Radian_Normalize(rad_omega * pThis->Timer);
        pThis->ST_Defference[0] = add_move + (center + base_move * HH_MathWrapper_Cosf(rad)) / 512.0f;
        pThis->ST_Defference[1] = add_move + (center + base_move * HH_MathWrapper_Sinf(rad)) / 512.0f;
        area = HH_Class_WaterCommon_WaveArea_CurrentArea_Search(pElement->Option.Vector[0], _Area_Info_List, 1);
        HH_Class_WaterCommon_Area_Enable_Table_Clear(pThis->Area_Enable_Table, 1);
        if (area != -1) {
            HH_Class_WaterCommon_Area_Enable_Manager(pThis->Area_Enable_Table, _Area_Info_List, 1, area);
        } else {
            pThis->Area_Enable_Table[0] = 1;
        }
        if (pElement->Option.Int_Value[0]) {
            int area;
            struct Wave_Element wave_element;
            unsigned int x;
            unsigned int z;
            unsigned int i;

            area = HH_Class_WaterCommon_WaveArea_CurrentArea_Search(pElement->Option.Vector[0], _Area_Info_List, 1);
            if (area != -1) {
                CurrentPosition_AreaIndex_Calculator(pElement, &_Area_Info_List[area], &x, &z);
                wave_element.Enable = 1;
                wave_element.Area = area;
                wave_element.Impact_Grid_Index[0] = x;
                wave_element.Impact_Grid_Index[1] = z;
                wave_element.Max_Distance0 = _distance;
                wave_element.Lost_Time = _lost_time;
                wave_element.Arrival = _arri;
                wave_element.Omega = 0.017453292f * _omega;
                wave_element.Verocity = _v;
                wave_element.Timer = 0.0f;
                HH_Class_WaterCommon_WaveElement_Addition(pThis->Wave_Info, 20, &wave_element);
            }
            pElement->Option.Int_Value[0] = 0;
        }
        {
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
                        if (rand() % 3) {
                            if (rand() % 3) {
                                z = 0;
                            } else {
                                z = pInfo->Grid_Index[1] - 1;
                            }
                        } else {
                            if (rand() % 3) {
                                x = 0;
                            } else {
                                x = pInfo->Grid_Index[0] - 1;
                            }
                        }
                        wave_element.Enable = 1;
                        wave_element.Area = i;
                        wave_element.Impact_Grid_Index[0] = x;
                        wave_element.Impact_Grid_Index[1] = z;
                        wave_element.Max_Distance0 = __distance;
                        wave_element.Lost_Time = __lost_time;
                        wave_element.Arrival = __arri;
                        wave_element.Omega = 0.017453292f * __omega;
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

            for (i = 0; i < 1; i++) {
                if (pThis->Area_Enable_Table[i]) {
                    struct WaveArea_Infomeation *pArea_Info = &_Area_Info_List[i];
                    float world_location[4];

                    sceVu0AddVector(world_location, pArea_Info->World_Location, pThis->Location_Defference);
                    HH_Class_WaterCommon_WaveArea_Calculator(pArea_Info, pThis->Wave_Info, 20, i);
                    Object_Draw(pThis, pThis->pArea_Grid_Y_Value_Table[i], world_location, pArea_Info->Grid_Index[0], pArea_Info->Grid_Index[1]);
                }
            }
        }
        HH_Class_WaterCommon_WaveElement_Time_Count(pThis->Wave_Info, 20);
        if (GAME_FLAG(147)) {
            float add_vec[4] = { 0.0f, 1500.0f * (1.0f / 30.0f) / time, 0.0f, 0.0f };

            sceVu0AddVector(pThis->Location_Defference, pThis->Location_Defference, add_vec);
            if (pThis->Location_Defference[1] > 1700.0f) {
                GAME_FLAG_ON(148);
            }
        }
        if (GAME_FLAG(148)) {
            pThis->Step = 2;
        } else {
            pThis->Timer += 1.0f / 30.0f;
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
