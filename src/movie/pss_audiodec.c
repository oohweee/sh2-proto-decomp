/* Matching: sd_call is called without a prototype, as in the original (arguments passed
 * unconverted; the DWARF prototype has four parameters, this file passes five), so this file
 * leaves out the generated prototype (config/prototype_overrides.txt). */
#define SH2_LOCAL_sd_call
#include "sh2.h"
#include "libc/string.h"
#include "sdk/eekernel.h"
#include "sdk/libsdr.h"
#include "sdk/sifdev.h"
#include "sdk/sifrpc.h"

/*
 * pss_audiodec.c: the movie player's PCM stream (its own unit in the DWARF):
 * the demuxer fills a ring buffer here, and audioDecSendToIOP moves it on
 * into the IOP buffer that the SPU2 plays by auto-DMA.
 */

int sd_call();

#define AUDEC_INIT 0
#define AUDEC_PRESET 1
#define AUDEC_PLAY 2
#define AUDEC_PAUSE 3

#define AUDEC_HEADER_SIZE (sizeof(ad->sshd) + sizeof(ad->ssbd))

#define SILENCE_SIZE 0x1000

/* sceSdRemote commands (rSdBlockTrans and rSdVoiceTrans as the game's messages name them) */
#define SDREMOTE_SET_PARAM 0x8010
#define rSdVoiceTrans 0x80D0
#define rSdBlockTrans 0x80E0
#define SDREMOTE_BLOCK_TRANS_STATUS 0x8100

#define min(a, b) ((a) > (b) ? (b) : (a))

static int sendToIOP2area(int pd0, int d0, int pd1, int d1, unsigned char *ps0, int s0, unsigned char *ps1, int s1);
static int sendToIOP(int dst, unsigned char *src, int size);
static void changeInputVolume(unsigned int val);
static void iopGetArea(int *pd0, int *d0, int *pd1, int *d1, AudioDec *ad, int pos);

static u_long128 _0_buf[SILENCE_SIZE / 16] __attribute__((aligned(64)));

/** Allocates the IOP buffers once and clears the zero buffer. */
void pssAudioColdInit(AudioDec *ad, int iopBuffSize) {
    ad->iopBuff = (int)sceSifAllocIopHeap(iopBuffSize);
    if (ad->iopBuff < 0) {
        printf("Cannot allocate IOP memory\n");
    }
    ad->iopZero = (int)sceSifAllocIopHeap(SILENCE_SIZE);
    if (ad->iopZero < 0) {
        printf("Cannot allocate IOP memory\n");
    }
    memset(_0_buf, 0, SILENCE_SIZE);
    sendToIOP(ad->iopZero, (unsigned char *)_0_buf, SILENCE_SIZE);
}

/**
 * Sets up audio decoder `ad` with the ring buffer `buff` of `buffSize` bytes and an IOP buffer of
 * `iopBuffSize` bytes. Returns 1.
 */
int audioDecCreate(AudioDec *ad, unsigned char *buff, int buffSize, int iopBuffSize) {
    ad->state = AUDEC_INIT;
    ad->hdrCount = 0;
    ad->data = buff;
    ad->put = 0;
    ad->count = 0;
    ad->size = buffSize;
    ad->totalBytes = 0;
    ad->totalBytesSent = 0;
    ad->iopBuffSize = iopBuffSize;
    ad->iopLastPos = 0;
    ad->iopPausePos = 0;
    return 1;
}

/** Does nothing (the buffer argument is unused). Returns 1. */
int audioDecDelete(AudioDec *ad) {
    return 1;
}

/**
 * Pauses the sound output of `ad`: mutes the input volume and stops the SPU2 auto-DMA, remembering
 * where it stopped.
 */
