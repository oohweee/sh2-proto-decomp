#ifndef FOG_HELPERS_H
#define FOG_HELPERS_H

/*
 * Copy helpers shared by the fog files (src/Fog: fog.c, fog_blow.c, fogdata.c).
 *
 * The header's name and the helpers' names are ours: these are inline-only, so they left no
 * DWARF, and the original header they came from is unknown. The bodies are the ones the matched
 * files use; each appears inline, in this form, in more than one fog file. Like asm_helpers.h,
 * they are MWCC native `asm { }` blocks.
 */

/** Copies a FOG_PART_DATA (6 qwords, 0x60 bytes) from s to d through t5/t6/t7. */
inline void fogCopyPart(void *s, void *d) {
    asm {
        lq t5, 0x0(s)
        lq t6, 0x10(s)
        lq t7, 0x20(s)
        sq t5, 0x0(d)
        sq t6, 0x10(d)
        sq t7, 0x20(d)
        lq t5, 0x30(s)
        lq t6, 0x40(s)
        lq t7, 0x50(s)
        sq t5, 0x30(d)
        sq t6, 0x40(d)
        sq t7, 0x50(d)
    }
}

/** Copies a FOG_OBJ_DATA (3 qwords, 0x30 bytes) from s to d through t0/t1/t2. */
inline void fogCopyObj(void *s, void *d) {
    asm {
        lq t0, 0x0(s)
        lq t1, 0x10(s)
        lq t2, 0x20(s)
        sq t0, 0x0(d)
        sq t1, 0x10(d)
        sq t2, 0x20(d)
    }
}

#endif
