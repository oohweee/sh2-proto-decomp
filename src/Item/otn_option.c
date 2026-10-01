/*
 * otn_option.c: the options screen (brightness, key config, screen position,
 * extra options) and its board/cursor/arrow drawing.
 */
#include "sh2.h"

static OptionWork t;
static char key_cur;
static char key_cur2;
static unsigned char key_rem[20];
static struct PicDraw_Data i_pic;

/* left / right on the d-pad or the analog stick (one step per push) */
#define PAD_L (shPadTrigger(0, 0x200) || (shPadPress(0, 0x40) < 0x40 && t.ana_x == 0))
#define PAD_R (shPadTrigger(0, 0x100) || (shPadPress(0, 0x40) > 0xC0 && t.ana_x == 0))
#define PAD_LR                                                                              \
    (shPadTrigger(0, 0x200) || shPadTrigger(0, 0x100) ||                                    \
     (shPadPress(0, 0x40) < 0x40 && t.ana_x == 0) || (shPadPress(0, 0x40) > 0xC0 && t.ana_x == 0))
/* left / right with key repeat (the stick repeats every 3rd frame after 16) */
#define PAD_REP_L                                                                           \
    (shPadRepeat(0, 0x200) ||                                                               \
     (shPadPress(0, 0x40) < 0x40 && (t.ana_x == 0 || (t.ana_x > 15 && t.ana_x % 3 == 0))))
#define PAD_REP_R                                                                           \
    (shPadRepeat(0, 0x100) ||                                                               \
     (shPadPress(0, 0x40) > 0xC0 && (t.ana_x == 0 || (t.ana_x > 15 && t.ana_x % 3 == 0))))

/** Options screen main, one step per call: runs the current page (main, brightness, key config,
 * screen position) and, on leaving, applies the sound setting and returns to the caller's step.
 * @return 1 once the options screen has closed */
int option_main(void) {
    float f;

    t.timer += shGetDF();
    if (Sh2sys.step[4] == 0) {
        t.option_step = 0;
        Sh2sys.step[4]++;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
    }
    fontClear();
    f = 255.0f;
    fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0xFF);
    option_font();
    look_scr();
    switch (t.option_step) {
    case 0:
        t.fade = 0xFF;
        t.extra_mode = 0;
        t.cursol = 0;
        t.timer = 0.0f;
        t.option_step = 1;
        t.cursol_pos = 0.0f;
        t.fade_flag = 0;
        t.hoge = 0;
        t.ana_x = 0;
        t.ana_y = 0;
        DataLoadMessage(2);
        KeyConfigPictureLoad();
        break;
    case 1:
        if (fsSync(1, -1) >= 0) {
            t.option_step = 2;
            ScreenEffectFadeStart(3, 0.0f);
        }
        break;
    case 2:
        option_mainmain();
        break;
    case 3:
        bright_main();
        break;
    case 4:
        config_main();
        break;
    case 5:
        position_main();
        break;
    case 6:
        switch (Sh2sys.step[3]) {
        case 0:
            Sh2sys.step[3] = 1;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
            break;
        case 1:
            break;
        case 2:
            Sh2sys.step[2] = 6;
            Sh2sys.step[3] = 0;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
            break;
        case 3:
            Sh2sys.step[2] = 1;
            Sh2sys.step[3] = 0;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
            Sh2sys.step[3] = 6;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
            break;
        }
        t.option_step = 0;
        if (playing.sound == 0) {
            shSdCall(0x3ED, 0, 0, 0);
            shSdCall(0x411, 0, 0, 0);
        } else if (playing.sound == 1) {
            shSdCall(0x3ED, 0, 0, 0);
            shSdCall(0x410, 0, 0, 0);
        } else {
            shSdCall(0x3EE, 0, 0, 0);
            shSdCall(0x411, 0, 0, 0);
        }
        return 1;
    }
    if (shPadPress(0, 0x40) <= 0xC0 && shPadPress(0, 0x40) >= 0x40) {
        t.ana_x = 0;
    } else {
        t.ana_x++;
    }
    if (shPadPress(0, 0x80) <= 0xC0 && shPadPress(0, 0x80) >= 0x40) {
        t.ana_y = 0;
    } else {
        t.ana_y++;
    }
    return 0;
}

