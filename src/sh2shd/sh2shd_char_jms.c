/*
 * Character stencil shadows (sh2shd): per character, the shadow-casting parts (by skeleton
 * node), their matrices and light positions, the VU1 reference tags of their shadow volumes
 * for spot, parallel and drop shadows (some ordered, some for Maria's variants), and James's
 * self-shadow light position.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "libc/math.h"
#include "sdk/libvu0.h"
/* Matching: fitted stand-in for double-precision code (docs/stand-ins.md; STRIPPED_DOUBLE_CODE in common.h). */
STRIPPED_DOUBLE_CODE()

#define SHD_REFTAG_POOL_SIZE 0x1900
#define SHD_KICK_PACKET_SIZE 0x280

/**
 * Sets up a character shadow manager.
 * @param man         the manager
 * @param scp         the character
 * @param raw_data    the character's shadow data (its header gives the part count)
 * @param kind        character kind
 * @param id          character id
 * @param light_kind  light kind
 * @param light_pos   light position
 * @param light_dir   light direction
 * @param light_param cone parameters ([0] cos of the cone angle; [1] and [2] are set here)
 */
void sh2shd_init_char_man(struct SHADOW_CHAR_MAN *man, struct SubCharacter *scp, union Q_WORDDATA *raw_data, unsigned short kind, short id, int light_kind, float *light_pos, float *light_dir, float *light_param) {
    struct SHADOW_CHAR_HEAD char_head;
    short obj_id;
    union Q_WORDDATA *raw;

    man->scp = scp;
    man->raw_data = raw_data;
    raw = raw_data;
    char_head = *(struct SHADOW_CHAR_HEAD *)raw;
    man->kind = kind;
    man->id = id;
    man->obj_num = char_head.obj_num;
    man->light_kind = light_kind;
    vcopy_gcc(man->light_pos, light_pos);
    vcopy_gcc(man->light_dir, light_dir);
    vcopy_gcc(man->light_param, light_param);
    man->light_param[1] = sqrt(1.0f - man->light_param[0] * man->light_param[0]);
    man->light_param[2] = 5000.0f;
}

/**
 * Builds the reference tags of a character's shadow volumes for a spot light.
 * @param man        the manager
 * @param ref_packet reference tag buffer
 * @param rawdata    the character's shadow data
 */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 145
void sh2shd_make_reftag_pool_char(struct SHADOW_CHAR_MAN *man, struct SHADOW_PACKET_BUF *ref_packet, union Q_WORDDATA *rawdata) {
    struct SHADOW_CHAR_HEAD char_head;
    struct SHADOW_CHAR_OBJ_HEAD char_obj_head;
    struct SHADOW_GEOM_HEAD geom_head;
    union Q_WORDDATA *ref;
    union Q_WORDDATA *raw;
    short shape_no;

    ref = ref_packet->curr;
    shape_no = -1;
    raw = rawdata;
    char_head = *(struct SHADOW_CHAR_HEAD *)raw;
    raw++;
    for (; char_head.obj_num != 0; char_head.obj_num--) {
        shape_no++;
        char_obj_head = *(struct SHADOW_CHAR_OBJ_HEAD *)raw;
        man->shape[shape_no].pRefPacket = (unsigned int *)ref;
        man->shape[shape_no].obj_id = char_obj_head.obj_id;
        ref[0].ui32[0] = 0x30000001;
        ref[0].ui32[1] = (unsigned int)(raw + 1) & 0x7FFFFFFF;
        ref[0].ui32[2] = 0x1000101;
        ref[0].ui32[3] = 0x6D020000;
        raw += 6;
        ref[1].ui32[0] = 0x30000006;
        ref[1].ui32[1] = (unsigned int)&man->shape[shape_no] & 0x7FFFFFFF;
        ref[1].ui32[2] = 0x1000101;
        ref[1].ui32[3] = 0x6C060006;
        ref[2].ui32[0] = 0x10000000;
        ref[2].ui32[1] = 0;
        ref[2].ui32[2] = 0x14000000;
        ref[2].ui32[3] = 0x11000000;
        ref += 3;
        for (; char_obj_head.geom_num != 0; char_obj_head.geom_num--) {
            geom_head = *(struct SHADOW_GEOM_HEAD *)raw;
            if (geom_head.prim == 5 || geom_head.prim == 6) {
                ref[0].ui32[0] = 0x30000001;
                ref[0].ui32[1] = (unsigned int)raw;
                ref[0].ui32[2] = 0x1000101;
                ref[0].ui32[3] = 0x6D028000;
                ref[1].ui32[0] = 0x30000001;
                ref[1].ui32[1] = (unsigned int)(raw + 1);
                ref[1].ui32[2] = 0x1000102;
                ref[1].ui32[3] = 0x6D028002;
                ref[2].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                ref[2].ui32[1] = (unsigned int)(raw + 2);
                ref[2].ui32[2] = 0x1000203;
                ref[2].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008006;
                ref += 3;
            }
            ref[0].ui32[0] = 0x10000000;
            ref[0].ui32[1] = 0;
            ref[0].ui32[2] = 0;
            ref[0].ui32[3] = 0x17000000;
            ref++;
            raw += geom_head.ee_memory_size;
        }
        ref[0].ui32[0] = 0x60000000;
        ref[0].ui32[1] = 0;
        ref[0].ui32[2] = 0;
        ref[0].ui32[3] = 0;
        ref++;
    }
    ref_packet->curr = ref;
    assert(ref_packet->curr - ref_packet->head < SHD_REFTAG_POOL_SIZE);
}

