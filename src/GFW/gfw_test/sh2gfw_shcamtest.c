/*
 * Camera setup for the graphics framework (GFW test code, used by the game): screen
 * parameters and the per-frame camera update.
 */
#include "sh2.h"
#include "sdk/libvu0.h"

/** Sets the screen parameters (VbScreenInfo) from Env_ctl.camera_parms/camera_parms2 and rebuilds
 * the view-screen matrix. */
void sh2gfw_set_ViewScreenM(void) {
    VbScreenInfo.sx = 512.0f * Env_ctl.camera_parms2[0];
    VbScreenInfo.sy = 448.0f * Env_ctl.camera_parms2[1];
    VbScreenInfo.cx = 2048.0f;
    VbScreenInfo.cy = 2048.0f;
    VbScreenInfo.zmin = 2.0f;
    VbScreenInfo.zmax = Env_ctl.camera_parms2[3];
    VbScreenInfo.nearz = Env_ctl.camera_parms[3];
    VbScreenInfo.farz = Env_ctl.camera_parms2[2];
    VbScreenInfo.scr_z = Env_ctl.camera_parms[2];
    vbCalcViewScreenMatrix();
}

/** Initializes the camera: sets the screen parameters. */
void sh2gfw_init_shcamera(void) {
    sh2gfw_set_ViewScreenM();
}

/**
 * Per-frame camera update: depth range, view-screen matrix, camera movement, and the camera
 * position, angles and rotation matrix in Env_ctl.
 */
void sh2gfw_test_shcamera_main(void) {
    if (BgIsOut(0)) {
        VbScreenInfo.zmin = 2.0f;
        VbScreenInfo.zmax = Env_ctl.camera_parms2[3];
        VbScreenInfo.nearz = Env_ctl.camera_parms[3];
        VbScreenInfo.farz = Env_ctl.camera_parms2[2];
    } else {
        VbScreenInfo.zmin = 2.0f;
        VbScreenInfo.zmax = Env_ctl.camera_parms2[3];
        VbScreenInfo.nearz = Env_ctl.camera_parms[3];
        VbScreenInfo.farz = Env_ctl.camera_parms2[2];
    }
    vbCalcViewScreenMatrix();
    vcMoveAndSetCamera(0, 0, 0, 0, 0, 0, 0, 0);
    vwGetViewPosition(Env_ctl.camera_p);
    Env_ctl.camera_p[3] = 0.0f;
    vwGetViewAngle(Env_ctl.camera_rot);
    sceVu0UnitMatrix(Env_ctl.camera_mat);
    vwRotMatrixYXZ(Env_ctl.camera_rot, Env_ctl.camera_mat);
}
