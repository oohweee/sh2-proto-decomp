/*
 * mc.c: the memory card job system (queued jobs stepped by mcExec), directory
 * scanning, save/load/delete, and the save data codec.
 *
 * All 67 functions match. __stripped_mc_code stands in for a function the
 * original linker dead-stripped; see its comment.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "libc/string.h"
#include "sdk/libmc.h"

static void mcEncodeStart(void);
static void mcCodec(void);
static void mcMakeDirDataSub(void);
static void mcSetDirName(char dn);
static void mcJoinDirName(char dn);
static void mcSetFileName(char dn, char fn);
static void mcSetExtraDirName(char *name);
static void mcSetExtraFileName(char *name);
static unsigned int mcRot(unsigned int n, int s);
static void mcPortError(); /* Matching: K&R; one caller passes the port, which it ignores. */
static void mcAutoInfo(void);
static void mcOpenRW(char *name);
static void mcOpenWO(char *name);
static void mcDirBroken(void);
static sceMcTblGetDir *mcGetDt(char *name);
static void mcOpenRO(char *name);
static int cmpstr(char *str1, char *str2);
static void mcSearchDir(char port);
static void mcSearchDir2(char port);
static void mcLoadData2(char port, short n);
static void mcJobCheckDir(void);
static void mcJobSearchDir(void);
static void mcJobSearchDir2(void);
static void mcJobNewDir(void);
static void mcJobSaveIconSys(void);
static void mcJobSaveIcon(void);
static void mcJobSaveData(void);
static void mcJobLoadData(void);
static void mcJobSaveSystemData(void);
static void mcJobSaveExtraData(void);
static void mcJobLoadExtraData(void);
static void mcJobDeleteData(void);
static void mcJobDeleteDir(void);

static sceMcIconSys mc_IconSys = {
    { 'P', 'S', '2', 'D' },
    0, 0x1A, 0, 0x4D,
    { { 0xEE, 0xE1, 0xD0, 0x0 }, { 0xEE, 0xE1, 0xD0, 0x0 }, { 0xEE, 0xE1, 0xD0, 0x0 }, { 0xEE, 0xE1, 0xD0, 0x0 } },
    { { -0.517884016f, -1.94917595f, -3.4750309f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } },
    { { 1.29999995f, 1.29999995f, 1.29999995f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } },
    { 0.200000003f, 0.220859006f, 0.33129999f, 0.0f },
    "\x82\x72\x82\x89\x82\x8C\x82\x85\x82\x8E\x82\x94\x81\x40\x82\x67\x82\x89\x82\x8C\x82\x8C\x81\x40\x82\x51\x82\x65\x82\x89\x82\x8C\x82\x85\x81\x7C\x82\x4F",
    "icon",
    "icon",
    "icon",
};

static sceMcIconSys mc_IconSys2 = {
    { 'P', 'S', '2', 'D' },
    0, 0x1A, 0, 0x4D,
    { { 0xEE, 0xE1, 0xD0, 0x0 }, { 0xEE, 0xE1, 0xD0, 0x0 }, { 0xEE, 0xE1, 0xD0, 0x0 }, { 0xEE, 0xE1, 0xD0, 0x0 } },
    { { -0.878884017f, -2.48931289f, -3.03489304f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } },
    { { 1.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } },
    { 0.300599992f, 0.134968996f, 0.0f, 0.0f },
    "\x82\x72\x82\x89\x82\x8C\x82\x85\x82\x8E\x82\x94\x81\x40\x82\x67\x82\x89\x82\x8C\x82\x8C\x81\x40\x82\x51\x82\x65\x82\x89\x82\x8C\x82\x85\x81\x7C\x82\x4F\x81\x99",
    "icon",
    "icon",
    "icon",
};

static char mc_Dname[19] = "BASLUS-20228 FILE-";
static char mc_Dname_Base[14] = "BASLUS-20228 ";
static char *mc_iconsysname = "icon.sys";
static char mc_message[28] = "\x83\x54\x83\x43\x83\x8C\x83\x93\x83\x67\x83\x71\x83\x8B\x82\x51\x8C\xA9\x82\xBF\x82\xE1\x82\xBE\x82\xDF\x0A";

struct SAVE_DATA_ALL SaveDataAll;
struct MC_WORK mc;
struct MC_WORK2 *mcw __attribute__((aligned(32)));
static char sc1;
static char sc2;

static int cmpstr(char *str1, char *str2) {
    while (*str1 == *str2) {
        if (!*str1) {
            return 1;
        }
        str1++;
        str2++;
    }
    return 0;
}

static void print_sc1(char st) {
    if (sc1 != st) {
        sc1 = st;
        switch (st) {
        case 0:
            printf("mc start check: now checking\n");
            break;
        case 1:
            printf("mc start check: OK\n");
            break;
        case 2:
            printf("mc start check: autoload slot 1\n");
            break;
        case 3:
            printf("mc start check: autoload slot 2\n");
            break;
        case 4:
            printf("mc start check: autoload end\n");
            break;
        case -1:
            printf("mc start check: no card\n");
            break;
        case -2:
            printf("mc start check: empty block\n");
            break;
        case -3:
            printf("mc start check: autoload failure\n");
            break;
        }
    }
}

static void print_sc2(char st) {
    if (sc2 != st) {
        sc2 = st;
        switch (st) {
        case 0:
            printf("mc start check2: now checking or no data\n");
            break;
        case 1:
            printf("mc start check2: load & continue\n");
            break;
        case 2:
            printf("mc start check2: continue only\n");
            break;
        case 3:
            printf("mc start check2: load only\n");
            break;
        }
    }
}

static void mcNextJob(void) {
    mcw->job = 0;
    mcw->job_num = (mcw->job_num + 1) % 16;
}

static void mcBreakJob(void) {
    printf("break %d:%d result:%d\n", mcw->job, mcw->job_step, mcw->result);
    mcNextJob();
    mcw->job_num = mcw->job_end;
    mc.status |= 2;
}

static void mcBreakJob2(void) {
    printf("break %d:%d result:%d\n", mcw->job, mcw->job_step, mcw->result);
    mcNextJob();
    mcw->job_num = mcw->job_end;
    mc.status |= 0x800;
}

/* Matching: K&R definition, see the declaration above. */
static void mcPortError() {
    int i;

    printf("port error:%d %d:%d result:%d\n", mcw->job_port, mcw->job, mcw->job_step, mcw->result);
    mcNextJob();
    for (i = 0; i < 5; i++) {
        mcw->dirstatus[mcw->job_port][i] = 0;
        mcw->dirid[mcw->job_port][i] = -1;
    }
    mcSetGetInfo(mcw->job_port);
    if (mcw->menu_port == mcw->job_port) {
        mc.status |= 0x802;
    }
}

static void mcDirBroken(void) {
    int fn;

    printf("dir broken %d:%d\n", mcw->job, mcw->job_step);
    mcw->job_step = -1;
    mcw->dirstatus[mcw->port][mcw->dirnum] = 6;
    mcw->dirid[mcw->port][mcw->dirnum] = -1;
    fn = mcw->files++;
    mcw->tmpinfo[fn].savecount = 1;
    mcw->tmpinfo[fn].dirid = mcw->dirnum;
    mcw->tmpinfo[fn].status = 0x80;
    mcSetGetInfo2(mcw->port);
}

static void mcPortAbnormal(char port) {
    int i;

    for (i = 0; i < 5; i++) {
        mcw->dirstatus[port][i] = 8;
        mcw->dirid[port][i] = -1;
    }
    mcw->filemax[port] = 0;
    mcw->d_ent[port] = 0;
    if (mcw->menu_port == port) {
        mc.status |= 0x802;
    }
}

/** Clears the memory card work (placed in the shared gp data buffer).
 * @return the memory card state (mc) */
struct MC_WORK *mcInit(void) {
    mc.status = 0;
    mcw = (struct MC_WORK2 *)(MemShare_gp_data_buf + 0x140000);
    shQzero(mcw, sizeof(struct MC_WORK2));
    return &mc;
}

/** Per-frame memory card update: runs the save data codec, keeps polling the cards and steps the
 * current job, starting the next queued one when it ends. */