/**
 * sh2shd_make_reftag_pool_char for the character kinds with Maria's shadow data layout.
 */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 277
void sh2shd_make_reftag_pool_char_p_maria(struct SHADOW_CHAR_MAN *man, struct SHADOW_PACKET_BUF *ref_packet, union Q_WORDDATA *rawdata) {
    struct SHADOW_CHAR_HEAD char_head;
    struct SHADOW_CHAR_OBJ_HEAD char_obj_head;
    struct SHADOW_GEOM_HEAD geom_head;
    union Q_WORDDATA *ref;
    union Q_WORDDATA *raw;
    short shape_no;

    ref = ref_packet->curr;
    shape_no = -1;
    raw = rawdata;
    char_head = *(struct SHADOW_CHAR_HEAD *)raw;
    raw++;
    for (; char_head.obj_num != 0; char_head.obj_num--) {
        shape_no++;
        char_obj_head = *(struct SHADOW_CHAR_OBJ_HEAD *)raw;
        man->shape[shape_no].pRefPacket = (unsigned int *)ref;
        switch (char_obj_head.obj_id) {
        case 9:
            char_obj_head.obj_id = 9;
            break;
        case 6:
            char_obj_head.obj_id = 6;
            break;
        case 4:
            char_obj_head.obj_id = 4;
            break;
        case 0:
            char_obj_head.obj_id = 0;
            break;
        case 1:
            char_obj_head.obj_id = 1;
            break;
        case 0x20:
            char_obj_head.obj_id = 0x17;
            break;
        case 0x26:
            char_obj_head.obj_id = 0x1D;
            break;
        case 0x29:
            char_obj_head.obj_id = 0x20;
            break;
        case 0x1D:
            char_obj_head.obj_id = 0x15;
            break;
        case 0x28:
            char_obj_head.obj_id = 0x1C;
            break;
        case 0x27:
            char_obj_head.obj_id = 0x1E;
            break;
        case 3:
            char_obj_head.obj_id = 3;
            break;
        case 0xA:
            char_obj_head.obj_id = 7;
            break;
        case 0x1B:
            char_obj_head.obj_id = 0x14;
            break;
        case 5:
            char_obj_head.obj_id = 5;
            break;
        case 0xF:
            char_obj_head.obj_id = 8;
            break;
        case 0x1E:
            char_obj_head.obj_id = 0x16;
            break;
        }
        man->shape[shape_no].obj_id = char_obj_head.obj_id;
        ref[0].ui32[0] = 0x30000001;
        ref[0].ui32[1] = (unsigned int)(raw + 1) & 0x7FFFFFFF;
        ref[0].ui32[2] = 0x1000101;
        ref[0].ui32[3] = 0x6D020000;
        raw += 6;
        ref[1].ui32[0] = 0x30000006;
        ref[1].ui32[1] = (unsigned int)&man->shape[shape_no] & 0x7FFFFFFF;
        ref[1].ui32[2] = 0x1000101;
        ref[1].ui32[3] = 0x6C060006;
        ref[2].ui32[0] = 0x10000000;
        ref[2].ui32[1] = 0;
        ref[2].ui32[2] = 0x14000000;
        ref[2].ui32[3] = 0x11000000;
        ref += 3;
        for (; char_obj_head.geom_num != 0; char_obj_head.geom_num--) {
            geom_head = *(struct SHADOW_GEOM_HEAD *)raw;
            if (geom_head.prim == 5 || geom_head.prim == 6) {
                ref[0].ui32[0] = 0x30000001;
                ref[0].ui32[1] = (unsigned int)raw;
                ref[0].ui32[2] = 0x1000101;
                ref[0].ui32[3] = 0x6D028000;
                ref[1].ui32[0] = 0x30000001;
                ref[1].ui32[1] = (unsigned int)(raw + 1);
                ref[1].ui32[2] = 0x1000102;
                ref[1].ui32[3] = 0x6D028002;
                ref[2].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                ref[2].ui32[1] = (unsigned int)(raw + 2);
                ref[2].ui32[2] = 0x1000203;
                ref[2].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008006;
                ref += 3;
            }
            ref[0].ui32[0] = 0x10000000;
            ref[0].ui32[1] = 0;
            ref[0].ui32[2] = 0;
            ref[0].ui32[3] = 0x17000000;
            ref++;
            raw += geom_head.ee_memory_size;
        }
        ref[0].ui32[0] = 0x60000000;
        ref[0].ui32[1] = 0;
        ref[0].ui32[2] = 0;
        ref[0].ui32[3] = 0;
        ref++;
    }
    ref_packet->curr = ref;
    assert(ref_packet->curr - ref_packet->head < SHD_REFTAG_POOL_SIZE);
}

