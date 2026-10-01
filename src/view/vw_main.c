/*
 * vw_main.c: the view point: builds the camera's coordinate system from a reference point,
 * heading, roll, height and distance, and keeps its world position and angles (vwViewPointInfo).
 */
#include "sh2.h"
#include "asm_helpers.h"

struct _VW_VIEW_WORK vwViewPointInfo;

/** Resets the view point (at the origin, looking along +z) and its coordinate system. */
void vwInitViewInfo(void) {
    *(u_long128 *)vwViewPointInfo.rview.vp = 0;
    *(u_long128 *)vwViewPointInfo.rview.vr = 0;
    vwViewPointInfo.rview.vr[2] = 1.0f;
    vwViewPointInfo.rview.rz = 0.0f;
    vwViewPointInfo.rview.super = &vwViewPointInfo.vwcoord;
    vbInitCoordinate(NULL, &vwViewPointInfo.vwcoord);
    vwSetViewInfo();
}

/** Copies the view's world position.
 * @param pos result */
void vwGetViewPosition(float *pos) {
    vcopy(vwViewPointInfo.worldpos, pos);
}

/** Copies the view's world angles.
 * @param ang result */
void vwGetViewAngle(float *ang) {
    vcopy(vwViewPointInfo.worldang, ang);
}

/** Places the view on a cylinder around a reference point ("entou" = cylinder), looking at it.
 * @param parent_p parent coordinate system, or NULL
 * @param ref reference point
 * @param cam_ang_y heading around the point
 * @param cam_ang_z roll
 * @param cam_y height above the point
 * @param cam_xz_r horizontal distance */
void vwSetCoordRefAndEntou(struct _VbCOORDINATE *parent_p, float *ref, float cam_ang_y, float cam_ang_z, float cam_y, float cam_xz_r) {
    float view_ang[4];

    vwViewPointInfo.vwcoord.flg = 0;
    vwViewPointInfo.vwcoord.super = parent_p;
    view_ang[0] = shAtan2(cam_xz_r, -cam_y);
    view_ang[1] = cam_ang_y;
    view_ang[2] = cam_ang_z;
    view_ang[3] = 1.0f;
    view_ang[0] *= -1.0f;
    view_ang[1] += 3.1415927f;
    view_ang[0] = shAngleRegulate(view_ang[0]);
    view_ang[1] = shAngleRegulate(view_ang[1]);
    view_ang[2] = shAngleRegulate(view_ang[2]);
    unitmatrix(vwViewPointInfo.vwcoord.coord);
    vwRotMatrixYXZ(view_ang, vwViewPointInfo.vwcoord.coord);
    vwViewPointInfo.vwcoord.coord[3][0] = ref[0] + cam_xz_r * shSinF(cam_ang_y);
    vwViewPointInfo.vwcoord.coord[3][1] = ref[1] + cam_y;
    vwViewPointInfo.vwcoord.coord[3][2] = ref[2] + cam_xz_r * shCosF(cam_ang_y);
}

/** Applies the view: builds the view matrices and updates the world position and angles. */
void vwSetViewInfo(void) {
    vbSetRefView(&vwViewPointInfo.rview);
    vcopy(vwViewPointInfo.vwcoord.lw[3], vwViewPointInfo.worldpos);
    vwMatrixToAngleYXZ(vwViewPointInfo.worldang, vwViewPointInfo.vwcoord.lw);
}

/** Sets the view's coordinate system from a camera matrix directly.
 * @param pcoord parent coordinate system, or NULL
 * @param cammat the camera matrix */
void vwSetViewInfoDirectMatrix(struct _VbCOORDINATE *pcoord, float (*cammat)[4]) {
    vwViewPointInfo.vwcoord.flg = 0;
    vwViewPointInfo.vwcoord.super = pcoord;
    mcopy(cammat, vwViewPointInfo.vwcoord.coord);
}
