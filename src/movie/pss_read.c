#include "sh2.h"
#include "libc/string.h"

/*
 * pss_read.c: the demuxer's stream callbacks (its own unit in the DWARF).
 * They copy one elementary-stream packet out of the read ring buffer into the
 * video or audio decoder's input buffer.
 */

#define min(a, b) ((a) > (b) ? (b) : (a))
#define UNCACHED(p) (((unsigned int)(p) & 0x0FFFFFFF) | 0x20000000)

static int copy2area(unsigned char *pd0, int d0, unsigned char *pd1, int d1, unsigned char *ps0, int s0,
                     unsigned char *ps1, int s1);

/**
 * Demuxer callback for a video packet `str`: copies its data out of the read buffer into the video
 * decoder's input buffer and queues its time stamp. `data` is the read buffer; `mp` is unused.
 */
int videoCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data) {
    ReadBuf *rb = (ReadBuf *)data;
    unsigned char *ps0 = str->data;
    unsigned char *ps1 = rb->data;
    int s0 = min(rb->data + rb->size - str->data, str->len);
    int s1 = str->len - s0;
    unsigned char *pd0;
    unsigned char *pd1;
    unsigned char *pd0Unc;
    unsigned char *pd1Unc;
    int d0;
    int d1;
    int len;

    videoDecBeginPut(&videoDec, &pd0, &d0, &pd1, &d1);

    pd0Unc = (unsigned char *)UNCACHED(pd0);
    pd1Unc = (unsigned char *)UNCACHED(pd1);

    len = copy2area(pd0Unc, d0, pd1Unc, d1, ps0, s0, ps1, s1);

    if (len > 0) {
        if (!videoDecPutTs(&videoDec, str->pts, str->dts, pd0, len)) {
            ErrMessage("pts buffer overflow\n");
        }
    }

    videoDecEndPut(&videoDec, len);

    return len > 0 ? 1 : 0;
}

/**
 * Demuxer callback for an audio packet `str`: copies its PCM data out of the read buffer into the
 * audio decoder's ring buffer. `data` is the read buffer; `mp` is unused.
 */
int pcmCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data) {
    ReadBuf *rb = (ReadBuf *)data;
    unsigned char *ps1 = rb->data;
    unsigned char *ps0 = str->data;
    int s0;
    int s1;
    unsigned char *pd0;
    unsigned char *pd1;
    int d0;
    int d1;
    int len;
    int ret;

    if ((ps0 += 4) >= rb->data + rb->size) {
        ps0 -= rb->size;
    }
    len = str->len - 4;
    s0 = min(rb->data + rb->size - ps0, len);
    s1 = len - s0;

    audioDecBeginPut(&audioDec, &pd0, &d0, &pd1, &d1);

    ret = copy2area(pd0, d0, pd1, d1, ps0, s0, ps1, s1);

    audioDecEndPut(&audioDec, ret);

    return ret > 0 ? 1 : 0;
}

static int copy2area(unsigned char *pd0, int d0, unsigned char *pd1, int d1, unsigned char *ps0, int s0,
                     unsigned char *ps1, int s1) {
    if (d0 + d1 < s0 + s1) {
        return 0;
    }

    if (s0 >= d0) {
        memcpy(pd0, ps0, d0);
        memcpy(pd1, ps0 + d0, s0 - d0);
        memcpy(pd1 + s0 - d0, ps1, s1);
    } else {
        if (s1 >= d0 - s0) {
            memcpy(pd0, ps0, s0);
            memcpy(pd0 + s0, ps1, d0 - s0);
            memcpy(pd1, ps1 + d0 - s0, s1 - (d0 - s0));
        } else {
            memcpy(pd0, ps0, s0);
            memcpy(pd0 + s0, ps1, s1);
        }
    }
    return s0 + s1;
}
