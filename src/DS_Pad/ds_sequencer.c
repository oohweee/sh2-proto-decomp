/*
 * ds_sequencer.c: DualShock 2 vibration sequencer. Vibration records (DSR files)
 * are entered into a table of EntryRecords; every frame the sequencer walks the
 * table, computes each record's actuator level for its current time and sends
 * the per-controller totals. Record state changes go through an event queue.
 */

#include "sh2.h"
#include "libc/string.h"

#define DSR_EntryTable_Max 20
#define DSR_EventQueue_Max 100

static void SequencerManager(void);
static void Sequencer_Type_Hispeed(struct EntryRecord *pER);
static void Sequencer_Type_Lowspeed(struct EntryRecord *pER);
static void Sequencer_Type_Hispeed_Edit(struct EntryRecord *pER);
static void Sequencer_Type_Lowspeed_Edit(struct EntryRecord *pER);
static unsigned int EntryRecord_Enable_Check(struct EntryRecord *pER);
static unsigned int EntryRecord_TimeOver_Check(struct EntryRecord *pER);
static unsigned int EntryRecord_Type_Get(struct EntryRecord *pER);
static unsigned int EntryRecord_ID_Get(struct EntryRecord *pER);
static unsigned int EntryRecord_Attribute_Get(struct EntryRecord *pER);
static unsigned int EntryRecord_Condition_Get(struct EntryRecord *pER);
static void EntryRecord_Condition_Set(struct EntryRecord *pER, unsigned int Condition);
static unsigned int EntryRecord_Handle_Get(struct EntryRecord *pER);
static void EntryRecord_Initialize(struct EntryRecord *pER);
static unsigned int EntryRecord_EntryCount_Get(void);
static unsigned int EntryRecord_EntryFreeCount_Get(void);
static unsigned int EntryRecord_EntryCount_Increment(void);
static unsigned int EntryRecord_EntryCount_Decrement(void);
static float Sequence_Different_Time_Get(void);
static void Sequence_Different_Time_Set(float Time);
static unsigned int EntryRecord_Handle_Create(void);
/* (blank lines keep the assert below on its original source line) */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 324
static struct EntryRecord *EntryRecord_Get_fromTableIndex(unsigned int EntryTable_Index);
static struct EntryRecord *EntryRecord_Get_fromHandle(unsigned int Handle);
static unsigned int EntryRecord_Handle_Search(unsigned int Handle);
static unsigned int EntryRecord_ID_Search(unsigned int ID);
static unsigned int EntryRecord_Attribute_Search(unsigned int Attribute);
static void EntryRecord_Time_Count(struct EntryRecord *pER);
static void EntryRecord_Time_Set(struct EntryRecord *pER, float Time);
static void EntryRecord_Ratio_Set(struct EntryRecord *pER, float Ratio);
static struct EntryRecord *EntryRecordTable_FreeSpace_Search(void);
static void EntryRecordTable_All_Initialize(void);
static unsigned int EntryRecord_Entry(unsigned int *pHandleArray, struct DS_Record_Header *pHeader, unsigned int ControllerID, float Ratio);
static unsigned int DSR_FileFormat_ErrorChecker(struct DS_Record_Header *pHeader);
static unsigned int DSR_FF_Header_ErrorChecker(struct DS_Record_Header *pHeader);
static void TotalActuaterLV_Keeper(unsigned int ControllerID, unsigned int ActuaterType, float ActuaterLV);
static void TotalActuaterLV_Send(unsigned int ControllerID, unsigned int ActuaterType);
static void TotalActuaterLV_Initialize(unsigned int ControllerID, unsigned int ActuaterType);
static float ActuaterLV_Complement(struct DS_Record *pDSR, float Time);
static float ActuaterLV_Complement_Edit(struct DS_Record_Edit *pDSR, float Time);
static int Node_Next_Search(struct Record_Info *pInfo, float Time);
static int Node_Current_Search(struct Record_Info *pInfo, float Time);
static struct DS_Record_Edit *EditNode_Current_Search(struct Record_Info *pInfo, float Time);
static void EventManager(void);
static unsigned int EventMessage_Post(unsigned int Handle, unsigned int EventID, float Value);
static unsigned int EventMessageQueue_enQueue(struct DSR_MU_EventDescriptor *pDescriptor);
static unsigned int EventMessageQueue_deQueue(struct DSR_MU_EventDescriptor *pDescriptor);
static void EventMessageQueue_Initialize(void);
static unsigned int EventMessageQueue_Length_Get(void);
static void DSR_MUD_Initialize(void);

