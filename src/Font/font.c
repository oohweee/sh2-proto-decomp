/*
 * font.c: the game's text renderer. Glyphs are unpacked on demand from the
 * compressed font data into a 512x512 4-bit texture cache (25 x 17 cells,
 * LRU list in font.upper/font.lower), and strings become FONT_STREAM_DATA
 * sprites that fontFlush turns into a GS packet.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sdk/eekernel.h"
#include "sdk/libgraph.h"

typedef unsigned int u_int;

#define FONT_STREAM_BUFFER_SIZE 0x4000

extern unsigned char FontData[];
extern unsigned char FontData2[];

short FontSize[2][2] = { { 20, 30 }, { 16, 24 } };
unsigned char *FontDataTable[2] = { FontData, FontData2 };
unsigned long font_dma_data[34] __attribute__((aligned(64))) = {
    0x0000000010000006, 0x5000000600000000, 0x1000000000000004, 0x000000000000000E,
    0x1408000000000000, 0x0000000000000050, 0x0000000000000000, 0x0000000000000051,
    0x0000020000000200, 0x0000000000000052, 0x0000000000000000, 0x0000000000000053,
    0x0800000000002000, 0x0000000000000000, 0x0000000030002000, 0x5000200000000000,
    0x0000000010000006, 0x5000000600000000, 0x1000000000000004, 0x000000000000000E,
    0x0001000000000000, 0x0000000000000050, 0x0000000000000000, 0x0000000000000051,
    0x0000000100000008, 0x0000000000000052, 0x0000000000000000, 0x0000000000000053,
    0x0800000000008002, 0x0000000000000000, 0x0000000030000002, 0x5000000200000000,
    0x0000000070000000, 0x0000000000000000,
};
unsigned long font_after_env[8] __attribute__((aligned(64))) = {
    0x0000000070000003, 0x5000000300000000, 0x1000000000008002, 0x000000000000000E,
    0x000000000003000D, 0x0000000000000047, 0x000000013A0001C0, 0x000000000000004E,
};

struct FONT_DATA font;
char font_stream_buf[FONT_STREAM_BUFFER_SIZE];


/** Initializes the font system: clears the font work, sets up the glyph cache, the DMA data and the
 * default messages.
 * @return the font work */
struct FONT_DATA *fontInit(void) {
    int i;
    unsigned short defmes[8] = { 0x0112, 0x0D89, 0x095C, 0x0ADF, 0x0172, 0x0113, 0xFFFF, 0x8000 };

    shQzero(&font, sizeof(font));
    ((u_int *)font_dma_data)[29] = (u_int)font.texbuf;
    ((u_int *)font_dma_data)[61] = (u_int)font.clut;
    font.clut[0] = 0;
    for (i = 1; i < 7; i++) {
        font.clut[i] = ((i * 2 + 3) << 28) | 0x0FA0A0A0;
    }
    font.base_x = font.base_y = 0;
    font.base_z = 0xFFFFFF;
    fontClear();
    font.top = -1;
    font.flag = 1;
    for (i = 0; i < 10; i++) {
        memcpy(font.mes_v[i], defmes, sizeof(defmes));
        font.mes_v[i][4] += i;
    }
    font.m_h = font.m_w = 32;
    font.m_sx = 9;
    font.m_sy = 10;
    font.m_top = -1;
    font.m_base_x = 0x7000;
    font.m_base_y = 0x7000;
    font.m_base_z = 0xFFFFFF;
    font.m_rgba = 0x50808080;
    {
        unsigned short fake_mes[22] = {
            0x0000, 0x0237, 0x024E, 0x00FA, 0x0242, 0x0232, 0x0225, 0x0237, 0x0203, 0x0201, 0x0BD0,
            0x01B3, 0x01CE, 0x01F3, 0x01F0, 0x00FA, 0x01D6, 0x01C6, 0x01F6, 0x00E8, 0xFFFD, 0xFFFF,
        };
        unsigned short *str;
        unsigned short c;

        str = fake_mes;
        while ((c = *str++) != 0xFFFF) {
            if (c == 0xFFFD) {
                while (font.bottom % 25) {
                    fontLoad(0);
                    font.code[font.bottom - 1] = 0x7FFF;
                }
            } else {
                fontLoad(c);
                font.code[font.bottom - 1] = 0x7FFF;
            }
        }
    }
    return &font;
}

/** Clears the text on screen and resets colour, shadow and message state. */
void fontClear(void) {
    font.st_num = font.w_st_num = 0;
    fontSetColor(0);
    font.rgb_s[0] = 0;
    font.shadow_max = 1;
    font.shadow_now = 0;
    font.alpha = font.alpha_base = 0x80;
    font.fonttype = 0;
    font.flag = (font.flag & 0x38F8) | 1;
    fontSetStreamMax(0x200, 0x40, 0x200);
}

/** Splits the stream buffer between normal, wide and extra (mfont) glyph sprites.
 * @param s_max number of normal glyphs
 * @param ws_max number of wide glyphs
 * @param ms_max number of extra glyphs */
void fontSetStreamMax(unsigned short s_max, unsigned short ws_max, unsigned short ms_max) {
    font.stream_max = s_max;
    font.w_stream_max = ws_max;
    font.m_stream_max = ms_max;
    font.stream = (struct FONT_STREAM_DATA *)font_stream_buf;
    font.w_stream = (struct WFONT_STREAM_DATA *)(font.stream + s_max);
    font.m_stream = (struct MFONT_STREAM_DATA *)(font.w_stream + ws_max);
    /* Matching: the assert bakes its original line number into the object. */
#line 297
    fjAssert(((u_int)(font.m_stream + ms_max) - (u_int)font_stream_buf) <= FONT_STREAM_BUFFER_SIZE);
}

static void fontGetData(void);

/** Makes sure a glyph is in the texture cache, unpacking it from the font data if needed (least
 * recently used cell first).
 * @param code font code
 * @return the glyph's cache cell */
/*
 * Matching: GCC-style asm in the body with its operands passed as expressions; the original's
 * DWARF (fw, fh, b in v0, adr_p in t5) and declaration order are reproduced exactly.
 */
int fontLoad(unsigned short code) {
    int num;
    int nbak;
    unsigned short c2;
    unsigned short *adr_p;
    unsigned short adr;
    unsigned int n;
    unsigned char *b;
    int y;
    int fw;
    int fh;
    unsigned short *f_code;
    short *f_upper;
    short *f_lower;
    unsigned char *fontdata;

    f_code = font.code;
    f_upper = font.upper;
    f_lower = font.lower;
    c2 = code + (font.fonttype << 13);
    nbak = 0;
    num = font.top;
    while (num != -1) {
        if (c2 == f_code[num]) {
            if (num == font.top) {
                return num;
            }
            if (f_upper[num] != -1) {
                f_lower[f_upper[num]] = f_lower[num];
            }
            if (f_lower[num] != -1) {
                f_upper[f_lower[num]] = f_upper[num];
            }
            f_upper[num] = -1;
            f_lower[num] = font.top;
            f_upper[font.top] = num;
            font.top = num;
            return num;
        }
        nbak = num;
        num = f_lower[num];
    }
    num = font.bottom;
    if (num == 400) {
        num = nbak;
        f_lower[f_upper[nbak]] = -1;
    } else {
        font.bottom = num + 1;
    }
    f_code[num] = c2;
    f_upper[num] = -1;
    f_lower[num] = font.top;
    if (font.top != -1) {
        f_upper[font.top] = num;
    }
    font.top = num;
    fontdata = FontDataTable[font.fonttype];
    n = 0;
    adr_p = (unsigned short *)fontdata;
    while ((adr = *adr_p++) != 0) {
        if (adr <= code) {
            n++;
        }
    }
    adr = ((unsigned short *)(fontdata + 0xF0))[code];
    b = &font.texbuf[((num % 25) * 20 + (num / 25) * 15360) / 2];
    if (code < 0xE0) {
        fw = fontdata[code + 0x10];
    } else {
        fw = FontSize[font.fonttype][0];
    }
    fh = FontSize[font.fonttype][1];
    for (y = 0; y < 30; y++) {
        memset(b + y * 256, 0, 10);
    }
    if (adr == 0 || fw == 0) {
        return num;
    }
    __asm__ __volatile__("
        .set noreorder
        addu    t4, %1, zero
        addi    t5, zero, 0x20
        xor     t6, t6, t6
        xor     t2, t2, t2
        xor     s0, s0, s0
        srl     s1, %2, 1
        addi    s2, zero, 0x100
        xor     t1, t1, t1
        subu    s1, s2, s1
    L_row:
        xor     t0, t0, t0
    L_pix:
        xor     t3, t3, t3
        bnel    t2, zero, L_put
        addiu   t2, t2, -0x1
        jal     fontGetData
        addi    s2, zero, 0x7
        add     t3, t7, zero
        bne     t7, s2, L_put
        nop
        jal     fontGetData
        xor     t3, t3, t3
        bnez    t7, L_put
        add     t2, t7, zero
        jal     fontGetData
        nop
        bnez    t7, L_put
        addi    t2, t7, 0x7
        jal     fontGetData
        nop
        bnez    t7, L_put
        addi    t2, t7, 0xE
        jal     fontGetData
        nop
        jal     fontGetData
        add     t2, t7, zero
        sll     t7, t7, 3
        or      t7, t7, t2
        bnez    t7, L_put
        addi    t2, t7, 0x15
        jal     fontGetData
        nop
        jal     fontGetData
        add     t2, t7, zero
        sll     t7, t7, 3
        jal     fontGetData
        or      t2, t2, t7
        sll     t7, t7, 6
        or      t7, t7, t2
        addi    t2, t7, 0x54
    L_put:
        andi    s2, t0, 0x1
        beql    s2, zero, L_next
        add     s0, t3, zero
        sll     t3, t3, 4
        or      t3, t3, s0
        sb      t3, 0x0(%0)
        addiu   %0, %0, 0x1
    L_next:
        addi    t0, t0, 0x1
        bne     t0, %2, L_pix
        andi    s2, %2, 0x1
        bnel    s2, zero, L_odd
        sb      s0, 0x0(%0)
    L_odd:
        addi    t1, t1, 0x1
        bne     t1, %3, L_row
        addu    %0, %0, s1
    " : : "r"(b), "r"((unsigned short *)(fontdata + adr * 4 + (n << 18))), "r"(fw), "r"(fh));
    font.flag |= 4;
    return num;
}

/* The glyph decompressor's bit reader, for asm callers (t4 = source, t5 = bit count, t6 = bits). */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 553); the return's delay slot is filled. */
static asm void fontGetData(void) {
    .set noreorder
    addiu   t7, t5, -0x1E
    bltz    t7, @1
    nop
    lw      t7, 0x0(t4)
    addiu   t4, t4, 0x4
    dsll32  t7, t7, 0
    dsrlv   t7, t7, t5
    addiu   t5, t5, -0x20
    or      t6, t6, t7
@1:
    addiu   t5, t5, 0x3
    andi    t7, t6, 0x7
    jr      ra
    dsrl    t6, t6, 3
}

