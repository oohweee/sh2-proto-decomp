/*
 * hh_class_fire_phenomenon_00.c: posts the fires of a burning room, timed by the demo frame.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Fire_Phenomenon_00 runs
 * once a frame for each instance and posts other effect objects.
 */
#include "sh2.h"
#include "sdk/libvu0.h"

static struct Descriptor_Post_Infomeation _post_info_demo[23] = {
    { { -100550.0f, -3600.0f, -74800.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 5, 2 }, 0, { 0, 0 } },
    { { -100550.0f, -3700.0f, -75600.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 1, 2 }, 0, { 0, 0 } },
    { { -100400.0f, 0.0f, -79400.0f, 0.0f }, { 0.0f, -700.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 7, 3 }, 0, { 0, 0 } },
    { { -100500.0f, -400.0f, -78700.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 1666.0f, { 0.0f, 0.0f }, { 0, 0 }, 0, { 0, 0 } },
    { { -100500.0f, -400.0f, -78700.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 45, 47 }, 1665.0f, 2556.0f, { 0.0f, 0.0f }, { 0, 2 }, 0, { 0, 0 } },
    { { -100500.0f, -400.0f, -78700.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 2555.0f, 10000.0f, { 0.0f, 0.0f }, { 0, 0 }, 0, { 0, 0 } },
    { { -100500.0f, -750.0f, -78450.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 1666.0f, { 0.0f, 0.0f }, { 0, 0 }, 0, { 0, 0 } },
    { { -100500.0f, -750.0f, -78450.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 45, 47 }, 1665.0f, 2556.0f, { 0.0f, 0.0f }, { 0, 2 }, 0, { 0, 0 } },
    { { -100500.0f, -750.0f, -78450.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 2555.0f, 10000.0f, { 0.0f, 0.0f }, { 0, 0 }, 0, { 0, 0 } },
    { { -100500.0f, -1200.0f, -78100.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 1666.0f, { 0.0f, 0.0f }, { 4, 2 }, 0, { 0, 0 } },
    { { -100500.0f, -1200.0f, -77950.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 45, 47 }, 1665.0f, 2556.0f, { 0.0f, 0.0f }, { 4, 2 }, 0, { 0, 0 } },
    { { -100500.0f, -1200.0f, -78100.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 2555.0f, 10000.0f, { 0.0f, 0.0f }, { 4, 2 }, 0, { 0, 0 } },
    { { -100500.0f, -2500.0f, -76800.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 0, 0 }, 0, { 0, 0 } },
    { { -100550.0f, -2000.0f, -76400.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 4, 2 }, 0, { 0, 0 } },
    { { -99500.0f, -2800.0f, -76800.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 1, 2 }, 0, { 0, 0 } },
    { { -99500.0f, -2200.0f, -76100.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 5, 2 }, 0, { 0, 0 } },
    { { -99500.0f, -4000.0f, -74900.0f, 0.0f }, { 0.0f, -700.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 7, 3 }, 0, { 0, 0 } },
    { { -99800.0f, -2500.0f, -75700.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 6, 2 }, 0, { 0, 0 } },
    { { -100200.0f, -4500.0f, -73700.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 2, 1 }, 0, { 0, 0 } },
    { { -99750.0f, -5600.0f, -72600.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 2, 1 }, 0, { 0, 0 } },
    { { -99500.0f, 0.0f, -78400.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 44, 0 }, 0.0f, 10000.0f, { 0.0f, 0.0f }, { 6, 2 }, 0, { 0, 0 } },
    { { -100300.0f, -910.0f, -77300.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 44, 0 }, 5000.0f, 10000.0f, { 0.0f, 3.5f }, { 6, 2 }, 0, { 0, 0 } },
    { { -99850.0f, -900.0f, -77300.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 44, 0 }, 5000.0f, 10000.0f, { 0.0f, 4.0f }, { 2, 2 }, 0, { 0, 0 } },
};

static struct Descriptor_Post_Infomeation _post_info_normal[16] = {
    { { -100550.0f, -3600.0f, -74800.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 5, 2 }, 0, { 0, 0 } },
    { { -100550.0f, -3700.0f, -75600.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 1, 2 }, 0, { 0, 0 } },
    { { -100500.0f, -400.0f, -78700.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 0, 0 }, 0, { 0, 0 } },
    { { -100500.0f, -750.0f, -78450.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 0, 0 }, 0, { 0, 0 } },
    { { -100500.0f, -1200.0f, -78100.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 4, 2 }, 0, { 0, 0 } },
    { { -100500.0f, -2500.0f, -76800.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 0, 0 }, 0, { 0, 0 } },
    { { -100550.0f, -2000.0f, -76400.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 4, 2 }, 0, { 0, 0 } },
    { { -99500.0f, -2800.0f, -76800.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 1, 2 }, 0, { 0, 0 } },
    { { -99500.0f, -2200.0f, -76100.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 5, 2 }, 0, { 0, 0 } },
    { { -99500.0f, -4000.0f, -74900.0f, 0.0f }, { 0.0f, -700.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 7, 3 }, 0, { 0, 0 } },
    { { -99800.0f, -2500.0f, -75700.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 6, 2 }, 0, { 0, 0 } },
    { { -100200.0f, -4500.0f, -73700.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 2, 1 }, 0, { 0, 0 } },
    { { -99750.0f, -5600.0f, -72600.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 2, 1 }, 0, { 0, 0 } },
    { { -99500.0f, 0.0f, -78400.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 6, 2 }, 0, { 0, 0 } },
    { { -100300.0f, -910.0f, -77300.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 6, 2 }, 0, { 0, 0 } },
    { { -99850.0f, -900.0f, -77300.0f, 0.0f }, { 0.0f, -500.0f, 0.0f, 0.0f }, { 43, 47 }, 0.0f, 0.0f, { 0.0f, 0.0f }, { 2, 1 }, 0, { 0, 0 } },
};

/* pElement is unused (so missing from the DWARF), but passing it keeps a1 live in the caller. */
static unsigned int Object_Initialize(struct HH_Object_Fire_Phenomenon_00 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    unsigned int i;

    pThis->Timer = 0.0f;
    for (i = 0; i < ARRAY_COUNT(_post_info_demo); i++) {
        struct Descriptor_Post_Infomeation *pInfo;

        pInfo = &_post_info_demo[i];
        pInfo->Step = 0;
    }
    for (i = 0; i < ARRAY_COUNT(_post_info_normal); i++) {
        struct Descriptor_Post_Infomeation *pInfo;

        pInfo = &_post_info_normal[i];
        pInfo->Step = 0;
    }
    result = 1;
    return result;
}

/* Matching: an invented getter (name ours), there to load demo_frame before the pInfo field in
 * the compares as the original does; a plain read of demo_frame doesn't match. */
static inline float Demo_Frame_Get(void) {
    return demo_frame;
}

/**
 * Class main: once a demo is playing (Sh2sys.main_status bit 6), posts and deletes the fire and
 * smoke objects of the _post_info_demo table at their demo frames. When the demo ends, clears
 * those classes (flames 43-45, smoke 47), posts the fires of the _post_info_normal table and ends.
 * @param pBlock   the instance's data block (struct HH_Object_Fire_Phenomenon_00)
 * @param pElement the element that created it (passed to Object_Initialize)
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Fire_Phenomenon_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Fire_Phenomenon_00 *pThis;
    unsigned int i;
    struct ImpactQueue_Element descriptor;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis, pElement);
        pThis->Step = 1;
        break;
    case 1:
        if (!((Sh2sys.main_status >> 6) & 1)) {
            break;
        }
        pThis->Step = 2;
    case 2:
        descriptor.hInstance = 0;
        descriptor.Option.Int_Value[1] = 0;
        for (i = 0; i < ARRAY_COUNT(_post_info_demo); i++) {
            struct Descriptor_Post_Infomeation *pInfo;

            pInfo = &_post_info_demo[i];
            switch (pInfo->Step) {
            case 0:
                if (pInfo->Post_Frame <= Demo_Frame_Get()) {
                    descriptor.Option.Float_Value[0] = pInfo->Float_Argument[0];
                    descriptor.Option.Float_Value[1] = pInfo->Float_Argument[1];
                    sceVu0CopyVector(descriptor.Option.Vector[0], pInfo->Location);
                    descriptor.Class_Descriptor = pInfo->Class_Descriptor[0];
                    descriptor.pResultHandle_Address = &pInfo->hInstance[0];
                    descriptor.Option.Int_Value[0] = pInfo->Int_Argument[0];
                    HH_Effect_Object_Impact_Post(&descriptor);
                    sceVu0AddVector(descriptor.Option.Vector[0], pInfo->Location, pInfo->Offset);
                    descriptor.Class_Descriptor = pInfo->Class_Descriptor[1];
                    descriptor.pResultHandle_Address = &pInfo->hInstance[1];
                    descriptor.Option.Int_Value[0] = pInfo->Int_Argument[1];
                    HH_Effect_Object_Impact_Post(&descriptor);
                    pInfo->Step = 1;
                }
                break;
            case 1:
                if (pInfo->Delete_Frame < Demo_Frame_Get()) {
                    HH_Effect_Object_DesignateHandleInstance_Clear(pInfo->hInstance[0]);
                    HH_Effect_Object_DesignateHandleInstance_Clear(pInfo->hInstance[1]);
                    pInfo->Step = 2;
                }
                break;
            }
        }
        if (!((Sh2sys.main_status >> 6) & 1)) {
            HH_Effect_Object_DesignateClassInstance_Clear(43);
            HH_Effect_Object_DesignateClassInstance_Clear(44);
            HH_Effect_Object_DesignateClassInstance_Clear(45);
            HH_Effect_Object_DesignateClassInstance_Clear(47);
            for (i = 0; i < ARRAY_COUNT(_post_info_normal); i++) {
                struct Descriptor_Post_Infomeation *pInfo;

                pInfo = &_post_info_normal[i];
                switch (pInfo->Step) {
                case 0:
                    descriptor.Option.Float_Value[0] = pInfo->Float_Argument[0];
                    descriptor.Option.Float_Value[1] = pInfo->Float_Argument[1];
                    sceVu0CopyVector(descriptor.Option.Vector[0], pInfo->Location);
                    descriptor.Class_Descriptor = pInfo->Class_Descriptor[0];
                    descriptor.pResultHandle_Address = &pInfo->hInstance[0];
                    descriptor.Option.Int_Value[0] = pInfo->Int_Argument[0];
                    HH_Effect_Object_Impact_Post(&descriptor);
                    sceVu0AddVector(descriptor.Option.Vector[0], pInfo->Location, pInfo->Offset);
                    descriptor.Class_Descriptor = pInfo->Class_Descriptor[1];
                    descriptor.pResultHandle_Address = &pInfo->hInstance[1];
                    descriptor.Option.Int_Value[0] = pInfo->Int_Argument[1];
                    HH_Effect_Object_Impact_Post(&descriptor);
                    pInfo->Step = 2;
                    break;
                }
            }
            pThis->Step = 3;
        }
        pThis->Timer += 1.0f / 30.0f;
        break;
    case 3:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    return result;
}