static struct DSR_MU_EventDescriptor _EventQueue[DSR_EventQueue_Max];
static struct Sequencer_Data _sd[2];
static struct DSR_MUD _mud;
static struct EntryRecord EntryTable[DSR_EntryTable_Max];

static struct Sequencer_Data *pSD = _sd;
static struct DSR_MUD *pMUD = &_mud;

static void SequencerManager(void) {
    unsigned int i;
    struct EntryRecord *pER;
    unsigned int Handle;

    for (i = 0; i < DSR_EntryTable_Max; i++) {
        pER = EntryRecord_Get_fromTableIndex(i);
        if (!EntryRecord_Enable_Check(pER)) {
            continue;
        }
        Handle = EntryRecord_Handle_Get(pER);
        switch (EntryRecord_Condition_Get(pER)) {
        case 2:
            switch (EntryRecord_Type_Get(pER)) {
            case 0:
                Sequencer_Type_Hispeed(pER);
                break;
            case 1:
                Sequencer_Type_Lowspeed(pER);
                break;
            case 2:
                Sequencer_Type_Hispeed_Edit(pER);
                break;
            case 3:
                Sequencer_Type_Lowspeed_Edit(pER);
                break;
            }
            EventMessage_Post(Handle, 2, 0.0f);
            if (EntryRecord_TimeOver_Check(pER)) {
                switch (EntryRecord_Type_Get(pER)) {
                case 0:
                case 1:
                    EventMessage_Post(Handle, 1, 0.0f);
                    break;
                case 2:
                case 3:
                    EventMessage_Post(Handle, 6, 0.0f);
                    EventMessage_Post(Handle, 3, 0.0f);
                    break;
                }
            }
            break;
        case 1:
            break;
        case 3:
            break;
        case 0:
            break;
        }
    }
}

static void Sequencer_Type_Hispeed(struct EntryRecord *pER) {
    struct Record_Info *pInfo;
    float time;
    int Node;
    int Node_Next;
    struct DS_Record *pDSR;
    unsigned int now_act_lv_i;
    float section_0;
    float section_1;

    pInfo = &pER->Info;
    time = pER->Time_Count;
    Node = Node_Current_Search(pInfo, time);
    Node_Next = Node_Next_Search(pInfo, time);
    if (Node != -1 && Node_Next != -1) {
        pDSR = (struct DS_Record *)pInfo->pAddress + Node;
        now_act_lv_i = 0;
        if (pDSR->Complement_Enable) {
            now_act_lv_i = pDSR->Actuater_LV ? 1 : 0;
        } else {
            section_0 = pDSR->Time;
            section_1 = section_0 + Sequence_Different_Time_Get();
            if (section_0 <= time && time < section_1) {
                now_act_lv_i = pDSR->Actuater_LV ? 1 : 0;
            }
        }
        now_act_lv_i *= (pER->Ratio > 0.0f) ? 1 : 0;
        TotalActuaterLV_Keeper(pER->Controller_ID, 0, now_act_lv_i);
    }
}

