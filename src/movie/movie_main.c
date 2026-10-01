#include "sh2.h"

/*
 * movie_main.c: the game's interface to the PSS movie player: picks the
 * memory for a movie, starts it, and runs the player loop.
 */

static void movieSubPreSet(union fsFileIndex *id, void *wk);

int movieLastExitStatus;
int movieExecDummy;
union fsFileIndex *movieLastFile = NULL;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 24
/* Background slot whose memory an event movie borrows. */
static int MovieSelectSlot(union fsFileIndex *id) {
    int slot;

    if (id == data_movie_deai_pss) {
        slot = loadBg2x2_GetSlotOutdoor(1, 4);
    } else if (id == data_movie_end_dog_pss) {
        slot = 0;
    } else if (id == data_movie_ending_pss) {
        slot = 0;
    } else if (id == data_movie_gero_pss) {
        slot = loadBg2x2_GetFreeSlotIndoor(30);
    } else if (id == data_movie_hakaba_pss) {
        slot = loadBg2x2_GetSlotOutdoor(0, -1);
    } else if (id == data_movie_hei_pss) {
        slot = loadBg2x2_GetSlotOutdoor(0, 1);
    } else if (id == data_movie_knife_pss) {
        slot = loadBg2x2_GetFreeSlotIndoor(36);
    } else if (id == data_movie_korosu_a_pss) {
        slot = loadBg2x2_GetFreeSlotIndoor(108);
    } else if (id == data_movie_korosu_b_pss) {
        slot = loadBg2x2_GetFreeSlotIndoor(108);
    } else if (id == data_movie_murder_pss) {
        slot = loadBg2x2_GetFreeSlotIndoor(162);
    } else if (id == data_movie_open_bgm_pss) {
        slot = -1;
    } else if (id == data_movie_open_voc_pss) {
        slot = -1;
    } else if (id == data_movie_rouya_pss) {
        slot = loadBg2x2_GetFreeSlotIndoor(137);
    } else if (id == data_movie_saikai_pss) {
        slot = loadBg2x2_GetFreeSlotIndoor(89);
    } else if (id == data_movie_toilet_pss) {
        slot = loadBg2x2_GetFreeSlotIndoor(1);
    } else {
        slot = -1;
    }
    assert_dw(0<=slot&&slot<8);
    return slot;
}

/**
 * Prepares event movie `id`: borrows the background slot memory it needs and sets the movie up for
 * playing.
 */
void MoviePreSet(union fsFileIndex *id) {
    int slot;

    slot = MovieSelectSlot(id);
    MemShareAllocateEventMovie(slot);
    movieSubPreSet(id, NULL);
}

/** Prepares title movie `id`, which takes over the shared memory. */
void MovieTitlePreSet(union fsFileIndex *id) {
    MemShareAllocateTitleMovie();
    movieSubPreSet(id, NULL);
}

/** Cold init of the movie player; clears the last exit status. */
void MovieInit(void) {
    pssSystemColdInit();
    movieLastExitStatus = 0;
    movieExecDummy = 0;
}

/** Plays the preset movie to the end; -1 if the player cancelled it. */
int MovieMain(void) {
    int result;
    int ret;

    ret = 0;
    if (!movieExecDummy) {
        pssInit();
        pssSetMaskSwitch(0);
        do {
            pssCheckMovieCancel();
            pssSetControlData(2, NULL, 0);
            if (movieLastFile != data_movie_gero_pss) {
                SeBgmManager();
            }
            result = pssMain();
        } while (result);
        do { /* probably a macro in the original; the do/while(0) adds a nop at the join */
            if (pssGetPssAbortFlag() == 1) {
                ret = -1;
            } else {
                ret = 0;
            }
        } while (0);
        movieLastExitStatus = ret;
    }
    shSetDF(2);
    return ret;
}

/** Returns whether the prepared movie is ready to play (always 1 in this build). */
int MovieWaitReady(void) {
    return movieExecDummy ? 1 : 1; /* both cases are 1 in this build */
}

/** Starts playing the prepared movie. */
void MoviePlayFromReady(void) {
    pssSetControlData(2, NULL, 0);
}

/** Returns whether the movie player is idle. */
int MovieCheckSleep(void) {
    return pssGetPssStatus() == 0;
}

/** The opening movie, as a step machine on Sh2sys.step[2]. */
int MoviePlayOPMovie(void) {
    int result;

    result = 1;
    switch (Sh2sys.step[2]) {
    case 0:
        MemShareWaitRealloc(0);
        MovieTitlePreSet(data_movie_open_voc_pss);
        Sh2sys.step[2]++;
        Sh2sys.step[3] = 0;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        break;
    case 1:
        if (MovieWaitReady()) {
            Sh2sys.step[2]++;
            Sh2sys.step[3] = 0;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
        }
        break;
    case 2:
        if (movieExecDummy) {
            result = 0;
        } else {
            pssInit();
            pssSetMaskSwitch(1);
            pssSetControlData(2, NULL, 0);
            movieSetSubTitleData(NULL, NULL, 0);
            do {
                pssCheckMovieCancel();
                pssSetControlData(2, NULL, 0);
            } while (pssMain());
            if (pssGetPssAbortFlag() == 1) {
                result = -1;
            } else {
                result = 0;
            }
            pssExit();
        }
        break;
    }
    movieLastExitStatus = result;
    return result;
}

/* Hands the player its work areas (from wk, or from the shared memory) and starts reading id. */
static void movieSubPreSet(union fsFileIndex *id, void *wk) {
    static u_long128 work[5040];

    if (wk) {
        unsigned char *ppp;
        int p0;
        int p1;
        int p2;
        int p3;
        int p4;

        ppp = wk;
        wk = work;
        p0 = 0x13B80;
        p1 = p0 + 0xD8880;
        p2 = p1 + 0x50080;
        p3 = p2 + 0x80080;
        p4 = p3 + 0x1E0080;
        pssSetWorkAddress(wk, ppp + p0, ppp + p1, ppp + p2, ppp + p3, ppp + p4);
    } else {
        void *p0;
        void *p1;
        void *p2;
        void *p3;
        void *p4;

        wk = work;
        p0 = MemShareGetMovieMpegWorkAddr();
        p1 = MemShareGetMovieReadWorkAddr();
        p2 = MemShareGetMovieViWorkAddr();
        p3 = MemShareGetMovieVoWorkAddr();
        p4 = MemShareGetMovieTagWorkAddr();
        movieExecDummy = wk == NULL;
        pssSetWorkAddress(wk, p0, p1, p2, p3, p4);
    }
    movieLastFile = id;
    pssSetControlData(1, id, 0);
    {
        int fid;

        while ((fid = FcRead(movieLastFile, NULL)) == -1) {
        }
    }
}

/** Returns 0 when the last movie ended with status -1 (cancelled), 1 otherwise. */
int movieGetLastExitStatus(void) {
    return !(movieLastExitStatus == -1);
}

/**
 * Sets the subtitles of the next movie: messages `msg_bufp` shown at the times in `adr_msg_time`,
 * from message `msg_start` on.
 */
void movieSetSubTitleData(unsigned short *msg_bufp, void *adr_msg_time, int msg_start) {
    pssSetSubTitleData(msg_bufp, adr_msg_time, msg_start);
}
