/*
 * Shadow VU1 microprogram frames (sh2shd): the parameter blocks (matrices, GIF tags, colors,
 * alpha registers) of the stencil-shadow and drop-shadow VU1 microprograms, and the DMA tags
 * that send a frame and its raw shadow data to VU1 and start the program.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "fi_libvu0_inline.h"

#define SHD_KICK_PACKET_SIZE 640

/* Matching: quadword copy through a fixed a2; drop_shadow_micro_init moves ds_env out of a2 for it. */
inline void qcopy(void *s, void *d) {
    asm {
        lq a2, 0(s)
        sq a2, 0(d)
    }
}

/** Sets up a stencil-shadow microprogram frame: shadow color, GIF tags, alpha registers from env. */
void shadow_micro_init(struct SHADOW_MICRO_FRAME *mic, struct shGsAllEnv *env) {
    mic->color.ui32[0] = 8;
    mic->color.ui32[1] = 8;
    mic->color.ui32[2] = 8;
    mic->color.ui32[3] = 0x80;
    mic->giftag_color.ul64[0] = 0x1000000000000001;
    mic->giftag_color.ul64[1] = 1;
    mic->giftag_dummy.ul64[0] = 0x8000;
    mic->giftag_dummy.ul64[1] = 0;
    mic->giftag_default.ul64[0] = 0x5022C00000000001;
    mic->giftag_default.ul64[1] = 0x4444E;
    mic->giftag_3.ul64[0] = 0x5021C00000008001;
    mic->giftag_3.ul64[1] = 0x444E1;
    mic->giftag_4.ul64[0] = 0x6022C00000008001;
    mic->giftag_4.ul64[1] = 0x4444E1;
    mic->giftag_5.ul64[0] = 0x7022C00000008001;
    mic->giftag_5.ul64[1] = 0x44444E1;
    mic->giftag_6.ul64[0] = 0x8022C00000008001;
    mic->giftag_6.ul64[1] = 0x444444E1;
    mic->giftag_7.ul64[0] = 0x9022C00000008001;
    mic->giftag_7.ul64[1] = 0x4444444E1;
    mic->giftag_8.ul64[0] = 0xA022C00000008001;
    mic->giftag_8.ul64[1] = 0x44444444E1;
    mic->giftag_9.ul64[0] = 0xB022C00000008001;
    mic->giftag_9.ul64[1] = 0x444444444E1;
    mic->giftag_10.ul64[0] = 0xC022C00000008001;
    mic->giftag_10.ul64[1] = 0x4444444444E1;
    qcopy(&env->GsReg_ALPHA_C[1], &mic->alpha0);
    qcopy(&env->GsReg_ALPHA_B[1], &mic->alpha1);
    mic->flags.ui32[0] = 1;
    mic->flags.ui32[1] = 0;
    mic->flags.ui32[2] = 0;
    mic->flags.ui32[3] = 0;
    mic->y_unit[0] = 0.0f;
    mic->y_unit[1] = 1.0f;
    mic->y_unit[2] = 0.0f;
    mic->y_unit[3] = 0.0f;
    mic->pKickAddr = NULL;
    mic->pRawData = NULL;
}

/**
 * Sets a stencil-shadow frame's per-frame parameters.
 * @param drop_shadow   the shadow projection matrix
 * @param spot_cam_flag stored in the frame's flags
 */
void shadow_set_micro_params(struct SHADOW_MICRO_FRAME *mic, struct sh2gfw_CAMERA *cam, float (*drop_shadow)[4], int spot_cam_flag) {
    mcopy(drop_shadow, mic->drop_shadow);
    mcopy(cam->world_clip, mic->world_clip);
    mcopy(cam->clip_screen, mic->clip_screen);
    vcGetNowCamPos(mic->cam_pos);
    mic->flags.ui32[2] = spot_cam_flag;
}

/**
 * Appends the DMA tags that call the raw shadow data, unpack the stencil-shadow frame to VU1
 * and start the microprogram.
 */
void shadow_add_micro2kick_packet(struct SHADOW_MICRO_FRAME *mic, struct SHADOW_PACKET_BUF *packbuf, unsigned int *rawdata) {
    union Q_WORDDATA *curr;

    curr = packbuf->curr;
    mic->pKickAddr = (unsigned int *)packbuf;
    mic->pRawData = rawdata;
    curr[0].ui32[0] = 0x50000000;
    curr[0].ui32[1] = (unsigned int)mic->pRawData;
    curr[0].ui32[2] = 0;
    curr[0].ui32[3] = 0;
    curr[1].ui32[0] = 0x30000022;
    curr[1].ui32[1] = (unsigned int)mic & 0x7FFFFFFF;
    curr[1].ui32[2] = 0x1000101;
    curr[1].ui32[3] = 0x6C220010;
    curr[2].ui32[0] = 0x10000000;
    curr[2].ui32[1] = 0;
    curr[2].ui32[2] = 0x300006E;
    curr[2].ui32[3] = 0x20001C9;
    curr[3].ui32[0] = 0x70000000;
    curr[3].ui32[1] = 0;
    curr[3].ul64[1] = 0;
    packbuf->curr = curr + 3;
    /* Matching: the #line keeps the assert string below on the original's line. */
#line 157
    assert_dw(packbuf->curr - packbuf->head < SHD_KICK_PACKET_SIZE);
}

