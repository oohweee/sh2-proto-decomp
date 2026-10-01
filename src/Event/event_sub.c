/*
 * event_sub.c: building blocks for the event programs (ev_prog). Each one is a
 * small state machine driven by ev_s_step and returns 1 once it is done:
 * messages, item use / pick-up, full-screen pictures (maps, memos), movies.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"


/* item kind -> the model (kind << 8 | id) that stands for it in the stage */
static short item_to_chara[75] = {
    0x0000, 0x0700, 0x0701, 0x0733, 0x0702, 0x0703, 0x0743, 0x0724,
    0x073B, 0x0723, 0x0744, 0x0704, 0x0745, 0x050C, 0x0746, 0x0705,
    0x0706, 0x070C, 0x070B, 0x0000, 0x070E, 0x0000, 0x0000, 0x0722,
    0x072F, 0x041F, 0x072C, 0x0732, 0x0731, 0x0730, 0x0000, 0x0729,
    0x0707, 0x0728, 0x070A, 0x072A, 0x071B, 0x0726, 0x073F, 0x0742,
    0x073D, 0x0000, 0x073E, 0x0735, 0x0736, 0x0747, 0x072D, 0x071D,
    0x071F, 0x071C, 0x0708, 0x0709, 0x072B, 0x0712, 0x0711, 0x0710,
    0x0738, 0x0737, 0x0739, 0x073A, 0x073C, 0x070D, 0x0714, 0x070F,
    0x0719, 0x071A, 0x0718, 0x0720, 0x0734, 0x0000, 0x0000, 0x0721,
    0x0713, 0x0717, 0x071E,
};

char *layer_adr;
char cursor_adr[0xC800] __attribute__((aligned(64)));
struct Item item;
static int ev_filter_on;

static int ItemUseSeTiming(int kind, int boa);

/* Matching: the do/while leaves the original's nop where a use ends a body. */
#define EV_S_STEP(n)     \
    do {                 \
        ev_s_step = (n); \
    } while (0)

/* Set the draw color (status bit 0x10: use r/g/b). Matching: the do/while leaves the original's
 * nop where a use ends a body. */
#define PIC_SET_RGB(p, rr, gg, bb) \
    do {                           \
        (p).r = (rr);              \
        (p).g = (gg);              \
        (p).b = (bb);              \
        (p).status |= 0x10;        \
    } while (0)

/** Shows message msg and waits until it is closed (or the event is cancelled).
 * @param msg message number
 * @return 1 when done */
int EvSubMessage(int msg) {
    switch (ev_s_step) {
    case 0:
        fontMessageNum(msg_buffer, msg);
        ev_s_step++;
        break;
    case 1:
        if (fontGetStatus() != -1 || ev_cancel) {
            if (ev_cancel) {
                ev_prog_flag_set = 0;
            }
            fontClear();
            ev_s_step++;
        }
        break;
    case 2:
        return 1;
    }
    return 0;
}

/** Shows message msg and waits until it is answered or closed.
 * @param msg message number
 * @return 1 when done */
int EvSubQuestion(int msg) {
    switch (ev_s_step) {
    case 0:
        fontMessageNum(msg_buffer, msg);
        ev_s_step++;
        break;
    case 1:
        if (fontGetStatus() != -1) {
            fontClear();
            ev_s_step++;
        }
        break;
    case 2:
        return 1;
    }
    return 0;
}

/** Uses item kind: consumes it, shows its message and plays its sound at the item's timing.
 * @param kind item kind
 * @param message message number
 * @param se sound id, or 0
 * @param stereo stereo position of the sound when there is no pos
 * @param pos 3D position of the sound, or NULL
 * @param xxx non-zero to take control of the player (event mode) meanwhile
 * @return 1 when done */
int EvSubItemUse0(int kind, int message, int se, int stereo, float *pos, int xxx) {
    switch (ev_s_step) {
    case 0:
        if (xxx) {
            SCNowPlayableEventSwitch(sh2jms.player, 1);
        }
        ItemUse(kind);
        fontMessageNum(msg_buffer, message);
        if (se && ItemUseSeTiming(kind, 0)) {
            if (pos) {
                SeCallPos(se, 1.0f, pos, 0);
            } else {
                SeCall(se, 1.0f, stereo);
            }
        }
        ev_s_step++;
        break;
    case 1:
        if (fontGetStatus() == -2) {
            fontClear();
            if (se && ItemUseSeTiming(kind, 1)) {
                if (pos) {
                    SeCallPos(se, 1.0f, pos, 0);
                } else {
                    SeCall(se, 1.0f, stereo);
                }
            }
            ev_s_step++;
        }
        break;
    case 2:
        if (xxx) {
            SCNowPlayableEventSwitch(sh2jms.player, 0);
        }
        return 1;
    }
    return 0;
}

