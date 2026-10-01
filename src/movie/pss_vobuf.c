#include "sh2.h"
#include "sdk/eekernel.h"

/*
 * pss_vobuf.c: the ring buffer of decoded video frames (the voBuf*
 * functions): `data` holds the frames, `tag` the per-frame status.
 */


/**
 * Sets up frame buffer `f` with `size` frames at `data` and their tags `tagInter`. `tag`, `arg4`
 * and `arg5` are unused.
 */
/*
 * Seven parameters: the DWARF lists only the used ones (tagInter in a3, size in
 * t2); the prototype comes from config/prototype_overrides.txt.
 */
void voBufCreate(VoBuf *f, VoBufData *data, VoBufTag *tag, VoBufTag *tagInter, int arg4, int arg5, int size) {
    int i;

    f->data = data;
    f->tagInter = tagInter;
    f->tag = tagInter;
    f->size = size;
    f->count = 0;
    f->write = 0;
    for (i = 0; i < size; i++) {
        f->tagInter[i].status = 0;
    }
}

/** Does nothing (the buffer argument is unused). */
void voBufDelete(VoBuf *f) {
}

/** Empties the frame buffer `f`. */
void voBufReset(VoBuf *f) {
    f->count = 0;
    f->write = 0;
}

/** Returns whether every frame of `f` is decoded and waiting to be shown. */
int voBufIsFull(VoBuf *f) {
    return f->count == f->size;
}

/** Marks the frame at the write position as decoded (status 2) and advances. */
void voBufIncCount(VoBuf *f) {
    DIntr();
    f->tag[f->write].status = 2;
    f->count++;
    f->write = (f->write + 1) % f->size;
    EIntr();
}

/** The frame to decode into next, or NULL when the buffer is full. */
VoBufData *voBufGetData(VoBuf *f) {
    if (voBufIsFull(f)) {
        return NULL;
    }
    return f->data + f->write;
}

/** Returns whether `f` has no decoded frame. */
int voBufIsEmpty(VoBuf *f) {
    return f->count == 0;
}

/** The oldest decoded frame's tag, or NULL when the buffer is empty. */
VoBufTag *voBufGetTag(VoBuf *f) {
    if (voBufIsEmpty(f)) {
        return NULL;
    }
    return f->tag + (f->write - f->count + f->size) % f->size;
}

/** Releases the oldest decoded frame of `f` (after it was shown). */
/* Matching: the volatile cast. count is updated from the decode thread and the original
 * reloads it after the test (a volatile field; the DWARF drops volatile). */
void voBufDecCount(VoBuf *f) {
    if (f->count > 0) {
        ((volatile VoBuf *)f)->count--;
    }
}