/** Sets up a drop-shadow microprogram frame: matrices, color and alpha range from ds_env, fog, GIF tags. */
void drop_shadow_micro_init(struct DROP_SHADOW_MICRO_FRAME *mic, struct shGsAllEnv *env, struct DROP_SHADOW_ENV *ds_env) {
    float unit[4][4];

    _sceVu0UnitMatrix(unit);
    mcopy(unit, mic->world_clip);
    mcopy(unit, mic->clip_screen);
    mic->color.ui32[0] = ds_env->color;
    mic->color.ui32[1] = ds_env->color;
    mic->color.ui32[2] = ds_env->color;
    mic->color.ui32[3] = 0x80;
    mic->giftag_color.ul64[0] = 0x1000000000000001;
    mic->giftag_color.ul64[1] = 1;
    mic->giftag_dummy.ul64[0] = 0x8000;
    mic->giftag_dummy.ul64[1] = 0;
    mic->giftag_default.ul64[0] = 0x2022400000008000;
    mic->giftag_default.ul64[1] = 0x41;
    mic->fog_param.fl32[2] = (Env_ctl.fogparm.fl32[0] * Env_ctl.fogparm.fl32[2] - Env_ctl.fogparm.fl32[1] * Env_ctl.fogparm.fl32[3]) / (Env_ctl.fogparm.fl32[0] - Env_ctl.fogparm.fl32[1]);
    mic->fog_param.fl32[3] = Env_ctl.fogparm.fl32[0] * Env_ctl.fogparm.fl32[1] * (Env_ctl.fogparm.fl32[3] - Env_ctl.fogparm.fl32[2]) / (Env_ctl.fogparm.fl32[0] - Env_ctl.fogparm.fl32[1]);
    qcopy(&env->GsReg_ALPHA_B[1], &mic->alpha0);
    mic->alpha_range.fl32[0] = ds_env->alpha_min;
    mic->alpha_range.fl32[1] = ds_env->alpha_max;
    mic->alpha_range.fl32[2] = 0.0f;
    mic->alpha_range.fl32[3] = 0.0f;
    mic->alpha_switch.ui32[0] = 0;
    mic->pKickAddr = NULL;
    mic->pRawData = NULL;
}

/** Copies the camera's clip matrices into a drop-shadow frame. */
void drop_shadow_set_micro_params(struct DROP_SHADOW_MICRO_FRAME *mic, struct sh2gfw_CAMERA *cam) {
    mcopy(cam->world_clip, mic->world_clip);
    mcopy(cam->clip_screen, mic->clip_screen);
}

/**
 * Appends the DMA tags that call the raw drop-shadow data, unpack the frame to VU1 and start
 * the microprogram.
 */
void drop_shadow_add_micro2kick_packet(struct DROP_SHADOW_MICRO_FRAME *mic, struct SHADOW_PACKET_BUF *packbuf, unsigned int *rawdata) {
    union Q_WORDDATA *curr;

    curr = packbuf->curr;
    mic->pKickAddr = (unsigned int *)packbuf;
    mic->pRawData = rawdata;
    curr[0].ui32[0] = 0x50000000;
    curr[0].ui32[1] = (unsigned int)mic->pRawData;
    curr[0].ui32[2] = 0;
    curr[0].ui32[3] = 0;
    curr[1].ui32[0] = 0x30000010;
    curr[1].ui32[1] = (unsigned int)mic->world_clip & 0x7FFFFFFF;
    curr[1].ui32[2] = 0x1000101;
    curr[1].ui32[3] = 0x6C100010;
    curr[2].ui32[0] = 0x10000000;
    curr[2].ui32[1] = 0;
    curr[2].ui32[2] = 0x300006E;
    curr[2].ui32[3] = 0x20001C9;
    curr[3].ui32[0] = 0x70000000;
    curr[3].ui32[1] = 0;
    curr[3].ul64[1] = 0;
    packbuf->curr = curr + 3;
    /* Matching: the #line keeps the assert string below on the original's line. */
#line 593
    assert(packbuf->curr - packbuf->head < SHD_KICK_PACKET_SIZE);
}
