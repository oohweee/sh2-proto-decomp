#ifndef GFW_HELPERS_H
#define GFW_HELPERS_H

/*
 * Plain-C helpers shared by the GFW drawing files (src/GFW). Include after sh2.h (for
 * u_long128).
 *
 * The header's name and the helpers' names are ours: they are inline-only (no DWARF), and the
 * original header is unknown. The bodies are the ones the matched files use, identical in each.
 */

/** Copies one quadword from src to dst (lq; sq). Both must be 16-byte aligned. */
static inline void CopyQword(void *dst, void *src) {
    *(u_long128 *)dst = *(u_long128 *)src;
}

#endif