/** The main options page: cursor, value changes (with the left/right arrows) and leaving. */
void option_mainmain(void) {
    if (t.fade_flag == 0) {
        fade_in();
    }
    look_board();
    look_cur();
    if (t.extra_mode == 0) {
        if (shPadTrigger(0, key_config.cancel) && t.fade == 0) {
            t.fade_flag = 1;
        }
        if (shPadTrigger(0, key_config.enter) && t.fade == 0) {
            if (t.cursol == 0) {
                t.fade_flag = 2;
            }
            if (t.cursol == 1) {
                t.fade_flag = 3;
            }
            if (t.cursol == 2) {
                t.fade_flag = 4;
            }
            t.key_config_set = key_config;
        }
        if ((shPadTrigger(0, 0x10000) || shPadTrigger(0, 0x20000)) && t.fade == 0) {
            t.fade_flag = 6;
        }
        switch (t.cursol) {
        case 3:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(110, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_LR) {
                playing.control_type = 1 - playing.control_type;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 4:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(102, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_R && playing.vibration == 3) {
                playing.vibration = 0;
            } else if (PAD_R) {
                playing.vibration++;
            } else if (PAD_L && playing.vibration == 0) {
                playing.vibration = 3;
            } else if (PAD_L) {
                playing.vibration--;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 5:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(58, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_LR) {
                playing.auto_load = 1 - playing.auto_load;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 6:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(110, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_R && playing.language == 1) {
                playing.language = 0;
                DataLoadMessage(2);
                fsSync(0, -1);
            } else if (PAD_R) {
                playing.language++;
                DataLoadMessage(2);
                fsSync(0, -1);
            } else if (PAD_L && playing.language == 0) {
                playing.language = 1;
                DataLoadMessage(2);
                fsSync(0, -1);
            } else if (PAD_L) {
                playing.language--;
                DataLoadMessage(2);
                fsSync(0, -1);
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 7:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(58, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_LR) {
                playing.subtitles = 1 - playing.subtitles;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 8:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(220, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_R && playing.sound == 2) {
                playing.sound = 0;
            } else if (PAD_R) {
                playing.sound++;
            } else if (PAD_L && playing.sound == 0) {
                playing.sound = 2;
            } else if (PAD_L) {
                playing.sound--;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 9:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(197, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_REP_L && playing.bgm_volume > 0) {
                playing.bgm_volume--;
                SeMasterVolumeChange();
                SeCall(0x4A49, 1.0f, 0);
            }
            if (PAD_REP_R && playing.bgm_volume < 15) {
                playing.bgm_volume++;
                SeMasterVolumeChange();
                SeCall(0x4A49, 1.0f, 0);
            }
            break;
        case 10:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(197, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_REP_L && playing.se_volume > 0) {
                playing.se_volume--;
                SeMasterVolumeChange();
                SeCall(0x4A49, 1.0f, 0);
            }
            if (PAD_REP_R && playing.se_volume < 15) {
                playing.se_volume++;
                SeMasterVolumeChange();
                SeCall(0x4A49, 1.0f, 0);
            }
            break;
        }
        if (t.fade_flag == 1) {
            t.option_step = fade_out(6);
        }
        if (t.fade_flag == 2) {
            t.option_step = fade_out(3);
        }
        if (t.fade_flag == 3) {
            t.option_step = fade_out(5);
        }
        if (t.fade_flag == 4) {
            t.option_step = fade_out(4);
        }
        if (t.fade_flag == 6) {
            t.option_step = fade_out(2);
        }
        if (ScreenEffectFadeCheck() && t.fade == 0xFF && t.option_step == 2) {
            t.cursol = 0;
            t.extra_mode = 1;
        }
        look_bgm();
        look_se();
    } else {
        if ((shPadTrigger(0, key_config.cancel) || shPadTrigger(0, 0x10000) || shPadTrigger(0, 0x20000)) &&
            t.fade == 0) {
            t.fade_flag = 1;
        }
        if (t.fade_flag == 1) {
            t.option_step = fade_out(2);
        }
        switch (t.cursol) {
        case 0:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(92, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_LR) {
                playing.weapon_control = 1 - playing.weapon_control;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 1:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(101, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_R && playing.blood_color == 3) {
                playing.blood_color = 0;
            } else if (PAD_R) {
                playing.blood_color++;
            } else if (PAD_L && playing.blood_color == 0) {
                playing.blood_color = 3;
            } else if (PAD_L) {
                playing.blood_color--;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 2:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(101, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_LR) {
                playing.view_control = 1 - playing.view_control;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 3:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(101, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_LR) {
                playing.retreat_turn = 1 - playing.retreat_turn;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 4:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(101, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_LR) {
                playing.walk_run_control = 1 - playing.walk_run_control;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 5:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(120, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_LR) {
                playing.view_mode = 1 - playing.view_mode;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        case 6:
            allow_l(-5, t.cursol * 27 - 140);
            allow_r(61, t.cursol * 27 - 140);
            if (t.fade != 0) {
                break;
            }
            if (PAD_R && playing.bullet_adjust == 6) {
                playing.bullet_adjust = 1;
            } else if (PAD_R) {
                playing.bullet_adjust++;
            } else if (PAD_L && playing.bullet_adjust == 1) {
                playing.bullet_adjust = 6;
            } else if (PAD_L) {
                playing.bullet_adjust--;
            }
            if (PAD_LR) {
                SeCall(10000, 1.0f, 0);
            }
            break;
        }
        if (ScreenEffectFadeCheck() && t.fade == 0xFF && t.option_step == 2) {
            t.cursol = 0;
            t.extra_mode = 0;
        }
    }
}

/** The brightness page. */
void bright_main(void) {
    int i;

    if (t.fade_flag == 0) {
        fade_in();
    }
    if (shPadTrigger(0, key_config.cancel) && t.fade == 0) {
        t.fade_flag = 1;
    }
    if (t.fade_flag == 1) {
        t.option_step = fade_out(2);
    }
    if (t.fade == 0) {
        if (PAD_REP_R && playing.brightness_level < 7) {
            playing.brightness_level++;
        }
        if (PAD_REP_L && playing.brightness_level > 0) {
            playing.brightness_level--;
        }
    }
    sh2gfw_Set_Brightness(playing.brightness_level);
    for (i = 0; i < 20; i++) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0004, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
        *spack.pos++ = 0x41;
        *spack.pos++ = ((long)(playing.brightness_level * 12) << 24) | 0x808080;
        *spack.pos++ = (long)zs(i * 20 - 190) | ((long)zs(-80) << 16) | ((long)zs(0) << 32) | 0xFF00000000000000;
        *spack.pos++ = (long)zs(i * 20 - 190) | ((long)zs(150) << 16) | ((long)zs(0) << 32) | 0xFF00000000000000;
        spkCloseGiftag();
    }
    allow_l(10, 180);
    allow_r(90, 180);
}

/** Draws the button picture of one key-config entry.
 * @param con which button (0-11)
 * @param xx x position (advanced past the picture)
 * @param yy row */
void print_config(int con, int *xx, int yy) {
    yy = yy * 27 + 79;
    if (con == 0) {
        key_draw(5, *xx, yy);
    }
    if (con == 1) {
        key_draw(4, *xx, yy);
    }
    if (con == 2) {
        key_draw(6, *xx, yy);
    }
    if (con == 3) {
        key_draw(7, *xx, yy);
    }
    if (con == 12) {
        key_draw(2, *xx, yy);
    }
    if (con == 13) {
        key_draw(1, *xx, yy);
    }
    if (con == 14) {
        key_draw(3, *xx, yy);
    }
    if (con == 15) {
        key_draw(0, *xx, yy);
    }
    if (con == 16) {
        key_draw(8, *xx, yy);
    }
    if (con == 17) {
        key_draw(11, *xx, yy);
    }
    if (con == 18) {
        key_draw(9, *xx, yy);
    }
    if (con == 19) {
        key_draw(10, *xx, yy);
    }
    *xx += 27;
}

/** Returns the mask of the buttons pressed this frame (the ones a key can be assigned to). */
int key_check(void) {
    int push_key;

    push_key = 0;
    if (shPadTrigger(0, 0x1000)) {
        push_key |= 0x1000;
    }
    if (shPadTrigger(0, 0x8000)) {
        push_key |= 0x8000;
    }
    if (shPadTrigger(0, 0x4000)) {
        push_key |= 0x4000;
    }
    if (shPadTrigger(0, 0x2000)) {
        push_key |= 0x2000;
    }
    if (shPadTrigger(0, 0x10000)) {
        push_key |= 0x10000;
    }
    if (shPadTrigger(0, 0x20000)) {
        push_key |= 0x20000;
    }
    if (shPadTrigger(0, 0x40000)) {
        push_key |= 0x40000;
    }
    if (shPadTrigger(0, 0x80000)) {
        push_key |= 0x80000;
    }
    if (shPadTrigger(0, 1)) {
        push_key |= 1;
    }
    if (shPadTrigger(0, 2)) {
        push_key |= 2;
    }
    if (shPadTrigger(0, 8)) {
        push_key |= 8;
    }
    if (shPadTrigger(0, 4)) {
        push_key |= 4;
    }
    return push_key;
}

/** Sets key configuration type 1 (enter/action on 0x2000, cancel/dash on 0x4000, light 0x8000). */
void key_type1(void) {
    t.key_config_set.enter = 0x2000;
    t.key_config_set.cancel = 0x4000;
    t.key_config_set.skip = 4;
    t.key_config_set.right_move = 0x20000;
    t.key_config_set.left_move = 0x10000;
    t.key_config_set.action = 0x2000;
    t.key_config_set.dash = 0x4000;
    t.key_config_set.light = 0x8000;
    t.key_config_set.item = 0xC;
    t.key_config_set.search_view = 0x40000;
    t.key_config_set.ready = 0x80000;
    t.key_config_set.map = 0x1000;
    t.key_config_set.pause = 0;
}

/** Sets key configuration type 2 (enter/action on 0x4000, cancel/dash on 0x8000, light 0x2000). */
void key_type2(void) {
    t.key_config_set.enter = 0x4000;
    t.key_config_set.cancel = 0x8000;
    t.key_config_set.skip = 4;
    t.key_config_set.right_move = 0x20000;
    t.key_config_set.left_move = 0x10000;
    t.key_config_set.action = 0x4000;
    t.key_config_set.dash = 0x8000;
    t.key_config_set.light = 0x2000;
    t.key_config_set.item = 0xC;
    t.key_config_set.search_view = 0x40000;
    t.key_config_set.ready = 0x80000;
    t.key_config_set.map = 0x1000;
    t.key_config_set.pause = 0;
}

/** Sets key configuration type 3. */
void key_type3(void) {
    t.key_config_set.enter = 0xA004;
    t.key_config_set.cancel = 0x4000;
    t.key_config_set.skip = 4;
    t.key_config_set.right_move = 0x80000;
    t.key_config_set.left_move = 0x40000;
    t.key_config_set.action = 0xA000;
    t.key_config_set.dash = 0x4000;
    t.key_config_set.light = 0x1000;
    t.key_config_set.item = 4;
    t.key_config_set.search_view = 0x10000;
    t.key_config_set.ready = 0x20000;
    t.key_config_set.map = 8;
    t.key_config_set.pause = 0;
}

/** Takes the buttons in *key_a away from *key_b; if that would leave an important action with no
 * button, *key_a loses *key_b's buttons instead.
 * @param key_a the new buttons
 * @param key_b another action's buttons
 * @param important non-zero if key_b's action must keep a button */
/* Matching: ~x is written `x ^ 0xFFFFFFFF` at all three places, as the original's `li v1,-1; xor` shows;
 * the other spellings give `not` (docs/matching-notes.md#otn_option-key_conf). */
void key_conf(int *key_a, int *key_b, unsigned char important) {
    int i;

    i = *key_b;
    i &= *key_a ^ 0xFFFFFFFF;
    if (!important || i) {
        *key_b &= *key_a ^ 0xFFFFFFFF;
    } else {
        *key_a &= *key_b ^ 0xFFFFFFFF;
    }
}

/** Takes the buttons of the action just changed away from every other action (key_conf).
 * @param key_num index of the action just changed
 * @param i its button mask */
void key_conf_check(int key_num, int *i) {
    if (key_num != 4) {
        key_conf(i, &t.key_config_set.right_move, 0);
    }
    if (key_num != 5) {
        key_conf(i, &t.key_config_set.left_move, 0);
    }
    if (key_num != 6) {
        key_conf(i, &t.key_config_set.action, 1);
    }
    if (key_num != 7) {
        key_conf(i, &t.key_config_set.dash, 0);
    }
    if (key_num != 8) {
        key_conf(i, &t.key_config_set.light, 0);
    }
    if (key_num != 9) {
        key_conf(i, &t.key_config_set.item, 1);
    }
    if (key_num != 10) {
        key_conf(i, &t.key_config_set.search_view, 0);
    }
    if (key_num != 11) {
        key_conf(i, &t.key_config_set.ready, 1);
    }
    if (key_num != 12) {
        key_conf(i, &t.key_config_set.map, 0);
    }
    if (key_num != 13) {
        key_conf(i, &t.key_config_set.pause, 0);
    }
}

/** Sets the text colour of a key-config line (bright for the cursor line).
 * @param col the line */
void key_color(unsigned char col) {
    if (col == key_cur) {
        fontSetColorDirect(0x80, 0x80, 0x80, 0xFF);
    } else {
        fontSetColorDirect(0x20, 0x20, 0x20, 0xFF);
    }
}

#define PRINT_KEYS(n, key)                              \
    if (i == (n) && (t.key_config_set.key & (1 << j))) { \
        print_config(j, &k, i);                         \
    }

/** The key-config page. */
void config_main(void) {
    int i;
    int j;
    int k;
    char buf[2];

    if (t.fade_flag == 0) {
        fade_in();
    }
    if ((shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) && t.fade == 0 && key_cur == 0 &&
        key_cur2 == 0) {
        t.fade_flag = 1;
    }
    if (t.fade_flag == 1) {
        key_config = t.key_config_set;
        key_config.attack = key_config.action;
        shPadSetGameKeyAssign();
        t.option_step = fade_out(2);
    }
    KeyConfigPitureStart();
    key_color(0);
    if (key_cur2 == 0 && key_cur == 0) { fontSetColorDirect(0x80, 0x80, 0x80, 0xFF); }
    else { fontSetColorDirect(0x20, 0x20, 0x20, 0xFF); }
    fontPrintStrNum(msg_buffer, 8, 30, 60);
    if (key_cur2 == 1 && key_cur == 0) { fontSetColorDirect(0x80, 0x80, 0x80, 0xFF); }
    else { fontSetColorDirect(0x20, 0x20, 0x20, 0xFF); }
    sprintf(buf, "1");
    fontSetMes(0, dicSetStr(buf));
    fontPrintStrNum(msg_buffer, 9, 140, 60);
    if (key_cur2 == 2 && key_cur == 0) { fontSetColorDirect(0x80, 0x80, 0x80, 0xFF); }
    else { fontSetColorDirect(0x20, 0x20, 0x20, 0xFF); }
    sprintf(buf, "2");
    fontSetMes(0, dicSetStr(buf));
    fontPrintStrNum(msg_buffer, 9, 230, 60);
    if (key_cur2 == 3 && key_cur == 0) { fontSetColorDirect(0x80, 0x80, 0x80, 0xFF); }
    else { fontSetColorDirect(0x20, 0x20, 0x20, 0xFF); }
    sprintf(buf, "3");
    fontSetMes(0, dicSetStr(buf));
    fontPrintStrNum(msg_buffer, 9, 320, 60);
    if (key_cur2 == 4 && key_cur == 0) { fontSetColorDirect(0x80, 0x80, 0x80, 0xFF); }
    else { fontSetColorDirect(0x20, 0x20, 0x20, 0xFF); }
    fontPrintStrNum(msg_buffer, 24, 410, 60);
    key_color(1);
    fontPrintStrNum(msg_buffer, 10, 56, 102);
    key_color(2);
    fontPrintStrNum(msg_buffer, 11, 40, 129);
    key_color(3);
    fontPrintStrNum(msg_buffer, 12, 77, 156);
    key_color(4);
    fontPrintStrNum(msg_buffer, 19, 53, 183);
    key_color(5);
    fontPrintStrNum(msg_buffer, 18, 53, 210);
    key_color(6);
    fontPrintStrNum(msg_buffer, 13, 40, 237);
    key_color(7);
    fontPrintStrNum(msg_buffer, 16, 77, 264);
    key_color(8);
    fontPrintStrNum(msg_buffer, 15, 56, 291);
    key_color(9);
    fontPrintStrNum(msg_buffer, 21, 68, 318);
    key_color(10);
    fontPrintStrNum(msg_buffer, 17, 66, 345);
    key_color(11);
    fontPrintStrNum(msg_buffer, 14, 78, 372);
    key_color(12);
    fontPrintStrNum(msg_buffer, 22, 73, 399);
    key_color(13);
    fontPrintStrNum(msg_buffer, 20, 53, 426);
    for (i = 0; i < 14; i++) {
        k = 140;
        for (j = 0; j < 20; j++) {
            PRINT_KEYS(1, enter);
            PRINT_KEYS(2, cancel);
            PRINT_KEYS(3, skip);
            PRINT_KEYS(4, right_move);
            PRINT_KEYS(5, left_move);
            PRINT_KEYS(6, action);
            PRINT_KEYS(7, dash);
            PRINT_KEYS(8, light);
            PRINT_KEYS(9, item);
            PRINT_KEYS(10, search_view);
            PRINT_KEYS(11, ready);
            PRINT_KEYS(12, map);
            PRINT_KEYS(13, pause);
            key_rem[j] = i;
        }
    }
    if (key_cur == 0 && key_cur2 == 1 && shPadTrigger(0, key_config.enter)) {
        key_type1();
    }
    if (key_cur == 0 && key_cur2 == 2 && shPadTrigger(0, key_config.enter)) {
        key_type2();
    }
    if (key_cur == 0 && key_cur2 == 3 && shPadTrigger(0, key_config.enter)) {
        key_type3();
    }
    if (key_cur == 0 && key_cur2 == 4 && shPadTrigger(0, key_config.enter)) {
        t.key_config_set = key_config;
    }
    if (key_cur == 0 && shPadTrigger(0, key_config.cancel)) {
        key_cur2 = 0;
    }
    switch (key_cur) {
    case 1:
        i = t.key_config_set.enter;
        if (i ^ key_check()) {
            i ^= key_check();
        }
        key_conf(&i, &t.key_config_set.cancel, 1);
        if (i) {
            t.key_config_set.enter = i;
        }
        break;
    case 2:
        i = t.key_config_set.cancel;
        if (i ^ key_check()) {
            i ^= key_check();
        }
        key_conf(&i, &t.key_config_set.enter, 1);
        if (i) {
            t.key_config_set.cancel = i;
        }
        break;
    case 3:
        t.key_config_set.skip ^= key_check();
        break;
    case 4:
        i = t.key_config_set.right_move;
        i ^= key_check();
        key_conf_check(4, &i);
        t.key_config_set.right_move = i;
        break;
    case 5:
        i = t.key_config_set.left_move;
        i ^= key_check();
        key_conf_check(5, &i);
        t.key_config_set.left_move = i;
        break;
    case 6:
        i = t.key_config_set.action;
        if (i ^ key_check()) { i ^= key_check(); }
        key_conf_check(6, &i);
        if (i) { t.key_config_set.action = i; }
        break;
    case 7:
        i = t.key_config_set.dash;
        i ^= key_check();
        key_conf_check(7, &i);
        t.key_config_set.dash = i;
        break;
    case 8:
        i = t.key_config_set.light;
        i ^= key_check();
        key_conf_check(8, &i);
        t.key_config_set.light = i;
        break;
    case 9:
        i = t.key_config_set.item;
        if (i ^ key_check()) { i ^= key_check(); }
        key_conf_check(9, &i);
        if (i) { t.key_config_set.item = i; }
        break;
    case 10:
        i = t.key_config_set.search_view;
        i ^= key_check();
        key_conf_check(10, &i);
        t.key_config_set.search_view = i;
        break;
    case 11:
        i = t.key_config_set.ready;
        if (i ^ key_check()) { i ^= key_check(); }
        key_conf_check(11, &i);
        if (i) { t.key_config_set.ready = i; }
        break;
    case 12:
        i = t.key_config_set.map;
        i ^= key_check();
        key_conf_check(12, &i);
        t.key_config_set.map = i;
        break;
    case 13:
        i = t.key_config_set.pause;
        i ^= key_check();
        key_conf_check(13, &i);
        t.key_config_set.pause = i;
        break;
    }
    if (ScreenEffectFadeCheck()) {
        if (shPadRepeat(0, 0x800) ||
            (shPadPress(0, 0x80) > 0xC0 && (t.ana_y == 0 || (t.ana_y > 15 && t.ana_y % 3 == 0)))) {
            key_cur++;
        }
        if (shPadRepeat(0, 0x400) ||
            (shPadPress(0, 0x80) < 0x40 && (t.ana_y == 0 || (t.ana_y > 15 && t.ana_y % 3 == 0)))) {
            key_cur--;
        }
        if (PAD_REP_R) {
            key_cur2++;
        }
        if (PAD_REP_L) {
            key_cur2--;
        }
        if (key_cur > 13) {
            key_cur = 0;
        }
        if (key_cur < 0) {
            key_cur = 13;
        }
        if (key_cur2 > 4) {
            key_cur2 = 0;
        }
        if (key_cur2 < 0) {
            key_cur2 = 4;
        }
        if (key_cur != 0) {
            key_cur2 = 0;
        }
    }
}

#define PAD_REP_D                                                                           \
    (shPadRepeat(0, 0x800) ||                                                               \
     (shPadPress(0, 0x80) > 0xC0 && (t.ana_y == 0 || (t.ana_y > 15 && t.ana_y % 3 == 0))))
#define PAD_REP_U                                                                           \
    (shPadRepeat(0, 0x400) ||                                                               \
     (shPadPress(0, 0x80) < 0x40 && (t.ana_y == 0 || (t.ana_y > 15 && t.ana_y % 3 == 0))))

#define XYZ(x, y) ((long)zs(x) | ((long)zs(y) << 16) | ((long)zs(0) << 32) | 0xFF00000000000000)
#define RGB_GRAY(c) ((long)(c) | ((long)(c) << 8) | ((long)(c) << 16) | 0x80000000)

/** The screen-position page. */
void position_main(void) {
    int i;
    int j;
    int rgb;
    int fade;

    if (t.fade_flag == 0) {
        fade_in();
    }
    if (shPadTrigger(0, key_config.cancel) && t.fade == 0) {
        t.fade_flag = 1;
    }
    if (t.fade_flag == 1) {
        t.option_step = fade_out(2);
    }
    if (t.fade == 0) {
        if (PAD_REP_D) {
            playing.screen_position_y += 3;
        }
        if (PAD_REP_U) {
            playing.screen_position_y -= 3;
        }
        if (playing.screen_position_x < -22) {
            playing.screen_position_x = -22;
        }
        if (playing.screen_position_x > 22) {
            playing.screen_position_x = 22;
        }
        if (playing.screen_position_y < -24) {
            playing.screen_position_y = -24;
        }
        if (playing.screen_position_y > 24) {
            playing.screen_position_y = 24;
        }
        shGs_SetDefaultDispArea();
        shGs_TrimDispArea(playing.screen_position_x / 4, playing.screen_position_y / 3);
    }
    /* blink color: dead code (the value is overwritten below), the branch survives */
    if ((int)t.timer % 40 < 20) {
        rgb = 0x80;
    } else {
        rgb = 0x40;
    }
    if (t.hoge == 0 || t.hoge == 0xFF) {
        if (t.fade_flag == 0) {
            if (shPadTrigger(0, key_config.enter) && t.fade == 0) {
                t.hoge += 32;
            }
        }
    } else {
        t.hoge += 32;
    }
    if (t.hoge == 0x100) {
        t.hoge = 0xFF;
    }
    if (t.hoge == 0x1FF) {
        t.hoge = 0;
    }
    fade = t.hoge;
    fade = fade < 0x100 ? fade : 0x1FF - fade;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0008, 0);
            *spack.pos++ = 0x30000;
            *spack.pos++ = 0x47;
            spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
            *spack.pos++ = 6;
            *spack.pos++ = RGB_GRAY(fade);
            *spack.pos++ = XYZ(i * 64 - 256, j * 64 - 256);
            *spack.pos++ = XYZ(i * 64 - 192, j * 64 - 192);
            spkCloseGiftag();
        }
    }
    for (i = 0; i < 23; i++) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0004, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
        *spack.pos++ = 6;
        if (i % 10 == 1) {
            *spack.pos++ = 0x804040FF;
        } else {
            rgb = 0xFF - fade;
            *spack.pos++ = RGB_GRAY(rgb);
        }
        *spack.pos++ = XYZ(i * 24 - 265, -270);
        *spack.pos++ = XYZ(i * 24 - 263, 270);
        spkCloseGiftag();
    }
    for (i = 0; i < 17; i++) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0004, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
        *spack.pos++ = 6;
        if (i % 6 == 2) {
            *spack.pos++ = 0x804040FF;
        } else {
            rgb = 0xFF - fade;
            *spack.pos++ = RGB_GRAY(rgb);
        }
        *spack.pos++ = XYZ(-270, i * 32 - 257);
        *spack.pos++ = XYZ(270, i * 32 - 255);
        spkCloseGiftag();
    }
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
    *spack.pos++ = 0x30000;
    *spack.pos++ = 0x47;
    spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
    *spack.pos++ = 6;
    *spack.pos++ = 0x80FFFFFF;
    *spack.pos++ = XYZ(-122, 48);
    *spack.pos++ = XYZ(122, 112);
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
    *spack.pos++ = 0x30000;
    *spack.pos++ = 0x47;
    spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
    *spack.pos++ = 6;
    *spack.pos++ = 0x80000000;
    *spack.pos++ = XYZ(-120, 50);
    *spack.pos++ = XYZ(120, 110);
    spkCloseGiftag();
    allow_u(0, -165);
    allow_d(0, 165);
    allow_l(-230, 0);
    allow_r(230, 0);
}

/** Draws the BGM volume gauge. */
void look_bgm(void) {
    int i;
    int j;
    int abe;

    if (t.cursol == 9) {
        abe = 1;
    } else {
        abe = 0;
    }
    for (i = 0; i < playing.bgm_volume; i++) {
        for (j = 0; j < 2; j++) {
            spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0004, 0);
            *spack.pos++ = 0x30000;
            *spack.pos++ = 0x47;
            spkCloseOpenDGiftag(0x8400000000008000, 0x44444410);
            if (abe) {
                *spack.pos++ = 4;
            } else {
                *spack.pos++ = 0x44;
            }
            if (j == 0) {
                *spack.pos++ = 0x40B0B0B0;
            } else {
                *spack.pos++ = 0x40404040;
            }
            *spack.pos++ = XYZ(i * 11 + 24, 92);
            *spack.pos++ = XYZ(i * 11 + 22, 94);
            if (j == 0) {
                *spack.pos++ = XYZ(i * 11 + 15, 92);
            } else {
                *spack.pos++ = XYZ(i * 11 + 24, 115);
            }
            if (j == 0) {
                *spack.pos++ = XYZ(i * 11 + 17, 94);
            } else {
                *spack.pos++ = XYZ(i * 11 + 22, 113);
            }
            *spack.pos++ = XYZ(i * 11 + 15, 115);
            *spack.pos++ = XYZ(i * 11 + 17, 113);
            spkCloseGiftag();
        }
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
        if (abe) {
            *spack.pos++ = 4;
        } else {
            *spack.pos++ = 0x44;
        }
        *spack.pos++ = 0x40808080;
        *spack.pos++ = XYZ(i * 11 + 17, 94);
        *spack.pos++ = XYZ(i * 11 + 22, 94);
        *spack.pos++ = XYZ(i * 11 + 17, 113);
        *spack.pos++ = XYZ(i * 11 + 22, 113);
        spkCloseGiftag();
    }
    for (i = playing.bgm_volume; i < 15; i++) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
        if (abe) {
            *spack.pos++ = 4;
        } else {
            *spack.pos++ = 0x44;
        }
        *spack.pos++ = 0x40505050;
        *spack.pos++ = XYZ(i * 11 + 17, 94);
        *spack.pos++ = XYZ(i * 11 + 22, 94);
        *spack.pos++ = XYZ(i * 11 + 17, 113);
        *spack.pos++ = XYZ(i * 11 + 22, 113);
        spkCloseGiftag();
    }
}

/** Draws the sound-effect volume gauge. */
void look_se(void) {
    int i;
    int j;
    int abe;

    if (t.cursol == 10) {
        abe = 1;
    } else {
        abe = 0;
    }
    for (i = 0; i < playing.se_volume; i++) {
        for (j = 0; j < 2; j++) {
            spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0004, 0);
            *spack.pos++ = 0x30000;
            *spack.pos++ = 0x47;
            spkCloseOpenDGiftag(0x8400000000008000, 0x44444410);
            if (abe) {
                *spack.pos++ = 4;
            } else {
                *spack.pos++ = 0x44;
            }
            if (j == 0) {
                *spack.pos++ = 0x40B0B0B0;
            } else {
                *spack.pos++ = 0x40404040;
            }
            *spack.pos++ = XYZ(i * 11 + 24, 119);
            *spack.pos++ = XYZ(i * 11 + 22, 121);
            if (j == 0) {
                *spack.pos++ = XYZ(i * 11 + 15, 119);
            } else {
                *spack.pos++ = XYZ(i * 11 + 24, 142);
            }
            if (j == 0) {
                *spack.pos++ = XYZ(i * 11 + 17, 121);
            } else {
                *spack.pos++ = XYZ(i * 11 + 22, 140);
            }
            *spack.pos++ = XYZ(i * 11 + 15, 142);
            *spack.pos++ = XYZ(i * 11 + 17, 140);
            spkCloseGiftag();
        }
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
        if (abe) {
            *spack.pos++ = 4;
        } else {
            *spack.pos++ = 0x44;
        }
        *spack.pos++ = 0x40808080;
        *spack.pos++ = XYZ(i * 11 + 17, 121);
        *spack.pos++ = XYZ(i * 11 + 22, 121);
        *spack.pos++ = XYZ(i * 11 + 17, 140);
        *spack.pos++ = XYZ(i * 11 + 22, 140);
        spkCloseGiftag();
    }
    for (i = playing.se_volume; i < 15; i++) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
        if (abe) {
            *spack.pos++ = 4;
        } else {
            *spack.pos++ = 0x44;
        }
        *spack.pos++ = 0x40505050;
        *spack.pos++ = XYZ(i * 11 + 17, 121);
        *spack.pos++ = XYZ(i * 11 + 22, 121);
        *spack.pos++ = XYZ(i * 11 + 17, 140);
        *spack.pos++ = XYZ(i * 11 + 22, 140);
        spkCloseGiftag();
    }
}

/** Moves f2 toward f3 by a proportional step (distance / f0) plus a fixed step f1.
 * @param f0 proportional divisor
 * @param f1 fixed step
 * @param f2 current value
 * @param f3 target
 * @return the new value (as int) */
int option_near(float f0, float f1, float f2, float f3) {
    if (f2 < f3) {
        f2 += (f3 - f2) / f0;
        f2 += f1;
        if (f2 >= f3) {
            f2 = f3;
        }
    } else {
        f2 -= (f2 - f3) / f0;
        f2 -= f1;
        if (f2 <= f3) {
            f2 = f3;
        }
    }
    return f2;
}

/** Fades the screen in; clears t.fade when done.
 * @return 1 when done */
int fade_in(void) {
    ScreenEffectFadeStart(4, 1.0f);
    if (ScreenEffectFadeCheck()) {
        t.fade = 0;
        return 1;
    }
    return 0;
}

/** Fades the screen out; when done, sets t.fade to 0xFF and ends the fade.
 * @param mode unused here
 * @return the options step (t.option_step) */
int fade_out(int mode) {
    ScreenEffectFadeStart(1, 1.0f);
    if (ScreenEffectFadeCheck()) {
        t.fade = 0xFF;
        t.fade_flag = 0;
        t.hoge = 0;
        return mode;
    }
    return t.option_step;
}

/** Draws the options board. */
void look_board(void) {
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0006, 0);
    *spack.pos++ = 0x30000;
    *spack.pos++ = 0x47;
    spkCloseOpenDGiftag(0x8400000000008000, 0x44444410);
    *spack.pos++ = 0x44;
    *spack.pos++ = 0x20808080;
    *spack.pos++ = XYZ(-210, -165);
    *spack.pos++ = XYZ(210, -165);
    *spack.pos++ = XYZ(-227, -145);
    *spack.pos++ = XYZ(227, -145);
    *spack.pos++ = XYZ(-227, 155);
    *spack.pos++ = XYZ(227, 155);
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0006, 0);
    *spack.pos++ = 0x30000;
    *spack.pos++ = 0x47;
    spkCloseOpenDGiftag(0x9400000000008000, 0x444444410);
    *spack.pos++ = 0x42;
    *spack.pos++ = 0x30D06060;
    *spack.pos++ = XYZ(-210, -165);
    *spack.pos++ = XYZ(-227, -145);
    *spack.pos++ = XYZ(-227, 155);
    *spack.pos++ = XYZ(227, 155);
    *spack.pos++ = XYZ(227, -145);
    *spack.pos++ = XYZ(210, -165);
    *spack.pos++ = XYZ(-210, -165);
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0006, 0);
    *spack.pos++ = 0x30000;
    *spack.pos++ = 0x47;
    spkCloseOpenDGiftag(0x8400000000008000, 0x44444410);
    *spack.pos++ = 0x44;
    *spack.pos++ = 0x20808080;
    *spack.pos++ = XYZ(-227, 161);
    *spack.pos++ = XYZ(227, 161);
    *spack.pos++ = XYZ(-227, 185);
    *spack.pos++ = XYZ(227, 185);
    *spack.pos++ = XYZ(-210, 205);
    *spack.pos++ = XYZ(210, 205);
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0006, 0);
    *spack.pos++ = 0x30000;
    *spack.pos++ = 0x47;
    spkCloseOpenDGiftag(0x9400000000008000, 0x444444410);
    *spack.pos++ = 0x42;
    *spack.pos++ = 0x30D06060;
    *spack.pos++ = XYZ(-227, 161);
    *spack.pos++ = XYZ(-227, 185);
    *spack.pos++ = XYZ(-210, 205);
    *spack.pos++ = XYZ(210, 205);
    *spack.pos++ = XYZ(227, 185);
    *spack.pos++ = XYZ(227, 161);
    *spack.pos++ = XYZ(-227, 161);
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0006, 0);
    *spack.pos++ = 0x30000;
    *spack.pos++ = 0x47;
    spkCloseOpenDGiftag(0xB400000000008000, 0x44444444410);
    *spack.pos++ = 0x42;
    *spack.pos++ = 0x30D06060;
    *spack.pos++ = XYZ(-212, -170);
    *spack.pos++ = XYZ(-232, -147);
    *spack.pos++ = XYZ(-232, 187);
    *spack.pos++ = XYZ(-212, 210);
    *spack.pos++ = XYZ(212, 210);
    *spack.pos++ = XYZ(232, 187);
    *spack.pos++ = XYZ(232, -147);
    *spack.pos++ = XYZ(212, -170);
    *spack.pos++ = XYZ(-212, -170);
    spkCloseGiftag();
}

