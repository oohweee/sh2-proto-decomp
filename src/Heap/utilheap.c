/*
 * utilheap.c: a simple first-fit heap inside a caller-supplied buffer.
 *
 * Layout: a utilHeapCtrl header, then a doubly linked list of blocks, each
 * a 16-byte utilHeapMBlock header followed by its data. A block is free when
 * its heapctrl is NULL; used blocks point back at their heap. Sizes are
 * multiples of 16 and don't include the block header.
 *
 * Matching: the two error messages bake in lines 191 and 201 (blank lines keep them there).
 */

#include "sh2.h"

/* printf prefixed with "<file>:<line>> "; the line is baked into the string. */
#define UH_PRINT(msg) printf(__FILE__ ":" SH_STRINGIFY(__LINE__) "> " msg)

/* Rounds up to a multiple of 16 (one quadword). */
#define UTILHEAP_ALIGN(n) (((int)(n) + 15) & ~15)

/* Lowest valid heap address (the start of EE main RAM user space). */
#define UTILHEAP_MIN_ADDR 0x100000

/** Sets up a heap in @p buf (@p bytesize bytes); returns NULL if buf is invalid or too small. */
struct utilHeapCtrl *utilHeapInit(void *buf, unsigned int bytesize) {
    struct utilHeapCtrl *heapctrl;
    struct utilHeapMBlock *mblock;
    u_long128 *bufhead;
    u_long128 *buftail;
    unsigned int size;
    unsigned int free;

    if ((unsigned int)buf < UTILHEAP_MIN_ADDR) {
        return NULL;
    }
    bufhead = (u_long128 *)(((unsigned int)buf + 15) & ~15);
    buftail = (u_long128 *)(((unsigned int)buf + bytesize) & ~15);
    size = (unsigned int)buftail - (unsigned int)bufhead;
    if (size < 0x100) {
        return NULL;
    }
    heapctrl = (struct utilHeapCtrl *)bufhead;
    heapctrl->size = size;
    free = size - 0x20;
    heapctrl->free = free;
    mblock = (struct utilHeapMBlock *)(heapctrl + 1);
    mblock->size = free;
    mblock->heapctrl = NULL;
    heapctrl->head = mblock;
    heapctrl->tail = mblock;
    mblock->prev = NULL;
    mblock->next = NULL;
    return heapctrl;
}

/** Allocates @p n bytes (rounded up to 16) from @p heapctrl, first fit; splits the block when
 *  the rest can hold another block. Returns NULL if nothing fits. */
void *utilHeapMalloc(struct utilHeapCtrl *heapctrl, unsigned int n) {
    struct utilHeapMBlock *mblock;
    unsigned int size;
    unsigned int postsize;
    struct utilHeapMBlock *postblock;
    struct utilHeapMBlock *next;

    n = UTILHEAP_ALIGN(n);
    if (n == 0) {
        return NULL;
    }
    if (heapctrl->size < n) {
        return NULL;
    }
    for (mblock = heapctrl->head; mblock != NULL; mblock = mblock->next) {
        if (mblock->heapctrl != NULL) {
            continue;
        }
        size = mblock->size;
        if (size < n) {
            continue;
        }
        heapctrl->free -= n;
        if (size != n) {
            postblock = (struct utilHeapMBlock *)((char *)(mblock + 1) + n);
            postsize = size - n - 16;
            postblock->size = postsize;
            postblock->heapctrl = NULL;
            mblock->size = n;
            heapctrl->free -= 16;
            next = mblock->next;
            postblock->next = next;
            postblock->prev = mblock;
            mblock->next = postblock;
            if (next != NULL) {
                next->prev = postblock;
            } else {
                heapctrl->tail = postblock;
            }
        }
        mblock->heapctrl = heapctrl;
        return mblock + 1;
    }
    return NULL;
}

/** Frees @p obj and merges it with free neighbours. */
void utilHeapFree(void *obj) {
    struct utilHeapMBlock *mblock;
    struct utilHeapCtrl *heapctrl;

    if (obj == NULL) {
        return;
    }
    if ((unsigned int)obj & 0xF) {
        /* obj has to be a pointer returned by utilHeapMalloc (16-byte aligned). */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 191
        UH_PRINT("illegal address error in utilHeapFree().\n");
        return;
    }
    mblock = (struct utilHeapMBlock *)obj - 1;
    heapctrl = mblock->heapctrl;
    if ((unsigned int)heapctrl < UTILHEAP_MIN_ADDR) {

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 201
        UH_PRINT("illegal address error in utilHeapFree().\n");
        return;
    }
    mblock->heapctrl = NULL;
    heapctrl->free += mblock->size;
    {
        struct utilHeapMBlock *postblock;
        unsigned int combinesize;
        struct utilHeapMBlock *next;

        postblock = mblock->next;
        if (postblock != NULL && postblock->heapctrl == NULL) {
            combinesize = mblock->size;
            combinesize += postblock->size + 16;
            mblock->size = combinesize;
            heapctrl->free += 16;
            next = postblock->next;
            mblock->next = next;
            if (next != NULL) {
                next->prev = mblock;
            } else {
                heapctrl->tail = mblock;
            }
        }
    }
    {
        struct utilHeapMBlock *postblock;
        struct utilHeapMBlock *preblock;
        unsigned int combinesize;
        struct utilHeapMBlock *next;

        preblock = mblock->prev;
        if (preblock != NULL && preblock->heapctrl == NULL) {
            postblock = mblock->next; /* Matching: dead; the original's DWARF has postblock here too */
            combinesize = preblock->size;
            combinesize += mblock->size + 16;
            preblock->size = combinesize;
            heapctrl->free += 16;
            next = mblock->next;
            preblock->next = next;
            if (next != NULL) {
                next->prev = preblock;
            } else {
                /* @bug should be preblock */
                heapctrl->tail = mblock;
            }
        }
    }
}
