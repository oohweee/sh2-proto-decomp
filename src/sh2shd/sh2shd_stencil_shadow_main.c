/*
 * Per-frame entry points of the shadow renderer (sh2shd).
 */
#include "sh2.h"

/**
 * Casts stencil shadows from every loaded background map, lit by the
 * flashlight, except in two rooms that use a fixed light instead.
 * @param cam the camera
 * @return the shadow packet to send
 */
union Q_WORDDATA *stencil_shadow_main(struct sh2gfw_CAMERA *cam) {
    union Q_WORDDATA *ret_addr;
    void **list;
    void *addr;
    int i;
    struct SPOT_LIGHT spot;
    int glb_coord;
    int map_id;
    int light_kind;

    sh2gde_getspotParams_for_Shadow(spot.c, spot.zdir, spot.range);
    light_kind = 0;
    get_map_id(&glb_coord, &map_id);
    if (glb_coord == 12 && map_id == 91) {
        light_kind = 4;
        spot.zdir[0] = 0.07311368f;
        spot.zdir[1] = 0.683614f;
        spot.zdir[2] = 0.7261726f;
    } else if (glb_coord == 9 && map_id == 42 && !sh2gfw_Check_JmsSpotOnOff()) {
        light_kind = 0;
        spot.c[0] = -58350.0f;
        spot.c[1] = -700.0f;
        spot.c[2] = 19275.0f;
        spot.c[3] = 1.0f;
        spot.zdir[0] = -0.362f;
        spot.zdir[1] = 0.769f;
        spot.zdir[2] = 0.326f;
        spot.zdir[3] = 0.0f;
    }

    list = loadBgKG2_GetLoadedDataAddrList();
    for (i = 0; (addr = *list) != NULL; i++, list++) {
        sh2shd_add_map(i, addr, light_kind, spot.c, spot.zdir, spot.range);
    }
    ret_addr = sh2shd_exe_shadow(cam);
    return ret_addr;
}

/** Builds the drop (blob) shadows; returns their packet. */
union Q_WORDDATA *not_stencil_shadow_main(struct sh2gfw_CAMERA *cam) {
    union Q_WORDDATA *ret_addr;

    ret_addr = sh2shd_exe_drop_shadow(cam);
    return ret_addr;
}