/*
 * Matching: stand-ins for dead-stripped code before look_cur: double code (a2 temporaries) and
 * float constants (they fix the option_near() argument order).
 */
STRIPPED_DOUBLE_CODE()
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f + 35.0f + 37.0f + 39.0f + 41.0f + 43.0f + 45.0f + 47.0f + 49.0f + 51.0f + 53.0f + 55.0f + 57.0f + 59.0f + 61.0f; }

/** Moves (with key repeat) and draws the cursor of the main page. */
void look_cur(void) {
    int i;

    if (t.fade == 0) {
        if (PAD_REP_D) {
            t.cursol++;
        }
        if (PAD_REP_U) {
            t.cursol--;
        }
    }
    if (t.extra_mode == 0) {
        if (t.cursol < 0) {
            t.cursol = 10;
        }
        if (t.cursol > 10) {
            t.cursol = 0;
        }
        if (t.fade == 0 && (shPadRepeat(0, 0x800) || shPadRepeat(0, 0x400) ||
                            (shPadPress(0, 0x80) > 0xC0 && (t.ana_y == 0 || (t.ana_y > 15 && t.ana_y % 3 == 0))) ||
                            (shPadPress(0, 0x80) < 0x40 && (t.ana_y == 0 || (t.ana_y > 15 && t.ana_y % 3 == 0))))) {
            SeCall(10000, 1.0f, 0);
        }
    } else {
        if (t.cursol < 0) {
            t.cursol = 6;
        }
        if (t.cursol > 6) {
            t.cursol = 0;
        }
        if (t.fade == 0 && (shPadRepeat(0, 0x800) || shPadRepeat(0, 0x400) ||
                            (shPadPress(0, 0x80) > 0xC0 && (t.ana_y == 0 || (t.ana_y > 15 && t.ana_y % 3 == 0))) ||
                            (shPadPress(0, 0x80) < 0x40 && (t.ana_y == 0 || (t.ana_y > 15 && t.ana_y % 3 == 0))))) {
            SeCall(10000, 1.0f, 0);
        }
    }
    t.cursol_pos = option_near(2.0f, 1.0f, t.cursol_pos, t.cursol * 27 - 140);
    for (i = 0; i < 15; i++) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0005, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
        *spack.pos++ = 0x46;
        *spack.pos++ = (long)(15 - i) << 24;
        *spack.pos++ = XYZ(-226, (int)t.cursol_pos + i);
        *spack.pos++ = XYZ(226, (int)t.cursol_pos + i + 1);
        spkCloseGiftag();
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0005, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
        *spack.pos++ = 0x46;
        *spack.pos++ = (long)(15 - i) << 24;
        *spack.pos++ = XYZ(-226, (int)t.cursol_pos - i);
        *spack.pos++ = XYZ(226, (int)t.cursol_pos - i - 1);
        spkCloseGiftag();
    }
}

