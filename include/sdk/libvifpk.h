#ifndef SDK_LIBVIFPK_H
#define SDK_LIBVIFPK_H

/*
 * The EE packet library's VIF0/VIF1 functions the game calls (sceVif0Pk*, sceVif1Pk*): they build
 * DMA packets of VIF codes and data. Also the VIF codes the game writes with sceVif1PkAddCode.
 * The library is linked as assembly (no DWARF).
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites, and sceVif0PkRefMpg's `unsigned short` from the library's
 * own code, which zero-extends that argument (andi 0xFFFF); the parameter names are ours (the call
 * sites show what most carry: e.g. the game passes VIF codes as sceVif*PkRef's fourth and fifth
 * arguments).
 * The packet structures have the DWARF's layouts (sh2/types.h). The VIF code layouts are the VIF
 * hardware's (public documentation: PCSX2, ps2sdk) and agree with the values in the binary; the
 * VIF_* macro names are ours. No SDK header or other SDK file was used.
 */

#include "sh2/types.h"

void sceVif0PkAddCode(sceVif0Packet *pkt, unsigned int code);
void sceVif0PkAddData(sceVif0Packet *pkt, unsigned int data);
void sceVif0PkCall(sceVif0Packet *pkt, u_long128 *addr, unsigned int flags);
void sceVif0PkCnt(sceVif0Packet *pkt, unsigned int flags);
void sceVif0PkEnd(sceVif0Packet *pkt, unsigned int flags);
void sceVif0PkInit(sceVif0Packet *pkt, u_long128 *buf);
void sceVif0PkRef(sceVif0Packet *pkt, u_long128 *addr, unsigned int qwc, unsigned int code0,
                  unsigned int code1, unsigned int flags);
void sceVif0PkRefMpg(sceVif0Packet *pkt, unsigned short vu_addr, u_long128 *code, unsigned int size,
                     unsigned int flags);
unsigned int *sceVif0PkReserve(sceVif0Packet *pkt, unsigned int count);
u_long128 *sceVif0PkTerminate(sceVif0Packet *pkt);

void sceVif1PkAddCode(sceVif1Packet *pkt, unsigned int code);
void sceVif1PkAddData(sceVif1Packet *pkt, unsigned int data);
void sceVif1PkAddGsAD(sceVif1Packet *pkt, unsigned int reg, unsigned long data);
void sceVif1PkAddGsData(sceVif1Packet *pkt, unsigned long data);
void sceVif1PkAddUpkData128(sceVif1Packet *pkt, u_long128 data);
void sceVif1PkAddUpkData32(sceVif1Packet *pkt, unsigned int data);
void sceVif1PkCall(sceVif1Packet *pkt, u_long128 *addr, unsigned int flags);
void sceVif1PkCloseDirectCode(sceVif1Packet *pkt);
void sceVif1PkCloseGifTag(sceVif1Packet *pkt);
void sceVif1PkCnt(sceVif1Packet *pkt, unsigned int flags);
void sceVif1PkEnd(sceVif1Packet *pkt, unsigned int flags);
void sceVif1PkInit(sceVif1Packet *pkt, u_long128 *buf);
void sceVif1PkOpenDirectCode(sceVif1Packet *pkt, int stall);
void sceVif1PkOpenGifTag(sceVif1Packet *pkt, u_long128 giftag);
void sceVif1PkRef(sceVif1Packet *pkt, u_long128 *addr, unsigned int qwc, unsigned int code0,
                  unsigned int code1, unsigned int flags);
unsigned int *sceVif1PkReserve(sceVif1Packet *pkt, unsigned int count);
void sceVif1PkReset(sceVif1Packet *pkt);
u_long128 *sceVif1PkTerminate(sceVif1Packet *pkt);

/*
 * VIF codes: IMMEDIATE (bits 0-15), NUM (16-23), CMD (24-30), and the interrupt bit (31).
 * Matching: each macro keeps the expression shape the game's code needs; VIF_FIELD only renames
 * the cast and shift.
 */
#define VIF_FIELD(v, pos) ((unsigned int)(v) << (pos))

#define VIF_FLUSH(irq) ((0x11 << 24) | VIF_FIELD(irq, 31))
#define VIF_FLUSHE(irq) ((0x10 << 24) | VIF_FIELD(irq, 31))
#define VIF_MSCALF(vu_addr, irq) ((unsigned int)(vu_addr) | (0x15 << 24) | VIF_FIELD(irq, 31))
#define VIF_MSCNT(irq) ((0x17 << 24) | VIF_FIELD(irq, 31))
#define VIF_STCYCL(wl, cl, irq) ((unsigned int)(cl) | VIF_FIELD(wl, 8) | (0x01 << 24) | VIF_FIELD(irq, 31))
#define VIF_STMASK(irq) ((0x20 << 24) | VIF_FIELD(irq, 31))
#define VIF_UNPACK(vu_addr, num, fmt, irq) \
    ((unsigned int)(vu_addr) | VIF_FIELD(num, 16) | VIF_FIELD(0x60 | (fmt), 24) | VIF_FIELD(irq, 31))

#endif
