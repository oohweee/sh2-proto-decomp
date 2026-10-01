/*
 * logo.c: the boot sequence before the title: memory card check, the CESA and
 * SCE warning screens and the Konami logo.
 */

#include "sh2.h"
#include "asm_helpers.h"

/* "<file>:<line>> " prefix of the log messages. */
#define VB(s) __FILE__ ":" SH_STRINGIFY(__LINE__) "> " s

#define SH2SYS_STEP1(v) { Sh2sys.step[1] = (v); Sh2sys.step[2] = 0; Sh2sys.step[3] = 0; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_STEP2(v) { Sh2sys.step[2] = (v); Sh2sys.step[3] = 0; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_NEXT1() { Sh2sys.step[1]++; Sh2sys.step[2] = 0; Sh2sys.step[3] = 0; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_NEXT2() { Sh2sys.step[2]++; Sh2sys.step[3] = 0; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }
#define SH2SYS_NEXT3() { Sh2sys.step[3]++; Sh2sys.step[4] = 0; Sh2sys.step[5] = 0; Sh2sys.step[6] = 0; Sh2sys.step[7] = 0; }

static void logoDrawTexture(void);
static int logoTellYesNo(void);

int logoDispTimer;
struct PicDraw_Data Pic0;

/**
 * Boot step: loads the boot messages and checks the memory card, asking the player about a missing
 * card or save space as needed (a step machine on Sh2sys.step[2]).
 */
void logoCheckingMemcard(void) {
    static int msg_fid;
    static short msg_id;
    int sts;
    unsigned short *fontGetMesAdr(); /* Matching: an unprototyped block-scope declaration, as in the original. */

    switch (Sh2sys.step[2]) {
    case 0:
        msg_fid = DataLoadMessage(4);
        ScreenEffectInit();
        SH2SYS_NEXT2();
        break;
    case 1:
        if (fsSync(1, msg_fid) >= 0) {
            msg_id = 0;
            mcInit();
            SH2SYS_NEXT2();
        }
        break;
    case 2:
        sts = mcStartCheck();
        fontClear();
        if (msg_id == 0x4A || msg_id == 0x4B) {
            if ((unsigned int)(sts - 2) < 3) {
                fontPrintWord(fontGetMesAdr(msg_buffer, msg_id), 0x100, 0xF0, 1, 1);
            }
        } else if (msg_id == 0x2C) {
            fontSetColor(2);
            fontPrintWord(fontGetMesAdr(msg_buffer, msg_id), 0x100, 0xF0, 1, 1);
            if (--logoDispTimer <= 0 || shPadTrigger(0, key_config.enter)) {
                SH2SYS_STEP2(3);
                break;
            }
        } else if (msg_id != 0) {
            fontSetColor(2);
            fontPrintWord(fontGetMesAdr(msg_buffer, msg_id), 0x100, 0x70, 1, 0);
            if (sts == 0 && mcw->menu_yesno) {
                switch (logoTellYesNo()) {
                case 1:
                    SH2SYS_STEP2(3);
                    return;
                case 2:
                    mcStepInit();
                    break;
                }
            }
        }
        switch (sts) {
        case 1:
            SH2SYS_STEP2(3);
            break;
        case 2:
            msg_id = 0x4A;
            break;
        case 3:
            msg_id = 0x4B;
            break;
        case 4:
            fontPrintWord(fontGetMesAdr(msg_buffer, 0x4C), 0x100, 0x140, 1, 1);
            mcStepInit();
            ExtGameData();
            Sh2sys.main_status |= 0x20;
            SH2SYS_STEP2(4);
            break;
        case -2:
            msg_id = 0x33;
            mcw->menu_yesno = 2;
            break;
        case -1:
            msg_id = 0x34;
            mcw->menu_yesno = 2;
            break;
        case -3:
            msg_id = 0x2C;
            logoDispTimer = shGetFPS() * 3.0f;
            break;
        }
        break;
    case 3:
        if (Sh2sys.step[3] == 1) {
            fontClear();
            ScreenEffectFadeStart(1, 0.3f);
        }
        if (Sh2sys.step[3] < 2) {
            SH2SYS_NEXT3();
        } else if (ScreenEffectFadeCheck()) {
            SH2SYS_STEP1(5);
        }
        break;
    case 4:
        if (Sh2sys.step[3] == 1) {
            fontClear();
            SeCall(0x3A9A, 1.0f, 0);
            ScreenEffectFadeStart(1, 3.0f);
        }
        if (Sh2sys.step[3] < 2) {
            SH2SYS_NEXT3();
        } else if (ScreenEffectFadeCheck()) {
            SH2SYS_STEP1(0xD);
            SH2SYS_STEP2(1);
        }
        break;
    }
}

