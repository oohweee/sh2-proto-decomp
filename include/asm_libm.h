#ifndef ASM_LIBM_H
#define ASM_LIBM_H

/*
 * Inline-asm stand-ins for libm functions: fabsf, fmaxf and fminf as single FPU instructions.
 *
 * As with asm_helpers.h, the original header and names are unknown (inline-only functions leave
 * no DWARF); the bodies are the ones the matched files use. They are kept apart from
 * asm_helpers.h because they shadow the library functions: some files (anime, lake_wave,
 * m3_boat, light_n, loadbg_chara) call the real fabsf (`jal fabsf`) and must not include this
 * header. No file in the binary uses both.
 */

/** Returns |x| (abs.s). */
inline float fabsf(float x) {
    asm {
        abs.s x, x
    }
    return x;
}

/** Returns the larger of a and b (max.s). */
inline float fmaxf(float a, float b) {
    asm {
        max.s a, a, b
    }
    return a;
}

/** Returns the smaller of a and b (min.s). */
inline float fminf(float a, float b) {
    asm {
        min.s a, a, b
    }
    return a;
}

#endif
