/*
 * The DWARF drops unused parameters: videoDecSetStream's `vd` and the (mp, cb,
 * user) of the mpegNodata/StopDMA/RestartDMA callbacks, so sh2.h declares them
 * with fewer. Their real prototypes are declared below.
 */

#include "sh2.h"
#include "libc/string.h"
#include "sdk/libmpeg.h"

/*
 * pss_videodec.c: the movie's video decoder (the videoDec* functions): the
 * demuxer fills the video input buffer (pss_vibuf.c) and decBs0 decodes
 * pictures with sceMpeg into the output frame buffer (pss_vobuf.c).
 */

#define VIDEODEC_NORMAL 0
#define VIDEODEC_ABORT 1
#define VIDEODEC_FLUSH 2
#define VIDEODEC_END 3

#define UNCACHED(val) ((unsigned char *)(((unsigned int)(val) & 0x0FFFFFFF) | 0x20000000))

int decBs0(VideoDec *vd);



static int cpy2area(unsigned char *pd0, int d0, unsigned char *pd1, int d1, unsigned char *ps0, int s0,
                    unsigned char *ps1, int s1);

/**
 * Sets up video decoder `vd`: the sceMpeg decoder in `mpegWork` (`mpegWorkSize` bytes) with its
 * callbacks, the video input buffer (`data`, `tag`, `tagSize` elements, `n_pts` time stamps at
 * `pts`) and the VU1 colour space conversion. Returns 1.
 */
int videoDecCreate(VideoDec *vd, unsigned char *mpegWork, int mpegWorkSize, u_long128 *data, u_long128 *tag,
                   int tagSize, ViBufTs *pts, int n_pts) {
    sceMpegCreate(&sys_mpeg, mpegWork, mpegWorkSize);
    sceMpegAddCallback(&sys_mpeg, sceMpegCbError, (MpegCallback)mpegError, NULL);
    sceMpegAddCallback(&sys_mpeg, sceMpegCbNodata, mpegNodata, NULL);
    sceMpegAddCallback(&sys_mpeg, sceMpegCbStopDMA, mpegStopDMA, NULL);
    sceMpegAddCallback(&sys_mpeg, sceMpegCbRestartDMA, mpegRestartDMA, NULL);
    sceMpegAddCallback(&sys_mpeg, sceMpegCbTimeStamp, (MpegCallback)mpegTS, NULL);

    vd->state = VIDEODEC_NORMAL;
    viBufCreate(&vd->vibuf, data, tag, tagSize, pts, n_pts);
    cscVu1Init(&vd->csc);

    return 1;
}

/**
 * Registers the demuxer callback `cb` (with `data`) for stream `strType`/`ch`. `vd` is unused.
 * Returns 1.
 */
int videoDecSetStream(VideoDec *vd, int strType, int ch, MpegCallback cb, void *data) {
    sceMpegAddStrCallback(&sys_mpeg, strType, ch, cb, data);
    return 1;
}

/** Returns the free space of the video input buffer of `vd` (two parts when it wraps). */
void videoDecBeginPut(VideoDec *vd, unsigned char **ptr0, int *len0, unsigned char **ptr1, int *len1) {
    viBufBeginPut(&vd->vibuf, ptr0, len0, ptr1, len1);
}

/** Commits `size` bytes written after videoDecBeginPut(). */
void videoDecEndPut(VideoDec *vd, int size) {
    viBufEndPut(&vd->vibuf, size);
}

/** Deletes the video input buffer of `vd` and the sceMpeg decoder. Returns 1. */
int videoDecDelete(VideoDec *vd) {
    viBufDelete(&vd->vibuf);
    sceMpegDelete(&sys_mpeg);
    return 1;
}

/** Makes `vd` stop decoding. */
void videoDecAbort(VideoDec *vd) {
    vd->state = VIDEODEC_ABORT;
}

/** Returns the state of `vd` (VIDEODEC_*). */
unsigned int videoDecGetState(VideoDec *vd) {
    return vd->state;
}

/** Sets the state of `vd` to `state`. Returns the old state. */
unsigned int videoDecSetState(VideoDec *vd, unsigned int state) {
    unsigned int old;

    old = vd->state;
    vd->state = state;
    return old;
}

/**
 * Queues the time stamps `pts_val`/`dts_val` of the `len` bytes at `start` in the video input
 * buffer of `vd`. Returns 0 when the queue is full.
 */
int videoDecPutTs(VideoDec *vd, long pts_val, long dts_val, unsigned char *start, int len) {
    ViBufTs ts;

    ts.pts = pts_val;
    ts.dts = dts_val;
    ts.pos = start - (unsigned char *)vd->vibuf.data;
    ts.len = len;
    return viBufPutTs(&videoDec.vibuf, &ts);
}

/**
 * Appends a sequence end code to the video input stream of `vd` so the last picture is decoded.
 * Returns 0 when there is no room yet.
 */
