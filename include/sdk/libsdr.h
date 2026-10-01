#ifndef SDK_LIBSDR_H
#define SDK_LIBSDR_H

/*
 * libsdr, the EE library for remote calls into the IOP sound library (the binary's
 * "PsIIlibsdr  2200" stamp): the one function the game calls. Linked as assembly (no DWARF).
 *
 * Provenance: the function name is the binary's symbol; the types are inferred from the call sites
 * (the game passes 1, a command number and the command's arguments) and the parameter name is
 * ours. No SDK header or other SDK file was used.
 */

int sceSdRemote(int arg, ...);

#endif
