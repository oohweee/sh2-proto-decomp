/*
 * hh_effect_object_def.c: the Effect2 object group: its class table, memory pools, instance
 * table and impact queue. A class's pool exists always (Existent 1), never (0), or only in the
 * rooms on its list (2); some classes are posted automatically when their room loads.
 */
#include "sh2.h"
#include "libc/string.h"

/*
 * FIXME(header): Object_DataPool_Infomeation is 16-byte aligned in the original (it sits at 0x10
 * in MemoryPool_AssociationInfomeation and is copied with lq/sq). The generated header only
 * aligns the member; this local twin carries the alignment for the copies.
 */
struct Object_DataPool_Infomeation_A {
    unsigned int Block_Size;
    unsigned int Block_Index_Max;
    void *pBlock_Table;
    struct Object_DataBlock_Header *pFreeBlock_List;
} __attribute__((aligned(16)));
#define POOL(x) (*(struct Object_DataPool_Infomeation_A *)&(x))

/*
 * The prefix/suffix functions take (void *, unsigned int) like the class table expects, but never
 * use their arguments, so the DWARF (and the generated prototypes) show them as (void).
 */
#define CLASS_FIX(f) ((unsigned int (*)(void *, unsigned int))(f))

static struct Object_Class _pObject_Class_List[48] = {
    { NULL, NULL, NULL, 0 },
    { NULL, HH_Class_Plural_Phenomenon_00, NULL, 0 },
    { NULL, HH_Class_Plural_Phenomenon_01, NULL, 0 },
    { NULL, HH_Class_Blood_Pool_Phenomenon_00, NULL, 0 },
    { NULL, HH_Class_Blood_Pool_Phenomenon_01, NULL, 0 },
    { NULL, HH_Class_Blood_Splash_Phenomenon_00, NULL, 0 },
    { NULL, HH_Class_Blood_Splash_Phenomenon_01, NULL, 0 },
    { NULL, HH_Class_Blood_Splash_Phenomenon_02, NULL, 0 },
    { NULL, HH_Class_Blood_Stick_00, NULL, 0 },
    { CLASS_FIX(HH_Class_Prefix_Blood_FootMark_00), HH_Class_Blood_FootMark_00, CLASS_FIX(HH_Class_Suffix_Blood_FootMark_00), 0 },
    { CLASS_FIX(HH_Class_Prefix_Blood_00), HH_Class_Blood_00, CLASS_FIX(HH_Class_Suffix_Blood_00), 0 },
    { CLASS_FIX(HH_Class_Prefix_Blood_01), HH_Class_Blood_01, CLASS_FIX(HH_Class_Suffix_Blood_01), 0 },
    { CLASS_FIX(HH_Class_Prefix_Blood_02), HH_Class_Blood_02, CLASS_FIX(HH_Class_Suffix_Blood_02), 0 },
    { CLASS_FIX(HH_Class_Prefix_Blood_03), HH_Class_Blood_03, CLASS_FIX(HH_Class_Suffix_Blood_03), 0 },
    { CLASS_FIX(HH_Class_Prefix_Blood_04), HH_Class_Blood_04, CLASS_FIX(HH_Class_Suffix_Blood_04), 0 },
    { CLASS_FIX(HH_Class_Prefix_Blood_05), HH_Class_Blood_05, CLASS_FIX(HH_Class_Suffix_Blood_05), 0 },
    { NULL, HH_Class_Raise_Dust_Phenomenon_00, NULL, 0 },
    { CLASS_FIX(HH_Class_Prefix_Dust_00), HH_Class_Dust_00, CLASS_FIX(HH_Class_Suffix_Dust_00), 0 },
    { NULL, HH_Class_Water_Splash_Phenomenon_00, NULL, 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_20), HH_Class_Water_20, CLASS_FIX(HH_Class_Suffix_Water_20), 0 },
    { NULL, HH_Class_Glass_Break_Phenomenon_00, NULL, 0 },
    { CLASS_FIX(HH_Class_Prefix_GlassPiece_00), HH_Class_GlassPiece_00, CLASS_FIX(HH_Class_Suffix_GlassPiece_00), 0 },
    { CLASS_FIX(HH_Class_Prefix_Boat_Mask), HH_Class_Boat_Mask, CLASS_FIX(HH_Class_Suffix_Boat_Mask), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_00), HH_Class_Water_00, CLASS_FIX(HH_Class_Suffix_Water_00), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_01), HH_Class_Water_01, CLASS_FIX(HH_Class_Suffix_Water_01), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_02), HH_Class_Water_02, CLASS_FIX(HH_Class_Suffix_Water_02), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_03), HH_Class_Water_03, CLASS_FIX(HH_Class_Suffix_Water_03), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_04), HH_Class_Water_04, CLASS_FIX(HH_Class_Suffix_Water_04), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_06), HH_Class_Water_06, CLASS_FIX(HH_Class_Suffix_Water_06), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_07), HH_Class_Water_07, CLASS_FIX(HH_Class_Suffix_Water_07), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_08), HH_Class_Water_08, CLASS_FIX(HH_Class_Suffix_Water_08), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_09), HH_Class_Water_09, CLASS_FIX(HH_Class_Suffix_Water_09), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_10), HH_Class_Water_10, CLASS_FIX(HH_Class_Suffix_Water_10), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_11), HH_Class_Water_11, CLASS_FIX(HH_Class_Suffix_Water_11), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_12), HH_Class_Water_12, CLASS_FIX(HH_Class_Suffix_Water_12), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_13), HH_Class_Water_13, CLASS_FIX(HH_Class_Suffix_Water_13), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_14), HH_Class_Water_14, CLASS_FIX(HH_Class_Suffix_Water_14), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_15), HH_Class_Water_15, CLASS_FIX(HH_Class_Suffix_Water_15), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_30), HH_Class_Water_30, CLASS_FIX(HH_Class_Suffix_Water_30), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_80), HH_Class_Water_80, CLASS_FIX(HH_Class_Suffix_Water_80), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_90), HH_Class_Water_90, CLASS_FIX(HH_Class_Suffix_Water_90), 0 },
    { CLASS_FIX(HH_Class_Prefix_Water_Pool_00), HH_Class_Water_Pool_00, CLASS_FIX(HH_Class_Suffix_Water_Pool_00), 0 },
    { NULL, HH_Class_Fire_Phenomenon_00, NULL, 0 },
    { CLASS_FIX(HH_Class_Prefix_Flame_00), HH_Class_Flame_00, CLASS_FIX(HH_Class_Suffix_Flame_00), 0 },
    { CLASS_FIX(HH_Class_Prefix_Flame_01), HH_Class_Flame_01, CLASS_FIX(HH_Class_Suffix_Flame_01), 0 },
    { CLASS_FIX(HH_Class_Prefix_Flame_02), HH_Class_Flame_02, CLASS_FIX(HH_Class_Suffix_Flame_02), 0 },
    { CLASS_FIX(HH_Class_Prefix_Flame_03), HH_Class_Flame_03, CLASS_FIX(HH_Class_Suffix_Flame_03), 0 },
    { CLASS_FIX(HH_Class_Prefix_Smoke_00), HH_Class_Smoke_00, CLASS_FIX(HH_Class_Suffix_Smoke_00), 0 },
};

