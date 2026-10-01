#ifndef FOG_PARAM_H
#define FOG_PARAM_H

/*
 * The two fog terms derived from Env_ctl.fogparm (near/far distances and densities), as inline
 * helpers. The names and this header are ours: inline-only, so they left no DWARF. The original
 * used helpers of this shape: the code that computes these terms matches only through them, and
 * its line table has one statement where plain expressions would need locals the DWARF lacks
 * (model3_n.c, ef_common.c, ef_rain.c).
 */

/** Slope of the fog value in w. */
static inline float FogParamA(float *f) {
    return f[0] * f[1] * (f[3] - f[2]) / (f[0] - f[1]);
}

/** Fog value at w = 0. */
static inline float FogParamB(float *f) {
    return (f[0] * f[2] - f[1] * f[3]) / (f[0] - f[1]);
}

#endif /* FOG_PARAM_H */
