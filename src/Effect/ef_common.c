/*
 * ef_common.c: the EFCT effect system shared by the ef_* effects: the effect task list and its
 * object pool, animation frames, sprite vertices, 3D transform and clipping, and the GS packets
 * that draw them.
 *
 * Matching: each `#line` puts the assert after it on its line in the original file (the asserts
 * bake "<file>:<line>" into their strings).
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "fi_libvu0_inline.h"
#include "fog_param.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

float passing_time;
unsigned char EFCTTaskBuf[131072];
u_long128 efctPacket[4096];
u_long128 efctheap[32768];
struct EFCTObject EFCTLocalDataBuffer[32];
struct EFCTTexEnvInfo TexImage[11];

static void EFCTInitEffectTask(void);
static void SetGunFire(float *Pos, float *vec, int wep_kind, unsigned char light);
static void SetGunSmoke(float *Pos, int wep_kind, unsigned char light);
static unsigned short GetEffectLayerNum(short EffectKind);
static void InitEffectTexEnv(int EffectKind);
static struct EFCTTask *EFCTEntryEffectTask(short Kind);
static int EFCTAutoNextFrame(struct EFCTObject *pObj);
static void EFCTMakePacket(struct EFCTObject *pObj, sceVif1Packet *pck);
static void SetVertexPkData(sceVif1Packet *pck, struct EFCTVertexData *pVertex, unsigned int nVertexNum);
static void EFCTGetGiftag(int Kind, unsigned long *giftag);
static void EFCTSetAlphaEnvironment(sceVif1Packet *pck, short kind);
static int EFCTDeleteOldBloodDropTask(void);
static int EFCTDeleteOldTask(short kind);
static void EFCTDoCtrlDummy(void *ptr);

static void (*EFCTControlFunc[9])(void *) = {
    EFCTDoCtrlDummy,
    DrawBloodSpray,
    DrawBloodDrop,
    DrawBloodPool,
    DrawGunFire,
    DrawGunSmoke,
    DrawBrokenGlass,
    DrawFlame,
    DrawSmoke,
};

/** Initializes the effect system: task list, effect heap, effect packet buffer and object pool. */
void EFCTInit(void) {
    shTSKInitTaskList(EFCTTaskBuf, sizeof(EFCTTaskBuf));
    if (efctheap != NULL) {
        EfctInitHeap(efctheap, sizeof(efctheap));
    }
    if (efctPacket != NULL) {
        shEfctPkInit(efctPacket);
    }
    EFCTInitEffectTask();
}

static void EFCTInitEffectTask(void) {
    int i;

    for (i = 0; i < 32; i++) {
        EFCTLocalDataBuffer[i].Using = 0;
        if (EFCTLocalDataBuffer[i].pAnimData) {
            EfctFree(EFCTLocalDataBuffer[i].pAnimData);
            EFCTLocalDataBuffer[i].pAnimData = NULL;
        }
        if (EFCTLocalDataBuffer[i].pVertex) {
            EfctFree(EFCTLocalDataBuffer[i].pVertex);
            EFCTLocalDataBuffer[i].pVertex = NULL;
        }
    }
    shTSKFreeTaskLine(4);
}

/** Runs one frame of every effect task (resets the packet buffer and sets the frame time first). */
void EFCTDoTask(void) {
    shEfctPkReset();
    EFCTSetPassingTimePerFrame(shGetDT());
    shTSKExecuteTask(4);
}

/** Sends this frame's effect packet over DMA channel 1. */
void EFCTKickPacket(void) {
    void *addr;

    addr = shEfctPkGetKickAddrByd1cSend();
    d1cSend(addr);
}

/**
 * Starts James's muzzle flash and gun smoke, for his current weapon and light state.
 * @param Pos muzzle position
 * @param vec firing direction
 */
void EFCTSetGunFire(float *Pos, float *vec) {
    int weapon_kind;
    unsigned char light;

    weapon_kind = PlayerGetJamesWeapon();
    light = item.light_switch;
    SetGunFire(Pos, vec, weapon_kind, light);
    SetGunSmoke(Pos, weapon_kind, light);
}

