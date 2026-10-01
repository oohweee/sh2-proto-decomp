/*
 * vw_calc.c: view math: velocities and angular speeds eased toward targets, vector/angle
 * conversions, YXZ rotation matrices, and a polyline interpolation.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "sh_vu0.h"

inline void transposematrix(float (*m)[4]) {
    asm {
        lq     t0, 0x0(m)
        lq     t1, 0x10(m)
        lq     t2, 0x20(m)
        lq     t3, 0x30(m)
        pextlw t4, t1, t0
        pextuw t5, t1, t0
        pextlw t6, t3, t2
        pextuw t7, t3, t2
        pcpyld t0, t6, t4
        pcpyud t1, t4, t6
        pcpyld t2, t7, t5
        pcpyud t3, t5, t7
        sq     t0, 0x0(m)
        sq     t1, 0x10(m)
        sq     t2, 0x20(m)
        sq     t3, 0x30(m)
    }
}

/** Updates an XZ velocity toward a target: accelerates toward it, caps the speed, limits the
 * speed toward it by the distance left (stopping tgt_r short) and damps the sideways part.
 * @param velo_xz the velocity (updated)
 * @param now_pos current position
 * @param tgt_pos target position
 * @param tgt_r stopping distance from the target
 * @param accel acceleration per second
 * @param total_max_spd maximum speed
 * @param dec_forwd_lim_spd forward speed limit per unit of distance left
 * @param dec_accel_side sideways deceleration per second */
void vwRenewalXZVelocityToTargetPos(float *velo_xz, float *now_pos, float *tgt_pos, float tgt_r, float accel, float total_max_spd, float dec_forwd_lim_spd, float dec_accel_side) {
    float cam2tgt_ang_y;
    float cam_mv_ang_y;
    float cam2tgt_dir_vec[4];
    float add_spd;
    float spd;
    float ang_y;
    float to_tgt_ang_y;
    float to_tgt_dist;
    float lim_spd;
    float vec_xz[4];

    cam2tgt_ang_y = shAtan2(tgt_pos[2] - now_pos[2], tgt_pos[0] - now_pos[0]);
    shSinCosV(cam2tgt_dir_vec, cam2tgt_ang_y);
    cam_mv_ang_y = shAtanV(velo_xz);
    add_spd = accel * shGetDT();
    velo_xz[0] += add_spd * cam2tgt_dir_vec[0];
    velo_xz[2] += add_spd * cam2tgt_dir_vec[2];
    spd = lengthXZ(velo_xz);
    if (spd > total_max_spd) {
        ang_y = shAtanV(velo_xz);
        velo_xz[0] -= (spd - total_max_spd) * shSinF(ang_y);
        velo_xz[2] -= (spd - total_max_spd) * shCosF(ang_y);
    }
    _shSubVectorXYZ(vec_xz, tgt_pos, now_pos);
    to_tgt_ang_y = shAtanV(vec_xz);
    to_tgt_dist = lengthXZ(vec_xz);
    lim_spd = dec_forwd_lim_spd * (to_tgt_dist - tgt_r);
    lim_spd = fmaxf(lim_spd, 0.0f);
    vwLimitOverLimVector(velo_xz, lim_spd, to_tgt_ang_y);
    vwDecreaseSideOfVector(velo_xz, dec_accel_side * shGetDT(), lim_spd / 2.0f, to_tgt_ang_y);
}

/** Removes the part of a vector along a direction that exceeds a length.
 * @param vec_xz the vector (updated)
 * @param lim_vec_len the length limit
 * @param lim_vec_ang_y the direction */
void vwLimitOverLimVector(float *vec_xz, float lim_vec_len, float lim_vec_ang_y) {
    float lim_spd_dir_vec_xz[4];
    float over_spd;

    shSinCosV(lim_spd_dir_vec_xz, lim_vec_ang_y);
    over_spd = vec_xz[0] * lim_spd_dir_vec_xz[0] + vec_xz[2] * lim_spd_dir_vec_xz[2] - lim_vec_len;
    if (over_spd > 0.0f) {
        vec_xz[0] -= over_spd * lim_spd_dir_vec_xz[0];
        vec_xz[2] -= over_spd * lim_spd_dir_vec_xz[2];
    }
}

/** Clamps the sideways part of a vector (across a direction) and reduces it toward 0.
 * @param vec_xz the vector (updated)
 * @param dec_val reduction
 * @param max_side_vec_len clamp for the sideways part
 * @param dir_ang_y the direction */
void vwDecreaseSideOfVector(float *vec_xz, float dec_val, float max_side_vec_len, float dir_ang_y) {
    float side_val;
    float sv_val;

    side_val = vec_xz[0] * shSinF(1.5707964f + dir_ang_y) + vec_xz[2] * shCosF(1.5707964f + dir_ang_y);
    sv_val = side_val;
    side_val = side_val < -max_side_vec_len ? -max_side_vec_len : (side_val > max_side_vec_len ? max_side_vec_len : side_val);
    if (side_val > dec_val) {
        side_val -= dec_val;
    } else if (side_val < -dec_val) {
        side_val += dec_val;
    } else {
        side_val = 0.0f;
    }
    vec_xz[0] += (side_val - sv_val) * shSinF(1.5707964f + dir_ang_y);
    vec_xz[2] += (side_val - sv_val) * shCosF(1.5707964f + dir_ang_y);
}

/** Returns a speed accelerated toward a target value, capped, and limited by the distance left.
 * @param now_spd current speed
 * @param mv_pos current value
 * @param tgt_pos target value
 * @param accel acceleration per second
 * @param total_max_spd maximum speed
 * @param dec_val_lim_spd speed limit per unit of distance left */
