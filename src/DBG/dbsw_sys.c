/* dbsw_sys.c: the "system" page of the debug switches and what it prints. */

#include "sh2.h"
#include "libc/stdio.h"
#include "lib/libShPad.h"

/** Returns the help text of system switch `Y`, or NULL when it has none. */
char *dbSwitchSysHelp(int Y) {
    char *help;

    help = NULL;
    switch (Y) {
    case 0:
        help = "display this help";
        break;
    case 1:
        help = "enable pad port for debug";
        break;
    case 2:
        help = "display switch indicator not-only port 1";
        break;
    case 3:
        help = "display all switch pages";
        break;
    case 4:
        help = "display RTC";
        break;
    case 5:
        help = "display File Server status";
        break;
    case 6:
        help = "display Load and Init command Server status";
        break;
    case 7:
        help = "verbose at command server status";
        break;
    case 8:
        help = "display default language by ps2 system configuration";
        break;
    case 19:
        help = "diaplay special characters(0x20-0x9F)";
        break;
    case 9:
        help = "display allinfo at pad(0,0)";
        break;
    case 10:
        help = "display pad step at pad(0,0).";
        break;
    case 11:
        help = "display paddata at pad(0,0).";
        break;
    case 12:
        help = "display keydata at pad(0,0).";
        break;
    case 13:
        help = "display allinfo at pad(1,0).";
        break;
    case 14:
        help = "display pad step at pad(1,0).";
        break;
    case 15:
        help = "display paddata at pad(1,0).";
        break;
    case 16:
        help = "display keydata at pad(1,0).";
        break;
    case 17:
        help = "semaphore param.";
        break;
    case 18:
        help = "thread param.";
        break;
    case 30:
        help = "debug menu from soft reset.";
        break;
    case 31:
        help = "movie dummy step.";
        break;
    }
    return help;
}

static int dbSwitchSys(int Y) {
    return dbSwitch(DBSW_SYS, Y);
}

static void dbSwitchSetSys(int Y, int on) {
    dbSwitchSet(DBSW_SYS, Y, on);
}

static void printR_date(void) {
    struct sceCdCLOCK rtc[1];
    char strbuf[32];
    char *str;

    shCdReadClock(rtc);
    str = strbuf;
    str += sh2ScfMakeDateStrByLocalTimeFromJST(str, rtc);
    str += sprintf(str, " ");
    sh2ScfMakeTimeStrByLocalTimeFromJST(str, rtc);
    dbfntprintfR("%s\n", strbuf);
}

static void printR_cmdserv(struct CmdServStat *stat, char *prefix, int *exec) {
    char syms[4] = { '|', '/', '-', '\x81' };
    char sym;

    sym = ' ';
    if (exec) {
        if (stat->clen) {
            sym = syms[(*exec)++ % 4];
        } else {
            *exec = 0;
        }
    }
    dbfntprintfR(dbSwitchSys(7) ? "%s: %3d%1c(%03d-%03d) (%3d/%3d)\n" : "%s: %3d%1c\n", prefix,
                 stat->clen % 1000, sym, stat->id % 1000, stat->last_id % 1000, stat->qlen % 1000,
                 stat->qsize % 1000);
}

static void printR_fileserv(void) {
    static int exec[1] = { 0 };
    struct CmdServStat stat[1];

    fsGetStat(stat);
    printR_cmdserv(stat, "fs", exec);
}

static void printR_loadinit(void) {
    static int exec[1] = { 0 };
    struct CmdServStat stat[1];

    lisGetStat(stat);
    printR_cmdserv(stat, "lis", exec);
}

static void printR_test(void) {
    dbfntprintR("[ !\"#$%&' ()*+,-./]\n"
                "[01234567 89:;<=>?]\n"
                "[@ABCDEFG HIJKLMNO]\n"
                "[PQRSTUVW XYZ[\\]^_]\n"
                "[`abcdefg hijklmno]\n"
                "[pqrstuvw xyz{|}~\x7f]\n"
                "[\x80\x81\x82\x83\x84\x85\x86\x87 \x88\x89\x8a\x8b\x8c\x8d\x8e\x8f]\n"
                "[\x90\x91\x92\x93\x94\x95\x96\x97 \x98\x99\x9a\x9b\x9c\x9d\x9e\x9f]\n");
}

static void printR_padstep(int port, int slot) {
    dbfntprintfR("pad(%d,%d) trans step: %04x\n", port, slot, libShPadStep[port][slot]);
}

static void printR_paddata(char *paddata) {
    unsigned int padp[8] = { 0 };
    int idx;
    int ofs;

    for (ofs = 31; ofs >= 0; ofs--) {
        idx = ofs / 4;
        padp[idx] <<= 8;
        padp[idx] |= (unsigned char)paddata[ofs];
    }
    dbfntprintfR("%08x %08x\n%08x %08x\n%08x %08x\n%08x %08x\n", padp[1], padp[0], padp[3], padp[2],
                 padp[5], padp[4], padp[7], padp[6]);
}

static void printR_keydata(union shGameKeyData *key) {
    dbfntprintfR("type:%1d  DRINK:%1d\nRADIO:%1d  LIGHT:%1d\nITEM:%1d    MAP:%1d\nDECIDE:%1d CANCEL:%1d\n",
                 key->f.type, key->f.DRINK, key->f.RADIO, key->f.LIGHT, key->f.ITEM, key->f.MAP,
                 key->f.DECIDE, key->f.CANCEL);
    dbfntprintfR("SKIP:%1d  PAUSE:%1d\nACTION:%1d   DASH:%1d\nLSLIDE:%1d RSLIDE:%1d\nREADY:%1d   VIEW:%1d\n",
                 key->f.SKIP, key->f.CANCEL, key->f.ACTION, key->f.DASH, key->f.LSLIDE, key->f.RSLIDE,
                 key->f.READY, key->f.VIEW);
    dbfntprintfR("A(X,Y):(%+1d,%+1d)\nB(X,Y):(%+1d,%+1d)\nC(X,Y):(%+1d,%+1d)\nlen:%5d\n", key->f.AX,
                 key->f.AY, key->f.BX, key->f.BY, key->f.CX, key->f.CY, key->f.len);
}