void mcExec(void) {
    char port;
    short jd;
    char n;

    if (mcw->cd.mode) {
        mcCodec();
    }
    if (!mcw->portstatus[0]) {
        mcSetGetInfo(0);
    }
    if (!mcw->portstatus[1]) {
        mcSetGetInfo(1);
    }
    if (!mcw->job) {
        mcw->job_step = 0;
        if (mcw->job_num != mcw->job_end) {
            mcw->job = mcw->jobs[mcw->job_num].job;
            mcw->job_data = mcw->jobs[mcw->job_num].data;
            port = mcw->job_port = mcw->jobs[mcw->job_num].port;
            switch (mcw->job) {
            case 1:
                mcw->type_old = mcw->type[port] | mcw->format[port] << 4;
                mcw->retry = 0;
                sceMcGetInfo(port, 0, &mcw->type[port], NULL, &mcw->format[port]);
                mcw->job_step = 1;
                break;
            case 2:
                mcw->type_old = mcw->type[port] | mcw->format[port] << 4;
                mcw->retry = 0;
                sceMcGetInfo(port, 0, &mcw->type[port], &mcw->free[port], &mcw->format[port]);
                mcw->job_step = 1;
                break;
            case 16:
                sceMcGetInfo(port, 0, NULL, NULL, NULL);
                mc.status &= ~2;
                mcw->job_step = 1;
                break;
            case 0:
                mcNextJob();
                return;
            }
        }
    }
    if (!mcw->job) {
        if (mc.status & 8) {
            mcw->info_count++;
        }
        if (mcw->info_count >= 5) {
            mcAutoInfo();
        }
        return;
    }
    if (!sceMcSync(1, 0, &mcw->result)) {
        return;
    }
    port = mcw->job_port;
    switch (mcw->job) {
    case 1:
        if (mcw->result) {
            mcSetGetInfo(port);
            mcNextJob();
            break;
        }
    case 2:
        if (mcw->job_step == 2) {
            if (mcw->result < 0) {
                mcSetGetInfo(port);
                mcNextJob();
                break;
            }
            if (mcw->result) {
                mcw->d_ent[port] |= 1;
            } else {
                mcw->d_ent[port] &= ~1;
            }
            mcw->portstatus[port] = mcw->free[port] < ((mcw->d_ent[port] & 1) ? 0 : 1) + 0x5D ? 3 : 2;
            mcNextJob();
            break;
        }
        mcw->info_count = 0;
        mcw->info[port] = mcw->result;
        if (mcw->result < 0 || mcw->type_old != (mcw->type[port] | mcw->format[port] << 4) || !mcw->format[port]) {
            if (++mcw->retry < 4) {
                mcw->type_old = mcw->type[port] | mcw->format[port] << 4;
                if (mcw->job == 1) {
                    sceMcGetInfo(port, 0, &mcw->type[port], NULL, &mcw->format[port]);
                } else {
                    sceMcGetInfo(port, 0, &mcw->type[port], &mcw->free[port], &mcw->format[port]);
                }
                break;
            }
        }
        n = 0;
        switch (mcw->type[port]) {
        case 1:
            mcPortAbnormal(port);
            n = 6;
            break;
        case 3:
            mcPortAbnormal(port);
            n = 7;
            break;
        case 0:
            mcPortAbnormal(port);
            n = 5;
            break;
        case 2:
            if (!mcw->format[port]) {
                mcPortAbnormal(port);
                n = 4;
                break;
            }
            if (mcw->job == 2) {
                n = mcw->portstatus[port];
                sceMcGetEntSpace(port, 0, "/");
                mcw->job_step = 2;
            } else {
                if (mcw->free[port] < ((mcw->d_ent[port] & 1) ? 0 : 1) + 0x5D) {
                    n = 3;
                } else {
                    n = 2;
                }
            }
            break;
        }
        mcw->portstatus[port] = n;
        if (mcw->job_step != 2) {
            mcNextJob();
        }
        break;
    case 3:
        mcJobCheckDir();
        break;
    case 4:
        mcJobSearchDir();
        break;
    case 5:
        mcJobSearchDir2();
        break;
    case 6:
        mcJobNewDir();
        break;
    case 7:
        mcJobSaveIconSys();
        break;
    case 8:
        mcJobSaveIcon();
        break;
    case 9:
        mcJobSaveData();
        break;
    case 10:
        mcJobLoadData();
        break;
    case 11:
        mcJobSaveSystemData();
        break;
    case 12:
        mcJobSaveExtraData();
        break;
    case 13:
        mcJobLoadExtraData();
        break;
    case 14:
        mcJobDeleteData();
        break;
    case 15:
        mcJobDeleteDir();
        break;
    case 16:
        if (mcw->result < 0 || mc.status & 2) {
            mcPortError(port);
            break;
        }
        switch (mcw->job_step++) {
        case 1:
            sceMcFormat(port, 0);
            break;
        case 2:
            mcSetGetInfo(port);
            mcNextJob();
            break;
        }
        break;
    default:
        mcNextJob();
        break;
    }
}

/** Queues a memory card job (16 entries).
 * @param job job number
 * @param port card port
 * @param data job argument (directory, file...) */
void mcSetJob(char job, char port, short data) {
    if ((mcw->job_end + 1) % 16 == mcw->job_num) {
        printf("mcSetJob: buffer over!\n");
        return;
    }
    mcw->jobs[mcw->job_end].job = job;
    mcw->jobs[mcw->job_end].port = port;
    mcw->jobs[mcw->job_end].data = data;
    mcw->job_end = (mcw->job_end + 1) % 16;
}

static void mcAutoInfo(void) {
    int num;
    int job;
    int cp[2];

    mcw->info_count = 0;
    cp[0] = cp[1] = 0;
    for (num = mcw->job_num; num != mcw->job_end; num = (num + 1) % 16) {
        job = mcw->jobs[num].job;
        if (job == 1 || job == 2) {
            cp[mcw->jobs[num].port] = 1;
        }
    }
    if (!cp[0]) {
        mcSetJob(1, 0, 0);
    }
    if (!cp[1]) {
        mcSetJob(1, 1, 0);
    }
}

/** Queues a card-info check of a port, dropping queued info jobs of that port.
 * @param port card port */
void mcSetGetInfo(char port) {
    int num;
    int job;

    mcw->info_count = 0;
    for (num = mcw->job_num; num != mcw->job_end; num = (num + 1) % 16) {
        if (mcw->jobs[num].port == port) {
            job = mcw->jobs[num].job;
            if (job == 1) {
                mcw->jobs[num].job = 0;
            } else if (job == 2) {
                return;
            }
        }
    }
    mcSetJob(2, port, 0);
}

/** Queues a card-info check of a port unless one is already queued.
 * @param port card port */
void mcSetGetInfo2(char port) {
    int num;
    int job;

    mcw->info_count = 0;
    for (num = mcw->job_num; num != mcw->job_end; num = (num + 1) % 16) {
        if (mcw->jobs[num].port == port) {
            job = mcw->jobs[num].job;
            if (job == 1 || job == 2) {
                return;
            }
        }
    }
    mcSetJob(1, port, 0);
}

/** Clears the job queue and queues a full check of both cards. */
void mcCheckAll(void) {
    int port;
    int i;

    mcw->job_end = 0;
    mcw->job_num = 0;
    mcw->job = 0;
    for (port = 0; port < 2; port++) {
        mcw->portstatus[0] = 1; /* @bug [0], not [port]: port 1's status is never reset here */
        for (i = 0; i < 5; i++) {
            mcw->dirstatus[port][i] = 1;
        }
    }
    mcw->portstatus[2] = 0;
    mcSetGetInfo(0);
    mcSetGetInfo(1);
    mcCheckDir(0);
    mcCheckDir(1);
}

/** Makes the next menu call re-initialize the memory card work. */
void mcStepInit(void) {
    mc.status &= ~4;
}

/** Start-up card check, one step per call: looks for save data (and the auto-load file) on
 * both cards.
 * @return 0 while checking, else the result code (negative: the check failed, see the menu steps) */
int mcStartCheck(void) {
    int i;
    int n;
    char p1;
    char p2;

    mcExec();
    if (!(mc.status & 4)) {
        mcInit();
        mc.status |= 4;
    }
    switch (mcw->menu_step) {
    case 0:
        for (i = 0; i < 2; i++) {
            mcSetGetInfo(i);
            mcw->portstatus[i] = 1;
        }
        mcw->autoload = -1;
        mcw->menu_step++;
        break;
    case 1:
        p1 = mcw->portstatus[0];
        p2 = mcw->portstatus[1];
        if (p1 >= 5) {
            mcw->menu_info |= 1;
            if (p2 >= 5) {
                mcw->tmp_status[0] = mcw->portstatus[0];
                mcw->tmp_status[1] = mcw->portstatus[1];
                mc.status |= 8;
                mcw->menu_step = 4;
                print_sc1(-1);
                return -1;
            }
        } else if (p2 >= 5) {
            mcw->menu_info |= 2;
        }
        if (p1 == 4) {
            mcw->menu_info |= 5;
        }
        if (p2 == 4) {
            mcw->menu_info |= 10;
        }
        if ((mcw->menu_info & 3) == 3) {
            if (mcw->menu_info & 0xC) {
                mcw->menu_step = 0;
                print_sc1(1);
                return 1;
            }
            mcw->tmp_status[0] = mcw->portstatus[0];
            mcw->tmp_status[1] = mcw->portstatus[1];
            mc.status |= 8;
            mcw->menu_step = 4;
            print_sc1(-2);
            return -2;
        }
        if ((p1 == 2 || p1 == 3) && !(mcw->menu_info & 1)) {
            mcSearchDir(0);
            mcw->menu_step = 2;
        }
        if ((p2 == 2 || p2 == 3) && !(mcw->menu_info & 2)) {
            mcSearchDir(1);
            mcw->menu_step = 2;
        }
        break;
    case 2:
        if (mcw->dirnum >= 5) {
            if (mcw->menu_num[0] != -1) {
                mcw->autoload = mcw->menu_num[0] + mcw->port * 75;
                mcLoadData2(mcw->port, mcw->menu_num[0]);
                mcw->menu_step = 3;
                print_sc1(0);
                return 0;
            }
            n = 0;
            for (i = 0; i < 5; i++) {
                if (mcw->dirstatus[mcw->port][i] >= 6) {
                    n++;
                }
            }
            if (n < 5) {
                mcw->menu_info |= 4 << mcw->port;
            }
            mcw->menu_info |= 1 << mcw->port;
            mcw->menu_step = 1;
        }
        break;
    case 3:
        if (mc.status & 2) {
            mcw->tmp_status[0] = mcw->portstatus[0];
            mcw->tmp_status[1] = mcw->portstatus[1];
            mc.status |= 8;
            mcw->menu_step = 4;
            print_sc1(-3);
            return -3;
        }
        if (mc.status & 1) {
            mcw->menu_step = 0;
            print_sc1(4);
            return 4;
        }
        print_sc1(mcw->port + 2);
        return mcw->port + 2;
    case 4:
        if (mcw->tmp_status[0] != mcw->portstatus[0] || mcw->tmp_status[1] != mcw->portstatus[1]) {
            mcStepInit();
        }
        break;
    }
    print_sc1(0);
    return 0;
}

/** Second start-up check, one step per call (after the first has found the cards).
 * @return 0 while checking, else the result code */
