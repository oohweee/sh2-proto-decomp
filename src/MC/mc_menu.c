/*
 * mc_menu.c: the memory card save/load menus (state machines stepped once per
 * frame by mcSaveMenu/mcLoadMenu) and their drawing.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "libc/stdio.h"

#define MC_MAXFILES 15

/* Next menu step. Matching: the do/while leaves the original's nop where a use ends a body. */
#define MC_STEP(s) do { mcw->menu_step = (s); mcw->menu_sstep = 0; } while (0)

static int mcCheckTimer(void);
static int mcMenuControl(void);
static int mcTellYesNo(void);
static int mcSelectData(void);
static int mcGetBlinkAlpha(void);
static int mcPutMes(short n, short x, short y, short align, short align2);
static void mcPutMes2(short n, short x, short y);
static void mcPutBigFont(short n, short y);
static void mcDrawFrame(void);
static void mcDrawSlot(void);
static void mcDrawPlace(short num, short y);
static void mcDrawAddInfo(struct MC_FILEINFO *fi, short y);
static void mcDrawWarning(short num);
static void mcDrawStatus(void);
static void mcDrawBG(void);
static void mcDrawBGWord(void);
static void mcDmaKick(void);
static void mcLoadMenuData(void);
static void mcSoundCursor(void);
static void mcSoundDecide(void);
static void mcSoundSelect(void);
static void mcSoundCancel(void);
static void mcSoundError(void);
static void mcSoundStart(void);

static char tag_curve1[5][2] = { { 3, 18 }, { 5, 10 }, { 8, 5 }, { 13, 1 }, { 16, 0 } };
static char tag_curve2[4][2] = { { 0, 8 }, { 1, 3 }, { 3, 1 }, { 8, 0 } };
char mc_cs_mes[7] = { 0x24, 0x23, 0x36, 0x41, 0x36, 0x23, 0x27 };

/*
 * The assert in mcSaveMenu bakes its line number (459) into the binary; the
 * original file had about 240 more lines above this point.
 */

/** Save menu, one step per call: packs the game state, reads the cards, then lets the player pick
 * a slot and saves.
 * @return non-zero once the menu has closed */
