/*
 * hh_class_wrapper.c: what the HH effect classes need from the rest of the game, behind one
 * interface: camera matrices and clip matrices (rebuilt once a frame), light, fog and ambient
 * parameters, James's position, and the VU0 transform/clip helpers used to build GS packets.
 *
 * Matching: the transform and copy helpers are VU0/EE assembly in C functions, as in the original
 * (its DWARF has their parameters, which the asm names). vclipw is a .word: the assembler lacks it.
 */
#include "sh2.h"
#include "sdk/libvu0.h"

static struct HH_Class_Wrapper_Work _work;
static struct HH_Class_Wrapper_Work *_pWork = &_work;

static void ViewFrustum_Primitive_ClipMatrix_Create(void) {
    float clip_mat[4][4];
    float wvm[4][4];
    float vsm[4][4];
    float x_range;
    float y_range;
    float z_near;
    float z_far;

    z_near = HH_ClassWrapper_ViewingFrustumParamerter_NearZ_Get();
    z_far = HH_ClassWrapper_ViewingFrustumParamerter_FarZ_Get();
    x_range = 1024.0f;
    y_range = 1024.0f;
    HH_ClassWrapper_WorldViewMatrix_Get(wvm);
    HH_ClassWrapper_ViewScreenMatrix_Get(vsm);
    sceVu0UnitMatrix(clip_mat);
    clip_mat[0][0] = 2.0f * vsm[0][0] / x_range;
    clip_mat[1][1] = 2.0f * vsm[1][1] / y_range;
    clip_mat[2][2] = (z_far + z_near) / (z_far - z_near);
    clip_mat[2][3] = 1.0f;
    clip_mat[3][2] = -2.0f * (z_far * z_near) / (z_far - z_near);
    clip_mat[3][3] = 0.0f;
    sceVu0MulMatrix(_pWork->ViewFrustum_Primitive_ClipMatrix, clip_mat, wvm);
}

static void ViewFrustum_BoundingBox_ClipMatrix_Create(void) {
    float clip_mat[4][4];
    float wvm[4][4];
    float vsm[4][4];
    float x_range;
    float y_range;
    float z_near;
    float z_far;

    z_near = HH_ClassWrapper_ViewingFrustumParamerter_NearZ_Get();
    z_far = HH_ClassWrapper_ViewingFrustumParamerter_FarZ_Get() - 200.0f;
    x_range = 1500.0f;
    y_range = 1500.0f;
    HH_ClassWrapper_WorldViewMatrix_Get(wvm);
    HH_ClassWrapper_ViewScreenMatrix_Get(vsm);
    sceVu0UnitMatrix(clip_mat);
    clip_mat[0][0] = 2.0f * vsm[0][0] / x_range;
    clip_mat[1][1] = 2.0f * vsm[1][1] / y_range;
    clip_mat[2][2] = (z_far + z_near) / (z_far - z_near);
    clip_mat[2][3] = 1.0f;
    clip_mat[3][2] = -2.0f * (z_far * z_near) / (z_far - z_near);
    clip_mat[3][3] = 0.0f;
    sceVu0MulMatrix(_pWork->ViewFrustum_BoundingBox_ClipMatrix, clip_mat, wvm);
}

static void AlwaysFront_WorldView_Matrix_Create(void) {
    float wvm[4][4];
    float inv_wvm[4][4];

    HH_ClassWrapper_WorldViewMatrix_Get(wvm);
    wvm[3][0] = 0.0f;
    wvm[3][1] = 0.0f;
    wvm[3][2] = 0.0f;
    sceVu0TransposeMatrix(inv_wvm, wvm);
    sceVu0CopyMatrix(_pWork->AlwaysFront_WorldView_Matrix, inv_wvm);
}