/**
 * Starts Eddie's muzzle flash and gun smoke (weapon kind 1, light on).
 * @param Pos muzzle position
 * @param vec firing direction
 */
void EFCTSetGunFireEddie(float *Pos, float *vec) {
    SetGunFire(Pos, vec, 1, 1);
    SetGunSmoke(Pos, 1, 1);
}

static void SetGunFire(float *Pos, float *vec, int wep_kind, unsigned char light) {
    struct EFCTTask *pTask;
    unsigned short LayerNum;
    int i;

    LayerNum = GetEffectLayerNum(4);
    for (i = 0; i < LayerNum; i++) {
        pTask = EFCTEntryEffectTask(4);
        if (pTask == NULL) {
            if (EFCTDeleteOldBloodDropTask()) {
                continue;
            }
            return;
        }
        if (!InitEffectObjectGunFire(pTask->pObj, i, Pos, vec, wep_kind, light)) {
            EFCTCutEffectTask(pTask);
            return;
        }
        pTask->pObj->LayerNum = LayerNum;
    }
    InitEffectTexEnv(4);
}

/** Does nothing (empty in the original; the DWARF drops the unused Pos). */
void EFCTSetGunSmoke(float *Pos) {
}

static void SetGunSmoke(float *Pos, int wep_kind, unsigned char light) {
    struct EFCTTask *pTask;
    int i;
    unsigned short LayerNum;

    LayerNum = GetEffectLayerNum(5);
    for (i = 0; i < LayerNum; i++) {
        pTask = EFCTEntryEffectTask(5);
        if (pTask == NULL) {
            if (EFCTDeleteOldBloodDropTask()) {
                continue;
            }
            return;
        }
        if (!InitEffectObjectGunSmoke(pTask->pObj, i, Pos, wep_kind, light)) {
            EFCTCutEffectTask(pTask);
            return;
        }
        pTask->pObj->LayerNum = LayerNum;
    }
    InitEffectTexEnv(5);
}

/**
 * Starts the broken glass and smoke of the TV in the Angela/father drama demo, at fixed positions.
 */
void EFCTSetDramaDemoAngelaPapa(void) {
    float tv_pos[4] = { 19799.402f, -290.5385f, 61208.95f, 1.0f };
    float tv_dir[4] = { 0.491818f, -0.790657f, -0.36466f, 0.0f };
    float chara_pos[2][4] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };

    EFCTSetBrokenGlass(tv_pos, tv_dir, chara_pos);
    tv_pos[1] += 50.0f;
    EFCTSetSmoke(tv_pos, 2);
}

/**
 * Starts the broken-glass effect.
 * @param parent_pos       origin of the effect
 * @param parent_direction direction the shards fly off in
 * @param chara_pos        two positions passed on to InitEffectObjectBrokenGlass
 */
void EFCTSetBrokenGlass(float *parent_pos, float *parent_direction, float (*chara_pos)[4]) {
    struct EFCTTask *pTask;
    int i;
    unsigned short LayerNum;

    LayerNum = GetEffectLayerNum(6);
    for (i = 0; i < LayerNum; i++) {
        pTask = EFCTEntryEffectTask(6);
        if (pTask == NULL) {
            return;
        }
        if (!InitEffectObjectBrokenGlass(pTask->pObj, i, parent_pos, parent_direction, chara_pos)) {
            EFCTCutEffectTask(pTask);
            return;
        }
        pTask->pObj->LayerNum = LayerNum;
    }
    InitEffectTexEnv(6);
}

/**
 * Starts a smoke effect.
 * @param pos  origin of the smoke
 * @param kind smoke kind, passed on to InitEffectObjectSmoke
 */
void EFCTSetSmoke(float *pos, unsigned char kind) {
    struct EFCTTask *pTask;
    int i;
    unsigned short LayerNum;

    LayerNum = 1;
    for (i = 0; i < LayerNum; i++) {
        pTask = EFCTEntryEffectTask(8);
        if (pTask == NULL) {
            return;
        }
        if (!InitEffectObjectSmoke(pTask->pObj, i, pos, kind)) {
            EFCTCutEffectTask(pTask);
            return;
        }
        pTask->pObj->LayerNum = LayerNum;
    }
    InitEffectTexEnv(8);
}

