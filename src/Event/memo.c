/*
 * memo.c: the memo (notes) menu. Lists the memos the player has found, shows the
 * picked one full screen (with a layer image drawn over it for some puzzles) and
 * its text.
 *
 * Sh2sys.step[3]: 0 init, 1 select, 2 display, 3 end; step[4] is the sub-step.
 */
#include "sh2.h"
#include "asm_helpers.h"

/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)

#include "math_const.h"

#define SH2SYS_STEP2(v)   \
    Sh2sys.step[2] = (v); \
    Sh2sys.step[3] = 0;   \
    Sh2sys.step[4] = 0;   \
    Sh2sys.step[5] = 0;   \
    Sh2sys.step[6] = 0;   \
    Sh2sys.step[7] = 0

#define SH2SYS_STEP3(v)   \
    Sh2sys.step[3] = (v); \
    Sh2sys.step[4] = 0;   \
    Sh2sys.step[5] = 0;   \
    Sh2sys.step[6] = 0;   \
    Sh2sys.step[7] = 0

#define SH2SYS_STEP4(v)   \
    Sh2sys.step[4] = (v); \
    Sh2sys.step[5] = 0;   \
    Sh2sys.step[6] = 0;   \
    Sh2sys.step[7] = 0

#define SH2SYS_STEP4_NEXT()   \
    Sh2sys.step[4]++;       \
    Sh2sys.step[5] = 0;     \
    Sh2sys.step[6] = 0;     \
    Sh2sys.step[7] = 0


/* flag: game flag that makes the memo available; status: 1 fade in, 2 no text */
static struct Memo_Data data[45] = {
    { 50, 1, 38, 85, NULL, NULL },
    { 52, 1, 39, 86, NULL, NULL },
    { 54, 1, 40, 87, NULL, NULL },
    { 56, 1, 41, 88, NULL, NULL },
    { 58, 1, 42, 89, NULL, NULL },
    { 610, 1, 43, 90, NULL, NULL },
    { 614, 1, 44, 92, NULL, NULL },
    { 615, 1, 47, 91, NULL, NULL },
    { 44, 1, 0, 49, data_pic_out_p_swamp_tex, NULL },
    { 63, 1, 1, 50, data_pic_apt_p_endhint_tex, NULL },
    { 64, 1, 2, 51, data_pic_apt_p_endhint_tex, NULL },
    { 65, 1, 3, 52, data_pic_apt_p_endhint_tex, NULL },
    { 74, 2, 4, 53, data_pic_apt_clock_name_tex, NULL },
    { 78, 1, 5, 54, data_pic_apt_clock_memo_tex, NULL },
    { 75, 1, 6, 55, data_pic_apt_clock_memo_tex, NULL },
    { 96, 1, 7, 56, data_pic_apt_dust_out_tex, NULL },
    { 84, 1, 8, 57, data_pic_apt_p_tourist_tex, NULL },
    { 102, 1, 9, 58, data_pic_apt_p_safe_close_tex, data_pic_apt_p_safe_close2_tex },
    { 111, 1, 11, 59, NULL, NULL },
    { 123, 1, 12, 60, data_pic_apt_p_desk_hint_tex, NULL },
    { 124, 1, 13, 61, data_pic_apt_p_desk_hint_tex, NULL },
    { 125, 1, 14, 62, data_pic_apt_p_desk_hint_tex, NULL },
    { 609, 1, 15, 63, data_pic_apt_p_desk_hint_tex, NULL },
    { 179, 1, 16, 64, data_pic_hsp_p_patient_tex, NULL },
    { 169, 1, 17, 65, data_pic_hsp_p_doctormemo_tex, NULL },
    { 180, 1, 18, 66, data_pic_hsp_whiteboard_tex, NULL },
    { 608, 1, 48, 84, NULL, NULL },
    { 183, 1, 19, 67, data_pic_hsp_carbon_tex, NULL },
    { 613, 1, 19, 68, data_pic_hsp_carbon_tex, NULL },
    { 204, 1, 20, 69, data_pic_hsp_p_diary_tex, NULL },
    { 192, 2, 21, 70, data_pic_hsp_p_boxnumber_tex, data_pic_hsp_p_boxnumber_2_tex },
    { 191, 1, 22, 71, data_pic_hsp_p_hair_tex, data_pic_hsp_p_hair_hair_tex },
    { 231, 1, 23, 72, data_pic_hsp_p_female_tex, data_pic_hsp_p_female_ring_tex },
    { 242, 1, 24, 73, NULL, NULL },
    { 263, 1, 26, 74, NULL, NULL },
    { 262, 2, 27, 83, data_pic_dls_p_sankaku_tex, NULL },
    { 287, 2, 28, 75, data_pic_dls_p_tablet_tex, data_pic_dls_p_tablet_plate_tex },
    { 279, 1, 29, 76, data_pic_dls_p_magazin_tex, NULL },
    { 331, 1, 30, 77, NULL, NULL },
    { 361, 1, 31, 78, data_pic_etc_comingsoon_tex, NULL },
    { 362, 1, 32, 79, data_pic_etc_comingsoon_tex, NULL },
    { 363, 1, 33, 80, data_pic_etc_comingsoon_tex, NULL },
    { 612, 1, 34, 81, data_pic_etc_comingsoon_tex, NULL },
    { 390, 1, 35, 82, NULL, NULL },
    { 412, 0, 36, 0, data_pic_etc_comingsoon_tex, NULL },
};