int mcSaveMenu(void) {
    int i;
    char port;
    char n;

    port = mcw->menu_port;
    if (!(mc.status & 4) || !Sh2sys.step[3]) {
        mcInit();
        mc.status |= 4;
    }
    switch (mcw->menu_step) {
    case 0:
        mcw->saveload = 0;
        mcLoadMenuData();
        SetSaveData();
        mcMakeSaveData();
        mcCheckAll();
        mc.status &= ~0x100;
        MC_STEP(1);
        Sh2sys.step[3]++;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        break;
    case 1:
        if (!(mc.status & 0x20)) {
            if (fsSync(1, mcw->fid[0]) >= 0) {
                mc.status |= 0x20;
            }
        } else if (fsSync(1, mcw->fid[1]) >= 0) {
            mc.status |= 0x40;
            ScreenEffectFadeStart(12, 1.5f);
            mc.status |= 8;
            MC_STEP(2);
            mcLoadIconData();
        }
        break;
    case 2:
        if (mcw->dirstatus[port][0] == 8 || mcw->dirstatus[port][0] == 0) {
            if (mcw->portstatus[port] == 2 || mcw->portstatus[port] == 3) {
                mcw->dirstatus[port][0] = 1;
                mcCheckDir(port);
            }
        }
        mcw->menu_info = -2;
        for (i = 0; i < 5; i++) {
            if (mcw->dirstatus[port][i] == 1) {
                mcw->menu_info = 27;
                break;
            }
        }
        if (mcw->menu_info == -2) {
            switch (mcSelectData()) {
            case 1:
                if (mcCheckEndLoadIconData()) {
                    if (mcw->portstatus[port] == 4) {
                        MC_STEP(3);
                        mcSoundDecide();
                    } else if (mcw->portstatus[port] != 2 && mcw->portstatus[port] != 3) {
                        mcSoundError();
                    } else if (mcw->menu_num[port] < mcw->filemax[port]) {
                        if ((mcw->fileinfo[mcw->menu_port][mcw->menu_num[port]].status & 0x80) ||
                            mcw->dirstatus[port][mcw->fileinfo[mcw->menu_port][mcw->menu_num[port]].dirid] == 6) {
                            mcSoundError();
                        } else {
                            MC_STEP(5);
                            mcSoundDecide();
                        }
                    } else {
                        n = mcCheckCanSave(port);
                        if (n < 2) {
                            MC_STEP(4);
                            mcSoundDecide();
                        } else {
                            mcSoundError();
                        }
                    }
                }
                break;
            case 2:
                mc.status |= 0x100;
                MC_STEP(-1);
                break;
            case 3:
                if (mcw->menu_num[port] < mcw->filemax[port]) {
                    MC_STEP(9);
                    mcSoundDecide();
                } else {
                    mcSoundError();
                }
                break;
            case 4:
            case 5:
            case 6:
                break;
            }
        }
        break;
    case 3:
        if (mcw->menu_sstep == 0) {
            mc.status |= 0x100;
            mcw->menu_yesno = 2;
            mcw->menu_info = 30;
            mcw->menu_sstep++;
        } else if (mcw->menu_sstep == 2) {
            if (mc.status & 2) {
                MC_STEP(8);
                break;
            }
            if (mcw->portstatus[port] == 2 || port == 2) {
                for (i = 0; i < 5; i++) {
                    mcw->dirstatus[port][i] = 2;
                }
                MC_STEP(4);
                break;
            }
            if (mcw->portstatus[port] <= 1) {
                break;
            }
            MC_STEP(8);
            break;
        }
        if (mcw->portstatus[port] != 4) {
            MC_STEP(2);
            mcw->menu_yesno = 0;
            break;
        }
        switch (mcTellYesNo()) {
        case 1:
            Sh2sys.soft_reset = 0;
            mcFormat(port);
            mcw->menu_info = 37;
            mcw->menu_sstep = 2;
            break;
        case 2:
            MC_STEP(2);
            break;
        }
        break;
    case 4:
        if (mcw->menu_sstep == 0) {
            Sh2sys.soft_reset = 0;
            if (mcw->menu_num[port] < mcw->filemax[port]) {
                mcSaveData(port, mcw->fileinfo[mcw->menu_port][mcw->menu_num[port]].dirid,
                           mcw->fileinfo[mcw->menu_port][mcw->menu_num[port]].fileid);
            } else {
                for (n = 0; n < MC_MAXFILES; n++) {
                    for (i = 0; i < mcw->filemax[port]; i++) {
                        if (mcw->fileinfo[mcw->menu_port][i].dirid == mcw->menu_newdir &&
                            mcw->fileinfo[mcw->menu_port][i].fileid == n) {
                            i = -1;
                            break;
                        }
                    }
                    if (i != -1) {
                        mcSaveData(port, mcw->menu_newdir, n);
                        break;
                    }
                }
                /* Matching: the assert bakes its original line number into the object. */
#line 459
                fjAssert(n < MC_MAXFILES);
            }
            mc.status |= 0x100;
            mcw->menu_info = 39;
            mcw->menu_bk = 1;
            mcw->menu_sstep++;
        }
        if (mc.status & 2) {
            MC_STEP(7);
        } else if (mc.status & 1) {
            MC_STEP(6);
        }
        break;
    case 5:
        if (mcCheckStatus(port)) {
            MC_STEP(2);
            mcw->menu_yesno = 0;
            break;
        }
        if (mcw->menu_sstep == 0) {
            mcw->menu_yesno = 2;
            mcw->menu_info = 46;
            mcw->menu_sstep++;
        }
        switch (mcTellYesNo()) {
        case 1:
            MC_STEP(4);
            break;
        case 2:
            MC_STEP(2);
            break;
        }
        break;
    case 6:
        if (mcCheckStatus(port)) {
            MC_STEP(2);
            break;
        }
        if (mcw->menu_sstep == 0) {
            Sh2sys.soft_reset = 1;
            mcw->menu_info = 40;
            mcw->menu_timer = 90;
            mcw->menu_sstep++;
        }
        if (mcCheckTimer()) {
            for (i = 0; i < 5; i++) {
                if (mcw->dirstatus[port][i] == 1) {
                    break;
                }
            }
            if (i == 5) {
                mcw->saveload = 1;
                mc.status &= ~0x100;
                MC_STEP(2);
            }
        }
        break;
    case 7:
        if (mcw->menu_sstep == 0) {
            mcSoundError();
            Sh2sys.soft_reset = 1;
            mcw->menu_info = 41;
            mcw->menu_timer = 90;
            mcw->menu_sstep++;
        }
        if (mcCheckTimer()) {
            mc.status &= ~0x100;
            MC_STEP(2);
        }
        break;
    case 8:
        if (mcw->menu_sstep == 0) {
            mcSoundError();
            Sh2sys.soft_reset = 1;
            mcw->menu_info = 38;
            mcw->menu_timer = 90;
            mcw->menu_sstep++;
        }
        if (mcCheckTimer()) {
            mc.status &= ~0x100;
            MC_STEP(2);
        }
        break;
    case 9:
        if (mcCheckStatus(port)) {
            MC_STEP(2);
            mcw->menu_yesno = 0;
            break;
        }
        if (mcw->menu_sstep == 0) {
            mcw->menu_yesno = 2;
            mcw->menu_info = 66;
            mcw->menu_sstep++;
        }
        switch (mcTellYesNo()) {
        case 1:
            MC_STEP(10);
            break;
        case 2:
            MC_STEP(2);
            break;
        }
        break;
    case 10:
        if (mcw->menu_sstep == 0) {
            Sh2sys.soft_reset = 0;
            mcDeleteData(port, mcw->fileinfo[mcw->menu_port][mcw->menu_num[port]].dirid,
                         mcw->fileinfo[mcw->menu_port][mcw->menu_num[port]].fileid);
            mc.status |= 0x100;
            mcw->menu_info = 67;
            mcw->menu_sstep++;
        }
        if (mc.status & 2) {
            MC_STEP(12);
        } else if (mc.status & 1) {
            if (--mcw->filemax[port] >= 4 && mcw->menu_base[port] >= mcw->filemax[port] - 4) {
                mcw->menu_base[port]--;
            }
            MC_STEP(11);
        }
        break;
    case 11:
        if (mcCheckStatus(port)) {
            MC_STEP(2);
            break;
        }
        if (mcw->menu_sstep == 0) {
            Sh2sys.soft_reset = 1;
            mcw->menu_info = 68;
            mcw->menu_timer = 90;
            mcw->menu_sstep++;
        }
        if (mcCheckTimer()) {
            mc.status &= ~0x100;
            mcw->saveload = 1;
            MC_STEP(2);
        }
        break;
    case 12:
        if (mcw->menu_sstep == 0) {
            mcSoundError();
            Sh2sys.soft_reset = 1;
            mcw->menu_info = 69;
            mcw->menu_timer = 90;
            mcw->menu_sstep++;
        }
        if (mcCheckTimer()) {
            mc.status &= ~0x100;
            MC_STEP(2);
        }
        break;
    case 13:
        if (mcw->menu_sstep == 0) {
            mcw->menu_info = 47;
            mcw->menu_sstep++;
        }
        switch (mcTellYesNo()) {
        case 1:
            mc.status |= 0x100;
            mcw->menu_info = -1;
            MC_STEP(-1);
            break;
        case 2:
            MC_STEP(2);
        }
        break;
    }
    mcExec();
    if (mcMenuControl()) {
        fontClear();
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
        mcStepInit();
        return 1;
    }
    mcDrawMenu();
    return 0;
}