static unsigned short GetEffectLayerNum(short EffectKind) {
    short Ret;

    Ret = 1;
    switch (EffectKind) {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        Ret = GetStageEffectLayerNum(EffectKind);
        break;
    case 6:
        Ret = GetBrokenGlassEffectLayerNum();
        break;
    case 7:
        Ret = GetFlameEffectLayerNum();
        break;
    }
    return Ret;
}

static void InitEffectTexEnv(int EffectKind) {
    switch (EffectKind) {
    case 1:
    case 2:
    case 3:
        InitBloodTexEnv(&TexImage[EffectKind]);
        break;
    case 4:
        InitGunFireTexEnv(&TexImage[EffectKind]);
        break;
    case 5:
        InitGunSmokeTexEnv(&TexImage[EffectKind]);
        break;
    case 6:
        InitBrokenGlassTexEnv(&TexImage[EffectKind]);
        break;
    case 7:
        InitFlameTexEnv(&TexImage[EffectKind]);
        break;
    case 8:
        InitSmokeTexEnv(&TexImage[EffectKind]);
        break;
    default:
#line 1000
        assert(0);
    }
}

static struct EFCTTask *EFCTEntryEffectTask(short Kind) {
    struct EFCTObject *pObject;
    struct _shTskTASK *pTask;
    int i;

    i = 0;
    pObject = EFCTLocalDataBuffer;
    while (pObject->Using == 1) {
        if (i >= 32) {
            return NULL;
        }
        i++;
        pObject++;
    }
    pTask = shTSKSetTask(EFCTControlFunc[Kind], 4);
    if (pTask == NULL) {
        return NULL;
    }
    pObject->EffectKind = Kind;
    pObject->Using = 1;
    ((struct EFCTTask *)pTask)->pObj = pObject;
    pTask->exe.atr = Kind;
    return (struct EFCTTask *)pTask;
}

/**
 * Ends an effect task: frees its object's vertices and animation data and deletes the task.
 * @param ptr the task
 */
void EFCTCutEffectTask(struct EFCTTask *ptr) {
    if (ptr->pObj->Using == 1) {
        ptr->pObj->Using = 0;
        if (ptr->pObj->pVertex) {
            EfctFree(ptr->pObj->pVertex);
            ptr->pObj->pVertex = NULL;
        }
        if (ptr->pObj->pAnimData) {
            EfctFree(ptr->pObj->pAnimData);
            ptr->pObj->pAnimData = NULL;
        }
        shTSKDelTask((struct _shTskTASK *)ptr);
    }
}

/**
 * Allocates and initializes an effect's animation data (replacing any it had).
 * @param TotalFrame    number of frames
 * @param DrawFrameWait time each frame is shown
 * @param StartFrame    first frame; the animation ends on the frame before it
 * @param pAnim         receives the allocated data
 * @return 1 on success, 0 if the allocation failed
 */
int InitEffectAnimData(unsigned short TotalFrame, float DrawFrameWait, short StartFrame, struct EFCTAnimationData **pAnim) {
    struct EFCTAnimationData AnimData;

    if (*pAnim) {
        EfctFree(*pAnim);
        *pAnim = NULL;
    }
    *pAnim = EfctMalloc(sizeof(struct EFCTAnimationData));
    if (*pAnim == NULL) {
        return 0;
    }
#line 1073
    assert(StartFrame < TotalFrame);
    AnimData.Status = 0;
    AnimData.StartFrameNo = StartFrame;
    if (StartFrame <= 0) {
        AnimData.FinishFrameNo = TotalFrame - 1;
    } else {
        AnimData.FinishFrameNo = StartFrame - 1;
    }
    AnimData.TotalFrame = TotalFrame;
    AnimData.DrawFrameWait = DrawFrameWait;
    AnimData.CurrentFrameNo = AnimData.StartFrameNo;
    AnimData.DrawingTime = 0.0f;
    AnimData.SetAnimParam = NULL;
    **pAnim = AnimData;
    return 1;
}

