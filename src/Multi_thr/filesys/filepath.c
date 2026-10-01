/*
 * filepath.c: builds full device paths ("cdrom0:\PATH\FILE.EXT;1", "host0:path/file")
 * for cd and hd (host) files from the boot option directories.
 */

#include "sh2.h"

extern char *execEnv_database_path;

static char *devstr_cdrom0 = "cdrom0:";
static char *devstr_host0 = "host0:";

/*
 * Returns 0 when `filename` must not get a directory prefix: it starts with
 * "/", "\", "./", "../" or a drive ("x:").
 */
static int pathname_skipcheck(char *filename) {
    switch (filename[0]) {
    case '/':
    case '\\':
        return 0;
    case '.':
        switch (filename[1]) {
        case '/':
        case '\\':
            return 0;
        case '.':
            switch (filename[2]) {
            case '/':
            case '\\':
                return 0;
            }
            break;
        }
        break;
    default:
        if (filename[1] == ':') {
            /* @bug filename[1] is ':' here, so the upper bounds always hold (filename[0] was meant):
             * any character from 'A' up before a ':' counts as a drive letter. */
            if (filename[0] >= 'a' && filename[1] <= 'z') {
                return 0;
            }
            if (filename[0] >= 'A' && filename[1] <= 'Z') {
                return 0;
            }
        }
        break;
    }
    return 1;
}

/** Returns whether `filename` is used as is, without a directory prefix. */
int shPathCheckExecRoot(char *filename) {
    return pathname_skipcheck(filename) == 0;
}

static int shPathMakeCd(char *fullpath, char *pathname, char *filename) {
    char *prefix = devstr_cdrom0;
    char suffix[3] = ";1";
    int len;
    char *work;
    char work1[256];
    char work2[260];

    if (pathname_skipcheck(filename)) {
        UtilStrPathCatL(work1, pathname, filename, sizeof(work1));
    } else {
        while (*filename == '.') {
            filename++;
        }
        UtilStrCpyL(work1, filename, sizeof(work1));
    }
    work1[sizeof(work1) - 1] = '\0';
    for (work = work1; *work == '.'; work++) {
    }
    UtilStrPathCatL(work2, "/", work, 256);
    UtilStrConvertCdPath(work2);
    len = UtilStrCat3L(fullpath, prefix, work2, suffix, 256);
    return len < 256;
}

static int shPathMakeHd(char *fullpath, char *rootpath, char *pathname, char *filename) {
    char *prefix;
    int len;
    char work1[256];
    char work2[256];

    prefix = devstr_host0;
    if (pathname_skipcheck(pathname)) {
        UtilStrPathCatL(work1, rootpath, pathname, sizeof(work1));
    } else {
        UtilStrCpyL(work1, pathname, sizeof(work1));
    }
    work1[sizeof(work1) - 1] = '\0';
    if (pathname_skipcheck(filename)) {
        UtilStrPathCatL(work2, work1, filename, sizeof(work2));
    } else {
        UtilStrCpyL(work2, filename, sizeof(work2));
    }
    work2[sizeof(work2) - 1] = '\0';
    UtilStrConvertHdPath(work2);
    len = UtilStrCatL(fullpath, prefix, work2, 256);
    return len < 256;
}

/**
 * Builds the full path of IOP module `filename` into `fullpath`: on the HD when the IOP HD path
 * boot option is set, otherwise on the CD.
 */
int shPathMakeIop(char *fullpath, char *filename) {
    char *ioppath;
    char *hdpath;
    char nullstr[1] = "";

    hdpath = execEnv_iop_path_hd;
    if (hdpath == NULL) {
        ioppath = execEnv_iop_path;
        if (ioppath == NULL) {
            ioppath = nullstr;
        }
        return shPathMakeCd(fullpath, ioppath, filename);
    } else {
        ioppath = execEnv_host_path;
        if (ioppath == NULL) {
            ioppath = nullstr;
        }
        return shPathMakeHd(fullpath, ioppath, hdpath, filename);
    }
}

/** Builds the full CD path of data file `filename` (data path boot option) into `fullpath`. */
int shPathMakeCdData(char *fullpath, char *filename) {
    char *datapath;
    char nullstr[1] = "";

    datapath = execEnv_data_path;
    if (datapath == NULL) {
        datapath = nullstr;
    }
    return shPathMakeCd(fullpath, datapath, filename);
}

/** Builds the full HD path of data file `filename` (data path boot option) into `fullpath`. */
int shPathMakeHdData(char *fullpath, char *filename) {
    char *datapath;
    char *hdpath;
    char nullstr[1] = "";

    datapath = execEnv_data_path;
    if (datapath == NULL) {
        datapath = nullstr;
    }
    hdpath = execEnv_host_path;
    if (hdpath == NULL) {
        hdpath = nullstr;
    }
    return shPathMakeHd(fullpath, hdpath, datapath, filename);
}

/** Builds the full CD path of `filename` under the database path into `fullpath`. */
int shPathMakeCdDataBase(char *fullpath, char *filename) {
    char *datapath;
    char nullstr[1] = "";

    datapath = execEnv_database_path;
    if (datapath == NULL) {
        datapath = nullstr;
    }
    return shPathMakeCd(fullpath, datapath, filename);
}

/** Builds the full HD path of `filename` under the database path into `fullpath`. */
int shPathMakeHdDataBase(char *fullpath, char *filename) {
    char *datapath;
    char *hdpath;
    char nullstr[1] = "";

    datapath = execEnv_database_path;
    if (datapath == NULL) {
        datapath = nullstr;
    }
    hdpath = execEnv_host_path;
    if (hdpath == NULL) {
        hdpath = nullstr;
    }
    return shPathMakeHd(fullpath, hdpath, datapath, filename);
}

/** Same as shPathMakeCdDataBase(). */
int ___shPathMakeCdDataBase(char *fullpath, char *filename) {
    return shPathMakeCdDataBase(fullpath, filename);
}

/** Same as shPathMakeCdData(). */
int ___shPathMakeCdData(char *fullpath, char *filename) {
    return shPathMakeCdData(fullpath, filename);
}

/** Same as shPathMakeHdDataBase(). */
int ___shPathMakeHdDataBase(char *fullpath, char *filename) {
    return shPathMakeHdDataBase(fullpath, filename);
}

/** Same as shPathMakeHdData(). */
int ___shPathMakeHdData(char *fullpath, char *filename) {
    return shPathMakeHdData(fullpath, filename);
}