static int select;
static int list_point;
static int disp_point;

static void MemoInit(void);
static void MemoSelect(void);
static void MemoDisplay(void);
static void MemoEnd(void);

/*
 * Load the memo picture (file0) and its layer image (file1, right after it).
 * A few layers only exist once the matching puzzle has been solved.
 */
static void MemoPictureLoad(union fsFileIndex *file0, union fsFileIndex *file1);
static void MemoPictureBaseDraw(int rgb);
static void MemoSelectBarDraw(int msg, int y);
static void MemoPictureLayerDraw(int rgb);
static void MemoPictureLayerDrawSafeLock(void);
static void MemoPictureLayerDrawGuruguru(void);
static void MemoPictureLayerDrawAngelRing(void);
static void MemoPictureLayerDrawHair(void);
static void MemoPictureLayerDrawTablet(void);
static void MemoMessageWallet(void);

/** Memo menu main, one step per call: init, select, display or end (Sh2sys.step[3]). */
void MemoMain(void) {
    switch (Sh2sys.step[3]) {
    case 0:
    default:
        MemoInit();
        break;
    case 1:
        MemoSelect();
        break;
    case 2:
        MemoDisplay();
        break;
    case 3:
        MemoEnd();
        break;
    }
}

/** Returns non-zero if the player has found any memo (so the menu can offer it). */
int MemoCommandCheck(void) {
    int i;

    for (i = 0; i < 45; i++) {
        if (GAME_FLAG(data[i].flag)) {
            return 1;
        }
    }
    return 0;
}

static void MemoInit(void) {
    int i;

    switch (Sh2sys.step[4]) {
    case 0:
        select = playing.memo_select;
        list_point = select;
        for (i = 0; i <= select; i++) {
            if (!GAME_FLAG(data[i].flag)) {
                list_point--;
            }
        }
        if (list_point < 0) {
            list_point = 0;
        }
        disp_point = 0;
        DataLoadMessage(3);
        FcRead(data_pic_etc_p_memo_tex, get_gp_data_buf_addr());
        SH2SYS_STEP4_NEXT();
    case 1:
        if (fsSync(1, -1) < 0) {
            break;
        }
        SH2SYS_STEP4_NEXT();
    case 2:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        SH2SYS_STEP4_NEXT();
    case 3:
        ScreenEffectFadeStart(4, 0.0f);
        SH2SYS_STEP3(1);
        break;
    }
}