/**
 * Allocates a 4-vertex sprite (replacing any vertices there were) and sets its corners,
 * rotation and colour.
 * @param pos       unused
 * @param width     sprite width
 * @param height    sprite height
 * @param PlaneKind 0 for an XY-plane sprite (any other value asserts)
 * @param pVertex   receives the allocated vertices
 * @param rot       rotation (x, y, z) applied to the corners
 * @param rgba      colour of every vertex
 * @return the number of vertices (4), or 0 if the allocation failed
 */
unsigned int InitEffectVertexSprite(float *pos, float width, float height, unsigned short PlaneKind, struct EFCTVertexData **pVertex, float *rot, unsigned char *rgba) {
    int i;

    if (*pVertex) {
        EfctFree(*pVertex);
        *pVertex = NULL;
    }
    *pVertex = EfctMalloc(sizeof(struct EFCTVertexData) * 4);
    if (*pVertex == NULL) {
        return 0;
    }
    if (PlaneKind == 0) {
        SetEffectVertexSpriteXY(width, height, *pVertex);
    /* Matching: the original tests 0 twice; the XZ case is unreachable. */
    } else if (PlaneKind == 0) {
        SetEffectVertexSpriteXZ(width, height, *pVertex);
    } else {
#line 1118
        assert_dw(0);
    }
    SpriteLocalRot(*pVertex, 4, rot);
    for (i = 0; i < 4; i++) {
        (*pVertex)[i].rgba[0] = rgba[0];
        (*pVertex)[i].rgba[1] = rgba[1];
        (*pVertex)[i].rgba[2] = rgba[2];
        (*pVertex)[i].rgba[3] = rgba[3];
        (*pVertex)[i].is_valid = 1;
    }
    return 4;
}

/**
 * Sets the local positions of a sprite's 4 vertices as a width x height rectangle in the XY
 * plane, centred on the origin.
 */
void SetEffectVertexSpriteXY(float width, float height, struct EFCTVertexData *VertexData) {
    VertexData[0].LocalPos[0] = -width / 2.0f;
    VertexData[0].LocalPos[1] = -height / 2.0f;
    VertexData[0].LocalPos[2] = 0.0f;
    VertexData[0].LocalPos[3] = 1.0f;
    VertexData[1].LocalPos[0] = width / 2.0f;
    VertexData[1].LocalPos[1] = -height / 2.0f;
    VertexData[1].LocalPos[2] = 0.0f;
    VertexData[1].LocalPos[3] = 1.0f;
    VertexData[2].LocalPos[0] = -width / 2.0f;
    VertexData[2].LocalPos[1] = height / 2.0f;
    VertexData[2].LocalPos[2] = 0.0f;
    VertexData[2].LocalPos[3] = 1.0f;
    VertexData[3].LocalPos[0] = width / 2.0f;
    VertexData[3].LocalPos[1] = height / 2.0f;
    VertexData[3].LocalPos[2] = 0.0f;
    VertexData[3].LocalPos[3] = 1.0f;
}

/**
 * Sets the local positions of a sprite's 4 vertices as a width x height rectangle in the XZ
 * plane, centred on the origin.
 */
void SetEffectVertexSpriteXZ(float width, float height, struct EFCTVertexData *VertexData) {
    VertexData[0].LocalPos[0] = -width / 2.0f;
    VertexData[0].LocalPos[1] = 0.0f;
    VertexData[0].LocalPos[2] = -height / 2.0f;
    VertexData[0].LocalPos[3] = 1.0f;
    VertexData[1].LocalPos[0] = width / 2.0f;
    VertexData[1].LocalPos[1] = 0.0f;
    VertexData[1].LocalPos[2] = -height / 2.0f;
    VertexData[1].LocalPos[3] = 1.0f;
    VertexData[2].LocalPos[0] = -width / 2.0f;
    VertexData[2].LocalPos[1] = 0.0f;
    VertexData[2].LocalPos[2] = height / 2.0f;
    VertexData[2].LocalPos[3] = 1.0f;
    VertexData[3].LocalPos[0] = width / 2.0f;
    VertexData[3].LocalPos[1] = 0.0f;
    VertexData[3].LocalPos[2] = height / 2.0f;
    VertexData[3].LocalPos[3] = 1.0f;
}

