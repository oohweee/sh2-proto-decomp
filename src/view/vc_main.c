/*
 * vc_main.c: the view camera system: picks the camera road for the player's position, then
 * each frame moves the camera and its watch target by the road's camera type (normal, fixed
 * angle, circle, through-door, self view, far watch), keeps the player in the picture and
 * hands the result to the view (vw_main.c).
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "sh_vu0.h"
#include "fi_libvu0_inline.h"

/*
 * Degrees to radians. The definition is the original's: this file's asserts stringize their
 * argument after macro expansion (see VC_ASSERT), and the one in vcAdjCamOfsAngByCharaInScreen
 * reads ((3.14159265358979f/180.0f)*(5.0f)) in the binary. The name is invented. Float constants
 * that are bit-exactly DEG2RAD() of a round number of degrees are written with it (180 and 90
 * included, though a pi macro of the same value would give the same constants there), except in
 * vcAdjCamOfsAngByOfsAngSpd's third call and in vcSetDataToVwSystem: written with DEG2RAD() their
 * float-argument order is not the original's (the folded constant takes the float-constant flag
 * of the macro's first operand; docs/toolchain.md, "Root cause"). Neither refitting the stand-ins
 * before vcSetDataToVwSystem (about 4,000 sizes tried) nor respelling the third call's angles or
 * the previous function's statements restored it, so those constants stay literal.
 * Matching: a negative angle is written DEG2RAD(-x) rather than -DEG2RAD(x) (the same constant).
 * Which one the original used is unknown; this one gives vcAdjustWatchYLimitHighWhenFarView's
 * float-constant order (config/order_fits.txt).
 */
#define DEG2RAD(x) ((3.14159265358979f/180.0f)*(x))

/* This file's NULL is 0L: its asserts' text reads `!= 0L` where a pointer is compared with it. */
#undef NULL
#define NULL 0L

/*
 * The asserts of this file stringize their argument after macro expansion (0L and DEG2RAD's
 * expansion in the binary's text), unlike common.h's assert() and assert_dw(), whose text in
 * other files is unexpanded ("Now!=NULL"). The names are invented.
 */
#define VC_ASSERT(x) assert(x)
#define VC_ASSERT_DW(x) assert_dw(x)

typedef enum {
    VC_CHK_RD = 0,
    VC_CHK_SW = 1,
} VC_CHK_TYPE;

typedef enum {
    VC_MAKE_NORMAL = 0,
    VC_MAKE_SELF_VIEW = 1,
} VC_MAKE_TYPE;

static int vcRetSmoothCamMvF(float *old_pos, float *now_pos, float *old_ang, float *now_ang);
static enum _VC_CAM_MV_TYPE vcRetCurCamMvType(struct _VC_WORK *w_p);
static int vcRetThroughDoorCamEndF(struct _VC_WORK *w_p);
static float vcRetFarWatchRate(int far_watch_button_prs_f, enum _VC_CAM_MV_TYPE cur_cam_mv_type, struct _VC_WORK *w_p);
static float vcRetSelfViewEffectRate(enum _VC_CAM_MV_TYPE cur_cam_mv_type, float far_watch_rate, struct _VC_WORK *w_p);
static void vcSetFlagsByCamMvType(enum _VC_CAM_MV_TYPE cam_mv_type, float far_watch_rate, int all_warp_f);
static void vcPreSetDataInVC_WORK(struct _VC_WORK *w_p, struct _VC_ROAD_DATA **vc_road_ary_list);
static void vcSetTHROUGH_DOOR_CAM_PARAM_in_VC_WORK(struct _VC_WORK *w_p, enum _THROUGH_DOOR_SET_CMD_TYPE set_cmd_type);
static void vcSetNearestEnemyDataInVC_WORK(struct _VC_WORK *w_p);
static void vcSetNearestItemDataInVC_WORK(struct _VC_WORK *w_p);
static void vcSetNearRoadAryByCharaPos(struct _VC_WORK *w_p, struct _VC_ROAD_DATA **road_ary_list, float half_w, int near_enemy_f);
static int vcRetRoadUsePriority(enum _VC_ROAD_TYPE rd_type);
static int vcSetCurNearRoadInVC_WORK(struct _VC_WORK *w_p);
static float vcGetBestNewCurNearRoad(struct _VC_NEAR_ROAD_DATA **new_cur_pp, VC_CHK_TYPE chk_type, float *pos, struct _VC_WORK *w_p);
static float vcGetNearestNEAR_ROAD_DATA(struct _VC_NEAR_ROAD_DATA **out_nearest_p_addr, VC_CHK_TYPE chk_type, enum _VC_ROAD_TYPE rd_type, float *pos, struct _VC_WORK *w_p, int chk_only_set_marge_f);
static float vcAdvantageDistOfOldCurRoad(struct _VC_NEAR_ROAD_DATA *old_cur_p);
static void vcAutoRenewalWatchTgtPosAndAngZ(struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type, enum _VC_AREA_SIZE_TYPE cur_rd_area_size, float far_watch_rate, float self_view_eff_rate);
static void vcMakeNormalWatchTgtPos(float *watch_tgt_pos, float *watch_tgt_ang_z_p, struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type, enum _VC_AREA_SIZE_TYPE cur_rd_area_size);
static void vcMixSelfViewEffectToWatchTgtPos(float *watch_tgt_pos, float *watch_tgt_ang_z_p, float effect_rate, struct _VC_WORK *w_p, float (*head_mat)[4], int anim_status);
static void vcMakeFarWatchTgtPos(float *watch_tgt_pos, struct _VC_WORK *w_p, enum _VC_AREA_SIZE_TYPE cur_rd_area_size);
static void vcSetWatchTgtXzPos(float *watch_pos, float *center_pos, float *cam_pos, float tgt_chara2watch_cir_dist, float tgt_watch_cir_r, float watch_cir_ang_y);
static void vcSetWatchTgtYParam(float *watch_pos, struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type, float watch_y);
static void vcAdjustWatchYLimitHighWhenFarView(float *watch_pos, float *cam_pos);
static void vcAutoRenewalCamTgtPos(struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type, struct _VC_CAM_MV_PARAM *cam_mv_prm_p, enum _VC_ROAD_FLAGS cur_rd_flags, enum _VC_AREA_SIZE_TYPE cur_rd_area_size, float far_watch_rate);
static float vcRetMaxTgtMvXzLen(struct _VC_WORK *w_p, struct _VC_CAM_MV_PARAM *cam_mv_prm_p);
static void vcMakeIdealCamPosByHeadPos(float *ideal_pos, struct _VC_WORK *w_p, enum _VC_AREA_SIZE_TYPE cur_rd_area_size);
static void vcMakeIdealCamPosForFixAngCam(float *ideal_pos, struct _VC_WORK *w_p);
static void vcMakeIdealCamPosForThroughDoorCam(float *ideal_pos, struct _VC_WORK *w_p);
static void vcMakeIdealCamPosForLocusCircleCam(float *ideal_pos, struct _VC_WORK *w_p);
static void vcMakeIdealCamPosUseVC_ROAD_DATA(float *ideal_pos, struct _VC_WORK *w_p, enum _VC_AREA_SIZE_TYPE cur_rd_area_size);
static void vcAdjustXzInLimAreaUsingMIN_IN_ROAD_DIST(float *x_p, float *z_p, struct _VC_LIMIT_AREA *lim_p);
static void vcMakeBasicCamTgtMvVec(float *tgt_mv_vec, float *ideal_pos, struct _VC_WORK *w_p, float max_tgt_mv_xz_len);
static void vcAdjTgtMvVecYByCurNearRoad(float *tgt_mv_vec, struct _VC_WORK *w_p);
static void vcCamTgtMvVecIsFlipedFromCharaFront(float *tgt_mv_vec, struct _VC_WORK *w_p, float max_tgt_mv_xz_len, enum _VC_AREA_SIZE_TYPE cur_rd_area_size);
static float vcFlipFromCamExclusionArea(float *flip_ang_y_p, float *old_cam_excl_area_r_p, float *in_pos, float *chara_pos, float chara_eye_ang_y, enum _VC_AREA_SIZE_TYPE cur_rd_area_size);
static void vcGetUseWatchAndCamMvParam(struct _VC_WATCH_MV_PARAM **watch_mv_prm_pp, struct _VC_CAM_MV_PARAM **cam_mv_prm_pp, float self_view_eff_rate, float far_watch_rate, struct _VC_WORK *w_p);
static void vcRenewalCamData(struct _VC_WORK *w_p, struct _VC_CAM_MV_PARAM *cam_mv_prm_p);
static void vcRenewalCamMatAng(struct _VC_WORK *w_p, struct _VC_WATCH_MV_PARAM *watch_mv_prm_p, enum _VC_CAM_MV_TYPE cam_mv_type, int visible_chara_f);
static void vcMakeNewBaseCamAng(float *new_base_ang, enum _VC_CAM_MV_TYPE cam_mv_type, struct _VC_WORK *w_p);
static void vcRenewalBaseCamAngAndAdjustOfsCamAng(struct _VC_WORK *w_p, float *new_base_cam_ang);
static void vcMakeOfsCamTgtAng(float *ofs_tgt_ang, float (*base_matT)[4], struct _VC_WORK *w_p);
static void vcMakeOfsCam2CharaBottomAndTopAngByBaseMatT(float *ofs_cam2chara_btm_ang, float *ofs_cam2chara_top_ang, float (*base_matT)[4], float *cam_pos, float *chara_pos, float chara_bottom_y, float chara_top_y);
static void vcAdjCamOfsAngByCharaInScreen(float *cam_ang, float *ofs_cam2chara_btm_ang, float *ofs_cam2chara_top_ang, struct _VC_WORK *w_p);
static void vcAdjCamOfsAngByOfsAngSpd(float *ofs_ang, float *ofs_ang_spd, float *ofs_tgt_ang, struct _VC_WATCH_MV_PARAM *prm_p);
static void vcMakeCamMatAndCamAngByBaseAngAndOfsAng(float *cam_mat_ang, float (*cam_mat)[4], float *base_cam_ang, float *ofs_cam_ang, float *cam_pos);
static void vcSetDataToVwSystem(struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type);
static float vcCamMatNoise(float noise_w, float ang_spd1, float ang_spd2);
static float vcGetXZSumDistFromLimArea(float *out_vec_x_p, float *out_vec_z_p, float chk_wld_x, float chk_wld_z, float lim_min_x, float lim_max_x, float lim_min_z, float lim_max_z, int can_ret_minus_dist_f);

