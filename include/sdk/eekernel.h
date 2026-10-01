#ifndef SDK_EEKERNEL_H
#define SDK_EEKERNEL_H

/*
 * The EE kernel calls the game uses: threads, semaphores, interrupt and DMA handlers, caches,
 * alarms. The library code (the binary links libkernl, its "PsIIlibkernl2240" stamp) is linked as
 * assembly, without DWARF. The game's own additions (CreateSema2, SignalSemaLimit...) are in
 * lib/sh_kernel.h.
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names are ours. The layouts of ThreadParam
 * and SemaParam are the DWARF's (sh2/types.h). The INTC_* and DMAC_* names and ExitHandler are
 * the names ps2sdk's kernel.h uses; the values are the EE's interrupt cause and DMA channel
 * numbers (public EE hardware documentation: PCSX2, ps2sdk). No SDK header or other SDK file was
 * used.
 */

#include "sh2/types.h"

/* Interrupt causes (INTC) and DMA channels (DMAC) for the handler functions. */
#define INTC_GS 0
#define INTC_VBLANK_S 2
#define INTC_VBLANK_E 3
#define DMAC_VIF1 1
#define DMAC_GIF 2

/*
 * The two instructions that end the game's interrupt handlers. MWCC's GCC-style asm needs them
 * separated by ';' (with "\n" it drops the ei).
 */
#define ExitHandler() asm volatile("sync.l; ei")

/* Threads */
int ChangeThreadPriority(int thread, int priority);
int CreateThread(struct ThreadParam *param);
int DeleteThread(int thread);
void Exit(int code);
int GetThreadId(void);
int ReferThreadStatus(int thread, struct ThreadParam *out);
int RotateThreadReadyQueue(int priority);
int StartThread(int thread, void *arg);
int TerminateThread(int thread);

/* Semaphores */
int CreateSema(struct SemaParam *param);
int DeleteSema(int sema);
int iSignalSema(int sema);
int SignalSema(int sema);
int WaitSema(int sema);

/* Interrupts and DMA channel handlers */
int AddDmacHandler(int channel, int (*fn)(void), int where);
int AddIntcHandler(int cause, int (*fn)(int), int where);
int AddIntcHandler2(int cause, int (*fn)(int, void *), int where, void *arg);
int DIntr(void);
int DisableDmac(int channel);
int DisableIntc(int cause);
int EIntr(void);
int EnableDmac(int channel);
int EnableIntc(int cause);
int RemoveIntcHandler(int cause, int handler);

/* Caches and alarms */
void FlushCache(int which);
void iFlushCache(int which);
void InvalidDCache(void *from, void *to);
int SetAlarm(int hlines, void (*fn)(void), void *arg);
void SyncDCache(void *from, void *to);

#endif