/**
 * Rotates the local positions of num vertices by rot (Z, then Y, then X).
 * @param pVertex vertices
 * @param num     number of vertices
 * @param rot     rotation angles (x, y, z)
 */
void SpriteLocalRot(struct EFCTVertexData *pVertex, int num, float *rot) {
    float mtx[4][4];
    int i;

    _sceVu0UnitMatrix(mtx);
    shRotMatrixZ(mtx, mtx, rot[2]);
    shRotMatrixY(mtx, mtx, rot[1]);
    shRotMatrixX(mtx, mtx, rot[0]);
    for (i = 0; i < num; i++) {
        _sceVu0ApplyMatrix(pVertex[i].LocalPos, mtx, pVertex[i].LocalPos);
    }
}

/**
 * Advances an effect's animation by this frame's time.
 * @param pObj   the effect object
 * @param DoLoop non-zero to loop, 0 to stop on the last frame
 * @return 1 if the frame changed (or the animation started), else 0
 */
int EFCTNextFrame(struct EFCTObject *pObj, int DoLoop) {
    int nRet;

    pObj->pAnimData->DoLoop = DoLoop;
    nRet = EFCTAutoNextFrame(pObj);
    return nRet;
}

static int EFCTAutoNextFrame(struct EFCTObject *pObj) {
    int nRet;
    struct EFCTAnimationData *pAnim;

    nRet = 0;
    pAnim = pObj->pAnimData;
    if (pAnim->Status == 0) {
        pAnim->CurrentFrameNo = pAnim->StartFrameNo;
        pAnim->DrawingTime = 0.0f;
        pAnim->Status = 1;
        if (pAnim->SetAnimParam) {
            pAnim->SetAnimParam(pObj);
        }
        nRet = 1;
    } else if (pAnim->Status == 1) {
        pAnim->DrawingTime += EFCTGetPassingTimePerFrame();
        if (pAnim->DrawingTime < pAnim->DrawFrameWait) {
            nRet = 0;
        } else {
            pAnim->DrawingTime = 0.0f;
            if (pAnim->CurrentFrameNo == pAnim->FinishFrameNo) {
                if (pAnim->DoLoop == 0) {
                    pAnim->Status = 2;
                } else {
                    pAnim->CurrentFrameNo = pAnim->StartFrameNo;
                }
            } else if (pAnim->StartFrameNo > 0) {
                if (pAnim->CurrentFrameNo < pAnim->TotalFrame - 1) {
                    pAnim->CurrentFrameNo = pAnim->CurrentFrameNo + 1;
                } else {
                    pAnim->CurrentFrameNo = 0;
                }
            } else if (pAnim->StartFrameNo == 0) {
                pAnim->CurrentFrameNo = pAnim->CurrentFrameNo + 1;
            } else {
#line 1324
                assert(0);
            }
            nRet = 1;
        }
    }
    return nRet;
}

/* Matching: a fitted stand-in for software-double code (docs/stand-ins.md); it moves later functions' temporaries
 * from a0/a1 to a2 (docs/decomp-workflow.md). */
STRIPPED_DOUBLE_CODE()

/**
 * Transforms an object's vertices to screen space: object position plus translation, then the
 * camera's world-screen matrix. Stores the 12.4 fixed-point screen XY, Z, W and Q.
 */
