#ifndef SDK_LIBMPEG_H
#define SDK_LIBMPEG_H

/*
 * The EE MPEG-2 decoder library's functions (sceMpeg*), as the movie player (src/movie) uses them,
 * with its callback type and the callback and stream kinds. The library is linked as assembly (no
 * DWARF for its code).
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names are ours. The callback kinds'
 * enumerators (sceMpegCbError...) are an enumeration in the DWARF (sceMpegCbData's `type`); the
 * names MpegCallback, MpegCallbackType, MpegStreamType and MPEG_STREAM_* are ours, with the
 * values the game passes. sceMpeg, sceMpegCbData and sceIpuRAW8 have the DWARF's layouts
 * (sh2/types.h). No SDK header or other SDK file was used.
 */

#include "sh2/types.h"

typedef int (*MpegCallback)(sceMpeg *mp, sceMpegCbData *cb, void *user);

typedef enum {
    sceMpegCbError = 0,
    sceMpegCbNodata = 1,
    sceMpegCbStopDMA = 2,
    sceMpegCbRestartDMA = 3,
    sceMpegCbBackground = 4,
    sceMpegCbTimeStamp = 5,
    sceMpegCbStr = 6
} MpegCallbackType;

/* The elementary streams the player demuxes: video and PCM audio. */
typedef enum {
    MPEG_STREAM_M2V = 0,
    MPEG_STREAM_PCM = 2
} MpegStreamType;

MpegCallback sceMpegAddCallback(sceMpeg *mp, MpegCallbackType type, MpegCallback fn, void *user);
MpegCallback sceMpegAddStrCallback(sceMpeg *mp, MpegStreamType type, int channel, MpegCallback fn,
                                   void *user);
int sceMpegCreate(sceMpeg *mp, unsigned char *work, int work_size);
int sceMpegDelete(sceMpeg *mp);
int sceMpegDemuxPssRing(sceMpeg *mp, unsigned char *data, int len, unsigned char *ring, int ring_size);
int sceMpegGetPictureRAW8(sceMpeg *mp, sceIpuRAW8 *out, int mb_count);
int sceMpegInit(void);
int sceMpegIsEnd(sceMpeg *mp);
int sceMpegReset(sceMpeg *mp);

#endif
