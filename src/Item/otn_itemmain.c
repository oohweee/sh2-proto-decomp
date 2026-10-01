/*
 * otn_itemmain.c: the item menu (inventory ring, commands, examine view).
 *
 * set_position is a fake match: the order in which the constant float arguments of its move_near
 * calls are materialized (lui/mtc1 order) depends on leftovers in the compiler's arena from
 * item_main_setup (docs/toolchain.md, "Root cause"), and fitted spellings reproduce it (see there).
 */
#include "sh2.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "sdk/libgraph.h"

/* Matching: fitted stand-in for double code (docs/stand-ins.md): later functions use a2 for temporaries. */
STRIPPED_DOUBLE_CODE()

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

#define SH2SYS_STEP3_NEXT() \
    Sh2sys.step[3]++;       \
    Sh2sys.step[4] = 0;     \
    Sh2sys.step[5] = 0;     \
    Sh2sys.step[6] = 0;     \
    Sh2sys.step[7] = 0

#define PK_ADD(v) (*spack.pos++ = (v))

/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)

#include "math_const.h"

/*
 * Random numbers for look_hp's noise: rand() % n, and rand() % n + base. Names invented.
 * Matching: the original's DWARF has look_hp's j, j0 and j1 in v0 (no register), which MWCC
 * gives a local that only copies an inline function's result; written out, they get s1-s3.
 * The code is the same either way.
 */
inline int irand(int n) {
    return rand() % n;
}

inline int rrand(int n, int base) {
    return rand() % n + base;
}

int lcolor = 45;
int scolor = 60;
struct ItemMainWork t;
struct ItemSelect item_select[75];
int examine_rgb;

/** Item menu main, one step per call: set-up and loading, the menu itself, the examine view and
 * the exit. A button combination (0x4000 + 4) at the start goes to the save screen instead.
 * @return non-zero once the menu has closed */
int itemmain(void) {
    if (Sh2sys.step[3] == 0) {
        t.main_step = 0;
        SH2SYS_STEP3_NEXT();
    }
    switch (t.main_step) {
    case 0:
        if (shPadPress(0, 0x4000) && shPadPress(0, 4)) {
            mcStepInit();
            SetSavePointName(0);
            SH2SYS_STEP2(9);
            return 0;
        }
        item_main_setup();
        TgsItemPictureLoad();
        DataLoadMessage(1);
        DataLoadMessage(0);
        t.main_step++;
        break;
    case 1:
        if (fsSync(1, -1) >= 0) {
            ScreenEffectFadeStart(3, 0.0f);
            ScreenEffectFadeStart(4, 0.5f);
            t.main_step++;
        }
        break;
    case 2:
        TgsItemPitureStart();
        fontClear();
        itemmainmain();
        break;
    case 3:
        examine_main();
        break;
    case 4:
        fontClear();
        item.last_cursor = t.item_kind;
        t.main_step = 0;
        SH2SYS_STEP2(t.fade_step0);
        SH2SYS_STEP3(t.fade_step1);
        return 1;
    }
    return 0;
}

/** The item menu proper, one frame: fades, cursor and command handling, then draws the ring,
 * the displays and the texts. */
void itemmainmain(void) {
    int i;
    int fog;

    item_fade_in();
    item_fade_out();
    if (t.fade_flag == 2) {
        item_examine_fade_out();
    }
    if (shPadPress(0, 0x40) > 192) {
        if (t.analog[2] <= 1) {
            t.analog[2]++;
        }
    } else {
        t.analog[2] = 0;
    }
    if (shPadPress(0, 0x40) <= 63) {
        if (t.analog[1] <= 1) {
            t.analog[1]++;
        }
    } else {
        t.analog[1] = 0;
    }
    if (shPadPress(0, 0x80) > 192) {
        if (t.analog[0] <= 1) {
            t.analog[0]++;
        }
    } else {
        t.analog[0] = 0;
    }
    if (shPadPress(0, 0x80) <= 63) {
        if (t.analog[3] <= 1) {
            t.analog[3]++;
        }
    } else {
        t.analog[3] = 0;
    }
    t.shelf = GAME_FLAG(416);
    item_turn();
    for (i = 0; i < t.item_count; i++) {
        if (item_select[i].count) {
            item_position(&item_select[i].pos[0], &item_select[i].pos[1], item_select[i].count, &item_select[i].rot);
        }
    }
    if (t.prs_btn2) {
        t.turnf = 0.0f;
    } else {
        t.turnf += 0.02f;
    }
    for (i = 0; i < t.item_count; i++) {
        if (item_select[i].count) {
            fog = (-item_select[i].pos[1] - 300.0f) / 5.5;
            if (fog < 0) {
                fog = 0;
            }
            TgsItemPitureDraw(item_select[i].kind, 2.5 * item_select[i].pos[0], 0, fog, 5,
                              (0.3f + -item_select[i].pos[1] / 1666.0f) * item_select[i].item_scale);
            if (item_select[i].del == 0) {
                if (item_select[i].count == 5 && item_select[i].item_scale < 1.0f) {
                    item_select[i].item_scale += 0.1f;
                } else if (item_select[i].count != 5 && item_select[i].item_scale > 0.8f) {
                    item_select[i].item_scale -= 0.1f;
                }
                if (item_select[i].count == 5 && item_select[i].item_scale > 1.0f) {
                    item_select[i].item_scale = 1.0f;
                }
                if (item_select[i].item_scale < 0.8f) {
                    item_select[i].item_scale = 0.8f;
                }
            } else {
                item_select[i].item_scale -= 0.15f;
            }
            if (item_select[i].item_scale < 0.0f) {
                item_select[i].item_scale = 0.0f;
            }
            if (item_select[i].count == 5) {
                t.item_kind = item_select[i].kind;
            }
            if (item_select[i].del) {
                item_select[i].item_scale -= 0.15f;
                if (item_select[i].item_scale < 0.0f) {
                    item_select[i].item_scale = 0.0f;
                    item_select[i].kind = 0;
                    item_select[i].del = 0;
                }
            }
        }
    }
    if (item.equip == 4) {
        t.weapon_scale[0] += 0.4f;
    } else if (item.equip == 11) {
        t.weapon_scale[1] += 0.4f;
    } else if (item.equip == 6) {
        t.weapon_scale[2] += 0.4f;
    } else if (item.equip == 8) {
        t.weapon_scale[3] += 0.4f;
    } else if (item.equip == 10) {
        t.weapon_scale[4] += 0.4f;
    } else if (item.equip == 12) {
        t.weapon_scale[5] += 0.4f;
    } else if (item.equip == 13) {
        t.weapon_scale[6] += 0.4f;
    } else if (item.equip == 14) {
        t.weapon_scale[7] += 0.4f;
    }
    for (i = 0; i < 8; i++) {
        t.weapon_scale[i] -= 0.2f;
        if (t.weapon_scale[i] > 0.9f) {
            t.weapon_scale[i] = 0.9f;
        }
        if (t.weapon_scale[i] < 0.0f) {
            t.weapon_scale[i] = 0.0f;
        }
    }
    TgsItemPitureDraw(4, 0, -2144, 0x80, 1, t.weapon_scale[0]);
    TgsItemPitureDraw(11, 0, -2144, 0x80, 1, t.weapon_scale[1]);
    TgsItemPitureDraw(6, 0, -2144, 0x80, 1, t.weapon_scale[2]);
    TgsItemPitureDraw(8, 0, -2144, 0x80, 1, t.weapon_scale[3]);
    TgsItemPitureDraw(10, 0, -2144, 0x80, 1, t.weapon_scale[4]);
    TgsItemPitureDraw(12, 0, -2144, 0x80, 1, t.weapon_scale[5]);
    TgsItemPitureDraw(13, 0, -2144, 0x80, 1, t.weapon_scale[6]);
    TgsItemPitureDraw(14, 0, -2144, 0x80, 1, t.weapon_scale[7]);
    look_hp();
    if (t.fade_flag == 0) {
        set_position(t.step);
    }
    if (t.box[0][0] == t.boxblur[1][0][0] && t.box[0][1] == t.boxblur[1][0][1]) {
        t.prs_btn = 0;
    } else {
        t.prs_btn = 1;
    }
    if (t.sprite_time >= 2.0f) {
        t.sprite_time = 0.0f;
    }
    if (t.prs_btn) {
        t.sprite_time = 0.0f;
    }
    t.sprite_time += 0.04f;
    sprite();
    if (Sh2sys.step[2] == 6) {
        font_print();
    }
    if (t.fade_flag == 0) {
        cur_step();
    }
    look_combine();
    del_check();
    if ((t.command_abe == 0 && t.step == 1) || (t.command_abe == 0 && t.step == 12)) {
        t.command_cur = t.command_light = t.command_volume = 0;
    }
    look_command(t.gosa);
    if (t.step == 6 || t.step == 7) {
        t.command_abe += 6;
    } else {
        t.command_abe -= 6;
    }
    if (t.command_abe > 40) {
        t.command_abe = 40;
    }
    if (t.command_abe < 0) {
        t.command_abe = 0;
    }
    if (t.command_volume == 1) {
        t.command_abe = 0;
    }
    if ((int)t.allay_abe) {
        item_allay();
    }
    if (t.step == 1) {
        t.allay_abe += 4.0f;
    } else {
        t.allay_abe -= 4.0f;
    }
    if (t.allay_abe > 32.0f) {
        t.allay_abe = 32.0f;
    }
    if (t.allay_abe < 0.0f) {
        t.allay_abe = 0.0f;
    }
    for (i = 6; i > 0; i--) {
        t.boxblur[i] = t.boxblur[i - 1];
    }
    t.boxblur[0] = t.box;
    for (i = 0; i < 6; i++) {
        fog = 20.0f * (1.0f - i / 7.0f);
        t.boxblur[i][0][3] = fog;
        t.boxblur[i][3][3] = fog;
        t.boxblur[i][2][3] = fog;
        t.boxblur[i][1][3] = fog;
        look_zanzo(t.boxblur[i][0], t.boxblur[i][1], t.boxblur[i + 1][0], t.boxblur[i + 1][1]);
        look_zanzo(t.boxblur[i][1], t.boxblur[i][2], t.boxblur[i + 1][1], t.boxblur[i + 1][2]);
        look_zanzo(t.boxblur[i][2], t.boxblur[i][3], t.boxblur[i + 1][2], t.boxblur[i + 1][3]);
        look_zanzo(t.boxblur[i][3], t.boxblur[i][0], t.boxblur[i + 1][3], t.boxblur[i + 1][0]);
    }
    if (t.step != 12 && t.step != 13) {
        lookline(t.box);
    }
    look_blackscr(1);
}