static void MemoSelect(void) {
    short list[45][2];
    int list_number;
    int work;
    int alpha;
    int i;
    int j;

    list_number = 0;
    for (i = 0; i < 45; i++) {
        if (GAME_FLAG(data[i].flag)) {
            list[list_number][0] = i;
            list[list_number][1] = data[i].msg_label;
            list_number++;
        }
    }
    if (shPadTrigger(0, key_config.enter)) {
        SH2SYS_STEP3(2);
    } else if (shPadTrigger(0, key_config.cancel)) {
        SH2SYS_STEP3(3);
    } else if (shPadRepeat(0, 0x400)) {
        list_point--;
        if (list_point < 0) {
            list_point = list_number - 1;
        }
        disp_point++;
        if (disp_point > 3) {
            disp_point = 3;
        }
    } else if (shPadRepeat(0, 0x800)) {
        list_point++;
        if (list_point >= list_number) {
            list_point = 0;
        }
        disp_point--;
        if (disp_point < -3) {
            disp_point = -3;
        }
    }
    select = list[list_point][0];
    MemoPictureBaseDraw(0x60);
    fontClear();
    if (list_number < 12) {
        for (i = 0; i < list_number; i++) {
            work = 0x100 - list_number * 16 + i * 32;
            fontPrintStrNum(msg_buffer, list[i][1], 0x100, work);
            if (i == list_point) {
                MemoSelectBarDraw(list[i][1], work);
            }
        }
    } else {
        for (i = 0; i < 13; i++) {
            j = i + (list_point + disp_point - 6);
            if (j < 0) {
                j += list_number;
            }
            if (j >= list_number) {
                j -= list_number;
            }
            work = i * 32 + 0x30;
            switch (i) {
            default:
                alpha = 0x80;
                break;
            case 2:
            case 10:
                alpha = 0x40;
                break;
            case 1:
            case 11:
                alpha = 0x20;
                break;
            case 0:
            case 12:
                alpha = 0x10;
                break;
            }
            fontSetAlpha(alpha);
            fontPrintStrNum(msg_buffer, list[j][1], 0x100, work);
            if (j == list_point) {
                MemoSelectBarDraw(list[j][1], work);
            }
        }
    }
}

/* Matching: fitted stand-in for double code (docs/stand-ins.md), not recovered:
 * MemoPictureLayerDrawSafeLock's lock/i end up in a2/a3 as after one. It sits in the gap before
 * MemoDisplay, where the original's line table leaves room for code (16 lines; the file's usual
 * gap between functions is 4). Its body is still fitted. */