/** Adds a character's shadow reference tags to the kick packet, disabling switched-off parts. */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 666
void sh2shd_add_char_to_kick_packet(struct SHADOW_CHAR_MAN *man, struct SHADOW_PACKET_BUF *kick_packet) {
    int i;
    union Q_WORDDATA *curr;
    unsigned int *reftag_pool_addr;
    int count_call_tag_num;
    union Q_WORDDATA *top;
    int j;
    int k;

    top = kick_packet->curr;
    count_call_tag_num = 0;
    top->ui32[0] = 0x30000003;
    top->ui32[1] = (unsigned long)(unsigned int)&man->light_pos & 0x7FFFFFFF;
    top->ui32[2] = 0x1000101;
    top->ui32[3] = 0x6C03000C;
    curr = top + 1;
    top = curr;
    for (i = 0; i < man->obj_num; i++) {
        reftag_pool_addr = man->shape[i].pRefPacket;
        man->shape[i].pKickAddr = (unsigned int *)curr;
        curr->ui32[0] = 0x50000000;
        curr->ui32[1] = (unsigned int)reftag_pool_addr;
        curr->ui32[2] = 0;
        curr->ui32[3] = 0x11000000;
        curr++;
        count_call_tag_num++;
    }
    curr->ui32[0] = 0x70000000;
    curr->ui32[1] = 0;
    curr->ul64[1] = 0;
    kick_packet->curr = curr;
    assert(kick_packet->curr - kick_packet->head < SHD_KICK_PACKET_SIZE);
    for (i = 0; i < 2; i++) {
        if (man->kind == shadow_off_work_char[i].kind && man->id == shadow_off_work_char[i].id) {
            if (shadow_off_work_char[i].obj_id[0] == 999) {
                for (k = 0; k < count_call_tag_num; k++) {
                    top[k].ui32[0] = 0x10000000;
                }
                top[k].ui32[1] = 0;
                top[k].ui32[2] = 0;
                top[k].ui32[3] = 0;
            } else {
                for (j = 0; j < 22; j++) {
                    if (shadow_off_work_char[i].obj_id[j] == -1) {
                        break;
                    }
                    for (k = 0; k < man->obj_num; k++) {
                        if (man->shape[k].obj_id == shadow_off_work_char[i].obj_id[j]) {
                            break;
                        }
                    }
                    if (k < count_call_tag_num) {
                        top[k].ui32[0] = 0x10000000;
                    }
                    top[k].ui32[1] = 0;
                    top[k].ui32[2] = 0;
                    top[k].ui32[3] = 0;
                }
            }
        }
    }
}

/**
 * Per-frame update of a character shadow manager: each part's matrix, the light position in
 * its local space, the shadow length leng and the clip kind.
 */
void sh2shd_renew_char_man(struct SHADOW_CHAR_MAN *man, float *spot_pos, float leng, struct SHADOW_ENV *shadow_env) {
    int i;
    float world_local[4][4];

    for (i = 0; i < man->obj_num; i++) {
        shCharacterGetPartsMatrixForShadow(man->shape[i].local_world, man->kind, man->id, man->shape[i].obj_id);
        sceVu0InversMatrix(world_local, man->shape[i].local_world);
        sceVu0ApplyMatrix(man->shape[i].local_light_position, world_local, spot_pos);
        man->shape[i].length.fl32[0] = leng;
        man->shape[i].length.si32[1] = shadow_env->clip_kind;
    }
}

/** sh2shd_renew_char_man with the length from shadow_env (leng is not used). */
void sh2shd_renew_char_man_for_spot(struct SHADOW_CHAR_MAN *man, float *spot_pos, float leng, struct SHADOW_ENV *shadow_env) {
    int i;
    float world_local[4][4];

    for (i = 0; i < man->obj_num; i++) {
        shCharacterGetPartsMatrixForShadow(man->shape[i].local_world, man->kind, man->id, man->shape[i].obj_id);
        sceVu0InversMatrix(world_local, man->shape[i].local_world);
        sceVu0ApplyMatrix(man->shape[i].local_light_position, world_local, spot_pos);
        man->shape[i].length.fl32[0] = shadow_env->leng;
        man->shape[i].length.si32[1] = shadow_env->clip_kind;
    }
}

/** sh2shd_renew_char_man for a parallel light (a distant virtual light position, length 10000). */
void sh2shd_renew_char_man_parallel(struct SHADOW_CHAR_MAN *man, float *virtual_light_position) {
    int i;
    float leng;
    float world_local[4][4];

    for (i = 0; i < man->obj_num; i++) {
        shCharacterGetPartsMatrixForShadow(man->shape[i].local_world, man->kind, man->id, man->shape[i].obj_id);
        sceVu0InversMatrix(world_local, man->shape[i].local_world);
        sceVu0ApplyMatrix(man->shape[i].local_light_position, world_local, virtual_light_position);
        leng = 10000.0f;
        man->shape[i].length.fl32[0] = leng;
    }
}

/**
 * Computes the virtual light direction for a character's self-shadow: horizontally along its
 * root facing, scaled from its height, lowered by env's height correction.
 * @param light_pos receives the light vector (w 0)
 * @param chara_pos not used
 * @param height    subtracted from the height of part 3
 */