/** Boot step: shows the CESA rating screen with a fade in and out. */
void logoDrawWarningCESA(void) {
    static int fid;
    static int wait_loop;

    switch (Sh2sys.step[2]) {
    case 0:
        fontClear();
        ScreenEffectInit();
        ScreenEffectFadeStart(3, 0.0f);
        fid = FcRead(data_pic_etc_cesa_tex, get_gp_data_buf_addr());
        wait_loop = 0;
        SH2SYS_NEXT2();
        break;
    case 1:
        wait_loop++;
        switch (fsSync(1, fid)) {
        case 1:
            ScreenEffectFadeStart(4, 0.3f);
            logoDrawTexture();

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 388
            printf(VB("title.tex: read finished(%d.%02d)\n"), wait_loop / 60, wait_loop % 60 * 100 / 60);
            SH2SYS_NEXT2();
            break;
        case 0:

            printf(VB("!!! illegal fid=%d\n"), fid);
            break;
        case -1:
            if (wait_loop % 60 == 0) {
                printf(VB("title.tex: now reading(%d.)...\n"), wait_loop / 60);
            }
            break;
        case -2:
            if (wait_loop % 60 == 0) {
                printf(VB("title.tex: now waiting(%d.)...\n"), wait_loop / 60);
            }
            break;
        default:
            printf(VB("illegal return value\n"));
            break;
        }
        break;
    case 2:
        logoDrawTexture();
        if (ScreenEffectFadeCheck()) {
            logoDispTimer = shGetFPS() * 3.0f;
            SH2SYS_NEXT2();
        }
        break;
    case 3:
        logoDrawTexture();
        logoDispTimer--;
        if (logoDispTimer <= 0) {
            ScreenEffectFadeStart(1, 0.3f);
            SH2SYS_NEXT2();
        }
        break;
    case 4:
        logoDrawTexture();
        if (ScreenEffectFadeCheck()) {
            SH2SYS_NEXT1();
        }
        break;
    }
}

/** Boot step: shows the SCE screen with a fade in and out. */
void logoDrawWarningSCE(void) {
    static int fid;
    static int wait_loop;

    switch (Sh2sys.step[2]) {
    case 0:
        ScreenEffectFadeStart(3, 0.0f);
        fid = FcRead(data_pic_etc_sce_tex, get_gp_data_buf_addr());
        wait_loop = 0;
        SH2SYS_NEXT2();
        break;
    case 1:
        wait_loop++;
        switch (fsSync(1, fid)) {
        case 1:
            ScreenEffectFadeStart(4, 0.3f);
            logoDrawTexture();

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 494
            printf(VB("title.tex: read finished(%d.%02d)\n"), wait_loop / 60, wait_loop % 60 * 100 / 60);
            SH2SYS_NEXT2();
            break;
        case 0:

            printf(VB("!!! illegal fid=%d\n"), fid);
            break;
        case -1:
            if (wait_loop % 60 == 0) {
                printf(VB("title.tex: now reading(%d.)...\n"), wait_loop / 60);
            }
            break;
        case -2:
            if (wait_loop % 60 == 0) {
                printf(VB("title.tex: now waiting(%d.)...\n"), wait_loop / 60);
            }
            break;
        default:
            printf(VB("illegal return value\n"));
            break;
        }
        break;
    case 2:
        logoDrawTexture();
        if (ScreenEffectFadeCheck()) {
            logoDispTimer = shGetFPS() * 3.0f;
            SH2SYS_NEXT2();
        }
        break;
    case 3:
        logoDrawTexture();
        logoDispTimer--;
        if (logoDispTimer <= 0) {
            ScreenEffectFadeStart(2, 0.3f);
            SH2SYS_NEXT2();
        }
        break;
    case 4:
        logoDrawTexture();
        if (ScreenEffectFadeCheck()) {
            SH2SYS_NEXT1();
        }
        break;
    }
}

