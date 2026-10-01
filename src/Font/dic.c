/*
 * dic.c: converts game text (ASCII, Shift-JIS and backslash/ESC control sequences)
 * into the 16-bit font code stream the font renderer draws.
 */
#include "sh2.h"

static unsigned short dicCodeAsc(unsigned char c);
static unsigned short dicCode81(unsigned char c);
static unsigned short dicCode82(unsigned char c);
static unsigned short dicCode83(unsigned char c);
static unsigned short dicCode84(unsigned char c);
static unsigned short dicCode87(unsigned char c);

unsigned short str_buf[4][128];
unsigned short buf_switch;

/* Reads a decimal number; *c is left on the first non-digit. */
static inline int dicGetNum(unsigned char **ucp, unsigned char *c) {
    int num;

    for (num = 0; (*c = *(*ucp)++) != 0; num = num * 10 + *c - '0') {
        if (*c < '0' || *c > '9') {
            break;
        }
    }
    return num;
}

/** Converts a text into font codes, in the next of four rotating buffers (str_buf).
 * @param str the text (ASCII / Shift-JIS with control sequences, 0-terminated)
 * @return the font code string */
unsigned short *dicSetStr(void *str) {
    int code;
    int latin;
    int wf;
    unsigned char *ucp;
    unsigned short *spos;
    unsigned char c;

    ucp = str;
    spos = str_buf[buf_switch];
    wf = 0;
    latin = 0;
    code = 0;
    c = *ucp++;
    while (c) {
        if (c == '\t' || c == '\r') {
        } else if (c == '\n') {
            *spos++ = 0xFFFD;
        } else if (c == ' ') {
            if (!latin) {
                *spos++ = 0xFFFE;
            } else {
                *spos++ = 0;
            }
        } else if (c == 0x1B) {
            c = *ucp++;
            if (c == 'k' || c == 'K') {
                wf = 0x1000;
            } else if (c == 'w' || c == 'W') {
                wf = dicGetNum(&ucp, &c) + 0x2000;
            } else if (c == 'a' || c == 'A') {
                wf = dicGetNum(&ucp, &c) + 0x3000;
            } else {
                wf = 0;
            }
            break;
        } else if (c <= 0x20 || c == 0x7F) {
        } else if (code != 0) {
            code = (code << 8) + c;
            *spos++ = dicTransCode(code);
            code = 0;
        } else if (c >= 0x80) {
            if (!latin) {
                code = c;
            } else {
                *spos++ = c - 0x20;
            }
        } else if (c != '\\') {
            if (!latin) {
                *spos++ = dicCodeAsc(c);
            } else {
                *spos++ = c - 0x20;
            }
        } else {
            c = *ucp++;
            if (c == 'n') {
                *spos++ = 0xFFFD;
            } else if (c == '+') {
                *spos++ = 0xFD00 + dicGetNum(&ucp, &c);
                continue;
            } else if (c == '-') {
                *spos++ = 0xFC00 + dicGetNum(&ucp, &c);
                continue;
            } else if (c == 'l' || c == 'L') {
                *spos++ = 0xFFFC;
            } else if (c == 'c' || c == 'C') {
                *spos++ = 0xFFFB;
            } else if (c == 'a' || c == 'A') {
                *spos++ = 0xFFFA;
            } else if (c == 'x' || c == 'X') {
                *spos++ = 0xFA00 + dicGetNum(&ucp, &c);
                continue;
            } else if (c == 'y' || c == 'Y') {
                *spos++ = 0xF800 + dicGetNum(&ucp, &c);
                continue;
            } else if (c == 'h' || c == 'H') {
                latin = 1;
            } else if (c == 'z' || c == 'Z') {
                latin = 0;
            } else if (c >= '0' && c <= '9') {
                code = c - '0';
                c = *ucp++;
                if (c < '0' || c > '9') {
                    *spos++ = 0xFF00 + code;
                    code = 0;
                    continue;
                } else {
                    *spos++ = c + code * 10 + 0xFF00 - '0';
                    code = 0;
                }
            } else if (c == 's' || c == 'S') {
                *spos++ = 0xFFF9;
            } else if (c == 'd' || c == 'D') {
                *spos++ = 0xFFF8;
            } else if (c == 'b' || c == 'B') {
                *spos++ = 0xFFF7;
            } else if (c == 'v' || c == 'V') {
                c = *ucp++;
                if (c >= '0' && c <= '9') {
                    *spos++ = 0xFFE0 + c - '0';
                } else {
                    *spos++ = 0xFFE0;
                    continue;
                }
            } else if (c == '_') {
                *spos++ = 0xFE00 + dicGetNum(&ucp, &c);
                continue;
            } else if (c == 'p' || c == 'P') {
                *spos = 0xFFFF;
                c = *ucp++;
                if (c == 'k' || c == 'K') {
                    wf = 0x1000;
                    c = *ucp++;
                } else if (c == 'w' || c == 'W') {
                    wf = dicGetNum(&ucp, &c) + 0x2000;
                } else if (c == 'a' || c == 'A') {
                    wf = dicGetNum(&ucp, &c) + 0x3000;
                } else {
                    wf = 0;
                }
                spos[1] = wf;
                spos += 2;
                wf = 0;
                continue;
            } else if (c == 'e' || c == 'E') {
                c = *ucp++;
                if (c == 'k' || c == 'K') {
                    wf = 0x1000;
                } else if (c == 'w' || c == 'W') {
                    wf = dicGetNum(&ucp, &c) + 0x2000;
                } else if (c == 'a' || c == 'A') {
                    wf = dicGetNum(&ucp, &c) + 0x3000;
                } else {
                    wf = 0;
                }
                break;
            } else if (c == '!') {
                latin = 1;
            } else if (c == '\\') {
                if (!latin) {
                    *spos++ = dicCodeAsc('\\');
                } else {
                    *spos++ = 0x3C;
                }
            } else if (c != '|') {
                continue;
            }
        }
        c = *ucp++;
    }
    *spos = 0xFFFF;
    spos[1] = wf | 0x8000;
    spos = str_buf[buf_switch++];
    buf_switch &= 3;
    return spos;
}