/** Load menu, one step per call: reads the cards, lets the player pick a save and loads it.
 * @return non-zero once the menu has closed */
int mcLoadMenu(void) {
    int i;
    char port;
    char n;

    port = mcw->menu_port;
    if (!(mc.status & 4) || !Sh2sys.step[3]) {
        mcInit();
        mc.status |= 4;
    }
    switch (mcw->menu_step) {
    case 0:
        mcLoadMenuData();
        mcw->saveload = 2;
        mcCheckAll();
        mc.status &= ~0x100;
        ScreenEffectFadeStart(3, 0.0f);
        MC_STEP(1);
        Sh2sys.step[3]++;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        break;
    case 1:
        if (!(mc.status & 0x20)) {
            if (fsSync(1, mcw->fid[0]) >= 0) {
                mc.status |= 0x20;
            }
        } else if (fsSync(1, mcw->fid[1]) >= 0) {
            mc.status |= 0x40;
            ScreenEffectFadeStart(4, 1.5f);
            mc.status |= 8;
            MC_STEP(2);
        }
        break;
    case 2:
        if (mcw->dirstatus[port][0] == 8 || mcw->dirstatus[port][0] == 0) {
            if (mcw->portstatus[port] == 2 || mcw->portstatus[port] == 3) {
                mcw->dirstatus[port][0] = 1;
                mcCheckDir(port);
            }
        }
        mcw->menu_info = -2;
        for (i = 0; i < 5; i++) {
            if (mcw->dirstatus[port][i] == 1) {
                mcw->menu_info = 27;
                break;
            }
        }
        if (mcw->menu_info == -2) {
            switch (mcSelectData()) {
            case 1:
                n = mcw->menu_num[port];
                if ((mcw->portstatus[port] != 2 && mcw->portstatus[port] != 3) || !mcw->filemax[port]) {
                    mcSoundError();
                } else if ((mcw->fileinfo[mcw->menu_port][n].status & 0x80) ||
                           mcw->dirstatus[port][mcw->fileinfo[mcw->menu_port][n].dirid] == 6) {
                    mcSoundError();
                } else {
                    mcSoundDecide();
                    MC_STEP(3);
                }
                break;
            case 2:
                mc.status |= 0x100;
                MC_STEP(-1);
                break;
            case 3:
                mcSoundError();
                break;
            case 4:
            case 5:
            case 6:
                break;
            }
        }
        break;
    case 3:
        if (mcw->menu_sstep == 0) {
            Sh2sys.soft_reset = 0;
            mcLoadData(port, mcw->fileinfo[mcw->menu_port][mcw->menu_num[port]].dirid,
                       mcw->fileinfo[mcw->menu_port][mcw->menu_num[port]].fileid);
            mc.status |= 0x100;
            mcw->menu_info = 42;
            mcw->menu_sstep++;
        }
        if (mc.status & 2) {
            MC_STEP(5);
        } else if (mc.status & 1) {
            MC_STEP(4);
        }
        break;
    case 4:
        if (mcw->menu_sstep == 0) {
            mcSoundStart();
            Sh2sys.soft_reset = 1;
            mcw->menu_info = 43;
            mcw->menu_sstep++;
        } else {
            mcCodecAll();
            MC_STEP(-3);
        }
        break;
    case 5:
        if (mcw->menu_sstep == 0) {
            mcSoundError();
            Sh2sys.soft_reset = 1;
            mcw->menu_info = 44;
            mcw->menu_timer = 90;
            mcw->menu_sstep++;
        }
        if (mcCheckTimer()) {
            mc.status &= ~0x100;
            MC_STEP(2);
        }
        break;
    }
    mcExec();
    if (mcMenuControl()) {
        fontClear();
        Sh2sys.step[1] = 7;
        Sh2sys.step[2] = 0;
        Sh2sys.step[3] = 0;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        mcStepInit();
        return 1;
    }
    mcDrawMenu();
    if (mcw->menu_step == -3 && !(mc.status & 0x200)) {
        ExtGameData();
        mc.status |= 0x200;
        mcStepInit();
        Sh2sys.main_status |= 0x20;
        Sh2sys.step[1] = 13;
        Sh2sys.step[2] = 0;
        Sh2sys.step[3] = 0;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        Sh2sys.step[2] = 1;
        Sh2sys.step[3] = 0;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
    }
    return 0;
}

/** Waits for the fade after a load to finish, then resets the menu state.
 * @return 1 when done (or when there was no load) */
int mcAfterLoadMenu(void) {
    if (!(mc.status & 0x200)) {
        return 1;
    }
    if (ScreenEffectFadeCheck()) {
        mc.status &= ~0x200;
        mcStepInit();
        fontClear();
        Sh2sys.soft_reset = 1;
        return 1;
    }
    return 0;
}

static int mcCheckTimer(void) {
    if ((mcw->menu_timer -= (char)shGetDF()) <= 0 || shPadPress(0, key_config.enter | key_config.cancel | 0xF00) ||
        shPadPress(0, 0x80) < 0x20 || shPadPress(0, 0x80) > 0xA0) {
        return 1;
    }
    return 0;
}