/** Boot step: shows the Konami logo with a fade in and out. */
void logoDrawKonamiLogo(void) {
    static int fid;
    static int wait_loop;

    switch (Sh2sys.step[2]) {
    case 0:
        ScreenEffectFadeStart(3, 0.0f);
        fid = FcRead(data_pic_etc_konami_r_tex, get_gp_data_buf_addr());
        wait_loop = 0;
        SH2SYS_NEXT2();
        break;
    case 1:
        wait_loop++;
        switch (fsSync(1, fid)) {
        case 1:
            ScreenEffectFadeStart(4, 0.3f);
            logoDrawTexture();

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 607
            printf(VB("title.tex: read finished(%d.%02d)\n"), wait_loop / 60, wait_loop % 60 * 100 / 60);
            SH2SYS_NEXT2();
            break;
        case 0:

            printf(VB("!!! illegal fid=%d\n"), fid);
            break;
        case -1:
            if (wait_loop % 60 == 0) {
                printf(VB("title.tex: now reading(%d.)...\n"), wait_loop / 60);
            }
            break;
        case -2:
            if (wait_loop % 60 == 0) {
                printf(VB("title.tex: now waiting(%d.)...\n"), wait_loop / 60);
            }
            break;
        default:
            printf(VB("illegal return value\n"));
            break;
        }
        break;
    case 2:
        logoDrawTexture();
        if (ScreenEffectFadeCheck()) {
            logoDispTimer = shGetFPS() * 3.0f;
            SH2SYS_NEXT2();
        }
        break;
    case 3:
        logoDrawTexture();
        logoDispTimer--;
        if (logoDispTimer <= 0) {
            ScreenEffectFadeStart(2, 0.3f);
            SH2SYS_NEXT2();
        }
        break;
    case 4:
        logoDrawTexture();
        if (ScreenEffectFadeCheck()) {
            SH2SYS_NEXT1();
        }
        break;
    }
}

static void logoDrawTexture(void) {
    void shQzero(); /* Matching: an unprototyped block-scope declaration, as in the original. */

    spkResetOT();
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(), 0, -1, -1);
    shQzero(&Pic0, sizeof(struct PicDraw_Data));
    Pic0.ap = (struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr();
    Pic0.clut = Pic0.tex = -1;
    Pic0.status |= 1;
    Pic0.otp = 0;
    PictureDraw(&Pic0);
    d1cSend(spkDmaKick());
}

/*
 * Matching: the inline-asm ftoi() (asm_helpers.h) also turns off MWCC's constant propagation
 * for the whole function (y stays in s1, mcw is reloaded), which is what the original shows.
 */
static int logoTellYesNo(void) {
    int x;
    int y;
    int a;
    int n;

    if ((mcw->menu_count += shGetDT()) > 1.0f) {
        mcw->menu_count -= 1.0f;
    }
    if (mcw->menu_count > 0.5f) {
        a = ftoi((1.0f - mcw->menu_count) / 0.5f * 32.0f);
    } else {
        a = ftoi(mcw->menu_count / 0.5f * 32.0f);
    }
    y = 0x182;
    fontSetColor(0);
    fontSetYesNo(y);
    if (shPadRepeat(0, 0x200) || shPadRepeat(0, 0x100)) {
        SeCall(10000, 1.0f, 0);
        if (mcw->menu_yesno == 1) {
            mcw->menu_yesno = 2;
        } else {
            mcw->menu_yesno = 1;
        }
    }
    if (mcw->menu_yesno == 1) {
        x = 200;
    } else {
        x = 312;
    }
    spkOpenDGiftag(0x4400000000008000, 0x5D10, 0xFFFF0005, 0);
    *spack.pos++ = 0x46;
    *spack.pos++ = ((a + 0x20) << 24) | 0x208080;
    *spack.pos++ = ((x + 0x6D8) << 4) | ((long)(y + 0x700) << 20);
    *spack.pos++ = ((x + 0x728) << 4) | ((long)(y + 0x720) << 20);
    spkCloseGiftag();
    d1cSend(spkDmaKick());
    if (shPadTrigger(0, key_config.enter)) {
        n = mcw->menu_yesno;
        SeCall(0x2712, 1.0f, 0);
        mcw->menu_yesno = 0;
        return n;
    }
    if (shPadTrigger(0, key_config.cancel)) {
        SeCall(0x2713, 1.0f, 0);
        mcw->menu_yesno = 0;
        return 2;
    }
    return 0;
}