void sh2shd_calc_light_position_for_self(float *light_pos, unsigned short kind, short id, struct JMS_SHADOW_ENV *env, float *chara_pos, float height) {
    float tmp[4];
    float z_unit[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    float mat[4][4];
    float height_revision;
    float bias;
    float scale;
    float sc;

    height_revision = env->height_revision;
    bias = env->bias;
    scale = env->scale;
    shCharacterGetPartsMatrixForShadow(mat, kind, id, 0);
    mat[0][1] = mat[1][0] = mat[1][1] = mat[1][2] = mat[2][1] = mat[3][0] = mat[3][1] = mat[3][2] = 0.0f;
    sceVu0ApplyMatrix(z_unit, mat, z_unit);
    shCharacterGetPartsMatrixForShadow(mat, kind, id, 3);
    mat[3][1] -= height;
    vcopy_gcc(tmp, z_unit);
    tmp[1] = 0.0f;
    _shNormalize(tmp, tmp);
    sc = bias + (654.0f + mat[3][1]) * scale;
    if (sc < 0.0f) {
        sc *= -1.0f;
    }
    _shScaleVector(tmp, tmp, sc);
    if (height != height_revision) {
        tmp[1] = -height_revision;
    } else {
        tmp[1] = 0.1f;
    }
    tmp[3] = 0.0f;
    vcopy_gcc(light_pos, tmp);
}

/** sh2shd_calc_light_position_for_self giving a light position around chara_pos (w 1). */
void sh2shd_calc_light_position_for_self_spot(float *light_pos, unsigned short kind, short id, struct JMS_SHADOW_ENV *env, float *chara_pos, float height) {
    float tmp[4];
    float z_unit[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    float mat[4][4];
    float height_revision;
    float bias;
    float scale;

    height_revision = env->height_revision;
    bias = env->bias;
    scale = env->scale;
    shCharacterGetPartsMatrixForShadow(mat, kind, id, 0);
    mat[0][1] = mat[1][0] = mat[1][1] = mat[1][2] = mat[2][1] = mat[3][0] = mat[3][1] = mat[3][2] = 0.0f;
    sceVu0ApplyMatrix(z_unit, mat, z_unit);
    shCharacterGetPartsMatrixForShadow(mat, kind, id, 3);
    mat[3][1] -= height;
    vcopy_gcc(tmp, z_unit);
    tmp[1] = 0.0f;
    _shNormalize(tmp, tmp);
    _shScaleVector(tmp, tmp, bias + (654.0f + mat[3][1]) * scale);
    tmp[0] += chara_pos[0];
    if (height != height_revision) {
        tmp[1] = height - height_revision;
    } else {
        tmp[1] = 0.1f;
    }
    tmp[2] += chara_pos[2];
    tmp[3] = 1.0f;
    vcopy_gcc(light_pos, tmp);
}

/*
 * Matching: stand-in for a function the original linker dead-stripped
 * (config/stripped_functions.txt; tools/mwcc_fixup.py empties it): the older, unordered
 * version of the function below. Only its assert(0) string survives in .rodata (shared with
 * the later asserts). Name unknown; the body is a guess.
 */
void __stripped_sh2shd_char_jms_code(struct SHADOW_CHAR_MAN *man, struct SHADOW_PACKET_BUF *ref_packet, union Q_WORDDATA *rawdata) {
    struct SHADOW_CHAR_HEAD char_head;
    struct SHADOW_CHAR_OBJ_HEAD char_obj_head;
    struct SHADOW_GEOM_HEAD geom_head;
    union Q_WORDDATA *ref;
    union Q_WORDDATA *raw;
    short shape_no;

    ref = ref_packet->curr;
    shape_no = -1;
    raw = rawdata;
    char_head = *(struct SHADOW_CHAR_HEAD *)raw;
    raw++;
    for (; char_head.obj_num != 0; char_head.obj_num--) {
        shape_no++;
        char_obj_head = *(struct SHADOW_CHAR_OBJ_HEAD *)raw;
        man->shape[shape_no].pRefPacket = (unsigned int *)ref;
        man->shape[shape_no].obj_id = char_obj_head.obj_id;
        ref[0].ui32[0] = 0x30000001;
        ref[0].ui32[1] = (unsigned int)(raw + 1) & 0x7FFFFFFF;
        ref[0].ui32[2] = 0x1000101;
        ref[0].ui32[3] = 0x6D020000;
        raw += 6;
        ref[1].ui32[0] = 0x30000004;
        ref[1].ui32[1] = (unsigned int)&man->drop_shadow_matrix & 0x7FFFFFFF;
        ref[1].ui32[2] = 0x1000101;
        ref[1].ui32[3] = 0x6C04000C;
        ref[2].ui32[0] = 0x30000006;
        ref[2].ui32[1] = (unsigned long)(unsigned int)&man->shape[shape_no] & 0x7FFFFFFF;
        ref[2].ui32[2] = 0x1000101;
        ref[2].ui32[3] = 0x6C060006;
        ref[3].ui32[0] = 0x10000000;
        ref[3].ui32[1] = 0;
        ref[3].ui32[2] = 0x14000000;
        ref[3].ui32[3] = 0x11000000;
        ref += 4;
        for (; char_obj_head.geom_num != 0; char_obj_head.geom_num--) {
            geom_head = *(struct SHADOW_GEOM_HEAD *)raw;
            if (geom_head.prim == 5 || geom_head.prim == 6) {
                ref[0].ui32[0] = 0x30000001;
                ref[0].ui32[1] = (unsigned int)raw;
                ref[0].ui32[2] = 0x1000101;
                ref[0].ui32[3] = 0x6D028000;
                ref[1].ui32[0] = 0x30000001;
                ref[1].ui32[1] = (unsigned int)(raw + 1);
                ref[1].ui32[2] = 0x1000101;
                ref[1].ui32[3] = 0x6D028002;
                ref[2].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                ref[2].ui32[1] = (unsigned int)(raw + 2);
                ref[2].ui32[2] = 0x1000101;
                ref[2].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008004;
                ref += 3;
            } else {
                assert(0);
            }
            ref[0].ui32[0] = 0x10000000;
            ref[0].ui32[1] = 0;
            ref[0].ui32[2] = 0;
            ref[0].ui32[3] = 0x17000000;
            ref++;
            raw += geom_head.ee_memory_size;
        }
        ref[0].ui32[0] = 0x60000000;
        ref[0].ui32[1] = 0;
        ref[0].ui32[2] = 0;
        ref[0].ui32[3] = 0;
        ref++;
    }
    ref_packet->curr = ref;
    assert(ref_packet->curr - ref_packet->head < SHD_REFTAG_POOL_SIZE);
}

/** Builds the reference tags of a character's drop-shadow volumes, parts in draw order. */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 1104
void sh2shd_make_reftag_pool_char_for_drop_with_order(struct SHADOW_CHAR_MAN *man, struct SHADOW_PACKET_BUF *ref_packet, union Q_WORDDATA *rawdata) {
    struct SHADOW_CHAR_HEAD char_head;
    struct SHADOW_CHAR_OBJ_HEAD char_obj_head;
    struct SHADOW_GEOM_HEAD geom_head;
    union Q_WORDDATA *ref;
    union Q_WORDDATA *raw;
    short shape_no2;
    int shape_no;
    int a;

    ref = ref_packet->curr;
    raw = rawdata;
    shape_no2 = -1;
    char_head = *(struct SHADOW_CHAR_HEAD *)raw;
    raw++;
    for (; char_head.obj_num != 0; char_head.obj_num--) {
        shape_no2++;
        char_obj_head = *(struct SHADOW_CHAR_OBJ_HEAD *)raw;
        a = sh2shd_get_shape_no(man->kind, char_obj_head.obj_id);
        shape_no = a;
        if (shape_no == -1) {
            shape_no = shape_no2;
        }
        man->shape[shape_no].pRefPacket = (unsigned int *)ref;
        man->shape[shape_no].obj_id = char_obj_head.obj_id;
        ref[0].ui32[0] = 0x30000001;
        ref[0].ui32[1] = (unsigned int)(raw + 1) & 0x7FFFFFFF;
        ref[0].ui32[2] = 0x1000101;
        ref[0].ui32[3] = 0x6D020000;
        raw += 6;
        ref[1].ui32[0] = 0x30000004;
        ref[1].ui32[1] = (unsigned int)&man->drop_shadow_matrix & 0x7FFFFFFF;
        ref[1].ui32[2] = 0x1000101;
        ref[1].ui32[3] = 0x6C04000C;
        ref[2].ui32[0] = 0x30000006;
        ref[2].ui32[1] = (unsigned long)(unsigned int)&man->shape[shape_no] & 0x7FFFFFFF;
        ref[2].ui32[2] = 0x1000101;
        ref[2].ui32[3] = 0x6C060006;
        ref[3].ui32[0] = 0x10000000;
        ref[3].ui32[1] = 0;
        ref[3].ui32[2] = 0x14000000;
        ref[3].ui32[3] = 0x11000000;
        ref += 4;
        for (; char_obj_head.geom_num != 0; char_obj_head.geom_num--) {
            geom_head = *(struct SHADOW_GEOM_HEAD *)raw;
            if (geom_head.prim == 5 || geom_head.prim == 6) {
                ref[0].ui32[0] = 0x30000001;
                ref[0].ui32[1] = (unsigned int)raw;
                ref[0].ui32[2] = 0x1000101;
                ref[0].ui32[3] = 0x6D028000;
                ref[1].ui32[0] = 0x30000001;
                ref[1].ui32[1] = (unsigned int)(raw + 1);
                ref[1].ui32[2] = 0x1000101;
                ref[1].ui32[3] = 0x6D028002;
                ref[2].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                ref[2].ui32[1] = (unsigned int)(raw + 2);
                ref[2].ui32[2] = 0x1000101;
                ref[2].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008004;
                ref += 3;
            } else {
                a = shape_no;
            }
            ref[0].ui32[0] = 0x10000000;
            ref[0].ui32[1] = 0;
            ref[0].ui32[2] = 0;
            ref[0].ui32[3] = 0x17000000;
            ref++;
            raw += geom_head.ee_memory_size;
        }
        ref[0].ui32[0] = 0x60000000;
        ref[0].ui32[1] = 0;
        ref[0].ui32[2] = 0;
        ref[0].ui32[3] = 0;
        ref++;
    }
    ref_packet->curr = ref;
    assert(ref_packet->curr - ref_packet->head < SHD_REFTAG_POOL_SIZE);
}

/** sh2shd_make_reftag_pool_char_for_drop_with_order for Maria's shadow data layout. */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 1225
void sh2shd_make_reftag_pool_char_for_drop_with_order_p_maria(struct SHADOW_CHAR_MAN *man, struct SHADOW_PACKET_BUF *ref_packet, union Q_WORDDATA *rawdata) {
    struct SHADOW_CHAR_HEAD char_head;
    struct SHADOW_CHAR_OBJ_HEAD char_obj_head;
    struct SHADOW_GEOM_HEAD geom_head;
    union Q_WORDDATA *ref;
    union Q_WORDDATA *raw;
    short shape_no2;
    int shape_no;

    ref = ref_packet->curr;
    raw = rawdata;
    shape_no2 = -1;
    char_head = *(struct SHADOW_CHAR_HEAD *)raw;
    raw++;
    for (; char_head.obj_num != 0; char_head.obj_num--) {
        shape_no2++;
        char_obj_head = *(struct SHADOW_CHAR_OBJ_HEAD *)raw;
        switch (char_obj_head.obj_id) {
        case 9:
            char_obj_head.obj_id = 9;
            break;
        case 6:
            char_obj_head.obj_id = 6;
            break;
        case 4:
            char_obj_head.obj_id = 4;
            break;
        case 0:
            char_obj_head.obj_id = 0;
            break;
        case 1:
            char_obj_head.obj_id = 1;
            break;
        case 0x20:
            char_obj_head.obj_id = 0x17;
            break;
        case 0x26:
            char_obj_head.obj_id = 0x1D;
            break;
        case 0x29:
            char_obj_head.obj_id = 0x20;
            break;
        case 0x1D:
            char_obj_head.obj_id = 0x15;
            break;
        case 0x28:
            char_obj_head.obj_id = 0x1C;
            break;
        case 0x27:
            char_obj_head.obj_id = 0x1E;
            break;
        case 3:
            char_obj_head.obj_id = 3;
            break;
        case 0xA:
            char_obj_head.obj_id = 7;
            break;
        case 0x1B:
            char_obj_head.obj_id = 0x14;
            break;
        case 5:
            char_obj_head.obj_id = 5;
            break;
        case 0xF:
            char_obj_head.obj_id = 8;
            break;
        case 0x1E:
            char_obj_head.obj_id = 0x16;
            break;
        }
        shape_no = sh2shd_get_shape_no(man->kind, char_obj_head.obj_id);
        if (shape_no == -1) {
            shape_no = shape_no2;
        }
        man->shape[shape_no].pRefPacket = (unsigned int *)ref;
        man->shape[shape_no].obj_id = char_obj_head.obj_id;
        ref[0].ui32[0] = 0x30000001;
        ref[0].ui32[1] = (unsigned int)(raw + 1) & 0x7FFFFFFF;
        ref[0].ui32[2] = 0x1000101;
        ref[0].ui32[3] = 0x6D020000;
        raw += 6;
        ref[1].ui32[0] = 0x30000004;
        ref[1].ui32[1] = (unsigned int)&man->drop_shadow_matrix & 0x7FFFFFFF;
        ref[1].ui32[2] = 0x1000101;
        ref[1].ui32[3] = 0x6C04000C;
        ref[2].ui32[0] = 0x30000006;
        ref[2].ui32[1] = (unsigned long)(unsigned int)&man->shape[shape_no] & 0x7FFFFFFF;
        ref[2].ui32[2] = 0x1000101;
        ref[2].ui32[3] = 0x6C060006;
        ref[3].ui32[0] = 0x10000000;
        ref[3].ui32[1] = 0;
        ref[3].ui32[2] = 0x14000000;
        ref[3].ui32[3] = 0x11000000;
        ref += 4;
        for (; char_obj_head.geom_num != 0; char_obj_head.geom_num--) {
            geom_head = *(struct SHADOW_GEOM_HEAD *)raw;
            if (geom_head.prim == 5 || geom_head.prim == 6) {
                ref[0].ui32[0] = 0x30000001;
                ref[0].ui32[1] = (unsigned int)raw;
                ref[0].ui32[2] = 0x1000101;
                ref[0].ui32[3] = 0x6D028000;
                ref[1].ui32[0] = 0x30000001;
                ref[1].ui32[1] = (unsigned int)(raw + 1);
                ref[1].ui32[2] = 0x1000101;
                ref[1].ui32[3] = 0x6D028002;
                ref[2].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                ref[2].ui32[1] = (unsigned int)(raw + 2);
                ref[2].ui32[2] = 0x1000101;
                ref[2].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008004;
                ref += 3;
            } else {
                assert(0);
            }
            ref[0].ui32[0] = 0x10000000;
            ref[0].ui32[1] = 0;
            ref[0].ui32[2] = 0;
            ref[0].ui32[3] = 0x17000000;
            ref++;
            raw += geom_head.ee_memory_size;
        }
        ref[0].ui32[0] = 0x60000000;
        ref[0].ui32[1] = 0;
        ref[0].ui32[2] = 0;
        ref[0].ui32[3] = 0;
        ref++;
    }
    ref_packet->curr = ref;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 1359
    assert(ref_packet->curr - ref_packet->head < SHD_REFTAG_POOL_SIZE);
}

/**
 * Per-frame update of a character's drop shadow: each part's matrix and light position, and
 * the alpha height range (James's range is reused for the boat).
 */
void sh2shd_renew_char_man_for_drop(struct SHADOW_CHAR_MAN *man, float *spot_pos, float alpha_decay) {
    static float jms_high;
    static float jms_low;
    int i;
    float world_local[4][4];
    float high;
    float low;
    float tmp;

    sh2shd_get_height_for_alpha(&high, &low, man->kind, man->id);
    if (high > low) {
        tmp = high;
        high = low;
        low = tmp;
    }
    high = low - 700.0f;
    if (man->kind >= 0x100 && man->kind < 0x104) {
        jms_high = high;
        jms_low = low;
    }
    if (man->kind >= 0x800 && man->kind < 0x809) {
        high = jms_high;
        low = jms_low;
    }
    for (i = 0; i < man->obj_num; i++) {
        shCharacterGetPartsMatrixForShadow(man->shape[i].local_world, man->kind, man->id, man->shape[i].obj_id);
        sceVu0InversMatrix(world_local, man->shape[i].local_world);
        sceVu0ApplyMatrix(man->shape[i].local_light_position, world_local, spot_pos);
        man->shape[i].length.fl32[0] = high;
        man->shape[i].length.fl32[1] = low;
        man->shape[i].length.fl32[3] = alpha_decay;
        if (DramaDemoNumber() == 3) {
            man->shape[i].length.fl32[0] = low;
        }
    }
}

/** Gets the height range over which a character's drop shadow fades. */
void sh2shd_get_height_for_alpha(float *high, float *low, unsigned short kind, unsigned short id) {
    float mat[4][4];

    switch (kind) {
    case 0x100:
    case 0x101:
    case 0x102:
    case 0x103:
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 8);
        *high = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x1E);
        *low = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x1D);
        if (*low < mat[3][1]) {
            *low = mat[3][1];
        }
        break;
    case 0x105:
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 9);
        *high = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x16);
        *low = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x14);
        if (*low < mat[3][1]) {
            *low = mat[3][1];
        }
        break;
    case 0x106:
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 9);
        *high = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x1E);
        *low = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x1B);
        if (*low < mat[3][1]) {
            *low = mat[3][1];
        }
        break;
    case 0x107:
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 9);
        *high = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x22);
        *low = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x24);
        if (*low < mat[3][1]) {
            *low = mat[3][1];
        }
        break;
    case 0x200:
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0xB);
        *high = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x14);
        *low = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0x15);
        if (*low < mat[3][1]) {
            *low = mat[3][1];
        }
        break;
    case 0x201:
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 9);
        *high = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0xB);
        if (*high > mat[3][1]) {
            *high = mat[3][1];
        }
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0xD);
        *low = mat[3][1];
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0xE);
        if (*low < mat[3][1]) {
            *low = mat[3][1];
        }
        break;
    case 0x801:
    case 0x802:
    case 0x803:
    case 0x804:
    case 0x805:
    case 0x806:
    case 0x807:
    case 0x808:
        shCharacterGetPartsMatrixForShadow(mat, kind, id, 0);
        *high = mat[3][1];
        *low = mat[3][1];
        break;
    }
}

