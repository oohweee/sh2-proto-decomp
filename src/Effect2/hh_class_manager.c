/*
 * hh_class_manager.c: the HH effect object manager. Impact descriptors are queued; each frame the
 * manager turns them into instances (or passes them to the instance they name), then runs every
 * class in priority order: its prefix, its main function once per instance, its suffix. Each
 * instance gets a data block from its class's pool; a disabled block frees the instance.
 */
#include "sh2.h"

/* Matching: called without a prototype in the original (arguments evaluated last to first). */
void *memset();

/*
 * FIXME(header): ImpactQueue_ElementOption's vectors are 16-byte aligned in the original (as
 * common.h's sceVu0FVECTOR), so ImpactQueue_Element and its Option are copied with lq/sq. The
 * generated header lacks the alignment; these local twins carry it for the copies.
 */
struct ImpactQueue_ElementOption_A {
    float Vector[2][4];
    float Float_Value[2];
    int Int_Value[2];
} __attribute__((aligned(16)));

struct ImpactQueue_Element_A {
    unsigned int Class_Descriptor;
    unsigned int hInstance;
    unsigned int *pResultHandle_Address;
    unsigned int Reserved[1];
    struct ImpactQueue_ElementOption_A Option;
};

#define OPTION_A(x) (*(struct ImpactQueue_ElementOption_A *)&(x))
#define ELEMENT_A(x) (*(struct ImpactQueue_Element_A *)&(x))

static unsigned int QueueObject_deQueue(struct ImpactQueue_Object *pQueue, struct ImpactQueue_Element *pElement);
static struct Object_Instance *Instance_Search_from_InstanceHandle(struct Object_Group_Infomeation *pInfo, unsigned int hInstance);
static unsigned int Instance_Create(struct Object_Group_Infomeation *pInfo, struct ImpactQueue_Element *pElement);
static unsigned int FreeInstance_Stack_FreeCheck(struct Object_InstanceTable_Infomeation *pInfo);
static unsigned int FreeInstance_Stack_Push(struct Object_InstanceTable_Infomeation *pInfo, struct Object_Instance *pInstance);
static unsigned int InstanceHierarchyTable_Discard(struct Object_InstanceTable_Infomeation *pInfo, struct Object_Instance *pInstance);
static unsigned int InstanceHierarchyTable_DesignateInstance_Initialize(struct Object_InstanceTable_Infomeation *pInstance_Info, struct Object_DataPool_Infomeation *pPool_Info, struct Object_Instance *pInstance);
static unsigned int FreeDataBlock_Stack_FreeCheck(struct Object_DataPool_Infomeation *pInfo);
static unsigned int FreeDataBlock_Stack_Push(struct Object_DataPool_Infomeation *pInfo, struct Object_DataBlock_Header *pHeader);
static unsigned int Exception_Handling_Instance_Create(struct Object_Group_Infomeation *pInfo, unsigned int Flag, unsigned int Class_Descriptor);

static unsigned int ImpactManager(struct Object_Group_Infomeation *pInfo) {
    unsigned int result;
    struct ImpactQueue_Element element;
    struct Object_InstanceTable_Infomeation *pInstance_Info;
    struct Object_Instance *pInstance;
    unsigned int free_instance_check;
    unsigned int free_data_block_check;

    result = 0;
    pInstance_Info = &pInfo->InstanceTable_Info;
    while (QueueObject_deQueue(&pInfo->Queue_Info, &element)) {
        if (pInfo->Association_Info.pClass_List[element.Class_Descriptor].Disable) {
            continue;
        }
        if (element.hInstance != 0) {
            pInstance = Instance_Search_from_InstanceHandle(pInfo, element.hInstance);
            if (pInstance != NULL) {
                OPTION_A(pInstance->Element.Option) = OPTION_A(element.Option);
                result = 1;
            }
        } else {
            free_instance_check = FreeInstance_Stack_FreeCheck(pInstance_Info);
            free_data_block_check = FreeDataBlock_Stack_FreeCheck(&pInfo->Association_Info.pDataPool_Info[element.Class_Descriptor]);
            if (!free_instance_check || !free_data_block_check) {
                Exception_Handling_Instance_Create(pInfo, 1, element.Class_Descriptor);
            }
            if (Instance_Create(pInfo, &element)) {
                result = 1;
            }
        }
    }
    return result;
}