/** Draws the blinking up arrow, highlighted while up is held.
 * @param x x position
 * @param y y position */
void allow_u(int x, int y) {
    int rgb;

    if ((int)t.timer % 40 < 20) {
        rgb = (int)t.timer % 40 * 3;
    } else {
        rgb = 120 - (int)t.timer % 40 * 3;
    }
    if (shPadPress(0, 0x400) || shPadPress(0, 0x80) < 0x40) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        *spack.pos++ = 0x43;
        *spack.pos++ = ((long)(rgb + 100) << 24) | 0xFFA0B0;
        *spack.pos++ = XYZ(x, y - 2);
        *spack.pos++ = XYZ(x - 22, y + 13);
        *spack.pos++ = XYZ(x + 22, y + 13);
        spkCloseGiftag();
    } else {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        *spack.pos++ = 0x43;
        *spack.pos++ = ((long)(rgb + 50) << 24) | 0xFFA0B0;
        *spack.pos++ = XYZ(x, y);
        *spack.pos++ = XYZ(x - 17, y + 13);
        *spack.pos++ = XYZ(x + 17, y + 13);
        spkCloseGiftag();
    }
}

/** Draws the blinking down arrow, highlighted while down is held.
 * @param x x position
 * @param y y position */
void allow_d(int x, int y) {
    int rgb;

    if ((int)t.timer % 40 < 20) {
        rgb = (int)t.timer % 40 * 3;
    } else {
        rgb = 120 - (int)t.timer % 40 * 3;
    }
    if (shPadPress(0, 0x800) || shPadPress(0, 0x80) > 0xC0) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        *spack.pos++ = 0x43;
        *spack.pos++ = ((long)(rgb + 100) << 24) | 0xFFA0B0;
        *spack.pos++ = XYZ(x, y + 2);
        *spack.pos++ = XYZ(x - 22, y - 13);
        *spack.pos++ = XYZ(x + 22, y - 13);
        spkCloseGiftag();
    } else {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        *spack.pos++ = 0x43;
        *spack.pos++ = ((long)(rgb + 50) << 24) | 0xFFA0B0;
        *spack.pos++ = XYZ(x, y);
        *spack.pos++ = XYZ(x - 17, y - 13);
        *spack.pos++ = XYZ(x + 17, y - 13);
        spkCloseGiftag();
    }
}