/** Queues one glyph at (x, y) (as a wide glyph while wide mode is on).
 * @param code font code
 * @param x x position (GS units)
 * @param y y position (GS units) */
void fontSet(unsigned short code, unsigned short x, unsigned short y) {
    int num;

    if (font.flag & 0x400) {
        struct WFONT_STREAM_DATA *fstream;

        if (font.w_st_num >= font.w_stream_max) {
            printf("wfont over.\n");
            return;
        }
        num = fontLoad(code);
        fstream = &font.w_stream[font.w_st_num];
        fstream->x = x << 4;
        fstream->y = y << 4;
        fstream->u = (num % 25) * 20;
        fstream->v = (num / 25) * 30;
        fstream->rgb_u = font.rgb_u | (font.alpha << 24);
        fstream->rgb_d = font.rgb_d | (font.shadow_now << 24);
        fstream->w = FontSize[font.fonttype][0] << 4;
        fstream->h = FontSize[font.fonttype][1] << 4;
        fstream->vw = font.wide_w << 4;
        fstream->vh = font.wide_h << 4;
        font.w_st_num++;
    } else {
        struct FONT_STREAM_DATA *fstream;

        if (font.st_num >= font.stream_max) {
            printf("font over.\n");
            return;
        }
        num = fontLoad(code);
        fstream = &font.stream[font.st_num];
        fstream->x = x << 4;
        fstream->y = y << 4;
        fstream->u = (num % 25) * 20;
        fstream->v = (num / 25) * 30;
        fstream->rgb_u = font.rgb_u | (font.alpha << 24);
        fstream->rgb_d = font.rgb_d | (font.shadow_now << 24);
        fstream->w = FontSize[font.fonttype][0] << 4;
        fstream->h = FontSize[font.fonttype][1] << 4;
        font.st_num++;
    }
}

/** Queues one glyph scaled to w x h.
 * @param code font code
 * @param x x position (GS units)
 * @param y y position (GS units)
 * @param w width
 * @param h height */
void fontSetWide(unsigned short code, unsigned short x, unsigned short y, unsigned short w, unsigned short h) {
    int num;
    int tx;
    int ty;
    struct WFONT_STREAM_DATA *fstream;

    if (font.w_st_num >= font.w_stream_max) {
        printf("wfont over.\n");
        return;
    }
    num = fontLoad(code);
    tx = (x << 4) - font.base_x;
    ty = (y << 4) - font.base_y;
    if (tx + (w << 4) < 0x7000 || ty + (h << 4) < 0x7000 || tx - (w << 4) > 0x9000 || ty - (h << 4) > 0x9000) {
        return;
    }
    fstream = &font.w_stream[font.w_st_num];
    fstream->x = x << 4;
    fstream->y = y << 4;
    fstream->u = (num % 25) * 20;
    fstream->v = (num / 25) * 30;
    fstream->rgb_u = font.rgb_u | (font.alpha << 24);
    fstream->rgb_d = font.rgb_d | (font.shadow_now << 24);
    fstream->w = FontSize[font.fonttype][0] << 4;
    fstream->h = FontSize[font.fonttype][1] << 4;
    fstream->vw = w << 4;
    fstream->vh = h << 4;
    font.w_st_num++;
}

/** Queues a blank box from x0 to x1 on line y.
 * @param x0 left
 * @param x1 right
 * @param y line */
void fontSetBlankBox(int x0, int x1, int y) {
    struct FONT_STREAM_DATA *fstream;

    if (font.st_num >= font.stream_max) {
        printf("font over.\n");
        return;
    }
    fstream = &font.stream[font.st_num];
    fstream->x = x0 << 4;
    fstream->y = y << 4;
    fstream->u = x1 << 4;
    fstream->v = 0xFFF7;
    fstream->rgb_u = font.rgb_u | (font.alpha << 24);
    fstream->rgb_d = font.rgb_d | (font.shadow_now << 24);
    fstream->h = FontSize[font.fonttype][1] << 4;
    font.st_num++;
}

/** Queues a horizontal line of width w at (x, y).
 * @param x left
 * @param w width
 * @param y line */
void fontSetLine(int x, int w, int y) {
    struct FONT_STREAM_DATA *fstream;
    unsigned int r;
    unsigned int g;
    unsigned int b;

    if (font.st_num >= font.stream_max) {
        printf("font over.\n");
        return;
    }
    fstream = &font.stream[font.st_num];
    fstream->x = x << 4;
    if (font.flag & 0x400) {
        fstream->y = ((y + font.wide_h / 2) >> 1) << 5;
    } else {
        fstream->y = ((y + FontSize[font.fonttype][1] / 2) >> 1) << 5;
    }
    fstream->u = ((x + w) << 4) - 1;
    fstream->v = 0xFE00;
    r = ((font.rgb_u & 0xFF) + (font.rgb_d & 0xFF)) >> 1;
    g = (((font.rgb_u >> 8) & 0xFF) + ((font.rgb_d >> 8) & 0xFF)) >> 1;
    b = (((font.rgb_u >> 16) & 0xFF) + ((font.rgb_d >> 16) & 0xFF)) >> 1;
    fstream->rgb_u = (font.alpha << 24) | ((b << 16) | (r | (g << 8)));
    fstream->rgb_d = font.shadow_now;
    font.st_num++;
}

/** Prints a font code string at (x, y) (screen pixels; negative: don't draw).
 * @param str the string (NULL: nothing)
 * @param x x position
 * @param y y position */
void fontPrintStr(unsigned short *str, int x, int y) {
    if (str) {
        font.x = x;
        font.y = y;
        fontPrintStrMain(&str, x >= 0 && y >= 0);
    }
}

/** Prints message num of a message file at (x, y).
 * @param str the message file
 * @param num message number
 * @param x x position
 * @param y y position */
void fontPrintStrNum(unsigned short *str, unsigned short num, int x, int y) {
    fontPrintStr(fontGetMesAdr(str, num), x, y);
}

/* Reads one character code from a byte string (0xE0 escapes a 16-bit code). */
static inline unsigned short fontGetCode(unsigned char **pstr) {
    unsigned short c;
    short hi;

    c = *(*pstr)++;
    if (c == 0xE0) {
        c = (*pstr)[0];
        hi = (*pstr)[1];
        c |= (unsigned short)(hi << 8);
        *pstr += 2;
    } else if (c > 0xE0) {
        c = (c << 8) | *(*pstr)++;
    }
    return c;
}

