/* hh_effect_object_texture.c: loads, registers and transfers the effect textures. */
#include "sh2.h"
#include "libc/string.h"


#define TEXTURE_CONTEXT_MAX 21
#define TEXTURE_BUFFER_BLOCK_MAX 5
#define EFF_VALID_ID 0xEF04

static int _fire_01_list[1] = { 170 };
static int _ps_ground_list[6] = { 122, 124, 126, 128, 130, 133 };
static int _ps_gesui_list[1] = { 99 };
static int _ru_bar_list[1] = { 182 };
static int _ru_kitchen_list[1] = { 183 };
static int _ru_passba_list[1] = { 184 };
static int _ru_passbb_list[1] = { 185 };
static int _ru_stair_list[1] = { 171 };
static int _ap_stair_list[1] = { 33 };
static int _water_01_list[1] = { 33 };
static int _water_02_list[3] = { 171, 182, 183 };
static int _water_40_list[3] = { 98, 37, 38 };
static int _glass_list[1] = { 138 };
static int _fly_01_list[2] = { 22, 187 };
static int _msi_00_list[1] = { 101 };
static int _boatmask_list[1] = { 14 };

static struct HH_Local_TextureInfomeation _TextureInfomeation_Table[TEXTURE_CONTEXT_MAX] = {
    { 0, data_pic_effect_playable0_tbn2, 2, 0, 0, NULL, NULL },
    { 1, data_pic_effect_fog_tex, 1, 1, 0, NULL, NULL },
    { 2, data_pic_effect_poison_tex, 2, 1, 0, NULL, NULL },
    { 3, data_pic_effect_fire01_tbn2, 3, 0, 1, _fire_01_list, NULL },
    { 4, data_pic_effect_psground02_tbn2, 3, 0, 6, _ps_ground_list, NULL },
    { 5, data_pic_effect_gesui01_tbn2, 3, 0, 1, _ps_gesui_list, NULL },
    { 6, data_pic_effect_bar00_tbn2, 3, 0, 1, _ru_bar_list, NULL },
    { 7, data_pic_effect_kitchen00_tbn2, 3, 0, 1, _ru_kitchen_list, NULL },
    { 8, data_pic_effect_passba00_tbn2, 3, 0, 1, _ru_passba_list, NULL },
    { 9, data_pic_effect_passbb00_tbn2, 3, 0, 1, _ru_passbb_list, NULL },
    { 10, data_pic_effect_hbstair00_tbn2, 3, 0, 1, _ru_stair_list, NULL },
    { 11, data_pic_effect_apstair00_tbn2, 3, 0, 1, _ap_stair_list, NULL },
    { 12, data_pic_effect_water00_tbn2, 3, 0, 0, NULL, NULL },
    { 13, data_pic_effect_water01_tbn2, 0, 0, 1, _water_01_list, NULL },
    { 14, data_pic_effect_water02_tbn2, 3, 0, 3, _water_02_list, NULL },
    { 15, data_pic_effect_gesui00_tbn2, 3, 0, 0, NULL, NULL },
    { 16, data_pic_effect_water40_tbn2, 3, 0, 3, _water_40_list, NULL },
    { 17, data_pic_effect_glas03_tbn2, 3, 0, 1, _glass_list, NULL },
    { 18, data_pic_effect_fly1_tbn2, 3, 1, 2, _fly_01_list, NULL },
    { 19, data_pic_effect_msi00_tbn2, 3, 1, 1, _msi_00_list, NULL },
    { 20, data_pic_effect_boatmask_tbn2, 3, 0, 1, _boatmask_list, NULL },
};

static int *_TextureHeader_Table[TEXTURE_CONTEXT_MAX] = { NULL };

static unsigned int _Transport_Current_Priority;
static struct HH_Local_TextureContext _TextureContext_Table[TEXTURE_CONTEXT_MAX];
static unsigned int _texture_buffer_enable[TEXTURE_BUFFER_BLOCK_MAX];
static unsigned int _finish_count;
static unsigned int _sync_count;
static unsigned int _send_count;

/*
 * Matching: the tables are read through small inline getters (names invented). With them, the
 * locations our DWARF gives pContext, pTex_Info and priority are the original's (v0: variables
 * that only copy an inline function's result) in every function of the file; read directly they
 * get s-registers, and TextureBinary_DesignateTexture_Load_toAlwaysBuffer's registers differ.
 */
