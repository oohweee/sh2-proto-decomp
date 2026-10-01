/*
 * filecmd.c: the file commands run by the file server thread (fileserv.c).
 *
 * A union fsFile describes a file; check.type holds flags:
 *   0x01 search the CD first      0x02 search the HD first
 *   0x04 on CD                    0x08 on HD
 *   0x10 member of a parent file (fsMgcFile/fsMgfFile)
 *   0x20 name is relative to the base directory
 *   0x40 needs fixing (location not resolved yet)
 *   0x80 found on the HD
 * check.number is the disk the CD location was resolved on (0: not on CD).
 *
 * "Fixing" a file resolves its location (lsn/size on the CD, or the size on
 * the HD) through the fix function list of the current disk selection.
 */

#include "sh2.h"
#include "libc/string.h"
#include "sdk/eekernel.h"

#define ID_fsFile(idx) ((idx)->index.fp)

static int (*fsCmdParam_check_func)(union fsFile **, void **);
static void **fsCmdParam_buflist;
static union fsFile **fsCmdParam_fplist;
static int fsCmdParam_media_permission;

static int ___fsSubCmdCdFixFile(union fsFile *fp);
static int ___fsSubCmdCd1stFixFile(union fsFile *fp);
static int ___fsSubCmdHdFixFile(union fsFile *fp);
static int ___fsSubCmdHd1stFixFile(union fsFile *fp);

static int (*___fsSubCmdFixFuncListC[2])(union fsFile *);

static struct fsSubCmdStatT fs_stat = { ___fsSubCmdFixFuncListC, 0xFF };

static int (*___fsSubCmdFixFuncListC[2])(union fsFile *) = {
    ___fsSubCmdCdFixFile, NULL,
};
static int (*___fsSubCmdFixFuncListCH[5])(union fsFile *) = {
    ___fsSubCmdCd1stFixFile, ___fsSubCmdHd1stFixFile, ___fsSubCmdCdFixFile, ___fsSubCmdHdFixFile, NULL,
};
static int (*___fsSubCmdFixFuncListHC[5])(union fsFile *) = {
    ___fsSubCmdHd1stFixFile, ___fsSubCmdCd1stFixFile, ___fsSubCmdHdFixFile, ___fsSubCmdCdFixFile, NULL,
};
static int (*___fsSubCmdFixFuncListH[2])(union fsFile *) = {
    ___fsSubCmdHdFixFile, NULL,
};

/* fp if it is a CD/HD file (not a member file), else NULL. */
static union fsFile *___fsSubCmdCheckCdHdFile(union fsFile *fp) {
    if (fp == NULL) {
        return NULL;
    }
    if (fp->check.type & 0x10) {
        return NULL;
    }
    return (fp->check.type & 0xF) ? fp : NULL;
}

/* Walks up the parents of a member file, turning start/end members into offset/size ones. */
static union fsFile *___fsSubCmdCheckRootFile(union fsFile *fp) {
    while (1) {
        if (fp == NULL || !(fp->check.type & 0x10)) {
            break;
        }
        if (!(fp->check.type & 0x40)) {
            char *start;
            char *end;
            int offset;
            int size;

            start = fp->mgc.start;
            end = fp->mgc.end;
            offset = start - (char *)NULL;
            size = end - start;
            fp->mgf.offset = offset;
            fp->mgf.size = size;
            fp->check.type = 0x50;
        }
        fp = fp->mgf.parent;
    }
    return fp;
}

/* Returns fp if it still has to be fixed, NULL if its location is valid. */
static union fsFile *___fsSubCmdCheckCdHdFixFile(union fsFile *fp) {
    if (fp->check.type & 0x40) {
        if ((fp->check.type & 0x5) && fp->check.number && fs_stat.disk_number) {
            fp->check.type &= 0x7F;
            if ((fp->check.number > 0 && fp->check.number != fs_stat.disk_number) ||
                (fp->check.number < 0 && fs_stat.disk_number < fp->check.number)) {
                fp->check.type &= ~0x40;
                return fp;
            }
        }
        return NULL;
    }
    return fp;
}

/**
 * Returns the file to use for `fp` (a root file resolved to its entry) when it can be found on the
 * CD or HD, otherwise NULL.
 */
union fsFile *fsCmdCheckFixFile(union fsFile *fp) {
    union fsFile *ret;