static int mcMenuControl(void) {
    if (mcw->menu_scroll < 0.0f) {
        if ((mcw->menu_scroll += 5.0f * shGetDT()) > 0.0f) {
            mcw->menu_scroll = 0.0f;
        }
    } else if (mcw->menu_scroll > 0.0f) {
        if ((mcw->menu_scroll -= 5.0f * shGetDT()) < 0.0f) {
            mcw->menu_scroll = 0.0f;
        }
    }
    mcw->menu_count += shGetDT();
    if (mcw->menu_count > 1.0f) {
        if (mc.status & 0x100) {
            mcw->menu_count = 1.0f;
        } else {
            mcw->menu_count -= 1.0f;
        }
    }
    if (mcw->menu_step == -2) {
        if (ScreenEffectFadeCheck()) {
            return 1;
        }
    } else if (mcw->menu_step < 0) {
        mc.status &= ~8;
        if (mcw->menu_step == -3) {
            ScreenEffectFadeStart(1, 3.0f);
        } else {
            ScreenEffectFadeStart(2, 2.0f);
            if (mcw->menu_step == -1) {
                mcw->menu_step = -2;
            }
        }
    }
    return 0;
}

static int mcTellYesNo(void) {
    int n;

    if (shPadRepeat(0, 0x200) || shPadRepeat(0, 0x100)) {
        mcSoundCursor();
        if (mcw->menu_yesno == 1) {
            mcw->menu_yesno = 2;
        } else {
            mcw->menu_yesno = 1;
        }
    }
    if (shPadTrigger(0, key_config.enter)) {
        n = mcw->menu_yesno;
        mcSoundDecide();
        mcw->menu_yesno = 0;
        return n;
    }
    if (shPadTrigger(0, key_config.cancel)) {
        mcSoundCancel();
        mcw->menu_yesno = 0;
        return 2;
    }
    return 0;
}

/* Matching: itof() (inline asm) compiles the function without the global optimizer, as the original's code
 * shows; the page-down `num = fmax` leaves an empty then-block, as in the original
 * (docs/matching-notes.md#mc_menu-mcselectdata). */
static int mcSelectData(void) {
    char port;
    short fmax;
    short num;
    short base;
    int an;

    port = mcw->menu_port;
    fmax = mcw->filemax[port] + (mcw->saveload < 2) - 1;
    num = mcw->menu_num[port];
    base = mcw->menu_base[port];
    if (mcw->menu_info == 27) {
        if (shPadTrigger(0, key_config.cancel)) {
            mcSoundCancel();
            return 2;
        }
        if (shPadTrigger(0, 0x10000) && port != 0) {
            mcw->menu_port = 0;
            mcw->menu_scroll = 0.0f;
            mcSoundSelect();
            return 4;
        }
        if (shPadTrigger(0, 0x20000) && port != 1) {
            mcw->menu_port = 1;
            mcw->menu_scroll = 0.0f;
            mcSoundSelect();
            return 5;
        }
        return 0;
    }
    if (num < base) {
        num = base;
    }
    if (fmax >= 4 && num >= fmax - 1 && base != fmax - 4) {
        base = fmax - 4;
    }
    if (num == 0 && fmax != 0 && shPadTrigger(0, 0x400)) {
        mcSoundCursor();
        num = fmax;
        base = fmax - 4;
        if (base < 0) {
            base = 0;
        }
        mcw->menu_scroll = -1.0f;
    } else if (num > 0 && shPadRepeat(0, 0x400)) {
        mcSoundCursor();
        num--;
        if (num <= base && base > 0) {
            base--;
        }
        mcw->menu_scroll = 1.0f;
    }
    if (num == fmax && fmax != 0 && shPadTrigger(0, 0x800)) {
        mcSoundCursor();
        num = base = 0;
        mcw->menu_scroll = 1.0f;
    } else if (num < fmax && shPadRepeat(0, 0x800)) {
        mcSoundCursor();
        num++;
        if (num >= base + 4 && base < fmax - 4) {
            base++;
        }
        mcw->menu_scroll = -1.0f;
    }
    if (num > 0 && shPadRepeat(0, 0x200)) {
        mcSoundCursor();
        if (num == fmax) {
            num -= 5;
            if (num <= 0) {
                num = base = 0;
            } else {
                base = num - 3;
                if (base < 0) {
                    base = 0;
                }
            }
        } else {
            base -= 5;
            num -= 5;
            if (num < 0) {
                num = base = 0;
            } else if (base < 0) {
                base = 0;
            }
        }
        mcw->menu_scroll = 1.0f;
    }
    if (num < fmax && shPadRepeat(0, 0x100)) {
        mcSoundCursor();
        if (num == 0) {
            if (fmax < 5) {
                num = fmax;
            } else {
                base = 2;
                num = 5;
            }
        } else {
            base += 5;
            if (base >= fmax - 4) {
                num = fmax;
                if (fmax < 5) {
                    base = 0;
                } else {
                    base = fmax - 4;
                }
            } else {
                num += 5;
            }
        }
        mcw->menu_scroll = -1.0f;
    }
    an = shPadPress(0, 0x80);
    if (an > 0x60 && an < 0xA0) {
        mcw->menu_an_count = 0.0f;
    } else if (an < 0x80) {
        if (mcw->menu_an_count == 0.0f) {
            mcw->menu_an_count = -1.000001f;
        } else {
            mcw->menu_an_count -= itof(0x60 - an) / 9.6f * shGetDT();
        }
    } else {
        if (mcw->menu_an_count == 0.0f) {
            mcw->menu_an_count = 1.000001f;
        } else {
            mcw->menu_an_count += itof(an - 0xA0) / 9.6f * shGetDT();
        }
    }
    if (num > 0 && mcw->menu_an_count < -1.0f) {
        mcw->menu_an_count += 1.0f;
        mcSoundCursor();
        num--;
        if (num <= base && base > 0) {
            base--;
        }
        mcw->menu_scroll = 1.0f;
    }
    if (num < fmax && mcw->menu_an_count > 1.0f) {
        mcw->menu_an_count -= 1.0f;
        mcSoundCursor();
        num++;
        if (num >= base + 4 && base < fmax - 4) {
            base++;
        }
        mcw->menu_scroll = -1.0f;
    }
    mcw->menu_num[port] = num;
    mcw->menu_base[port] = base;
    if (shPadRepeat(0, key_config.light)) {
        if (++mcw->menu_infotype >= ((mc.status & 0x10) ? 3 : 2)) {
            mcw->menu_infotype = 0;
        }
    }
    if (shPadTrigger(0, key_config.enter)) {
        return 1;
    }
    if (shPadTrigger(0, key_config.cancel)) {
        mcSoundCancel();
        return 2;
    }
    if (shPadTrigger(0, 8)) {
        return 3;
    }
    if (shPadTrigger(0, 0x10000) && port != 0) {
        mcw->menu_port = 0;
        mcw->menu_scroll = 0.0f;
        mcSoundSelect();
        return 4;
    }
    if (shPadTrigger(0, 0x20000) && port != 1) {
        mcw->menu_port = 1;
        mcw->menu_scroll = 0.0f;
        mcSoundSelect();
        return 5;
    }
    return 0;
}