static int _blood_splash_02_list[1] = { 0x7 };
static int _blood_05_list[1] = { 0x7 };
static int _fire_pheno_list[1] = { 0xAA };
static int _glass_list[1] = { 0x8A };
static int _water_01_list[1] = { 0x63 };
static int _water_03_list[1] = { 0x80 };
static int _water_04_list[1] = { 0x85 };
static int _water_06_list[1] = { 0x7A };
static int _water_07_list[1] = { 0x7C };
static int _water_08_list[1] = { 0x7E };
static int _water_09_list[1] = { 0x82 };
static int _water_10_list[1] = { 0xBB };
static int _water_11_list[1] = { 0xB6 };
static int _water_12_list[1] = { 0xB7 };
static int _water_13_list[1] = { 0xAB };
static int _water_14_list[1] = { 0xB8 };
static int _water_15_list[1] = { 0xB9 };
static int _water_30_list[1] = { 0x21 };
static int _water_80_list[1] = { 0xE };
static int _water_90_list[1] = { 0x3 };
static int _boat_mask_list[1] = { 0xE };

static struct MemoryPool_AssociationInfomeation _MemoryPool_AssociationInfo_Table[48] = {
    { 0, { 0, 0, NULL, NULL }, 0, NULL },
    { 1, { 32, 2, NULL, NULL }, 0, NULL },
    { 1, { 32, 16, NULL, NULL }, 0, NULL },
    { 1, { 32, 4, NULL, NULL }, 0, NULL },
    { 1, { 32, 4, NULL, NULL }, 0, NULL },
    { 1, { 80, 20, NULL, NULL }, 0, NULL },
    { 1, { 32, 2, NULL, NULL }, 0, NULL },
    { 1, { 80, 20, NULL, NULL }, 1, _blood_splash_02_list },
    { 1, { 32, 2, NULL, NULL }, 0, NULL },
    { 1, { 32, 20, NULL, NULL }, 0, NULL },
    { 1, { 64, 200, NULL, NULL }, 0, NULL },
    { 1, { 64, 500, NULL, NULL }, 0, NULL },
    { 1, { 64, 42, NULL, NULL }, 0, NULL },
    { 1, { 64, 20, NULL, NULL }, 0, NULL },
    { 1, { 64, 42, NULL, NULL }, 0, NULL },
    { 1, { 64, 500, NULL, NULL }, 1, _blood_05_list },
    { 1, { 32, 16, NULL, NULL }, 0, NULL },
    { 1, { 64, 192, NULL, NULL }, 0, NULL },
    { 1, { 32, 16, NULL, NULL }, 0, NULL },
    { 1, { 64, 160, NULL, NULL }, 0, NULL },
    { 0, { 64, 1, NULL, NULL }, 1, _glass_list },
    { 0, { 64, 1, NULL, NULL }, 1, _glass_list },
    { 2, { 32, 1, NULL, NULL }, 1, _boat_mask_list },
    { 0, { 18800, 1, NULL, NULL }, 0, NULL },
    { 2, { 5072, 1, NULL, NULL }, 1, _water_01_list },
    { 0, { 3856, 1, NULL, NULL }, 0, NULL },
    { 2, { 6432, 1, NULL, NULL }, 1, _water_03_list },
    { 2, { 6864, 1, NULL, NULL }, 1, _water_04_list },
    { 2, { 8288, 1, NULL, NULL }, 1, _water_06_list },
    { 2, { 8976, 1, NULL, NULL }, 1, _water_07_list },
    { 2, { 5888, 1, NULL, NULL }, 1, _water_08_list },
    { 2, { 3696, 1, NULL, NULL }, 1, _water_09_list },
    { 2, { 5184, 1, NULL, NULL }, 1, _water_10_list },
    { 2, { 5440, 1, NULL, NULL }, 1, _water_11_list },
    { 2, { 3712, 1, NULL, NULL }, 1, _water_12_list },
    { 2, { 3248, 1, NULL, NULL }, 1, _water_13_list },
    { 2, { 4288, 1, NULL, NULL }, 1, _water_14_list },
    { 2, { 6368, 1, NULL, NULL }, 1, _water_15_list },
    { 2, { 3040, 1, NULL, NULL }, 1, _water_30_list },
    { 2, { 9856, 1, NULL, NULL }, 1, _water_80_list },
    { 2, { 8832, 1, NULL, NULL }, 1, _water_90_list },
    { 1, { 64, 400, NULL, NULL }, 0, NULL },
    { 2, { 32, 1, NULL, NULL }, 1, _fire_pheno_list },
    { 2, { 8048, 20, NULL, NULL }, 1, _fire_pheno_list },
    { 2, { 8048, 4, NULL, NULL }, 1, _fire_pheno_list },
    { 2, { 8048, 3, NULL, NULL }, 1, _fire_pheno_list },
    { 1, { 8048, 10, NULL, NULL }, 0, NULL },
    { 2, { 1632, 20, NULL, NULL }, 1, _fire_pheno_list },
};

