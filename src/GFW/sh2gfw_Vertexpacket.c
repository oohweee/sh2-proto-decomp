/*
 * Background geometry packets (GFW): splits a triangle-strip geometry block into VU1-sized
 * batches, each with its GIF tags, microprogram parameters, the vertex data unpack and the
 * MSCAL, building the shared header quadwords in scratchpad memory first.
 */
#include "sh2.h"
#include "gfw_helpers.h"

#define SPR ((union Q_WORDDATA *)0x70000000)

static void GetPhongParm(struct sh2gfw_VU_HEAD *pV, union Q_WORDDATA *pq) {
    union Q_WORDDATA *qwd;
    float cs;
    float amax;
    float fldeg;
    float flb;

    qwd = (union Q_WORDDATA *)pV;
    amax = qwd->fl32[1];
    flb = qwd->fl32[0];
    amax = 1.0f + amax;
    fldeg = 84.15f / amax + 7.6499996f;
    cs = shCosF(fldeg * 3.1415 / 180.0);
    amax = 255.0f * flb / (1.08f - cs);
    pq->fl32[0] = amax;
    pq->fl32[1] = -amax * cs;
}

static void GetBlinnParm(struct sh2gfw_VU_HEAD *pV, union Q_WORDDATA *pq) {
    union Q_WORDDATA *qwd;
    float exponent;
    float bright;

    qwd = (union Q_WORDDATA *)pV;
    exponent = qwd->fl32[1];
    bright = qwd->fl32[0];
    pq->fl32[2] = 0.7f * bright;
    pq->fl32[3] = 0.05f * exponent;
}

/**
 * Writes the VU1 packets of an opaque geometry block, in batches of packsize vertices
 * (consecutive strip batches overlap by two vertices).
 * @param pV       the block's VU header (specular parameters)
 * @param sGE      the geometry header; the vertex data follows it
 * @param packsize vertices per batch
 * @param xgkick   VU1 address of the GIF packet
 * @param execaddr VU1 microprogram entry (selects the GIF tags and parameters)
 * @param ppqwd    write pointer, advanced past the packets
 */
/* Matching: `curr_vernor += (packsize - 2) * 16` spells next's expression out (next then has no register,
 * as in the DWARF); locals in a fitted order; the SPR[2] words go through qwd; `vNum = rest_vNum;` is a
 * reconstructed dead statement (docs/matching-notes.md#sh2gfw_vertexpacket-sh2gfw_geom_makepacket). */
