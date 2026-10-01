/*
 * Character lighting (Chacter_Draw). A table of LIGHT_MAX lights (kind 1 parallel, 2 point,
 * 3 spot, 4 reflection; "fake" point and spot lights are parallel lights with a position) and
 * the matrices the model renderers take from it: per scene the normal-light (NLM), light-color
 * (LCM) and reflection (NHM) matrices, per model position the strongest point/spot lights.
 *
 * Matching: the #line directives in this file keep the assert strings on the original's lines.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "libc/math.h"
#include "libc/stdlib.h"
#include "libc/string.h"
#include "sdk/libvu0.h"

#define LIGHT_MAX 12
#define EXTRA_MAX 6

typedef unsigned int u_int;

struct LightWork light_work;

static float monochrome_vector[4] = {0.3f, 0.6f, 0.1f, 0.0f};

/** Returns x clamped to [l, h]. */
float ktClampFloat(float x, float l, float h) {
    return (x < l) ? l : ((x > h) ? h : x);
}

/** Returns light n (asserts n < LIGHT_MAX). */
#line 56
struct Light *LightPointer(int n) {

    assert_dw((u_int)n < LIGHT_MAX);
    return &light_work.lights[n];
}

static void UpdateIntensity(struct Light *l) {
    float *c = l->color;

    l->intensity2 = sceVu0InnerProduct(c, monochrome_vector);
    l->intensity = fabsf(l->intensity2);
}

static void UpdateFParam(struct Light *l) {
    float a;
    float b;

    a = 1.0f / (l->f_start - l->f_end);
    b = -(a * l->f_end);
    l->f_a = a;
    l->f_b = b;
    l->f_rb = l->f_start / (l->f_start - l->f_end);
    l->f_ra = -l->f_rb * l->f_end;
}

static void UpdateSParam(struct Light *l) {
    float a;
    float b;

    a = 1.0f / (l->s_start - l->s_end);
    b = -(a * l->s_end);
    l->s_a = a;
    l->s_b = b;
}

/** Switches light n off (kind 0). */
void LightDelete(int n) {
    struct Light *l;

    l = LightPointer(n);
    l->kind = 0;
}

/** Switches every light off. */
void LightDeleteAll(void) {
    int i;

    for (i = 0; i < LIGHT_MAX; i++) {
        LightDelete(i);
    }
}

/**
 * Makes light n a parallel light.
 * @param n     light number
 * @param dir   direction (normalized here)
 * @param color RGBA color
 */
void LightSetParallel(int n, float *dir, float *color) {
    struct Light *l;

    l = LightPointer(n);
    memset(l, 0, sizeof(struct Light));
    l->kind = 1;
    LightSetDir(n, dir);
    LightSetColor(n, color);
}

/**
 * Makes light n a point light.
 * @param n       light number
 * @param pos     position
 * @param color   RGBA color
 * @param f_start distance where the falloff starts
 * @param f_end   distance where the light reaches zero
 */
void LightSetPoint(int n, float *pos, float *color, float f_start, float f_end) {
    struct Light *l;

    l = LightPointer(n);
    memset(l, 0, sizeof(struct Light));
    l->kind = 2;
    LightSetPos(n, pos);
    LightSetDir(n, unit_fvector_z);
    LightSetColor(n, color);
    LightSetFalloff(n, f_start, f_end);
}

/** Like LightSetPoint, but the light is lit as a parallel light (kind 1, fakekind 1). */
void LightSetFakePoint(int n, float *pos, float *color, float f_start, float f_end) {
    struct Light *l;

    l = LightPointer(n);
    memset(l, 0, sizeof(struct Light));
    l->kind = 1;
    l->fakekind = 1;
    LightSetPos(n, pos);
    LightSetDir(n, unit_fvector_z);
    LightSetColor(n, color);
    LightSetFalloff(n, f_start, f_end);
}

/**
 * Makes light n a fake spot light: lit as a parallel light (kind 1, fakekind 2), with a
 * position, falloff and spread. The dir argument is not used; the direction is set to +z.
 * @param s_start spread where the cone falloff starts
 * @param s_end   spread where the cone reaches zero
 */