int mcStartCheck2(void) {
    int i;
    int n;
    int f;
    char p1;
    char p2;

    mcExec();
    if (!(mc.status & 4)) {
        mcInit();
        mc.status |= 4;
    }
    switch (mcw->menu_step) {
    case 0:
        for (i = 0; i < 2; i++) {
            mcSetGetInfo(i);
            mcw->portstatus[i] = 1;
            for (n = 0; n < 5; n++) {
                mcw->dirstatus[i][n] = 1;
            }
        }
        mcw->menu_info = 0;
        if (SaveDataAll.d.version != 4) {
            mcw->autoload = -1;
        } else {
            mcw->autoload = -2;
        }
        mcw->menu_step++;
        break;
    case 1:
        p1 = mcw->portstatus[0];
        p2 = mcw->portstatus[1];
        if (p1 >= 4) {
            mcw->menu_info |= 1;
            if (mcw->autoload >= 0 && mcw->autoload < 75) {
                mcw->autoload = -1;
            }
        }
        if (p2 >= 4) {
            mcw->menu_info |= 2;
            if (mcw->autoload >= 75 && mcw->autoload < 150) {
                mcw->autoload = -1;
            }
        }
        if ((mcw->menu_info & 3) == 3) {
            mcw->tmp_status[0] = mcw->portstatus[0];
            mcw->tmp_status[1] = mcw->portstatus[1];
            mcw->menu_step = 3;
            if (mcw->autoload >= 0) {
                mcLoadData2(mcw->autoload / 75, mcw->autoload % 75);
            }
            break;
        }
        if ((p1 == 2 || p1 == 3) && !(mcw->menu_info & 1)) {
            mcSearchDir2(0);
            mcw->menu_step = 2;
        }
        if ((p2 == 2 || p2 == 3) && !(mcw->menu_info & 2)) {
            mcSearchDir2(1);
            mcw->menu_step = 2;
        }
        break;
    case 2:
        if (mcw->dirnum >= 5) {
            if (mcw->autoload == -1) {
                if (mcw->menu_num[0] != -1) {
                    mcw->autoload = mcw->menu_num[0] + mcw->port * 75;
                } else if (mcw->menu_num[1] != -1) {
                    mcw->autoload = mcw->menu_num[1] + mcw->port * 75;
                }
            }
            f = 0;
            n = 0;
            for (i = 0; i < 5; i++) {
                if (mcw->dirstatus[mcw->port][i] <= 2 || mcw->dirstatus[mcw->port][i] >= 6) {
                    n++;
                }
                if (mcw->dirstatus[mcw->port][i] == 6) {
                    f = 1;
                }
            }
            if (n < 5) {
                mcw->menu_info |= 4 << mcw->port;
            } else if (f == 1) {
                mcw->menu_info |= 0x10;
            }
            mcw->menu_info |= 1 << mcw->port;
            mcw->menu_step = 1;
        }
        break;
    case 3:
        if (mcw->portstatus[0] != mcw->tmp_status[0] || mcw->portstatus[1] != mcw->tmp_status[1]) {
            if (mcw->autoload != -2) {
                SaveDataAll.d.version = 0;
            }
            mcw->menu_step = 0;
        }
        if (mcw->autoload >= 0 && mc.status & 2) {
            mcw->autoload = -1;
            mcw->menu_info &= ~0xC;
            mcw->menu_info |= 0x10;
            SaveDataAll.d.version = 0;
            break;
        }
        if (mcw->autoload < 0 || mc.status & 1) {
            if (mcw->menu_info & 0xC) {
                if (mcw->autoload == -1) {
                    print_sc2(3);
                    return 3;
                }
                print_sc2(1);
                return 1;
            }
            if (mcw->menu_info & 0x10) {
                if (mcw->autoload == -2) {
                    print_sc2(1);
                    return 1;
                }
                print_sc2(3);
                return 3;
            }
        }
        break;
    }
    if (mcw->autoload == -2) {
        print_sc2(2);
        return 2;
    }
    print_sc2(0);
    return 0;
}


/** Queues a directory check of a port unless one is already queued.
 * @param port card port */
void mcCheckDir(char port) {
    int num;

    for (num = mcw->job_num; num != mcw->job_end; num = (num + 1) % 16) {
        if (mcw->jobs[num].port == port && mcw->jobs[num].job == 3) {
            return;
        }
    }
    mcSetJob(3, port, 0);
    mc.status &= ~2;
}

static void mcJobCheckDir(void) {
    char port;
    int i;
    int n;
    int fn;
    int f;
    sceMcTblGetDir *dt;
    char *ename;

    port = mcw->job_port;
    switch (mcw->job_step) {
    case 0:
        mcw->port = port;
        mcw->dirnum = 0;
        mcw->files = 0;
        mcw->tmp_num = -1;
        if (mcw->portstatus[port] >= 4) {
            mcw->filemax[port] = 0;
            for (i = 0; i < 5; i++) {
                mcw->dirstatus[port][i] = 8;
            }
            mcNextJob();
            break;
        }
        for (i = 0; i < 5; i++) {
            mcw->dirstatus[port][i] = 1;
        }
        sceMcGetInfo(port, 0, NULL, NULL, NULL);
        mcw->job_step++;
        break;
    case 1:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->job_step++;
    case 2:
        mcSetDirName(mcw->dirnum);
        strcat(mcw->fname, "*");
        sceMcGetDir(port, 0, mcw->fname, 0, 20, mcw->dirtbl);
        mcw->job_step++;
        break;
    case 3:
        if (mcw->result == -4) {
            if (mcw->free[port] >= ((mcw->d_ent[port] & 1) ? 0 : 1) + 0x5D) {
                mcw->dirstatus[port][mcw->dirnum] = 2;
            } else {
                mcw->dirstatus[port][mcw->dirnum] = 7;
            }
            mcw->job_step = -1;
            break;
        }
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->tbl_files = mcw->result;
        mcw->fname[0] = 0;
        mcJoinDirName(mcw->dirnum);
        dt = mcw->dirtbl;
        f = 0;
        for (i = 0; i < mcw->tbl_files; i++, dt++) {
            ename = (char *)dt->EntryName;
            if (ename[0] == '.') {
                continue;
            }
            if (cmpstr(ename, mcw->fname)) {
                if (dt->FileSizeByte != sizeof(struct MC_DIRDATA)) {
                    f = 0;
                    break;
                }
                f |= 1;
                continue;
            }
            if (cmpstr(ename, mc_iconsysname)) {
                if (dt->FileSizeByte != sizeof(sceMcIconSys)) {
                    f = 0;
                    break;
                }
                f |= 2;
                continue;
            }
            if (cmpstr(ename, (char *)mc_IconSys.FnameView)) {
                if (dt->FileSizeByte != 0x13EB8 && dt->FileSizeByte != 0xD198) {
                    f = 0;
                    break;
                }
                f |= 4;
                continue;
            }
            if (ename[0] != 'D' || ename[1] != 'A' || ename[2] != 'T' || ename[3] != 'A' || ename[4] != '-' ||
                ename[5] < '0' || ename[5] > '9' || ename[6] < '0' || ename[6] > '9' || ename[7] != 0) {
                f = 0;
                break;
            }
            n = ename[6] - '1' + (ename[5] - '0') * 10;
            if (n < 0 || n >= 15) {
                f = 0;
                break;
            }
        }
        if (f != 7) {
            mcDirBroken();
            break;
        }
        mcSetDirName(mcw->dirnum);
        mcJoinDirName(mcw->dirnum);
        mcOpenRO(mcw->fname);
        mcw->job_step++;
        break;
    case 4:
        if (mcw->result < 0) {
            mcDirBroken();
            break;
        }
        mcw->fd = mcw->result;
        sceMcRead(mcw->fd, &mcw->dirdata, sizeof(mcw->dirdata));
        mcw->job_step++;
        break;
    case 5:
        if (mcw->result < 0) {
            mcDirBroken();
            break;
        }
        sceMcClose(mcw->fd);
        if (mcExtDirData()) {
            mcDirBroken();
            break;
        }
        mcw->job_step++;
        break;
    case 6:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->dirid[port][mcw->dirnum] = mcw->dirdata.id;
        n = 0;
        fn = mcw->files;
        for (i = 0; i < 15; i++) {
            if (mcw->dirdata.file[i].savecount) {
                mcSetFileName(mcw->dirnum, i);
                dt = mcGetDt(mcw->fname);
                if (!dt || dt->FileSizeByte != sizeof(struct MC_SAVEDATA)) {
                    mcw->dirdata.file[i].status = 0x80;
                }
                if (mcw->dirdata.file[i].status & 0x1F) {
                    mc.status |= 0x10;
                }
                memcpy(&mcw->tmpinfo[fn], &mcw->dirdata.file[i], sizeof(struct MC_FILEINFO));
                if (port == mc.ls_port && mcw->dirnum == mc.ls_dir && i == mc.ls_file && mcw->dirdata.id == mc.ls_dir_id) {
                    mcw->tmp_num = fn;
                }
                fn++;
                n++;
            }
        }
        mcw->files = fn;
        if (n == 15) {
            mcw->dirstatus[port][mcw->dirnum] = 5;
            mcw->d_ent[port] &= ~(2 << mcw->dirnum);
            mcw->job_step = -1;
            break;
        }
        mcSetDirName(mcw->dirnum);
        sceMcGetEntSpace(port, 0, mcw->fname);
        mcw->job_step++;
        break;
    case 7:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        if (mcw->result) {
            mcw->d_ent[port] |= 2 << mcw->dirnum;
            n = 0;
        } else {
            mcw->d_ent[port] &= ~(2 << mcw->dirnum);
            n = 1;
        }
        if (mcw->free[port] - n < 8) {
            mcw->dirstatus[port][mcw->dirnum] = 4;
        } else {
            mcw->dirstatus[port][mcw->dirnum] = 3;
        }
    case -1:
        if (++mcw->dirnum < 5) {
            mcw->job_step = 2;
            break;
        }
        memcpy(mcw->fileinfo[port], mcw->tmpinfo, mcw->files * sizeof(struct MC_FILEINFO));
        mcw->filemax[port] = mcw->files;
        if (mcw->tmp_num != -1) {
            mcw->menu_port = port;
            mcw->menu_base[port] = mcw->menu_num[port] = mcw->tmp_num;
            if (mcw->menu_base[port] > 0) {
                mcw->menu_base[port]--;
            }
            if (mcw->menu_base[port] > mcw->files - 4) {
                if (mcw->files < 5) {
                    mcw->menu_base[port] = 0;
                } else {
                    mcw->menu_base[port] = mcw->files - 5;
                }
            }
        }
        mcNextJob();
        break;
    }
}


static void mcSearchDir(char port) {
    mcw->dirnum = 0;
    mcSetJob(4, port, 0);
}

