/*
 * vc_util.c: camera start-up and the per-frame camera update: warps, the free camera moved by
 * the pad, and the reference position around the player's head.
 */
#include "sh2.h"
#include "sh_vu0.h"

/* asm_helpers.h's mcopy. Matching: kept local, because including asm_helpers.h moves a float-constant
 * load in vcMoveAndSetCamera (this file's fitted stand-in below depends on the code before it). */
inline void mcopy(float (*s)[4], float (*d)[4]) {
    asm {
        lq t6, 0x0(s)
        lq t7, 0x10(s)
        sq t6, 0x0(d)
        sq t7, 0x10(d)
        lq t6, 0x20(s)
        lq t7, 0x30(s)
        sq t6, 0x20(d)
        sq t7, 0x30(d)
    }
}

struct _VC_CAMERA_INTINFO vcCameraInternalInfo;
float vcRefPosSt[4];

/* v0.xyz = v1.xyz / q, like sh_vu0.h's _shDivVectorXYZ but with its scratch registers listed as
 * clobbers, which changes the caller's register allocation. Matching: kept local for that. */
static inline void divvector(float *v0, float *v1, float q) {
    __asm__ __volatile__("
    lui       $15, 0x3f80
    mtc1      $15, $f8
    div.s     $f8, $f8, %2
    lqc2      vf4, 0x0(%1)
    mfc1      $15, $f8
    qmtc2.ni  $15, vf5
    vmulx.xyz vf4, vf4, vf5x
    sqc2      vf4, 0x0(%0)
    " : : "r"(v0), "r"(v1), "f"(q) : "$15", "$f8");
}

/** Starts the camera system: player data, warp to the player, view info, roads, projection and
 * the free-camera defaults.
 * @param roadarray_list the camera road lists of the stage */
void vcInitCamera(struct _VC_ROAD_DATA **roadarray_list) {
    vcCameraInternalInfo.mode = 0;
    vcCameraInternalInfo.mv_smooth = 0;
    vcCameraInternalInfo.ev_cam_rate = 0.0f;
    vcPreSetCharaDataForCamera();
    vcSetCameraUseWarp(sys.hero.pos, sys.hero.ang[1]);
    vwInitViewInfo();
    vcInitVCSystem(roadarray_list);
    vcStartCameraSystem();
    vcSetProjectionValue(0.0f, 0);
    vcWork.flags |= VC_PROJ_MOMENT_CHANGE_F;
    sys.cam_ang_z = 0.0f;
    sys.cam_r_xz = 1500.0f;
    sys.cam_y = 0.0f;
}

/** Places the camera behind a character at once (a warp).
 * @param chr_pos character position
 * @param chr_ang_y character heading */
void vcSetCameraUseWarp(float *chr_pos, float chr_ang_y) {
    float cam_pos[4];
    float cam_ang[4];

    *(u_long128 *)cam_ang = 0;
    cam_ang[1] = chr_ang_y;
    cam_pos[0] = chr_pos[0] - 795.0f * shSinF(chr_ang_y);
    cam_pos[1] = -900.0f + chr_pos[1];
    cam_pos[2] = chr_pos[2] - 795.0f * shCosF(chr_ang_y);
    cam_pos[3] = 1.0f;
    /* Matching: reconstructed: the original keeps the stores to the unused cam_pos (its address escapes)
     * and has two code-less lines here in its line table; a disabled use of cam_pos, form unknown. */
    if (0) { vwGetViewPosition(cam_pos); }
    vcSetFirstCamWork(chr_pos, chr_ang_y, (Sh2sys.main_status >> 2) & 1);
    Sh2sys.main_status &= ~4;
}

/** Returns whether the last camera move was smooth (no cut). */
int vcRetCamMvSmoothF(void) {
    return vcCameraInternalInfo.mv_smooth;
}

/* Matching: fitted stand-in for float code (docs/stand-ins.md). The -100.0f (cam_y) argument of
 * vwSetCoordRefAndEntou in case 3 is materialized before 2.8797932f and 0.0f only with this much
 * float code compiled earlier (docs/toolchain.md). */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f + 53.0f + 55.0f + 57.0f + 59.0f + 61.0f + 63.0f + 65.0f + 67.0f + 69.0f + 71.0f + 73.0f + 75.0f + 77.0f + 79.0f + 81.0f + 83.0f + 85.0f + 87.0f + 89.0f + 91.0f + 93.0f + 95.0f + 97.0f + 99.0f + 101.0f + 103.0f + 105.0f + 107.0f + 109.0f + 111.0f + 113.0f + 115.0f + 117.0f + 119.0f + 121.0f + 123.0f + 125.0f + 127.0f + 129.0f + 131.0f + 133.0f + 135.0f + 137.0f + 139.0f + 141.0f + 143.0f + 145.0f + 147.0f + 149.0f + 151.0f + 153.0f + 155.0f + 157.0f + 159.0f + 161.0f + 163.0f + 165.0f + 167.0f + 169.0f + 171.0f + 173.0f + 175.0f + 177.0f + 179.0f + 181.0f + 183.0f + 185.0f + 187.0f + 189.0f + 191.0f + 193.0f + 195.0f + 197.0f + 199.0f + 201.0f + 203.0f + 205.0f + 207.0f + 209.0f + 211.0f + 213.0f + 215.0f + 217.0f + 219.0f + 221.0f + 223.0f + 225.0f + 227.0f + 229.0f + 231.0f + 233.0f + 235.0f + 237.0f + 239.0f + 241.0f + 243.0f + 245.0f + 247.0f + 249.0f + 251.0f + 253.0f + 255.0f + 257.0f + 259.0f + 261.0f + 263.0f + 265.0f + 267.0f + 269.0f + 271.0f + 273.0f + 275.0f + 277.0f + 279.0f + 281.0f + 283.0f + 285.0f + 287.0f + 289.0f + 291.0f + 293.0f + 295.0f + 297.0f + 299.0f + 301.0f + 303.0f + 305.0f + 307.0f + 309.0f + 311.0f + 313.0f + 315.0f + 317.0f + 319.0f + 321.0f + 323.0f + 325.0f + 327.0f + 329.0f + 331.0f; } /* fitted, not recovered: 165 constants */
