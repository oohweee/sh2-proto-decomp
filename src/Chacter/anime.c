/*
 * Skeletal animation of characters (shAnime3d): decoding the packed animation data, playing
 * drama and gameplay animation with interpolation between animations, per-node overrides
 * (neck/body angles), and static-model posing and scaling.
 */

#include "sh2.h"
#include "fi_libvu0_inline.h"
#include "fi_calc.h"
#include "libc/math.h"
#include "sdk/libvu0.h"
/* Matching: for __stripped_anime_code below, a dead-stripped function whose 0x80-byte
 * zero-initializer .bss template survives at the start of this unit's .bss. */
void __stripped_anime_use(void *p);

/* Matching: an inferred wrapper. The original's out-of-line copy of _sceVu0ApplyMatrix_1 shows the call
 * went through an inline function; its name (invented) and header are unknown
 * (docs/matching-notes.md#anime-inlineapplymatrix_1). */
static inline void inlineApplyMatrix_1(float *dest, float (*mat)[4], float *src) {
    _sceVu0ApplyMatrix_1(dest, mat, src);
}

/* Matching: an inline function (name ours); written in place, shExec doesn't match. */
static inline unsigned int GetUInt(unsigned short *p) {
    return p[0] | (p[1] << 16);
}

static inline float GetFloat(void *adr) {
    unsigned short *p;
    union {
        int i;
        float f;
    } fi;

    p = adr;
    fi.i = p[0] | (p[1] << 16);
    return fi.f;
}

static inline float IntBits(int i) {
    union {
        int i;
        float f;
    } fi;

    fi.i = i;
    return fi.f;
}

