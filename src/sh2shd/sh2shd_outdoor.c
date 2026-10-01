/*
 * Background (map) stencil shadows (sh2shd): sets up the shadow-casting objects of a loaded
 * shadow map, and per frame builds their VU1 reference tags for a spot or parallel light and
 * adds them to the kick packet, skipping objects whose shadows are switched off.
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
 * Sets up a background shadow manager from raw shadow map data: records each object's
 * local-to-world matrix, and stores the light.
 * @param man         the manager
 * @param raw_data    the shadow map data
 * @param arg2        not used
 * @param light_kind  light kind
 * @param light_pos   light position
 * @param light_dir   light direction
 * @param light_param cone parameters ([0] cos of the cone angle; [1] and [2] are set here)
 */
void sh2shd_init_outdoor_man2(struct SHADOW_OUTDOOR_MAN *man, union Q_WORDDATA *raw_data, short arg2, short light_kind, float *light_pos, float *light_dir, float *light_param) {
    struct SHADOW_OUTDOOR_HEAD outdoor_head;
    struct SHADOW_OUTDOOR_OBJ_HEAD obj_head;
    struct SHADOW_GEOM_HEAD geom_head;
    short obj_id;
    union Q_WORDDATA *raw;

    obj_id = -1;
    raw = raw_data;
    man->raw_data = raw_data;
    outdoor_head = *(struct SHADOW_OUTDOOR_HEAD *)raw;
    man->kind = outdoor_head.kind;
    man->map_id = outdoor_head.map_id;
    man->obj_num = outdoor_head.obj_num;
    raw++;
    for (; outdoor_head.obj_num != 0; outdoor_head.obj_num--) {
        obj_head = *(struct SHADOW_OUTDOOR_OBJ_HEAD *)raw;
        raw++;
        obj_id++;
        raw += 5;
        geom_head = *(struct SHADOW_GEOM_HEAD *)raw;
        if (geom_head.prim == 3 || geom_head.prim == 4 || geom_head.prim == 10) {
            sceVu0CopyMatrix(man->shape[obj_id].local_world, (float (*)[4])(raw - 4));
            raw += geom_head.ee_memory_size;
        } else {
            sceVu0CopyMatrix(man->shape[obj_id].local_world, (float (*)[4])(raw - 4));
            raw += geom_head.ee_memory_size;
            for (obj_head.geom_num--; obj_head.geom_num != 0; obj_head.geom_num--) {
                geom_head = *(struct SHADOW_GEOM_HEAD *)raw;
                raw += geom_head.ee_memory_size;
            }
        }
    }
    man->light_kind = light_kind;
    vcopy_gcc(man->light_pos, light_pos);
    vcopy_gcc(man->light_dir, light_dir);
    vcopy_gcc(man->light_param, light_param);
    man->light_param[1] = sqrt(1.0f - man->light_param[0] * man->light_param[0]);
    man->light_param[2] = 5000.0f;
}

/**
 * Builds the reference tags of a background map's shadow volumes for a spot light.
 * @param man            the manager
 * @param ref_packet     reference tag buffer
 * @param spot_cam_angle angle between the spot light and the camera
 */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 245
