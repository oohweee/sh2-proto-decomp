/*
 * keydata.c: normalizes the raw pad data (digital / analog / pressure modes)
 * and converts it to the game's key bits through the key assignment.
 */

#include "sh2.h"

static struct shSysKeyAdjustData adjust_prs = {
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x5B, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x5B, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x5B, 0xBF, 0xFF, 0xFF },
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x5B, 0xBF, 0xFF, 0xFF },
};

struct shGameKeyAssign gkey_assign = {
    0, 0x11, 0x22, 0x04, 0x08, 0x04, 0x08,
    0x1000, 0x2000, 0x0020, 0x4000, 0x0010, 0x0040, 0x0080, 0x0010,
    0x8000, 0x0040, 0x0080, 0x0100, 0x0200, 0x0800, 0x0400,
};

static int st_idx_conv[6] = { 6, 7, 4, 5, 24, 25 };

/**
 * Converts the raw pad data `paddata` in place to one layout whatever the pad's mode (digital,
 * analog or pressure-sensitive). Returns 0 for NULL.
 */
int shSysKeyNormalize(char *paddata) {
    int stat;
    int idlen;
    unsigned short button;
    unsigned char *pd;
    int an;
    int add;

    if (!paddata) {
        return 0;
    }
    pd = (unsigned char *)paddata;
    stat = pd[0];
    if (stat == 0) {
        idlen = pd[1];
        button = (pd[3] << 8) | pd[2];
    } else {
        idlen = 0;
        button = 0;
    }
    if (idlen == 0x79) {
        if (!pd[12]) { pd[12] = (button & 0x1000) != 0; }
        if (!pd[13]) { pd[13] = (button & 0x2000) != 0; }
        if (!pd[14]) { pd[14] = (button & 0x4000) != 0; }
        if (!pd[15]) { pd[15] = (button & 0x8000) != 0; }
        if (!pd[16]) { pd[16] = (button & 0x400) != 0; }
        if (!pd[17]) { pd[17] = (button & 0x800) != 0; }
        if (!pd[18]) { pd[18] = (button & 0x100) != 0; }
        if (!pd[19]) { pd[19] = (button & 0x200) != 0; }
        if (!pd[8]) { pd[8] = (button & 0x20) != 0; }
        if (!pd[9]) { pd[9] = (button & 0x80) != 0; }
        if (!pd[10]) { pd[10] = (button & 0x10) != 0; }
        if (!pd[11]) { pd[11] = (button & 0x40) != 0; }
    } else {
        pd[12] = -((button & 0x1000) != 0);
        pd[13] = -((button & 0x2000) != 0);
        pd[14] = -((button & 0x4000) != 0);
        pd[15] = -((button & 0x8000) != 0);
        pd[16] = -((button & 0x400) != 0);
        pd[17] = -((button & 0x800) != 0);
        pd[18] = -((button & 0x100) != 0);
        pd[19] = -((button & 0x200) != 0);
        pd[8] = -((button & 0x20) != 0);
        pd[9] = -((button & 0x80) != 0);
        pd[10] = -((button & 0x10) != 0);
        pd[11] = -((button & 0x40) != 0);
    }
    switch (idlen >> 4) {
    case 0:
    case 1:
    case 3:
    case 4:
        pd[4] = 0x80;
        pd[5] = 0x80;
        pd[6] = 0x80;
        pd[7] = 0x80;
        pd[24] = 0x80;
        pd[25] = 0x80;
        break;
    }
    an = pd[8] - pd[9];
    add = an >> 1;
    if (add == 0 && an != 0) {
        if (an > 0) { add = 1; }
        if (an < 0) { add = 0; }
    }
    pd[24] = add + 0x80;
    an = pd[11] - pd[10];
    add = an >> 1;
    if (add == 0 && an != 0) {
        if (an > 0) { add = 1; }
        if (an < 0) { add = 0; }
    }
    pd[25] = add + 0x80;
    pd[20] = -((button & 0x2) != 0);
    pd[21] = -((button & 0x4) != 0);
    pd[22] = -((button & 0x8) != 0);
    pd[23] = -((button & 0x1) != 0);
    return 1;
}

