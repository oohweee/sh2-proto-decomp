/*
 * sh2_ch_malloc.c: the character animation heap (ASC data). Blocks are laid
 * out so that the data after utilheap's 16-byte block header is 64-byte
 * aligned: the heap starts 0x20 into a 64-byte line and every block size is
 * 0x30 mod 0x40.
 *
 * Matching: the two messages with a "<file>:<line>" prefix bake in lines
 * 52 and 90, so those CH_PRINT calls have to stay on those lines (blank padding).
 */

#include "sh2.h"
#include "libc/string.h"

/* printf prefixed with "<file>:<line>> "; the line is baked into the string. */
#define CH_PRINT(msg) printf(__FILE__ ":" SH_STRINGIFY(__LINE__) "> " msg)

static u_long128 Anim_Skl_Cls_Data[32772];
static struct utilHeapCtrl *shCh_ASCDAT_Ctrl;

/** Sets up the ASC heap in @p bufhead (@p size bytes; hangs if not 16-byte aligned). */
struct utilHeapCtrl *shCh_ASC_InitHead(void *bufhead, int size) {
    unsigned int align;

    align = (unsigned int)bufhead & 0x3F;
    switch (align) {
    case 0x10:
        bufhead = (char *)bufhead + 0x10;
        break;
    case 0x20:
        break;
    case 0x30:
        bufhead = (char *)bufhead + 0x30;
        break;
    case 0:
        bufhead = (char *)bufhead + 0x20;
        break;
    default:
        /*
         * bufhead has to be at least 16-byte aligned.
         */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 52
        CH_PRINT("heap init error\n");
        while (1) {
        }
    }
    shCh_ASCDAT_Ctrl = utilHeapInit(bufhead, size);
    if (bufhead != shCh_ASCDAT_Ctrl) {
        printf("ASC-address mismatch.\n");
        while (1) {
        }
    }
    return shCh_ASCDAT_Ctrl;
}

/** Allocates @p n zero-filled bytes (rounded up, see above); hangs on failure. */
void *shCh_ASC_Malloc(unsigned int n) {
    void *p;

    n += 15;
    n &= ~15;
    switch (n & 0x3F) {
    case 0:
        n += 0x30;
        break;
    case 0x10:
        n += 0x20;
        break;
    case 0x20:
        n += 0x10;
        break;
    case 0x30:
        break;
    default:
        /* Can't happen after the rounding above. */

        /* Matching: blank lines keep the message below on its original source line. */



        CH_PRINT("halted!\n");
        while (1) {
        }
    }
    p = utilHeapMalloc(shCh_ASCDAT_Ctrl, n);
    if ((unsigned int)p & 0x3F) {
        printf("allocate buffer align error.\n");
        while (1) {
        }
    }
    if (p == NULL) {
        printf("ASC-can't allocated.\n");
        while (1) {
        }
    }
    memset(p, 0, n);
    return p;
}

/** Frees an ASC heap block. */
void shCh_ASC_Free(void *p) {
    utilHeapFree(p);
}

/** Sets up the ASC heap in its static buffer. */
void kari_ChAlloc_Init(void) {
    shCh_ASC_InitHead(Anim_Skl_Cls_Data, sizeof(Anim_Skl_Cls_Data));
}