/** Returns the shadow shape number of skeleton node skel_no of a character kind, or -1 for kinds without one. */
int sh2shd_get_shape_no(unsigned short kind, int skel_no) {
    int all_skel_num;
    int shape_no;

    switch (kind) {
    case 0x100:
    case 0x101:
    case 0x102:
    case 0x103:
        skel_no = sh2shd_get_shape_no_jms(skel_no);
        break;
    case 0x105:
        skel_no = sh2shd_get_shape_no_p_mar(skel_no);
        break;
    case 0x106:
        skel_no = sh2shd_get_shape_no_mar(skel_no);
        break;
    case 0x200:
        skel_no = sh2shd_get_shape_no_scu(skel_no);
        break;
    case 0x201:
        skel_no = sh2shd_get_shape_no_mkn(skel_no);
        break;
    case 0x205:
        skel_no = sh2shd_get_shape_no_edb(skel_no);
        break;
    case 0x207:
    case 0x20B:
        skel_no = sh2shd_get_shape_no_nse(skel_no);
        break;
    case 0x208:
        skel_no = sh2shd_get_shape_no_red(skel_no);
        break;
    case 0x209:
        skel_no = sh2shd_get_shape_no_oni(skel_no);
        break;
    default:
        return -1;
    }
    return skel_no;
}
/** Shape number of James's skeleton node skel_no. */
int sh2shd_get_shape_no_jms(int skel_no) {
    switch (skel_no) {
    case 0x1A:
        return 0;
        break;
    case 0x19:
        return 1;
        break;
    case 0xE:
        return 2;
        break;
    case 0xD:
        return 3;
        break;
    case 5:
        return 4;
        break;
    case 4:
        return 5;
        break;
    case 1:
        return 6;
        break;
    case 3:
        return 7;
        break;
    case 0x1C:
        return 8;
        break;
    case 0x1B:
        return 9;
        break;
    case 0x22:
        return 0xA;
        break;
    case 0x21:
        return 0xB;
        break;
    case 0x23:
        return 0xC;
        break;
    case 0x28:
        return 0xD;
        break;
    case 8:
        return 0xE;
        break;
    case 0xF:
        return 0xF;
        break;
    }
}