float vwRetNewVelocityToTargetVal(float now_spd, float mv_pos, float tgt_pos, float accel, float total_max_spd, float dec_val_lim_spd) {
    float new_velo;
    float left;
    float abs_lim_spd;

    left = tgt_pos - mv_pos;
    if (left >= 0.0f) {
        new_velo = now_spd + accel * shGetDT();
    } else {
        new_velo = now_spd - accel * shGetDT();
    }
    new_velo = new_velo < -total_max_spd ? -total_max_spd : (new_velo > total_max_spd ? total_max_spd : new_velo);
    abs_lim_spd = dec_val_lim_spd * fabsf(left);
    if (left >= 0.0f) {
        new_velo = fminf(new_velo, abs_lim_spd);
    } else {
        new_velo = fmaxf(new_velo, -abs_lim_spd);
    }
    return new_velo;
}

/** vwRetNewVelocityToTargetVal for angles (the difference taken in [-pi, pi], divided by pi).
 * @param now_ang_spd current angular speed
 * @param now_ang current angle
 * @param tgt_ang target angle
 * @param accel_spd acceleration per second
 * @param total_max_ang_spd maximum angular speed
 * @param dec_val_lim_spd speed limit per unit of angle left */
float vwRetNewAngSpdToTargetAng(float now_ang_spd, float now_ang, float tgt_ang, float accel_spd, float total_max_ang_spd, float dec_val_lim_spd) {
    float ret_ang_spd;

    ret_ang_spd = vwRetNewVelocityToTargetVal(now_ang_spd, 0.0f, shAngleRegulate(tgt_ang - now_ang) / 3.1415927f, accel_spd, total_max_ang_spd, dec_val_lim_spd);
    return ret_ang_spd;
}

/** Makes a vector of length r from angles (x: pitch, y: heading).
 * @param vec result
 * @param ang the angles
 * @param r the length */
void vwAngleToVector(float *vec, float *ang, float r) {
    float entou_r;

    entou_r = r * shCosF(ang[0]);
    vec[1] = -r * shSinF(ang[0]);
    vec[0] = entou_r * shSinF(ang[1]);
    vec[2] = entou_r * shCosF(ang[1]);
}

/** Gets the pitch and heading of a vector (roll 0).
 * @param ang result
 * @param vec the vector
 * @return the vector's length */
float vwVectorToAngle(float *ang, float *vec) {
    float ret_r;

    ret_r = lengthXYZ(vec);
    ang[0] = shAtan2(lengthXZ(vec), -vec[1]);
    ang[1] = shAtanV(vec);
    ang[2] = 0.0f;
    return ret_r;
}

/** Polyline interpolation: y_suu values evenly spaced over [min_x, max_x], interpolated at
 * input_x (clamped to the ends).
 * @param y_ary the values
 * @param y_suu number of values
 * @param input_x where to interpolate
 * @param min_x x of the first value
 * @param max_x x of the last value */
int vwOresenHokan(int *y_ary, int y_suu, int input_x, int min_x, int max_x) {
    int output_y;
    int amari;
    int kukan_w;
    int kukan_no;

    if (input_x >= max_x) {
        return y_ary[y_suu - 1];
    }
    if (input_x < min_x) {
        return y_ary[0];
    }
    kukan_w = (max_x - min_x) / (y_suu - 1);
    kukan_no = (input_x - min_x) / kukan_w;
    if (kukan_no >= y_suu - 1) {
        return y_ary[y_suu - 1];
    }
    if (kukan_no < 0) {
        return y_ary[0];
    }
    amari = (input_x - min_x) % kukan_w;
    output_y = (amari * y_ary[kukan_no + 1] + y_ary[kukan_no] * (kukan_w - amari)) / kukan_w;
    return output_y;
}

/** mat = mat rotated by rot.z, then rot.x, then rot.y.
 * @param rot the angles
 * @param mat the matrix (updated) */
void vwRotMatrixYXZ(float *rot, float (*mat)[4]) {
    shRotMatrixZ(mat, mat, rot[2]);
    shRotMatrixX(mat, mat, rot[0]);
    shRotMatrixY(mat, mat, rot[1]);
}

/** Gets the angles of a YXZ rotation matrix (the inverse of vwRotMatrixYXZ).
 * @param ang result
 * @param mat the matrix (transposed and restored in place) */
void vwMatrixToAngleYXZ(float *ang, float (*mat)[4]) {
    float r_xz;

    transposematrix(mat);
    r_xz = mat[0][2] * mat[0][2] + mat[2][2] * mat[2][2];
    asm {
        sqrt.s r_xz, r_xz
    }
    ang[0] = shAtan2(r_xz, -mat[1][2]);
    if (ang[0] == 1.5707964f) {
        ang[2] = 0.0f;
        ang[1] = shAtan2(mat[2][1], mat[0][1]);
    } else if (ang[0] == -1.5707964f) {
        ang[2] = 0.0f;
        ang[1] = shAtan2(-mat[2][1], -mat[0][1]);
    } else {
        ang[2] = shAtan2(mat[1][1], mat[1][0]);
        ang[1] = shAtan2(mat[2][2], mat[0][2]);
    }
    transposematrix(mat);
}

/** Returns sqrt(x * x + y * y + z * z). C, not asm: the original's line table has one entry for the
 * whole body, where asm gets one per instruction; MWCC fuses the sum into adda.s/madd.s itself.
 * @param x x
 * @param y y
 * @param z z */
float vwRet3DLength(float x, float y, float z) {
    return _shSqrt(x * x + y * y + z * z);
}
