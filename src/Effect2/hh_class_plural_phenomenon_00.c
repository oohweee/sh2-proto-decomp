/*
 * hh_class_plural_phenomenon_00.c: a footstep that may land in a blood pool: then it splashes
 * and puts blood on the character's soles.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Plural_Phenomenon_00 runs
 * once a frame for each instance and posts other effect objects.
 */
#include "sh2.h"
#include "sdk/libvu0.h"

/*
 * FIXME(header): ImpactQueue_ElementOption's Vector members are 16-byte aligned (as common.h's
 * sceVu0FVECTOR), so the struct is 16-aligned and copied with lq/sq. The generated header can't
 * see that (Vector sits at offset 0); this local twin carries the alignment.
 */

struct ImpactQueue_ElementOption_A {
    sceVu0FVECTOR Vector[2];
    float Float_Value[2];
    int Int_Value[2];
};
#define OPTION(x) (*(struct ImpactQueue_ElementOption_A *)&(x))

static unsigned int Object_Initialize(struct HH_Object_Plural_Phenomenon_00 *pThis) {
    unsigned int result;

    pThis->Timer = 0.0f;
    result = 1;
    return result;
}

static unsigned int ObjectInstance_BLOOD_STICK_00_Search_and_Post(struct ImpactQueue_Element *pElement, unsigned int Count_Max, float Set_Parameter) {
    unsigned int result;
    unsigned int i;
    struct Object_Group_Infomeation *pInfo;
    unsigned int hInstance;
    struct ImpactQueue_Element *pElement_Stick_00;
    struct ImpactQueue_Element descriptor;

    result = 0;
    pInfo = HH_Effect_Object_Infomeation_Get();
    for (i = 0; i < Count_Max; i++) {
        hInstance = ObjectInstanceHandle_Get_from_ClassDescriptor_and_AttachCount(pInfo, 8, i);
        pElement_Stick_00 = ObjectInstance_Element_Get(pInfo, hInstance);
        if (pElement->Option.Int_Value[0] == pElement_Stick_00->Option.Int_Value[0] &&
            pElement->Option.Int_Value[1] == pElement_Stick_00->Option.Int_Value[1]) {
            descriptor.hInstance = hInstance;
            descriptor.pResultHandle_Address = NULL;
            descriptor.Class_Descriptor = 8;
            OPTION(descriptor.Option) = OPTION(pElement->Option);
            descriptor.Option.Float_Value[0] = Set_Parameter;
            HH_Effect_Object_Impact_Post(&descriptor);
            result = 1;
            break;
        }
    }
    return result;
}

/**
 * Class main, run once per footstep: if the middle of the foot lands inside a blood pool
 * (class 12, HH_Class_Blood_02), adds blood to the character's blood-on-soles object (class 8,
 * HH_Class_Blood_Stick_00, posting one if there is none) and posts a small blood splash (class
 * 6); otherwise tells the blood-on-soles object a step was taken. Then ends.
 * @param pBlock   the instance's data block (struct HH_Object_Plural_Phenomenon_00)
 * @param pElement the element that created it; Option.Vector[0] and Vector[1] are the toe and
 *                 heel positions, Int_Value[0] and Int_Value[1] the footmark kind and character
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Plural_Phenomenon_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    static float direction[4] = { 0.0f, -2500.0f, 0.0f, 1.0f };
    unsigned int result;
    struct HH_Object_Plural_Phenomenon_00 *pThis;
    float center_location[4];
    float *src_location;
    float *src_direction;
    struct Object_Group_Infomeation *pInfo;
    unsigned int count_stick_00;
    unsigned int count_blood_02;
    unsigned int i;
    unsigned int collision_check;
    unsigned int hInstance;
    struct ImpactQueue_Element *pElement_Blood_02;
    void *pBlock_Blood_02;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis);
        pThis->Step = 1;
        break;
    case 1:
        src_location = pElement->Option.Vector[0];
        sceVu0AddVector(center_location, src_location, pElement->Option.Vector[1]);
        sceVu0ScaleVectorXYZ(center_location, center_location, 0.5f);
        pInfo = HH_Effect_Object_Infomeation_Get();
        count_stick_00 = ObjectInstance_DesignateClassDescriptorAttach_Count(pInfo, 8);
        count_blood_02 = ObjectInstance_DesignateClassDescriptorAttach_Count(pInfo, 12);
        collision_check = 0;
        for (i = 0; i < count_blood_02; i++) {
            hInstance = ObjectInstanceHandle_Get_from_ClassDescriptor_and_AttachCount(pInfo, 12, i);
            pElement_Blood_02 = ObjectInstance_Element_Get(pInfo, hInstance);
            pBlock_Blood_02 = ObjectInstance_DataBlock_Get(pInfo, hInstance);
            if (HH_Class_Blood_02_DesignateLocation_CollisionCheck(pBlock_Blood_02, pElement_Blood_02, center_location)) {
                collision_check = 1;
                break;
            }
        }
        if (collision_check) {
            if (!ObjectInstance_BLOOD_STICK_00_Search_and_Post(pElement, count_stick_00, 5.0f)) {
                struct ImpactQueue_Element descriptor;

                descriptor.hInstance = 0;
                descriptor.pResultHandle_Address = NULL;
                descriptor.Class_Descriptor = 8;
                OPTION(descriptor.Option) = OPTION(pElement->Option);
                descriptor.Option.Float_Value[0] = 5.0f;
                HH_Effect_Object_Impact_Post(&descriptor);
            }
            {
                struct ImpactQueue_Element descriptor;
                float *dst_location;
                float *dst_direction;

                dst_location = descriptor.Option.Vector[0];
                dst_direction = descriptor.Option.Vector[1];
                src_direction = direction;
                descriptor.hInstance = 0;
                descriptor.pResultHandle_Address = NULL;
                descriptor.Class_Descriptor = 6;
                sceVu0CopyVector(dst_location, src_location);
                sceVu0CopyVector(dst_direction, src_direction);
                HH_Effect_Object_Impact_Post(&descriptor);
            }
        } else {
            ObjectInstance_BLOOD_STICK_00_Search_and_Post(pElement, count_stick_00, -1.0f);
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
