/*
 * GS local-to-host image transfer (Lens): reads a rectangle of GS memory (for the lens flare,
 * one column of the Z buffer) back to main memory: sh2gfw_GsSetDefStoreImage builds the packet,
 * sh2gfw_GsExecStoreImage runs the transfer.
 */
#include "sh2.h"
#include "sdk/libgraph.h"

#define UNCACHED(p) ((void *)((unsigned int)(p) | 0x20000000))

/**
 * Builds the VIF/GIF packet that starts a GS-to-host transfer.
 * @param sp   the packet
 * @param sbp  source buffer base pointer
 * @param sbw  source buffer width (in 64-pixel units)
 * @param spsm source pixel format
 * @param x    rectangle x
 * @param y    rectangle y
 * @param w    rectangle width
 * @param h    rectangle height
 * @return the packet size in quadwords
 */
int sh2gfw_GsSetDefStoreImage(struct sceGsStoreImage *sp, short sbp, short sbw, short spsm, short x, short y, short w, short h) {
    sp->vifcode[0] = 0;
    sp->vifcode[1] = 0;
    sp->vifcode[2] = 0x13000000;
    sp->vifcode[3] = 0x50000006;
    *(u_long128 *)&sp->giftag = 0;
    sp->giftag.NLOOP = 5;
    sp->giftag.EOP = 1;
    sp->giftag.NREG = 1;
    sp->giftag.REGS0 = 0xE;
    *(unsigned long *)&sp->bitbltbuf = (unsigned long)sbp | ((unsigned long)sbw << 16) | ((unsigned long)spsm << 24);
    sp->bitbltbufaddr = 0x50;
    *(unsigned long *)&sp->trxpos = (unsigned long)x | ((unsigned long)y << 16);
    sp->trxposaddr = 0x51;
    *(unsigned long *)&sp->trxreg = (unsigned long)w | ((unsigned long)h << 32);
    sp->trxregaddr = 0x52;
    *(unsigned long *)&sp->finish = 0;
    sp->finishaddr = 0x61;
    *(unsigned long *)&sp->trxdir = 1;
    sp->trxdiraddr = 0x53;
    asm volatile("sync.l");
    return sizeof(struct sceGsStoreImage) / 16;
}

/**
 * Runs a GS-to-host transfer set up by sh2gfw_GsSetDefStoreImage: sends the packet on
 * VIF1, reverses the bus direction, and reads the image into dstaddr (DMA channel 1, the rest
 * from the VIF1 FIFO). Declared int in the DWARF, but no value is returned.
 */
/* Matching: the masks are unsigned (`~7U`): a signed ~7 is a small constant the backend reuses in
   a fresh register, where the unsigned one is kept in a temporary numbered below the others. */