/** Handles the cursor on the item ring: turning it, selecting an item, opening its commands. */
void cur_step(void) {
    int i;

    switch (t.step) {
    case 0:
        t.step++;
        break;
    case 1:
        if (!t.prs_btn) {
            if (shPadPress(0, 0x800) || t.analog[0]) {
                t.step = 4;
                SeCall(0x2710, 1.0f, 0);
            }
            if (shPadPress(0, 0x400) || t.analog[3]) {
                if (item.equip) {
                    t.step = 5;
                    SeCall(0x2710, 1.0f, 0);
                }
            }
            if (shPadTrigger(0, key_config.enter)) {
                if (!shPadPress(0, 0x200) && !shPadPress(0, 0x100) && !t.analog[1] && !t.analog[2] && !t.use_item) {
                    if (t.item_kind && (command_kind(t.item_kind) || t.shelf)) {
                        t.step = 6;
                        examine_file_load();
                        SeCall(0x2712, 1.0f, 0);
                    }
                }
            }
        }
        if (!t.prs_btn2) {
            if (shPadPress(0, 0x100) || t.analog[2]) {
                SeCall(0x2710, 1.0f, -30);
                t.item_no++;
                t.prs_btn2 = 1;
                if (t.item_no == t.item_count) {
                    t.item_no = 0;
                }
                if (shPadPress(0, 0x100)) {
                    t.turn_speed = 64;
                } else {
                    t.turn_speed = shPadPress(0, 0x40) - 192;
                }
            }
            if (shPadPress(0, 0x200) || t.analog[1]) {
                SeCall(0x2710, 1.0f, 30);
                t.item_no--;
                t.prs_btn2 = 1;
                if (t.item_no == -1) {
                    t.item_no = t.item_count - 1;
                }
                if (shPadPress(0, 0x200)) {
                    t.turn_speed = 64;
                } else {
                    t.turn_speed = -(shPadPress(0, 0x40) - 64);
                }
            }
        }
        if (shPadTrigger(0, key_config.cancel) && !t.prs_btn && !t.prs_btn2 && !t.combine[0]) {
            t.fade_flag = 1;
            t.fade_step0 = 1;
            t.fade_step1 = 6;
        }
        if (shPadTrigger(0, key_config.cancel)) {
            t.combine[0] = t.combine[1] = t.combine[2] = 0;
        }
        break;
    case 2:
        if (!t.prs_btn) {
            if (shPadPress(0, 0x100) || t.analog[2]) {
                t.step = 4;
                SeCall(0x2710, 1.0f, 60);
            }
            if (shPadPress(0, 0x400) || t.analog[3]) {
                t.step = 1;
                SeCall(0x2710, 1.0f, 60);
            }
        }
        if (shPadPress(0, key_config.enter)) {
            t.fade_flag = 1;
            t.fade_step0 = 7;
            t.fade_step1 = 2;
        }
        if (shPadTrigger(0, key_config.cancel) && !t.prs_btn && !t.prs_btn2 && !t.combine[0]) {
            t.fade_flag = 1;
            t.fade_step0 = 1;
            t.fade_step1 = 6;
        }
        if (shPadTrigger(0, key_config.cancel)) {
            t.combine[2] = 0;
            t.combine[1] = 0;
            t.combine[0] = 0;
        }
        break;
    case 3:
        if (!t.prs_btn) {
            if (shPadPress(0, 0x200) || t.analog[1]) {
                t.step = 4;
                SeCall(0x2710, 1.0f, -60);
            }
            if (shPadPress(0, 0x400) || t.analog[3]) {
                t.step = 1;
                SeCall(0x2710, 1.0f, 0);
            }
        }
        if (shPadTrigger(0, key_config.enter)) {
            if (MemoCommandCheck()) {
                t.fade_flag = 1;
                t.fade_step0 = 8;
                t.fade_step1 = 0;
            }
        }
        if (shPadTrigger(0, key_config.cancel) && !t.prs_btn && !t.prs_btn2 && !t.combine[0]) {
            t.fade_flag = 1;
            t.fade_step0 = 1;
            t.fade_step1 = 6;
        }
        if (shPadTrigger(0, key_config.cancel)) {
            t.combine[2] = 0;
            t.combine[1] = 0;
            t.combine[0] = 0;
        }
        break;
    case 4:
        if (!t.prs_btn) {
            if (shPadPress(0, 0x200) || t.analog[1]) {
                t.step = 2;
                SeCall(0x2710, 1.0f, 20);
            }
            if (shPadPress(0, 0x100) || t.analog[2]) {
                if (MemoCommandCheck()) {
                    t.step = 3;
                    SeCall(0x2710, 1.0f, -20);
                }
            }
            if (shPadPress(0, 0x400) || t.analog[3]) {
                t.step = 1;
                SeCall(0x2710, 1.0f, 0);
            }
            if (shPadPress(0, key_config.enter)) {
                t.fade_flag = 1;
                t.fade_step0 = 5;
                t.fade_step1 = 1;
            }
        }
        if (shPadTrigger(0, key_config.cancel) && !t.prs_btn && !t.prs_btn2 && !t.combine[0]) {
            t.fade_flag = 1;
            t.fade_step0 = 1;
            t.fade_step1 = 6;
        }
        if (shPadTrigger(0, key_config.cancel)) {
            t.combine[2] = 0;
            t.combine[1] = 0;
            t.combine[0] = 0;
        }
        break;
    case 5:
        if (!t.prs_btn) {
            if (shPadPress(0, 0x800) || t.analog[0]) {
                t.step = 1;
                SeCall(0x2710, 1.0f, 0);
            }
            if (shPadPress(0, key_config.enter)) {
                t.step = 7;
                SeCall(0x2710, 1.0f, 0);
            }
        }
        if (shPadTrigger(0, key_config.cancel) && !t.prs_btn && !t.prs_btn2) {
            t.step = 1;
            SeCall(0x2710, 1.0f, 60);
        }
        break;
    case 6:
        i = command_kind(t.item_kind);
        command_main(i);
        if (!t.prs_btn) {
            if (shPadPress(0, key_config.cancel) && !t.use_item) {
                t.command_volume = 0;
                t.step = 1;
                SeCall(0x2713, 1.0f, -30);
            }
        }
        break;
    case 7:
        i = command_kind(item.equip);
        weapon_command_main(i);
        if (!t.prs_btn) {
            if (shPadPress(0, key_config.cancel)) {
                t.command_volume = 0;
                t.step = 1;
                SeCall(0x2713, 1.0f, -30);
            }
        }
        break;
    case 8:
        if (!t.prs_btn) {
            t.step = 1;
            SeCall(0x2712, 1.0f, 30);
        }
        break;
    case 9:
        if (!t.prs_btn) {
            t.step = 1;
        }
        break;
    case 11:
        if (shPadTrigger(0, key_config.enter) && !t.use_item) {
            t.step = 1;
        }
        break;
    case 12:
        examine2_main();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            t.step = 13;
            if (t.item_kind == 0x30) {
                FcRead(data_pic_item_x_coinelder_ura_tex, layer_adr);
            } else if (t.item_kind == 0x31) {
                FcRead(data_pic_item_x_coinprisoner_ura_tex, layer_adr);
            } else if (t.item_kind == 0x2F) {
                FcRead(data_pic_item_x_coinsnake_ura_tex, layer_adr);
            }
        }
        break;
    case 13:
        examine2_main();
        if (!t.prs_btn) {
            if ((t.item_kind == 0x30 || t.item_kind == 0x31 || t.item_kind == 0x2F) && t.examine_step == 1) {
                if (fsSync(1, -1) >= 0) {
                    t.step = 12;
                    t.examine_step++;
                }
            } else {
                t.step = 1;
                t.examine_step = 0;
            }
            SeCall(0x2712, 1.0f, 30);
        }
        break;
    }
}


#define VOLUME_REPEAT() \
    ((t.volume_time % 20 == 1 && t.volume_time < 20) || (t.volume_time % 3 == 1 && t.volume_time >= 20))

/** Handles the command list of the selected item (use, combine, examine, ...).
 * @param command_step the command menu's step */