static inline struct HH_Local_TextureInfomeation *TextureInfomeation_Get(unsigned int i) { return &_TextureInfomeation_Table[i]; }
static inline struct HH_Local_TextureContext *TextureContext_Get(unsigned int id) { return &_TextureContext_Table[id]; }
static inline unsigned int Transport_Current_Priority_Get(void) { return _Transport_Current_Priority; }

static inline unsigned int TextureBuffer_Enable_Get(unsigned int Buffer_Index) {

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 470
    assert(Buffer_Index < TEXTURE_BUFFER_BLOCK_MAX);
    return _texture_buffer_enable[Buffer_Index];
}

static inline void TextureBuffer_Enable_Set(unsigned int Buffer_Index) {

    assert(Buffer_Index < TEXTURE_BUFFER_BLOCK_MAX);
    _texture_buffer_enable[Buffer_Index] = 1;
}

static inline void TextureBuffer_Enable_Clear(unsigned int Buffer_Index) {

    assert(Buffer_Index < TEXTURE_BUFFER_BLOCK_MAX);
    _texture_buffer_enable[Buffer_Index] = 0;
}

/* Finds a free texture buffer (the last one is reserved for the always-resident textures). */
static inline int TextureBuffer_Free_Search(void) {
    int result;
    unsigned int i;

    result = -1;
    for (i = 0; i < TEXTURE_BUFFER_BLOCK_MAX - 1; i++) {
        if (!TextureBuffer_Enable_Get(i)) {
            result = i;
            break;
        }
    }
    return result;
}

/* Is the current room one of the texture's rooms? */
static inline unsigned int Room_Link_Check(struct HH_Local_TextureInfomeation *pTexture_Infomeation) {
    unsigned int check;
    int room_name;
    unsigned int i;

    check = 0;
    room_name = RoomNameJms();
    for (i = 0; i < pTexture_Infomeation->LinkList_Max; i++) {
        if (room_name == pTexture_Infomeation->pLinkList[i]) {
            check = 1;
            break;
        }
    }
    return check;
}

static void LocalWrapper_TextureTransport_Entry(struct sh2gfw_Effect_Man *pEffectTexture_Management, struct sh2gfw_TEX_HEAD *pTexture_Header, struct sh2gfw_CLUTS_HEAD *pCluts_Header, unsigned int Texture_ID) {
    memset(pEffectTexture_Management, 0, sizeof(struct sh2gfw_Effect_Man));
    pEffectTexture_Management->pTexHead = pTexture_Header;
    pEffectTexture_Management->pTexMAN = sh2gfw_set_TexToTrasMan(&AllTexSync_Man, pTexture_Header, pCluts_Header, pEffectTexture_Management, Texture_ID | 0xE000);
    pEffectTexture_Management->valid_id = EFF_VALID_ID;
}

static unsigned int LocalWrapper_TextureTransport_Entry_Delete(struct sh2gfw_Effect_Man *pEffectTexture_Management) {
    unsigned int result;

    result = 0;
    if (sh2gfw_del_TexMAN(&AllTexSync_Man, pEffectTexture_Management->pTexMAN) != NULL) {
        result = 1;
    }
    return result;
}

static unsigned int TextureContext_DesignateEntryLevel_EntryCheck(unsigned int Entry_Level, struct HH_Local_TextureInfomeation *pTexture_Infomeation) {
    unsigned int result;

    result = 0;
    switch (Entry_Level) {
    case 2:
        switch (pTexture_Infomeation->Entry_Level) {
        case 2:
        case 4:
            if (pTexture_Infomeation->pException_Judge != NULL) {
                if (pTexture_Infomeation->pException_Judge()) {
                    result = 1;
                }
            } else {
                result = 1;
            }
            break;
        case 0:
        case 1:
            break;
        }
        break;
    case 3:
        switch (pTexture_Infomeation->Entry_Level) {
        case 4:
            if (pTexture_Infomeation->pException_Judge == NULL || pTexture_Infomeation->pException_Judge()) {
                break;
            }
        case 3:
            if (Room_Link_Check(pTexture_Infomeation)) {
                result = 1;
            }
            break;
        case 0:
        case 1:
            break;
        }
        break;
    case 0:
    case 1:
        break;
    }
    return result;
}