/**
 * Transforms a point to the screen (12.4 fixed-point XY, integer Z) and tests it against the
 * clip volume.
 * @param Dst_iVector            receives the screen position
 * @param pReverse_W             receives 1/w
 * @param LocalScreen_Matrix     local-to-screen matrix
 * @param LocalScreen_ClipMatrix local-to-clip matrix
 * @param Src_fVector            the point
 * @return 1 if the point is outside the clip volume, else 0
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
unsigned int HH_ClassWrapper_RotTrans_PerspectiveProjection_Clip(int *Dst_iVector, float *pReverse_W, float (*LocalScreen_Matrix)[4], float (*LocalScreen_ClipMatrix)[4], float *Src_fVector) {
    unsigned int result;
    float reverse_w;
    unsigned int clip;

    asm {
        lqc2         vf24, 0x0(LocalScreen_Matrix)
        lqc2         vf25, 0x10(LocalScreen_Matrix)
        lqc2         vf26, 0x20(LocalScreen_Matrix)
        lqc2         vf27, 0x30(LocalScreen_Matrix)
        lqc2         vf28, 0x0(LocalScreen_ClipMatrix)
        lqc2         vf29, 0x10(LocalScreen_ClipMatrix)
        lqc2         vf30, 0x20(LocalScreen_ClipMatrix)
        lqc2         vf31, 0x30(LocalScreen_ClipMatrix)
        lqc2         vf4, 0x0(Src_fVector)
        vmulax.xyzw  ACC, vf28, vf4x
        vmadday.xyzw ACC, vf29, vf4y
        vmaddaz.xyzw ACC, vf30, vf4z
        vmaddw.xyzw  vf7, vf31, vf0w
        .word        0x4BC739FF /* vclipw.xyz vf7, vf7w */
        vmulax.xyzw  ACC, vf24, vf4x
        vmadday.xyzw ACC, vf25, vf4y
        vmaddaz.xyzw ACC, vf26, vf4z
        vmaddw.xyzw  vf5, vf27, vf0w
        vdiv         Q, vf0w, vf5w
        vwaitq
        vmulq.xyz    vf5, vf5, Q
        vftoi4.xy    vf6, vf5
        vftoi0.z     vf6, vf5
        sqc2         vf6, 0x0(Dst_iVector)
        vaddq.x      vf1, vf0, Q
        qmfc2.ni     clip, vf1
        mtc1         clip, reverse_w
        cfc2.ni      clip, vi18
    }
    if (clip & 0x3FFFF) {
        result = 1;
    } else {
        result = 0;
    }
    *pReverse_W = reverse_w;
    return result;
}

/**
 * Transforms a vertex for a triangle strip: screen position (12.4 XY, Z) with the ADC bit set
 * (the vertex doesn't draw a triangle) if it is outside the clip volume, and STQ scaled by 1/w.
 * @param Dst_iVector            receives the screen position
 * @param STQ_fVector            texture S and T in, S/w, T/w and 1/w out
 * @param LocalScreen_Matrix     local-to-screen matrix
 * @param LocalScreen_ClipMatrix local-to-clip matrix
 * @param Src_fVector            the vertex
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void HH_ClassWrapper_Transform_PerspectiveProjection_Clip_forTriangleStrip(int *Dst_iVector, float *STQ_fVector, float (*LocalScreen_Matrix)[4], float (*LocalScreen_ClipMatrix)[4], float *Src_fVector) {
    asm {
        .set noreorder
        lqc2         vf24, 0x0(LocalScreen_Matrix)
        lqc2         vf25, 0x10(LocalScreen_Matrix)
        lqc2         vf26, 0x20(LocalScreen_Matrix)
        lqc2         vf27, 0x30(LocalScreen_Matrix)
        lqc2         vf28, 0x0(LocalScreen_ClipMatrix)
        lqc2         vf29, 0x10(LocalScreen_ClipMatrix)
        lqc2         vf30, 0x20(LocalScreen_ClipMatrix)
        lqc2         vf31, 0x30(LocalScreen_ClipMatrix)
        ori          a2, zero, 0x8000
        lui          v1, 0x3
        ori          v1, v1, 0xFFFF
        lqc2         vf3, 0x0(STQ_fVector)
        lqc2         vf4, 0x0(Src_fVector)
        vmulax.xyzw  ACC, vf28, vf4x
        vmadday.xyzw ACC, vf29, vf4y
        vmaddaz.xyzw ACC, vf30, vf4z
        vmaddw.xyzw  vf7, vf31, vf0w
        .word        0x4BC739FF /* vclipw.xyz vf7, vf7w */
        vmulax.xyzw  ACC, vf24, vf4x
        vmadday.xyzw ACC, vf25, vf4y
        vmaddaz.xyzw ACC, vf26, vf4z
        vmaddw.xyzw  vf5, vf27, vf0w
        vdiv         Q, vf0w, vf5w
        vwaitq
        vmulq.xy     vf3, vf3, Q
        vaddq.z      vf3, vf0, Q
        sqc2         vf3, 0x0(STQ_fVector)
        vmulq.xyz    vf5, vf5, Q
        vftoi4.xyz   vf6, vf5
        vsubw.w      vf6, vf0, vf0w
        cfc2.ni      t1, vi18
        and          t1, v1, t1
        beqz         t1, @1
        nop
        ctc2.ni      a2, vi1
        vmfir.w      vf6, vi1
    @1:
        sqc2         vf6, 0x0(Dst_iVector)
        .set reorder
    }
}