void sh2shd_make_reftag_pool_outdoor(struct SHADOW_OUTDOOR_MAN *man, struct SHADOW_PACKET_BUF *ref_packet, float spot_cam_angle) {
    struct SHADOW_OUTDOOR_HEAD outdoor_head;
    struct SHADOW_OUTDOOR_OBJ_HEAD obj_head;
    struct SHADOW_GEOM_HEAD geom_head;
    union Q_WORDDATA *ref;
    union Q_WORDDATA *raw;

    ref = ref_packet->curr;
    raw = man->raw_data;
    outdoor_head = *(struct SHADOW_OUTDOOR_HEAD *)raw;
    raw++;
    man->map_id = outdoor_head.map_id;
    for (; outdoor_head.obj_num != 0; outdoor_head.obj_num--) {
        obj_head = *(struct SHADOW_OUTDOOR_OBJ_HEAD *)raw;
        man->shape[obj_head.obj_id].pRawData = (unsigned int *)raw;
        man->shape[obj_head.obj_id].pRefPacket = (unsigned int *)ref;
        ref[0].ui32[0] = 0x30000004;
        ref[0].ui32[1] = (unsigned int)(raw + 1) & 0x7FFFFFFF;
        ref[0].ui32[2] = 0x1000101;
        ref[0].ui32[3] = 0x6D080000;
        raw += 6;
        ref[1].ui32[0] = 0x30000006;
        ref[1].ui32[1] = (unsigned int)&man->shape[obj_head.obj_id] & 0x7FFFFFFF;
        ref[1].ui32[2] = 0x1000101;
        ref[1].ui32[3] = 0x6C060006;
        ref[2].ui32[0] = 0x10000000;
        ref[2].ui32[1] = 0;
        ref[2].ui32[2] = 0x14000000;
        ref[2].ui32[3] = 0x11000000;
        ref += 3;
        for (; obj_head.geom_num != 0; obj_head.geom_num--) {
            geom_head = *(struct SHADOW_GEOM_HEAD *)raw;
            if (geom_head.prim == 1 || geom_head.prim == 2) {
                ref[0].ui32[0] = 0x30000002;
                ref[0].ui32[1] = (unsigned int)raw;
                ref[0].ui32[2] = 0x1000101;
                ref[0].ui32[3] = 0x6D048000;
                ref[1].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                ref[1].ui32[1] = (unsigned int)(raw + 2);
                ref[1].ui32[2] = 0x1000102;
                ref[1].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008005;
                ref += 2;
            } else if (geom_head.prim == 5 || geom_head.prim == 6) {
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
            } else if (geom_head.prim == 3 || geom_head.prim == 4) {
                ref[0].ui32[0] = 0x30000001;
                ref[0].ui32[1] = (unsigned int)raw;
                ref[0].ui32[2] = 0x1000101;
                ref[0].ui32[3] = 0x6D028000;
                ref[1].ui32[0] = 0x30000001;
                ref[1].ui32[1] = (unsigned int)(raw + 1);
                ref[1].ui32[2] = 0x1000203;
                ref[1].ui32[3] = 0x6D028002;
                ref[2].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                ref[2].ui32[1] = (unsigned int)(raw + 2);
                ref[2].ui32[2] = 0x1000102;
                ref[2].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008005;
                ref += 3;
            }
            if (spot_cam_angle < 0.0f) {
                if (geom_head.prim == 7) {
                    ref[0].ui32[0] = 0x30000002;
                    ref[0].ui32[1] = (unsigned int)raw;
                    ref[0].ui32[2] = 0x1000101;
                    ref[0].ui32[3] = 0x6D048000;
                    ref[1].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                    ref[1].ui32[1] = (unsigned int)(raw + 2);
                    ref[1].ui32[2] = 0x1000102;
                    ref[1].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008005;
                    ref += 2;
                } else if (geom_head.prim == 8 || geom_head.prim == 9) {
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
                } else if (geom_head.prim == 10) {
                    ref[0].ui32[0] = 0x30000001;
                    ref[0].ui32[1] = (unsigned int)raw;
                    ref[0].ui32[2] = 0x1000101;
                    ref[0].ui32[3] = 0x6D028000;
                    ref[1].ui32[0] = 0x30000001;
                    ref[1].ui32[1] = (unsigned int)(raw + 1);
                    ref[1].ui32[2] = 0x1000203;
                    ref[1].ui32[3] = 0x6D028002;
                    ref[2].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                    ref[2].ui32[1] = (unsigned int)(raw + 2);
                    ref[2].ui32[2] = 0x1000102;
                    ref[2].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008005;
                    ref += 3;
                }
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

/** Builds the reference tags of a background map's shadow volumes for a parallel light. */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 533
void sh2shd_make_reftag_pool_outdoor_for_parallel(struct SHADOW_OUTDOOR_MAN *man, struct SHADOW_PACKET_BUF *ref_packet) {
    struct SHADOW_OUTDOOR_HEAD outdoor_head;
    struct SHADOW_OUTDOOR_OBJ_HEAD obj_head;
    struct SHADOW_GEOM_HEAD geom_head;
    union Q_WORDDATA *ref;
    union Q_WORDDATA *raw;

    ref = ref_packet->curr;
    raw = man->raw_data;
    outdoor_head = *(struct SHADOW_OUTDOOR_HEAD *)raw;
    raw++;
    man->map_id = outdoor_head.map_id;
    for (; outdoor_head.obj_num != 0; outdoor_head.obj_num--) {
        obj_head = *(struct SHADOW_OUTDOOR_OBJ_HEAD *)raw;
        man->shape[obj_head.obj_id].pRawData = (unsigned int *)raw;
        man->shape[obj_head.obj_id].pRefPacket = (unsigned int *)ref;
        ref[0].ui32[0] = 0x30000004;
        ref[0].ui32[1] = (unsigned int)(raw + 1) & 0x7FFFFFFF;
        ref[0].ui32[2] = 0x1000101;
        ref[0].ui32[3] = 0x6D080000;
        raw += 6;
        ref[1].ui32[0] = 0x30000006;
        ref[1].ui32[1] = (unsigned int)&man->shape[obj_head.obj_id] & 0x7FFFFFFF;
        ref[1].ui32[2] = 0x1000101;
        ref[1].ui32[3] = 0x6C060006;
        ref[2].ui32[0] = 0x10000000;
        ref[2].ui32[1] = 0;
        ref[2].ui32[2] = 0x14000000;
        ref[2].ui32[3] = 0x11000000;
        ref += 3;
        for (; obj_head.geom_num != 0; obj_head.geom_num--) {
            geom_head = *(struct SHADOW_GEOM_HEAD *)raw;
            if (geom_head.prim == 1 || geom_head.prim == 2) {
                ref[0].ui32[0] = 0x30000002;
                ref[0].ui32[1] = (unsigned int)raw;
                ref[0].ui32[2] = 0x1000101;
                ref[0].ui32[3] = 0x6D048000;
                ref[1].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                ref[1].ui32[1] = (unsigned int)(raw + 2);
                ref[1].ui32[2] = 0x1000102;
                ref[1].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008005;
                ref[2].ui32[0] = 0x10000000;
                ref[2].ui32[1] = 0;
                ref[2].ui32[2] = 0;
                ref[2].ui32[3] = 0x17000000;
                ref += 3;
            }
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
                ref[3].ui32[0] = 0x10000000;
                ref[3].ui32[1] = 0;
                ref[3].ui32[2] = 0;
                ref[3].ui32[3] = 0x17000000;
                ref += 4;
            } else if (geom_head.prim == 3 || geom_head.prim == 4) {
                ref[0].ui32[0] = 0x30000001;
                ref[0].ui32[1] = (unsigned int)raw;
                ref[0].ui32[2] = 0x1000101;
                ref[0].ui32[3] = 0x6D028000;
                ref[1].ui32[0] = 0x30000001;
                ref[1].ui32[1] = (unsigned int)(raw + 1);
                ref[1].ui32[2] = 0x1000203;
                ref[1].ui32[3] = 0x6D028002;
                ref[2].ui32[0] = (geom_head.ee_memory_size - 2) | 0x30000000;
                ref[2].ui32[1] = (unsigned int)(raw + 2);
                ref[2].ui32[2] = 0x1000102;
                ref[2].ui32[3] = ((geom_head.send_data_num - 3) << 16) | 0x6D008005;
                ref[3].ui32[0] = 0x10000000;
                ref[3].ui32[1] = 0;
                ref[3].ui32[2] = 0;
                ref[3].ui32[3] = 0x17000000;
                ref += 4;
            }
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

/** Adds a background map's shadow reference tags to the kick packet, disabling switched-off objects. */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 1140
void sh2shd_add_outdoor_to_kick_packet(struct SHADOW_OUTDOOR_MAN *man, struct SHADOW_PACKET_BUF *kick_packet) {
    int i;
    union Q_WORDDATA *top;
    union Q_WORDDATA *curr;
    unsigned int *reftag_pool_addr;
    int count_call_tag_num;
    int j;

    curr = kick_packet->curr;
    count_call_tag_num = 0;
    curr->ui32[0] = 0x30000003;
    curr->ui32[1] = (unsigned long)(unsigned int)&man->light_pos & 0x7FFFFFFF;
    curr->ui32[2] = 0x1000101;
    curr->ui32[3] = 0x6C03000C;
    curr++;
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
    assert_dw(kick_packet->curr - kick_packet->head < SHD_KICK_PACKET_SIZE);
    for (i = 0; i < 4; i++) {
        if (man->map_id == shadow_off_work_bg[i].map_id) {
            for (j = 0; j < 32; j++) {
                if (shadow_off_work_bg[i].obj_id[j] == -1) {
                    break;
                }
                if (shadow_off_work_bg[i].obj_id[j] < count_call_tag_num) {
                    top[shadow_off_work_bg[i].obj_id[j]].ui32[0] = 0x10000000;
                }
                top[shadow_off_work_bg[i].obj_id[j]].ui32[1] = 0;
                top[shadow_off_work_bg[i].obj_id[j]].ui32[2] = 0;
                top[shadow_off_work_bg[i].obj_id[j]].ui32[3] = 0;
            }
        }
    }
    kick_packet->curr = curr;
}

/**
 * Per-frame update of a background shadow manager for a spot light: the light position in
 * each object's local space and the shadow length and clip kind; then moves the light 300
 * units back along its horizontal direction.
 */
void sh2shd_renew_outdoor_man(struct SHADOW_OUTDOOR_MAN *man, float *spot_pos, struct SHADOW_ENV *shadow_env) {
    int i;
    float world_local[4][4];
    struct SHADOW_GEOM_HEAD geom_head;
    float light_pos_revision[4];

    for (i = 0; i < man->obj_num; i++) {
        geom_head = *(struct SHADOW_GEOM_HEAD *)((union Q_WORDDATA *)man->shape[i].pRawData + 6);
        sceVu0InversMatrix(world_local, man->shape[i].local_world);
        sceVu0ApplyMatrix(man->shape[i].local_light_position, world_local, spot_pos);
        if (geom_head.prim == 3) {
            if (shadow_env->leng < 8000.0f) {
                man->shape[i].length.fl32[0] = 8000.0f;
            } else {
                man->shape[i].length.fl32[0] = shadow_env->leng;
            }
        } else if (geom_head.prim == 4) {
            if (shadow_env->leng < 10000.0f) {
                man->shape[i].length.fl32[0] = 10000.0f;
            } else {
                man->shape[i].length.fl32[0] = shadow_env->leng;
            }
        } else {
            man->shape[i].length.fl32[0] = shadow_env->leng;
        }
        man->shape[i].length.si32[1] = shadow_env->clip_kind;
    }
    vcopy_gcc(light_pos_revision, man->light_dir);
    light_pos_revision[1] = 0.0f;
    _shNormalize(light_pos_revision, light_pos_revision);
    _shScaleVector(light_pos_revision, light_pos_revision, 300.0f);
    _shSubVector(light_pos_revision, man->light_pos, light_pos_revision);
    vcopy_gcc(man->light_pos, light_pos_revision);
}

/** sh2shd_renew_outdoor_man for a parallel light (the light position is not moved). */
void sh2shd_renew_outdoor_man_for_parallel(struct SHADOW_OUTDOOR_MAN *man, float *spot_pos, struct SHADOW_ENV *shadow_env) {
    int i;
    float world_local[4][4];
    struct SHADOW_GEOM_HEAD geom_head;

    for (i = 0; i < man->obj_num; i++) {
        geom_head = *(struct SHADOW_GEOM_HEAD *)((union Q_WORDDATA *)man->shape[i].pRawData + 6);
        sceVu0InversMatrix(world_local, man->shape[i].local_world);
        sceVu0ApplyMatrix(man->shape[i].local_light_position, world_local, spot_pos);
        if (geom_head.prim == 3) {
            if (shadow_env->leng < 8000.0f) {
                man->shape[i].length.fl32[0] = 8000.0f;
            } else {
                man->shape[i].length.fl32[0] = shadow_env->leng;
            }
        } else if (geom_head.prim == 4) {
            if (shadow_env->leng < 10000.0f) {
                man->shape[i].length.fl32[0] = 10000.0f;
            } else {
                man->shape[i].length.fl32[0] = shadow_env->leng;
            }
        } else {
            man->shape[i].length.fl32[0] = shadow_env->leng;
        }
        man->shape[i].length.si32[1] = shadow_env->clip_kind;
    }
}
