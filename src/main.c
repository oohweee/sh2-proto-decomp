/*
 * main.c: the program entry and the top-level main loop.
 *
 * Sh2sys.step[] is a stack of state numbers: step[0] is the top-level state
 * (0-2: (re)initialize, 3: run the game), deeper levels belong to whoever runs
 * inside it. Setting a level clears every level below it.
 */

#include "sh2.h"
#include "lib/sh_kernel.h"

#define DB_FLOW_CHECK(name) \
    ___dbFlowSetCheckPoint("`" name "'(" __FILE__ ":" SH_STRINGIFY(__LINE__) ")")

#define SH2SYS_STEP0(v)          \
    Sh2sys.step[0] = (v);        \
    Sh2sys.step[1] = 0;          \
    Sh2sys.step[2] = 0;          \
    Sh2sys.step[3] = 0;          \
    Sh2sys.step[4] = 0;          \
    Sh2sys.step[5] = 0;          \
    Sh2sys.step[6] = 0;          \
    Sh2sys.step[7] = 0

#define SH2SYS_STEP1(v)          \
    Sh2sys.step[1] = (v);        \
    Sh2sys.step[2] = 0;          \
    Sh2sys.step[3] = 0;          \
    Sh2sys.step[4] = 0;          \
    Sh2sys.step[5] = 0;          \
    Sh2sys.step[6] = 0;          \
    Sh2sys.step[7] = 0

struct _SH2_SYS Sh2sys;
union DB_WATCH_POINT *db_watch_point;

/* Matching: #line: the original has 79 lines without code here. */
#line 118

/**
 * Program entry: parses the boot options (`argc`, `argv`), starts the system and runs the main loop
 * forever.
 */
int main(int argc, char **argv) {
    int db_test_dvd;
    int step;
    int dbFlagSet(); /* Matching: an unprototyped block-scope declaration, as in the original. */

    if (argc < 2) {
        printf_skip(1);
    } else if (argv[1][0] != '-') {
        argv[1] = argv[0];
        argc--;
        argv++;
        printf_skip(1);
    } else {
        wrap_printf_init();
    }
    BootOptGet(argc, argv);
    db_watch_point = (union DB_WATCH_POINT *)0x2000000;
    db_test_dvd = dbFlag(0x10) != 0;
    dbSwitchAllInit(dbFlagSet(0x10) != 0);
    systemColdInit();
    check_build_environment(argv[0]);
    SH2SYS_STEP0(0);
    dbFlowStartCheck(1);

    while (1) {
        step = Sh2sys.step[0];
        DB_FLOW_CHECK("main loop.");
        switch (step) {
        case 0:
        case 1:
        case 2:
            DB_FLOW_CHECK("before hot init");
            if (systemHotInit()) {
                dbSwitchDispEnable(db_test_dvd);
                SH2SYS_STEP0(3);
                switch (step) {
                case 0:
                    SH2SYS_STEP1(1);
                    break;
                case 1:
                case 2:
                    SH2SYS_STEP1(6);
                    break;
                }
            }
            /* Matching: #line: the original has 5 lines without code here. */
#line 173
            DB_FLOW_CHECK("after hot init");
            break;
        case 3:
            DrawLopp_Pre();
            dbFreeze();
            /* Matching: #line: the original has 2 lines without code here. */
#line 180
            DB_FLOW_CHECK("before game main");
            GameMain();
            DB_FLOW_CHECK("after game main");
            dbSwitchAllPrint();
            DrawLopp_Post();
            GameKeyCheck();
            break;
        }
        /* Matching: #line: the original has 1 line without code here. */
#line 189
        DB_FLOW_CHECK("before SE vsync");
        Sh2sys.frame_cnt++;
        /* Matching: #line: the original has 1 line without code here. */
#line 192
        DB_FLOW_CHECK("after SE vsync");
    }
}

/** Soft reset: SELECT + START + L1 + R1 held on pad 0 goes back to the hot init (if Sh2sys.soft_reset). */
void GameKeyCheck(void) {
    if (Sh2sys.soft_reset && shPadPress(0, 8) && shPadPress(0, 4) && shPadPress(0, 0x10000) &&
        shPadPress(0, 0x20000)) {
        fsSync(0, -1);
        lisSync(0, -1);
        SH2SYS_STEP0(2);
        Env_ctl.stat_ctl_1.ui32[0] <<= 8;
        Env_ctl.stat_ctl_1.uc8[0] = 0;
    }
}
