#ifndef M3_HELPERS_H
#define M3_HELPERS_H

/*
 * Plain-C helpers shared by the player and Maria files (src/Chacter/m3_*.c).
 *
 * The header's name and the helpers' names are ours: they are inline-only (no DWARF), and the
 * original header is unknown. The bodies are the ones the matched files use, identical in each.
 */

/** Returns angle wrapped into [-PI, PI] (one step: angle must be within 3 * PI).
 *  Matching: `!= 0` materializes the first compare, as the original's code does. */
static inline float PlayerAngleWrap(float angle) {
    return ((angle > 3.1415927f) != 0) ? angle - 6.2831855f : ((angle < -3.1415927f) ? 6.2831855f + angle : angle);
}

#endif