/** Per-frame camera update by mode: 0 follows the player (camera system), 1 and 3 are the free
 * cameras moved by the flags, 2 is moved by the pad; then sets the view.
 * @param in_connect_f non-zero while connecting stages (fixed head position)
 * @param arg1 unused (the DWARF drops it)
 * @param for_f move forward
 * @param back_f move back
 * @param right_f turn right
 * @param left_f turn left
 * @param up_f move up
 * @param down_f move down */
void vcMoveAndSetCamera(int in_connect_f, int arg1, int for_f, int back_f, int right_f, int left_f, int up_f, int down_f) {
    float first_cam_pos[4];
    struct _KANRI *hr_p;
    float hr_head_pos[4];
    float hero_bottom_y;
    float hero_top_y;
    float grnd_y;
    struct _VbCOORDINATE vbcoord;
    float rpos[4];
    /* Matching: called without a prototype, with arguments the (void) definition ignores. */
    void vcSetRefPosAndCamPosAngByPad();

    vcPreSetCharaDataForCamera();
    switch (vcCameraInternalInfo.mode) {
    default:
        vcCameraInternalInfo.mode = 0;
        first_cam_pos[0] = 3500.0f + sys.hero.pos[0];
        first_cam_pos[1] = -1100.0f;
        first_cam_pos[2] = sys.hero.pos[2];
        first_cam_pos[3] = 1.0f;
        vcSetFirstCamWork(first_cam_pos, sys.hero.ang[1], 0);
    case 0:
        hr_p = &sys.hero;
        if (in_connect_f) {
            hr_head_pos[0] = hr_p->pos[0];
            hr_head_pos[1] = -950.0f + hr_p->pos[1];
            hr_head_pos[2] = hr_p->pos[2];
            hr_head_pos[3] = 1.0f;
            grnd_y = -2.0f;
        } else {
            grnd_y = hr_p->pos[1];
            vcMakeHeroHeadPos(hr_head_pos);
        }
        hero_top_y = hr_p->pos[1] - 925.0f;
        hero_bottom_y = hr_p->pos[1] + -250.0f * vcCameraInternalInfo.ev_cam_rate;
        if (vcCameraInternalInfo.ev_cam_rate > 0.0f) {
            vcWork.flags |= VC_INHIBIT_FAR_WATCH_F;
        } else {
            vcWork.flags &= ~VC_INHIBIT_FAR_WATCH_F;
        }
        vcSetSubjChara(hr_p->pos, hero_bottom_y, hero_top_y, grnd_y, hr_head_pos, hr_p->velo_xz, hr_p->velo_houi,
                       hr_p->rot_spd[1], hr_p->ang[1], 2.0943952f, 5500.0f);
        vcCameraInternalInfo.mv_smooth = vcExecCamera();
        break;
    case 1:
        vcSetRefPosAndSysRef2CamParam(vcRefPosSt, &sys, for_f, back_f, right_f, left_f, up_f, down_f);
        vwSetCoordRefAndEntou(NULL, vcRefPosSt, sys.cam_ang_y, sys.cam_ang_z, sys.cam_y, sys.cam_r_xz);
        break;
    case 2:
        vcSetRefPosAndCamPosAngByPad(vcRefPosSt, &sys);
        break;
    case 3:
        vcSetRefPosAndSysRef2CamParam(vcRefPosSt, &sys, for_f, back_f, right_f, left_f, up_f, down_f);
        mcopy(vcPreInfo.hero_neck_wm, vbcoord.workm);
        mcopy(vcPreInfo.hero_neck_lm, vbcoord.coord);
        mcopy(vcPreInfo.hero_neck_lwm, vbcoord.lw);
        asm __volatile__("
        sqc2 vf0, 0x0(%0)
        " : : "r"(rpos));
        rpos[1] = -75.0f;
        rpos[2] = 500.0f;
        vwSetCoordRefAndEntou(&vbcoord, rpos, 2.8797932f, 0.0f, -100.0f, 500.0f);
        break;
    }
    vwSetViewInfo();
}

/** Gets the player's head position from his neck matrix (50 up the neck, then 150 higher).
 * @param head_pos result */
void vcMakeHeroHeadPos(float *head_pos) {
    float neck_lwm[4][4];
    float fpos[4];

    mcopy(vcPreInfo.hero_neck_lwm, neck_lwm);
    *(u_long128 *)fpos = 0;
    fpos[1] = -50.0f;
    vbApplyMatrixWithoutTr(fpos, neck_lwm, fpos);
    _shAddVector(head_pos, fpos, neck_lwm[3]);
    head_pos[1] += -150.0f;
    head_pos[3] = 1.0f;
}

/** out_pos = in_pos moved ofs_xz_r along heading ang_y and ofs_y vertically.
 * @param out_pos result
 * @param in_pos start position
 * @param ofs_xz_r horizontal distance
 * @param ang_y heading
 * @param ofs_y vertical offset */
void vcAddOfsToPos(float *out_pos, float *in_pos, float ofs_xz_r, float ang_y, float ofs_y) {
    out_pos[0] = in_pos[0] + ofs_xz_r * shSinF(ang_y);
    out_pos[2] = in_pos[2] + ofs_xz_r * shCosF(ang_y);
    out_pos[1] = in_pos[1] + ofs_y;
}

/** Free camera: changes its distance, heading and height by the flags and sets the reference
 * point in front of the player.
 * @param ref_pos result: reference position
 * @param sys_p the camera parameters
 * @param for_f closer
 * @param back_f farther
 * @param right_f turn right
 * @param left_f turn left
 * @param up_f up
 * @param down_f down */
void vcSetRefPosAndSysRef2CamParam(float *ref_pos, struct _SYS_W *sys_p, int for_f, int back_f, int right_f, int left_f, int up_f, int down_f) {
    if (for_f) {
        sys_p->cam_r_xz -= 50.0f;
    }
    if (back_f) {
        sys_p->cam_r_xz += 50.0f;
    }
    if (right_f) {
        sys_p->cam_ang_y -= 0.017453292f;
    }
    if (left_f) {
        sys_p->cam_ang_y += 0.017453292f;
    }
    if (up_f) {
        sys_p->cam_y -= 50.0f;
    }
    if (down_f) {
        sys_p->cam_y += 50.0f;
    }
    sys_p->cam_ang_y = shAngleRegulate(sys_p->cam_ang_y);
    if (sys_p->cam_r_xz < 500.0f) {
        sys_p->cam_r_xz = 500.0f;
    }
    vcAddOfsToPos(ref_pos, sys.hero.pos, 250.0f, sys.hero.ang[1], -500.0f);
}

/** Pad camera: reads the view position (divided by 500) and angle into locals; nothing uses them. */
void vcSetRefPosAndCamPosAngByPad(void) {
    float cam_ang[4];
    float cam_pos[4];

    vwGetViewPosition(cam_pos);
    divvector(cam_pos, cam_pos, 500.0f);
    vwGetViewAngle(cam_ang);
}
