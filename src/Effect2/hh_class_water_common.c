/*
 * hh_class_water_common.c: the wave simulation shared by the grid water classes. A surface is a
 * set of rectangular grid areas; waves are circular sine waves spreading from an impact grid
 * point, fading with time and distance, and the grid heights they produce are summed. Areas are
 * enabled around the one the player is in and only while they are in view.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "sdk/libvu0.h"

static float Wave_Calculator(struct Wave_Element *pElement, float Distance, float Amplitude_ratio) {
    float result;
    float theta;

    theta = pElement->Omega * (pElement->Timer + Distance / pElement->Verocity);
    result = Amplitude_ratio * pElement->Arrival * HH_MathWrapper_Sinf(Radian_Normalize(theta));
    return result;
}

static unsigned int WaveArea_Calculator(struct WaveArea_Infomeation *pInfo, struct Wave_Element *pElement_Table, unsigned int Element_Max, unsigned int Area_Index) {
    unsigned int result;
    unsigned int i;
    unsigned int Grid_X_Max;
    unsigned int Grid_Z_Max;
    float Grid_WH;
    float *pGrid_Y_Value;
    struct Wave_Element *pElement;
    float distance_max;
    unsigned int impact_x;
    unsigned int impact_z;
    unsigned int x_count;
    unsigned int z_count;
    float arrival_ratio;
    float arrival_distance;
    float z_distance;
    float x_distance;
    float distance;
    float amplitude_ratio;
    float y;

    Grid_X_Max = pInfo->Grid_Index[0];
    Grid_Z_Max = pInfo->Grid_Index[1];
    Grid_WH = pInfo->Grid_WH[3];
    pGrid_Y_Value = pInfo->pGrid_Y_Value;
    for (i = 0; i < Element_Max; i++) {
        pElement = &pElement_Table[i];
        if (pElement->Enable && pElement->Area == Area_Index) {
            distance_max = pElement->Max_Distance0;
            impact_x = pElement->Impact_Grid_Index[0];
            impact_z = pElement->Impact_Grid_Index[1];
            arrival_ratio = 1.0f - pElement->Timer / pElement->Lost_Time;
            arrival_distance = pElement->Verocity * pElement->Timer;
            if (arrival_ratio < 0.0f) {
                arrival_ratio = 0.0f;
            }
            distance_max *= arrival_ratio;
            for (z_count = 0; z_count < Grid_Z_Max; z_count++) {
                z_distance = (impact_z - z_count) * (impact_z - z_count) * Grid_WH * Grid_WH;
                for (x_count = 0; x_count < Grid_X_Max; x_count++) {
                    x_distance = (impact_x - x_count) * (impact_x - x_count) * Grid_WH * Grid_WH;
                    distance = HH_MathWrapper_Sqrtf(z_distance + x_distance);
                    if (distance <= distance_max && distance <= arrival_distance) {
                        amplitude_ratio = 1.0f - distance / (distance_max < arrival_distance ? distance_max : arrival_distance);
                        y = Wave_Calculator(pElement, distance, amplitude_ratio);
                        pGrid_Y_Value[z_count * Grid_X_Max + x_count] += y;
                    }
                }
            }
        }
    }
    result = 0;
    return result;
}

static unsigned int WaveArea_GridLink_Y_Value_Calculator(struct WaveArea_GridLink_Infomeation *pInfo) {
    unsigned int result;
    unsigned int i;
    unsigned int link_index0;
    unsigned int link_index1;
    unsigned int index0_add;
    unsigned int index1_add;

    index1_add = index0_add = 1;
    switch (pInfo->Vertical_Horizontal_Flag) {
    case 0:
        index0_add = pInfo->X_Index_Max[0];
        index1_add = pInfo->X_Index_Max[1];
        break;
    case 1:
        break;
    }
    link_index0 = pInfo->X_Index_Start[0] + pInfo->X_Index_Max[0] * pInfo->Z_Index_Start[0];
    link_index1 = pInfo->X_Index_Start[1] + pInfo->X_Index_Max[1] * pInfo->Z_Index_Start[1];
    for (i = 0; i < pInfo->Length; i++, link_index0 += index0_add, link_index1 += index1_add) {
        pInfo->pGrid_Y_Value_Link[0][link_index0] = pInfo->pGrid_Y_Value_Link[1][link_index1] =
            pInfo->pGrid_Y_Value_Link[0][link_index0] + pInfo->pGrid_Y_Value_Link[1][link_index1];
    }
    result = 1;
    return result;
}

static unsigned int WaveArea_CurrentArea_CollisionCheck(float *Position, struct WaveArea_Infomeation *pInfo);

static int WaveArea_CurrentArea_Search(float *Position, struct WaveArea_Infomeation *pInfo_Table, unsigned int Table_Max) {
    int result;
    unsigned int i;
    struct WaveArea_Infomeation *pInfo;

    result = -1;
    for (i = 0; i < Table_Max; i++) {
        pInfo = &pInfo_Table[i];
        if (WaveArea_CurrentArea_CollisionCheck(Position, pInfo)) {
            result = i;
            break;
        }
    }
    return result;
}

static unsigned int WaveArea_CurrentArea_CollisionCheck(float *Position, struct WaveArea_Infomeation *pInfo) {
    unsigned int result;
    float check_pos[4];

    result = 0;
    sceVu0SubVector(check_pos, Position, pInfo->World_Location);
    if (0.0f < check_pos[0] && check_pos[0] < pInfo->Grid_WH[0] && 0.0f < check_pos[2] && check_pos[2] < pInfo->Grid_WH[2]) {
        result = 1;
    }
    return result;
}

static struct Wave_Element *WaveElement_Free_Search(struct Wave_Element *pElement_Table, unsigned int Element_Max) {
    struct Wave_Element *result;
    unsigned int i;
    struct Wave_Element *pElement;

    result = NULL;
    for (i = 0; i < Element_Max; i++) {
        pElement = &pElement_Table[i];
        if (!pElement->Enable) {
            result = pElement;
            break;
        }
    }
    return result;
}

static struct Wave_Element *WaveElement_Oldest_Search(struct Wave_Element *pElement_Table, unsigned int Element_Max) {
    struct Wave_Element *result;
    unsigned int i;
    float time;
    struct Wave_Element *pElement;

    result = NULL;
    time = 0.0f;
    for (i = 0; i < Element_Max; i++) {
        pElement = &pElement_Table[i];
        if (pElement->Enable && time < pElement->Timer) {
            time = pElement->Timer;
            result = pElement;
        }
    }
    return result;
}

static unsigned int WaveElement_Addition(struct Wave_Element *pElement_Table, unsigned int Element_Max, struct Wave_Element *pAdd_Element) {
    unsigned int result;
    struct Wave_Element *pFree_Element;

    pFree_Element = WaveElement_Free_Search(pElement_Table, Element_Max);
    if (pFree_Element == NULL) {
        pFree_Element = WaveElement_Oldest_Search(pElement_Table, Element_Max);
    }
    *pFree_Element = *pAdd_Element;
    result = 1;
    return result;
}

static void Area_Enable_Table_Clear(unsigned int *pArea_Enable_Table, unsigned int Table_Max) {
    unsigned int i;

    for (i = 0; i < Table_Max; i++) {
        pArea_Enable_Table[i] = 0;
    }
}

static unsigned int Area_ViewFrustum_Judge(struct WaveArea_Infomeation *pInfo);

static void Area_Enable_Manager(unsigned int *pArea_Enable_Table, struct WaveArea_Infomeation *pInfo_Table, unsigned int Table_Max, int Current_Area) {
    struct WaveArea_Infomeation *pInfo_Cur;
    unsigned int i;
    unsigned int area;
    struct WaveArea_Infomeation *pInfo;

    pInfo_Cur = &pInfo_Table[Current_Area];
    for (i = 0; i < pInfo_Cur->ViewArea_List_Max; i++) {
        area = pInfo_Cur->pViewArea_List[i];
        pArea_Enable_Table[area] = 1;
    }
    for (i = 0; i < Table_Max; i++) {
        pInfo = &pInfo_Table[i];
        if (pArea_Enable_Table[i] && i != Current_Area && !Area_ViewFrustum_Judge(pInfo)) {
            pArea_Enable_Table[i] = 0;
        }
    }
}

static unsigned int Area_ViewFrustum_Judge(struct WaveArea_Infomeation *pInfo) {
    unsigned int result;
    float clip_mat[4][4];
    float vec_array[4][4];
    unsigned int j;

    result = 0;
    for (j = 0; j < 4; j++) {
        sceVu0CopyVector(vec_array[j], pInfo->World_Location);
    }
    vec_array[1][2] += pInfo->Grid_WH[2];
    vec_array[2][0] += pInfo->Grid_WH[0];
    vec_array[3][0] += pInfo->Grid_WH[0];
    vec_array[3][2] += pInfo->Grid_WH[2];
    HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(clip_mat);
    if (HH_ClassWrapper_Point_Clip_Judge(clip_mat, vec_array, 4) != 4) {
        result = 1;
    }
    return result;
}

/**
 * Adds the height of every live wave of one area to the area's grid heights.
 * @param pInfo         the area
 * @param pElement_Table wave table
 * @param Element_Max   number of waves in the table
 * @param Area_Index    the area's index (waves belong to one area)
 * @return 0
 */
