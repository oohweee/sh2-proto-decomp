#ifndef SDK_LIBPAD_H
#define SDK_LIBPAD_H

/*
 * libpad, the EE controller library (the binary's "PsIIlibpad  2200" stamp): the one function the
 * game calls directly (the rest goes through the game's libShPad, lib/libShPad.h). Linked as
 * assembly (no DWARF).
 *
 * Provenance: the function name is the binary's symbol; the types are inferred from the call site
 * and the parameter name is ours. No SDK header or other SDK file was used.
 */

int scePadSetWarningLevel(int level);

#endif