void EFCTThreeDWork(struct EFCTObject *pObj) {
    float lw_mtx[4][4];
    float ls_mtx[4][4];
    float ws_mtx[4][4];
    int i;
    float vec[4];

    _sceVu0UnitMatrix(lw_mtx);
    _sceVu0TransMatrix(lw_mtx, lw_mtx, pObj->Pos);
    _sceVu0TransMatrix(lw_mtx, lw_mtx, pObj->trans);
    sceVu0CopyMatrix(ws_mtx, cam0.world_screen);
    shMulMatrix(ls_mtx, ws_mtx, lw_mtx);
    for (i = 0; i < pObj->VertexNum; i++) {
        pObj->pVertex[i].stq[2] = _shRotTransPersQ(vec, ls_mtx, pObj->pVertex[i].LocalPos);
        pObj->pVertex[i].ScreenPos[0] = ftoi4(vec[0]);
        pObj->pVertex[i].ScreenPos[1] = ftoi4(vec[1]);
        pObj->pVertex[i].ScreenPos[2] = (unsigned int)vec[2];
        pObj->pVertex[i].ScreenPos[3] = (unsigned int)vec[3];
    }
}

/**
 * EFCTThreeDWork for camera-facing objects: the local matrix is the inverse of the view
 * rotation, so the vertices face the camera.
 */
void EFCTTinyThreeDWork(struct EFCTObject *pObj) {
    float lw_mtx[4][4];
    float ls_mtx[4][4];
    float wv_mtx[4][4];
    float ws_mtx[4][4];
    int i;
    float vec[4];

    sceVu0CopyMatrix(wv_mtx, VbWvsMatrix.wvm);
    wv_mtx[3][0] = wv_mtx[3][1] = wv_mtx[3][2] = 0.0f;
    wv_mtx[3][3] = 1.0f;
    sceVu0InversMatrix(lw_mtx, wv_mtx);
    _sceVu0TransMatrix(lw_mtx, lw_mtx, pObj->Pos);
    _sceVu0TransMatrix(lw_mtx, lw_mtx, pObj->trans);
    sceVu0CopyMatrix(ws_mtx, cam0.world_screen);
    shMulMatrix(ls_mtx, ws_mtx, lw_mtx);
    for (i = 0; i < pObj->VertexNum; i++) {
        pObj->pVertex[i].stq[2] = _shRotTransPersQ(vec, ls_mtx, pObj->pVertex[i].LocalPos);
        pObj->pVertex[i].ScreenPos[0] = ftoi4(vec[0]);
        pObj->pVertex[i].ScreenPos[1] = ftoi4(vec[1]);
        pObj->pVertex[i].ScreenPos[2] = (unsigned int)vec[2];
        pObj->pVertex[i].ScreenPos[3] = (unsigned int)vec[3];
    }
}

/**
 * Adds an object's GS packet (alpha/texture setup, then its vertices) to the effect packet buffer.
 */
void DrawPrimitive(struct EFCTObject *pObj) {
    sceVif1Packet *vif1packet;

    vif1packet = shEfctPkTaskHead();
    EFCTSetAlphaEnvironment(vif1packet, pObj->EffectKind);
    EFCTMakePacket(pObj, vif1packet);
    shEfctPkTaskTail();
}

static void EFCTMakePacket(struct EFCTObject *pObj, sceVif1Packet *pck) {
    unsigned long giftag0[2] = { 0x1400000000000000, 0 };
    unsigned long giftag1[2];

    EFCTGetGiftag(pObj->EffectKind, giftag1);
    sceVif1PkOpenGifTag(pck, *(u_long128 *)giftag0);
    sceVif1PkAddGsData(pck, 0x7C);
    sceVif1PkCloseGifTag(pck);
    sceVif1PkOpenGifTag(pck, *(u_long128 *)giftag1);
    SetVertexPkData(pck, pObj->pVertex, pObj->VertexNum);
    sceVif1PkCloseGifTag(pck);
    {
        unsigned long giftag0[2] = { 0x1000000000008000, 0xE };

        sceVif1PkOpenGifTag(pck, *(u_long128 *)giftag0);
        sceVif1PkAddGsAD(pck, 0x4E, 0x3A0001C0);
        sceVif1PkCloseGifTag(pck);
    }
}

