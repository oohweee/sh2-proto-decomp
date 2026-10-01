/*
 * now_loading.c: the "Now loading" text shown during long screen fades.
 */

#include "sh2.h"

static int now_loading_enable = 0;
static int now_loading_draw = 0;

/** Asks for the text to be shown this frame (if the fade allows it). */
void NowLoadingEnable(void) {
    now_loading_enable = 1;
}

/**
 * Stretches the current screen fade to at least 4 frames and decides whether the text is drawn this
 * frame; clears the request.
 */
void NowLoadingCheck(void) {
    int time;
    int fade;

    time = scr_efct.fade_type;
    switch (time) {
    case 2:
    case 1:
        fade = scr_efct.fade_timer_max;
        if (fade < 4.0f) {
            if (ScreenEffectFadeCheck()) {
                time = scr_efct.fade_timer_now;
                ScreenEffectInit();
                ScreenEffectFadeStart(1, 4.0f);
                scr_efct.fade_timer_now = time;
                scr_efct.fade_timer_max = 4.0f;
            }
        } else if (time == 2 && scr_efct.fade_timer_now < scr_efct.fade_timer_max) {
        } else if (scr_efct.fade_timer_now < 4.0f) {
        } else if (scr_efct.fade_timer_max != 4.0f || time != 1) {
            ScreenEffectInit();
            ScreenEffectFadeStart(1, 4.0f);
            scr_efct.fade_timer_now = 0.0f;
            scr_efct.fade_timer_max = 4.0f;
        } else if (now_loading_enable) {
            now_loading_draw = 1;
        }
        break;
    }
    now_loading_enable = 0;
}

/** Draws the text (with a small jitter) every 64 frames while it is on. */
void NowLoadingDraw(void) {
    static int count;
    int x;
    int y;

    if (now_loading_draw) {
        if (count % 64 == 0) {
            y = (count * 0x2F6F) % 5 - 2;
            x = (count * 0xCE5FDD) % 5 - 2;
            sh2gfw_InclimentLoopCounter(&shGs_AllEnv);
            fontSetColorDirect(0x80, 0x80, 0x80, 0x80);
            fontPrintStr(dicSetStr("\\c\\hNow loading"), x + 0x100, y + 0x100);
            fjFontDrawExecVif1();
            fontClear();
            sh2gfw_DeclimentLoopCounter(&shGs_AllEnv);
        }
        count++;
    } else {
        count = 0;
    }
    now_loading_draw = 0;
}
