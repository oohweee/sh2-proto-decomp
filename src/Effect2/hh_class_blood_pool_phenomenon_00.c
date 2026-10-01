/*
 * hh_class_blood_pool_phenomenon_00.c: posts three blood pools around a point over time.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Blood_Pool_Phenomenon_00
 * runs once a frame for each instance and posts other effect objects.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "sdk/libvu0.h"

static struct TimeTable_Infomeation _Time_Table[5] = {
    { 0.0f, 1.0f, 2.0f },
    { 0.0f, 2.0f, 4.0f },
    { 1.0f, 2.0f, 4.0f },
    { 2.0f, 3.0f, 4.0f },
    { 1.6f, 3.0f, 5.0f },
};

static unsigned int Object_Initialize(struct HH_Object_Blood_Pool_Phenomenon_00 *pThis) {
    unsigned int result;

    pThis->Timer = 0.0f;
    pThis->Time_Table_Index = rand() % ARRAY_COUNT(_Time_Table);
    pThis->Post_Count = 0;
    result = 1;
    return result;
}

/**
 * Class main: posts three blood pools (class 12, HH_Class_Blood_02) at random points 50-200
 * units around the position, at times from a randomly chosen table, then ends.
 * @param pBlock   the instance's data block (struct HH_Object_Blood_Pool_Phenomenon_00)
 * @param pElement the element that created it; Option.Vector[0] is the centre and
 *                 Int_Value[0] is passed on to the pools
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Blood_Pool_Phenomenon_00(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Blood_Pool_Phenomenon_00 *pThis;
    float radian;
    float radius_rand;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis);
        pThis->Step = 1;
        break;
    case 1:
        if (pThis->Timer > _Time_Table[pThis->Time_Table_Index].Time[pThis->Post_Count]) {
            radian = Radian_Normalize((rand() % 360) * 0.017453292f);
            radius_rand = 50.0f + rand() % 150;
            {
                float vec[4] = { radius_rand, 0.0f, 0.0f, 1.0f };
                float lwm[4][4];
                struct ImpactQueue_Element descriptor;

                sceVu0UnitMatrix(lwm);
                sceVu0RotMatrixY(lwm, lwm, radian);
                sceVu0TransMatrix(lwm, lwm, pElement->Option.Vector[0]);
                sceVu0ApplyMatrix(descriptor.Option.Vector[0], lwm, vec);
                descriptor.hInstance = 0;
                descriptor.pResultHandle_Address = NULL;
                descriptor.Class_Descriptor = 12;
                descriptor.Option.Int_Value[0] = pElement->Option.Int_Value[0];
                descriptor.Option.Int_Value[1] = 0;
                HH_Effect_Object_Impact_Post(&descriptor);
            }
            pThis->Post_Count++;
            if (pThis->Post_Count >= 3) {
                pThis->Step++;
            }
        }
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
