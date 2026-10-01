/*
 * spack.c: the fog/particle packet builder. Packets are written into a local
 * buffer (through the uncached mirror), linked into an ordering table by
 * depth (W) and turned into one DMA chain by spkDmaKick.
 */

#include "sh2.h"
#include "libc/stdio.h"
#include "libc/string.h"

typedef unsigned int u_int;

#define SPACK_ENV_MAX 32
#define SPACK_PBUF_SIZE 0x80000
#define SPACK_DMABUF_SIZE 4096


#define UNCACHED(p) ((u_int)(p) | 0x20000000)
#define PHYS(p) ((u_int)(p) & 0x0FFFFFFF)

static void *ot_link(void);
static void ot_qsort_asm(void);
static void ot_qsort_asm2(void);
static void ot_check(void);

struct SPACK_DATA spack;
static struct SPACK_LOCAL_BUF spack_local_buf __attribute__((aligned(64)));

/**
 * Clears the packet builder, sets up its packet, ordering table and DMA buffers, and returns it.
 */
struct SPACK_DATA *spkInit(void) {
    u_long128 ENVtag = 0;
    u_long128 *pENVtag;
    int i;
    int s;

    shQzero(&spack, sizeof(spack));
    spack.packet = spack_local_buf.packet;
    spack.dma_top = spack_local_buf.dmatag;
    spack.env_top = spack_local_buf.envdata;
    spack.ot_top = spack_local_buf.ot_top;
    spack.ot_last = spack_local_buf.ot_last;
    spack.ot_max = spack_local_buf.ot_max;
    spack.ot_size = 32;
    for (i = 0, s = 1; (u_int)s < 0x7FFFF && (u_int)i < 32; i++) {
        s <<= 1;
    }
    spack.ot_width = i;
    printf("spack work %#x(%dkb)\n", 0x80178, 0x201);
    ((struct SPACK_ENV_DATA *)&ENVtag)->DmaId = 0x1000;
    pENVtag = (u_long128 *)spack.env_top;
    for (i = 0; i < SPACK_ENV_MAX; i++) {
        *pENVtag = ENVtag;
        pENVtag++;
    }
    spkResetOT();
    return &spack;
}

/**
 * Starts a new frame: empties the ordering table and the DMA list and rewinds the packet buffer.
 */
void spkResetOT(void) {
    shQzero(spack.ot_top, (spack.ot_size + 3) * 4);
    spack.old_lastpos = spack.pk_last;
    spack.dmabuf_pos = spack.dma_top;
    spack.pk_last = spack.pos = (unsigned long *)UNCACHED(spack.packet);
    spack.w_mini = -1;
    spack.w_max = 0;
}

/** Empties the ordering table and the DMA list, keeping the packets built so far. */
void spkResetOT2(void) {
    shQzero(spack.ot_top, (spack.ot_size + 3) * 4);
    spack.dmabuf_pos = spack.dma_top;
    spack.w_mini = -1;
    spack.w_max = 0;
}

/**
 * Starts a packet at depth `w` using environment `envid`, opening it with the GIF tag `giftag` (two
 * 64-bit words).
 */
void spkOpenGiftag(void *giftag, unsigned int w, unsigned short envid) {
    unsigned long *pos;
    struct SPACK_OT_DATA *pd;
    unsigned long d[2];

    pos = spack.pk_last;
    pd = spack.dmabuf_pos;
    pd->Addr = (void *)PHYS(pos);
    pd->W = w;
    pd->EnvID = envid;
    pd->VifDirect = 0x50;
    memcpy(d, giftag, sizeof(d));
    spack.pgiftag = pos;
    pos[0] = d[0];
    pos[1] = d[1];
    spack.pos = pos + 2;
    spack.giftag_b = d[0];
}