static void Sequencer_Type_Lowspeed(struct EntryRecord *pER) {
    struct Record_Info *pInfo;
    float time;
    int Node;
    int Node_Next;
    struct DS_Record *pDSR;
    float now_act_lv_f;

    pInfo = &pER->Info;
    time = pER->Time_Count;
    Node = Node_Current_Search(pInfo, time);
    Node_Next = Node_Next_Search(pInfo, time);
    if (Node != -1 && Node_Next != -1) {
        pDSR = (struct DS_Record *)pInfo->pAddress + Node;
        now_act_lv_f = 0.0f;
        if (pDSR->Complement_Enable) {
            now_act_lv_f = ActuaterLV_Complement(pDSR, time);
        }
        now_act_lv_f *= pER->Ratio;
        TotalActuaterLV_Keeper(pER->Controller_ID, 1, now_act_lv_f);
    }
}

static void Sequencer_Type_Hispeed_Edit(struct EntryRecord *pER) {
    struct Record_Info *pInfo;
    float time;
    struct DS_Record_Edit *pDSR;
    unsigned int now_act_lv_i;
    float section_0;
    float section_1;

    time = pER->Time_Count;
    pInfo = &pER->Info;
    pDSR = EditNode_Current_Search(pInfo, time);
    if (pDSR && pDSR->pNext) {
        now_act_lv_i = 0;
        if (pDSR->Record.Complement_Enable) {
            now_act_lv_i = pDSR->Record.Actuater_LV ? 1 : 0;
        } else {
            section_0 = pDSR->Record.Time;
            section_1 = section_0 + Sequence_Different_Time_Get();
            if (section_0 <= time && time < section_1) {
                now_act_lv_i = pDSR->Record.Actuater_LV ? 1 : 0;
            }
        }
        now_act_lv_i *= (pER->Ratio > 0.0f) ? 1 : 0;
        TotalActuaterLV_Keeper(pER->Controller_ID, 0, now_act_lv_i);
    }
}

static void Sequencer_Type_Lowspeed_Edit(struct EntryRecord *pER) {
    struct Record_Info *pInfo;
    float time;
    struct DS_Record_Edit *pDSR;
    float now_act_lv_f;

    time = pER->Time_Count;
    pInfo = &pER->Info;
    pDSR = EditNode_Current_Search(pInfo, time);
    if (pDSR && pDSR->pNext) {
        now_act_lv_f = 0.0f;
        if (pDSR->Record.Complement_Enable) {
            now_act_lv_f = ActuaterLV_Complement_Edit(pDSR, time);
        }
        now_act_lv_f *= pER->Ratio;
        TotalActuaterLV_Keeper(pER->Controller_ID, 1, now_act_lv_f);
    }
}

static unsigned int EntryRecord_Enable_Check(struct EntryRecord *pER) {
    return pER->Enable;
}

static unsigned int EntryRecord_TimeOver_Check(struct EntryRecord *pER) {
    unsigned int result;

    result = 0;
    if (pER->Time_Count > pER->Time_Max) {
        result = 1;
    }
    return result;
}

static unsigned int EntryRecord_Type_Get(struct EntryRecord *pER) {
    return pER->Info.pObject->Type;
}

static unsigned int EntryRecord_ID_Get(struct EntryRecord *pER) {
    return pER->Info.pObject->ID;
}

static unsigned int EntryRecord_Attribute_Get(struct EntryRecord *pER) {
    return pER->Info.pObject->Attribute;
}

static unsigned int EntryRecord_Condition_Get(struct EntryRecord *pER) {
    return pER->Condition;
}

static void EntryRecord_Condition_Set(struct EntryRecord *pER, unsigned int Condition) {
    pER->Condition = Condition;
}

static unsigned int EntryRecord_Handle_Get(struct EntryRecord *pER) {
    return pER->Handle;
}

static void EntryRecord_Initialize(struct EntryRecord *pER) {
    memset(pER, 0, sizeof(struct EntryRecord));
}

static unsigned int EntryRecord_EntryCount_Get(void) {
    return pMUD->EntryRecord_Count;
}

static unsigned int EntryRecord_EntryFreeCount_Get(void) {
    return DSR_EntryTable_Max - pMUD->EntryRecord_Count;
}

static unsigned int EntryRecord_EntryCount_Increment(void) {
    return pMUD->EntryRecord_Count++;
}