void command_main(int command_step) {
    switch (command_step) {
    case 1:
        t.gosa = -143;
        t.cur_max = 1;
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
            if (t.shelf && t.command_cur == 0) {
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                if (t.command_cur == t.shelf) {
                    if (!t.combine[0]) { event_item_use(t.item_kind); }
                    else if (t.item_kind == t.combine[0] && t.combine[1] == 0) { event_item_use(t.item_kind); }
                    else if (t.item_kind == t.combine[0] || t.item_kind == t.combine[1]) { combine_item_use(0); }
                    else { combine_item_use(t.item_kind); }
                    if (!t.use_item) {
                        t.step = 1;
                    }
                    SeCall(0x2712, 1.0f, -30);
                } else {
                    if (!t.combine[0]) { t.combine[0] = t.item_kind; }
                    else if (!t.combine[1] && t.combine[0] != t.item_kind) { t.combine[1] = t.item_kind; }
                    else if (t.combine[1] && t.combine[0] != t.item_kind && t.combine[1] != t.item_kind) { t.combine[2] = t.item_kind; }
                    else if (t.combine[1]) { combine_item_use(0); }
                    if (!t.use_item) {
                        t.step = 1;
                    }
                    SeCall(0x2712, 1.0f, -30);
                }
            }
        }
        break;
    case 2:
        t.gosa = -133 - t.shelf * 10;
        t.cur_max = t.shelf;
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
            if (t.shelf && t.command_cur == 0) {
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                ItemMedicineUse(t.item_kind);
                t.step = 8;
                SeCall(0x2712, 1.0f, -30);
            }
        }
        break;
    case 3:
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
            if (t.shelf && t.command_cur == 0) {
                if (t.item_kind == item.equip) {
                    item.equip = 0;
                }
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                if (item.equip != t.item_kind) {
                    item.equip = t.item_kind;
                } else {
                    item.equip = 0;
                }
                t.step = 9;
                SeCall(0x2712, 1.0f, -30);
            }
        }
        t.gosa = -133 - t.shelf * 10;
        t.cur_max = t.shelf;
        break;
    case 4:
        t.gosa = -143 - t.shelf * 15;
        t.cur_max = t.shelf + 1;
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
            if (t.shelf && t.command_cur == 0) {
                if (t.item_kind == item.equip) {
                    item.equip = 0;
                }
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                if (t.command_cur == t.shelf) {
                    if (item.equip != t.item_kind) {
                        item.equip = t.item_kind;
                    } else {
                        item.equip = 0;
                    }
                    t.step = 9;
                } else {
                    ItemWeaponReload(t.item_kind, 1);
                    t.step = 1;
                }
            }
            SeCall(0x2712, 1.0f, -30);
        }
        break;
    case 5:
        t.gosa = -133 - t.shelf * 10;
        t.cur_max = t.shelf;
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
            if (t.shelf && t.command_cur == 0) {
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                if ((item.flag[(t.item_kind - 1) >> 5] >> ((t.item_kind - 1) & 31)) & 1) {
                    ItemWeaponReload(t.item_kind, 1);
                }
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            }
        }
        break;
    case 6:
        if (t.command_light == 0) {
            if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
                if (t.shelf && t.command_cur == 0) {
                    if (!t.use_item) { t.use_item = 8; }
                    item.event_use[0] = t.item_kind;
                    ItemPutForShelf();
                    item_select[kind_no(t.item_kind)].del = 1;
                    t.step = 1;
                    SeCall(0x2712, 1.0f, -30);
                } else {
                    t.command_cur = 0;
                    t.command_light = 1;
                    SeCall(0x2712, 1.0f, -30);
                }
            }
            t.gosa = -133 - t.shelf * 10;
            t.cur_max = t.shelf;
        } else {
            if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
                if (t.command_cur == 0) {
                    item.light_switch = 1;
                } else {
                    item.light_switch = 0;
                }
                t.command_light = 0;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            }
            t.gosa = -135;
            t.cur_max = 1;
        }
        break;
    case 7:
        if (t.command_volume == 0) {
            if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
                if (t.shelf && t.command_cur == 0) {
                    if (!t.use_item) { t.use_item = 8; }
                    item.event_use[0] = t.item_kind;
                    ItemPutForShelf();
                    item_select[kind_no(t.item_kind)].del = 1;
                    t.step = 1;
                    SeCall(0x2712, 1.0f, -30);
                } else {
                    if (t.command_cur == t.shelf) {
                        t.command_cur = 0;
                        t.command_volume = 2;
                        SeCall(0x2712, 1.0f, -30);
                    } else {
                        t.command_cur = 0;
                        t.command_volume = 1;
                        SeCall(0x2712, 1.0f, -30);
                    }
                }
            }
        } else {
            if (t.command_volume == 2) {
                if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
                    if (t.command_cur == 0) {
                        item.radio_switch = 1;
                        t.command_volume = 0;
                        t.step = 1;
                        SeCall(0x2712, 1.0f, -30);
                    } else {
                        item.radio_switch = 0;
                        t.command_volume = 0;
                        t.step = 1;
                        SeCall(0x2712, 1.0f, -30);
                    }
                }
            } else {
                if (shPadPress(0, 0x100) || shPadPress(0, 0x200) || t.analog[1] || t.analog[2]) {
                    t.volume_time++;
                } else {
                    t.volume_time = 0;
                }
                if (shPadPress(0, 0x100) || t.analog[2]) {
                    if (VOLUME_REPEAT()) {
                        if (item.radio_volume < 15) {
                            item.radio_volume++;
                            SeCall(0x4A49, (4.0f + item.radio_volume) / 20.0f, -30);
                        }
                    }
                }
                if (shPadPress(0, 0x200) || t.analog[1]) {
                    if (VOLUME_REPEAT()) {
                        if (item.radio_volume > 0) {
                            item.radio_volume--;
                            SeCall(0x4A49, (4.0f + item.radio_volume) / 20.0f, -30);
                        }
                    }
                }
                if (shPadTrigger(0, key_config.enter) && t.volume_time == 0) {
                    t.command_volume = 0;
                    t.step = 1;
                    SeCall(0x2712, 1.0f, -30);
                }
                look_volume();
            }
        }
        if (t.command_volume == 0) {
            t.gosa = -143 - t.shelf * 15;
            t.cur_max = t.shelf + 1;
        } else if (t.command_volume == 1) {
            t.gosa = -135;
            t.cur_max = 0;
        } else {
            t.gosa = -135;
            t.cur_max = 1;
        }
        break;
    case 8:
        t.gosa = -143 - t.shelf * 15;
        t.cur_max = t.shelf + 1;
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn && !t.use_item) {
            if (t.shelf && t.command_cur == 0) {
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                if (t.command_cur == t.shelf) {
                    event_item_use(t.item_kind);
                    if (!t.use_item) {
                        t.step = 1;
                    }
                    SeCall(0x2712, 1.0f, -30);
                } else {
                    t.use_item = 4;
                    t.step = 11;
                    SeCall(0x2712, 1.0f, -30);
                }
            }
        }
        break;
    case 9:
        t.gosa = -143 - t.shelf * 15;
        t.cur_max = t.shelf + 1;
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn && !t.use_item) {
            if (t.shelf && t.command_cur == 0) {
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                if (t.command_cur == t.shelf) {
                    event_item_use(t.item_kind);
                    SeCall(0x2712, 1.0f, -30);
                } else {
                    t.fade_flag = 2;
                    ScreenEffectFadeStart(1, 0.5f);
                    if (t.item_kind == 0x12) {
                        t.examine_msg = 0xB8;
                        FcRead(data_pic_etc_p_letterm_tex, get_gp_data_buf_addr());
                    } else {
                        if (t.item_kind == 0x13) {
                            t.examine_msg = 0xB9;
                            FcRead(data_pic_etc_p_laura_letter_tex, get_gp_data_buf_addr());
                        } else {
                            if (t.item_kind == 0x47) {
                                t.examine_msg = 0xBA;
                                FcRead(data_pic_out_p_lostmemory_tex, get_gp_data_buf_addr());
                            } else {
                                if (t.item_kind == 0x48) {
                                    t.examine_msg = 0xBB;
                                    FcRead(data_pic_htl_p_redreling_tex, get_gp_data_buf_addr());
                                }
                            }
                        }
                    }
                    SeCall(0x2712, 1.0f, -30);
                }
            }
        }
        break;
    case 10:
        t.gosa = -158;
        t.cur_max = 2;
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn && !t.use_item) {
            if (t.shelf && t.command_cur == 0) {
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                if (t.command_cur == t.shelf) {
                    if (!t.combine[0]) { event_item_use(t.item_kind); }
                    else if (t.item_kind == t.combine[0] && t.combine[1] == 0) { event_item_use(t.item_kind); }
                    else if (t.item_kind == t.combine[0] || t.item_kind == t.combine[1]) { combine_item_use(0); }
                    else { combine_item_use(t.item_kind); }
                    SeCall(0x2712, 1.0f, -30);
                } else {
                    if (!t.shelf && t.command_cur == 1) {
                        if (!t.combine[0]) { t.combine[0] = t.item_kind; }
                        else if (!t.combine[1] && t.combine[0] != t.item_kind) { t.combine[1] = t.item_kind; }
                        else if (t.combine[1] && t.combine[0] != t.item_kind && t.combine[1] != t.item_kind) { t.combine[2] = t.item_kind; }
                        else if (t.combine[1]) { combine_item_use(0); }
                        if (!t.use_item) {
                            t.step = 1;
                        }
                        SeCall(0x2712, 1.0f, -30);
                    } else {
                        t.step = 12;
                        SeCall(0x2712, 1.0f, -30);
                    }
                }
            }
        }
        break;
    case 11:
        t.gosa = -143 - t.shelf * 15;
        t.cur_max = t.shelf + 1;
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn && !t.use_item) {
            if (t.shelf && t.command_cur == 0) {
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                if (t.command_cur == t.shelf) {
                    if (!t.combine[0]) { event_item_use(t.item_kind); }
                    else if (t.item_kind == t.combine[0] && t.combine[1] == 0) { event_item_use(t.item_kind); }
                    else if (t.item_kind == t.combine[0] || t.item_kind == t.combine[1]) { combine_item_use(0); }
                    else { combine_item_use(t.item_kind); }
                    SeCall(0x2712, 1.0f, -30);
                } else {
                    t.step = 12;
                    SeCall(0x2712, 1.0f, -30);
                }
            }
        }
        break;
    case 12:
        break;
    default:
        t.gosa = -133;
        t.cur_max = 0;
        if (shPadTrigger(0, key_config.enter) && !t.prs_btn && !t.use_item) {
            if (t.shelf && t.command_cur == 0) {
                if (!t.use_item) { t.use_item = 8; }
                item.event_use[0] = t.item_kind;
                ItemPutForShelf();
                item_select[kind_no(t.item_kind)].del = 1;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            }
        }
        break;
    }
    if ((shPadTrigger(0, 0x800) || t.analog[0] == 1) && !t.use_item) {
        if (t.command_cur < t.cur_max) {
            t.command_cur++;
            SeCall(0x2710, 1.0f, -30);
        }
    }
    if ((shPadTrigger(0, 0x400) || t.analog[3] == 1) && !t.use_item) {
        if (t.command_cur > 0) {
            t.command_cur--;
            SeCall(0x2710, 1.0f, -30);
        }
    }
}

/** Handles the command list of the selected weapon (unequip, reload, put on a shelf, ...).
 * @param command_step the command menu's step */
void weapon_command_main(int command_step) {
    switch (command_step) {
    case 3:
        if (shPadTrigger(0, key_config.enter) && t.prs_btn == 0) {
            if (t.shelf && t.command_cur == 0) {
                if (t.use_item == 0) {
                    t.use_item = 8;
                }
                item.event_use[0] = item.equip;
                ItemPutForShelf();
                item_select[kind_no(item.equip)].del = 1;
                item.equip = 0;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                item.equip = 0;
                t.step = 9;
                SeCall(0x2712, 1.0f, -30);
            }
        }
        t.gosa = -133 - t.shelf * 10;
        t.cur_max = t.shelf;
        break;
    case 4:
        t.gosa = -143 - t.shelf * 15;
        t.cur_max = t.shelf + 1;
        if (shPadTrigger(0, key_config.enter) && t.prs_btn == 0) {
            if (t.shelf && t.command_cur == 0) {
                if (t.use_item == 0) {
                    t.use_item = 8;
                }
                item.event_use[0] = item.equip;
                ItemPutForShelf();
                item_select[kind_no(item.equip)].del = 1;
                item.equip = 0;
                t.step = 1;
                SeCall(0x2712, 1.0f, -30);
            } else {
                if (t.command_cur == t.shelf) {
                    item.equip = 0;
                    t.step = 9;
                    SeCall(0x2712, 1.0f, -30);
                } else {
                    ItemWeaponReload(item.equip, 1);
                    t.step = 1;
                    SeCall(0x2712, 1.0f, -30);
                }
            }
        }
        break;
    }
    if (shPadTrigger(0, 0x800) || t.analog[0] == 1) {
        if (t.command_cur < t.cur_max) {
            t.command_cur++;
            SeCall(0x2710, 1.0f, -30);
        }
    }
    if (shPadTrigger(0, 0x400) || t.analog[3] == 1) {
        if (t.command_cur > 0) {
            t.command_cur--;
            SeCall(0x2710, 1.0f, -30);
        }
    }
}

/** Resets the item menu state and builds the list of items the player has. */
/* Matching: the original's line table has t.combine[2..0] = 0 on one line and j++ on the line of
 * item_select[j].kind = i, so they are written as one statement each (same code). Of the spellings
 * that fit the line table, these are the ones that leave the arena state set_position's float
 * order needs (docs/toolchain.md, "Root cause"). */
void item_main_setup(void) {
    int i;
    int j;

    t.main_step = 0;
    t.step = 0;
    t.examine_step = 0;
    t.turn_speed = 0;
    t.command_cur = 0;
    t.command_abe = 0;
    t.command_move = 0.0f;
    t.gosa = -143;
    t.cur_max = 0;
    t.command_volume = 0;
    t.command_light = 0;
    t.item_no = 0;
    t.item_kind = 0;
    t.item_count = 0;
    t.turnf = 0.0f;
    t.volume_time = 0;
    t.hp_time1 = -150.0f;
    t.hp_time2 = 0.0f;
    t.hp_abe = 0.0f;
    t.allay_time = 0;
    t.allay_abe = 0.0f;
    t.sprite_time = 0.0f;
    t.use_item = 0;
    t.weapon_scale[1] = 0.0f;
    t.weapon_scale[0] = 0.0f;
    t.combine[0] = t.combine[1] = t.combine[2] = 0;
    t.fade = 1.0f;
    t.fade_flag = 0;
    t.fade_step0 = 0;
    for (i = 0; i < 75; i++) {
        item_select[i].kind = 0;
        item_select[i].count = 0;
        item_select[i].del = 0;
        item_select[i].rot = 0.0f;
        item_select[i].item_scale = 0.0f;
    }
    set_position(t.step);
    for (j = 0; j < 6; j++) {
        for (i = 6; i > 0; i--) {
            t.boxblur[i] = t.boxblur[i - 1];
        }
        t.boxblur[0] = t.box;
    }
    j = 0;
    t.item_count = 0;
    for (i = 0; i < 75; i++) {
        if ((item.flag[i >> 5] >> (i & 31)) & 1) {
            item_select[j++].kind = i;
            t.item_count++;
        }
    }
    if (t.item_count < 9) {
        t.item_count = 9;
    }
    t.item_no = t.item_count - 5;
    for (i = 0; i < t.item_count; i++) {
        if (item.last_cursor == 0) {
            break;
        }
        if ((unsigned char)item.last_cursor == item_select[i].kind) {
            t.item_no = i - 5;
            if (t.item_no < 0) {
                t.item_no += t.item_count;
            }
            break;
        }
    }
    item_turn();
    if (item.equip == 4) {
        t.weapon_scale[0] = 0.9f;
    } else if (item.equip == 11) {
        t.weapon_scale[1] = 0.9f;
    }
    enWaitAllInsect();
}

/** Sets or moves (move_near) the corners of the menu boxes (t.box) for a menu step.
 * @param step the menu step */
