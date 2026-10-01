#include "sh2.h"
#include "sdk/eekernel.h"
#include "sdk/libcdvd.h"
#include "sdk/sifdev.h"
#include "sdk/sifrpc.h"
#include "lib/sh_kernel.h"

/*
 * sh_cdvd.c: thread-safe wrappers around libcdvd, sif and the HDD file calls.
 *
 * Two semaphores serialize access: exec_sid guards whole operations (a read
 * with its retries), cmd_sid guards single libcdvd commands. They are created
 * on first use. wait_sid is signalled by the libcdvd callback when a
 * non-blocking command ends.
 */

/* Matching: #line keeps the original numbering (register defines moved to eeregs.h, declarations to headers). */
#line 85

struct shCdWorkT shCdWork = { -1, -1, -1, 0 };
volatile struct shHdWork shHdWork = { -1, -1, NULL, -1, 0 }; /* volatile: WaitHd loads hd_sid again right after testing it */

static int exec_cnt;
static int cmd_cnt;
static sceCdCLOCK old_rtc;

static int WaitExec(void) {
    int ret;

    if (shCdWork.exec_sid != -1) {
        ret = WaitSema(shCdWork.exec_sid);
    } else {
        ret = shCdWork.exec_sid = CreateSema2(0, 1, NULL);
    }
    exec_cnt++;
    return ret;
}

static int SignalExec(void) {
    exec_cnt--;
    return SignalSema(shCdWork.exec_sid);
}

static int WaitCmd(void) {
    int ret;

    if (shCdWork.cmd_sid != -1) {
        ret = WaitSema(shCdWork.cmd_sid);
    } else {
        ret = shCdWork.cmd_sid = CreateSema2(0, 1, NULL);
    }
    cmd_cnt++;
    return ret;
}

static int SignalCmd(void) {
    cmd_cnt--;
    return SignalSema(shCdWork.cmd_sid);
}

/* mmode 0: detect the media type (waits until a disc is in). */
static int ___shCdMmode(int mmode) {
    WaitCmd();
    while (mmode == 0) {
        sceCdDiskReady(0);
        switch (sceCdGetDiskType()) {
        case SCECdPS2DVD:
        case SCECdDVDV:
            mmode = CDVD_MMODE_DVD;
            break;
        case SCECdPS2CD:
        case SCECdPS2CDDA:
        case SCECdPSCD:
        case SCECdPSCDDA:
        case SCECdCDDA:
            mmode = CDVD_MMODE_CD;
            break;
        case SCECdDETCT:
        case SCECdNODISC:
            break;
        case CDVD_TYPE_ILLEGAL:
        default:
            assert(!"illegal media.");
            break;
        }
    }
    if (!sceCdMmode(mmode)) {
        mmode = 0;
    }
    SignalCmd();
    return mmode;
}

static int ___shCdInit(int initmode) {
    int ret;

    WaitCmd();
    shCdWork.rtc_ok = 1;
    ret = sceCdInit(initmode);
    SignalCmd();
    return ret;
}

/**
 * Initializes libcdvd (sceCdInit(`initmode`)) and sets media mode `mmode` (0: detect). Returns 0 on
 * failure.
 */
int shCdInit(int initmode, int mmode) {
    int ret;

    WaitExec();
    ret = ___shCdInit(initmode);
    if (ret) {
        ret = mmode;
        ___shCdMmode(mmode);
    }
    SignalExec();
    return ret;
}

/**
 * Sets the media mode `mmode` (0: detect, waiting for a disc). Returns the mode set, 0 on failure.
 */
int shCdMmode(int mmode) {
    WaitExec();
    mmode = ___shCdMmode(mmode);
    SignalExec();
    return mmode;
}

/** shCdInit(), retried until it succeeds. */
int shCdInitR(int initmode, int mmode) {
    int ret;

    do {
        ret = shCdInit(initmode, mmode);
    } while (ret == 0);
    return ret;
}

static void shCdCallbackFunc(void) {
    iSignalSema(shCdWork.wait_sid);
}

/**
 * Sets up libcdvd's callback thread (priority `cb_prio`, stack `stack` of `stack_size` bytes) and
 * the callback that ends non-blocking commands.
 */
