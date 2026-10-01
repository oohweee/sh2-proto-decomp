/* vc_newest.c: newer additions to the view camera (fixed-angle camera warps, circle radius). */
#include "sh2.h"
#include "asm_libm.h"

static const float far_tgt_watch_cir_r[4] = { 1600.0f, 2800.0f, 5000.0f, 6000.0f };

/** End-of-frame reset of the camera work: clears the switch and warp flags. */
void vcEndProcessingVC_WORK(void) {
    vcWork.flags &= ~VC_SWITCH_NEAR_RD_DATA_F;
    vcWork.flags &= ~VC_ALL_WARP_FLAGS;
    vcWork.through_door_activate_init_f = 0;
}

/** End processing of the near road the camera has left: clears its fixed-camera exception.
 * @param old_cur_p the road left (unused)
 * @param w_p the camera work */
void vcEndProcessingOldNearRoad(struct _VC_NEAR_ROAD_DATA *old_cur_p, struct _VC_WORK *w_p) {
    if (w_p->fix_man.exception_f) {
        w_p->fix_man.exception_f = 0;
    }
}

/** Clamps a camera angle offset to the limits of the road's camera type.
 * @param ofs_cam_ang the angle offset (corrected in place)
 * @param rd_data the road (by value) */
void vcCorrectCamMatAngForcibly(float *ofs_cam_ang, struct _VC_ROAD_DATA rd_data) {
    float deflt_lr_lim_ang_y;
    float deflt_rr_lim_ang_y;
    float ofs_lr_lim_ang_y;
    float ofs_rr_lim_ang_y;

    switch (rd_data.cam_mv_type) {
    case 0:
        if (rd_data.flags & 0x200) {
            deflt_lr_lim_ang_y = rd_data.tmp.fix.ofs_hy;
            deflt_rr_lim_ang_y = rd_data.tmp.fix.cam2wth_dist;
            break;
        }
        return;
    case 1:
        deflt_lr_lim_ang_y = rd_data.tmp.fix.ofs_hy;
        deflt_rr_lim_ang_y = rd_data.tmp.fix.cam2wth_dist;
        break;
    default:
        return;
    }
    if (deflt_lr_lim_ang_y == -17.451548f && deflt_rr_lim_ang_y == 17.451548f) {
        return;
    }
    ofs_lr_lim_ang_y = shAngleRegulate(ofs_cam_ang[1] - deflt_lr_lim_ang_y);
    ofs_rr_lim_ang_y = shAngleRegulate(ofs_cam_ang[1] - deflt_rr_lim_ang_y);
    if (ofs_lr_lim_ang_y >= 0.0f && ofs_rr_lim_ang_y <= 0.0f) {
        ofs_cam_ang[1] = shAngleRegulate(ofs_cam_ang[1]);
    } else if (fabsf(ofs_lr_lim_ang_y) < fabsf(ofs_rr_lim_ang_y)) {
        ofs_cam_ang[1] = shAngleRegulate(deflt_lr_lim_ang_y);
    } else {
        ofs_cam_ang[1] = shAngleRegulate(deflt_rr_lim_ang_y);
    }
}

/** On a fixed-angle camera road (kind 1), flips the camera to the other side (a warp) when the
 * character looks more than 135 degrees away from it.
 * @param w_p the camera work */
void vcWarpForFixAngCam(struct _VC_WORK *w_p) {
    float ofs_ang_y;
    float actual_ang_y;

    actual_ang_y = shAngleRegulate(w_p->cur_near_road.road_p->tmp.fix.ang_y + w_p->fix_man.add_ang_y);
    ofs_ang_y = shAngleRegulate(w_p->chara_eye_ang_y - actual_ang_y);
    if (fabsf(ofs_ang_y) >= 2.3561945f && w_p->cur_near_road.road_p->kind_id == 1) {
        vcWork.flags |= VC_ALL_WARP_FLAGS;
        w_p->fix_man.exception_f = w_p->fix_man.exception_f ? 0 : 1;
        if (w_p->fix_man.exception_f) {
            w_p->fix_man.add_ang_y += 3.1415927f;
        } else {
            w_p->fix_man.add_ang_y -= 3.1415927f;
        }
        switch (w_p->cur_near_road.rd_dir_type) {
        case 0:
            if (-1.5707964f <= actual_ang_y && actual_ang_y < 1.5707964f) {
                w_p->fix_man.add_rd_z += 800.0f;
            } else {
                w_p->fix_man.add_rd_z -= 800.0f;
            }
            break;
        case 1:
            if (actual_ang_y > 0.0f) {
                w_p->fix_man.add_rd_x += 800.0f;
            } else {
                w_p->fix_man.add_rd_x -= 800.0f;
            }
            break;
        }
    }
}

/** Returns the radius of the far-target watch circle for the current camera and character
 * positions, reduced as the character faces away from the camera.
 * @param w_p the camera work
 * Matching: the two asm blocks stand for what the original's line table shows as one statement
 * each (an inline helper such as asm_helpers.h's distXZ, and a hypot of x and z). Written that way
 * the structure is the original's but cam2chr_xz_dist lands in f23, not f24 (10 words), so the
 * asm stays in the body. */
float vcRetCirRadiusReduction(struct _VC_WORK *w_p) {
    float real_r;
    float deflt_r;
    float ofs_ang_y;
    float x;
    float cam2chr_xz_dist;
    float z;
    float rate;

    asm {
        lwc1 cam2chr_xz_dist, 0x70(w_p)
        lwc1 $f8, 0x1A0(w_p)
        lwc1 $f9, 0x78(w_p)
        lwc1 $f10, 0x1A8(w_p)
        sub.s cam2chr_xz_dist, cam2chr_xz_dist, $f8
        sub.s $f9, $f9, $f10
        mula.s cam2chr_xz_dist, cam2chr_xz_dist
        madd.s cam2chr_xz_dist, $f9, $f9
        sqrt.s cam2chr_xz_dist, cam2chr_xz_dist
    }
    cam2chr_xz_dist /= 2.0f;
    ofs_ang_y = shAtan2(w_p->chara_pos[2] - w_p->cam_pos[2], w_p->chara_pos[0] - w_p->cam_pos[0]);
    deflt_r = far_tgt_watch_cir_r[w_p->cur_near_road.road_p->area_size_type];
    x = cam2chr_xz_dist * shSinF(ofs_ang_y) + deflt_r * shSinF(w_p->chara_eye_ang_y);
    z = cam2chr_xz_dist * shCosF(ofs_ang_y) + deflt_r * shCosF(w_p->chara_eye_ang_y);
    asm {
        mula.s x, x
        madd.s real_r, z, z
        sqrt.s real_r, real_r
    }
    rate = fabsf(shAngleRegulate(w_p->chara_eye_ang_y - ofs_ang_y) / 3.1415927f);
    rate = 1.0f - rate;
    return real_r * rate;
}

/** On large roads with height types 4 and 5, sets the road's projection from the camera height.
 * @param near_rd_p the near road
 * @param mv_vec_y the camera's vertical offset */
void vcChangeProjByDist(struct _VC_NEAR_ROAD_DATA *near_rd_p, float mv_vec_y) {
    if ((unsigned char)near_rd_p->road_p->area_size_type == 3 &&
        ((unsigned char)near_rd_p->road_p->mv_y_type == 4 || (unsigned char)near_rd_p->road_p->mv_y_type == 5)) {
        near_rd_p->road_p->projection = 448.0f + 1.2f * fabsf(-1375.0f - mv_vec_y);
        vcSetProjectionValue(near_rd_p->road_p->projection, 0);
    }
}
