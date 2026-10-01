/*
 * Lens flare drawing ("kari": provisional; Lens): fades the flare strength toward its target
 * while the light is visible, and builds the VIF1 packet of textured sprites that make up the
 * flare, sent after the flare texture.
 */
#include "sh2.h"
#include "hh_math.h"
#include "libc/math.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"

#define EFF_VALID_ID 0xEF04

/*
 * sqrt(x) with sqrt.s, computed in place ("+f"). Matching: kept local: sh_vu0.h's _shSqrt takes a
 * separate output and changes kari_shLensFlareEffect_Draw's code.
 */
static inline float _shSqrtIP(float x) {
    __asm__ __volatile__("
    sqrt.s %0, %0
    " : "+f"(x));
    return x;
}

u_long128 kari_kick_packet[0x500];

static void shLensFlareSpriteAddPacketGif(sceVif1Packet *packet, IVEC c, IVEC v0, IVEC v1, FVEC st0, FVEC st1);
static void *kari_shLensFlareEffect_Draw(int center_visible_f, struct shLensFlareWork *lf_work, struct shLensFlareScreenInfo *sc_info, int mode);

/** Sends the flare texture to a GS texture slot (DMA channel 2) and records its TEX0. */
/* Matching: the #line keeps the assert string below on the original's line. */
#line 192
void kari_Thr_LFD2TextureSend(void) {
    assert_dw(LF_Tex_Work.valid_id==EFF_VALID_ID);
    sh2gfw_Thr_d2TextureSend(LF_Tex_Work.pTexMAN, 0, &LF_Tex_Work.thr_cid, &LF_Tex_Work.thr_sid);
    LF_Tex_Work.Tex0Data = *sh2gfw_Get_RegTEX0(LF_Tex_Work.pTexMAN, 0, 1);
}

/** Sends the flare packet pktop on DMA channel 1 once the flare texture transfer is done. */
void kari_Thr_LFD1D2SyncKick(void *pktop) {
    sh2gfw_Thr_d1d2SyncKick(pktop, LF_Tex_Work.thr_cid, LF_Tex_Work.thr_sid);
}

static void shLensFlarePolyFT4AddPacketGif(sceVif1Packet *packet, IVEC c, IVEC v0, IVEC v1, IVEC v2, IVEC v3, FVEC st0, FVEC st1, FVEC st2, FVEC st3) {
    unsigned long giftag_polyf4[2] = { 0xE400000000008000, 0x0051251251251260 };
    float q;
    u_long128 *pTex0;

    q = 1.0f;
    sceVif1PkOpenGifTag(packet, *(u_long128 *)giftag_polyf4);
    sceVif1PkAddGsData(packet, 0x5C);
    pTex0 = &LF_Tex_Work.Tex0Data;
    sceVif1PkAddGsData(packet, *(unsigned long *)pTex0);
    sceVif1PkAddGsData(packet, GS_SET_ST(Float_Bits(st0.x * q), Float_Bits(st0.y * q)));
    sceVif1PkAddGsData(packet, GS_SET_RGBAQ(c.x, c.y, c.z, 0x80, *(unsigned int *)&q));
    sceVif1PkAddGsData(packet, GS_SET_XYZ(v0.x, v0.y, v0.z));
    sceVif1PkAddGsData(packet, GS_SET_ST(Float_Bits(st1.x * q), Float_Bits(st1.y * q)));
    sceVif1PkAddGsData(packet, GS_SET_RGBAQ(c.x, c.y, c.z, 0x80, *(unsigned int *)&q));
    sceVif1PkAddGsData(packet, GS_SET_XYZ(v1.x, v1.y, v1.z));
    sceVif1PkAddGsData(packet, GS_SET_ST(Float_Bits(st2.x * q), Float_Bits(st2.y * q)));
    sceVif1PkAddGsData(packet, GS_SET_RGBAQ(c.x, c.y, c.z, 0x80, *(unsigned int *)&q));
    sceVif1PkAddGsData(packet, GS_SET_XYZ(v2.x, v2.y, v2.z));
    sceVif1PkAddGsData(packet, GS_SET_ST(Float_Bits(st3.x * q), Float_Bits(st3.y * q)));
    sceVif1PkAddGsData(packet, GS_SET_RGBAQ(c.x, c.y, c.z, 0x80, *(unsigned int *)&q));
    sceVif1PkAddGsData(packet, GS_SET_XYZ(v3.x, v3.y, v3.z));
    sceVif1PkCloseGifTag(packet);
}

#define FIX4(x) (16.0f * (x))
#define ASPECT(y) (512.0f * (y) / 448.0f)

static void shLensFlareDrawCommon(sceVif1Packet *packet, struct shLensFlareWork *lf_work, struct shLensFlareScreenInfo *sc_info, IVEC *base_color, float base_r, float base_vector, FVEC st0, FVEC st1, unsigned short z_value) {
    IVEC color;
    IVEC prim_p[4];
    float r;
    float _rate;

    _rate = lf_work->now_l_eff_rate;
    color.x = base_color->x * _rate;
    color.y = base_color->y * _rate;
    color.z = base_color->z * _rate;
    if (color.x > 255) {
        color.x = 255;
    }
    if (color.y > 255) {
        color.y = 255;
    }
    if (color.z > 255) {
        color.z = 255;
    }
    r = base_r * ((3.0f + lf_work->now_l_eff_rate) / 4.0f);
    prim_p[0].x = FIX4(sc_info->center_x + base_vector * (lf_work->scr_l_pos.x - sc_info->center_x) - r);
    prim_p[0].y = FIX4(sc_info->center_y + base_vector * (lf_work->scr_l_pos.y - sc_info->center_y) - ASPECT(r));
    prim_p[1].x = FIX4(r + (sc_info->center_x + base_vector * (lf_work->scr_l_pos.x - sc_info->center_x)));
    prim_p[1].y = FIX4(ASPECT(r) + (sc_info->center_y + base_vector * (lf_work->scr_l_pos.y - sc_info->center_y)));
    prim_p[0].z = prim_p[1].z = z_value;
    shLensFlareSpriteAddPacketGif(packet, color, prim_p[0], prim_p[1], st0, st1);
}

static void shLensFlareGetScreenInfo(void) {
    screen_info.center_x = VbScreenInfo.cx;
    screen_info.center_y = VbScreenInfo.cy;
    screen_info.width = VbScreenInfo.sx;
    screen_info.height = VbScreenInfo.sy;
}

/**
 * Sends the flare texture, moves the flare strength 30% toward its target (or toward 0 when
 * the light is hidden) and builds the flare packet.
 * @return the packet, or NULL when no flare is drawn this frame
 */
void *kari_shLensFlareDraw(void) {
    int center_visible_f;
    int count;
    float add_rate;

    count = *T0_COUNT;
    kari_Thr_LFD2TextureSend();
    if (light_flare_work[0].lfl_sync_draw_func_exec_f == 0) {
        return NULL;
    }
    light_flare_work[0].lfl_sync_draw_func_exec_f = 0;
    if (light_flare_work[0].flare_inhibit_f) {
        return NULL;
    }
    if (!((item.flag[0] >> 15) & 1) && sh2gde_CheckSpot_JmsOrBG()) {
        return NULL;
    }
    center_visible_f = shLensFlareLightCenterIsVisible(light_flare_work);
    add_rate = (center_visible_f ? light_flare_work[0].tgt_l_eff_rate - light_flare_work[0].now_l_eff_rate : -light_flare_work[0].now_l_eff_rate) * 0.3;
    light_flare_work[0].now_l_eff_rate += add_rate;
    if (!shLensFlareCameraIsSmooth()) {
        light_flare_work[0].now_l_eff_rate = 0.0f;
        center_visible_f = 0;
    }
    return kari_shLensFlareEffect_Draw(center_visible_f, light_flare_work, &screen_info, 0);
}

static void shLensFlareSetAlphaEnvironment(sceVif1Packet *packet) {
    unsigned long giftag_alpha[2] = { 0x1000000000008000, 0xE };

    sceVif1PkOpenGifTag(packet, *(u_long128 *)giftag_alpha);
    sceVif1PkAddGsAD(packet, 0x3F, 0);
    sceVif1PkAddGsAD(packet, 0x42, 0x8000000048);
    sceVif1PkAddGsAD(packet, 0x47, 0x50000);
    sceVif1PkAddGsAD(packet, 0x4E, 0x13A0001C0);
    sceVif1PkCloseGifTag(packet);
}

#define CLAMP_COLOR(c)      \
    if ((c).x > 255) {      \
        (c).x = 255;        \
    }                       \
    if ((c).y > 255) {      \
        (c).y = 255;        \
    }                       \
    if ((c).z > 255) {      \
        (c).z = 255;        \
    }

static void *kari_shLensFlareEffect_Draw(int center_visible_f, struct shLensFlareWork *lf_work, struct shLensFlareScreenInfo *sc_info, int mode) {
    IVEC color;
    IVEC prim_p[4];
    FVEC prim_st[7][2] = {
        { { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.5f, 0.5f, 0.0f, 0.0f } },
        { { 0.5f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.5f, 0.0f, 0.0f } },
        { { 0.25f, 0.5f, 0.0f, 0.0f }, { 0.625f, 1.0f, 0.0f, 0.0f } },
        { { 0.625f, 0.5f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f, 0.0f } },
        { { 0.0547f, 0.5547f, 0.0f, 0.0f }, { 0.25f, 0.5547f, 0.0f, 0.0f } },
        { { 0.0547f, 1.0f, 0.0f, 0.0f }, { 0.25f, 1.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.5f, 0.0f, 0.0f }, { 0.25f, 1.0f, 0.0f, 0.0f } },
    };
    float hensin_x;
    float hensin_y;
    float r;
    float hensin_rate_x;
    float hensin_rate_y;
    int i2;
    int i1;
    int no;
    float add_flare_ang;
    float add_mask_ang;
    float line_r;
    sceVif1Packet *vif1pk;
    sceVif1Packet vif1packet;
    FVEC pos[4];
    int repeat_num;
    int flare_drawn;
    float base_vector[7] = { 0.4f, 0.2f, -0.3f, -0.7f, -1.4f, -2.2f, -2.5f };
    float base_r[7] = { 12.0f, 10.0f, 28.0f, 20.0f, 20.0f, 58.0f, 50.0f };
    int base_st_index[7] = { 3, 2, 2, 3, 2, 3, 6 };
    unsigned short z_value[7] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
    IVEC base_color[7] = {
        { 0x48, 0x30, 0x48, 0 },
        { 0x28, 0x2C, 0x2C, 0 },
        { 0x28, 0x2E, 0x28, 0 },
        { 0x24, 0x24, 0x24, 0 },
        { 0x28, 0x28, 0x20, 0 },
        { 0x40, 0x30, 0x28, 0 },
        { 0x80, 0x80, 0x80, 0 },
    };

    vif1pk = &vif1packet;
    sceVif1PkInit(vif1pk, kari_kick_packet);
    sceVif1PkCnt(vif1pk, 0);
    sceVif1PkAddCode(vif1pk, 0x11000000);
    sceVif1PkOpenDirectCode(vif1pk, 0);
    shLensFlareGetScreenInfo();
    if (PlayerReverseLightCalcIsOn()) {
        repeat_num = 2;
    } else {
        repeat_num = 1;
    }
    flare_drawn = 0;
    for (i1 = 0; i1 < repeat_num; i1++, lf_work++) {
        if (lf_work->scr_l_pos.x >= sc_info->center_x - sc_info->width && lf_work->scr_l_pos.x <= sc_info->center_x + sc_info->width &&
            lf_work->scr_l_pos.y >= sc_info->center_y - sc_info->height && lf_work->scr_l_pos.y <= sc_info->center_y + sc_info->height) {
            flare_drawn = 1;
            line_r = 128.0f - _shSqrtIP(lf_work->scr_l_pos.z * 1.5);
            if (line_r < 0.0f) {
                line_r = 0.0f;
            }
            shLensFlareSetAlphaEnvironment(vif1pk);
            hensin_rate_x = sinf(lf_work->scr_l_ang_xy) * cosf(lf_work->scr_l_ang_xy);
            hensin_rate_y = sinf(lf_work->scr_l_ang_z) * sinf(lf_work->scr_l_ang_xy);
            if (Env_ctl.SpotL0.color.fl32[0] >= 0.002f) {
                hensin_x = 5.0f * hensin_rate_x;
                hensin_y = 5.0f * hensin_rate_y;
                {
                    float _rate;

                    _rate = (lf_work->now_l_eff_rate + 3.0) * 0.25;
                    color.x = 40.0f * _rate;
                    color.y = 40.0f * _rate;
                    color.z = 38.0f * _rate;
                    CLAMP_COLOR(color);
                    if (i1) {
                        color.x = color.x * reverse_light_rate;
                        color.y = color.y * reverse_light_rate;
                        color.z = color.z * reverse_light_rate;
                    }
                    r = line_r * (0.25f * (2.0f + lf_work->now_l_eff_rate));
                    prim_p[0].x = lf_work->l_screen_pos.x - (int)FIX4(-hensin_x + r);
                    prim_p[0].y = lf_work->l_screen_pos.y - (int)ASPECT(FIX4(-hensin_y + r));
                    prim_p[1].x = lf_work->l_screen_pos.x + (int)FIX4(hensin_x + r);
                    prim_p[1].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(hensin_y + r));
                    prim_p[0].w = prim_p[1].w = prim_p[2].w = prim_p[3].w = 1;
                    if (center_visible_f && i1 == 0) {
                        prim_p[0].z = prim_p[1].z = 0xFFFF;
                    } else if (lf_work->scr_l_ang_z > 1.5707964f) {
                        prim_p[0].z = prim_p[1].z = lf_work->l_screen_pos.z;
                    } else {
                        prim_p[0].z = prim_p[1].z = lf_work->l_screen_pos.z;
                    }
                    shLensFlareSpriteAddPacketGif(vif1pk, color, prim_p[0], prim_p[1], prim_st[0][0], prim_st[0][1]);
                    color.x = color.y = color.z = 0x20;
                    if (i1) {
                        color.x = color.x * reverse_light_rate;
                        color.y = color.y * reverse_light_rate;
                        color.z = color.z * reverse_light_rate;
                    }
                    prim_p[0].x = lf_work->l_screen_pos.x - 0xB0;
                    prim_p[0].y = lf_work->l_screen_pos.y - 0xC9;
                    prim_p[1].x = lf_work->l_screen_pos.x + 0xB0;
                    prim_p[1].y = lf_work->l_screen_pos.y + 0xC9;
                    if (center_visible_f && i1 == 0) {
                        prim_p[0].z = prim_p[1].z = 0xFFFF;
                    } else if (lf_work->scr_l_ang_z > 1.5707964f) {
                        prim_p[0].z = prim_p[1].z = lf_work->l_screen_pos.z;
                    } else {
                        prim_p[0].z = prim_p[1].z = lf_work->l_screen_pos.z;
                    }
                    shLensFlareSpriteAddPacketGif(vif1pk, color, prim_p[0], prim_p[1], prim_st[0][0], prim_st[0][1]);
                }
                hensin_x = 9.0f * hensin_rate_x;
                hensin_y = 9.0f * hensin_rate_y;
                {
                    float _rate;

                    _rate = 2.0f * lf_work->now_l_eff_rate;
                    _rate = (1.0f > _rate) ? _rate : 1.0f;
                    color.x = 40.0f * _rate;
                    color.y = 40.0f * _rate;
                    color.z = 40.0f * _rate;
                    CLAMP_COLOR(color);
                    if (i1) {
                        color.x = color.x * reverse_light_rate;
                        color.y = color.y * reverse_light_rate;
                        color.z = color.z * reverse_light_rate;
                    }
                    r = 2.0f * (0.5f * line_r * ((3.0f + lf_work->now_l_eff_rate) / 4.0f));
                    prim_p[0].x = lf_work->l_screen_pos.x - (int)FIX4(-hensin_x + r);
                    prim_p[0].y = lf_work->l_screen_pos.y - (int)ASPECT(FIX4(-hensin_y + r));
                    prim_p[1].x = lf_work->l_screen_pos.x + (int)FIX4(hensin_x + r);
                    prim_p[1].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(hensin_y + r));
                    prim_p[0].z = prim_p[1].z = 0xFFFF;
                    shLensFlareSpriteAddPacketGif(vif1pk, color, prim_p[0], prim_p[1], prim_st[1][0], prim_st[1][1]);
                }
            }
            if (lf_work->now_l_eff_rate > 0.001f && i1 == 0) {
                pos[0].x = -(line_r * 0.061);
                pos[0].y = -(line_r * 0.03);
                pos[1].x = line_r - line_r * 0.061;
                pos[1].y = -(line_r * 0.03);
                pos[2].x = -(line_r * 0.061);
                pos[2].y = line_r - line_r * 0.03;
                pos[3].x = line_r - line_r * 0.061;
                pos[3].y = line_r - line_r * 0.03;
                i2 = 0;
                add_flare_ang = (lf_work->scr_l_pos.x + lf_work->scr_l_pos.y) * 0.006136 + -lf_work->scr_l_ang_z * 0.5f + 1.0471976f;
                add_mask_ang = (lf_work->scr_l_pos.x + lf_work->scr_l_pos.y) * 0.024544 + lf_work->scr_l_pos.z * 0.006136;
                for (; i2 < 4; i2++) {
                    static float ang_dat[4] = { -3.1415927f, -1.5707964f, 0.0f, 1.5707964f };
                    float ang;
                    float sin_val;
                    float cos_val;
                    float _rate;

                    ang = add_flare_ang + ang_dat[i2];
                    sin_val = sinf(ang);
                    cos_val = cosf(ang);
                    prim_p[0].z = prim_p[1].z = prim_p[2].z = prim_p[3].z = 0xFFFF;
                    _rate = lf_work->now_l_eff_rate;
                    color.x = 28.0f * _rate;
                    color.y = 28.0f * _rate;
                    color.z = 28.0f * _rate;
                    CLAMP_COLOR(color);
                    prim_p[0].x = lf_work->l_screen_pos.x + (int)FIX4(pos[0].x * cos_val - pos[0].y * sin_val);
                    prim_p[0].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(pos[0].x * sin_val + pos[0].y * cos_val));
                    prim_p[1].x = lf_work->l_screen_pos.x + (int)FIX4(pos[1].x * cos_val - pos[1].y * sin_val);
                    prim_p[1].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(pos[1].x * sin_val + pos[1].y * cos_val));
                    prim_p[2].x = lf_work->l_screen_pos.x + (int)FIX4(pos[2].x * cos_val - pos[2].y * sin_val);
                    prim_p[2].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(pos[2].x * sin_val + pos[2].y * cos_val));
                    prim_p[3].x = lf_work->l_screen_pos.x + (int)FIX4(pos[3].x * cos_val - pos[3].y * sin_val);
                    prim_p[3].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(pos[3].x * sin_val + pos[3].y * cos_val));
                    shLensFlarePolyFT4AddPacketGif(vif1pk, color, prim_p[0], prim_p[1], prim_p[2], prim_p[3], prim_st[4][0], prim_st[4][1], prim_st[5][0], prim_st[5][1]);
                }
                i2 = 0;
                add_flare_ang = (lf_work->scr_l_pos.x + lf_work->scr_l_pos.y) * 0.006136 + -lf_work->scr_l_ang_z / 2.0f + 1.0471976f;
                add_mask_ang = (lf_work->scr_l_pos.x + lf_work->scr_l_pos.y) * 0.024544 + lf_work->scr_l_pos.z * 0.006136;
                for (; i2 < 12; i2++) {
                    static float ang_dat[12] = { -2.9321532f, -2.268928f, -1.0471976f, -0.87266463f, -0.2617994f, 0.34906587f,
                                                 0.69813174f, 1.2217306f, 2.1293018f, 2.5656343f, 2.8797934f, 0.0f };
                    float ang;
                    float rate;
                    float sin_val;
                    float cos_val;
                    float _rate;

                    ang = add_flare_ang + ang_dat[i2];
                    rate = (0.85f + cosf(ang - lf_work->scr_l_ang_xy)) * (0.5f * (0.5f + cosf(12.0f * ang + add_mask_ang)));
                    rate = (0.75f > rate) ? rate : 0.75f;
                    sin_val = sinf(ang);
                    cos_val = cosf(ang);
                    prim_p[0].z = prim_p[1].z = prim_p[2].z = prim_p[3].z = 0xFFFF;
                    _rate = lf_work->now_l_eff_rate * rate;
                    color.x = 16.0f * _rate;
                    color.y = 16.0f * _rate;
                    color.z = 16.0f * _rate;
                    CLAMP_COLOR(color);
                    prim_p[0].x = lf_work->l_screen_pos.x + (int)FIX4(pos[0].x * cos_val - pos[0].y * sin_val);
                    prim_p[0].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(pos[0].x * sin_val + pos[0].y * cos_val));
                    prim_p[1].x = lf_work->l_screen_pos.x + (int)FIX4(pos[1].x * cos_val - pos[1].y * sin_val);
                    prim_p[1].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(pos[1].x * sin_val + pos[1].y * cos_val));
                    prim_p[2].x = lf_work->l_screen_pos.x + (int)FIX4(pos[2].x * cos_val - pos[2].y * sin_val);
                    prim_p[2].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(pos[2].x * sin_val + pos[2].y * cos_val));
                    prim_p[3].x = lf_work->l_screen_pos.x + (int)FIX4(pos[3].x * cos_val - pos[3].y * sin_val);
                    prim_p[3].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(pos[3].x * sin_val + pos[3].y * cos_val));
                    shLensFlarePolyFT4AddPacketGif(vif1pk, color, prim_p[0], prim_p[1], prim_p[2], prim_p[3], prim_st[4][0], prim_st[4][1], prim_st[5][0], prim_st[5][1]);
                }
                for (no = 0; no < 6; no++) {
                    shLensFlareDrawCommon(vif1pk, lf_work, sc_info, &base_color[no], base_r[no], base_vector[no], prim_st[base_st_index[no]][0], prim_st[base_st_index[no]][1], z_value[no]);
                }
                {
                    float _rate;

                    _rate = lf_work->now_l_eff_rate;
                    color.x = 32.0f * _rate;
                    color.y = 32.0f * _rate;
                    color.z = 32.0f * _rate;
                    CLAMP_COLOR(color);
                    line_r = 160.0f - _shSqrtIP(2.0f * lf_work->scr_l_pos.z);
                    r = line_r;
                    prim_p[0].x = lf_work->l_screen_pos.x - (int)FIX4(r);
                    prim_p[0].y = lf_work->l_screen_pos.y - (int)ASPECT(FIX4(r));
                    prim_p[1].x = lf_work->l_screen_pos.x + (int)FIX4(r);
                    prim_p[1].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(r));
                    prim_p[0].z = prim_p[1].z = 0xFFFF;
                    shLensFlareSpriteAddPacketGif(vif1pk, color, prim_p[0], prim_p[1], prim_st[0][0], prim_st[0][1]);
                }
                {
                    float _rate;

                    _rate = lf_work->now_l_eff_rate;
                    color.x = 64.0f * _rate;
                    color.y = 24.0f * _rate;
                    color.z = 94.0f * _rate;
                    CLAMP_COLOR(color);
                    r = 10.0f;
                    prim_p[0].x = lf_work->l_screen_pos.x - (int)FIX4(r);
                    prim_p[0].y = lf_work->l_screen_pos.y - (int)ASPECT(FIX4(r));
                    prim_p[1].x = lf_work->l_screen_pos.x + (int)FIX4(r);
                    prim_p[1].y = lf_work->l_screen_pos.y + (int)ASPECT(FIX4(r));
                    prim_p[0].z = prim_p[1].z = 0xFFFF;
                    shLensFlareSpriteAddPacketGif(vif1pk, color, prim_p[0], prim_p[1], prim_st[0][0], prim_st[0][1]);
                }
            }
        }
    }
    if (flare_drawn) {
        unsigned long giftag_Z[2] = { 0x1000000000008000, 0xE };

        sceVif1PkOpenGifTag(vif1pk, *(u_long128 *)giftag_Z);
        sceVif1PkAddGsAD(vif1pk, 0x4E, 0x13A0001C0);
        sceVif1PkCloseGifTag(vif1pk);
        sceVif1PkCloseDirectCode(vif1pk);
        sceVif1PkEnd(vif1pk, 0);
        sceVif1PkTerminate(vif1pk);
        return vif1pk->pBase;
    }
    return NULL;
}

