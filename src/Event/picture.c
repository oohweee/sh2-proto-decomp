/*
 * picture.c: draws a 2D picture (a texture from a map area file, or a flat colored
 * rectangle) through the sorted packet system (spack), and uploads its texture/CLUT.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sdk/libgraph.h"

#define PK_ADD(v) (*spack.pos++ = (v))

/* sorted packet depth of an ot position */
#define PK_W(otp) ((unsigned short)(0x100 - (otp)) + 0xFFFF0000)

#define PK_UVST(u, v)                       \
    if (uvset) {                            \
        PK_ADD(GS_SET_UV(u, v));        \
    } else {                                \
        PK_ADD(GS_SET_ST(u, v));        \
    }

#define PK_XYZ2(x, y) PK_ADD(GS_SET_XYZ((x) + 0x8000, (y) + 0x8000, 0xFFFD))

/** Draws a picture: a textured or flat rectangle with the alpha, test and colour settings of pic.
 * @param pic what to draw and how */
/* Matching: ST and Q are written as float bits through fbits() (inline asm), which compiles the function
 * without the global optimizer, as the original's code shows
 * (docs/matching-notes.md#picture-picturedraw). */
void PictureDraw(struct PicDraw_Data *pic) {
    struct sh2gfw_TEX_HEAD *tp;
    unsigned long giftag_spr;
    int uvset;
    int alpset;
    int xy2set;
    int len;
    int prim;

    if (!(pic->status & 2)) {
        pic->x0 = pic->y0 = -0x1000;
        pic->x1 = pic->y1 = 0x1000;
    }
    if (!(pic->status & 0x10)) {
        pic->r = 0x80;
        pic->g = 0x80;
        pic->b = 0x80;
    }
    if (!(pic->status & 0x20)) {
        alpset = 0;
        pic->a = 0x80;
    } else {
        alpset = 1;
    }
    if (pic->status & 0x80) { xy2set = 1; } else { xy2set = 0; }
    if (pic->status & 1) {
        if (pic->tex == -1) {
            pic->tex = sh2gfw_Get_BaseTBP0for2D();
        }
        if (pic->clut == -1) {
            pic->clut = 0x3640;
        }
        tp = (struct sh2gfw_TEX_HEAD *)((char *)pic->ap + pic->ap->toGlobalTexHead);
        if (!(pic->status & 4) && !(pic->status & 8)) {
            pic->us0 = pic->vt0 = fbits(0.0f);
            pic->us1 = pic->vt1 = fbits(1.0f);
        }
        if (pic->status & 4) { uvset = 1; } else { uvset = 0; }
        spkOpenDGiftag(0x1000000000008000, 0xE, PK_W(pic->otp), 0);
        PK_ADD(0);
        PK_ADD(GS_REG_TEXFLUSH);
        if (pic->status & 0x40) {
            PK_ADD(GS_SET_TEST(pic->test_ate, pic->test_atst, pic->test_aref, pic->test_afail, pic->test_date,
                                   pic->test_datm, pic->test_zte, pic->test_ztst));
            PK_ADD(GS_REG_TEST_1);
        }
        if (alpset) {
            PK_ADD(GS_SET_ALPHA(pic->alpha_a, pic->alpha_b, pic->alpha_c, pic->alpha_d, pic->alpha_fix));
            PK_ADD(GS_REG_ALPHA_1);
        }
        if (xy2set) {
            len = 11;
            prim = 4;
            if (uvset) {
                giftag_spr = 0x53535353106;
            } else {
                giftag_spr = 0x52525252106;
            }
        } else {
            len = 7;
            prim = 6;
            if (uvset) {
                giftag_spr = 0x5353106;
            } else {
                giftag_spr = 0x5252106;
            }
        }
        spkCloseOpenDGiftag(((unsigned long)len << 60) | 0x0400000000008000, giftag_spr);
        PK_ADD(GS_SET_TEX0(pic->tex, tp->w / 64, tp->drawpsm, tp->bitw, tp->bith, 1, 0, pic->clut, 0, 0, 0, 1));
        PK_ADD(GS_SET_PRIM(prim, 0, 1, 0, alpset, 0, uvset, 0, 0));
        PK_ADD(GS_SET_RGBAQ(pic->r, pic->g, pic->b, pic->a, fbits(1.0f)));
        PK_UVST(pic->us0, pic->vt0);
        PK_XYZ2(pic->x0, pic->y0);
        if (xy2set) {
            PK_UVST(pic->us1, pic->vt0);
        } else {
            PK_UVST(pic->us1, pic->vt1);
        }
        PK_XYZ2(pic->x1, pic->y1);
        if (xy2set) {
            PK_UVST(pic->us0, pic->vt1);
            PK_XYZ2(pic->x2, pic->y2);
            PK_UVST(pic->us1, pic->vt1);
            PK_XYZ2(pic->x3, pic->y3);
        }
        spkCloseGiftag();
    } else {
        spkOpenDGiftag(0x1000000000008000, 0xE, PK_W(pic->otp), 0);
        if (alpset) {
            PK_ADD(1);
            PK_ADD(0x4A);
            PK_ADD(GS_SET_ALPHA(pic->alpha_a, pic->alpha_b, pic->alpha_c, pic->alpha_d, pic->alpha_fix));
            PK_ADD(GS_REG_ALPHA_1);
        }
        if (pic->status & 0x40) {
            PK_ADD(GS_SET_TEST(pic->test_ate, pic->test_atst, pic->test_aref, pic->test_afail, pic->test_date,
                                   pic->test_datm, pic->test_zte, pic->test_ztst));
            PK_ADD(GS_REG_TEST_1);
        }
        spkCloseOpenDGiftag(0x4400000000008000, 0x5510);
        PK_ADD(GS_SET_PRIM(6, 0, 0, 0, alpset, 0, 0, 0, 0));
        PK_ADD(GS_SET_RGBAQ(pic->r, pic->g, pic->b, pic->a, fbits(1.0f)));
        PK_XYZ2(pic->x0, pic->y0);
        PK_XYZ2(pic->x1, pic->y1);
        if (pic->status & 0x40) {
            spkCloseOpenDGiftag(0x1000000000008000, 0xE);
            PK_ADD(0x5801B);
            PK_ADD(GS_REG_TEST_1);
        }
        spkCloseGiftag();
    }
}

