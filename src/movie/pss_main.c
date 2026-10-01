/*
 * The DWARF drops unused parameters: videoDecSetStream's `vd` and the buffer
 * argument of readBufDelete, voBufDelete and audioDecDelete, so sh2.h declares
 * them with fewer. Their real prototypes are declared below.
 */

#include "sh2.h"
#include "libc/string.h"
#include "sdk/eekernel.h"
#include "sdk/libgraph.h"
#include "sdk/libmpeg.h"

/*
 * pss_main.c: the PSS movie player's control (its own unit in the DWARF):
 * pssMain steps the player state machine once per frame, feeding the stream
 * through the read buffer into the demuxer.
 */


extern int _gp;

#define PCW pss_common_work


#define UNCACHED(val) ((void *)(((unsigned int)(val) & 0x0FFFFFFF) | 0x20000000))

/* pssExecCtrl.status */
#define PSS_STAT_IDLE 0
#define PSS_STAT_PRELOAD 1
#define PSS_STAT_LOAD 2
#define PSS_STAT_READY 3
#define PSS_STAT_PLAY 4
#define PSS_STAT_PAUSE 5
#define PSS_STAT_FINISH 6

#define READ_CHUNK_SIZE 0x10000

static int readMpeg(VideoDec *vd, ReadBuf *rb);
static void pssMpegFinish(VideoDec *vd);
static void pssMpegPlayStart(void);
static int isAudioOK(void);
static int initAll(union fsFileIndex *id);
static void termAll(void);
static void defMain(void);

int isWithAudio = 1;

sceMpeg sys_mpeg;
static VoBufTag voBufTagInter[5] __attribute__((aligned(64)));
static u_long128 viBufTag[0x101] __attribute__((aligned(64)));
VideoDec videoDec;
AudioDec audioDec;
VoBuf voBuf;
int frd;
static int intc_vs_changed;
static int intc_gs_changed;

/** Cold init of the movie player: idle state, and the IOP audio buffer (24 KB). */
void pssSystemColdInit(void) {
    pssExecCtrl.status = PSS_STAT_IDLE;
    pssExecCtrl.ctrl = 0;
    pssAudioColdInit(&audioDec, 0x6000);
}

/* (blank lines keep the assert below on its original line, 162) */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 156
/**
 * Sets the player's work areas: the common work `p0` (cleared) and the buffers `p1`-`p5` (MPEG
 * work, read buffer, video input data, frame data, frame tags).
 */
void pssSetWorkAddress(void *p0, void *p1, void *p2, void *p3, void *p4, void *p5) {
    PCW = p0;
    assert(PCW);
    memset(PCW, 0, sizeof(struct _PssCommonWork));
    PCW->mpegWorkp = p1;
    PCW->readBufp = p2;
    PCW->viBufDatap = p3;
    PCW->voBufDatap = p4;
    PCW->tagInterDatap = p5;
    FlushCache(0);
}

/** Prepares a movie session: audio on, no abort, display set up, one vblank per game frame. */
void pssInit(void) {
    isWithAudio = 1;
    pssExecCtrl.movieabort = 0;
    pssInitDisplay();
    shSetDF(1);
    fontClear();
}

/** Clears the movie display. */
void pssExit(void) {
    pssDispClear();
}

/**
 * Requests control `ctrl` of the player (1 or 3: open file `id` at frame `frame`; 2: play the
 * prepared movie; 4: pause; 5: resume; 6: stop). Returns 0 when a request is pending or the state
 * doesn't allow it.
 */
int pssSetControlData(int ctrl, union fsFileIndex *id, int frame) {
    if (pssExecCtrl.ctrl) {
        return 0;
    }

    switch (ctrl) {
    case 0:
        break;
    case 1:
        if (pssExecCtrl.status) {
            return 0;
        }
        pssExecCtrl.file = id;
        pssExecCtrl.stFrame = frame;
        break;
    case 2:
        if (pssExecCtrl.status != PSS_STAT_READY) {
            return 0;
        }
        break;
    case 3:
        if (pssExecCtrl.status) {
            return 0;
        }
        pssExecCtrl.file = id;
        pssExecCtrl.stFrame = frame;
        break;
    case 4:
        if (pssExecCtrl.status != PSS_STAT_PLAY) {
            return 0;
        }
        pssExecCtrl.isPaused = 1;
        break;
    case 5:
        if (pssExecCtrl.status != PSS_STAT_PAUSE) {
            return 0;
        }
        pssExecCtrl.isPaused = 0;
        break;
    case 6:
        if (pssExecCtrl.status != PSS_STAT_PLAY && pssExecCtrl.status != PSS_STAT_PAUSE) {
            return 0;
        }
        break;
    }

    pssExecCtrl.ctrl = ctrl;
    return 1;
}