static int ItemUseSeTiming(int kind, int boa) {
    switch (kind) {
    case 0x38:
    case 0x39:
    case 0x3A:
        if (!boa) {
            return 0;
        }
        break;
    default:
        if (boa) {
            return 0;
        }
        break;
    }
    return 1;
}

/** Shows the message of item kind; once closed, adds the item and deletes its model.
 * @param kind item kind
 * @param message message number
 * @return 1 when done */
int EvSubItemGet(int kind, int message) {
    /* Matching: the original called it without a prototype (arguments unconverted; its line table leaves no
     * line for a declaration, so probably implicitly, which sh2.h's prototype rules out here). */
    int shCharacter_Manage_Delete();
    switch (ev_s_step) {
    case 0:
        fontMessageNum(msg_buffer, message);
        ev_s_step++;
        break;
    case 1:
        if (fontGetStatus() == -2) {
            ItemGet(kind);
            shCharacter_Manage_Delete(NULL, item_to_chara[kind], 0);
            fontClear();
            ev_s_step++;
        }
        break;
    case 2:
        return 1;
    }
    return 0;
}

/** Picks up item kind with the player's pick-up animation (when the item's model is near his
 * height) and shows its message.
 * @param kind item kind
 * @param message message number
 * @return 1 when done */
int EvSubItemGetAndAnim(int kind, int message) {
    struct SubCharacter *scp;

    switch (ev_s_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        scp = shCharacterGetSubCharacter(item_to_chara[kind], 0);
        if (scp && sh2jms.player->pos.y - scp->pos.y < 100.0f) {
            PlayerEventAnimeSet(0x4E21);
            EV_S_STEP(9);
        } else {
            PlayerEventAnimeSet(0x65);
            EV_S_STEP(10);
        }
        break;
    case 9:
        if (ev_cancel) {
            ev_prog_flag_set = 0;
            EV_S_STEP(11);
        } else if (PlayerEventAnimeSuccessFrame()) {
            shCharacterAnimePause(sh2jms.player);
            EV_S_STEP(10);
        }
        break;
    case 10:
        fontMessageNum(msg_buffer, message);
        SeCall(0x2B21, 1.0f, 0);
        EV_S_STEP(3);
        break;
    case 3:
        if (fontGetStatus() == -2 || ev_cancel) {
            ItemGet(kind);
            fontClear();
            scp = shCharacterGetSubCharacter(item_to_chara[kind], 0);
            if (scp && sh2jms.player->pos.y - scp->pos.y < 100.0f) {
                shCharacterAnimeRestart(sh2jms.player);
                EV_S_STEP(8);
            } else {
                EV_S_STEP(11);
            }
            if (scp) {
                shCharacter_Manage_Delete(scp, 0, 0);
            }
        }
        break;
    case 8:
        if (shCharacterAnimeIsEnd(sh2jms.player) || ev_cancel) {
            EV_S_STEP(11);
        }
        break;
    case 11:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

/**
 * Loads a picture (file_0, into the gp data buffer) and optionally a layer image drawn over it
 * (file_1, right after it: layer_adr) while fading the screen out.
 * @param mode unused (the DWARF drops it)
 * @param file_0 the picture, or NULL
 * @param file_1 the layer, or NULL
 * @return 1 when done
 */
int EvSubFileLoadAndFadeOut(int mode, union fsFileIndex *file_0, union fsFileIndex *file_1) {
    /* Matching: the assert bakes its original line number into the object. */
#line 332
    assert(file_0 || !file_1);
    if (ev_s_step == 0) {
        if (file_0) {
            FcRead(file_0, get_gp_data_buf_addr());
            layer_adr = ((FcGetFileSize(file_0) + 0x7FF) & ~0x7FF) + get_gp_data_buf_addr();
        }
        if (file_1) {
            FcRead(file_1, layer_adr);
        }
        ScreenEffectFadeStart(1, 0.0f);
        ev_s_step = 3;
    }
    if (fsSync(1, -1) >= 0 && ScreenEffectFadeCheck()) {
        return 1;
    }
    return 0;
}

/** Draws the loaded picture and fades it in.
 * @param fade fade speed
 * @return 1 when done */
int EvSubPictureDisplayAndFadeIn(float fade) {
    if (ev_s_step == 0) {
        ScreenEffectFadeStart(4, fade);
        ev_s_step = 3;
    }
    EvSubPictureStart();
    EvSubPictureDisplayOnly();
    EvSubPictureEnd();
    return ScreenEffectFadeCheck();
}

/** Draws the loaded picture (in the gp data buffer) for this frame.
 * @return 1 */
int EvSubPictureDisplayOnly(void) {
    struct PicDraw_Data pic;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(), 0, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr();
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.otp = 1;
    PictureDraw(&pic);
    return 1;
}

/** Draws the loaded picture and fades it out.
 * @param fade fade speed
 * @return 1 when done */
int EvSubPictureDisplayAndFadeOut(float fade) {
    if (ev_s_step == 0) {
        ScreenEffectFadeStart(1, fade);
        ev_s_step = 3;
    }
    EvSubPictureStart();
    EvSubPictureDisplayOnly();
    EvSubPictureEnd();
    return ScreenEffectFadeCheck();
}

/** Full sequence for a picture: load it, fade in, show it with message msg, fade out.
 * @param file the picture
 * @param msg message number, or 0
 * @return 1 when done */
int EvSubPictureDisplay(union fsFileIndex *file, int msg) {
    switch (ev_s_step) {
    case 0:
        FcRead(file, get_gp_data_buf_addr());
        ScreenEffectFadeStart(1, 0.0f);
        EvSubPictureInit();
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        ev_s_step = 4;
        break;
    case 4:
        if (fsSync(1, -1) >= 0 && ScreenEffectFadeCheck()) {
            ScreenEffectFadeStart(4, 0.0f);
            ev_s_step = 7;
        }
        break;
    case 7:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (ScreenEffectFadeCheck()) {
            ev_s_step = 2;
        }
        break;
    case 2:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            fontMessageNum(msg_buffer, msg);
            ev_s_step = 3;
        }
        break;
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (fontGetStatus() == -2) {
            fontClear();
            ScreenEffectFadeStart(2, 0.0f);
            ev_s_step = 6;
        }
        break;
    case 6:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (ScreenEffectFadeCheck()) {
            ev_s_step = 11;
        }
        break;
    case 11:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        ScreenEffectFadeStart(4, 0.0f);
        return 1;
    }
    return 0;
}

/** Shows a picked-up map (EvSubPictureDisplay).
 * @param file the map picture
 * @param msg message number
 * @return 1 when done */
int EvSubMapGet(union fsFileIndex *file, int msg) {
    return EvSubPictureDisplay(file, msg);
}

/** Draws the layer image (layer_adr) over the picture in a rectangle.
 * @param x0 left
 * @param y0 top
 * @param x1 right
 * @param y1 bottom
 * @param alpha opacity */
void EvSubPictureLayer(int x0, int y0, int x1, int y1, int alpha) {
    struct PicDraw_Data pic;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.otp = 3;
    pic.x0 = x0;
    pic.y0 = y0;
    pic.x1 = x1;
    pic.y1 = y1;
    pic.status |= 2;
    pic.a = alpha;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    PictureDraw(&pic);
}

/** Keeps the dark filter over the picture on for this frame. */
void EvSubPictureFilter(void) {
    ev_filter_on = 1;
}

/** Resets the picture filter. */
void EvSubPictureInit(void) {
    ev_filter_on = 0;
    ev_filter = 0.0f;
}

/** Starts a picture frame: clears the screen to black and sets Sh2sys.main_status bit 0. */
void EvSubPictureStart(void) {
    sh2gfw_Black_Clear();
    Sh2sys.main_status |= 1;
}

/** Fades the dark filter over the picture in (if EvSubPictureFilter was called this frame) or
 * out, and draws it. */
void EvSubPictureEnd(void) {
    struct PicDraw_Data pic;

    if (ev_filter_on) {
        ev_filter += 4.0f * shGetDT();
        if (ev_filter > 1.0f) {
            ev_filter = 1.0f;
        }
    } else {
        ev_filter -= 4.0f * shGetDT();
        if (ev_filter < 0.0f) {
            ev_filter = 0.0f;
        }
    }
    ev_filter_on = 0;
    if (ev_filter > 0.0f) {
        shQzero(&pic, sizeof(pic));
        pic.r = 0;
        pic.g = 0;
        pic.b = 0;
        pic.status |= 0x10;
        pic.a = 0x80;
        pic.alpha_a = 2;
        pic.alpha_b = 1;
        pic.alpha_c = 2;
        pic.alpha_d = 1;
        pic.alpha_fix = ftoi(64.0f * ev_filter);
        pic.status |= 0x20;
        pic.otp = 8;
        PictureDraw(&pic);
    }
    d1cSend(spkDmaKick());
}

/** Moves the cursor over a picture with the left stick and draws it.
 * @param color cursor colour */
void EvSubPictureCursor(int color) {
    struct PicDraw_Data pic;
    int px;
    int py;
    float anx;
    float any;
    unsigned char lsx;
    unsigned char lsy;

    lsx = shPadPress(0, 0x40);
    if (lsx >= 0x9B) {
        anx = (lsx - 0x9B) / 100.0f;
    } else if (lsx <= 0x64) {
        anx = (lsx - 0x64) / 100.0f;
    } else {
        anx = 0.0f;
    }
    lsy = shPadPress(0, 0x80);
    if (lsy >= 0x9B) {
        any = (lsy - 0x9B) / 100.0f;
    } else if (lsy <= 0x64) {
        any = (lsy - 0x64) / 100.0f;
    } else {
        any = 0.0f;
    }
    if (anx != 0.0f || any != 0.0f) {
        ev_cursor_x += anx * (153.6f * shGetDT());
        ev_cursor_y += any * (192.0f * shGetDT());
    } else {
        if (shPadPress(0, 0x100)) {
            ev_cursor_x += 153.6f * shGetDT();
        }
        if (shPadPress(0, 0x200)) {
            ev_cursor_x -= 153.6f * shGetDT();
        }
        if (shPadPress(0, 0x800)) {
            ev_cursor_y += 192.0f * shGetDT();
        }
        if (shPadPress(0, 0x400)) {
            ev_cursor_y -= 192.0f * shGetDT();
        }
    }
    ev_cursor_x = fmaxf(-256.0f, fminf(256.0f, ev_cursor_x));
    ev_cursor_y = fmaxf(-192.0f, fminf(192.0f, ev_cursor_y));
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)cursor_adr, 8, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)cursor_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.otp = 9;
    pic.a = 0x80;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    px = ftoi4(ev_cursor_x - 5.0f);
    py = ftoi4(ev_cursor_y - 6.0f);
    pic.x0 = px;
    pic.y0 = py;
    pic.x1 = px + 0x180;
    pic.y1 = py + 0x200;
    pic.status |= 2;
    pic.us0 = 0x600;
    pic.vt0 = 0x400;
    pic.us1 = 0x8F0;
    pic.vt1 = 0x7F0;
    pic.status |= 4;
    switch (color) {
    case 1:
        PIC_SET_RGB(pic, 0x60, 0x60, 0x80);
        break;
    case 2:
        PIC_SET_RGB(pic, 0x80, 0x60, 0x60);
        break;
    case 4:
        PIC_SET_RGB(pic, 0x60, 0x80, 0x60);
        break;
    }
    PictureDraw(&pic);
}