static unsigned int InstanceManager(struct Object_Group_Infomeation *pInfo) {
    unsigned int result;
    unsigned int i;
    struct Object_Class_Association_Infomeation *pClass_Info;
    struct Object_InstanceTable_Infomeation *pInstance_Info;
    struct Object_Class *pClass;
    struct Object_Instance *pInstance;
    struct Object_DataPool_Infomeation *pPool_Info;
    void *pBlock_Table;
    unsigned int Block_Index_Max;
    struct Object_Instance *pInstance_Current;

    result = 0;
    pInstance_Info = &pInfo->InstanceTable_Info;
    for (i = 0; i < pInfo->Association_Info.Class_Max; i++) {
        pClass_Info = &pInfo->Association_Info;
        pClass = &pClass_Info->pClass_List[pClass_Info->pClass_Priority_List[i]];
        pInstance = pInstance_Info->pHierarchyTable[pClass_Info->pClass_Priority_List[i]];
        pPool_Info = &pClass_Info->pDataPool_Info[pClass_Info->pClass_Priority_List[i]];
        pBlock_Table = pPool_Info->pBlock_Table;
        Block_Index_Max = pPool_Info->Block_Index_Max;
        if (pBlock_Table == NULL || pInstance == NULL) {
            continue;
        }
        if (pClass->pPrefix != NULL) {
            pClass->pPrefix(pBlock_Table, Block_Index_Max);
        }
        while (pInstance != NULL) {
            pInstance_Current = pInstance;
            pInstance = pInstance->pNext;
            pClass->pMain(pInstance_Current->pBlock, &pInstance_Current->Element);
            pInstance_Current->Timer += 1.0f / 30.0f;
            if (!((struct Object_DataBlock_Header *)pInstance_Current->pBlock)->Enable) {
                InstanceHierarchyTable_DesignateInstance_Initialize(pInstance_Info, pPool_Info, pInstance_Current);
            }
        }
        if (pClass->pSuffix != NULL) {
            pClass->pSuffix(pBlock_Table, Block_Index_Max);
        }
        result = 1;
    }
    return result;
}

static unsigned int InstanceCreateSuppressEnvironment_Initialize(struct Object_Group_Infomeation *pInfo) {
    unsigned int result;
    unsigned int i;

    for (i = 1; i < pInfo->Association_Info.Class_Max; i++) {
        pInfo->Association_Info.pClass_List[i].Disable = 0;
    }
    pInfo->Association_Info.pClass_List[0].Disable = 1;
    result = 0;
    return result;
}

static void QueueObject_Initialize(struct ImpactQueue_Object *pQueue) {
    pQueue->enQueue = 0;
    pQueue->deQueue = 0;
    pQueue->Length_Current = 0;
}

static unsigned int QueueObject_enQueue(struct ImpactQueue_Object *pQueue, struct ImpactQueue_Element *pElement) {
    unsigned int result;

    result = 0;
    if (pQueue->Length_Current < pQueue->Length_Max) {
        ELEMENT_A(pQueue->pElement[pQueue->enQueue++]) = ELEMENT_A(*pElement);
        pQueue->enQueue %= pQueue->Length_Max;
        pQueue->Length_Current++;
        result = 1;
    }
    return result;
}

static unsigned int QueueObject_deQueue(struct ImpactQueue_Object *pQueue, struct ImpactQueue_Element *pElement) {
    unsigned int result;

    result = 0;
    if (pQueue->Length_Current != 0) {
        ELEMENT_A(*pElement) = ELEMENT_A(pQueue->pElement[pQueue->deQueue++]);
        pQueue->deQueue %= pQueue->Length_Max;
        pQueue->Length_Current--;
        result = 1;
    }
    return result;
}

/* pInfo is unused (so it is missing from the DWARF): a handle is the instance's address. */
static struct Object_Instance *Instance_Search_from_InstanceHandle(struct Object_Group_Infomeation *pInfo, unsigned int hInstance) {
    struct Object_Instance *result;

    result = NULL;
    if (((struct Object_Instance *)hInstance)->Enable) {
        result = (struct Object_Instance *)hInstance;
    }
    return result;
}

static unsigned int Instance_DesignateClassDescriptorAttach_Count(struct Object_InstanceTable_Infomeation *pInfo, unsigned int Class_Descriptor) {
    unsigned int result;
    struct Object_Instance *pInstance;

    result = 0;
    pInstance = pInfo->pHierarchyTable[Class_Descriptor];
    while (pInstance != NULL) {
        result++;
        pInstance = pInstance->pNext;
    }
    return result;
}