static void SetVertexPkData(sceVif1Packet *pck, struct EFCTVertexData *pVertex, unsigned int nVertexNum) {
    int i;
    float s;
    float t;
    float f;

    for (i = 0; i < nVertexNum; i++) {
        if (pVertex[i].is_valid == 1) {
            s = pVertex[i].stq[0] * pVertex[i].stq[2];
            t = pVertex[i].stq[1] * pVertex[i].stq[2];
            sceVif1PkAddGsData(pck, (unsigned long)*(unsigned int *)&s | ((unsigned long)*(unsigned int *)&t << 32));
            sceVif1PkAddGsData(pck, (unsigned long)pVertex[i].rgba[0] | ((unsigned long)pVertex[i].rgba[1] << 8) | ((unsigned long)pVertex[i].rgba[2] << 16) | ((unsigned long)pVertex[i].rgba[3] << 24) | ((unsigned long)*(unsigned int *)&pVertex[i].stq[2] << 32));
            f = FogParamB(Env_ctl.fogparm.fl32) + pVertex[i].stq[2] * FogParamA(Env_ctl.fogparm.fl32);
            sceVif1PkAddGsData(pck, (unsigned long)pVertex[i].ScreenPos[0] | ((unsigned long)pVertex[i].ScreenPos[1] << 16) | ((unsigned long)pVertex[i].ScreenPos[2] << 32) | ((unsigned long)iclamp((int)f, 0, 0xFF) << 56));
        }
    }
}

static void EFCTGetGiftag(int Kind, unsigned long *giftag) {
    switch (Kind) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        giftag[0] = 0xC400000000008000;
        giftag[1] = 0x412412C12C12;
        break;
    default:
#line 1644
        assert(0);
    }
}

static void EFCTSetAlphaEnvironment(sceVif1Packet *pck, short kind) {
    unsigned long giftag0[2] = { 0x1000000000008000, 0xE };
    unsigned long tex0;
    unsigned int tex_id;

    switch (kind) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 8:
        tex_id = 0;
        break;
    case 6:
        tex_id = 0x11;
        break;
    }
    tex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(tex_id, 0);
    sceVif1PkOpenGifTag(pck, *(u_long128 *)giftag0);
    sceVif1PkAddGsAD(pck, 0x6, tex0);
    sceVif1PkAddGsAD(pck, 0x47, 0x50003);
    switch (TexImage[kind].transparency) {
    case 0:
        sceVif1PkAddGsAD(pck, 0x42, 0x80000000A1);
        break;
    case 1:
        sceVif1PkAddGsAD(pck, 0x42, 0x48);
        break;
    case 2:
        sceVif1PkAddGsAD(pck, 0x42, 0x44);
        break;
    default:
#line 1697
        assert_dw(0);
    }
    sceVif1PkAddGsAD(pck, 0x4E, 0x13A0001C0);
    sceVif1PkCloseGifTag(pck);
}

/**
 * Ends every effect task of one kind.
 * @param Kind effect kind (the task attribute)
 */
void EFCTDeleteTask(short Kind) {
    struct _shTskTASK *pTask;

    pTask = shTskTaskListTop[4]->exe.next;
    pTask = shTSKSearchTaskWithAtr(Kind, pTask, 4);
    while (pTask != NULL) {
        EFCTCutEffectTask((struct EFCTTask *)pTask);
        pTask = shTSKSearchTaskWithAtr(Kind, pTask->exe.next, 4);
    }
}

static int EFCTDeleteOldBloodDropTask(void) {
    int i;
    int ret;
    unsigned short LayerNum;

    ret = 1;
    LayerNum = GetEffectLayerNum(2);
    for (i = 0; i < LayerNum; i++) {
        ret |= EFCTDeleteOldTask(2);
    }
    return ret;
}

static int EFCTDeleteOldTask(short kind) {
    int ret;
    struct _shTskTASK *pTask;

    pTask = shTskTaskListTop[4]->exe.next;
    pTask = shTSKSearchTaskWithAtr(kind, pTask, 4);
    if (pTask) {
        EFCTCutEffectTask((struct EFCTTask *)pTask);
        ret = 1;
    } else {
        ret = 0;
    }
    return ret;
}