unsigned int HH_Class_WaterCommon_WaveArea_Calculator(struct WaveArea_Infomeation *pInfo, struct Wave_Element *pElement_Table, unsigned int Element_Max, unsigned int Area_Index) {
    return WaveArea_Calculator(pInfo, pElement_Table, Element_Max, Area_Index);
}

/**
 * Joins two areas along a shared edge: both edge rows get the sum of their heights.
 * @param pInfo the link (areas, start indices, length, direction)
 * @return 1
 */
unsigned int HH_Class_WaterCommon_WaveArea_GridLink_Y_Value_Calculator(struct WaveArea_GridLink_Infomeation *pInfo) {
    return WaveArea_GridLink_Y_Value_Calculator(pInfo);
}

/**
 * Returns the index of the area whose rectangle (in X and Z) contains Position, or -1.
 * @param Position    the point
 * @param pInfo_Table areas
 * @param Table_Max   number of areas
 */
int HH_Class_WaterCommon_WaveArea_CurrentArea_Search(float *Position, struct WaveArea_Infomeation *pInfo_Table, unsigned int Table_Max) {
    return WaveArea_CurrentArea_Search(Position, pInfo_Table, Table_Max);
}

/**
 * Adds a wave to the table, in a free slot or else over the oldest wave.
 * @param pElement_Table wave table
 * @param Element_Max    number of waves in the table
 * @param pAdd_Element   the wave (copied)
 * @return 1
 */
