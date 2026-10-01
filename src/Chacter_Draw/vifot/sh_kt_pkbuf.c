/*
 * Double-buffered DMA packet buffers (Chacter_Draw/vifot): a ktPkBuf holds two buffers and
 * flips between them each frame.
 */
#include "sh2.h"

/**
 * Sets up a packet double buffer on page 0.
 * @param p    the buffer pair
 * @param buf0 first buffer
 * @param buf1 second buffer
 * @param size size of each buffer in bytes
 */
void ktPkBufInit(struct ktPkBuf *p, void *buf0, void *buf1, int size) {
    p->page = 0;
    p->current = buf0;
    p->buffers[0] = buf0;
    p->buffers[1] = buf1;
    p->buffer_size = size;
}

/** Flips p to its other buffer and returns it. */
void *ktPkBufNext(struct ktPkBuf *p) {
    p->page ^= 1;
    p->current = p->buffers[p->page];
    return p->current;
}

/** Returns p's current buffer. */
void *ktPkBufCurrent(struct ktPkBuf *p) {
    return p->current;
}
