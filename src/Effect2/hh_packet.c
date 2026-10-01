/*
 * hh_packet.c: the Effect2 VIF1 packet buffer.
 *
 * Effects write GS primitives into one VIF1 packet (DIRECT code) that lives in memory block 0.
 * The start addresses of the packets to kick are kept in a small queue and sent with d1cSend().
 *
 * Matching: asserts and the overflow message bake line numbers into their strings, so blank
 * lines keep them on their original lines.
 */
#include "sh2.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"



/* GIF tags (A+D, packed) */
static unsigned long _GifTag_First[2] = { 0x1000400000000000, 0xE };
static unsigned long _GifTag_Final[2] = { 0x1000400000008000, 0xE };
static unsigned long _GifTag_GS_AD[2] = { 0x1000000000000000, 0xE };
static unsigned long _GifTag_Sprite[2] = { 0x502F400000000000, 0x42421 };
static unsigned long _GifTag_TriangleStrip[2] = { 0x302E400000000000, 0x412 };
static unsigned long _GifTag_TriangleFan[2] = { 0x302EC00000000000, 0x412 };

static sceVif1Packet _Vif1_Packet;
static struct Queue_Object _queue;
static struct Queue_Object *pQueue = &_queue;

static void QueueObject_Initialize(struct Queue_Object *pQueue) {
    pQueue->enQueue = 0;
    pQueue->deQueue = 0;
    pQueue->Length_Current = 0;
    pQueue->Length_Max = 8;
}

static unsigned int QueueObject_enQueue(struct Queue_Object *pQueue, struct Queue_Element *pElement) {
    unsigned int result;

    result = 0;
    if (pQueue->Length_Current < pQueue->Length_Max) {
        pQueue->Element[pQueue->enQueue++] = *pElement;
        pQueue->enQueue %= pQueue->Length_Max;
        pQueue->Length_Current++;
        result = 1;
    }
    return result;
}

static unsigned int QueueObject_deQueue(struct Queue_Object *pQueue, struct Queue_Element *pElement) {
    unsigned int result;

    result = 0;
    if (pQueue->Length_Current != 0) {
        *pElement = pQueue->Element[pQueue->deQueue++];
        pQueue->deQueue %= pQueue->Length_Max;
        pQueue->Length_Current--;
        result = 1;
    }
    return result;
}

/** Returns 1 if the packet buffer (memory block 0) is allocated, else 0. */
unsigned int HH_Vif1PacketBuffer_Memory_Allocate_Check(void) {
    unsigned int result;

    result = 0;
    if (HH_MemoryManager_AllocateMemoryBlock_Check(0)) {
        result = 1;
    }
    return result;
}

/** Returns the Effect2 VIF1 packet. */
sceVif1Packet *HH_Vif1Packet_Current_Get(void) {
    return &_Vif1_Packet;
}

/**
 * Starts a frame's packet at the start of memory block 0: queues it for sending and opens a DMA cnt
 * tag and a DIRECT code.
 */
void HH_Vif1PacketBuffer_Initialize(void) {
    u_long128 *pPacketBuffer;
    sceVif1Packet *pPacket;

    pPacketBuffer = HH_MemoryManager_AllocateMemoryBlock_Get(0);
    pPacket = &_Vif1_Packet;
    sceVif1PkInit(&_Vif1_Packet, pPacketBuffer);
    QueueObject_Initialize(pQueue);
    {
        struct Queue_Element element = { 0 };

        element.pAddress = _Vif1_Packet.pBase;
        if (!QueueObject_enQueue(pQueue, &element)) {
            printf("----------Send Address Queue Over\n");

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 397
            assert_dw(0);
        }
    }
    sceVif1PkCnt(pPacket, 0);
    sceVif1PkOpenDirectCode(pPacket, 0);
}

/**
 * Opens the frame's first GIF tag and sets the GS state every class starts from (FRAME with alpha
 * masked, ZBUF, TEST, CLAMP, TEX1).
 */
void HH_Vif1PacketBuffer_Prefix_GifTag_Open(void) {
    sceVif1Packet *pPacket;

    pPacket = &_Vif1_Packet;
    HH_Vif1PacketBuffer_GifTag_Open();
    sceVif1PkOpenGifTag(pPacket, *(u_long128 *)_GifTag_First);
    *(u_long128 *)pPacket->pCurrent = *HH_ClassWrapper_GS_EnvironmentRegister_Frame_AlphaMask_Get();
    pPacket->pCurrent += 4;
    sceVif1PkAddGsAD(pPacket, GS_REG_ZBUF_1, 0x10A0001C0);
    sceVif1PkAddGsAD(pPacket, GS_REG_TEST_1, 0x50002);
    sceVif1PkAddGsAD(pPacket, GS_REG_CLAMP_1, 5);
    sceVif1PkAddGsAD(pPacket, GS_REG_TEX1_1, 0x1000180160);
}