/** Uploads the texture of a map area file to GS memory, and its CLUT when `drawpsm` is non-zero.
 * @param ap the area file
 * @param otp ordering-table position of the upload
 * @param tex_adr texture base pointer, or -1 for the default 2D one
 * @param clut_adr CLUT address, or -1 for the default (0x3640) */
void PictureLoadImage(struct sh2gfw_AREA_HEAD *ap, int otp, int tex_adr, int clut_adr) {
    struct sh2gfw_TEX_HEAD *tp;
    struct sh2gfw_CLUTS_HEAD *cp;
    int tpws;
    int load_img_all;
    int load_img;
    int i;

    tp = (struct sh2gfw_TEX_HEAD *)((char *)ap + ap->toGlobalTexHead);
    cp = (struct sh2gfw_CLUTS_HEAD *)((char *)ap + ap->toGlobalClutsHead);
    tpws = tp->w >> tp->bitshift;
    load_img_all = tp->datasize / 4;
    if (tex_adr == -1) {
        tex_adr = sh2gfw_Get_BaseTBP0for2D();
    }
    if (clut_adr == -1) {
        clut_adr = 0x3640;
    }
    spkStartEnvLoadImage(otp + 0x10);
    for (i = 0; load_img_all != 0; i++) {
        if (load_img_all > 0x10000) {
            load_img = 0x10000;
        } else {
            load_img = load_img_all;
        }
        load_img_all -= load_img;
        spkSetEnvLoadImage((char *)tp + tp->padbyte + i * 0x40000 + 0x30, tex_adr + i * 0x400, tpws >> 6,
                           tp->sendpsm, 0, 0, tpws, load_img / tpws);
    }
    if (tp->drawpsm) {
        spkSetEnvLoadImage((char *)cp + cp->toRawClut + 0x30, clut_adr, cp->clw >> 6, 0, 0, 0, cp->clw, cp->clh);
    }
    spkEndEnvLoadImage();
    spkOpenDGiftag(0x1000000000008000, 0xE, PK_W(otp), otp + 0x10);
    PK_ADD(0);
    PK_ADD(GS_REG_TEXFLUSH);
    spkCloseGiftag();
}
