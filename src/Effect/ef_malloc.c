/* ef_malloc.c: the effect heap, a thin wrapper around utilheap. */
#include "sh2.h"
#include "libc/string.h"

/* Matching: #line keeps the original line numbers (declarations moved to headers). */
#line 6
/* printf prefixed with "<file>:<line>> "; the line is baked into the string. */
#define EF_PRINT(msg) printf(__FILE__ ":" SH_STRINGIFY(__LINE__) "> " msg)

static struct utilHeapCtrl *EfctCtrl;

/**
 * Sets up the effect heap in size bytes at buf.
 * @return the heap's control block, which utilHeapInit puts at the start of buf
 */
void *EfctInitHeap(void *buf, unsigned int size) {
    EfctCtrl = utilHeapInit(buf, size);
    if (buf != EfctCtrl) {
        EF_PRINT("address mismatch.\n");
    }
    return EfctCtrl;
}

/** Allocates n zero-filled bytes of effect heap; NULL before EfctInitHeap or when it is full. */
void *EfctMalloc(unsigned int n) {
    void *p;

    if (EfctCtrl == NULL) {
        return NULL;
    }

    p = utilHeapMalloc(EfctCtrl, n);

    /* Out of effect heap: report it and hand back NULL. */

    if (p == NULL) {
        EF_PRINT("can't allocated.\n");
    } else {
        memset(p, 0, n);
    }
    return p;
}

/** Frees a block allocated with EfctMalloc. */
void EfctFree(void *p) {
    utilHeapFree(p);
}