static unsigned int TextureBinary_DesignateEntryLevel_Load(unsigned int Entry_Level) {
    unsigned int result;
    unsigned int i;
    int fid;
    void *pBuffer;
    struct HH_Local_TextureInfomeation *pTex_Info;
    struct HH_Local_TextureContext *pContext;
    int buffer_index;

    result = 0;
    for (i = 0; i < TEXTURE_CONTEXT_MAX; i++) {
        pTex_Info = TextureInfomeation_Get(i);
        if (!TextureContext_DesignateEntryLevel_EntryCheck(Entry_Level, pTex_Info)) {
            continue;
        }
        buffer_index = TextureBuffer_Free_Search();
        if (buffer_index != -1) {
            pContext = TextureContext_Get(pTex_Info->Register_Texture_ID);
            if (!pContext->Enable) {
                pBuffer = HH_MemoryManager_AllocateMemoryBlock_Get(2);
                pBuffer = HH_MemoryManager_DesignateSize_Alignment64Address_Calculator(pBuffer, 0x44800, buffer_index);
                fid = FcRead(pTex_Info->pFileID, pBuffer);
                if (fid != -1) {
                    while (fsSync(0, fid) < 0) {}
                    result = 1;
                    pContext->Enable = 1;
                    pContext->Buffer_Index = buffer_index;
                    pContext->Entry_Level = Entry_Level;
                    pContext->pTexture_Infomeation = pTex_Info;
                    TextureBuffer_Enable_Set(buffer_index);
                    _TextureHeader_Table[pTex_Info->Register_Texture_ID] = pBuffer;
                }
            } else {
                printf("Texture Context Already Regist: Multiple Regist!!\n");

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 770
                assert(0);
            }
        } else {

            assert(0);
        }
    }
    return result;
}

static unsigned int AlwaysTexture_Context_Entry(struct HH_Local_TextureInfomeation *pTex_Info, struct HH_Local_TextureContext *pContext, void *pBuffer) {
    unsigned int result;

    result = 1;
    pContext->Enable = 1;
    pContext->Buffer_Index = TEXTURE_BUFFER_BLOCK_MAX - 1;
    pContext->Entry_Level = 1;
    pContext->pTexture_Infomeation = pTex_Info;
    _texture_buffer_enable[TEXTURE_BUFFER_BLOCK_MAX - 1] = 1;
    _TextureHeader_Table[pTex_Info->Register_Texture_ID] = pBuffer;
    return result;
}

/** Loads an always-resident texture into memory and registers its context. @return 1 if loaded. */
static unsigned int TextureBinary_DesignateTexture_Load_toAlwaysBuffer(struct HH_Local_TextureInfomeation *pTex_Info) {
    unsigned int result;
    void *pBuffer;
    int fid;
    struct HH_Local_TextureContext *pContext;

    result = 0;
    pContext = TextureContext_Get(pTex_Info->Register_Texture_ID);
    if (!pContext->Enable) {
        pBuffer = HH_MemoryManager_AllocateMemoryBlock_Get(3);
        fid = FcRead(pTex_Info->pFileID, pBuffer);
        if (fid != -1) {
            while (fsSync(0, fid) < 0) {}
            AlwaysTexture_Context_Entry(pTex_Info, pContext, pBuffer);
            result = 1;
        }
    } else {
        printf("Texture Context Already Regist: Multiple Regist!!\n");

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 851
        assert_dw(0);
    }
    return result;
}

static unsigned int TextureContext_DesignateEntryLevel_Entry(unsigned int Entry_Level) {
    unsigned int result;
    unsigned int i;
    struct HH_Local_TextureContext *pContext;
    struct sh2gfw_TEX_HEAD *pTex_Header;
    struct sh2gfw_CLUTS_HEAD *pCluts_Header;

    result = 0;
    for (i = 0; i < TEXTURE_CONTEXT_MAX; i++) {
        pContext = TextureContext_Get(i);
        if (pContext->Enable && pContext->Entry_Level == Entry_Level) {
            pTex_Header = (struct sh2gfw_TEX_HEAD *)_TextureHeader_Table[i];
            pCluts_Header = (struct sh2gfw_CLUTS_HEAD *)((char *)pTex_Header + pTex_Header->allsize);
            LocalWrapper_TextureTransport_Entry(&pContext->EffectTexture_Management, pTex_Header, pCluts_Header, pContext->pTexture_Infomeation->Register_Texture_ID);
            result = 1;
        }
    }
    return result;
}

