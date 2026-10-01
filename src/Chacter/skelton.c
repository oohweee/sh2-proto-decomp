/*
 * Skeleton node pool (sh2skelton): 400 nodes on a free list, handed out as chains.
 */

#include "sh2.h"
#include "libc/string.h"

struct shSkeltonWork sh2skelton;

/** Clears the pool and links all 400 nodes into the free list. */
void shCharacterInitSkeltons(void) {
    struct shSkelton *stp;
    int i;

    memset(&sh2skelton, 0, sizeof(sh2skelton));
    sh2skelton.last = 400;
    stp = sh2skelton.work;
    sh2skelton.free = sh2skelton.work;
    for (i = 0; i < 399; i++, stp++) {
        stp->next = stp + 1;
    }
    stp->next = NULL;
}

/** Returns the chain starting at @p top to the free list. */
void shCharacterFreeSkeltons(struct shSkelton *top) {
    int i;
    struct shSkelton *free;

    i = 0;
    if (top != NULL) {
        free = sh2skelton.free;
        sh2skelton.free = top;
        while (top->next != NULL) {
            sh2skelton.last++;
            i++;
            top = top->next;
        }
        sh2skelton.last++;
        top->next = free;
        i++;
        printf("skeltons free %d rest %d\n", i, sh2skelton.last);
    }
}

/**
 * Takes a chain of @p n nodes from the free list and links each to its parent.
 * @param hrc per node, the index of its parent in the chain (0xFF: no parent).
 * @return the first node, or NULL if @p n is 0 or not enough nodes are free.
 */
struct shSkelton *shCharacterGetSkeletons(int n, unsigned char *hrc) {
    struct shSkelton *stp;
    struct shSkelton *top_stp;
    struct shSkelton *pre_stp;
    int i;
    struct shSkelton *parent;
    int j;

    if (sh2skelton.last < n || n == 0) {
        return NULL;
    }
    sh2skelton.last -= n;
    top_stp = stp = sh2skelton.free;
    pre_stp = NULL;
    for (i = 0; i < n; i++, pre_stp = stp, stp = stp->next, hrc++) {
        if (*hrc == 0xFF) {
            stp->parent = NULL;
        } else {
            parent = top_stp;
            for (j = *hrc; j > 0 && parent != NULL; j--) {
                parent = parent->next;
            }
            stp->parent = parent;
        }
    }
    printf("skeltons get %d rest %d\n", n, sh2skelton.last);
    sh2skelton.free = pre_stp->next;
    pre_stp->next = NULL;
    return top_stp;
}