static unsigned int InstanceHandle_Get_from_ClassDescriptor_and_AttachCount(struct Object_InstanceTable_Infomeation *pInfo, unsigned int Class_Descriptor, unsigned int CountIndex) {
    unsigned int result;
    unsigned int count;
    struct Object_Instance *pInstance;

    result = 0;
    count = 0;
    pInstance = pInfo->pHierarchyTable[Class_Descriptor];
    while (pInstance != NULL) {
        if (CountIndex == count) {
            result = pInstance->hInstance;
            break;
        }
        count++;
        pInstance = pInstance->pNext;
    }
    return result;
}

static struct Object_Instance *FreeInstance_Stack_Pop(struct Object_InstanceTable_Infomeation *pInfo);
static unsigned int InstanceHierarchyTable_Registry(struct Object_InstanceTable_Infomeation *pInfo, struct Object_Instance *pInstance);
static struct Object_DataBlock_Header *FreeDataBlock_Stack_Pop(struct Object_DataPool_Infomeation *pInfo);

/* Matching: an inline helper (name ours): loading pBlock (the argument) before the 1 needs the
 * inline call. */
static inline void DataBlock_Enable(void *pBlock) {
    ((struct Object_DataBlock_Header *)pBlock)->Enable = 1;
}

static unsigned int Instance_Create(struct Object_Group_Infomeation *pInfo, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct Object_Instance *pInstance;
    struct Object_DataPool_Infomeation *pPool_Info;

    result = 0;
    pPool_Info = &pInfo->Association_Info.pDataPool_Info[pElement->Class_Descriptor];
    if (FreeDataBlock_Stack_FreeCheck(pPool_Info)) {
        if ((pInstance = FreeInstance_Stack_Pop(&pInfo->InstanceTable_Info)) != NULL) {
            pInstance->Enable = 1;
            pInstance->hInstance = (unsigned int)pInstance;
            ELEMENT_A(pInstance->Element) = ELEMENT_A(*pElement);
            pInstance->Element.hInstance = pInstance->hInstance;
            if (pInstance->Element.pResultHandle_Address != NULL) {
                *pInstance->Element.pResultHandle_Address = pInstance->hInstance;
            }
            pInstance->pBlock = FreeDataBlock_Stack_Pop(pPool_Info);
            DataBlock_Enable(pInstance->pBlock);
            InstanceHierarchyTable_Registry(&pInfo->InstanceTable_Info, pInstance);
            result = 1;
        }
    }
    return result;
}

static unsigned int Instance_OldestEntry_Search(struct Object_Group_Infomeation *pInfo, unsigned int Flag, unsigned int Class_Descriptor) {
    unsigned int result;
    unsigned int i;
    unsigned int max;
    float old_time;
    struct Object_Instance *pInstance;

    result = 0;
    i = 0;
    max = pInfo->Association_Info.Class_Max;
    old_time = 0.0f;
    if (Flag) {
        i = Class_Descriptor;
        max = Class_Descriptor + 1;
    }
    for (; i < max; i++) {
        pInstance = pInfo->InstanceTable_Info.pHierarchyTable[i];
        while (pInstance != NULL) {
            if (pInstance->Timer > old_time) {
                old_time = pInstance->Timer;
                result = pInstance->hInstance;
            }
            pInstance = pInstance->pNext;
        }
    }
    return result;
}

static unsigned int FreeInstance_Stack_FreeCheck(struct Object_InstanceTable_Infomeation *pInfo) {
    unsigned int result;

    result = 0;
    if (pInfo->pFreeTable != NULL) {
        result = 1;
    }
    return result;
}

static struct Object_Instance *FreeInstance_Stack_Pop(struct Object_InstanceTable_Infomeation *pInfo) {
    struct Object_Instance *result;

    result = NULL;
    if (pInfo->pFreeTable != NULL) {
        result = pInfo->pFreeTable;
        pInfo->pFreeTable = result->pNext;
        if (pInfo->pFreeTable != NULL) {
            pInfo->pFreeTable->pPrev = NULL;
        }
        result->pPrev = NULL;
        result->pNext = NULL;
    }
    return result;
}