/**
 * HH_ClassWrapper_Transform_PerspectiveProjection_Clip_forTriangleStrip with a caller-chosen
 * mask of clip flags that set the ADC bit.
 * @param Clip_Mask VU0 clip flags to test
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(int *Dst_iVector, float *STQ_fVector, float (*LocalScreen_Matrix)[4], float (*LocalScreen_ClipMatrix)[4], float *Src_fVector, unsigned int Clip_Mask) {
    asm {
        .set noreorder
        lqc2         vf24, 0x0(LocalScreen_Matrix)
        lqc2         vf25, 0x10(LocalScreen_Matrix)
        lqc2         vf26, 0x20(LocalScreen_Matrix)
        lqc2         vf27, 0x30(LocalScreen_Matrix)
        lqc2         vf28, 0x0(LocalScreen_ClipMatrix)
        lqc2         vf29, 0x10(LocalScreen_ClipMatrix)
        lqc2         vf30, 0x20(LocalScreen_ClipMatrix)
        lqc2         vf31, 0x30(LocalScreen_ClipMatrix)
        ori          v1, zero, 0x8000
        paddub       a2, Clip_Mask, zero
        lqc2         vf3, 0x0(STQ_fVector)
        lqc2         vf4, 0x0(Src_fVector)
        vmulax.xyzw  ACC, vf28, vf4x
        vmadday.xyzw ACC, vf29, vf4y
        vmaddaz.xyzw ACC, vf30, vf4z
        vmaddw.xyzw  vf7, vf31, vf0w
        .word        0x4BC739FF /* vclipw.xyz vf7, vf7w */
        vmulax.xyzw  ACC, vf24, vf4x
        vmadday.xyzw ACC, vf25, vf4y
        vmaddaz.xyzw ACC, vf26, vf4z
        vmaddw.xyzw  vf5, vf27, vf0w
        vdiv         Q, vf0w, vf5w
        vwaitq
        vmulq.xy     vf3, vf3, Q
        vaddq.z      vf3, vf0, Q
        sqc2         vf3, 0x0(STQ_fVector)
        vmulq.xyz    vf5, vf5, Q
        vftoi4.xyz   vf6, vf5
        vsubw.w      vf6, vf0, vf0w
        cfc2.ni      t1, vi18
        and          t1, a2, t1
        beqz         t1, @1
        nop
        ctc2.ni      v1, vi1
        vmfir.w      vf6, vi1
    @1:
        sqc2         vf6, 0x0(Dst_iVector)
        .set reorder
    }
}

/**
 * Counts how many of Array_Max points are outside the clip volume.
 * @param LocalScreen_ClipMatrix local-to-clip matrix
 * @param pSrc_fVector_Array     the points
 * @param Array_Max              number of points
 * @return the number of points outside (0: all inside)
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
unsigned int HH_ClassWrapper_Point_Clip_Judge(float (*LocalScreen_ClipMatrix)[4], float (*pSrc_fVector_Array)[4], unsigned int Array_Max) {
    unsigned int clip_judge_acc;

    asm {
        .set noreorder
        paddub       clip_judge_acc, zero, zero
        lqc2         vf28, 0x0(LocalScreen_ClipMatrix)
        lqc2         vf29, 0x10(LocalScreen_ClipMatrix)
        lqc2         vf30, 0x20(LocalScreen_ClipMatrix)
        lqc2         vf31, 0x30(LocalScreen_ClipMatrix)
        lqc2         vf4, 0x0(pSrc_fVector_Array)
        nop
    @loop:
        vmulax.xyzw  ACC, vf28, vf4x
        vmadday.xyzw ACC, vf29, vf4y
        vmaddaz.xyzw ACC, vf30, vf4z
        vmaddw.xyzw  vf4, vf31, vf0w
        .word        0x4BC421FF /* vclipw.xyz vf4, vf4w */
        vnop
        vnop
        addi         Array_Max, Array_Max, -0x1
        addiu        pSrc_fVector_Array, pSrc_fVector_Array, 0x10
        lqc2         vf4, 0x0(pSrc_fVector_Array)
        cfc2.ni      t1, vi18
        andi         t1, t1, 0x3F
        beq          zero, t1, @1
        nop
        addiu        clip_judge_acc, clip_judge_acc, 0x1
    @1:
        bgtz         Array_Max, @loop
        nop
        .set reorder
    }
    return clip_judge_acc;
}