static void mcJobSearchDir(void) {
    char port;
    int i;
    int fn;
    int f;
    int n;
    int tn;
    sceMcTblGetDir *dt;
    unsigned long t;
    unsigned long dtime;
    char *ename;

    port = mcw->job_port;
    switch (mcw->job_step) {
    case 0:
        mcw->port = port;
        for (i = 0; i < 5; i++) {
            mcw->dirstatus[port][i] = 1;
        }
        mcw->menu_num[0] = -1;
        mcw->dtime[0] = 0;
        mcw->dirnum = 0;
        mcw->files = 0;
        mcw->job_step++;
    case 1:
        mcSetDirName(mcw->dirnum);
        mcJoinDirName(mcw->dirnum);
        mcOpenRO(mcw->fname);
        mcw->job_step++;
        break;
    case 2:
        if (mcw->result == -4) {
            if (mcw->free[port] >= ((mcw->d_ent[port] & 1) ? 0 : 1) + 0x5D) {
                mcw->dirstatus[port][mcw->dirnum] = 2;
            } else {
                mcw->dirstatus[port][mcw->dirnum] = 7;
            }
            mcw->job_step = -1;
            break;
        }
        if (mcw->result < 0) {
            mcDirBroken();
            break;
        }
        mcw->fd = mcw->result;
        sceMcRead(mcw->fd, &mcw->dirdata, sizeof(mcw->dirdata));
        mcw->job_step++;
        break;
    case 3:
        if (mcw->result < 0) {
            mcDirBroken();
            break;
        }
        sceMcClose(mcw->fd);
        if (mcExtDirData()) {
            mcDirBroken();
            break;
        }
        mcw->job_step++;
        break;
    case 4:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcSetDirName(mcw->dirnum);
        strcat(mcw->fname, "*");
        sceMcGetDir(port, 0, mcw->fname, 0, 20, mcw->dirtbl);
        mcw->job_step++;
        break;
    case 5:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->tbl_files = mcw->result;
        dt = mcw->dirtbl;
        tn = -1;
        f = 0;
        dtime = mcw->dtime[0];
        mcw->fname[0] = 0;
        mcJoinDirName(mcw->dirnum);
        for (i = 0; i < mcw->result; i++, dt++) {
            ename = (char *)dt->EntryName;
            if (ename[0] == '.') {
                continue;
            }
            if (cmpstr(ename, mcw->fname)) {
                if (dt->FileSizeByte != sizeof(struct MC_DIRDATA)) {
                    f = 0;
                    break;
                }
                f |= 1;
                continue;
            }
            if (cmpstr(ename, mc_iconsysname)) {
                if (dt->FileSizeByte != sizeof(sceMcIconSys)) {
                    f = 0;
                    break;
                }
                f |= 2;
                continue;
            }
            if (cmpstr(ename, (char *)mc_IconSys.FnameView)) {
                if (dt->FileSizeByte != 0x13EB8 && dt->FileSizeByte != 0xD198) {
                    f = 0;
                    break;
                }
                f |= 4;
                continue;
            }
            if (ename[0] != 'D' || ename[1] != 'A' || ename[2] != 'T' || ename[3] != 'A' || ename[4] != '-' ||
                ename[5] < '0' || ename[5] > '9' || ename[6] < '0' || ename[6] > '9' || ename[7] != 0) {
                f = 0;
                break;
            }
            n = ename[6] - '1' + (ename[5] - '0') * 10;
            if (n < 0 || n >= 15) {
                f = 0;
                break;
            }
            if (dt->FileSizeByte != sizeof(struct MC_SAVEDATA)) {
                mcw->dirdata.file[n].status = 0x80;
                continue;
            }
            if (mcw->dirdata.playing.auto_load && n == mcw->dirdata.lastsave) {
                t = *(unsigned long *)&dt->_Modify;
                if (t > dtime) {
                    dtime = t;
                    tn = n;
                }
            }
        }
        if (f != 7) {
            mcDirBroken();
            break;
        }
        mcw->dirid[port][mcw->dirnum] = mcw->dirdata.id;
        fn = mcw->files;
        for (i = 0; i < 15; i++) {
            if (mcw->dirdata.file[i].savecount) {
                mcSetFileName(mcw->dirnum, i);
                if (!mcGetDt(mcw->fname)) {
                    mcw->dirdata.file[i].status |= 0x80;
                }
                memcpy(&mcw->tmpinfo[fn], &mcw->dirdata.file[i], sizeof(struct MC_FILEINFO));
                if (mcw->dirdata.file[i].fileid == tn && !(mcw->dirdata.file[i].status & 0x80)) {
                    mcw->dtime[0] = dtime;
                    mcw->menu_num[0] = fn;
                }
                fn++;
            }
        }
        mcw->files = fn;
        mcw->dirstatus[port][mcw->dirnum] = 5;
    case -1:
        if (++mcw->dirnum < 5) {
            mcw->job_step = 1;
            break;
        }
        memcpy(mcw->fileinfo[port], mcw->tmpinfo, mcw->files * sizeof(struct MC_FILEINFO));
        mcw->filemax[port] = mcw->files;
        mcNextJob();
        break;
    }
}


static void mcSearchDir2(char port) {
    mcw->dirnum = 0;
    mcSetJob(5, port, 0);
}

static void mcJobSearchDir2(void) {
    char port;
    int i;
    int fn;
    int f;
    int n;
    int tn;
    sceMcTblGetDir *dt;
    unsigned long t;
    unsigned long dtime;
    char *ename;

    port = mcw->job_port;
    switch (mcw->job_step) {
    case 0:
        mcw->port = port;
        for (i = 0; i < 5; i++) {
            mcw->dirstatus[port][i] = 1;
        }
        mcw->menu_num[0] = -1;
        mcw->menu_num[1] = -1;
        mcw->dtime[1] = 0;
        mcw->dtime[0] = 0;
        mcw->dirnum = 0;
        mcw->files = 0;
        mcw->filemax[port] = 0;
        mcw->job_step++;
    case 1:
        mcSetDirName(mcw->dirnum);
        mcJoinDirName(mcw->dirnum);
        mcOpenRO(mcw->fname);
        mcw->job_step++;
        break;
    case 2:
        if (mcw->result == -4) {
            mcw->fname[0] = '/';
            mcw->fname[1] = 0;
            mcJoinDirName(mcw->dirnum);
            sceMcGetDir(port, 0, mcw->fname, 0, 20, mcw->dirtbl);
            mcw->job_step = -2;
            break;
        }
        if (mcw->result < 0) {
            mcDirBroken();
            break;
        }
        mcw->fd = mcw->result;
        sceMcRead(mcw->fd, &mcw->dirdata, sizeof(mcw->dirdata));
        mcw->job_step++;
        break;
    case 3:
        if (mcw->result < 0) {
            mcDirBroken();
            break;
        }
        sceMcClose(mcw->fd);
        if (mcExtDirData()) {
            mcDirBroken();
            break;
        }
        mcw->job_step++;
        break;
    case 4:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcSetDirName(mcw->dirnum);
        strcat(mcw->fname, "*");
        sceMcGetDir(port, 0, mcw->fname, 0, 20, mcw->dirtbl);
        mcw->job_step++;
        break;
    case 5:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->tbl_files = mcw->result;
        dt = mcw->dirtbl;
        tn = -1;
        f = 0;
        if (mcw->dirdata.lastsave == -1) {
            dtime = mcw->dtime[1];
        } else {
            dtime = mcw->dtime[0];
        }
        mcw->fname[0] = 0;
        mcJoinDirName(mcw->dirnum);
        for (i = 0; i < mcw->result; i++, dt++) {
            ename = (char *)dt->EntryName;
            if (ename[0] == '.') {
                continue;
            }
            if (cmpstr(ename, mcw->fname)) {
                if (dt->FileSizeByte != sizeof(struct MC_DIRDATA)) {
                    f = 0;
                    break;
                }
                f |= 1;
                continue;
            }
            if (cmpstr(ename, mc_iconsysname)) {
                if (dt->FileSizeByte != sizeof(sceMcIconSys)) {
                    f = 0;
                    break;
                }
                f |= 2;
                continue;
            }
            if (cmpstr(ename, (char *)mc_IconSys.FnameView)) {
                if (dt->FileSizeByte != 0x13EB8 && dt->FileSizeByte != 0xD198) {
                    f = 0;
                    break;
                }
                f |= 4;
                continue;
            }
            if (ename[0] != 'D' || ename[1] != 'A' || ename[2] != 'T' || ename[3] != 'A' || ename[4] != '-' ||
                ename[5] < '0' || ename[5] > '9' || ename[6] < '0' || ename[6] > '9' || ename[7] != 0) {
                f = 0;
                break;
            }
            n = ename[6] - '1' + (ename[5] - '0') * 10;
            if (n < 0 || n >= 15) {
                f = 0;
                break;
            }
            if (!mcw->dirdata.file[n].savecount) {
                continue;
            }
            if (dt->FileSizeByte != sizeof(struct MC_SAVEDATA)) {
                mcw->dirdata.file[n].status = 0x80;
                continue;
            }
            t = *(unsigned long *)&dt->_Modify;
            if (mcw->dirdata.lastsave == -1) {
                if (t > dtime) {
                    dtime = t;
                    tn = n;
                }
            } else if (n == mcw->dirdata.lastsave) {
                if (t > dtime) {
                    dtime = t;
                    tn = n;
                }
            }
        }
        if (f != 7) {
            mcDirBroken();
            break;
        }
        mcw->dirid[port][mcw->dirnum] = mcw->dirdata.id;
        fn = mcw->files;
        for (i = 0; i < 15; i++) {
            if (mcw->dirdata.file[i].savecount) {
                mcSetFileName(mcw->dirnum, i);
                if (!mcGetDt(mcw->fname)) {
                    mcw->dirdata.file[i].status |= 0x80;
                }
                memcpy(&mcw->tmpinfo[fn], &mcw->dirdata.file[i], sizeof(struct MC_FILEINFO));
                if (mcw->dirdata.file[i].fileid == tn && !(mcw->dirdata.file[i].status & 0x80)) {
                    if (mcw->dirdata.lastsave == -1) {
                        mcw->dtime[1] = dtime;
                        mcw->menu_num[1] = fn;
                    } else {
                        mcw->dtime[0] = dtime;
                        mcw->menu_num[0] = fn;
                    }
                }
                fn++;
            }
        }
        mcw->files = fn;
        mcw->dirstatus[port][mcw->dirnum] = 5;
        mcw->job_step = -1;
    case -1:
        if (++mcw->dirnum < 5) {
            mcw->job_step = 1;
            break;
        }
        memcpy(mcw->fileinfo[port], mcw->tmpinfo, mcw->files * sizeof(struct MC_FILEINFO));
        mcw->filemax[port] = mcw->files;
        mcNextJob();
        break;
    case -2:
        if (mcw->result > 0) {
            mcDirBroken();
        } else if (mcw->free[port] >= ((mcw->d_ent[port] & 1) ? 0 : 1) + 0x5D) {
            mcw->dirstatus[port][mcw->dirnum] = 2;
        } else {
            mcw->dirstatus[port][mcw->dirnum] = 7;
        }
        mcw->job_step = -1;
        break;
    }
}


