/*
 * screen_effect.c: full-screen fades to and from black, white or red, driven through the
 * sh2gfw filter commands. fade_status: 0 clear, 1 fading out, 2 faded out, 3 fading in.
 */
#include "sh2.h"

struct ScreenEffect_Parameter scr_efct;

/** Clears the fade state. */
void ScreenEffectInit(void) {
    scr_efct.fade_status = 0;
    scr_efct.fade_type = 0;
    scr_efct.fade_timer_now = scr_efct.fade_timer_max = 0.0f;
}

/**
 * Per-frame update: advances the fade timer; when it runs out, a fade-out becomes "faded out" and a
 * fade-in ends.
 */
void ScreenEffectManager(void) {
    scr_efct.fade_timer_now += shGetDT();
    if (scr_efct.fade_timer_now >= scr_efct.fade_timer_max) {
        scr_efct.fade_timer_now = scr_efct.fade_timer_max;
        if (scr_efct.fade_status == 1) {
            scr_efct.fade_status = 2;
        }
        if (scr_efct.fade_status == 3) {
            scr_efct.fade_status = 0;
        }
    }
}

/**
 * Starts a fade, unless the screen is already in (or heading to) that state.
 * @param type 1/2/3 black out (retained, normal, immediate), 4/5 in from black (timed,
 *             immediate), 6-10 the same for white, 11 red out, 12 in from red
 * @param time fade duration; 0 means 1.2
 */
void ScreenEffectFadeStart(int type, float time) {
    if (time == 0.0f) {
        time = 1.2f;
    }

    switch (type) {
    case 1:
    case 2:
    case 6:
    case 7:
    case 11:
        if (scr_efct.fade_status == 1 || scr_efct.fade_status == 2) {
            return;
        }
        scr_efct.fade_status = 1;
        break;
    case 3:
    case 8:
        if (scr_efct.fade_status == 2) {
            return;
        }
        scr_efct.fade_status = 2;
        break;
    case 4:
    case 9:
    case 12:
        if (scr_efct.fade_status == 3 || scr_efct.fade_status == 0) {
            return;
        }
        scr_efct.fade_status = 3;
        break;
    case 5:
    case 10:
        if (scr_efct.fade_status == 0) {
            return;
        }
        scr_efct.fade_status = 0;
        break;
    }

    scr_efct.fade_type = type;
    scr_efct.fade_timer_now = 0.0f;
    scr_efct.fade_timer_max = time;

    switch (type) {
    case 0:
        break;
    case 1:
        sh2gfw_Set_FadeOutRetain_Black(time);
        break;
    case 2:
        sh2gfw_Set_FadeOut_Black(time);
        break;
    case 3:
        sh2gfw_Set_FadeOut_Black(-1.0f);
        break;
    case 4:
        sh2gfw_Set_FadeIn_Black(time);
        break;
    case 5:
        sh2gfw_Set_FadeIn_Black(-1.0f);
        break;
    case 6:
        sh2gfw_Set_FadeOutRetain_White(time);
        break;
    case 7:
        sh2gfw_Set_FadeOut_White(time);
        break;
    case 8:
        sh2gfw_Set_FadeOut_White(-1.0f);
        break;
    case 9:
        sh2gfw_Set_FadeIn_White(time);
        break;
    case 10:
        sh2gfw_Set_FadeIn_White(-1.0f);
        break;
    case 11: {
        /* Matching: called without a prototype in the original (arguments passed unconverted). */
        void sh2gfw_Set_FadeOutRetain_Red();
        sh2gfw_Set_FadeOutRetain_Red(time);
        break;
    }
    case 12:
        sh2gfw_Set_FadeIn_Red(time);
        break;
    }
}

/** Returns non-zero when no fade is in progress (the screen is clear or fully faded out). */
int ScreenEffectFadeCheck(void) {
    return scr_efct.fade_status == 0 || scr_efct.fade_status == 2;
}

/** Cancels any fade: resets the filter commands and the fade state. */
void ScreenEffectFadeStop(void) {
    sh2gfw_Reset_FilterCommand();
    ScreenEffectInit();
}
