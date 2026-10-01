/*
 * sh2gfw_all_sysinit.c: hardware and graphics-framework initialization.
 */

#include "sh2.h"
#include "sdk/libgraph.h"
#include "sdk/libvu0.h"

static void init_Env_ctl(void);
static void init_GSandVU(void);

/** Per-vblank callback: counts vblanks for the frame rate. Returns `ca`. */
int Vcallback_test(int ca) {
    mct.ui32[0]++;
    mct.ui32[2]++;
    return ca;
}

/** Resets the GS and VU0 and sets up the timers and the GIF/VIF1 registers. */
void init_PS2(void) {
    sceGsResetPath();
    sceGsResetGraph(0, 1, 2, 0);
    sceVpu0Reset();
    *T0_MODE = 0x83;
    *T1_MODE = 0x83;
    mct.ul128 = 0;
    *GS_SIGLBLID = 0;
    sceGsPutIMR(0xFFFFFFFFFFFFFEFF);
    *GS_CSR = 3;
    *VIF1_MARK = 0;
    *VIF1_ERR = 6;
    *GIF_MODE = 4;
}

/**
 * One-time initialization of the graphics framework: clears the background textures and maps, and
 * sets up the GS, VUs and the effect systems.
 */
void step_init_ONE(void) {
    loadBgTEX_Replace(0, 0, 0, 0);
    loadBgTEX_Replace(1, 0, 0, 0);
    loadBgMAP_AllClear();
    init_GSandVU();
    ktVif0Init();
    ktVif1Init();
    sh2_ktVif1kBufInit();
    sh2_ktVif0PkBufInit();
    sh2gfw_AllInit_CharacterOT();
    shDBG_InitFontEnv();
    sh2gfw_init_shcamera();
    sh2shd_init_shadow();
    fontClear();
    sh2gfw_test_MakeNoise();
    HH_MemoryManager_MemoryBlock_All_Allocate();
    HH_Effect_Object_Texture_AlwaysTexture_Initialize();
    fjInitAll();
    shLensFlareInit();
}

/**
 * Per-stage initialization of the graphics framework: environment, fog, effects and the stage's
 * area data.
 */
void step_init_STAGE(void) {
    struct FilesBgBlock *bgfiles;
    unsigned int sh2gfw_process_AreaDATA(); /* Matching: an unprototyped block-scope declaration, as in the original. */

    init_Env_ctl();
    fogInit();
    sh2gfw_init_fogTexture();
    EFCTInit();
    HH_Effect_Object_Texture_DesignateEntryLevel_Initialize(2);
    sh2gfw_process_AreaDATA(stage->glb_crd << 16, &Area_Data_Man);
    bgfiles = FilesGetBgBlock(stage->glb_crd, 0);
    if (bgfiles) {
        loadBgCAM_LoadData(0, bgfiles->cam, stage->glb_crd << 16);
    }
}

/** Clears the room texture and maps and reloads the background data of the current map. */
void map_DATA_LOAD(void) {
    int i;
    int mapid;
    void *trfile;

    loadBgTEX_Replace(1, 0, 0, 0);
    loadBgMAP_AllClear();
    for (i = 0; i < 4; i++) {
        sh2gfw_init_zeroQ_BMAN(&b_man[i]);
    }
    sh2gfw_AllClear_TrMAN();
    clAllInitCollisionData();
    kari_init_colidata();
    sh2gfw_util_zeroq((union Q_WORDDATA *)&sh2_TR_MAN, 0x25);
    sh2gfw_Init_AllVertCounter();
    trfile = loadBg1x1_GetTrTexFile();
    mapid = loadBg1x1_GetIdForDrawEnv();
    Set_DrawEnvData(mapid, 1);
    sh2gfw_LoadSet_SemiTransTEX(mapid, trfile);
}

static void init_Env_ctl(void) {
}

static void init_GSandVU(void) {
    void shGs_InitDefTBuff(); /* Matching: an unprototyped block-scope declaration, as in the original. */
    void shGs_InitGsStencilBuff(); /* (also unprototyped) */
    void shGs_InitGsTinyStencilBuff(); /* (also unprototyped) */

    shGs_InitDefTBuff(&shGs_AllEnv, 0, 1, 0x200, 0x200, 2, 0x3A, 1);
    shGs_InitGsStencilBuff(&shGs_AllEnv, 0x200, 0x200, 2, 0x3A, 1);
    shGs_InitGsTinyStencilBuff(&shGs_AllEnv, 0x100, 0x100, 0x3A);
    shGs_InitDefaultRegsEnv(&shGs_AllEnv);
    sh2gfw_init_VU_PARAMS(&VU1_PARMS);
    sh2gfw_util_zeroq((union Q_WORDDATA *)&AllTexSync_Man, 0x1F05);
    sh2gfw_allinit_TexMANlist(&AllTexSync_Man);
    sh2gfw_allinit_lightenv(&LightEnv);
    sh2gfw_ALLInit_SpotMan();
}