/** Shape number of Maria's skeleton node skel_no. */
int sh2shd_get_shape_no_mar(int skel_no) {
    switch (skel_no) {
    case 0x1E:
        return 0;
        break;
    case 0x1B:
        return 1;
        break;
    case 0xF:
        return 2;
        break;
    case 0xA:
        return 3;
        break;
    case 5:
        return 4;
        break;
    case 3:
        return 5;
        break;
    case 1:
        return 6;
        break;
    case 0:
        return 7;
        break;
    case 4:
        return 8;
        break;
    case 0x20:
        return 9;
        break;
    case 0x1D:
        return 0xA;
        break;
    case 0x26:
        return 0xB;
        break;
    case 0x28:
        return 0xC;
        break;
    case 0x29:
        return 0xD;
        break;
    case 0x27:
        return 0xE;
        break;
    case 6:
        return 0xF;
        break;
    case 9:
        return 0x10;
        break;
    /* Matching: #line keeps the assert below on its original line. */
#line 1928
    default:
        assert_dw(0);
    }
}

/** Shape number of skeleton node skel_no of Maria's other model (kind 0x105). */
int sh2shd_get_shape_no_p_mar(int skel_no) {
    switch (skel_no) {
    case 0x16:
        return 0;
        break;
    case 0x14:
        return 1;
        break;
    case 8:
        return 2;
        break;
    case 7:
        return 3;
        break;
    case 5:
        return 4;
        break;
    case 3:
        return 5;
        break;
    case 1:
        return 6;
        break;
    case 0:
        return 7;
        break;
    case 4:
        return 8;
        break;
    case 0x17:
        return 9;
        break;
    case 0x15:
        return 0xA;
        break;
    case 0x1D:
        return 0xB;
        break;
    case 0x1C:
        return 0xC;
        break;
    case 0x20:
        return 0xD;
        break;
    case 0x1E:
        return 0xE;
        break;
    case 6:
        return 0xF;
        break;
    case 9:
        return 0x10;
        break;
    /* Matching: #line keeps the assert below on its original line. */
#line 2075
    default:
        assert_dw(0);
    }
}