/* FAKEMATCH: the move_near arguments' constant order is fitted, not recovered: a block per box, comma
 * expressions joining some boxes' two assignments, and redundant (float) casts on some third arguments
 * (docs/matching-notes.md#otn_itemmain-set_position). */
void set_position(int step) {
    switch (step) {
    case 0:
        t.box[0][0] = -120.0f; t.box[0][1] = -120.0f; t.box[0][3] = 255.0f;
        t.box[1][0] = -120.0f; t.box[1][1] = 120.0f; t.box[1][3] = 255.0f;
        t.box[2][0] = 120.0f; t.box[2][1] = 120.0f; t.box[2][3] = 255.0f;
        t.box[3][0] = 120.0f; t.box[3][1] = -120.0f; t.box[3][3] = 255.0f;
        break;

    case 1:
        { t.box[0][0] = move_near(2.0f, 1.0f, t.box[0][0], -55); t.box[0][1] = move_near(2.0f, 1.0f, t.box[0][1], -65); }
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], -55); t.box[1][1] = move_near(2.0f, 1.0f, t.box[1][1], 65); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], 55); t.box[2][1] = move_near(2.0f, 1.0f, t.box[2][1], 65); }
        { t.box[3][0] = move_near(2.0f, 1.0f, t.box[3][0], 55); t.box[3][1] = move_near(2.0f, 1.0f, t.box[3][1], -65); }
        break;

    case 2:
        { t.box[0][0] = move_near(2.0f, 1.0f, t.box[0][0], -240); t.box[0][1] = move_near(2.0f, 1.0f, t.box[0][1], 173); }
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], -240); t.box[1][1] = move_near(2.0f, 1.0f, t.box[1][1], 203); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], -80); t.box[2][1] = move_near(2.0f, 1.0f, t.box[2][1], 203); }
        { t.box[3][0] = move_near(2.0f, 1.0f, t.box[3][0], -80); t.box[3][1] = move_near(2.0f, 1.0f, t.box[3][1], 173); }
        break;

    case 4:
        { t.box[0][0] = move_near(2.0f, 1.0f, t.box[0][0], -80); t.box[0][1] = move_near(2.0f, 1.0f, t.box[0][1], 173); }
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], -80); t.box[1][1] = move_near(2.0f, 1.0f, (float)t.box[1][1], 203); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], 80); t.box[2][1] = move_near(2.0f, 1.0f, t.box[2][1], 203); }
        { t.box[3][0] = move_near(2.0f, 1.0f, t.box[3][0], 80); t.box[3][1] = move_near(2.0f, 1.0f, t.box[3][1], 173); }
        break;

    case 3:
        { t.box[0][0] = move_near(2.0f, 1.0f, (float)t.box[0][0], 80); t.box[0][1] = move_near(2.0f, 1.0f, (float)t.box[0][1], 173); }
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], 80); t.box[1][1] = move_near(2.0f, 1.0f, t.box[1][1], 203); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], 240); t.box[2][1] = move_near(2.0f, 1.0f, (float)t.box[2][1], 203); }
        { t.box[3][0] = move_near(2.0f, 1.0f, (float)t.box[3][0], 240), t.box[3][1] = move_near(2.0f, 1.0f, (float)t.box[3][1], 173); }
        break;

    case 5:
    case 9:
        { t.box[0][0] = move_near(2.0f, 1.0f, (float)t.box[0][0], -80); t.box[0][1] = move_near(2.0f, 1.0f, (float)t.box[0][1], -203); }
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], -80); t.box[1][1] = move_near(2.0f, 1.0f, t.box[1][1], -63); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], 80); t.box[2][1] = move_near(2.0f, 1.0f, t.box[2][1], -63); }
        { t.box[3][0] = move_near(2.0f, 1.0f, t.box[3][0], 80), t.box[3][1] = move_near(2.0f, 1.0f, t.box[3][1], -203); }
        break;

    case 6:
    case 7:
        { t.box[0][0] = move_near(2.0f, 1.0f, t.box[0][0], 80); t.box[0][1] = move_near(2.0f, 1.0f, t.box[0][1], -203); }
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], 80); t.box[1][1] = move_near(2.0f, 1.0f, t.box[1][1], -80); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], 240); t.box[2][1] = move_near(2.0f, 1.0f, t.box[2][1], -80); }
        { t.box[3][0] = move_near(2.0f, 1.0f, (float)t.box[3][0], 240); t.box[3][1] = move_near(2.0f, 1.0f, (float)t.box[3][1], -203); }
        break;

    case 8:
        t.box[0][0] = move_near(2.0f, 1.0f, t.box[0][0], -240), t.box[0][1] = move_near(2.0f, 1.0f, t.box[0][1], -203);
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], -240); t.box[1][1] = move_near(2.0f, 1.0f, t.box[1][1], -60); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], -80); t.box[2][1] = move_near(2.0f, 1.0f, t.box[2][1], -60); }
        { t.box[3][0] = move_near(2.0f, 1.0f, t.box[3][0], -80); t.box[3][1] = move_near(2.0f, 1.0f, (float)t.box[3][1], -203); }
        break;

    case 11:
        { t.box[0][0] = move_near(2.0f, 1.0f, (float)t.box[0][0], -230); t.box[0][1] = move_near(2.0f, 1.0f, (float)t.box[0][1], 80); }
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], -230); t.box[1][1] = move_near(2.0f, 1.0f, t.box[1][1], 170); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], 230); t.box[2][1] = move_near(2.0f, 1.0f, t.box[2][1], 170); }
        { t.box[3][0] = move_near(2.0f, 1.0f, t.box[3][0], 230); t.box[3][1] = move_near(2.0f, 1.0f, t.box[3][1], 80); }
        break;

    case 12:
        { t.box[0][0] = move_near(2.0f, 1.0f, t.box[0][0], -100); t.box[0][1] = move_near(2.0f, 1.0f, t.box[0][1], -150); }
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], -100); t.box[1][1] = move_near(2.0f, 1.0f, t.box[1][1], 50); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], 100); t.box[2][1] = move_near(2.0f, 1.0f, t.box[2][1], 50); }
        { t.box[3][0] = move_near(2.0f, 1.0f, t.box[3][0], 100); t.box[3][1] = move_near(2.0f, 1.0f, t.box[3][1], -150); }
        break;

    case 13:
        { t.box[0][0] = move_near(2.0f, 1.0f, t.box[0][0], -55), t.box[0][1] = move_near(2.0f, 1.0f, t.box[0][1], -52); }
        { t.box[1][0] = move_near(2.0f, 1.0f, t.box[1][0], -55); t.box[1][1] = move_near(2.0f, 1.0f, t.box[1][1], -48); }
        { t.box[2][0] = move_near(2.0f, 1.0f, t.box[2][0], 55); t.box[2][1] = move_near(2.0f, 1.0f, t.box[2][1], -48); }
        { t.box[3][0] = move_near(2.0f, 1.0f, t.box[3][0], 55); t.box[3][1] = move_near(2.0f, 1.0f, t.box[3][1], -52); }
        break;
    }
}

/** Assigns each item its slot on the ring, counting from the selected item. */
void item_turn(void) {
    int i;
    int j;

    for (i = 0; i < t.item_count; i++) {
        item_select[i].count = 0;
    }
    for (i = t.item_no, j = 0; i < t.item_no + 10; i++, j++) {
        if (i < t.item_count) {
            item_select[i].count = j;
        } else {
            item_select[i - t.item_count].count = j;
        }
    }
}

/* The redundant (float) cast makes the third argument evaluate before the converted fourth. */
/** Moves an item's ring position toward its slot and returns its place on the ring.
 * @param f0 result: x
 * @param f1 result: z
 * @param count target slot
 * @param r0 current position on the ring (updated) */
void item_position(float *f0, float *f1, int count, float *r0) {
    *r0 = move_near(20.0f, 110.0f + t.turn_speed, (float)(*r0 * 1000), count * 1000);
    *r0 /= 1000.0f;
    if (count - *r0 > 1.5f || count - *r0 < -1.5f) {
        *r0 = count;
    }
    *f0 = 1500.0 * sin(PI * (*r0 - 5.0f) / 5.0f / 1.6f);
    *f1 = 1000.0 * -cos(PI * (*r0 - 5.0f) / 5.0f / 1.6f);
    if (count == *r0 && count == 5) {
        t.prs_btn2 = 0;
    }
}

/** Moves f2 toward f3 by a proportional step (distance / f0) plus a fixed step f1, per frame at 30 fps.
 * @param f0 proportional divisor
 * @param f1 fixed step
 * @param f2 current value
 * @param f3 target
 * @return the new value (as int) */
int move_near(float f0, float f1, float f2, float f3) {
    float f;

    if (f2 < f3) {
        f2 += (f3 - f2) / f0 * (30.0f * shGetDT());
        f2 += f1 * (30.0f * shGetDT());
        if (!(f2 < f3)) {
            f2 = f3;
        }
    } else {
        f2 -= (f2 - f3) / f0 * (30.0f * shGetDT());
        f2 -= f1 * (30.0f * shGetDT());
        if (f2 <= f3) {
            f2 = f3;
        }
    }
    f = f2; /* Matching: reconstructed; the original's DWARF has f, with no code of its own */
    return f;
}

/** Returns the command-list kind (1-11) an item uses, 0 for none.
 * @param kind item kind */
int command_kind(int kind) {
    switch (kind) {
    case 22: case 24: case 30: case 31: case 32: case 34: case 35: case 36: case 37: case 39:
    case 41: case 42: case 43: case 44: case 45: case 46: case 50: case 51: case 52: case 55:
    case 59: case 60: case 61: case 62: case 63: case 64: case 65: case 66: case 67: case 68:
    case 69: case 70: case 73:
        return 1;
    case 1: case 2: case 3:
        return 2;
    case 10: case 11: case 12: case 13: case 14:
        return 3;
    case 4: case 6: case 8:
        return 4;
    case 5: case 7: case 9:
        return 5;
    case 15:
        return 6;
    case 16:
        return 7;
    case 20: case 21: case 26: case 27: case 28: case 40: case 74:
        return 8;
    case 18: case 19: case 71: case 72:
        return 9;
    case 23: case 25: case 29: case 33: case 38: case 47: case 48: case 49: case 53: case 54:
    case 56: case 57: case 58:
        return 10;
    case 17:
        return 11;
    }
    return 0;
}

/** Marks the used-up medicines and ammunition for removal from the ring. */
void del_check(void) {
    int i;

    for (i = 0; i < t.item_count; i++) {
        switch (item_select[i].kind) {
        case 1:
        case 2:
        case 3:
        case 5:
        case 7:
        case 9:
            if (item.number[item_select[i].kind] == 0) {
                item_select[kind_no(item_select[i].kind)].del = 1;
            }
            break;
        }
    }
}

/** Returns the ring index of an item kind (0 if it isn't there).
 * @param kind item kind */
int kind_no(int kind) {
    int i;

    for (i = 0; i < t.item_count; i++) {
        if (kind == item_select[i].kind) {
            return i;
        }
    }
    return 0;
}

/** Uses an item: if it triggers an event, fades out to it; otherwise sets use_item = 1 for the
 * menu's message.
 * @param kind item kind */
void event_item_use(int kind) {
    if (ItemEventCheck(kind, 0, 0) != -1) {
        item.event_use[0] = kind;
        item.event_use[1] = 0;
        item.event_use[2] = 0;
        t.fade_flag = 1;
        t.fade_step0 = 1;
        t.fade_step1 = 6;
    } else {
        t.use_item = 1;
    }
}

/** Uses an item together with the items picked to combine: fades out to the event if they
 * trigger one, else sets use_item (3: they don't combine, 1: no event) for the menu's message; then adds
 * the item to the combination.
 * @param kind item kind */
