/*
 * pad.c: the game's pad input: reads both pads once per frame and answers
 * press / trigger / auto-repeat queries per key mask. Port 0 is the player's
 * pad; the second pad (debug) can be moved between ports 1-12 from the debug
 * switches.
 */

#include "sh2.h"
#include "lib/libShPad.h"

struct Pad_KeyConfig key_config;
static unsigned char pad[2][32];
static unsigned char pad_bak[2][20];
static float padf[2][20];
static int pad_x;
static int repeat[2];

/* max(a, b) with slt/movn through t7 (inline asm in the original). */
inline int imax(int a, int b) {
    asm { slt t7, a, b; movn a, b, t7 }
    return a;
}

/** Clears the pad state and applies the key configuration. */
void shPadInit(void) {
    shQzero(pad, sizeof(pad));
    shQzero(pad_bak, sizeof(pad_bak));
    shQzero(padf, sizeof(pad_bak)); /* @bug sizeof(pad_bak): clears 40 of padf's 160 bytes */
    pad_x = 1;
    key_config.item = 4;
    key_config.front_move = 0x400;
    key_config.back_move = 0x800;
    key_config.right_move = 0x20000;
    key_config.left_move = 0x10000;
    key_config.right_turn = 0x100;
    key_config.left_turn = 0x200;
    key_config.search_view = 0x40000;
    key_config.ready = 0x80000;
    key_config.pause = 8;
    key_config.map = 0x1000;
    key_config.skip = 4;
    key_config.enter = 0x4000;
    key_config.action = 0x4000;
    key_config.cancel = 0x8000;
    key_config.light = 0x2000;
    key_config.dash = 0x8000;
    key_config.attack = 0x4000;
    shPadSetGameKeyAssign();
}

/**
 * Per-frame pad update: reads and normalizes both pads and updates the auto-repeat timers (first
 * press, then every 0.1 s after 0.8 s).
 */
/* Matching: the `work = ...` statement is reconstructed: the line table and the DWARF show a dead read of
 * pad[i][2] and pad[i][3] into work, whose exact form is lost (docs/matching-notes.md#pad-shpadset). */
void shPadSet(void) {
    int work;
    int i;
    int j;

    for (i = 0; i < 20; i++) {
        pad_bak[0][i] = pad[0][i];
        pad_bak[1][i] = pad[1][i];
    }
    repeat[0] = repeat[1] = 0;
    libShPadRead(0, 0, (char *)pad[0]);
    libShPadRead(1, 0, (char *)pad[1]);
    shSysKeyNormalize((char *)pad[0]);
    shSysKeyNormalize((char *)pad[1]);
    for (i = 0; i < 2; i++) {
        work = pad[i][2] + pad[i][3];
        pad[i][0] = pad[i][20];
        pad[i][1] = pad[i][21];
        pad[i][2] = pad[i][22];
        pad[i][3] = pad[i][23];
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 20; j++) {
            if (pad[i][j]) {
                if (padf[i][j] == 0.0f) {
                    repeat[i] |= 1 << j;
                }
                padf[i][j] += shGetDT();
                if (padf[i][j] > 0.8f) {
                    repeat[i] |= 1 << j;
                    padf[i][j] -= 0.1f;
                }
            } else {
                padf[i][j] = 0.0f;
            }
        }
    }
    if (shPadTrigger(pad_x, 8)) {
        pad_x++;
        if (pad_x > 12) {
            pad_x = 1;
        }
        printf("Contoroler 2 port change : %d\n", pad_x);
    }
}

/**
 * Returns the port the debug pad answers on: 0 without debug flags, the selected port when the
 * debug pad port switch is on, otherwise 1.
 */
int shPadGetPort(void) {
    if (!dbFlag(-1)) {
        return 0;
    }
    if (dbSwitchSysPadPort()) {
        return pad_x;
    }
    return 1;
}

