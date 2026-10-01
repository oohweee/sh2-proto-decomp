/*
 * Lens flare logic (Lens): per frame, projects James's flashlight (or the room's spot light,
 * and a reversed second light) to the screen, works out its screen angle and distance, and
 * sets the target flare strength; the flare's visibility is tested against the Z buffer.
 * Drawing is in kari_lf_draw.c.
 *
 * Compiled with inlining off (config/file_flags.txt): the header inline functions it calls
 * (fi_libvu0_inline.h, fi_calc.h, GFW/sh2_get_drawenv.h) are emitted out of line, each right
 * after its first caller, as in the original.
 */
#include "sh2.h"
#include "fi_libvu0_inline.h"
#include "fi_calc.h"
#include "GFW/sh2_get_drawenv.h"
#include "libc/math.h"
#include "sdk/libvu0.h"

/* The lens flare texture (texture head followed by its CLUT head), in the data segment. */
extern unsigned char D_002AE310[];

struct sh2gfw_Effect_Man LF_Tex_Work;
struct shLensFlareWork light_flare_work[2];
struct shLensFlareLightInfo light_info[2];
struct shLensFlareScreenInfo screen_info;
float reverse_light_rate;
static int lf_flicker_on;

static void shLensFlareGetScreenInfo(void) {
    screen_info.center_x = VbScreenInfo.cx;
    screen_info.center_y = VbScreenInfo.cy;
    screen_info.width = VbScreenInfo.sx;
    screen_info.height = VbScreenInfo.sy;
}

/** Returns 1 unless bit 0 of demo_status is set. */
int shLensFlareCameraIsSmooth(void) {
    return !(demo_status & 1);
}

/**
 * Returns whether the light's screen position is unoccluded: reads that pixel's depth back from
 * the GS Z buffer and compares it with the light's depth. Visibility must hold for a few
 * frames in a row before 1 is returned.
 */
int shLensFlareLightCenterIsVisible(struct shLensFlareWork *lf_info) {
    struct sceGsStoreImage *StoreIm;
    unsigned short *now_z_value;
    unsigned int zbuffer;
    int zval;

    if (lf_info->draw_center_f == 0) { return 0; }
    StoreIm = (struct sceGsStoreImage *)0x70000000;
    sh2gfw_GsSetDefStoreImage(StoreIm, 0x3800, 8, 0x3A, (lf_info->l_screen_pos.x >> 4) - 0x700, (lf_info->l_screen_pos.y >> 4) - 0x700, 1, 0x40);
    now_z_value = (unsigned short *)0x70002000;
    sh2gfw_GsExecStoreImage(StoreIm, (u_long128 *)now_z_value);
    zbuffer = *now_z_value;
    zval = zbuffer;
    if (zval <= lf_info->l_screen_pos.z) {
        if (lf_flicker_on > 3) {
            return 1;
        } else {
            lf_flicker_on++;
            return 0;
        }
    } else {
        lf_flicker_on = 0;
        return 0;
    }
}

static void shLensFlareSetLightSeed(struct shLensFlareWork *lf_work, struct shLensFlareScreenInfo *sc_info, int type) {
    struct IVEC vi0;
    float wsm[4][4];

    sh2gde_getWorldScreenMatrix(wsm);
    shLensFlareGetScreenInfo();
    _sceVu0RotTransPers((int *)&vi0, wsm, (float *)&light_info[type].world_light_pos, 1);
    lf_work->l_screen_pos = vi0;
    lf_work->draw_center_f = lf_work->scr_l_pos.x >= sc_info->center_x - sc_info->width / 2.0f &&
                             lf_work->scr_l_pos.x <= sc_info->center_x + sc_info->width / 2.0f &&
                             lf_work->scr_l_pos.y >= sc_info->center_y - sc_info->height / 2.0f &&
                             lf_work->scr_l_pos.y <= sc_info->center_y + sc_info->height / 2.0f &&
                             lf_work->l_screen_pos.z >= 0 && lf_work->scr_l_ang_z >= 1.3613569f;
}

