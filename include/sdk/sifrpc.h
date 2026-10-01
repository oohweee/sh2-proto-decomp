#ifndef SDK_SIFRPC_H
#define SDK_SIFRPC_H

/*
 * The SIF (EE-IOP interface) functions the game calls: RPC set-up and SIF DMA. The library code
 * is linked as assembly, without DWARF.
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names are ours. sceSifDmaData's layout is
 * the DWARF's (sh2/types.h). No SDK header or other SDK file was used.
 */

#include "sh2/types.h"

int sceSifDmaStat(unsigned int id);
void sceSifInitRpc(unsigned int mode);
unsigned int sceSifSetDma(struct sceSifDmaData *xfers, int count);

#endif
