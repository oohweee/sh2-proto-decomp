#ifndef SH2_GET_DRAWENV_H
#define SH2_GET_DRAWENV_H

/*
 * Drawing-environment getters (original: src\GFW\sh2_get_drawenv.h).
 *
 * Known from the DWARF and the symbol table: sh2gde_getWorldScreenMatrix (lines 142-148) and
 * sh2gde_getWorldViewMatrix (lines 157-159) are defined here; their out-of-line copies (in
 * lens_flare.c) are global with ELF binding 13 (STB_LOPROC), the binding MWCC gives the copy of a
 * non-static `inline` function (a plain function gets 1), so they are `inline`. The bodies are
 * the copies'. The rest of the original header's content is unknown (presumably declarations of
 * GFW/sh2_get_drawenv.c's sh2gde_* functions, which sh2.h already has).
 */

#include "sh2.h"

#include "sdk/libvu0.h"

/** Copies the camera's world-to-screen matrix to wsm. */
/* Matching: each #line puts a definition on its original line (the line tables of the copies). */
#line 142
inline void sh2gde_getWorldScreenMatrix(float (*wsm)[4]) {
    sceVu0CopyMatrix(wsm, cam0.world_screen);
}

/** Copies the world-to-view matrix to wsm. */
#line 157
inline void sh2gde_getWorldViewMatrix(float (*wsm)[4]) {
    sceVu0CopyMatrix(wsm, VbWvsMatrix.wvm);
}

#endif
