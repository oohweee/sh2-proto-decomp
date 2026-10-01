/*
 * hh_class_object_execute.c: the Effect2 entry points called by the game (init, per-frame
 * execute, packet kick) and the impact posts used by characters (blood, footsteps, splashes).
 * The Class_Descriptor numbers are indices into the class table in hh_effect_object_def.c.
 */
#include "sh2.h"
#include "libc/string.h"
#include "sdk/libvu0.h"

static void Prefix_Operation(void) {
}

static void Suffix_Operation(void) {
}

static void Debug_Functions(void) {
}

/**
 * Initializes Effect2: allocates the object memory blocks and posts the objects the room starts
 * with.
 */
void HH_Class_Object_Initialize(void) {
    HH_Effect_Object_MemoryBlock_Allocate();
    HH_Effect_Object_AutoPost();
}

/**
 * Runs one frame of Effect2: updates the camera matrices, starts the packet, runs every effect
 * class and closes the packet. Does nothing until the packet memory is allocated.
 */
void HH_Class_Object_Execute(void) {
    if (HH_Vif1PacketBuffer_Memory_Allocate_Check()) {
        HH_ClassWrapper_Matrix_Group_Update();
        HH_Vif1PacketBuffer_Initialize();
        HH_Vif1PacketBuffer_Prefix_GifTag_Open();
        Prefix_Operation();
        Debug_Functions();
        HH_Effect_Object_Manager();
        Suffix_Operation();
        HH_Vif1PacketBuffer_Suffix_GifTag_Open();
    }
}

/** Sends this frame's Effect2 packet (if the packet memory is allocated). */
void HH_Class_Object_Packet_Kick(void) {
    if (HH_Vif1PacketBuffer_Memory_Allocate_Check()) {
        HH_Vif1Packet_Send();
    }
}

/**
 * Posts a blood pool effect: in the rooms with water surfaces a blood spray (class 4,
 * HH_Class_Blood_Pool_Phenomenon_01), elsewhere pools (class 3, HH_Class_Blood_Pool_Phenomenon_00).
 * @param Location where the blood falls
 * @param kind     unused
 */
void HH_Effect_Object_Blood_Pool_Impact_Post(float *Location, int kind) {
    struct ImpactQueue_Element descriptor;
    int room_name;

    room_name = RoomNameJms();
    switch (room_name) {
    case 0x63:
    case 0x62:
    case 0x80:
    case 0x85:
    case 0x7A:
    case 0x7C:
    case 0x7E:
    case 0x82:
    case 0xBB:
    case 0xB6:
    case 0xB7:
    case 0xAB:
    case 0xB8:
    case 0xB9:
    case 0x21: {
        float dir[4] = { 0.0f, 2000.0f, 0.0f, 0.0f };

        descriptor.Class_Descriptor = 4;
        sceVu0CopyVector(descriptor.Option.Vector[1], dir);
        break;
    }
    default:
        descriptor.Class_Descriptor = 3;
        break;
    }
    descriptor.hInstance = 0;
    descriptor.pResultHandle_Address = NULL;
    sceVu0CopyVector(descriptor.Option.Vector[0], Location);
    ImpactDescriptor_Post(HH_Effect_Object_Infomeation_Get(), &descriptor);
}

/**
 * Posts blood spurting from a wounded character (class 5, or class 7 in rooms 0x07 and 0x21).
 * @param Location    wound position (only its height is used; x and z come from the character)
 * @param Direction   direction of the hit
 * @param Scp_Address the character (a SubCharacter pointer)
 * @param Impact_Kind 0: blood thrown along Direction at a fixed speed; non-zero: thrown back
 *                    against Direction, scaled by 5
 */
void HH_Effect_Object_Blood_Splash_Impact_Post(float *Location, float *Direction, unsigned int Scp_Address, unsigned int Impact_Kind) {
    int room_name;
    struct ImpactQueue_Element descriptor;
    struct SubCharacter *pSubChar;

    room_name = RoomNameJms();
    switch (room_name) {
    case 7:
    case 0x21:
        descriptor.Class_Descriptor = 7;
        break;
    default:
        descriptor.Class_Descriptor = 5;
        break;
    }
    descriptor.hInstance = 0;
    descriptor.pResultHandle_Address = NULL;
    pSubChar = (struct SubCharacter *)Scp_Address;
    sceVu0CopyVector(descriptor.Option.Vector[0], (float *)&pSubChar->pos);
    descriptor.Option.Vector[0][1] = Location[1];
    descriptor.Option.Int_Value[0] = Scp_Address;
    if (Impact_Kind) {
        float _Direction[4];

        sceVu0ScaleVectorXYZ(_Direction, Direction, 5.0f);
        _Direction[0] *= -1.0f;
        _Direction[2] *= -1.0f;
        sceVu0CopyVector(descriptor.Option.Vector[1], _Direction);
    } else {
        float _Direction[4];

        sceVu0Normalize(_Direction, Direction);
        sceVu0ScaleVectorXYZ(_Direction, _Direction, 2000.0f);
        sceVu0CopyVector(descriptor.Option.Vector[1], _Direction);
    }
    ImpactDescriptor_Post(HH_Effect_Object_Infomeation_Get(), &descriptor);
}

