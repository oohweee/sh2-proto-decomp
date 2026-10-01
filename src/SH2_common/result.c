/*
 * This file has its own static font_print(), so it leaves out the prototype of the global
 * font_print(void) (font.c).
 */
#define SH2_LOCAL_font_print
#include "sh2.h"
#include "asm_helpers.h"
#include "libc/stdarg.h"
#include "libc/stdio.h"

/*
 * result.c: the results screen shown after the ending (rank, times, counts), run as a
 * step machine from the game state (Sh2sys.step[3]).
 */

#define PK_ADD(v) (*spack.pos++ = (v))

static unsigned short result_message[15] = {
    1, 2, 6, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
};

static void font_print(int num, int x, int y, int align) {
    fontPrintWord(fontGetMesAdr(msg_buffer, num), x, y, align, 0);
}

static void font_printf(char *str, int x, int y, int align, ...) {
    char buf[256];
    char *argp;

    va_start(argp, align);
    vsprintf(buf, str, argp);
    fontPrintWord(dicSetStr(buf), x, y, align, 0);
}

static void clear_screen(void) {
    spkOpenDGiftag(0x4400000000008000, 0x5D10, 0x80000000, 0);
    PK_ADD(6);
    PK_ADD(0x80000000);
    PK_ADD(0x70007000);
    PK_ADD(0x90009000);
    spkCloseGiftag();
}

static void color1(void) {
    fontSetColor(0);
}

static void color2(void) {
    fontSetColor(4);
}

/**
 * Per-frame step of the results screen: saves the totals, loads the result messages, and draws the
 * result pages until the player moves on. Without a battle level (no results) it goes straight to
 * the next game state.
 */
