/* dbfreeze.c: the debug freeze: stop and single-step the game from pad port 1. */
#include "sh2.h"
#include "lib/libShPad.h"
#define DB_FLOW_CHECK(name) \
    ___dbFlowSetCheckPoint("`" name "'(" __FILE__ ":" SH_STRINGIFY(__LINE__) ")")
/** Freezes the game while pad port 1 asks for it; returns to run one frame on request. */
void dbFreeze(void) {
    static int fz_step;
    unsigned char paddata[32];
    int loop;

    if (shPadGetPort() == 1) {
        do {
            DB_FLOW_CHECK("dbfreeze loop");
            libShPadRead(1, 0, (char *)paddata);
            shSysKeyNormalize((char *)paddata);
            loop = 1;
            switch (fz_step) {
            case 0:
                loop = 0;
                if (paddata[0x16]) {
                    fz_step = 1;
                }
                /* Matching: #line keeps the original numbering after the added brace. */
#line 23
                break;
            case 1:
                if (!paddata[0x16]) { fz_step = 2; }
                break;
            case 2:
                if (paddata[0x16]) { fz_step = 3; }
                else if (paddata[0x11]) { fz_step = 4; }
                else if (paddata[0x13]) { loop = 0; }
                break;
            case 3:
                if (!paddata[0x16]) { fz_step = 0; }
                break;
            case 4:
                loop = 0;
                fz_step = 5;
                break;
            case 5:
                if (!paddata[0x11]) { fz_step = 2; }
                break;
            }
        } while (loop);
        DB_FLOW_CHECK("exit dbfreeze");
    }
}
