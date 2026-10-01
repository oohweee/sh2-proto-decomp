/*
 * Emotion Engine and GS hardware registers used by the game: timers, IPU, GIF, VIF, DMA channels
 * and the GS privileged registers.
 *
 * Provenance: the addresses are the hardware's (public EE and GS documentation: PCSX2, ps2sdk).
 * The names join the unit and the register's hardware name (T0_COUNT, D1_CHCR, GS_CSR), as the
 * game's own debug strings do ("T0_COUNT Delta", "D1_CHCR", "VIF1_STAT"). The same convention is
 * common in public PS2 code (ps2sdk's ee_regs.h). No SDK header or other SDK file was used.
 */
#ifndef EEREGS_H
#define EEREGS_H

/* Timers */
#define T0_COUNT   ((volatile unsigned int *)0x10000000)
#define T0_MODE    ((volatile unsigned int *)0x10000010)
#define T1_COUNT   ((volatile unsigned int *)0x10000800)
#define T1_MODE    ((volatile unsigned int *)0x10000810)
/* Matching: the game reads timer 3 as 16 bits (lhu). */
#define T3_COUNT   ((volatile unsigned short *)0x10001800)

/* IPU */
#define IPU_CMD    ((volatile unsigned int *)0x10002000)
#define IPU_CTRL   ((volatile unsigned int *)0x10002010)
#define IPU_BP     ((volatile unsigned int *)0x10002020)

/* GIF */
#define GIF_MODE   ((volatile unsigned int *)0x10003010)

/* VIF0 / VIF1 */
#define VIF0_STAT  ((volatile unsigned int *)0x10003800)
#define VIF1_STAT  ((volatile unsigned int *)0x10003C00)
#define VIF1_ERR   ((volatile unsigned int *)0x10003C20)
#define VIF1_MARK  ((volatile unsigned int *)0x10003C30)
#define VIF1_FIFO  ((volatile u_long128 *)0x10005000)

/* DMA channel 0 (VIF0) */
#define D0_CHCR    ((volatile unsigned int *)0x10008000)
#define D0_MADR    ((volatile unsigned int *)0x10008010)
#define D0_QWC     ((volatile unsigned int *)0x10008020)
#define D0_TADR    ((volatile unsigned int *)0x10008030)
/* DMA channel 1 (VIF1) */
#define D1_CHCR    ((volatile unsigned int *)0x10009000)
#define D1_MADR    ((volatile unsigned int *)0x10009010)
#define D1_QWC     ((volatile unsigned int *)0x10009020)
#define D1_TADR    ((volatile unsigned int *)0x10009030)
/* DMA channel 2 (GIF) */
#define D2_CHCR    ((volatile unsigned int *)0x1000A000)
#define D2_MADR    ((volatile unsigned int *)0x1000A010)
#define D2_QWC     ((volatile unsigned int *)0x1000A020)
#define D2_TADR    ((volatile unsigned int *)0x1000A030)
/* DMA channel 3 (fromIPU) */
#define D3_CHCR    ((volatile unsigned int *)0x1000B000)
#define D3_MADR    ((volatile unsigned int *)0x1000B010)
#define D3_QWC     ((volatile unsigned int *)0x1000B020)
/* DMA channel 4 (toIPU) */
#define D4_CHCR    ((volatile unsigned int *)0x1000B400)
#define D4_MADR    ((volatile unsigned int *)0x1000B410)
#define D4_QWC     ((volatile unsigned int *)0x1000B420)
#define D4_TADR    ((volatile unsigned int *)0x1000B430)
/* DMA channel 8 (fromSPR) */
#define D8_CHCR    ((volatile unsigned int *)0x1000D000)
#define D8_MADR    ((volatile unsigned int *)0x1000D010)
#define D8_QWC     ((volatile unsigned int *)0x1000D020)
#define D8_SADR    ((volatile unsigned int *)0x1000D080)
/* DMA channel 9 (toSPR) */
#define D9_CHCR    ((volatile unsigned int *)0x1000D400)
#define D9_MADR    ((volatile unsigned int *)0x1000D410)
#define D9_QWC     ((volatile unsigned int *)0x1000D420)
#define D9_SADR    ((volatile unsigned int *)0x1000D480)
/* DMA controller */
#define D_STAT     ((volatile unsigned int *)0x1000E010)
#define D_ENABLER  ((volatile unsigned int *)0x1000F520)
#define D_ENABLEW  ((volatile unsigned int *)0x1000F590)

/* GS privileged registers (64-bit) */
#define GS_CSR     ((volatile unsigned long *)0x12001000)
#define GS_BUSDIR  ((volatile unsigned long *)0x12001040)
#define GS_SIGLBLID ((volatile unsigned long *)0x12001080)

#endif /* EEREGS_H */