void sh2gfw_Geom_MakePacket(struct sh2gfw_VU_HEAD *pV, struct sh2gfw_GEOM_HEAD *sGE, int packsize, int xgkick, int execaddr, Q_WORDDATA **ppqwd) {
    int dpacksize;
    int vNum;
    int rest_vNum;
    unsigned char *curr_vernor;
    union Q_WORDDATA *g_pack;
    int next;
    int tsl;
    unsigned char *curr_stcol;
    union Q_WORDDATA *qwd;
    union Q_WORDDATA *giftag;
    union Q_WORDDATA *headbuf;

    next = 0;
    g_pack = *ppqwd;
    rest_vNum = sGE->tsleng;
    sh2gfw_Add_VertexNum(rest_vNum);
    curr_vernor = (unsigned char *)(sGE + 1);
    tsl = packsize;
    if (rest_vNum % (packsize - 1) < 3) {
        packsize -= 2;
    }
    qwd = &SPR[2]; /* Matching: see above */
    qwd->ui32[3] = 0; qwd->ui32[2] = 0;
    qwd->ui32[1] = tsl * 3 + xgkick + 7;
    qwd->ui32[0] = xgkick;
    SPR[3].ui32[0] = 0x10000000;
    SPR[3].ui32[1] = 0;
    SPR[3].ui32[2] = 0;
    SPR[3].ui32[3] = 0;
    SPR[3].ui32[3] = 0x17000000;
    SPR[4].ui32[0] = packsize | 0x30000000;
    SPR[4].ui32[1] = 0;
    SPR[4].ui32[2] = 0;
    SPR[4].ui32[3] = (packsize << 17) | 0x6D008004;
    SPR[0].ui32[3] = 0;
    SPR[0].ui32[2] = 0x412;
    SPR[0].ui32[1] = (sGE->prim << 15) | 0x30004000;
    SPR[0].ui32[0] = packsize | 0x8000;
    switch (execaddr) {
    case 6:
        SPR[1].ui32[3] = 0;
        SPR[1].ui32[2] = 0x41;
        SPR[1].ui32[1] = 0x20064000;
        SPR[1].ui32[0] = packsize | 0x8000;
        break;
    case 0x10:
    case 0x18:
        GetBlinnParm(pV, &SPR[5]);
        GetPhongParm(pV, &SPR[5]);
        break;
    case 0x22:
    case 0x24:
        SPR[1].ui32[3] = 0;
        SPR[1].ui32[2] = 0x412;
        SPR[1].ui32[1] = 0x303E4000;
        SPR[1].ui32[0] = packsize | 0x8000;
        SPR[2].ui32[3] = 0;
        SPR[2].ui32[2] = 0;
        SPR[2].ui32[1] = sh2gfw_GetVuKick_Addr2(execaddr);
        SPR[2].ui32[0] = sh2gfw_GetVuKick_Addr(execaddr);
        GetBlinnParm(pV, &SPR[5]);
        GetPhongParm(pV, &SPR[5]);
        break;
    default:
        SPR[1].ui32[3] = 0;
        SPR[1].ui32[2] = 0x412;
        SPR[1].ui32[1] = 0x303E4000;
        SPR[1].ui32[0] = packsize | 0x8000;
        break;
    }
    if (rest_vNum < packsize) {
        vNum = rest_vNum; /* Matching: reconstructed, see above */
    } else {
        g_pack[0].ui32[0] = 0x10000004;
        g_pack[0].ui32[1] = 0;
        g_pack[0].ui32[2] = 0;
        g_pack[0].ui32[3] = 0;
        g_pack[0].ui32[2] = 0x1000101;
        g_pack[0].ui32[3] = 0x6C048000;
        g_pack[1] = SPR[0];
        g_pack[2] = SPR[2];
        g_pack[3] = SPR[1];
        g_pack[4] = SPR[5];
        g_pack[5].ui32[0] = packsize | 0x30000000;
        g_pack[5].ui32[1] = (unsigned int)curr_vernor;
        g_pack[5].ui32[2] = 0x1000202;
        g_pack[5].ui32[3] = (packsize << 17) | 0x6D008004;
        g_pack[6] = SPR[3];
        g_pack += 7;
        next = (packsize - 2) * 16;
        rest_vNum -= packsize - 2;
        if (packsize < rest_vNum) {
            curr_vernor += next;
            g_pack[0].ui32[0] = 0x10000004;
            g_pack[0].ui32[1] = 0;
            g_pack[0].ui32[2] = 0;
            g_pack[0].ui32[3] = 0;
            g_pack[0].ui32[2] = 0x1000101;
            g_pack[0].ui32[3] = 0x6C048000;
            g_pack[1] = SPR[0];
            g_pack[2] = SPR[2];
            g_pack[3] = SPR[1];
            g_pack[4] = SPR[5];
            g_pack[5].ui32[0] = packsize | 0x30000000;
            g_pack[5].ui32[1] = (unsigned int)curr_vernor;
            g_pack[5].ui32[2] = 0x1000202;
            g_pack[5].ui32[3] = (packsize << 17) | 0x6D008004;
            g_pack[6] = SPR[3];
            g_pack += 7;
            rest_vNum -= packsize - 2;
            while (rest_vNum > packsize) {
                curr_vernor += (packsize - 2) * 16; /* Matching: == next; see above */
                g_pack[0] = SPR[4];
                g_pack[0].ui32[1] = (unsigned int)curr_vernor;
                g_pack[1] = SPR[3];
                g_pack += 2;
                rest_vNum -= packsize - 2;
            }
        }
    }
    curr_vernor += next;
    SPR[0].ui32[0] = rest_vNum | 0x8000;
    SPR[1].ui32[0] = rest_vNum | 0x8000;
    g_pack[0].ui32[0] = 0x10000004;
    g_pack[0].ui32[1] = 0;
    g_pack[0].ui32[2] = 0;
    g_pack[0].ui32[3] = 0;
    g_pack[0].ui32[2] = 0x1000101;
    g_pack[0].ui32[3] = 0x6C048000;
    g_pack[1] = SPR[0];
    g_pack[2] = SPR[2];
    g_pack[3] = SPR[1];
    g_pack[4] = SPR[5];
    g_pack[5].ui32[0] = rest_vNum | 0x30000000;
    g_pack[5].ui32[1] = (unsigned int)curr_vernor;
    g_pack[5].ui32[2] = 0x1000202;
    g_pack[5].ui32[3] = (rest_vNum << 17) | 0x6D008004;
    g_pack[6] = SPR[3];
    g_pack[7].ul128 = 0;
    g_pack[7].ui32[0] = 0x60000000;
    *ppqwd = g_pack + 8;
}

/**
 * Writes the VU1 packets of a semi-transparent geometry block (see sh2gfw_Geom_MakePacket).
 * @param sGE    the semi-transparent geometry header; the vertex data follows it
 * @param gid    geometry id, stored in the packets' DMA tags
 * @param inf    quadword copied to the start of the packets
 * @param pkhead write pointer, advanced past the packets
 */