/** Converts one character code (ASCII in the low byte, or a Shift-JIS lead byte 0x81-0x87 in the
 * high byte) to its font code.
 * @param code the character code
 * @return the font code */
unsigned short dicTransCode(unsigned short code) {
    unsigned char ch;
    unsigned char cl;

    ch = code >> 8;
    cl = code & 0xFF;
    switch (ch) {
    case 0x00:
        return dicCodeAsc(cl);
    case 0x81:
        return dicCode81(cl);
    case 0x82:
        return dicCode82(cl);
    case 0x83:
        return dicCode83(cl);
    case 0x84:
        return dicCode84(cl);
    case 0x87:
        return dicCode87(cl);
    }
    if (cl < 0x40 || cl == 0x7F || cl > 0xFC) {
        return 0x7FFF;
    }
    if (code >= 0x889F && code <= 0x9872) {
        if (cl > 0x7F) {
            cl--;
        }
        return cl + (ch - 0x88) * 188 + 0x2A0;
    }
    if (code >= 0x989F && code <= 0x9FFC) {
        if (cl > 0x7F) {
            cl--;
        }
        return cl + (ch - 0x98) * 188 + 0xE35;
    }
    if (code >= 0xE040 && code <= 0xEAA4) {
        if (cl > 0x7F) {
            cl--;
        }
        return cl + (ch - 0xE0) * 188 + 0x1415;
    }
    if (code >= 0xED40 && code <= 0xEEEC) {
        if (cl > 0x7F) {
            cl--;
        }
        return cl + (ch - 0xED) * 188 + 0x1BED;
    }
    if (code >= 0xFA40 && code <= 0xFC4B) {
        if (cl > 0x7F) {
            cl--;
        }
        return cl + (ch - 0xFA) * 188 + 0x1BD1;
    }
    return 0x7FFF;
}