static unsigned int _Object_Class_Priority_List[49] = {
    0, 3, 4, 5, 6, 7, 8, 9, 41, 10, 12, 16, 17, 14, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 11, 15, 13, 18, 19, 42, 43, 44, 45, 46, 47, 20, 21, 1, 2, 48,
};

static int _AutoPost_Association_List[18][2] = {
    { 0x63, 24 },
    { 0x80, 26 },
    { 0x85, 27 },
    { 0x7A, 28 },
    { 0x7C, 29 },
    { 0x7E, 30 },
    { 0x82, 31 },
    { 0xBB, 32 },
    { 0xB6, 33 },
    { 0xB7, 34 },
    { 0xAB, 35 },
    { 0xB8, 36 },
    { 0xB9, 37 },
    { 0x21, 38 },
    { 0xE, 39 },
    { 0xE, 22 },
    { 0x3, 40 },
    { 0xAA, 42 },
};

static struct ImpactQueue_Element _Queue[500];
static struct Object_Instance _Object_instance_table[600] __attribute__((aligned(64)));
static struct Object_Instance *_pHierarchyTable[48];
static struct Object_DataPool_Infomeation _Object_Data_Table[48];
static struct Object_Group_Infomeation _Object_Group_Info[1];

static unsigned int MemoryPool_AllRelease(struct Object_DataPool_Infomeation *pPool_Info_Table, struct MemoryPool_AssociationInfomeation *pAssoci_Info_Table, unsigned int Class_Kind_Max) {
    unsigned int result;
    unsigned int i;
    struct Object_DataPool_Infomeation DataInfo_NULL;
    struct MemoryPool_AssociationInfomeation *pAssoci_Info;
    struct Object_DataPool_Infomeation *pPool_Info;

    memset(&DataInfo_NULL, 0, sizeof(DataInfo_NULL));
    for (i = 0; i < Class_Kind_Max; i++) {
        pPool_Info = &pPool_Info_Table[i];
        pAssoci_Info = &pAssoci_Info_Table[i];
        POOL(*pPool_Info) = POOL(DataInfo_NULL);
    }
    result = 1;
    return result;
}