static unsigned int EntryRecord_EntryCount_Decrement(void) {
    if (pMUD->EntryRecord_Count) {
        pMUD->EntryRecord_Count--;
    }
    return pMUD->EntryRecord_Count;
}

static float Sequence_Different_Time_Get(void) {
    return pMUD->Different_Time;
}

static void Sequence_Different_Time_Set(float Time) {
    pMUD->Different_Time = Time;
}

static unsigned int EntryRecord_Handle_Create(void) {
    unsigned int result;

    result = pMUD->Handle_History + 1;
    while (EntryRecord_Handle_Search(result)) {
        result++;
    }
    pMUD->Handle_History = result;
    return result;
}

static struct EntryRecord *EntryRecord_Get_fromTableIndex(unsigned int EntryTable_Index) {
    assert_dw((EntryTable_Index < DSR_EntryTable_Max));
    return &EntryTable[EntryTable_Index];
}

static struct EntryRecord *EntryRecord_Get_fromHandle(unsigned int Handle) {
    void *result;
    int i;
    struct EntryRecord *pER;

    result = NULL;
    for (i = 0; i < DSR_EntryTable_Max; i++) {
        pER = EntryRecord_Get_fromTableIndex(i);
        if (EntryRecord_Enable_Check(pER) && EntryRecord_Handle_Get(pER) == Handle) {
            result = pER;
            break;
        }
    }
    return result;
}

static unsigned int EntryRecord_Handle_Search(unsigned int Handle) {
    unsigned int result;

    result = 0;
    if (EntryRecord_Get_fromHandle(Handle)) {
        result = 1;
    }
    return result;
}

static unsigned int EntryRecord_ID_Search(unsigned int ID) {
    unsigned int result;
    int i;
    struct EntryRecord *pER;

    result = 0;
    for (i = 0; i < DSR_EntryTable_Max; i++) {
        pER = EntryRecord_Get_fromTableIndex(i);
        if (EntryRecord_Enable_Check(pER) && EntryRecord_ID_Get(pER) == ID) {
            result = EntryRecord_Handle_Get(pER);
            break;
        }
    }
    return result;
}

static unsigned int EntryRecord_Attribute_Search(unsigned int Attribute) {
    unsigned int result;
    int i;
    struct EntryRecord *pER;

    result = 0;
    for (i = 0; i < DSR_EntryTable_Max; i++) {
        pER = EntryRecord_Get_fromTableIndex(i);
        if (EntryRecord_Enable_Check(pER) && EntryRecord_Attribute_Get(pER) == Attribute) {
            result = EntryRecord_Handle_Get(pER);
            break;
        }
    }
    return result;
}

static void EntryRecord_Time_Count(struct EntryRecord *pER) {
    pER->Time_Count += Sequence_Different_Time_Get();
}

static void EntryRecord_Time_Set(struct EntryRecord *pER, float Time) {
    pER->Time_Count = Time;
}

static void EntryRecord_Ratio_Set(struct EntryRecord *pER, float Ratio) {
    pER->Ratio = Ratio;
}

static struct EntryRecord *EntryRecordTable_FreeSpace_Search(void) {
    void *result;
    int i;
    struct EntryRecord *pER;

    result = NULL;
    for (i = 0; i < DSR_EntryTable_Max; i++) {
        pER = EntryRecord_Get_fromTableIndex(i);
        if (pER->Enable == 0) {
            result = pER;
            break;
        }
    }
    return result;
}

static void EntryRecordTable_All_Initialize(void) {
    unsigned int i;

    for (i = 0; i < DSR_EntryTable_Max; i++) {
        EntryRecord_Initialize(EntryRecord_Get_fromTableIndex(i));
    }
}