void audioDecPause(AudioDec *ad) {
    int ret;
    int retry;

    ad->state = AUDEC_PAUSE;
    changeInputVolume(0);

    retry = 64;
    while ((ret = sceSdRemote(1, rSdBlockTrans, 0, 2, 0, 0)) < 0) {
        if (--retry < 0) {
            printf("rSdBlockTrans failed\n");
            break;
        }
    }
    ad->iopPausePos = (ret & 0x00FFFFFF) - ad->iopBuff;

    sd_call(0x3F8, 0, 0, 0, 0);

    retry = 64;
    while (sceSdRemote(1, rSdVoiceTrans, 0, 0, ad->iopZero, 0x4000, SILENCE_SIZE) < 0) {
        if (--retry < 0) {
            printf("rSdVoiceTrans failed\n");
            break;
        }
    }
}

/** Restarts the SPU2 auto-DMA of `ad` where it stopped, at full volume. */
void audioDecResume(AudioDec *ad) {
    changeInputVolume(0x7FFF);
    while (sceSdRemote(1, rSdBlockTrans, 0, 0x13, ad->iopBuff, ad->iopBuffSize / 1024 * 1024,
                       ad->iopBuff + ad->iopPausePos) < 0) {
        printf("rSdBlockTrans failed\n");
    }
    ad->state = AUDEC_PLAY;
}

/** Starts the sound output of `ad` (audioDecResume()). */
void audioDecStart(AudioDec *ad) {
    audioDecResume(ad);
}

/** Stops the output of `ad` and empties its buffers, back to waiting for the stream header. */
void audioDecReset(AudioDec *ad) {
    audioDecPause(ad);
    ad->state = AUDEC_INIT;
    ad->hdrCount = 0;
    ad->put = 0;
    ad->count = 0;
    ad->totalBytes = 0;
    ad->totalBytesSent = 0;
    ad->iopLastPos = 0;
    ad->iopPausePos = 0;
}

/** Where the demuxer writes next: the stream header first, then the ring buffer. */
void audioDecBeginPut(AudioDec *ad, unsigned char **ptr0, int *len0, unsigned char **ptr1, int *len1) {
    int len;

    if (ad->state == AUDEC_INIT) {
        *ptr0 = (unsigned char *)&ad->sshd + ad->hdrCount;
        *len0 = AUDEC_HEADER_SIZE - ad->hdrCount;
        *ptr1 = ad->data;
        *len1 = ad->size;
        return;
    }

    len = ad->size - ad->count;
    if (ad->size - ad->put >= len) {
        *ptr0 = ad->data + ad->put;
        *len0 = len;
        *ptr1 = NULL;
        *len1 = 0;
    } else {
        *ptr0 = ad->data + ad->put;
        *len0 = ad->size - ad->put;
        *ptr1 = ad->data;
        *len1 = len - (ad->size - ad->put);
    }
}

/**
 * Commits `size` bytes written after audioDecBeginPut(): first to the stream header, then to the
 * PCM ring buffer of `ad`.
 */
void audioDecEndPut(AudioDec *ad, int size) {
    if (ad->state == AUDEC_INIT) {
        int hdr_add = min(size, AUDEC_HEADER_SIZE - ad->hdrCount);

        ad->hdrCount += hdr_add;
        if (ad->hdrCount >= AUDEC_HEADER_SIZE) {
            ad->state = AUDEC_PRESET;
        }
        size -= hdr_add;
    }
    ad->put = (ad->put + size) % ad->size;
    ad->count += size;
    ad->totalBytes += size;
}

/** Returns whether the IOP buffer of `ad` has been filled once (playing can start). */
int audioDecIsPreset(AudioDec *ad) {
    return ad->totalBytesSent >= ad->iopBuffSize;
}