void LightSetFakeSpot(int n, float *pos, float *dir, float *color, float f_start, float f_end, float s_start, float s_end) {
    struct Light *l;

    l = LightPointer(n);
    memset(l, 0, sizeof(struct Light));
    l->kind = 1;
    l->fakekind = 2;
    LightSetPos(n, pos);
    LightSetDir(n, unit_fvector_z);
    LightSetColor(n, color);
    LightSetFalloff(n, f_start, f_end);
    LightSetSpread(n, s_start, s_end);
}

/** Attaches the draw-environment light record ded to light n. */
void sh_Set_DrawEnvLightData(int n, void *ded) {
    struct Light *plight;

    plight = LightPointer(n);
    plight->DrawEnv_LightData = ded;
}

/**
 * Makes light n a spot light from a packed parameter vector ("kari": provisional).
 * @param sv [0] cone spread, [2] falloff start, [3] falloff end
 */
void sh_Kari_LightSetSpot(int n, float *pos, float *dir, float *color, float *sv) {
    struct Light *l;

    l = LightPointer(n);
    memset(l, 0, sizeof(struct Light));
    l->kind = 3;
    LightSetPos(n, pos);
    LightSetDir(n, dir);
    LightSetColor(n, color);
    l->f_start = sv[2];
    l->f_end = sv[3];
    l->s_start = 1.0f;
    l->s_end = sv[0];
    l->f_rb = sv[2] / (sv[2] - sv[3]);
    l->f_ra = sv[3] * sv[2] / (sv[3] - sv[2]);
    l->s_a = 1.0f;
    l->s_b = 0.8f * -sv[0];
    l->f_a = 1.0f / (sv[2] - sv[3]);
    l->f_b = -l->f_a * sv[3];
}

/** Makes light n a reflection (specular) light with direction dir and color color. */
void LightSetReflection(int n, float *dir, float *color) {
    struct Light *l;

    l = LightPointer(n);
    memset(l, 0, sizeof(struct Light));
    l->kind = 4;
    LightSetDir(n, dir);
    LightSetColor(n, color);
}

/** Sets the position of light n. */
void LightSetPos(int n, float *pos) {
    struct Light *l;

    l = LightPointer(n);
    sceVu0CopyVector(l->pos, pos);
}

/** Sets the direction of light n, normalized; a zero vector gives +z. */
void LightSetDir(int n, float *dir) {
    struct Light *l;

    l = LightPointer(n);
    if (dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f) {
        sceVu0CopyVector(l->dir, unit_fvector_z);
    } else {
        sceVu0Normalize(l->dir, dir);
    }
}

/** Sets the color of light n and updates its intensity (luminance). */
void LightSetColor(int n, float *color) {
    struct Light *l;

    l = LightPointer(n);
    sceVu0CopyVector(l->color, color);
    UpdateIntensity(l);
}

/** Sets the distance falloff of light n (full at f_start, zero at f_end). */
void LightSetFalloff(int n, float f_start, float f_end) {
    struct Light *l;

    l = LightPointer(n);
    l->f_start = f_start;
    l->f_end = f_end;
    UpdateFParam(l);
}

/** Sets the cone falloff of spot light n (full at s_start, zero at s_end). */
void LightSetSpread(int n, float s_start, float s_end) {
    struct Light *l;

    l = LightPointer(n);
    l->s_start = s_start;
    l->s_end = s_end;
    UpdateSParam(l);
}

static void CalcInfluence(struct Light *l, float *center, float radius) {
    float diff[4];
    float dist;
    float f_inf;

    switch (l->kind) {
        case 0:
            l->influence = 0.0f;
            l->influence2 = 0.0f;
            break;
        case 1:
            l->inf_fac = 0.2f * l->color[1];
        case 4:
            l->influence = l->intensity;
            l->influence2 = l->intensity2;
            break;
        case 2:
        case 3:
            sceVu0SubVector(diff, center, l->pos);
            dist = sqrtf(sceVu0InnerProduct(diff, diff)) - radius;
            f_inf = ktClampFloat(l->f_b + l->f_a * dist, 0.0f, 1.0f);
            l->inf_fac = f_inf;
            if (!sh2gfw_Get_ChrClip_FLG() || !sh2gfw_Check_ClipOKChar(UniModelDW_Man.testSubChar)) {
                f_inf = 1.0f;
            }
            if (l->kind == 3) {
                _shNormalize(diff, diff);
                l->inf_fac *= fclamp(l->s_b + l->s_a * _shInnerProduct(diff, l->dir), 0.0f, 1.0f);
            }
            l->influence = l->intensity * f_inf;
            l->influence2 = l->intensity2 * f_inf;
            break;
    }
}