void combine_item_use(int kind) {
    if (ItemEventCheck(kind, t.combine[0], t.combine[1]) != -1 &&
        ItemEventCheck(kind, t.combine[0], t.combine[1]) != -2) {
        item.event_use[0] = kind;
        item.event_use[1] = t.combine[0];
        item.event_use[2] = t.combine[1];
        t.fade_flag = 1;
        t.fade_step0 = 1;
        t.fade_step1 = 6;
    } else {
        if (ItemEventCheck(kind, t.combine[0], t.combine[1]) != -1) {
            t.use_item = 3;
        } else {
            t.use_item = 1;
        }
    }
    if (t.combine[1]) {
        t.combine[2] = kind;
    } else {
        t.combine[1] = kind;
    }
}

/** The full-screen examine view of an item (picture, then back to the menu). */
void examine_main(void) {
    static struct PicDraw_Data i_pic;

    switch (t.examine_step) {
    case 0:
        fontClear();
        examine_rgb = 0x80;
        PictureLoadImage((struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(), 0, -1, -1);
        shQzero(&i_pic, sizeof(i_pic));
        i_pic.ap = (struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(); i_pic.tex = -1; i_pic.clut = -1; i_pic.status |= 1;
        i_pic.a = 0x80; i_pic.alpha_a = 0; i_pic.alpha_b = 1; i_pic.alpha_c = 0; i_pic.alpha_d = 1; i_pic.alpha_fix = 0x80; i_pic.status |= 0x20;
        i_pic.r = 0x80; i_pic.g = 0x80; i_pic.b = 0x80; i_pic.status |= 0x10;
        i_pic.otp = 1;
        PictureDraw(&i_pic);
        look_blackscr(1);
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            t.examine_step = 1;
        }
        break;
    case 1:
        if (examine_rgb > 69) {
            examine_rgb -= 300.0f * shGetDT();
        }
        PictureLoadImage((struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(), 0, -1, -1);
        shQzero(&i_pic, sizeof(i_pic));
        i_pic.ap = (struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(); i_pic.tex = -1; i_pic.clut = -1; i_pic.status |= 1;
        i_pic.a = 0x80; i_pic.alpha_a = 0; i_pic.alpha_b = 1; i_pic.alpha_c = 0; i_pic.alpha_d = 1; i_pic.alpha_fix = 0x80; i_pic.status |= 0x20;
        i_pic.r = examine_rgb; i_pic.g = examine_rgb; i_pic.b = examine_rgb; i_pic.status |= 0x10;
        i_pic.otp = 1;
        PictureDraw(&i_pic);
        look_blackscr(1);
        if (EvSubMessage(t.examine_msg)) {
            t.examine_step = 2;
            ScreenEffectFadeStart(1, 0.0f);
            item_main_setup();
            TgsItemPictureLoad();
        }
        break;
    case 2:
        if (ScreenEffectFadeCheck()) {
            t.examine_step = 0;
            t.main_step = 1;
        }
        break;
    }
}

/** Starts loading the examine picture of the selected item behind the item sheet (layer_adr). */
void examine_file_load(void) {
    layer_adr = get_gp_data_buf_addr() + ((FcGetFileSize(data_pic_etc_itemmenu2_tex) + 0x7FF) & ~0x7FF);
    switch (t.item_kind) {
    case 0x2F:
        FcRead(data_pic_item_x_coinsnake_tex, layer_adr);
        break;
    case 0x30:
        FcRead(data_pic_item_x_coinelder_tex, layer_adr);
        break;
    case 0x31:
        FcRead(data_pic_item_x_coinprisoner_tex, layer_adr);
        break;
    case 0x19:
        FcRead(data_pic_item_x_keyclock_tex, layer_adr);
        break;
    case 0x38:
        FcRead(data_pic_item_x_plate_kick_tex, layer_adr);
        break;
    case 0x39:
        FcRead(data_pic_item_x_plate_pig_tex, layer_adr);
        break;
    case 0x3A:
        FcRead(data_pic_item_x_plate_female_tex, layer_adr);
        break;
    case 0x17:
        FcRead(data_pic_item_x_keygate_tex, layer_adr);
        break;
    case 0x1D:
        FcRead(data_pic_item_x_keynorth_tex, layer_adr);
        break;
    case 0x21:
        FcRead(data_pic_item_x_keyrapis_tex, layer_adr);
        break;
    case 0x26:
        FcRead(data_pic_item_x_keyspiral_tex, layer_adr);
        break;
    case 0x36:
        FcRead(data_pic_item_x_ringlead_tex, layer_adr);
        break;
    case 0x35:
        FcRead(data_pic_item_x_ringopper_tex, layer_adr);
        break;
    case 0x11:
        FcRead(data_pic_item_x_mary_p_tex, layer_adr);
        break;
    }
}

/** The examine view of items with their own picture, loaded by examine_file_load. */
void examine2_main(void) {
    static struct PicDraw_Data i_pic;
    static int i;

    switch (t.examine_step) {
    case 0:
        if (fsSync(1, -1) >= 0) {
            t.examine_step++;
        }
        if (t.item_kind == 29 || t.item_kind == 25 || t.item_kind == 38 || t.item_kind == 47 ||
            t.item_kind == 48 || t.item_kind == 49) {
            t.use_item = 7;
        }
        if (t.item_kind == 25 && !GAME_FLAG(77)) {
            t.use_item = 0;
        }
        if (t.item_kind == 23) {
            game_flag.flag[39] |= 0x400000;
        }
        break;
    case 1:
    case 2:
        PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 6, -1, -1);
        shQzero(&i_pic, sizeof(i_pic));
        i_pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
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
        i_pic.x0 = (int)t.box[0][0] * 16;
        i_pic.y0 = (int)t.box[0][1] * 16;
        i_pic.x1 = (int)t.box[2][0] * 16;
        i_pic.y1 = (int)t.box[2][1] * 16;
        i_pic.status |= 2;
        i_pic.r = 0x80;
        i_pic.g = 0x80;
        i_pic.b = 0x80;
        i_pic.status |= 0x10;
        i_pic.otp = 7;
        PictureDraw(&i_pic);
        break;
    }
    for (i = 0; i < 4; i++) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0001, 0);
        PK_ADD(0x30000);
        PK_ADD(0x47);
        spkCloseOpenDGiftag(0x7400000000008000, 0x4444410);
        PK_ADD(2);
        PK_ADD(GS_SET_RGBAQ(lcolor * (3 - i) + 30, lcolor * (3 - i) + 30, lcolor * (3 - i) + 30, 0x20, 0));
        PK_ADD(GS_SET_XYZF(zs(t.box[0][0] - i), zs(t.box[0][1] - i), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(t.box[1][0] - i), zs(t.box[1][1] + i), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(t.box[2][0] + i), zs(t.box[2][1] + i), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(t.box[3][0] + i), zs(t.box[3][1] - i), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(t.box[0][0] - i), zs(t.box[0][1] - i), zs(-10), 0xFF));
        spkCloseGiftag();
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0001, 0);
        PK_ADD(0x30000);
        PK_ADD(0x47);
        spkCloseOpenDGiftag(0x7400000000008000, 0x4444410);
        PK_ADD(2);
        PK_ADD(GS_SET_RGBAQ(lcolor * (3 - i) + 30, lcolor * (3 - i) + 30, lcolor * (3 - i) + 30, 0x20, 0));
        PK_ADD(GS_SET_XYZF(zs(t.box[0][0] + i), zs(t.box[0][1] + i), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(t.box[1][0] + i), zs(t.box[1][1] - i), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(t.box[2][0] - i), zs(t.box[2][1] - i), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(t.box[3][0] - i), zs(t.box[3][1] + i), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(t.box[0][0] + i), zs(t.box[0][1] + i), zs(-10), 0xFF));
        spkCloseGiftag();
    }
}