int shCdInitW(int cb_prio, void *stack, int stack_size) {
    int ret;

    if (shCdWork.wait_sid == -1) {
        shCdWork.wait_sid = CreateSema2(0, 1, NULL);
    }
    ret = sceCdInitEeCB(cb_prio, stack, stack_size);
    sceCdCallback(shCdCallbackFunc);
    return ret;
}

/**
 * Initializes SIF RPC and resets the file loader and the IOP file system, under the exec and
 * command locks.
 */
int shSifInit(void) {
    int ret;

    WaitExec();
    WaitCmd();
    sceSifInitRpc(0);
    sceSifLoadFileReset();
    ret = sceFsReset();
    SignalCmd();
    SignalExec();
    return ret;
}

/** Does nothing. Returns 1. */
int shSdSifInit(void) {
    return 1;
}

/** sceSifSyncIop() under the exec and command locks. */
int shSifSyncIop(void) {
    int ret;

    WaitExec();
    WaitCmd();
    ret = sceSifSyncIop();
    SignalCmd();
    SignalExec();
    return ret;
}

/** Loads IOP module `module` with arguments `args`/`argp` under the exec and command locks. */
int shSifLoadModule(char *module, int args, char *argp) {
    int ret;

    WaitExec();
    WaitCmd();
    ret = sceSifLoadModule(module, args, argp);
    SignalCmd();
    SignalExec();
    return ret;
}

/** shSifLoadModule() until it succeeds, 60 V-blanks between tries; complains every 22nd failure. */
int shSifLoadModuleR(char *module, int args, char *argp) {
    int ret;
    int count;

    count = 20;
    while ((ret = shSifLoadModule(module, args, argp)) < 0) {
        if (count-- < 0) {
            verbose(1, "sh_cdvd.c:298> %s: can't load iop"); /* @bug no argument for the %s */
            count = 20;
        }
        { int i; for (i = 60; i > 0; i--) { shSyncVEnd(0); } }
    }
    return ret;
}

/** Reboots the IOP with image `imgfile` under the exec and command locks. */
int shSifRebootIop(char *imgfile) {
    int ret;

    WaitExec();
    WaitCmd();
    ret = sceSifRebootIop(imgfile);
    SignalCmd();
    SignalExec();
    return ret;
}

/** shSifRebootIop(), then shSifSyncIop(), each until it succeeds (60 V-blanks between tries). Returns 1. */
int shSifRebootIopR(char *imgfile) {
    int ret;

    ret = 20;
    while (!shSifRebootIop(imgfile)) {
        if (ret-- < 0) {
            verbose(1, "sh_cdvd.c:329> %s: can't reboot"); /* every 22nd failure; @bug no argument for the %s */
            ret = 20;
        }
        { int i; for (i = 60; i > 0; i--) { shSyncVEnd(0); } }
    }
    ret = 5;
    while (!shSifSyncIop()) {
        if (ret-- < 0) {
            verbose(1, "sh_cdvd.c:340> %s: can't sync iop"); /* every 7th failure; @bug no argument for the %s */
            ret = 5;
        }
        { int i; for (i = 60; i > 0; i--) { shSyncVEnd(0); } }
    }
    return 1;
}

/* Reads are DMA transfers: the buffer has to be 64-byte aligned. */
static void checkReadAlign(void *buffer) {
    if ((int)buffer & 0x3F) {
        printf("sh_cdvd.c:384> buffer alignment error! 0x%08x\n", buffer);
        while (1) {}
    }
}

/** Starts reading `sectors` sectors at `lsn` into `buf` (non-blocking). */
int shCdRead(int lsn, int sectors, void *buf, sceCdRMode *mode) {
    int ret;

    checkReadAlign(buf);
    WaitCmd();
    ret = sceCdRead(lsn, sectors, buf, mode);
    SignalCmd();
    return ret;
}

/** Starts a seek to `lsn` (non-blocking). */
int shCdSeek(int lsn) {
    int ret;

    WaitCmd();
    ret = sceCdSeek(lsn);
    SignalCmd();
    return ret;
}