static inline void sqvector(float *v0, float *v1) {
    __asm__ __volatile__("
    lqc2       vf4, 0x0(%1)
    vmove.xyzw vf5, vf4
    vmul.xyzw  vf6, vf4, vf5
    sqc2       vf6, 0x0(%0)
    " : : "r"(v0), "r"(v1));
}

static inline void RotTransposeMatrix(float (*m)[4], struct FVEC *rot) {
    struct FVEC r;
    struct FMAT tmp;

    r.x = -rot->x;
    r.y = -rot->y;
    r.z = -rot->z;
    sceVu0RotMatrix(tmp.d, kt_unit_matrix.d, &r.x);
    sceVu0TransposeMatrix(m, tmp.d);
}

static float GetSF(void *adr) {
    unsigned short *sp;
    int sign;
    int coeff;
    int exp;
    int i;
    union {
        int i;
        float f;
    } fi;

    sp = adr;
    i = *sp;
    sign = (i >> 15) & 1;
    exp = (i >> 10) & 0x1F;
    coeff = i & 0x3FF;
    if (!(exp + 0x70) && !(coeff << 13)) {
        return 0.0f;
    }
    fi.i = (sign << 31) | ((exp + 0x70) << 23) | (coeff << 13);
    return fi.f;
}

static void shCharacterAnimeInterMatrix(struct FMAT *out, struct FMAT *in1, struct FMAT *in2, float t) {
    struct FVEC v1;
    struct FVEC v2;
    int i1;

    for (i1 = 0; i1 < 4; i1++) {
        v1 = *(struct FVEC *)in1->d[i1];
        v2 = *(struct FVEC *)in2->d[i1];
        _sceVu0InterVector(out->d[i1], (float *)&v1, (float *)&v2, t);
    }
}

/** Rotates node @p stp by @p rot about the origin (item-screen models). */
void shCharacterStayModelExecItem(struct shSkelton *stp, float *rot) {
    float pos[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    shCharacterStayModelExecNthParts(stp, pos, rot);
}

/** Sets node @p stp's matrix to its rest matrix rotated by @p rot and moved to @p pos. */
void shCharacterStayModelExecNthParts(struct shSkelton *stp, float *pos, float *rot) {
    struct FVEC buf;

    buf = *(struct FVEC *)stp->src_m.d[3];
    stp->src_m.d[3][0] = stp->src_m.d[3][1] = stp->src_m.d[3][2] = 0.0f;
    sceVu0RotMatrix(stp->src_m.d, stp->des_m.d, rot);
    stp->src_m.d[3][0] = pos[0];
    stp->src_m.d[3][1] = pos[1];
    stp->src_m.d[3][2] = pos[2];
    stp->src_t.x = pos[0];
    stp->src_t.y = pos[1];
    stp->src_t.z = pos[2];
}

static void *shExec(void *anime, struct shSkelton *stp, int inter_type, int *ret) {
    unsigned int flag;
    int bits;
    short *sp;
    int type;
    struct FVEC rot;
    struct FVEC axis;
    struct shSkelton dummy_st;

    if (!anime) {
        return NULL;
    }
    sp = anime;
    flag = GetUInt((unsigned short *)sp);
    sp += 2;
    bits = 8;
    for (; stp; stp = stp->next) {
        if (!bits--) {
            flag = GetUInt((unsigned short *)sp);
            sp += 2;
            bits = 7;
        }
        type = flag & 0xF;
        flag >>= 4;
        stp->is_key = type & 8;
        type &= ~8;
        if (!type) {
            stp->change = 0;
            continue;
        }
        stp->change = 1;
        *ret = 1;
        if (stp->untouchable) {
            dummy_st.next = stp->next;
            dummy_st.parent = stp->parent;
            stp = &dummy_st;
        }
        switch (type) {
            case 2:
                if (!inter_type) {
                    if (stp->parent) {
                        stp->src_t.x = GetSF(sp);
                        stp->src_t.y = GetSF(sp + 1);
                        stp->src_t.z = GetSF(sp + 2);
                        stp->src_t.w = 0.0f;
                        sp += 3;
                    } else {
                        stp->src_t.x = GetFloat(sp);
                        stp->src_t.y = GetFloat(sp + 2);
                        stp->src_t.z = GetFloat(sp + 4);
                        stp->src_t.w = 0.0f;
                        sp += 6;
                    }
                } else {
                    if (stp->parent) {
                        stp->des_t.x = GetSF(sp);
                        stp->des_t.y = GetSF(sp + 1);
                        stp->des_t.z = GetSF(sp + 2);
                        stp->des_t.w = 0.0f;
                        sp += 3;
                    } else {
                        stp->des_t.x = GetFloat(sp);
                        stp->des_t.y = GetFloat(sp + 2);
                        stp->des_t.z = GetFloat(sp + 4);
                        stp->des_t.w = 0.0f;
                        sp += 6;
                    }
                }
            case 1:
                rot.x = IntBits(sp[0]);
                rot.y = IntBits(sp[1]);
                rot.z = IntBits(sp[2]);
                sp += 3;
                _sceVu0ItoF12Vector((float *)&rot);
                if (!inter_type) {
                    RotTransposeMatrix(stp->src_m.d, &rot);
                } else {
                    RotTransposeMatrix(stp->des_m.d, &rot);
                }
                break;
            case 6:
                if (stp->parent) {
                    stp->src_t.x = GetSF(sp);
                    stp->src_t.y = GetSF(sp + 1);
                    stp->src_t.z = GetSF(sp + 2);
                    sp += 3;
                } else {
                    stp->src_t.x = GetFloat(sp);
                    stp->src_t.y = GetFloat(sp + 2);
                    stp->src_t.z = GetFloat(sp + 4);
                    sp += 6;
                }
                goto des_trans;
            case 4:
                stp->src_t = stp->des_t;
            des_trans:
                if (stp->parent) {
                    stp->des_t.x = GetSF(sp);
                    stp->des_t.y = GetSF(sp + 1);
                    stp->des_t.z = GetSF(sp + 2);
                    stp->des_t.w = 0.0f;
                    sp += 3;
                } else { /* @bug GetSF (16-bit) on the root's 32-bit floats; the other root paths use GetFloat */
                    stp->des_t.x = GetSF(sp);
                    stp->des_t.y = GetSF(sp + 2);
                    stp->des_t.z = GetSF(sp + 4);
                    stp->des_t.w = 0.0f;
                    sp += 6;
                }
            rot_axis:
                rot.x = IntBits(sp[0]);
                rot.y = IntBits(sp[1]);
                rot.z = IntBits(sp[2]);
                sp += 3;
                _sceVu0ItoF12Vector((float *)&rot);
                if (!inter_type) {
                    RotTransposeMatrix(stp->src_m.d, &rot);
                } else {
                    RotTransposeMatrix(stp->des_m.d, &rot);
                }
                axis.x = IntBits(sp[0]);
                axis.y = IntBits(sp[1]);
                axis.z = IntBits(sp[2]);
                sp += 3;
                _sceVu0ItoF15Vector((float *)&axis);
                stp->axis = axis;
                rot.x = IntBits(sp[0]);
                _sceVu0ItoF12Vector((float *)&rot);
                stp->axis.w = rot.x;
                sp += 2;
                break;
            case 3:
                if (inter_type) {
                    goto rot_axis;
                }
                stp->src_t = stp->des_t;
                goto rot_axis;
            case 5:
                if (stp->parent) {
                    stp->src_t.x = GetSF(sp);
                    stp->src_t.y = GetSF(sp + 1);
                    stp->src_t.z = GetSF(sp + 2);
                    stp->src_t.w = 0.0f;
                    sp += 3;
                } else {
                    stp->src_t.x = GetFloat(sp);
                    stp->src_t.y = GetFloat(sp + 2);
                    stp->src_t.z = GetFloat(sp + 4);
                    stp->src_t.w = 0.0f;
                    sp += 6;
                }
                stp->des_t = stp->src_t;
                goto rot_axis;
        }
    }
    return sp;
}

/** Applies extra rotation @p rot to node @p stp of @p ap (neck/body turning). */
void shCharacterAnimePartsControl(struct shAnime3d *ap, struct shSkelton *stp, struct FVEC *rot) {
    struct FMAT test_mat;
    float x;
    float y;
    float z;

    x = stp->src_m.d[3][0];
    y = stp->src_m.d[3][1];
    z = stp->src_m.d[3][2];
    stp->src_m.d[3][0] = 0.0f;
    stp->src_m.d[3][1] = 0.0f;
    stp->src_m.d[3][2] = 0.0f;
    RotTransposeMatrix(test_mat.d, rot);
    sceVu0MulMatrix(stp->src_m.d, test_mat.d, stp->src_m.d);
    stp->src_m.d[3][0] = x;
    stp->src_m.d[3][1] = y;
    stp->src_m.d[3][2] = z;
    stp->pad = 0;
}

/* Matching: dead-stripped; see top */
void __stripped_anime_code(void) { u_long128 work[8] = { 0 }; __stripped_anime_use(work); }
static void shCharacterAnimeReconstruct(struct shAnime3d *ap, int inter_type, int type, int frame) {
    static struct FVEC trans_tmp;
    static struct FMAT mat_tmp;
    int i;
    int count;
    int inter_rate;
    struct FMAT r_mat;
    struct shSkelton *stp;
    struct shSkelton *parent;
    struct FVEC *pos;
    struct FMAT *mat;
    struct shSkelton *p_stp;
    float delta;
    float s;
    float c;
    float c_1;
    float d;
    float d2;
    struct FVEC *axis;

    i = 0;
    inter_rate = frame << 12;
    switch (inter_type) {
        case 3:
        case 5:
        case 7:
            count = ap->c_count.y;
            break;
        case 1:
            count = ap->c_count.x;
            break;
        case -1:
            count = 0;
            break;
    }
    r_mat.d[0][3] = 0.0f;
    r_mat.d[1][3] = 0.0f;
    r_mat.d[2][3] = 0.0f;
    r_mat.d[3][0] = 0.0f;
    r_mat.d[3][1] = 0.0f;
    r_mat.d[3][2] = 0.0f;
    r_mat.d[3][3] = 1.0f;
    for (stp = ap->top; stp; i++, stp = stp->next) {
        if (!stp->untouchable) {
            pos = &trans_tmp;
            mat = &mat_tmp;
            delta = (1.0f / inter_rate) * (stp->theta * (float)((frame << 12) - count));
            c = cosf(delta);
            s = sinf(delta);
            c_1 = 1.0f - c;
            r_mat.d[0][0] = c + c_1 * stp->xx;
            r_mat.d[1][1] = c + c_1 * stp->yy;
            r_mat.d[2][2] = c + c_1 * stp->zz;
            axis = &stp->axis;
            d = axis->z * s;
            d2 = c_1 * stp->xy;
            r_mat.d[1][0] = d2 + d;
            r_mat.d[0][1] = d2 - d;
            d = axis->y * s;
            d2 = c_1 * stp->zx;
            r_mat.d[2][0] = d2 - d;
            r_mat.d[0][2] = d2 + d;
            d = axis->x * s;
            d2 = c_1 * stp->yz;
            r_mat.d[2][1] = d2 + d;
            r_mat.d[1][2] = d2 - d;
            shMulMatrix(mat->d, (float (*)[4])&stp->des_m, (float (*)[4])&r_mat);
            _sceVu0InterVector((float *)pos, (float *)&stp->des_t, (float *)&stp->src_t, count * (1.0f / inter_rate));
            parent = NULL;
            p_stp = stp->parent;
            stp->src_m.d[3][3] = 1.0f;
            if (!p_stp) {
                if (ap->first_bone_type != 1) {
                    stp->src_m = *mat;
                }
                if (type) {
                    stp->src_m.d[3][0] = stp->next->src_m.d[3][0];
                    stp->src_m.d[3][1] = stp->next->src_m.d[3][1];
                    stp->src_m.d[3][2] = stp->next->src_m.d[3][2];
                } else {
                    stp->src_m.d[3][0] = pos->x;
                    stp->src_m.d[3][1] = pos->y;
                    stp->src_m.d[3][2] = pos->z;
                }
                if (type && !i) {
                    struct FVEC rot = {0.0f, 0.0f, 0.0f, 0.0f};

                    rot.x = ap->rot_body_neck.x;
                    rot.y = ap->rot_body_neck.y + ap->rot_body.y;
                    shCharacterAnimePartsControl(ap, stp, &rot);
                }
            } else {
                if (parent != p_stp) {
                    kt_gte.matrix = p_stp->src_m;
                }
                pos->w = 1.0f;
                sceVu0MulMatrix(stp->src_m.d, kt_gte.matrix.d, mat->d);
                inlineApplyMatrix_1(stp->src_m.d[3], kt_gte.matrix.d, (float *)pos);
                stp->src_m.d[3][3] = 1.0f;
                if (type) {
                    if (i == 8) {
                        shCharacterAnimePartsControl(ap, stp, &ap->rot_neck);
                    }
                    switch (PlayerGetJamesWeapon()) {
                        case 1:
                            if (i == 0x1B || i == 0x1C) {
                                shCharacterAnimePartsControl(ap, stp, &ap->rot_arms);
                            }
                            break;
                        case 3:
                            if (i == 8 || i == 9 || i == 10) {
                                shCharacterAnimePartsControl(ap, stp, &ap->rot_arms);
                            }
                            break;
                    }
                }
            }
        }
        stp->change = 1;
        stp->src_m.d[3][3] = 1.0f;
    }
}

static void EigenVector(struct FMAT *m, struct FVEC *t) {
    struct FMAT me;
    float sm[9];
    int maxs;
    float max;
    struct FVEC v[2];
    struct FVEC nv;
    int i;
    int j;

    me.d[0][0] = m->d[0][0] - 1.0f;
    me.d[0][1] = m->d[0][1];
    me.d[0][2] = m->d[0][2];
    me.d[1][0] = m->d[1][0];
    me.d[1][1] = m->d[1][1] - 1.0f;
    me.d[1][2] = m->d[1][2];
    me.d[2][0] = m->d[2][0];
    me.d[2][1] = m->d[2][1];
    me.d[2][2] = m->d[2][2] - 1.0f;
    sm[0] = me.d[1][1] * me.d[2][2] - me.d[2][1] * me.d[1][2];
    sm[1] = me.d[0][1] * me.d[2][2] - me.d[2][1] * me.d[0][2];
    sm[2] = me.d[0][1] * me.d[1][2] - me.d[1][1] * me.d[0][2];
    sm[3] = me.d[1][0] * me.d[2][2] - me.d[2][0] * me.d[1][2];
    sm[4] = me.d[0][0] * me.d[2][2] - me.d[2][0] * me.d[0][2];
    sm[5] = me.d[0][0] * me.d[1][2] - me.d[1][0] * me.d[0][2];
    sm[6] = me.d[1][0] * me.d[2][1] - me.d[2][0] * me.d[1][1];
    sm[7] = me.d[0][0] * me.d[2][1] - me.d[2][0] * me.d[0][1];
    sm[8] = me.d[0][0] * me.d[1][1] - me.d[1][0] * me.d[0][1];
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (sm[i * 3 + j] < 0.0f) {
                sm[i * 3 + j] = -sm[i * 3 + j];
            }
        }
    }
    maxs = 0;
    max = sm[0];
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (max < sm[i * 3 + j]) {
                maxs = i * 3 + j;
                max = sm[i * 3 + j];
            }
        }
    }
    switch (maxs) {
        case 6:
        case 7:
        case 8:
            v[0].x = me.d[0][0];
            v[0].y = me.d[1][0];
            v[0].z = me.d[2][0];
            v[1].x = me.d[0][1];
            v[1].y = me.d[1][1];
            v[1].z = me.d[2][1];
            break;
        case 3:
        case 4:
        case 5:
            v[0].x = me.d[0][0];
            v[0].y = me.d[1][0];
            v[0].z = me.d[2][0];
            v[1].x = me.d[0][2];
            v[1].y = me.d[1][2];
            v[1].z = me.d[2][2];
            break;
        case 0:
        case 1:
        case 2:
            v[0].x = me.d[0][1];
            v[0].y = me.d[1][1];
            v[0].z = me.d[2][1];
            v[1].x = me.d[0][2];
            v[1].y = me.d[1][2];
            v[1].z = me.d[2][2];
            break;
    }
    _sceVu0OuterProduct((float *)&nv, (float *)&v[0], (float *)&v[1]);
    ktVectorNormal(t, &nv);
}