/** Prints the menu texts: item name and description, commands and messages. */
void font_print(void) {
    int i;
    float f;

    f = 128.0 * (1.0 - t.fade) * (1.0 - t.fade);
    fontSetColorDirect(f, f, f, 0xFF);
    fontCrushOn();
    if ((t.item_kind || t.use_item == 8) &&
        (t.step == 1 || t.step == 6 || t.step == 9 || t.step == 8 || t.step == 11 || t.step == 12 || t.step == 13)) {
        if (t.item_kind && !t.use_item) {
            kage_font(msg_buffer, (t.item_kind - 1) * 2 + 20, 20, 325);
        }
        if (!t.use_item) {
            kage_font(msg_buffer, (t.item_kind - 1) * 2 + 21, 45, 360);
        } else {
            if (t.use_item == 1) {
                kage_font(msg_buffer, 16, 45, 360);
                if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
                    t.step = 1;
                    t.prs_btn = 1;
                    t.use_item = 0;
                    t.combine[2] = 0;
                    t.combine[1] = 0;
                    t.combine[0] = 0;
                }
            } else {
                if (t.use_item == 2) {
                    kage_font(msg_buffer, 17, 45, 360);
                    if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
                        t.step = 1;
                        t.prs_btn = 1;
                        t.use_item = 0;
                        t.combine[2] = 0;
                        t.combine[1] = 0;
                        t.combine[0] = 0;
                    }
                } else {
                    if (t.use_item == 3) {
                        kage_font(msg_buffer, 18, 45, 360);
                        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
                            t.step = 1;
                            t.prs_btn = 1;
                            t.use_item = 0;
                            t.combine[2] = 0;
                            t.combine[1] = 0;
                            t.combine[0] = 0;
                        }
                    } else {
                        if (t.use_item == 4) {
                            t.use_item++;
                        } else {
                            if (t.use_item == 5) {
                                if (t.item_kind == 0x15) {
                                    t.use_item++;
                                } else if (t.item_kind == 0x1C) {
                                    t.use_item++;
                                } else if (t.item_kind == 0x28) {
                                    t.use_item++;
                                } else if (t.item_kind == 0x4A) {
                                    t.use_item++;
                                } else if (t.item_kind == 0x1A) {
                                    t.use_item++;
                                } else if (t.item_kind == 0x14) {
                                    kage_font(msg_buffer, 181, 45, 340);
                                    if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
                                        t.use_item++;
                                    }
                                } else if (t.item_kind == 0x1B) {
                                    kage_font(msg_buffer, 175, 45, 340);
                                    if (shPadTrigger(0, key_config.enter) && !t.prs_btn) {
                                        t.use_item++;
                                    }
                                } else {
                                    t.use_item++;
                                }
                            } else {
                                if (t.use_item == 6) {
                                    if (t.item_kind == 0x15) {
                                        kage_font(msg_buffer, 168, 45, 340);
                                    } else if (t.item_kind == 0x1C) {
                                        kage_font(msg_buffer, 177, 45, 340);
                                    } else if (t.item_kind == 0x28) {
                                        kage_font(msg_buffer, 180, 45, 340);
                                    } else if (t.item_kind == 0x4A) {
                                        kage_font(msg_buffer, 183, 45, 340);
                                    } else if (t.item_kind == 0x1A) {
                                        kage_font(msg_buffer, 173, 45, 340);
                                    } else if (t.item_kind == 0x14) {
                                        kage_font(msg_buffer, 182, 45, 340);
                                    } else if (t.item_kind == 0x1B) {
                                        kage_font(msg_buffer, 174, 45, 340);
                                    } else {
                                        t.step = 1;
                                        t.use_item = 0;
                                    }
                                    if ((shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) && !t.prs_btn) {
                                        t.step = 1;
                                        t.use_item = 0;
                                        t.prs_btn = 1;
                                    }
                                } else {
                                    if (t.use_item == 7) {
                                        if (t.item_kind == 0x1D) {
                                            kage_font(msg_buffer, 178, 45, 340);
                                        } else if (t.item_kind == 0x19) {
                                            kage_font(msg_buffer, 169, 45, 340);
                                        } else if (t.item_kind == 0x26) {
                                            kage_font(msg_buffer, 179, 45, 340);
                                        } else if (t.item_kind == 0x2F) {
                                            kage_font(msg_buffer, 170, 45, 340);
                                        } else if (t.item_kind == 0x30) {
                                            kage_font(msg_buffer, 171, 45, 340);
                                        } else if (t.item_kind == 0x31) {
                                            kage_font(msg_buffer, 172, 45, 340);
                                        }
                                        if (t.step == 1) {
                                            t.use_item = 0;
                                        }
                                    } else {
                                        if (t.use_item == 8) {
                                            if (t.item_kind) {
                                                t.use_item_kind = t.item_kind;
                                            }
                                            if (t.item_kind == t.use_item_kind || !t.item_kind) {
                                                if (t.use_item_kind == 1) {
                                                    fontPrintStrNum(msg_buffer, 190, 45, 360);
                                                }
                                                if (t.use_item_kind == 2) {
                                                    fontPrintStrNum(msg_buffer, 191, 45, 360);
                                                }
                                                if (t.use_item_kind == 3) {
                                                    fontPrintStrNum(msg_buffer, 192, 45, 360);
                                                }
                                                if (t.use_item_kind == 4) {
                                                    fontPrintStrNum(msg_buffer, 193, 45, 360);
                                                }
                                                if (t.use_item_kind == 5) {
                                                    fontPrintStrNum(msg_buffer, 194, 45, 360);
                                                }
                                                if (t.use_item_kind == 6) {
                                                    fontPrintStrNum(msg_buffer, 195, 45, 360);
                                                }
                                                if (t.use_item_kind == 7) {
                                                    fontPrintStrNum(msg_buffer, 196, 45, 360);
                                                }
                                                if (t.use_item_kind == 8) {
                                                    fontPrintStrNum(msg_buffer, 197, 45, 360);
                                                }
                                                if (t.use_item_kind == 9) {
                                                    fontPrintStrNum(msg_buffer, 198, 45, 360);
                                                }
                                                if (t.use_item_kind == 10) {
                                                    fontPrintStrNum(msg_buffer, 199, 45, 360);
                                                }
                                                if (t.use_item_kind == 11) {
                                                    fontPrintStrNum(msg_buffer, 200, 45, 360);
                                                }
                                                if (t.use_item_kind == 12) {
                                                    fontPrintStrNum(msg_buffer, 201, 45, 360);
                                                }
                                                if (t.use_item_kind == 13) {
                                                    fontPrintStrNum(msg_buffer, 202, 45, 360);
                                                }
                                                if (t.use_item_kind == 14) {
                                                    fontPrintStrNum(msg_buffer, 203, 45, 360);
                                                }
                                                if (t.use_item_kind == 15) {
                                                    fontPrintStrNum(msg_buffer, 204, 45, 360);
                                                }
                                                if (t.use_item_kind == 16) {
                                                    fontPrintStrNum(msg_buffer, 205, 45, 360);
                                                }
                                                if (t.use_item_kind == 17) {
                                                    fontPrintStrNum(msg_buffer, 206, 45, 360);
                                                }
                                                if (t.use_item_kind == 18) {
                                                    fontPrintStrNum(msg_buffer, 207, 45, 360);
                                                }
                                                if (t.use_item_kind == 19) {
                                                    fontPrintStrNum(msg_buffer, 208, 45, 360);
                                                }
                                                if (t.use_item_kind == 21) {
                                                    fontPrintStrNum(msg_buffer, 209, 45, 360);
                                                }
                                                if (t.use_item_kind == 22) {
                                                    fontPrintStrNum(msg_buffer, 218, 45, 360);
                                                }
                                                if (t.use_item_kind == 30) {
                                                    fontPrintStrNum(msg_buffer, 210, 45, 360);
                                                }
                                                if (t.use_item_kind == 40) {
                                                    fontPrintStrNum(msg_buffer, 211, 45, 360);
                                                }
                                                if (t.use_item_kind == 63) {
                                                    fontPrintStrNum(msg_buffer, 212, 45, 360);
                                                }
                                                if (t.use_item_kind == 64) {
                                                    fontPrintStrNum(msg_buffer, 213, 45, 360);
                                                }
                                                if (t.use_item_kind == 65) {
                                                    fontPrintStrNum(msg_buffer, 214, 45, 360);
                                                }
                                                if (t.use_item_kind == 71) {
                                                    fontPrintStrNum(msg_buffer, 215, 45, 360);
                                                }
                                                if (t.use_item_kind == 73) {
                                                    fontPrintStrNum(msg_buffer, 216, 45, 360);
                                                }
                                                if (t.use_item_kind == 74) {
                                                    fontPrintStrNum(msg_buffer, 217, 45, 360);
                                                }
                                            }
                                            if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel) || shPadTrigger(0, 0x800) ||
                                                shPadTrigger(0, 0x400) || t.analog[0] == 1 || t.analog[3] == 1 || t.prs_btn2) {
                                                t.use_item = 0;
                                            }
                                        } else {
                                            t.use_item = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if (item.equip && (t.step == 5 || t.step == 7)) {
            kage_font(msg_buffer, (item.equip - 1) * 2 + 20, 20, 325);
            kage_font(msg_buffer, (item.equip - 1) * 2 + 21, 45, 360);
        }
    }
    fontCrushOff();
    fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0xFF);
    if (!MemoCommandCheck()) {
        fontPrintStrNum(msg_buffer, 11, 391, 428);
    }
    fontSetColorDirect(f, f, f, 0xFF);
    fontPrintStrNum(msg_buffer, 9, 64, 428);
    fontPrintStrNum(msg_buffer, 10, 240, 428);
    if (MemoCommandCheck()) {
        fontPrintStrNum(msg_buffer, 11, 391, 428);
    }
    fontPrintStrNum(msg_buffer, 12, 69, 50);
    fontPrintStrNum(msg_buffer, 13, 205, 50);
    fontPrintStrNum(msg_buffer, 14, 368, 50);
    if (t.step == 6 || t.step == 7) {
        fontSetColorDirect(f, f, f, 0xFF);
    } else {
        fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0xFF);
    }
    if (t.step != 7 && t.step != 5) {
        i = command_kind(t.item_kind);
    } else {
        i = command_kind(item.equip);
    }
    if (t.shelf) {
        switch (i) {
        case 1:
            fontPrintStrNum(msg_buffer, 19, 360, 95);
            fontPrintStrNum(msg_buffer, 0, 360, 120);
            break;
        case 2:
            fontPrintStrNum(msg_buffer, 19, 360, 95);
            fontPrintStrNum(msg_buffer, 0, 360, 120);
            break;
        case 3:
            fontPrintStrNum(msg_buffer, 19, 360, 95);
            if (item.equip != t.item_kind && t.step != 7 && t.step != 5) {
                fontPrintStrNum(msg_buffer, 7, 360, 120);
            } else {
                fontPrintStrNum(msg_buffer, 8, 360, 120);
            }
            break;
        case 4:
            fontPrintStrNum(msg_buffer, 19, 360, 80);
            if (item.equip != t.item_kind && t.step != 7 && t.step != 5) {
                fontPrintStrNum(msg_buffer, 7, 360, 105);
            } else {
                fontPrintStrNum(msg_buffer, 8, 360, 105);
            }
            fontPrintStrNum(msg_buffer, 1, 360, 130);
            break;
        case 5:
            fontPrintStrNum(msg_buffer, 19, 360, 95);
            fontPrintStrNum(msg_buffer, 1, 360, 120);
            break;
        case 6:
            if (!t.command_light) {
                fontPrintStrNum(msg_buffer, 19, 350, 95);
                fontPrintStrNum(msg_buffer, 3, 350, 120);
            } else {
                fontPrintStrNum(msg_buffer, 3, 350, 80);
                fontPrintStrNum(msg_buffer, 4, 360, 105);
                fontPrintStrNum(msg_buffer, 5, 360, 130);
            }
            break;
        case 7:
            if (t.command_volume == 0) {
                fontPrintStrNum(msg_buffer, 19, 350, 80);
                fontPrintStrNum(msg_buffer, 3, 350, 105);
                fontPrintStrNum(msg_buffer, 2, 350, 130);
            } else {
                if (t.command_volume == 2) {
                    fontPrintStrNum(msg_buffer, 3, 350, 80);
                    fontPrintStrNum(msg_buffer, 4, 360, 105);
                    fontPrintStrNum(msg_buffer, 5, 360, 130);
                } else {
                    fontPrintStrNum(msg_buffer, 2, 350, 90);
                }
            }
            break;
        case 8:
            fontPrintStrNum(msg_buffer, 19, 360, 80);
            fontPrintStrNum(msg_buffer, 0, 360, 105);
            fontPrintStrNum(msg_buffer, 6, 360, 130);
            break;
        case 9:
            fontPrintStrNum(msg_buffer, 19, 360, 80);
            fontPrintStrNum(msg_buffer, 0, 360, 105);
            fontPrintStrNum(msg_buffer, 6, 360, 130);
            break;
        case 10:
            fontPrintStrNum(msg_buffer, 19, 360, 80);
            fontPrintStrNum(msg_buffer, 0, 360, 105);
            fontPrintStrNum(msg_buffer, 6, 360, 130);
            break;
        case 11:
            fontPrintStrNum(msg_buffer, 19, 360, 80);
            fontPrintStrNum(msg_buffer, 0, 360, 105);
            fontPrintStrNum(msg_buffer, 6, 360, 130);
            break;
        }
    } else {
        switch (i) {
        case 1:
            fontPrintStrNum(msg_buffer, 0, 360, 95);
            fontPrintStrNum(msg_buffer, 15, 360, 120);
            break;
        case 2:
            fontPrintStrNum(msg_buffer, 0, 360, 105);
            break;
        case 3:
            if (item.equip != t.item_kind && t.step != 7 && t.step != 5) {
                fontPrintStrNum(msg_buffer, 7, 360, 105);
            } else {
                fontPrintStrNum(msg_buffer, 8, 360, 105);
            }
            break;
        case 4:
            if (item.equip != t.item_kind && t.step != 7 && t.step != 5) {
                fontPrintStrNum(msg_buffer, 7, 360, 95);
            } else {
                fontPrintStrNum(msg_buffer, 8, 360, 95);
            }
            fontPrintStrNum(msg_buffer, 1, 360, 120);
            break;
        case 5:
            fontPrintStrNum(msg_buffer, 1, 360, 105);
            break;
        case 6:
            if (!t.command_light) {
                fontPrintStrNum(msg_buffer, 3, 350, 105);
            } else {
                fontPrintStrNum(msg_buffer, 3, 350, 80);
                fontPrintStrNum(msg_buffer, 4, 360, 105);
                fontPrintStrNum(msg_buffer, 5, 360, 130);
            }
            break;
        case 7:
            if (t.command_volume == 0) {
                fontPrintStrNum(msg_buffer, 3, 350, 95);
                fontPrintStrNum(msg_buffer, 2, 350, 120);
            } else {
                if (t.command_volume == 2) {
                    fontPrintStrNum(msg_buffer, 3, 350, 80);
                    fontPrintStrNum(msg_buffer, 4, 360, 105);
                    fontPrintStrNum(msg_buffer, 5, 360, 130);
                } else {
                    fontPrintStrNum(msg_buffer, 2, 350, 90);
                }
            }
            break;
        case 8:
            fontPrintStrNum(msg_buffer, 0, 360, 95);
            fontPrintStrNum(msg_buffer, 6, 360, 120);
            break;
        case 9:
            fontPrintStrNum(msg_buffer, 0, 360, 95);
            fontPrintStrNum(msg_buffer, 6, 360, 120);
            break;
        case 10:
            fontPrintStrNum(msg_buffer, 0, 360, 80);
            fontPrintStrNum(msg_buffer, 15, 360, 105);
            fontPrintStrNum(msg_buffer, 6, 360, 130);
            break;
        case 11:
            fontPrintStrNum(msg_buffer, 0, 360, 95);
            fontPrintStrNum(msg_buffer, 6, 360, 120);
            break;
        }
    }
    fontSetColorDirect(f, f, f, 0xFF);
    if (t.item_kind &&
        (command_kind(t.item_kind) == 2 || command_kind(t.item_kind) == 4 || command_kind(t.item_kind) == 5) &&
        !t.prs_btn2) {
        if (item.number[t.item_kind] < 10) {
            fontPrintDec(item.number[t.item_kind], 236, 289, 3, 0);
            stock_line(31, 0);
        } else {
            if (item.number[t.item_kind] < 100) {
                fontPrintDec(item.number[t.item_kind], 243, 289, 3, 0);
                stock_line(45, 0);
            } else {
                fontPrintDec(item.number[t.item_kind], 250, 289, 3, 0);
                stock_line(59, 0);
            }
        }
    }
    if (!t.prs_btn2) {
        if (t.item_kind == 15) {
            if (item.light_switch) {
                fontPrintStrNum(msg_buffer, 4, 264, 290);
            } else {
                fontPrintStrNum(msg_buffer, 5, 261, 290);
            }
            stock_line(50, 0);
        } else {
            if (t.item_kind == 16) {
                if (item.radio_switch) {
                    fontPrintStrNum(msg_buffer, 4, 264, 290);
                } else {
                    fontPrintStrNum(msg_buffer, 5, 261, 290);
                }
                stock_line(50, 0);
            }
        }
    }
    if (t.step != 12) {
        switch (item.equip) {
        case 4:
        case 6:
        case 8:
            if (item.number[item.equip] < 10) {
                fontPrintDec(item.number[item.equip], 262, 160, 3, 0);
                stock_line(31, 1);
            } else {
                fontPrintDec(item.number[item.equip], 269, 160, 3, 0);
                stock_line(45, 1);
            }
            break;
        }
    }
}

/** Prints a message with a drop shadow.
 * @param str message buffer
 * @param num message number
 * @param x x position
 * @param y y position */
void kage_font(unsigned short *str, unsigned short num, int x, int y) {
    float f;

    f = 128.0 * (1.0 - t.fade) * (1.0 - t.fade);
    fontSetColorDirect((unsigned char)f / 4, (unsigned char)f / 4, (unsigned char)f / 4, 0xFF);
    fontPrintStrNum(str, num, x + 2, y + 2);
    fontSetColorDirect(f, f, f, 0xFF);
    fontPrintStrNum(str, num, x, y);
}

/** Draws the frame of a box (4 corners), in one of two styles by box[0][3].
 * @param box the corners (xy, w: style) */
void lookline(float (*box)[4]) {
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0x7400000000008000, 0x4444410);
    if ((int)box[0][3] == 255) {
        PK_ADD(2);
    } else {
        PK_ADD(0x42);
    }
    PK_ADD(GS_SET_RGBAQ(0x30, 0x60, 0xF0, (int)box[0][3], 0));
    PK_ADD(GS_SET_XYZF(zs(box[0][0]), zs(box[0][1]), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(box[1][0]), zs(box[1][1]), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(box[2][0]), zs(box[2][1]), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(box[3][0]), zs(box[3][1]), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(box[0][0]), zs(box[0][1]), zs(0), 1));
    spkCloseGiftag();
    if ((int)box[0][3] == 255 && !t.prs_btn && !t.prs_btn2) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
        PK_ADD(0x30000);
        PK_ADD(0x47);
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        PK_ADD(2);
        PK_ADD(GS_SET_RGBAQ(0x30, 0x60, 0xF0, (int)box[0][3], 0));
        PK_ADD(GS_SET_XYZF(zs(box[1][0] - 6.0f), zs(box[1][1] - (box[1][1] - box[0][1]) / 2.0f), zs(0), 1));
        PK_ADD(GS_SET_XYZF(zs(box[0][0] - 6.0f), zs(box[0][1] - 6.0f), zs(0), 1));
        PK_ADD(GS_SET_XYZF(zs(box[3][0] - (box[3][0] - box[0][0]) / 2.0f), zs(box[3][1] - 6.0f), zs(0), 1));
        spkCloseGiftag();
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
        PK_ADD(0x30000);
        PK_ADD(0x47);
        spkCloseOpenDGiftag(0x5400000000008000, 0x44410);
        PK_ADD(2);
        PK_ADD(GS_SET_RGBAQ(0x30, 0x60, 0xF0, (int)box[0][3], 0));
        PK_ADD(GS_SET_XYZF(zs(box[1][0] + (box[3][0] - box[0][0]) / 2.0f), zs(5.0f + box[1][1]), zs(0), 1));
        PK_ADD(GS_SET_XYZF(zs(6.0f + box[2][0]), zs(5.0f + box[2][1]), zs(0), 1));
        PK_ADD(GS_SET_XYZF(zs(6.0f + box[3][0]), zs(box[3][1] + (box[1][1] - box[0][1]) / 2.0f), zs(0), 1));
        spkCloseGiftag();
    }
}