/** spkOpenGiftag() with the GIF tag given as the two words `giftag1` and `giftag2`. */
void spkOpenDGiftag(unsigned long giftag1, unsigned long giftag2, unsigned int w, unsigned short envid) {
    unsigned long *pos;
    struct SPACK_OT_DATA *pd;

    pos = spack.pk_last;
    pd = spack.dmabuf_pos;
    pd->Addr = (void *)PHYS(pos);
    pd->W = w;
    pd->EnvID = envid;
    pd->VifDirect = 0x50;
    spack.pgiftag = pos;
    pos[0] = giftag1;
    pos[1] = giftag2;
    spack.pos = pos + 2;
    spack.giftag_b = giftag1;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 172
/** Closes the open GIF tag (sets its loop count from the data written) and ends the packet. */
void spkCloseGiftag(void) {
    unsigned int n;
    unsigned int n2;
    unsigned int nreg;
    struct SPACK_OT_DATA *pd;
    unsigned long *pgt;
    unsigned long tag;
    unsigned long *pos;

    pd = spack.dmabuf_pos;
    pgt = spack.pgiftag;
    tag = spack.giftag_b & 0xFFFFFFFFFFFF8000;
    pos = spack.pos;
    n = ((u_int)pos - (u_int)pgt - 16) >> 3;
    if ((u_int)pos & 0xF) {
        *pos++ = 0;
        spack.pos = pos;
    }
    n2 = (PHYS(pos) - (u_int)pd->Addr) >> 4;
    if (n == 0) {
        if (n2 > 1) {
            pd->VifQwc = pd->DmaQwc = n2 - 1;
            spack.pk_last = pos - 2;
            spkSetOT();
        }
        return;
    }
    pd->VifQwc = pd->DmaQwc = n2;
    if (!(tag & 0x0C00000000000000)) {
        n >>= 1;
    }
    nreg = (tag >> 60) & 0xF;
    if (nreg == 0) {
        nreg = 16;
    }
    *pgt = tag | (n / nreg);
    fjAssert((n % nreg) == 0);
    spack.pk_last = pos;
    spkSetOT();
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 248
/** Closes the open GIF tag and opens another (`giftag1`, `giftag2`) in the same packet. */
void spkCloseOpenDGiftag(unsigned long giftag1, unsigned long giftag2) {
    unsigned int n;
    unsigned int nreg;
    unsigned long *pgt;
    unsigned long tag;
    unsigned long *pos;

    pgt = spack.pgiftag;
    tag = spack.giftag_b & 0xFFFFFFFFFFFF8000;
    pos = spack.pos;
    n = ((u_int)pos - (u_int)pgt - 16) >> 3;
    if ((u_int)pos & 0xF) {
        *pos++ = 0;
    }
    if (n == 0) {
        pos -= 2;
    } else {
        if (!(tag & 0x0C00000000000000)) {
            n >>= 1;
        }
        nreg = (tag >> 60) & 0xF;
        if (nreg == 0) {
            nreg = 16;
        }
        *pgt = tag | (n / nreg);
        fjAssert((n % nreg) == 0);
    }
    spack.pgiftag = pos;
    pos[0] = giftag1;
    pos[1] = giftag2;
    spack.pos = pos + 2;
    spack.giftag_b = giftag1;
}

/** Starts a static (prebuilt, reusable) packet at `adr`. */
void spkStartPacketS(u_long128 *adr) {
    spack.ps_top = (struct SPACK_STATIC_DATA *)UNCACHED(PHYS(adr));
    spack.pos = (unsigned long *)spack.ps_top;
}

/** Opens GIF tag `giftag` in the static packet. */
void spkOpenGiftagS(void *giftag) {
    unsigned long d[2];
    unsigned long *pos;

    pos = spack.pos;
    spack.pgiftag = pos;
    memcpy(d, giftag, sizeof(d));
    pos[0] = d[0];
    pos[1] = d[1];
    spack.pos = pos + 2;
    spack.giftag_b = d[0];
}

/** Opens the GIF tag (`giftag1`, `giftag2`) in the static packet. */
void spkOpenDGiftagS(unsigned long giftag1, unsigned long giftag2) {
    unsigned long *pos;

    pos = spack.pos;
    spack.pgiftag = pos;
    pos[0] = giftag1;
    pos[1] = giftag2;
    spack.pos = pos + 2;
    spack.giftag_b = giftag1;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 325
/** Closes the open GIF tag of the static packet. */
void spkCloseGiftagS(void) {
    unsigned int n;
    unsigned int nreg;
    unsigned long *pgt;
    unsigned long tag;
    unsigned long *pos;

    pgt = spack.pgiftag;
    tag = spack.giftag_b & 0xFFFFFFFFFFFF8000;
    pos = spack.pos;
    n = ((u_int)pos - (u_int)pgt - 16) >> 3;
    if ((u_int)pos & 0xF) {
        *pos++ = 0;
        spack.pos = pos;
    }
    if (n == 0) {
        spack.pos = pos - 2;
    } else {
        if (!(tag & 0x0C00000000000000)) {
            n >>= 1;
        }
        nreg = (tag >> 60) & 0xF;
        if (nreg == 0) {
            nreg = 16;
        }
        *pgt = tag | (n / nreg);
        fjAssert((n % nreg) == 0);
    }
}

/** Ends the static packet (stores its size) and returns the address after it. */
u_long128 *spkEndPacketS(void) {
    unsigned short n;
    struct SPACK_STATIC_DATA *pt;
    u_long128 *pos;

    pt = spack.ps_top;
    pos = (u_long128 *)spack.pos;
    n = ((u_int)pos - (u_int)pt) >> 4;
    fjAssert(n > 0);
    pt->DmaQwc = n;
    return pos;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 373
/** Makes the static packet at `adr` environment `envid`. */
void spkSetEnvPacket(u_long128 *adr, unsigned short envid) {
    struct SPACK_ENV_DATA *pe;

    pe = &spack.env_top[envid];
    fjAssert(envid < SPACK_ENV_MAX);
    pe->VifQwc = pe->DmaQwc = ((struct SPACK_STATIC_DATA *)UNCACHED(PHYS(adr)))->DmaQwc;
    pe->DmaId = 0x3000;
    pe->Addr = (void *)PHYS(adr);
    pe->VifDirect = 0x50;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 405
/** Starts environment `envid` as an image upload. */
void spkStartEnvLoadImage(unsigned short envid) {
    struct SPACK_ENV_DATA *pe;

    pe = &spack.env_top[envid];
    fjAssert(envid < SPACK_ENV_MAX);
    *(u_long128 *)pe = 0;
    pe->DmaId = 0x5000;
    pe->Addr = (void *)PHYS(spack.pk_last);
    spack.pos = spack.pk_last;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 424
/**
 * Adds the upload of `image` (`w` x `h` at (`x`, `y`) in the buffer at `bp`, width `bw`, format
 * `psm`) to the environment being built.
 */
void spkSetEnvLoadImage(void *image, short bp, short bw, short psm, short x, short y, short w, short h) {
    int size;
    unsigned long *pos;
    unsigned int *pui;

    pui = (unsigned int *)spack.pos;
    switch (psm) {
    case 0:
    case 0x30:
        size = (w * h) >> 2;
        break;
    case 1:
    case 0x31:
        size = (w * h * 3) >> 4;
        break;
    case 2:
    case 0xA:
    case 0x32:
    case 0x3A:
        size = (w * h) >> 3;
        break;
    case 0x13:
    case 0x1B:
        size = (w * h) >> 4;
        break;
    case 0x14:
    case 0x24:
    case 0x2C:
        size = (w * h) >> 5;
        break;
    default:
        printf("spkSetEnvLoadImage: Illegal data type! (%d)\n", psm);
        fjAssert(0);
        return;
    }
    if (size > 0x7FFF) {
        printf("spkSetEnvLoadImage: Too big size! (%dx%d)\n", w, h);


        fjAssert(0);
        return;
    }
    fjAssert(size);
    pui[0] = 0x10000006;
    pui[1] = 0;
    pui[2] = 0;
    pui[3] = 0x50000006;
    pui = (unsigned int *)((u_long128 *)pui + 1);
    pos = (unsigned long *)pui;
    pos[0] = 0x1000000000000004;
    pos[1] = 0xE;
    pos[2] = ((long)bp << 32) | ((long)bw << 48) | ((long)psm << 56);
    pos[3] = 0x50;
    pos[4] = ((long)x << 32) | ((long)y << 48);
    pos[5] = 0x51;
    pos[6] = (long)w | ((long)h << 32);
    pos[7] = 0x52;
    pos[8] = 0;
    pos[9] = 0x53;
    pos[10] = (long)size | 0x0800000000008000;
    pos[11] = 0;
    pui = (unsigned int *)((u_long128 *)pui + 6);
    pui[0] = size | 0x30000000;
    pui[1] = (u_int)image;
    pui[2] = 0;
    pui[3] = size | 0x50000000;
    spack.pos = (unsigned long *)(pui + 4);
}

/** Ends the image upload environment. */
void spkEndEnvLoadImage(void) {
    unsigned long *pos;

    pos = spack.pos;
    pos[0] = 0x60000000;
    pos[1] = 0;
    spack.pk_last = pos + 2;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 518
/**
 * Links the static packet at `adr` into the ordering table at depth `w`, with environment `envid`.
 */
void spkSetOTPacketS(u_long128 *adr, unsigned int w, unsigned char envid) {
    struct SPACK_OT_DATA *pd;

    pd = spack.dmabuf_pos;
    fjAssert(envid < SPACK_ENV_MAX);
    pd->Addr = (void *)PHYS(adr);
    pd->W = w;
    pd->EnvID = envid;
    pd->VifDirect = 0x50;
    pd->VifQwc = pd->DmaQwc = ((struct SPACK_STATIC_DATA *)UNCACHED(PHYS(adr)))->DmaQwc;
    spkSetOT();
}

/** spkSetOTPacketS() for asm callers (arguments in t registers; hand-written asm). */
/* t5 = packet, t6 = w, t7 = envid */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 570); the return's delay slot is filled. */
asm void spkSetOTPacketS_asm(void) {
    .set noreorder
    addiu   sp, sp, -0x50
    la      t4, spack
    sq      t0, 0x0(sp)
    sq      t1, 0x10(sp)
    sq      t2, 0x20(sp)
    sq      t3, 0x30(sp)
    sq      ra, 0x40(sp)
    lw      t4, 0x24(t4)
    dsll32  t5, t5, 4
    ori     t7, t7, 0x5000
    dsrl32  t5, t5, 4
    sh      t7, 0xE(t4)
    lui     t7, 0x2000
    sw      t5, 0x4(t4)
    or      t5, t5, t7
    sw      t6, 0x8(t4)
    lhu     t7, 0x2(t5)
    sh      t7, 0x0(t4)
    jal     spkSetOT
    sh      t7, 0xC(t4)
    lq      ra, 0x40(sp)
    lq      t3, 0x30(sp)
    lq      t2, 0x20(sp)
    lq      t1, 0x10(sp)
    lq      t0, 0x0(sp)
    jr      ra
    addiu   sp, sp, 0x50
}

/** Links the packet just built into the ordering table (hand-written asm). */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 678); the return's delay slot is filled. */
asm void spkSetOT(void) {
    .set noreorder
    la      t0, spack
    lw      t1, 0x24(t0)
    addiu   t3, t1, 0x10
    lw      t2, 0x8(t1)
    sw      t3, 0x24(t0)
    lui     t3, 0xFFFF
    lui     t4, 0x8000
    and     t5, t2, t3
    lw      t6, 0x14(t0)
    bne     t5, t4, L_notfront
    sh      zero, 0x2(t1)
    andi    t4, t2, 0xFFFF
    lw      t5, 0x8(t0)
    xori    t4, t4, 0xFFFF
    addi    t2, t5, 0x2
    b       L_link
    sw      t4, 0x8(t1)
L_notfront:
    sltu    t4, t2, t6
    bne     t5, t3, L_normal
    lw      t6, 0x18(t0)
    andi    t4, t2, 0xFFFF
    addu    t2, zero, zero
    b       L_link
    sw      t4, 0x8(t1)
L_normal:
    sltu    t5, t6, t2
    bnel    t4, zero, L_1
    sw      t2, 0x14(t0)
L_1:
    lw      t4, 0x10(t0)
    bnel    t5, zero, L_2
    sw      t2, 0x18(t0)
L_2:
    subu    t2, t2, t4
    sb      zero, 0xB(t1)
    lw      t4, 0xC(t0)
    lw      t5, 0x8(t0)
    blezl   t2, L_3
    addu    t2, zero, zero
    srlv    t2, t2, t4
L_3:
    sltu    t6, t5, t2
    bnel    t6, zero, L_4
    addu    t2, zero, t5
L_4:
    addiu   t2, t2, 0x1
L_link:
    lw      t3, 0x0(t0)
    sll     t5, t2, 2
    lw      t4, 0x4(t0)
    addu    t3, t3, t5
    addu    t4, t4, t5
    lw      t6, 0x0(t3)
    beql    t6, zero, L_end
    sw      t1, 0x0(t3)
    lw      t5, 0x20(t0)
    lw      t6, 0x0(t4)
    subu    t5, t1, t5
    srl     t5, t5, 4
    sh      t5, 0x2(t6)
L_end:
    jr      ra
    sw      t1, 0x0(t4)
}

/*
 * Number of doublings of 1 (at most 32) that reach n: the ordering table's width in bits.
 * Matching: spkDmaKick's DWARF has no locals for this loop and its line table puts the whole
 * computation on one line, so the original computed it in an inlined helper. The name is
 * invented (an inlined static has no DWARF entry).
 */
static inline u_int ot_width_get(u_int n) {
    u_int i;
    u_int s;

    for (i = 0, s = 1; s < n && i < 32; s <<= 1, i++) {
    }
    return i;
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 686
/**
 * Sorts the ordering table by depth, links it into one DMA chain and returns the chain's start.
 * Asserts that the buffers did not overflow.
 */
void *spkDmaKick(void) {
    void *top;

    spkDmaWaitfromSPR(0);
    fjAssert(((u_int)spack.pk_last & 0x0fffffff) - (u_int)spack.packet <= SPACK_PBUF_SIZE);
    fjAssert(spack.dmabuf_pos - spack.dma_top <= SPACK_DMABUF_SIZE);
    ot_check();



    fjAssert(((u_int)spack.pk_last & 0x0fffffff) - (u_int)spack.packet + (spack.dmabuf_pos - spack.dma_top) * 32 <= SPACK_PBUF_SIZE);
    top = ot_link();
    if (top) {
        unsigned long *pos;

        pos = spack.pk_last;
        pos[0] = 0x70000002;
        pos[1] = (unsigned long)0x50000002 << 32;
        pos[2] = 0x1000000000008001;
        pos[3] = 0xE;
        pos[4] = 0;
        pos[5] = 0x3F;
        spack.pk_last = pos + 6;
    } else {
        unsigned long *pos;

        pos = spack.pk_last;
        pos[0] = 0x70000000;
        pos[1] = 0;
        spack.pk_last = pos + 2;
        top = pos;
        return spack.kick_top = (void *)PHYS(top);
    }
    if (spack.w_mini != -1) {
        spack.w_ofs = spack.w_mini;
        spack.ot_width = ot_width_get((spack.w_max - spack.w_mini) / spack.ot_size);
    }
    return spack.kick_top = (void *)PHYS(top);
}

/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 1035); the return's delay slot is filled. */
static asm void *ot_link(void) {
    .set noreorder
    addiu   sp, sp, -0x90
    sq      s0, 0x0(sp)
    sq      s1, 0x10(sp)
    sq      s2, 0x20(sp)
    sq      s3, 0x30(sp)
    sq      s4, 0x40(sp)
    sq      s5, 0x50(sp)
    sq      s6, 0x60(sp)
    sq      s7, 0x70(sp)
    sq      ra, 0x80(sp)
    la      s0, spack
    lui     s7, 0x7000
    lw      t0, 0x8(s0)
    lw      t1, 0x0(s0)
    addi    t0, t0, 0x2
    lw      t4, 0x2C(s0)
    sll     t6, t0, 2
    addu    t5, zero, zero
    addu    t1, t1, t6
L_loop:
    sll     t6, t0, 1
    lw      t2, 0x0(t1)
    lw      t7, 0x34(s0)
    beqz    t2, L_next
    xor     s1, s1, s1
    addu    t7, t7, t6
    addu    t3, zero, s7
    lh      t6, 0x0(t7)
    slti    t7, t6, 0x200
    lw      s2, 0x20(s0)
    beqz    t7, L_big
    slti    t6, t6, 0x400
L_copy1:
    lq      t6, 0x0(t2)
    addiu   t7, zero, 0x3000
    srl     s3, t6, 16
    sq      t6, 0x0(t3)
    andi    s3, s3, 0xFFFF
    sh      t7, 0x2(t3)
    beqz    s3, L_copied1
    addi    s1, s1, 0x1
    sll     s3, s3, 4
    addiu   t3, t3, 0x10
    b       L_copy1
    addu    t2, s2, s3
L_copied1:
    slti    t6, s1, 0x2
    bnez    t6, L_nosort1
    addiu   s6, s7, 0x3FF0
    addu    s4, zero, s7
    sq      t4, 0x0(s6)
    jal     ot_qsort_asm
    add     s5, zero, s1
    lq      t4, 0x0(s6)
L_nosort1:
    addu    t3, zero, s7
    add     s3, zero, s1
    lw      s2, 0x28(s0)
L_out1:
    lbu     t6, 0xE(t3)
    beqz    t6, L_out1b
    sll     t7, t6, 4
    beq     t6, t5, L_out1b
    add     t7, s2, t7
    lq      t7, 0x0(t7)
    add     t5, zero, t6
    sq      t7, 0x0(t4)
    addiu   t4, t4, 0x10
L_out1b:
    lq      t7, 0x0(t3)
    addi    s3, s3, -0x1
    addiu   t3, t3, 0x10
    sq      t7, 0x0(t4)
    bnez    s3, L_out1
    addiu   t4, t4, 0x10
    b       L_next
    nop
L_big:
    beqz    t6, L_nosort
    nop
L_copy2:
    lw      t6, 0x8(t2)
    sw      t2, 0x0(t3)
    lh      s3, 0x2(t2)
    sw      t6, 0x4(t3)
    beqz    s3, L_copied2
    addi    s1, s1, 0x1
    sll     s3, s3, 4
    addiu   t3, t3, 0x8
    b       L_copy2
    addu    t2, s2, s3
L_copied2:
    slti    t6, s1, 0x2
    bnez    t6, L_nosort2
    addiu   s6, s7, 0x3FF0
    addu    s4, zero, s7
    sq      t4, 0x0(s6)
    jal     ot_qsort_asm2
    add     s5, zero, s1
    lq      t4, 0x0(s6)
L_nosort2:
    addu    t3, zero, s7
    add     s3, zero, s1
    lw      s2, 0x28(s0)
L_out2:
    lw      t2, 0x0(t3)
    lbu     t6, 0xE(t2)
    beqz    t6, L_out2b
    sll     t7, t6, 4
    beq     t6, t5, L_out2b
    add     t7, s2, t7
    lq      t7, 0x0(t7)
    add     t5, zero, t6
    sq      t7, 0x0(t4)
    addiu   t4, t4, 0x10
L_out2b:
    lq      t7, 0x0(t2)
    addi    s3, s3, -0x1
    addiu   t6, zero, 0x3000
    sq      t7, 0x0(t4)
    addiu   t3, t3, 0x8
    sh      t6, 0x2(t4)
    bnez    s3, L_out2
    addiu   t4, t4, 0x10
    b       L_next
    nop
L_nosort:
    lw      s2, 0x28(s0)
    lw      t3, 0x20(s0)
L_out3:
    lbu     t6, 0xE(t2)
    beqz    t6, L_out3b
    sll     t7, t6, 4
    beq     t6, t5, L_out3b
    add     t7, s2, t7
    lq      t7, 0x0(t7)
    add     t5, zero, t6
    sq      t7, 0x0(t4)
    addiu   t4, t4, 0x10
L_out3b:
    lq      t7, 0x0(t2)
    lh      s3, 0x2(t2)
    addiu   t6, zero, 0x3000
    sq      t7, 0x0(t4)
    sh      t6, 0x2(t4)
    beqz    s3, L_next
    addiu   t4, t4, 0x10
    sll     s3, s3, 4
    b       L_out3
    addu    t2, t3, s3
L_next:
    addi    t0, t0, -0x1
    bgez    t0, L_loop
    addiu   t1, t1, -0x4
    lw      v0, 0x2C(s0)
    sw      t4, 0x30(s0)
    beql    t4, v0, L_ret
    addu    v0, zero, zero
    sw      t4, 0x2C(s0)
L_ret:
    lq      ra, 0x80(sp)
    lq      s7, 0x70(sp)
    lq      s6, 0x60(sp)
    lq      s5, 0x50(sp)
    lq      s4, 0x40(sp)
    lq      s3, 0x30(sp)
    lq      s2, 0x20(sp)
    lq      s1, 0x10(sp)
    lq      s0, 0x0(sp)
    jr      ra
    addiu   sp, sp, 0x90
}

/* Sorts s5 16-byte OT entries at s4 by W (descending); s6 = save stack. */
/* Original asm: its 3 returns are on their own lines right after the code in the line table (lines
 * 1175, 1186, 1189); 2 return delay slots are filled. */
static asm void ot_qsort_asm(void) {
    .set noreorder
L_top:
    slti    t7, s5, 0x3
    addi    t6, zero, 0x2
    bnez    t7, L_small
    addu    s2, zero, s4
    addiu   s6, s6, -0x10
    slti    t7, s5, 0x8
    sw      t0, 0x0(s6)
    sw      t1, 0x4(s6)
    sw      t2, 0x8(s6)
    beqz    t7, L_quick
    sw      ra, 0xC(s6)
    xor     t0, t0, t0
L_sel:
    add     t2, zero, t0
    addi    t1, t0, 0x1
    addiu   s3, s2, 0x10
    lw      t3, 0x8(s2)
L_sel2:
    lw      t4, 0x8(s3)
    slt     t7, t3, t4
    movn    t2, t1, t7
    addi    t1, t1, 0x1
    movn    t3, t4, t7
    bne     t1, s5, L_sel2
    addiu   s3, s3, 0x10
    beq     t2, t0, L_noswap
    sll     s3, t2, 4
    addu    s3, s3, s4
    lq      t6, 0x0(s2)
    lq      t7, 0x0(s3)
    sq      t6, 0x0(s3)
    sq      t7, 0x0(s2)
L_noswap:
    addi    t7, t0, 0x2
    addi    t0, t0, 0x1
    bne     t7, s5, L_sel
    addiu   s2, s2, 0x10
L_return:
    lwu     ra, 0xC(s6)
    lwu     t0, 0x0(s6)
    lwu     t1, 0x4(s6)
    lwu     t2, 0x8(s6)
    jr      ra
    addiu   s6, s6, 0x10
L_small:
    bne     s5, t6, L_done
    lw      t6, 0x8(s4)
    lw      t7, 0x18(s4)
    slt     t7, t6, t7
    lq      s2, 0x0(s4)
    beqz    t7, L_done
    lq      s3, 0x10(s4)
    sq      s2, 0x10(s4)
    jr      ra
    sq      s3, 0x0(s4)
L_done:
    jr      ra
    nop
L_quick:
    srl     t6, s5, 1
    addi    t7, s5, -0x1
    sll     t6, t6, 4
    sll     t7, t7, 4
    addu    t6, t6, s4
    addu    t7, t7, s4
    lw      t0, 0x8(s4)
    lw      t1, 0x8(t6)
    lw      t2, 0x8(t7)
    slt     t7, t0, t1
    slt     t6, t1, t2
    beqz    t7, L_med2
    slt     t4, t0, t2
    bnel    t6, zero, L_med
    addu    t3, zero, t1
    bnel    t4, zero, L_med
    addu    t3, zero, t2
    b       L_med
    addu    t3, zero, t0
L_med2:
    beql    t6, zero, L_med
    addu    t3, zero, t1
    bnel    t4, zero, L_med
    addu    t3, zero, t0
    addu    t3, zero, t2
L_med:
    addi    t1, s5, -0x1
    addu    t0, zero, s4
    sll     t1, t1, 4
    add     t2, zero, s5
    addu    t1, t1, s4
    add     t4, zero, zero
L_left:
    lw      t6, 0x8(t0)
    slt     t7, t3, t6
    beqz    t7, L_right
    nop
    addi    t2, t2, -0x1
    addiu   t0, t0, 0x10
    blez    t2, L_split
    addi    t4, t4, 0x1
    b       L_left
    nop
L_right:
    lw      t6, 0x8(t1)
    slt     t7, t6, t3
    beqz    t7, L_swap
    nop
    addi    t2, t2, -0x1
    blez    t2, L_split
    addiu   t1, t1, -0x10
    b       L_right
    nop
L_swap:
    lq      t6, 0x0(t0)
    lq      t7, 0x0(t1)
    sq      t6, 0x0(t1)
    sq      t7, 0x0(t0)
    addi    t2, t2, -0x2
    addiu   t0, t0, 0x10
    addiu   t1, t1, -0x10
    bgtz    t2, L_left
    addi    t4, t4, 0x1
L_split:
    slti    t7, t4, 0x2
    sub     t1, s5, t4
    bnez    t7, L_skip
    slti    t2, t1, 0x2
    jal     ot_qsort_asm
    add     s5, zero, t4
L_skip:
    bnez    t2, L_return
    addu    s4, zero, t0
    la      ra, L_return
    b       L_top
    add     s5, zero, t1
}

/* Same for 8-byte (pointer, W) pairs. */
/* Original asm: its 3 returns are on their own lines right after the code in the line table (lines
 * 1406, 1417, 1420); 2 return delay slots are filled. */
static asm void ot_qsort_asm2(void) {
    .set noreorder
L_top:
    slti    t7, s5, 0x3
    addi    t6, zero, 0x2
    bnez    t7, L_small
    addu    s2, zero, s4
    addiu   s6, s6, -0x10
    slti    t7, s5, 0x8
    sw      t0, 0x0(s6)
    sw      t1, 0x4(s6)
    sw      t2, 0x8(s6)
    beqz    t7, L_quick
    sw      ra, 0xC(s6)
    xor     t0, t0, t0
L_sel:
    add     t2, zero, t0
    addi    t1, t0, 0x1
    addiu   s3, s2, 0x8
    lw      t3, 0x4(s2)
L_sel2:
    lw      t4, 0x4(s3)
    slt     t7, t3, t4
    movn    t2, t1, t7
    addi    t1, t1, 0x1
    movn    t3, t4, t7
    bne     t1, s5, L_sel2
    addiu   s3, s3, 0x8
    beq     t2, t0, L_noswap
    sll     s3, t2, 3
    addu    s3, s3, s4
    ld      t6, 0x0(s2)
    ld      t7, 0x0(s3)
    sd      t6, 0x0(s3)
    sd      t7, 0x0(s2)
L_noswap:
    addi    t7, t0, 0x2
    addi    t0, t0, 0x1
    bne     t7, s5, L_sel
    addiu   s2, s2, 0x8
L_return:
    lwu     ra, 0xC(s6)
    lwu     t0, 0x0(s6)
    lwu     t1, 0x4(s6)
    lwu     t2, 0x8(s6)
    jr      ra
    addiu   s6, s6, 0x10
L_small:
    bne     s5, t6, L_done
    lw      t6, 0x4(s4)
    lw      t7, 0xC(s4)
    slt     t7, t6, t7
    ld      s2, 0x0(s4)
    beqz    t7, L_done
    ld      s3, 0x8(s4)
    sd      s2, 0x8(s4)
    jr      ra
    sd      s3, 0x0(s4)
L_done:
    jr      ra
    nop
L_quick:
    srl     t6, s5, 1
    addi    t7, s5, -0x1
    sll     t6, t6, 3
    sll     t7, t7, 3
    addu    t6, t6, s4
    addu    t7, t7, s4
    lw      t0, 0x4(s4)
    lw      t1, 0x4(t6)
    lw      t2, 0x4(t7)
    slt     t7, t0, t1
    slt     t6, t1, t2
    beqz    t7, L_med2
    slt     t4, t0, t2
    bnel    t6, zero, L_med
    addu    t3, zero, t1
    bnel    t4, zero, L_med
    addu    t3, zero, t2
    b       L_med
    addu    t3, zero, t0
L_med2:
    beql    t6, zero, L_med
    addu    t3, zero, t1
    bnel    t4, zero, L_med
    addu    t3, zero, t0
    addu    t3, zero, t2
L_med:
    addi    t1, s5, -0x1
    addu    t0, zero, s4
    sll     t1, t1, 3
    add     t2, zero, s5
    addu    t1, t1, s4
    add     t4, zero, zero
L_left:
    lw      t6, 0x4(t0)
    slt     t7, t3, t6
    beqz    t7, L_right
    nop
    addi    t2, t2, -0x1
    addiu   t0, t0, 0x8
    blez    t2, L_split
    addi    t4, t4, 0x1
    b       L_left
    nop
L_right:
    lw      t6, 0x4(t1)
    slt     t7, t6, t3
    beqz    t7, L_swap
    nop
    addi    t2, t2, -0x1
    blez    t2, L_split
    addiu   t1, t1, -0x8
    b       L_right
    nop
L_swap:
    ld      t6, 0x0(t0)
    ld      t7, 0x0(t1)
    sd      t6, 0x0(t1)
    sd      t7, 0x0(t0)
    addi    t2, t2, -0x2
    addiu   t0, t0, 0x8
    addiu   t1, t1, -0x8
    bgtz    t2, L_left
    addi    t4, t4, 0x1
L_split:
    slti    t7, t4, 0x2
    sub     t1, s5, t4
    bnez    t7, L_skip
    slti    t2, t1, 0x2
    jal     ot_qsort_asm2
    add     s5, zero, t4
L_skip:
    bnez    t2, L_return
    addu    s4, zero, t0
    la      ra, L_return
    b       L_top
    add     s5, zero, t1
}

/* Counts the packets on each OT slot into spack.ot_max. */
/* Original asm: the return (jr ra) is on its own line right after the code in the line table (line
 * 1572). */
static asm void ot_check(void) {
    .set noreorder
    la      t5, spack
    lw      t0, 0x8(t5)
    lw      t1, 0x0(t5)
    lw      t3, 0x20(t5)
    addi    t0, t0, 0x2
    lw      t5, 0x34(t5)
    sll     t6, t0, 2
    sll     t7, t0, 1
    addu    t1, t1, t6
    addu    t5, t5, t7
L_slot:
    lw      t2, 0x0(t1)
    xor     t4, t4, t4
    beqz    t2, L_store
    nop
L_count:
    lh      t6, 0x2(t2)
    addi    t4, t4, 0x1
    beqz    t6, L_store
    sll     t7, t6, 4
    addu    t2, t3, t7
    b       L_count
    nop
L_store:
    sh      t4, 0x0(t5)
    addi    t0, t0, -0x1
    addiu   t1, t1, -0x4
    bgez    t0, L_slot
    addiu   t5, t5, -0x2
    jr      ra
    nop
}

/**
 * Waits for the DMA to the scratchpad (channel 9) to finish, for at most `timeout` polls (0:
 * 0x1000000).
 */
void spkDmaWaittoSPR(int timeout) {
    int c;

    c = 0;
    if (timeout == 0) {
        timeout = 0x1000000;
    }
    while (*D9_CHCR & 0x100) {
        if (++c > timeout) {
            printf("spack: DmaWaittoSPR: timeout\n");
            break;
        }
    }
}

/**
 * Waits for the DMA from the scratchpad (channel 8) to finish, for at most `timeout` polls (0:
 * 0x1000000).
 */
void spkDmaWaitfromSPR(int timeout) {
    int c;

    c = 0;
    if (timeout == 0) {
        timeout = 0x1000000;
    }
    while (*D8_CHCR & 0x100) {
        if (++c > timeout) {
            printf("spack: DmaWaitfromSPR: timeout\n");
            break;
        }
    }
}

/** DMAs `qwc` qwords from `madr` to scratchpad address `sadr`. */
void spkDmatoSPR(unsigned int qwc, unsigned int sadr, void *madr) {
    spkDmaWaittoSPR(0);
    *D9_QWC = qwc;
    *D9_SADR = sadr & 0x3FF0;
    *D9_MADR = PHYS(madr);
    *D9_CHCR = 0x100;
}

/** DMAs `qwc` qwords from scratchpad address `sadr` to `madr`. */
void spkDmafromSPR(unsigned int qwc, unsigned int sadr, void *madr) {
    spkDmaWaitfromSPR(0);
    *D8_QWC = qwc;
    *D8_SADR = sadr & 0x3FF0;
    *D8_MADR = PHYS(madr);
    *D8_CHCR = 0x100;
}

/** Prints the packet builder's buffer use on the debug font. */
void spkDebugPrint(void) {
    char nbuf[3];
    char buf[14];
    int i;
    int m;
    int x;
    int y;

    shDBG_print_string(" all top last", 8, 0x46);
    nbuf[0] = ' ';
    nbuf[2] = 0;
    x = 0x68;
    y = 0x46;
    m = 0;
    for (i = 0; i <= spack.ot_size; i++) {
        if (i >= 10) {
            nbuf[0] = i / 10 + '0';
        }
        nbuf[1] = i % 10 + '0';
        sprintf(buf, "%4d", spack.ot_max[i + 1]);
        m += spack.ot_max[i + 1];
        shDBG_print_string(nbuf, x + 16, y);
        shDBG_print_string(buf, x, y + 10);
        x += 32;
        if (i == 9 || i == 22) {
            x = 8;
            y += 24;
        }
    }
    m += spack.ot_max[0] + spack.ot_max[spack.ot_size + 2];
    sprintf(buf, "%5d%4d%4d", m, spack.ot_max[spack.ot_size + 2], spack.ot_max[0]);
    shDBG_print_string(buf, 0, 0x50);
}