static int CompareByIntensity(void *xx, void *yy) {
    struct Light **xp;
    struct Light **yp;
    struct Light *x;
    struct Light *y;

    xp = xx;
    yp = yy;
    x = *xp;
    y = *yp;
    if (x->intensity < y->intensity) {
        return 1;
    }
    if (x->intensity > y->intensity) {
        return -1;
    }
    return 0;
}

static int CompareByInfluence(void *xx, void *yy) {
    struct Light **xp;
    struct Light **yp;
    struct Light *x;
    struct Light *y;

    xp = xx;
    yp = yy;
    x = *xp;
    y = *yp;
    if (x->influence < y->influence) {
        return 1;
    }
    if (x->influence > y->influence) {
        return -1;
    }
    return 0;
}

/** Transforms every active light's position and direction into view space. */
void UpdateViewParams(void) {
    float wvm[4][4];
    int i;
    struct Light *l;

    sceVu0CopyMatrix(wvm, VbWvsMatrix.wvm);
    for (i = 0; i < LIGHT_MAX; i++) {
        l = LightPointer(i);
        switch (l->kind) {
            case 0:
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                ktVu0ApplyMatrixXYZ1(l->vpos, wvm, l->pos);
                ktVu0ApplyMatrixXYZ0(l->vdir, wvm, l->dir);
                break;
        }
    }
}

/**
 * Builds the parallel-light matrices: the normal-light matrices (world and view space) and the
 * light-color matrices, three lights in the first and four in the second. Also passes the
 * parallel lights to the shadow code.
 */
void UpdateParallels(void) {
    struct Light *tmplight[12];
    int n_parallels;
    int matrix_no;
    int offset;
    int i;
    struct Light *plp;
    struct Light *l;
    float (*nlm)[4];
    float (*vnlm)[4];
    float (*lcm)[4];

    n_parallels = 0;
    light_work.n_valid_parallels = 0;
    light_work.n_valid_parallel_matrices = 0;
    for (i = 0; i < 2; i++) {
        sceVu0CopyMatrix(light_work.nlms[i], zero_fmatrix);
        sceVu0CopyMatrix(light_work.vnlms[i], zero_fmatrix);
        sceVu0CopyMatrix(light_work.lcms[i], zero_fmatrix);
    }
    for (i = 0; i < LIGHT_MAX; i++) {
        plp = LightPointer(i);
        switch (plp->kind) {
            case 1:
                tmplight[n_parallels] = plp;
                n_parallels++;
                break;
        }
    }
    sh2gfw_Store_ShadowParallelLight(tmplight, n_parallels);
    matrix_no = 0;
    offset = 0;
    for (i = 0; i < n_parallels; i++) {
        l = tmplight[i];
        switch (l->kind) {
            case 1:
                if (matrix_no <= 1) {
                    nlm = light_work.nlms[matrix_no];
                    vnlm = light_work.vnlms[matrix_no];
                    lcm = light_work.lcms[matrix_no];
                    nlm[0][offset] = l->dir[0];
                    nlm[1][offset] = l->dir[1];
                    nlm[2][offset] = l->dir[2];
                    vnlm[0][offset] = l->vdir[0];
                    vnlm[1][offset] = l->vdir[1];
                    vnlm[2][offset] = l->vdir[2];
                    lcm[offset][0] = l->color[0];
                    lcm[offset][1] = l->color[1];
                    lcm[offset][2] = l->color[2];
                    lcm[offset][3] = l->color[3];
                    offset++;
                    if ((offset == 3 && matrix_no == 0) || offset == 4) {
                        matrix_no++;
                        offset = 0;
                    }
                    light_work.n_valid_parallels++;
                }
                break;
        }
    }
    light_work.n_valid_parallel_matrices = (light_work.n_valid_parallels + 4) / 4;
}

