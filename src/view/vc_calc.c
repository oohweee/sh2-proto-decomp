/*
 * vc_calc.c: camera calculation helpers: position ratios inside a road's switch area, the road
 * area transforms, and the conversion of loaded camera files.
 */
/* Matching: the generated header can't express the original's _VC_CIR_CAM_MANAGER: its members
 * were 16-byte aligned vectors (sceVu0FVECTOR), so the by-value copy uses lq/sq. This file
 * declares the function itself, with a local struct that has that alignment
 * (config/prototype_overrides.txt). */
#define SH2_LOCAL_vcRetNearRatioSwitchAreaForCircleCam
#include "sh2.h"
#include "asm_libm.h"
#include "sh_vu0.h"

typedef struct {
    float origin[4] __attribute__((aligned(16)));
    float sw_l[4];
} VC_CIR_CAM_MANAGER_Q;
float vcRetNearRatioSwitchAreaForCircleCam(struct _VC_NEAR_ROAD_DATA cur_near_road, VC_CIR_CAM_MANAGER_Q cir_man, float *chr_pos);

/** Returns how far (0 to 1) the character is through a near road's switch area, along the road's
 * direction and seen from the camera target's side.
 * @param cur_near_road the road (by value)
 * @param chr_pos character position
 * @param cam_tgt_pos camera target position */
float vcRetNearRatioSwitchAreaInXZPos(struct _VC_NEAR_ROAD_DATA cur_near_road, float *chr_pos, float *cam_tgt_pos) {
    float near_ratio;
    float ofs_ang_y;
    float ppos[4];

    near_ratio = 0.0f;
    ofs_ang_y = shAngleRegulate(shAtan2(chr_pos[2] - cam_tgt_pos[2], chr_pos[0] - cam_tgt_pos[0]));
    vcTransRotRoadArea(ppos, cur_near_road.sw_rzm, chr_pos);
    switch (cur_near_road.rd_dir_type) {
    case 0:
        if (0.0f <= ofs_ang_y && ofs_ang_y < 3.1415927f) {
            near_ratio = (cur_near_road.sw.max_hx - ppos[0]) / (cur_near_road.sw.max_hx - cur_near_road.sw.min_hx);
        } else {
            near_ratio = (ppos[0] - cur_near_road.sw.min_hx) / (cur_near_road.sw.max_hx - cur_near_road.sw.min_hx);
        }
        break;
    case 1:
        if (-1.5707964f <= ofs_ang_y && ofs_ang_y < 1.5707964f) {
            near_ratio = (cur_near_road.sw.max_hz - ppos[2]) / (cur_near_road.sw.max_hz - cur_near_road.sw.min_hz);
        } else {
            near_ratio = (ppos[2] - cur_near_road.sw.min_hz) / (cur_near_road.sw.max_hz - cur_near_road.sw.min_hz);
        }
        break;
    }
    return near_ratio;
}

/** vcRetNearRatioSwitchAreaInXZPos for a circle camera: the ratio along x or z from the circle's
 * origin, by the character's angle around it.
 * @param cur_near_road the road (by value)
 * @param cir_man the circle camera (by value)
 * @param chr_pos character position */
float vcRetNearRatioSwitchAreaForCircleCam(struct _VC_NEAR_ROAD_DATA cur_near_road, VC_CIR_CAM_MANAGER_Q cir_man, float *chr_pos) {
    float near_ratio;
    float ofs_ang_y;
    float ppos[4];

    ofs_ang_y = shAngleRegulate(shAtan2(chr_pos[2] - cir_man.origin[2], chr_pos[0] - cir_man.origin[0]));
    vcTransRotRoadArea(ppos, cur_near_road.sw_rzm, chr_pos);
    if ((-2.3561945f <= ofs_ang_y && ofs_ang_y < -0.7853982f) || (0.7853982f <= ofs_ang_y && ofs_ang_y < 2.3561945f)) {
        near_ratio = fabsf((cir_man.origin[0] - chr_pos[0]) / cir_man.sw_l[0]);
    } else {
        near_ratio = fabsf((cir_man.origin[2] - chr_pos[2]) / cir_man.sw_l[2]);
    }
    return near_ratio;
}

/** Transforms a point into a road area's space: v0 = rotation of m x (v1 + m[3]).
 * @param v0 result
 * @param m the area matrix
 * @param v1 the point */
void vcTransRotRoadArea(float *v0, float (*m)[4], float *v1) {
    _shAddVector(v0, v1, m[3]);
    vbApplyMatrixWithoutTr(v0, m, v0);
}

/** Transforms a point out of a road area's space: v0 = rotation of m x v1 + m[3].
 * @param v0 result
 * @param m the area matrix
 * @param v1 the point */
void vcRotTransRoadArea(float *v0, float (*m)[4], float *v1) {
    vbApplyMatrixWithoutTr(v0, m, v1);
    _shAddVector(v0, v0, m[3]);
}

/** Returns the minimum distance inside a road: 500 outdoors, 400 indoors. */
float vcGetMinInRoadDist(void) {
    if (BgIsOut(0)) {
        return 500.0f;
    }
    return 400.0f;
}

/** Converts a loaded camera road file in place: sign-extends the kind ids and replaces the marker
 * values (10000, 10001, ...) of heights and angles with their real values.
 * @param road_ary the roads, ended by VC_RD_END_DATA_F */
void vcConvertCamFile(struct _VC_ROAD_DATA *road_ary) {
    struct _VC_ROAD_DATA *rd_p;

    for (rd_p = road_ary; !(rd_p->flags & VC_RD_END_DATA_F); rd_p++) {
        rd_p->kind_id <<= 16;
        rd_p->kind_id >>= 16;

        if (rd_p->ofs_watch_hy == 10000.0f) {
            rd_p->ofs_watch_hy = -300.0f;
        } else if (rd_p->ofs_watch_hy == 10001.0f) {
            rd_p->ofs_watch_hy = -375.0f;
        } else if (rd_p->ofs_watch_hy == 10002.0f) {
            rd_p->ofs_watch_hy = -600.0f;
        }

        if (!rd_p->projection) {
            rd_p->projection = 448.0f;
        } else if (rd_p->projection == 10001.0f) {
            rd_p->projection = 384.0f;
        }

        if (rd_p->trace_btm_hy == 10000.0f) {
            rd_p->trace_btm_hy = -300.0f;
        } else if (rd_p->trace_btm_hy == 10001.0f) {
            rd_p->trace_btm_hy = -150.0f;
        } else if (rd_p->trace_btm_hy == 10002.0f) {
            rd_p->trace_btm_hy = 150.0f;
        }

        switch (rd_p->cam_mv_type) {
        case 0:
            if (rd_p->tmp.chs.ofs_hy == 10000.0f) {
                rd_p->tmp.chs.ofs_hy = -500.0f;
            }
            if (rd_p->tmp.chs.ofs_hy == 10001.0f) {
                rd_p->tmp.chs.ofs_hy = -125.0f;
            }
            break;
        case 2:
            if (rd_p->tmp.fix.ang_x == 10000.0f) {
                rd_p->tmp.fix.ang_x = -0.5235988f;
            }
            break;
        }
    }
}
