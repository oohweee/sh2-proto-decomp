/*
 * View, screen and clip matrices for VU0 (GFW test code: the DWARF's gfw_test/camera.c).
 */
#include "sh2.h"
#include "sdk/libvu0.h"

/**
 * Builds the view-screen, view-clip and clip-screen matrices.
 * view_screen: view -> screen; view_clip: view -> clip volume;
 * clip_screen: clip volume -> screen. The screen is scrz in front of the eye,
 * scaled by (ax, ay) and centered on (cx, cy); z maps [nearz, farz] to
 * [zmax, zmin]. clip_vol holds the clip volume's half extents at scrz.
 */
void SetVu0ViewScreenClipMatrix(float view_screen[4][4], float view_clip[4][4], float clip_screen[4][4], float *clip_vol, float scrz, float ax, float ay, float cx, float cy, float zmin, float zmax, float nearz, float farz) {
    float az;
    float cz;
    float gsx;
    float gsy;
    float mt[4][4];

    gsx = nearz * clip_vol[0] / scrz;
    gsy = nearz * clip_vol[1] / scrz;
    cz = (-zmax * nearz + zmin * farz) / (-nearz + farz);
    az = farz * nearz * (-zmin + zmax) / (-nearz + farz);

    sceVu0UnitMatrix(view_screen);
    view_screen[0][0] = scrz;
    view_screen[1][1] = scrz;
    view_screen[2][2] = 0.0f;
    view_screen[3][3] = 0.0f;
    view_screen[3][2] = 1.0f;
    view_screen[2][3] = 1.0f;

    sceVu0UnitMatrix(mt);
    mt[0][0] = ax;
    mt[1][1] = ay;
    mt[2][2] = az;
    mt[3][0] = cx;
    mt[3][1] = cy;
    mt[3][2] = cz;
    sceVu0MulMatrix(view_screen, mt, view_screen);

    sceVu0UnitMatrix(view_clip);
    view_clip[0][0] = 2.0f * nearz / (gsx + gsx);
    view_clip[1][1] = 2.0f * nearz / (gsy + gsy);
    view_clip[2][2] = (farz + nearz) / (farz - nearz);
    view_clip[3][2] = -2.0f * (farz * nearz) / (farz - nearz);
    view_clip[2][3] = 1.0f;
    view_clip[3][3] = 0.0f;

    sceVu0UnitMatrix(clip_screen);
    clip_screen[0][0] = gsx * (scrz * ax) / nearz;
    clip_screen[1][1] = gsy * (scrz * ay) / nearz;
    clip_screen[2][2] = (-zmax + zmin) / 2.0f;
    clip_screen[3][2] = (zmax + zmin) / 2.0f;
    clip_screen[3][0] = cx;
    clip_screen[3][1] = cy;
    clip_screen[3][3] = 1.0f;
}