/**
 * Rebuilds the per-frame matrices: the two view-frustum clip matrices and the always-facing-camera
 * matrix.
 */
void HH_ClassWrapper_Matrix_Group_Update(void) {
    ViewFrustum_Primitive_ClipMatrix_Create();
    ViewFrustum_BoundingBox_ClipMatrix_Create();
    AlwaysFront_WorldView_Matrix_Create();
}

/** Copies the world-to-clip matrix the classes clip primitives and positions with. */
void HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(float (*ViewFrustum_Primitive_ClipMatrix)[4]) {
    sceVu0CopyMatrix(ViewFrustum_Primitive_ClipMatrix, _pWork->ViewFrustum_Primitive_ClipMatrix);
}

/** Copies the same world-to-clip matrix as HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get. */
void HH_ClassWrapper_ViewFrustum_ClipMatrix_Get(float (*ViewFrustum_Primitive_ClipMatrix)[4]) {
    sceVu0CopyMatrix(ViewFrustum_Primitive_ClipMatrix, _pWork->ViewFrustum_Primitive_ClipMatrix);
}

/** Copies the inverse of the view rotation, which turns a local matrix to face the camera. */
void HH_ClassWrapper_AlwaysFront_WorldView_Matrix_Get(float (*AlwaysFront_WorldView_Matrix)[4]) {
    sceVu0CopyMatrix(AlwaysFront_WorldView_Matrix, _pWork->AlwaysFront_WorldView_Matrix);
}

/** Copies the current world-to-view matrix. */
void HH_ClassWrapper_WorldViewMatrix_Get(float (*WorldView_Matrix)[4]) {
    sceVu0CopyMatrix(WorldView_Matrix, VbWvsMatrix.wvm);
}

/** Copies the current view-to-screen matrix. */
void HH_ClassWrapper_ViewScreenMatrix_Get(float (*ViewScreen_Matrix)[4]) {
    sceVu0CopyMatrix(ViewScreen_Matrix, cam0.view_screen);
}

/** Copies the current world-to-screen matrix. */
void HH_ClassWrapper_WorldScreenMatrix_Get(float (*WorldScreen_Matrix)[4]) {
    sceVu0CopyMatrix(WorldScreen_Matrix, cam0.world_screen);
}

/** Gets the camera's view direction. */
void HH_ClassWrapper_ViewDirection_Get(float *View_Direction) {
    sh2gde_getCameraDir(View_Direction);
}

/** Gets the direction of James's flashlight. */
void HH_ClassWrapper_LightDirection_Get(float *Light_Direction) {
    float tmp[4];

    shGetJamesLightPos(tmp, Light_Direction);
}

/** Returns the camera's near Z. */
float HH_ClassWrapper_ViewingFrustumParamerter_NearZ_Get(void) {
    return Env_ctl.camera_parms[3];
}

/** Returns the camera's far Z. */
float HH_ClassWrapper_ViewingFrustumParamerter_FarZ_Get(void) {
    return Env_ctl.camera_parms2[2];
}

/** Returns the constant term of the fog value as a linear function of 1/w (see FogParameter_B). */
float HH_ClassWrapper_FogParameter_A_Get(void) {
    return (Env_ctl.fogparm.fl32[0] * Env_ctl.fogparm.fl32[2] - Env_ctl.fogparm.fl32[1] * Env_ctl.fogparm.fl32[3]) /
           (Env_ctl.fogparm.fl32[0] - Env_ctl.fogparm.fl32[1]);
}

/** Returns the 1/w factor of the fog value: fog = A + B * (1/w). */
float HH_ClassWrapper_FogParameter_B_Get(void) {
    return Env_ctl.fogparm.fl32[0] * Env_ctl.fogparm.fl32[1] * (Env_ctl.fogparm.fl32[3] - Env_ctl.fogparm.fl32[2]) /
           (Env_ctl.fogparm.fl32[0] - Env_ctl.fogparm.fl32[1]);
}

