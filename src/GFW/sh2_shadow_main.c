/*
 * Per-frame shadow control (GFW). Stencil shadows (with the shadow filter pass) when the
 * room has shadows, drop shadows otherwise.
 */
#include "sh2.h"

/**
 * Draws the frame's shadows: at night, stencil shadows if the room's shadow density is
 * non-zero (background shadows off where the lighting calls for it), else drop shadows; by
 * day, drop shadows only.
 */
void sh2gfw_ShadowControl_Main(void) {
    int dense;
    struct sh2gfw_CAMERA *cam_tmp;

    cam_tmp = &cam0;
    dense = Get_NowRoomShadowDense();
    if (sh2gfw_Get_NightOrDay()) {
        if (!sh2gfw_Check_JmsSpotOnOff() || !LightSpotOnOffCheck()) {
            if (check_bg_light_exist() == 9) {
                sh2shd_bg_shadow_off();
            }
        }
        if (dense != 0) {
            if (dense == 2 && check_bg_light_exist() == 9) {
                sh2shd_bg_shadow_off();
            }
            shadow_main(paddata2, cam_tmp, b_man[0].Local_World);
        } else {
            drop_shadow_main(paddata2, cam_tmp, 1, 1);
        }
    } else {
        Get_NowRoomShadowDense();
        drop_shadow_main(paddata2, cam_tmp, 1, 1);
    }
}

/**
 * Draws the stencil shadows and applies the shadow filter, with the texture slots locked.
 * @param paddata     pad state (not used)
 * @param cam         the camera
 * @param local_world not used
 */
void shadow_main(unsigned int paddata, struct sh2gfw_CAMERA *cam, float (*local_world)[4]) {
    union Q_WORDDATA *call_addr;
    union Q_WORDDATA *qkick;

    call_addr = stencil_shadow_main(cam);
    if (call_addr != NULL) {
        sh2gfw_Lock_AllTexSlot_For_Shadow();
        sh2gfw_ChangeClear_StencilBuf(&shGs_AllEnv, 0x80, 0x80, 0x80, 0x70);
        sh2gfw_StartShadowEnv(&shGs_AllEnv);
        qkick = sh2gfw_setEND_gsctl();
        d1cSend(qkick);
        d1cSend(call_addr);
        sh2gfw_setREF_TEXFLUSH();
        shdw_shadow_filter_main(1);
        sh2gfw_UnLock_AllTexSlot_For_Shadow();
    }
}

/**
 * Draws the drop (blob) shadows.
 * @param paddata pad state (not used)
 * @param cam     the camera
 * @param arg2    not used
 * @param arg3    not used
 */
void drop_shadow_main(unsigned int paddata, struct sh2gfw_CAMERA *cam, int arg2, int arg3) {
    union Q_WORDDATA *call_addr;

    call_addr = not_stencil_shadow_main(cam);
    if (call_addr != NULL) {
        d1cSend(call_addr);
    }
}