STRIPPED_DOUBLE_CODE()
static void MemoDisplay(void) {
    static float alpha;
    unsigned char c_work[8];

    switch (Sh2sys.step[4]) {
    case 0:
        ScreenEffectFadeStart(1, 0.0f);
        MemoPictureLoad(data[select].file0, data[select].file1);
        if (data[select].status == 1) {
            alpha = 0.0f;
        } else {
            alpha = 1.0f;
        }
        SH2SYS_STEP4_NEXT();
    case 1:
        if (fsSync(1, -1) < 0 || !ScreenEffectFadeCheck()) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        fontClear();
        SH2SYS_STEP4_NEXT();
    case 2:
        MemoPictureBaseDraw(ftoi(64.0f * alpha) + 0x40);
        MemoPictureLayerDraw(ftoi(64.0f * alpha) + 0x40);
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        if (data[select].status == 1) {
            SH2SYS_STEP4(4);
        } else {
            SH2SYS_STEP4(3);
        }
        break;
    case 3:
        MemoPictureBaseDraw(ftoi(64.0f * alpha) + 0x40);
        MemoPictureLayerDraw(ftoi(64.0f * alpha) + 0x40);
        if (!shPadTrigger(0, key_config.enter + key_config.cancel)) {
            break;
        }
        if (data[select].status == 2) {
            SH2SYS_STEP4(4);
        } else {
            ScreenEffectFadeStart(1, 0.0f);
            SH2SYS_STEP4(6);
        }
        break;
    case 4:
        switch (select) {
        case 0x11:
            MemoMessageWallet();
            break;
        case 0x1B:
        case 0x1C:
            c_work[0] = (game_flag.carbon >> 12 & 0xF) + '0';
            c_work[1] = (game_flag.carbon >> 8 & 0xF) + '0';
            c_work[2] = (game_flag.carbon >> 4 & 0xF) + '0';
            c_work[3] = (game_flag.carbon & 0xF) + '0';
            c_work[4] = 0;
            fontSetMes(0, dicSetStr(c_work));
            break;
        }
        fontMessageNum(msg_buffer, data[select].msg_memo);
        SH2SYS_STEP4_NEXT();
    case 5:
        alpha -= shGetDT();
        if (alpha < 0.0f) {
            alpha = 0.0f;
        }
        MemoPictureBaseDraw(ftoi(64.0f * alpha) + 0x40);
        MemoPictureLayerDraw(ftoi(64.0f * alpha) + 0x40);
        if (fontGetStatus() == -1) {
            break;
        }
        fontClear();
        ScreenEffectFadeStart(2, 0.0f);
        SH2SYS_STEP4(6);
        break;
    case 6:
        MemoPictureBaseDraw(ftoi(64.0f * alpha) + 0x40);
        MemoPictureLayerDraw(ftoi(64.0f * alpha) + 0x40);
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        SH2SYS_STEP4_NEXT();
        break;
    case 7:
        if (data[select].file0) {
            FcRead(data_pic_etc_p_memo_tex, get_gp_data_buf_addr());
        }
        SH2SYS_STEP4_NEXT();
    case 8:
        if (fsSync(1, -1) < 0) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        SH2SYS_STEP3(1);
        break;
    }
}

static void MemoEnd(void) {
    playing.memo_select = select;
    ScreenEffectFadeStart(1, 0.0f);
    SH2SYS_STEP2(6);
}

static void MemoPictureLoad(union fsFileIndex *file0, union fsFileIndex *file1) {
    /* Matching: the assert bakes its original line number into the object. */
#line 475
    assert(file0 || !file1);
    switch (select) {
    case 0x11:
        if (!GAME_FLAG(607)) {
            file0 = file1 = NULL;
        }
        break;
    case 0x1F:
        if (!GAME_FLAG(171)) {
            file0 = file1 = NULL;
        }
        break;
    case 0x20:
        if (!GAME_FLAG(227) && !GAME_FLAG(228)) {
            file0 = file1 = NULL;
        }
        break;
    }
    if (file0) {
        FcRead(file0, get_gp_data_buf_addr());
        layer_adr = ((FcGetFileSize(file0) + 0x7FF) & ~0x7FF) + get_gp_data_buf_addr();
    }
    if (file1) {
        FcRead(file1, layer_adr);
    }
}

static void MemoPictureBaseDraw(int rgb) {
    struct PicDraw_Data pic;

    sh2gfw_Black_Clear();
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(), 0, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr();
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.otp = 1;
    pic.r = rgb;
    pic.g = rgb;
    pic.b = rgb;
    pic.status |= 0x10;
    PictureDraw(&pic);
}

static void MemoSelectBarDraw(int msg, int y) {
    struct PicDraw_Data pic;
    int wl[8];
    int w;

    fontGetMesWidth(wl, fontGetMesAdr(msg_buffer, msg));
    w = (wl[0] >> 1) + 4;
    shQzero(&pic, sizeof(pic));
    pic.otp = 8;
    pic.x0 = -w << 4;
    pic.y0 = (y - 0x100) << 4;
    pic.x1 = w << 4;
    pic.y1 = (y - 0xE0) << 4;
    pic.status |= 2;
    pic.r = 0x20;
    pic.g = 0x60;
    pic.b = 0x20;
    pic.status |= 0x10;
    pic.a = 0x40;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    PictureDraw(&pic);
}

