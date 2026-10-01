/*
 * hh_class_blood_stick_00.c: bloody soles; leaves footmarks while blood is left on them.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Blood_Stick_00 runs once
 * a frame for each instance and posts other effect objects.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "sdk/libvu0.h"

static float _unit_vector_x[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
static float _unit_vector_z[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

static unsigned int Object_Initialize(struct HH_Object_Blood_Stick_00 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;

    pThis->Timer = 0.0f;
    pThis->FootMark_Kind = pElement->Option.Int_Value[0];
    pThis->Character_ID = pElement->Option.Int_Value[1];
    result = 1;
    return result;
}

static unsigned int Object_Monitor(struct HH_Object_Blood_Stick_00 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;

    result = 0;
    if (pElement->Option.Float_Value[0] != 0.0f) {
        if (pElement->Option.Float_Value[0] > 0.0f && pThis->Leftover < 10) {
            pThis->Leftover += (unsigned char)pElement->Option.Float_Value[0];
        }
        if (pElement->Option.Float_Value[0] < 0.0f && pThis->Leftover > 0) {
            pThis->Leftover--;
            result = 1;
        }
        pElement->Option.Float_Value[0] = 0.0f;
    }
    return result;
}

static unsigned int Object_Manager(struct HH_Object_Blood_Stick_00 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    float center_location[4];
    float toe_dir[4];
    float cos_theta;
    float cos_phai;
    float rot_y;
    float *toe_location;
    float *heel_location;
    struct ImpactQueue_Element descriptor;

    result = 1;
    switch (pThis->Monitor_Step) {
    case 0:
        if (!Object_Monitor(pThis, pElement)) {
            break;
        }
        pThis->Monitor_Step = 1;
    case 1:
        toe_location = pElement->Option.Vector[0];
        heel_location = pElement->Option.Vector[1];
        sceVu0AddVector(center_location, toe_location, heel_location);
        sceVu0ScaleVectorXYZ(center_location, center_location, 0.5f);
        center_location[3] = 1.0f;
        sceVu0SubVector(toe_dir, toe_location, heel_location);
        sceVu0Normalize(toe_dir, toe_dir);
        cos_theta = sceVu0InnerProduct(toe_dir, _unit_vector_z);
        cos_phai = sceVu0InnerProduct(toe_dir, _unit_vector_x);
        rot_y = acosf(cos_theta);
        if (cos_phai < 0.0f) {
            rot_y = -rot_y;
        }
        descriptor.hInstance = 0;
        descriptor.pResultHandle_Address = NULL;
        descriptor.Class_Descriptor = 9;
        sceVu0CopyVector(descriptor.Option.Vector[0], center_location);
        descriptor.Option.Float_Value[0] = Radian_Normalize(rot_y);
        descriptor.Option.Float_Value[1] = 1.0f;
        descriptor.Option.Int_Value[0] = pThis->FootMark_Kind;
        descriptor.Option.Int_Value[1] = pThis->Character_ID;
        HH_Effect_Object_Impact_Post(&descriptor);
        pThis->Monitor_Step = 0;
        break;
    default:
        result = 0;
        break;
    }
    return result;
}

/**
 * Class main: tracks how much blood is left on a character's soles and leaves footmarks while
 * there is some; ends when it runs out or after 120 seconds.
 * @param pBlock   the instance's data block (struct HH_Object_Blood_Stick_00)
 * @param pElement the element that created it; Option.Int_Value[0] is the footmark kind,
 *                 Int_Value[1] the character, Float_Value[0] a blood amount to add (positive)
 *                 or a step taken (negative), cleared once read; Vector[0] and Vector[1] are
 *                 the toe and heel positions
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Blood_Stick_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Blood_Stick_00 *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis, pElement);
        pThis->Step = 1;
        break;
    case 1:
        Object_Manager(pThis, pElement);
        pThis->Timer += 1.0f / 30.0f;
        if (pThis->Leftover <= 0) {
            pThis->Step = 2;
        }
        if (pThis->Timer > 120.0f) {
            pThis->Step = 2;
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
