/* dbfntprint.c: printf-style text on the debug font, left- or right-aligned. */

#include "sh2.h"
#include "libc/stdarg.h"
#include "libc/stdio.h"
#include "libc/string.h"

/* Print positions. No DWARF for this object; the ELF symbol is the static `d`, field names are guesses. */
static struct {
    int x0;   /* 0x00 left margin (dbfntlocate) */
    int y0;   /* 0x04 */
    int x;    /* 0x08 cursor */
    int y;    /* 0x0C */
    int w;    /* 0x10 character width */
    int h;    /* 0x14 line height */
    int tab;  /* 0x18 tab width in characters */
    int rx;   /* 0x1C right margin (dbfntlocateR) */
    int ry0;  /* 0x20 */
    int ry;   /* 0x24 cursor for right-aligned text */
} d = { 0, 0, 0, 0, 8, 8, 4, 0x200, 0, 0 };

/** Moves the left-aligned print position (and its left margin) to (`x`, `y`). */
void dbfntlocate(int x, int y) {
    d.x0 = d.x = x;
    d.y0 = d.y = y;
}

/** Moves the right-aligned print position to (`x`, `y`); text ends at column `x`. */
void dbfntlocateR(int x, int y) {
    d.rx = x;
    d.ry0 = d.ry = y;
}

static int printline(char *cp, char *top) {
    int l;
    char line[128];

    l = cp - top;
    if (l > 0) {
        if (l >= sizeof(line)) {
            l = sizeof(line) - 1;
        }
        memcpy(line, top, l);
        line[l] = 0;
        _shDBG_print_string(line, d.x, d.y);
    } else {
        l = 0;
    }
    return l;
}

static int printlineR(char *cp, char *top) {
    int l;
    char line[128];

    l = cp - top;
    if (l > 0) {
        if (l >= sizeof(line)) {
            top += l - (sizeof(line) - 1);
            l = sizeof(line) - 1;
        }
        memcpy(line, top, l);
        line[l] = 0;
        _shDBG_print_string(line, d.rx - l * d.w, d.ry);
    } else {
        l = 0;
    }
    return l;
}

static void _dbfntprint(char *buf) {
    char *cp;
    char *t;

    for (t = cp = buf; *cp != 0; cp++) {
        switch (*cp) {
        case '\n':
            printline(cp, t);
            t = cp + 1;
            d.x = d.x0;
            d.y += d.h;
            break;
        case '\t':
            d.x += d.w * printline(cp, t);
            t = cp + 1;
            d.x += d.w * d.tab;
            break;
        case '\r':
            printline(cp, t);
            t = cp + 1;
            d.x = d.x0;
            break;
        case '\b':
            d.x += d.w * printline(cp, t);
            t = cp + 1;
            d.x -= d.w;
            break;
        }
    }
    printline(cp, t);
}

static void _dbfntprintR(char *buf) {
    char *cp;
    char *t;

    for (t = cp = buf; *cp != 0; cp++) {
        switch (*cp) {
        case '\n':
        case '\t':
        case '\b':
            printlineR(cp, t);
            t = cp + 1;
            d.ry += d.h;
            break;
        case '\r':
            printlineR(cp, t);
            t = cp + 1;
            break;
        }
    }
    printlineR(cp, t);
}

/**
 * Prints `buf` left-aligned at the current position (handles \n, \t, \r and \b), if the debug
 * display is on.
 */
void dbfntprint(char *buf) {
    if (dbSwitchDispEnable(-1)) {
        _dbfntprint(buf);
    }
}

/** Prints `buf` right-aligned, one line per \n, \t or \b, if the debug display is on. */
void dbfntprintR(char *buf) {
    if (dbSwitchDispEnable(-1)) {
        _dbfntprintR(buf);
    }
}

static int _dbfntvsnprintf(void (*dbfntprintfunc)(char *), char *buf, int limit, char *fmt, char *argp) {
    int len;

    len = vsprintf(buf, fmt, argp);
    if (len >= limit) {
        printf("dbfntprint.c:247> _dbfntvsnprintf strbuf overflow!!\n"
               "dbfntprint.c:248> %s"
               "dbfntprint.c:249> halted by error\n", buf);
        while (1) {}
    }
    dbfntprintfunc(buf);
    return len;
}

/**
 * printf-style dbfntprint(). Returns the length of the text, or 0 when the debug display is off.
 */
int dbfntprintf(char *fmt, ...) {
    char buf[512];
    char *argp;

    va_start(argp, fmt);
    if (dbSwitchDispEnable(-1)) {
        return _dbfntvsnprintf(dbfntprint, buf, sizeof(buf), fmt, argp);
    }
    return 0;
}

/**
 * printf-style dbfntprintR(). Returns the length of the text, or 0 when the debug display is off.
 */
int dbfntprintfR(char *fmt, ...) {
    char buf[512];
    char *argp;

    va_start(argp, fmt);
    if (dbSwitchDispEnable(-1)) {
        return _dbfntvsnprintf(dbfntprintR, buf, sizeof(buf), fmt, argp);
    }
    return 0;
}