/* Matching: the GIF tag goes through SPR[3] (not giftag), next's expression is spelled out, and a
 * `(unsigned int)rest_vNum` cast is reconstructed from the DWARF (which of two places had it is unknown)
 * (docs/matching-notes.md#sh2gfw_vertexpacket-sh2gfw_semitrans_geompacket). */
void sh2gfw_SemiTrans_GeomPacket(struct sh2gfw_TRANSGEOM_HEAD *sGE, int gid, union Q_WORDDATA *inf, union Q_WORDDATA **pkhead) {
    int packsize;
    int dpacksize;
    int vNum;
    int rest_vNum;
    unsigned char *curr_vernor;
    union Q_WORDDATA *g_pack;
    int next;
    int geom_id;
    unsigned char *curr_stcol;
    union Q_WORDDATA *qwd;
    union Q_WORDDATA *giftag;
    union Q_WORDDATA *headbuf;

    next = 0;
    /* Matching: giftag is not used; the GIF tag is written as SPR[3], see above */
    g_pack = *pkhead;
    rest_vNum = sGE->vNum;
    sh2gfw_Add_SemiTransVertNum(rest_vNum);
    curr_vernor = (unsigned char *)(sGE + 1);
    packsize = 0x20;
    if (rest_vNum % (packsize - 1) < 3) {
        packsize -= 2;
    }
    SPR[2].ui32[3] = packsize;
    SPR[2].ui32[2] = 0;
    SPR[2].ui32[1] = packsize * 3 + 0x99;
    SPR[2].ui32[0] = 0x92;
    SPR[3].ui32[0] = 0x70000000;
    SPR[3].ui32[1] = 0;
    SPR[3].ui32[2] = 0x17000000;
    SPR[3].ui32[3] = 0x10000000;
    SPR[4].ui32[0] = packsize | 0x30000000 | (gid << 16);
    SPR[4].ui32[1] = 0;
    SPR[4].ui32[2] = 0;
    SPR[4].ui32[3] = (packsize << 17) | 0x6D000050;
    SPR[5].ui32[0] = packsize | 0x30000000 | (gid << 16);
    SPR[5].ui32[1] = 0;
    SPR[5].ui32[2] = packsize | 0x04000000;
    SPR[5].ui32[3] = (packsize << 17) | 0x6D000050;
    CopyQword(g_pack, inf);
    headbuf = g_pack;
    g_pack++;
    if (packsize <= rest_vNum) {
        g_pack[0].ui32[0] = packsize | 0x30000000;
        g_pack[0].ui32[1] = (unsigned int)curr_vernor;
        g_pack[0].ui32[2] = packsize | 0x04000000;
        g_pack[0].ui32[3] = (packsize << 17) | 0x6D000050;
        g_pack[0].uc8[2] = gid;
        g_pack[1] = SPR[3];
        g_pack[1].ui32[1] = packsize;
        g_pack += 2;
        next = (packsize - 2) * 16;
        rest_vNum -= packsize - 2;
        if (packsize < rest_vNum) {
            curr_vernor += next;
            g_pack[0].ui32[0] = packsize | 0x30000000;
            g_pack[0].ui32[1] = (unsigned int)curr_vernor;
            g_pack[0].ui32[2] = packsize | 0x04000000;
            g_pack[0].ui32[3] = (packsize << 17) | 0x6D000050;
            g_pack[0].uc8[2] = gid;
            g_pack[1] = SPR[3];
            g_pack[1].ui32[1] = packsize;
            g_pack += 2;
            rest_vNum -= packsize - 2;
            while (rest_vNum > packsize) {
                curr_vernor += (packsize - 2) * 16; /* Matching: == next; see above */
                g_pack[0] = SPR[5];
                g_pack[0].ui32[1] = (unsigned int)curr_vernor;
                g_pack[1] = SPR[3];
                g_pack[1].ui32[1] = packsize;
                g_pack += 2;
                rest_vNum -= packsize - 2;
            }
        }
    }
    curr_vernor += next;
    g_pack[0].ui32[0] = (unsigned int)rest_vNum | 0x30000000;
    g_pack[0].ui32[1] = (unsigned int)curr_vernor;
    g_pack[0].ui32[2] = rest_vNum | 0x04000000;
    g_pack[0].ui32[3] = (rest_vNum << 17) | 0x6D000050;
    g_pack[0].ui32[0] |= gid << 16;
    g_pack[1] = SPR[3];
    g_pack[1].uc8[3] |= 1;
    g_pack[1].ui32[1] = rest_vNum;
    headbuf->ui32[2] = (unsigned int)(g_pack + 2);
    headbuf->us16[2] = gid;
    *pkhead = g_pack + 2;
}
