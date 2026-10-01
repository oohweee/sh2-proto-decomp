#include "sh2.h"

/*
 * pss_readbuf.c: the ring buffer between the stream reader and the demuxer
 * (the readBuf* functions). `put` is the write offset, `count` the number of
 * unread bytes.
 */



#define min(a, b) ((a) < (b) ? (a) : (b))

/** Empties read buffer `b`. */
void readBufCreate(ReadBuf *b) {
    b->count = 0;
    b->put = 0;
    b->size = sizeof(b->data);
}

/** Does nothing (the buffer argument is unused). */
void readBufDelete(ReadBuf *b) {
}

/** Where to write next; returns the free size. */
int readBufBeginPut(ReadBuf *b, unsigned char **ptr) {
    int size;

    size = b->size - b->count;
    if (size) {
        *ptr = b->data + b->put;
    }
    return size;
}

/** Commits `size` written bytes (at most the free size). */
int readBufEndPut(ReadBuf *b, int size) {
    int size_ok;

    size_ok = min(size, b->size - b->count);
    b->put = (b->put + size_ok) % b->size;
    b->count += size_ok;
    return size_ok;
}

/** Where to read next; returns the unread size. */
int readBufBeginGet(ReadBuf *b, unsigned char **ptr) {
    if (b->count < 0 || b->count > b->size) {
        printf("readBufBeginGet: b->count=%d\n", b->count);
        PssBreakPoint();
    }
    if (b->put < 0 || b->size <= b->put) {
        printf("readBufBeginGet: b->put=%d\n", b->put);
        PssBreakPoint();
    }
    if (b->count) {
        *ptr = b->data + (b->put - b->count + b->size) % b->size;
    }
    return b->count;
}

/** Releases `size` read bytes (at most the unread size). */
int readBufEndGet(ReadBuf *b, int size) {
    int size_ok;

    size_ok = min(size, b->count);
    b->count -= size_ok;
    return size_ok;
}