static unsigned int MemoryPool_Inspect_and_Allocate(struct Object_DataPool_Infomeation *pPool_Info_Table, struct MemoryPool_AssociationInfomeation *pAssoci_Info_Table, unsigned int Class_Kind_Max) {
    unsigned int i;
    unsigned int result;
    void *pAddress;
    unsigned int Base;
    unsigned int End;
    struct MemoryPool_AssociationInfomeation *pAssoci_Info;
    struct Object_DataPool_Infomeation *pPool_Info;
    int room_name;
    unsigned int j;

    result = 0;
    pAddress = HH_MemoryManager_AllocateMemoryBlock_Get(1);
    Base = (unsigned int)pAddress;
    for (i = 0; i < Class_Kind_Max; i++) {
        pAssoci_Info = &pAssoci_Info_Table[i];
        pPool_Info = &pPool_Info_Table[i];
        switch (pAssoci_Info->Existent) {
        case 1:
            POOL(*pPool_Info) = POOL(pAssoci_Info->DataPool_Info);
            if (pPool_Info->pBlock_Table == NULL) {
                pPool_Info->pBlock_Table = pAddress;
                pAddress = HH_MemoryManager_DesignateSize_Alignment16Address_Calculator(pAddress, pPool_Info->Block_Size, pPool_Info->Block_Index_Max);
            }
            result = 1;
            break;
        case 2:
            room_name = RoomNameJms();
            for (j = 0; j < pAssoci_Info->LinkList_Max; j++) {
                if (room_name == pAssoci_Info->pLinkList[j]) {
                    POOL(*pPool_Info) = POOL(pAssoci_Info->DataPool_Info);
                    pPool_Info->pBlock_Table = pAddress;
                    pAddress = HH_MemoryManager_DesignateSize_Alignment16Address_Calculator(pAddress, pPool_Info->Block_Size, pPool_Info->Block_Index_Max);
                    result = 1;
                    break;
                }
            }
            break;
        case 0:
            break;
        }
    }
    End = (unsigned int)pAddress;
    printf("Object Work Allocate Size = %d kB\n", (End - Base) / 1024);
    return result;
}

static unsigned int MemoryPool_AllClear(struct Object_DataPool_Infomeation *pPool_Info_Table, struct MemoryPool_AssociationInfomeation *pAssoci_Info_Table, unsigned int Class_Kind_Max) {
    unsigned int i;
    unsigned int result;
    struct MemoryPool_AssociationInfomeation *pAssoci_Info;
    struct Object_DataPool_Infomeation *pPool_Info;

    result = 0;
    for (i = 0; i < Class_Kind_Max; i++) {
        pPool_Info = &pPool_Info_Table[i];
        pAssoci_Info = &pAssoci_Info_Table[i];
        if (pPool_Info->pBlock_Table != NULL) {
            memset(pPool_Info->pBlock_Table, 0, pPool_Info->Block_Size * pPool_Info->Block_Index_Max);
            result = 1;
        }
    }
    return result;
}

