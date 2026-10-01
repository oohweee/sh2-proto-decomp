#ifndef SDK_LIBDMA_H
#define SDK_LIBDMA_H

/*
 * libdma, the EE library for the DMA controller (the binary's "PsIIlibdma  2200" stamp): the
 * functions the game calls. The library is linked as assembly (no DWARF).
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names are ours. sceDmaChan's layout is the
 * DWARF's (sh2/types.h). No SDK header or other SDK file was used.
 */

#include "sh2/types.h"

sceDmaChan *sceDmaGetChan(int channel);
void sceDmaSend(sceDmaChan *chan, void *tag);
void sceDmaSendN(sceDmaChan *chan, void *addr, int size);
int sceDmaSync(sceDmaChan *chan, int mode, int timeout);

#endif