static void EigenVec2Mat(struct FVEC *ev, struct FMAT *smat) {
    struct FVEC v1;
    struct FVEC v2;
    struct FVEC mv0;
    struct FVEC mv1;
    struct FVEC mv2;

    ktVectorNormal(&mv0, ev);
    smat->d[0][0] = mv0.x;
    smat->d[0][1] = mv0.y;
    smat->d[0][2] = mv0.z;
    if (fabsf(mv0.y) > fabsf(mv0.x)) {
        if (fabsf(mv0.x) > fabsf(mv0.z)) {
            v1.x = mv0.y;
            v1.y = -mv0.x;
            if (mv0.z) {
                v1.z = -(mv0.x * v1.x + mv0.y * v1.y) / mv0.z;
            } else {
                v1.z = 0.0f;
            }
        } else {
            if (mv0.y) {
                v1.z = mv0.y;
                v1.y = -mv0.z;
            } else {
                v1.z = -mv0.y;
                v1.y = mv0.z;
            }
            if (mv0.x) {
                v1.x = -(mv0.z * v1.z + mv0.y * v1.y) / mv0.x;
            } else {
                v1.x = 0.0f;
            }
        }
    } else {
        if (fabsf(mv0.y) > fabsf(mv0.z)) {
            v1.x = mv0.y;
            v1.y = -mv0.x;
            if (mv0.z) {
                v1.z = -(mv0.x * v1.x + mv0.y * v1.y) / mv0.z;
            } else {
                v1.z = 0.0f;
            }
        } else {
            if (mv0.z) {
                v1.x = mv0.z;
                v1.z = -mv0.x;
            } else {
                v1.x = -mv0.z;
                v1.z = mv0.x;
            }
            if (mv0.y) {
                v1.y = -(mv0.x * v1.x + mv0.z * v1.z) / mv0.y;
            } else {
                v1.y = 0.0f;
            }
        }
    }
    ktVectorNormal(&mv1, &v1);
    smat->d[1][0] = mv1.x;
    smat->d[1][1] = mv1.y;
    smat->d[1][2] = mv1.z;
    _sceVu0OuterProduct((float *)&v2, (float *)&mv0, (float *)&mv1);
    ktVectorNormal(&mv2, &v2);
    smat->d[2][0] = mv2.x;
    smat->d[2][1] = mv2.y;
    smat->d[2][2] = mv2.z;
    smat->d[3][0] = 0.0f;
    smat->d[3][1] = 0.0f;
    smat->d[3][2] = 0.0f;
    smat->d[3][3] = 1.0f;
    smat->d[0][3] = 0.0f;
    smat->d[1][3] = 0.0f;
    smat->d[2][3] = 0.0f;
}