/** Draws the blinking marks around the selection (allay_time). */
void item_allay(void) {
    int rgb;

    if (t.allay_time > 350) {
        t.allay_time = 50;
    }
    rgb = t.allay_time < 200 ? t.allay_time : 400 - t.allay_time;
    t.allay_time += 30.0f * (6.0f * shGetDT());
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0x8400000000008000, 0x44444410);
    PK_ADD(0x43);
    PK_ADD(GS_SET_RGBAQ(rgb, rgb, rgb, (int)t.allay_abe, 0));
    PK_ADD(GS_SET_XYZF(zs(78), zs(-11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(78), zs(11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(95), zs(0), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(95), zs(-11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(95), zs(11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(112), zs(0), zs(0), 1));
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0x8400000000008000, 0x44444410);
    PK_ADD(0x43);
    PK_ADD(GS_SET_RGBAQ(rgb, rgb, rgb, (int)t.allay_abe, 0));
    PK_ADD(GS_SET_XYZF(zs(-78), zs(-11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-78), zs(11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-95), zs(0), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-95), zs(-11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-95), zs(11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-112), zs(0), zs(0), 1));
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
    PK_ADD(0x42);
    PK_ADD(GS_SET_RGBAQ(0x80, 0x80, 0x80, (int)t.allay_abe * 5, 0));
    PK_ADD(GS_SET_XYZF(zs(78), zs(-11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(78), zs(11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(95), zs(0), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(78), zs(-11), zs(0), 1));
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
    PK_ADD(0x42);
    PK_ADD(GS_SET_RGBAQ(0x80, 0x80, 0x80, (int)t.allay_abe * 5, 0));
    PK_ADD(GS_SET_XYZF(zs(95), zs(-11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(95), zs(11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(112), zs(0), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(95), zs(-11), zs(0), 1));
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
    PK_ADD(0x42);
    PK_ADD(GS_SET_RGBAQ(0x80, 0x80, 0x80, (int)t.allay_abe * 5, 0));
    PK_ADD(GS_SET_XYZF(zs(-78), zs(-11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-78), zs(11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-95), zs(0), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-78), zs(-11), zs(0), 1));
    spkCloseGiftag();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
    PK_ADD(0x42);
    PK_ADD(GS_SET_RGBAQ(0x80, 0x80, 0x80, (int)t.allay_abe * 5, 0));
    PK_ADD(GS_SET_XYZF(zs(-95), zs(-11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-95), zs(11), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-112), zs(0), zs(0), 1));
    PK_ADD(GS_SET_XYZF(zs(-95), zs(-11), zs(0), 1));
    spkCloseGiftag();
}

/** Draws the gradient bars of the menu background. */
void sprite(void) {
    int i;
    int j;
    int k;
    int rgb;

    for (i = -1; i < 2; i += 2) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 6; k++) {
                rgb = scolor - k * (scolor / 6.0f);
                spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
                PK_ADD(0x30000);
                PK_ADD(0x47);
                spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
                PK_ADD(6);
                if ((t.step == 5 && i == -1 && j == 1) || (t.step == 6 && i == -1 && j == 2) ||
                    (t.step == 7 && i == -1 && j == 2) || (t.step == 2 && i == 1 && j == 0) ||
                    (t.step == 4 && i == 1 && j == 1) || (t.step == 3 && i == 1 && j == 2)) {
                    if (t.sprite_time < 1.0f) {
                        PK_ADD(GS_SET_RGBAQ(t.sprite_time * (rgb * 4), rgb + t.sprite_time * (rgb * 3),
                                                rgb * 3 + rgb * t.sprite_time, 0x20, 0));
                    } else {
                        PK_ADD(GS_SET_RGBAQ((2.0f - t.sprite_time) * (rgb * 4),
                                                rgb + (2.0f - t.sprite_time) * (rgb * 3),
                                                rgb * 3 + rgb * (2.0f - t.sprite_time), 0x20, 0));
                    }
                } else {
                    PK_ADD(GS_SET_RGBAQ(0, rgb, rgb * 3, 0x20, 0));
                }
                PK_ADD(GS_SET_XYZF(zs(j * 160 - 230 + (float)(k * k / 10)), zs(k + i * 189), zs(0), 0xFF));
                PK_ADD(GS_SET_XYZF(zs(j * 160 - 90 - (float)(k * k / 10)), zs(k + i * 189 + 1), zs(0), 0xFF));
                PK_ADD(GS_SET_XYZF(zs(j * 160 - 230 + (float)(k * k / 10)), zs(i * 189 - k), zs(0), 0xFF));
                PK_ADD(GS_SET_XYZF(zs(j * 160 - 90 - (float)(k * k / 10)), zs(i * 189 - k - 1), zs(0), 0xFF));
                spkCloseGiftag();
            }
        }
    }
}

/** Draws the command list and its moving highlight.
 * @param gosa vertical offset of the list */
void look_command(int gosa) {
    int k;

    t.command_move = move_near(2.0f, 0.2f, 1000.0f * t.command_move, 1000.0f * t.command_cur);
    t.command_move /= 1000.0f;
    for (k = 0; k < 6; k++) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
        PK_ADD(0x30000);
        PK_ADD(0x47);
        spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
        PK_ADD(6);
        PK_ADD(GS_SET_RGBAQ(0, t.command_abe / 2 * (6 - k), t.command_abe / 4 * (6 - k), 0x20, 0));
        PK_ADD(GS_SET_XYZF(zs(90.0f + k * k / 10), zs(26.0f * t.command_move + gosa + k), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(230.0f - k * k / 10), zs(26.0f * t.command_move + gosa + k + 1.0f), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(90.0f + k * k / 10), zs(26.0f * t.command_move + gosa - k), zs(-10), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(230.0f - k * k / 10), zs(26.0f * t.command_move + gosa - k - 1.0f), zs(-10), 0xFF));
        spkCloseGiftag();
    }
}

/** Draws the radio volume gauge (16 steps). */
void look_volume(void) {
    int i;

    for (i = 0; i < 16; i++) {
        spkOpenDGiftag(0x4400000000008000, 0x4410, 0xFFFF0003, 0);
        PK_ADD(6);
        if (i <= item.radio_volume) {
            PK_ADD(0x2000B000);
        } else {
            PK_ADD(0x20003000);
        }
        PK_ADD(GS_SET_XYZF(zs(i * 7 + 105), zs(-120), zs(0), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(i * 7 + 109), zs(-100), zs(0), 0xFF));
        spkCloseGiftag();
    }
}

