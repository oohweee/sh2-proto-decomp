/*
 * The VIF1 packet double buffer (Chacter_Draw/vifot).
 */
#include "sh2.h"

/* DMA packet buffers, cache-line aligned. Matching: bss statics are laid out in reverse order. */
static u_long128 vif1_pk_buf_db[2][6144] __attribute__((aligned(128)));
static struct ktPkBuf vif1_pkbuf;

/**
 * Sets up the VIF1 packet double buffer.
 * @param buf0 first buffer
 * @param buf1 second buffer
 * @param size size of each buffer in bytes
 */
void ktVif1PkBufInit(void *buf0, void *buf1, int size) {
    ktPkBufInit(&vif1_pkbuf, buf0, buf1, size);
}

/** Sets up the VIF1 packet double buffer on the built-in buffers. */
void sh2_ktVif1kBufInit(void) {
    ktVif1PkBufInit(vif1_pk_buf_db[0], vif1_pk_buf_db[1], sizeof(vif1_pk_buf_db[0]));
}

/** Flips the VIF1 packet buffer and returns the new current one. */
void *ktVif1PkBufNext(void) {
    return ktPkBufNext(&vif1_pkbuf);
}
