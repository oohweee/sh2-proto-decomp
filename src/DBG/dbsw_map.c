/*
 * dbsw_map.c: the "map" page of the debug switches.
 */

#include "sh2.h"

/** Returns the help text of map switch `Y`, or NULL when it has none. */
char *dbSwitchMapHelp(int Y) {
    char *help;

    help = NULL;
    switch (Y) {
    case 0:
        help = "display map-info.";
        break;
    case 2:
        help = "display map-global-id.";
        break;
    case 3:
        help = "display ref-pos.";
        break;
    case 4:
        help = "display room-name.";
        break;
    case 5:
        help = "display room-info.";
        break;
    case 6:
        help = "display map-block.";
        break;
    case 7:
        help = "wide-screen for display map-block.";
        break;
    case 8:
        help = "full-screen for display map-block.";
        break;
    case 9:
        help = "display current position marker on map-block.";
        break;
    case 10:
        help = "init map 0 if no clear-flag.";
        break;
    case 11:
        help = "map 0 clear-flag.";
        break;
    case 12:
        help = "init map 1 if no clear-flag.";
        break;
    case 13:
        help = "map 1 clear-flag.";
        break;
    case 14:
        help = "init map 2 if no clear-flag.";
        break;
    case 15:
        help = "map 2 clear-flag.";
        break;
    case 16:
        help = "init map 3 if no clear-flag.";
        break;
    case 17:
        help = "map 3 clear-flag.";
        break;
    }
    return help;
}

static int dbSwitchMap(int Y) {
    return dbSwitch(DBSW_MAP, Y);
}

static void printR_mapblock(void) {
}

static void init_map(int i) {
    int clear;

    switch (i) {
    case 0:
        clear = 11;
        break;
    case 1:
        clear = 13;
        break;
    case 2:
        clear = 15;
        break;
    case 3:
        clear = 17;
        break;
    default:
        return;
    }
    if (!dbSwitchMap(clear)) {
        dbSwitchSet(DBSW_MAP, clear, 1);
    }
}

/**
 * Runs the actions of the map page's switches that are on (map clear-flag initialization; the map
 * block display is empty in this build).
 */
void dbSwitchMapPrint(void) {
    int Y;

    for (Y = 0; Y < 32; Y++) {
        switch (Y) {
        case 0:
            if (dbSwitchMap(Y)) {
                printR_mapblock();
            }
            break;
        case 10:
            if (dbSwitchMap(Y)) {
                init_map(0);
            }
            break;
        case 12:
            if (dbSwitchMap(Y)) {
                init_map(1);
            }
            break;
        case 14:
            if (dbSwitchMap(Y)) {
                init_map(2);
            }
            break;
        case 16:
            if (dbSwitchMap(Y)) {
                init_map(3);
            }
            break;
        }
    }
}

/** Default map page: switches 0 and 1 on. */
void dbSwitchMapInit(void) {
    dbSwitchSet(DBSW_MAP, 0, 1);
    dbSwitchSet(DBSW_MAP, 1, 1);
}