static void shCharacterAnimeCalcComplement(struct shAnime3d *ap) {
    struct shSkelton *stp;
    struct FMAT mat;
    struct FMAT eig;
    struct FMAT ieig;
    struct FVEC axis;
    float theta;

    for (stp = ap->top; stp; stp = stp->next) {
        if (stp->untouchable) {
            continue;
        }
        _sceVu0TransposeMatrix(mat.d, stp->des_m.d);
        shMulMatrix(mat.d, mat.d, stp->src_m.d);
        EigenVector(&mat, &axis);
        EigenVec2Mat(&axis, &eig);
        _sceVu0TransposeMatrix(ieig.d, eig.d);
        shMulMatrix(mat.d, ieig.d, mat.d);
        shMulMatrix(mat.d, mat.d, eig.d);
        theta = atan2f(mat.d[1][2], mat.d[1][1]);
        stp->theta = -theta;
        *(struct FVEC *)&stp->axis = axis;
        stp->xy = axis.x * axis.y;
        stp->yz = axis.y * axis.z;
        stp->zx = axis.z * axis.x;
        sqvector((float *)&axis, (float *)&axis);
        stp->xx = axis.x;
        stp->yy = axis.y;
        stp->zz = axis.z;
    }
}

/** Attaches skeleton @p stp to animation @p ap and to the track before it (ap[-1]). */
void shCharacterAnimeSetSkelton(struct shAnime3d *ap, struct shSkelton *stp) {
    ap->top = stp;
    ap[-1].top = stp;
}