    utilExclLockOtherThread();
    fp = ___fsSubCmdCheckRootFile(fp);
    if (!___fsSubCmdCheckCdHdFile(fp)) {
        ret = NULL;
    } else {
        ret = ___fsSubCmdCheckCdHdFixFile(fp);
    }
    utilExclUnlockOtherThread();
    return ret;
}

/* 1: on CD, 2: on HD, -1: not found, 0: not fixed yet. */
static int ___fsSubCmdCheckCdHdExistFile(union fsFile *fp) {
    if (___fsSubCmdCheckCdHdFixFile(fp)) {
        return 0;
    }
    if ((fp->check.type & 0x5) && fp->check.number) {
        return 1;
    }
    if ((fp->check.type & 0xA) && (fp->check.type & 0x80)) {
        return 2;
    }
    return -1;
}

/** Returns whether file `fp` exists on its device; -2 when it can't be on either. */
int fsCmdCheckExistFile(union fsFile *fp) {
    int ret;

    utilExclLockOtherThread();
    fp = ___fsSubCmdCheckRootFile(fp);
    ret = ___fsSubCmdCheckCdHdFile(fp) == NULL ? -2 : ___fsSubCmdCheckCdHdExistFile(fp);
    utilExclUnlockOtherThread();
    return ret;
}

static union fsFile *___fsSubCmdSetRealFile0(union fsFile *realfp, union fsFile *fp);

static union fsFile *fsSubCmdSetRealFile0(union fsFile *realfp, union fsFile *fp) {
    union fsFile *ret;

    utilExclLockOtherThread();
    ret = ___fsSubCmdSetRealFile0(realfp, fp);
    utilExclUnlockOtherThread();
    return ret;
}

/*
 * Resolves fp (possibly a member file) to a plain CD/HD location in realfp.
 * Returns NULL on success, else the file that isn't fixed yet.
 */
static union fsFile *___fsSubCmdSetRealFile0(union fsFile *realfp, union fsFile *fp) {
    union fsFile *ret;
    int offset;
    int size;

    if (___fsSubCmdCheckCdHdFile(fp)) {
        if (___fsSubCmdCheckCdHdExistFile(fp) <= 0) {
            return fp;
        }
        realfp->pack = fp->pack;
        return NULL;
    }
    if ((fp->check.type & 0x50) == 0x50) {
        if ((ret = ___fsSubCmdSetRealFile0(realfp, fp->mgf.parent))) {
            return ret;
        }
        offset = fp->mgf.offset / 2048 * 2048;
        size = fp->mgf.size;
        if ((realfp->check.type & 0x5) && realfp->check.number) {
            int plsn;
            int psize;

            plsn = realfp->cd.lsn;
            psize = realfp->cd.size;
            if (psize < offset) {
                size = 0;
            } else {
                psize -= offset;
                if (psize < size) {
                    size = psize;
                }
            }
            realfp->cd.lsn = plsn + offset / 2048;
            realfp->cd.size = size;
            return NULL;
        } else if (realfp->check.type & 0xA) {
            int poffset;
            int psize;

            poffset = realfp->hd.offset;
            psize = realfp->hd.size;
            if (psize < offset) {
                size = 0;
            } else {
                psize -= offset;
                if (psize < size) {
                    size = psize;
                }
            }
            realfp->hd.offset = poffset + offset;
            realfp->hd.size = size;
            return NULL;
        } else {
            return fp;
        }
    }
    return fp;
}

static union fsFile *fsSubCmdSetRealFile(union fsFile *realfp, union fsFile *fp) {
    union fsFile *ret;
    int err;

    utilExclLockOtherThread();
    ret = ___fsSubCmdCheckRootFile(fp);
    err = ___fsSubCmdCheckCdHdFile(ret) == NULL;
    if (!err) {
        ret = ___fsSubCmdSetRealFile0(realfp, fp);
    }
    utilExclUnlockOtherThread();
    return ret;
}

/**
 * Stores in `realfp` the real device file behind `fp` (resolving member files). Returns non-zero on
 * success.
 */
