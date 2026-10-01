/*
 * Specular map texture for the character renderer (Chacter_Draw): builds a 64x64 highlight
 * texture (a pow(z, 8) spot) and uploads it to fixed GS memory, and returns its TEX0 value.
 */
#include "sh2.h"
#include "libc/math.h"
#include "sdk/eekernel.h"
#include "sdk/libgraph.h"

/** Returns the GS TEX0 register value of the specular map (base 0x35C0, 64x64, PSMCT32). */
unsigned long sh2_SpecularMappingTEX0(void) {
    int bp;
    int tbw;
    int tw;
    int th;
    unsigned long ul;

    bp = 0x35C0;
    tbw = 1;
    tw = 6;
    th = 6;
    ul = (unsigned long)bp | ((unsigned long)tbw << 14) | ((unsigned long)tw << 26) |
         ((unsigned long)th << 30) | (1UL << 34);
    return ul;
}

/*
 * Returns x clamped to [min, max], in C: not the max.s/min.s fclamp of asm_helpers.h.
 * Matching: inline, so its arguments are evaluated last to first (1.0f before the difference).
 */
static inline float fclamp_c(float x, float min, float max) {
    return (x < min) ? min : (x > max) ? max : x;
}

/**
 * Draws the 64x64 specular highlight into texbuf: gray pow(z, 8) of a unit hemisphere, alpha 128.
 * @param texbuf 64*64 32-bit texels
 */
void mdl_SpecularMappingMakeTexture(int *texbuf) {
    int w;
    int h;
    unsigned int *texture;
    float dy;
    float dy2;
    float dx;
    float dx2;
    float dz2;
    float dz;
    float i;
    float r;
    float g;
    float b;
    float a;
    int ir;
    int ig;
    int ib;
    int ia;

    texture = (unsigned int *)texbuf;
    for (h = 0; h < 64; h++) {
        dy = -1.0f + 2.0f * (h / 64.0f);
        dy2 = dy * dy;
        for (w = 0; w < 64; w++) {
            dx = -1.0f + 2.0f * (w / 64.0f);
            dx2 = dx * dx;
            dz2 = fclamp_c(1.0f - (dx2 + dy2), 0.0f, 1.0f);
            dz = sqrtf(dz2);
            i = powf(dz, 8.0f);
            r = g = b = 255.0f * i;
            a = 128.0f;
            ir = r;
            ig = ir;
            ib = ir;
            ia = a;
            texture[h * 64 + w] = (ia << 24) | (ib << 16) | (ig << 8) | ir;
        }
    }
}

/** Builds the specular map texture on the stack and uploads it to the GS over DMA channel 2 (PATH3). */
void mdl_SpecularMappingLoadTexture(void) {
    union Q_WORDDATA pqwd[16];
    int bp;
    int qwc;
    int w;
    int h;
    unsigned int texbuf[4096];

    bp = 0x35C0;
    w = 64;
    h = 64;
    qwc = w * h * sizeof(unsigned int) / 16;
    mdl_SpecularMappingMakeTexture((int *)texbuf);
    pqwd[0].ui32[0] = 0x10000006;
    pqwd[0].ui32[1] = 0;
    pqwd[0].ui32[2] = 0;
    pqwd[0].ui32[3] = 0;
    pqwd[1].ui32[3] = 0;
    pqwd[1].ui32[2] = 0xE;
    pqwd[1].ui32[1] = 0x10000000;
    pqwd[1].ui32[0] = 0x8004;
    pqwd[2].ul64[1] = 0x50;
    /* BITBLTBUF: SBW 1, DBP bp, DBW 1. */
    pqwd[2].ul64[0] = ((unsigned long)1 << 48) | ((unsigned long)bp << 32) | 0x10000;
    pqwd[3].ul64[1] = 0x51;
    pqwd[3].ul64[0] = 0;
    pqwd[4].ul64[1] = 0x52;
    pqwd[4].ui32[0] = w;
    pqwd[4].ui32[1] = h;
    pqwd[5].ul64[1] = 0x53;
    pqwd[5].ul64[0] = 0;
    pqwd[6].ui32[3] = 0;
    pqwd[6].ui32[2] = 0;
    pqwd[6].ui32[1] = 0x08000000;
    pqwd[6].ui32[0] = qwc | 0x8000;
    pqwd[7].ui32[0] = qwc | 0x30000000;
    pqwd[7].ui32[1] = (unsigned int)texbuf & 0x7FFFFFFF;
    pqwd[7].ui32[2] = 0;
    pqwd[7].ui32[3] = 0;
    pqwd[8].ul128 = 0;
    pqwd[8].ui32[0] = 0x70000000;
    FlushCache(0);
    sceGsSyncPath(0, 0);
    *D2_QWC = 0;
    *D2_TADR = (unsigned int)pqwd;
    *D2_CHCR = 0x105;
    sceGsSyncPath(0, 0);
}
