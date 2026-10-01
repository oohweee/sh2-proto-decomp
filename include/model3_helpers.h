#ifndef MODEL3_HELPERS_H
#define MODEL3_HELPERS_H

/*
 * Plain-C helpers shared by the model3 drawing files (src/Chacter_Draw/model3_*_n.c).
 *
 * The header's name and the helpers' names are ours: they are inline-only (no DWARF), and the
 * original header is unknown. The bodies are the ones the matched files use, identical in each.
 * model3_vu1_n.c has a Vif1 version of PkAddFloat, kept there.
 */

#include "sdk/libvifpk.h"

/** Adds f's bit pattern to the Vif0 packet pk as a data word. */
static inline void PkAddFloat(sceVif0Packet *pk, float f) {
    unsigned int d;

    d = *(unsigned int *)&f;
    sceVif0PkAddData(pk, d);
}

#endif
