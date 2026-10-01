/*
 * fcread.c: file access by file index (fsFileIndex), with the merge-file
 * (-m boot option) redirection to plain hd files.
 */

#include "sh2.h"

static void warning_no_entry(union fsFileIndex *id) {
    printf("fcread.c:20> WARNING! file entry is none! %x\n", id);
}

/* With merge files disabled, turn a file inside a merge file into a direct hd file. */
static int file_trans_merge_to_direct(union fsFileIndex *id) {
    int ret;
    union fsFile *file;

    ret = 0;
    file = id->index.fp;
    if (execEnv_hd_merge_file && (file->check.type & 0x10)) {
        file->check.pad2 = 0;
        file->hd.type = 3;
        file->hd.padding = 0;
        file->hd.name = id->index.name;
        file->hd.offset = 0;
        file->hd.size = 0;
        ret = 1;
    }
    return ret;
}

/**
 * Resolves where the file of `id` is (CD or HD), on the file server thread. Returns the command ID,
 * or -1.
 */
int FcFixFile(union fsFileIndex *id) {
    union fsFile *file;

    if (id == NULL) {
        warning_no_entry(id);
        return -1;
    }
    file = id->index.fp;
    file_trans_merge_to_direct(id);
    file = fsCmdCheckFixFile(file);
    if (execEnv_hd_merge_file) {
        return file ? fcFixFile(file) : -1;
    }
    if (file) {
        return fcFixFile(file);
    }
    return -1;
}

/** Returns the size of `file`. */
int fcGetFileSize(union fsFile *file) {
    union fsFile real[1];

    fsCmdSetRealFile(real, file);
    return real[0].cd.size;
}

/** Returns the size of the file of `id` (0 when there is none). */
int FcGetFileSize(union fsFileIndex *id) {
    union fsFile *file;
    int fid;

    if (id == NULL) {
        warning_no_entry(id);
        return 0;
    }
    fid = FcFixFile(id);
    if (fid != -1) {
        fsSync(0, fid);
    }
    file = id->index.fp;
    return fcGetFileSize(file);
}

/** Queues a read of the whole file of `id` into `databuf`. Returns the command ID, or -1. */
int FcRead(union fsFileIndex *id, void *databuf) {
    union fsFile *file;

    if (id == NULL) {
        warning_no_entry(id);
        return -1;
    }
    file = id->index.fp;
    file_trans_merge_to_direct(id);
    fcFixFile(file);
    return fcRead(file, databuf);
}

/**
 * Queues a read of `size` bytes at `offset` of the file of `id` into `databuf`. Returns the command
 * ID, or -1.
 */
int FcReadPart(union fsFileIndex *id, void *databuf, int offset, int size) {
    union fsFile *file;

    if (id == NULL) {
        warning_no_entry(id);
        return -1;
    }
    file = id->index.fp;
    file_trans_merge_to_direct(id);
    fcFixFile(file);
    return fcReadPart(file, databuf, offset, size);
}