static void printR_padport(int port, int slot, int pad_normalize, int pad_adjust, int dispstep, int disppad,
                           int dispkey) {
    unsigned char paddata[32];
    union shGameKeyData key[1];

    if (disppad || dispkey) {
        libShPadRead(port, slot, (char *)paddata);
        if (pad_normalize) {
            shSysKeyNormalize((char *)paddata);
        }
        if (pad_adjust) {
            shSysKeyAdjust((char *)paddata);
        }
    }
    if (dispstep) {
        printR_padstep(port, slot);
    }
    if (disppad) {
        printR_paddata((char *)paddata);
    }
    if (dispkey) {
        shGameKeyConvert(key, (char *)paddata);
        printR_keydata(key);
    }
}

static void printR_pad00(void) {
    if (dbSwitchSys(9) && !dbSwitchSys(10) && !dbSwitchSys(11) && !dbSwitchSys(12)) {
        dbSwitchSet(DBSW_SYS, 10, 1);
        dbSwitchSet(DBSW_SYS, 11, 1);
        dbSwitchSet(DBSW_SYS, 12, 1);
    }
    if (!dbSwitchSys(9) && dbSwitchSys(10) && dbSwitchSys(11) && dbSwitchSys(12)) {
        dbSwitchSet(DBSW_SYS, 10, 0);
        dbSwitchSet(DBSW_SYS, 11, 0);
        dbSwitchSet(DBSW_SYS, 12, 0);
    }
    printR_padport(0, 0, 1, 1, dbSwitchSys(10), dbSwitchSys(11), dbSwitchSys(12));
}

static void printR_pad10(void) {
    if (dbSwitchSys(13) && !dbSwitchSys(14) && !dbSwitchSys(15) && !dbSwitchSys(16)) {
        dbSwitchSet(DBSW_SYS, 14, 1);
        dbSwitchSet(DBSW_SYS, 15, 1);
        dbSwitchSet(DBSW_SYS, 16, 1);
    }
    if (!dbSwitchSys(13) && dbSwitchSys(14) && dbSwitchSys(15) && dbSwitchSys(16)) {
        dbSwitchSet(DBSW_SYS, 14, 0);
        dbSwitchSet(DBSW_SYS, 15, 0);
        dbSwitchSet(DBSW_SYS, 16, 0);
    }
    printR_padport(1, 0, 1, 1, dbSwitchSys(14), dbSwitchSys(15), dbSwitchSys(16));
}

static void printR_lang(void) {
    char *str;

    switch (sh2ScfGetDefaultLanguage()) {
    case SPEC_LANG_JPN_O:
        str = "JPN";
        break;
    case SPEC_LANG_ENG_O:
        str = "ENG";
        break;
    case SPEC_LANG_FRN_O:
        str = "FRN";
        break;
    case SPEC_LANG_GER_O:
        str = "GER";
        break;
    case SPEC_LANG_ITA_O:
        str = "ITA";
        break;
    case SPEC_LANG_SPN_O:
        str = "SPN";
        break;
    }
    dbfntprintfR("lang:%s\n", str);
}

/**
 * Prints what the system page's switches ask for (clock, server status, pads, language, threads).
 */
void dbSwitchSysPrint(void) {
    int Y;

    for (Y = 0; Y < 32; Y++) {
        switch (Y) {
        case 4:
            if (dbSwitchSys(Y)) {
                printR_date();
            }
            break;
        case 5:
            if (dbSwitchSys(Y)) {
                printR_fileserv();
            }
            break;
        case 6:
            if (dbSwitchSys(Y)) {
                printR_loadinit();
            }
            break;
        case 19:
            if (dbSwitchSys(Y)) {
                printR_test();
            }
            break;
        case 9:
            printR_pad00();
            break;
        case 13:
            printR_pad10();
            break;
        case 8:
            if (dbSwitchSys(Y)) {
                printR_lang();
            }
            break;
        case 17:
            break;
        case 18:
            if (dbSwitchSys(Y)) {
                dbScrPrintThreAll();
            }
            break;
        }
    }
}

/** Returns whether the switch help line is shown (system switch 0). */
int dbSwitchSysPrintHelp(void) {
    return dbSwitchSys(0);
}

/** Returns whether the debug pad port is enabled (system switch 1). */
int dbSwitchSysPadPort(void) {
    return dbSwitchSys(1);
}

/** Returns whether every switch page is printed, not just the current one (system switch 3). */
int dbSwitchSysPrintAllPages(void) {
    return dbSwitchSys(3);
}

/** Returns whether the switch indicator is shown for pads other than port 1 (system switch 2). */
int dbSwitchSysPrintNotOnlyPort1(void) {
    return dbSwitchSys(2);
}

/** Default system page: switches 1, 2, 3, 5, 6, 30 and 31 on. */
void dbSwitchSysInit(void) {
    dbSwitchSetSys(1, 1);
    dbSwitchSetSys(2, 1);
    dbSwitchSetSys(3, 1);
    dbSwitchSetSys(5, 1);
    dbSwitchSetSys(6, 1);
    dbSwitchSetSys(30, 1);
    dbSwitchSetSys(31, 1);
}