static int mcGetBlinkAlpha(void) {
    int a;

    if (mcw->menu_count > 0.5f) {
        a = ftoi(32.0f * ((1.0f - mcw->menu_count) / 0.5f));
    } else {
        a = ftoi(32.0f * (mcw->menu_count / 0.5f));
    }
    return a;
}

static int mcPutMes(short n, short x, short y, short align, short align2) {
    if (mc.status & 0x20) {
        return fontPrintWord(fontGetMesAdr(msg_buffer, n), x, y, align, align2);
    }
    return 0;
}

static void mcPutMes2(short n, short x, short y) {
    if (mc.status & 0x20) {
        fontAllCenterOn();
        fontAllCenter2On();
        fontPrintStr(fontGetMesAdr(msg_buffer, n), x, y);
        fontAllCenterOff();
        fontAllCenter2Off();
    }
}

static void mcPutBigFont(short n, short y) {
    unsigned int bak;

    if (mc.status & 0x20) {
        bak = font.flag;
        font.flag = 0x100;
        fontPrintStrWide(fontGetMesAdr(msg_buffer, n), 0x100, y, 0xB4, 0xB4);
        font.flag = bak;
    }
}

/** Draws the save/load menu (background, frame, slots, status) while it is shown. */
void mcDrawMenu(void) {
    fontClear();
    if (mc.status & 0x40) {
        fontShadowOff();
        mcDrawBG();
        if (!playing.language) {
            fontCrushOn();
            mcDrawBGWord();
            fontCrushOff();
        }
        mcDrawBGWord();
        mcDmaKick();
        fontShadowOff();
        mcDrawFrame();
        mcDrawSlot();
        mcDrawStatus();
        d1cSend(spkDmaKick());
    }
}

/* Packet word append. Matching: an inline function, not a macro, in the original (see
 * mcDrawFrame's register use). */
static inline void PK_ADD(unsigned long v) {
    *spack.pos++ = v;
}
#define PK_XY(x, y) (((x) << 4) | ((long)(y) << 20))

static void mcDrawFrame(void) {
    int i;
    int d;

    d = mcw->menu_port == 0 ? -1 : 1;
    spkOpenDGiftag(0x2400000000000000, 0x10, 0x80000002, 0);
    PK_ADD(0x44);
    PK_ADD(0x40000030);
    spkCloseOpenDGiftag(0x1400000000000000, 5);
    for (i = 4; i >= 0; i--) {
        PK_ADD(PK_XY(0x800 - d * tag_curve1[i][0], tag_curve1[i][1] + 0x73C));
        PK_ADD(PK_XY(0x800 - d * (0xEB - tag_curve1[i][0]), tag_curve1[i][1] + 0x73C));
    }
    PK_ADD(PK_XY(0x800, 0x764));
    PK_ADD(PK_XY(0x800 - d * 0xEB, 0x764));
    spkCloseOpenDGiftag(0x8400000000000000, 0x10555DD0);
    PK_ADD(0x45);
    PK_ADD(PK_XY(0x800 - d * 0xEB, 0x764));
    for (i = 3; i >= 0; i--) {
        PK_ADD(PK_XY(0x800 - d * (0xEB - tag_curve2[i][0]), tag_curve2[i][1] + 0x764));
    }
    PK_ADD(0x42);
    PK_ADD(0x60000000);
    spkCloseOpenDGiftag(0x1400000000000000, 5);
    PK_ADD(PK_XY(d * 0xEB + 0x800, 0x764));
    for (i = 0; i < 5; i++) {
        PK_ADD(PK_XY(d * (0xEB - tag_curve1[i][0]) + 0x800, tag_curve1[i][1] + 0x73C));
    }
    for (i = 4; i >= 0; i--) {
        PK_ADD(PK_XY(d * tag_curve1[i][0] + 0x800, tag_curve1[i][1] + 0x73C));
    }
    PK_ADD(PK_XY(0x800, 0x764));
    for (i = 3; i >= 0; i--) {
        PK_ADD(PK_XY(0x800 - d * (0xEB - tag_curve2[i][0]), tag_curve2[i][1] + 0x764));
    }
    for (i = 0; i < 4; i++) {
        PK_ADD(PK_XY(0x800 - d * (0xEB - tag_curve2[i][0]), 0x872 - tag_curve2[i][1]));
    }
    for (i = 3; i >= 0; i--) {
        PK_ADD(PK_XY(d * (0xEB - tag_curve2[i][0]) + 0x800, 0x872 - tag_curve2[i][1]));
    }
    PK_ADD(PK_XY(d * 0xEB + 0x800, 0x764));
    spkCloseGiftag();
    fontCrushOn();
    if (mcw->menu_port == 0) {
        fontSetColorDirect(0x80, 0x80, 0x60, 0x50);
    } else {
        fontSetColorDirect(0, 0, 0, 0x30);
    }
    mcPutMes(0x30, 0x8B, 0x50, 1, 1);
    if (mcw->menu_port == 1) {
        fontSetColorDirect(0x80, 0x80, 0x60, 0x50);
    } else {
        fontSetColorDirect(0, 0, 0, 0x30);
    }
    mcPutMes(0x31, 0x175, 0x50, 1, 1);
    fontCrushOff();
}