/**
 * Closes the classes' data with a last GIF tag that restores the GS state (TEST, FRAME, ZBUF,
 * CLAMP).
 */
void HH_Vif1PacketBuffer_Suffix_GifTag_Open(void) {
    sceVif1Packet *pPacket;

    pPacket = &_Vif1_Packet;
    sceVif1PkCloseGifTag(pPacket);
    sceVif1PkOpenGifTag(pPacket, *(u_long128 *)_GifTag_Final);
    sceVif1PkAddGsAD(pPacket, GS_REG_TEST_1, 0x50003);
    *(u_long128 *)pPacket->pCurrent = *HH_ClassWrapper_GS_EnvironmentRegister_Frame_NoMask_Get();
    pPacket->pCurrent += 4;
    sceVif1PkAddGsAD(pPacket, GS_REG_ZBUF_1, 0xA0001C0);
    sceVif1PkAddGsAD(pPacket, GS_REG_CLAMP_1, 5);
    sceVif1PkCloseGifTag(pPacket);
    HH_Vif1PacketBuffer_GifTag_Close();
}

/** Checks the packet for overflow (prints the excess and asserts) before a GIF tag is opened. */
void HH_Vif1PacketBuffer_GifTag_Open(void) {
    if (HH_Vif1Packet_FreePacketSize_Get() < 0) {
        printf("over size = %d\n", HH_Vif1Packet_FreePacketSize_Get());

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 495
        printf("%s:%d:packet over!!\n", __FILE__, __LINE__);
        assert(0);
    }
}

/** Checks the packet for overflow (prints the excess and asserts) after a GIF tag is closed. */
void HH_Vif1PacketBuffer_GifTag_Close(void) {
    if (HH_Vif1Packet_FreePacketSize_Get() < 0) {
        printf("over size = %d\n", HH_Vif1Packet_FreePacketSize_Get());

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 535
        printf("%s:%d:packet over!!\n", __FILE__, __LINE__);
        assert(0);
    }
}

/**
 * Closes and terminates the frame's packet and sends every queued packet with d1cSend.
 *
 * Matching: the DWARF has Command_ID, which keeps d1cSend's result without using it.
 */
void HH_Vif1Packet_Send(void) {
    sceVif1Packet *pPacket;
    struct Queue_Element element;
    int Command_ID;

    pPacket = &_Vif1_Packet;
    sceVif1PkCloseDirectCode(pPacket);
    sceVif1PkEnd(pPacket, 0);
    sceVif1PkTerminate(pPacket);
    while (QueueObject_deQueue(pQueue, &element)) {
        Command_ID = d1cSend(element.pAddress);
    }
}

/**
 * Returns the free space left in the packet buffer, in quadwords (negative once it has overflowed).
 */
int HH_Vif1Packet_FreePacketSize_Get(void) {
    u_long128 *pPacketBuffer;

    pPacketBuffer = HH_MemoryManager_AllocateMemoryBlock_Get(0);
    return (unsigned int)(pPacketBuffer + 0x6000) / 16 - (unsigned int)_Vif1_Packet.pCurrent / 16;
}

/** Opens a GIF tag for A+D register writes. */
void HH_Vif1Packet_GeneralGifTag_GS_AD_Open(void) {
    sceVif1PkOpenGifTag(&_Vif1_Packet, *(u_long128 *)_GifTag_GS_AD);
}

/** Opens a GIF tag for textured sprites. */
void HH_Vif1Packet_GeneralGifTag_Sprite_Open(void) {
    sceVif1PkOpenGifTag(&_Vif1_Packet, *(u_long128 *)_GifTag_Sprite);
}

/** Opens a GIF tag for a textured triangle strip. */
void HH_Vif1Packet_GeneralGifTag_TriangleStrip_Open(void) {
    sceVif1PkOpenGifTag(&_Vif1_Packet, *(u_long128 *)_GifTag_TriangleStrip);
}

/** Opens a GIF tag for a textured triangle fan. */
void HH_Vif1Packet_GeneralGifTag_TriangleFan_Open(void) {
    sceVif1PkOpenGifTag(&_Vif1_Packet, *(u_long128 *)_GifTag_TriangleFan);
}
