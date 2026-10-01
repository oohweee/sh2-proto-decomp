/*
 * Ripples on the lake (test code, "miz" prefix): a ring of WAVE_NUM expanding circular waves
 * around a fixed centre, and the wave height/direction at a character's position.
 */

#include "sh2.h"
#include "libc/math.h"

#define WAVE_NUM 20

static const struct FVEC base_point = { -100.0f, 0.0f, -120.0f, 1.0f };

static int rand_seed;
struct _shLakeWaveInfo wave[WAVE_NUM];
int newest_wave;

/** Starts the WAVE_NUM waves at staggered ages and links them into a ring. */
void mizTestLakeWaveInit(void) {
    int i1;

    for (i1 = 0; i1 < WAVE_NUM; i1++) {
        wave[i1].timer = 4.0f * i1;
        wave[i1].distance[0] = 3.0f * wave[i1].timer;
        wave[i1].distance[1] = 3.0f + 0.03f * wave[i1].timer;
        wave[i1].energy = 0.5f - 0.015f * wave[i1].timer;
        wave[i1].prev = i1 + 1;
        wave[i1].next = i1 - 1;
    }
    wave[0].next = WAVE_NUM - 1;
    wave[WAVE_NUM - 1].prev = 0;
    newest_wave = 0;
    rand_seed = 0;
}

/**
 * Ages the waves (restarting those that reached their full radius) and finds the wave under
 * @p scp. @return that wave, with distance[0] set to its height there and distance[1] to its
 * direction (atan2 of the offset from the centre), or NULL if no wave reaches it.
 */
struct _shLakeWaveInfo *mizTestLakeWaveMain(struct SubCharacter *scp) {
    int i1;
    float pos_x;
    float pos_z;
    float angle;
    float distance;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 146
    assert_dw(newest_wave >= 0 && newest_wave < WAVE_NUM); /* Matching: do/while(0) form (its nop) */
    for (i1 = 0; i1 < WAVE_NUM; i1++) {
        wave[i1].timer += shGetDT();
        wave[i1].distance[0] = 3.0f * wave[i1].timer;
        wave[i1].distance[1] = 3.0f + 0.03f * wave[i1].timer;
        wave[i1].energy -= 0.015f * shGetDT();
        if (wave[i1].distance[0] >= 240.0f) {
            newest_wave = i1;
            wave[i1].distance[0] = 0.0f;
            wave[i1].distance[1] = 3.0f;
            wave[i1].timer = 0.0f;
            wave[i1].energy = 0.5f + 0.25f * sinf(3.1415927f * rand_seed / 180.0);
        }
    }

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 176
    assert_dw(newest_wave >= 0 && newest_wave < WAVE_NUM); /* Matching: do/while(0) form (its nop) */
    pos_x = scp->pos.x / 500.0f - base_point.x;
    pos_z = scp->pos.z / 500.0f - base_point.z;
    distance = sqrtf(pos_x * pos_x + pos_z * pos_z);
    i1 = newest_wave;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 190
    assert_dw(newest_wave >= 0 && newest_wave < WAVE_NUM); /* Matching: do/while(0) form (its nop) */
    while (wave[i1].prev != newest_wave) {
        if (wave[i1].distance[0] - wave[i1].distance[1] < distance &&
            distance <= wave[i1].distance[0] + wave[i1].distance[1]) {
            angle = 3.1415927f * (fabsf(wave[i1].distance[0] - distance) / wave[i1].distance[1]);
            wave[i1].distance[0] = wave[i1].energy + wave[i1].energy * cosf(angle);
            wave[i1].distance[1] = atan2f(pos_x, pos_z);
            rand_seed++;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 209
            assert_dw(newest_wave >= 0 && newest_wave < WAVE_NUM); /* Matching: do/while(0) form (its nop) */
            return &wave[i1];
        }
        i1 = wave[i1].prev;

        assert_dw(i1 >= 0 && i1 < WAVE_NUM); /* Matching: do/while(0) form (its nop) */
    }

    assert_dw(newest_wave >= 0 && newest_wave < WAVE_NUM); /* Matching: do/while(0) form (its nop) */
    return NULL;
}
