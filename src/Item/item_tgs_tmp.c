/* item_tgs_tmp.c: temporary item pictures (TGS build) drawn from itemmenu2.tex. */
#include "sh2.h"
#include "asm_helpers.h"

/* Matching: fitted stand-in for double code (docs/stand-ins.md): later functions use a2 for temporaries. */
STRIPPED_DOUBLE_CODE()


float item_size[80] = {
    1.0f, 1.4f, 1.6f, 2.1f, 1.6f, 1.3f, 1.3f, 1.1f,
    1.6f, 1.1f, 1.1f, 0.9f, 1.1f, 1.1f, 0.9f, 1.0f,
    1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f,
    1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 0.8f, 1.2f, 1.1f,
    1.1f, 1.2f, 1.2f, 1.1f, 1.3f, 1.2f, 1.1f, 1.1f,
    1.1f, 1.2f, 1.4f, 1.1f, 1.3f, 1.2f, 1.1f, 1.4f,
    1.1f, 1.1f, 1.3f, 1.1f, 1.1f, 1.1f, 1.1f, 1.2f,
    1.3f, 1.1f, 0.8f, 1.1f, 1.1f, 1.3f, 1.3f, 1.1f,
    1.4f, 0.9f, 1.4f, 1.0f, 1.1f, 1.1f, 1.1f, 1.3f,
    1.3f, 1.1f, 0.9f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f,
};

static struct PicDraw_Data i_pic;

/** Starts loading the item picture sheet (itemmenu2.tex) into the gp data buffer. */
void TgsItemPictureLoad(void) {
    FcRead(data_pic_etc_itemmenu2_tex, get_gp_data_buf_addr());
}

/** Uploads the item picture sheet and sets up the picture used to draw the items. */
void TgsItemPitureStart(void) {
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(), 0, -1, -1);
    shQzero(&i_pic, sizeof(i_pic));
    i_pic.ap = (struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr();
    i_pic.tex = -1;
    i_pic.clut = -1;
    i_pic.status |= 1;
    i_pic.a = 0x80;
    i_pic.alpha_a = 0;
    i_pic.alpha_b = 1;
    i_pic.alpha_c = 0;
    i_pic.alpha_d = 1;
    i_pic.alpha_fix = 0x80;
    i_pic.status |= 0x20;
}

/**
 * Draws the picture of an item from the sheet, centred at (cx, cy) and scaled by size and the
 * item's own scale (item_size).
 * @param kind item kind
 * @param cx centre x (12.4 fixed point)
 * @param cy centre y (12.4 fixed point)
 * @param rgb brightness (r = g = b)
 * @param otp ordering-table position
 * @param size scale
 */
void TgsItemPitureDraw(int kind, int cx, int cy, int rgb, int otp, float size) {
    static short tex[80][3] = {
        { 20, 0, 97 },
        { 123, 0, 225 },
        { 236, 0, 338 },
        { 363, 0, 436 },
        { 460, 0, 564 },
        { 570, 0, 680 },
        { 700, 0, 770 },
        { 790, 0, 868 },
        { 900, 0, 954 },
        { 973, 0, 999 },
        { 26, 128, 100 },
        { 120, 128, 214 },
        { 214, 128, 311 },
        { 330, 128, 422 },
        { 435, 128, 527 },
        { 560, 128, 604 },
        { 640, 128, 722 },
        { 745, 128, 808 },
        { 825, 128, 900 },
        { 937, 128, 981 },
        { 24, 256, 96 },
        { 118, 256, 185 },
        { 208, 256, 286 },
        { 298, 256, 386 },
        { 397, 256, 480 },
        { 490, 256, 578 },
        { 586, 256, 690 },
        { 696, 256, 787 },
        { 794, 256, 900 },
        { 922, 256, 992 },
        { 12, 384, 112 },
        { 137, 384, 196 },
        { 220, 384, 304 },
        { 323, 384, 414 },
        { 424, 384, 515 },
        { 535, 384, 616 },
        { 633, 384, 728 },
        { 738, 384, 826 },
        { 840, 384, 935 },
        { 943, 384, 999 },
        { 26, 512, 104 },
        { 120, 512, 215 },
        { 224, 512, 318 },
        { 330, 512, 420 },
        { 425, 512, 517 },
        { 520, 512, 613 },
        { 618, 512, 708 },
        { 712, 512, 822 },
        { 828, 512, 918 },
        { 928, 512, 999 },
        { 12, 640, 101 },
        { 122, 640, 195 },
        { 218, 640, 307 },
        { 323, 640, 390 },
        { 425, 640, 500 },
        { 526, 640, 599 },
        { 616, 640, 708 },
        { 716, 640, 815 },
        { 836, 640, 904 },
        { 938, 640, 980 },
        { 28, 768, 96 },
        { 121, 768, 195 },
        { 216, 768, 292 },
        { 312, 768, 376 },
        { 394, 768, 475 },
        { 506, 768, 565 },
        { 590, 768, 690 },
        { 712, 768, 805 },
        { 824, 768, 904 },
        { 916, 768, 1001 },
        { 24, 896, 100 },
        { 138, 896, 180 },
        { 214, 896, 264 },
        { 300, 896, 404 },
        { 424, 896, 500 },
        { 522, 896, 598 },
        { 618, 896, 708 },
        { 724, 896, 820 },
        { 824, 896, 910 },
        { 920, 896, 999 },
    };
    static short texsize[80];
    static int item_list[80] = {
        10, 6, 8, 12, 13, 14, 4, 21, 11, 0,
        5, 7, 9, 62, 17, 15, 67, 54, 53, 52,
        48, 49, 47, 57, 58, 56, 65, 64, 66, 61,
        16, 63, 24, 41, 40, 45, 43, 35, 37, 0,
        25, 26, 34, 27, 42, 30, 39, 44, 23, 0,
        36, 28, 29, 32, 33, 31, 38, 0, 68, 73,
        60, 71, 72, 51, 50, 1, 46, 18, 3, 22,
        74, 69, 70, 2, 59, 55, 20, 19, 0, 0,
    };
    int i;
    int xy;

    for (i = 0; i < 80; i++) {
        texsize[i] = tex[i][2] - tex[i][0];
    }
    for (i = 0; i < 79; i++) {
        if (kind == item_list[i]) {
            break;
        }
    }

    xy = ftoi4(64.0f * size);
    i_pic.x0 = cx - ((int)(size * (texsize[i] / 2) * item_size[i]) << 4);
    i_pic.y0 = cy - (int)(xy * item_size[i]);
    i_pic.x1 = cx + ((int)(size * (texsize[i] / 2) * item_size[i]) << 4);
    i_pic.y1 = cy + (int)(xy * item_size[i]);
    i_pic.status |= 2;
    i_pic.us0 = tex[i][0] << 4;
    i_pic.vt0 = tex[i][1] << 4;
    i_pic.us1 = (tex[i][0] + texsize[i]) << 4;
    i_pic.vt1 = (tex[i][1] + 127) << 4;
    i_pic.status |= 4;
    i_pic.r = rgb;
    i_pic.g = rgb;
    i_pic.b = rgb;
    i_pic.status |= 0x10;
    i_pic.otp = otp;
    PictureDraw(&i_pic);
}
