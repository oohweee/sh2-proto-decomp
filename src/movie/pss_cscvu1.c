#include "sh2.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"

/*
 * pss_cscvu1.c: colour space conversion of the decoded movie frames on VU1
 * (the cscVu1* functions). For every 16x16 macroblock a VIF1 packet
 * uploads its Y, Cb and Cr samples (plus the neighbouring chroma rows for the
 * interpolation) into one of three VU1 memory banks and kicks the microcode,
 * which converts them to RGB and draws them with the GS.
 */



typedef struct anon_180_5cbefc7f CscRAW8; /* sceIpuRAW8's bytes as 2-D y/cb/cr blocks */

/* VU1 microcode (linked as data; the names are the binary's symbols) */
extern unsigned int vu1_init[];
extern unsigned int vu1_XYZ2Offset[];
extern unsigned int load_yuvfrfl0_mpg[];
extern unsigned int load_yuvfrfl1_mpg[];
extern unsigned int load_yuvprg0_mpg[];
extern unsigned int load_yuvprg1_mpg[];


#define DMA_ADDR(val) ((void *)((unsigned int)(val) & 0x0FFFFFFF))
#define UNCACHED(val) ((void *)(((unsigned int)(val) & 0x0FFFFFFF) | 0x20000000))

#define UNPACK_V2_32 0x04
#define UNPACK_V4_32 0x0C
#define UNPACK_V4_8 0x12
#define UNPACK_UNSIGNED 0x4000

static unsigned int *setGIFtag(unsigned int *p, unsigned long regs, unsigned int nreg, unsigned int flg,
                               unsigned int prim, unsigned int pre, unsigned int eop, unsigned int nloop);
static void mkrefpacket(sceVif1Packet *pkt, void *src, unsigned int upkcmd, unsigned int vuaddr,
                        unsigned int vunum, unsigned int dmarate);
static void mkcntpacket(sceVif1Packet *pkt, unsigned int code1, unsigned int code2);

/* The three VU1 memory banks the macroblocks rotate through. */
unsigned int vu1_base_adr[3] = {7, 0x15A, 0x2AD};

/**
 * Sets up `csc` with the addresses of the VU1 conversion microcode (frame and field pictures, top
 * and bottom).
 */
void cscVu1Init(CscVu1 *csc) {
    csc->micro[0][0] = load_yuvfrfl0_mpg;
    csc->micro[0][1] = load_yuvfrfl1_mpg;
    csc->micro[2][0] = load_yuvprg0_mpg;
    csc->micro[2][1] = load_yuvprg1_mpg;
}

/**
 * Patches the VU1 init packet with the microcode of picture `type` (`isBottom` field) and the draw
 * offset (`xoff`, `yoff`).
 */
void cscVu1Xyz2offset(CscVu1 *csc, int type, int isBottom, int xoff, int yoff) {
    unsigned int *tag;
    unsigned int *xyz2offset;

    tag = UNCACHED(vu1_init);
    tag[1] = (unsigned int)csc->micro[type][isBottom];

    xyz2offset = UNCACHED(vu1_XYZ2Offset);
    xyz2offset[0] = xoff;
    xyz2offset[1] = yoff;
}

/** Sends the VIF1 DMA chain `tags`. */
void cscVu1Kick(unsigned int *tags) {
    *D1_TADR = (unsigned int)DMA_ADDR(tags);
    *D1_QWC = 0;
    *D1_CHCR = 0x145;
}

static unsigned int *setGIFtag(unsigned int *p, unsigned long regs, unsigned int nreg, unsigned int flg,
                               unsigned int prim, unsigned int pre, unsigned int eop, unsigned int nloop) {
    p[0] = (eop << 15) | nloop;
    p[1] = (pre << 14) | (prim << 15) | (flg << 26) | (nreg << 28);
    p[2] = regs & 0xFFFFFFFF;
    p[3] = (unsigned int)(regs >> 32);
    return p + 4;
}

