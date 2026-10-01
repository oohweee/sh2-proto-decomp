/*
 * GS control DMA chain (GFW): builds the chain in GSCTL_man out of DMA tags. Tag ids: 0x3 ref,
 * 0x5 call, 0x7 end; the second word of a ref/call is the address, the high dword the VIF
 * codes (0x50 DIRECT).
 */
#include "sh2.h"

/** Resets a GS control chain to the start of GSENV_CTL_PACK. */
void sh2gfw_init_GSPACKMAN(struct sh2gfw_GSPACK_MAN *Gman) {
    Gman->GS_kick = GSENV_CTL_PACK;
    Gman->GS_tail = GSENV_CTL_PACK;
    Gman->main_num = 0;
    Gman->transnum = 0;
}

/** Appends a ref tag that sends the GIF packet gt (NLOOP + 1 quadwords) with DIRECT. */
void sh2gfw_setREF_gsctl(union Q_WORDDATA *gt) {
    union Q_WORDDATA *qwd;
    unsigned short leng;

    qwd = GSCTL_man.GS_tail;
    leng = (gt->ss16[0] & 0x7FFF) + 1;
    qwd->ui32[0] = leng | 0x30000000;
    qwd->ui32[1] = (unsigned int)gt & 0x7FFFFFFF;
    qwd->ui32[2] = 0;
    qwd->ui32[3] = 0;
    qwd->ui32[3] = leng | 0x50000000;
    GSCTL_man.GS_tail = qwd + 1;
}

/**
 * sh2gfw_setREF_gsctl for any chain.
 * @param pqwd the chain's write pointer, advanced past the tag
 * @param gt   the GIF packet
 */
void sh2gfw_setREF_tagchain(union Q_WORDDATA **pqwd, union Q_WORDDATA *gt) {
    union Q_WORDDATA *qwd;
    unsigned short leng;

    qwd = *pqwd;
    leng = (gt->ss16[0] & 0x7FFF) + 1;
    qwd->ui32[0] = leng | 0x30000000;
    qwd->ui32[1] = (unsigned int)gt & 0x7FFFFFFF;
    qwd->ui32[2] = 0;
    qwd->ui32[3] = 0;
    qwd->ui32[3] = leng | 0x50000000;
    *pqwd = qwd + 1;
}

/** Closes the chain with an end tag; returns its start and begins a new one after it. */
union Q_WORDDATA *sh2gfw_setEND_gsctl(void) {
    union Q_WORDDATA *qwd;

    qwd = GSCTL_man.GS_tail;
    qwd->ui32[0] = 0x70000000;
    qwd->ui32[1] = 0;
    qwd->ul64[1] = 0;
    GSCTL_man.GS_tail = qwd + 1;
    qwd = GSCTL_man.GS_kick;
    GSCTL_man.GS_kick = GSCTL_man.GS_tail;
    return qwd;
}

/** Appends a call tag to the DMA chain pf. */
void sh2gfw_setCALL_gsctl(union Q_WORDDATA *pf) {
    union Q_WORDDATA *qwd;

    qwd = GSCTL_man.GS_tail;
    qwd->ui32[0] = 0x50000000;
    qwd->ui32[1] = (unsigned int)pf;
    qwd->ui32[2] = 0x07009999;
    qwd->ui32[3] = 0;
    GSCTL_man.GS_tail = qwd + 1;
}

/** Appends a ref tag that sends the dummy TEXFLUSH GIF packet (texture sync). */
void sh2gfw_setREF_TEXFLUSH(void) {
    union Q_WORDDATA *qwd;

    qwd = GSCTL_man.GS_tail;
    qwd->ui32[0] = 0x30000002;
    qwd->ui32[1] = (unsigned int)shGs_AllEnv.GsSync_DummyTEXFLUSH;
    qwd->ui32[2] = 0x11000000;
    qwd->ui32[3] = 0x50000002;
    GSCTL_man.GS_tail = qwd + 1;
}