static unsigned int EntryRecord_Entry(unsigned int *pHandleArray, struct DS_Record_Header *pHeader, unsigned int ControllerID, float Ratio) {
    unsigned int result;
    unsigned int i;
    unsigned int permission_check;
    struct DS_Object_Info *pObject_Info;

    result = 0;
    if (DSR_FileFormat_ErrorChecker(pHeader) == 0 && EntryRecord_EntryFreeCount_Get() >= pHeader->Object_Num) {
        permission_check = 1;
        pObject_Info = (struct DS_Object_Info *)(pHeader + 1);
        switch (pObject_Info->Permission) {
        case 1:
            if (EntryRecord_Attribute_Search(pObject_Info->Attribute)) {
                permission_check = 0;
            }
            break;
        case 2:
            if (EntryRecord_ID_Search(pObject_Info->ID)) {
                permission_check = 0;
            }
            break;
        case 3:
            if (EntryRecord_Attribute_Search(pObject_Info->Attribute) || EntryRecord_ID_Search(pObject_Info->ID)) {
                permission_check = 0;
            }
            break;
        case 4:
            if (EntryRecord_Attribute_Search(pObject_Info->Attribute) && EntryRecord_ID_Search(pObject_Info->ID)) {
                permission_check = 0;
            }
            break;
        case 0:
            break;
        }
        if (permission_check) {
            switch (pObject_Info->Type) {
            case 0:
            case 1:
                for (i = 0; i < pHeader->Object_Num; i++, pObject_Info++) {
                    struct EntryRecord *pER;

                    pER = EntryRecordTable_FreeSpace_Search();
                    pER->Enable = 1;
                    pER->Controller_ID = ControllerID;
                    pER->Handle = EntryRecord_Handle_Create();
                    pER->Ratio = Ratio;
                    pER->Info.pObject = pObject_Info;
                    pER->Info.pAddress = (char *)pHeader + pObject_Info->Offset;
                    pER->Time_Max = ((struct DS_Record *)pER->Info.pAddress)[pER->Info.pObject->DataNode_num - 1].Time;
                    if (pHandleArray) {
                        pHandleArray[i] = pER->Handle;
                    }
                    EntryRecord_EntryCount_Increment();
                }
                break;
            case 2:
            case 3:
                for (i = 0; i < pHeader->Object_Num; i++, pObject_Info++) {
                    struct EntryRecord *pER;
                    struct DS_Record_Edit *pTail;

                    pER = EntryRecordTable_FreeSpace_Search();
                    pER->Enable = 1;
                    pER->Controller_ID = ControllerID;
                    pER->Handle = EntryRecord_Handle_Create();
                    pER->Ratio = Ratio;
                    pER->Info.pObject = pObject_Info;
                    pER->Info.pAddress = (void *)pObject_Info->Offset;
                    pTail = pER->Info.pAddress;
                    while (pTail->pNext) {
                        pTail = pTail->pNext;
                    }
                    pER->Time_Max = pTail->Record.Time;
                    if (pHandleArray) {
                        pHandleArray[i] = pER->Handle;
                    }
                    EntryRecord_EntryCount_Increment();
                }
                break;
            }
            result = 1;
        }
    }
    return result;
}

static unsigned int DSR_FileFormat_ErrorChecker(struct DS_Record_Header *pHeader) {
    unsigned int result;
    unsigned int num;
    struct DS_Object_Info *pInfo;

    result = DSR_FF_Header_ErrorChecker(pHeader);
    if (result == 0) {
        pInfo = (struct DS_Object_Info *)(pHeader + 1);
        for (num = 0; num < pHeader->Object_Num; num++, pInfo++) {
            if (pInfo->Offset == 0) {
                result++;
            }
            switch (pInfo->Type) {
            case 0:
            case 1:
            case 2:
            case 3:
                break;
            default:
                result++;
                break;
            }
            if (pInfo->DataNode_num <= 1) {
                result++;
            }
        }
    }
    return result;
}

/* (blank lines keep the assert below on its original source line) */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 901
static unsigned int DSR_FF_Header_ErrorChecker(struct DS_Record_Header *pHeader) {
    unsigned int result;

    result = 0;
    if (pHeader == NULL) {
        assert_dw(0);
    }
    if (pHeader->Revision != 1) {
        result++;
    }
    if (pHeader->Object_Num == 0) {
        result++;
    }
    return result;
}

