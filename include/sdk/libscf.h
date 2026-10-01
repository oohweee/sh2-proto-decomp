#ifndef SDK_LIBSCF_H
#define SDK_LIBSCF_H

/*
 * libscf, the EE library for the console's system configuration (language, aspect, time zone;
 * the binary's "PsIIlibscf  2200" stamp): the functions the game calls. Linked as assembly (no
 * DWARF).
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names are ours. sceScfT10kConfig and
 * sceCdCLOCK have the DWARF's layouts (sh2/types.h). No SDK header or other SDK file was used.
 */

#include "sh2/types.h"

int sceScfGetAspect(void);
int sceScfGetDateNotation(void);
int sceScfGetLanguage(void);
void sceScfGetLocalTimefromRTC(struct sceCdCLOCK *clock);
int sceScfGetSpdif(void);
int sceScfGetSummerTime(void);
int sceScfGetTimeNotation(void);
int sceScfGetTimeZone(void);
void sceScfSetT10kConfig(struct sceScfT10kConfig *config);

#endif