static int excl_r_ary[9] = { 0x4000, 0x4000, 0x3B33, 0x3333, 0x2B33, 0x24CC, 0x1E66, 0x1800, 0x14CC };
static int mv_stl_ang_ary[5] = { 0x0, 0x430, 0x10C1, 0x6E000, 0x6E000 };
struct _VC_ROAD_DATA vcNullRoadArray[2] = {
    {
        { -2000000.0f, 2000000.0f, 2000000.0f, 2000000.0f, 2000000.0f, -2000000.0f, 2000000.0f, -2000000.0f },
        { -2000000.0f, 2000000.0f, 2000000.0f, 2000000.0f, 2000000.0f, -2000000.0f, 2000000.0f, -2000000.0f },
        0, 0, 2, 0, 3, -300.0f, 0.0f, 0, 448.0f,
    },
    {
        { 0.0f },
        { 0.0f },
        0, 1,
    },
};
struct _VC_ROAD_DATA *vcNullRoadArrayList[2] = { vcNullRoadArray, NULL };
struct _VC_NEAR_ROAD_DATA vcNullNearRoad = {
    vcNullRoadArray, 0, 0, 0.0f, 0.0f, 0.0f,
    { -2000000.0f, 2000000.0f, -2000000.0f, 2000000.0f, 2000000.0f, -2000000.0f },
    { -2000000.0f, 2000000.0f, -2000000.0f, 2000000.0f, 2000000.0f, -2000000.0f },
    { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
    { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

static const struct _VC_WATCH_MV_PARAM watch_mv_prm_user = { 7.539823f, 13.823008f, 1.3823007f, 4.3982296f };
static const struct _VC_WATCH_MV_PARAM watch_mv_prm_nrml = { 6.031858f, 11.058406f, 1.1058406f, 3.5185838f };
static const struct _VC_WATCH_MV_PARAM watch_mv_prm_outdoor = { 8.4823f, 15.550882f, 1.5550883f, 4.9480085f };
static const struct _VC_WATCH_MV_PARAM watch_mv_prm_on_boat = { 2.261947f, 4.1469026f, 1.1058406f, 3.5185838f };
static const struct _VC_WATCH_MV_PARAM self_view_watch_mv_prm = { 10.555752f, 19.35221f, 2.7646015f, 8.796459f };
static const struct _VC_CAM_MV_PARAM cam_mv_prm_user = { 5000.0f, 1500.0f, 3000.0f, 1000.0f };
static const struct _VC_CAM_MV_PARAM cam_mv_prm_nrml = { 4000.0f, 1500.0f, 2400.0f, 1000.0f };
static const struct _VC_CAM_MV_PARAM cam_mv_prm_outdoor = { 5625.0f, 1687.5f, 3375.0f, 1125.0f };
static const struct _VC_CAM_MV_PARAM cam_mv_prm_on_boat = { 1500.0f, 450.0f, 2400.0f, 800.0f };
static const float nml_tgt_watch_cir_r[4] = { 400.0f, 600.0f, 1000.0f, 1200.0f };
static const float nml_cam2wth_min_dist[4] = { 1400.0f, 1800.0f, 2800.0f, 3200.0f };
static const float far_tgt_watch_cir_r[4] = { 1600.0f, 2800.0f, 5000.0f, 6000.0f };
static const float mv_nml_chr2cam_r[4] = { 600.0f, 800.0f, 1000.0f, 1250.0f };
static const float mv_nml_no_adj_max_dist[4] = { 400.0f, 600.0f, 800.0f, 1000.0f };
static const float mv_nml_full_adj_min_dist[4] = { 1200.0f, 1600.0f, 2400.0f, 2800.0f };
static const float extra_boundary_width[4] = { 75.0f, 125.0f, 175.0f, 250.0f };
static const float excl_max_rate[4] = { 0.7f, 0.85f, 0.95f, 1.0f };

struct _SYS_W sys;
struct _VC_WORK vcWork;
float vcSelfViewTimer;
struct _VC_WATCH_MV_PARAM vcWatchMvPrmSt;
struct _VC_CAM_MV_PARAM vcCamMvPrmSt;
struct VC_PROJECTION_PARAM vcProjectionParam;

/* max.s / min.s as GCC-style asm. Matching: asm_libm.h's fmaxf/fminf are native asm blocks, which MWCC
 * treats differently (docs/decomp-workflow.md), so these keep their own names. */
inline float fmaxf_gcc(float a, float b) {
    __asm__ __volatile__("
    max.s %0, %1, %2
    " : "=f"(a) : "f"(a), "f"(b));
    return a;
}

inline float fminf_gcc(float a, float b) {
    __asm__ __volatile__("
    min.s %0, %1, %2
    " : "=f"(a) : "f"(a), "f"(b));
    return a;
}

/* Squared XZ distance between two points (FPU asm). */
static inline float vcSquareDistXZ(float *v0, float *v1) {
    float r;

    __asm__ __volatile__("
    lwc1    %0, 0x0(%1)
    lwc1    $f8, 0x0(%2)
    lwc1    $f9, 0x8(%1)
    lwc1    $f10, 0x8(%2)
    sub.s   %0, %0, $f8
    sub.s   $f9, $f9, $f10
    mula.s  %0, %0
    madd.s  %0, $f9, $f9
    " : "=f"(r) : "r"(v0), "r"(v1) : "$f8", "$f9", "$f10");
    return r;
}

/* x * x in C (asm_helpers.h's sqr is an in-place mul.s asm block, which this file doesn't use). */
inline float sqr_c(float v) {
    v *= v;
    return v;
}

/** Initializes the camera system with the stage's camera roads (the null road list when NULL)
 * and resets its state and movement parameters.
 * @param vc_road_ary_list the road lists, or NULL */
void vcInitVCSystem(struct _VC_ROAD_DATA **vc_road_ary_list) {
    vcWork.view_cam_active_f = 0;
    if (vc_road_ary_list == NULL) {
        vcWork.vc_road_ary_list = vcNullRoadArrayList;
    } else {
        vcWork.vc_road_ary_list = vc_road_ary_list;
    }
    vcWork.cur_near_road = vcNullNearRoad;
    vcWork.old_cam_excl_area_r = -1.0f;
    vcWork.watch_tgt_max_y = 50000.0f;
    vcSelfViewTimer = 0.0f;
    vcWatchMvPrmSt.ang_accel_x = 0.0f;
    vcWatchMvPrmSt.ang_accel_y = 0.0f;
    vcWatchMvPrmSt.max_ang_spd_x = 0.0f;
    vcWatchMvPrmSt.max_ang_spd_y = 0.0f;
    vcCamMvPrmSt.accel_xz = 0.0f;
    vcCamMvPrmSt.accel_y = 0.0f;
    vcCamMvPrmSt.max_spd_xz = 0.0f;
    vcCamMvPrmSt.max_spd_y = 0.0f;
}

/** Activates the camera system (vcExecCamera does nothing until then). */
void vcStartCameraSystem(void) {
    vcWork.view_cam_active_f = 1;
}

/** Places the camera and its target at a position at once, facing the character.
 * @param cam_pos camera position
 * @param chara_eye_ang_y the character's heading
 * @param use_through_door_cam_f non-zero to start with the through-door camera */
void vcSetFirstCamWork(float *cam_pos, float chara_eye_ang_y, int use_through_door_cam_f) {
    *(u_long128 *)vcWork.ofs_cam_ang_spd = 0;
    vcWork.flags = VC_INIT_FLAGS;
    vcopy(cam_pos, vcWork.cam_pos);
    vcWork.cam_mv_ang_y = 0.0f;
    *(u_long128 *)vcWork.cam_velo = 0;
    vcopy(cam_pos, vcWork.cam_tgt_pos);
    *(u_long128 *)vcWork.cam_tgt_velo = 0;
    vcWork.cam_mv_ang_y = 0.0f;
    vcWork.cam_tgt_spd = 0.0f;
    vcWork.cam_chara2ideal_ang_y = shAngleRegulate(DEG2RAD(180.0f) + chara_eye_ang_y);
    vcWork.cur_near_road = vcNullNearRoad;
    vcWork.through_door_activate_init_f = use_through_door_cam_f;
    vcSetTHROUGH_DOOR_CAM_PARAM_in_VC_WORK(&vcWork, 1);
}

/** Makes the camera look at a given point (user watch mode).
 * @param watch_tgt_pos the point
 * @param watch_prm_p movement parameters, or NULL for the default user ones
 * @param rot_z roll
 * @param warp_watch_f non-zero to turn at once */
void vcUserWatchTarget(float *watch_tgt_pos, struct _VC_WATCH_MV_PARAM *watch_prm_p, float rot_z, int warp_watch_f) {
    vcWork.flags &= ~VC_ALL_WATCH_FLAGS;
    vcWork.flags |= VC_USER_WATCH_F;
    if (warp_watch_f) {
        vcWork.flags |= VC_WARP_WATCH_F;
    }
    vcopy(watch_tgt_pos, vcWork.watch_tgt_pos);
    vcWork.watch_tgt_ang_z = rot_z;
    if (watch_prm_p == NULL) {
        vcWork.user_watch_mv_prm = watch_mv_prm_user;
    } else {
        vcWork.user_watch_mv_prm = *watch_prm_p;
    }
}

/** Moves the camera to a given point (user camera mode).
 * @param cam_tgt_pos the point
 * @param cam_prm_p movement parameters, or NULL for the default user ones
 * @param warp_cam_f non-zero to move at once */
void vcUserCamTarget(float *cam_tgt_pos, struct _VC_CAM_MV_PARAM *cam_prm_p, int warp_cam_f) {
    vcWork.flags &= ~VC_ALL_CAM_FLAGS;
    vcWork.flags |= VC_USER_CAM_F;
    if (warp_cam_f) {
        vcWork.flags |= VC_WARP_CAM_F;
    }
    vcopy(cam_tgt_pos, vcWork.cam_tgt_pos);
    if (cam_prm_p == NULL) {
        vcWork.user_cam_mv_prm = cam_mv_prm_user;
    } else {
        vcWork.user_cam_mv_prm = *cam_prm_p;
    }
}

/** Gets the point the camera is looking at: along its current angles, at the watch target's
 * distance.
 * @param watch_pos result */
void vcGetNowWatchPos(float *watch_pos) {
    float r;
    float cos_x;
    float sin_x;
    float cos_y;
    float sin_y;

    cos_x = shCosF(vcWork.cam_mat_ang[0]);
    sin_x = shSinF(vcWork.cam_mat_ang[0]);
    cos_y = shCosF(vcWork.cam_mat_ang[1]);
    sin_y = shSinF(vcWork.cam_mat_ang[1]);
    r = distXYZ(vcWork.watch_tgt_pos, vcWork.cam_pos);
    watch_pos[0] = vcWork.cam_pos[0] + cos_x * (r * sin_y);
    watch_pos[2] = vcWork.cam_pos[2] + cos_x * (r * cos_y);
    watch_pos[1] = vcWork.cam_pos[1] - r * sin_x;
}

/** Copies the camera position.
 * @param cam_pos result */
void vcGetNowCamPos(float *cam_pos) {
    vcopy(vcWork.cam_pos, cam_pos);
}

/** Leaves the user camera modes and restores the road's projection.
 * @param warp_f non-zero to return with a warp (cut) */
void vcReturnPreAutoCamWork(int warp_f) {
    if (warp_f) {
        vcWork.flags |= VC_ALL_WARP_FLAGS;
    }
    vcWork.flags &= ~VC_USER_MODE_FLAGS;
    vcSetProjectionValue(vcWork.cur_near_road.road_p->projection, 0);
}

/** Sets the character the camera follows and its state for this frame.
 * @param chara_pos position
 * @param chara_bottom_y bottom height
 * @param chara_top_y top height
 * @param chara_grnd_y ground height
 * @param chara_head_pos head position
 * @param chara_mv_spd movement speed
 * @param chara_mv_ang_y movement direction
 * @param chara_ang_spd_y turning speed
 * @param chara_eye_ang_y heading
 * @param chara_eye_ang_wy view angle width
 * @param chara_watch_xz_r watch distance */
void vcSetSubjChara(float *chara_pos, float chara_bottom_y, float chara_top_y, float chara_grnd_y, float *chara_head_pos, float chara_mv_spd, float chara_mv_ang_y, float chara_ang_spd_y, float chara_eye_ang_y, float chara_eye_ang_wy, float chara_watch_xz_r) {
    vcopy(chara_pos, vcWork.chara_pos);
    vcWork.chara_bottom_y = chara_bottom_y;
    vcWork.chara_top_y = chara_top_y;
    vcWork.chara_center_y = (chara_bottom_y + chara_top_y) / 2.0f;
    vcWork.chara_grnd_y = chara_grnd_y;
    vcopy(chara_head_pos, vcWork.chara_head_pos);
    vcWork.chara_mv_spd = chara_mv_spd;
    vcWork.chara_mv_ang_y = chara_mv_ang_y;
    vcWork.chara_ang_spd_y = chara_ang_spd_y;
    vcWork.chara_eye_ang_y = chara_eye_ang_y;
    vcWork.chara_eye_ang_wy = chara_eye_ang_wy;
    vcWork.chara_watch_xz_r = chara_watch_xz_r;
}

/** Runs the camera for one frame: picks the road, the camera type and its parameters, moves the
 * camera and its watch target, and hands the result to the view.
 * @return non-zero if the camera moved smoothly (no cut) */
int vcExecCamera(void) {
    enum _VC_ROAD_FLAGS cur_rd_flags;
    enum _VC_CAM_MV_TYPE cur_cam_mv_type;
    enum _VC_AREA_SIZE_TYPE cur_rd_area_size;
    float far_watch_rate;
    float self_view_eff_rate;
    float sv_old_cam_pos[4];
    float sv_old_cam_mat_ang[4];
    int warp_f;
    struct _VC_WATCH_MV_PARAM *watch_mv_prm_p;
    struct _VC_CAM_MV_PARAM *cam_mv_prm_p;
    int smooth_f;

    vcopy(vcWork.cam_pos, sv_old_cam_pos);
    vcopy(vcWork.cam_mat_ang, sv_old_cam_mat_ang);
    if (!vcWork.view_cam_active_f) {
        return 0;
    }
    vcSetAllNpcDeadTimer();
    vcPreSetDataInVC_WORK(&vcWork, vcWork.vc_road_ary_list);
    warp_f = vcSetCurNearRoadInVC_WORK(&vcWork);
    /* Matching: the assert bakes its original line number into the object. */
#line 862
    VC_ASSERT(vcWork.cur_near_road.road_p != NULL);
    cur_rd_flags = vcWork.cur_near_road.road_p->flags;
    cur_rd_area_size = vcWork.cur_near_road.road_p->area_size_type;
    cur_cam_mv_type = vcRetCurCamMvType(&vcWork);
    far_watch_rate = vcRetFarWatchRate(vcWork.flags & VC_PRS_F_VIEW_F, cur_cam_mv_type, &vcWork);
    self_view_eff_rate = vcRetSelfViewEffectRate(cur_cam_mv_type, far_watch_rate, &vcWork);
    if (!(vcWork.flags & VC_USER_MODE_FLAGS)) {
        vcSetFlagsByCamMvType(cur_cam_mv_type, far_watch_rate, warp_f);
    }
    if (vcWork.flags & VC_WARP_CAM_TGT_F) {
        vcWork.old_cam_excl_area_r = -1.0f;
    }
    vcGetUseWatchAndCamMvParam(&watch_mv_prm_p, &cam_mv_prm_p, self_view_eff_rate, far_watch_rate, &vcWork);
    if (!(vcWork.flags & VC_USER_CAM_F)) {
        vcAutoRenewalCamTgtPos(&vcWork, cur_cam_mv_type, cam_mv_prm_p, cur_rd_flags, cur_rd_area_size, far_watch_rate);
    }
    vcRenewalCamData(&vcWork, cam_mv_prm_p);
    if (!(vcWork.flags & VC_USER_CAM_F)) {
        vcChangeProjectionValue(&vcWork);
    }
    if (!(vcWork.flags & VC_USER_WATCH_F)) {
        vcAutoRenewalWatchTgtPosAndAngZ(&vcWork, cur_cam_mv_type, cur_rd_area_size, far_watch_rate, self_view_eff_rate);
        if ((vcWork.cur_near_road.road_p->flags & 0x10) &&
            (vcWork.cur_near_road.road_p->cam_mv_type == 0 || cur_cam_mv_type == VC_MV_SELF_VIEW)) {
            vcAdjustWatchYLimitHighWhenFarView(vcWork.watch_tgt_pos, vcWork.cam_pos);
        }
    }
    vcRenewalCamMatAng(&vcWork, watch_mv_prm_p, cur_cam_mv_type, vcWork.flags & VC_VISIBLE_CHARA_F);
    vcSetDataToVwSystem(&vcWork, cur_cam_mv_type);
    vcEndProcessingVC_WORK();
    smooth_f = vcRetSmoothCamMvF(sv_old_cam_pos, vcWork.cam_pos, sv_old_cam_mat_ang, vcWork.cam_mat_ang);
    return smooth_f;
}

/** Counts up (to 10 s) the dead time of the characters near the camera, resetting it for the
 * living ones. */
void vcSetAllNpcDeadTimer(void) {
    struct SubCharacter *sc_p;
    int i;

    for (i = 0; i < 20; i++) {
        sc_p = shCameraGetNearTarget(i, 0);
        if (sc_p) {
            if (sc_p->battle.status & 2) {
                sc_p->battle.dead_timer += shGetDT();
            } else {
                sc_p->battle.dead_timer = 0.0f;
            }
            if (sc_p->battle.dead_timer > 10.0f) {
                sc_p->battle.dead_timer = 10.0f;
            }
        }
    }
}

static int vcRetSmoothCamMvF(float *old_pos, float *now_pos, float *old_ang, float *now_ang) {
    float mv_vec;
    float rot_x;
    float rot_y;
    int intrpt;

    intrpt = shGetDF();
    intrpt = intrpt < 1.0f ? 1.0f : (intrpt > 4.0f ? 4.0f : intrpt);
    mv_vec = distXYZ(old_pos, now_pos) / intrpt;
    if (mv_vec > 100.0f) {
        return 0;
    }
    rot_x = fabsf(now_ang[0] - old_ang[0]) / intrpt;
    if (rot_x > DEG2RAD(20.0f)) {
        return 0;
    }
    rot_y = fabsf(shAngleRegulate(now_ang[1] - old_ang[1])) / intrpt;
    rot_y *= shCosF(now_ang[0]);
    if (rot_y > DEG2RAD(30.0f)) {
        return 0;
    }
    return 1;
}

static enum _VC_CAM_MV_TYPE vcRetCurCamMvType(struct _VC_WORK *w_p) {
    if (w_p->through_door.active_f) {
        if (!vcRetThroughDoorCamEndF(w_p)) {
            return VC_MV_THROUGH_DOOR;
        }
        vcSetTHROUGH_DOOR_CAM_PARAM_in_VC_WORK(w_p, VC_TDSC_END);
    }
    return w_p->cur_near_road.road_p->cam_mv_type;
}

static int vcRetThroughDoorCamEndF(struct _VC_WORK *w_p) {
    struct _VC_THROUGH_DOOR_CAM_PARAM *prm_p;
    float rail2chara_dist;
    float abs_ofs_ang_y;

    prm_p = &w_p->through_door;
    rail2chara_dist = prm_p->rail_sta_to_chara_dist;
    if (!prm_p->active_f) {
        return 1;
    }
    if (prm_p->timer > 1.2f && w_p->nearest_enemy_xz_dist < 600.0f) {
        return 1;
    }
    if (rail2chara_dist > 800.0f) {
        return 1;
    }
    if (rail2chara_dist > 250.0f) {
        abs_ofs_ang_y = fabsf(shAngleRegulate(w_p->chara_eye_ang_y - shAtan2(w_p->chara_pos[2] - w_p->through_door.rail_sta_pos[2], w_p->chara_pos[0] - w_p->through_door.rail_sta_pos[0])));
        if (abs_ofs_ang_y > DEG2RAD(70.0f)) {
            return 1;
        }
    }
    return 0;
}

static float vcRetFarWatchRate(int far_watch_button_prs_f, enum _VC_CAM_MV_TYPE cur_cam_mv_type, struct _VC_WORK *w_p) {
    float far_watch_rate;
    float dist;
    float abs_ofs_ang_y;

    if ((vcWork.flags & (VC_USER_WATCH_F | VC_INHIBIT_FAR_WATCH_F)) | (w_p->cur_near_road.road_p->flags & 0x800)) {
        far_watch_rate = 0.0f;
    } else {
        switch (cur_cam_mv_type) {
        case VC_MV_FIX_ANG:
            far_watch_rate = 0.0f;
            if (far_watch_button_prs_f && !BgIsOut(0)) {
                vcWarpForFixAngCam(w_p);
            }
            break;
        case VC_MV_SELF_VIEW:
            far_watch_rate = 0.0f;
            break;
        case VC_MV_CHASE:
        case VC_MV_SETTLE:
        case VC_MV_LOCUS_CIRCLE:
            if (far_watch_button_prs_f) {
                far_watch_rate = 1.0f;
            } else {
                far_watch_rate = 0.0f;
            }
            break;
        case VC_MV_THROUGH_DOOR:
            if (far_watch_button_prs_f) {
                far_watch_rate = 1.0f;
            } else {
                dist = w_p->through_door.rail_sta_to_chara_dist;
                far_watch_rate = 0.9f - 0.9f * dist / 800.0f;
                far_watch_rate = fmaxf_gcc(far_watch_rate, 0.0f);
                if (dist > 250.0f) {
                    abs_ofs_ang_y = fabsf(shAngleRegulate(w_p->chara_eye_ang_y - shAtan2(w_p->chara_pos[2] - w_p->through_door.rail_sta_pos[2], w_p->chara_pos[0] - w_p->through_door.rail_sta_pos[0])));
                    far_watch_rate = far_watch_rate * (DEG2RAD(70.0f) - abs_ofs_ang_y) / DEG2RAD(70.0f);
                    far_watch_rate = fmaxf_gcc(far_watch_rate, 0.0f);
                }
            }
            break;
        default:
            /* Matching: the assert bakes its original line number into the object. */
#line 1241
            VC_ASSERT(0);
        }
    }
    return far_watch_rate;
}

/* far_watch_rate is unused (vcExecCamera passes it; the DWARF drops it). */
static float vcRetSelfViewEffectRate(enum _VC_CAM_MV_TYPE cur_cam_mv_type, float far_watch_rate, struct _VC_WORK *w_p) {
    float ret_eff_rate;
    float xyz_dist;
    float max_rate;
    float mul_rate;

    if (!(cur_cam_mv_type == VC_MV_THROUGH_DOOR || cur_cam_mv_type == VC_MV_SELF_VIEW)) {
        return 0.0f;
    }
    if (cur_cam_mv_type == VC_MV_SELF_VIEW) {
        max_rate = 1.0f;
    } else {
        max_rate = 0.35f;
    }
    xyz_dist = distXYZ(w_p->chara_head_pos, w_p->cam_pos);
    if (xyz_dist < 250.0f) {
        ret_eff_rate = max_rate;
    } else if (xyz_dist > 600.0f) {
        ret_eff_rate = 0.0f;
    } else {
        ret_eff_rate = max_rate * (600.0f - xyz_dist) / 350.0f;
    }
    if (w_p->nearest_enemy_xz_dist > 2000.0f) {
        mul_rate = 1.0f;
    } else if (w_p->nearest_enemy_xz_dist < 1000.0f) {
        mul_rate = 0.0f;
    } else {
        mul_rate = (w_p->nearest_enemy_xz_dist - 1000.0f) / 1000.0f;
    }
    ret_eff_rate *= mul_rate;
    ret_eff_rate = ret_eff_rate < 0.0f ? 0.0f : (ret_eff_rate > max_rate ? max_rate : ret_eff_rate);
    return ret_eff_rate;
}

static void vcSetFlagsByCamMvType(enum _VC_CAM_MV_TYPE cam_mv_type, float far_watch_rate, int all_warp_f) {
    if (far_watch_rate != 0.0f) {
        vcWork.flags &= ~VC_VISIBLE_CHARA_F;
    } else {
        switch (cam_mv_type) {
        case VC_MV_CHASE:
        case VC_MV_SETTLE:
        case VC_MV_LOCUS_CIRCLE:
            vcWork.flags |= VC_VISIBLE_CHARA_F;
            break;
        case VC_MV_FIX_ANG:
        case VC_MV_SELF_VIEW:
        case VC_MV_THROUGH_DOOR:
            vcWork.flags &= ~VC_VISIBLE_CHARA_F;
            break;
        default:
            /* Matching: the assert bakes its original line number into the object. */
#line 1379
            VC_ASSERT(0);
        }
    }
    if (cam_mv_type == VC_MV_SELF_VIEW) {
        vcWork.flags |= VC_WARP_CAM_F | VC_WARP_CAM_TGT_F;
    }
    if (all_warp_f) {
        vcWork.flags |= VC_ALL_WARP_FLAGS;
    }
    if (far_watch_rate != 0.0f) {
        vcWork.flags |= VC_WARP_CAM_TGT_F;
    }
}

static void vcPreSetDataInVC_WORK(struct _VC_WORK *w_p, struct _VC_ROAD_DATA **vc_road_ary_list) {
    if (shGetDT() != 0.0f) {
        if (vcWork.flags & VC_PRS_F_VIEW_F) {
            vcWork.flags |= VC_OLD_PRS_F_VIEW_F;
        } else {
            vcWork.flags &= ~VC_OLD_PRS_F_VIEW_F;
        }
        if (PlayerSearchVIewButtonCheck()) {
            vcWork.flags |= VC_PRS_F_VIEW_F;
        } else {
            vcWork.flags &= ~VC_PRS_F_VIEW_F;
        }
    }
    vcWork.scr_half_ang_wx = shAtan2(VbScreenInfo.scr_z, VbScreenInfo.sx) / 2.0f;
    vcWork.scr_half_ang_wy = shAtan2(VbScreenInfo.scr_z, VbScreenInfo.sy) / 2.0f;
    if (vcWork.through_door_activate_init_f) {
        vcSetTHROUGH_DOOR_CAM_PARAM_in_VC_WORK(&vcWork, VC_TDSC_START);
    }
    vcSetTHROUGH_DOOR_CAM_PARAM_in_VC_WORK(&vcWork, VC_TDSC_MAIN);
    vcSetNearestEnemyDataInVC_WORK(w_p);
    if (w_p->nearest_enemy_p == NULL) {
        vcSetNearestItemDataInVC_WORK(w_p);
    }
    vcSetNearRoadAryByCharaPos(w_p, vc_road_ary_list, 10000.0f, w_p->nearest_enemy_p != NULL);
    if (BgIsOut(0) || stage->glb_crd == 5) {
        w_p->ideal_cam_pos_h = -450.0f + vcWork.chara_top_y;
    } else {
        w_p->ideal_cam_pos_h = -200.0f + vcWork.chara_top_y;
    }
}

static void vcSetTHROUGH_DOOR_CAM_PARAM_in_VC_WORK(struct _VC_WORK *w_p, enum _THROUGH_DOOR_SET_CMD_TYPE set_cmd_type) {
    struct _VC_THROUGH_DOOR_CAM_PARAM *prm_p;

    prm_p = &w_p->through_door;
    switch (set_cmd_type) {
    case VC_TDSC_START:
        prm_p->active_f = 1;
        prm_p->timer = 0.0f;
        prm_p->rail_ang_y = w_p->chara_eye_ang_y;
        vcopy3(w_p->chara_pos, prm_p->rail_sta_pos);
        prm_p->rail_sta_pos[1] = w_p->ideal_cam_pos_h;
        break;
    case VC_TDSC_END:
        prm_p->active_f = 0;
        prm_p->timer = 0.0f;
        break;
    case VC_TDSC_MAIN:
        if (prm_p->active_f) {
            prm_p->rail_sta_to_chara_dist = distXZ(w_p->through_door.rail_sta_pos, w_p->chara_pos);
            prm_p->timer += shGetDT();
            if (prm_p->timer <= 0.75f) {
                prm_p->rail_sta_pos[1] += -250.0f * shGetDT();
            }
        }
        break;
    }
}

static void vcSetNearestEnemyDataInVC_WORK(struct _VC_WORK *w_p) {
    struct SubCharacter *sc_p;
    struct SubCharacter *all_min_sc_p;
    struct SubCharacter *active_min_sc_p;
    float all_min_dist;
    float active_min_dist;
    int i;
    float ofs_xz[4];
    float xz_dist;
    int adv_sc_f;
    int set_active_data_f;

    all_min_sc_p = NULL;
    active_min_sc_p = NULL;
    all_min_dist = active_min_dist = 7500.0f;
    if (sh2jms.player->battle.status & 0x10) {
        w_p->nearest_enemy_p = NULL;
        w_p->nearest_enemy_xz_dist = 7500.0f;
        return;
    }
    for (i = 0; i < 20; i++) {
        sc_p = shCameraGetNearTarget(i, 0);
        if (sc_p == NULL) {
            break;
        }
        if ((sc_p->kind >> 8) == 2 && (sc_p->battle.dead_timer <= 1.5f || !(sc_p->battle.status & 2)) && !(sc_p->battle.status & 0x20)) {
            ofs_xz[0] = sc_p->pos.x - w_p->chara_pos[0];
            ofs_xz[2] = sc_p->pos.z - w_p->chara_pos[2];
            if (fabsf(ofs_xz[0]) < 7500.0f && fabsf(ofs_xz[2]) < 7500.0f) {
                xz_dist = lengthXZ(ofs_xz);
                if (xz_dist < all_min_dist) {
                    all_min_dist = xz_dist;
                    all_min_sc_p = sc_p;
                }
                if (!(sc_p->battle.status & 4)) {
                    set_active_data_f = 1;
                } else if (sc_p == (struct SubCharacter *)&sh2jms.target && sh2jms.hold_type != -1) {
                    set_active_data_f = 1;
                } else {
                    set_active_data_f = 0;
                }
                if (set_active_data_f && xz_dist < active_min_dist) {
                    active_min_dist = xz_dist;
                    active_min_sc_p = sc_p;
                }
            }
        }
    }
    if (active_min_sc_p) {
        w_p->nearest_enemy_p = active_min_sc_p;
        w_p->nearest_enemy_xz_dist = active_min_dist;
    } else {
        w_p->nearest_enemy_p = all_min_sc_p;
        w_p->nearest_enemy_xz_dist = all_min_dist;
    }
}

static void vcSetNearestItemDataInVC_WORK(struct _VC_WORK *w_p) {
    struct SubCharacter *sc_p;
    struct SubCharacter *all_min_sc_p;
    float all_min_dist;
    int i;
    float ofs_xz[4];
    int adv_sc_f;
    float xz_dist;

    all_min_sc_p = NULL;
    all_min_dist = 5000.0f;
    if (sh2jms.player->battle.status & 0x20) {
        w_p->nearest_item_p = NULL;
        w_p->nearest_item_xz_dist = 5000.0f;
        return;
    }
    for (i = 0; i < 20; i++) {
        sc_p = shCameraGetNearTarget(i, 1);
        if (sc_p == NULL) {
            break;
        }
        ofs_xz[0] = sc_p->pos.x - w_p->chara_pos[0];
        ofs_xz[2] = sc_p->pos.z - w_p->chara_pos[2];
        if (fabsf(ofs_xz[0]) < 5000.0f && fabsf(ofs_xz[2]) < 5000.0f) {
            xz_dist = lengthXZ(ofs_xz);
            if (xz_dist < all_min_dist) {
                all_min_dist = xz_dist;
                all_min_sc_p = sc_p;
            }
        }
    }
    w_p->nearest_enemy_p = all_min_sc_p;
    w_p->nearest_enemy_xz_dist = all_min_dist;
}

static void vcSetNearRoadAryByCharaPos(struct _VC_WORK *w_p, struct _VC_ROAD_DATA **road_ary_list, float half_w, int near_enemy_f) {
    struct _VC_ROAD_DATA *road_ary;
    struct _VC_ROAD_DATA *rd_p;
    float get_min_x;
    float get_max_x;
    float get_min_z;
    float get_max_z;
    float tmpvec0[4];
    float ppos[4];
    struct _VC_LIMIT_AREA rd;
    struct _VC_LIMIT_AREA sw;
    float rdrot;
    float swrot;
    float rd_rm[4][4];
    float sw_rm[4][4];
    float sw_min_x;
    float sw_max_x;
    float sw_min_z;
    float sw_max_z;
    float rd_min_x;
    float rd_max_x;
    float rd_min_z;
    float rd_max_z;
    float xl;
    float zl;
    float tmpvec1[4];
    float tmpvec2[4];
    struct _VC_NEAR_ROAD_DATA *near_rd_p;
    float add;

    _sceVu0UnitVector(tmpvec0);
    w_p->near_road_suu = 0;
    while ((road_ary = *road_ary_list++) != NULL) {
        for (rd_p = road_ary; !(rd_p->flags & 1); rd_p++) {
            if (near_enemy_f) {
                if (rd_p->flags & 0x20) {
                    continue;
                }
            } else {
                if (rd_p->flags & 0x40) {
                    continue;
                }
            }
            xl = rd_p->lim_rd.x0 - rd_p->lim_rd.x1;
            zl = rd_p->lim_rd.z0 - rd_p->lim_rd.z1;
            rdrot = shAtan2(zl, xl);
            unitmatrix(rd_rm);
            shRotMatrixY(rd_rm, rd_rm, shAngleRegulate(-rdrot));
            rd_rm[3][0] = rd_p->lim_rd.x1;
            rd_rm[3][2] = rd_p->lim_rd.z1;
            tmpvec0[0] = rd_p->lim_rd.x0;
            tmpvec0[2] = rd_p->lim_rd.z0;
            vcTransRotRoadArea(tmpvec1, rd_rm, tmpvec0);
            tmpvec0[0] = rd_p->lim_rd.x2;
            tmpvec0[2] = rd_p->lim_rd.z2;
            vcTransRotRoadArea(tmpvec2, rd_rm, tmpvec0);
            if (tmpvec1[0] > tmpvec2[0]) {
                rd.min_hx = tmpvec2[0];
                rd.max_hx = tmpvec1[0];
            } else {
                rd.min_hx = tmpvec1[0];
                rd.max_hx = tmpvec2[0];
            }
            if (tmpvec1[2] > tmpvec2[2]) {
                rd.min_hz = tmpvec2[2];
                rd.max_hz = tmpvec1[2];
            } else {
                rd.min_hz = tmpvec1[2];
                rd.max_hz = tmpvec2[2];
            }
            xl = rd_p->lim_sw.x0 - rd_p->lim_sw.x1;
            zl = rd_p->lim_sw.z0 - rd_p->lim_sw.z1;
            swrot = shAtan2(zl, xl);
            unitmatrix(sw_rm);
            shRotMatrixY(sw_rm, sw_rm, shAngleRegulate(-swrot));
            sw_rm[3][0] = rd_p->lim_sw.x1;
            sw_rm[3][2] = rd_p->lim_sw.z1;
            tmpvec0[0] = rd_p->lim_sw.x0;
            tmpvec0[2] = rd_p->lim_sw.z0;
            vcTransRotRoadArea(tmpvec1, sw_rm, tmpvec0);
            tmpvec0[0] = rd_p->lim_sw.x2;
            tmpvec0[2] = rd_p->lim_sw.z2;
            vcTransRotRoadArea(tmpvec2, sw_rm, tmpvec0);
            if (tmpvec1[0] > tmpvec2[0]) {
                sw.min_hx = tmpvec2[0];
                sw.max_hx = tmpvec1[0];
            } else {
                sw.min_hx = tmpvec1[0];
                sw.max_hx = tmpvec2[0];
            }
            if (tmpvec1[2] > tmpvec2[2]) {
                sw.min_hz = tmpvec2[2];
                sw.max_hz = tmpvec1[2];
            } else {
                sw.min_hz = tmpvec1[2];
                sw.max_hz = tmpvec2[2];
            }
            tmpvec0[0] = w_p->chara_pos[0];
            tmpvec0[2] = w_p->chara_pos[2];
            vcTransRotRoadArea(ppos, sw_rm, tmpvec0);
            get_min_x = ppos[0] - half_w;
            get_max_x = ppos[0] + half_w;
            get_min_z = ppos[2] - half_w;
            get_max_z = ppos[2] + half_w;
            rd_min_x = rd.min_hx;
            rd_max_x = rd.max_hx;
            rd_min_z = rd.min_hz;
            rd_max_z = rd.max_hz;
            sw_min_x = sw.min_hx;
            sw_max_x = sw.max_hx;
            sw_min_z = sw.min_hz;
            sw_max_z = sw.max_hz;
            if ((sw_min_x <= get_max_x && sw_max_x >= get_min_x && sw_min_z <= get_max_z && sw_max_z >= get_min_z)
                || (rd_min_x <= get_max_x && rd_max_x >= get_min_x && rd_min_z <= get_max_z && rd_max_z >= get_min_z)) {
                near_rd_p = &w_p->near_road_ary[w_p->near_road_suu];
                near_rd_p->road_p = rd_p;
                near_rd_p->chara2road_sum_dist = vcGetXZSumDistFromLimArea(&near_rd_p->chara2road_vec_x, &near_rd_p->chara2road_vec_z, ppos[0], ppos[2], sw_min_x, sw_max_x, sw_min_z, sw_max_z, near_rd_p->road_p->flags & 0x80);
                if (near_rd_p->sw.min_hy < w_p->chara_pos[1]) {
                    add = fabsf(w_p->chara_pos[1] - near_rd_p->sw.min_hy);
                } else if (near_rd_p->sw.max_hy > w_p->chara_pos[1]) {
                    add = fabsf(near_rd_p->sw.min_hy - w_p->chara_pos[1]);
                } else {
                    add = 0.0f;
                }
                near_rd_p->chara2road_sum_dist += add;
                near_rd_p->rd_dir_type = rd_p->rd_dir_type;
                near_rd_p->use_priority = vcRetRoadUsePriority(rd_p->rd_type);
                near_rd_p->rd.min_hx = rd_min_x;
                near_rd_p->rd.max_hx = rd_max_x;
                near_rd_p->rd.min_hz = rd_min_z;
                near_rd_p->rd.max_hz = rd_max_z;
                near_rd_p->sw.min_hx = sw_min_x;
                near_rd_p->sw.max_hx = sw_max_x;
                near_rd_p->sw.min_hz = sw_min_z;
                near_rd_p->sw.max_hz = sw_max_z;
                near_rd_p->sw.min_hy = rd_p->lim_sw.min_hy;
                near_rd_p->sw.max_hy = rd_p->lim_sw.max_hy;
                near_rd_p->rd.min_hy = rd_p->lim_rd.min_hy;
                near_rd_p->rd.max_hy = rd_p->lim_rd.max_hy;
                mcopy(rd_rm, near_rd_p->rd_rzm);
                unitmatrix(rd_rm);
                shRotMatrixY(rd_rm, rd_rm, rdrot);
                rd_rm[3][0] = -rd_p->lim_rd.x1;
                rd_rm[3][2] = -rd_p->lim_rd.z1;
                mcopy(rd_rm, near_rd_p->rd_rim);
                mcopy(sw_rm, near_rd_p->sw_rzm);
                unitmatrix(sw_rm);
                shRotMatrixY(sw_rm, sw_rm, swrot);
                sw_rm[3][0] = -rd_p->lim_sw.x1;
                sw_rm[3][2] = -rd_p->lim_sw.z1;
                mcopy(sw_rm, near_rd_p->sw_rim);
                w_p->near_road_suu++;
                /* Matching: the assert bakes its original line number into the object. */
#line 1910
                VC_ASSERT(w_p->near_road_suu <= 128);
            }
        }
    }
}

static int vcRetRoadUsePriority(enum _VC_ROAD_TYPE rd_type) {
    switch (rd_type) {
    case VC_RD_TYPE_EVENT:
        return vcRetPrioStgEvnt(stage->glb_crd) ? 0 : 6;
    case VC_RD_TYPE_EFFECT:
        return vcRetPrioStgEvnt(stage->glb_crd) ? 0 : 5;
    case VC_RD_TYPE_ROAD_PRIO_HIGH:
        return 3;
    case VC_RD_TYPE_ROAD:
        return 2;
    case VC_RD_TYPE_ROAD_PRIO_LOW:
        return 1;
    case VC_RD_TYPE_SV_ONLY:
        return (vcWork.flags & VC_PRS_F_VIEW_F) ? 4 : 0;
    }
    return 0;
}

#define VC_SET_NEW_CUR_NEAR_ROAD(new_cur_p)                                                                  \
    if ((new_cur_p)->road_p->flags & VC_RD_WARP_IN_F) {                                                     \
        ret_warp_f = 1;                                                                                     \
    }                                                                                                       \
    if (w_p->cur_near_road.road_p->flags & VC_RD_WARP_OUT_F) {                                              \
        ret_warp_f = 1;                                                                                     \
    }                                                                                                       \
    w_p->cur_near_road = *(new_cur_p);                                                                      \
    if ((new_cur_p)->road_p->proj_sec == 0.0f) {                                                            \
        proj_frame = 0;                                                                                     \
    } else {                                                                                                \
        proj_frame = (new_cur_p)->road_p->proj_sec * shGetFPS();                                            \
    }                                                                                                       \
    vcSetProjectionValue((new_cur_p)->road_p->projection, proj_frame);                                      \
    vcWork.flags |= VC_SWITCH_NEAR_RD_DATA_F;

static int vcSetCurNearRoadInVC_WORK(struct _VC_WORK *w_p) {
    struct _VC_NEAR_ROAD_DATA *n_rd_p;
    struct _VC_NEAR_ROAD_DATA *new_cur_p;
    struct _VC_NEAR_ROAD_DATA *old_cur_p;
    float new_cur_sum_dist;
    float adv_old_cur_dist;
    int ret_warp_f;
    int proj_frame;
    float ofs_ang_y;
    float old_cur_rd_ang_y;
    float old_cur_sum_dist;

    ret_warp_f = 0;
    new_cur_sum_dist = vcGetBestNewCurNearRoad(&new_cur_p, VC_CHK_SW, w_p->chara_pos, w_p);
    for (n_rd_p = w_p->near_road_ary, old_cur_p = NULL; n_rd_p < &w_p->near_road_ary[w_p->near_road_suu]; n_rd_p++) {
        if (n_rd_p->road_p == w_p->cur_near_road.road_p) {
            old_cur_p = n_rd_p;
        }
    }
    if (old_cur_p == NULL) {
        VC_SET_NEW_CUR_NEAR_ROAD(new_cur_p);
        return ret_warp_f;
    }
    adv_old_cur_dist = vcAdvantageDistOfOldCurRoad(old_cur_p);
    if (old_cur_p->use_priority > new_cur_p->use_priority && old_cur_p->chara2road_sum_dist < 2.0f * adv_old_cur_dist) {
        w_p->cur_near_road = *old_cur_p;
    } else if (old_cur_p->use_priority < new_cur_p->use_priority && new_cur_p->chara2road_sum_dist <= 0.0f) {
        VC_SET_NEW_CUR_NEAR_ROAD(new_cur_p);
    } else if ((old_cur_p->road_p->flags & VC_RD_NO_EXTRA_AREA_F)
               && ((old_cur_p->road_p->mv_y_type == 5 && new_cur_p->road_p->mv_y_type == 0)
                   || (old_cur_p->road_p->mv_y_type == 0 && new_cur_p->road_p->mv_y_type == 5))) {
        VC_SET_NEW_CUR_NEAR_ROAD(new_cur_p);
    } else {
        old_cur_sum_dist = old_cur_p->chara2road_sum_dist;
        switch (old_cur_p->rd_dir_type) {
        case 0:
            old_cur_rd_ang_y = 0.0f;
            break;
        case 1:
            old_cur_rd_ang_y = DEG2RAD(90.0f);
            break;
        case 2:
            old_cur_rd_ang_y = 0.0f;
            break;
        default:
            /* Matching: the assert bakes its original line number into the object. */
#line 2068
            VC_ASSERT_DW(0);
        }
        ofs_ang_y = shAngleRegulate(w_p->chara_mv_ang_y - old_cur_rd_ang_y);
        if (ofs_ang_y < 0.0f) {
            ofs_ang_y += DEG2RAD(180.0f);
        }
        if (ofs_ang_y > DEG2RAD(90.0f)) {
            ofs_ang_y = DEG2RAD(180.0f) - ofs_ang_y;
        }
        if (old_cur_sum_dist - adv_old_cur_dist <= new_cur_sum_dist) {
            w_p->cur_near_road = *old_cur_p;
        } else if (old_cur_sum_dist < 0.0f && ofs_ang_y < DEG2RAD(20.0f)) {
            w_p->cur_near_road = *old_cur_p;
        } else {
            VC_SET_NEW_CUR_NEAR_ROAD(new_cur_p);
        }
    }
    if ((new_cur_p->road_p->flags & VC_RD_NOT_WARP_F) || (old_cur_p->road_p->flags & VC_RD_NOT_WARP_F)) {
        ret_warp_f = 0;
    }
    if (vcWork.flags & VC_SWITCH_NEAR_RD_DATA_F) {
        vcEndProcessingOldNearRoad(old_cur_p, w_p);
    }
    return ret_warp_f;
}

#define VC_RENEWAL_NEW_CUR(nearest_p, min_dist)                                                             \
    {                                                                                                       \
        int renewal_f;                                                                                      \
                                                                                                            \
        renewal_f = 0;                                                                                      \
        if (nearest_p) {                                                                                    \
            if ((nearest_p)->use_priority > new_cur_priority) {                                             \
                if ((min_dist) <= 0.0f || (min_dist) < new_cur_dist) {                                      \
                    renewal_f = 1;                                                                          \
                }                                                                                           \
            } else if ((nearest_p)->use_priority < new_cur_priority) {                                      \
                if (new_cur_dist > 0.0f && (min_dist) < new_cur_dist) {                                     \
                    renewal_f = 1;                                                                          \
                }                                                                                           \
            } else {                                                                                        \
                if ((min_dist) < new_cur_dist) {                                                            \
                    renewal_f = 1;                                                                          \
                }                                                                                           \
            }                                                                                               \
        }                                                                                                   \
        if (renewal_f) {                                                                                    \
            new_cur_p = (nearest_p);                                                                        \
            new_cur_dist = (min_dist);                                                                      \
            new_cur_priority = (nearest_p)->use_priority;                                                   \
        }                                                                                                   \
    }

static float vcGetBestNewCurNearRoad(struct _VC_NEAR_ROAD_DATA **new_cur_pp, VC_CHK_TYPE chk_type, float *pos, struct _VC_WORK *w_p) {
    struct _VC_NEAR_ROAD_DATA *new_cur_p;
    float new_cur_dist;
    struct _VC_NEAR_ROAD_DATA *road_nearest_p;
    struct _VC_NEAR_ROAD_DATA *eff_nearest_p;
    struct _VC_NEAR_ROAD_DATA *evnt_nearest_p;
    float road_min_dist;
    float eff_min_dist;
    float evnt_min_dist;
    struct _VC_NEAR_ROAD_DATA *rd_high_nearest_p;
    struct _VC_NEAR_ROAD_DATA *rd_low_nearest_p;
    struct _VC_NEAR_ROAD_DATA *sv_only_nearest_p;
    float rd_high_min_dist;
    float rd_low_min_dist;
    float sv_only_min_dist;
    int new_cur_priority;

    new_cur_p = NULL;
    new_cur_dist = 3.4028235e+38f;
    evnt_min_dist = vcGetNearestNEAR_ROAD_DATA(&evnt_nearest_p, chk_type, VC_RD_TYPE_EVENT, pos, w_p, 0);
    eff_min_dist = vcGetNearestNEAR_ROAD_DATA(&eff_nearest_p, chk_type, VC_RD_TYPE_EFFECT, pos, w_p, 0);
    road_min_dist = vcGetNearestNEAR_ROAD_DATA(&road_nearest_p, chk_type, VC_RD_TYPE_ROAD, pos, w_p, 0);
    rd_high_min_dist = vcGetNearestNEAR_ROAD_DATA(&rd_high_nearest_p, chk_type, VC_RD_TYPE_ROAD_PRIO_HIGH, pos, w_p, 0);
    rd_low_min_dist = vcGetNearestNEAR_ROAD_DATA(&rd_low_nearest_p, chk_type, VC_RD_TYPE_ROAD_PRIO_LOW, pos, w_p, 0);
    sv_only_min_dist = vcGetNearestNEAR_ROAD_DATA(&sv_only_nearest_p, chk_type, VC_RD_TYPE_SV_ONLY, pos, w_p, 0);
    new_cur_priority = 0;
    VC_RENEWAL_NEW_CUR(evnt_nearest_p, evnt_min_dist);
    VC_RENEWAL_NEW_CUR(road_nearest_p, road_min_dist);
    VC_RENEWAL_NEW_CUR(eff_nearest_p, eff_min_dist);
    VC_RENEWAL_NEW_CUR(rd_high_nearest_p, rd_high_min_dist);
    VC_RENEWAL_NEW_CUR(rd_low_nearest_p, rd_low_min_dist);
    VC_RENEWAL_NEW_CUR(sv_only_nearest_p, sv_only_min_dist);
    if (new_cur_p == NULL) {
        float dummy;

        new_cur_p = &vcNullNearRoad;
        new_cur_dist = vcGetXZSumDistFromLimArea(&dummy, &dummy, pos[0], pos[2], vcNullNearRoad.rd.min_hx, vcNullNearRoad.rd.max_hx, vcNullNearRoad.rd.min_hz, vcNullNearRoad.rd.max_hz, vcNullNearRoad.road_p->flags & VC_RD_MARGE_ROAD_F);
    }
    *new_cur_pp = new_cur_p;
    return new_cur_dist;
}

static float vcGetNearestNEAR_ROAD_DATA(struct _VC_NEAR_ROAD_DATA **out_nearest_p_addr, VC_CHK_TYPE chk_type, enum _VC_ROAD_TYPE rd_type, float *pos, struct _VC_WORK *w_p, int chk_only_set_marge_f) {
    struct _VC_NEAR_ROAD_DATA *nearest_p;
    struct _VC_NEAR_ROAD_DATA *n_rd_p;
    float min_sum_dist;
    float dummy;
    float cppos[4];
    float min_x;
    float max_x;
    float min_z;
    float max_z;
    float dist;

    nearest_p = NULL;
    min_sum_dist = 3.4028235e+38f;
    for (n_rd_p = w_p->near_road_ary; n_rd_p < &w_p->near_road_ary[w_p->near_road_suu]; n_rd_p++) {
        if (n_rd_p->road_p->rd_type != rd_type) {
            continue;
        }
        if (chk_only_set_marge_f && !(n_rd_p->road_p->flags & VC_RD_MARGE_ROAD_F)) {
            continue;
        }
        switch (chk_type) {
        case VC_CHK_RD:
            min_x = n_rd_p->rd.min_hx;
            max_x = n_rd_p->rd.max_hx;
            min_z = n_rd_p->rd.min_hz;
            max_z = n_rd_p->rd.max_hz;
            break;
        case VC_CHK_SW:
            min_x = n_rd_p->sw.min_hx;
            max_x = n_rd_p->sw.max_hx;
            min_z = n_rd_p->sw.min_hz;
            max_z = n_rd_p->sw.max_hz;
            break;
        default:
            /* Matching: the assert bakes its original line number into the object. */
#line 2288
            VC_ASSERT(0);
        }
        vcTransRotRoadArea(cppos, n_rd_p->sw_rzm, pos);
        dist = vcGetXZSumDistFromLimArea(&dummy, &dummy, cppos[0], cppos[2], min_x, max_x, min_z, max_z, n_rd_p->road_p->flags & VC_RD_MARGE_ROAD_F);
        if (dist <= min_sum_dist && pos[1] <= n_rd_p->sw.min_hy && pos[1] >= n_rd_p->sw.max_hy) {
            min_sum_dist = dist;
            nearest_p = n_rd_p;
        }
    }
    *out_nearest_p_addr = nearest_p;
    return min_sum_dist;
}

static float vcAdvantageDistOfOldCurRoad(struct _VC_NEAR_ROAD_DATA *old_cur_p) {
    if ((enum _VC_ROAD_FLAGS)old_cur_p->road_p->flags & VC_RD_NO_EXTRA_AREA_F) {
        return 0.0f;
    }
    switch (old_cur_p->road_p->rd_type) {
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 2346
        VC_ASSERT_DW(0);
    case VC_RD_TYPE_ROAD:
    case VC_RD_TYPE_ROAD_PRIO_LOW:
    case VC_RD_TYPE_ROAD_PRIO_HIGH:
    case VC_RD_TYPE_SV_ONLY:
        return extra_boundary_width[old_cur_p->road_p->area_size_type];
    case VC_RD_TYPE_EFFECT:
    case VC_RD_TYPE_EVENT:
        return 50.0f;
    }
}

static void vcAutoRenewalWatchTgtPosAndAngZ(struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type, enum _VC_AREA_SIZE_TYPE cur_rd_area_size, float far_watch_rate, float self_view_eff_rate) {
    float far_watch_pos[4];
    float vec[4];

    vcMakeFarWatchTgtPos(far_watch_pos, w_p, cur_rd_area_size);
    switch (cam_mv_type) {
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 2395
        VC_ASSERT_DW(0);
    case VC_MV_CHASE:
    case VC_MV_SETTLE:
    case VC_MV_FIX_ANG:
    case VC_MV_LOCUS_CIRCLE:
    case VC_MV_THROUGH_DOOR:
        vcMakeNormalWatchTgtPos(w_p->watch_tgt_pos, &w_p->watch_tgt_ang_z, w_p, cam_mv_type, cur_rd_area_size);
        if (far_watch_rate != 0.0f) {
            if (playing.control_type == 1 && (vcWork.cur_near_road.road_p->cam_mv_type == VC_MV_CHASE || vcWork.cur_near_road.road_p->cam_mv_type == VC_MV_LOCUS_CIRCLE)) {
                vcopy(w_p->chara_pos, w_p->watch_tgt_pos);
                w_p->watch_tgt_pos[1] = 225.0f + w_p->chara_top_y;
            }
            _shSubVectorXYZ(vec, far_watch_pos, w_p->watch_tgt_pos);
            _shScaleVectorXYZ(vec, vec, far_watch_rate);
            _shAddVectorXYZ(w_p->watch_tgt_pos, w_p->watch_tgt_pos, vec);
        }
        break;
    case VC_MV_SELF_VIEW:
        vcopy(far_watch_pos, w_p->watch_tgt_pos);
        break;
    }
    vcMixSelfViewEffectToWatchTgtPos(w_p->watch_tgt_pos, &w_p->watch_tgt_ang_z, self_view_eff_rate, w_p, vcPreInfo.hero_neck_wm, sh2jms.lower_now);
    if (w_p->watch_tgt_pos[1] > w_p->watch_tgt_max_y) {
        w_p->watch_tgt_pos[1] = w_p->watch_tgt_max_y;
    }
}

static void vcMakeNormalWatchTgtPos(float *watch_tgt_pos, float *watch_tgt_ang_z_p, struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type, enum _VC_AREA_SIZE_TYPE cur_rd_area_size) {
    float watch_y;
    float tgt_watch_cir_r;
    float tgt_chara2watch_cir_dist;
    float ang[4];
    float vec[4];
    float min_cam2watch_dist;
    float cam2chara_dist;

    *watch_tgt_ang_z_p = 0.0f;
    if (cam_mv_type == VC_MV_FIX_ANG) {
        _sceVu0UnitVector(ang);
        ang[0] = w_p->cur_near_road.road_p->tmp.fix.ang_x;
        ang[1] = shAngleRegulate(w_p->cur_near_road.road_p->tmp.fix.ang_y + w_p->fix_man.add_ang_y);
        vwAngleToVector(vec, ang, w_p->cur_near_road.road_p->tmp.fix.cam2wth_dist);
        _shAddVector(watch_tgt_pos, vec, w_p->cam_pos);
        watch_tgt_pos[3] = 1.0f;
        return;
    }
    cam2chara_dist = distXZ(w_p->cam_pos, w_p->chara_pos);
    switch (cam_mv_type) {
    case VC_MV_SELF_VIEW:
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 2533
        VC_ASSERT_DW(0);
    case VC_MV_SETTLE:
        tgt_watch_cir_r = nml_tgt_watch_cir_r[cur_rd_area_size] / 2.0f;
        tgt_chara2watch_cir_dist = 0.0f;
        break;
    case VC_MV_THROUGH_DOOR:
    case VC_MV_CHASE:
    case VC_MV_LOCUS_CIRCLE:
        if (stage->glb_crd == 1 && !(sh2jms.player->pos.x < -20000.0f)) {
            tgt_watch_cir_r = 1.75f * nml_tgt_watch_cir_r[cur_rd_area_size];
            min_cam2watch_dist = 1.75f * nml_cam2wth_min_dist[cur_rd_area_size];
        } else if (sh2jms.player->status & 0x20000) {
            tgt_watch_cir_r = 2000.0f;
            min_cam2watch_dist = 5000.0f;
        } else {
            tgt_watch_cir_r = nml_tgt_watch_cir_r[cur_rd_area_size];
            min_cam2watch_dist = nml_cam2wth_min_dist[cur_rd_area_size];
        }
        min_cam2watch_dist = tgt_watch_cir_r + min_cam2watch_dist;
        if (cam2chara_dist < min_cam2watch_dist) {
            tgt_chara2watch_cir_dist = min_cam2watch_dist - cam2chara_dist;
        } else {
            tgt_chara2watch_cir_dist = 0.0f;
        }
        break;
    }
    watch_y = vcWork.cur_near_road.road_p->ofs_watch_hy + w_p->chara_bottom_y;
    vcSetWatchTgtXzPos(watch_tgt_pos, w_p->chara_pos, w_p->cam_pos, tgt_chara2watch_cir_dist, tgt_watch_cir_r, w_p->chara_eye_ang_y);
    vcSetWatchTgtYParam(watch_tgt_pos, w_p, cam_mv_type, watch_y);
}

static void vcMixSelfViewEffectToWatchTgtPos(float *watch_tgt_pos, float *watch_tgt_ang_z_p, float effect_rate, struct _VC_WORK *w_p, float (*head_mat)[4], int anim_status) {
    float cam_ang[4];
    float chara2head_ofs_ang_y;
    float far_ang_x;
    float chara2far_ofs_ang_y;
    float view_xyz_len;
    float vec[4];
    float xz_dist;

    _shSubVectorXYZ(vec, watch_tgt_pos, w_p->cam_pos);
    view_xyz_len = lengthXYZ(vec);
    xz_dist = lengthXZ(vec);
    far_ang_x = shAtan2(xz_dist, -vec[1]);
    far_ang_x = far_ang_x < DEG2RAD(-70.0f) ? DEG2RAD(-70.0f) : (far_ang_x > DEG2RAD(70.0f) ? DEG2RAD(70.0f) : far_ang_x);
    chara2far_ofs_ang_y = shAngleRegulate(shAtanV(vec) - sys.hero.ang[1]);
    vwMatrixToAngleYXZ(cam_ang, head_mat);
    chara2head_ofs_ang_y = shAngleRegulate(cam_ang[1] - sys.hero.ang[1]);
    switch (anim_status) {
    case JMS_ST_L_RUN1:
    case JMS_ST_L_RUN2:
    case JMS_ST_L_RUN3:
        break;
    case JMS_ST_L_RELAX:
        if (w_p->nearest_enemy_p != NULL) {
            cam_ang[2] = 0.0f;
        }
        break;
    default:
        cam_ang[2] /= 2.0f;
        break;
    }
    switch (anim_status) {
    case JMS_ST_L_LSRUN:
    case JMS_ST_L_RSRUN:
    default:
        cam_ang[1] = sys.hero.ang[1];
        break;
    case JMS_ST_L_RELAX:
        if (w_p->nearest_enemy_p != NULL) {
            cam_ang[1] = sys.hero.ang[1];
        } else {
            cam_ang[1] += DEG2RAD(30.0f);
        }
        break;
    case JMS_ST_L_FALL:
        break;
    case JMS_ST_L_TIRED: {
        float abs_ofs_ang_y;
        float add_ang_y;

        abs_ofs_ang_y = fabsf(chara2head_ofs_ang_y);
        if (abs_ofs_ang_y > DEG2RAD(4.0f)) {
            add_ang_y = DEG2RAD(4.0f) + (abs_ofs_ang_y - DEG2RAD(4.0f)) / 8.0f;
            if (chara2head_ofs_ang_y < 0.0f) {
                add_ang_y *= -1.0f;
            }
        } else {
            add_ang_y = chara2head_ofs_ang_y;
        }
        cam_ang[1] = add_ang_y + sys.hero.ang[1];
        break;
    }
    case JMS_ST_L_WALK:
    case JMS_ST_L_RUN1:
    case JMS_ST_L_RUN2:
    case JMS_ST_L_RUN3:
    case JMS_ST_L_RROUND:
    case JMS_ST_L_LROUND:
        chara2head_ofs_ang_y /= 8.0f;
        chara2head_ofs_ang_y = chara2head_ofs_ang_y < DEG2RAD(-10.0f) ? DEG2RAD(-10.0f) : (chara2head_ofs_ang_y > DEG2RAD(10.0f) ? DEG2RAD(10.0f) : chara2head_ofs_ang_y);
        cam_ang[1] = chara2head_ofs_ang_y + sys.hero.ang[1];
        break;
    }
    switch (anim_status) {
    case JMS_ST_L_TIRED:
        cam_ang[0] += DEG2RAD(-8.0f);
        break;
    case JMS_ST_L_RELAX:
        if (w_p->nearest_enemy_p != NULL) {
            cam_ang[0] = DEG2RAD(-7.0f);
        } else {
            cam_ang[0] += DEG2RAD(-8.0f);
        }
        break;
    case JMS_ST_L_WALK:
    case JMS_ST_L_RUN1:
    case JMS_ST_L_RUN2:
    case JMS_ST_L_RUN3:
        break;
    case JMS_ST_L_RROUND:
    case JMS_ST_L_LROUND: {
        int jouge_val;

        jouge_val = ((int)sys.hero.ang[1] >> 7) & 0xF;
        switch (jouge_val) {
        case 0:
        case 5:
            cam_ang[0] += DEG2RAD(-1.0f);
            break;
        }
        cam_ang[0] += DEG2RAD(-6.0f);
        break;
    }
    }
    switch (anim_status) {
    default:
        cam_ang[0] = 0.7f * far_ang_x;
        break;
    case JMS_ST_L_TIRED:
    case JMS_ST_L_RELAX:
    case JMS_ST_L_WALK:
    case JMS_ST_L_RUN1:
    case JMS_ST_L_RUN2:
    case JMS_ST_L_RUN3:
    case JMS_ST_L_RROUND:
    case JMS_ST_L_LROUND:
        cam_ang[0] += far_ang_x / 2.0f;
        break;
    }
    cam_ang[0] = cam_ang[0] < DEG2RAD(-80.0f) ? DEG2RAD(-80.0f) : (cam_ang[0] > DEG2RAD(80.0f) ? DEG2RAD(80.0f) : cam_ang[0]);
    {
        float eff_pos[4];
        float ofs_ang;

        ofs_ang = shAngleRegulate(cam_ang[2] - *watch_tgt_ang_z_p);
        *watch_tgt_ang_z_p += ofs_ang * effect_rate;
        eff_pos[0] = w_p->cam_pos[0] + shCosF(cam_ang[0]) * (view_xyz_len * shSinF(cam_ang[1]));
        eff_pos[2] = w_p->cam_pos[2] + shCosF(cam_ang[0]) * (view_xyz_len * shCosF(cam_ang[1]));
        eff_pos[1] = w_p->cam_pos[1] - view_xyz_len * shSinF(cam_ang[0]);
        _shSubVectorXYZ(eff_pos, eff_pos, watch_tgt_pos);
        _shScaleVectorXYZ(eff_pos, eff_pos, effect_rate);
        _shAddVectorXYZ(watch_tgt_pos, watch_tgt_pos, eff_pos);
    }
}

/* NON_MATCHING: sqr_c(cir_r) squares a kept copy (mov.s; mul.s) that our compiler build folds away, which
 * shifts the tail (116 words): a compiler-build difference
 * (docs/matching-notes.md#vc_main-vcmakefarwatchtgtpos). */
static void vcMakeFarWatchTgtPos(float *watch_tgt_pos, struct _VC_WORK *w_p, enum _VC_AREA_SIZE_TYPE cur_rd_area_size) {
    float cir_r, watch_y, search[4], ofs_ang_x, actual_dist, use_dist;
    struct SubCharacter *sc_p;
    float adj_dist, lim_y, real_ang_y, real_ang_x, tmpvec[4], chr2cam_dist, dist, cir_3d_r, ofs_eye_ang_y;

    /* Matching: several locals per declaration, as the original's 7 lines for 16 locals require; the
     * grouping is unknown (DWARF order) (docs/matching-notes.md#vc_main-vcmakefarwatchtgtpos-locals). */

    if (stage->glb_crd == 1 && !(sh2jms.player->pos.x < -20000.0f)) {
        cir_r = 1.75f * far_tgt_watch_cir_r[cur_rd_area_size];
    } else if (stage->glb_crd == 9 || stage->glb_crd == 12 || stage->glb_crd == 13) {
        cir_r = vcRetCirRadiusReduction(w_p);
    } else if (sh2jms.player->status & 0x20000) {
        cir_r = 7200.0f;
    } else {
        cir_r = far_tgt_watch_cir_r[cur_rd_area_size];
    }
    if (w_p->nearest_enemy_p || w_p->nearest_item_p) {
        if (w_p->nearest_enemy_p) {
            sc_p = w_p->nearest_enemy_p;
            actual_dist = use_dist = w_p->nearest_enemy_xz_dist;
        } else {
            sc_p = w_p->nearest_item_p;
            actual_dist = use_dist = w_p->nearest_item_xz_dist;
        }
        if (use_dist < 850.0f) {
            adj_dist = -350.0f * use_dist / 850.0f;
        } else {
            adj_dist = -350.0f;
        }
        use_dist += adj_dist;
        if (cir_r > use_dist) {
            cir_r = use_dist;
        }
        if (cir_r < 200.0f) {
            cir_r = 200.0f;
        }
        watch_y = sc_p->eye_y;
        lim_y = -250.0f + w_p->chara_pos[1] + actual_dist / 2.0f;
        if (watch_y > lim_y) {
            watch_y = lim_y;
        }
    } else {
        watch_y = -450.0f + w_p->chara_pos[1];
    }
    watch_tgt_pos[0] = w_p->chara_pos[0] + cir_r * shSinF(w_p->chara_eye_ang_y);
    watch_tgt_pos[1] = watch_y;
    watch_tgt_pos[2] = w_p->chara_pos[2] + cir_r * shCosF(w_p->chara_eye_ang_y);
    watch_tgt_pos[3] = 1.0f;
    ofs_eye_ang_y = shAngleRegulate(w_p->chara_eye_ang_y + vcPreInfo.hero_head_ang_y);
    chr2cam_dist = vcSquareDistXZ(w_p->chara_pos, w_p->cam_pos);
    watch_tgt_pos[0] = w_p->chara_pos[0] + cir_r * shSinF(ofs_eye_ang_y);
    watch_tgt_pos[2] = w_p->chara_pos[2] + cir_r * shCosF(ofs_eye_ang_y);
    _shSubVector(tmpvec, w_p->cam_pos, watch_tgt_pos);
    real_ang_y = shAtanV(tmpvec);
    cir_3d_r = fsqrt(sqr_c(cir_r) + sqr_c(watch_y - vcPreInfo.hero_eye_y));
    watch_tgt_pos[0] = w_p->chara_pos[0] + cir_r * shSinF(DEG2RAD(180.0f) + real_ang_y);
    watch_tgt_pos[2] = w_p->chara_pos[2] + cir_r * shCosF(DEG2RAD(180.0f) + real_ang_y);
    watch_tgt_pos[1] = watch_y;
    watch_tgt_pos[3] = 1.0f;
    tmpvec[0] = w_p->cam_pos[1] - watch_tgt_pos[1];
    tmpvec[2] = distXZ(watch_tgt_pos, w_p->cam_pos);
    ofs_ang_x = shAtanV(tmpvec);
    tmpvec[1] = -cir_3d_r * shSinF(vcPreInfo.hero_head_ang_x);
    watch_tgt_pos[1] += tmpvec[1];
    tmpvec[0] = w_p->cam_pos[1] - watch_tgt_pos[1];
    tmpvec[2] = distXZ(watch_tgt_pos, w_p->cam_pos);
    real_ang_x = shAtanV(tmpvec);
    real_ang_x -= ofs_ang_x;
    watch_tgt_pos[1] = watch_y;
    dist = distXYZ(watch_tgt_pos, w_p->cam_pos);
    search[1] = -dist * shSinF(real_ang_x);
    watch_tgt_pos[1] = watch_y + search[1];
    watch_tgt_pos[3] = 1.0f;
}

static void vcSetWatchTgtXzPos(float *watch_pos, float *center_pos, float *cam_pos, float tgt_chara2watch_cir_dist, float tgt_watch_cir_r, float watch_cir_ang_y) {
    float cam2chr_ang;
    float chr2watch_x;
    float chr2watch_z;

    cam2chr_ang = shAtan2(center_pos[2] - cam_pos[2], center_pos[0] - cam_pos[0]);
    chr2watch_x = tgt_chara2watch_cir_dist * shSinF(cam2chr_ang) + tgt_watch_cir_r * shSinF(watch_cir_ang_y);
    chr2watch_z = tgt_chara2watch_cir_dist * shCosF(cam2chr_ang) + tgt_watch_cir_r * shCosF(watch_cir_ang_y);
    watch_pos[0] = center_pos[0] + chr2watch_x;
    watch_pos[2] = center_pos[2] + chr2watch_z;
}

static void vcSetWatchTgtYParam(float *watch_pos, struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type, float watch_y) {
    switch (cam_mv_type) {
    case VC_MV_SELF_VIEW:
        watch_pos[1] = w_p->chara_center_y;
        break;
    default:
        watch_pos[1] = watch_y;
        break;
    }
}

/*
 * Matching: the statements are the original's (line table: one statement for max_cam_ang_x, and
 * ofs_y set just before cam_ang_x); whether the first also assigned ofs_y is unknown (`-(ofs_y =
 * -2500.0f - cam_pos[1])` compiles the same). Its 6500.0f is loaded first because of the DEG2RAD(-x)
 * spelling in vcMixSelfViewEffectToWatchTgtPos (see DEG2RAD; docs/toolchain.md "Root cause").
 */
static void vcAdjustWatchYLimitHighWhenFarView(float *watch_pos, float *cam_pos) {
    float max_cam_ang_x;
    float cam_ang_x;
    float dist;
    float ofs_y;

    max_cam_ang_x = shAtan2(6500.0f, -(-2500.0f - cam_pos[1])) - shAtan2(VbScreenInfo.scr_z, VbScreenInfo.sy / 2.0f);
    dist = distXZ(cam_pos, watch_pos);
    ofs_y = watch_pos[1] - cam_pos[1];
    cam_ang_x = shAtan2(dist, -ofs_y);
    if (cam_ang_x > max_cam_ang_x) {
        watch_pos[1] = cam_pos[1] - dist * shSinF(max_cam_ang_x) / shCosF(max_cam_ang_x);
    }
    switch (stage->glb_crd) {
    case 9:
        if (cam_ang_x > DEG2RAD(-22.5f)) {
            watch_pos[1] = cam_pos[1] - dist * shSinF(DEG2RAD(-22.5f)) / shCosF(DEG2RAD(-22.5f));
        }
        break;
    }
}

static void vcAutoRenewalCamTgtPos(struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type, struct _VC_CAM_MV_PARAM *cam_mv_prm_p, enum _VC_ROAD_FLAGS cur_rd_flags, enum _VC_AREA_SIZE_TYPE cur_rd_area_size, float far_watch_rate) {
    float tgt_vec[4];
    float ideal_pos[4];
    float max_tgt_mv_xz_len;
    VC_MAKE_TYPE make_type;

    switch (cam_mv_type) {
    case VC_MV_SELF_VIEW:
        vcMakeIdealCamPosByHeadPos(ideal_pos, w_p, cur_rd_area_size);
        break;
    case VC_MV_FIX_ANG:
        vcMakeIdealCamPosForFixAngCam(ideal_pos, w_p);
        break;
    case VC_MV_THROUGH_DOOR:
        vcMakeIdealCamPosForThroughDoorCam(ideal_pos, w_p);
        break;
    case VC_MV_LOCUS_CIRCLE:
        vcMakeIdealCamPosForLocusCircleCam(ideal_pos, w_p);
        break;
    default:
        vcMakeIdealCamPosUseVC_ROAD_DATA(ideal_pos, w_p, cur_rd_area_size);
        break;
    }
    if (vcWork.flags & VC_WARP_CAM_TGT_F) {
        vcopy(ideal_pos, w_p->cam_tgt_pos);
    }
    switch (cam_mv_type) {
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 3137
        VC_ASSERT_DW(0);
    case VC_MV_CHASE:
    case VC_MV_SETTLE:
    case VC_MV_FIX_ANG:
    case VC_MV_LOCUS_CIRCLE:
    case VC_MV_THROUGH_DOOR:
        make_type = VC_MAKE_NORMAL;
        break;
    case VC_MV_SELF_VIEW:
        make_type = VC_MAKE_SELF_VIEW;
        break;
    }
    switch (make_type) {
    case VC_MAKE_NORMAL:
        max_tgt_mv_xz_len = vcRetMaxTgtMvXzLen(w_p, cam_mv_prm_p);
        vcMakeBasicCamTgtMvVec(tgt_vec, ideal_pos, w_p, max_tgt_mv_xz_len);
        if ((cam_mv_type == VC_MV_CHASE || cam_mv_type == VC_MV_LOCUS_CIRCLE) && !(cur_rd_flags & VC_RD_NO_FRONT_FLIP_F)) {
            vcCamTgtMvVecIsFlipedFromCharaFront(tgt_vec, w_p, max_tgt_mv_xz_len, cur_rd_area_size);
        }
        if (cam_mv_type != VC_MV_THROUGH_DOOR) {
            vcAdjTgtMvVecYByCurNearRoad(tgt_vec, w_p);
        }
        break;
    case VC_MAKE_SELF_VIEW:
        max_tgt_mv_xz_len = 500.0f;
        vcMakeBasicCamTgtMvVec(tgt_vec, ideal_pos, w_p, max_tgt_mv_xz_len);
        break;
    }
    w_p->cam_tgt_mv_ang_y = shAtanV(tgt_vec);
    if (shGetDT() != 0.0f || (vcWork.flags & VC_WARP_CAM_TGT_F)) {
        _shAddVectorXYZ(w_p->cam_tgt_pos, w_p->cam_tgt_pos, tgt_vec);
        _shDivVectorXYZ(w_p->cam_tgt_velo, tgt_vec, shGetDT());
        w_p->cam_tgt_spd = lengthXZ(w_p->cam_tgt_velo);
    } else {
        w_p->cam_tgt_velo[0] = 0.0f;
        w_p->cam_tgt_velo[2] = 0.0f;
        w_p->cam_tgt_spd = 0.0f;
    }
    vcChangeProjByDist(&w_p->cur_near_road, w_p->cam_tgt_pos[1] - w_p->chara_bottom_y);
}

static float vcRetMaxTgtMvXzLen(struct _VC_WORK *w_p, struct _VC_CAM_MV_PARAM *cam_mv_prm_p) {
    float max_spd_xz;
    float max_spd_xz_stg;

    max_spd_xz = 500.0f + w_p->chara_mv_spd + fabsf(8.0f * w_p->chara_ang_spd_y);
    if (BgIsOut(0) || stage->glb_crd == 5) {
        if (!(sh2jms.player->status & 0x20000)) {
            max_spd_xz_stg = 3300.0f;
        } else {
            max_spd_xz_stg = 1650.0f;
        }
    } else {
        max_spd_xz_stg = 1200.0f;
    }
    max_spd_xz = fclamp(max_spd_xz, max_spd_xz_stg, cam_mv_prm_p->max_spd_xz);
    return max_spd_xz * shGetDT();
}

static void vcMakeIdealCamPosByHeadPos(float *ideal_pos, struct _VC_WORK *w_p, enum _VC_AREA_SIZE_TYPE cur_rd_area_size) {
    float chara2cam_ang_y;

    if (w_p->flags & VC_WARP_WATCH_F) {
        vcopy3(w_p->chara_pos, ideal_pos);
        ideal_pos[1] = w_p->chara_top_y;
    } else {
        chara2cam_ang_y = shAngleRegulate(DEG2RAD(170.0f) + w_p->chara_eye_ang_y);
        ideal_pos[1] = 50.0f + w_p->chara_head_pos[1];
        ideal_pos[0] = w_p->chara_head_pos[0] + 90.0f * shSinF(chara2cam_ang_y);
        ideal_pos[2] = w_p->chara_head_pos[2] + 90.0f * shCosF(chara2cam_ang_y);
    }
}

static void vcMakeIdealCamPosForFixAngCam(float *ideal_pos, struct _VC_WORK *w_p) {
    float ang[4];
    struct _VC_LIMIT_AREA lim_rd;
    float front2cam_dist;
    float chara_front_r;
    float abs_ofs_x;
    float abs_ofs_z;
    float lim2chara_dist;
    float cppos[4];
    float chara2cam_dist;
    float ratio;
    float base_front_r;
    float zero_front_dist;
    float full_front_dist;
    float rate;

    if (vcWork.flags & VC_SWITCH_NEAR_RD_DATA_F) {
        w_p->fix_man.exception_f = 0;
        w_p->fix_man.add_ang_y = 0.0f;
        w_p->fix_man.add_rd_x = 0.0f;
        w_p->fix_man.add_rd_z = 0.0f;
    }
    _sceVu0UnitVector(ang);
    ang[0] = w_p->cur_near_road.road_p->tmp.fix.ang_x;
    ang[1] = shAngleRegulate(w_p->cur_near_road.road_p->tmp.fix.ang_y + w_p->fix_man.add_ang_y);
    lim_rd.min_hx = w_p->cur_near_road.rd.min_hx + w_p->fix_man.add_rd_x;
    lim_rd.max_hx = w_p->cur_near_road.rd.max_hx + w_p->fix_man.add_rd_x;
    lim_rd.min_hz = w_p->cur_near_road.rd.min_hz + w_p->fix_man.add_rd_z;
    lim_rd.max_hz = w_p->cur_near_road.rd.max_hz + w_p->fix_man.add_rd_z;
    vcTransRotRoadArea(cppos, w_p->cur_near_road.rd_rzm, w_p->chara_pos);
    lim2chara_dist = vcGetXZSumDistFromLimArea(&abs_ofs_x, &abs_ofs_z, cppos[0], cppos[2], w_p->cur_near_road.rd.min_hx, w_p->cur_near_road.rd.max_hx, w_p->cur_near_road.rd.min_hz, w_p->cur_near_road.rd.max_hz, 0);
    abs_ofs_x = fabsf(abs_ofs_x);
    abs_ofs_z = fabsf(abs_ofs_z);
    front2cam_dist = 1000.0f + fmaxf_gcc(abs_ofs_x, abs_ofs_z) / 2.0f;
    ratio = w_p->cur_near_road.road_p->tmp.fix.cam2wth_dist / 2000.0f;
    base_front_r = 350.0f * ratio;
    zero_front_dist = 3500.0f * ratio;
    full_front_dist = 750.0f * ratio;
    chara2cam_dist = distXZ(w_p->cam_pos, w_p->chara_pos);
    if (chara2cam_dist >= zero_front_dist) {
        chara_front_r = 0.0f;
    } else if (chara2cam_dist <= full_front_dist) {
        chara_front_r = base_front_r;
    } else {
        rate = (chara2cam_dist - zero_front_dist) / (full_front_dist - zero_front_dist);
        chara_front_r = rate * base_front_r;
    }
    ideal_pos[0] = w_p->chara_pos[0] + chara_front_r * shSinF(w_p->chara_eye_ang_y) + front2cam_dist * shSinF(DEG2RAD(180.0f) + ang[1]);
    ideal_pos[2] = w_p->chara_pos[2] + chara_front_r * shCosF(w_p->chara_eye_ang_y) + front2cam_dist * shCosF(DEG2RAD(180.0f) + ang[1]);
    ideal_pos[1] = w_p->chara_pos[1];
    w_p->chara_top_y += w_p->cur_near_road.road_p->tmp.fix.ofs_hy;
    vcTransRotRoadArea(ideal_pos, w_p->cur_near_road.rd_rzm, ideal_pos);
    vcAdjustXzInLimAreaUsingMIN_IN_ROAD_DIST(&ideal_pos[0], &ideal_pos[2], &lim_rd);
    vcRotTransRoadArea(ideal_pos, w_p->cur_near_road.rd_rim, ideal_pos);
}

/*
 * Matching: the clamped and scaled angle is computed in cos_ang_y over three statements, as the
 * original's line table has them (fmaxf_gcc; the scaling; mv_forwd_dist with shCosF inline). The
 * DWARF gives cos_ang_y and abs_ofs_ang_y the same stack home and no register.
 */
static void vcMakeIdealCamPosForThroughDoorCam(float *ideal_pos, struct _VC_WORK *w_p) {
    struct _VC_THROUGH_DOOR_CAM_PARAM *prm_p;
    float mv_forwd_dist;
    float mv_right_dist;
    float cut_ang_y;
    float base_forwd_dist;
    float max_add_forwd_dist;
    float max_add_right_dist;
    float abs_ofs_ang_y;
    float cos_ang_y;

    prm_p = &w_p->through_door;
    if (!w_p->through_door.active_f) {
        return;
    }
    if (w_p->through_door_activate_init_f) {
        mv_forwd_dist = -650.0f;
        mv_right_dist = 0.0f;
    } else {
        cut_ang_y = DEG2RAD(45.0f);
        base_forwd_dist = 375.0f;
        max_add_forwd_dist = 350.0f;
        max_add_right_dist = 300.0f;
        abs_ofs_ang_y = fabsf(shAngleRegulate(w_p->chara_eye_ang_y - prm_p->rail_ang_y));
        cos_ang_y = fmaxf_gcc(0.0f, abs_ofs_ang_y - cut_ang_y);
        cos_ang_y = DEG2RAD(180.0f) * cos_ang_y / (DEG2RAD(180.0f) - cut_ang_y);
        mv_forwd_dist = base_forwd_dist + -max_add_forwd_dist * shCosF(cos_ang_y);
        mv_right_dist = -max_add_right_dist * shSinF(w_p->chara_eye_ang_y - prm_p->rail_ang_y);
    }
    ideal_pos[0] = prm_p->rail_sta_pos[0] + mv_forwd_dist * shSinF(prm_p->rail_ang_y) + mv_right_dist * shCosF(prm_p->rail_ang_y);
    ideal_pos[2] = prm_p->rail_sta_pos[2] + mv_forwd_dist * shCosF(prm_p->rail_ang_y) + mv_right_dist * -shSinF(prm_p->rail_ang_y);
    ideal_pos[1] = prm_p->rail_sta_pos[1];
}

static void vcMakeIdealCamPosForLocusCircleCam(float *ideal_pos, struct _VC_WORK *w_p) {
    float origin[4];
    float sw_l[4];
    float ofs_ang_y;
    float cir_ang_y;
    float cir_radius;

    cir_ang_y = shAngleRegulate(w_p->cur_near_road.road_p->tmp.cir.ang_y);
    cir_radius = w_p->cur_near_road.road_p->tmp.cir.radius;
    if (vcWork.flags & VC_SWITCH_NEAR_RD_DATA_F) {
        *(u_long128 *)w_p->cir_man.sw_l = 0;
        _sceVu0UnitVector(w_p->cir_man.origin);
        w_p->cir_man.sw_l[0] = fabsf(w_p->cur_near_road.sw.max_hx - w_p->cur_near_road.sw.min_hx);
        w_p->cir_man.sw_l[2] = fabsf(w_p->cur_near_road.sw.max_hz - w_p->cur_near_road.sw.min_hz);
        if (DEG2RAD(-180.0f) <= cir_ang_y && cir_ang_y < DEG2RAD(-90.0f)) {
            w_p->cir_man.origin[0] = fmaxf_gcc(w_p->cur_near_road.road_p->lim_sw.x0, w_p->cur_near_road.road_p->lim_sw.x2);
            w_p->cir_man.origin[2] = fmaxf_gcc(w_p->cur_near_road.road_p->lim_sw.z0, w_p->cur_near_road.road_p->lim_sw.z2);
        } else if (DEG2RAD(-90.0f) <= cir_ang_y && cir_ang_y < 0.0f) {
            w_p->cir_man.origin[0] = fmaxf_gcc(w_p->cur_near_road.road_p->lim_sw.x0, w_p->cur_near_road.road_p->lim_sw.x2);
            w_p->cir_man.origin[2] = fminf_gcc(w_p->cur_near_road.road_p->lim_sw.z0, w_p->cur_near_road.road_p->lim_sw.z2);
        } else if (0.0f <= cir_ang_y && cir_ang_y < DEG2RAD(90.0f)) {
            w_p->cir_man.origin[0] = fminf_gcc(w_p->cur_near_road.road_p->lim_sw.x0, w_p->cur_near_road.road_p->lim_sw.x2);
            w_p->cir_man.origin[2] = fminf_gcc(w_p->cur_near_road.road_p->lim_sw.z0, w_p->cur_near_road.road_p->lim_sw.z2);
        } else {
            w_p->cir_man.origin[0] = fminf_gcc(w_p->cur_near_road.road_p->lim_sw.x0, w_p->cur_near_road.road_p->lim_sw.x2);
            w_p->cir_man.origin[2] = fmaxf_gcc(w_p->cur_near_road.road_p->lim_sw.z0, w_p->cur_near_road.road_p->lim_sw.z2);
        }
    }
    if (w_p->cur_near_road.road_p->tmp.cir.origin_x != 0.0f && w_p->cur_near_road.road_p->tmp.cir.origin_z != 0.0f) {
        origin[0] = w_p->cur_near_road.road_p->tmp.cir.origin_x;
        origin[2] = w_p->cur_near_road.road_p->tmp.cir.origin_z;
    } else {
        vcopy(w_p->cir_man.origin, origin);
    }
    vcopy(w_p->cir_man.sw_l, sw_l);
    ofs_ang_y = shAtan2(w_p->chara_pos[2] - origin[2], w_p->chara_pos[0] - origin[0]) - cir_ang_y;
    ofs_ang_y *= 2.0f;
    if (DEG2RAD(-180.0f) <= cir_ang_y && cir_ang_y < DEG2RAD(-90.0f)) {
        if (ofs_ang_y > 0.0f) {
            ideal_pos[0] = origin[0] - sw_l[0] - cir_radius * shSinF(ofs_ang_y);
            ideal_pos[2] = origin[2] - sw_l[2] * shCosF(ofs_ang_y);
        } else if (ofs_ang_y < 0.0f) {
            ideal_pos[0] = origin[0] - sw_l[0] * shCosF(ofs_ang_y);
            ideal_pos[2] = origin[2] - sw_l[2] - cir_radius * -shSinF(ofs_ang_y);
        } else {
            ideal_pos[0] = origin[0] - sw_l[0];
            ideal_pos[2] = origin[2] - sw_l[2];
        }
    } else if (DEG2RAD(-90.0f) <= cir_ang_y && cir_ang_y < 0.0f) {
        if (ofs_ang_y > 0.0f) {
            ideal_pos[0] = origin[0] - sw_l[0] * shCosF(ofs_ang_y);
            ideal_pos[2] = origin[2] + sw_l[2] + cir_radius * shSinF(ofs_ang_y);
        } else if (ofs_ang_y < 0.0f) {
            ideal_pos[0] = origin[0] - sw_l[0] - cir_radius * -shSinF(ofs_ang_y);
            ideal_pos[2] = origin[2] + sw_l[2] * shCosF(ofs_ang_y);
        } else {
            ideal_pos[0] = origin[0] - sw_l[0];
            ideal_pos[2] = origin[2] + sw_l[2];
        }
    } else if (0.0f < cir_ang_y && cir_ang_y < DEG2RAD(90.0f)) {
        if (ofs_ang_y > 0.0f) {
            ideal_pos[0] = origin[0] + sw_l[0] + cir_radius * shSinF(ofs_ang_y);
            ideal_pos[2] = origin[2] + sw_l[2] * shCosF(ofs_ang_y);
        } else if (ofs_ang_y < 0.0f) {
            ideal_pos[0] = origin[0] + sw_l[0] * shCosF(ofs_ang_y);
            ideal_pos[2] = origin[2] + sw_l[2] + cir_radius * -shSinF(ofs_ang_y);
        } else {
            ideal_pos[0] = origin[0] + sw_l[0];
            ideal_pos[2] = origin[2] + sw_l[2];
        }
    } else {
        if (ofs_ang_y > 0.0f) {
            ideal_pos[0] = origin[0] + sw_l[0] * shCosF(ofs_ang_y);
            ideal_pos[2] = origin[2] - sw_l[2] - cir_radius * shSinF(ofs_ang_y);
        } else if (ofs_ang_y < 0.0f) {
            ideal_pos[0] = origin[0] + sw_l[0] + cir_radius * -shSinF(ofs_ang_y);
            ideal_pos[2] = origin[2] - sw_l[2] * shCosF(ofs_ang_y);
        } else {
            ideal_pos[0] = origin[0] + sw_l[0];
            ideal_pos[2] = origin[2] - sw_l[2];
        }
    }
    ideal_pos[1] = w_p->ideal_cam_pos_h;
}

static void vcMakeIdealCamPosUseVC_ROAD_DATA(float *ideal_pos, struct _VC_WORK *w_p, enum _VC_AREA_SIZE_TYPE cur_rd_area_size) {
    float chara2ideal_r;
    float cppos[4];
    float now_ang_y;
    float ofs_ang_y;
    float adj_x;
    float adj_z;
    float abs_chr2cam_y;
    float real_chr2cam_r;
    struct _VC_NEAR_ROAD_DATA *use_near_p;
    float out_rd_len;
    float no_adj_max_dist;
    float full_adj_min_dist;

    now_ang_y = DEG2RAD(180.0f) + w_p->chara_eye_ang_y;
    ofs_ang_y = shAngleRegulate(w_p->cam_chara2ideal_ang_y - now_ang_y);
    if (fabsf(w_p->chara_ang_spd_y) > DEG2RAD(20.0f)) {
        ofs_ang_y = ofs_ang_y < DEG2RAD(-12.0f) ? DEG2RAD(-12.0f) : (ofs_ang_y > DEG2RAD(12.0f) ? DEG2RAD(12.0f) : ofs_ang_y);
    } else {
        ofs_ang_y = ofs_ang_y >= 0.0f ? DEG2RAD(12.0f) : DEG2RAD(-12.0f);
    }
    if (playing.control_type == 1) {
        w_p->cam_chara2ideal_ang_y = shAngleRegulate(now_ang_y);
    } else {
        w_p->cam_chara2ideal_ang_y = shAngleRegulate(ofs_ang_y + now_ang_y);
    }
    if (stage->glb_crd == 1 && !(sh2jms.player->pos.x < -20000.0f)) {
        real_chr2cam_r = 1.75f * mv_nml_chr2cam_r[cur_rd_area_size];
    } else if (sh2jms.player->status & 0x20000) {
        real_chr2cam_r = 2000.0f;
    } else {
        real_chr2cam_r = mv_nml_chr2cam_r[cur_rd_area_size];
    }
    ideal_pos[0] = w_p->chara_pos[0] + real_chr2cam_r * shSinF(w_p->cam_chara2ideal_ang_y);
    ideal_pos[2] = w_p->chara_pos[2] + real_chr2cam_r * shCosF(w_p->cam_chara2ideal_ang_y);
    switch (w_p->cur_near_road.road_p->cam_mv_type) {
    case VC_MV_CHASE:
        ideal_pos[1] = w_p->ideal_cam_pos_h + w_p->cur_near_road.road_p->tmp.chs.ofs_hy;
        break;
    case VC_MV_SETTLE:
        ideal_pos[1] = w_p->ideal_cam_pos_h;
        break;
    }
    ideal_pos[3] = 1.0f;
    adj_x = 0.0f;
    adj_z = 0.0f;
    use_near_p = &w_p->cur_near_road;
    abs_chr2cam_y = fabsf(w_p->cam_pos[1] - w_p->chara_pos[1]);
    abs_chr2cam_y = fmaxf_gcc(abs_chr2cam_y - 750.0f, 0.0f);
    ideal_pos[1] = ideal_pos[1] < use_near_p->road_p->lim_rd.max_hy ? use_near_p->road_p->lim_rd.max_hy : (ideal_pos[1] > use_near_p->road_p->lim_rd.min_hy ? use_near_p->road_p->lim_rd.min_hy : ideal_pos[1]);
    vcTransRotRoadArea(cppos, use_near_p->rd_rzm, w_p->chara_pos);
    adj_x = cppos[0];
    adj_z = cppos[2];
    vcAdjustXzInLimAreaUsingMIN_IN_ROAD_DIST(&adj_x, &adj_z, &use_near_p->rd);
    _sceVu0UnitVector(cppos);
    cppos[0] = adj_x;
    cppos[2] = adj_z;
    vcRotTransRoadArea(cppos, use_near_p->rd_rim, cppos);
    adj_x = cppos[0];
    adj_z = cppos[2];
    out_rd_len = vwRet3DLength(adj_x - w_p->chara_pos[0], abs_chr2cam_y, adj_z - w_p->chara_pos[2]);
    if (stage->glb_crd == 1 && !(sh2jms.player->pos.x < -20000.0f)) {
        no_adj_max_dist = 1.75f * mv_nml_no_adj_max_dist[cur_rd_area_size];
        full_adj_min_dist = 1.75f * mv_nml_full_adj_min_dist[cur_rd_area_size];
    } else if (sh2jms.player->status & 0x20000) {
        no_adj_max_dist = 1600.0f;
        full_adj_min_dist = 5000.0f;
    } else {
        no_adj_max_dist = mv_nml_no_adj_max_dist[cur_rd_area_size];
        full_adj_min_dist = mv_nml_full_adj_min_dist[cur_rd_area_size];
    }
    if (out_rd_len > full_adj_min_dist) {
        chara2ideal_r = 200.0f;
    } else if (out_rd_len > no_adj_max_dist) {
        chara2ideal_r = real_chr2cam_r + (200.0f - real_chr2cam_r) * (out_rd_len - no_adj_max_dist) / (full_adj_min_dist - no_adj_max_dist);
    } else {
        chara2ideal_r = real_chr2cam_r;
    }
    ideal_pos[0] = w_p->chara_pos[0] + chara2ideal_r * shSinF(w_p->cam_chara2ideal_ang_y);
    ideal_pos[2] = w_p->chara_pos[2] + chara2ideal_r * shCosF(w_p->cam_chara2ideal_ang_y);
    vcTransRotRoadArea(cppos, use_near_p->rd_rzm, ideal_pos);
    vcAdjustXzInLimAreaUsingMIN_IN_ROAD_DIST(&cppos[0], &cppos[2], &use_near_p->rd);
    vcRotTransRoadArea(cppos, use_near_p->rd_rim, cppos);
    ideal_pos[0] = cppos[0];
    ideal_pos[2] = cppos[2];
}

static void vcAdjustXzInLimAreaUsingMIN_IN_ROAD_DIST(float *x_p, float *z_p, struct _VC_LIMIT_AREA *lim_p) {
    float x;
    float z;
    float min_x;
    float max_x;
    float min_z;
    float max_z;
    float min_in_road_dist;

    x = *x_p;
    z = *z_p;
    min_in_road_dist = vcGetMinInRoadDist();
    min_x = lim_p->min_hx + min_in_road_dist;
    max_x = lim_p->max_hx - min_in_road_dist;
    min_z = lim_p->min_hz + min_in_road_dist;
    max_z = lim_p->max_hz - min_in_road_dist;
    if (min_x > max_x) {
        min_x = max_x = (min_x + max_x) / 2.0f;
    }
    if (min_z > max_z) {
        min_z = max_z = (min_z + max_z) / 2.0f;
    }
    x = x < min_x ? min_x : (x > max_x ? max_x : x);
    z = z < min_z ? min_z : (z > max_z ? max_z : z);
    *x_p = x;
    *z_p = z;
}

static void vcMakeBasicCamTgtMvVec(float *tgt_mv_vec, float *ideal_pos, struct _VC_WORK *w_p, float max_tgt_mv_xz_len) {
    float xz_vec[4];
    float now2ideal_tgt_ang_y;
    float now2ideal_tgt_dist;
    float sincos_y[4];

    _shSubVectorXYZ(xz_vec, ideal_pos, w_p->cam_tgt_pos);
    now2ideal_tgt_dist = lengthXZ(xz_vec);
    now2ideal_tgt_ang_y = shAtanV(xz_vec);
    if (now2ideal_tgt_dist < max_tgt_mv_xz_len) {
        vcopy3(xz_vec, tgt_mv_vec);
    } else {
        shSinCosV(sincos_y, now2ideal_tgt_ang_y);
        _shScaleVectorXYZ(tgt_mv_vec, sincos_y, max_tgt_mv_xz_len);
    }
    if (shGetDT() == 0.0f && !(vcWork.flags & VC_WARP_CAM_TGT_F)) {
        tgt_mv_vec[1] = 0.0f;
    } else {
        tgt_mv_vec[1] = ideal_pos[1] - w_p->cam_tgt_pos[1];
    }
}

static void vcAdjTgtMvVecYByCurNearRoad(float *tgt_mv_vec, struct _VC_WORK *w_p) {
    float max_tgt_y;
    float min_tgt_y;
    float to_chara_dist;
    float near_ratio;
    struct _VC_ROAD_DATA *cur_rd_p;
    float dist;
    float max_dist;
    float min_dist;
    float abs_ofs_y;

    cur_rd_p = w_p->cur_near_road.road_p;
    to_chara_dist = distXZ(w_p->cam_tgt_pos, w_p->chara_pos);
    if (cur_rd_p->mv_y_type == 4 || cur_rd_p->mv_y_type == 5) {
        if (w_p->cur_near_road.road_p->cam_mv_type != VC_MV_LOCUS_CIRCLE) {
            near_ratio = vcRetNearRatioSwitchAreaInXZPos(w_p->cur_near_road, w_p->chara_pos, w_p->cam_tgt_pos);
        } else {
            near_ratio = vcRetNearRatioSwitchAreaForCircleCam(w_p->cur_near_road, w_p->cir_man, w_p->chara_pos);
            if (cur_rd_p->tmp.cir.ang_y < DEG2RAD(-180.0f) || cur_rd_p->tmp.cir.ang_y >= DEG2RAD(180.0f)) {
                near_ratio = 1.0f - near_ratio;
            }
        }
    } else {
        dist = to_chara_dist;
        if (stage->glb_crd != 5 && stage->glb_crd != 2) {
            max_dist = 3500.0f;
            min_dist = 600.0f;
        } else {
            max_dist = 15500.0f;
            min_dist = 500.0f;
        }
        dist = fclamp(dist, min_dist, max_dist);
        near_ratio = (max_dist - dist) * (1.0f / (max_dist - min_dist));
    }
    near_ratio = near_ratio < 0.0f ? 0.0f : (near_ratio > 1.0f ? 1.0f : near_ratio);
    switch (w_p->cur_near_road.road_p->mv_y_type) {
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 4052
        VC_ASSERT(0);
    case 0:
        abs_ofs_y = 0.0f;
        abs_ofs_y = fmaxf_gcc(abs_ofs_y, (to_chara_dist - 150.0f) / 4.0f);
        if (stage->glb_crd == 1 && !(sh2jms.player->pos.x < -20000.0f)) {
            max_tgt_y = -750.0f + (abs_ofs_y + w_p->chara_top_y);
            min_tgt_y = -750.0f + (-abs_ofs_y + w_p->chara_top_y);
        } else {
            max_tgt_y = -250.0f + (abs_ofs_y + w_p->chara_top_y);
            min_tgt_y = -250.0f + (-abs_ofs_y + w_p->chara_top_y);
        }
        break;
    case 1:
        max_tgt_y = min_tgt_y = cur_rd_p->lim_rd.min_hy * (1.0f - near_ratio) + cur_rd_p->lim_rd.max_hy * near_ratio;
        break;
    case 2:
        max_tgt_y = min_tgt_y = cur_rd_p->lim_rd.max_hy * (1.0f - near_ratio) + cur_rd_p->lim_rd.min_hy * near_ratio;
        break;
    case 3:
        min_tgt_y = cur_rd_p->lim_rd.max_hy;
        max_tgt_y = cur_rd_p->lim_rd.min_hy;
        break;
    case 4: {
        float ofs_y;

        if (BgIsOut(0) || stage->glb_crd == 5) {
            ofs_y = -250.0f;
        } else {
            ofs_y = 0.0f;
        }
        min_tgt_y = max_tgt_y = (1.0f - near_ratio) * (ofs_y + (cur_rd_p->lim_rd.min_hy + w_p->chara_pos[1])) + w_p->ideal_cam_pos_h * near_ratio;
        break;
    }
    case 5: {
        float ofs_y;

        if (BgIsOut(0) || stage->glb_crd == 5) {
            ofs_y = -250.0f;
        } else {
            ofs_y = 0.0f;
        }
        max_tgt_y = min_tgt_y = (1.0f - near_ratio) * (ofs_y + (cur_rd_p->lim_rd.max_hy + w_p->chara_pos[1])) + w_p->ideal_cam_pos_h * near_ratio;
        break;
    }
    }
    {
        float tgt_y;

        tgt_y = w_p->cam_tgt_pos[1] + tgt_mv_vec[1];
        tgt_y = tgt_y < min_tgt_y ? min_tgt_y : (tgt_y > max_tgt_y ? max_tgt_y : tgt_y);
        tgt_mv_vec[1] = tgt_y - w_p->cam_tgt_pos[1];
    }
}

static void vcCamTgtMvVecIsFlipedFromCharaFront(float *tgt_mv_vec, struct _VC_WORK *w_p, float max_tgt_mv_xz_len, enum _VC_AREA_SIZE_TYPE cur_rd_area_size) {
    float flip_dist;
    float flip_ang_y;
    float pre_tgt_pos[4];
    float chk_pos[4];
    float chk_near_dist;
    float pre_nearest_dist;
    float post_tgt_pos[4];
    float sincos_y[4];
    struct _VC_NEAR_ROAD_DATA *use_nearest_p;
    float min_x;
    float max_x;
    float min_z;
    float max_z;
    float min_in_road_dist;
    float pos[4];
    float mv_len;
    float ang_y;

    _shAddVectorXYZ(pre_tgt_pos, tgt_mv_vec, w_p->cam_tgt_pos);
    pre_tgt_pos[1] = 0.0f;
    pre_tgt_pos[3] = 1.0f;
    flip_dist = vcFlipFromCamExclusionArea(&flip_ang_y, &w_p->old_cam_excl_area_r, pre_tgt_pos, w_p->chara_pos, w_p->chara_eye_ang_y, cur_rd_area_size);
    if (flip_dist > 0.0f) {
        if (flip_dist > 250.0f) {
            chk_near_dist = 250.0f;
        } else {
            chk_near_dist = flip_dist;
        }
        *(u_long128 *)sincos_y = 0;
        shSinCosV(sincos_y, flip_ang_y);
        _shScaleVectorXYZ(chk_pos, sincos_y, chk_near_dist);
        _shAddVectorXYZ(chk_pos, pre_tgt_pos, chk_pos);
        if (!(w_p->cur_near_road.road_p->flags & VC_RD_MARGE_ROAD_F)) {
            use_nearest_p = &w_p->cur_near_road;
        } else {
            pre_nearest_dist = vcGetNearestNEAR_ROAD_DATA(&use_nearest_p, VC_CHK_RD, w_p->cur_near_road.road_p->rd_type, pre_tgt_pos, w_p, 1);
            if (use_nearest_p == NULL) {
                use_nearest_p = &vcNullNearRoad;
            } else if (pre_nearest_dist > 0.0f) {
                use_nearest_p = &w_p->cur_near_road;
            }
        }
        _shScaleVectorXYZ(post_tgt_pos, sincos_y, flip_dist);
        _shAddVectorXYZ(post_tgt_pos, pre_tgt_pos, post_tgt_pos);
        /* Matching: the assert bakes its original line number into the object. */
#line 4222
        VC_ASSERT_DW(use_nearest_p != NULL);
        min_in_road_dist = vcGetMinInRoadDist();
        min_x = use_nearest_p->rd.min_hx + min_in_road_dist;
        max_x = use_nearest_p->rd.max_hx - min_in_road_dist;
        min_z = use_nearest_p->rd.min_hz + min_in_road_dist;
        max_z = use_nearest_p->rd.max_hz - min_in_road_dist;
        if (min_x > max_x) {
            min_x = max_x = (min_x + max_x) / 2.0f;
        }
        if (min_z > max_z) {
            min_z = max_z = (min_z + max_z) / 2.0f;
        }
        vcTransRotRoadArea(pos, use_nearest_p->rd_rzm, post_tgt_pos);
        pos[0] = pos[0] < min_x ? min_x : (pos[0] > max_x ? max_x : pos[0]);
        pos[2] = pos[2] < min_z ? min_z : (pos[2] > max_z ? max_z : pos[2]);
        vcRotTransRoadArea(post_tgt_pos, use_nearest_p->rd_rim, pos);
        tgt_mv_vec[0] = post_tgt_pos[0] - w_p->cam_tgt_pos[0];
        tgt_mv_vec[2] = post_tgt_pos[2] - w_p->cam_tgt_pos[2];
        mv_len = lengthXZ(tgt_mv_vec);
        if (mv_len > max_tgt_mv_xz_len) {
            ang_y = shAtanV(tgt_mv_vec);
            shSinCosV(sincos_y, ang_y);
            _shScaleVectorXYZ(tgt_mv_vec, sincos_y, max_tgt_mv_xz_len);
        }
    }
}

static float vcFlipFromCamExclusionArea(float *flip_ang_y_p, float *old_cam_excl_area_r_p, float *in_pos, float *chara_pos, float chara_eye_ang_y, enum _VC_AREA_SIZE_TYPE cur_rd_area_size) {
    float cntr_x;
    float cntr_z;
    float cntr2pos_ang_y;
    float cam_excl_area_r;
    float ret_flip_dist;
    float ofs_ang_y;
    float rate;
    float min_add_dist;
    float add_dist;
    float dist;

    cntr2pos_ang_y = shAtan2(in_pos[2] - chara_pos[2], in_pos[0] - chara_pos[0]);
    /* Matching: dead; the original's DWARF has cntr_x and cntr_z (values reconstructed) */
    cntr_x = chara_pos[0];
    cntr_z = chara_pos[2];
    ofs_ang_y = shAngleRegulate(cntr2pos_ang_y - chara_eye_ang_y);
    if (ofs_ang_y < 0.0f) {
        ofs_ang_y *= -1.0f;
    }
    if (stage->glb_crd == 1 && !(sh2jms.player->pos.x < -20000.0f)) {
        rate = 1.25f * excl_max_rate[cur_rd_area_size];
    } else {
        rate = excl_max_rate[cur_rd_area_size];
    }
    cam_excl_area_r = vwOresenHokan(excl_r_ary, 9, 4096.0f * ofs_ang_y, 0, 0x3243);
    cam_excl_area_r = 500.0f * cam_excl_area_r;
    cam_excl_area_r = cam_excl_area_r / 4096.0f;
    cam_excl_area_r = rate * cam_excl_area_r;
    if (*old_cam_excl_area_r_p != -1.0f) {
        min_add_dist = -250.0f * shGetDT();
        add_dist = cam_excl_area_r - *old_cam_excl_area_r_p;
        if (add_dist < min_add_dist) {
            add_dist = min_add_dist;
        }
        cam_excl_area_r = *old_cam_excl_area_r_p + add_dist;
    }
    *old_cam_excl_area_r_p = cam_excl_area_r;
    dist = distXZ(chara_pos, in_pos);
    if (dist < cam_excl_area_r) {
        ret_flip_dist = cam_excl_area_r - dist;
    } else {
        ret_flip_dist = 0.0f;
    }
    *flip_ang_y_p = cntr2pos_ang_y;
    return ret_flip_dist;
}

static void vcGetUseWatchAndCamMvParam(struct _VC_WATCH_MV_PARAM **watch_mv_prm_pp, struct _VC_CAM_MV_PARAM **cam_mv_prm_pp, float self_view_eff_rate, float far_watch_rate, struct _VC_WORK *w_p) {
    struct _VC_WATCH_MV_PARAM *wth_mv_prm_stg_p;
    float add_ang_accel_y;
    struct _VC_CAM_MV_PARAM *cam_mv_prm_stg_p;
    float diameter;

    if (w_p->flags & VC_USER_WATCH_F) {
        *watch_mv_prm_pp = &w_p->user_watch_mv_prm;
    } else {
        if (BgIsOut(0) || stage->glb_crd == 5) {
            if (!(sh2jms.player->status & 0x20000)) {
                wth_mv_prm_stg_p = (struct _VC_WATCH_MV_PARAM *)&watch_mv_prm_outdoor;
            } else {
                wth_mv_prm_stg_p = (struct _VC_WATCH_MV_PARAM *)&watch_mv_prm_on_boat;
            }
        } else {
            wth_mv_prm_stg_p = (struct _VC_WATCH_MV_PARAM *)&watch_mv_prm_nrml;
        }
        vcWatchMvPrmSt.ang_accel_x = wth_mv_prm_stg_p->ang_accel_x + self_view_eff_rate * (self_view_watch_mv_prm.ang_accel_x - wth_mv_prm_stg_p->ang_accel_x);
        vcWatchMvPrmSt.ang_accel_y = wth_mv_prm_stg_p->ang_accel_y + self_view_eff_rate * (self_view_watch_mv_prm.ang_accel_y - wth_mv_prm_stg_p->ang_accel_y);
        vcWatchMvPrmSt.max_ang_spd_x = wth_mv_prm_stg_p->max_ang_spd_x + self_view_eff_rate * (self_view_watch_mv_prm.max_ang_spd_x - wth_mv_prm_stg_p->max_ang_spd_x);
        vcWatchMvPrmSt.max_ang_spd_y = wth_mv_prm_stg_p->max_ang_spd_y + self_view_eff_rate * (self_view_watch_mv_prm.max_ang_spd_y - wth_mv_prm_stg_p->max_ang_spd_y);
        *watch_mv_prm_pp = &vcWatchMvPrmSt;
        if (!BgIsOut(0) && stage->glb_crd != 5) {
            vcWatchMvPrmSt.ang_accel_x += wth_mv_prm_stg_p->ang_accel_x * far_watch_rate / 2.0f;
            vcWatchMvPrmSt.ang_accel_y += wth_mv_prm_stg_p->ang_accel_y * far_watch_rate / 2.0f;
            vcWatchMvPrmSt.max_ang_spd_x += wth_mv_prm_stg_p->max_ang_spd_x * far_watch_rate / 2.0f;
            vcWatchMvPrmSt.max_ang_spd_y += wth_mv_prm_stg_p->max_ang_spd_y * far_watch_rate / 2.0f;
        }
        add_ang_accel_y = w_p->chara_mv_spd < 0.0f ? 0.0f : (w_p->chara_mv_spd > 2.0f ? 2.0f : w_p->chara_mv_spd);
        vcWatchMvPrmSt.ang_accel_y += add_ang_accel_y;
    }
    if (w_p->flags & VC_USER_CAM_F) {
        *cam_mv_prm_pp = &w_p->user_cam_mv_prm;
    } else {
        if (BgIsOut(0) || stage->glb_crd == 5) {
            if (!(sh2jms.player->status & 0x20000)) {
                cam_mv_prm_stg_p = (struct _VC_CAM_MV_PARAM *)&cam_mv_prm_outdoor;
                diameter = 1.0f;
            } else {
                cam_mv_prm_stg_p = (struct _VC_CAM_MV_PARAM *)&cam_mv_prm_on_boat;
                diameter = 1.0f;
            }
        } else {
            cam_mv_prm_stg_p = (struct _VC_CAM_MV_PARAM *)&cam_mv_prm_nrml;
            diameter = 1.0f + far_watch_rate;
        }
        vcCamMvPrmSt.accel_xz = cam_mv_prm_stg_p->accel_xz * diameter;
        vcCamMvPrmSt.accel_y = cam_mv_prm_stg_p->accel_y * diameter;
        vcCamMvPrmSt.max_spd_xz = cam_mv_prm_stg_p->max_spd_xz * diameter;
        vcCamMvPrmSt.max_spd_y = cam_mv_prm_stg_p->max_spd_y * diameter;
        *cam_mv_prm_pp = &vcCamMvPrmSt;
    }
}

/*
 * Matching: the final pos += velo * dt is VU0 asm in the body, one instruction per line (the
 * original's line table has an entry per instruction); its operands are bound velo first, as the
 * original computes &w_p->cam_velo before &w_p->cam_pos. The locals are declared in the else
 * block: the line table has the if on the line after the header, and only that branch uses them.
 */
static void vcRenewalCamData(struct _VC_WORK *w_p, struct _VC_CAM_MV_PARAM *cam_mv_prm_p) {
    if (w_p->flags & VC_WARP_CAM_F) {
        w_p->cam_mv_ang_y = shAtan2(w_p->cam_tgt_pos[2] - w_p->cam_pos[2], w_p->cam_tgt_pos[0] - w_p->cam_pos[0]);
        vcopy(w_p->cam_tgt_pos, w_p->cam_pos);
        *(u_long128 *)w_p->cam_velo = 0;
    } else {
        float dec_spd_per_dist_xz;
        float dec_spd_per_dist_y;
        float cpos[4];
        float tpos[4];
        float dt;

        dec_spd_per_dist_xz = 0.4f * cam_mv_prm_p->accel_xz;
        dec_spd_per_dist_y = cam_mv_prm_p->accel_y;
        vcopy(w_p->cam_pos, cpos);
        vcopy(w_p->cam_tgt_pos, tpos);
        cpos[0] /= 500.0f;
        cpos[2] /= 500.0f;
        tpos[0] /= 500.0f;
        tpos[2] /= 500.0f;
        vwRenewalXZVelocityToTargetPos(w_p->cam_velo, cpos, tpos, 0.1f, cam_mv_prm_p->accel_xz, cam_mv_prm_p->max_spd_xz, dec_spd_per_dist_xz, 6000.0f);
        w_p->cam_velo[1] = vwRetNewVelocityToTargetVal(w_p->cam_velo[1], w_p->cam_pos[1] / 500.0f, w_p->cam_tgt_pos[1] / 500.0f, cam_mv_prm_p->accel_y, cam_mv_prm_p->max_spd_y, dec_spd_per_dist_y);
        w_p->cam_mv_ang_y = shAtanV(w_p->cam_velo);
        dt = shGetDT();
        __asm__ __volatile__("
        lqc2       vf4, 0x0(%0)
        lqc2       vf5, 0x0(%1)
        mfc1       t7, %2
        qmtc2.ni   t7, vf6
        vmulx.xyzw vf4, vf4, vf6x
        vadd.xyzw  vf4, vf4, vf5
        sqc2       vf4, 0x0(%1)
        " : : "r"(w_p->cam_velo), "r"(w_p->cam_pos), "f"(dt));
    }
}

static void vcRenewalCamMatAng(struct _VC_WORK *w_p, struct _VC_WATCH_MV_PARAM *watch_mv_prm_p, enum _VC_CAM_MV_TYPE cam_mv_type, int visible_chara_f) {
    float ofs_tgt_ang[4];
    float new_base_cam_ang[4];
    float new_base_matT[4][4];
    float ofs_cam2chara_btm_ang[4];
    float ofs_cam2chara_top_ang[4];

    vcMakeNewBaseCamAng(new_base_cam_ang, cam_mv_type, w_p);
    if (new_base_cam_ang[0] != w_p->base_cam_ang[0] || new_base_cam_ang[1] != w_p->base_cam_ang[1] || new_base_cam_ang[2] != w_p->base_cam_ang[2]) {
        vcRenewalBaseCamAngAndAdjustOfsCamAng(w_p, new_base_cam_ang);
    }
    unitmatrix(new_base_matT);
    vwRotMatrixYXZ(w_p->base_cam_ang, new_base_matT);
    vbTransposeMatrixWithoutTr(new_base_matT, new_base_matT);
    vcMakeOfsCamTgtAng(ofs_tgt_ang, new_base_matT, w_p);
    if (visible_chara_f) {
        vcMakeOfsCam2CharaBottomAndTopAngByBaseMatT(ofs_cam2chara_btm_ang, ofs_cam2chara_top_ang, new_base_matT, w_p->cam_pos, w_p->chara_pos, w_p->chara_bottom_y + w_p->cur_near_road.road_p->trace_btm_hy, w_p->chara_top_y);
        vcAdjCamOfsAngByCharaInScreen(ofs_tgt_ang, ofs_cam2chara_btm_ang, ofs_cam2chara_top_ang, w_p);
    }
    if (w_p->flags & VC_WARP_WATCH_F) {
        vcopy(ofs_tgt_ang, w_p->ofs_cam_ang);
        *(u_long128 *)w_p->ofs_cam_ang_spd = 0;
    } else {
        vcAdjCamOfsAngByOfsAngSpd(w_p->ofs_cam_ang, w_p->ofs_cam_ang_spd, ofs_tgt_ang, watch_mv_prm_p);
    }
    if (!(vcWork.flags & VC_USER_MODE_FLAGS)) {
        vcCorrectCamMatAngForcibly(w_p->ofs_cam_ang, *w_p->cur_near_road.road_p);
    }
    vcMakeCamMatAndCamAngByBaseAngAndOfsAng(w_p->cam_mat_ang, w_p->cam_mat, new_base_cam_ang, w_p->ofs_cam_ang, w_p->cam_pos);
    w_p->ofs_cam_ang[0] = shAngleRegulate(w_p->ofs_cam_ang[0]);
    w_p->ofs_cam_ang[1] = shAngleRegulate(w_p->ofs_cam_ang[1]);
    w_p->ofs_cam_ang[2] = shAngleRegulate(w_p->ofs_cam_ang[2]);
}

static void vcMakeNewBaseCamAng(float *new_base_ang, enum _VC_CAM_MV_TYPE cam_mv_type, struct _VC_WORK *w_p) {
    float xyz_vec[4];
    float new_base_ang_x;
    float new_base_ang_y;
    float cam2watch_ang_x;
    float cam2watch_ang_y;
    float deflt_sta_base_ang_y;
    float deflt_end_base_ang_y;
    float ofs_sta_ang_y;
    float ofs_end_ang_y;
    float mv_ang_y;
    float max_ang_y_spd;

    _shSubVectorXYZ(xyz_vec, w_p->watch_tgt_pos, w_p->cam_pos);
    if (w_p->flags & VC_USER_WATCH_F) {
        *(u_long128 *)new_base_ang = 0;
        return;
    }
    switch (cam_mv_type) {
    default:
        /* Matching: the assert bakes its original line number into the object. */
#line 4703
        VC_ASSERT(0);
    case VC_MV_CHASE:
    case VC_MV_FIX_ANG:
    case VC_MV_SELF_VIEW:
    case VC_MV_LOCUS_CIRCLE:
    case VC_MV_THROUGH_DOOR:
        *(u_long128 *)new_base_ang = 0;
        break;
    case VC_MV_SETTLE:
        cam2watch_ang_x = shAtan2(lengthXZ(xyz_vec), -xyz_vec[1]);
        cam2watch_ang_y = shAtanV(xyz_vec);
        deflt_sta_base_ang_y = w_p->cur_near_road.road_p->tmp.stl.sta_base_ang_y;
        deflt_end_base_ang_y = w_p->cur_near_road.road_p->tmp.stl.end_base_ang_y;
        ofs_sta_ang_y = shAngleRegulate(cam2watch_ang_y - deflt_sta_base_ang_y);
        ofs_end_ang_y = shAngleRegulate(cam2watch_ang_y - deflt_end_base_ang_y);
        if (ofs_sta_ang_y >= 0.0f && ofs_end_ang_y <= 0.0f) {
            mv_ang_y = cam2watch_ang_y;
        } else if (fabsf(ofs_sta_ang_y) < fabsf(ofs_end_ang_y)) {
            mv_ang_y = deflt_sta_base_ang_y;
        } else {
            mv_ang_y = deflt_end_base_ang_y;
        }
        if (w_p->flags & VC_WARP_WATCH_F) {
            new_base_ang_y = mv_ang_y;
        } else if (fzero(w_p->chara_mv_spd) && fabsf(cam2watch_ang_x) < DEG2RAD(75.0f)) {
            mv_ang_y = shAngleRegulate(mv_ang_y - w_p->base_cam_ang[1]);
            max_ang_y_spd = DEG2RAD(120.0f) * shGetDT();
            mv_ang_y = mv_ang_y < -max_ang_y_spd ? -max_ang_y_spd : (mv_ang_y > max_ang_y_spd ? max_ang_y_spd : mv_ang_y);
            new_base_ang_y = mv_ang_y + w_p->base_cam_ang[1];
        } else {
            new_base_ang_y = w_p->base_cam_ang[1];
        }
        new_base_ang_x = cam2watch_ang_x;
        if (cam2watch_ang_x < 0.0f) {
            new_base_ang_x *= -1.0f;
        }
        new_base_ang_x = vwOresenHokan(mv_stl_ang_ary, 5, 4096.0f * new_base_ang_x, 0, 0x1921) / 4096.0f;
        new_base_ang_x = new_base_ang_x < 0.0f ? 0.0f : (new_base_ang_x > DEG2RAD(90.0f) ? DEG2RAD(90.0f) : new_base_ang_x);
        if (cam2watch_ang_x < 0.0f) {
            new_base_ang_x *= -1.0f;
        }
        _sceVu0UnitVector(new_base_ang);
        new_base_ang[0] = new_base_ang_x;
        new_base_ang[1] = new_base_ang_y;
        break;
    }
}

static void vcRenewalBaseCamAngAndAdjustOfsCamAng(struct _VC_WORK *w_p, float *new_base_cam_ang) {
    float old_base_mat[4][4];
    float new_base_mat[4][4];
    float new_base_matT[4][4];
    float adj_ofs_mat[4][4];
    float ofs_mat[4][4];

    unitmatrix(adj_ofs_mat);
    unitmatrix(old_base_mat);
    unitmatrix(new_base_mat);
    unitmatrix(ofs_mat);
    vwRotMatrixYXZ(w_p->base_cam_ang, old_base_mat);
    vwRotMatrixYXZ(new_base_cam_ang, new_base_mat);
    vbTransposeMatrixWithoutTr(new_base_matT, new_base_mat);
    vwRotMatrixYXZ(w_p->ofs_cam_ang, ofs_mat);
    shMulMatrix(adj_ofs_mat, new_base_matT, old_base_mat);
    shMulMatrix(ofs_mat, adj_ofs_mat, ofs_mat);
    vwMatrixToAngleYXZ(w_p->ofs_cam_ang, ofs_mat);
    vcopy(new_base_cam_ang, w_p->base_cam_ang);
}

static void vcMakeOfsCamTgtAng(float *ofs_tgt_ang, float (*base_matT)[4], struct _VC_WORK *w_p) {
    float vec[4];

    _shSubVectorXYZ(vec, w_p->watch_tgt_pos, w_p->cam_pos);
    vbApplyMatrixWithoutTr(vec, base_matT, vec);
    vwVectorToAngle(ofs_tgt_ang, vec);
    ofs_tgt_ang[2] = w_p->watch_tgt_ang_z;
}

static void vcMakeOfsCam2CharaBottomAndTopAngByBaseMatT(float *ofs_cam2chara_btm_ang, float *ofs_cam2chara_top_ang, float (*base_matT)[4], float *cam_pos, float *chara_pos, float chara_bottom_y, float chara_top_y) {
    float vec[4];

    _shSubVectorXYZ(vec, chara_pos, cam_pos);
    vec[1] = chara_bottom_y - cam_pos[1];
    vbApplyMatrixWithoutTr(vec, base_matT, vec);
    vwVectorToAngle(ofs_cam2chara_btm_ang, vec);
    _shSubVectorXYZ(vec, chara_pos, cam_pos);
    vec[1] = chara_top_y - cam_pos[1];
    vbApplyMatrixWithoutTr(vec, base_matT, vec);
    vwVectorToAngle(ofs_cam2chara_top_ang, vec);
}

static void vcAdjCamOfsAngByCharaInScreen(float *cam_ang, float *ofs_cam2chara_btm_ang, float *ofs_cam2chara_top_ang, struct _VC_WORK *w_p) {
    float adj_cam_ang_y;
    float adj_cam_ang_x;
    float watch2chr_ofs_ang_y;
    float watch2chr_bottom_ofs_ang_x;
    float watch2chr_top_ofs_ang_x;

    watch2chr_ofs_ang_y = shAngleRegulate(ofs_cam2chara_top_ang[1] - cam_ang[1]);
    watch2chr_bottom_ofs_ang_x = shAngleRegulate(ofs_cam2chara_btm_ang[0] - cam_ang[0]);
    watch2chr_top_ofs_ang_x = shAngleRegulate(ofs_cam2chara_top_ang[0] - cam_ang[0]);
    if (watch2chr_ofs_ang_y > w_p->scr_half_ang_wx) {
        adj_cam_ang_y = shAngleRegulate(watch2chr_ofs_ang_y - w_p->scr_half_ang_wx);
    } else if (watch2chr_ofs_ang_y < -w_p->scr_half_ang_wx) {
        adj_cam_ang_y = shAngleRegulate(watch2chr_ofs_ang_y + w_p->scr_half_ang_wx);
    } else {
        adj_cam_ang_y = 0.0f;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 4954
    VC_ASSERT_DW(w_p->scr_half_ang_wy > DEG2RAD(5.0f));
    if (watch2chr_bottom_ofs_ang_x < -w_p->scr_half_ang_wy) {
        adj_cam_ang_x = watch2chr_bottom_ofs_ang_x + w_p->scr_half_ang_wy;
    } else {
        adj_cam_ang_x = 0.0f;
    }
    if (watch2chr_top_ofs_ang_x - adj_cam_ang_x > w_p->scr_half_ang_wy) {
        adj_cam_ang_x += watch2chr_top_ofs_ang_x - adj_cam_ang_x - w_p->scr_half_ang_wy;
    }
    adj_cam_ang_x = adj_cam_ang_x < DEG2RAD(-30.0f) ? DEG2RAD(-30.0f) : (adj_cam_ang_x > DEG2RAD(30.0f) ? DEG2RAD(30.0f) : adj_cam_ang_x);
    cam_ang[1] += adj_cam_ang_y;
    cam_ang[0] += adj_cam_ang_x;
}

/* Matching: ofs_ang += ofs_ang_spd * dt is VU0 asm in the body, one instruction per line (the line
 * table has an entry per instruction). The third call's angles stay literal (see DEG2RAD). */
static void vcAdjCamOfsAngByOfsAngSpd(float *ofs_ang, float *ofs_ang_spd, float *ofs_tgt_ang, struct _VC_WATCH_MV_PARAM *prm_p) {
    float max_spd_dec_per_dist[4];
    float dt;

    max_spd_dec_per_dist[0] = 8.0f * prm_p->ang_accel_x;
    max_spd_dec_per_dist[1] = 3.0f * prm_p->ang_accel_y;
    ofs_ang_spd[0] = vwRetNewAngSpdToTargetAng(ofs_ang_spd[0], ofs_ang[0], ofs_tgt_ang[0], prm_p->ang_accel_x, prm_p->max_ang_spd_x, max_spd_dec_per_dist[0]);
    ofs_ang_spd[1] = vwRetNewAngSpdToTargetAng(ofs_ang_spd[1], ofs_ang[1], ofs_tgt_ang[1], prm_p->ang_accel_y, prm_p->max_ang_spd_y, max_spd_dec_per_dist[1]);
    ofs_ang_spd[2] = vwRetNewAngSpdToTargetAng(ofs_ang_spd[2], ofs_ang[2], ofs_tgt_ang[2], 2.5132742f, 2.5132742f, 18.849556f);
    dt = shGetDT();
    __asm__ __volatile__("
    lqc2       vf4, 0x0(%0)
    lqc2       vf5, 0x0(%1)
    mfc1       t7, %2
    qmtc2.ni   t7, vf6
    vmulx.xyzw vf4, vf4, vf6x
    vadd.xyzw  vf4, vf4, vf5
    sqc2       vf4, 0x0(%1)
    " : : "r"(ofs_ang_spd), "r"(ofs_ang), "f"(dt));
}

static void vcMakeCamMatAndCamAngByBaseAngAndOfsAng(float *cam_mat_ang, float (*cam_mat)[4], float *base_cam_ang, float *ofs_cam_ang, float *cam_pos) {
    float base_mat[4][4];
    float ofs_mat[4][4];

    unitmatrix(base_mat);
    unitmatrix(ofs_mat);
    vwRotMatrixYXZ(base_cam_ang, base_mat);
    vwRotMatrixYXZ(ofs_cam_ang, ofs_mat);
    shMulMatrix(cam_mat, base_mat, ofs_mat);
    _sceVu0UnitVector(cam_mat[3]);
    vcopy3(cam_pos, cam_mat[3]);
    vwMatrixToAngleYXZ(cam_mat_ang, cam_mat);
}

/* Matching: float code before this function sets the vcCamMatNoise() argument order (fitted, not
 * recovered). */
static float __stripped_float_code_2(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 0.1f + 0.3f + 0.7f; }
static float __stripped_float_code_3(float x) { return x + 3.0f + 5.0f + 0.1f + 0.3f + 0.7f + 1.1f + 1.3f; }
/* The locals are declared in the else block: the original's line table has the if on the line
 * after the header, and only that branch uses them. */
static void vcSetDataToVwSystem(struct _VC_WORK *w_p, enum _VC_CAM_MV_TYPE cam_mv_type) {
    if (cam_mv_type != VC_MV_SELF_VIEW) {
        vwSetViewInfoDirectMatrix(NULL, w_p->cam_mat);
    } else {
        float noise_cam_mat[4][4];
        float noise_mat[4][4];
        float noise_ang[4];

        vcSelfViewTimer += shGetDT();
        noise_ang[0] = vcCamMatNoise(0.0034906585f, 2.443461f, 1.3962634f);
        noise_ang[1] = vcCamMatNoise(0.0021816615f, 0.6981317f, -1.3962634f);
        noise_ang[2] = 0.0f;
        noise_ang[3] = 1.0f;
        unitmatrix(noise_mat);
        vwRotMatrixYXZ(noise_ang, noise_mat);
        noise_mat[0][0] += vcCamMatNoise(0.004f, -0.34906584f, 3.1415927f);
        noise_mat[1][0] += vcCamMatNoise(0.004f, -2.0943952f, -1.3962634f);
        noise_mat[2][0] += vcCamMatNoise(0.004f, -2.0943952f, 1.3962634f);
        noise_mat[0][1] += vcCamMatNoise(0.004f, 2.443461f, 2.443461f);
        noise_mat[1][1] += vcCamMatNoise(0.004f, 3.1415927f, 0.6981317f);
        noise_mat[2][1] += vcCamMatNoise(0.004f, -1.2217305f, -2.268928f);
        shMulMatrix(noise_cam_mat, w_p->cam_mat, noise_mat);
        vcopy3(w_p->cam_mat[3], noise_cam_mat[3]);
        vwSetViewInfoDirectMatrix(NULL, noise_cam_mat);
    }
}

static float vcCamMatNoise(float noise_w, float ang_spd1, float ang_spd2) {
    float noise;

    ang_spd1 = shAngleRegulate(ang_spd1 * vcSelfViewTimer);
    ang_spd2 = shAngleRegulate(ang_spd2 * vcSelfViewTimer);
    noise = (shCosF(ang_spd1) + shCosF(ang_spd2)) / 2.0f;
    return noise * noise_w;
}

static float vcGetXZSumDistFromLimArea(float *out_vec_x_p, float *out_vec_z_p, float chk_wld_x, float chk_wld_z, float lim_min_x, float lim_max_x, float lim_min_z, float lim_max_z, int can_ret_minus_dist_f) {
    float x_dist;
    float z_dist;
    float ret_dist;
    float cntr_x;
    float cntr_z;

    if (chk_wld_x > lim_max_x) {
        *out_vec_x_p = lim_max_x - chk_wld_x;
        x_dist = -*out_vec_x_p;
    } else if (chk_wld_x < lim_min_x) {
        *out_vec_x_p = lim_min_x - chk_wld_x;
        x_dist = *out_vec_x_p;
    } else {
        cntr_x = (lim_max_x + lim_min_x) / 2.0f;
        *out_vec_x_p = 0.0f;
        if (chk_wld_x >= cntr_x) {
            x_dist = chk_wld_x - lim_max_x;
        } else {
            x_dist = lim_min_x - chk_wld_x;
        }
    }
    if (chk_wld_z > lim_max_z) {
        *out_vec_z_p = lim_max_z - chk_wld_z;
        z_dist = -*out_vec_z_p;
    } else if (chk_wld_z < lim_min_z) {
        *out_vec_z_p = lim_min_z - chk_wld_z;
        z_dist = *out_vec_z_p;
    } else {
        cntr_z = (lim_max_z + lim_min_z) / 2.0f;
        *out_vec_z_p = 0.0f;
        if (chk_wld_z >= cntr_z) {
            z_dist = chk_wld_z - lim_max_z;
        } else {
            z_dist = lim_min_z - chk_wld_z;
        }
    }
    if (x_dist >= 0.0f) {
        if (z_dist >= 0.0f) {
            ret_dist = x_dist + z_dist;
        } else {
            ret_dist = x_dist;
        }
    } else {
        if (z_dist >= 0.0f) {
            ret_dist = z_dist;
        } else {
            ret_dist = fmaxf_gcc(x_dist, z_dist);
        }
    }
    if (!can_ret_minus_dist_f && ret_dist < 0.0f) {
        ret_dist = 0.0f;
    }
    return ret_dist;
}

/** Sets an event camera: position vp and watch point vr (user camera and watch modes).
 * @param vp camera position
 * @param cam_prm_p camera movement parameters, or NULL
 * @param vr watch point
 * @param watch_prm_p watch movement parameters, or NULL
 * @param rot_z roll
 * @param warp_flg non-zero to cut */
void vcSetEventCamParamRefView(float *vp, struct _VC_CAM_MV_PARAM *cam_prm_p, float *vr, struct _VC_WATCH_MV_PARAM *watch_prm_p, float rot_z, int warp_flg) {
    vcUserCamTarget(vp, cam_prm_p, warp_flg);
    vcUserWatchTarget(vr, watch_prm_p, rot_z, warp_flg);
}

/** Starts changing the projection (screen distance) to a new value over some frames.
 * @param new new value (0: the default 448)
 * @param framecnt frames to take (below 2, or a pending instant change: at once) */
void vcSetProjectionValue(float new, int framecnt) {
    if (new == 0.0f) {
        vcProjectionParam.new = 448.0f;
    } else {
        vcProjectionParam.new = new;
    }
    vcProjectionParam.old = VbScreenInfo.scr_z;
    if (vcWork.flags & VC_PROJ_MOMENT_CHANGE_F) {
        framecnt = 1;
        vcWork.flags &= ~VC_PROJ_MOMENT_CHANGE_F;
    }
    if (framecnt < 2) {
        vcProjectionParam.flg = 2;
    } else {
        vcProjectionParam.delta = (vcProjectionParam.new - vcProjectionParam.old) / framecnt;
        vcProjectionParam.flg = 1;
    }
}

/** Steps the projection change and updates the view-to-screen matrix and the screen half
 * angles.
 * @param w_p the camera work */
void vcChangeProjectionValue(struct _VC_WORK *w_p) {
    float now;

    if (vcProjectionParam.flg == 0) {
        return;
    }
    if (vcProjectionParam.flg == 2) {
        now = vcProjectionParam.new;
        vcProjectionParam.flg = 0;
    } else {
        now = VbScreenInfo.scr_z + vcProjectionParam.delta;
        if (vcProjectionParam.new > vcProjectionParam.old) {
            if (now > vcProjectionParam.new) {
                now = vcProjectionParam.new;
                vcProjectionParam.flg = 0;
            }
        } else {
            if (now < vcProjectionParam.new) {
                now = vcProjectionParam.new;
                vcProjectionParam.flg = 0;
            }
        }
    }
    VbScreenInfo.scr_z = now;
    vbCalcViewScreenMatrix();
    w_p->scr_half_ang_wx = shAtan2(VbScreenInfo.scr_z, VbScreenInfo.sx) / 2.0f;
    w_p->scr_half_ang_wy = shAtan2(VbScreenInfo.scr_z, VbScreenInfo.sy) / 2.0f;
}