void ResultMain(void) {
    int i;
    int n;
    int t;
    unsigned short buf[11];

    clear_screen();
    fontClear();
    switch (Sh2sys.step[3]) {
    case 0:
        GameSavePreviousTotalRank();
        GameSaveSprayPower();
        if (!playing.battle_level) {
            Sh2sys.step[1] = 7;
            Sh2sys.step[2] = 0;
            Sh2sys.step[3] = 0;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
            break;
        }
        ScreenEffectFadeStart(3, 0.0f);
        DataLoadMessage(5);
        if ((playing.clear_end_kind & 0x10) && !playing.language && shPadPress(0, 1) && shPadPress(0, 2) &&
            shPadPress(0, 0x40) <= 0x5F && shPadPress(0, 0x80) > 0xA0 && shPadPress(0, 0x10) > 0xA0 &&
            shPadPress(0, 0x20) > 0xA0 && !shPadPress(0, 0xFFF0C)) {
            FcRead(data_menu_mc_result_msg_extra_mes, msg_buffer);
        }
        Sh2sys.step[3]++;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        break;
    case 1:
        if (fsSync(1, -1) >= 0) {
            ScreenEffectFadeStart(4, 1.0f);
            Sh2sys.step[3]++;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
        }
        return;
    case 3:
        if (ScreenEffectFadeCheck()) {
            Sh2sys.step[1] = 7;
            Sh2sys.step[2] = 0;
            Sh2sys.step[3] = 0;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
        }
        return;
    default:
        fontSetColor(7);
        font_print(0, 0x100, 0x20, 1);
        fontCrushOn();
        fontSetColor(10);
        for (i = 0; i < 15; i++) {
            font_print(result_message[i], 0xF0, i * 26 + 0x48, 3);
        }
        if (GameCalcRankBattleLevel() >= 5) {
            color2();
        } else {
            color1();
        }
        font_print(playing.battle_level + 2, 0x100, 0x48, 0);
        if (GameCalcRankRiddleLevel() >= 5) {
            color2();
        } else {
            color1();
        }
        font_print(playing.riddle_level + 3, 0x100, 0x62, 0);
        color1();
        n = playing.clear_end_kind;
        if (n & 0x10) {
            color2();
            n = 11;
        } else if (n & 0x8) {
            color2();
            n = 10;
        } else if (n & 0x4) {
            n = 9;
        } else if (n & 0x2) {
            n = 8;
        } else if (n & 0x1) {
            n = 7;
        }
        if (n) {
            font_print(n, 0x100, 0x7C, 0);
        }
        if (GameCalcRankEndingKind() >= 20) {
            color2();
        } else {
            color1();
        }
        if (n & 0x10) { /* @bug never true: n is 7-11 here, or has bits 0-4 clear */
            font_printf("\\h%d/4(+1)", 0x100, 0x96, 0, playing.clear_end_number - 1);
        } else {
            font_printf("\\h%d/4", 0x100, 0x96, 0, playing.clear_end_number);
        }
        if (GameCalcRankSaveCount() >= 5) {
            color2();
        } else {
            color1();
        }
        font_printf("\\h%d", 0x100, 0xB0, 0, playing.savecount);
        if (GameCalcRankClearTime() >= 10) {
            color2();
        } else {
            color1();
        }
        t = ftoi(playing.time);
        font_printf("\\h%dh %02dm %02ds", 0x100, 0xCA, 0, t / 3600, t / 60 % 60, t % 60);
        color1();
        font_printf("\\h%.2fkm", 0x100, 0xE4, 0, playing.walk_distance / 500.0f / 1000.0f);
        font_printf("\\h%.2fkm", 0x100, 0xFE, 0, playing.run_distance / 500.0f / 1000.0f);
        if (GameCalcRankItemGet() >= 10) {
            color2();
        } else {
            color1();
        }
        if (playing.hidden_item_get) {
            font_printf("\\h%d%s(+%d)", 0x100, 0x118, 0, playing.item_get,
                        GameCalcRankHiddenItemGet() >= 5 ? "\\c4" : "\\c0", playing.hidden_item_get);
        } else {
            font_printf("\\h%d", 0x100, 0x118, 0, playing.item_get);
        }
        if (GameCalcRankKillByShot() >= 15) {
            color2();
        } else {
            color1();
        }
        font_printf("\\h%d", 0x100, 0x132, 0, playing.kill_by_shot);
        if (GameCalcRankKillByFight() >= 15) {
            color2();
        } else {
            color1();
        }
        font_printf("\\h%d", 0x100, 0x14C, 0, playing.kill_by_fight);
        if (GameCalcRankBoatClearTime() >= 5) {
            color2();
        } else {
            color1();
        }
        t = ftoi(playing.boat_clear_time);
        if (t >= 3600) {
            font_printf("\\h%dh %02dm %02ds", 0x100, 0x166, 0, t / 3600, t / 60 % 60, t % 60);
        } else {
            font_printf("\\h%dm %02ds", 0x100, 0x166, 0, t / 60 % 60, t % 60);
        }
        color1();
        font_printf("\\h%.2fm/s", 0x100, 0x180, 0, playing.boat_max_speed / 500.0f);
        if (GameCalcRankJamesDamage() >= 5) {
            color2();
        } else {
            color1();
        }
        font_printf("\\h%d pt", 0x100, 0x19A, 0, ftoi(playing.jms_damage_total));
        fontSetColor(4);
        n = GameCalcRankTotal();
        if (n <= 9) {
            for (i = 0; i < n; i++) {
                buf[i] = 0x138;
            }
            buf[i] = 0xFFFF;
            font_printf((char *)buf, 0x100, 0x1B4, 0);
        } else {
            for (i = 0; i < n / 10; i++) {
                buf[i] = 0x138;
            }
            buf[i] = 0xFFFF;
            fontCrushOff();
            fontPrintWord(buf, 0x100, 0x1B0, 0, 0);
            fontCrushOn();
            if (n % 10) {
                for (i = 0; i < n % 10; i++) {
                    buf[i] = 0x138;
                }
                buf[i] = 0xFFFF;
                fontPrintWord(buf, 0x100, 0x1CA, 0, 0);
            }
        }
        fontCrushOff();
        d1cSend(spkDmaKick());
        if (shPadTrigger(0, key_config.enter | 4)) {
            ScreenEffectFadeStart(1, 1.0f);
            Sh2sys.step[3]++;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
        }
        break;
    }
}