/** Moves as much buffered PCM (in 1KB units) to the IOP buffer as there is room for. */
int audioDecSendToIOP(AudioDec *ad) {
    int pd0;
    int pd1;
    int d0;
    int d1;
    unsigned char *ps0;
    unsigned char *ps1;
    int s0;
    int s1;
    int count_sent;
    int countAdj;
    int pos;

    count_sent = 0;
    switch (ad->state) {
    case AUDEC_INIT:
        return 0;
    case AUDEC_PRESET:
        pd0 = ad->iopBuff + ad->totalBytesSent % ad->iopBuffSize;
        d0 = ad->iopBuffSize - ad->totalBytesSent;
        pd1 = 0;
        d1 = 0;
        break;
    case AUDEC_PLAY:
        pos = (sceSdRemote(1, SDREMOTE_BLOCK_TRANS_STATUS, 0) & 0x00FFFFFF) - ad->iopBuff;
        if (!(0 <= pos && pos <= ad->iopBuffSize)) {
            printf("audioDecSendToIOP: pos=%d\n", pos);
        }
        iopGetArea(&pd0, &d0, &pd1, &d1, ad, pos);
        break;
    case AUDEC_PAUSE:
        return 0;
    }

    ps0 = ad->data + (ad->put - ad->count + ad->size) % ad->size;
    ps1 = ad->data;

    countAdj = ad->count / 1024 * 1024;
    s0 = min(ad->data + ad->size - ps0, countAdj);
    s1 = countAdj - s0;

    if (d0 + d1 >= 1024 && s0 + s1 >= 1024) {
        count_sent = sendToIOP2area(pd0, d0, pd1, d1, ps0, s0, ps1, s1);
    }

    ad->count -= count_sent;
    ad->totalBytesSent += count_sent;
    ad->iopLastPos = (ad->iopLastPos + count_sent) % ad->iopBuffSize;

    return count_sent;
}

/* Free area of the IOP buffer behind the play position pos. */
static void iopGetArea(int *pd0, int *d0, int *pd1, int *d1, AudioDec *ad, int pos) {
    int len;

    len = (pos + ad->iopBuffSize - ad->iopLastPos - 1024) % ad->iopBuffSize;
    len = len / 1024 * 1024;

    if (ad->iopBuffSize - ad->iopLastPos >= len) {
        *pd0 = ad->iopBuff + ad->iopLastPos;
        *d0 = len;
        *pd1 = 0;
        *d1 = 0;
    } else {
        *pd0 = ad->iopBuff + ad->iopLastPos;
        *d0 = ad->iopBuffSize - ad->iopLastPos;
        *pd1 = ad->iopBuff;
        *d1 = len - (ad->iopBuffSize - ad->iopLastPos);
    }
}

static int sendToIOP2area(int pd0, int d0, int pd1, int d1, unsigned char *ps0, int s0, unsigned char *ps1, int s1) {
    if (d0 + d1 < s0 + s1) {
        int diff = (s0 + s1) - (d0 + d1);

        if (diff >= s1) {
            s0 -= diff - s1;
            s1 = 0;
        } else {
            s1 -= diff;
        }
    }

    if (s0 >= d0) {
        sendToIOP(pd0, ps0, d0);
        sendToIOP(pd1, ps0 + d0, s0 - d0);
        sendToIOP(pd1 + s0 - d0, ps1, s1);
    } else {
        if (s1 >= d0 - s0) {
            sendToIOP(pd0, ps0, s0);
            sendToIOP(pd0 + s0, ps1, d0 - s0);
            sendToIOP(pd1, ps1 + d0 - s0, s1 - (d0 - s0));
        } else {
            sendToIOP(pd0, ps0, s0);
            sendToIOP(pd0 + s0, ps1, s1);
        }
    }
    return s0 + s1;
}

static int sendToIOP(int dst, unsigned char *src, int size) {
    sceSifDmaData transData;
    int did;

    if (size <= 0) {
        return 0;
    }

    transData.data = (unsigned int)src;
    transData.addr = (unsigned int)dst;
    transData.size = size;
    transData.mode = 0;
    FlushCache(0);

    while (!(did = sceSifSetDma(&transData, 1))) {
        printf("sceSifSetDma() failed\n");
    }

    while (sceSifDmaStat(did) >= 0) {
    }

    return size;
}

static void changeInputVolume(unsigned int val) {
    sceSdRemote(1, SDREMOTE_SET_PARAM, 0xF80, val);
    sceSdRemote(1, SDREMOTE_SET_PARAM, 0x1080, val);
}
