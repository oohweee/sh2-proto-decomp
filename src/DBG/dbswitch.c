/*
 * dbswitch.c: the debug switches. Each page (enum DBSW_ID) is a word of 32
 * on/off switches; a cursor (dbSwitchX page, dbSwitchY bit) edits them on screen.
 */

#include "sh2.h"

/* Number of switches per page: one bit each of dbSwitchStat[X]. */
#define DB_SWITCH_BITS (sizeof(dbSwitchStat[0]) * 8)

static int dbSwitchDispF;
static int dbSwitchY;
static int dbSwitchX;
static unsigned int dbSwitchStat[DB_SWITCH_MAX] = { 0, 0 };

/**
 * Moves the cursor by (`dx`, `dy`), toggles the switch under it when `o` is set, toggles the
 * display when `x` is set, and draws the indicator of the current page. Does nothing unless
 * `enable`.
 */
void dbSwitchDispIndicator(int enable, int o, int x, int dx, int dy) {
    int Y;
    char *sym[2][2] = {
        { "-\n", "\x8a-\n" },
        { "O\n", "\x8aO\n" },
    };
    int bit;
    int just;

    if (enable) {
        dy = -dy;
        if (x || o) {
            dy = 0;
            dx = 0;
        }
        if (x) {
            dbSwitchDispF = !dbSwitchDispF;
        }
        if (dbSwitchDispF) {
            dbSwitchX += dx;
            dbSwitchY += dy;
            if (dbSwitchX < 0) {
                dbSwitchX = 0;
            }
            if (dbSwitchX > DB_SWITCH_MAX - 1) {
                dbSwitchX = DB_SWITCH_MAX - 1;
            }
            if (dbSwitchY < 0) {
                dbSwitchY = 0;
            }
            if (dbSwitchY > DB_SWITCH_BITS - 1) {
                dbSwitchY = DB_SWITCH_BITS - 1;
            }
            if (o) {
                dbSwitchStat[dbSwitchX] ^= 1U << dbSwitchY;
            }
            dbfntprintfR("%1X\n", dbSwitchX);
            for (Y = 0; Y < DB_SWITCH_BITS; Y++) {
                if (Y % 8 == 0) {
                    dbfntprintR("\n");
                }
                bit = (1U << Y & dbSwitchStat[dbSwitchX]) != 0;
                just = dbSwitchY == Y;
                dbfntprintR(sym[bit][just]);
            }
        }
    }
}

/**
 * Sets the display flag to `enable`, or returns it unchanged when `enable` is negative. Returns the
 * flag.
 */
int dbSwitchDispEnable(int enable) {
    if (enable < 0) {
        enable = dbSwitchDispF;
    } else {
        dbSwitchDispF = enable;
    }
    return enable;
}

/**
 * Sets every switch of page `_X` from `bit_pattern`. Returns 0 for an invalid page, 1 otherwise.
 */
int dbSwitchInit(enum DBSW_ID _X, unsigned long bit_pattern) {
    int X;

    X = _X;
    if (X < 0) {
        return 0;
    }
    if (X > DB_SWITCH_MAX - 1) {
        return 0;
    }
    dbSwitchStat[X] = bit_pattern;
    return 1;
}

/**
 * Sets (`set` non-zero) or clears switch `Y` of page `_X`. Returns 0 for an invalid page or switch,
 * 1 otherwise.
 */
int dbSwitchSet(enum DBSW_ID _X, int Y, int set) {
    int X;

    X = _X;
    if (X < 0) {
        return 0;
    }
    if (X > DB_SWITCH_MAX - 1) {
        return 0;
    }
    if (Y < 0) {
        return 0;
    }
    if (Y > DB_SWITCH_BITS - 1) {
        return 0;
    }
    if (set) {
        dbSwitchStat[X] |= 1U << Y;
    } else {
        dbSwitchStat[X] &= ~(1 << Y);
    }
    return 1;
}

/** Returns whether switch `Y` of page `_X` is on; always 0 while the display is off. */
int dbSwitch(enum DBSW_ID _X, int Y) {
    int X;

    X = _X;
    if (X < 0) {
        return 0;
    }
    if (X > DB_SWITCH_MAX - 1) {
        return 0;
    }
    if (Y < 0) {
        return 0;
    }
    if (Y > DB_SWITCH_BITS - 1) {
        return 0;
    }
    if (dbSwitchDispF) {
        return ((1 << Y) & dbSwitchStat[X]) != 0;
    }
    return 0;
}

/**
 * Stores the cursor's page in `*Xp` and switch in `*Yp` (either may be NULL) and returns the state
 * of the switch under it.
 */
int dbSwitchGetPos(int *Xp, int *Yp) {
    if (Xp) {
        *Xp = dbSwitchX;
    }
    if (Yp) {
        *Yp = dbSwitchY;
    }
    return dbSwitch(dbSwitchX, dbSwitchY);
}