/** Shape number of skeleton node skel_no of character kind 0x200. */
int sh2shd_get_shape_no_scu(int skel_no) {
    switch (skel_no) {
    case 0:
        return 8;
        break;
    case 2:
        return 0xB;
        break;
    case 6:
        return 6;
        break;
    case 7:
        return 7;
        break;
    case 8:
        return 0xE;
        break;
    case 9:
        return 4;
        break;
    case 0xA:
        return 5;
        break;
    case 0xB:
        return 0xF;
        break;
    case 0xC:
        return 2;
        break;
    case 0xD:
        return 3;
        break;
    case 0x14:
        return 0;
        break;
    case 0x15:
        return 1;
        break;
    case 0x16:
        return 0xC;
        break;
    case 0x17:
        return 0xD;
        break;
    case 0x18:
        return 9;
        break;
    case 0x19:
        return 0xA;
        break;
    }
}

/** Shape number of skeleton node skel_no of character kind 0x201. */
int sh2shd_get_shape_no_mkn(int skel_no) {
    switch (skel_no) {
    case 0:
        return 6;
        break;
    case 1:
        return 7;
        break;
    case 2:
        return 8;
        break;
    case 4:
        return 4;
        break;
    case 3:
        return 5;
        break;
    case 5:
        return 0xA;
        break;
    case 7:
        return 9;
        break;
    case 9:
        return 0xC;
        break;
    case 10:
        return 3;
        break;
    case 11:
        return 0xB;
        break;
    case 12:
        return 2;
        break;
    case 13:
        return 1;
        break;
    case 14:
        return 0;
        break;
    }
}