static sceMcTblGetDir *mcGetDt(char *name) {
    int i;
    char *str;
    char *name2;

    name2 = str = name;
    while (*str) {
        str++;
    }
    while (--str > name) {
        if (*str == '/') {
            name2 = str + 1;
            break;
        }
    }
    for (i = 0; i < mcw->tbl_files; i++) {
        if (cmpstr((char *)mcw->dirtbl[i].EntryName, name2)) {
            return &mcw->dirtbl[i];
        }
    }
    return NULL;
}

/** Drops the queued jobs of a port and queues a format of its card.
 * @param port card port */
void mcFormat(char port) {
    int num;

    mcw->info_count = 0;
    for (num = mcw->job_num; num != mcw->job_end; num = (num + 1) % 16) {
        if (mcw->jobs[num].port == port) {
            mcw->jobs[num].job = 0;
        }
    }
    mcw->portstatus[port] = 1;
    mc.status &= ~2;
    if ((mcw->job == 1 || mcw->job == 2) && mcw->job_port == port) {
        mcw->job = 0;
    }
    mcSetJob(16, port, 0);
}

/** Returns non-zero if a port's card isn't ready to use (status other than 2 or 3).
 * @param port card port */
int mcCheckStatus(char port) {
    if (mcw->portstatus[port] != 2 && mcw->portstatus[port] != 3) {
        return 1;
    }
    return 0;
}

/** Checks whether a save can be made on a port's card.
 * @param port card port
 * @return 0 if it can; 1-5 by the state of the card's save directories (5: card not ready) */
int mcCheckCanSave(char port) {
    int i;
    int f;
    int s;
    int b;

    if (mcw->portstatus[port] != 2 && mcw->portstatus[port] != 3) {
        return 5;
    }
    if (mcw->filemax[port] == 75) {
        return 3;
    }
    f = 0;
    b = 0;
    for (i = 0; i < 5; i++) {
        s = mcw->dirstatus[port][i];
        if (s == 3) {
            return 1;
        }
        if (s == 2) {
            f |= 1;
        } else if (s == 4 || s == 5) {
            f |= 2;
        } else if (s == 6) {
            b++;
        } else if (s <= 1 || s == 8) {
            return 5;
        }
    }
    if (f & 1) {
        return 0;
    }
    if (f & 2) {
        return 2;
    }
    if (b == 5) {
        return 3;
    }
    return 4;
}

static void mcJobNewDir(void) {
    switch (mcw->job_step++) {
    case 0:
        sceMcGetInfo(mcw->job_port, 0, NULL, NULL, NULL);
        break;
    case 1:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->dirnum = mcw->job_data;
        mcw->fname[0] = '/';
        mcw->fname[1] = 0;
        mcJoinDirName(mcw->dirnum);
        sceMcMkdir(mcw->job_port, 0, mcw->fname);
        break;
    case 2:
        if (mcw->result != 0 && mcw->result != -4) {
            mcPortError();
        } else {
            mcNextJob();
        }
        break;
    }
}

static void mcOpenWO(char *name) {
    sceMcTblGetDir *dt;

    if (!(dt = mcGetDt(name)) && mcw->free[mcw->job_port] <= 0) {
        mcBreakJob();
        return;
    }
    sceMcOpen(mcw->job_port, 0, name, 0x202);
}

static void mcOpenRO(char *name) {
    sceMcOpen(mcw->job_port, 0, name, 1);
}

static void mcOpenRW(char *name) {
    sceMcOpen(mcw->job_port, 0, name, 3);
}

/** Queues writing the icon.sys file of a save directory.
 * @param port card port
 * @param dn directory number */
void mcSaveIconSys(char port, char dn) {
    mcSetJob(7, port, dn);
}

static void mcJobSaveIconSys(void) {
    switch (mcw->job_step++) {
    case 0:
        mcSetDirName(mcw->job_data);
        strcat(mcw->fname, mc_iconsysname);
        mcOpenWO(mcw->fname);
        break;
    case 1:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->fd = mcw->result;
        if (mc.status & 0x1000) {
            mc_IconSys2.TitleName[36] = 0x82;
            mc_IconSys2.TitleName[37] = mcw->job_data + 0x50;
            sceMcWrite(mcw->fd, &mc_IconSys2, sizeof(mc_IconSys2));
        } else {
            mc_IconSys.TitleName[36] = 0x82;
            mc_IconSys.TitleName[37] = mcw->job_data + 0x50;
            sceMcWrite(mcw->fd, &mc_IconSys, sizeof(mc_IconSys));
        }
        break;
    case 2:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        break;
    case 3:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        if (mc.status & 0x1000) {
            mcw->job = 8;
            mcw->job_step = 0;
        } else {
            mcNextJob();
        }
        break;
    }
}

/** Queues writing the icon file of a save directory.
 * @param port card port
 * @param dn directory number */
void mcSaveIcon(char port, char dn) {
    mcSetJob(8, port, dn);
}

static void mcJobSaveIcon(void) {
    if (!mcCheckEndLoadIconData()) {
        return;
    }
    switch (mcw->job_step++) {
    case 0:
        mcSetDirName(mcw->job_data);
        strcat(mcw->fname, (char *)mc_IconSys.FnameView);
        mcOpenWO(mcw->fname);
        break;
    case 1:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->fd = mcw->result;
        if (mc.status & 0x1000) {
            sceMcWrite(mcw->fd, MemShare_gp_data_buf + 0xE0000, 0xD198);
        } else {
            sceMcWrite(mcw->fd, MemShare_gp_data_buf + 0xC0000, 0x13EB8);
        }
        break;
    case 2:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        if (mc.status & 0x1000) {
            mc.status &= ~0x1000;
            if (mc.status & 0x2000) {
                mc.status |= 1;
                mc.ls_dir_id = mcw->dirdata.id;
                mcNextJob();
            } else {
                mcw->job = 9;
                mcw->job_step = 7;
            }
        }
        break;
    case 3:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcNextJob();
        break;
    }
}

/** Saves the game into a file: encodes the data (if not yet), then queues the directory and
 * file writes the directory still needs.
 * @param port card port
 * @param dn directory number
 * @param fn file number */
void mcSaveData(char port, char dn, char fn) {
    if (!mcw->cd.step) {
        mcMakeSaveData();
    }
    mc.status &= ~3;
    if (mcw->dirstatus[port][dn] == 2) {
        mcSetJob(6, port, dn);
        mcSaveIconSys(port, dn);
        mcSaveIcon(port, dn);
    }
    mcSetJob(9, port, fn + dn * 15);
}

static void mcJobSaveData(void) {
    switch (mcw->job_step++) {
    case 0:
        sceMcGetInfo(mcw->job_port, 0, NULL, NULL, NULL);
        break;
    case 1:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcSetFileName(mcw->job_data / 15, mcw->job_data % 15);
        mcOpenWO(mcw->fname);
        break;
    case 2:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->fd = mcw->result;
        mcCodecAll();
        if (mcw->cd.step == -2) {
            mcBreakJob();
            break;
        }
        sceMcWrite(mcw->fd, &mcw->savedata, sizeof(mcw->savedata));
        break;
    case 3:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        break;
    case 4:
        if (mcw->result) {
            mcPortError();
            break;
        }
        mcSetDirName(mcw->job_data / 15);
        mcJoinDirName(mcw->job_data / 15);
        mcOpenWO(mcw->fname);
        break;
    case 5:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->fd = mcw->result;
        mcMakeDirData(mcw->job_port, mcw->job_data / 15, mcw->job_data % 15);
        sceMcWrite(mcw->fd, &mcw->dirdata, sizeof(mcw->dirdata));
        break;
    case 6:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        break;
    case 7:
        if (mcw->result) {
            mcPortError();
        } else {
            if (mc.status & 0x1000) {
                mcw->job = 7;
                mcw->job_step = 0;
                mcw->job_data = mcw->dirnum;
                break;
            }
            mc.status |= 1;
            mc.ls_port = mcw->job_port;
            mc.ls_dir = mcw->job_data / 15;
            mc.ls_file = mcw->job_data % 15;
            mc.ls_dir_id = mcw->dirid_new;
            playing.savecount++;
            mcMakeSaveData();
        }
        mcSetGetInfo(mcw->job_port);
        mcCheckDir(mcw->job_port);
        mcNextJob();
        break;
    }
}


/** Queues loading a save file.
 * @param port card port
 * @param dn directory number
 * @param fn file number */
void mcLoadData(char port, char dn, char fn) {
    struct MC_FILEINFO *fi;
    int i;

    mc.status &= ~3;
    for (fi = mcw->fileinfo[port], i = 0; i < 75; i++, fi++) {
        if (fi->dirid == dn && fi->fileid == fn) {
            mcw->loaddata = fi;
            break;
        }
    }
    if (i == 75) {
        mc.status |= 2;
        return;
    }
    if (fi->status & 0x80) {
        mc.status |= 2;
        return;
    }
    mcSetJob(10, port, fn + dn * 15);
}

static void mcJobLoadData(void) {
    switch (mcw->job_step++) {
    case 0:
        sceMcGetInfo(mcw->job_port, 0, NULL, NULL, NULL);
        break;
    case 1:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcSetFileName(mcw->job_data / 15, mcw->job_data % 15);
        mcOpenRO(mcw->fname);
        break;
    case 2:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->fd = mcw->result;
        sceMcRead(mcw->fd, &mcw->savedata, sizeof(mcw->savedata));
        break;
    case 3:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcDecodeStart();
        sceMcClose(mcw->fd);
        break;
    case 4:
        mcCodecAll();
        if (mcw->cd.step == -2 || mcw->result != 0) {
            mcPortError();
            break;
        }
        mc.status |= 1;
        mc.ls_port = mcw->job_port;
        mc.ls_dir = mcw->job_data / 15;
        mc.ls_file = mcw->job_data % 15;
        mc.ls_dir_id = mcw->dirid[mc.ls_port][mc.ls_dir];
        mcNextJob();
        break;
    }
}

static void mcLoadData2(char port, short n) {
    struct MC_FILEINFO *fi;

    mc.status &= ~3;
    fi = &mcw->fileinfo[port][n];
    mcw->loaddata = fi;
    if (fi->status & 0x80) {
        mc.status |= 2;
        return;
    }
    mcw->menu_port = port;
    mcSetJob(10, port, fi->fileid + fi->dirid * 15);
}