/** Draws the blinking left arrow, highlighted while left is held.
 * @param x x position
 * @param y y position */
void allow_l(int x, int y) {
    int rgb;

    if ((int)t.timer % 40 < 20) {
        rgb = (int)t.timer % 40 * 3;
    } else {
        rgb = 120 - (int)t.timer % 40 * 3;
    }
    if (shPadPress(0, 0x200) || shPadPress(0, 0x40) < 0x40) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        *spack.pos++ = 0x43;
        *spack.pos++ = ((long)(rgb + 100) << 24) | 0xFFA0B0;
        *spack.pos++ = XYZ(x - 2, y);
        *spack.pos++ = XYZ(x + 12, y - 23);
        *spack.pos++ = XYZ(x + 12, y + 23);
        spkCloseGiftag();
    } else {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        *spack.pos++ = 0x43;
        *spack.pos++ = ((long)(rgb + 50) << 24) | 0xFFA0B0;
        *spack.pos++ = XYZ(x, y);
        *spack.pos++ = XYZ(x + 12, y - 18);
        *spack.pos++ = XYZ(x + 12, y + 18);
        spkCloseGiftag();
    }
}

/** Draws the blinking right arrow, highlighted while right is held.
 * @param x x position
 * @param y y position */
void allow_r(int x, int y) {
    int rgb;

    if ((int)t.timer % 40 < 20) {
        rgb = (int)t.timer % 40 * 3;
    } else {
        rgb = 120 - (int)t.timer % 40 * 3;
    }
    if (shPadPress(0, 0x100) || shPadPress(0, 0x40) > 0xC0) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        *spack.pos++ = 0x43;
        *spack.pos++ = ((long)(rgb + 100) << 24) | 0xFFA0B0;
        *spack.pos++ = XYZ(x + 2, y);
        *spack.pos++ = XYZ(x - 12, y - 23);
        *spack.pos++ = XYZ(x - 12, y + 23);
        spkCloseGiftag();
    } else {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0003, 0);
        *spack.pos++ = 0x30000;
        *spack.pos++ = 0x47;
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        *spack.pos++ = 0x43;
        *spack.pos++ = ((long)(rgb + 50) << 24) | 0xFFA0B0;
        *spack.pos++ = XYZ(x, y);
        *spack.pos++ = XYZ(x - 12, y - 18);
        *spack.pos++ = XYZ(x - 12, y + 18);
        spkCloseGiftag();
    }
}