/** Blocking read; if the read can't be started, retries while the drive is ready (else 0x31). Returns the libcdvd error code. */
int shCdReadW(int lsn, int sectors, void *buf, sceCdRMode *mode) {
    int ret;
    unsigned short hcnt0;
    unsigned short hcnt1;

    hcnt0 = *T3_COUNT;
    verbose(2, "sh_cdvd.c:442> now CD reading...\n");
    WaitExec();
    SyncDCache(buf, (char *)buf + sectors * 2048);
    InvalidDCache(buf, (char *)buf + sectors * 2048);
    do {
        ret = shCdRead(lsn, sectors, buf, mode);
        if (ret) {
            WaitSema(shCdWork.wait_sid);
            ret = shCdGetError();
            break;
        }
        ret = 0x31;
    } while (___shCdDiskReady(1) == CDVD_READY);
    InvalidDCache(buf, (char *)buf + sectors * 2048);
    SyncDCache(buf, (char *)buf + sectors * 2048);
    SignalExec();
    hcnt1 = *T3_COUNT;
    verbose(2, "sh_cdvd.c:469> cd read time:%d\n", (unsigned short)(hcnt1 - hcnt0));
    return ret;
}

/** Blocking seek to `lsn`; if the seek can't be started, retries while the drive is ready (else 0x31). Returns the libcdvd error code. */
int shCdSeekW(int lsn) {
    int ret;
    unsigned short hcnt0;
    unsigned short hcnt1;

    hcnt0 = *T3_COUNT;
    WaitExec();
    do {
        ret = shCdSeek(lsn);
        if (ret) {
            WaitSema(shCdWork.wait_sid);
            ret = shCdGetError();
            break;
        }
        ret = 0x31;
    } while (___shCdDiskReady(1) == CDVD_READY);
    SignalExec();
    hcnt1 = *T3_COUNT;
    verbose(2, "sh_cdvd.c:496> cd seek time:%d\n", (unsigned short)(hcnt1 - hcnt0));
    return ret;
}

/* Alarm handler: the clock may be read again. */
static void shCdReadClockRecover(void) {
    shCdWork.rtc_ok = 1;
    ExitHandler();
}

/** Reads the RTC at most every 4725 H-lines; returns the cached value otherwise. */
int shCdReadClock(sceCdCLOCK *rtc) {
    int ret;

    WaitCmd();
    if (shCdWork.rtc_ok) {
        shCdWork.rtc_ok = 0;
        ret = sceCdReadClock(rtc);
        old_rtc = *rtc;
        SetAlarm(4725, shCdReadClockRecover, NULL);
    } else {
        ret = 0;
        *rtc = old_rtc;
    }
    SignalCmd();
    return ret;
}

/*
 * Skips a "cdrom0:" style device prefix.
 */
static char *name_skip_cdrom0(char *name) {
    char *ret;

    if (*name == '\\') {
        ret = name;
    } else {
        for (ret = name; *ret != '\0' && *ret != ':'; ret++) {
        }
        if (*ret == ':') {
            ret++;
        }
    }
    return ret;
}

/** Looks up `fullpath` on the disc into `file`, timing the search. */
int shCdSearchFile(sceCdlFILE *file, char *fullpath) {
    int ret;
    unsigned short hcnt0;
    unsigned short hcnt1;

    hcnt0 = *T3_COUNT;
    ret = ___shCdSearchFile(file, fullpath);
    hcnt1 = *T3_COUNT;
    verbose(2, "sh_cdvd.c:639> cd search-file time:%d\n", (unsigned short)(hcnt1 - hcnt0));
    return ret;
}

/**
 * sceCdSearchFile() of `name` (without its "cdrom0:" prefix) into `fp`, under the exec and command
 * locks.
 */
int ___shCdSearchFile(sceCdlFILE *fp, char *name) {
    int ret;

    WaitExec();
    WaitCmd();
    name = name_skip_cdrom0(name);
    if (*name == '\\') {
        ret = sceCdSearchFile(fp, name);
    } else {
        ret = 0;
    }
    SignalCmd();
    SignalExec();
    return ret;
}