/** Sets bit no in the display on/off mask of room (in the current stage) in a model list;
 * the list holds pairs (stage << 16 | room, mask) ended by 0.
 * @param list the model list
 * @param room room number
 * @param no model bit, or negative to only add the room */
void EvDispControlModelEntry(int *list, int room, int no) {
    int map_id;

    map_id = stage->glb_crd << 16 | room;
    while (*list != 0) {
        if (*list == map_id) {
            break;
        }
        list += 2;
    }
    if (*list == 0) {
        list[1] = 0;
        list[2] = 0;
    }
    list[0] = map_id;
    if (no >= 0) {
        list[1] |= 1 << no;
    }
}

/** Applies a model list's display on/off masks to the stage models.
 * @param list the model list */
void EvDispControlModelExec(int *list) {
    if (BgIsOut(0)) {
        sh2gfw_Init_DispOnOffObj();
        for (; *list != 0; list += 2) {
            sh2gfw_FastSet_DispOnOffObj(list[0], list[1]);
        }
    } else {
        while (*list != 0) {
            sh2gfw_Set_DispOnOffObj(list[0], list[1]);
            list += 2;
        }
    }
}

/** Prepares a movie with its subtitles, after the drama demo playing (if any) has reached its end.
 * @param file the movie
 * @param msg_time subtitle start/end frames
 * @param msg_no first subtitle message */