static unsigned int MemoryPool_Controller(struct Object_DataPool_Infomeation *pPool_Info_Table, struct MemoryPool_AssociationInfomeation *pAssoci_Info_Table, unsigned int Class_Kind_Max) {
    unsigned int result;

    result = 0;
    if (HH_MemoryManager_AllocateMemoryBlock_Check(1)) {
        result = 1;
        result *= MemoryPool_AllRelease(pPool_Info_Table, pAssoci_Info_Table, Class_Kind_Max);
        result *= MemoryPool_Inspect_and_Allocate(pPool_Info_Table, pAssoci_Info_Table, Class_Kind_Max);
        result *= MemoryPool_AllClear(pPool_Info_Table, pAssoci_Info_Table, Class_Kind_Max);
    }
    return result;
}

static void Effect_Object_Initialize(void) {
    struct Object_Group_Infomeation *pInfo;

    pInfo = HH_Effect_Object_Infomeation_Get();
    Object_Group_Infomeation_Set(pInfo);
    Object_Group_QueueInfomeation_Set(pInfo, _Queue, 500);
    Object_Group_ClassAssociationInfomeation_Set(pInfo, _pObject_Class_List, _Object_Data_Table, _Object_Class_Priority_List, 48);
    Object_Group_InstanceTableInfomeation_Set(pInfo, _Object_instance_table, _pHierarchyTable, 600);
    Object_Group_All_Initialize(pInfo);
}

/**
 * Posts an impact descriptor to the Effect2 object group: a new effect object of class
 * pElement->Class_Descriptor, or new options for the instance pElement->hInstance.
 */
void HH_Effect_Object_Impact_Post(struct ImpactQueue_Element *pElement) {
    struct Object_Group_Infomeation *pInfo;

    pInfo = HH_Effect_Object_Infomeation_Get();
    ImpactDescriptor_Post(pInfo, pElement);
}

/** Ends every effect object of one class. */
void HH_Effect_Object_DesignateClassInstance_Clear(unsigned int Class_Descriptor) {
    struct Object_Group_Infomeation *pInfo;

    pInfo = HH_Effect_Object_Infomeation_Get();
    Object_Group_InstanceTable_DesignateClassDescriptorAttach_Initialize(pInfo, Class_Descriptor);
}

/** Ends the effect object with handle hInstance (0: none). */
void HH_Effect_Object_DesignateHandleInstance_Clear(unsigned int hInstance) {
    struct Object_Group_Infomeation *pInfo;

    pInfo = HH_Effect_Object_Infomeation_Get();
    Object_Group_InstanceTable_DesignateInstanceHandleAttach_Initialize(pInfo, hInstance);
}

/** Returns the Effect2 object group. */
struct Object_Group_Infomeation *HH_Effect_Object_Infomeation_Get(void) {
    return _Object_Group_Info;
}

/** Runs the Effect2 object group for one frame. */
void HH_Effect_Object_Manager(void) {
    struct Object_Group_Infomeation *pInfo;

    pInfo = HH_Effect_Object_Infomeation_Get();
    Object_Group_Manager(pInfo);
    shPadTrigger(0, 0x4000); /* the results are unused (left-over debug code) */
    shPadTrigger(0, 0x80000);
}

/**
 * Lays out the class data pools for the current room in memory block 1 and initializes the
 * object group. Returns 1, or 0 if block 1 isn't allocated or no pool was laid out.
 */
unsigned int HH_Effect_Object_MemoryBlock_Allocate(void) {
    unsigned int result;
    unsigned int kind_max;
    struct MemoryPool_AssociationInfomeation *pAssoci_Info_Table;
    struct Object_DataPool_Infomeation *pPool_Info_Table;

    result = 0;
    pPool_Info_Table = _Object_Data_Table;
    pAssoci_Info_Table = _MemoryPool_AssociationInfo_Table;
    kind_max = 48;
    if (MemoryPool_Controller(pPool_Info_Table, pAssoci_Info_Table, kind_max)) {
        Effect_Object_Initialize();
        result = 1;
    }
    return result;
}

/** Posts the effect objects the current room always has (its water surface, boat mask, fires). */
void HH_Effect_Object_AutoPost(void) {
    unsigned int i;
    int room_name;
    struct ImpactQueue_Element descriptor;

    room_name = RoomNameJms();
    for (i = 0; i < 18; i++) {
        if (room_name == _AutoPost_Association_List[i][0]) {
            descriptor.Class_Descriptor = _AutoPost_Association_List[i][1];
            descriptor.hInstance = 0;
            descriptor.pResultHandle_Address = NULL;
            ImpactDescriptor_Post(_Object_Group_Info, &descriptor);
        }
    }
}