static float shLensFlareOresenHokan(float *Y_ary, int Y_suu, float input_X, float min_X, float max_X) {
    float output_Y;
    float amari;
    float kukan_w;
    int kukan_no;
    float tmp;

    if (input_X >= max_X) {
        output_Y = Y_ary[Y_suu - 1];
    } else if (input_X < min_X) {
        output_Y = Y_ary[0];
    } else {
        kukan_w = (max_X - min_X) / (Y_suu - 1);
        amari = input_X - min_X;
        kukan_no = amari / kukan_w;
        if (kukan_no >= Y_suu - 1) {
            output_Y = Y_ary[Y_suu - 1];
        } else if (kukan_no < 0) {
            output_Y = Y_ary[0];
        } else {
            tmp = amari;
            while (amari >= kukan_w) {
                amari -= kukan_w;
            }
            output_Y = (Y_ary[kukan_no + 1] * amari + Y_ary[kukan_no] * (kukan_w - amari)) / kukan_w;
        }
    }
    return output_Y;
}

/* Matching: float code before this function sets the argument order of the three calls (fitted, not recovered). */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 0.1f; }
static float shLensFlareMakeEffectTargetRate(float light_eff_pow, struct shLensFlareWork *lf_work) {
    static float pow_rate_dat[2] = { 0.0f, 1.0f };
    static float dist_rate_dat[7] = { 4.0f, 1.3f, 0.9f, 0.7f, 0.5f, 0.35f, 0.2f };
    static float ang_z_rate_dat[7] = { 0.15f, 0.2f, 0.25f, 0.3f, 0.35f, 0.5f, 1.3f };
    float ret_tgt_rate;
    float pow_rate;
    float dist_rate;
    float ang_z_rate;

    pow_rate = shLensFlareOresenHokan(pow_rate_dat, 2, light_eff_pow, 0.0f, 1.0f);
    dist_rate = shLensFlareOresenHokan(dist_rate_dat, 7, lf_work->scr_l_pos.z, 204.8f, 3328.0);
    ang_z_rate = shLensFlareOresenHokan(ang_z_rate_dat, 7, lf_work->scr_l_ang_z, 1.3613569f, 2.9147);
    ret_tgt_rate = dist_rate * (ang_z_rate * pow_rate);
    return (2.0f > ret_tgt_rate) ? ret_tgt_rate : 2.0f;
}

static void shLensFlareMakeScreenPos(struct shLensFlareWork *lf_work, struct FVEC *ws_l_sxyz_p, struct FVEC *ws_l_vec_p, int scr_z) {
    float geom_x;
    float geom_y;
    struct FVEC tmp_pos;
    float tmp_vec_z;

    if (ws_l_sxyz_p->z > scr_z / 2) {
        tmp_pos = *ws_l_sxyz_p;
    } else {
        tmp_vec_z = ws_l_vec_p->z;
        if (tmp_vec_z == 0.0f) {
            tmp_vec_z = 1.0f;
        }
        tmp_pos.z = scr_z / 2;
        tmp_pos.x = ws_l_sxyz_p->x + ws_l_vec_p->x * (tmp_pos.z - ws_l_sxyz_p->z) / tmp_vec_z;
        tmp_pos.y = ws_l_sxyz_p->y + ws_l_vec_p->y * (tmp_pos.z - ws_l_sxyz_p->z) / tmp_vec_z;
    }
    geom_x = VbScreenInfo.cx;
    geom_y = VbScreenInfo.cy;
    lf_work->scr_l_pos.z = tmp_pos.z;
    lf_work->scr_l_pos.x = geom_x + tmp_pos.x * scr_z / tmp_pos.z;
    lf_work->scr_l_pos.y = geom_y + tmp_pos.y * scr_z / tmp_pos.z;
}

static void shLensFlareMakeScreenAngle(struct shLensFlareWork *lf_work, struct FVEC *ws_l_sxyz_p, struct FVEC *ws_l_vec_p) {
    struct FVEC one_pos;
    struct FVEC one_l_vec;
    struct FVEC one_op_vec;
    float cos_z;
    float sin_z;
    struct FVEC vec;

    ktVectorNormal(&one_pos, ws_l_sxyz_p);
    ktVectorNormal(&one_l_vec, ws_l_vec_p);
    sceVu0OuterProduct((float *)&one_op_vec, (float *)&one_pos, (float *)&one_l_vec);
    ktVectorNormal(&one_op_vec, &one_op_vec);
    lf_work->scr_l_ang_xy = atan2f(one_op_vec.y, one_op_vec.x);
    cos_z = sceVu0InnerProduct((float *)&one_pos, (float *)&one_l_vec);
    sceVu0OuterProduct((float *)&vec, (float *)&one_op_vec, (float *)&one_pos);
    sin_z = sceVu0InnerProduct((float *)&one_l_vec, (float *)&vec);
    lf_work->scr_l_ang_z = atan2f(sin_z, cos_z);
}

