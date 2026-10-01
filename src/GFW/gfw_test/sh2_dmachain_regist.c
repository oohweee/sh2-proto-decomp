/*
 * DMA chain building blocks for the background renderer (GFW test code, used by the game):
 * tags that call draw packets, load VU1 microcode, and send matrices, GS registers and lights
 * to VU1 memory.
 */
#include "sh2.h"

extern unsigned int SH2GFW_MF_LOAD_HEAD;

/**
 * Appends a next tag to the packet at addr (with a VIF MARK of itnum) and a cnt tag with a
 * small GIF packet, and writes an end tag at quadword tagtail of the packet at addr.
 * @param ppqwd   the chain's write pointer, advanced past the four quadwords
 * @param addr    the packet the next tag points to
 * @param tagtail quadword offset of that packet's end tag
 * @param itnum   VIF MARK value
 */
void sh2gfw_registDrawTag(union Q_WORDDATA **ppqwd, unsigned int addr, unsigned int tagtail, unsigned int itnum) {
    union Q_WORDDATA *qwd;
    union Q_WORDDATA *pushad;

    qwd = *ppqwd;
    qwd[0].ui32[0] = 0x20000000;
    qwd[0].ui32[1] = addr;
    qwd[0].ui32[2] = (itnum & 0xFFFF) | 0x07000000;
    qwd[0].ui32[3] = 0;
    qwd[1].ui32[0] = 0x10000002;
    qwd[1].ui32[1] = 0;
    qwd[1].ui32[2] = 0x11000000;
    qwd[1].ui32[3] = 0x50000002;
    qwd[2].ui32[3] = 0;
    qwd[2].ui32[2] = 0xE;
    qwd[2].ui32[1] = 0x10000000;
    qwd[2].ui32[0] = 0x8001;
    qwd[3].ul64[1] = 0xF;
    *ppqwd = qwd + 4;
    pushad = (union Q_WORDDATA *)addr;
    qwd = pushad + tagtail;
    qwd->ui32[0] = 0x70000000;
    qwd->ui32[1] = 0;
    qwd->ul64[1] = 0;
}

/** Appends an end tag to the chain at *ppqwd. */
void sh2gfw_setEND_chain(union Q_WORDDATA **ppqwd) {
    union Q_WORDDATA *qwd;

    qwd = *ppqwd;
    qwd->ui32[0] = 0x70000000;
    qwd->ui32[1] = 0;
    qwd->ul64[1] = 0;
    *ppqwd = ++qwd;
}

/** Appends a call tag to the VU1 microcode load packet (SH2GFW_MF_LOAD_HEAD) with the VU1 BASE/OFFSET settings. */
void kari_sh2gfw_VUMICRO_registchain(union Q_WORDDATA **ppcurr) {
    union Q_WORDDATA *qwd;

    qwd = *ppcurr;
    qwd->ui32[0] = 0x50000000;
    qwd->ui32[1] = (unsigned int)&SH2GFW_MF_LOAD_HEAD;
    qwd->ui32[2] = sh2gfw_get_VuBase(0) | 0x03000000;
    qwd->ui32[3] = sh2gfw_get_VuOffset(0) | 0x02000000;
    *ppcurr = qwd + 1;
}

/** Appends a ref tag that unpacks the 16 matrix quadwords of VU1_PARMS to VU1 address 0x10. */
void sh2gfw_VUMATIRICES_registchain(struct sh2gfw_VU_PARMS *VU1_PARMS, union Q_WORDDATA **ppcurr) {
    union Q_WORDDATA *qwd;

    qwd = *ppcurr;
    qwd->ui32[0] = 0x30000010;
    qwd->ui32[1] = (unsigned int)VU1_PARMS & 0x7FFFFFFF;
    qwd->ui32[2] = 0;
    qwd->ui32[3] = 0;
    qwd->ui32[2] = 0x01000101;
    qwd->ui32[3] = 0x6C100010;
    *ppcurr = qwd + 1;
}

/** Appends a ref tag that unpacks the 8 GIF tag quadwords of VU1_PARMS to VU1 address 0x3A. */
void sh2gfw_VUGSREGS_registchain(struct sh2gfw_VU_PARMS *VU1_PARMS, union Q_WORDDATA **ppcurr) {
    union Q_WORDDATA *qwd;

    qwd = *ppcurr;
    qwd->ui32[0] = 0x30000008;
    qwd->ui32[1] = (unsigned int)&VU1_PARMS->GifTag_mskNORMAL & 0x7FFFFFFF;
    qwd->ui32[2] = 0;
    qwd->ui32[3] = 0;
    qwd->ui32[2] = 0x01000101;
    qwd->ui32[3] = 0x6C08003A;
    *ppcurr = qwd + 1;
}

/** Appends a FLUSH, the unpack of the block's 12 matrix quadwords to VU1 address 0, and an MSCAL 0x0E. */
void sh2gfw_regist_BLOCKMATRICES(struct sh2gfw_BLOCK_MAN *pB_man, union Q_WORDDATA **ppqwd) {
    union Q_WORDDATA *qwd;

    qwd = *ppqwd;
    qwd[0].ul128 = 0;
    qwd[0].ui32[0] = 0x10000000;
    qwd[0].ui32[3] = 0x11000000;
    qwd[1].ui32[0] = 0x3000000C;
    qwd[1].ui32[1] = (unsigned int)pB_man->Local_World & 0x7FFFFFFF;
    qwd[1].ui32[2] = 0;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0x01000101;
    qwd[1].ui32[3] = 0x6C0C0000;
    qwd[2].ui32[0] = 0x10000000;
    qwd[2].ui32[1] = 0;
    qwd[2].ui32[2] = 0x1400000E;
    qwd[2].ui32[3] = 0;
    *ppqwd = qwd + 3;
}

/** Appends a FLUSH and the unpack of the block's 18 light quadwords to VU1 address 0x28. */
void sh2gfw_regist_BLOCKLIGHTS(struct sh2gfw_BLOCK_MAN *pB_man, union Q_WORDDATA **ppqwd) {
    union Q_WORDDATA *qwd;

    qwd = *ppqwd;
    qwd[0].ul128 = 0;
    qwd[0].ui32[0] = 0x10000000;
    qwd[0].ui32[3] = 0x11000000;
    qwd[1].ui32[0] = 0x30000012;
    qwd[1].ui32[1] = (unsigned int)&pB_man->blk_LightData & 0x7FFFFFFF;
    qwd[1].ui32[2] = 0;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0x01000101;
    qwd[1].ui32[3] = 0x6C120028;
    *ppqwd = qwd + 2;
}
