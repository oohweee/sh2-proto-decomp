/*
 * hh_class_flame_01.c: a fire made of flame sprites rising from a point.
 * An effect class of the HH object manager (hh_class_manager.c): HH_Class_Flame_01 runs once a
 * frame for each instance, between the class's prefix and suffix.
 */
#include "sh2.h"
#include "libc/stdlib.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

static float _square_00_vertex[4][4] = {
    { -1.0f, -2.0f, 0.0f, 1.0f },
    { -1.0f, 0.0f, 0.0f, 1.0f },
    { 1.0f, -2.0f, 0.0f, 1.0f },
    { 1.0f, 0.0f, 0.0f, 1.0f },
};

static float _square_stq_00[4][4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.24804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.0f, 1.0f, 0.0f },
    { 0.24804688f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_stq_01[4][4] = {
    { 0.0f, 0.25f, 1.0f, 0.0f },
    { 0.0f, 0.49804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.25f, 1.0f, 0.0f },
    { 0.24804688f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_stq_02[4][4] = {
    { 0.0f, 0.5f, 1.0f, 0.0f },
    { 0.0f, 0.7480469f, 1.0f, 0.0f },
    { 0.24804688f, 0.5f, 1.0f, 0.0f },
    { 0.24804688f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_stq_03[4][4] = {
    { 0.0f, 0.75f, 1.0f, 0.0f },
    { 0.0f, 0.9980469f, 1.0f, 0.0f },
    { 0.24804688f, 0.75f, 1.0f, 0.0f },
    { 0.24804688f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_stq_10[4][4] = {
    { 0.25f, 0.0f, 1.0f, 0.0f },
    { 0.25f, 0.24804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.0f, 1.0f, 0.0f },
    { 0.49804688f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_stq_11[4][4] = {
    { 0.25f, 0.25f, 1.0f, 0.0f },
    { 0.25f, 0.49804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.25f, 1.0f, 0.0f },
    { 0.49804688f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_stq_12[4][4] = {
    { 0.25f, 0.5f, 1.0f, 0.0f },
    { 0.25f, 0.7480469f, 1.0f, 0.0f },
    { 0.49804688f, 0.5f, 1.0f, 0.0f },
    { 0.49804688f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_stq_13[4][4] = {
    { 0.25f, 0.75f, 1.0f, 0.0f },
    { 0.25f, 0.9980469f, 1.0f, 0.0f },
    { 0.49804688f, 0.75f, 1.0f, 0.0f },
    { 0.49804688f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_stq_20[4][4] = {
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.24804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.0f, 1.0f, 0.0f },
    { 0.7480469f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_stq_21[4][4] = {
    { 0.5f, 0.25f, 1.0f, 0.0f },
    { 0.5f, 0.49804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.25f, 1.0f, 0.0f },
    { 0.7480469f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_stq_22[4][4] = {
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.7480469f, 1.0f, 0.0f },
    { 0.7480469f, 0.5f, 1.0f, 0.0f },
    { 0.7480469f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_stq_23[4][4] = {
    { 0.5f, 0.75f, 1.0f, 0.0f },
    { 0.5f, 0.9980469f, 1.0f, 0.0f },
    { 0.7480469f, 0.75f, 1.0f, 0.0f },
    { 0.7480469f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_stq_30[4][4] = {
    { 0.75f, 0.0f, 1.0f, 0.0f },
    { 0.75f, 0.24804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.0f, 1.0f, 0.0f },
    { 0.9980469f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_stq_31[4][4] = {
    { 0.75f, 0.25f, 1.0f, 0.0f },
    { 0.75f, 0.49804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.25f, 1.0f, 0.0f },
    { 0.9980469f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_stq_32[4][4] = {
    { 0.75f, 0.5f, 1.0f, 0.0f },
    { 0.75f, 0.7480469f, 1.0f, 0.0f },
    { 0.9980469f, 0.5f, 1.0f, 0.0f },
    { 0.9980469f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_stq_33[4][4] = {
    { 0.75f, 0.75f, 1.0f, 0.0f },
    { 0.75f, 0.9980469f, 1.0f, 0.0f },
    { 0.9980469f, 0.75f, 1.0f, 0.0f },
    { 0.9980469f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_stq_rev_00[4][4] = {
    { 0.0f, 0.24804688f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.24804688f, 0.24804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.0f, 1.0f, 0.0f },
};

static float _square_stq_rev_01[4][4] = {
    { 0.0f, 0.49804688f, 1.0f, 0.0f },
    { 0.0f, 0.25f, 1.0f, 0.0f },
    { 0.24804688f, 0.49804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.25f, 1.0f, 0.0f },
};

static float _square_stq_rev_02[4][4] = {
    { 0.0f, 0.7480469f, 1.0f, 0.0f },
    { 0.0f, 0.5f, 1.0f, 0.0f },
    { 0.24804688f, 0.7480469f, 1.0f, 0.0f },
    { 0.24804688f, 0.5f, 1.0f, 0.0f },
};

static float _square_stq_rev_03[4][4] = {
    { 0.0f, 0.9980469f, 1.0f, 0.0f },
    { 0.0f, 0.75f, 1.0f, 0.0f },
    { 0.24804688f, 0.9980469f, 1.0f, 0.0f },
    { 0.24804688f, 0.75f, 1.0f, 0.0f },
};

static float _square_stq_rev_10[4][4] = {
    { 0.25f, 0.24804688f, 1.0f, 0.0f },
    { 0.25f, 0.0f, 1.0f, 0.0f },
    { 0.49804688f, 0.24804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.0f, 1.0f, 0.0f },
};

static float _square_stq_rev_11[4][4] = {
    { 0.25f, 0.49804688f, 1.0f, 0.0f },
    { 0.25f, 0.25f, 1.0f, 0.0f },
    { 0.49804688f, 0.49804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.25f, 1.0f, 0.0f },
};

static float _square_stq_rev_12[4][4] = {
    { 0.25f, 0.7480469f, 1.0f, 0.0f },
    { 0.25f, 0.5f, 1.0f, 0.0f },
    { 0.49804688f, 0.7480469f, 1.0f, 0.0f },
    { 0.49804688f, 0.5f, 1.0f, 0.0f },
};

static float _square_stq_rev_13[4][4] = {
    { 0.25f, 0.9980469f, 1.0f, 0.0f },
    { 0.25f, 0.75f, 1.0f, 0.0f },
    { 0.49804688f, 0.9980469f, 1.0f, 0.0f },
    { 0.49804688f, 0.75f, 1.0f, 0.0f },
};

static float _square_stq_rev_20[4][4] = {
    { 0.5f, 0.24804688f, 1.0f, 0.0f },
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.7480469f, 0.24804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.0f, 1.0f, 0.0f },
};

static float _square_stq_rev_21[4][4] = {
    { 0.5f, 0.49804688f, 1.0f, 0.0f },
    { 0.5f, 0.25f, 1.0f, 0.0f },
    { 0.7480469f, 0.49804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.25f, 1.0f, 0.0f },
};

static float _square_stq_rev_22[4][4] = {
    { 0.5f, 0.7480469f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 0.7480469f, 0.7480469f, 1.0f, 0.0f },
    { 0.7480469f, 0.5f, 1.0f, 0.0f },
};

static float _square_stq_rev_23[4][4] = {
    { 0.5f, 0.9980469f, 1.0f, 0.0f },
    { 0.5f, 0.75f, 1.0f, 0.0f },
    { 0.7480469f, 0.9980469f, 1.0f, 0.0f },
    { 0.7480469f, 0.75f, 1.0f, 0.0f },
};

static float _square_stq_rev_30[4][4] = {
    { 0.75f, 0.24804688f, 1.0f, 0.0f },
    { 0.75f, 0.0f, 1.0f, 0.0f },
    { 0.9980469f, 0.24804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.0f, 1.0f, 0.0f },
};

static float _square_stq_rev_31[4][4] = {
    { 0.75f, 0.49804688f, 1.0f, 0.0f },
    { 0.75f, 0.25f, 1.0f, 0.0f },
    { 0.9980469f, 0.49804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.25f, 1.0f, 0.0f },
};

static float _square_stq_rev_32[4][4] = {
    { 0.75f, 0.7480469f, 1.0f, 0.0f },
    { 0.75f, 0.5f, 1.0f, 0.0f },
    { 0.9980469f, 0.7480469f, 1.0f, 0.0f },
    { 0.9980469f, 0.5f, 1.0f, 0.0f },
};

static float _square_stq_rev_33[4][4] = {
    { 0.75f, 0.9980469f, 1.0f, 0.0f },
    { 0.75f, 0.75f, 1.0f, 0.0f },
    { 0.9980469f, 0.9980469f, 1.0f, 0.0f },
    { 0.9980469f, 0.75f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_00[4][4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.24804688f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.24804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_01[4][4] = {
    { 0.0f, 0.25f, 1.0f, 0.0f },
    { 0.24804688f, 0.25f, 1.0f, 0.0f },
    { 0.0f, 0.49804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_02[4][4] = {
    { 0.0f, 0.5f, 1.0f, 0.0f },
    { 0.24804688f, 0.5f, 1.0f, 0.0f },
    { 0.0f, 0.7480469f, 1.0f, 0.0f },
    { 0.24804688f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_03[4][4] = {
    { 0.0f, 0.75f, 1.0f, 0.0f },
    { 0.24804688f, 0.75f, 1.0f, 0.0f },
    { 0.0f, 0.9980469f, 1.0f, 0.0f },
    { 0.24804688f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_10[4][4] = {
    { 0.25f, 0.0f, 1.0f, 0.0f },
    { 0.49804688f, 0.0f, 1.0f, 0.0f },
    { 0.25f, 0.24804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_11[4][4] = {
    { 0.25f, 0.25f, 1.0f, 0.0f },
    { 0.49804688f, 0.25f, 1.0f, 0.0f },
    { 0.25f, 0.49804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_12[4][4] = {
    { 0.25f, 0.5f, 1.0f, 0.0f },
    { 0.49804688f, 0.5f, 1.0f, 0.0f },
    { 0.25f, 0.7480469f, 1.0f, 0.0f },
    { 0.49804688f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_13[4][4] = {
    { 0.25f, 0.75f, 1.0f, 0.0f },
    { 0.49804688f, 0.75f, 1.0f, 0.0f },
    { 0.25f, 0.9980469f, 1.0f, 0.0f },
    { 0.49804688f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_20[4][4] = {
    { 0.5f, 0.0f, 1.0f, 0.0f },
    { 0.7480469f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.24804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_21[4][4] = {
    { 0.5f, 0.25f, 1.0f, 0.0f },
    { 0.7480469f, 0.25f, 1.0f, 0.0f },
    { 0.5f, 0.49804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_22[4][4] = {
    { 0.5f, 0.5f, 1.0f, 0.0f },
    { 0.7480469f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.7480469f, 1.0f, 0.0f },
    { 0.7480469f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_23[4][4] = {
    { 0.5f, 0.75f, 1.0f, 0.0f },
    { 0.7480469f, 0.75f, 1.0f, 0.0f },
    { 0.5f, 0.9980469f, 1.0f, 0.0f },
    { 0.7480469f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_30[4][4] = {
    { 0.75f, 0.0f, 1.0f, 0.0f },
    { 0.9980469f, 0.0f, 1.0f, 0.0f },
    { 0.75f, 0.24804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.24804688f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_31[4][4] = {
    { 0.75f, 0.25f, 1.0f, 0.0f },
    { 0.9980469f, 0.25f, 1.0f, 0.0f },
    { 0.75f, 0.49804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.49804688f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_32[4][4] = {
    { 0.75f, 0.5f, 1.0f, 0.0f },
    { 0.9980469f, 0.5f, 1.0f, 0.0f },
    { 0.75f, 0.7480469f, 1.0f, 0.0f },
    { 0.9980469f, 0.7480469f, 1.0f, 0.0f },
};

static float _square_stq_lr_l_33[4][4] = {
    { 0.75f, 0.75f, 1.0f, 0.0f },
    { 0.9980469f, 0.75f, 1.0f, 0.0f },
    { 0.75f, 0.9980469f, 1.0f, 0.0f },
    { 0.9980469f, 0.9980469f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_00[4][4] = {
    { 0.24804688f, 0.24804688f, 1.0f, 0.0f },
    { 0.0f, 0.24804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_01[4][4] = {
    { 0.24804688f, 0.49804688f, 1.0f, 0.0f },
    { 0.0f, 0.49804688f, 1.0f, 0.0f },
    { 0.24804688f, 0.25f, 1.0f, 0.0f },
    { 0.0f, 0.25f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_02[4][4] = {
    { 0.24804688f, 0.7480469f, 1.0f, 0.0f },
    { 0.0f, 0.7480469f, 1.0f, 0.0f },
    { 0.24804688f, 0.5f, 1.0f, 0.0f },
    { 0.0f, 0.5f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_03[4][4] = {
    { 0.24804688f, 0.9980469f, 1.0f, 0.0f },
    { 0.0f, 0.9980469f, 1.0f, 0.0f },
    { 0.24804688f, 0.75f, 1.0f, 0.0f },
    { 0.0f, 0.75f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_10[4][4] = {
    { 0.49804688f, 0.24804688f, 1.0f, 0.0f },
    { 0.25f, 0.24804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.0f, 1.0f, 0.0f },
    { 0.25f, 0.0f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_11[4][4] = {
    { 0.49804688f, 0.49804688f, 1.0f, 0.0f },
    { 0.25f, 0.49804688f, 1.0f, 0.0f },
    { 0.49804688f, 0.25f, 1.0f, 0.0f },
    { 0.25f, 0.25f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_12[4][4] = {
    { 0.49804688f, 0.7480469f, 1.0f, 0.0f },
    { 0.25f, 0.7480469f, 1.0f, 0.0f },
    { 0.49804688f, 0.5f, 1.0f, 0.0f },
    { 0.25f, 0.5f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_13[4][4] = {
    { 0.49804688f, 0.9980469f, 1.0f, 0.0f },
    { 0.25f, 0.9980469f, 1.0f, 0.0f },
    { 0.49804688f, 0.75f, 1.0f, 0.0f },
    { 0.25f, 0.75f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_20[4][4] = {
    { 0.7480469f, 0.24804688f, 1.0f, 0.0f },
    { 0.5f, 0.24804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.0f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_21[4][4] = {
    { 0.7480469f, 0.49804688f, 1.0f, 0.0f },
    { 0.5f, 0.49804688f, 1.0f, 0.0f },
    { 0.7480469f, 0.25f, 1.0f, 0.0f },
    { 0.5f, 0.25f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_22[4][4] = {
    { 0.7480469f, 0.7480469f, 1.0f, 0.0f },
    { 0.5f, 0.7480469f, 1.0f, 0.0f },
    { 0.7480469f, 0.5f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_23[4][4] = {
    { 0.7480469f, 0.9980469f, 1.0f, 0.0f },
    { 0.5f, 0.9980469f, 1.0f, 0.0f },
    { 0.7480469f, 0.75f, 1.0f, 0.0f },
    { 0.5f, 0.75f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_30[4][4] = {
    { 0.9980469f, 0.24804688f, 1.0f, 0.0f },
    { 0.75f, 0.24804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.0f, 1.0f, 0.0f },
    { 0.75f, 0.0f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_31[4][4] = {
    { 0.9980469f, 0.49804688f, 1.0f, 0.0f },
    { 0.75f, 0.49804688f, 1.0f, 0.0f },
    { 0.9980469f, 0.25f, 1.0f, 0.0f },
    { 0.75f, 0.25f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_32[4][4] = {
    { 0.9980469f, 0.7480469f, 1.0f, 0.0f },
    { 0.75f, 0.7480469f, 1.0f, 0.0f },
    { 0.9980469f, 0.5f, 1.0f, 0.0f },
    { 0.75f, 0.5f, 1.0f, 0.0f },
};

static float _square_stq_lr_r_33[4][4] = {
    { 0.9980469f, 0.9980469f, 1.0f, 0.0f },
    { 0.75f, 0.9980469f, 1.0f, 0.0f },
    { 0.9980469f, 0.75f, 1.0f, 0.0f },
    { 0.75f, 0.75f, 1.0f, 0.0f },
};

static float (*_square_stq_list[64])[4] = {
    _square_stq_00, _square_stq_01, _square_stq_02, _square_stq_03,
    _square_stq_10, _square_stq_11, _square_stq_12, _square_stq_13,
    _square_stq_20, _square_stq_21, _square_stq_22, _square_stq_23,
    _square_stq_30, _square_stq_31, _square_stq_32, _square_stq_33,
    _square_stq_rev_00, _square_stq_rev_01, _square_stq_rev_02, _square_stq_rev_03,
    _square_stq_rev_10, _square_stq_rev_11, _square_stq_rev_12, _square_stq_rev_13,
    _square_stq_rev_20, _square_stq_rev_21, _square_stq_rev_22, _square_stq_rev_23,
    _square_stq_rev_30, _square_stq_rev_31, _square_stq_rev_32, _square_stq_rev_33,
    _square_stq_lr_l_00, _square_stq_lr_l_01, _square_stq_lr_l_02, _square_stq_lr_l_03,
    _square_stq_lr_l_10, _square_stq_lr_l_11, _square_stq_lr_l_12, _square_stq_lr_l_13,
    _square_stq_lr_l_20, _square_stq_lr_l_21, _square_stq_lr_l_22, _square_stq_lr_l_23,
    _square_stq_lr_l_30, _square_stq_lr_l_31, _square_stq_lr_l_32, _square_stq_lr_l_33,
    _square_stq_lr_r_00, _square_stq_lr_r_01, _square_stq_lr_r_02, _square_stq_lr_r_03,
    _square_stq_lr_r_10, _square_stq_lr_r_11, _square_stq_lr_r_12, _square_stq_lr_r_13,
    _square_stq_lr_r_20, _square_stq_lr_r_21, _square_stq_lr_r_22, _square_stq_lr_r_23,
    _square_stq_lr_r_30, _square_stq_lr_r_31, _square_stq_lr_r_32, _square_stq_lr_r_33,
};

static struct Flame_Initialize_Parameter _flame_init_parm_table[8] = {
    { 80, { 0, 0, 1 }, 1.0f, -0.03f, 0.18f, 80.0f, 190.0f, 0.4f, 1.8f, { 700, 100, 700, 0 }, { 10, 250, 500, 0 }, { 255.0f, 255.0f, 255.0f, 0.0f } },
    { 100, { 0, 0, 1 }, 1.0f, -0.035f, 0.3f, 90.0f, 210.0f, 0.4f, 1.8f, { 100, 100, 700, 0 }, { 10, 350, 700, 0 }, { 255.0f, 255.0f, 255.0f, 0.0f } },
    { 80, { 1, 0, 0 }, 1.0f, -0.03f, 0.18f, 80.0f, 190.0f, 0.4f, 1.8f, { 700, 100, 700, 0 }, { 500, 250, 10, 0 }, { 255.0f, 255.0f, 255.0f, 0.0f } },
    { 80, { 0, 1, 0 }, 1.0f, -0.03f, 0.2f, 100.0f, 170.0f, 0.4f, 1.8f, { 700, 100, 700, 0 }, { 10, 600, 100, 0 }, { 255.0f, 255.0f, 255.0f, 0.0f } },
    { 100, { 0, 1, 0 }, 1.0f, -0.03f, 0.2f, 80.0f, 180.0f, 0.4f, 2.0f, { 700, 100, 700, 0 }, { 10, 700, 100, 0 }, { 255.0f, 255.0f, 255.0f, 0.0f } },
    { 100, { 0, 1, 0 }, 1.0f, -0.03f, 0.25f, 90.0f, 190.0f, 0.4f, 2.0f, { 100, 100, 700, 0 }, { 10, 600, 200, 0 }, { 255.0f, 255.0f, 255.0f, 0.0f } },
    { 70, { 0, 0, 0 }, 1.0f, -0.03f, 0.15f, 128.0f, 120.0f, 0.4f, 1.8f, { 700, 100, 700, 0 }, { 150, 200, 150, 0 }, { 255.0f, 255.0f, 255.0f, 0.0f } },
    { 100, { 0, 0, 0 }, 1.0f, -0.03f, 0.3f, 100.0f, 200.0f, 0.4f, 1.8f, { 900, 300, 900, 0 }, { 400, 300, 400, 0 }, { 255.0f, 255.0f, 255.0f, 0.0f } },
};

static void Particle_Initialize(struct HH_Object_Flame_01_Particle *pParticle, struct Flame_Initialize_Parameter *pParameter);

static unsigned int Object_Initialize(struct HH_Object_Flame_01 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    unsigned int i;
    struct Flame_Initialize_Parameter *pParameter;
    struct HH_Object_Flame_01_Particle *pParticle;
    float ratio;

    pThis->Timer = 0.0f;
    pElement->Option.Int_Value[0] = pElement->Option.Int_Value[0] % 8;
    pParameter = &_flame_init_parm_table[pElement->Option.Int_Value[0]];
    for (i = 0; i < pParameter->ParticleDraw_Num; i++) {
        pParticle = &pThis->Particle[i];
        Particle_Initialize(pParticle, pParameter);
        ratio = (rand() % 100) * 0.01f;
        pParticle->Time = pParticle->Life_Time * ratio;
    }
    pThis->Particle_DrawNum = 100;
    result = 1;
    return result;
}

static void Particle_Initialize(struct HH_Object_Flame_01_Particle *pParticle, struct Flame_Initialize_Parameter *pParameter) {
    float ratio;
    float length_max_pow_f;

    length_max_pow_f = 0.5f * (pParameter->Length_Max[0] + pParameter->Length_Max[2]);
    if (pParameter->Flag[0]) {
        length_max_pow_f = pParameter->Length_Max[0];
    }
    if (pParameter->Flag[1]) {
        length_max_pow_f = pParameter->Length_Max[1];
    }
    if (pParameter->Flag[2]) {
        length_max_pow_f = pParameter->Length_Max[2];
    }
    length_max_pow_f *= length_max_pow_f;
    pParticle->Location[0] = rand() % pParameter->Length_Max[0] - (pParameter->Length_Max[0] >> 1);
    pParticle->Location[1] = rand() % pParameter->Length_Max[1] - (pParameter->Length_Max[1] >> 1);
    pParticle->Location[2] = rand() % pParameter->Length_Max[2] - (pParameter->Length_Max[2] >> 1);
    pParticle->Verocity_0[0] = rand() % pParameter->Verocity_0[0] - (pParameter->Verocity_0[0] >> 1);
    pParticle->Verocity_0[1] = rand() % pParameter->Verocity_0[1] - (pParameter->Verocity_0[1] >> 1);
    pParticle->Verocity_0[2] = rand() % pParameter->Verocity_0[2] - (pParameter->Verocity_0[2] >> 1);
    pParticle->Verocity_0[3] = -2450.0f * pParameter->Gravity_Suppress_Coff;
    sceVu0CopyVector(pParticle->RGBA, pParameter->RGBA);
    pParticle->Time = 0.0f;
    pParticle->Scale = pParameter->Base_Scale;
    pParticle->Convergence_Coff = pParameter->Convergence_Coff;
    pParticle->Texture_Kind = rand() % 64U;
    ratio = sceVu0InnerProduct(pParticle->Location, pParticle->Location) / length_max_pow_f;
    if (ratio > 1.0f) {
        ratio = 1.0f;
    }
    pParticle->Life_Time = pParameter->Base_Time + pParameter->Rand_Time_Width * (1.0f - ratio);
}

static void Particle_Calculator(struct HH_Object_Flame_01_Particle *pParticle, float *Position);

static void Particle_Motion(struct HH_Object_Flame_01_Particle *pParticle, struct Flame_Initialize_Parameter *pParameter, float *Position, float Ratio) {
    float pre_time;
    float life_time_rev;
    float scale_ratio;
    float alpha_ratio;

    if (pParticle->Time > pParticle->Life_Time) {
        Particle_Initialize(pParticle, pParameter);
        pParticle->Life_Time *= Ratio;
        sceVu0ScaleVector(pParticle->Location, pParticle->Location, Ratio);
    }
    pre_time = pParticle->Life_Time * 0.25f;
    life_time_rev = 1.0f / pParticle->Life_Time;
    scale_ratio = 1.0f - pParticle->Time * life_time_rev;
    if (pParticle->Time < pre_time) {
        alpha_ratio = pParticle->Time / pre_time;
    } else {
        alpha_ratio = 1.0f - life_time_rev * (pParticle->Time - pre_time) * 1.3333334f;
    }
    Particle_Calculator(pParticle, Position);
    if (pParticle->Convergence_Coff > 0.0f) {
        pParticle->Convergence_Coff += pParameter->Convergence_Coff_Sub;
    }
    pParticle->Time += 1.0f / 30.0f;
    pParticle->RGBA[3] = Ratio * (pParameter->Base_Alpha * alpha_ratio);
    pParticle->Scale = Ratio * (pParameter->Base_Scale * scale_ratio);
}

static void Particle_Calculator(struct HH_Object_Flame_01_Particle *pParticle, float *Position) {
    float time_mat[4][4];

    sceVu0UnitMatrix(time_mat);
    time_mat[0][0] = time_mat[2][2] = pParticle->Time * pParticle->Convergence_Coff;
    time_mat[1][1] = pParticle->Time;
    time_mat[3][1] = pParticle->Time * pParticle->Time;
    time_mat[3][3] = 0.0f;
    sceVu0ApplyMatrix(Position, time_mat, pParticle->Verocity_0);
}

static unsigned int Object_Draw(struct HH_Object_Flame_01 *pThis, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    sceVif1Packet *pPk;
    float (*pVertex)[4];
    float lwm[4][4];
    float lsm[4][4];
    float _lsm[4][4];
    float clip_mat[4][4];
    float _clip_mat[4][4];
    int xyzf[4];
    int rgba[4];
    float (*pStq)[4];
    int position[4];
    float stq_dummy[4];
    unsigned int i;
    struct HH_Object_Flame_01_Particle *pParticle;
    struct Flame_Initialize_Parameter *pParameter;

    pParameter = &_flame_init_parm_table[pElement->Option.Int_Value[0]];
    pPk = HH_Vif1Packet_Current_Get();
    pVertex = _square_00_vertex;
    HH_ClassWrapper_WorldScreenMatrix_Get(_lsm);
    HH_ClassWrapper_ViewFrustum_Primitive_ClipMatrix_Get(_clip_mat);
    HH_Vif1PacketBuffer_GifTag_Open();
    for (i = 0; i < pParameter->ParticleDraw_Num; i++) {
        pParticle = &pThis->Particle[i];
        Particle_Motion(pParticle, pParameter, (float *)position, pThis->Ratio);
        HH_ClassWrapper_AlwaysFront_WorldView_Matrix_Get(lwm);
        sceVu0ScaleVector(lwm[0], lwm[0], pParticle->Scale);
        sceVu0ScaleVector(lwm[1], lwm[1], pParticle->Scale);
        sceVu0ScaleVector(lwm[2], lwm[2], pParticle->Scale);
        sceVu0TransMatrix(lwm, lwm, pElement->Option.Vector[0]);
        sceVu0TransMatrix(lwm, lwm, pParticle->Location);
        sceVu0TransMatrix(lwm, lwm, (float *)position);
        sceVu0MulMatrix(lsm, _lsm, lwm);
        sceVu0MulMatrix(clip_mat, _clip_mat, lwm);
        pStq = _square_stq_list[pParticle->Texture_Kind];
        sceVu0FTOI0Vector(rgba, pParticle->RGBA);
        HH_Vif1Packet_GeneralGifTag_Sprite_Open();
        ((u_long128 *)pPk->pCurrent)[0] = *(u_long128 *)rgba;
        HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq_dummy, lsm, clip_mat, pVertex[0], 0x3F);
        ((u_long128 *)pPk->pCurrent)[1] = *(u_long128 *)pStq[0];
        xyzf[2] += 800;
        ((u_long128 *)pPk->pCurrent)[2] = *(u_long128 *)xyzf;
        HH_ClassWrapper_Transform_PerspectiveProjection_Clip_N(xyzf, stq_dummy, lsm, clip_mat, pVertex[3], 0x3F);
        ((u_long128 *)pPk->pCurrent)[3] = *(u_long128 *)pStq[3];
        xyzf[2] += 800;
        ((u_long128 *)pPk->pCurrent)[4] = *(u_long128 *)xyzf;
        pPk->pCurrent += 20;
        sceVif1PkCloseGifTag(pPk);
    }
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 0;
    return result;
}

/**
 * Class prefix, run once a frame before this class's instances: sets the GS TEX0, CLAMP and ALPHA
 * registers they draw with. Returns 1.
 */
unsigned int HH_Class_Prefix_Flame_01(void) {
    unsigned int result;
    sceVif1Packet *pPk;
    unsigned long tex0;

    pPk = HH_Vif1Packet_Current_Get();
    HH_Vif1PacketBuffer_GifTag_Open();
    tex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(3, 0);
    sceVif1PkAddGsAD(pPk, GS_REG_TEX0_1, tex0);
    sceVif1PkAddGsAD(pPk, GS_REG_CLAMP_1, 0x1FF001FFF);
    sceVif1PkAddGsAD(pPk, GS_REG_ALPHA_1, GS_SET_ALPHA(0, 1, 0, 1, 0x80));
    sceVif1PkCloseGifTag(pPk);
    HH_Vif1PacketBuffer_GifTag_Close();
    result = 1;
    return result;
}

/**
 * Class suffix, run once a frame after this class's instances: opens a general A+D GIF tag in the
 * packet. Returns 1.
 */
unsigned int HH_Class_Suffix_Flame_01(void) {
    unsigned int result;

    HH_Vif1Packet_GeneralGifTag_GS_AD_Open();
    result = 1;
    return result;
}

/**
 * Class main: HH_Class_Flame_00 with a fire that grows to full size over a given time.
 * @param pBlock   the instance's data block (struct HH_Object_Flame_01)
 * @param pElement the element that created it; Option.Vector[0] is the position,
 *                 Int_Value[0] the parameter set (taken modulo 8), Float_Value[0] the burning
 *                 time (0: no limit) and Float_Value[1] the time to reach full size
 * @return 1 while the instance runs, 0 once it has ended
 */
unsigned int HH_Class_Flame_01(void *pBlock, struct ImpactQueue_Element *pElement) {
    unsigned int result;
    struct HH_Object_Flame_01 *pThis;

    result = 1;
    pThis = pBlock;
    switch (pThis->Step) {
    case 0:
        Object_Initialize(pThis, pElement);
        if (pElement->Option.Float_Value[0] == 0.0f) {
            pThis->Step = 2;
        } else {
            pThis->Step = 1;
        }
        break;
    case 1:
        if (pThis->Timer > pElement->Option.Float_Value[0]) {
            pThis->Step = 3;
        }
    case 2:
        if (pThis->Timer > pElement->Option.Float_Value[1]) {
            pThis->Ratio = 1.0f;
        } else {
            pThis->Ratio = pThis->Timer / pElement->Option.Float_Value[1];
        }
        Object_Draw(pThis, pElement);
        pThis->Timer += 1.0f / 30.0f;
        break;
    case 3:
    default:
        pThis->Header.Enable = 0;
        result = 0;
        break;
    }
    return result;
}