static void shLensFlareSpriteAddPacketGif(sceVif1Packet *packet, IVEC c, IVEC v0, IVEC v1, FVEC st0, FVEC st1) {
    u_long128 *pTex0;
    unsigned long giftag_alpha_test[2] = { 0x8400000000008000, 0x51251260 };
    float q;

    q = 1.0f;
    sceVif1PkOpenGifTag(packet, *(u_long128 *)giftag_alpha_test);
    sceVif1PkAddGsData(packet, 0xDE);
    pTex0 = &LF_Tex_Work.Tex0Data;
    sceVif1PkAddGsData(packet, *(unsigned long *)pTex0);
    sceVif1PkAddGsData(packet, GS_SET_ST(Float_Bits(st0.x * q), Float_Bits(st0.y * q)));
    sceVif1PkAddGsData(packet, GS_SET_RGBAQ(c.x, c.y, c.z, 0x80, *(unsigned int *)&q));
    sceVif1PkAddGsData(packet, GS_SET_XYZ(v0.x, v0.y, v0.z));
    sceVif1PkAddGsData(packet, GS_SET_ST(Float_Bits(st1.x * q), Float_Bits(st1.y * q)));
    sceVif1PkAddGsData(packet, GS_SET_RGBAQ(c.x, c.y, c.z, 0x80, *(unsigned int *)&q));
    sceVif1PkAddGsData(packet, GS_SET_XYZ(v1.x, v1.y, v1.z));
    sceVif1PkCloseGifTag(packet);
}

/** Per frame: draws the lens flare, or kicks an empty packet to release the texture slot. */
void Kari_LensFlare_DrawExec(void) {
    static union Q_WORDDATA dum = { { 0x70000000, 0, 0, 0 } };
    void *pak;

    pak = kari_shLensFlareDraw();
    if (pak) {
        kari_Thr_LFD1D2SyncKick(pak);
    } else {
        kari_Thr_LFD1D2SyncKick(&dum);
    }
}