static unsigned int FreeInstance_Stack_Push(struct Object_InstanceTable_Infomeation *pInfo, struct Object_Instance *pInstance) {
    unsigned int result;

    memset(pInstance, 0, sizeof(struct Object_Instance));
    if (pInfo->pFreeTable != NULL) {
        pInstance->pNext = pInfo->pFreeTable;
        pInfo->pFreeTable->pPrev = pInstance;
        pInfo->pFreeTable = pInstance;
    } else {
        pInfo->pFreeTable = pInstance;
    }
    result = 1;
    return result;
}

static unsigned int InstanceHierarchyTable_Registry(struct Object_InstanceTable_Infomeation *pInfo, struct Object_Instance *pInstance) {
    unsigned int result;
    struct Object_Instance **pRegistry_Hierarchy;

    pRegistry_Hierarchy = &pInfo->pHierarchyTable[pInstance->Element.Class_Descriptor];
    if (*pRegistry_Hierarchy != NULL) {
        pInstance->pPrev = NULL;
        pInstance->pNext = *pRegistry_Hierarchy;
        (*pRegistry_Hierarchy)->pPrev = pInstance;
        *pRegistry_Hierarchy = pInstance;
    } else {
        *pRegistry_Hierarchy = pInstance;
        pInstance->pPrev = NULL;
        pInstance->pNext = NULL;
    }
    result = 1;
    return result;
}

static unsigned int InstanceHierarchyTable_Discard(struct Object_InstanceTable_Infomeation *pInfo, struct Object_Instance *pInstance) {
    unsigned int result;
    struct Object_Instance **pRegistry_Hierarchy;

    pRegistry_Hierarchy = &pInfo->pHierarchyTable[pInstance->Element.Class_Descriptor];
    if (pInstance->pPrev != NULL) {
        pInstance->pPrev->pNext = pInstance->pNext;
    } else {
        *pRegistry_Hierarchy = pInstance->pNext;
    }
    if (pInstance->pNext != NULL) {
        pInstance->pNext->pPrev = pInstance->pPrev;
    }
    result = 1;
    return result;
}

static unsigned int InstanceHierarchyTable_DesignateInstance_Initialize(struct Object_InstanceTable_Infomeation *pInstance_Info, struct Object_DataPool_Infomeation *pPool_Info, struct Object_Instance *pInstance) {
    unsigned int result;

    if (pInstance->Element.pResultHandle_Address != NULL) {
        *pInstance->Element.pResultHandle_Address = 0;
    }
    FreeDataBlock_Stack_Push(pPool_Info, pInstance->pBlock);
    InstanceHierarchyTable_Discard(pInstance_Info, pInstance);
    FreeInstance_Stack_Push(pInstance_Info, pInstance);
    result = 1;
    return result;
}

static unsigned int FreeDataBlock_Stack_FreeCheck(struct Object_DataPool_Infomeation *pInfo) {
    unsigned int result;

    result = 0;
    if (pInfo->pFreeBlock_List != NULL) {
        result = 1;
    }
    return result;
}

static struct Object_DataBlock_Header *FreeDataBlock_Stack_Pop(struct Object_DataPool_Infomeation *pInfo) {
    struct Object_DataBlock_Header *result;

    result = NULL;
    if (pInfo->pFreeBlock_List != NULL) {
        result = pInfo->pFreeBlock_List;
        pInfo->pFreeBlock_List = result->pNext;
        result->pNext = NULL;
    }
    return result;
}

static unsigned int FreeDataBlock_Stack_Push(struct Object_DataPool_Infomeation *pInfo, struct Object_DataBlock_Header *pHeader) {
    unsigned int result;

    memset(pHeader, 0, pInfo->Block_Size);
    if (pInfo->pFreeBlock_List != NULL) {
        pHeader->pNext = pInfo->pFreeBlock_List;
        pInfo->pFreeBlock_List = pHeader;
    } else {
        pInfo->pFreeBlock_List = pHeader;
    }
    result = 1;
    return result;
}

static unsigned int Exception_Handling_Instance_Create(struct Object_Group_Infomeation *pInfo, unsigned int Flag, unsigned int Class_Descriptor) {
    unsigned int result;
    unsigned int hInstance;
    struct Object_Instance *pInstance;
    struct Object_Class_Association_Infomeation *pClass_Info;
    struct Object_DataPool_Infomeation *pPool_Info;

    result = 0;
    hInstance = Instance_OldestEntry_Search(pInfo, Flag, Class_Descriptor);
    if (hInstance != 0) {
        pInstance = Instance_Search_from_InstanceHandle(pInfo, hInstance);
        pClass_Info = &pInfo->Association_Info;
        pPool_Info = &pClass_Info->pDataPool_Info[Class_Descriptor];
        if (pInstance->Element.pResultHandle_Address != NULL) {
            *pInstance->Element.pResultHandle_Address = 0;
        }
        FreeDataBlock_Stack_Push(pPool_Info, pInstance->pBlock);
        InstanceHierarchyTable_Discard(&pInfo->InstanceTable_Info, pInstance);
        FreeInstance_Stack_Push(&pInfo->InstanceTable_Info, pInstance);
        result = 1;
    }
    return result;
}