/** Starts loading the button pictures (botan.tex) into the gp data buffer. */
void KeyConfigPictureLoad(void) {
    FcRead(data_pic_etc_botan_tex, get_gp_data_buf_addr());
}

/** Uploads the button pictures and sets up the picture used to draw them. */
void KeyConfigPitureStart(void) {
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

/** Draws one button picture (24x24) from the sheet.
 * @param key_kind which button (0-11)
 * @param xx x position
 * @param yy y position */
void key_draw(int key_kind, int xx, int yy) {
    unsigned char key_pos[12][2] = {
        { 1, 3 }, { 67, 3 }, { 131, 3 }, { 194, 3 },
        { 3, 70 }, { 68, 70 }, { 131, 70 }, { 197, 70 },
        { 3, 133 }, { 68, 133 }, { 129, 133 }, { 195, 133 },
    };

    i_pic.x0 = (xx - 256) * 16;
    i_pic.y0 = (yy - 256) * 16;
    i_pic.x1 = (xx + 24 - 256) * 16;
    i_pic.y1 = (yy + 24 - 256) * 16;
    i_pic.status |= 2;
    i_pic.us0 = key_pos[key_kind][0] * 16;
    i_pic.vt0 = key_pos[key_kind][1] * 16;
    i_pic.us1 = key_pos[key_kind][0] * 16 + 60 * 16;
    i_pic.vt1 = key_pos[key_kind][1] * 16 + 60 * 16;
    i_pic.status |= 4;
    i_pic.r = 0x80;
    i_pic.g = 0x80;
    i_pic.b = 0x80;
    i_pic.status |= 0x10;
    i_pic.otp = 1;
    PictureDraw(&i_pic);
}

#define MSG(n, x, y) fontPrintStrNum(msg_buffer, n, x, y)

/** Prints the texts of the current options page. */
void option_font(void) {
    char buf[4];
    char buf2[4];
    float f;

    f = 255.0f;
    fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0xFF);
    if (t.option_step == 2) {
        if (t.extra_mode == 0) {
            MSG(0x48, 180, 52);
            fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0x60);
            MSG(0x02, 180, 100);
            MSG(0x19, 190, 127);
            MSG(0x06, 195, 154);
            MSG(0x54, 119, 181);
            MSG(0x1C, 152, 208);
            MSG(0x20, 145, 235);
            MSG(0x4C, 151, 262);
            MSG(0x4A, 161, 289);
            MSG(0x22, 187, 316);
            MSG(0x28, 120, 343);
            MSG(0x2A, 142, 370);
            fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0xFF);
            if (t.cursol == 0) {
                MSG(0x02, 178, 98);
            }
            if (t.cursol == 1) {
                MSG(0x19, 188, 125);
            }
            if (t.cursol == 2) {
                MSG(0x06, 193, 152);
            }
            if (t.cursol == 3) {
                MSG(0x54, 117, 179);
            }
            if (t.cursol == 4) {
                MSG(0x1C, 150, 206);
            }
            if (t.cursol == 5) {
                MSG(0x20, 143, 233);
            }
            if (t.cursol == 6) {
                MSG(0x4C, 149, 260);
            }
            if (t.cursol == 7) {
                MSG(0x4A, 159, 287);
            }
            if (t.cursol == 8) {
                MSG(0x22, 185, 314);
            }
            if (t.cursol == 9) {
                MSG(0x28, 118, 341);
            }
            if (t.cursol == 10) {
                MSG(0x2A, 140, 368);
            }
            fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0x60);
            if (playing.control_type == 0) {
                MSG(0x55, 270, 181);
            } else if (playing.control_type == 1) {
                MSG(0x56, 270, 181);
            }
            if (playing.vibration == 0) {
                MSG(0x45, 270, 208);
            } else if (playing.vibration == 1) {
                MSG(0x1E, 270, 208);
            } else if (playing.vibration == 2) {
                MSG(0x46, 270, 208);
            } else if (playing.vibration == 3) {
                MSG(0x1F, 270, 208);
            }
            if (playing.auto_load) {
                MSG(0x44, 270, 235);
            } else {
                MSG(0x45, 270, 235);
            }
            if (playing.language == 0) {
                MSG(0x4E, 270, 262);
            } else if (playing.language == 1) {
                MSG(0x4F, 270, 262);
            } else if (playing.language == 2) {
                MSG(0x51, 270, 262);
            } else if (playing.language == 3) {
                MSG(0x50, 270, 262);
            } else if (playing.language == 4) {
                MSG(0x52, 270, 262);
            } else if (playing.language == 5) {
                MSG(0x53, 270, 262);
            }
            if (playing.subtitles) {
                MSG(0x44, 270, 289);
            } else {
                MSG(0x45, 270, 289);
            }
            if (playing.sound == 0) {
                MSG(0x25, 270, 316);
            } else if (playing.sound == 1) {
                MSG(0x26, 270, 316);
            } else {
                MSG(0x27, 270, 316);
            }
            fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0xFF);
            if (t.cursol == 3) {
                if (playing.control_type == 0) {
                    MSG(0x55, 268, 179);
                } else if (playing.control_type == 1) {
                    MSG(0x56, 268, 179);
                }
            }
            if (t.cursol == 4) {
                if (playing.vibration == 0) {
                    MSG(0x45, 268, 206);
                } else if (playing.vibration == 1) {
                    MSG(0x1E, 268, 206);
                } else if (playing.vibration == 2) {
                    MSG(0x46, 268, 206);
                } else if (playing.vibration == 3) {
                    MSG(0x1F, 268, 206);
                }
            }
            if (t.cursol == 5) {
                if (playing.auto_load) {
                    MSG(0x44, 268, 233);
                } else {
                    MSG(0x45, 268, 233);
                }
            }
            if (t.cursol == 6) {
                if (playing.language == 0) {
                    MSG(0x4E, 268, 260);
                } else if (playing.language == 1) {
                    MSG(0x4F, 268, 260);
                } else if (playing.language == 2) {
                    MSG(0x51, 268, 260);
                } else if (playing.language == 3) {
                    MSG(0x50, 268, 260);
                } else if (playing.language == 4) {
                    MSG(0x52, 268, 260);
                } else if (playing.language == 5) {
                    MSG(0x53, 268, 260);
                }
            }
            if (t.cursol == 7) {
                if (playing.subtitles) {
                    MSG(0x44, 268, 287);
                } else {
                    MSG(0x45, 268, 287);
                }
            }
            if (t.cursol == 8) {
                if (playing.sound == 0) {
                    MSG(0x25, 268, 314);
                } else if (playing.sound == 1) {
                    MSG(0x26, 268, 314);
                } else {
                    MSG(0x27, 268, 314);
                }
            }
            if (t.cursol == 0) {
                MSG(0x03, 70, 422);
            }
            if (t.cursol == 1) {
                MSG(0x1A, 70, 422);
            }
            if (t.cursol == 2) {
                MSG(0x07, 70, 422);
            }
            if (t.cursol == 3) {
                MSG(0x57, 70, 422);
            }
            if (t.cursol == 4) {
                MSG(0x1D, 70, 422);
            }
            if (t.cursol == 5) {
                MSG(0x21, 70, 422);
            }
            if (t.cursol == 6) {
                MSG(0x4D, 70, 422);
            }
            if (t.cursol == 7) {
                MSG(0x4B, 70, 422);
            }
            if (t.cursol == 8) {
                MSG(0x23, 70, 422);
            }
            if (t.cursol == 9) {
                MSG(0x29, 70, 422);
            }
            if (t.cursol == 10) {
                MSG(0x2B, 70, 422);
            }
        } else {
            MSG(0x49, 120, 52);
            fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0x60);
            MSG(0x2C, 91, 100);
            MSG(0x30, 130, 127);
            MSG(0x35, 118, 154);
            MSG(0x37, 123, 181);
            MSG(0x39, 68, 208);
            MSG(0x3D, 137, 235);
            MSG(0x41, 117, 262);
            fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0xFF);
            if (t.cursol == 0) {
                MSG(0x2C, 89, 98);
            }
            if (t.cursol == 1) {
                MSG(0x30, 128, 125);
            }
            if (t.cursol == 2) {
                MSG(0x35, 116, 152);
            }
            if (t.cursol == 3) {
                MSG(0x37, 121, 179);
            }
            if (t.cursol == 4) {
                MSG(0x39, 66, 206);
            }
            if (t.cursol == 5) {
                MSG(0x3D, 135, 233);
            }
            if (t.cursol == 6) {
                MSG(0x41, 115, 260);
            }
            fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0x60);
            if (playing.weapon_control) {
                MSG(0x2F, 270, 100);
            } else {
                MSG(0x2E, 270, 100);
            }
            if (playing.blood_color == 0) {
                MSG(0x46, 270, 127);
            } else if (playing.blood_color == 1) {
                MSG(0x32, 270, 127);
            } else if (playing.blood_color == 2) {
                MSG(0x33, 270, 127);
            } else if (playing.blood_color == 3) {
                MSG(0x34, 270, 127);
            }
            if (playing.view_control) {
                MSG(0x47, 270, 154);
            } else {
                MSG(0x46, 270, 154);
            }
            if (playing.retreat_turn) {
                MSG(0x47, 270, 181);
            } else {
                MSG(0x46, 270, 181);
            }
            if (playing.walk_run_control) {
                MSG(0x47, 270, 208);
            } else {
                MSG(0x46, 270, 208);
            }
            if (playing.view_mode) {
                MSG(0x40, 270, 235);
            } else {
                MSG(0x3F, 270, 235);
            }
            sprintf(buf, "%d", playing.bullet_adjust);
            fontSetMes(0, dicSetStr(buf));
            MSG(0x43, 270, 262);
            fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0xFF);
            if (t.cursol == 0) {
                if (playing.weapon_control) {
                    MSG(0x2F, 268, 98);
                } else {
                    MSG(0x2E, 268, 98);
                }
            }
            if (t.cursol == 1) {
                if (playing.blood_color == 0) {
                    MSG(0x46, 268, 125);
                } else if (playing.blood_color == 1) {
                    MSG(0x32, 268, 125);
                } else if (playing.blood_color == 2) {
                    MSG(0x33, 268, 125);
                } else if (playing.blood_color == 3) {
                    MSG(0x34, 268, 125);
                }
            }
            if (t.cursol == 2) {
                if (playing.view_control) {
                    MSG(0x47, 268, 152);
                } else {
                    MSG(0x46, 268, 152);
                }
            }
            if (t.cursol == 3) {
                if (playing.retreat_turn) {
                    MSG(0x47, 268, 179);
                } else {
                    MSG(0x46, 268, 179);
                }
            }
            if (t.cursol == 4) {
                if (playing.walk_run_control) {
                    MSG(0x47, 268, 206);
                } else {
                    MSG(0x46, 268, 206);
                }
            }
            if (t.cursol == 5) {
                if (playing.view_mode) {
                    MSG(0x40, 268, 233);
                } else {
                    MSG(0x3F, 268, 233);
                }
            }
            if (t.cursol == 6) {
                sprintf(buf, "%d", playing.bullet_adjust);
                fontSetMes(0, dicSetStr(buf));
                MSG(0x43, 268, 260);
            }
            if (t.cursol == 0) {
                MSG(0x2D, 80, 422);
            }
            if (t.cursol == 1) {
                MSG(0x31, 80, 422);
            }
            if (t.cursol == 2) {
                MSG(0x36, 80, 422);
            }
            if (t.cursol == 3) {
                MSG(0x38, 80, 422);
            }
            if (t.cursol == 4) {
                MSG(0x3A, 80, 422);
            }
            if (t.cursol == 5) {
                MSG(0x3E, 80, 422);
            }
            if (t.cursol == 6) {
                MSG(0x42, 80, 422);
            }
        }
    } else {
        if (t.option_step == 3) {
            MSG(0x04, 25, 100);
            sprintf(buf, "%2d", playing.brightness_level);
            fontSetMes(0, dicSetStr(buf));
            MSG(0x05, 160, 420);
        } else {
            if (t.option_step == 5) {
                sprintf(buf, "%3d", playing.screen_position_x / 2);
                sprintf(buf2, "%3d", playing.screen_position_y / 3);
                fontSetMes(0, dicSetStr(buf));
                fontSetMes(1, dicSetStr(buf2));
                MSG(0x1B, 155, 320);
            }
        }
    }
}

/** Draws an 8x8 grid of black quads over the screen. */
void look_scr(void) {
    int i;
    int j;

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            spkOpenDGiftag(0x1000000000008000, 0xE, 0x80000005, 0);
            *spack.pos++ = 0x30000;
            *spack.pos++ = 0x47;
            spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
            *spack.pos++ = 6;
            *spack.pos++ = 0x80000000;
            *spack.pos++ = (long)zs(i * 64 - 256) | ((long)zs(j * 64 - 256) << 16) | ((long)zs(0) << 32) |
                           0xFF00000000000000;
            *spack.pos++ = (long)zs(i * 64 - 192) | ((long)zs(j * 64 - 192) << 16) | ((long)zs(0) << 32) |
                           0xFF00000000000000;
            spkCloseGiftag();
        }
    }
}

/** Converts a screen coordinate to GS 12.4 fixed point with the 2048 offset.
 * @param hoge the coordinate */
int zs(int hoge) {
    return (hoge + 0x800) * 16;
}
