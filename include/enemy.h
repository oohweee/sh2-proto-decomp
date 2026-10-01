#ifndef ENEMY_H
#define ENEMY_H

/*
 * Helpers shared by the enemy AI files (src/Enemy/en_*.c), collected from the matched code of
 * en_bos, en_ike, en_pap and en_nse. The original enemy files shared a local header like this
 * (identical helper code in every file). Inline functions generate no code unless used, so
 * including this usually changes nothing by itself; the exceptions are en_fly.c and en_nse.c,
 * where the extra definitions change MWCC's float-constant order, so they keep renamed copies
 * (`*_local`, see their notes). A file whose call sites need a different variant should define that
 * variant locally under another name.
 *
 * The general helpers (fabsf, ftoi, itof, fsqrt, vcopy, vcopy3, mcopy, iabs, lengthXYZ, distXZ,
 * distXYZ, _shSign, the _sh* vector operations) come from the shared headers; only the
 * enemy-specific ones are defined here: the distances enDistXZ and enDist, as the native asm
 * blocks the enemy files use.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "sh_vu0.h"

typedef struct EnANIME_DATA EnANIME_DATA;

/* Set a new sub level (slv) and restart its step (sslv).
 * Matching: the do/while leaves the original's extra nop when this ends a body that falls into a
 * join; some sites wrote the two stores out instead (no nop), so check each one. */
#define EN_SET_LEVEL(dp, lv) do { (dp)->slv = (lv); (dp)->sslv = 0; } while (0)

#include "math_const.h"

/* Appends a 64-bit word to the sorted packet (spack). */
#define PK_ADD(v) (*spack.pos++ = (v))

/** Returns the XZ distance between points a and b, sqrt((a.x - b.x)^2 + (a.z - b.z)^2), on the FPU.
 *  asm_helpers.h's distXZ(b, a) is the same asm with the parameters the other way round.
 *  Matching: both are needed: inline-call arguments are evaluated last to first, so writing these
 *  calls as distXZ(b, a) changes the code of 17 enemy functions (see distXZ). */
inline float enDistXZ(float *a, float *b) {
    float d;

    asm {
        lwc1   d, 0(a)
        lwc1   $f8, 0(b)
        lwc1   $f9, 8(a)
        lwc1   $f10, 8(b)
        sub.s  d, d, $f8
        sub.s  $f9, $f9, $f10
        mula.s d, d
        madd.s d, $f9, $f9
        sqrt.s d, d
    }
    return d;
}

/** Returns the distance between points a and b (xyz), on the FPU (asm_helpers.h's distXYZ(b, a)
 *  with the parameters the other way round; both are needed, as for enDistXZ: 2 functions of
 *  en_insect.c change with distXYZ). */
inline float enDist(float *a, float *b) {
    float d;

    asm {
        lwc1    d, 0(a)
        lwc1    $f8, 0(b)
        lwc1    $f9, 4(a)
        sub.s   d, d, $f8
        lwc1    $f10, 4(b)
        mula.s  d, d
        lwc1    d, 8(a)
        lwc1    $f8, 8(b)
        sub.s   $f9, $f9, $f10
        sub.s   d, d, $f8
        madda.s $f9, $f9
        madd.s  d, d, d
        sqrt.s  d, d
    }
    return d;
}

#endif