/**
 * Adjusts the normalized pad data `paddata` in place: remaps the button pressures through a table
 * and turns each stick axis into signed steps with a dead zone around the centre. Returns 0 for
 * NULL.
 */
int shSysKeyAdjust(char *paddata) {
    int idx;
    int st;
    int i;
    unsigned char *pd;

    if (!paddata) {
        return 0;
    }
    if (((unsigned char *)paddata)[8]) { paddata[8] = adjust_prs.AN_6[((unsigned char *)paddata)[8] >> 4]; }
    if (((unsigned char *)paddata)[9]) { paddata[9] = adjust_prs.AN_4[((unsigned char *)paddata)[9] >> 4]; }
    if (((unsigned char *)paddata)[10]) { paddata[10] = adjust_prs.AN_8[((unsigned char *)paddata)[10] >> 4]; }
    if (((unsigned char *)paddata)[11]) { paddata[11] = adjust_prs.AN_2[((unsigned char *)paddata)[11] >> 4]; }
    if (((unsigned char *)paddata)[12]) { paddata[12] = adjust_prs.AN_A[((unsigned char *)paddata)[12] >> 4]; }
    if (((unsigned char *)paddata)[13]) { paddata[13] = adjust_prs.AN_O[((unsigned char *)paddata)[13] >> 4]; }
    if (((unsigned char *)paddata)[14]) { paddata[14] = adjust_prs.AN_X[((unsigned char *)paddata)[14] >> 4]; }
    if (((unsigned char *)paddata)[15]) { paddata[15] = adjust_prs.AN_D[((unsigned char *)paddata)[15] >> 4]; }
    if (((unsigned char *)paddata)[16]) { paddata[16] = adjust_prs.AN_L1[((unsigned char *)paddata)[16] >> 4]; }
    if (((unsigned char *)paddata)[17]) { paddata[17] = adjust_prs.AN_R1[((unsigned char *)paddata)[17] >> 4]; }
    if (((unsigned char *)paddata)[18]) { paddata[18] = adjust_prs.AN_L2[((unsigned char *)paddata)[18] >> 4]; }
    if (((unsigned char *)paddata)[19]) { paddata[19] = adjust_prs.AN_R2[((unsigned char *)paddata)[19] >> 4]; }
    if (((unsigned char *)paddata)[20]) { paddata[20] = adjust_prs.AN_L3[((unsigned char *)paddata)[20] >> 4]; }
    if (((unsigned char *)paddata)[21]) { paddata[21] = adjust_prs.AN_R3[((unsigned char *)paddata)[21] >> 4]; }
    if (((unsigned char *)paddata)[22]) { paddata[22] = adjust_prs.AN_STA[((unsigned char *)paddata)[22] >> 4]; }
    if (((unsigned char *)paddata)[23]) { paddata[23] = adjust_prs.AN_SEL[((unsigned char *)paddata)[23] >> 4]; }
    for (i = 0; i < 6; i++) {
        pd = (unsigned char *)paddata + st_idx_conv[i];
        st = *pd;
        if (st <= 0x53) {
            idx = -((0x54 - st) / 12);
        } else if (st > 0xAB) {
            idx = (st - 0xAB) / 12;
        } else {
            idx = 0;
        }
        *pd = idx << 4;
    }
    return 1;
}

static unsigned int shGameKeyGetAnalog(char *paddata, unsigned short an_assign, int bit) {
    unsigned char *pd;
    int an_idx;
    int key;
    int k;

    for (an_idx = 8, key = 0; (k = an_assign) && an_idx <= 23; an_assign >>= 1, an_idx++) {
        if (k & 1) {
            pd = (unsigned char *)paddata + an_idx;
            k = *pd >> (8 - bit);
            if (k == 0 && *pd != 0) {
                k = 1;
            }
            if (key < k) {
                key = k;
            }
            if (key != (1 << bit) - 1) {
                break;
            }
        }
    }
    return key;
}