static unsigned short dicCodeAsc(unsigned char c) {
    unsigned short num1[15] = { 0x0008, 0x0027, 0x0052, 0x004E, 0x0051, 0x0053, 0x0025, 0x0028,
                                0x0029, 0x0054, 0x003A, 0x0002, 0x003B, 0x0003, 0x001D };
    unsigned short num2[7] = { 0x0005, 0x0006, 0x0041, 0x003F, 0x0042, 0x0007, 0x0055 };
    unsigned short num3[6] = { 0x002C, 0x004D, 0x002D, 0x000E, 0x0010, 0x0024 };
    unsigned short num4[4] = { 0x002E, 0x0021, 0x002F, 0x001F };

    if (c == ' ') {
        return 0;
    } else if (c <= 0x2F) {
        return num1[c - 0x21] + 0xE0;
    } else if (c <= 0x39) {
        return c + 0x142;
    } else if (c <= 0x40) {
        return num2[c - 0x3A] + 0xE0;
    } else if (c <= 0x5A) {
        return c + 0x13B;
    } else if (c <= 0x60) {
        return num3[c - 0x5B] + 0xE0;
    } else if (c <= 0x7A) {
        return c + 0x135;
    } else {
        return num4[c - 0x7B] + 0xE0;
    }
}

static unsigned short dicCode81(unsigned char c) {
    if (c < 0x40 || c == 0x7F || (c >= 0xAD && c <= 0xB7) || (c >= 0xC0 && c <= 0xC7) ||
        (c >= 0xCF && c <= 0xD9) || (c >= 0xE9 && c <= 0xEF) || (c >= 0xF8 && c <= 0xFB) || c >= 0xFD) {
        return 0x7FFF;
    }
    if (c == 0x40) {
        return 0xFFFE;
    } else if (c <= 0x7E) {
        return c + 0x9F;
    } else if (c <= 0xAC) {
        return c + 0x9E;
    } else if (c <= 0xBF) {
        return c + 0x93;
    } else if (c <= 0xCE) {
        return c + 0x8B;
    } else if (c <= 0xE8) {
        return c + 0x80;
    } else if (c <= 0xF7) {
        return c + 0x79;
    } else {
        return 0x171;
    }
}

static unsigned short dicCode82(unsigned char c) {
    if (c < 0x4F || (c >= 0x59 && c <= 0x5F) || (c >= 0x7A && c <= 0x80) || (c >= 0x9B && c <= 0x9E) ||
        c >= 0xF2) {
        return 0x7FFF;
    }
    if (c <= 0x58) {
        return c + 0x123;
    } else if (c <= 0x79) {
        return c + 0x11C;
    } else if (c <= 0x9A) {
        return c + 0x115;
    } else {
        return c + 0x111;
    }
}

static unsigned short dicCode83(unsigned char c) {
    if (c < 0x40 || c == 0x7F || (c >= 0x97 && c <= 0x9E) || (c >= 0xB7 && c <= 0xBE) || c >= 0xD7) {
        return 0x7FFF;
    }
    if (c <= 0x7E) {
        return c + 0x1C3;
    } else if (c <= 0x96) {
        return c + 0x1C2;
    } else if (c <= 0xB6) {
        return c + 0x1BA;
    } else {
        return c + 0x1B2;
    }
}

static unsigned short dicCode84(unsigned char c) {
    if (c < 0x40 || (c >= 0x61 && c <= 0x6F) || c == 0x7F || (c >= 0x92 && c <= 0x9E) || c >= 0xBF) {
        return 0x7FFF;
    }
    if (c <= 0x60) {
        return c + 0x249;
    } else if (c <= 0x7E) {
        return c + 0x23A;
    } else if (c <= 0x91) {
        return c + 0x239;
    } else {
        return c + 0x22C;
    }
}

static unsigned short dicCode87(unsigned char c) {
    if (c < 0x40 || c == 0x5E || (c >= 0x76 && c <= 0x7D) || c == 0x7F || c >= 0x9D) {
        return 0x7FFF;
    }
    if (c <= 0x5D) {
        return c + 0x2AB;
    } else if (c <= 0x75) {
        return c + 0x2AA;
    } else if (c == 0x7E) {
        return 0x320;
    } else {
        return c + 0x2A1;
    }
}
