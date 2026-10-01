/*
 * dbkernel.c: the thread list page of the debug switches.
 */

#include "sh2.h"
#include "sdk/eekernel.h"

static int ___dbPrintThre(int tid, int (*printf)(void)) {
    int ret;
    char str[256];
    struct ThreadParam thre;
    char *help;

    ret = ReferThreadStatus(tid, &thre);
    if (ret) {
        help = (char *)(thre.option & 0x1FFFFFF);
        if (help < (char *)0x100000 || help >= (char *)0x2000000 || help[0] != 'f' || help[1] != 'o' || help[2] != 'r') {
            help = "?";
        }
        UtilStrCpyL(str, help, 255);
        str[255] = 0;
    }
    return ret;
}

static void ___dbPrintThreAll(int (*printf)(void)) {
    int tid;
    int count;

    count = 0;
    for (tid = 0; tid < 256; tid++) {
        if (___dbPrintThre(tid, printf)) {
            count++;
        }
    }
}

/**
 * Walks every thread id at the debug font position (8, 128). The printing itself is empty in this
 * build.
 */
void dbScrPrintThreAll(void) {
    dbfntlocate(8, 128);
    ___dbPrintThreAll((int (*)(void))dbfntprintf);
}