/** Shape number of skeleton node skel_no of character kinds 0x207 and 0x20B. */
int sh2shd_get_shape_no_nse(int skel_no) {
    switch (skel_no) {
    case 13:
        return 0;
        break;
    case 12:
        return 1;
        break;
    case 8:
        return 2;
        break;
    case 6:
        return 3;
        break;
    case 5:
        return 4;
        break;
    case 4:
        return 5;
        break;
    case 1:
        return 6;
        break;
    case 0:
        return 7;
        break;
    case 2:
        return 8;
        break;
    case 24:
        return 9;
        break;
    case 23:
        return 0xA;
        break;
    case 20:
        return 0xB;
        break;
    case 19:
        return 0xC;
        break;
    case 16:
        return 0xD;
        break;
    case 15:
        return 0xE;
        break;
    case 7:
        return 0xF;
        break;
    case 9:
        return 0x10;
        break;
    }
}

/** Shape number of skeleton node skel_no of character kind 0x208. */
int sh2shd_get_shape_no_red(int skel_no) {
    switch (skel_no) {
    case 0xC:
        return 0;
        break;
    case 0xD:
        return 1;
        break;
    case 7:
        return 2;
        break;
    case 6:
        return 3;
        break;
    case 4:
        return 4;
        break;
    case 3:
        return 5;
        break;
    case 0:
        return 6;
        break;
    case 2:
        return 7;
        break;
    case 0x24:
        return 8;
        break;
    case 0x1D:
        return 9;
        break;
    case 0x1C:
        return 0xA;
        break;
    case 0x17:
        return 0xB;
        break;
    case 0x15:
        return 0xC;
        break;
    case 0x10:
        return 0xD;
        break;
    case 0xF:
        return 0xE;
        break;
    case 0xE:
        return 0xF;
        break;
    }
}

/** Shape number of skeleton node skel_no of character kind 0x209. */
int sh2shd_get_shape_no_oni(int skel_no) {
    switch (skel_no) {
    case 0xC:
        return 0;
        break;
    case 0xD:
        return 1;
        break;
    case 7:
        return 2;
        break;
    case 6:
        return 3;
        break;
    case 4:
        return 4;
        break;
    case 3:
        return 5;
        break;
    case 0:
        return 6;
        break;
    case 2:
        return 7;
        break;
    case 0x24:
        return 8;
        break;
    case 0x19:
        return 9;
        break;
    case 0x18:
        return 0xA;
        break;
    case 0x15:
        return 0xB;
        break;
    case 0x14:
        return 0xC;
        break;
    case 0x10:
        return 0xD;
        break;
    case 0xF:
        return 0xE;
        break;
    case 5:
        return 0xF;
        break;
    case 0xE:
        return 0x10;
        break;
    }
}

/** Shape number of skeleton node skel_no of character kind 0x205. */
int sh2shd_get_shape_no_edb(int skel_no) {
    switch (skel_no) {
    case 0xF:
        return 0;
        break;
    case 0xE:
        return 1;
        break;
    case 4:
        return 2;
        break;
    case 3:
        return 3;
        break;
    case 0:
        return 4;
        break;
    case 5:
        return 5;
        break;
    case 0x16:
        return 6;
        break;
    case 0x11:
        return 7;
        break;
    case 0x1C:
        return 8;
        break;
    case 0x1B:
        return 9;
        break;
    case 0x1F:
        return 0xA;
        break;
    case 0x1E:
        return 0xB;
        break;
    case 6:
        return 0xC;
        break;
    case 0xD:
        return 0xD;
        break;
    }
}
