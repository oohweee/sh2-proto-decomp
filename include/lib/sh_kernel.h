#ifndef LIB_SH_KERNEL_H
#define LIB_SH_KERNEL_H

/*
 * The game's kernel additions: semaphore and thread calls the EE kernel lacks (each a jump to its
 * sh* implementation, e.g. CreateSema2 -> shCreateSema2) and a buffered printf. They are one
 * object, linked as assembly (lib/PollSemaAll in config/main.yaml) without DWARF, so the original
 * header is unknown and the name of this one is ours. The function names are the binary's
 * symbols; the signatures are inferred from the call sites (Multi_thr, sound, main.c) and the
 * parameter names are ours. The kernel's own calls are in sdk/eekernel.h.
 */

int CreateSema2(int init_count, int max_count, void *name);
int CreateThread2(void (*fn)(void *), void *stack, int stack_size, int priority, int option);
int iReleaseSema(int sema);
int iSignalSemaLimit(int sema);
int PollSemaAll(int sema);
int SignalSemaLimit(int sema);
int SignalSemaMax(int sema);

void printf_enable(void);
void printf_init(void *buf, int size, int mode);
void printf_skip(int skip);

#endif