/**
 * Builds in `tags` the VIF1 chain that converts and draws the `width` x `height` picture `image`
 * (RAW8 macroblocks) for frame/field pictures (type 0).
 */
void cscVu1SetTag_frfl(unsigned int *tags, void *image, int width, int height) {
    sceVif1Packet pkt;
    int i;
    int j;
    int dstx;
    int dsty;
    int pdstx;
    int pdsty;
    int basep;
    int nextbasep;
    int mbx;
    int mby;
    u_long128 giftagPoint;
    u_long128 giftagGsAD;
    CscRAW8 *imgbase;
    CscRAW8 *img;
    char *src;
    int offset;
    int ir;

    basep = 1;
    mbx = width >> 4;
    mby = height >> 4;
    imgbase = image;

    sceVif1PkInit(&pkt, (u_long128 *)tags);
    sceVif1PkReset(&pkt);
    sceVif1PkCall(&pkt, (u_long128 *)vu1_init, 0);
    sceVif1PkCnt(&pkt, 0);
    sceVif1PkAddCode(&pkt, VIF_FLUSHE(0));
    sceVif1PkAddCode(&pkt, VIF_STCYCL(4, 4, 0));

    setGIFtag((unsigned int *)&giftagPoint, 0x51, 2, 0, 0, 1, 1, 0x80);
    sceVif1PkAddCode(&pkt, VIF_UNPACK(7, 1, UNPACK_V4_32, 0));
    sceVif1PkAddUpkData128(&pkt, giftagPoint);
    sceVif1PkAddCode(&pkt, VIF_UNPACK(0x15A, 1, UNPACK_V4_32, 0));
    sceVif1PkAddUpkData128(&pkt, giftagPoint);
    sceVif1PkAddCode(&pkt, VIF_UNPACK(0x2AD, 1, UNPACK_V4_32, 0));
    sceVif1PkAddUpkData128(&pkt, giftagPoint);

    pdstx = 0;
    pdsty = 0;
    for (i = 0; i < mbx; i++) {
        for (j = 0; j < mby; j++) {
            img = &imgbase[j + i * mby];
            nextbasep = (basep + 1) % 3;
            dstx = i * 256;
            dsty = j * 128;

            sceVif1PkCnt(&pkt, 0);
            sceVif1PkAddCode(&pkt, VIF_UNPACK(vu1_base_adr[basep] + 1, 2, UNPACK_V2_32, 0));
            sceVif1PkAddUpkData32(&pkt, vu1_base_adr[nextbasep]);
            sceVif1PkAddUpkData32(&pkt, 0);
            sceVif1PkAddUpkData32(&pkt, dstx - pdstx);
            sceVif1PkAddUpkData32(&pkt, dsty - pdsty);
            sceVif1PkAddCode(&pkt, VIF_STMASK(0));
            sceVif1PkAddData(&pkt, 0xBCBCBCBC);
            sceVif1PkRef(&pkt, DMA_ADDR(img), 0x10, 0,
                         VIF_UNPACK(vu1_base_adr[basep] + 3, 0, UNPACK_V4_8, 0) | UNPACK_UNSIGNED, 0);

            if (i < mbx - 1) {
                offset = 0;
                ir = i + 1;
            } else {
                offset = -7;
                ir = i;
            }

            /* Cb */
            if (j > 0) {
                src = (char *)imgbase[j + ir * mby - 1].cb[6];
            } else {
                src = (char *)imgbase[j + ir * mby].cb[0];
            }
            mkcntpacket(&pkt, VIF_STMASK(0), 0xF3F3F3F3);
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x13 + offset, 0x10, 0x10);
            mkrefpacket(&pkt, j < mby - 1 ? imgbase[j + ir * mby + 1].cb[0] : imgbase[j + ir * mby].cb[6],
                        UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x63 + offset, 0x10, 0x10);
            mkrefpacket(&pkt, imgbase[j + ir * mby].cb[0], UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x23 + offset, 0x40,
                        0x10);
            mkrefpacket(&pkt, j > 0 ? imgbase[j + i * mby - 1].cb[6] : img->cb[0], UNPACK_V4_8 | 0x60,
                        vu1_base_adr[basep] + 0xB3, 0x10, 0x10);
            mkrefpacket(&pkt, j < mby - 1 ? imgbase[j + i * mby + 1].cb[0] : img->cb[6], UNPACK_V4_8 | 0x60,
                        vu1_base_adr[basep] + 0x103, 0x10, 0x10);
            mkrefpacket(&pkt, img->cb[0], UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0xC3, 0x40, 0x10);

            /* Cr */
            if (j > 0) {
                src = (char *)imgbase[j + ir * mby - 1].cr[6];
            } else {
                src = (char *)imgbase[j + ir * mby].cr[0];
            }
            mkcntpacket(&pkt, VIF_STMASK(0), 0xCFCFCFCF);
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x13 + offset, 0x10, 0x10);
            mkrefpacket(&pkt, j < mby - 1 ? imgbase[j + ir * mby + 1].cr[0] : imgbase[j + ir * mby].cr[6],
                        UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x63 + offset, 0x10, 0x10);
            mkrefpacket(&pkt, imgbase[j + ir * mby].cr[0], UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x23 + offset, 0x40,
                        0x10);
            mkrefpacket(&pkt, j > 0 ? imgbase[j + i * mby - 1].cr[6] : img->cr[0], UNPACK_V4_8 | 0x60,
                        vu1_base_adr[basep] + 0xB3, 0x10, 0x10);
            mkrefpacket(&pkt, j < mby - 1 ? imgbase[j + i * mby + 1].cr[0] : img->cr[6], UNPACK_V4_8 | 0x60,
                        vu1_base_adr[basep] + 0x103, 0x10, 0x10);
            mkrefpacket(&pkt, img->cr[0], UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0xC3, 0x40, 0x10);

            sceVif1PkCnt(&pkt, 0);
            sceVif1PkAddCode(&pkt, VIF_FLUSH(0));
            sceVif1PkAddCode(&pkt, VIF_MSCNT(0));

            pdstx = dstx;
            pdsty = dsty;
            basep = nextbasep;
        }
    }

    sceVif1PkCnt(&pkt, 0);
    sceVif1PkAddCode(&pkt, VIF_MSCALF(0x7FC, 0));
    sceVif1PkEnd(&pkt, 0);
    sceVif1PkAddCode(&pkt, VIF_FLUSH(0));
    sceVif1PkOpenDirectCode(&pkt, 0);
    setGIFtag((unsigned int *)&giftagGsAD, 0xE, 1, 0, 0, 0, 1, 0);
    sceVif1PkOpenGifTag(&pkt, giftagGsAD);
    sceVif1PkAddGsAD(&pkt, GS_REG_FINISH, 0);
    sceVif1PkCloseGifTag(&pkt);
    sceVif1PkCloseDirectCode(&pkt);
    sceVif1PkAddCode(&pkt, VIF_FLUSH(0));
    sceVif1PkTerminate(&pkt);
}