/**
 * Steps the player state machine once per frame: prepares, plays (reading the stream and feeding
 * the demuxer), pauses and stops movies as requested. Returns the player status.
 */
int pssMain(void) {
    int sts;

    switch (sts = pssExecCtrl.status) {
    case PSS_STAT_IDLE:
        if (pssExecCtrl.ctrl == 1) {
            initAll(pssExecCtrl.file);
            pssExecCtrl.ctrl = 0;
            pssExecCtrl.status = PSS_STAT_PRELOAD;
            pssExecCtrl.cnt = 0;
            pssExecCtrl.proceed_zero_count = 0;
        } else if (pssExecCtrl.ctrl == 3) {
            initAll(pssExecCtrl.file);
            pssExecCtrl.ctrl = 0;
            pssExecCtrl.status = PSS_STAT_LOAD;
            pssExecCtrl.cnt = 0;
            pssExecCtrl.proceed_zero_count = 0;
        } else {
            break;
        }
        pssExecCtrl.readrest = strFileSize();
        pssExecCtrl.writerest = strFileSize();
        pssExecCtrl.isPaused = 0;
        pssExecCtrl.isStarted = 0;
        pssExecCtrl.proceed_zero_count = 0;
        pssExecCtrl.framecnt = 0;
        break;
    case PSS_STAT_PRELOAD:
    case PSS_STAT_LOAD:
        if (readMpeg(&videoDec, PCW->readBufp) <= 0) {
            pssExecCtrl.status = PSS_STAT_FINISH;
        }
        break;
    case PSS_STAT_READY:
        if (pssExecCtrl.ctrl == 2) {
            pssExecCtrl.status = PSS_STAT_PLAY;
            pssMpegPlayStart();
        }
        if (readMpeg(&videoDec, PCW->readBufp) <= 0) {
            pssExecCtrl.status = PSS_STAT_FINISH;
        }
        break;
    case PSS_STAT_PLAY:
        if (pssExecCtrl.ctrl == 6) {
            pssExecCtrl.status = PSS_STAT_FINISH;
        } else if (pssExecCtrl.ctrl == 4) {
            pssExecCtrl.status = PSS_STAT_PAUSE;
            pssExecCtrl.isPaused = 1;
            endDisplay();
        }
        if (readMpeg(&videoDec, PCW->readBufp) <= 0) {
            pssExecCtrl.status = PSS_STAT_FINISH;
        }
        break;
    case PSS_STAT_PAUSE:
        if (pssExecCtrl.ctrl == 5) {
            pssExecCtrl.status = PSS_STAT_PLAY;
            pssExecCtrl.isPaused = 0;
            startDisplay(1);
        } else if (pssExecCtrl.ctrl == 6) {
            pssExecCtrl.status = PSS_STAT_FINISH;
        }
        if (readMpeg(&videoDec, PCW->readBufp) <= 0) {
            pssExecCtrl.status = PSS_STAT_FINISH;
        }
        break;
    case PSS_STAT_FINISH:
        pssMpegFinish(&videoDec);
        termAll();
        pssExecCtrl.status = PSS_STAT_IDLE;
        break;
    }

    pssExecCtrl.ctrl = 0;
    return pssExecCtrl.status;
}