/** Queues deleting a save file, or its whole directory when it holds no other file.
 * @param port card port
 * @param dn directory number
 * @param fn file number */
void mcDeleteData(char port, char dn, char fn) {
    int i;

    mc.status &= ~3;
    if (port == 2) {
        return;
    }
    if (mcw->dirstatus[port][dn] != 6) {
        for (i = 0; i < mcw->filemax[port]; i++) {
            if (mcw->fileinfo[port][i].dirid == dn && mcw->fileinfo[port][i].fileid != fn) {
                mcSetJob(14, port, fn + dn * 15);
                return;
            }
        }
    }
    mcSetJob(15, port, dn);
}

static void mcJobDeleteData(void) {
    switch (mcw->job_step++) {
    case 0:
        mcSetFileName(mcw->job_data / 15, mcw->job_data % 15);
        sceMcDelete(mcw->job_port, 0, mcw->fname);
        break;
    case 1:
        if (mcw->result != 0 && mcw->result != -4) {
            mcPortError();
            break;
        }
        mcSetDirName(mcw->job_data / 15);
        mcJoinDirName(mcw->job_data / 15);
        mcOpenRW(mcw->fname);
        break;
    case 2:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcRead(mcw->fd, &mcw->dirdata, sizeof(mcw->dirdata)); /* @bug fd not taken from the open's result */
        break;
    case 3:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        if (mcExtDirData()) {
            mcBreakJob(); /* @bug no break: the seek below is still issued */
        }
        sceMcSeek(mcw->fd, 0, 0);
        break;
    case 4:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcMakeDirData2(mcw->job_port, mcw->job_data / 15, mcw->job_data % 15);
        sceMcWrite(mcw->fd, &mcw->dirdata, sizeof(mcw->dirdata));
        break;
    case 5:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        break;
    case 6:
        if (mcw->result) {
            mc.status |= 2;
        } else {
            mc.status |= 1;
        }
        mcSetGetInfo(mcw->job_port);
        mcCheckDir(mcw->job_port);
        mcNextJob();
        break;
    }
}

static void mcJobDeleteDir(void) {
    switch (mcw->job_step) {
    case 0:
        mcw->dirnum = mcw->job_data;
        mcSetDirName(mcw->dirnum);
        strcat(mcw->fname, "*");
        mcw->tmp_num = 0;
        sceMcGetDir(mcw->job_port, 0, mcw->fname, 0, 20, mcw->dirtbl);
        mcw->job_step++;
        break;
    case 1:
        if (mcw->result < 0) {
            if (mcw->result == 4) { /* @bug never true: result is negative here (other jobs test -4) */
                mcw->job_step = 4;
                break;
            }
            mcPortError();
            break;
        }
        mcw->job_data = mcw->result;
        mcw->job_step++;
    case 2:
        while (mcw->dirtbl[mcw->tmp_num].EntryName[0] == '.') {
            if (++mcw->tmp_num >= mcw->job_data) {
                mcw->job_step++;
                break; /* @bug leaves only the loop: the delete below still runs, past the listed entries */
            }
        }
        mcSetDirName(mcw->dirnum);
        strcat(mcw->fname, (char *)mcw->dirtbl[mcw->tmp_num].EntryName);
        sceMcDelete(mcw->job_port, 0, mcw->fname);
        mcw->job_step++;
        break;
    case 3:
        if (mcw->result < 0 && mcw->result != -4) {
            mcPortError();
            break;
        }
        if (++mcw->tmp_num < mcw->job_data) {
            mcw->job_step = 2;
            break;
        }
    case 4:
        mcw->fname[0] = '/';
        mcw->fname[1] = 0;
        mcJoinDirName(mcw->dirnum);
        sceMcDelete(mcw->job_port, 0, mcw->fname);
        mcw->job_step = 5;
        break;
    case 5:
        if (mcw->result < 0 && mcw->result != -4) {
            mcPortError();
            break;
        }
        mc.status |= 1;
        mcSetGetInfo(mcw->job_port);
        mcCheckDir(mcw->job_port);
        mcNextJob();
        break;
    }
}

static void mcJobSaveSystemData(void) {
    switch (mcw->job_step++) {
    case 0:
        if (mcw->portstatus[mc.ls_port] >= 4) {
            mc.status &= ~0x2000;
            mcNextJob();
            break;
        }
        mcSetDirName(mc.ls_dir);
        mcJoinDirName(mc.ls_dir);
        mcOpenRW(mcw->fname);
        break;
    case 1:
        if (mcw->result < 0) {
            mc.status &= ~0x2000;
            mcNextJob();
            break;
        }
        sceMcRead(mcw->fd, &mcw->dirdata, sizeof(mcw->dirdata)); /* @bug fd not taken from the open's result */
        break;
    case 2:
        if (mcw->result < 0) {
            mcBreakJob();
            break;
        }
        if (mcExtDirData()) {
            mcBreakJob(); /* @bug no break: goes on to the id check */
        }
        if (mcw->dirdata.id != mc.ls_dir_id) {
            sceMcClose(mcw->fd);
            mc.status |= 2;
            mcNextJob();
            break;
        }
        sceMcSeek(mcw->fd, 0, 0);
        break;
    case 3:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcMakeDirData3();
        sceMcWrite(mcw->fd, &mcw->dirdata, sizeof(mcw->dirdata));
        break;
    case 4:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        break;
    case 5:
        if (mcw->result) {
            mc.status |= 2;
        } else {
            if (mc.status & 0x1000) {
                mcw->job = 7;
                mcw->job_step = 0;
                mcw->job_data = mc.ls_dir;
                break;
            }
            mc.status |= 1;
            mc.ls_dir_id = mcw->dirdata.id;
        }
        mcNextJob();
        break;
    }
}

/*
 * Matching: the original had an extra-data setter here that nothing calls, so the linker
 * dead-stripped it. Its assert strings ("mc.c", "strlen(name) < 8"), pooled with
 * mcSetExtraDirName's, stayed in .rodata ahead of mcJobSaveExtraData's jump
 * table. Listed in config/stripped_functions.txt. Its name is unknown; the body is a guess.
 */
void __stripped_mc_code(char *name, char *adr, int size) {
    /* Matching: the assert bakes its original line number into the object. */
#line 1988
    fjAssert(strlen(name) < 8);
    strcpy(mcw->extra_name, name);
    mcw->extra_adr = adr;
    mcw->extra_size = size;
}

static void mcJobSaveExtraData(void) {
    char port;
    sceMcTblGetDir *dt;
    char *str1;
    char *str2;

    port = mcw->job_port;
    switch (mcw->job_step++) {
    case 0:
        sceMcGetInfo(port, 0, NULL, NULL, NULL);
        break;
    case 1:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcSetExtraDirName(mcw->extra_name);
        strcat(mcw->fname, "*");
        sceMcGetDir(port, 0, mcw->fname, 0, 20, mcw->dirtbl);
        break;
    case 2:
        if (mcw->result >= 0) {
            if (!(dt = mcGetDt(mc_iconsysname)) || dt->FileSizeByte != sizeof(sceMcIconSys)) {
                mcPortError();
                break;
            }
            if (!(dt = mcGetDt((char *)mc_IconSys2.FnameView)) || dt->FileSizeByte != 0xD198) {
                mcPortError();
                break;
            }
            str1 = mcw->fname;
            str2 = mc_Dname_Base;
            while (*str2) { *str1++ = *str2++; }
            str2 = mcw->extra_name;
            while (*str2) { *str1++ = *str2++; }
            if (!mcGetDt(mcw->fname)) {
                mcPortError();
                break;
            }
            mcw->job_step = 10;
            break;
        }
        if (mcw->result != -4) {
            mcPortError();
            break;
        }
        if (mcw->free[port] < ((mcw->extra_size + 0x3FF) & ~0x3FF) / 0x400 + 0x55 + ((mcw->d_ent[port] & 1) ? 0 : 1)) {
            mcBreakJob2();
            break;
        }
        mcSetExtraDirName(mcw->extra_name);
        sceMcMkdir(port, 0, mcw->fname);
        break;
    case 3:
        if (mcw->result != 0 && mcw->result != -4) {
            mcPortError();
            break;
        }
        mcSetExtraDirName(mcw->extra_name);
        strcat(mcw->fname, mc_iconsysname);
        mcOpenWO(mcw->fname);
        break;
    case 4:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->fd = mcw->result;
        mc_IconSys.TitleName[36] = 0x82;
        mc_IconSys.TitleName[37] = 0x77;
        sceMcWrite(mcw->fd, &mc_IconSys, sizeof(mc_IconSys));
        break;
    case 5:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        break;
    case 6:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcSetExtraDirName(mcw->extra_name);
        strcat(mcw->fname, (char *)mc_IconSys.FnameView);
        mcOpenWO(mcw->fname);
        break;
    case 7:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->fd = mcw->result;
        sceMcWrite(mcw->fd, MemShare_gp_data_buf + 0xC0000, 0x13EB8);
        break;
    case 8:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        break;
    case 9:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->job_step++;
    case 10:
        mcSetExtraFileName(mcw->extra_name);
        mcOpenWO(mcw->fname);
        break;
    case 11:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mcw->fd = mcw->result;
        sceMcWrite(mcw->fd, mcw->extra_adr, mcw->extra_size);
        break;
    case 12:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        break;
    case 13:
        if (mcw->result) {
            mcPortError();
            break;
        }
        mc.status |= 0x400;
        mcNextJob();
        break;
    }
}


static void mcJobLoadExtraData(void) {
    switch (mcw->job_step++) {
    case 0:
        mcSetExtraFileName(mcw->extra_name);
        sceMcGetDir(mcw->job_port, 0, mcw->fname, 0, 20, mcw->dirtbl);
        break;
    case 1:
        if (mcw->result != 1) {
            mcBreakJob2();
            break;
        }
        mcw->extra_size = mcw->dirtbl[0].FileSizeByte;
        mcOpenRO(mcw->fname);
        break;
    case 2:
        if (mcw->result < 0) {
            mcBreakJob2();
            break;
        }
        mcw->fd = mcw->result;
        sceMcRead(mcw->fd, mcw->extra_adr, mcw->extra_size);
        break;
    case 3:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        sceMcClose(mcw->fd);
        break;
    case 4:
        if (mcw->result < 0) {
            mcPortError();
            break;
        }
        mc.status |= 0x400;
        mcNextJob();
        break;
    }
}

