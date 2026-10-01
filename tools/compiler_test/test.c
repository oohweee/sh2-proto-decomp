struct shPlayerWork { unsigned char pad[0x32A]; unsigned char light_reverse; unsigned char pad2[0x540 - 0x32B]; };
extern struct shPlayerWork sh2jms;

int PlayerReverseLightCalcIsOn(void) {
    return sh2jms.light_reverse;
}

struct EnFlags { int a; unsigned int flag; };
struct EnLOCAL_DATA { unsigned char pad[0x1C]; struct EnFlags *p; };

void enFlagSetDisplay(struct EnLOCAL_DATA *dp) {
    dp->p->flag |= 0x410;
}

struct DS_Object_Info { unsigned int Offset; unsigned int DataNode_num; unsigned short Type; unsigned short ID; unsigned short Attribute; unsigned short Permission; };
struct Record_Info { struct DS_Object_Info *pObject; void *pAddress; };
struct EntryRecord { unsigned short Enable; unsigned short Controller_ID; unsigned int Handle; unsigned int Group_Handle; unsigned int Condition; float Time_Count; float Time_Max; float Ratio; struct Record_Info Info; };

unsigned int EntryRecord_Type_Get(struct EntryRecord *pER) {
    return pER->Info.pObject->Type;
}

struct FogWork { unsigned char pad[0x15E68]; unsigned short flag; };
extern struct FogWork fwork;

void fogResetStayPos(void) {
    fwork.flag &= ~0x40;
}