/**
 * Builds in `tags` the VIF1 chain that converts and draws the `width` x `height` picture `image`
 * (RAW8 macroblocks) for progressive pictures (type 2).
 */
void cscVu1SetTag_prog(unsigned int *tags, void *image, int width, int height) {
    sceVif1Packet pkt;
    int i;
    int j;
    int dstx;
    int dsty;
    int pdstx;
    int pdsty;
    int basep;
    int nextbasep;
    int mbx;
    int mby;
    u_long128 giftagPoint;
    u_long128 giftagGsAD;
    CscRAW8 *imgbase;
    CscRAW8 *img;
    char *src;
    int ioffset;
    int joffset;
    int ir;

    basep = 1;
    mbx = width >> 4;
    mby = height >> 4;
    imgbase = image;

    sceVif1PkInit(&pkt, (u_long128 *)tags);
    sceVif1PkReset(&pkt);
    sceVif1PkCall(&pkt, (u_long128 *)vu1_init, 0);
    sceVif1PkCnt(&pkt, 0);
    sceVif1PkAddCode(&pkt, VIF_FLUSHE(0));
    sceVif1PkAddCode(&pkt, VIF_STCYCL(4, 4, 0));

    setGIFtag((unsigned int *)&giftagPoint, 0x51, 2, 0, 0, 1, 1, 0x80);
    sceVif1PkAddCode(&pkt, VIF_UNPACK(7, 1, UNPACK_V4_32, 0));
    sceVif1PkAddUpkData128(&pkt, giftagPoint);
    sceVif1PkAddCode(&pkt, VIF_UNPACK(0x15A, 1, UNPACK_V4_32, 0));
    sceVif1PkAddUpkData128(&pkt, giftagPoint);
    sceVif1PkAddCode(&pkt, VIF_UNPACK(0x2AD, 1, UNPACK_V4_32, 0));
    sceVif1PkAddUpkData128(&pkt, giftagPoint);

    pdstx = 0;
    pdsty = 0;
    for (i = 0; i < mbx; i++) {
        for (j = 0; j < mby; j++) {
            img = &imgbase[j + i * mby];
            nextbasep = (basep + 1) % 3;
            dstx = i * 256;
            dsty = j * 128;

            sceVif1PkCnt(&pkt, 0);
            sceVif1PkAddCode(&pkt, VIF_UNPACK(vu1_base_adr[basep] + 1, 2, UNPACK_V2_32, 0));
            sceVif1PkAddUpkData32(&pkt, vu1_base_adr[nextbasep]);
            sceVif1PkAddUpkData32(&pkt, 0);
            sceVif1PkAddUpkData32(&pkt, dstx - pdstx);
            sceVif1PkAddUpkData32(&pkt, dsty - pdsty);
            sceVif1PkAddCode(&pkt, VIF_STMASK(0));
            sceVif1PkAddData(&pkt, 0xBCBCBCBC);
            sceVif1PkRef(&pkt, DMA_ADDR(img), 0x10, 0,
                         VIF_UNPACK(vu1_base_adr[basep] + 3, 0, UNPACK_V4_8, 0) | UNPACK_UNSIGNED, 0);

            if (i < mbx - 1) {
                ioffset = 0;
                ir = i + 1;
            } else {
                ioffset = -7;
                ir = i;
            }

            /* Cb */
            if (j > 0) {
                src = (char *)imgbase[j + ir * mby - 1].cb[6];
                joffset = 0;
            } else {
                src = (char *)imgbase[j + ir * mby].cb[0];
                joffset = 8;
            }
            mkcntpacket(&pkt, VIF_STMASK(0), 0xF3F3F3F3);
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x13 + ioffset + joffset, 0x10, 0x10);
            if (j < mby - 1) {
                src = (char *)imgbase[j + ir * mby + 1].cb[0];
                joffset = 0;
            } else {
                src = (char *)imgbase[j + ir * mby].cb[6];
                joffset = -8;
            }
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x63 + ioffset + joffset, 0x10, 0x10);
            mkrefpacket(&pkt, imgbase[j + ir * mby].cb[0], UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x23 + ioffset, 0x40,
                        0x10);
            if (j > 0) {
                src = (char *)imgbase[j + i * mby - 1].cb[6];
                joffset = 0;
            } else {
                src = (char *)img->cb[0];
                joffset = 8;
            }
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0xB3 + joffset, 0x10, 0x10);
            if (j < mby - 1) {
                src = (char *)imgbase[j + i * mby + 1].cb[0];
                joffset = 0;
            } else {
                src = (char *)img->cb[6];
                joffset = -8;
            }
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x103 + joffset, 0x10, 0x10);
            mkrefpacket(&pkt, img->cb[0], UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0xC3, 0x40, 0x10);

            /* Cr */
            if (j > 0) {
                src = (char *)imgbase[j + ir * mby - 1].cr[6];
                joffset = 0;
            } else {
                src = (char *)imgbase[j + ir * mby].cr[0];
                joffset = 8;
            }
            mkcntpacket(&pkt, VIF_STMASK(0), 0xCFCFCFCF);
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x13 + ioffset + joffset, 0x10, 0x10);
            if (j < mby - 1) {
                src = (char *)imgbase[j + ir * mby + 1].cr[0];
                joffset = 0;
            } else {
                src = (char *)imgbase[j + ir * mby].cr[6];
                joffset = -8;
            }
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x63 + ioffset + joffset, 0x10, 0x10);
            mkrefpacket(&pkt, imgbase[j + ir * mby].cr[0], UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x23 + ioffset, 0x40,
                        0x10);
            if (j > 0) {
                src = (char *)imgbase[j + i * mby - 1].cr[6];
                joffset = 0;
            } else {
                src = (char *)img->cr[0];
                joffset = 8;
            }
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0xB3 + joffset, 0x10, 0x10);
            if (j < mby - 1) {
                src = (char *)imgbase[j + i * mby + 1].cr[0];
                joffset = 0;
            } else {
                src = (char *)img->cr[6];
                joffset = -8;
            }
            mkrefpacket(&pkt, src, UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0x103 + joffset, 0x10, 0x10);
            mkrefpacket(&pkt, img->cr[0], UNPACK_V4_8 | 0x60, vu1_base_adr[basep] + 0xC3, 0x40, 0x10);

            sceVif1PkCnt(&pkt, 0);
            sceVif1PkAddCode(&pkt, VIF_FLUSH(0));
            sceVif1PkAddCode(&pkt, VIF_MSCNT(0));

            pdstx = dstx;
            pdsty = dsty;
            basep = nextbasep;
        }
    }

    sceVif1PkCnt(&pkt, 0);
    sceVif1PkAddCode(&pkt, VIF_MSCALF(0x7FC, 0));
    sceVif1PkEnd(&pkt, 0);
    sceVif1PkAddCode(&pkt, VIF_FLUSH(0));
    sceVif1PkOpenDirectCode(&pkt, 0);
    setGIFtag((unsigned int *)&giftagGsAD, 0xE, 1, 0, 0, 0, 1, 0);
    sceVif1PkOpenGifTag(&pkt, giftagGsAD);
    sceVif1PkAddGsAD(&pkt, GS_REG_FINISH, 0);
    sceVif1PkCloseGifTag(&pkt);
    sceVif1PkCloseDirectCode(&pkt);
    sceVif1PkAddCode(&pkt, VIF_FLUSH(0));
    sceVif1PkTerminate(&pkt);
}

/**
 * Builds the conversion chain for `image` by picture `type`: 0 cscVu1SetTag_frfl(), 2
 * cscVu1SetTag_prog().
 */
void cscVu1SetTag(unsigned int *tags, int type, void *image, int width, int height) {
    switch (type) {
    case 0:
        cscVu1SetTag_frfl(tags, image, width, height);
        break;
    case 2:
        cscVu1SetTag_prog(tags, image, width, height);
        break;
    }
}

static void mkrefpacket(sceVif1Packet *pkt, void *src, unsigned int upkcmd, unsigned int vuaddr,
                        unsigned int vunum, unsigned int dmarate) {
    sceVif1PkRef(pkt, DMA_ADDR(src), vunum / dmarate, 0, (upkcmd << 24) | (vunum << 16) | UNPACK_UNSIGNED | vuaddr, 0);
}

static void mkcntpacket(sceVif1Packet *pkt, unsigned int code1, unsigned int code2) {
    sceVif1PkCnt(pkt, 0);
    sceVif1PkAddData(pkt, code1);
    sceVif1PkAddData(pkt, code2);
}