static unsigned int ClassAssociation_DataPool_Initialize(struct Object_Group_Infomeation *pInfo, unsigned int Class_Descriptor) {
    unsigned int result;
    struct Object_DataPool_Infomeation *pPool_Info;
    unsigned int i;
    struct Object_DataBlock_Header *pHeader;

    result = 0;
    if (pInfo != NULL) {
        pPool_Info = &pInfo->Association_Info.pDataPool_Info[Class_Descriptor];
        if (pPool_Info->pBlock_Table != NULL) {
            memset(pPool_Info->pBlock_Table, 0, pPool_Info->Block_Size * pPool_Info->Block_Index_Max);
            pHeader = pPool_Info->pBlock_Table;
            for (i = 0; i < pPool_Info->Block_Index_Max - 1; i++, pHeader = pHeader->pNext) {
                pHeader->pNext = (struct Object_DataBlock_Header *)((char *)pHeader + pPool_Info->Block_Size);
            }
            pHeader->pNext = NULL;
            pPool_Info->pFreeBlock_List = pPool_Info->pBlock_Table;
            result = 1;
        }
    }
    return result;
}

static unsigned int InstanceTable_DesignateClassDescriptorAttach_Initialize(struct Object_Group_Infomeation *pInfo, unsigned int Class_Descriptor) {
    unsigned int result;
    struct Object_Instance *pInstance;
    struct Object_DataPool_Infomeation *pPool_Info;
    struct Object_Instance *pInstance_Current;

    result = 0;
    pInstance = pInfo->InstanceTable_Info.pHierarchyTable[Class_Descriptor];
    pPool_Info = &pInfo->Association_Info.pDataPool_Info[Class_Descriptor];
    while (pInstance != NULL) {
        pInstance_Current = pInstance;
        pInstance = pInstance->pNext;
        if (pInstance_Current->Element.pResultHandle_Address != NULL) {
            *pInstance_Current->Element.pResultHandle_Address = 0;
        }
        FreeDataBlock_Stack_Push(pPool_Info, pInstance_Current->pBlock);
        InstanceHierarchyTable_Discard(&pInfo->InstanceTable_Info, pInstance_Current);
        FreeInstance_Stack_Push(&pInfo->InstanceTable_Info, pInstance_Current);
        result = 1;
    }
    return result;
}

static unsigned int ClassAssociation_DataPool_All_Initialize(struct Object_Group_Infomeation *pInfo) {
    unsigned int result;
    unsigned int i;

    result = 0;
    if (pInfo != NULL) {
        for (i = 0; i < pInfo->Association_Info.Class_Max; i++) {
            ClassAssociation_DataPool_Initialize(pInfo, i);
        }
        result = 1;
    }
    return result;
}

static unsigned int InstanceTable_All_Initialize(struct Object_Group_Infomeation *pInfo) {
    unsigned int result;
    unsigned int i;
    struct Object_Instance *pCurrent;
    struct Object_Instance *pPrev;

    result = 0;
    if (pInfo != NULL) {
        pInfo->InstanceTable_Info.hInstance_History = 0;
        memset(pInfo->InstanceTable_Info.pInstanceTable, 0, pInfo->InstanceTable_Info.Instance_Max * sizeof(struct Object_Instance));
        pInfo->InstanceTable_Info.pFreeTable = pInfo->InstanceTable_Info.pInstanceTable;
        memset(pInfo->InstanceTable_Info.pHierarchyTable, 0, pInfo->Association_Info.Class_Max * sizeof(struct Object_Instance *));
        pCurrent = pInfo->InstanceTable_Info.pInstanceTable;
        pPrev = NULL;
        for (i = 0; i < pInfo->InstanceTable_Info.Instance_Max; i++, pCurrent++) {
            pCurrent->pPrev = pPrev;
            pCurrent->pNext = pCurrent + 1;
            pPrev = pCurrent;
        }
        pCurrent[-1].pNext = NULL;
        result = 1;
    }
    return result;
}