static unsigned int TextureContext_DesignateEntryLevel_AllClear(unsigned int Entry_Level) {
    unsigned int result;
    unsigned int i;
    struct HH_Local_TextureContext *pContext;

    result = 0;
    for (i = 0; i < TEXTURE_CONTEXT_MAX; i++) {
        pContext = TextureContext_Get(i);
        if (pContext->Enable && pContext->Entry_Level == Entry_Level) {
            _TextureHeader_Table[i] = NULL;
            TextureBuffer_Enable_Clear(pContext->Buffer_Index);
            LocalWrapper_TextureTransport_Entry_Delete(&pContext->EffectTexture_Management);
            memset(pContext, 0, sizeof(struct HH_Local_TextureContext));
            result = 1;
        }
    }
    return result;
}

static unsigned int TextureContext_DesignateEntryLevelUnder_AllClear(unsigned int Entry_Level) {
    unsigned int result;
    unsigned int i;

    result = 0;
    for (i = Entry_Level; i <= 3; i++) {
        TextureContext_DesignateEntryLevel_AllClear(i);
        result = 1;
    }
    return result;
}

static unsigned int AlwaysTexture_Initialize(struct HH_Local_TextureInfomeation *pTex_Info) {
    unsigned int result;
    unsigned int Entry_Level;

    Entry_Level = 1;
    TextureContext_DesignateEntryLevel_AllClear(Entry_Level);
    TextureBinary_DesignateTexture_Load_toAlwaysBuffer(pTex_Info);
    TextureContext_DesignateEntryLevel_Entry(Entry_Level);
    result = 1;
    return result;
}

static void Object_SPK_Texture_Post(void) {
    unsigned int i;
    struct HH_Local_TextureContext *pContext;
    struct sh2gfw_Effect_Man *pTex_Manage;

    for (i = 0; i < TEXTURE_CONTEXT_MAX; i++) {
        pContext = TextureContext_Get(i);
        if (pContext->Enable && pContext->pTexture_Infomeation->Transport_Priority == 1) {
            pTex_Manage = &pContext->EffectTexture_Management;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1017
            assert(pTex_Manage->valid_id == EFF_VALID_ID);
            sh2gfw_EnQue_spkTexture(pTex_Manage->pTexMAN, &pTex_Manage->thr_cid, &pTex_Manage->thr_sid);
        }
    }
}

static void Object_Texture_Send(void) {
    unsigned int i;
    unsigned int priority;
    struct HH_Local_TextureContext *pContext;
    struct sh2gfw_Effect_Man *pTex_Manage;

    priority = Transport_Current_Priority_Get();
    for (i = 0; i < TEXTURE_CONTEXT_MAX; i++) {
        pContext = TextureContext_Get(i);
        if (pContext->Enable && pContext->pTexture_Infomeation->Transport_Priority == priority) {
            pTex_Manage = &pContext->EffectTexture_Management;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1047
            assert(pTex_Manage->valid_id == EFF_VALID_ID);
            sh2gfw_Thr_d2TextureSend(pTex_Manage->pTexMAN, 0, &pTex_Manage->thr_cid, &pTex_Manage->thr_sid);
            _send_count++;
        }
    }
}

static void Object_Texture_Sync(void) {
    unsigned int i;
    unsigned int priority;
    struct HH_Local_TextureContext *pContext;
    struct sh2gfw_Effect_Man *pTex_Manage;

    priority = Transport_Current_Priority_Get();
    for (i = 0; i < TEXTURE_CONTEXT_MAX; i++) {
        pContext = TextureContext_Get(i);
        if (pContext->Enable && pContext->pTexture_Infomeation->Transport_Priority == priority) {
            pTex_Manage = &pContext->EffectTexture_Management;
            d1tscSync(pTex_Manage->thr_cid);
            _sync_count++;
        }
    }
}

static void Object_Texture_Finish(void) {
    unsigned int i;
    unsigned int priority;
    struct HH_Local_TextureContext *pContext;
    struct sh2gfw_Effect_Man *pTex_Manage;

    priority = Transport_Current_Priority_Get();
    for (i = 0; i < TEXTURE_CONTEXT_MAX; i++) {
        pContext = TextureContext_Get(i);
        if (pContext->Enable && pContext->pTexture_Infomeation->Transport_Priority == priority) {
            pTex_Manage = &pContext->EffectTexture_Management;
            d1tscFinishToUseSlot(pTex_Manage->thr_sid);
            _finish_count++;
        }
    }
    _Transport_Current_Priority = priority + 1;
}