/** Returns Src_Value clamped to [Min, Max] (max.s; min.s). */
float HH_ClassWrapper_Float_Clamp(float Src_Value, float Min, float Max) {
    __asm__ __volatile__("
    max.s %0, %1, %2
    min.s %0, %0, %3
    " : "=f"(Src_Value) : "f"(Src_Value), "f"(Min), "f"(Max));
    return Src_Value;
}

/** Gets the ambient colour (the environment's ambient light, doubled). */
void HH_ClassWrapper_AmbientColor_Get(float *Ambient_Color) {
    sceVu0ScaleVector(Ambient_Color, Env_ctl.ambient, 2.0f);
}

/** Returns whether James's flashlight is on. */
unsigned int HH_ClassWrapper_SpotLight_Enable_Check(void) {
    return item.light_switch;
}

/**
 * Gets the flashlight's position, direction and colour, and its cone parameters (Parameter[0]
 * is the cosine of the cone angle, Parameter[2] the range).
 */
void HH_ClassWrapper_SpotLight_EnvironmentParameter_Get(float *Light_Position, float *Light_Direction, float *Light_Color, float *Parameter) {
    float tmp[4];

    sh2gde_getspotKTParams(Light_Position, Light_Direction, Light_Color, tmp, 0);
    kari_sh2gde_getspotParams(tmp, tmp, Parameter);
}

/**
 * Returns how strongly the spotlight lights a vertex: 0 outside the cone or beyond Far_Z, else
 * falling off linearly with distance and with the angle from the cone's axis.
 * @param Light_Position  spotlight position
 * @param Light_Direction spotlight direction (normalized)
 * @param Vertex          the vertex
 * @param Cos_Value       cosine of the cone angle
 * @param Far_Z           the light's range
 */
float HH_ClassWrapper_SpotLight_ColorRatio_Calculator(float *Light_Position, float *Light_Direction, float *Vertex, float Cos_Value, float Far_Z) {
    float result;
    float vec[4];
    float cos_phai;
    float vec_volume;
    float ratio_0;
    float ratio_1;

    result = 0.0f;
    sceVu0SubVector(vec, Vertex, Light_Position);
    vec_volume = HH_MathWrapper_Sqrtf(sceVu0InnerProduct(vec, vec));
    if (vec_volume <= Far_Z) {
        sceVu0Normalize(vec, vec);
        cos_phai = sceVu0InnerProduct(vec, Light_Direction);
        if (cos_phai > Cos_Value) {
            ratio_0 = 1.0f - vec_volume / Far_Z;
            ratio_1 = (cos_phai - Cos_Value) / (1.0f - Cos_Value);
            result = ratio_0 * ratio_1;
        }
    }
    return result;
}

/**
 * Returns a specular factor: 0 when the reflected light is more than acos(Cos_Value) away from
 * the view direction, rising linearly to 1 when they line up.
 * @param View_Direction  view direction
 * @param Light_Direction light direction
 * @param Normal_Vector   surface normal
 * @param Cos_Value       cosine of the highlight's angular size
 */
float HH_ClassWrapper_SpecularRatio0_Calculator(float *View_Direction, float *Light_Direction, float *Normal_Vector, float Cos_Value) {
    float result;
    float specular_coefficient;
    float input_light_power;
    float reverse_light_dir[4];
    float tmp_vec[4];
    float cos_theta;
    float cos_beta;

    cos_theta = sceVu0InnerProduct(Light_Direction, Normal_Vector);
    if (cos_theta < 0.0f) {
        cos_theta = -cos_theta;
    }
    sceVu0ScaleVectorXYZ(tmp_vec, Normal_Vector, 2.0f * cos_theta);
    sceVu0SubVector(reverse_light_dir, tmp_vec, Light_Direction);
    sceVu0Normalize(reverse_light_dir, reverse_light_dir);
    cos_beta = sceVu0InnerProduct(reverse_light_dir, View_Direction);
    if (cos_beta < 0.0f) {
        cos_beta = -cos_beta;
    }
    input_light_power = 1.0f;
    specular_coefficient = (cos_beta < Cos_Value) ? 0.0f : 1.0f - (1.0f - cos_beta) / (1.0f - Cos_Value);
    result = specular_coefficient * input_light_power;
    return result;
}

/**
 * iRGBA = int(clamp(RGBA_Base + Specular_RGBA_Base * Specular_Ratio, 0, 255)), on VU0.
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void HH_ClassWrapper_SpecularRGBA_Add_BaseRGBA(int *iRGBA, float *RGBA_Base, float *Specular_RGBA_Base, float Specular_Ratio) {
    __asm__ __volatile__("
    lqc2        vf30, 0x0(%1)
    lqc2        vf31, 0x0(%2)
    mfc1        t0, %3
    mfc1        t1, %4
    qmtc2.ni    t0, vf29
    ctc2.ni     t1, vi21
    vmulx.xyzw  vf31, vf31, vf29x
    vadd.xyzw   vf31, vf30, vf31
    vmaxx.xyzw  vf31, vf31, vf0x
    vminii.xyzw vf31, vf31, I
    vftoi0.xyzw vf31, vf31
    sqc2        vf31, 0x0(%0)
    " : : "r"(iRGBA), "r"(RGBA_Base), "r"(Specular_RGBA_Base), "f"(Specular_Ratio), "f"(255.0f));
}

/**
 * Copies Cycle_Max runs of LoadStore_Size quadwords, skipping Next_Offset quadwords after each
 * run in both source and destination (16-byte aligned).
 * @return 0
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
unsigned int HH_ClassWrapper_MemoryCopy128Align_DesignateCycle(void *pDestination_Address, void *pSource_Address, unsigned int Cycle_Max, unsigned int LoadStore_Size, unsigned int Next_Offset) {
    unsigned int result;

    asm {
        .set noreorder
        paddub       result, zero, zero
        addiu        t4, zero, 0x10
        multu        Next_Offset, t4
        nop
        nop
        mflo         Next_Offset
    @loop1:
        addu         t5, LoadStore_Size, zero
    @loop2:
        lq           t4, 0x0(pSource_Address)
        sq           t4, 0x0(pDestination_Address)
        addi         pSource_Address, pSource_Address, 0x10
        addi         pDestination_Address, pDestination_Address, 0x10
        addi         t5, t5, -0x1
        bgtz         t5, @loop2
        nop
        addu         pSource_Address, pSource_Address, Next_Offset
        addu         pDestination_Address, pDestination_Address, Next_Offset
        addi         Cycle_Max, Cycle_Max, -0x1
        bgtz         Cycle_Max, @loop1
        nop
        .set reorder
    }
    return result;
}

/** Returns the GS FRAME register setting that masks off alpha writes. */
u_long128 *HH_ClassWrapper_GS_EnvironmentRegister_Frame_AlphaMask_Get(void) {
    return sh2gfw_Get_FrameAlphaRegAddr();
}

/** Returns the normal GS FRAME register setting. */
u_long128 *HH_ClassWrapper_GS_EnvironmentRegister_Frame_NoMask_Get(void) {
    return sh2gfw_Get_FrameNormalRegAddr();
}

/**
 * Walks back over a triangle-strip mesh just written to the packet and sets each vertex's ADC
 * bit when either of the two vertices before it has it, so that no triangle with a clipped
 * vertex is drawn.
 * @param pPacket_End end of the mesh's packet data
 * @param Mesh_Width  vertices per row
 * @param Mesh_Height number of rows
 */
void HH_ClassWrapper_Packet_ADC_Flag_OnceMore_Set(unsigned int *pPacket_End, unsigned int Mesh_Width, unsigned int Mesh_Height) {
    unsigned int w_index;
    unsigned int h_index;
    unsigned int mesh_width;
    unsigned int status_accumlate;

    mesh_width = (Mesh_Width - 1) * 2;
    pPacket_End--;
    for (h_index = 0; h_index < Mesh_Height; h_index++) {
        for (w_index = 0; w_index < mesh_width; w_index++) {
            status_accumlate = (pPacket_End[-12] | pPacket_End[-24]) & 0x8000;
            *pPacket_End |= status_accumlate;
            pPacket_End -= 12;
        }
        pPacket_End -= 28;
    }
}

/** Gets James's position. Returns 1, or 0 (Position untouched) if there is no player. */
unsigned int HH_ClassWrapper_JMS_WorldPosition_Get(float *Position) {
    unsigned int result;

    result = 0;
    if (sh2jms.player != NULL) {
        sceVu0CopyVector(Position, (float *)&sh2jms.player->pos);
        result = 1;
    }
    return result;
}