/**
 * Posts a footstep: in water rooms a water footstep (class 2, HH_Class_Plural_Phenomenon_01),
 * elsewhere a blood footstep (class 1, HH_Class_Plural_Phenomenon_00) snapped to the floor.
 * @param Toe          toe position
 * @param Heel         heel position
 * @param Foot_Kind    which foot (footmark kind)
 * @param Character_ID the character
 */
void HH_Effect_Object_Ground_Impact_Post(float *Toe, float *Heel, unsigned int Foot_Kind, unsigned int Character_ID) {
    int room_name;
    struct ImpactQueue_Element descriptor;

    room_name = RoomNameJms();
    descriptor.hInstance = 0;
    descriptor.pResultHandle_Address = NULL;
    sceVu0CopyVector(descriptor.Option.Vector[0], Toe);
    sceVu0CopyVector(descriptor.Option.Vector[1], Heel);
    switch (room_name) {
    case 0x63:
    case 0x62:
    case 0x80:
    case 0x85:
    case 0x7A:
    case 0x7C:
    case 0x7E:
    case 0x82:
    case 0xBB:
    case 0xB6:
    case 0xB7:
    case 0xAB:
    case 0xB8:
    case 0xB9:
    case 0x21:
    case 0x25:
    case 0x26:
        descriptor.Class_Descriptor = 2;
        descriptor.Option.Vector[0][1] -= 50.0f;
        descriptor.Option.Vector[1][1] -= 50.0f;
        break;
    default: {
        struct _CL_VHIT_RESULT hit_result;
        float e_pos[4] = { 0.0f, 100.0f, 0.0f, 0.0f };

        memset(&hit_result, 0, sizeof(hit_result));
        sceVu0AddVector(e_pos, e_pos, descriptor.Option.Vector[0]);
        clCheckHitEyesOnlyFloor(&hit_result, 0, descriptor.Option.Vector[0], e_pos);
        if (hit_result.kind == 1) {
            descriptor.Option.Vector[0][1] = descriptor.Option.Vector[1][1] = hit_result.hobj.wall.cp[1];
        }
        descriptor.Class_Descriptor = 1;
        break;
    }
    }
    descriptor.Option.Int_Value[0] = Foot_Kind;
    descriptor.Option.Int_Value[1] = Character_ID;
    ImpactDescriptor_Post(HH_Effect_Object_Infomeation_Get(), &descriptor);
}

/**
 * Posts an enemy's footstep in the water rooms (class 2, HH_Class_Plural_Phenomenon_01); does
 * nothing elsewhere.
 * @param Foot_Location foot position
 * @param Foot_Kind     which foot
 */
void HH_Effect_Object_Ground_Impact_Post_forEnemy(float *Foot_Location, unsigned int Foot_Kind) {
    int room_name;
    struct ImpactQueue_Element descriptor;

    room_name = RoomNameJms();
    switch (room_name) {
    case 0x63:
    case 0x80:
    case 0x85:
    case 0x7A:
    case 0x7C:
    case 0x7E:
    case 0x82:
    case 0xB6:
    case 0xB7:
    case 0xB8:
    case 0xB9:
    case 0x21:
    case 0x25:
    case 0x26:
        descriptor.hInstance = 0;
        descriptor.pResultHandle_Address = NULL;
        sceVu0CopyVector(descriptor.Option.Vector[0], Foot_Location);
        sceVu0CopyVector(descriptor.Option.Vector[1], Foot_Location);
        descriptor.Class_Descriptor = 2;
        descriptor.Option.Vector[0][1] -= 50.0f;
        descriptor.Option.Vector[1][1] -= 50.0f;
        descriptor.Option.Int_Value[0] = Foot_Kind;
        ImpactDescriptor_Post(HH_Effect_Object_Infomeation_Get(), &descriptor);
        break;
    }
}

/**
 * Posts a water splash (class 18, HH_Class_Water_Splash_Phenomenon_00) at Location.
 * @param Location splash position
 * @param kind     unused
 */
void HH_Effect_Object_WaterSplash_Impact_Post(float *Location, int kind) {
    static float direction[4] = { 0.0f, -2500.0f, 0.0f, 1.0f };
    static float hoge = 3500.0f;
    struct ImpactQueue_Element descriptor;

    descriptor.hInstance = 0;
    descriptor.pResultHandle_Address = NULL;
    sceVu0CopyVector(descriptor.Option.Vector[0], Location);
    sceVu0CopyVector(descriptor.Option.Vector[1], direction);
    descriptor.Class_Descriptor = 18;
    descriptor.Option.Vector[0][1] -= 50.0f;
    descriptor.Option.Vector[1][1] -= 50.0f;
    descriptor.Option.Float_Value[0] = hoge;
    descriptor.Option.Int_Value[0] = 1;
    ImpactDescriptor_Post(HH_Effect_Object_Infomeation_Get(), &descriptor);
}
