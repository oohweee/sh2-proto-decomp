/*
 * pause.c: the pause / disc-error overlay ("Paused", "Wrong disc", ...): a dark belt
 * across the screen and a blinking message.
 */

#include "sh2.h"

static unsigned char pause_type = 0;

/** Sets the kind of pause screen to `type`. Returns the previous kind. */
int PauseSetType(int type) {
    int ret;

    ret = pause_type;
    pause_type = type;
    return ret;
}

static void __DrawRect(int x0, int y0, int x1, int y1) {
    static union Q_WORDDATA KARI_OBI[16];
    int id;

    /* DMA cnt tag + DIRECT 10 */
    KARI_OBI[0].ui32[0] = 0x1000000A;
    KARI_OBI[0].ui32[1] = 0;
    KARI_OBI[0].ui32[2] = 0;
    KARI_OBI[0].ui32[3] = 0x5000000A;
    /* GIF tag: A+D, 1 reg */
    KARI_OBI[1].ui32[3] = 0;
    KARI_OBI[1].ui32[2] = 0xE;
    KARI_OBI[1].ui32[1] = 0x10000000;
    KARI_OBI[1].ui32[0] = 0x8001;
    /* TEST_1 */
    KARI_OBI[2].ul64[1] = 0x47;
    KARI_OBI[2].ul64[0] = 0x3001C;
    /* GIF tag: sprite, RGBAQ XYZ2 XYZ2 */
    KARI_OBI[3].ul64[0] = 0x3003400000008001;
    KARI_OBI[3].ul64[1] = 0x551;
    KARI_OBI[4].ui32[0] = 0;
    KARI_OBI[4].ui32[1] = 0;
    KARI_OBI[4].ui32[2] = 0;
    KARI_OBI[4].ui32[3] = 2;
    KARI_OBI[5].ul64[0] = (unsigned long)((x0 + 0x700) << 4) | ((unsigned long)((y0 + 0x700) << 4) << 32);
    KARI_OBI[5].ul64[1] = 0x10;
    KARI_OBI[6].ul64[0] = (unsigned long)((x1 + 0x900) << 4) | ((unsigned long)((y1 + 0x700) << 4) << 32);
    KARI_OBI[6].ul64[1] = 0x10;
    /* DMA end tag */
    KARI_OBI[7].ui32[0] = 0x70000000;
    KARI_OBI[7].ui32[1] = 0;
    KARI_OBI[7].ui32[2] = 0;
    KARI_OBI[7].ui32[3] = 0;
    d1cSend(KARI_OBI);
    d1sSync(0, -1);
}

static void *draw_rect(void *pkt, int x, int y, int w, int h, int z, unsigned int c) {
    __DrawRect(x, y, x + w, y + h);
    return pkt;
}

static void *draw_belt(void *pkt, int y, int h, int z, unsigned int c) {
    return draw_rect(pkt, 0, y, 0x200, h, z, c);
}

static u_long128 buffer[9];

/* blink: 0 -> 0x80 -> 0 over 128 frames */
#define BLINK_COLOR(c) \
    ccolor = (c) & 0xFF; \
    if (ccolor > 0x80) { \
        ccolor = 0xFF - ccolor; \
    }

/** Draws the pause / disc error overlay of the current kind. Returns non-zero while it is shown. */
int PauseDisp(void) {
    static char m_please_insert[37] = "\\c\\hPlease insert Silent Hill 2 disc";
    static char m_paused[11] = "\\c\\hPaused";
    static char m_wrong_disc[15] = "\\c\\hWrong disc";
    static char m_not_connect[29] = "\\c\\hController not connected";
    static char m_not_insert[22] = "\\c\\hDisc not inserted";
    static char m_tray_open[19] = "\\c\\hDisc tray open";
    static char m_now_loding[16] = "\\c\\hNow loading";
    static char m_please_wait[16] = "\\c\\hPlease wait";
    static char m_checking[18] = "\\c\\hChecking disc";
    static int count;
    static int force_pause;
    int ret;
    u_long128 *packet;
    u_long128 *pkt;
    int pz;
    int ccolor;
    int belt;
    char *mes;
    unsigned int bcolor;

    ret = 1;
    packet = buffer;
    count++;
    switch (pause_type) {
    case 0:
        count = 0;
        ret = 0;
        force_pause = 0;
        break;
    }
    if (ret) {
        /* pz, bcolor: names from the original's DWARF, values from the draw_belt() call */
        pz = 0x10;
        bcolor = 0x8FFFFFF;
        ccolor = 0;
        belt = 0;
        mes = NULL;
        sh2gfw_Set_PauseRetain();
        kari_drawloop_main_2dSYNC();
        switch (pause_type) {
        case 2:
            belt = 1;
            BLINK_COLOR(count * 2);
            mes = m_paused;
            break;
        case 3:
            belt = 1;
            BLINK_COLOR(count * 2);
            if ((count * 2) & 0x100) {
                mes = m_please_wait;
            } else {
                mes = m_now_loding;
            }
            break;
        case 4:
            belt = 1;
            break;
        case 5:
            force_pause = 1;
            BLINK_COLOR(count * 2);
            if ((count * 2) & 0x100) {
                mes = m_please_insert;
            } else {
                mes = m_tray_open;
            }
            break;
        case 6:
            force_pause = 1;
            BLINK_COLOR(count * 2);
            if ((count * 2) & 0x100) {
                mes = m_please_insert;
            } else {
                mes = m_not_insert;
            }
            break;
        case 7:
            force_pause = 1;
            BLINK_COLOR(count * 2);
            mes = m_checking;
            break;
        case 8:
            force_pause = 1;
            BLINK_COLOR(count * 2);
            if ((count * 2) & 0x100) {
                mes = m_please_insert;
            } else {
                mes = m_wrong_disc;
            }
            break;
        case 9:
            force_pause = 1;
            BLINK_COLOR(count * 2);
            if ((count * 2) & 0x100) {
                mes = m_paused;
            } else {
                mes = m_not_connect;
            }
            break;
        }
        if (force_pause) {
            belt = 1;
        }
        if (mes) {
            if (belt) {
                pkt = draw_belt(packet, 0x100, 0x20, pz, bcolor);
            }
            sh2gfw_InclimentLoopCounter(&shGs_AllEnv);
            fontSetColorDirect(ccolor, ccolor, ccolor, 0x80);
            fontPrintStr(dicSetStr(mes), 0x100, 0x100);
            fjFontDrawExecVif1();
            fontClear();
            sh2gfw_DeclimentLoopCounter(&shGs_AllEnv);
        }
    } else {
        sh2gfw_Reset_PauseRetain();
    }
    pause_type = 0;
    return ret;
}