/* GS XYZ2 data word: x, y in 16 bits each, z in the upper 32 bits. */
static inline unsigned long fontXYZ(int x, int y, int z) {
    unsigned long r;

    __asm__("
    pextlh %0, %2, %1
    pextlw %0, %3, %0
    " : "=r"(r) : "r"(x), "r"(y), "r"(z));
    return r;
}

#define SCREEN_WIDTH 512

/** Prints the next page of a string: handles the control codes (colours, waits, choices, new
 * lines) and queues the glyphs.
 * @param pstr the string position (advanced past the page)
 * @param flag non-zero to draw (0: only lay out)
 * @return the control code that ended the page */
/*
 * Matching: if/else for the code fetch and `c & 0xFF` inline, as in the original's line table;
 * `(int)sy` and the glyph-width spelling are explained at fontPrintWord.
 */
unsigned short fontPrintStrMain(unsigned short **pstr, int flag) {
    unsigned short x;
    unsigned short y;
    unsigned short w;
    unsigned short wm;
    unsigned short c;
    unsigned short ck;
    unsigned short ln;
    unsigned short sx;
    unsigned short sy;
    unsigned short def_w;
    unsigned short *str;
    unsigned short *str_stack;
    unsigned char *str_c;
    unsigned char *str_stack_c;
    int align;
    int align2;
    int pos_x;
    int pos_y;
    int starty;
    int bbx;
    int lw[20];

    if (!flag) {
        align = 0;
        pos_x = 0x800;
        pos_y = -1;
    } else {
        align = 2;
        pos_x = font.x + 0x700;
        pos_y = font.y + 0x700;
    }
    if (font.flag & 0x100) {
        align = 1;
    }
    align2 = (font.flag & 0x200) ? 1 : 0;
    w = wm = 0;
    str = *pstr;
    str_c = NULL;
    str_stack = NULL;
    str_stack_c = NULL;
    ln = 0;
    bbx = starty = -1;
    font.sel_now = 0;
    font.sel_max = 0;
    font.sel_xl = 0xFFFF;
    x = 0;
    font.sel_xr = 0;
    if (!str) {
        return 0;
    }
    if (font.flag & 0x400) {
        sx = font.wide_w;
        sy = font.wide_h;
    } else {
        sx = FontSize[font.fonttype][0];
        sy = FontSize[font.fonttype][1];
    }
    def_w = FontSize[font.fonttype][0];
    c = *str++;
    while (c != 0xFFFF) {
        if (c < 0x7FFF) {
            if (c == 0) {
                w += sx * 5 / def_w + 2;
            } else if (c >= 0xE0) {
                w += sx + 2;
            } else {
                w += *(FontDataTable[font.fonttype] + c + 0x10) * sx / def_w + 2;
                if (font.fonttype == 1) {
                    w++;
                }
            }
        } else if ((ck = c & 0xFF00) == 0xFF00) {
            switch (c) {
            case 0xFFFE:
                w += sx + 2;
                break;
            case 0xFFF9:
            case 0xFFFD:
                lw[ln++] = w - 2;
                if (w > wm) {
                    wm = w;
                }
                w = 0;
                break;
            case 0xFFF5:
            case 0xFFF6:
                lw[ln++] = 0;
                break;
            case 0xFFFC:
                align = 0;
                break;
            case 0xFFFB:
                align = 1;
                break;
            case 0xFFFA:
                align2 = 1;
                break;
            default:
                if (c >= 0xFFE0 && c <= 0xFFE9) {
                    if (str_stack) {
                        printf("font: double stack!\n");
                    }
                    str_stack = str;
                    str_stack_c = str_c;
                    str = font.mes_v[c - 0xFFE0];
                    str_c = NULL;
                }
                break;
            }
        } else if (ck == 0xFE00) {
            w += (c & 0xFF) + 2;
        } else if (ck == 0xFD00) {
            w += c & 0xFF;
        } else if (ck == 0xFC00) {
            w -= c & 0xFF;
        } else if ((c & 0xFE00) == 0xFA00) {
            pos_x = (c & 0x1FF) + 0x700;
            align = 2;
        } else if ((c & 0xFE00) == 0xF800) {
            pos_y = (c & 0x1FF) + 0x700;
        } else if (c == 0x8000) {
            str_c = (unsigned char *)str;
        }
        if (str_c) {
            c = fontGetCode(&str_c);
        } else {
            c = *str++;
        }
        if (c == 0xFFFF && str_stack) {
            str = str_stack;
            str_c = str_stack_c;
            str_stack = NULL;
            if (str_c) {
                c = fontGetCode(&str_c);
            } else {
                c = *str++;
            }
        }
    }
    if (w != 0) {
        lw[ln++] = w - 2;
        if (w > wm) {
            wm = w;
        }
    }
    font.right_x = (wm + 0x200) / 2 + 0x700;
    if (align2) {
        if (pos_y == -1) {
            y = 0x800 - ln * sy / 2;
        } else {
            y = pos_y - ln * sy / 2;
        }
    } else if (pos_y != -1) {
        y = pos_y;
    } else {
        y = 0x8DC - (ln + 4) * sy / 2;
    }
    ln = 0;
    str = *pstr;
    str_c = NULL;
    c = *str++;
    for (; c != 0xFFFF; ln++) {
        switch (align) {
        case 1:
            x = pos_x - lw[ln] / 2;
            break;
        case 2:
            x = pos_x;
            break;
        default:
            x = pos_x - wm / 2;
            break;
        }
        if (!(font.flag & 0x2000)) {
            /* Matching: the assert bakes its original line number into the object. */
#line 909
            fjAssert(x >= (4096-SCREEN_WIDTH)/2);
        }
        if (starty != -1 && x < font.sel_xl) {
            font.sel_xl = x;
        }
        while (c != 0xFFFD && c != 0xFFFF && c != 0xFFF9 && c != 0xFFF6 && c != 0xFFF5) {
            if (c < 0x7FFF) {
                if (c == 0) {
                    x += sx * 5 / def_w + 2;
                } else {
                    if (bbx == -1) {
                        fontSet(c, x, y);
                    }
                    if (c >= 0xE0) {
                        x += sx + 2;
                    } else {
                        x += *(FontDataTable[font.fonttype] + c + 0x10) * sx / def_w + 2;
                        if (font.fonttype == 1) {
                            x++;
                        }
                    }
                }
            } else if ((ck = c & 0xFF00) == 0xFF00) {
                switch (c) {
                case 0xFFFE:
                    x += sx + 2;
                    break;
                case 0xFFF8:
                    font.sel_now = font.sel_max;
                    break;
                case 0xFFF7:
                    if (!(font.flag & 0x80)) {
                        if (bbx == -1) {
                            bbx = x;
                        } else {
                            fontSetBlankBox(bbx, x - 2, y);
                            bbx = -1;
                        }
                    }
                    break;
                default:
                    if (c <= 0xFF63) {
                        fontSetColor(c - 0xFF00);
                    } else if (c >= 0xFFE0 && c <= 0xFFE9) {
                        str_stack = str;
                        str_stack_c = str_c;
                        str = font.mes_v[c - 0xFFE0];
                        str_c = NULL;
                    }
                    break;
                }
            } else if (ck == 0xFE00) {
                fontSetLine(x, c & 0xFF, y);
                x += (c & 0xFF) + 2;

            } else if (ck == 0xFD00) {
                x += c & 0xFF;
            } else if (ck == 0xFC00) {
                x -= c & 0xFF;
            } else if (c == 0x8000) {
                str_c = (unsigned char *)str;
            }
            if (str_c) {
                c = fontGetCode(&str_c);
            } else {
                c = *str++;
            }
            if (c == 0xFFFF && str_stack) {
                str = str_stack;
                str_c = str_stack_c;
                str_stack = NULL;
                if (str_c) {
                    c = fontGetCode(&str_c);
                } else {
                    c = *str++;
                }
            }
        }
        if (c == 0xFFFD || c == 0xFFF9) {
            y += (int)sy;
            if (c == 0xFFF9) {
                if (starty != -1) {
                    if (x > font.sel_xr) {
                        font.sel_xr = x;
                    }
                    font.sel_yd[font.sel_max++] = y;
                }
                font.sel_yu[font.sel_max] = y;
                starty = y;
            }
            if (str_c) {
                c = fontGetCode(&str_c);
            } else {
                c = *str++;
            }
        } else if (c == 0xFFF6 || c == 0xFFF5) {
            if (c == 0xFFF5) {
                font.sel_now = 1;
            }
            y += (int)sy;
            fontSetYesNo(y - 0x700);
            font.sel_max = -1;
            font.sel_yu[0] = y;
            font.sel_yd[0] = y + sy;
            if (str_c) {
                c = fontGetCode(&str_c);
            } else {
                c = *str++;
            }
        }
    }
    if (starty != -1) {
        if (x > font.sel_xr) {
            font.sel_xr = x;
        }
        font.sel_yd[font.sel_max++] = y + sy;
    }
    font.wm = wm;
    font.hm = ln * sy;
    font.right_y = y;
    if (str_c) {
        str = (unsigned short *)((u_int)(str_c + 1) & ~1);
    }
    c = *str;
    *pstr = str + 1;
    if (c & 0x8000) {
        font.flag |= 1;
    }
    return c;
}

/** Prints a decimal number (up to 10 digits).
 * @param num the number
 * @param x x position
 * @param y y position
 * @param len minimum number of digits (the number is cut to len unless flag & 4)
 * @param flag 1: pad with zeros, 2: use the alternative digit glyphs, 4: never cut */
void fontPrintDec(int num, int x, int y, int len, int flag) {
    int buf[10];
    int vx;
    int vy;
    int point;
    int sx;
    int sy;
    unsigned short code;

    vx = x + 0x700;
    vy = y + 0x700;
    sx = FontSize[font.fonttype][0];
    sy = FontSize[font.fonttype][1]; /* Matching: dead; the original's DWARF has sy */
    if (len > 10) {
        len = 10;
    }
    point = 0;
    do {
        buf[point++] = num % 10;
        num /= 10;
    } while (num);
    if (!(flag & 4) && len > 0 && point > len) {
        point = len;
    }
    for (; len > point; len--) {
        if (flag & 1) {
            fontSet((flag & 2) ? 0x172 : 0x10, vx, vy);
        }
        vx += sx + 2;
    }
    while (--point >= 0) {
        code = buf[point];
        if (flag & 2) {
            code += 0x172;
        } else {
            code += 0x10;
        }
        fontSet(code, vx, vy);
        if (code >= 0xE0) {
            vx += sx + 2;
        } else {
            vx += FontDataTable[font.fonttype][code + 0x10] + 6;
        }
    }
}

/** Prints the "yes"/"no" choice on line y and sets up its selection bar.
 * @param y line */
void fontSetYesNo(int y) {
    int w;
    int w2;

    w = fontPrintWord(fontGetMesAdr(msg_station, 0), 200, y, 1, 0);
    w2 = fontPrintWord(fontGetMesAdr(msg_station, 1), 312, y, 1, 0);
    if (w > w2) {
        font.sel_yu[1] = w;
    } else {
        font.sel_yu[1] = w2;
    }
}

/** Prints one string with alignment and returns its width.
 * @param str the string
 * @param x x position
 * @param y y position
 * @param align horizontal alignment
 * @param align2 second alignment flag
 * @return the width */
/*
 * Matching: reconstructed. `ty += (int)sy;`: the original sign-extends then zero-extends sy here
 * (only here and in fontPrintStrMain in the whole game), so its source converted it somehow; the
 * cast is the least code that does. Glyph widths as *(FontDataTable[..] + c + 0x10) in all four.
 */
int fontPrintWord(unsigned short *str, int x, int y, int align, int align2) {
    unsigned short tx;
    unsigned short ty;
    unsigned short w;
    unsigned short wm;
    unsigned short c;
    unsigned short ck;
    unsigned short ln;
    unsigned short sx;
    unsigned short sy;
    unsigned short def_w;
    unsigned short *sstr;
    unsigned char *str_c;
    int lw[20];
    int b_hm;
    int b_wm;

    w = wm = 0;
    sstr = str;
    str_c = NULL;
    ln = 0;
    if (!str) {
        return 0;
    }
    if (font.flag & 0x400) {
        sx = font.wide_w;
        sy = font.wide_h;
    } else {
        sx = FontSize[font.fonttype][0];
        sy = FontSize[font.fonttype][1];
    }
    def_w = FontSize[font.fonttype][0];
    c = *str++;
    while (c != 0xFFFF) {
        if (c < 0x7FFF) {
            if (c == 0) {
                w += sx * 5 / def_w + 2;
            } else if (c >= 0xE0) {
                w += sx + 2;
            } else {
                w += *(FontDataTable[font.fonttype] + c + 0x10) * sx / def_w + 2;
            }
        } else if ((ck = c & 0xFF00) == 0xFF00) {
            switch (c) {
            case 0xFFFE:
                w += sx + 2;
                break;
            case 0xFFFD:
                lw[ln++] = w - 2;
                if (w > wm) {
                    wm = w;
                }
                w = 0;
                break;
            }
        } else if (ck == 0xFD00) {
            w += c & 0xFF;
        } else if (ck == 0xFC00) {
            w -= c & 0xFF;
        } else if (c == 0x8000) {
            str_c = (unsigned char *)str;
        }
        if (str_c) {
            c = fontGetCode(&str_c);
        } else {
            c = *str++;
        }
    }
    if (w != 0) {
        lw[ln++] = w - 2;
        if (w > wm) {
            wm = w;
        }
    }
    if (align2) {
        ty = y + 0x700 - ln * sy / 2;
    } else {
        ty = y + 0x700;
    }
    ln = 0;
    str_c = NULL;
    str = sstr;
    c = *str++;
    while (c != 0xFFFF) {
        switch (align) {
        case 1:
            tx = x + 0x700 - lw[ln] / 2;
            break;
        case 2:
            tx = x + 0x700 - wm / 2;
            break;
        case 3:
            tx = x + 0x700 - lw[ln];
            break;
        default:
            tx = x + 0x700;
            break;
        }
        while (c != 0xFFFD && c != 0xFFFF) {
            if (c < 0x7FFF) {
                if (c == 0) {
                    tx += sx * 5 / def_w + 2;
                } else {
                    fontSet(c, tx, ty);
                    if (c >= 0xE0) {
                        tx += sx + 2;
                    } else {
                        tx += *(FontDataTable[font.fonttype] + c + 0x10) * sx / def_w + 2;
                    }
                }
            } else if ((ck = c & 0xFF00) == 0xFF00) {
                switch (c) {
                case 0xFFFE:
                    tx += sx + 2;
                    break;
                default:
                    if (c <= 0xFF63) {
                        fontSetColor(c - 0xFF00);
                    }
                    break;
                }
            } else if (ck == 0xFD00) {
                tx += c & 0xFF;
            } else if (ck == 0xFC00) {
                tx -= c & 0xFF;
            } else if (c == 0x8000) {
                str_c = (unsigned char *)str;
            }
            if (str_c) {
                c = fontGetCode(&str_c);
            } else {
                c = *str++;
            }
        }
        if (c == 0xFFFD) {
            ty += (int)sy;
            if (str_c) {
                c = fontGetCode(&str_c);
            } else {
                c = *str++;
            }
        }
        ln++;
    }
    font.hm = ln * sy;
    if (str_c) {
        str = (unsigned short *)((u_int)(str_c + 1) & ~1);
    }
    if (*str && !(*str & 0x8000)) {
        b_hm = font.hm;
        b_wm = wm;
        wm = fontPrintWord(str + 1, x, ty + sy + sy / 2 - 0x700, align, align2);
        font.hm += b_hm + sy;
        if (b_wm > wm) {
            wm = b_wm;
        }
    }
    return wm;
}

/** Prints a string with wide glyphs of size sx x sy, aligned by the font flags.
 * @param str the string
 * @param pos_x x position
 * @param pos_y y position
 * @param sx glyph width
 * @param sy glyph height */
/* Matching: the same code-fetch and glyph-width spellings as fontPrintWord. */
void fontPrintStrWide(unsigned short *str, int pos_x, int pos_y, int sx, int sy) {
    unsigned short x;
    unsigned short y;
    unsigned short w;
    unsigned short wm;
    unsigned short c;
    unsigned short ck;
    unsigned short ln;
    unsigned short def_w;
    unsigned short *sstr;
    unsigned char *str_c;
    int align;
    int align2;
    int lw[20];

    align = 2;
    if (font.flag & 0x100) {
        align = 1;
    }
    align2 = (font.flag & 0x200) ? 1 : 0;
    pos_x += 0x700;
    pos_y += 0x700;
    w = wm = 0;
    sstr = str;
    str_c = NULL;
    ln = 0;
    if (!str) {
        return;
    }
    def_w = FontSize[font.fonttype][0];
    c = *str++;
    while (c != 0xFFFF) {
        if (c < 0x7FFF) {
            if (c == 0) {
                w += sx * 5 / def_w;
            } else if (c >= 0xE0) {
                w += sx;
            } else {
                w += sx * *(FontDataTable[font.fonttype] + c + 0x10) / def_w;
            }
        } else if ((ck = c & 0xFF00) == 0xFF00) {
            switch (c) {
            case 0xFFFE:
                w += sx;
                break;
            case 0xFFFD:
                lw[ln++] = w;
                if (w > wm) {
                    wm = w;
                }
                w = 0;
                break;
            case 0xFFFC:
                align = 0;
                break;
            case 0xFFFB:
                align = 1;
                break;
            case 0xFFFA:
                align2 = 1;
                break;
            }
        } else if (ck == 0xFD00) {
            w += c & 0xFF;
        } else if (ck == 0xFC00) {
            w -= c & 0xFF;
        } else if ((c & 0xFE00) == 0xFA00) {
            pos_x = (c & 0x1FF) + 0x700;
            align = 2;
        } else if ((c & 0xFE00) == 0xF800) {
            pos_y = (c & 0x1FF) + 0x700;
        } else if (c == 0x8000) {
            str_c = (unsigned char *)str;
        }
        if (str_c) {
            c = fontGetCode(&str_c);
        } else {
            c = *str++;
        }
    }
    if (w != 0) {
        lw[ln++] = w;
        if (w > wm) {
            wm = w;
        }
    }
    font.right_x = (wm + 0x200) / 2 + 0x700;
    if (align2) {
        y = pos_y - ln * sy / 2;
    } else {
        y = pos_y;
    }
    ln = 0;
    str_c = NULL;
    str = sstr;
    c = *str++;
    for (; c != 0xFFFF; ln++) {
        switch (align) {
        case 1:
            x = pos_x - lw[ln] / 2;
            break;
        case 2:
            x = pos_x;
            break;
        default:
            x = pos_x - wm / 2;
            break;
        }
        while (c != 0xFFFD && c != 0xFFFF) {
            if (c < 0x7FFF) {
                if (c == 0) {
                    x += sx * 5 / def_w;
                } else {
                    fontSetWide(c, x, y, sx, sy);
                    if (c >= 0xE0) {
                        x += sx;
                    } else {
                        x += sx * *(FontDataTable[font.fonttype] + c + 0x10) / def_w;
                    }
                }
            } else if ((ck = c & 0xFF00) == 0xFF00) {
                switch (c) {
                case 0xFFFE:
                    x += sx;
                    break;
                default:
                    if (c <= 0xFF63) {
                        fontSetColor(c - 0xFF00);
                    }
                    break;
                }
            } else if (ck == 0xFD00) {
                x += c & 0xFF;
            } else if (ck == 0xFC00) {
                x -= c & 0xFF;
            } else if (c == 0x8000) {
                str_c = (unsigned char *)str;
            }
            if (str_c) {
                c = fontGetCode(&str_c);
            } else {
                c = *str++;
            }
        }
        if (c == 0xFFFD) {
            y += sy;
            if (str_c) {
                c = fontGetCode(&str_c);
            } else {
                c = *str++;
            }
        }
    }
    font.wm = wm;
    font.hm = ln * sy;
}

/** Measures the lines of a string.
 * @param buf result: the width of each line
 * @param str the string
 * @return the number of lines */
/*
 * Matching: reconstructed. The DWARF has sstr and sy, unused here, and the line table has gaps
 * where fontPrintWord sets them: dead copies of those statements (copied code), kept.
 */
int fontGetMesWidth(int *buf, unsigned short *str) {
    unsigned short w;
    unsigned short c;
    unsigned short ck;
    unsigned short ln;
    unsigned short sx;
    unsigned short sy;
    unsigned short def_w;
    unsigned short *sstr;
    unsigned short *str_stack;
    unsigned char *str_c;
    unsigned char *str_stack_c;

    if (!str) {
        return 0;
    }
    w = 0;
    sstr = str;
    str_c = NULL;
    str_stack = NULL;
    str_stack_c = NULL;
    ln = 0;
    if (font.flag & 0x400) {
        sx = font.wide_w;
        sy = font.wide_h;
    } else {
        sx = FontSize[font.fonttype][0];
        sy = FontSize[font.fonttype][1];
    }
    def_w = FontSize[font.fonttype][0];
    c = *str++;
    while (c != 0xFFFF) {
        if (c < 0x7FFF) {
            if (c == 0) {
                w += sx * 5 / def_w + 2;
            } else if (c >= 0xE0) {
                w += sx + 2;
            } else {
                w += *(FontDataTable[font.fonttype] + c + 0x10) * sx / def_w + 2;
            }
        } else if ((ck = c & 0xFF00) == 0xFF00) {
            switch (c) {
            case 0xFFFE:
                w += sx + 2;
                break;
            case 0xFFFD:
                buf[ln++] = w - 2;
                w = 0;
                break;
            default:
                if (c >= 0xFFE0 && c <= 0xFFE9) {
                    if (str_stack) {
                        printf("font: double stack!\n");
                    }
                    str_stack = str;
                    str_stack_c = str_c;
                    str = font.mes_v[c - 0xFFE0];
                    str_c = NULL;
                }
                break;
            }
        } else if (ck == 0xFD00) {
            w += c & 0xFF;
        } else if (ck == 0xFC00) {
            w -= c & 0xFF;
        } else if (c == 0x8000) {
            str_c = (unsigned char *)str;
        }
        if (str_c) {
            c = fontGetCode(&str_c);
        } else {
            c = *str++;
        }
        if (c == 0xFFFF && str_stack) {
            str = str_stack;
            str_c = str_stack_c;
            str_stack = NULL;
            if (str_c) {
                c = fontGetCode(&str_c);
            } else {
                c = *str++;
            }
        }
    }
    if (w != 0) {
        buf[ln++] = w - 2;
    }
    return ln;
}

/** Sets the text colour to one of the 17 palette entries (top and bottom colour).
 * @param num palette index (0-16) */
void fontSetColor(int num) {
    unsigned int font_color[17][2] = {
        { 0x00808080, 0x00808080 }, { 0x00806060, 0x00806060 }, { 0x00303080, 0x00303080 },
        { 0x00608060, 0x00608060 }, { 0x00308080, 0x00308080 }, { 0x00808030, 0x00808030 },
        { 0x00803080, 0x00803080 }, { 0x00A05050, 0x0050A050 }, { 0x00000030, 0x00202050 },
        { 0x00000000, 0x00000000 }, { 0x00909090, 0x00202020 }, { 0x002810E0, 0x002810E0 },
        { 0x00A05870, 0x00A05870 }, { 0x0038D088, 0x0038D088 }, { 0x0010F8F8, 0x0010F8F8 },
        { 0x00A8B820, 0x00A8B820 }, { 0x00202020, 0x00202020 },
    };

    /* Matching: the assert bakes its original line number into the object. */
#line 1508
    fjAssert(num >= 0 && num <= 16);
    font.rgb_u = font_color[num][0];
    font.rgb_d = font_color[num][1];
}

/** Sets the text colour and alpha directly.
 * @param r red
 * @param g green
 * @param b blue
 * @param alp alpha */
void fontSetColorDirect(unsigned char r, unsigned char g, unsigned char b, unsigned char alp) {
    font.rgb_u = font.rgb_d = r | (g << 8) | (b << 16);
    font.alpha = alp;
}

/** Sets the text alpha.
 * @param alp alpha */
void fontSetAlpha(unsigned char alp) {
    font.alpha = alp;
}

static void fontBarBlink(void) {
    if ((font.bar_blink += shGetDT()) >= 1.0f) {
        font.bar_blink -= 1.0f;
    }
}

#define GS_XYZ(x, y, z) ((unsigned long)(x) | ((unsigned long)(y) << 16) | ((unsigned long)(z) << 32))

/** Builds the GS packet of the queued glyphs (normal, wide, blank boxes, lines and selection
 * bars) through the scratchpad.
 * @return the packet, for d1cSend */
u_long128 *fontFlush(void) {
    int i;
    int x;
    int y;
    int z;
    int n;
    int gn;
    int gn2;
    int sn;
    unsigned short u;
    unsigned short v;
    unsigned int alp;
    unsigned int a1;
    unsigned long *ptag;
    unsigned long *pgtag;
    unsigned long *pgtag2;
    unsigned long *pCur;
    struct FONT_STREAM_DATA *fstream;
    struct FONT_STREAM_DATA *bbox[16];
    struct FONT_STREAM_DATA *hline[16];
    struct WFONT_STREAM_DATA STR;
    int base;
    int bbnum;
    int hlnum;
    u_long128 *ptop;
    u_long128 *ppos;
    unsigned int *pui;
    struct WFONT_STREAM_DATA *wstream;

    bbnum = 0;
    hlnum = 0;
    if (!font.tex0 || (font.flag & 8) || (!font.st_num && !font.w_st_num)) {
        pCur = (unsigned long *)(((u_int)spack.pk_last + 0x3F) & ~0x3F);
        pCur[0] = 0x70000000;
        pCur[1] = 0;
        spack.pk_last = pCur + 2;
        return (u_long128 *)((u_int)pCur & 0x0FFFFFFF);
    }
    pui = (unsigned int *)(((u_int)spack.pk_last + 0x3F) & ~0x3F);
    ppos = (u_long128 *)pui;
    alp = font.alpha_base;
    base = 0;
    pCur = (unsigned long *)0x70000000;
    fstream = font.stream;
    z = font.base_z;
    spkDmaWaittoSPR(0);
    spkDmaWaitfromSPR(0);
    ptop = (u_long128 *)((u_int)pui & 0x0FFFFFFF);
    SyncDCache(ptop, spack.packet + 0x8000);
    InvalidDCache(ptop, spack.packet + 0x8000);
    *pCur++ = 0;
    *pCur++ = 0;
    if (font.wait_type == 4) {
        font.pCur = pCur;
        if (font.sel_max == -1) {
            fontPutYesNoSelectBar();
        } else {
            fontPutSelectBar();
        }
        pCur = font.pCur;
    }
    pCur[0] = 0x10AE400000000009;
    pCur[1] = 0xE;
    pCur[2] = 0;
    pCur[3] = 0x3F;
    pCur[4] = font.tex0;
    pCur[5] = 6;
    pCur[6] = 5;
    pCur[7] = 8;
    pCur[8] = 0x44;
    pCur[9] = 0x42;
    pCur[10] = 0;
    pCur[11] = 0x49;
    pCur[12] = 0x30000;
    pCur[13] = 0x47;
    pCur[14] = 0;
    pCur[15] = 0x14;
    pCur[16] = 0x13A0001C0;
    pCur[17] = 0x4E;
    pCur[18] = 1;
    pCur[19] = 0x46;
    pgtag = (unsigned long *)((u_int)pui + ((u_int)&pCur[20] & 0x1FFF));
    pCur[20] = 0xA400000000008000;
    pCur[21] = 0x0053531D3D31;
    pCur += 22;
    i = font.st_num;
    while (i > 0) {
        if (pCur > (unsigned long *)(base + 0x70001F00)) {
            n = ((u_int)pCur & 0x1FFF) >> 4;
            spkDmafromSPR(n, base, ppos);
            ppos += n;
            base ^= 0x2000;
            pCur = (unsigned long *)((base & 0x3FFF) + 0x70000000);
        }
        v = fstream->v;
        font.pCur = pCur;
        if (v == 0xFFF7) {
            bbox[bbnum++] = fstream;
        } else if (v == 0xFE00) {
            hline[hlnum++] = fstream;
        } else {
            a1 = fstream->rgb_u >> 24;
            a1 = (a1 * alp >> 7) << 24;
            STR.w = STR.vw = fstream->w;
            STR.h = STR.vh = fstream->h;
            STR.u = fstream->u;
            STR.v = v;
            sn = (fstream->rgb_d >> 24) & 0xFF;
            if (sn < 4) {
                STR.x = fstream->x + 16;
                STR.y = fstream->y + 16;
                STR.rgb_u = STR.rgb_d = font.rgb_s[sn] | a1;
                fontPut(&STR, -1);
            }
            STR.x = fstream->x;
            STR.y = fstream->y;
            STR.rgb_u = (fstream->rgb_u & 0xFFFFFF) | a1;
            STR.rgb_d = (fstream->rgb_d & 0xFFFFFF) | a1;
            fontPut(&STR, 0);
        }
        pCur = font.pCur;
        fstream++;
        i--;
    }
    n = ((u_int)pCur & 0x1FFF) >> 4;
    if (n > 0) {
        spkDmafromSPR(n, base, ppos);
        ppos += n;
        base ^= 0x2000;
        pCur = (unsigned long *)((base & 0x3FFF) + 0x70000000);
    }
    gn = ((u_int)ppos - (u_int)pgtag - 16) / 8 / 10;
    if (gn == 0) {
        ppos--;
    }
    gn2 = 0;
    if (font.w_st_num) {
        wstream = font.w_stream;
        pCur[0] = 0x10AE400000000001;
        pCur[1] = 0xE;
        pCur[2] = 0xFFFFFFFF00000061;
        pCur[3] = 0x14;
        pgtag2 = (unsigned long *)((u_int)ppos + ((u_int)&pCur[4] & 0x1FFF));
        pCur[4] = 0xA400000000008000;
        pCur[5] = 0x0053531D3D31;
        pCur += 6;
        for (i = font.w_st_num; i > 0; i--) {
            if (pCur > (unsigned long *)(base + 0x70001F00)) {
                n = ((u_int)pCur & 0x1FFF) >> 4;
                spkDmafromSPR(n, base, ppos);
                ppos += n;
                base ^= 0x2000;
                pCur = (unsigned long *)((base & 0x3FFF) + 0x70000000);
            }
            font.pCur = pCur;
            a1 = wstream->rgb_u >> 24;
            a1 = (a1 * alp >> 7) << 24;
            STR.w = wstream->w;
            STR.h = wstream->h;
            STR.u = wstream->u;
            STR.v = wstream->v;
            STR.vw = wstream->vw;
            STR.vh = wstream->vh;
            sn = (wstream->rgb_d >> 24) & 0xFF;
            if (sn < 4) {
                STR.x = wstream->x + 16;
                STR.y = wstream->y + 16;
                STR.rgb_u = STR.rgb_d = font.rgb_s[sn] | a1;
                fontPut(&STR, -1);
            }
            STR.x = wstream->x;
            STR.y = wstream->y;
            STR.rgb_u = (wstream->rgb_u & 0xFFFFFF) | a1;
            STR.rgb_d = (wstream->rgb_d & 0xFFFFFF) | a1;
            fontPut(&STR, 0);
            pCur = font.pCur;
            wstream++;
        }
        n = ((u_int)pCur & 0x1FFF) >> 4;
        if (n > 0) {
            spkDmafromSPR(n, base, ppos);
            ppos += n;
            base ^= 0x2000;
            pCur = (unsigned long *)((base & 0x3FFF) + 0x70000000);
        }
        gn2 = ((u_int)ppos - (u_int)pgtag2 - 16) / 8 / 10;
        if (gn2 == 0) {
            ppos--;
        }
    }
    if (bbnum) {
        ptag = pCur;
        pCur[0] = 0xC400000000008000;
        pCur[1] = 0x55D10551DD10;
        pCur += 2;
        for (i = 0; i < bbnum; i++) {
            fstream = bbox[i];
            a1 = fstream->rgb_u >> 24;
            a1 = ((alp * (a1 * 160)) >> 14) << 24;
            x = fstream->x + 16 + font.base_x;
            y = fstream->y + 48 + font.base_y;
            u = fstream->u - 32 + font.base_x;
            v = y + fstream->h - 64;
            pCur[0] = 0x4C;
            pCur[1] = (fstream->rgb_u & 0xFFFFFF) | a1;
            pCur[2] = GS_XYZ(x, y, z);
            pCur[3] = GS_XYZ(u, y, z);
            pCur[4] = (fstream->rgb_d & 0xFFFFFF) | a1;
            pCur[5] = GS_XYZ(x, v, z);
            pCur[6] = GS_XYZ(u, v, z);
            pCur[7] = 0x42;
            sn = (fstream->rgb_d >> 24) & 0xFF;
            if (sn < 4) {
                pCur[8] = font.rgb_s[sn] | a1;
                pCur += 9;
            } else {
                pCur[8] = 0;
                pCur += 9;
            }
            pCur[0] = GS_XYZ(x + 16, v, z);
            pCur[1] = GS_XYZ(u, v, z);
            pCur[2] = GS_XYZ(u, y + 16, z);
            pCur += 3;
        }
        *ptag |= (((u_int)pCur - (u_int)ptag - 16) >> 3) / 12;
        if ((u_int)pCur & 0xF) {
            *pCur++ = 0;
        }
    }
    if (hlnum) {
        ptag = pCur;
        pCur[0] = 0x7400000000008000;
        pCur[1] = 0x5D15D10;
        pCur += 2;
        for (i = 0; i < hlnum; i++) {
            fstream = hline[i];
            a1 = fstream->rgb_u >> 24;
            a1 = a1 * alp * 10 >> 10;
            if (a1 > 0xFF) {
                a1 = 0xFF;
            }
            x = fstream->x + font.base_x;
            y = fstream->y + font.base_y;
            u = fstream->u + font.base_x;
            pCur[0] = 0x46;
            if (fstream->rgb_d < 4) {
                pCur[1] = font.rgb_s[fstream->rgb_d] | (a1 << 24);
                pCur += 2;
            } else {
                pCur[1] = 0;
                pCur += 2;
            }
            pCur[0] = GS_XYZ(x + 16, y + 32, z);
            pCur[1] = GS_XYZ(u + 16, y + 48, z);
            pCur[2] = (fstream->rgb_u & 0xFFFFFF) | (a1 << 24);
            pCur[3] = GS_XYZ(x, y, z);
            pCur[4] = GS_XYZ(u, y + 32, z);
            pCur += 5;
        }
        *ptag |= (((u_int)pCur - (u_int)ptag - 16) >> 3) / 7;
        if ((u_int)pCur & 0xF) {
            *pCur++ = 0;
        }
    }
    n = ((u_int)pCur & 0x1FFF) >> 4;
    if (n > 0) {
        spkDmafromSPR(n, base, ppos);
        ppos += n;
    }
    n = ppos - (u_long128 *)pui;
    spkDmaWaitfromSPR(0);
    pui[0] = (n - 1) | 0x70000000;
    pui[3] = (n - 1) | 0x50000000;
    if (gn) {
        *pgtag |= gn;
    }
    if (gn2) {
        *pgtag2 |= gn2;
    }
    spack.pk_last = (unsigned long *)ppos;
    return ptop;
}

/** fontFlush without the scratchpad.
 * @return the packet, for d1cSend */
u_long128 *fontFlushNoSPR(void) {
    int i;
    int x;
    int y;
    int z;
    int n;
    int gn;
    int sn;
    unsigned short u;
    unsigned short v;
    unsigned int alp;
    unsigned int a1;
    unsigned long *ptag;
    unsigned long *pgtag;
    unsigned long *pCur;
    struct FONT_STREAM_DATA *fstream;
    struct FONT_STREAM_DATA *bbox[16];
    struct FONT_STREAM_DATA *hline[16];
    struct WFONT_STREAM_DATA STR;
    int bbnum;
    int hlnum;
    u_long128 *ptop;
    u_long128 *ppos;
    unsigned int *pui;

    bbnum = 0;
    hlnum = 0;
    if (!font.tex0 || (font.flag & 8) || (!font.st_num && !font.w_st_num)) {
        pCur = (unsigned long *)(((u_int)spack.pk_last + 0x3F) & ~0x3F);
        pCur[0] = 0x70000000;
        pCur[1] = 0;
        spack.pk_last = pCur + 2;
        return (u_long128 *)((u_int)pCur & 0x0FFFFFFF);
    }
    pui = (unsigned int *)(((u_int)spack.pk_last + 0x3F) & ~0x3F);
    ppos = (u_long128 *)pui; /* Matching: dead; the original's DWARF has ppos, as in fontFlush */
    pCur = (unsigned long *)pui;
    alp = font.alpha_base;
    fstream = font.stream;
    z = font.base_z;
    ptop = (u_long128 *)((u_int)pui & 0x0FFFFFFF);
    SyncDCache(ptop, spack.packet + 0x8000);
    InvalidDCache((u_long128 *)((u_int)pui & 0x0FFFFFFF), spack.packet + 0x8000);
    pCur[0] = 0;
    pCur[1] = 0;
    pCur[2] = 0x10AE400000000009;
    pCur[3] = 0xE;
    pCur[4] = 0;
    pCur[5] = 0x3F;
    pCur[6] = font.tex0;
    pCur[7] = 6;
    pCur[8] = 5;
    pCur[9] = 8;
    pCur[10] = 0x44;
    pCur[11] = 0x42;
    pCur[12] = 0;
    pCur[13] = 0x49;
    pCur[14] = 0x30000;
    pCur[15] = 0x47;
    pCur[16] = 0;
    pCur[17] = 0x14;
    pCur[18] = 0x13A0001C0;
    pCur[19] = 0x4E;
    pCur[20] = 1;
    pCur[21] = 0x46;
    pgtag = &pCur[22];
    pCur[22] = 0xA400000000000000;
    pCur[23] = 0x0053531D3D31;
    pCur += 24;
    i = font.st_num;
    while (i > 0) {
        v = fstream->v;
        font.pCur = pCur;
        if (v == 0xFFF7) {
            bbox[bbnum++] = fstream;
        } else if (v == 0xFE00) {
            hline[hlnum++] = fstream;
        } else {
            a1 = fstream->rgb_u >> 24;
            a1 = (a1 * alp >> 7) << 24;
            STR.w = STR.vw = fstream->w;
            STR.h = STR.vh = fstream->h;
            STR.u = fstream->u;
            STR.v = v;
            sn = (fstream->rgb_d >> 24) & 0xFF;
            if (sn < 4) {
                STR.x = fstream->x + 16;
                STR.y = fstream->y + 16;
                STR.rgb_u = STR.rgb_d = font.rgb_s[sn] | a1;
                fontPut(&STR, -1);
            }
            STR.x = fstream->x;
            STR.y = fstream->y;
            STR.rgb_u = (fstream->rgb_u & 0xFFFFFF) | a1;
            STR.rgb_d = (fstream->rgb_d & 0xFFFFFF) | a1;
            fontPut(&STR, 0);
        }
        pCur = font.pCur;
        fstream++;
        i--;
    }
    gn = ((u_int)pCur - (u_int)pgtag - 16) / 8 / 10;
    if (gn == 0) {
        pCur -= 2;
    }
    if (font.w_st_num) {
        unsigned long *pgtag2;
        struct WFONT_STREAM_DATA *wstream;

        wstream = font.w_stream;
        pCur[0] = 0x10AE400000000001;
        pCur[1] = 0xE;
        pCur[2] = 0xFFFFFFFF00000061;
        pCur[3] = 0x14;
        pgtag2 = &pCur[4];
        pCur[4] = 0xA400000000008000;
        pCur[5] = 0x0053531D3D31;
        font.pCur = pCur + 6;
        for (i = font.w_st_num; i > 0; i--) {
            a1 = wstream->rgb_u >> 24;
            a1 = (a1 * alp >> 7) << 24;
            STR.w = wstream->w;
            STR.h = wstream->h;
            STR.u = wstream->u;
            STR.v = wstream->v;
            STR.vw = wstream->vw;
            STR.vh = wstream->vh;
            sn = (wstream->rgb_d >> 24) & 0xFF;
            if (sn < 4) {
                STR.x = wstream->x + 16;
                STR.y = wstream->y + 16;
                STR.rgb_u = STR.rgb_d = font.rgb_s[sn] | a1;
                fontPut(&STR, -1);
            }
            STR.x = wstream->x;
            STR.y = wstream->y;
            STR.rgb_u = (wstream->rgb_u & 0xFFFFFF) | a1;
            STR.rgb_d = (wstream->rgb_d & 0xFFFFFF) | a1;
            fontPut(&STR, -1);
            wstream++;
        }
        pCur = font.pCur;
        n = ((u_int)pCur - (u_int)pgtag2 - 16) / 8 / 10;
        if (n == 0) {
            pCur -= 2;
        } else {
            *pgtag2 |= n;
        }
    }
    if (bbnum) {
        ptag = pCur;
        pCur[0] = 0xC400000000008000;
        pCur[1] = 0x55D10551DD10;
        pCur += 2;
        for (i = 0; i < bbnum; i++) {
            fstream = bbox[i];
            a1 = fstream->rgb_u >> 24;
            a1 = ((alp * (a1 * 160)) >> 14) << 24;
            x = fstream->x + 16 + font.base_x;
            y = fstream->y + 48 + font.base_y;
            u = fstream->u - 32 + font.base_x;
            v = y + fstream->h - 64;
            pCur[0] = 0x4C;
            pCur[1] = (fstream->rgb_u & 0xFFFFFF) | a1;
            pCur[2] = GS_XYZ(x, y, z);
            pCur[3] = GS_XYZ(u, y, z);
            pCur[4] = (fstream->rgb_d & 0xFFFFFF) | a1;
            pCur[5] = GS_XYZ(x, v, z);
            pCur[6] = GS_XYZ(u, v, z);
            pCur[7] = 0x42;
            sn = (fstream->rgb_d >> 24) & 0xFF;
            if (sn < 4) {
                pCur[8] = font.rgb_s[sn] | a1;
                pCur += 9;
            } else {
                pCur[8] = 0;
                pCur += 9;
            }
            pCur[0] = GS_XYZ(x + 16, v, z);
            pCur[1] = GS_XYZ(u, v, z);
            pCur[2] = GS_XYZ(u, y + 16, z);
            pCur += 3;
        }
        *ptag |= (((u_int)pCur - (u_int)ptag - 16) >> 3) / 12;
        if ((u_int)pCur & 0xF) {
            *pCur++ = 0;
        }
    }
    if (hlnum) {
        ptag = pCur;
        pCur[0] = 0x7400000000008000;
        pCur[1] = 0x5D15D10;
        pCur += 2;
        for (i = 0; i < hlnum; i++) {
            fstream = hline[i];
            a1 = fstream->rgb_u >> 24;
            a1 = a1 * alp * 10 >> 10;
            if (a1 > 0xFF) {
                a1 = 0xFF;
            }
            x = fstream->x + font.base_x;
            y = fstream->y + 48 + font.base_y;
            u = fstream->u + font.base_y;
            pCur[0] = 0x46;
            if (fstream->rgb_d < 4) {
                pCur[1] = font.rgb_s[fstream->rgb_d] | (a1 << 24);
                pCur += 2;
            } else {
                pCur[1] = 0;
                pCur += 2;
            }
            pCur[0] = GS_XYZ(x + 16, y + 32, z);
            pCur[1] = GS_XYZ(u + 16, y + 48, z);
            pCur[2] = (fstream->rgb_u & 0xFFFFFF) | (a1 << 24);
            pCur[3] = GS_XYZ(x, y, z);
            pCur[4] = GS_XYZ(u, y + 32, z);
            pCur += 5;
        }
        *ptag |= (((u_int)pCur - (u_int)ptag - 16) >> 3) / 7;
        if ((u_int)pCur & 0xF) {
            *pCur++ = 0;
        }
    }
    n = (u_long128 *)pCur - (u_long128 *)pui;
    pui[0] = (n - 1) | 0x70000000;
    pui[3] = (n - 1) | 0x50000000;
    if (gn) {
        *pgtag |= gn;
    }
    spack.pk_last = pCur;
    return ptop;
}

/** Adds one wide glyph to the packet as a sprite (an asm block).
 * @param pstr the glyph
 * @param z depth offset */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace, and its DWARF has the parameters pstr and z. */
void fontPut(struct WFONT_STREAM_DATA *pstr, int z) {
    asm {
        .set noreorder
        la      v1, font.pCur
        lhu     t0, 0x0(pstr)
        lhu     t1, 0x2(pstr)
        lw      t2, 0x4(v1)
        lw      t3, 0x8(v1)
        add     t0, t0, t2
        add     t1, t1, t3
        lhu     t2, 0x4(pstr)
        lhu     t3, 0x6(pstr)
        add     t2, t0, t2
        add     t3, t1, t3
        slti    t4, t2, 0x7000
        slti    t5, t3, 0x7000
        ori     t6, zero, 0x9000
        paddub  t7, t6, zero
        bnez    t4, L_end
        slt     t6, t6, t0
        bnez    t5, L_end
        slt     t7, t7, t1
        bnez    t6, L_end
        lw      t4, 0x0(v1)
        bnez    t7, L_end
        lw      t5, 0xC(v1)
        lwu     t6, 0xC(pstr)
        lwu     t7, 0x10(pstr)
        add     t5, t5, z
        sd      t6, 0x0(t4)
        sd      t7, 0x28(t4)
        pextlh  t6, t1, t0
        pextlh  t7, t1, t2
        pextlw  t6, t5, t6
        pextlw  t7, t5, t7
        sd      t6, 0x10(t4)
        sd      t7, 0x20(t4)
        pextlh  t6, t3, t0
        pextlh  t7, t3, t2
        pextlw  t6, t5, t6
        pextlw  t7, t5, t7
        sd      t6, 0x38(t4)
        sd      t7, 0x48(t4)
        lhu     t0, 0x8(pstr)
        lhu     t1, 0xA(pstr)
        sll     t0, t0, 4
        sll     t1, t1, 4
        lhu     t2, 0x14(pstr)
        lhu     t3, 0x16(pstr)
        add     t2, t0, t2
        add     t3, t1, t3
        addi    t5, t3, -0x8
        pextlh  t6, t1, t0
        pextlh  t7, t1, t2
        sd      t6, 0x8(t4)
        sd      t7, 0x18(t4)
        pextlh  t6, t5, t0
        pextlh  t7, t3, t2
        sd      t6, 0x30(t4)
        sd      t7, 0x40(t4)
        addiu   t4, t4, 0x50
        sw      t4, 0x0(v1)
    L_end:
        .set reorder
    }
}

/** Adds the selection bar of the current choice to the packet. */
void fontPutSelectBar(void) {
    int x0;
    int y0;
    int x1;
    int y1;
    int z;
    int a;
    unsigned long *pCur;

    pCur = font.pCur;
    x0 = (font.sel_xl << 4) + font.base_x;
    x1 = (font.sel_xr << 4) + font.base_x;
    y0 = ((font.sel_yu[font.sel_now] + 1) << 4) + font.base_y;
    y1 = ((font.sel_yd[font.sel_now] + 1) << 4) + font.base_y;
    z = font.base_z;
    fontBarBlink();
    if (font.bar_blink > 0.5f) {
        a = ftoi((1.0f - font.bar_blink) / 0.5f * 64.0f);
    } else {
        a = ftoi(font.bar_blink / 0.5f * 64.0f);
    }
    *pCur++ = 0x1023400000008003;
    *pCur++ = 0xE;
    *pCur++ = 0x44;
    *pCur++ = 0x42;
    *pCur++ = 0x30000;
    *pCur++ = 0x47;
    *pCur++ = 0;
    *pCur++ = 0x49;
    *pCur++ = 0x3400000000008001;
    *pCur++ = 0x551;
    *pCur++ = ((long)(a + 0x10) << 24) | 0xC02010;
    *pCur++ = fontXYZ(x0, y0, z);
    *pCur++ = fontXYZ(x1, y1, z);
    *pCur++ = 0;
    font.pCur = pCur;
}

/** Adds the selection bar of the yes/no choice to the packet. */
void fontPutYesNoSelectBar(void) {
    int x0;
    int y0;
    int x1;
    int y1;
    int z;
    int a;
    unsigned long *pCur;

    pCur = font.pCur;
    if (!font.sel_now) {
        x0 = 0x7C80;
    } else {
        x0 = 0x8380;
    }
    x0 += font.base_x;
    x1 = x0 + ((font.sel_yu[1] / 2 + 4) << 4);
    x0 = x0 - ((font.sel_yu[1] / 2 + 4) << 4);
    y0 = ((font.sel_yu[0] + 1) << 4) + font.base_y;
    y1 = ((font.sel_yd[0] + 1) << 4) + font.base_y;
    z = font.base_z;
    fontBarBlink();
    if (font.bar_blink > 0.5f) {
        a = ftoi((1.0f - font.bar_blink) / 0.5f * 64.0f);
    } else {
        a = ftoi(font.bar_blink / 0.5f * 64.0f);
    }
    *pCur++ = 0x1023400000008003;
    *pCur++ = 0xE;
    *pCur++ = 0x44;
    *pCur++ = 0x42;
    *pCur++ = 0x30000;
    *pCur++ = 0x47;
    *pCur++ = 0;
    *pCur++ = 0x49;
    *pCur++ = 0x3400000000008001;
    *pCur++ = 0x551;
    *pCur++ = ((long)(a + 0x10) << 24) | 0xC02010;
    *pCur++ = fontXYZ(x0, y0, z);
    *pCur++ = fontXYZ(x1, y1, z);
    *pCur++ = 0;
    font.pCur = pCur;
}

/** Sets the GS addresses of the font texture and CLUT.
 * @param texadr texture base pointer
 * @param clutadr CLUT base pointer
 * @return the DMA packet that uploads them */
void *fontTexLoad(int texadr, int clutadr) {
    font_dma_data[4] = GS_SET_BITBLTBUF(0, 0, 0, texadr, 8, GS_PSM_4);
    font_dma_data[20] = GS_SET_BITBLTBUF(0, 0, 0, clutadr, 1, GS_PSM_32);
    font.tex0 = GS_SET_TEX0(texadr, 8, GS_PSM_4, 9, 9, 1, 0, clutadr, 0, 0, 0, 1);
    return font_dma_data;
}

/** Returns the packet that restores the GS environment after the text. */
void *fontAfterEnv(void) {
    return font_after_env;
}

/** Per-frame font update: preloads glyphs, handles the choice cursor and the confirm/cancel
 * buttons, and advances timed messages. */
void fontEachTurn(void) {
    if (!font.tex0) {
        return;
    }
    if (font.prl_count) {
        fontPreload();
    } else if (font.bottom < 400) {
        fontLoad(++font.preload);
    }
    if (font.flag & 2) {
        font.flag &= ~2;
    } else if (!(font.flag & 0x20)) {
        if (font.sel_max == -1) {
            if (shPadTrigger(0, 0x200)) {
                fontSelectUp();
            }
            if (shPadTrigger(0, 0x100)) {
                fontSelectDown();
            }
        } else {
            if (shPadRepeat(0, 0x400)) {
                fontSelectUp();
            }
            if (shPadRepeat(0, 0x800)) {
                fontSelectDown();
            }
        }
        if (shPadTrigger(0, key_config.enter)) {
            fontPushButton();
        }
        if (shPadTrigger(0, key_config.cancel)) {
            fontPushButton2();
        }
    }
    if ((font.wait_type & 7) && font.wait_type != 4) {
        if (font.wait_count > 0 && !(font.flag & 0x10)) {
            font.wait_count -= (short)shGetDF();
        }
        if (font.wait_count == 0) {
            if (font.wait_type & 8) {
                font.st_num = 0;
                font.sel_max = 0;
                font.wait_type = 0;
            } else {
                fontNextMessage();
            }
        }
    }
}

/** Loads up to 20 glyphs of the preload string (font.prl_str) into the cache, continuing where
 * the last call stopped; counts down font.prl_count at each string end. */
void fontPreload(void) {
    unsigned short c;
    unsigned short *str;
    unsigned char *str_c;
    int n;

    n = 0;
    str = font.prl_str;
    str_c = (font.flag & 0x40) ? (unsigned char *)str : NULL;
    font.flag &= ~4;
    while (1) {
        c = str_c ? fontGetCode(&str_c) : *str++;
        if (c < 0x7FFF && c != 0) {
            fontLoad(c);
            if (font.flag & 4) {
                break;
            }
            if (++n >= 20) {
                break;
            }
        } else if (c == 0x8000) {
            font.flag |= 0x40;
            str_c = (unsigned char *)str;
        } else if (c == 0xFFFF) {
            if (str_c) {
                str = (unsigned short *)((u_int)(str_c + 1) & ~1);
                font.flag &= ~0x40;
                str_c = NULL;
            }
            c = *str++;
            if (c == 0 || c >= 0x8000) {
                if (--font.prl_count == 0) {
                    return;
                }
            }
        }
    }
    if (str_c) {
        str = (unsigned short *)str_c;
    }
    font.prl_str = str;
}

/** Copies a string into message slot num (0-9).
 * @param num the slot
 * @param str the string (ended by 0xFFFF) */
void fontSetMes(int num, unsigned short *str) {
    if (num < 0 || num > 9) {
        printf("fontSetMes: Illegal number!\n");
        num = 0;
    }
    fontCopyMessage(font.mes_v[num], str);
}

/** Copies a font code string, including its 0xFFFF end.
 * @param pto destination
 * @param pfrom source */
void fontCopyMessage(unsigned short *pto, unsigned short *pfrom) {
    unsigned short n;

    do {
        n = *pfrom++;
        *pto++ = n;
    } while (n != 0xFFFF);
}

/** Confirm button: picks the choice (if one is open) or skips the message wait. */
void fontPushButton(void) {
    if (font.wait_type == 4) {
        font.wait_type = 5;
        font.st_num = 0;
    } else if ((font.wait_type & 7) != 2 && !(font.flag & 0x10)) {
        font.wait_count = 0;
    }
}

/** Cancel button: skips the message wait. */
void fontPushButton2(void) {
    if ((font.wait_type & 7) != 2 && !(font.flag & 0x10)) {
        font.wait_count = 0;
    }
}

/** Moves the choice cursor up (wrapping), with its sound. */
void fontSelectUp(void) {
    if (font.wait_type == 4) {
        SeCall(10000, 1.0f, 0);
        if (font.sel_max == -1) {
            font.sel_now = 0;
        } else if (--font.sel_now < 0) {
            font.sel_now = font.sel_max - 1;
        }
    }
}

/** Moves the choice cursor down (wrapping), with its sound. */
void fontSelectDown(void) {
    if (font.wait_type == 4) {
        SeCall(10000, 1.0f, 0);
        if (font.sel_max == -1) {
            font.sel_now = 1;
        } else if (++font.sel_now >= font.sel_max) {
            font.sel_now = 0;
        }
    }
}

/** Returns message num of a message file.
 * @param str the message file (str[0]: number of messages; NULL: none)
 * @param num message number
 * @return the message, or NULL */
unsigned short *fontGetMesAdr(unsigned short *str, unsigned short num) {
    if (!str) {
        return NULL;
    }
    if (num >= str[0]) {
        printf("message number over! (%d/%d)\n", num, str[0]);
        return NULL;
    }
    return str + str[num + 1];
}

/** Shows message num of a message file (fontMessage).
 * @param str the message file
 * @param num message number */
void fontMessageNum(unsigned short *str, unsigned short num) {
    fontMessage(fontGetMesAdr(str, num));
}

/** Starts showing a message page by page.
 * @param str the message (NULL: stop) */
void fontMessage(unsigned short *str) {
    if (!str) {
        font.mes_str_now = NULL;
        return;
    }
    font.mes_str = str;
    font.flag &= ~1;
    fontNextMessage();
    if (!font.prl_count && font.wait_type > 0 && font.wait_type < 8) {
        font.prl_str = font.mes_str;
        font.prl_count = 1;
        font.flag &= ~0x40;
    }
}

/** Moves on to the next page of the current message, or ends it. */
void fontNextMessage(void) {
    unsigned int wm;

    font.st_num = 0;
    if (font.flag & 1) {
        font.mes_str_now = NULL;
        return;
    }
    fontSetColor(0);
    font.flag &= ~8;
    font.mes_str_now = font.mes_str;
    wm = fontPrintStrMain(&font.mes_str, 0);
    if (font.sel_max) {
        font.wait_type = 4;
        font.wait_count = -1;
    } else {
        font.wait_type = wm >> 12;
        if ((font.wait_type & 7) == 1 || (font.wait_type & 7) == 0) {
            font.wait_count = -1;
        } else {
            font.wait_count = (wm & 0xFFF) * 60 / 60;
        }
    }
    font.flag |= 2;
}

/** Returns the message state.
 * @return -1 while a message is showing, -2 when none is, the choice picked once one was made */
int fontGetStatus(void) {
    if (font.wait_type == 5) {
        return font.sel_now;
    }
    return font.st_num == 0 ? -2 : -1;
}

/** Turns on wide mode: glyphs drawn at w x h.
 * @param w width
 * @param h height */
void fontWide(unsigned short w, unsigned short h) {
    font.flag |= 0x400;
    font.wide_w = w;
    font.wide_h = h;
}

/** Centres all lines horizontally. */
void fontAllCenterOn(void) {
    font.flag |= 0x100;
}

/** Stops centring all lines horizontally. */
void fontAllCenterOff(void) {
    font.flag &= ~0x100;
}

/** Turns on the second alignment flag (0x200; fontPrintStrWide passes it to fontPrintWord). */
void fontAllCenter2On(void) {
    font.flag |= 0x200;
}

/** Turns off the second alignment flag (0x200). */
void fontAllCenter2Off(void) {
    font.flag &= ~0x200;
}

/** Turns the text shadow off (shadow_now + 4). */
void fontShadowOff(void) {
    if (font.shadow_now < 4) {
        font.shadow_now += 4;
    }
}

/** Selects the narrow font (16x24). */
void fontCrushOn(void) {
    font.fonttype = 1;
}

/** Selects the normal font (20x30). */
void fontCrushOff(void) {
    font.fonttype = 0;
}

/** Clears the extra (mfont) glyphs. */
void mfontClear(void) {
    font.m_st_num = 0;
}

/** Builds the GS packet of the extra (mfont) glyphs.
 * @return the packet, for d1cSend */
u_long128 *mfontFlush(void) {
    int i;
    int z;
    int n;
    unsigned short x;
    unsigned short y;
    unsigned short w;
    unsigned short h;
    unsigned short bx;
    unsigned short by;
    unsigned short u;
    unsigned short v;
    unsigned long *pCur;
    u_long128 *ptop;
    u_long128 *ppos;
    int base;
    unsigned int *pui;
    struct MFONT_STREAM_DATA *fstream;

    if ((font.flag & 0x800) || !font.m_st_num) {
        pCur = (unsigned long *)(((u_int)spack.pk_last + 0x3F) & ~0x3F);
        pui = (unsigned int *)((u_int)pCur & 0x0FFFFFFF);
        *pCur++ = 0x70000000;
        *pCur++ = 0;
        spack.pk_last = pCur;
        return (u_long128 *)pui;
    }
    pui = (unsigned int *)(((u_int)spack.pk_last + 0x3F) & ~0x3F);
    ppos = (u_long128 *)pui;
    base = 0;
    pCur = (unsigned long *)0x70000000;
    fstream = font.m_stream;
    z = font.m_base_z;
    bx = font.m_base_x;
    by = font.m_base_y;
    w = font.m_w << 3;
    h = font.m_h << 3;
    spkDmaWaittoSPR(0);
    spkDmaWaitfromSPR(0);
    ptop = (u_long128 *)((u_int)pui & 0x0FFFFFFF);
    SyncDCache(ptop, spack.packet + 0x8000);
    InvalidDCache((void *)((u_int)pui & 0x0FFFFFFF), spack.packet + 0x8000);
    *pCur++ = 0;
    *pCur++ = 0;
    *pCur++ = 0x10AB400000000009;
    *pCur++ = 0xE;
    *pCur++ = 0;
    *pCur++ = 0x3F;
    *pCur++ = font.tex0;
    *pCur++ = 6;
    *pCur++ = 5;
    *pCur++ = 8;
    *pCur++ = 0x44;
    *pCur++ = 0x42;
    *pCur++ = 0x30000;
    *pCur++ = 0x47;
    *pCur++ = 0;
    *pCur++ = 0x14;
    *pCur++ = 0x13A0001C0;
    *pCur++ = 0x4E;
    *pCur++ = 1;
    *pCur++ = 0x46;
    if (font.flag & 0x1000) {
        *pCur++ = font.m_rgba;
    } else {
        *pCur++ = font.m_rgba | 0xFF000000;
    }
    *pCur++ = 1;
    *pCur++ = 0x4400000000008000;
    *pCur++ = 0x53D3;
    i = font.m_st_num;
    while (i > 0) {
        if (pCur == (unsigned long *)(base + 0x70002000)) {
            spkDmafromSPR(0x200, base, ppos);
            ppos += 0x200;
            base ^= 0x2000;
            pCur = (unsigned long *)((base & 0x3FFF) + 0x70000000);
        }
        x = bx + fstream->x;
        y = by + fstream->y;
        u = fstream->u;
        v = fstream->v;
        *pCur++ = ((unsigned long)u << 4) | ((unsigned long)v << 20);
        *pCur++ = fontXYZ(x, y, z);
        *pCur++ = ((long)(u + 8) << 4) | ((long)(v + 8) << 20);
        *pCur++ = fontXYZ(x + w, y + h, z);
        fstream++;
        i--;
    }
    n = ((u_int)pCur - (base + 0x70000000)) >> 4;
    if (n > 0) {
        spkDmafromSPR(n, base, ppos);
        ppos += n;
    }
    n = ((u_int)ppos - (u_int)pui) >> 4;
    spkDmaWaitfromSPR(0);
    if (n <= 12) {
        *pui = 0x70000000;
        spack.pk_last++;
        return (u_long128 *)pui;
    }
    pui[0] = (n - 1) | 0x70000000;
    pui[3] = (n - 1) | 0x50000000;
    pui[44] |= (n - 12) / 2;
    spack.pk_last = (unsigned long *)ppos;
    return (u_long128 *)((u_int)pui & 0x0FFFFFFF);
}