int sh2gfw_GsExecStoreImage(struct sceGsStoreImage *sp, u_long128 *dstaddr) {
    static unsigned int init_mp3[4] = { 0x06000000, 0, 0, 0 };
    int dmasizeq;
    int rsizeq;
    int remq;
    int remb;
    unsigned long oldIMR;
    unsigned char tmpbuf[16] __attribute__((aligned(16)));
    int sizeb;
    int allsizeq;
    int i;
    int h;
    int ah;
    int w;
    unsigned int vcnt;

    w = sp->trxreg.RRW;
    h = sp->trxreg.RRH;
    switch (sp->bitbltbuf.SPSM) {
    case 0:
    case 0x30:
        sizeb = w * h * 4;
        remb = sizeb & 0xF;
        remq = (sizeb >> 4) & 7;
        dmasizeq = (sizeb >> 4) & ~7U;
        if (remb == 0) {
            rsizeq = 0;
            ah = h;
        } else {
            ah = (h + 3) & ~3U;
            allsizeq = (w * ah) >> 2;
            rsizeq = allsizeq - dmasizeq - remq - 1;
        }
        break;
    case 1:
    case 0x31:
        sizeb = w * h * 3;
        remb = sizeb & 0xF;
        remq = (sizeb >> 4) & 7;
        dmasizeq = (sizeb >> 4) & ~7U;
        if (remb == 0) {
            rsizeq = 0;
            ah = h;
        } else {
            ah = (h + 15) & ~15U;
            allsizeq = (w * ah * 3) >> 4;
            rsizeq = allsizeq - dmasizeq - remq - 1;
        }
        break;
    case 2:
    case 0xA:
    case 0x32:
    case 0x3A:
        sizeb = w * h * 2;
        remb = sizeb & 0xF;
        remq = (sizeb >> 4) & 7;
        dmasizeq = (sizeb >> 4) & ~7U;
        if (remb == 0) {
            rsizeq = 0;
            ah = h;
        } else {
            ah = (h + 7) & ~7U;
            allsizeq = (w * ah) >> 3;
            rsizeq = allsizeq - dmasizeq - remq - 1;
        }
        break;
    case 0x13:
    case 0x1B:
        sizeb = w * h;
        remb = sizeb & 0xF;
        remq = (sizeb >> 4) & 7;
        dmasizeq = (sizeb >> 4) & ~7U;
        if (remb == 0) {
            rsizeq = 0;
            ah = h;
        } else {
            ah = (h + 7) & ~7U;
            allsizeq = (w * ah) >> 4;
            rsizeq = allsizeq - dmasizeq - remq - 1;
        }
        break;
    case 0x14:
    case 0x24:
    case 0x2C:
        sizeb = (w * h) >> 1;
        remb = sizeb & 0xF;
        remq = (sizeb >> 4) & 7;
        dmasizeq = (sizeb >> 4) & ~7U;
        if (remb == 0) {
            rsizeq = 0;
            ah = h;
        } else {
            ah = (h + 7) & ~7U;
            allsizeq = (w * ah) >> 5;
            rsizeq = allsizeq - dmasizeq - remq - 1;
        }
        break;
    }
    if (remb) {
        *(unsigned long *)UNCACHED(&sp->trxreg) = (unsigned long)(unsigned short)w | ((unsigned long)ah << 32);
    }
    d1sSync(0, -1);
    d2sSync(0, -1);
    oldIMR = sceGsPutIMR(sceGsGetIMR() | 0x200);
    *GS_CSR = 2;
    *D1_QWC = 7;
    if (((unsigned int)sp & 0x70000000) == 0x70000000) {
        *D1_MADR = ((unsigned int)sp & 0x0FFFFFFF) | 0x80000000;
    } else {
        *D1_MADR = (unsigned int)sp & 0x0FFFFFFF;
    }
    *D1_CHCR = 0x101;
    while (*D1_CHCR & 0x100) {
    }
    while (!(*GS_CSR & 2)) {
    }
    *VIF1_STAT = 0x800000;
    *GS_BUSDIR = 1;
    *D1_QWC = dmasizeq;
    if (((unsigned int)dstaddr & 0x70000000) == 0x70000000) {
        *D1_MADR = ((unsigned int)dstaddr & 0x0FFFFFFF) | 0x80000000;
    } else {
        *D1_MADR = (unsigned int)dstaddr & 0x0FFFFFFF;
    }
    *D1_CHCR = 0x100;
    while (*D1_CHCR & 0x100) {
    }
    for (i = 0; i < remq; i++) {
        dstaddr[dmasizeq + i] = *VIF1_FIFO;
    }
    if (remb) {
        *(u_long128 *)tmpbuf = *VIF1_FIFO;
        for (i = 0; i < remb; i++) {
            ((unsigned char *)&dstaddr[dmasizeq + remq])[i] = tmpbuf[i];
        }
        for (i = 0; i < rsizeq; i++) {
            *(u_long128 *)tmpbuf = *VIF1_FIFO;
        }
    }
    *VIF1_STAT = 0;
    *GS_BUSDIR = 0;
    sceGsPutIMR(oldIMR);
    *GS_CSR = 2;
    *VIF1_FIFO = *(u_long128 *)init_mp3;
    *D1_CHCR = 0x45;
}
