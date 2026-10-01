/*
 * hh_class_glass_break_phenomenon_00.c: a burst of glass pieces thrown out of a breaking window.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Glass_Break_Phenomenon_00
 * runs once a frame for each instance and posts other effect objects.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "sdk/libvu0.h"

/* Matching: a fitted stand-in for software-double code (docs/stand-ins.md). It makes this file's functions use a2
 * for temporaries where they would use a0 (docs/decomp-workflow.md). */
STRIPPED_DOUBLE_CODE()

static float _unit_vector_x[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
static float _unit_vector_z[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

static unsigned int Object_Initialize(struct HH_Object_Glass_Break_Phenomenon_00 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    float p_y;
    float p_volume;
    float *src_direction;
    float p_xz[4];
    float lambda_1;
    float lambda_2;

    pThis->Timer = 0.0f;
    pThis->Next_Discharge_Time = 0.0f;
    pThis->Post_Count = 0;
    src_direction = pElement->Option.Vector[1];
    p_volume = sceVu0InnerProduct(src_direction, src_direction);
    p_volume = HH_MathWrapper_Sqrtf(p_volume);
    pThis->Verocity_0 = p_volume;
    p_y = src_direction[1];
    pThis->Beta = asinf(p_y / p_volume);
    pThis->Alpha = 4.0f * pThis->Beta;
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
    result = 1;
    return result;
}

/**
 * Class main: for 0.3 seconds, throws out glass pieces (class 21, HH_Class_GlassPiece_00) at a
 * steady rate, each with a random spread around the direction; then ends.
 * @param pBlock   the instance's data block (struct HH_Object_Glass_Break_Phenomenon_00)
 * @param pElement the element that created it; Option.Vector[0] is the window position and
 *                 Vector[1] the direction
 * @return 1 while the instance runs, 0 once it has ended
 *
 * Matching: the vx0/vz0 locals are declared in an order fitted to their float registers; the DWARF's order
 * (vx0_range, vx0_plus, vz0_range, vz0_plus) doesn't match (docs/dwarf-fidelity.md). Function scope as in
 * the DWARF (no lexical block), which the line table allows (298 `dst_direction[2] += ratio;`, 300
 * `vx0_range = ...`).
 */
unsigned int HH_Class_Glass_Break_Phenomenon_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Glass_Break_Phenomenon_00 *pThis;
    float diff_time;
    float count;
    unsigned int count_i;
    unsigned int i;
    unsigned int max;
    unsigned int free;
    struct ImpactQueue_Element descriptor;
    unsigned int phai_range;
    unsigned int theta_range;
    float v0_range;
    float v0_plus;
    float ratio;
    float phai;
    float theta;
    float *src_location;
    float *src_direction;
    float *dst_direction;
    float mat[4][4];
    float vx0_plus;
    float vz0_plus;
    float vx0_range;
    float vz0_range;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis, pElement);
        pThis->Step++;
        break;
    case 1:
        if (pThis->Timer > pThis->Next_Discharge_Time) {
            diff_time = pThis->Timer - pThis->Next_Discharge_Time;
            count = diff_time / 0.006f;
            count_i = count;
            max = 1;
            free = 50 - pThis->Post_Count;
            if (free < max) {
                max = free;
            } else if (count_i > 1) {
                max *= count_i;
            }
            for (i = 0; i < max; i++) {
                phai_range = Radian_To_Degree(pThis->Alpha);
                theta_range = Radian_To_Degree(2.0f * pThis->Delta);
                v0_range = pThis->Verocity_0;
                v0_plus = rand() % (unsigned int)v0_range;
                ratio = 0.3f * (pThis->Verocity_0 * sinf(pThis->Beta));
                phai = 0.0f;
                theta = 0.0f;
                src_location = pElement->Option.Vector[0];
                src_direction = pElement->Option.Vector[1];
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
                sceVu0CopyVector(descriptor.Option.Vector[0], src_location);
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
                descriptor.hInstance = 0;
                descriptor.pResultHandle_Address = NULL;
                descriptor.Class_Descriptor = 21;
                HH_Effect_Object_Impact_Post(&descriptor);
                pThis->Post_Count++;
                pThis->Next_Discharge_Time += 0.006f;
            }
        }
        if (pThis->Timer > 0.3f) {
            pThis->Step++;
        }
        break;
    case 2:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        printf("Object Discard!! %8.4f\n", pThis->Timer);
        break;
    }
    pThis->Timer += 1.0f / 30.0f;
    return result;
}