/**
 * Queues an impact descriptor for the next manager run (hangs if its class is out of range).
 * @param pInfo       the object group
 * @param pDescriptor the descriptor (copied)
 * @return 1 if queued, 0 if the queue was full
 */
unsigned int ImpactDescriptor_Post(struct Object_Group_Infomeation *pInfo, struct ImpactQueue_Element *pDescriptor) {
    unsigned int result;

    result = 0;
    if (pDescriptor->Class_Descriptor >= 48) {
        printf("--------------------Class_Descriptor Over!!");
        while (1) {}
    }
    if (QueueObject_enQueue(&pInfo->Queue_Info, pDescriptor)) {
        result = 1;
    }
    return result;
}

/**
 * Runs the object group for one frame: the first call only resets the class enables; later calls
 * (while the group is enabled) process the queued impacts and run every class.
 * @param pInfo the object group
 * @return 1, or 0 if pInfo is NULL
 */
unsigned int Object_Group_Manager(struct Object_Group_Infomeation *pInfo) {
    unsigned int result;

    result = 0;
    if (pInfo != NULL) {
        switch (pInfo->Step) {
        case 0:
            InstanceCreateSuppressEnvironment_Initialize(pInfo);
            pInfo->Step = 1;
            result = 1;
            break;
        case 1:
            if (pInfo->Enable) {
                ImpactManager(pInfo);
                InstanceCreateSuppressEnvironment_Initialize(pInfo);
                HH_DBG_Wrapper_T0_COUNT_Get();
                InstanceManager(pInfo);
                HH_DBG_Wrapper_T0_COUNT_Delta();
            }
            result = 1;
            break;
        }
    }
    return result;
}

/** Enables an object group and resets its step. Returns 1, or 0 if pInfo is NULL. */
unsigned int Object_Group_Infomeation_Set(struct Object_Group_Infomeation *pInfo) {
    unsigned int result;

    result = 0;
    if (pInfo != NULL) {
        pInfo->Enable = 1;
        pInfo->Step = 0;
        result = 1;
    }
    return result;
}

/**
 * Gives an object group its impact queue and empties it.
 * @param pInfo         the object group
 * @param pElement_Base queue storage
 * @param Length_Max    queue length
 * @return 1, or 0 if an argument is NULL or 0
 */
unsigned int Object_Group_QueueInfomeation_Set(struct Object_Group_Infomeation *pInfo, struct ImpactQueue_Element *pElement_Base, unsigned int Length_Max) {
    unsigned int result;
    struct ImpactQueue_Object *pQueue_Info;

    result = 0;
    pQueue_Info = &pInfo->Queue_Info;
    if (pInfo != NULL && pElement_Base != NULL && Length_Max != 0) {
        pQueue_Info->pElement = pElement_Base;
        pQueue_Info->Length_Max = Length_Max;
        QueueObject_Initialize(pQueue_Info);
        result = 1;
    }
    return result;
}

/**
 * Gives an object group its class table, data pools and class run order.
 * @param pInfo               the object group
 * @param pClass_List         class table (prefix, main, suffix functions)
 * @param pPool_Info_Base     one data pool per class
 * @param pClass_Priority_List class indices in the order they run
 * @param Class_Max           number of classes
 * @return 1, or 0 if an argument is NULL or 0
 */
unsigned int Object_Group_ClassAssociationInfomeation_Set(struct Object_Group_Infomeation *pInfo, struct Object_Class *pClass_List, struct Object_DataPool_Infomeation *pPool_Info_Base, unsigned int *pClass_Priority_List, unsigned int Class_Max) {
    unsigned int result;

    result = 0;
    if (pInfo != NULL && pClass_List != NULL && pPool_Info_Base != NULL && pClass_Priority_List != NULL && Class_Max != 0) {
        pInfo->Association_Info.pClass_List = pClass_List;
        pInfo->Association_Info.pDataPool_Info = pPool_Info_Base;
        pInfo->Association_Info.pClass_Priority_List = pClass_Priority_List;
        pInfo->Association_Info.Class_Max = Class_Max;
        result = 1;
    }
    return result;
}

/**
 * Gives an object group its instance table and per-class instance lists.
 * @param pInfo                    the object group
 * @param pInstance_Base           instance storage
 * @param pInstance_HierarchyTable one instance list head per class
 * @param Instance_Max             number of instances
 * @return 1, or 0 if an argument is NULL or 0
 */