/* (blank lines keep the assert below on its original line, 420) */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 392
static int readMpeg(VideoDec *vd, ReadBuf *rb) {
    unsigned char *put_ptr;
    unsigned char *get_ptr;
    int putsize;
    int getsize;
    int count;
    int proceed;

    if (pssExecCtrl.isPaused || (pssExecCtrl.writerest > 4 && !videoDecGetState(vd))) {
        putsize = readBufBeginPut(rb, &put_ptr);
        if (pssExecCtrl.readrest > 0 && putsize >= READ_CHUNK_SIZE) {
            count = strFileRead(put_ptr, READ_CHUNK_SIZE);
            if (count < 0) {
                videoDecAbort(&videoDec);
            } else {
                int tmp;

                tmp = readBufEndPut(rb, count);
                if (tmp != count) {
                    printf("readBufEndPut: count=%d, tmp=%d\n", count, tmp);
                    PssBreakPoint();
                }
                pssExecCtrl.readrest -= tmp;
            }
        }
        switchThread();

        getsize = readBufBeginGet(rb, &get_ptr);
        assert_dw(getsize >= 0);
        if (getsize > 0) {
            int tmp;

            proceed = sceMpegDemuxPssRing(&sys_mpeg, get_ptr, getsize, rb->data, rb->size);
            if (pssExecCtrl.isStarted && proceed == 0) {
                unsigned short diff;

                diff = *T0_COUNT - pssExecCtrl.proceed_zero_count;
                if (diff > 0x64000) {
                    printf("sceMpegDemuxPssRing() proceed timeout: %d\n", diff);
                    videoDecAbort(&videoDec);
                    return -1;
                }
            } else {
                pssExecCtrl.proceed_zero_count = *T0_COUNT;
            }

            tmp = readBufEndGet(rb, proceed);
            if (tmp != proceed) {
                printf("readBufEndGet: proceed=%d, tmp=%d\n", proceed, tmp);
            }
            pssExecCtrl.writerest -= tmp;
        }
        proceedAudio();

        if (!pssExecCtrl.isStarted && voBufIsFull(&voBuf) && isAudioOK()) {
            if (pssExecCtrl.status == PSS_STAT_LOAD && sys_mpeg.frameCount >= pssExecCtrl.stFrame) {
                pssMpegPlayStart();
            } else if (pssExecCtrl.status == PSS_STAT_PRELOAD && sys_mpeg.frameCount >= pssExecCtrl.stFrame) {
                pssExecCtrl.status = PSS_STAT_READY;
            }
        }
    }

    if ((pssExecCtrl.writerest < 5 && videoDecGetState(vd) == 0) || videoDecGetState(vd) == 1 ||
        videoDecGetState(vd) == 3) {
        return -1;
    }
    return 1;
}

static void pssMpegFinish(VideoDec *vd) {
    if (isWithAudio) {
        audioDecReset(&audioDec);
    }
    while (!videoDecFlush(vd)) {
        switchThread();
    }
    spkResetOT();
    fontClear();
    endDisplay();
}

static void pssMpegPlayStart(void) {
    videoDec.hid_vblank = AddIntcHandler(INTC_VBLANK_S, vblankHandler, -1);
    intc_vs_changed = EnableIntc(INTC_VBLANK_S);

    *GS_CSR |= 2;
    sceGsPutIMR(sceGsGetIMR() & ~0x200);
    videoDec.hid_endimage = AddIntcHandler(INTC_GS, handler_endimage, -1);
    intc_gs_changed = EnableIntc(INTC_GS);

    startDisplay(1);
    if (isWithAudio) {
        audioDecStart(&audioDec);
    }
    pssExecCtrl.isStarted = 1;
    pssExecCtrl.status = PSS_STAT_PLAY;
}

/** Lets the other threads at the player's priority run (rotates the ready queue at 0x24). */
void switchThread(void) {
    RotateThreadReadyQueue(0x24);
}

static int isAudioOK(void) {
    return isWithAudio ? audioDecIsPreset(&audioDec) : 1;
}

/*
 * Matches once linked: `&_gp` (0x3CFC70, an absolute linker symbol) is left
 * unrelocated in the original ELF, so diff_unit.py compares the zero HI16/LO16
 * fields of our object against the resolved lui/addiu and reports 4 words.
 */