/** Returns the strongest pressure among the keys in `key` held on pad `port` (0 when none). */
int shPadPress(int port, int key) {
    int ret;
    int i;

    if (port) {
        if (port != 1 && !dbSwitchSysPadPort()) {
            return 0;
        }
        if (port != pad_x && dbSwitchSysPadPort()) {
            return 0;
        }
        port = 1;
    }
    ret = 0;
    for (i = 0; i < 20; i++) {
        if ((key >> i) & 1) {
            ret = imax(ret, pad[port][i]);
        }
    }
    return ret;
}

/** shPadPress() for the keys in `key` pressed this frame only. */
int shPadTrigger(int port, int key) {
    int ret;
    int i;

    if (port) {
        if (port != 1 && !dbSwitchSysPadPort()) {
            return 0;
        }
        if (port != pad_x && dbSwitchSysPadPort()) {
            return 0;
        }
        port = 1;
    }
    ret = 0;
    for (i = 0; i < 20; i++) {
        if ((key >> i) & 1) {
            if (!pad_bak[port][i]) {
                ret = imax(ret, pad[port][i]);
            }
        }
    }
    return ret;
}

/** shPadPress() for the keys in `key` that auto-repeat this frame. */
int shPadRepeat(int port, int key) {
    int ret;
    int i;

    if (port) {
        if (port != 1 && !dbSwitchSysPadPort()) {
            return 0;
        }
        if (port != pad_x && dbSwitchSysPadPort()) {
            return 0;
        }
        port = 1;
    }
    ret = 0;
    for (i = 0; i < 20; i++) {
        if ((key >> i) & 1) {
            if (key & repeat[port]) { /* @bug any key of `key` repeating counts, not key i */
                ret = imax(ret, pad[port][i]);
            }
        }
    }
    return ret;
}

static unsigned long kc2ga(unsigned long kconf_button) {
    unsigned long rem;
    unsigned long bit;
    unsigned long ret;

    ret = 0;
    bit = 1;
    rem = kconf_button;
    while (rem) {
        switch (bit & kconf_button) {
        case 0x100:
            ret |= 0x1;
            break;
        case 0x200:
            ret |= 0x2;
            break;
        case 0x400:
            ret |= 0x4;
            break;
        case 0x800:
            ret |= 0x8;
            break;
        case 0x1000:
            ret |= 0x10;
            break;
        case 0x2000:
            ret |= 0x20;
            break;
        case 0x4000:
            ret |= 0x40;
            break;
        case 0x8000:
            ret |= 0x80;
            break;
        case 0x10000:
            ret |= 0x100;
            break;
        case 0x20000:
            ret |= 0x200;
            break;
        case 0x40000:
            ret |= 0x400;
            break;
        case 0x80000:
            ret |= 0x800;
            break;
        case 0x1:
            ret |= 0x1000;
            break;
        case 0x2:
            ret |= 0x2000;
            break;
        case 0x4:
            ret |= 0x4000;
            break;
        case 0x8:
            ret |= 0x8000;
            break;
        }
        bit <<= 1;
        rem >>= 1;
    }
    return ret;
}

/** Rebuilds the game key assignment from the key configuration (key_config). */
void shPadSetGameKeyAssign(void) {
    struct shGameKeyAssign assign;

    shGameKeyGetAssign(&assign);
    assign.LIGHT = kc2ga(key_config.light);
    assign.ITEM = kc2ga(key_config.item);
    assign.MAP = kc2ga(key_config.map);
    assign.DECIDE = kc2ga(key_config.enter);
    assign.CANCEL = kc2ga(key_config.cancel);
    assign.SKIP = kc2ga(key_config.skip);
    assign.PAUSE = kc2ga(key_config.pause);
    key_config.attack = assign.ACTION = kc2ga(key_config.action); /* @bug attack gets game-assign bits, not pad bits */
    assign.DASH = kc2ga(key_config.dash);
    assign.LSLIDE = kc2ga(key_config.left_move);
    assign.RSLIDE = kc2ga(key_config.right_move);
    assign.READY = kc2ga(key_config.ready);
    assign.VIEW = kc2ga(key_config.search_view);
    shGameKeySetAssign(&assign);
}