static void mcSetDirName(char dn) {
    char *str1;
    char *str2;

    str1 = mcw->fname;
    str2 = mc_Dname;
    *str1++ = '/';
    while (*str2) {
        *str1++ = *str2++;
    }
    str1[0] = dn + '1';
    str1[1] = '/';
    str1[2] = 0;
}

static void mcJoinDirName(char dn) {
    char *str1;
    char *str2;

    str1 = mcw->fname;
    str2 = mc_Dname;
    while (*str1) {
        str1++;
    }
    while (*str2) {
        *str1++ = *str2++;
    }
    str1[0] = dn + '1';
    str1[1] = 0;
}

static void mcSetFileName(char dn, char fn) {
    char *str1;
    char *str2;

    str1 = mcw->fname;
    str2 = "DATA-";
    mcSetDirName(dn);
    while (*str1) {
        str1++;
    }
    while (*str2) {
        *str1++ = *str2++;
    }
    if (fn < 0) {
        *str1++ = '*';
    } else {
        *str1++ = (fn + 1) / 10 + '0';
        *str1++ = (fn + 1) % 10 + '0';
    }
    *str1 = 0;
}

/*
 * Matching: the asserts below bake their original line numbers (2749, 2764) into the
 * binary; the original file had ~500 more lines above this point (comments or
 * dead code we can't recover), so pin the line numbering here.
 */
#line 2743
static void mcSetExtraDirName(char *name) {
    char *str1;
    char *str2;

    str1 = mcw->fname;
    str2 = mc_Dname_Base;
    fjAssert(strlen(name) < 8);

    *str1++ = '/';
    while (*str2) { *str1++ = *str2++; }
    str2 = name;
    while (*str2) { *str1++ = *str2++; }
    *str1 = 0;
}

static void mcSetExtraFileName(char *name) {
    char *str1;
    char *str2;

    str1 = mcw->fname;
    str2 = mc_Dname_Base;
    fjAssert(strlen(name) < 8);
    *str1++ = '/';
    while (*str2) {
        *str1++ = *str2++;
    }
    str2 = name;
    while (*str2) {
        *str1++ = *str2++;
    }
    *str1++ = '/';
    str2 = mc_Dname_Base;
    while (*str2) {
        *str1++ = *str2++;
    }
    str2 = name;
    while (*str2) {
        *str1++ = *str2++;
    }
    *str1 = 0;
}

/** Copies SaveDataAll (save count + 1) into the save buffer and starts encoding it. */
void mcMakeSaveData(void) {
    SaveDataAll.d.playing.savecount++;
    memcpy(mcw->savedata.data, &SaveDataAll, sizeof(SaveDataAll));
    mcEncodeStart();
}

static int mcExtSaveData(void) {
    struct SAVE_DATA *sd;
    struct MC_FILEINFO *fi;
    struct Playing_Info *pi;

    sd = (struct SAVE_DATA *)mcw->savedata.data;
    fi = mcw->loaddata;
    pi = &sd->playing;
    if (fi->scene != sd->scene || fi->savecount != pi->savecount || fi->b_level != pi->battle_level ||
        fi->r_level != pi->riddle_level || fi->status != sd->playing.clear_end_kind || fi->time != pi->time ||
        fzero(pi->total_time + pi->time - fi->total_time) != 0.0f) {
        printf("Verify error\n");
        mcw->cd.step = -2;
        mc.status |= 2;
        return 1;
    }
    memcpy(&SaveDataAll, mcw->savedata.data, sizeof(SaveDataAll));
    return 0;
}

static void mcEncodeStart(void) {
    int i;
    int zc;
    unsigned int csum;
    unsigned char c;

    mcw->cd.mode = 1;
    mcw->cd.step = 0;
    mcw->cd.pos = (unsigned long *)mcw->savedata.data;
    mcw->cd.count = 0x3F8;
    csum = 0;
    zc = 0;
    for (i = 0; i < 24; i++) {
        c = mcw->cd.key1[i] = shRandI() >> 10;
        if (!(c & 0xF)) {
            zc++;
        }
        csum += c;
    }
    for (i = 0; i < 15; i++) {
        c = mcw->cd.key2[i] = shRandI() >> 10;
        if (!(c & 0xF)) {
            zc++;
        }
        csum += c;
    }
    mcw->cd.key2[i] = c = (csum & 0xFF) ^ 0x21;
    if (!(c & 0xF)) {
        zc++;
    }
    csum += c;
    mcw->cd.zcount = zc;
    mcw->cd.csum1 = mcw->cd.csum2 = csum;
    mcw->savedata.pad = shRandI() ^ mcRot(shRandI(), 8);
}

/** Starts decoding the loaded save buffer. */
void mcDecodeStart(void) {
    mcw->cd.mode = 2;
    mcw->cd.step = 0;
    mcw->cd.pos = (unsigned long *)mcw->savedata.data;
    mcw->cd.count = 0x3FD;
    mcw->cd.zcount = 0;
    mcw->cd.csum1 = 0;
    mcw->cd.csum4 = 0;
    memcpy(MemShare_gp_data_buf + 0x148000, &SaveDataAll, sizeof(SaveDataAll.d));
}

static unsigned int mcRot(unsigned int n, int s) {
    s &= 31;
    return (n << s) | (n >> (32 - s));
}

/** Runs the save data codec to its end in one go. */
void mcCodecAll(void) {
    if (mcw->cd.mode) {
        do {
            mcCodec();
            if (mcw->cd.step == -2) {
                printf("codec fail!\n");
                mc.status |= 2;
                break;
            }
        } while (mcw->cd.step != -1);
    }
}

static void mcCodec(void) {
    struct MC_CODEC_DATA *cd;
    unsigned long n;
    unsigned long *pos;
    unsigned long *key1;
    unsigned long key2[16];
    unsigned long csum4;
    int i;
    int j;
    int count;
    int ck1;
    int ck2;
    int zcount;
    int csum;
    int csum2;
    unsigned char *p;
    unsigned char c;

    cd = &mcw->cd;
    key1 = (unsigned long *)cd->key1;
    p = (unsigned char *)key2;
    for (i = 0; i < 16; i++) {
        c = mcw->cd.key2[i];
        for (j = 0; j < 8; j++) {
            *p++ = c;
        }
    }
    switch (cd->mode * 100 + cd->step) {
    case 100:
        pos = cd->pos;
        count = cd->count;
        zcount = cd->zcount;
        csum = cd->csum1;
        i = 0;
        ck2 = 0;
        ck1 = 0;
        do {
            n = *pos;
            n ^= key1[ck1] ^ key2[ck2];
            *pos++ = n;
            if (++ck1 >= 3) {
                ck1 = 0;
                if (++ck2 >= 16) {
                    ck2 = 0;
                }
            }
            for (j = 0; j < 8; j++) {
                if (!(n & 0xF)) {
                    zcount++;
                }
                csum += n & 0xFF;
                n >>= 8;
            }
            if (--count <= 0) {
                cd->step++;
                mcw->savedata.csum1 = mcRot(csum, zcount);
                mcw->savedata.csum2 = mcRot(zcount - cd->csum2 - csum, zcount);
                mcw->savedata.csum3 = mcRot(csum * zcount, cd->csum2);
                cd->pos = (unsigned long *)&mcw->savedata.data[0x1FC0];
                zcount %= 0x3F8;
                cd->count = cd->zcount = zcount;
                return;
            }
        } while (++i < 0x1800);
        cd->pos = pos;
        cd->count = count;
        cd->zcount = zcount;
        cd->csum1 = csum;
        break;
    case 101:
        count = cd->count;
        pos = cd->pos;
        while (count-- > 0) {
            pos--;
            pos[5] = pos[0];
        }
        for (i = 0; i < 5; i++) {
            *pos++ = key1[i];
        }
        cd->step++;
        break;
    case 102:
        csum4 = 0;
        pos = (unsigned long *)&mcw->savedata;
        for (i = 0; i < sizeof(struct MC_SAVEDATA) / sizeof(unsigned long) - 1; i++) {
            csum4 ^= *pos++;
        }
        mcw->savedata.csum4 = csum4;
        cd->step = -1;
        break;
    case 200:
        pos = cd->pos;
        count = cd->count;
        zcount = cd->zcount;
        csum = cd->csum1;
        csum4 = cd->csum4;
        i = 0;
        do {
            n = *pos++;
            csum4 ^= n;
            for (j = 0; j < 8; j++) {
                if (!(n & 0xF)) {
                    zcount++;
                }
                csum += n & 0xFF;
                n >>= 8;
            }
            if (--count <= 0) {
                csum4 ^= *pos;
                csum4 ^= *(unsigned long *)&mcw->savedata;
                if (mcw->savedata.csum1 != mcRot(csum, zcount)) {
                    printf("CheckSum Error 1!\n");
                    cd->step = -2;
                    return;
                }
                if (mcw->savedata.csum4 != csum4) {
                    printf("CheckSum Error 4!\n");
                    cd->step = -2;
                    return;
                }
                pos = (unsigned long *)&mcw->savedata.data[0x1FC0] - (ck1 = zcount % 0x3F8);
                cd->pos = pos;
                csum2 = 0;
                for (i = 0; i < 5; i++) {
                    n = key1[i] = *pos++;
                    for (j = 0; j < 8; n >>= 8, j++) {
                        csum2 += n & 0xFF;
                    }
                }
                if (mcw->cd.key2[15] != (((csum2 - mcw->cd.key2[15]) & 0xFF) ^ 0x21)) {
                    printf("CheckSum Error K!\n");
                    cd->step = -2;
                    return;
                }
                if (mcw->savedata.csum2 != mcRot(zcount - csum2 - csum, zcount)) {
                    printf("CheckSum Error 2!\n");
                    cd->step = -2;
                    return;
                }
                if (mcw->savedata.csum3 != mcRot(csum * zcount, csum2)) {
                    printf("CheckSum Error 3!\n");
                    cd->step = -2;
                    return;
                }
                cd->count = ck1;
                cd->step++;
                return;
            }
        } while (++i < 0x1800);
        cd->pos = pos;
        cd->count = count;
        cd->zcount = zcount;
        cd->csum1 = csum;
        cd->csum4 = csum4;
        break;
    case 201:
        count = cd->count;
        pos = cd->pos;
        while (count-- > 0) {
            *pos++ = pos[5]; /* @bug unsequenced (undefined); MWCC reads pos[5] before the increment */
        }
        cd->pos = (unsigned long *)mcw->savedata.data;
        cd->count = 0x3F8;
        cd->step++;
        break;
    case 202:
        pos = cd->pos;
        count = cd->count;
        i = 0;
        ck2 = 0;
        ck1 = 0;
        do {
            n = *pos;
            n ^= key1[ck1] ^ key2[ck2];
            *pos++ = n;
            if (++ck1 >= 3) {
                ck1 = 0;
                if (++ck2 >= 16) {
                    ck2 = 0;
                }
            }
            if (--count <= 0) {
                if (mcExtSaveData() || CheckSaveData()) {
                    memcpy(&SaveDataAll, MemShare_gp_data_buf + 0x148000, sizeof(SaveDataAll.d));
                    cd->step = -2;
                } else {
                    cd->step = -1;
                }
                return;
            }
        } while (++i < 0x6000);
        cd->pos = pos;
        cd->count = count;
        break;
    }
}


