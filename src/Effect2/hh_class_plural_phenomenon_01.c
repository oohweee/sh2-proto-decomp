/*
 * hh_class_plural_phenomenon_01.c: water splashes and ripples, chosen by room.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Plural_Phenomenon_01 runs
 * once a frame for each instance and posts other effect objects.
 */
#include "sh2.h"
#include "libc/string.h"
#include "sdk/libvu0.h"

static unsigned int Object_Initialize(struct HH_Object_Plural_Phenomenon_01 *pThis) {
    unsigned int result;

    pThis->Timer = 0.0f;
    result = 1;
    return result;
}

/**
 * Class main, run once per footstep: by the current room, posts a water splash (class 18),
 * water pools (class 41), a ripple on the room's water surface class (24-38) and/or raised dust
 * (class 16) at the foot; some rooms first check the floor material. Then ends.
 * @param pBlock   the instance's data block (struct HH_Object_Plural_Phenomenon_01)
 * @param pElement the element that created it; Option.Vector[0] is the foot position
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Plural_Phenomenon_01(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Plural_Phenomenon_01 *pThis;
    int room_name;
    float *src_location;
    float *src_direction;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis);
        pThis->Step = 1;
        break;
    case 1:
        room_name = RoomNameJms();
        src_location = pElement->Option.Vector[0];
        /* Matching: unused; the DWARF has src_direction and the line table a line here without code. */
        src_direction = pElement->Option.Vector[1];
        {
            static float direction[4] = { 0.0f, -2500.0f, 0.0f, 1.0f };
            struct ImpactQueue_Element descriptor;
            float *dst_location;
            float *dst_direction;
            unsigned int check0;

            dst_location = descriptor.Option.Vector[0];
            dst_direction = descriptor.Option.Vector[1];
            check0 = 1;
            switch (room_name) {
            case 0x25:
            case 0x26: {
                static float add_vec[4] = { 0.0f, 1000.0f, 0.0f, 0.0f };
                struct _CL_VHIT_RESULT hit_result;
                float e_pos[4];

                memset(&hit_result, 0, sizeof(hit_result));
                sceVu0AddVector(e_pos, pElement->Option.Vector[0], add_vec);
                clCheckHitEyesOnlyFloor(&hit_result, 0, pElement->Option.Vector[0], e_pos);
                if (hit_result.hobj.wall.pd != NULL && hit_result.hobj.wall.pd->material != 6) {
                    check0 = 0;
                }
                break;
            }
            default:
                check0 = 0;
                break;
            }
            if (check0) {
                descriptor.hInstance = 0;
                descriptor.pResultHandle_Address = NULL;
                descriptor.Class_Descriptor = 18;
                sceVu0CopyVector(dst_location, src_location);
                sceVu0CopyVector(dst_direction, direction);
                descriptor.Option.Float_Value[0] = 0.0f;
                descriptor.Option.Int_Value[0] = 1;
                HH_Effect_Object_Impact_Post(&descriptor);
            }
        }
        {
            static float direction[4] = { 0.0f, -2500.0f, 0.0f, 1.0f };
            static float local_y = 30.0f;
            struct ImpactQueue_Element descriptor;
            float *dst_location;
            float *dst_direction;
            unsigned int check0;

            dst_location = descriptor.Option.Vector[0];
            dst_direction = descriptor.Option.Vector[1];
            check0 = 1;
            switch (room_name) {
            case 0x62:
            case 0x25:
            case 0x26:
                break;
            default:
                check0 = 0;
                break;
            }
            if (check0) {
                descriptor.hInstance = 0;
                descriptor.pResultHandle_Address = NULL;
                descriptor.Class_Descriptor = 41;
                sceVu0CopyVector(dst_location, src_location);
                sceVu0CopyVector(dst_direction, direction);
                dst_location[1] = local_y;
                HH_Effect_Object_Impact_Post(&descriptor);
                HH_Effect_Object_Impact_Post(&descriptor);
                HH_Effect_Object_Impact_Post(&descriptor);
                HH_Effect_Object_Impact_Post(&descriptor);
            }
        }
        {
            struct Object_Group_Infomeation *pInfo;
            unsigned int class_descriptor;
            unsigned int check0;
            unsigned int check0_1;
            unsigned int check1;
            unsigned int splash_flag;
            unsigned int i;

            pInfo = HH_Effect_Object_Infomeation_Get();
            check0 = 1;
            check0_1 = 1;
            check1 = 1;
            splash_flag = 0;
            switch (room_name) {
            case 0x63: {
                static float add_vec[4] = { 0.0f, 1000.0f, 0.0f, 0.0f };
                struct _CL_VHIT_RESULT hit_result;
                float e_pos[4];

                class_descriptor = 24;
                check1 = 0;
                memset(&hit_result, 0, sizeof(hit_result));
                sceVu0AddVector(e_pos, pElement->Option.Vector[0], add_vec);
                clCheckHitEyesOnlyFloor(&hit_result, 0, pElement->Option.Vector[0], e_pos);
                if (hit_result.hobj.wall.pd != NULL && hit_result.hobj.wall.pd->material != 6) {
                    check0_1 = 0;
                }
                break;
            }
            case 0x62:
                class_descriptor = 25;
                check1 = 0;
                splash_flag = 1;
                break;
            case 0x80:
                class_descriptor = 26;
                break;
            case 0x85:
                class_descriptor = 27;
                break;
            case 0x7A:
                class_descriptor = 28;
                break;
            case 0x7C:
                class_descriptor = 29;
                break;
            case 0x7E:
                class_descriptor = 30;
                break;
            case 0x82:
                class_descriptor = 31;
                break;
            case 0xBB:
                class_descriptor = 32;
                break;
            case 0xB6:
                class_descriptor = 33;
                break;
            case 0xB7:
                class_descriptor = 34;
                break;
            case 0xAB:
                class_descriptor = 35;
                break;
            case 0xB8:
                class_descriptor = 36;
                break;
            case 0xB9:
                class_descriptor = 37;
                break;
            case 0x21:
                class_descriptor = 38;
                check0_1 = 0;
                check1 = 0;
                break;
            default:
                check0 = 0;
                check1 = 0;
                break;
            }
            if (check0) {
                unsigned int count_water;

                count_water = ObjectInstance_DesignateClassDescriptorAttach_Count(pInfo, class_descriptor);
                for (i = 0; i < count_water; i++) {
                    struct ImpactQueue_Element descriptor;
                    unsigned int hInstance;

                    hInstance = ObjectInstanceHandle_Get_from_ClassDescriptor_and_AttachCount(pInfo, class_descriptor, i);
                    descriptor.hInstance = hInstance;
                    descriptor.pResultHandle_Address = NULL;
                    descriptor.Class_Descriptor = class_descriptor;
                    sceVu0CopyVector(descriptor.Option.Vector[0], pElement->Option.Vector[0]);
                    descriptor.Option.Int_Value[0] = 1;
                    HH_Effect_Object_Impact_Post(&descriptor);
                }
                if (check0_1) {
                    static float direction[4] = { 0.0f, -2500.0f, 0.0f, 1.0f };
                    struct ImpactQueue_Element descriptor;
                    float *dst_location;
                    float *dst_direction;

                    dst_location = descriptor.Option.Vector[0];
                    dst_direction = descriptor.Option.Vector[1];
                    descriptor.hInstance = 0;
                    descriptor.pResultHandle_Address = NULL;
                    descriptor.Class_Descriptor = 18;
                    sceVu0CopyVector(dst_location, src_location);
                    sceVu0CopyVector(dst_direction, direction);
                    dst_location[1] -= 200.0f;
                    descriptor.Option.Float_Value[0] = 0.0f;
                    descriptor.Option.Int_Value[0] = splash_flag;
                    HH_Effect_Object_Impact_Post(&descriptor);
                }
            }
            if (check1) {
                static float direction[4] = { 0.0f, -2500.0f, 0.0f, 1.0f };
                struct ImpactQueue_Element descriptor;
                float *dst_location;
                float *dst_direction;

                dst_location = descriptor.Option.Vector[0];
                dst_direction = descriptor.Option.Vector[1];
                descriptor.hInstance = 0;
                descriptor.pResultHandle_Address = NULL;
                descriptor.Class_Descriptor = 16;
                sceVu0CopyVector(dst_location, src_location);
                sceVu0CopyVector(dst_direction, direction);
                HH_Effect_Object_Impact_Post(&descriptor);
            }
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