/* Matching: A fitted double-code stand-in precedes mcDrawSlot (its inner i and mcw land in a2/a3). */
STRIPPED_DOUBLE_CODE()

/* Matching: the final clamp's then-block held a statement the optimizer removed (the original branches
 * around it); `n = fmax` is reconstructed, not recovered (docs/matching-notes.md#mc_menu-mcdrawslot). */
static void mcDrawSlot(void) {
    char port;
    int fmax;
    int i;
    int n;
    int s;

    port = mcw->menu_port;
    fmax = mcw->filemax[port];
    if (mcw->menu_info == 27) {
        mcDrawWarning(27);
        return;
    }
    switch (mcw->portstatus[port]) {
    case 0:
    case 1:
        if (mcw->saveload < 2 && mcw->menu_step == 3 && mcw->menu_sstep == 2) {
            mcDrawWarning(37);
        }
        return;
    case 5:
        mcw->filemax[port] = 0;
        mcw->menu_base[port] = 0;
        mcw->menu_num[port] = 0;
        mcDrawWarning(28);
        return;
    case 4:
        mcDrawWarning(29);
        return;
    case 6:
    case 7:
        mcDrawWarning(31);
        return;
    }
    if (mcw->saveload >= 2 && fmax == 0) {
        mcDrawWarning(33);
        return;
    }
    fontSetColorDirect(0x80, 0x80, 0x60, 0x80);
    n = 0;
    if (mcw->menu_num[port] < fmax) {
        n = mcw->fileinfo[mcw->menu_port][mcw->menu_num[port]].dirid + 1;
    } else {
        int i;

        for (i = 0; i < 5; i++) {
            if (mcw->dirstatus[port][i] == 3 || mcw->dirstatus[port][i] == 4) {
                n = i + 1;
                mcw->menu_newdir = i;
                break;
            }
        }
        if (n == 0) {
            for (i = 0; i < 5; i++) {
                if (mcw->dirstatus[port][i] == 2) {
                    n = i + 1;
                    mcw->menu_newdir = i;
                    break;
                }
            }
        }
    }
    if (n > 0 && n < 6) {
        font.mes_v[0][0] = n + 0x10;
        font.mes_v[0][1] = 0xFFFF;
        if (mc.status & 0x20) {
            fontPrintStr(fontGetMesAdr(msg_buffer, 0x40), 0x24, 0x6E);
        }
    }
    n = mcw->menu_base[port];
    for (i = 0; i < 5; i++, n++) {
        if (n > fmax) {
            break;
        }
        fontSetColorDirect(0x80, 0x80, 0x80, 0x80);
        if (n == fmax) {
            if (mcw->saveload >= 2) {
                break;
            }
            if (mcw->menu_step == 6) {
                if (!mcw->menu_bk) {
                    mcw->menu_bk = 1;
                }
                s = mcw->menu_bk;
            } else if (mcw->menu_step != 4) {
                s = mcCheckCanSave(port);
                if (s == 5 && fmax == 0) {
                    return;
                }
                mcw->menu_bk = s;
            } else if (mcw->menu_num[port] == n) {
                s = 6;
            } else {
                s = mcw->menu_bk;
            }
            mcDrawPlace(mc_cs_mes[s], i * 40 + 150);
            break;
        }
        if (mcw->fileinfo[mcw->menu_port][n].status & 0x80) {
            if (mcw->fileinfo[mcw->menu_port][n].dirid < 0 || mcw->fileinfo[mcw->menu_port][n].dirid >= 5 ||
                mcw->dirstatus[port][(int)mcw->fileinfo[mcw->menu_port][n].dirid] == 6) {
                mcDrawPlace(0x22, i * 40 + 150);
            } else {
                mcDrawPlace(0x2D, i * 40 + 150);
            }
        } else {
            mcDrawPlace(mcw->fileinfo[mcw->menu_port][n].scene, i * 40 + 150);
            mcDrawAddInfo(&mcw->fileinfo[mcw->menu_port][n], i * 40 + 150);
        }
    }
    n = mcw->menu_num[port] - mcw->menu_base[port];
    s = 40.0f * mcw->menu_scroll;
    spkOpenDGiftag(0x4400000000008000, 0x5D10, 0xFFFF0004, 0);
    PK_ADD(0x46);
    PK_ADD(((mcGetBlinkAlpha() + 0x20) << 24) | 0x208080);
    PK_ADD(PK_XY(0x724, n * 40 + 0x796 + s));
    PK_ADD(PK_XY(0x8DC, n * 40 + 0x7BD + s));
    spkCloseGiftag();
    if (mcw->menu_base[port] == 0) {
        spkOpenDGiftag(0x8400000000008000, 0x5DD5DD10, 0xFFFF0003, 0);
        PK_ADD(3);
        PK_ADD(0x40000000);
        PK_ADD(PK_XY(0x725, 0x797));
        PK_ADD(PK_XY(0x72B, 0x797));
        PK_ADD(PK_XY(0x725, 0x79D));
        PK_ADD(PK_XY(0x8DB, 0x797));
        PK_ADD(PK_XY(0x8D5, 0x797));
        PK_ADD(PK_XY(0x8DB, 0x79D));
        spkCloseGiftag();
    }
    if (mcw->saveload < 2) {
        fmax++;
    }
    if (mcw->menu_base[port] >= fmax - 5) {
        if (fmax < 5) { n = fmax; } else { fmax = 5; }
        spkOpenDGiftag(0x8400000000008000, 0x5DD5DD10, 0xFFFF0003, 0);
        PK_ADD(3);
        PK_ADD(0x40000000);
        PK_ADD(PK_XY(0x725, fmax * 40 + 0x795));
        PK_ADD(PK_XY(0x72B, fmax * 40 + 0x795));
        PK_ADD(PK_XY(0x725, fmax * 40 + 0x78F));
        PK_ADD(PK_XY(0x8DB, fmax * 40 + 0x795));
        PK_ADD(PK_XY(0x8D5, fmax * 40 + 0x795));
        PK_ADD(PK_XY(0x8DB, fmax * 40 + 0x78F));
        spkCloseGiftag();
    }
}