unsigned int HH_Class_WaterCommon_WaveElement_Addition(struct Wave_Element *pElement_Table, unsigned int Element_Max, struct Wave_Element *pAdd_Element) {
    return WaveElement_Addition(pElement_Table, Element_Max, pAdd_Element);
}

/** Advances every live wave's timer by a frame and ends the waves older than their lifetime. */
void HH_Class_WaterCommon_WaveElement_Time_Count(struct Wave_Element *pElement_Table, unsigned int Element_Max) {
    unsigned int i;
    struct Wave_Element *pElement;

    for (i = 0; i < Element_Max; i++) {
        pElement = &pElement_Table[i];
        if (pElement->Enable) {
            pElement->Timer += 1.0f / 30.0f;
            if (pElement->Timer > pElement->Lost_Time) {
                pElement->Enable = 0;
            }
        }
    }
}

/** Disables all Table_Max areas. */
void HH_Class_WaterCommon_Area_Enable_Table_Clear(unsigned int *pArea_Enable_Table, unsigned int Table_Max) {
    Area_Enable_Table_Clear(pArea_Enable_Table, Table_Max);
}

/**
 * Enables the areas visible from Current_Area (its view list), then disables those of them
 * (other than Current_Area) that are entirely outside the view frustum.
 */
void HH_Class_WaterCommon_Area_Enable_Manager(unsigned int *pArea_Enable_Table, struct WaveArea_Infomeation *pInfo_Table, unsigned int Table_Max, int Current_Area) {
    Area_Enable_Manager(pArea_Enable_Table, pInfo_Table, Table_Max, Current_Area);
}