/** Advances and applies drama animation @p ap for this frame, interpolating from the previous animation. @return 0. */
int shCharacterDramaAnimeExecMain(struct shAnime3d *ap) {
    int dt;
    int inter_cont;
    short src_frame;
    short des_frame;
    struct shSkelton *stp;
    void *adr;
    int result;
    int frame;

    dt = shGetDF();
    result = 0;
    frame = 0;
    switch (ap->comp_type) {
        case -1:
            frame = ap->cur_frame.y - ap->cur_frame.x;
            frame = frame ? frame : 1;
            shCharacterAnimeReconstruct(ap, 1, 0, frame);
            break;
        case 0:
            adr = ap->frame_top;
            shExec(adr, ap->top, 0, &result);
            shExec(adr, ap->top, 1, &result);
            ap->comp_type = -1;
            break;
        case 1:
            src_frame = ap->cur_frame.x;
            des_frame = ap->cur_frame.y;
            inter_cont = (ap->cur_frame.x == ap->cur_frame.y) ? 1 : 0;
            ap->total_speed.x = dt * (ap->anim_b->speed + ap->c_speed.x);
            ap->c_count.x += ap->total_speed.x;
            frame = ap->cur_frame.y - ap->cur_frame.x;
            frame = frame ? frame : 1;
            while (ap->c_count.x >= frame << 12) {
                ap->cur_frame.x += frame;
                ap->c_count.x -= frame << 12;
                inter_cont = 1;
            }
            ap->total_count += ap->total_speed.x;
            if (ap->cur_frame.x >= ap->anim_b->end) {
                ap->cur_frame.x = ap->anim_b->end;
                ap->total_count = ap->cur_frame.x << 12;
                ap->c_count.x = 0;
                ap->comp_type = -1;
            }
            if ((demo_status >> 6) & 1) {
                ap->cur_frame.x = ap->anim_b->end;
                inter_cont = 1;
            }
            if (ap->anim_b->loop == 1) {
                ap->total_count = ap->total_count % (ap->anim_b->frame << 12);
            } else {
                ap->total_count = (ap->total_count > ap->anim_b->end << 12) ? ap->anim_b->end << 12 : ap->total_count;
                if (ap->total_count == ap->anim_b->end << 12) {
                    ap->c_count.x = 0;
                    ap->cur_frame.x = ap->anim_b->end;
                }
            }
            if (inter_cont) {
                ap->cur_frame.x %= ap->anim_b->frame;
                if (des_frame != ap->cur_frame.x) {
                    for (stp = ap->top; stp; stp = stp->next) {
                        if (!stp->untouchable) {
                            stp->src_m = stp->des_m;
                            stp->src_t = stp->des_t;
                        }
                    }
                    for (adr = ap->frame_top; ap->cur_frame.y != ap->cur_frame.x; ap->cur_frame.y++) {
                        adr = shExec(adr, ap->top, 0, &result);
                    }
                    ap->frame_top = adr;
                    for (stp = ap->top; stp; stp = stp->next) {
                        if (!stp->untouchable) {
                            stp->des_m = stp->src_m;
                            stp->des_t = stp->src_t;
                        }
                    }
                } else {
                    for (stp = ap->top; stp; stp = stp->next) {
                        if (!stp->untouchable) {
                            stp->src_m = stp->des_m;
                            stp->src_t = stp->des_t;
                        }
                    }
                }
                adr = ap->frame_top;
                result = 0;
                frame = 0;
                while (!result) {
                    ap->cur_frame.y++;
                    frame++;
                    adr = shExec(adr, ap->top, 1, &result);
                    if (ap->cur_frame.y == ap->anim_b->end) {
                        break;
                    }
                }
                if (ap->anim_b->loop == 1) {
                    ap->cur_frame.y %= ap->anim_b->frame;
                } else {
                    if (ap->cur_frame.x == ap->anim_a->frame - 1) {
                        ap->comp_type = -1;
                    }
                    if (ap->cur_frame.y == ap->anim_b->frame) {
                        ap->cur_frame.y--;
                    }
                }
                ap->frame_top = adr;
                shCharacterAnimeCalcComplement(ap);
            }
            shCharacterAnimeReconstruct(ap, 1, 0, frame);
            break;
        case 2:
            adr = ap->frame_top;
            ap->frame_top = shExec(adr, ap->top, 0, &result);
            result = 0;
            while (!result) {
                adr = shExec(adr, ap->top, 1, &result);
                frame++;
                if (ap->anim_b->end == frame) {
                    break;
                }
            }
            ap->frame_top = adr;
            ap->cur_frame.y += frame;
            ap->comp_type = 1;
            shCharacterAnimeCalcComplement(ap);
            shCharacterAnimeReconstruct(ap, 1, 0, frame);
            if ((demo_status >> 6) & 1) {
                shCharacterDramaAnimeExecMain(ap);
            }
            break;
    }
    return 0;
}