static void mcDrawPlace(short num, short y) {
    spkOpenDGiftag(0x7400000000008000, 0x5555D10, 0xFFFF0003, 0);
    PK_ADD(2);
    PK_ADD(0x40000000);
    PK_ADD(PK_XY(0x724, y + 0x700));
    PK_ADD(PK_XY(0x8DC, y + 0x700));
    PK_ADD(PK_XY(0x8DC, y + 0x727));
    PK_ADD(PK_XY(0x724, y + 0x727));
    PK_ADD(PK_XY(0x724, y + 0x700));
    spkCloseGiftag();
    mcPutMes(num, 0x2C, y + 4, 0, 0);
}

static void mcDrawAddInfo(struct MC_FILEINFO *fi, short y) {
    switch (mcw->menu_infotype) {
    case 0: {
        char buf[9];
        unsigned int itime;

        itime = ftoi(fi->time) / 60;
        if (itime > 599999) {
            itime = 599999;
        }
        sprintf(buf, "%4d:%02d", itime / 60, itime % 60);
        fontPrintStr(dicSetStr(buf), 0x13C, y + 4);
        break;
    }
    case 1: {
        char buf[9];
        unsigned int itime;

        itime = ftoi(fi->total_time) / 60;
        if (itime > 599999) {
            itime = 599999;
        }
        sprintf(buf, "%4d:%02d", itime / 60, itime % 60);
        fontSetColorDirect(0x50, 0x50, 0x10, 0x80);
        fontPrintStr(dicSetStr(buf), 0x13C, y + 4);
        break;
    }
    case 2: {
        unsigned short buf[8];
        unsigned short data[7] = { 0x139, 0x140, 0x11D, 0x13E, 0x137, 0x13C, 0xE7 };
        int i;
        int n;

        n = 0;
        for (i = 0; i < 7; i++) {
            if (fi->status & (1 << i)) {
                buf[n] = data[i];
                n++;
            }
        }
        if (n) {
            buf[n] = 0xFFFF;
            fontPrintStr(buf, 0x1D4 - n * 22, y + 4);
        }
        break;
    }
    }
}

static void mcDrawWarning(short num) {
    fontSetColorDirect(0x80, 0x80, 0x20, 0x80);
    mcPutMes(num, 0x100, 0xEB, 1, 1);
}