void EvSubMovieReady(union fsFileIndex *file, struct DramaDemo_MessageTime *msg_time, int msg_no) {
    if (Sh2sys.main_status >> 6 & 1) {
        demo_status |= 0x200;
        if (demo_frame < total_demo_frame - 30.0f) {
            return;
        }
        if ((shSdStat() & 0xF0) && (shSdStat() & 0xF0) != 0x50) {
            return;
        }
    }
    if (MovieCheckSleep()) {
        Sh2sys.soft_reset = 0;
        MoviePreSet(file);
        movieSetSubTitleData(msg_buffer, msg_time, msg_no);
    }
}

/** Starts the prepared movie once it is ready, switching the game to the movie step.
 * @param demo non-zero to start it (0: only wait)
 * @return 1 once started */
int EvSubMovieStart(int demo) {
    int movie;

    movie = MovieWaitReady();
    if (movie && demo && shGs_AllEnv.loop3 % 3 == 1) {
        sh2gfw_Set_PauseRetain();
        Sh2sys.step[2] = 14;
        Sh2sys.step[3] = 0;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        return 1;
    }
    return 0;
}

/** Cleans up after a movie (soft reset allowed again, filters cleared). */
void EvSubMovieEnd(void) {
    Sh2sys.soft_reset = 1;
    sh2gfw_Reset_FilterCommand();
}