static void MemoPictureLayerDraw(int rgb) {
    switch (select) {
    case 0x11:
        if (GAME_FLAG(607)) {
            MemoPictureLayerDrawSafeLock();
        }
        break;
    case 0x1E:
        MemoPictureLayerDrawGuruguru();
        break;
    case 0x20:
        if (GAME_FLAG(227) || GAME_FLAG(228)) {
            MemoPictureLayerDrawAngelRing();
        }
        break;
    case 0x1F:
        MemoPictureLayerDrawHair();
        break;
    case 0x24:
        MemoPictureLayerDrawTablet();
        break;
    }
}


static void MemoPictureLayerDrawSafeLock(void) {
    struct PicDraw_Data pic;
    float cosrot;
    float sinrot;
    float pos[4][2];
    float rot;
    int lock;
    int i;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.a = 0x80;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    lock = 0;
    for (i = 0; i < 5; i++) {
        if (GAME_FLAG(i + 0x67)) {
            lock += 1 << i;
        }
    }
    rot = PI * (2.0 * (lock + 1)) / 20.0;
    sinrot = shSinF(-rot);
    cosrot = shCosF(-rot);
    pos[0][0] = 0.8f * (-80.0f * cosrot - -64.0f * sinrot);
    pos[0][1] = -80.0f * sinrot + -64.0f * cosrot;
    pos[1][0] = 0.8f * (80.0f * cosrot - -64.0f * sinrot);
    pos[1][1] = 80.0f * sinrot + -64.0f * cosrot;
    pos[2][0] = 0.8f * (-80.0f * cosrot - 64.0f * sinrot);
    pos[2][1] = -80.0f * sinrot + 64.0f * cosrot;
    pos[3][0] = 0.8f * (80.0f * cosrot - 64.0f * sinrot);
    pos[3][1] = 80.0f * sinrot + 64.0f * cosrot;
    pic.x0 = ftoi4(pos[0][0] - 14.5f);
    pic.y0 = ftoi4(pos[0][1] - 4.75f);
    pic.x1 = ftoi4(pos[1][0] - 14.5f);
    pic.y1 = ftoi4(pos[1][1] - 4.75f);
    pic.status |= 2;
    pic.x2 = ftoi4(pos[2][0] - 14.5f);
    pic.y2 = ftoi4(pos[2][1] - 4.75f);
    pic.x3 = ftoi4(pos[3][0] - 14.5f);
    pic.y3 = ftoi4(pos[3][1] - 4.75f);
    pic.status |= 0x80;
    pic.us0 = 0;
    pic.vt0 = 0;
    pic.us1 = 0x7F0;
    pic.vt1 = 0x7F0;
    pic.status |= 4;
    pic.otp = 3;
    PictureDraw(&pic);
}

