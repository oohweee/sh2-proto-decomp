/*
 * hh_class_blood_pool_phenomenon_01.c: sprays blood drops around a point along a direction.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Blood_Pool_Phenomenon_01
 * runs once a frame for each instance and posts other effect objects.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "sdk/libvu0.h"

static unsigned int Object_Initialize(struct HH_Object_Blood_Pool_Phenomenon_01 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    float p_y;
    float p_volume;
    float alpha;
    float *src_direction;

    pThis->Timer = 0.0f;
    src_direction = pElement->Option.Vector[1];
    p_volume = sceVu0InnerProduct(src_direction, src_direction);
    p_volume = HH_MathWrapper_Sqrtf(p_volume);
    p_y = src_direction[1];
    alpha = asinf(p_y / p_volume);
    if (alpha < 0.0f) {
        alpha = -alpha;
    }
    pThis->Alpha = 0.5f * alpha;
    result = 1;
    return result;
}

/**
 * Class main: posts six blood sprays (class 14, HH_Class_Blood_04) from random points around the
 * position, fanned around random headings at the speed of the given velocity, then ends.
 * @param pBlock   the instance's data block (struct HH_Object_Blood_Pool_Phenomenon_01)
 * @param pElement the element that created it; Option.Vector[0] is the position and Vector[1]
 *                 the velocity
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Blood_Pool_Phenomenon_01(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Blood_Pool_Phenomenon_01 *pThis;
    unsigned int i;
    float *src_direction;
    float *src_location;
    float p_volume;
    float radian;
    float alpha_randam_range;
    float radius_rand;
    float alpha;
    float lwm[4][4];
    struct ImpactQueue_Element descriptor;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis, pElement);
        pThis->Step = 1;
        break;
    case 1:
        src_direction = pElement->Option.Vector[1];
        p_volume = sceVu0InnerProduct(src_direction, src_direction);
        p_volume = HH_MathWrapper_Sqrtf(p_volume);
        for (i = 0; i < 6; i++) {
            radian = Radian_Normalize((rand() % 360) * 0.017453292f);
            alpha_randam_range = (rand() % 10) * 0.017453292f - 0.08726646f;
            radius_rand = 10.0f + rand() % 50;
            alpha = Radian_Normalize(pThis->Alpha + alpha_randam_range);
            {
                float vec[4] = { 0.0f, 0.0f, radius_rand, 1.0f };

                src_location = pElement->Option.Vector[0];
                sceVu0UnitMatrix(lwm);
                sceVu0RotMatrixY(lwm, lwm, radian);
                sceVu0TransMatrix(lwm, lwm, src_location);
                sceVu0ApplyMatrix(descriptor.Option.Vector[0], lwm, vec);
            }
            {
                float vec[4] = { 0.0f, 0.0f, p_volume, 1.0f };

                alpha = Radian_Normalize(alpha);
                sceVu0UnitMatrix(lwm);
                sceVu0RotMatrixX(lwm, lwm, alpha);
                sceVu0RotMatrixY(lwm, lwm, radian);
                sceVu0ApplyMatrix(descriptor.Option.Vector[1], lwm, vec);
            }
            descriptor.hInstance = 0;
            descriptor.pResultHandle_Address = NULL;
            descriptor.Class_Descriptor = 14;
            HH_Effect_Object_Impact_Post(&descriptor);
        }
        pThis->Step = 2;
        break;
    case 2:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    pThis->Timer += 1.0f / 30.0f;
    return result;
}
