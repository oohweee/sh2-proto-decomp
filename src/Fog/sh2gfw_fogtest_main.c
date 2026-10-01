/* sh2gfw_fogtest_main.c: per-frame setup of the fog (camera, matrices, fog objects). */

#include "sh2.h"
#include "sdk/libvu0.h"

/*
 * vcopy_gcc (asm_helpers.h) with float * parameters. Matching: kept local: with the header's void *
 * parameters the stand-in below no longer fixes the fogSetObj constant order.
 */
inline void vcopy_gcc_float(float *d, float *s) {
    __asm__ __volatile__("
    lq t7, 0(%1)
    sq t7, 0(%0)
    " : : "r"(d), "r"(s));
}

/*
 * Matching: fitted stand-in for float code (docs/stand-ins.md; found by tools/constcount.py, not
 * recovered): it makes the fogSetObj call load its 60.0f argument before `v1`.
 */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f + 17.0f + 19.0f + 21.0f + 23.0f + 25.0f + 27.0f + 29.0f + 31.0f + 33.0f; }
/**
 * Per-frame fog update: passes the camera, the fog position (the watch point or the player), the
 * matrices and the projection to the fog, moves the two fog objects that follow the player, then
 * moves the particles and builds their packets.
 */
void sh2gfw_fogtest_calcmain(void) {
    float ppos[4];
    float v0[4];
    float v1[4];
    float m0[4][4];

    vcopy_gcc_float(ppos, (float *)&GetPlayerInfoForCameraCtrl()->pos);
    vwGetViewPosition(v0);
    fogSetCameraPosV(v0);
    if (fwork.Global <= 5 || fwork.Global == 109) {
        vcGetNowWatchPos(v0);
        v0[1] = ppos[1];
        fogSetWorldPosV(v0);
    } else {
        fogSetWorldPosV(ppos);
    }
    sceVu0CopyMatrix(m0, cam0.world_screen);
    fogSetWorldScreenM(m0);
    sceVu0CopyMatrix(m0, VbWvsMatrix.wvm);
    fogSetWorldViewM(m0);
    fogSetProjection(VbScreenInfo.scr_z);

    /* Two fog objects follow the player: fog object 1 above, 2 higher up. */
    vcopy_gcc_float(v0, ppos);
    vcopy_gcc_float(v1, ppos);
    v0[1] -= 750.0f;
    v1[1] -= 820.0f;
    if (fogGetObj(1) == NULL) {
        fogSetObj2(1, v0, 110.0f);
        fogSetObj(2, v1, 60.0f);
    } else {
        fogMoveObj(1, v0);
        fogMoveObj(2, v1);
    }
    fogMoveParticle();
    fogMakePacket();
}

/** Empty in this build. */
void sh2gfw_init_fogTexture(void) {
}

/** Points the fog's TEX0 register at the current fog texture (effect texture 1). */
void sh2gfw_send_fogtex(void) {
    unsigned long *fog_Now_pTex0;

    fog_Now_pTex0 = fogTex0Adr();
    *fog_Now_pTex0 = HH_Effect_Object_Texture_GS_Register_Tex0_Get(1, 0);
}