static unsigned int shGameKeyGetStick(char *paddata, unsigned char st_assign, int bit) {
    int minus_absmax;
    int plus_absmax;
    int st;
    int key;
    int st_idx;
    int st_idx_idx;

    for (minus_absmax = 0, plus_absmax = 0, st_idx_idx = 0; (st = st_assign) && st_idx_idx < 6; st_assign >>= 1, st_idx_idx++) {
        if (st & 1) {
            st_idx = st_idx_conv[st_idx_idx];
            st = paddata[st_idx];
            key = st >> (8 - bit);
            if (key == 0) {
                if (st > 0) { key = 1; }
                if (st < 0) { key = -1; }
            }
            if (key < 0) {
                if (key < minus_absmax) { minus_absmax = key; }
            } else {
                if (plus_absmax < key) { plus_absmax = key; }
            }
        }
    }
    return plus_absmax + minus_absmax;
}

/**
 * Converts the normalized pad data `paddata` to the game's keys `key` through the current key
 * assignment. Returns 0 if either is NULL, 1 otherwise.
 */
int shGameKeyConvert(union shGameKeyData *key, char *paddata) {
    if (!key) {
        return 0;
    }
    if (!paddata) {
        return 0;
    }
    key->f.DRINK = shGameKeyGetAnalog(paddata, gkey_assign.DRINK, 1);
    key->f.RADIO = shGameKeyGetAnalog(paddata, gkey_assign.RADIO, 1);
    key->f.LIGHT = shGameKeyGetAnalog(paddata, gkey_assign.LIGHT, 1);
    key->f.ITEM = shGameKeyGetAnalog(paddata, gkey_assign.ITEM, 1);
    key->f.MAP = shGameKeyGetAnalog(paddata, gkey_assign.MAP, 1);
    key->f.DECIDE = shGameKeyGetAnalog(paddata, gkey_assign.DECIDE, 1);
    key->f.CANCEL = shGameKeyGetAnalog(paddata, gkey_assign.CANCEL, 1);
    key->f.SKIP = shGameKeyGetAnalog(paddata, gkey_assign.SKIP, 1);
    key->f.PAUSE = shGameKeyGetAnalog(paddata, gkey_assign.PAUSE, 1);
    key->f.ACTION = shGameKeyGetAnalog(paddata, gkey_assign.ACTION, 2);
    key->f.DASH = shGameKeyGetAnalog(paddata, gkey_assign.DASH, 2);
    key->f.LSLIDE = shGameKeyGetAnalog(paddata, gkey_assign.LSLIDE, 2);
    key->f.RSLIDE = shGameKeyGetAnalog(paddata, gkey_assign.RSLIDE, 2);
    key->f.READY = shGameKeyGetAnalog(paddata, gkey_assign.READY, 2);
    key->f.VIEW = shGameKeyGetAnalog(paddata, gkey_assign.VIEW, 2);
    key->f.AX = (char)shGameKeyGetStick(paddata, gkey_assign.DIR_AX, 4);
    key->f.AY = (char)shGameKeyGetStick(paddata, gkey_assign.DIR_AY, 4);
    key->f.BX = (char)shGameKeyGetStick(paddata, gkey_assign.DIR_BX, 2);
    key->f.BY = (char)shGameKeyGetStick(paddata, gkey_assign.DIR_BY, 2);
    key->f.CX = (char)shGameKeyGetStick(paddata, gkey_assign.DIR_CX, 4);
    key->f.CY = (char)shGameKeyGetStick(paddata, gkey_assign.DIR_CY, 4);
    return 1;
}

/** Copies the current key assignment to `assign` (if not NULL). */
void shGameKeyGetAssign(struct shGameKeyAssign *assign) {
    if (assign) {
        *assign = gkey_assign;
    }
}

/** Sets the key assignment from `assign` (if not NULL). */
void shGameKeySetAssign(struct shGameKeyAssign *assign) {
    if (assign) {
        gkey_assign = *assign;
    }
}
