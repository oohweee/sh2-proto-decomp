/*
 * The VIF0 packet double buffer (Chacter_Draw/vifot).
 */
#include "sh2.h"

/* DMA packet buffers, cache-line aligned. Matching: bss statics are laid out in reverse order. */
static u_long128 vif0_pk_buf_db[2][256] __attribute__((aligned(128)));
static struct ktPkBuf vif0_pkbuf;

/**
 * Sets up the VIF0 packet double buffer.
 * @param buf0 first buffer
 * @param buf1 second buffer
 * @param size size of each buffer in bytes
 */
void ktVif0PkBufInit(void *buf0, void *buf1, int size) {
    ktPkBufInit(&vif0_pkbuf, buf0, buf1, size);
}

/** Sets up the VIF0 packet double buffer on the built-in buffers. */
void sh2_ktVif0PkBufInit(void) {
    ktVif0PkBufInit(vif0_pk_buf_db[0], vif0_pk_buf_db[1], sizeof(vif0_pk_buf_db[0]));
}

/** Flips the VIF0 packet buffer and returns the new current one. */
void *ktVif0PkBufNext(void) {
    return ktPkBufNext(&vif0_pkbuf);
}

/** Returns the current VIF0 packet buffer. */
void *ktVif0PkBufCurrent(void) {
    return ktPkBufCurrent(&vif0_pkbuf);
}
