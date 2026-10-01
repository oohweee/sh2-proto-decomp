#include "sh2.h"

/*
 * pss_strfile.c: the movie stream source. One file is open at a time; reads
 * are sequential from the current offset.
 *
 * Matching: the assert message bakes in "pss_strfile.c:58", so the
 * assert has to stay on line 58; the blank lines below keep it there.
 */

static struct PssStrFile d = {NULL, 0, 0};

/** Opens `id` for streaming. */
int strFileOpen(union fsFileIndex *id) {

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 58
    assert(id!=0);
    d.id = id;
    d.ofs = 0;
    d.size = FcGetFileSize(id);
    return 1;
}

/** Returns the size of the open movie file. */
int strFileSize(void) {
    return FcGetFileSize(d.id);
}

/** Reads `size` bytes at the current offset; -1 at the end of the file. */
int strFileRead(void *buf, int size) {
    int count;
    int fid;

    if (d.id != NULL) {
        count = d.size - d.ofs;
        if (count <= 0) {
            return -1;
        }
        count = size;
        while ((fid = FcReadPart(d.id, buf, d.ofs, size)) == -1) {
        }
        fsSync(0, fid);
        d.ofs += size;
    }
    return count;
}

/** Closes the movie file. Returns 1. */
int strFileClose(void) {
    d.id = NULL;
    return 1;
}