/** Draws James's condition display, a pulse whose speed and colour follow his HP. */
void look_hp(void) {
    int z;
    int i;
    int j;
    int j0;
    int j1;
    int r;
    int g;
    int b;
    float j_hp;
    float j_max;

    j_hp = sh2jms.player->battle.hp;
    j_max = sh2jms.player->battle.hp_max;
    t.hp_time1 -= 30.0f * shGetDT();
    t.hp_time2 += 30.0f * ((1.4f + (1.0f - j_hp / j_max) / 0.7f) * shGetDT());
    if (t.hp_time2 > -70.0f) {
        t.hp_time2 = -210.0f;
    }
    t.hp_abe -= 0.034f + (1.0f - j_hp / j_max) / 25.0f;
    if (t.hp_abe < 0.0f) {
        t.hp_abe = 1.0f;
        t.hp_time1 = t.hp_time2;
    }
    z = 2048.0f + t.hp_time2;
    t.hp_kodo += shGetDT() / 2.0f;
    if (t.hp_kodo > 0.2f + j_hp / j_max) {
        t.hp_kodo = 0.0f;
    }
    if (t.hp_kodo < 0.1f && j_hp < 0.8f * j_max) {
        t.hp_abe2 = 1.0f - 6.0f * t.hp_kodo;
    } else if (t.hp_kodo < 0.2f && j_hp < 0.8f * j_max) {
        t.hp_abe2 = 6.0f * t.hp_kodo - 0.2f;
    } else {
        t.hp_abe2 = 1.0f;
    }
    spkOpenDGiftag(0x1000000000008000, 0xE, 0x80000003, 0);
    PK_ADD(0);
    PK_ADD(0x3F);
    spkCloseOpenDGiftag(0x1000000000008000, 0xE);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0x7400000000008000, 0x4242160);
    PK_ADD(0x16);
    PK_ADD(0x598007400);
    if (j_hp < j_max / 2.0f) {
        PK_ADD(GS_SET_RGBAQ(160.0f * t.hp_abe2,
                                160.0f * t.hp_abe2 - t.hp_abe2 * (160.0f * (1.0f - 2.0f * j_hp / j_max)),
                                160.0f * t.hp_abe2 - t.hp_abe2 * (160.0f * (1.0f - 2.0f * j_hp / j_max)), 0xA0,
                                0x3F800000));
    } else {
        PK_ADD(GS_SET_RGBAQ(160.0f * t.hp_abe2, 160.0f * t.hp_abe2, 160.0f * t.hp_abe2, 0xA0, 0x3F800000));
    }
    PK_ADD(0);
    PK_ADD(GS_SET_XYZF(zs(-230), zs(-175), 0, 0xFF));
    PK_ADD(0x3F8000003F800000);
    PK_ADD(GS_SET_XYZF(zs(-90), zs(-70), 0, 0xFF));
    spkCloseGiftag();
    for (i = 0; i < 10.0f - 10.0f * (j_hp / j_max); i++) {
        srand(t.seed++);
        j = rrand(140, -230);
        j0 = rrand(105, -173);
        j1 = irand(105);
        if (irand(6) <= 0) {
            if (j + j1 > -90) {
                j1 = -90 - j;
            }
            spkOpenDGiftag(0x1000000000008000, 0xE, 0x80000004, 0);
            PK_ADD(0);
            PK_ADD(0x3F);
            spkCloseOpenDGiftag(0x1000000000008000, 0xE);
            PK_ADD(0x30000);
            PK_ADD(0x47);
            spkCloseOpenDGiftag(0x7400000000008000, 0x4242160);
            PK_ADD(0x16);
            PK_ADD(0x598007400);
            if (j_hp < j_max / 2.0f) {
                PK_ADD(GS_SET_RGBAQ(160.0f * t.hp_abe2,
                                        160.0f * t.hp_abe2 - t.hp_abe2 * (160.0f * (1.0f - 2.0f * j_hp / j_max)),
                                        160.0f * t.hp_abe2 - t.hp_abe2 * (160.0f * (1.0f - 2.0f * j_hp / j_max)),
                                        0xA0, 0x3F800000));
            } else {
                PK_ADD(GS_SET_RGBAQ(160.0f * t.hp_abe2, 160.0f * t.hp_abe2, 160.0f * t.hp_abe2, 0xA0,
                                        0x3F800000));
            }
            PK_ADD(GS_SET_ST(0x3E000000 + 0x1700000 * ((j + 230) / 140.0f),
                                 0x3E000000 + 0x1700000 * ((j0 + 175) / 105.0f)));
            PK_ADD(GS_SET_XYZF(zs(j), zs(j0 - 2), 0, 0xFF));
            PK_ADD(GS_SET_ST(0x3E000000 + 0x1700000 * ((j + j1 + 230) / 140.0f),
                                 0x3E100000 + 0x1700000 * ((j0 + 175) / 105.0f)));
            PK_ADD(GS_SET_XYZF(zs(j + j1), zs(j0), 0, 0xFF));
            spkCloseGiftag();
        }
    }
}

/** Draws the two lines of the stock (ammunition) display.
 * @param leftx spacing of the lines
 * @param hoge layout variant */
void stock_line(int leftx, int hoge) {
    int i;

    for (i = 0; i < 2; i++) {
        spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
        PK_ADD(0x30000);
        PK_ADD(0x47);
        spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
        PK_ADD(6);
        PK_ADD(0xFFA0A0A0);
        if (hoge == 0) {
            PK_ADD(GS_SET_XYZF(zs(50 - i * leftx), zs(50), zs(0), 0xFF));
        } else {
            PK_ADD(GS_SET_XYZF(zs(76 - i * leftx), zs(-80), zs(0), 0xFF));
        }
        if (hoge == 0) {
            PK_ADD(GS_SET_XYZF(zs(38 - i * leftx), zs(52), zs(0), 0xFF));
        } else {
            PK_ADD(GS_SET_XYZF(zs(64 - i * leftx), zs(-78), zs(0), 0xFF));
        }
        spkCloseGiftag();
    }
}

/*
 * Matching: fitted stand-in for float code (docs/stand-ins.md). With it look_combine loads the 0.4f
 * argument of the TgsItemPitureDraw calls before sign-extending the kind (lh into v1), as the
 * original does.
 */
static float __stripped_float_code_1(float x) { return x + 3.0f; }
/** Draws the pictures of the items picked for combining. */
void look_combine(void) {
    int i;

    if (t.combine[0]) {
        TgsItemPitureDraw(t.combine[0], 1920, 1072, 0x80, 100, 0.4);
    }
    if (t.combine[1]) {
        TgsItemPitureDraw(t.combine[1], 2720, 1072, 0x80, 100, 0.4);
    }
    if (t.combine[2]) {
        TgsItemPitureDraw(t.combine[2], 3520, 1072, 0x80, 100, 0.4);
    }
    if (t.combine[0]) {
        t.combine_abe += 4.0f * shGetDT();
    } else {
        t.combine_abe -= 4.0f * shGetDT();
    }
    if (t.combine_abe > 1.0f) {
        t.combine_abe = 1.0f;
    } else if (t.combine_abe < 0.0f) {
        t.combine_abe = 0.0f;
    }
    for (i = 0; i < 3; i++) {
        spkOpenDGiftag(0x4400000000008000, 0x4410, 0x80000003, 0);
        PK_ADD(0x46);
        PK_ADD(GS_SET_RGBAQ(0x10, 0x30, 0x10, 80.0f * t.combine_abe, 0));
        PK_ADD(GS_SET_XYZF(zs(i * 50 + 100), zs(45), zs(0), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(i * 50 + 140), zs(95), zs(0), 0xFF));
        spkCloseGiftag();
        spkOpenDGiftag(0x7400000000008000, 0x4444410, 0x80000003, 0);
        PK_ADD(0x42);
        PK_ADD(GS_SET_RGBAQ(0x40, 0x80, 0x40, 80.0f * t.combine_abe, 0));
        PK_ADD(GS_SET_XYZF(zs(i * 50 + 100), zs(45), zs(0), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(i * 50 + 100), zs(95), zs(0), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(i * 50 + 140), zs(95), zs(0), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(i * 50 + 140), zs(45), zs(0), 0xFF));
        PK_ADD(GS_SET_XYZF(zs(i * 50 + 100), zs(45), zs(0), 0xFF));
        spkCloseGiftag();
    }
}

/** Draws a quad from four points with alpha from the first (the motion trail of the menu boxes,
 * t.boxblur).
 * @param za0 corner 0 (w: alpha)
 * @param za1 corner 1
 * @param za2 corner 2
 * @param za3 corner 3 */
void look_zanzo(float *za0, float *za1, float *za2, float *za3) {
    spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0002, 0);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    spkCloseOpenDGiftag(0x6400000000008000, 0x444410);
    PK_ADD(0x44);
    if (t.step != 12 && t.step != 13) {
        PK_ADD(GS_SET_RGBAQ(0x30, 0x60, 0xF0, (int)za0[3], 0));
    } else {
        PK_ADD(GS_SET_RGBAQ(0x8C, 0x8C, 0x8C, (int)za0[3] * 2, 0));
    }
    PK_ADD(GS_SET_XYZF(zs(za0[0]), zs(za0[1]), zs(0), 0xFF));
    PK_ADD(GS_SET_XYZF(zs(za1[0]), zs(za1[1]), zs(0), 0xFF));
    PK_ADD(GS_SET_XYZF(zs(za2[0]), zs(za2[1]), zs(0), 0xFF));
    PK_ADD(GS_SET_XYZF(zs(za3[0]), zs(za3[1]), zs(0), 0xFF));
    spkCloseGiftag();
}

/** Draws the dark screen behind the menu as an 8x8 grid of quads.
 * @param num 0 or 1: which packet set-up (as in the two branches) */
void look_blackscr(int num) {
    int i;
    int j;

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            if (num == 0) {
                spkOpenDGiftag(0x1000000000008000, 0xE, 0xFFFF0001, 0);
                PK_ADD(0x30000);
                PK_ADD(0x47);
                spkCloseOpenDGiftag(0x4400000000008000, 0x4410);
            } else {
                spkOpenDGiftag(0x4400000000008000, 0x4410, 0x80000001, 0);
            }
            PK_ADD(6);
            PK_ADD(0x80000000);
            PK_ADD(GS_SET_XYZF(zs(i * 64 - 256), zs(j * 64 - 256), zs(-100), 0xFF));
            PK_ADD(GS_SET_XYZF(zs(i * 64 - 192), zs(j * 64 - 192), zs(-100), 0xFF));
            spkCloseGiftag();
        }
    }
}

/** Resets the fade level while no fade is running. */
void item_fade_in(void) {
    if (t.fade_flag == 0) {
        t.fade = 0.0f;
    }
}

/** Fades out when leaving the menu, then remembers the selected item and moves to the exit step. */
void item_fade_out(void) {
    if (t.fade_flag == 1) {
        ScreenEffectFadeStart(1, 0.5f);
        if (ScreenEffectFadeCheck()) {
            t.fade = 1.0f;
            t.fade_flag = 0;
            fontClear();
            item.last_cursor = t.item_kind;
            t.main_step = 4;
        }
    }
}

/** Finishes the fade into the examine view once the picture has loaded. */
void item_examine_fade_out(void) {
    if (t.fade_flag == 2) {
        if (ScreenEffectFadeCheck() && fsSync(1, -1) >= 0) {
            t.fade = 1.0f;
            t.fade_flag = 0;
            item.last_cursor = t.item_kind;
            t.main_step = 3;
            ScreenEffectFadeStart(4, 0.5f);
            ev_s_step = 0;
        }
    }
}