static void TotalActuaterLV_Keeper(unsigned int ControllerID, unsigned int ActuaterType, float ActuaterLV) {
    pSD[ControllerID].ActuaterLV[ActuaterType] = ActuaterLV;
}

static void TotalActuaterLV_Send(unsigned int ControllerID, unsigned int ActuaterType) {
    float act_lv;
    float act_ratio;

    act_lv = pSD[ControllerID].ActuaterLV[ActuaterType];
    act_ratio = DSS_Wrapper_AllVibrationRatio_Get();
    switch (ActuaterType) {
    case 0:
    case 2:
        if (act_ratio != 0.0f) {
            act_ratio = 1.0f;
        }
        break;
    case 1:
    case 3:
        break;
    }
    act_lv *= act_ratio;
    DSS_Wrapper_ActuaterData_Send(ControllerID, ActuaterType, act_lv);
    TotalActuaterLV_Initialize(ControllerID, ActuaterType);
}

static void TotalActuaterLV_Initialize(unsigned int ControllerID, unsigned int ActuaterType) {
    pSD[ControllerID].ActuaterLV[ActuaterType] = 0.0f;
}

static float ActuaterLV_Complement(struct DS_Record *pDSR, float Time) {
    float result;
    float time_current;
    float time_next;
    float comp_ratio;
    float act_lv_current;
    float act_lv_next;

    time_current = pDSR[0].Time;
    time_next = pDSR[1].Time;
    comp_ratio = (Time - time_current) / (time_next - time_current);
    act_lv_current = pDSR[0].Actuater_LV;
    act_lv_next = pDSR[1].Actuater_LV;
    result = act_lv_next * comp_ratio + act_lv_current * (1.0f - comp_ratio);
    return result;
}

static float ActuaterLV_Complement_Edit(struct DS_Record_Edit *pDSR, float Time) {
    float result;
    float time_current;
    float time_next;
    float comp_ratio;
    float act_lv_current;
    float act_lv_next;

    time_current = pDSR->Record.Time;
    time_next = pDSR->pNext->Record.Time;
    comp_ratio = (Time - time_current) / (time_next - time_current);
    act_lv_current = pDSR->Record.Actuater_LV;
    act_lv_next = pDSR->pNext->Record.Actuater_LV;
    result = act_lv_next * comp_ratio + act_lv_current * (1.0f - comp_ratio);
    return result;
}

static int Node_Next_Search(struct Record_Info *pInfo, float Time) {
    unsigned int node_num;
    struct DS_Record *pDSR;
    unsigned int i;
    int result;

    node_num = pInfo->pObject->DataNode_num;
    pDSR = pInfo->pAddress;
    result = -1;
    for (i = 0; i < node_num; i++, pDSR++) {
        if (Time < pDSR->Time) {
            result = i;
            break;
        }
    }
    return result;
}

static int Node_Current_Search(struct Record_Info *pInfo, float Time) {
    int result;
    int num;

    result = -1;
    num = Node_Next_Search(pInfo, Time);
    if (num > 0) {
        result = num - 1;
    }
    return result;
}

static struct DS_Record_Edit *EditNode_Current_Search(struct Record_Info *pInfo, float Time) {
    struct DS_Record_Edit *result;
    struct DS_Record_Edit *pDSR;

    result = NULL;
    for (pDSR = pInfo->pAddress; pDSR; pDSR = pDSR->pNext) {
        if (Time < pDSR->Record.Time) {
            result = pDSR->pPrev;
            break;
        }
    }
    return result;
}

