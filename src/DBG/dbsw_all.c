/*
 * dbsw_all.c: the debug switch pages as a whole: initialization, the page
 * indicator, help and pad control (port 1), and printing of the current page.
 */

#include "sh2.h"

/**
 * Turns the debug switch display on or off (`enable`); turning it on also sets every page to its
 * defaults.
 */
void dbSwitchAllInit(int enable) {
    dbSwitchDispEnable(enable != 0);
    if (enable) {
        dbSwitchSysInit();
        dbSwitchMapInit();
    }
}

static void dbSwitchHelpPrint(void) {
    int X;
    int Y;
    int on;
    char *help;

    help = NULL;
    on = dbSwitchGetPos(&X, &Y);
    switch (X) {
    case DBSW_SYS:
        help = dbSwitchSysHelp(Y);
        break;
    case DBSW_MAP:
        help = dbSwitchMapHelp(Y);
        break;
    }
    if (!help) {
        help = "(NO HELP)";
    }
    dbfntprintfR("%s:%s\n", help, on ? "ON " : "OFF");
}

/**
 * Returns whether the switch indicator is shown: always from pad port 1, from other ports only when
 * switch "not-only port 1" is on.
 */
int dbSwitchIsVisible(void) {
    int port;
    int notonlyport1;
    int disp;

    port = shPadGetPort();
    notonlyport1 = dbSwitchSysPrintNotOnlyPort1();
    disp = port == 1 || notonlyport1;
    return disp;
}

/**
 * Per-frame debug switch update: moves the cursor and toggles switches from pad port 1, and prints
 * the help line and the current (or every) page.
 */
void dbSwitchAllPrint(void) {
    int X;
    int loop;
    int disp;

    disp = dbSwitchIsVisible();
    dbSwitchGetPos(&X, NULL);
    dbfntlocateR(508, 44);
    if (disp) {
        dbSwitchDispIndicator(disp, shPadRepeat(1, 0x2000) != 0, shPadRepeat(1, 0x4000) != 0,
                              (shPadRepeat(1, 0x100) != 0) - (shPadRepeat(1, 0x200) != 0),
                              (shPadRepeat(1, 0x400) != 0) - (shPadRepeat(1, 0x800) != 0));
    } else {
        dbSwitchDispIndicator(disp, 0, shPadRepeat(1, 0x4000) != 0, 0, 0);
    }
    if (shPadPress(1, 0x10000)) {
        if ((shPadPress(1, 0x8000) && shPadTrigger(1, 0x1000)) ||
            (shPadPress(1, 0x1000) && shPadTrigger(1, 0x8000))) {
            dbSwitchAllInit(!dbSwitchDispEnable(-1));
        } else {
            if (shPadTrigger(1, 0x1000)) {
                dbSwitchInit(X, 0xFFFFFFFF);
            }
            if (shPadTrigger(1, 0x8000)) {
                dbSwitchInit(X, 0);
            }
        }
    }
    if (disp) {
        dbfntlocateR(496, 44);
        if (dbSwitchSysPrintHelp()) {
            dbSwitchHelpPrint();
        }
        for (loop = 0; loop < DB_SWITCH_MAX; loop++) {
            if (loop == X || dbSwitchSysPrintAllPages()) {
                switch (loop) {
                case DBSW_SYS:
                    dbSwitchSysPrint();
                    break;
                case DBSW_MAP:
                    dbSwitchMapPrint();
                    break;
                }
            }
        }
    }
}