static void MemoPictureLayerDrawGuruguru(void) {
    static short tex[9][4][4] = {
        { { 0, 0, 32, 64 }, { 160, 0, 192, 64 }, { 320, 0, 352, 64 }, { 0, 128, 16, 176 } },
        { { 32, 0, 64, 64 }, { 192, 0, 224, 48 }, { 352, 0, 400, 64 }, { 16, 128, 48, 176 } },
        { { 64, 0, 96, 64 }, { 224, 0, 256, 48 }, { 400, 0, 432, 64 }, { 48, 128, 80, 192 } },
        { { 96, 0, 128, 64 }, { 256, 0, 288, 48 }, { 432, 0, 464, 80 }, { 80, 128, 112, 192 } },
        { { 128, 0, 160, 64 }, { 288, 0, 320, 64 }, { 464, 0, 496, 64 }, { 112, 128, 144, 192 } },
        { { 0, 64, 32, 128 }, { 160, 64, 192, 128 }, { 320, 64, 368, 112 }, { 144, 128, 176, 192 } },
        { { 32, 64, 64, 128 }, { 192, 48, 240, 112 }, { 368, 64, 400, 112 }, { 176, 128, 208, 224 } },
        { { 64, 64, 96, 128 }, { 240, 48, 272, 112 }, { 400, 80, 448, 144 }, { 208, 128, 240, 192 } },
        { { 96, 64, 128, 128 }, { 272, 64, 304, 128 }, { 464, 64, 496, 128 }, { 240, 128, 272, 192 } },
    };
    static short pos[9][4][2] = {
        { { -768, -1120 }, { -384, -832 }, { 64, -1152 }, { 656, -608 } },
        { { -896, -1072 }, { -384, -736 }, { 0, -1168 }, { 577, -736 } },
        { { -896, -1056 }, { -432, -672 }, { 80, -1088 }, { 560, -784 } },
        { { -880, -1088 }, { -368, -672 }, { 112, -1200 }, { 560, -736 } },
        { { -800, -1168 }, { -480, -944 }, { 0, -1072 }, { 576, -784 } },
        { { -800, -1184 }, { -480, -880 }, { 0, -864 }, { 544, -624 } },
        { { -768, -1152 }, { -464, -928 }, { 128, -928 }, { 609, -800 } },
        { { -864, -1120 }, { -416, -944 }, { -16, -1216 }, { 560, -879 } },
        { { -816, -1216 }, { -416, -976 }, { 80, -1136 }, { 528, -720 } },
    };
    struct PicDraw_Data pic;
    int no;
    int i;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.otp = 3;
    pic.a = 0x80;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    for (i = 0; i < 4; i++) {
        no = game_flag.guruguru[i];
        pic.x0 = pos[no][i][0];
        pic.y0 = pos[no][i][1];
        pic.x1 = pos[no][i][0] + (tex[no][i][2] - tex[no][i][0]) * 16;
        pic.y1 = pos[no][i][1] + (tex[no][i][3] - tex[no][i][1]) * 16;
        pic.status |= 2;
        pic.us0 = tex[no][i][0] * 16;
        pic.vt0 = tex[no][i][1] * 16;
        pic.us1 = (tex[no][i][2] - 1) * 16;
        pic.vt1 = (tex[no][i][3] - 1) * 16;
        pic.status |= 4;
        PictureDraw(&pic);
    }
}

static void MemoPictureLayerDrawAngelRing(void) {
    struct PicDraw_Data pic;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    if (GAME_FLAG(227)) {
        pic.x0 = 0x939;
        pic.y0 = 0x635;
        pic.x1 = 0xA39;
        pic.y1 = 0x735;
        pic.status |= 2;
        pic.us0 = 0;
        pic.vt0 = 0;
        pic.us1 = 0xF0;
        pic.vt1 = 0xF0;
        pic.status |= 4;
        pic.otp = 3;
        pic.a = 0x80;
        pic.alpha_a = 0;
        pic.alpha_b = 1;
        pic.alpha_c = 0;
        pic.alpha_d = 1;
        pic.alpha_fix = 0x80;
        pic.status |= 0x20;
        PictureDraw(&pic);
    }
    if (GAME_FLAG(228)) {
        pic.x0 = 0x472;
        pic.y0 = 0x6;
        pic.x1 = 0x572;
        pic.y1 = 0x106;
        pic.status |= 2;
        pic.us0 = 0x100;
        pic.vt0 = 0;
        pic.us1 = 0x1F0;
        pic.vt1 = 0xF0;
        pic.status |= 4;
        pic.otp = 3;
        pic.a = 0x80;
        pic.alpha_a = 0;
        pic.alpha_b = 1;
        pic.alpha_c = 0;
        pic.alpha_d = 1;
        pic.alpha_fix = 0x80;
        pic.status |= 0x20;
        PictureDraw(&pic);
    }
}