unsigned int Object_Group_InstanceTableInfomeation_Set(struct Object_Group_Infomeation *pInfo, struct Object_Instance *pInstance_Base, struct Object_Instance **pInstance_HierarchyTable, unsigned int Instance_Max) {
    unsigned int result;

    result = 0;
    if (pInfo != NULL && pInstance_Base != NULL && pInstance_HierarchyTable != NULL && Instance_Max != 0) {
        pInfo->InstanceTable_Info.hInstance_History = 0;
        pInfo->InstanceTable_Info.Instance_Max = Instance_Max;
        pInfo->InstanceTable_Info.pInstanceTable = pInstance_Base;
        pInfo->InstanceTable_Info.pHierarchyTable = pInstance_HierarchyTable;
        pInfo->InstanceTable_Info.pFreeTable = pInstance_Base;
        result = 1;
    }
    return result;
}

/**
 * Resets an object group: frees every data block and instance and empties the queue. Returns 1, or
 * 0 if pInfo is NULL.
 */
unsigned int Object_Group_All_Initialize(struct Object_Group_Infomeation *pInfo) {
    unsigned int result;

    result = 0;
    if (pInfo != NULL) {
        ClassAssociation_DataPool_All_Initialize(pInfo);
        InstanceTable_All_Initialize(pInfo);
        QueueObject_Initialize(&pInfo->Queue_Info);
        result = 1;
    }
    return result;
}

/** Frees every instance of one class. Returns 1 if there was any. */
unsigned int Object_Group_InstanceTable_DesignateClassDescriptorAttach_Initialize(struct Object_Group_Infomeation *pInfo, unsigned int Class_Descriptor) {
    return InstanceTable_DesignateClassDescriptorAttach_Initialize(pInfo, Class_Descriptor);
}

/** Frees the instance with handle hInstance (0: none). Returns 1 if one was freed. */
unsigned int Object_Group_InstanceTable_DesignateInstanceHandleAttach_Initialize(struct Object_Group_Infomeation *pInfo, unsigned int hInstance) {
    unsigned int result;
    struct Object_Instance *pInstance;

    result = 0;
    if (hInstance != 0) {
        pInstance = Instance_Search_from_InstanceHandle(pInfo, hInstance);
        result = InstanceHierarchyTable_DesignateInstance_Initialize(&pInfo->InstanceTable_Info, &pInfo->Association_Info.pDataPool_Info[pInstance->Element.Class_Descriptor], pInstance);
    }
    return result;
}

/** Returns the number of live instances of one class. */
unsigned int ObjectInstance_DesignateClassDescriptorAttach_Count(struct Object_Group_Infomeation *pInfo, unsigned int Class_Descriptor) {
    return Instance_DesignateClassDescriptorAttach_Count(&pInfo->InstanceTable_Info, Class_Descriptor);
}

/** Returns the handle of the CountIndex-th live instance of one class, or 0 if there are fewer. */
unsigned int ObjectInstanceHandle_Get_from_ClassDescriptor_and_AttachCount(struct Object_Group_Infomeation *pInfo, unsigned int Class_Descriptor, unsigned int CountIndex) {
    return InstanceHandle_Get_from_ClassDescriptor_and_AttachCount(&pInfo->InstanceTable_Info, Class_Descriptor, CountIndex);
}

/** Returns the impact element of the instance with handle hInstance, or NULL if it isn't live. */
struct ImpactQueue_Element *ObjectInstance_Element_Get(struct Object_Group_Infomeation *pInfo, unsigned int hInstance) {
    struct ImpactQueue_Element *result;
    struct Object_Instance *pInstance;

    result = NULL;
    pInstance = Instance_Search_from_InstanceHandle(pInfo, hInstance);
    if (pInstance != NULL) {
        result = &pInstance->Element;
    }
    return result;
}

/** Returns the data block of the instance with handle hInstance, or NULL if it isn't live. */
void *ObjectInstance_DataBlock_Get(struct Object_Group_Infomeation *pInfo, unsigned int hInstance) {
    void *result;
    struct Object_Instance *pInstance;

    result = NULL;
    pInstance = Instance_Search_from_InstanceHandle(pInfo, hInstance);
    if (pInstance != NULL) {
        result = pInstance->pBlock;
    }
    return result;
}