static void shGetJamesLightInfo(struct SubCharacter *scp, struct shLensFlareLightInfo *l_info, int type) {
    if (sh2gde_CheckSpot_JmsOrBG()) {
        if (type) {
            shGetJamesLightPosOriginal_Reverse((float *)&l_info->world_light_pos, (float *)&l_info->world_light_vector);
        } else {
            shGetJamesLightPosOriginal((float *)&l_info->world_light_pos, (float *)&l_info->world_light_vector);
        }
    } else {
        sceVu0CopyVector((float *)&l_info->world_light_pos, *sh2gde_Get_BGSpotPos());
        sceVu0CopyVector((float *)&l_info->world_light_vector, *sh2gde_Get_BGSpotDir());
        l_info->world_light_pos.y += 50.0f * l_info->world_light_vector.y - 3.0f;
        l_info->world_light_pos.z += 50.0f * l_info->world_light_vector.z - 20.0f;
    }
    l_info->light_pow = type ? 0.6f : 1.0f;
}

static void shLensFlareMakeScreenInfo(struct shLensFlareWork *lf_work, struct shLensFlareLightInfo *l_info) {
    struct FVEC ws_l_pos;
    struct FVEC ws_l_vec;
    float wsm[4][4];

    sh2gde_getWorldViewMatrix(wsm);
    sceVu0ApplyMatrix((float *)&ws_l_pos, wsm, (float *)&l_info->world_light_pos);
    sceVu0ApplyMatrix((float *)&ws_l_vec, wsm, (float *)&l_info->world_light_vector);
    shLensFlareMakeScreenAngle(lf_work, &ws_l_pos, &ws_l_vec);
    shLensFlareMakeScreenPos(lf_work, &ws_l_pos, &ws_l_vec, VbScreenInfo.scr_z);
}

/** Resets both lens flares and registers the flare texture. */
void shLensFlareInit(void) {
    int i;
    struct sh2gfw_TEX_HEAD *pTH;
    struct sh2gfw_CLUTS_HEAD *pCH;

    for (i = 0; i < 2; i++) {
        light_flare_work[i].flare_inhibit_f = 0;
        light_flare_work[i].lfl_sync_draw_func_exec_f = 0;
        light_flare_work[i].now_l_eff_rate = 0.0f;
    }
    pTH = (struct sh2gfw_TEX_HEAD *)D_002AE310;
    pCH = (struct sh2gfw_CLUTS_HEAD *)((char *)pTH + pTH->allsize);
    LF_Tex_Work.pTexMAN = sh2gfw_set_TexToTrasMan(&AllTexSync_Man, pTH, pCH, &LF_Tex_Work, 0xEE00);
    LF_Tex_Work.valid_id = 0xEF04;
}

/**
 * Per-frame lens flare update for one light: gets its position and direction, projects it,
 * and, when it is in front of the camera, sets the target flare strength and screen seed.
 * @param scp             James
 * @param light_intensity not used (the flashlight color is read instead)
 * @param type            0: the flashlight; 1: its reversed counterpart
 */
void shLensFlareExec(struct SubCharacter *scp, float light_intensity, int type) {
    int count;
    int proj;

    count = *T0_COUNT; /* unused, as in the original */
    light_intensity = 3.0f * Env_ctl.SpotL0.color.fl32[0] / 7.0f;
    shGetJamesLightInfo(scp, &light_info[type], type);
    if (light_flare_work[type].flare_inhibit_f == 0) {
        shLensFlareMakeScreenInfo(&light_flare_work[type], &light_info[type]);
        if (type) {
            reverse_light_rate = (light_flare_work[type].scr_l_ang_z < 1.5707964f) ? 0.0f : 0.63661975f * (light_flare_work[type].scr_l_ang_z - 1.5707964f);
        }
        proj = VbScreenInfo.scr_z;
        if (light_flare_work[type].scr_l_pos.z > proj / 2) {
            light_flare_work[type].lfl_sync_draw_func_exec_f = 1;
            light_flare_work[type].tgt_l_eff_rate = shLensFlareMakeEffectTargetRate(light_intensity, &light_flare_work[type]);
            shLensFlareSetLightSeed(&light_flare_work[type], &screen_info, type);
        }
    }
}