/** Builds the directory data (the file list shown in the menu) for a save into file fn.
 * @param port card port
 * @param dn directory number
 * @param fn file number */
void mcMakeDirData(char port, char dn, char fn) {
    int i;
    int j;
    unsigned int n;
    unsigned int csum;
    unsigned int *pos1;
    unsigned int *pos2;
    struct MC_FILEINFO *fi;
    struct Playing_Info *pi;

    pi = &SaveDataAll.d.playing;
    shQzero(mcw, sizeof(mcw->dirdata));
    memcpy(mcw->dirdata.message, mc_message, sizeof(mc_message));
    for (fi = mcw->fileinfo[port], i = 0; i < mcw->filemax[port]; i++, fi++) {
        if (fi->dirid == dn && fi->savecount > 0) {
            pos1 = (unsigned int *)&mcw->dirdata.file[fi->fileid];
            pos2 = (unsigned int *)fi;
            for (j = 0; j < sizeof(struct MC_FILEINFO) / sizeof(unsigned int); j++) {
                *pos1++ = *pos2++;
            }
        }
    }
    fi = &mcw->dirdata.file[fn];
    fi->dirid = dn;
    fi->fileid = fn;
    fi->scene = SaveDataAll.d.scene;
    fi->b_level = pi->battle_level;
    fi->r_level = pi->riddle_level;
    fi->status = pi->clear_end_kind;
    fi->savecount = pi->savecount;
    fi->time = pi->time;
    fi->total_time = pi->total_time + pi->time;
    pos1 = (unsigned int *)fi;
    csum = 0xB0B8AE8C;
    for (j = 0; j < sizeof(struct MC_FILEINFO) / sizeof(unsigned int) - 1; j++) {
        n = *pos1++;
        csum ^= mcRot(n * 0x573, j);
    }
    *pos1 = csum;
    memcpy(&mcw->dirdata.playing, pi, sizeof(*pi));
    mcw->dirdata.lastsave = fn;
    mc.status &= ~0x1000;
    if (mcw->dirdata.flag != 0x60125A01) {
        if (pi->clear_end_kind & 0x10) {
            mcw->dirdata.flag = 0x60125A01;
            mc.status |= 0x1000;
        } else if (pi->clear_end_number) {
            mcw->dirdata.flag = 0x4E9C6817;
        }
    }
    mcMakeDirDataSub();
}

/** Removes file fn from the directory data.
 * @param port card port
 * @param dn directory number
 * @param fn file number */
void mcMakeDirData2(char port, char dn, char fn) {
    int i;
    unsigned int *pos;

    pos = (unsigned int *)&mcw->dirdata.file[fn];
    for (i = 0; i < sizeof(struct MC_FILEINFO) / sizeof(unsigned int); i++) {
        *pos++ = 0;
    }
    if (mcw->dirdata.lastsave == fn) {
        mcw->dirdata.lastsave = -1;
    }
    if (mc.ls_port == port && mc.ls_dir == dn && mc.ls_file == fn && mcw->dirid[port][dn] == mc.ls_dir_id) {
        mc.ls_port = -1;
    }
    mcMakeDirDataSub();
}

/** Copies the play info into the directory data and updates its clear flag. */
void mcMakeDirData3(void) {
    struct Playing_Info *pi;

    pi = &playing;
    memcpy(&mcw->dirdata.playing, &playing, sizeof(playing));
    mc.status &= ~0x1000;
    if (mcw->dirdata.flag != 0x60125A01) {
        if (pi->clear_end_kind & 0x10) {
            mcw->dirdata.flag = 0x60125A01;
            mc.status |= 0x1000;
        } else if (pi->clear_end_number) {
            mcw->dirdata.flag = 0x4E9C6817;
        }
    }
    mcMakeDirDataSub();
}

static void mcMakeDirDataSub(void) {
    int i;
    unsigned int n;
    unsigned int csum;
    unsigned int csum2;
    unsigned int id;
    unsigned int *pos;
    unsigned long *pos2;
    unsigned long csum3;

    id = shRandI() ^ mcRot(shRandI(), 8);
    mcw->dirdata.id = id;
    pos = (unsigned int *)mcw->dirdata.file;
    csum = 0;
    csum2 = 0xB0B8AE8C;
    shPushRandSeed(0x23D);
    for (i = 0; i < ((unsigned int)mcw->dirdata.csum - (unsigned int)mcw->dirdata.file) / sizeof(unsigned int); i++) {
        n = *pos;
        csum += n;
        n = mcRot(n + 0x5BC679D8, i);
        n ^= mcRot(0x53F76697, -i * 3);
        n ^= shRandI();
        *pos = n;
        csum2 ^= n;
        pos++;
    }
    shPopRandSeed();
    pos = mcw->dirdata.pad;
    for (i = 0; i < sizeof(mcw->dirdata.pad) / sizeof(mcw->dirdata.pad[0]); i++) {
        *pos++ = shRandI() ^ mcRot(shRandI(), 16);
    }
    mcw->dirdata.csum[0] = mcRot(csum - csum2, 15);
    mcw->dirdata.csum[1] = mcw->dirdata.flag + (csum * 0x23D + 0xB0B8AE8C);
    mcw->dirdata.csum[2] = mcw->dirdata.flag * csum2 ^ mcRot((csum + 1) * 0x573, 7);
    mcw->dirdata.csum[3] = csum2 + id;
    csum3 = 0;
    pos2 = (unsigned long *)&mcw->dirdata;
    for (i = 0; i < sizeof(struct MC_DIRDATA) / sizeof(unsigned long) - 1; i++) {
        csum3 += *pos2++;
    }
    mcw->dirdata.csum_all = csum3;
    mcw->dirid_new = id;
}

/** Checks the loaded directory data (checksums and values) and takes over what it holds.
 * @return 0 if it is valid, 1 if not */
int mcExtDirData(void) {
    int i;
    int j;
    unsigned int n;
    unsigned int csum;
    unsigned int csum2;
    unsigned int fl;
    unsigned int *pos;
    unsigned long *pos2;
    unsigned long csum3;
    struct MC_FILEINFO *fi;

    csum3 = 0;
    pos2 = (unsigned long *)&mcw->dirdata;
    for (i = 0; i < sizeof(struct MC_DIRDATA) / sizeof(unsigned long) - 1; i++) {
        csum3 += *pos2++;
    }
    if (mcw->dirdata.csum_all != csum3) {
        printf("CheckSum Error A!\n");
        return 1;
    }
    fl = mcw->dirdata.flag;
    if (fl != 0 && fl != 0x4E9C6817 && fl != 0x60125A01) {
        printf("ClearCode Error\n");
        return 1;
    }
    pos = (unsigned int *)mcw->dirdata.file;
    csum = 0;
    csum2 = 0xB0B8AE8C;
    shPushRandSeed(0x23D);
    for (i = 0; i < ((unsigned int)mcw->dirdata.csum - (unsigned int)mcw->dirdata.file) / sizeof(unsigned int); i++) {
        n = *pos;
        csum2 ^= n;
        n ^= shRandI();
        n ^= mcRot(0x53F76697, -i * 3);
        n = mcRot(n, 32 - i) - 0x5BC679D8;
        csum += n;
        *pos = n;
        pos++;
    }
    shPopRandSeed();
    if (mcw->dirdata.csum[0] != mcRot(csum - csum2, 15) ||
        mcw->dirdata.csum[1] != fl + (csum * 0x23D + 0xB0B8AE8C) ||
        mcw->dirdata.csum[2] != (fl * csum2 ^ mcRot((csum + 1) * 0x573, 7)) ||
        mcw->dirdata.csum[3] != csum2 + mcw->dirdata.id) {
        printf("CheckSum Error D!\n");
        return 1;
    }
    for (i = 0; i < 28; i++) {
        if (mc_message[i] != mcw->dirdata.message[i]) {
            printf("Message Error D!\n");
            return 1;
        }
    }
    for (i = 0; i < 15; i++) {
        fi = &mcw->dirdata.file[i];
        if (fi->savecount > 0) {
            if (fi->status & 0x80) {
                continue;
            }
            if (fi->fileid != i || fi->scene > 25 || fi->b_level > 4 || fi->r_level > 3) {
                printf("DirData[%d] Error!\n", i);
                return 1;
            }
            csum = 0xB0B8AE8C;
            pos = (unsigned int *)fi;
            for (j = 0; j < sizeof(struct MC_FILEINFO) / sizeof(unsigned int) - 1; j++) {
                n = *pos++;
                csum ^= mcRot(n * 0x573, j);
            }
            if (fi->csum != csum) {
                printf("CheckSum Error D[%d]\n", i);
                return 1;
            }
        } else {
            pos = (unsigned int *)fi;
            for (j = 0; j < sizeof(struct MC_FILEINFO) / sizeof(unsigned int); j++) {
                if (*pos++) {
                    printf("DirData[%d] Error!(no zero)\n", i);
                    return 1;
                }
            }
        }
    }
    return 0;
}


/** Starts loading the memory card icons. */
void mcLoadIconData(void) {
    mcw->fid[0] = FcRead(data_menu_mc_icondata, MemShare_gp_data_buf + 0xC0000);
    mcw->fid[1] = FcRead(data_menu_mc_icondata2, MemShare_gp_data_buf + 0xE0000);
}

/** Returns non-zero once the icons have loaded. */
int mcCheckEndLoadIconData(void) {
    return fsSync(1, mcw->fid[0]) >= 0 && fsSync(1, mcw->fid[1]);
}
