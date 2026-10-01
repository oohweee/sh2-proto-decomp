/*
 * hh_class_blood_splash_phenomenon_02.c: bursts of blood drops thrown out of a wounded character.
 * An effect class of the HH object manager (hh_class_manager.c):
 * HH_Class_Blood_Splash_Phenomenon_02 runs once a frame for each instance and posts other effect
 * objects.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "sdk/libvu0.h"

static float _unit_vector_x[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
static float _unit_vector_z[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
static float _discharge_full_number[2] = { 15.0f, 10.0f };
static float _discharge_time[2] = { 0.0033333334f, 0.04f };

static unsigned int Object_Initialize(struct HH_Object_Blood_Splash_Phenomenon_02 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    float p_y;
    float p_volume;
    float *src_direction;
    float p_xz[4];
    float lambda_1;
    float lambda_2;
    struct SubCharacter *pSubChar;

    pThis->Timer = 0.0f;
    pThis->Next_Discharge_Time[0] = 0.0f;
    pThis->Next_Discharge_Time[1] = 0.0f;
    pThis->Post_Count[0] = 0;
    pThis->Post_Count[1] = 0;
    src_direction = pElement->Option.Vector[1];
    p_volume = HH_MathWrapper_Sqrtf(sceVu0InnerProduct(src_direction, src_direction));
    pThis->Verocity_0 = p_volume;
    p_y = src_direction[1];
    pThis->Beta = asinf(p_y / p_volume);
    pThis->Alpha = 4.0f * pThis->Beta + 0.17453292f;
    if (pThis->Alpha < 0.0f) {
        pThis->Alpha = -pThis->Alpha;
    }
    pThis->Gamma = 0.95f * pThis->Beta;
    pThis->Delta = 0.3926991f;
    sceVu0CopyVector(p_xz, src_direction);
    p_xz[1] = 0.0f;
    sceVu0Normalize(p_xz, p_xz);
    lambda_1 = sceVu0InnerProduct(p_xz, _unit_vector_z);
    lambda_2 = sceVu0InnerProduct(p_xz, _unit_vector_x);
    pThis->Lambda = lambda_1;
    if (lambda_2 < 1.5707964f) {
        pThis->Lambda = -pThis->Lambda;
    }
    pSubChar = (struct SubCharacter *)pElement->Option.Int_Value[0];
    pElement->Option.Float_Value[0] = pSubChar->pos.y - pSubChar->center_y;
    pElement->Option.Float_Value[1] = pSubChar->pos.y - pElement->Option.Vector[0][1];
    result = 1;
    return result;
}

static void Impact_Parameter_Calculator(struct HH_Object_Blood_Splash_Phenomenon_02 *pThis, float *src_location, float *src_direction, unsigned int Kind) {
    struct ImpactQueue_Element descriptor;
    unsigned int phai_range;
    unsigned int theta_range;
    float v0_range;
    float v0_plus;
    float ratio;
    float phai;
    float theta;
    float *dst_location;
    float *dst_direction;
    float mat[4][4];
    float vx0_plus;
    float vz0_plus;
    float vx0_range;
    float vz0_range;

    phai_range = Radian_To_Degree(pThis->Alpha);
    theta_range = Radian_To_Degree(2.0f * pThis->Delta);
    v0_range = pThis->Verocity_0;
    v0_plus = rand() % (unsigned int)v0_range;
    ratio = pThis->Verocity_0 * sinf(pThis->Beta);
    phai = 0.0f;
    theta = 0.0f;
    dst_location = descriptor.Option.Vector[0];
    dst_direction = descriptor.Option.Vector[1];
    if (phai_range != 0.0f) {
        phai = (rand() % phai_range) * 0.017453292f - 0.5f * pThis->Alpha;
    }
    if (theta_range != 0.0f) {
        theta = (rand() % theta_range) * 0.017453292f - pThis->Delta;
    }
    if (ratio < 0.0f) {
        ratio *= -1.0f;
    }
    sceVu0CopyVector(dst_location, src_location);
    sceVu0UnitMatrix(mat);
    theta = Radian_Normalize(theta);
    phai = Radian_Normalize(phai);
    sceVu0RotMatrixX(mat, mat, theta);
    sceVu0RotMatrixY(mat, mat, phai);
    sceVu0CopyVector(dst_direction, src_direction);
    dst_direction[0] += ratio;
    dst_direction[2] += ratio;
    vx0_range = dst_direction[0];
    vx0_plus = 0.0f;
    vz0_range = dst_direction[2];
    vz0_plus = 0.0f;
    if (vx0_range != 0.0f) {
        if (vx0_range < 0.0f) {
            vx0_range = -vx0_range;
        }
        vx0_plus = rand() % (unsigned int)vx0_range - 0.5f * vx0_range;
    }
    if (vz0_range != 0.0f) {
        if (dst_direction[2] < 0.0f) {
            vz0_range = -vz0_range;
        }
        vz0_plus = rand() % (unsigned int)vz0_range - 0.5f * vz0_range;
    }
    dst_direction[0] += vx0_plus;
    dst_direction[2] += vz0_plus;
    sceVu0ApplyMatrix(dst_direction, mat, dst_direction);
    descriptor.Option.Int_Value[0] = Kind;
    descriptor.hInstance = 0;
    descriptor.pResultHandle_Address = NULL;
    descriptor.Class_Descriptor = 15;
    HH_Effect_Object_Impact_Post(&descriptor);
    pThis->Post_Count[Kind]++;
    pThis->Next_Discharge_Time[Kind] += _discharge_time[Kind];
}

/**
 * Class main: HH_Class_Blood_Splash_Phenomenon_00 with blood spurts that stain the floor
 * (class 15, HH_Class_Blood_05) and the wound height kept relative to the character's position.
 * @param pBlock   the instance's data block (struct HH_Object_Blood_Splash_Phenomenon_02)
 * @param pElement the element that created it; Option.Int_Value[0] is the SubCharacter,
 *                 Vector[0] the wound position, Vector[1] the direction, Float_Value[0] and
 *                 Float_Value[1] the character's height and the wound's depth below it
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Blood_Splash_Phenomenon_02(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Blood_Splash_Phenomenon_02 *pThis;
    unsigned int j;
    struct SubCharacter *pSubChar;
    float def_vec[4];
    float y_ratio;
    float current_h;
    float diff_time;
    float count;
    unsigned int count_i;
    unsigned int i;
    unsigned int max;
    unsigned int free;
    float src_location[4];
    float src_direction[4];

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis, pElement);
        pThis->Step++;
        break;
    case 1:
        pSubChar = (struct SubCharacter *)pElement->Option.Int_Value[0];
        y_ratio = 0.0f;
        current_h = pSubChar->pos.y - pSubChar->center_y;
        if (pElement->Option.Float_Value[0] != 0.0f) {
            y_ratio = current_h / pElement->Option.Float_Value[0];
            if (y_ratio < 0.0f) {
                y_ratio *= -1.0f;
            }
        }
        sceVu0SubVector(def_vec, (float *)&pSubChar->pos, (float *)&pSubChar->b_pos);
        sceVu0AddVector(pElement->Option.Vector[0], pElement->Option.Vector[0], def_vec);
        pElement->Option.Float_Value[1] *= y_ratio;
        pElement->Option.Vector[0][1] = pSubChar->pos.y - pElement->Option.Float_Value[1];
        pElement->Option.Float_Value[0] = current_h;
        for (j = 0; j < 2; j++) {
            if (pThis->Timer > pThis->Next_Discharge_Time[j]) {
                diff_time = pThis->Timer - pThis->Next_Discharge_Time[j];
                count = diff_time / _discharge_time[j];
                count_i = count;
                max = 1;
                free = (unsigned int)_discharge_full_number[j] - pThis->Post_Count[j];
                sceVu0CopyVector(src_location, pElement->Option.Vector[0]);
                sceVu0CopyVector(src_direction, pElement->Option.Vector[1]);
                if (j == 1) {
                    src_direction[0] = 0.0f;
                    src_direction[1] = -500.0f;
                    src_direction[2] = 0.0f;
                }
                if (free != 0) {
                    if (max > free) {
                        max = free;
                    } else if (count_i > 1) {
                        max *= count_i;
                    }
                    for (i = 0; i < max; i++) {
                        Impact_Parameter_Calculator(pThis, src_location, src_direction, j);
                    }
                }
            }
        }
        if (pThis->Timer > 0.5f) {
            pThis->Step = 2;
        }
        break;
    case 2:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    pThis->Timer += 1.0f / 60.0f;
    return result;
}