static void EventManager(void) {
    struct DSR_MU_EventDescriptor Descriptor;
    struct EntryRecord *pER;

    while (EventMessageQueue_deQueue(&Descriptor)) {
        pER = EntryRecord_Get_fromHandle(Descriptor.Handle);
        if (pER) {
            switch (Descriptor.EventID) {
            case 0:
                break;
            case 1:
                EntryRecord_EntryCount_Decrement();
                EntryRecord_Initialize(pER);
                break;
            case 2:
                EntryRecord_Time_Count(pER);
                break;
            case 3:
                EntryRecord_Time_Set(pER, Descriptor.Value);
                break;
            case 4:
                EntryRecord_Ratio_Set(pER, Descriptor.Value);
                break;
            case 5:
                EntryRecord_Condition_Set(pER, 2);
                break;
            case 6:
                EntryRecord_Condition_Set(pER, 3);
                break;
            }
        }
    }
}

static unsigned int EventMessage_Post(unsigned int Handle, unsigned int EventID, float Value) {
    unsigned int result = 0;
    struct DSR_MU_EventDescriptor Descriptor = { 0, 0, 0.0f };

    Descriptor.Handle = Handle;
    Descriptor.EventID = EventID;
    Descriptor.Value = Value;
    if (EventMessageQueue_enQueue(&Descriptor)) {
        result = 1;
    }
    return result;
}

static unsigned int EventMessageQueue_enQueue(struct DSR_MU_EventDescriptor *pDescriptor) {
    unsigned int result;
    unsigned int length;

    result = 0;
    length = EventMessageQueue_Length_Get();
    if (pMUD->EventQueue_Count < length) {
        _EventQueue[pMUD->enQueue_Pos++] = *pDescriptor;
        pMUD->enQueue_Pos %= length;
        pMUD->EventQueue_Count++;
        result = 1;
    }
    return result;
}

static unsigned int EventMessageQueue_deQueue(struct DSR_MU_EventDescriptor *pDescriptor) {
    unsigned int result;
    unsigned int length;

    result = 0;
    length = EventMessageQueue_Length_Get();
    if (pMUD->EventQueue_Count) {
        *pDescriptor = _EventQueue[pMUD->deQueue_Pos++];
        pMUD->deQueue_Pos %= length;
        pMUD->EventQueue_Count--;
        result = 1;
    }
    return result;
}

static void EventMessageQueue_Initialize(void) {
    pMUD->deQueue_Pos = 0;
    pMUD->enQueue_Pos = 0;
    pMUD->EventQueue_Count = 0;
}

static unsigned int EventMessageQueue_Length_Get(void) {
    return DSR_EventQueue_Max;
}

static void DSR_MUD_Initialize(void) {
    pMUD->Handle_History = 0;
    pMUD->EntryRecord_Count = 0;
    Sequence_Different_Time_Set(1.0f / 30.0f);
    EventMessageQueue_Initialize();
}

unsigned int DSR_Entry0(void *pAddress, unsigned int ControllerID, float Ratio) {
    unsigned int result;
    struct DS_Record_Header *pHeader;
    int num;
    unsigned int handle[64];

    utilExclLockOtherThread();
    pHeader = pAddress;
    num = pHeader->Object_Num;
    result = EntryRecord_Entry(handle, pHeader, ControllerID, Ratio);
    if (result) {
        while (num--) {
            EventMessage_Post(handle[num], 5, 0.0f);
        }
    }
    utilExclUnlockOtherThread();
    return result;
}

void DSR_Sequencer_Initialize(void) {
    DSR_MUD_Initialize();
    EntryRecordTable_All_Initialize();
    TotalActuaterLV_Initialize(0, 0);
    TotalActuaterLV_Initialize(0, 1);
    TotalActuaterLV_Initialize(1, 0);
    TotalActuaterLV_Initialize(1, 1);
}

void DSR_Sequencer(void) {
    SequencerManager();
    EventManager();
    if (EntryRecord_EntryCount_Get()) {
        TotalActuaterLV_Send(0, 0);
        TotalActuaterLV_Send(0, 1);
        TotalActuaterLV_Send(1, 0);
        TotalActuaterLV_Send(1, 1);
    }
}

void DSR_Sequence_Different_Time_Set(float Different_Time) {
    Sequence_Different_Time_Set(Different_Time);
}
