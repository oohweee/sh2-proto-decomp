/*
 * shDBG_fontHandle.c: the debug font. Each character is drawn as a textured
 * sprite through a GIF packet built into sh_DBGFontPacket.
 */

#include "sh2.h"

/* Size of the character packet area, in KB (64 qwords per KB). */
#define DBG_FONT_KB 256

/* dbg_font_uv (the texture coordinates of each character) is defined with the font data. */

/* The debug font packet. */
struct shDBG_font_Packet sh_DBGFontPacket;

/** Empties the debug font's character packet. */
void shDBG_font_init(void) {
    sh_DBGFontPacket.Qtail = sh_DBGFontPacket.dbg_font_packet;
}

/**
 * Builds the GS environment packet of the debug font and the packet that restores the default
 * environment after it.
 */
/* Matching: qwd is a Q_WORDDATA * (the 16-byte-aligned typedef), for the original's aligned stack slot;
 * `id` is in the DWARF but no code survives for it, so its use is reconstructed
 * (docs/matching-notes.md#shdbg_fonthandle-shdbg_initfontenv). */
void shDBG_InitFontEnv(void) {
    Q_WORDDATA *qwd;
    int id;

    id = 0;
    qwd = sh_DBGFontPacket.dbg_font_env;
    qwd[0].ui32[0] = 0x10000007;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0x50000007;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8006;
    qwd[2].ul64[1] = 0x3F;
    qwd[2].ul64[0] = 0;
    qwd[3].ul64[1] = 0x47 + id;
    qwd[3].ul64[0] = 0x3001B;
    qwd[4].ul64[1] = 0x4E + id;
    qwd[4].ul64[0] = 0x13A0001C0;
    qwd[5].ul64[1] = 0x6 + id;
    qwd[5].ul64[0] = 0x2006F9859D40B7D0;
    qwd[6].ul64[0] = 0;
    qwd[6].ul64[1] = 0x14 + id;
    qwd[7].ul64[1] = 0x1;
    qwd[7].ul64[0] = 0x3F80000080808080;
    qwd[8].ui32[0] = 0x20000000;
    qwd[8].ui32[1] = (unsigned int)sh_DBGFontPacket.dbg_font_packet;
    qwd[8].ul64[1] = 0;
    qwd = sh_DBGFontPacket.ret_font_packet;
    sh2gfw_setREF_tagchain(&qwd, shGs_AllEnv.DefaultEnv);
    qwd[0].ui32[0] = 0x70000000;
    qwd[0].ui32[1] = 0;
    qwd[0].ul64[1] = 0;
}

/**
 * Appends the characters of `st` at pixel (`ix`, `iy`) to the debug font packet, one textured
 * sprite each.
 */
/* Matching: the original file had more above this; #line puts the assert on line 292. */
#line 257
void _shDBG_print_string(char *st, int ix, int iy) {
    Q_WORDDATA *qwd;
    int code;
    int ix2;
    int num;

    num = 0;
    qwd = sh_DBGFontPacket.Qtail;
    ix <<= 4;
    iy <<= 4;
    ix2 = ix + 0x80;
    qwd++;
    while (*st) {
        num++;
        code = (unsigned char)*st - 0x20;
        st++;
        qwd[0].ul64[0] = 0x408B400000008001;
        qwd[0].ul64[1] = 0x5353;
        qwd[1].ul64[0] = (unsigned long)dbg_font_uv[code][0] | (unsigned long)dbg_font_uv[code][1] << 32;
        qwd[1].ul64[1] = 0;
        qwd[2].ul64[0] = (unsigned long)(ix + 0x7000) | (unsigned long)(iy + 0x7000) << 32;
        qwd[2].ul64[1] = 0x10;
        qwd[3].ul64[0] = (unsigned long)dbg_font_uv[code][2] | (unsigned long)dbg_font_uv[code][3] << 32;
        qwd[3].ul64[1] = 0;
        qwd[4].ul64[0] = (unsigned long)(ix2 + 0x7000) | (unsigned long)(iy + 0x7080) << 32;
        qwd[4].ul64[1] = 0x10;
        qwd += 5;
        ix = ix2;
        ix2 += 0x80;
    }
    sh_DBGFontPacket.Qtail->ui32[0] = num * 5 | 0x10000000;
    sh_DBGFontPacket.Qtail->ui32[1] = 0;
    sh_DBGFontPacket.Qtail->ui32[2] = 0;
    sh_DBGFontPacket.Qtail->ui32[3] = num * 5 | 0x50000000;
    sh_DBGFontPacket.Qtail = qwd;
    assert((sh_DBGFontPacket.Qtail - &sh_DBGFontPacket.dbg_font_packet[0])<64*DBG_FONT_KB);
}

/** Prints `st` at (`ix`, `iy`) on the debug font, if the debug pad port switch is on. */
void shDBG_print_string(char *st, int ix, int iy) {
    if (dbSwitchSysPadPort()) {
        dbfntlocate(ix, iy);
        dbfntprint(st);
    }
}