static void MemoPictureLayerDrawHair(void) {
    struct PicDraw_Data pic;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.otp = 3;
    pic.a = 0x80;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    pic.x0 = -0x470;
    pic.y0 = -0x340;
    pic.x1 = 0xB90;
    pic.y1 = 0xC0;
    pic.status |= 2;
    PictureDraw(&pic);
}

static void MemoPictureLayerDrawTablet(void) {
    struct PicDraw_Data pic;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.a = 0x80;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    pic.otp = 3;
    if (GAME_FLAG(301)) {
        pic.x0 = -0xAD8;
        pic.y0 = -0x70;
        pic.x1 = -0x498;
        pic.y1 = 0x5D0;
        pic.status |= 2;
        pic.us0 = 0;
        pic.vt0 = 0;
        pic.us1 = 0x640;
        pic.vt1 = 0x640;
        pic.status |= 4;
        PictureDraw(&pic);
    }
    if (GAME_FLAG(302)) {
        pic.x0 = -0x2C5;
        pic.y0 = -0x70;
        pic.x1 = 0x37B;
        pic.y1 = 0x5D0;
        pic.status |= 2;
        pic.us0 = 0x640;
        pic.vt0 = 0;
        pic.us1 = 0xC80;
        pic.vt1 = 0x640;
        pic.status |= 4;
        PictureDraw(&pic);
    }
    if (GAME_FLAG(303)) {
        pic.x0 = 0x4EE;
        pic.y0 = -0x32;
        pic.x1 = 0xB2E;
        pic.y1 = 0x60E;
        pic.status |= 2;
        pic.us0 = 0;
        pic.vt0 = 0x640;
        pic.us1 = 0x640;
        pic.vt1 = 0xC80;
        pic.status |= 4;
        PictureDraw(&pic);
    }
}

/** The safe dial hints in the wallet memo, per riddle level. */
static void MemoMessageWallet(void) {
    unsigned char c_work[4];
    int work;
    int i;

    switch (playing.riddle_level) {
    case 0:
        for (i = 0; i < 4; i++) {
            if (i == 0) {
                work = game_flag.safe[0] + 1;
            } else if (i == 2) {
                work = (game_flag.safe[i] - game_flag.safe[i - 1] + 20) % 20;
            } else {
                work = (game_flag.safe[i - 1] - game_flag.safe[i] + 20) % 20;
            }
            c_work[0] = work / 10 + '0';
            c_work[1] = work % 10 + '0';
            c_work[2] = 0;
            fontSetMes(i, dicSetStr(c_work));
        }
        break;
    case 1:
        for (i = 0; i < 4; i++) {
            work = game_flag.safe[i];
            work++;
            c_work[0] = work / 10 + '0';
            c_work[1] = work % 10 + '0';
            c_work[2] = 0;
            fontSetMes(i, dicSetStr(c_work));
        }
        break;
    case 2:
        for (i = 0; i < 4; i++) {
            work = game_flag.safe[i] + 1;
            if (work >= 20) {
                c_work[0] = 'X';
                c_work[1] = 'X';
                c_work[2] = 0;
            } else if (work >= 10) {
                if (i & 1) {
                    c_work[0] = 'X';
                    c_work[1] = work % 10 + '0';
                    c_work[2] = 0;
                } else {
                    c_work[0] = 'V';
                    c_work[1] = 'V';
                    c_work[2] = work % 10 + '0';
                    c_work[3] = 0;
                }
            } else {
                c_work[0] = game_flag.safe[i] + '1';
                c_work[1] = 0;
            }
            fontSetMes(i, dicSetStr(c_work));
        }
        break;
    default:
        for (i = 0; i < 4; i++) {
            if (game_flag.safe[i] + 1 >= 10) {
                c_work[0] = game_flag.safe[i] + 'X';
                c_work[2] = 0; /* @bug c_work[1] is never set: stale from an earlier digit, or garbage */
            } else {
                c_work[0] = game_flag.safe[i] + '1';
                c_work[1] = 0;
            }
            fontSetMes(i, dicSetStr(c_work));
        }
        break;
    }
}