int videoDecFlush(VideoDec *vd) {
    unsigned char *pd0;
    unsigned char *pd1;
    unsigned char *pd0Unc;
    unsigned char *pd1Unc;
    unsigned char seq_end_code[4] = {0x00, 0x00, 0x01, 0xB7};
    int d0;
    int d1;
    int len;

    videoDecBeginPut(vd, &pd0, &d0, &pd1, &d1);

    if (d0 + d1 < 4) {
        return 0;
    }

    pd0Unc = UNCACHED(pd0);
    pd1Unc = UNCACHED(pd1);
    len = cpy2area(pd0Unc, d0, pd1Unc, d1, seq_end_code, 4, NULL, 0);

    videoDecEndPut(&videoDec, len);
    viBufFlush(&vd->vibuf);

    if (vd->state == VIDEODEC_NORMAL) {
        vd->state = VIDEODEC_FLUSH;
    }
    return 1;
}

/**
 * Body of the decode thread: resets the buffers, decodes the whole stream (decBs0()), then waits
 * until the frames are shown or the decode is aborted.
 */
void videoDecMain(VideoDec *vd) {
    viBufReset(&vd->vibuf);
    voBufReset(&voBuf);

    if (decBs0(vd) < 0) {
        printf("decBs0() failed\n");
    }

    while (voBuf.count) {
        if (videoDecGetState(vd) == VIDEODEC_ABORT) {
            printf("video dec main abort.\n");
            break;
        }
    }

    videoDecSetState(vd, VIDEODEC_END);
}

/* (blank lines keep the assert in decBs0 on its original line, 342) */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 294
/**
 * Decodes the pictures of the stream into the frame buffer until the end or an abort. Returns -1 on
 * an error.
 */
int decBs0(VideoDec *vd) {
    VoBufData *voData;
    sceIpuRAW8 *raw8;
    int ret;
    int status;
    sceMpeg *mp;
    int picture_structure;
    int i;

    status = 1;
    mp = &sys_mpeg;

    while (!sceMpegIsEnd(&sys_mpeg)) {
        if (videoDecGetState(vd) == VIDEODEC_ABORT) {
            status = -1;
            printf("decode thread: aborted\n");
            break;
        }

        while (!(voData = voBufGetData(&voBuf))) {
            switchThread();
        }
        raw8 = (sceIpuRAW8 *)voData;

        if (viBufAddDMA(&vd->vibuf) != 1) {
            printf("viBufAddDMA() failed\n");
            status = -1;
            break;
        }

        WaitSemaPss();
        ret = sceMpegGetPictureRAW8(mp, raw8, 0x400);
        SignalSemaPss();

        if (ret < 0) {
            printf("sceMpegGetPictureRAW8() failed\n");
            status = -1;
            break;
        }

        picture_structure = (mp->flags >> 3) & 3;
        if (picture_structure == 3) {
            csct = (mp->flags & 0x180) ? 2 : 0;
        } else {
            assert(0);
        }

        if (ret < 0) {
            ErrMessage("sceMpegGetPicture() decode error");
        }

        if (mp->frameCount == 0) {
            for (i = 0; i < 5; i++) {
                cscVu1SetTag(voBuf.tagInter[i].v, 0, &voBuf.data[i], mp->width, mp->height);
            }
        }

        voBufIncCount(&voBuf);
        switchThread();
    }

    sceMpegReset(mp);
    return status;
}

/** sceMpeg error callback: prints the message `cberror`. Returns 1. */
int mpegError(sceMpeg *mp, sceMpegCbDataError *cberror, void *user) {
    printf("%s\n", cberror->errMessage);
    return 1;
}

/**
 * sceMpeg no-data callback: lets the other threads run until more stream data arrives. Returns 1.
 */
int mpegNodata(sceMpeg *mp, sceMpegCbData *cb, void *user) {
    SignalSemaPss();
    switchThread();
    WaitSemaPss();
    viBufAddDMA(&videoDec.vibuf);
    return 1;
}

/** sceMpeg stop-DMA callback: does nothing. Returns 1. */
int mpegStopDMA(sceMpeg *mp, sceMpegCbData *cb, void *user) {
    viBufStopDMA(&videoDec.vibuf);
    return 1;
}

/** sceMpeg restart-DMA callback: does nothing. Returns 1. */
int mpegRestartDMA(sceMpeg *mp, sceMpegCbData *cb, void *user) {
    viBufRestartDMA(&videoDec.vibuf);
    return 1;
}

/**
 * sceMpeg time stamp callback: stores the time stamp of the picture being decoded in `cbts`.
 * Returns 1.
 */
int mpegTS(sceMpeg *mp, sceMpegCbDataTimeStamp *cbts, void *user) {
    ViBufTs ts;

    viBufGetTs(&videoDec.vibuf, &ts);
    cbts->pts = ts.pts;
    cbts->dts = ts.dts;
    return 1;
}

static int cpy2area(unsigned char *pd0, int d0, unsigned char *pd1, int d1, unsigned char *ps0, int s0,
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