static unsigned long Object_Texture_GS_Register_Tex0_Get(unsigned int Texture_ID, unsigned int Clut_ID) {
    struct HH_Local_TextureContext *pContext;
    unsigned long tex0;

    pContext = TextureContext_Get(Texture_ID);
    tex0 = 0;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1127
    assert(Clut_ID < 16);
    if (pContext->Enable) {
        tex0 = *(unsigned long *)sh2gfw_Get_RegTEX0(pContext->EffectTexture_Management.pTexMAN, Clut_ID, 1);
    }
    return tex0;
}

/** Starts a frame's texture transfers: resets the transfer priority and the counters. */
void HH_Effect_Object_Texture_TransportPriority_Initialize(void) {
    _Transport_Current_Priority = 0;
    _send_count = 0;
    _sync_count = 0;
    _finish_count = 0;
}

/** Queues the textures of transfer priority 1 for sending through the sh2gfw SPK texture queue. */
void HH_Effect_Object_SPK_Texture_Post(void) {
    Object_SPK_Texture_Post();
}

/** Starts the transfers of the textures of the current priority. */
void HH_Effect_Object_Texture_Send(void) {
    Object_Texture_Send();
}

/** Waits for the transfers of the textures of the current priority. */
void HH_Effect_Object_Texture_Sync(void) {
    Object_Texture_Sync();
}

/**
 * Releases the transfer slots of the current priority's textures and moves on to the next priority.
 */
void HH_Effect_Object_Texture_Finish(void) {
    Object_Texture_Finish();
}

/**
 * Returns the GS TEX0 register value for an effect texture, or 0 if it isn't loaded.
 * @param Texture_ID index into the texture table
 * @param Clut_ID    CLUT number (0-15)
 */
unsigned long HH_Effect_Object_Texture_GS_Register_Tex0_Get(unsigned int Texture_ID, unsigned int Clut_ID) {
    return Object_Texture_GS_Register_Tex0_Get(Texture_ID, Clut_ID);
}

/**
 * Loads and registers the textures of one entry level, after clearing levels Entry_Level to 3. If
 * the texture buffer isn't allocated yet, allocates all the memory blocks and loads levels 2 and 3.
 * @return 1 if the level was loaded into an existing buffer, else 0
 */
unsigned int HH_Effect_Object_Texture_DesignateEntryLevel_Initialize(unsigned int Entry_Level) {
    unsigned int result;

    result = 0;
    if (HH_MemoryManager_AllocateMemoryBlock_Check(2)) {
        TextureContext_DesignateEntryLevelUnder_AllClear(Entry_Level);
        TextureBinary_DesignateEntryLevel_Load(Entry_Level);
        TextureContext_DesignateEntryLevel_Entry(Entry_Level);
        result = 1;
    } else if (HH_MemoryManager_MemoryBlock_All_Allocate()) {
        TextureBinary_DesignateEntryLevel_Load(2);
        TextureContext_DesignateEntryLevel_Entry(2);
        TextureBinary_DesignateEntryLevel_Load(3);
        TextureContext_DesignateEntryLevel_Entry(3);
    }
    return result;
}

/** Unregisters the textures of entry levels Entry_Level to 3. Returns 1. */
unsigned int HH_Effect_Object_Texture_DesignateEntryLevel_Discard(unsigned int Entry_Level) {
    unsigned int result;

    TextureContext_DesignateEntryLevelUnder_AllClear(Entry_Level);
    result = 1;
    return result;
}

/**
 * Loads the first texture with Register_Texture_ID 1 into the always-resident buffer, as entry
 * level 1. Returns 1.
 */
unsigned int HH_Effect_Object_Texture_AlwaysTexture_Initialize(void) {
    unsigned int result;
    unsigned int i;
    struct HH_Local_TextureInfomeation *pTex_Info;

    for (i = 0; i < TEXTURE_CONTEXT_MAX; i++) {
        pTex_Info = TextureInfomeation_Get(i);
        if (pTex_Info->Register_Texture_ID == 1) {
            AlwaysTexture_Initialize(pTex_Info);
            break;
        }
    }
    result = 1;
    return result;
}
