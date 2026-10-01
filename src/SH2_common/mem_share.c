/*
 * mem_share.c: one big buffer (MemShare_gp_data_buf) shared between the
 * movie player and the game (effects, background characters, background
 * loading). A movie takes the memory over; afterwards the game's areas are
 * laid out again and the texture/packet buffers reallocated (through the
 * loader thread, lisPutCmd0).
 *
 * step: -1 not set up yet, 0 game, 1 movie, 2 movie released (reallocation
 * requested), 3 game areas laid out again.
 */

#include "sh2.h"

char MemShare_gp_data_buf[0xC92000];

static struct MemShareCtrl mem_share_ctrl = {-1, -1};

static int ___MemShareRemove(void) {
    HH_MemoryManager_MemoryBlock_All_Discard();
    return 1;
}

static int ___MemShareLoadInit(void) {
    HH_MemoryManager_MemoryBlock_Allocate_TextureBuffer();
    return 1;
}

static int ___MemShareLoadFinish(void) {
    HH_MemoryManager_MemoryBlock_Allocate_Packet_and_ObjectWork();
    return 1;
}

/* notall: bit 0 keeps the effect/chara/cache areas, bit 1 keeps the background load areas. */
static void MemShareClearWorkAddr(int notall) {
    int i;

    mem_share_ctrl.movie_tag = NULL;
    mem_share_ctrl.movie_vo = NULL;
    mem_share_ctrl.movie_vi = NULL;
    mem_share_ctrl.movie_mpeg = NULL;
    mem_share_ctrl.movie_read = NULL;
    if (!(notall & 1)) {
        mem_share_ctrl._effect = NULL;
        mem_share_ctrl._effect2 = NULL;
        mem_share_ctrl._bg_chara = NULL;
        mem_share_ctrl._bg_c_cache = NULL;
        mem_share_ctrl._bg_l_cache = NULL;
        mem_share_ctrl.effect = NULL;
        mem_share_ctrl.effect2 = NULL;
        mem_share_ctrl.bg_chara = NULL;
        mem_share_ctrl.bg_c_cache = NULL;
        mem_share_ctrl.bg_l_cache = NULL;
    }
    if (!(notall & 2)) {
        mem_share_ctrl._bg_loadall = NULL;
        for (i = 0; i < 8; i++) {
            mem_share_ctrl._bg_load[i] = NULL;
        }
        mem_share_ctrl.bg_loadall = NULL;
        for (i = 0; i < 8; i++) {
            mem_share_ctrl.bg_load[i] = NULL;
        }
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 160
/** The title movie takes everything from 0x150000 on. */
void MemShareAllocateTitleMovie(void) {
    char *addr;

    assert_dw(mem_share_ctrl.step!=-1);
    if (mem_share_ctrl.step == 2 || mem_share_ctrl.step == 3) {
        MemShareWaitRealloc(0);
    }
    if (mem_share_ctrl.step == 0) {
        mem_share_ctrl.step = 1;
        fsSync(0, -1);
        ___MemShareRemove();
        MemShareClearWorkAddr(0);
        addr = MemShare_gp_data_buf + 0x150000;
        mem_share_ctrl.movie_tag = addr;
        mem_share_ctrl.movie_vo = addr + 0x1F5400;
        mem_share_ctrl.movie_vi = addr + 0x3D5400;
        mem_share_ctrl.movie_mpeg = addr + 0x455400;
        mem_share_ctrl.movie_read = addr + 0x52DC00;
    }
}



/** An event movie keeps the background load areas except the slot pair it borrows for its output. */
void MemShareAllocateEventMovie(int slot) {
    char *addr;

    assert_dw(mem_share_ctrl.step!=-1);
    if (mem_share_ctrl.step == 0) {
        mem_share_ctrl.step = 1;
        fsSync(0, -1);
        ___MemShareRemove();
        MemShareClearWorkAddr(2);
        addr = MemShare_gp_data_buf + 0x50000;
        mem_share_ctrl.movie_tag = addr;
        mem_share_ctrl.movie_vi = addr + 0x1F5400;
        mem_share_ctrl.movie_mpeg = addr + 0x275400;
        mem_share_ctrl.movie_read = addr + 0x34DC00;
        addr = MemShare_gp_data_buf + 0x512000;
        addr += slot / 2 * 0x1E0000;
        mem_share_ctrl.movie_vo = addr;
        slot /= 2;
        mem_share_ctrl._bg_load[slot * 2] = NULL;
        mem_share_ctrl.bg_load[slot * 2] = NULL;
        mem_share_ctrl._bg_load[slot * 2 + 1] = NULL;
        mem_share_ctrl.bg_load[slot * 2 + 1] = NULL;
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 228
/* Lays out the game's areas. */
static void ___MemShareReleaseMovie(int notall) {
    char *addr;
    int i;

    MemShareClearWorkAddr(notall);
    addr = MemShare_gp_data_buf + 0x150000;
    mem_share_ctrl._effect = addr;
    mem_share_ctrl._effect2 = addr + 0x90000;
    mem_share_ctrl._bg_chara = addr + 0x282000;
    mem_share_ctrl._bg_l_cache = addr + 0x342000;
    addr = MemShare_gp_data_buf + 0x512000;
    mem_share_ctrl._bg_loadall = addr;
    for (i = 0; i < 8; i += 2) {
        mem_share_ctrl._bg_load[i] = addr;
        mem_share_ctrl._bg_load[i + 1] = addr;
        addr += 0x1E0000;
    }
}

/**
 * Ends a movie's use of the memory: lays the game areas out again and queues the texture buffer
 * reallocation on the load-init thread.
 */
void MemShareReleaseMovie(void) {
    int cid;

    assert_dw(mem_share_ctrl.step!=-1);
    if (mem_share_ctrl.step == 1) {
        fsSync(0, -1);
        ___MemShareReleaseMovie(3);
        while ((cid = ((int (*)(int (*)(void)))lisPutCmd0)(___MemShareLoadInit)) == -1) {
            /* Matching: lisPutCmd0 is called through a cast (the original gets `jalr`). */
        }
        mem_share_ctrl.cid = cid;
        mem_share_ctrl.step = 2;
    }
}

/** Releases the movie memory if still needed and restores the background load areas. */
void MemShareRecoverMovie(void) {
    assert_dw(mem_share_ctrl.step!=-1);
    if (mem_share_ctrl.step == 1) {
        MemShareReleaseMovie();
    }
    if (mem_share_ctrl.step == 2) {
        ___MemShareReleaseMovie(2);
        mem_share_ctrl.step = 3;
    }
}

/** Finishes a pending reallocation (mode as for lisSync: 0 waits). */
void MemShareWaitRealloc(int mode) {
    int cid;

    if (mem_share_ctrl.step == -1) {
        ___MemShareReleaseMovie(0);
        mem_share_ctrl.step = 0;
    }
    if (mem_share_ctrl.step == 1) {
        MemShareReleaseMovie();
    }
    if (mem_share_ctrl.cid != -1) {
        cid = lisSync(mode, mem_share_ctrl.cid);
        if (cid < 0) {
            return;
        }
    }
    if (mem_share_ctrl.step == 2) {
        MemShareRecoverMovie();
    }
    if (mem_share_ctrl.step == 3) {
        ___MemShareLoadFinish();
        mem_share_ctrl.cid = -1;
    }
    mem_share_ctrl.step = 0;
}

/**
 * Returns the background load work address the caller had, and makes the current one the next
 * answer (the getters below do the same).
 */
/* The getters publish the new address (the _ fields) and return the one the caller used so far. */
void *MemShareGetBgLoadWorkAddr(void) {
    void *addr;

    addr = mem_share_ctrl.bg_loadall;
    if (addr != mem_share_ctrl._bg_loadall) {
        mem_share_ctrl.bg_loadall = mem_share_ctrl._bg_loadall;
    }
    return addr;
}

/**
 * Returns the address of the background load section work of slot `slot` the caller had, and
 * publishes the current one.
 */
void *MemShareGetBgLoadSectionWorkAddr(int slot) {
    void *addr;

    addr = mem_share_ctrl.bg_load[slot];
    if (addr != mem_share_ctrl._bg_load[slot]) {
        mem_share_ctrl.bg_load[slot] = mem_share_ctrl._bg_load[slot];
    }
    return addr;
}

/**
 * Returns the address of the background load cache the caller had, and publishes the current one.
 */
void *MemShareGetBgLoadCacheAddr(void) {
    void *addr;

    addr = mem_share_ctrl.bg_l_cache;
    if (addr != mem_share_ctrl._bg_l_cache) {
        mem_share_ctrl.bg_l_cache = mem_share_ctrl._bg_l_cache;
    }
    return addr;
}

/**
 * Returns the address of the background character work the caller had, and publishes the current
 * one.
 */
void *MemShareGetBgCharaWorkAddr(void) {
    void *addr;

    addr = mem_share_ctrl.bg_chara;
    if (addr != mem_share_ctrl._bg_chara) {
        mem_share_ctrl.bg_chara = mem_share_ctrl._bg_chara;
    }
    return addr;
}

/** Returns the address of the effect 2 work the caller had, and publishes the current one. */
void *MemShareGetEffect2WorkAddr(void) {
    void *addr;

    addr = mem_share_ctrl.effect2;
    if (addr != mem_share_ctrl._effect2) {
        mem_share_ctrl.effect2 = mem_share_ctrl._effect2;
    }
    return addr;
}

/** Returns the movie tag (packet) work area. */
void *MemShareGetMovieTagWorkAddr(void) {
    return mem_share_ctrl.movie_tag;
}

/** Returns the movie video output buffer (pss_vobuf.c). */
void *MemShareGetMovieVoWorkAddr(void) {
    return mem_share_ctrl.movie_vo;
}

/** Returns the movie video input buffer (pss_vibuf.c). */
void *MemShareGetMovieViWorkAddr(void) {
    return mem_share_ctrl.movie_vi;
}

/** Returns the movie MPEG decoder work area. */
void *MemShareGetMovieMpegWorkAddr(void) {
    return mem_share_ctrl.movie_mpeg;
}

/** Returns the movie stream read buffer. */
void *MemShareGetMovieReadWorkAddr(void) {
    return mem_share_ctrl.movie_read;
}