/* Matching: the original file has ~1000 more lines here (the assert below bakes in line 2110). */

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1787
/**
 * Advances and applies gameplay animation @p ap for this frame, interpolating from the
 * previous animation. @param type track (0 main, 1 second). @return 0.
 */
int shCharacterPlayingAnimeExecMain(struct shAnime3d *ap, int type) {
    int dt;
    struct FMAT backup[41];
    float t;
    int i1;
    int inter_cont;
    int result;
    short src_frame;
    short des_frame;
    struct shSkelton *stp;
    void *adr;

    dt = shGetDF();
    switch (ap->comp_type) {
        case -1:
            shCharacterAnimeReconstruct(ap, 1, type, 1);
            break;
        case 0:
            adr = ap->frame_top;
            adr = shExec(adr, ap->top, 0, &result);
            adr = shExec(adr, ap->top, 1, &result);
            ap->cur_frame.y++;
            ap->comp_type = 1;
            ap->frame_top = adr;
            shCharacterAnimeCalcComplement(ap);
            shCharacterAnimeReconstruct(ap, 1, type, 1);
            break;
        case 1: {
            int start;
            int end;
            int t_count;

            src_frame = ap->cur_frame.x;
            des_frame = ap->cur_frame.y;
            inter_cont = ap->cur_frame.x == ap->cur_frame.y;
            start = ap->anim_b->start;
            end = ap->anim_b->end;
            ap->c_count.x = ap->total_count % 0x1000;
            ap->cur_frame.x = start + (ap->total_count >> 12);
            if (ap->cur_frame.x != src_frame && ap->comp_type != -1) {
                inter_cont = 1;
            }
            if (ap->anim_b->pad == -1) {
                inter_cont = 1;
            }
            if (inter_cont) {
                ap->cur_frame.y = ap->cur_frame.x + 1;
                if (ap->anim_b->loop == 1) {
                    if (ap->cur_frame.x == end) {
                        ap->cur_frame.y = start;
                    }
                } else {
                    if (ap->cur_frame.x == end) {
                        ap->cur_frame.y = ap->cur_frame.x;
                    }
                }
                if (ap->total_speed.x < 0) {
                    ap->frame_top = (char *)ap->anime + ap->frame_size * ap->cur_frame.x;
                    shExec(ap->frame_top, ap->top, 0, &result);
                    ap->frame_top = (char *)ap->anime + ap->frame_size * ap->cur_frame.y;
                    shExec(ap->frame_top, ap->top, 1, &result);
                } else if (ap->cur_frame.x == start + des_frame && ap->anim_b->pad != -1) {
                    for (stp = ap->top; stp; stp = stp->next) {
                        if (!stp->untouchable) {
                            stp->src_m = stp->des_m;
                            stp->src_t = stp->des_t;
                        }
                    }
                    ap->frame_top = (char *)ap->anime + ap->frame_size * ap->cur_frame.y;
                    shExec(ap->frame_top, ap->top, 1, &result);
                } else {
                    ap->frame_top = (char *)ap->anime + ap->frame_size * ap->cur_frame.x;
                    shExec(ap->frame_top, ap->top, 0, &result);
                    ap->frame_top = (char *)ap->anime + ap->frame_size * ap->cur_frame.y;
                    shExec(ap->frame_top, ap->top, 1, &result);
                }
                shCharacterAnimeCalcComplement(ap);
            }
            shCharacterAnimeReconstruct(ap, 1, type, 1);
            if (ap->comp_type != -1) {
                ap->total_speed.x = dt * (ap->anim_b->speed + ap->c_speed.x);
                ap->total_count += ap->total_speed.x;
            }
            t_count = ap->total_count;
            if (ap->anim_b->loop == 1) {
                if (t_count < 0) {
                    t_count += (end - start + 1) << 12;
                } else if (t_count >= (end - start + 1) << 12) {
                    t_count -= (end - start + 1) << 12;
                }
            } else {
                if (t_count <= 0) {
                    t_count = 0;
                    ap->comp_type = -1;
                    ap->cur_frame.x = start;
                } else if (t_count >= (end - start) << 12) {
                    t_count = (end - start) << 12;
                    ap->comp_type = -1;
                }
            }
            ap->total_count = t_count;
            break;
        }
        case 3:
        case 5:
        case 7: {
            int comp_tmp;

            if (ap->comp_type == 5) {
                ap->total_speed.y = 0x1000;
            } else {
                ap->total_speed.y = (ap->c_speed.y + 0x200) * dt;
            }
            ap->c_count.y += ap->total_speed.y;
            if (ap->c_count.y >= 0x1000) {
                comp_tmp = ap->comp_type;
                switch (comp_tmp) {
                    case 3:
                        if (ap->anim_b->speed < 0) {
                            ap->total_count = (ap->anim_b->end - ap->anim_b->start) << 12;
                        } else {
                            ap->total_count = 0;
                        }
                        ap->c_count.x = 0;
                        ap->c_speed.x = 0;
                        break;
                    case 5:
                        break;
                    case 7:
                        ap->total_count = ap[1].total_count;
                        ap->c_count.x = ap[1].c_count.x;
                        ap->c_speed.x = ap[1].c_speed.x;
                        ap->cur_frame.y = ap[1].cur_frame.y;
                        break;
                }
                ap->c_speed.y = 0;
                ap->c_count.y = 0;
                ap->total_speed.x = dt * (ap->anim_b->speed + ap->c_speed.x);
                ap->total_speed.y = 0;
                ap->cur_frame.x = ap->cur_frame.y;
                ap->anim_a = ap->anim_b;
                ap->comp_type = 1;
                shCharacterPlayingAnimeExecMain(ap, type);
                if (comp_tmp == 7) {
                    ap->total_count = ap[1].total_count;
                    ap->c_count.x = ap[1].c_count.x;
                    ap->c_speed.x = ap[1].c_speed.x;
                    ap->cur_frame.y = ap[1].cur_frame.y;
                }
            } else {
                if (ap->comp_type == 7) {
                    shExec(ap->p_frame_top, ap->top, 0, &result);
                    ap->frame_top = ap[1].frame_top;
                    shExec(ap->frame_top, ap->top, 1, &result);
                    shCharacterAnimeCalcComplement(ap);
                }
                shCharacterAnimeReconstruct(ap, 3, type, 1);
            }
            break;
        }
        case 4:
        case 6:
        case 8:
            if (ap->anim_a->speed > 0) {
                for (stp = ap->top; stp; stp = stp->next) {
                    if (!stp->untouchable) {
                        stp->src_m = stp->des_m;
                        stp->src_t = stp->des_t;
                    }
                }
            } else {
                ap->p_frame_top = (char *)ap->p_anime + ap->frame_size * ap->cur_frame.x;
                shExec(ap->p_frame_top, ap->top, 0, &result);
            }
            if (ap->comp_type == 8) {
                ap->frame_top = ap[1].frame_top;
            }
            shExec(ap->frame_top, ap->top, 1, &result);
            shCharacterAnimeCalcComplement(ap);
            ap->comp_type--;
            shCharacterAnimeReconstruct(ap, 3, type, 1);
            break;
        case 9: {
            int start;
            int end;
            int t_count;
            int c_frame_x;
            int c_frame_y;
            int count;

            count = *T0_COUNT; /* unused, as in the original */
            start = ap->anim_b->start;
            end = ap->anim_b->end;
            ap->c_count.x = ap->total_count % 0x1000;
            c_frame_x = ap->total_count >> 12;
            c_frame_y = c_frame_x + 1;
            end -= start;
            if (c_frame_x == end) {
                c_frame_y = 0;
            }
            ap->cur_frame.x = start + c_frame_x;
            ap->cur_frame.y = start + c_frame_y;
            ap->total_speed.x = dt * (ap->anim_b->speed + ap->c_speed.x);
            ap->total_count += ap->total_speed.x;
            t_count = ap->total_count;
            if (t_count < 0) {
                t_count += (end + 1) << 12;
            } else if (t_count >= (end + 1) << 12) {
                t_count -= (end + 1) << 12;
            }
            ap->total_count = t_count;
            ap->total_speed.y = ap->c_speed.y * dt;
            ap->c_count.y += ap->total_speed.y;
            if (ap->c_count.y >= 0x1000) {
                ap->c_count.y = 0;
                ap->total_speed.y = 0;
                ap->c_speed.y = 0;
                ap->comp_type = 1;
                ap->anim_a = ap->anim_b;
                ap->cur_frame.y = ap->cur_frame.x;
                shCharacterPlayingAnimeExecMain(ap, type);
                break;
            }
            ap->p_frame_top = (char *)ap->anime + ap->frame_size * (c_frame_x + ap->anim_a->start);
            shExec(ap->p_frame_top, ap->top, 0, &result);
            ap->frame_top = (char *)ap->anime + ap->frame_size * (c_frame_x + ap->anim_b->start);
            shExec(ap->frame_top, ap->top, 1, &result);
            shCharacterAnimeCalcComplement(ap);
            shCharacterAnimeReconstruct(ap, 3, type, 1);
            i1 = 0;
            for (stp = ap->top; stp; i1++, stp = stp->next) {
                if (!stp->untouchable) {
                    backup[i1] = stp->src_m;
                }
            }
            ap->p_frame_top = (char *)ap->anime + ap->frame_size * (c_frame_y + ap->anim_a->start);
            shExec(ap->p_frame_top, ap->top, 0, &result);
            ap->frame_top = (char *)ap->anime + ap->frame_size * (c_frame_y + ap->anim_b->start);
            shExec(ap->frame_top, ap->top, 1, &result);
            shCharacterAnimeCalcComplement(ap);
            shCharacterAnimeReconstruct(ap, 3, type, 1);
            i1 = 0;
            t = 0.00024414062f * ap->c_count.x;
            for (stp = ap->top; stp; i1++, stp = stp->next) {
                if (!stp->untouchable) {
                    shCharacterAnimeInterMatrix(&stp->src_m, &stp->src_m, &backup[i1], t);
                }
            }
            break;
        }
        case 10: {
            int start;
            int end;
            int t_count;
            int c_frame_x;
            int c_frame_y;

            start = ap->anim_b->start;
            end = ap->anim_b->end;
            ap->c_count.x = ap->total_count % 0x1000;
            c_frame_x = ap->total_count >> 12;
            c_frame_y = c_frame_x + 1;
            end -= start;
            if (c_frame_x == end) {
                c_frame_y = 0;
            }
            ap->cur_frame.x = start + c_frame_x;
            ap->cur_frame.y = start + c_frame_y;
            ap->total_speed.x = dt * (ap->anim_b->speed + ap->c_speed.x);
            ap->total_count += ap->total_speed.x;
            t_count = ap->total_count;
            if (t_count < 0) {
                t_count += (end + 1) << 12;
            } else if (t_count >= (end + 1) << 12) {
                t_count -= (end + 1) << 12;
            }
            ap->total_count = t_count;
            ap->total_speed.y = ap->c_speed.y * dt;
            ap->c_count.y += ap->total_speed.y;
            ap->p_frame_top = (char *)ap->anime + ap->frame_size * (c_frame_x + ap->anim_a->start);
            shExec(ap->p_frame_top, ap->top, 0, &result);
            ap->frame_top = (char *)ap->anime + ap->frame_size * (c_frame_x + ap->anim_b->start);
            shExec(ap->frame_top, ap->top, 1, &result);
            shCharacterAnimeCalcComplement(ap);
            shCharacterAnimeReconstruct(ap, 3, type, 1);
            i1 = 0;
            for (stp = ap->top; stp; i1++, stp = stp->next) {
                if (!stp->untouchable) {
                    backup[i1] = stp->src_m;
                }
            }
            ap->p_frame_top = (char *)ap->anime + ap->frame_size * (c_frame_y + ap->anim_a->start);
            shExec(ap->p_frame_top, ap->top, 0, &result);
            ap->frame_top = (char *)ap->anime + ap->frame_size * (c_frame_y + ap->anim_b->start);
            shExec(ap->frame_top, ap->top, 1, &result);
            shCharacterAnimeCalcComplement(ap);
            shCharacterAnimeReconstruct(ap, 3, type, 1);
            i1 = 0;
            t = 0.00024414062f * ap->c_count.x;
            for (stp = ap->top; stp; i1++, stp = stp->next) {
                if (!stp->untouchable) {
                    shCharacterAnimeInterMatrix(&stp->src_m, &stp->src_m, &backup[i1], t);
                }
            }
            ap->comp_type = 9;
            break;
        }
        case 2:
            if (ap->comp_type != -1) {
                ap->total_speed.x = dt * (ap->anim_b->speed + ap->c_speed.x);
                ap->total_count += ap->total_speed.x;
            }
            ap->comp_type = 1;
            shCharacterPlayingAnimeExecMain(ap, type);
            break;
    }
    if (ap->total_count < 0) {
        printf("%d %d \n", ap->total_count, ap->comp_type);
    }
    assert(ap->total_count >= 0);
    return 0;
}

/** Scales every node matrix of static model @p ap by ap->scale. */
void shCharacterStayModelScale(struct shAnime3d *ap) {
    float scale;
    struct shSkelton *stp;

    scale = ap->scale;
    for (stp = ap->top; stp; stp = stp->next) {
        sceVu0ScaleVectorXYZ(stp->src_m.d[0], stp->src_m.d[0], scale);
        sceVu0ScaleVectorXYZ(stp->src_m.d[1], stp->src_m.d[1], scale);
        sceVu0ScaleVectorXYZ(stp->src_m.d[2], stp->src_m.d[2], scale);
        sceVu0ScaleVectorXYZ(stp->des_m.d[0], stp->des_m.d[0], scale);
        sceVu0ScaleVectorXYZ(stp->des_m.d[1], stp->des_m.d[1], scale);
        sceVu0ScaleVectorXYZ(stp->des_m.d[2], stp->des_m.d[2], scale);
    }
}