static void EFCTDoCtrlDummy(void *ptr) {
    EFCTCutEffectTask(ptr);
}

/**
 * Returns the rotation of plane index of a sprite made of plane_num planes spread over
 * around_rot, plus a random offset of up to +-rand_rot_range/2 degrees.
 */
float GetSpriteRotAngle(float around_rot, unsigned int plane_num, unsigned int index, int rand_rot_range) {
    float BaseAng;

    BaseAng = index * (around_rot / plane_num);
    return BaseAng + 0.017453292f * (shRandI() % (rand_rot_range + 1) - rand_rot_range / 2);
}

/** Makes an object's vertices visible if any vertex is on screen, else hides them all. */
void ClipEffectObject(struct EFCTObject *pObj) {
    int i;
    int valid;

    valid = 0;
    for (i = 0; i < pObj->VertexNum; i++) {
        if (!EFCTClipVertex(pObj->pVertex[i].ScreenPos)) {
            valid = 1;
            break;
        }
    }
    for (i = 0; i < pObj->VertexNum; i++) {
        pObj->pVertex[i].is_valid = valid;
    }
}

/** Makes an object's vertices visible only if no vertex is off screen. */
void ClipEffectObject2(struct EFCTObject *pObj) {
    int i;
    int valid;

    valid = 1;
    for (i = 0; i < pObj->VertexNum; i++) {
        if (EFCTClipVertex(pObj->pVertex[i].ScreenPos) == 1) {
            valid = 0;
            break;
        }
    }
    for (i = 0; i < pObj->VertexNum; i++) {
        pObj->pVertex[i].is_valid = valid;
    }
}

/* Matching: inline getters (names ours); reading Env_ctl in place, EFCTClipVertex doesn't match. */
static inline float EFCTNearZ(void) {
    return Env_ctl.camera_parms[3];
}

static inline float EFCTFarZ(void) {
    return Env_ctl.camera_parms2[2];
}

/**
 * Tests a screen-space vertex against the screen rectangle and the camera's near and far Z.
 * @param vec 12.4 fixed-point screen XY, Z, W
 * @return 1 if the vertex is outside, 0 if inside
 */
int EFCTClipVertex(int *vec) {
    int ret;
    int vx;
    int vy;
    int vw;

    ret = 0;
    vx = vec[0] >> 4;
    vy = vec[1] >> 4;
    vw = vec[3];
    if (vx < 2048.0f - VbScreenInfo.sx / 2.0f || vx > 2048.0f + VbScreenInfo.sx / 2.0f ||
        vy < 2048.0f - VbScreenInfo.sy / 2.0f || vy > 2048.0f + VbScreenInfo.sy / 2.0f ||
        vw < EFCTNearZ() || vw > EFCTFarZ()) {
        ret = 1;
    }
    return ret;
}

/**
 * Sets the colour of 4 vertices.
 * @param rgba    colour (0-255 per component)
 * @param pVertex vertices
 */
void EFCTResetRGBA(int *rgba, struct EFCTVertexData *pVertex) {
    int i;

    for (i = 0; i < 4; i++) {
        pVertex[i].rgba[0] = rgba[0];
        pVertex[i].rgba[1] = rgba[1];
        pVertex[i].rgba[2] = rgba[2];
        pVertex[i].rgba[3] = rgba[3];
    }
}

/** Sets the frame time the effects advance by. */
void EFCTSetPassingTimePerFrame(float time) {
    passing_time = time;
}

/** Returns the frame time the effects advance by. */
float EFCTGetPassingTimePerFrame(void) {
    return passing_time;
}

/**
 * Adds z to the screen Z of vertex_num vertices.
 * @param vertex_num number of vertices
 * @param z          Z offset
 * @param pVertex    vertices
 */
void CalibrationZValue(unsigned int vertex_num, int z, struct EFCTVertexData *pVertex) {
    int i;

    for (i = 0; i < vertex_num; i++) {
        pVertex[i].ScreenPos[2] += z;
    }
}