static int initAll(union fsFileIndex *id) {
    int i;
    int result;
    struct ThreadParam th_param;

    readBufCreate(PCW->readBufp);
    sceMpegInit();
    videoDecCreate(&videoDec, PCW->mpegWorkp, 0xD8800, PCW->viBufDatap, viBufTag, 0x100, PCW->timeStamp, 0x200);
    audioDecCreate(&audioDec, PCW->audioBuff, 0xC000, 0x6000);

    videoDecSetStream(&videoDec, MPEG_STREAM_M2V, 0, (MpegCallback)videoCallback, PCW->readBufp);
    if (isWithAudio) {
        videoDecSetStream(&videoDec, MPEG_STREAM_PCM, 0, (MpegCallback)pcmCallback, PCW->readBufp);
    }

    for (i = 0; i < 5; i++) {
        voBufTagInter[i].v = PCW->tagInterDatap + i * 0x19100;
    }
    voBufCreate(&voBuf, UNCACHED(PCW->voBufDatap), NULL, voBufTagInter, 0, 0, 5);

    th_param.entry = defMain;
    th_param.stack = PCW->defStack;
    th_param.stackSize = sizeof(PCW->defStack);
    th_param.initPriority = 0x24;
    th_param.gpReg = &_gp;
    th_param.option = 0;
    PCW->defaultTh = CreateThread(&th_param);
    StartThread(PCW->defaultTh, NULL);

    th_param.entry = videoDecMain;
    th_param.stack = PCW->videoDecStack;
    th_param.stackSize = sizeof(PCW->videoDecStack);
    th_param.initPriority = 0x24;
    th_param.gpReg = &_gp;
    th_param.option = 0;
    PCW->videoDecTh = CreateThread(&th_param);
    StartThread(PCW->videoDecTh, &videoDec);

    switchThread();

    while (!(result = strFileOpen(id))) {
        printf("Can't open file %s\n", id->index.name);
    }
    return 0;
}

static void termAll(void) {
    int tmp;

    readBufDelete(PCW->readBufp);
    voBufDelete(&voBuf);

    WaitSemaPss();
    TerminateThread(PCW->videoDecTh);
    DeleteThread(PCW->videoDecTh);
    SignalSemaPss();

    TerminateThread(PCW->defaultTh);
    DeleteThread(PCW->defaultTh);

    DisableIntc(INTC_GS);
    tmp = RemoveIntcHandler(INTC_GS, videoDec.hid_endimage);
    if (!intc_gs_changed && tmp) {
        EnableIntc(INTC_GS);
    }
    DisableIntc(INTC_VBLANK_S);
    tmp = RemoveIntcHandler(INTC_VBLANK_S, videoDec.hid_vblank);
    if (!intc_vs_changed && tmp) {
        EnableIntc(INTC_VBLANK_S);
    }

    videoDecDelete(&videoDec);
    audioDecDelete(&audioDec);
    strFileClose();
}

static void defMain(void) {
    while (1) {
        switchThread();
    }
}

/** Returns the player status (PSS_STAT_*). */
int pssGetPssStatus(void) {
    return pssExecCtrl.status;
}

/** Returns whether the movie was cancelled. */
int pssGetPssAbortFlag(void) {
    return pssExecCtrl.movieabort;
}

/** Returns the mask switch. */
int pssGetMaskSwitch(void) {
    return pssExecCtrl.maskon;
}

/** Sets the mask switch to `flg`. */
void pssSetMaskSwitch(int flg) {
    pssExecCtrl.maskon = flg;
}

/**
 * While a movie plays, reads the pad and flags the movie as cancelled when the skip key is pressed.
 */
void pssCheckMovieCancel(void) {
    if (pssExecCtrl.status == PSS_STAT_PLAY) {
        shPadSet();
        if (shPadTrigger(0, key_config.skip)) {
            pssExecCtrl.movieabort = 1;
            pssSetControlData(6, NULL, 0);
        }
    }
}

/**
 * Sets the subtitles: messages `msg_bufp` shown at the times in `adr_msg_time`, from message
 * `msg_start` on.
 */
void pssSetSubTitleData(unsigned short *msg_bufp, void *adr_msg_time, int msg_start) {
    pssSubTitleCtrl.msg_start = msg_start;
    pssSubTitleCtrl.msg_no = 0;
    pssSubTitleCtrl.msg_bufp = msg_bufp;
    pssSubTitleCtrl.adr_msg_time = adr_msg_time;
}

/** Prints `message` as an error. */
void ErrMessage(char *message) {
    printf("[ Error ] %s\n", message);
}

/** Moves buffered audio on to the IOP (audioDecSendToIOP()). */
void proceedAudio(void) {
    audioDecSendToIOP(&audioDec);
}
