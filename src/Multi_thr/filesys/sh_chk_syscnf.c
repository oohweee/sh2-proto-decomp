/*
 * sh_chk_syscnf.c: the disc check through SYSTEM.CNF: a disc is accepted when
 * its boot line ("BOOT...:...<number>") carries the game's disc number.
 */

#include "sh2.h"

static u_long128 check_buf[1][128] __attribute__((aligned(64)));
static union fsFile *check_fplist[2] = { z_root_system_cnf__info, NULL };
static void *check_buflist[2] = { check_buf[0], NULL };

static int check_func(union fsFile **fplist, void **buflist) {
    int size;
    int disk_number;
    union fsFile realfp[1];
    char *chp;
    char ch;
    int step;

    if (fsCmdCheckExistFile(fplist[0]) <= 0) {
        return 0;
    }
    if (!fsCmdSetRealFile(realfp, fplist[0])) {
        return 0;
    }
    size = realfp[0].cd.size;
    if (size >= (int)sizeof(check_buf[0]) - 2) {
        size = sizeof(check_buf[0]) - 2;
    }
    if (((char *)buflist[0])[size - 1] == '\n') {
        ((char *)buflist[0])[size] = '\0';
    } else {
        ((char *)buflist[0])[size] = '\n';
        ((char *)buflist[0])[size + 1] = '\0';
    }

    step = 0;
    disk_number = 0;
    for (chp = buflist[0]; (ch = *chp) != '\0'; chp++) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            step = 0;
            continue;
        }
        switch (step) {
        case 0:
            step = (ch != 'B') ? 1 : 2;
            break;
        case 1:
            break;
        case 2:
            if (ch == ':') {
                disk_number = 1;
                step++;
            }
            break;
        default:
            if (ch >= '0' && ch <= '9') {
                disk_number *= 10;
                disk_number += ch - '0';
                step++;
            }
            if (disk_number > 100000) {
                disk_number -= 100000;
                return (disk_number != 20228) ? 0 : disk_number;
            }
            break;
        }
    }
    return -1;
}

/**
 * Makes SYSTEM.CNF the file the disc check reads, with the media types allowed (more with `debug`).
 * Returns the command ID.
 */
int fcSetParamForSystemCnf(int debug) {
    unsigned int media_type;

    media_type = 7;
    if (debug) {
        media_type |= 0x780;
    }
    return fcSetParamForCheckDisk(media_type, check_fplist, check_buflist, check_func);
}