/** sceCdDiskReady(`mode`) under the exec lock. */
int shCdDiskReady(int mode) {
    int ret;

    WaitExec();
    ret = ___shCdDiskReady(mode);
    SignalExec();
    return ret;
}

static int ___shCdDiskReady(int mode) {
    int ret;
    char *str;

    WaitCmd();
    ret = sceCdDiskReady(mode);
    SignalCmd();
    return ret;
}

/** sceCdGetDiskType() under the exec and command locks. */
int shCdGetDiskType(void) {
    int ret;
    char *str;

    WaitExec();
    WaitCmd();
    ret = sceCdGetDiskType();
    SignalCmd();
    SignalExec();
    return ret;
}

/** sceCdTrayReq(`mode`, `traycnt`) under the exec and command locks. */
int shCdTrayReq(int mode, unsigned int *traycnt) {
    int ret;

    WaitExec();
    WaitCmd();
    ret = sceCdTrayReq(mode, traycnt);
    SignalCmd();
    SignalExec();
    return ret;
}

/** sceCdGetError() under the command lock. */
int shCdGetError(void) {
    int error;

    WaitCmd();
    error = sceCdGetError();
    SignalCmd();
    return error;
}

/** sceCdStatus() under the exec and command locks. */
int shCdStatus(void) {
    int stat;

    WaitExec();
    WaitCmd();
    stat = sceCdStatus();
    SignalCmd();
    SignalExec();
    return stat;
}

/** sceCdSync(`mode`) under the command lock. */
int shCdSync(int mode) {
    int ret;

    WaitCmd();
    ret = sceCdSync(mode);
    SignalCmd();
    return ret;
}

/** Takes the exec lock, for the sound library's use of the drive. */
void shCdSdStart(void) {
    WaitExec();
}

/** Releases the exec lock taken by shCdSdStart(). */
void shCdSdEnd(void) {
    SignalExec();
}

static int WaitHd(void) {
    int ret;

    if (shHdWork.hd_sid != -1) {
        ret = WaitSema(shHdWork.hd_sid);
    } else {
        ret = shHdWork.hd_sid = CreateSema2(0, 1, NULL);
    }
    exec_cnt++;
    return ret;
}

static int SignalHd(void) {
    exec_cnt--;
    return SignalSema(shHdWork.hd_sid);
}

/** Forgets the cached HD file (last file name, descriptor and offset). */
int shHdInit(void) {
    int ret;

    WaitHd();
    shHdWork.last_fd = -1;
    shHdWork.last_filename = NULL;
    shHdWork.last_offset = 1;
    ret = SignalHd();
    return ret;
}

/** sceOpen(`name`, `mode`) under the HD lock; -1 for a NULL name. */
int shHdOpen(char *name, int mode) {
    int ret;

    WaitHd();
    if (name) {
        ret = sceOpen(name, mode);
    } else {
        ret = -1;
    }
    SignalHd();
    return ret;
}

/** sceRead() of `size` bytes from `fd` into `buffer` under the HD lock. */
int shHdRead(int fd, void *buffer, int size) {
    int ret;

    checkReadAlign(buffer);
    WaitHd();
    ret = sceRead(fd, buffer, size);
    SignalHd();
    return ret;
}

/** sceClose(`fd`) under the HD lock. */
int shHdClose(int fd) {
    int ret;

    WaitHd();
    ret = sceClose(fd);
    SignalHd();
    return ret;
}

/** sceLseek(`fd`, `offset`, `where`) under the HD lock. */
int shHdLseek(int fd, int offset, int where) {
    int ret;

    WaitHd();
    ret = sceLseek(fd, offset, where);
    SignalHd();
    return ret;
}

/** Returns the size of HD file `name`: -1 when it can't be opened, 0 when the seek fails. */
int ___shHdGetFileSize(char *name) {
    int ret;
    int fd;

    WaitExec();
    WaitCmd();
    fd = shHdOpen(name, FIO_RDONLY);
    if (fd >= 0) {
        ret = shHdLseek(fd, 0, FIO_SEEK_END);
        if (ret < 0) {
            ret = 0;
        }
        shHdClose(fd);
    } else {
        ret = -1;
    }
    SignalCmd();
    SignalExec();
    return ret;
}
