#ifndef SDK_LIBGIFPK_H
#define SDK_LIBGIFPK_H

/*
 * The EE packet library's GIF (PATH3) functions the game calls (sceGifPk*): they build DMA
 * packets of GIF tags and A+D data. The library is linked as assembly (no DWARF).
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names are ours (the game passes zeros for
 * sceGifPkEnd's last three). sceGifPacket's layout is the DWARF's (sh2/types.h). No SDK header
 * or other SDK file was used.
 */

#include "sh2/types.h"

void sceGifPkAddGsAD(sceGifPacket *pkt, unsigned int reg, unsigned long data);
void sceGifPkCloseGifTag(sceGifPacket *pkt);
void sceGifPkEnd(sceGifPacket *pkt, unsigned int arg1, unsigned int arg2, unsigned int arg3);
void sceGifPkInit(sceGifPacket *pkt, u_long128 *buf);
void sceGifPkOpenGifTag(sceGifPacket *pkt, u_long128 giftag);
void sceGifPkReset(sceGifPacket *pkt);
u_long128 *sceGifPkTerminate(sceGifPacket *pkt);

#endif