static void mcDrawStatus(void) {
    char buf[32];
    int n;
    int x;
    int y;

    if (mcw->menu_info == -2) {
        n = mcw->menu_num[mcw->menu_port];
        if (!(n < mcw->filemax[mcw->menu_port])) {
            if (mcw->saveload < 2) {
                fontSetColor(0);
                sprintf(buf, "%s%d", playing.language ? "\\h" : "", mcw->free[mcw->menu_port]);
                fontSetMes(0, dicSetStr(buf));
                switch (mcCheckCanSave(mcw->menu_port)) {
                case 0:
                    mcPutMes2(0x47, 0x100, 0x194);
                    mcPutMes2(0x48, 0x100, 0x1C0);
                    break;
                case 1:
                    mcPutMes2(0x46, 0x100, 0x194);
                    mcPutMes2(0x48, 0x100, 0x1C0);
                    break;
                }
            }
            return;
        }
        if (mcw->fileinfo[mcw->menu_port][n].status & 0x80) {
            if (mcw->saveload < 2) {
                fontSetColor(0);
                mcPutMes2(0x49, 0x100, 0x1C0);
            }
            if (mcw->dirstatus[mcw->menu_port][mcw->fileinfo[mcw->menu_port][n].dirid] != 6) {
                fontSetColor(10);
                fontWide(0x18, 0x24);
                mcPutMes(0x3C, 0x2E, 0x194, 0, 1);
                fontAllCenterOn();
                fontAllCenter2On();
                sprintf(buf, "\\h%d", mcw->fileinfo[mcw->menu_port][n].fileid + 1);
                fontPrintStr(dicSetStr(buf), 0x9C, 0x194);
                fontAllCenterOff();
                fontAllCenter2Off();
            }
            return;
        }
        fontSetColorDirect(0x60, 0x60, 0x60, 0x80);
        fontSetColor(10);
        fontWide(0x18, 0x24);
        mcPutMes(0x3C, 0x2E, 0x194, 0, 1);
        mcPutMes(0x3E, 0x10A, 0x194, 0, 1);
        mcPutMes(0x37, 0x2E, 0x1C0, 0, 1);
        mcPutMes(0x38, 0x10A, 0x1C0, 0, 1);
        fontAllCenterOn();
        fontAllCenter2On();
        sprintf(buf, "\\h%d", mcw->fileinfo[mcw->menu_port][n].fileid + 1);
        fontPrintStr(dicSetStr(buf), 0x9C, 0x194);
        sprintf(buf, "\\h%d", mcw->fileinfo[mcw->menu_port][n].savecount);
        fontPrintStr(dicSetStr(buf), 0x178, 0x194);
        if (mcw->fileinfo[mcw->menu_port][n].b_level > 0 && mcw->fileinfo[mcw->menu_port][n].b_level < 4) {
            mcPutMes(mcw->fileinfo[mcw->menu_port][n].b_level + 0x38, 0xBA, 0x1C0, 1, 1);
        }
        mcPutMes(mcw->fileinfo[mcw->menu_port][n].r_level + 0x39, 0x196, 0x1C0, 1, 1);
        fontAllCenterOff();
        fontAllCenter2Off();
        return;
    }
    if (mcw->menu_info == -1 || mcw->menu_info == 27) {
        return;
    }
    fontSetColorDirect(0x80, 0x80, 0x80, 0x80);
    mcPutMes(mcw->menu_info, 0x100, 0x194, 1, 1);
    if (mcw->menu_info == 37 || mcw->menu_info == 39 || mcw->menu_info == 42) {
        mcPutMes(0x3F, 0x100, 0x1C0, 1, 1);
        return;
    }
    if (mcw->menu_yesno) {
        if (mcw->menu_yesno == 1) {
            x = 200;
        } else {
            x = 312;
        }
        if (mcw->menu_info == 66) {
            y = 0x1CA;
        } else {
            y = 0x1C0;
        }
        fontSetYesNo(y - 14);
        spkOpenDGiftag(0x4400000000008000, 0x5D10, 0xFFFF0005, 0);
        PK_ADD(0x46);
        PK_ADD(((mcGetBlinkAlpha() + 0x20) << 24) | 0x208080);
        PK_ADD(PK_XY(x + 0x6D8, y + 0x6F2));
        PK_ADD(PK_XY(x + 0x728, y + 0x712));
        spkCloseGiftag();
    }
}

static void mcDrawBG(void) {
    int y;
    int base;

    base = sh2gfw_Get_BaseTBP0for2D();
    spkStartEnvLoadImage(0x10);
    for (y = 0; y < 0x200; y += 0x80) {
        spkSetEnvLoadImage(MemShare_gp_data_buf + y * 0x600, base, 8, 1, 0, y, 0x200, 0x80);
    }
    spkEndEnvLoadImage();
    spkOpenDGiftag(0x1000000000008000, 0xE, 0x80000001, 0x10);
    PK_ADD(0x44);
    PK_ADD(0x42);
    PK_ADD(0x30000);
    PK_ADD(0x47);
    PK_ADD(0);
    PK_ADD(0x14);
    PK_ADD(0x13A0001C0);
    PK_ADD(0x4E);
    PK_ADD(0);
    PK_ADD(0x3F);
    spkCloseOpenDGiftag(0x7400000000008000, 0x52D2160);
    PK_ADD(0x16);
    PK_ADD(base | 0x264120000);
    PK_ADD(0x3F80000080404040);
    PK_ADD(0);
    PK_ADD(0x70007000);
    PK_ADD(0x3F8000003F800000);
    PK_ADD(0x90009000);
    spkCloseGiftag();
}

static void mcDrawBGWord(void) {
    int i;
    int n;
    int y;

    if (!playing.language) {
        fontSetColorDirect(0x80, 0x80, 0x80, 0x10);
    } else {
        fontSetColorDirect(0x80, 0x80, 0x80, 0x1C);
    }
    y = 0xA6 - ftoi(3.0f * (180.0f * mcw->menu_scroll) / 2.0f);
    for (i = -2; i < 3; i++) {
        n = i + mcw->menu_num[mcw->menu_port];
        if (n >= 0 && n < mcw->filemax[mcw->menu_port] && !(mcw->fileinfo[mcw->menu_port][n].status & 0x80)) {
            mcPutBigFont(mcw->fileinfo[mcw->menu_port][n].scene, y + i * 270);
        }
    }
}

static void mcDmaKick(void) {
    d1cSend(spkDmaKick());
    d1cSend(fontTexLoad(sh2gfw_Get_BaseTBP0for2D(), 0x3600));
    d1cSend(fontFlush());
    spkResetOT2();
    fontClear();
}

static void mcLoadMenuData(void) {
    mc.status &= ~0x60;
    if ((mcw->fid[0] = DataLoadMessage(4)) == -2) {
        mc.status |= 0x20;
    }
    mcw->fid[1] = FcRead(data_menu_mc_savebg_raw, MemShare_gp_data_buf);
}

static void mcSoundCursor(void) {
    SeCall(10000, 1.0f, 0);
}

static void mcSoundDecide(void) {
    SeCall(10002, 1.0f, 0);
}

static void mcSoundSelect(void) {
    SeCall(10004, 1.0f, 0);
}

static void mcSoundCancel(void) {
    SeCall(10003, 1.0f, 0);
}

static void mcSoundError(void) {
    SeCall(10005, 1.0f, 0);
}

static void mcSoundStart(void) {
    SeCall(15002, 1.0f, 0);
}