/**
 * Builds the reflection terms from the brightest reflection light: its half vector goes into
 * column 3 of the first normal-light matrices, and the NHM matrices, brightness and color are
 * set from it.
 */
void UpdateReflections(void) {
    static float nhm3[4] = {0.5f, 0.5f, 0.0f, 1.0f};
    struct Light *a[12];
    int n_reflections;
    float wvm[4][4];
    float vwm[4][4];
    int i;
    struct Light *l;

    n_reflections = 0;
    light_work.reflection_brightness = 0.0f;
    sceVu0CopyVector(light_work.reflection_color, zero_fvector);
    sceVu0CopyMatrix(light_work.nhm, zero_fmatrix);
    for (i = 0; i < LIGHT_MAX; i++) {
        l = LightPointer(i);
        switch (l->kind) {
            case 4:
                a[n_reflections++] = l;
        }
    }
    if (n_reflections) {
        sceVu0CopyMatrix(wvm, VbWvsMatrix.wvm);
        sceVu0TransposeMatrix(vwm, wvm);
        vwm[2][3] = 0.0f;
        vwm[1][3] = 0.0f;
        qsort(a, n_reflections, sizeof(struct Light *), CompareByIntensity);
        {
            struct Light *l;
            float *dir_z;
            float hv[4];
            float vhv[4];
            float (*nlm)[4];
            float (*vnlm)[4];
            float (*nhm)[4];
            float (*vnhm)[4];
            float *dir_y;

            l = a[0];
            dir_z = vwm[2];
            sceVu0AddVector(hv, l->dir, dir_z);
            sceVu0Normalize(hv, hv);
            sceVu0AddVector(vhv, l->vdir, unit_fvector_z);
            sceVu0Normalize(vhv, vhv);
            nlm = light_work.nlms[0];
            vnlm = light_work.vnlms[0];
            nlm[0][3] = hv[0];
            nlm[1][3] = hv[1];
            nlm[2][3] = hv[2];
            vnlm[0][3] = vhv[0];
            vnlm[1][3] = vhv[1];
            vnlm[2][3] = vhv[2];
            light_work.reflection_brightness = sceVu0InnerProduct(l->color, monochrome_vector);
            nhm = light_work.nhm;
            vnhm = light_work.vnhm;
            dir_y = vwm[1];
            if (hv[0] == 0.0f && hv[1] == 0.0f && hv[2] == 0.0f) {
                hv[0] = 1.0f;
            }
            sceVu0OuterProduct(nhm[0], dir_y, hv);
            sceVu0OuterProduct(nhm[1], hv, nhm[0]);
            sceVu0Normalize(nhm[0], nhm[0]);
            sceVu0Normalize(nhm[1], nhm[1]);
            sceVu0CopyVector(nhm[2], hv);
            sceVu0CopyVector(nhm[3], unit_fvector_w);
            sceVu0TransposeMatrix(nhm, nhm);
            sceVu0CopyVector(nhm[3], nhm3);
            if (vhv[0] == 0.0f && vhv[1] == 0.0f && vhv[2] == 0.0f) {
                vhv[0] = 1.0f;
            }
            sceVu0OuterProduct(vnhm[0], unit_fvector_y, vhv);
            sceVu0OuterProduct(vnhm[1], vhv, vnhm[0]);
            sceVu0Normalize(vnhm[0], vnhm[0]);
            sceVu0Normalize(vnhm[1], vnhm[1]);
            sceVu0CopyVector(vnhm[2], vhv);
            sceVu0CopyVector(vnhm[3], unit_fvector_w);
            sceVu0TransposeMatrix(vnhm, vnhm);
            sceVu0CopyVector(vnhm[3], nhm3);
            sceVu0CopyVector(light_work.reflection_color, l->color);
        }
    }
}