int fsCmdSetRealFile(union fsFile *realfp, union fsFile *fp) {
    return fsSubCmdSetRealFile(realfp, fp) == NULL;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 480
static int ___fsSubCmdNullDevFixFile(union fsFile *fp) {
    int ret;

    assert(fp==ID_fsFile(data_null_dev));
    if (fp->check.type & 0x40) {
        ret = 1;
    } else {
        fp->check.number = 0;
        fp->check.type |= 0xC0;
        fp->hd.offset = 0;
        fp->hd.size = 0x40000000;
        ret = 2;
    }
    return ret;
}

static int ___fsSubCmdCdCommonFixFile(union fsFile *fp) {
    sceCdlFILE file[1];
    char fullpath[256];
    int ret;

    if (fp == ID_fsFile(data_null_dev)) {
        ret = ___fsSubCmdNullDevFixFile(fp);
    } else if (fp->check.type & 0x40) {
        ret = 1;
    } else {
        ret = 1;
        if (fp->check.type & 0x20) {
            if (!___shPathMakeCdDataBase(fullpath, fp->cd.name)) {
                ret = 0;
            }
        } else {
            if (!___shPathMakeCdData(fullpath, fp->cd.name)) {
                ret = 0;
            }
        }
        if (ret && fs_stat.disk_number == 0) {
            ret = 0;
        }
        if (ret && !___shCdSearchFile(file, fullpath)) {
            fp->check.number = 0;
            ret = -1;
        }
        if (ret) {
            fp->check.type &= 0x7F;
            fp->check.type |= 0x40;
            fp->check.number = fs_stat.disk_number;
            fp->cd.lsn = file->lsn;
            fp->cd.size = file->size;
            ret = 2;
        }
    }
    return ret;
}

static int ___fsSubCmdCd1stFixFile(union fsFile *fp) {
    if (fp->check.type & 0x1) {
        return ___fsSubCmdCdCommonFixFile(fp);
    }
    return 0;
}

static int ___fsSubCmdCdFixFile(union fsFile *fp) {
    if (fp->check.type & 0x5) {
        return ___fsSubCmdCdCommonFixFile(fp);
    }
    return 0;
}

static int ___fsSubCmdHdCommonFixFile(union fsFile *fp) {
    int size;
    char fullpath[256];
    int ret;

    if (fp == ID_fsFile(data_null_dev)) {
        ___fsSubCmdNullDevFixFile(fp);
    } else if (fp->check.type & 0x40) {
        ret = 1;
    } else {
        ret = 1;
        if (fp->check.type & 0x20) {
            if (!___shPathMakeHdDataBase(fullpath, fp->hd.name)) {
                ret = 0;
            }
        } else {
            if (!___shPathMakeHdData(fullpath, fp->hd.name)) {
                ret = 0;
            }
        }
        if (ret) {
            size = ___shHdGetFileSize(fullpath);
            if (size < 0) {
                fp->check.type &= 0x7F;
                ret = -1;
            } else {
                fp->check.number = 0;
                fp->check.type |= 0xC0;
                fp->hd.offset = 0;
                fp->hd.size = size;
                ret = 2;
            }
        }
    }
    return ret;
}

static int ___fsSubCmdHd1stFixFile(union fsFile *fp) {
    if (fp->check.type & 0x2) {
        return ___fsSubCmdHdCommonFixFile(fp);
    }
    return 0;
}

static int ___fsSubCmdHdFixFile(union fsFile *fp) {
    if (fp->check.type & 0xA) {
        return ___fsSubCmdHdCommonFixFile(fp);
    }
    return 0;
}

/** Searches files on the CD only. Returns 1. */
int fsCmdDiskSelectC(void) {
    fs_stat.fix_func_list = ___fsSubCmdFixFuncListC;
    return 1;
}

/** Searches files on the CD, then the HD. Returns 1. */
int fsCmdDiskSelectCH(void) {
    fs_stat.fix_func_list = ___fsSubCmdFixFuncListCH;
    return 1;
}

/** Searches files on the HD, then the CD. Returns 1. */
int fsCmdDiskSelectHC(void) {
    fs_stat.fix_func_list = ___fsSubCmdFixFuncListHC;
    return 1;
}

/** Sets the device the executable runs from to `mode`. Returns 1. */
int fsCmdExecDevSelect(int mode) {
    fs_stat.exec_dev = mode;
    return 1;
}

static int ___fsSubCmdFixFile(union fsFile *fp) {
    int (**funcp)(union fsFile *);
    int (*___fsFunc)(union fsFile *);
    int ret;

    ret = 0;
    if (!___fsSubCmdCheckCdHdFixFile(fp)) {
        ret = 1;
    } else {
        funcp = fs_stat.fix_func_list;
        if ((fp->check.type & 0xF) && shPathCheckExecRoot(fp->cd.name)) {
            switch (fs_stat.exec_dev) {
            case 1:
                funcp = ___fsSubCmdFixFuncListC;
                break;
            case 2:
                funcp = ___fsSubCmdFixFuncListH;
                break;
            }
        }
        while ((___fsFunc = *funcp) != NULL) {
            ret = ___fsFunc(fp);
            if (ret) {
                break;
            }
            funcp++;
        }
    }
    if (ret == 0) {
        fp->check.number = 0;
        fp->check.type &= 0x7F;
        fp->check.type |= 0x40;
    }
    return ret;
}

/** Resolves where file `fp` is. Returns 0 when it can be on neither device. */
int fsCmdFixFile(union fsFile *fp) {
    int ret;

    utilExclLockOtherThread();
    fp = ___fsSubCmdCheckRootFile(fp);
    ret = ___fsSubCmdCheckCdHdFile(fp) == NULL ? 0 : ___fsSubCmdFixFile(fp);
    utilExclUnlockOtherThread();
    return ret;
}

static int fsSubCmdCdCheckTrayOpen(void) {
    int stat;
    unsigned int traycnt;

    while (!shCdTrayReq(2, &traycnt)) {
        shSyncVEnd(0);
        shCdSync(1);
    }
    if (traycnt) {
        return 1;
    }
    stat = shCdStatus();
    if (stat == 1) {
        return 1;
    }
    return stat == 0x20;
}

static int fsSubCmdCdGetMedia(void) {
    int cur_media;

    switch (shCdGetDiskType()) {
    case 0x14:
        cur_media = 0x1;
        break;
    case 0x12:
        cur_media = 0x2;
        break;
    case 0x13:
        cur_media = 0x4;
        break;
    case 0x10:
        cur_media = 0x8;
        break;
    case 0x11:
        cur_media = 0x10;
        break;
    case 0xFD:
        cur_media = 0x20;
        break;
    case 0xFE:
        cur_media = 0x40;
        break;
    case 0x00:
        cur_media = 0x200;
        break;
    case 0x01:
        cur_media = 0x100;
        break;
    case 0xFF:
        cur_media = 0x400;
        break;
    case 0x05:
    default:
        cur_media = 0x80;
        break;
    }
    return cur_media;
}

static int fsSubCmdCdGetMmode(int media) {
    int mmode;

    if (media & 0x3E) {
        mmode = 1;
    } else if (media & 0x41) {
        mmode = 2;
    } else {
        mmode = 0;
    }
    return mmode;
}

static int fsSubCmdCdRead(int lsn, int sectors, void *buf) {
    static sceCdRMode rmode[1] = { { 0, 1, 0, 0 } };
    int ret;

    while (1) {
        switch (buf ? shCdReadW(lsn, sectors, buf, rmode) : shCdSeekW(lsn)) {
        case 0x00:
            ret = 1;
            break;
        case 0x30:
            ret = 0;
            break;
        case 0x31:
            ret = 0;
            break;
        case 0x01:
            ret = 0;
            break;
        case 0x32:
        case 0x13:
        case -1:
        case 0x14:
        default:
            ret = 0;
            break;
        }
        if (ret) {
            return 1;
        }
        fsCmdCdCheckDisk(0);
    }
}

static int fsSubCmdHdError(void);
static int fsSubCmdHdRead0(char *name, int offset, int size, void *buf);

static int (*fsSubCmdHdRead)() = fsSubCmdHdError;

/** Turns HD (host) file access on (`enable` non-zero) or off. Returns 1. */
int fsCmdHdInit(int enable) {
    if (enable) {
        shHdInit();
        fs_stat.disk_emulation = 1;
        fsSubCmdHdRead = fsSubCmdHdRead0;
    } else {
        fs_stat.disk_emulation = 0;
        fsSubCmdHdRead = fsSubCmdHdError;
    }
    return 1;
}

static int strcmp__(char *s, char *t) {
    for (; *s == *t; s++, t++) {
        if (*s == '\0') {
            return 0;
        }
    }
    return *s - *t;
}

static char hdopen_lastpath[256] = "";
static int hdopen_lastfd = -1;
static int hdopen_lastmode;

/* Keeps the last HD file open. */
static int shHdOpenC(char *fullpath, int mode) {
    if (strcmp__(fullpath, hdopen_lastpath) != 0 || hdopen_lastmode != mode) {
        if (hdopen_lastpath[0] != '\0') {
            shHdClose(hdopen_lastfd);
        }
        hdopen_lastfd = shHdOpen(fullpath, mode);
        hdopen_lastmode = mode;
        strcpy(hdopen_lastpath, fullpath);
    }
    return hdopen_lastfd;
}

static int shHdCloseC(int fd) {
    if (hdopen_lastfd == fd) {
        return 0;
    }
    return -1;
}

static int fsSubCmdHdRead0(char *name, int offset, int size, void *buf) {
    int retry;
    int ret;
    char *fullpath;
    int fd;
    int remainsize;
    int readsize;
    char *readbuf;

    retry = 10;
    fullpath = name;
    while (1) {
        fs_stat.now_reading = 1;
        if (buf) {
            SyncDCache(buf, (char *)buf + size);
            InvalidDCache(buf, (char *)buf + size);
        }
        fd = shHdOpenC(fullpath, 1);
        if (fd >= 0) {
            remainsize = size;
            readsize = 0x40000;
            readbuf = buf;
            ret = 1;
            if (shHdLseek(fd, offset, 0) == -1) {
                ret = 0;
            }
            if (readbuf) {
                while (fs_stat.now_reading && ret && remainsize > 0) {
                    if (remainsize < readsize) {
                        readsize = remainsize;
                    }
                    if (shHdRead(fd, readbuf, readsize) == -1) {
                        ret = 0;
                    }
                    readbuf += readsize;
                    remainsize -= readsize;
                }
            }
            if (!fs_stat.now_reading) {
                ret = 0;
            }
            fs_stat.now_reading = 0;
            if (shHdCloseC(fd) == -1) {
                ret = 0;
            }
        } else {
            ret = 0;
        }
        if (ret) {
            return 1;
        }
        if (--retry <= 0) {
            retry = 60;
        }
    }
}

static int fsSubCmdHdError(void) {
    return 0;
}

static int fsSubCmdRealRead(union fsFile *fp, void *buf) {
    char fullpath[256];
    char *name;

    if (fp == ID_fsFile(data_null_dev)) {
        if (buf) {
            UtilMemSet(buf, 0, fp->hd.offset);
        }
        return 1;
    }
    if (fp->check.number) {
        return fsSubCmdCdRead(fp->cd.lsn, (fp->cd.size + 2047) / 2048, buf);
    }
    if (fp->check.type & 0x80) {
        name = fp->hd.name;
        if (fp->check.type & 0x20) {
            if (!shPathMakeHdDataBase(fullpath, name)) {
                return 0;
            }
        } else {
            if (!shPathMakeHdData(fullpath, name)) {
                return 0;
            }
        }
        return fsSubCmdHdRead(fullpath, fp->hd.offset, fp->hd.size, buf);
    }
    return 0;
}

static int fsSubCmdRead(union fsFile *fp, void *buf) {
    int ret;
    union fsFile realfp[1];

    if (fsSubCmdSetRealFile0(realfp, fp)) {
        return 0;
    }
    ret = fsSubCmdRealRead(realfp, buf);
    verbose(3, "filecmd.c:1322> %s", realfp->hd.name);
    return ret;
}

static int ___fsCmdRead(union fsFile *fp, void *buf) {
    int ret;

    if (!fsCmdFixFile(fp)) {
        return 0;
    }
    ret = fsSubCmdRead(fp, buf);
    verbose(3, ": read %s.\n", ret ? "finished" : "failed");
    return ret;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1340
/** Reads the whole of file `fp` into `buf`. Reading null.dev is an error. */
int fsCmdRead(union fsFile *fp, void *buf) {
    if (fp == ID_fsFile(data_null_dev)) {
        assert(!"can't read null.dev");
    }
    return ___fsCmdRead(fp, buf);
}

/** Reads `size` bytes at `offset` (both multiples of 2048) of file `fp` into `buf`. */
int fsCmdReadPart(union fsFile *fp, void *buf, int offset, int size) {
    union fsFile partfile[1];

    assert(offset%2048==0);
    assert_dw(size%2048==0);
    partfile->check.type = 0x50;
    partfile->mgf.parent = fp;
    partfile->mgf.offset = offset;
    partfile->mgf.size = size;
    return ___fsCmdRead(partfile, buf);
}

/**
 * Sets the parameters of later disc checks: the media types allowed, the files `fplist` to read
 * into `buflist` and the function `check_func` that checks them. Returns 1.
 */
int fsCmdSetParamForCheckDisk(int media_permission, union fsFile **fplist, void **buflist, int (*check_func)(union fsFile **, void **)) {
    fsCmdParam_media_permission = media_permission;
    fsCmdParam_fplist = fplist;
    fsCmdParam_buflist = buflist;
    fsCmdParam_check_func = check_func;
    return 1;
}

/** fsCmdCdCheckDisk2() with the parameters set by fsCmdSetParamForCheckDisk(). */
int fsCmdCdCheckDisk(int force_check) {
    return fsCmdCdCheckDisk2(force_check, fsCmdParam_media_permission, fsCmdParam_fplist, fsCmdParam_buflist, fsCmdParam_check_func);
}

/**
 * The disc check: waits for the tray to close and a disc of an allowed media type
 * (`media_permission`), reads the files `fplist` into `buflist` and passes them to `check_func`,
 * which returns the disc number. Starts right away with `force_check`, otherwise only after the
 * tray was opened.
 */
int fsCmdCdCheckDisk2(int force_check, int media_permission, union fsFile **fplist, void **buflist, int (*check_func)(union fsFile **, void **)) {
    union fsFile **fpp;
    union fsFile *fp;
    void **bufp;
    void *buf;
    int media;
    int step;
    int open;
    int mmode;
    int number;

    fpp = NULL;
    fp = NULL;
    bufp = NULL;
    buf = NULL;
    media = 0;
    step = 0;
    if (force_check) {
        step = 1;
    }
    do {
        open = fsSubCmdCdCheckTrayOpen();
        switch (step) {
        case 0:
            if (open == 1) {
                fs_stat.disk_number = 0;
                step = 1;
            }
            break;
        case 1:
        case 2:
        default:
            if (open == 1) {
                fs_stat.disk_number = 0;
                step = 1;
                shSyncVEnd(0);
                shCdSync(1);
                break;
            }
            step = 2;
            if (shCdDiskReady(1) != 2) {
                break;
            }
            shSyncVEnd(0);
            shCdSync(1);
            media = fsSubCmdCdGetMedia();
            if (media & 0x180) {
                shSyncVEnd(0);
                shCdSync(1);
                break;
            }
            if (media & media_permission) {
                fs_stat.disk_count++;
                fs_stat.disk_count &= 0xFFFFFF;
                if (fs_stat.disk_count == 0) {
                    fs_stat.disk_count++;
                }
                fs_stat.disk_number = -fs_stat.disk_count;
                step = 5;
            } else if (media & 0x200) {
                step = 3;
            } else {
                step = 4;
            }
            break;
        case 3:
        case 4:
            if (open == 1) {
                step = 1;
                break;
            }
            shSyncVEnd(0);
            shCdSync(1);
            break;
        case 5:
            if (open == 1) {
                step = 1;
                break;
            }
            mmode = fsSubCmdCdGetMmode(media);
            fs_stat.media_type = mmode;
            shCdMmode(mmode);
            step = 6;
            break;
        case 6:
            if (check_func == NULL || fplist == NULL || (fp = *(fpp = fplist)) == NULL) {
                step = 0;
                break;
            }
            step = 7;
        case 7:
            if (open == 1) {
                step = 1;
                break;
            }
            fsCmdFixFile(fp);
            fp = *++fpp;
            if (fp == NULL) {
                fpp = fplist;
                fp = *fpp;
                bufp = buflist;
                buf = *bufp;
                step = 8;
            }
            break;
        case 8:
            if (open == 1) {
                step = 1;
                break;
            }
            if (buf && fsCmdCheckExistFile(fp) > 0) {
                if (fsSubCmdRead(fp, buf)) {
                    fpp++;
                    bufp++;
                } else {
                    shSyncVEnd(0);
                    shCdSync(1);
                }
            } else {
                fpp++;
                bufp++;
            }
            fp = *fpp;
            if (fp == NULL) {
                number = check_func(fplist, buflist);
                if (number > 0) {
                    fs_stat.disk_number = number;
                    step = 0;
                } else {
                    step = 4;
                }
            }
            break;
        }
        fs_stat.disk_check = step;
    } while (step != 0);
    return 1;
}

/**
 * Returns the state of the disc check: 0 idle, 1, 3 and 4 as the check's step, 2 for the other
 * steps.
 */
int fsCmdGetTrayStat(void) {
    int step;

    step = fs_stat.disk_check;
    switch (step) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 3:
        return 3;
    case 4:
        return 4;
    case 2:
    case 5:
    case 6:
    case 7:
    case 8:
    default:
        return 2;
    }
}