static void UpdateExtras(float *center, float radius) {
    struct Light *a[12];
    int n_extras;
    int i;
    struct Light *l;

    n_extras = 0;
    light_work.n_valid_extras = 0;
    for (i = 0; i < LIGHT_MAX; i++) {
        l = LightPointer(i);
        switch (l->kind) {
            case 2:
            case 3:
                CalcInfluence(l, center, radius);
                if (l->influence > 0.0f) {
                    a[n_extras++] = l;
                }
        }
    }
    if (n_extras == 0) {
        if (sh2gfw_Get_NightOrDay() && !DramaDemoNumber()) {
            float ref_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

            sceVu0ScaleVector(light_work.reflection_color, ref_color, 0.2f);
        }
        return;
    }
    qsort(a, n_extras, sizeof(struct Light *), CompareByInfluence);
    light_work.n_valid_extras = n_extras = (n_extras <= 5) ? n_extras : 6;
    for (i = 0; i < light_work.n_valid_extras; i++) {
        struct Light *l = a[i];
        light_work.valid_extras[i] = l;
    }
    if (!Check_IgnoreJmsSpot_for_Shadow()) {
        sh2gfw_Store_ShadowLight(light_work.valid_extras[0]);
    } else {
        for (i = 0; i < light_work.n_valid_extras; i++) {
            if (light_work.valid_extras[i]->DrawEnv_LightData) {
                break;
            }
        }
        if (i < light_work.n_valid_extras) {
            sh2gfw_Store_ShadowLight(light_work.valid_extras[i]);
        }
    }
    if (sh2gfw_Check_DemoRefrectionHightLight() && !DramaDemoNumber()) {
        float ref_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

        sceVu0ScaleVector(light_work.reflection_color, ref_color, 1.0f);
    }
}
/* Matching: #line keeps the original numbering (an inline setter was folded into UpdateExtras). */
#line 582

/** Per-scene light update: view-space lights, parallel-light and reflection matrices. */
void LightUpdateInfoByScene(void) {
    UpdateViewParams();
    UpdateParallels();
    UpdateReflections();
}

/**
 * Per-model light update: picks the point and spot lights that reach the model (at most
 * EXTRA_MAX, strongest first) and passes one to the shadow code.
 * @param center model center
 * @param radius model bounding radius
 */
void LightUpdateInfoByPos(float *center, float radius) {
    UpdateExtras(center, radius);
}

/** Returns the number of parallel-light matrices in use. */
int LightNValidParallelMatrices(void) {
    return (light_work.n_valid_parallels + 3) / 4;
}

/*
 * Matching: stand-in for a function the original linker dead-stripped
 * (config/stripped_functions.txt; tools/mwcc_fixup.py empties it). Its "n < 2" literal,
 * pooled with the two accessors below, stays in .rodata ahead of their assert formats.
 * Name unknown; the body is a guess (another per-n matrix accessor).
 */
void __stripped_light_n_code(float (*npm)[4], int n) {
    assert_dw(n < 2);
    sceVu0CopyMatrix(npm, light_work.vnlms[n]);
}

/** Copies view-space normal-light matrix n (0 or 1) to nlm. */
#line 1011
void LightGetNthViewNLM(float (*nlm)[4], int n) {

    assert_dw(n < 2);
    sceVu0CopyMatrix(nlm, light_work.vnlms[n]);
}

/** Copies light-color matrix n (0 or 1) to lcm. */
#line 1021
void LightGetNthLCM(float (*lcm)[4], int n) {

    assert_dw(n < 2);
    sceVu0CopyMatrix(lcm, light_work.lcms[n]);
}

/** Returns the brightness of the current reflection light. */
float LightReflectionBrightness(void) {
    return light_work.reflection_brightness;
}

/** Copies the current reflection color to color. */
void LightGetReflectionColor(float *color) {
    sceVu0CopyVector(color, light_work.reflection_color);
}

/** Returns a pointer to the current reflection color. */
float *LightReflectionColor(void) {
    return light_work.reflection_color;
}

/** Copies the view-space reflection matrix to nhm; there is only one, so n is not used. */
void LightGetNthViewNHM(float (*nhm)[4], int n) {
    sceVu0CopyMatrix(nhm, light_work.vnhm);
}

/** Returns the number of point/spot lights picked by the last LightUpdateInfoByPos. */
int LightNValidExtras(void) {
    return light_work.n_valid_extras;
}

/** Returns picked light n, or NULL if there are fewer. */
#line 1081
struct Light *LightNthValidExtra(int n) {

    assert_dw((u_int)n < EXTRA_MAX);
    if (n < light_work.n_valid_extras) {
        return light_work.valid_extras[n];
    }
    return NULL;
}
