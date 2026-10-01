/*
 * Shadow manager (sh2shd): the per-frame shadow system. Keeps the characters and background
 * maps that cast stencil shadows, picks their light (flashlight spot, point, parallel or
 * James's self-shadow), builds the reference tag pool and the VU1 kick packet, and does the
 * same for drop (blob) shadows; also the lists of shadows switched off per map or demo.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "libc/math.h"
#include "sdk/libvu0.h"

int check_self_spot(unsigned short kind);
int check_self_para(unsigned short kind);

#define OFF_OBJ_MAX_NUM_BG 32

extern unsigned int Shadow_micro_code[];
extern unsigned int Shadow_micro_code_parallel[];
extern unsigned int Drop_Shadow_micro_code[];

/*
 * x*x + y*y + z*z of one vector: sh_vu0.h's _shInnerProduct(v, v) with the second y load
 * written as a mov.s. Matching: kept local as a native asm block: _shInnerProduct(v, v) compiles
 * differently here.
 */
static inline float _shSquareLength(float *v) {
    float r;

    asm {
        lwc1    r, 0x0(v)
        lwc1    $f8, 0x0(v)
        lwc1    $f9, 0x4(v)
        mov.s   $f10, $f9
        mula.s  r, $f8
        lwc1    r, 0x8(v)
        lwc1    $f8, 0x8(v)
        madda.s $f9, $f10
        madd.s  r, r, $f8
    }
    return r;
}

struct SHADOW_MICRO_FRAME shadow_micro_frame[2];
struct SHADOW_MICRO_FRAME shadow_micro_frame_parallel[3];
struct SHADOW_MICRO_FRAME shadow_micro_frame_point;
struct DROP_SHADOW_MICRO_FRAME drop_shadow_micro_frame;
struct SHADOW_MAN shadow_man;
struct SHADOW_OFF_WORK_CHAR shadow_off_work_char[2];
struct SHADOW_OFF_WORK_BG shadow_off_work_bg[4];
struct JMS_SHADOW_ENV jms_shadow_env;
float light_pos_for_jms[4];
float light_dir_for_jms[4];
float light_param_for_jms[4];
short jms_added_flag;
static union Q_WORDDATA Shadow_Calcwork[6400];
static union Q_WORDDATA Shadow_Kick_Packet[640];
static union Q_WORDDATA Shadow_REFtag_Packet[6400];
struct utilHeapCtrl *shadow_calcheap;

/** Initializes the shadow system: managers, calculation heap, packet buffers, off lists and microprogram frames. */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 107
void sh2shd_init_shadow(void) {
    int i;

    shadow_man.change_flag = 0;
    shadow_man.spot_char_num = 0;
    shadow_man.spot_bg_num = 0;
    shadow_man.self_num = 0;
    shadow_man.parallel_char_num = 0;
    shadow_man.parallel_bg_num = 0;
    shadow_man.point_char_num = 0;
    shadow_man.char_man_num = 0;
    shadow_man.outdoor_man_num = 0;
    shadow_man.enemy_num = 0;
    shadow_man.chr_shadow_switch = 1;
    shadow_man.bg_shadow_switch = 1;
    for (i = 0; i < 16; i++) {
        shadow_man.char_man[i] = NULL;
    }
    for (i = 0; i < 4; i++) {
        shadow_man.outdoor_man[i] = NULL;
    }
    shadow_calcheap = utilHeapInit(&Shadow_Calcwork[1], sizeof(Shadow_Calcwork));
    assert_dw(shadow_calcheap);
    sh2shd_init_packet_buf(&shadow_man.kick_packet, Shadow_Kick_Packet);
    sh2shd_init_packet_buf(&shadow_man.reftag_pool, Shadow_REFtag_Packet);
    for (i = 0; i < 2; i++) {
        int j;

        shadow_off_work_char[i].kind = 0;
        shadow_off_work_char[i].id = -1;
        for (j = 0; j < 22; j++) {
            shadow_off_work_char[i].obj_id[j] = -1;
        }
    }
    for (i = 0; i < 4; i++) {
        int j;

        shadow_off_work_bg[i].map_id = -1;
        for (j = 0; j < 32; j++) {
            shadow_off_work_bg[i].obj_id[j] = -1;
        }
    }
    shadow_micro_init(&shadow_micro_frame[0], &shGs_AllEnv);
    shadow_micro_init(&shadow_micro_frame[1], &shGs_AllEnv);
    shadow_micro_init(&shadow_micro_frame_parallel[0], &shGs_AllEnv);
    shadow_micro_init(&shadow_micro_frame_point, &shGs_AllEnv);
    shadow_micro_init(&shadow_micro_frame_parallel[1], &shGs_AllEnv);
    shadow_micro_init(&shadow_micro_frame_parallel[2], &shGs_AllEnv);
    jms_added_flag = 0;
}

/** Frees every character and background shadow manager. */
void sh2shd_reset_shadow(void) {
    int i;

    for (i = 0; i < 16; i++) {
        if (shadow_man.char_man[i] != NULL) {
            utilHeapFree(shadow_man.char_man[i]->shape);
            shadow_man.char_man[i]->shape = NULL;
            utilHeapFree(shadow_man.char_man[i]);
            shadow_man.char_man[i] = NULL;
        }
    }
    for (i = 0; i < 4; i++) {
        if (shadow_man.outdoor_man[i] != NULL) {
            utilHeapFree(shadow_man.outdoor_man[i]->shape);
            shadow_man.outdoor_man[i]->shape = NULL;
            utilHeapFree(shadow_man.outdoor_man[i]);
            shadow_man.outdoor_man[i] = NULL;
        }
    }
    shadow_man.change_flag = 0;
    shadow_man.spot_char_num = 0;
    shadow_man.spot_bg_num = 0;
    shadow_man.self_num = 0;
    shadow_man.point_char_num = 0;
    shadow_man.parallel_char_num = 0;
    shadow_man.parallel_bg_num = 0;
    shadow_man.char_man_num = 0;
    shadow_man.outdoor_man_num = 0;
    shadow_man.enemy_num = 0;
    jms_added_flag = 0;
    shadow_man.chr_shadow_switch = 1;
    shadow_man.bg_shadow_switch = 1;
}

/**
 * Registers a character as a shadow caster for this frame (with per-demo and per-map light
 * overrides), creating its manager on first use.
 * @param scp         the character
 * @param raw_data    its shadow data
 * @param light_kind  shadow light kind
 * @param light_pos   light position
 * @param light_dir   light direction
 * @param light_param light cone parameters
 * @return 1, or 0 when the character casts no shadow
 */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 302
int sh2shd_add_char(struct SubCharacter *scp, union Q_WORDDATA *raw_data, short light_kind, float *light_pos, float *light_dir, float *light_param) {
    union Q_WORDDATA *cur;
    struct SHADOW_CHAR_HEAD char_head;
    int i;
    int demo_no;
    int glb_coord;
    int map_id;
    float chr_pos[4];
    float cam_pos[4];
    float light_vec[4];

    demo_no = DramaDemoNumber();
    get_map_id(&glb_coord, &map_id);
    if (demo_no == 0x4E) {
        if (scp->kind == 0x104) {
            light_kind = 4;
            light_dir[0] = -0.00404023f;
            light_dir[1] = 0.6262416f;
            light_dir[2] = 0.7796187f;
            light_dir[3] = 0.0f;
        }
    } else if (glb_coord == 9 && map_id == 0x1E && scp->kind == 0x201 && light_kind == 5) {
        return 1;
    }
    if (light_kind == 9 && (glb_coord != 9 || map_id != 0x48)) {
        return 1;
    }
    if (scp->kind >= 0x100 && scp->kind < 0x104) {
        if (demo_no) {
            sh2shd_get_demo_jms_shadow_env(&jms_shadow_env, demo_no);
        } else {
            sh2shd_get_jms_shadow_env(&jms_shadow_env, light_kind, light_dir);
        }
        if (jms_shadow_env.light_kind == -1) {
            jms_shadow_env.light_kind = light_kind;
        }
        if (jms_shadow_env.light_kind == 5 && !demo_no) {
            vwGetViewPosition(cam_pos);
            cam_pos[1] = 0.0f;
            vcopy_gcc(chr_pos, &scp->pos);
            chr_pos[1] = 0.0f;
            _shSubVector(cam_pos, cam_pos, chr_pos);
            _shNormalize(cam_pos, cam_pos);
            vcopy_gcc(light_vec, light_pos);
            light_vec[1] = 0.0f;
            _shSubVector(light_vec, light_vec, chr_pos);
            _shNormalize(light_vec, light_vec);
            if (_shInnerProduct(cam_pos, light_vec) < -0.707f) {
                vwGetViewPosition(cam_pos);
                _shSubVector(cam_pos, (float *)&scp->pos, cam_pos);
                if (sqrtf(_shSquareLength(cam_pos)) < 1800.0f) {
                    sh2shd_del_jms_upper_body(scp->kind, scp->id);
                }
            }
        }
        if (jms_shadow_env.light_kind == 9) {
            return 1;
        }
        jms_added_flag = 1;
    } else if (scp->kind >= 0x800 && scp->kind < 0x809) {
        if (jms_added_flag == 0) {
            return 1;
        }
        light_kind = jms_shadow_env.light_kind;
    } else if (scp->kind >= 0x200 && scp->kind < 0x300) {
        if (((scp->battle.status >> 1) & 1) || ((scp->battle.status >> 2) & 1)) {
            return 1;
        }
    }
    if (shadow_man.char_man_num >= 16) {
        return 0;
    }
    cur = raw_data;
    char_head = *(struct SHADOW_CHAR_HEAD *)cur;
    for (i = 0; i <= 16; i++) {
        if (shadow_man.char_man[i] == NULL) {
            break;
        }
    }
    if (i == 16) {
        return 0;
    }
    shadow_man.char_man[i] = utilHeapMalloc(shadow_calcheap, sizeof(struct SHADOW_CHAR_MAN));
    if (shadow_man.char_man[i] == NULL) {
        assert(shadow_man.char_man[i] != 0);
    }
    shadow_man.char_man[i]->shape = utilHeapMalloc(shadow_calcheap, char_head.obj_num * sizeof(struct SHADOW_SHAPE_FRAME));
    /* Matching: the #line keeps the assert string below on the original's line. */
#line 400
    assert(shadow_man.char_man[i]->shape != 0);
    shadow_man.char_man_num++;
    if (scp->kind >= 0x100 && scp->kind < 0x104 && jms_shadow_env.light_kind >= 0) {
        sh2shd_init_char_man(shadow_man.char_man[i], scp, raw_data, scp->kind, scp->id, jms_shadow_env.light_kind, light_pos, light_dir, light_param);
        light_kind = jms_shadow_env.light_kind;
        vcopy_gcc(light_pos_for_jms, light_pos);
        vcopy_gcc(light_dir_for_jms, light_dir);
        vcopy_gcc(light_param_for_jms, light_param);
    } else if (scp->kind >= 0x800 && scp->kind < 0x809) {
        sh2shd_init_char_man(shadow_man.char_man[i], scp, raw_data, scp->kind, scp->id, light_kind, light_pos_for_jms, light_dir_for_jms, light_param_for_jms);
    } else {
        sh2shd_init_char_man(shadow_man.char_man[i], scp, raw_data, scp->kind, scp->id, light_kind, light_pos, light_dir, light_param);
    }
    switch (light_kind) {
    case 0:
        shadow_man.spot_char_num++;
        break;
    case 6:
        shadow_man.spot_char_num++;
        break;
    case 7:
        shadow_man.spot_char_num++;
        break;
    case 8:
        shadow_man.spot_char_num++;
        break;
    case 1:
        shadow_man.self_num++;
        break;
    case 2:
        shadow_man.self_num++;
        break;
    case 3:
        shadow_man.self_num++;
        break;
    case 4:
        shadow_man.parallel_char_num++;
        break;
    case 5:
        shadow_man.point_char_num++;
        break;
    }
    if (scp->kind >= 0x200 && scp->kind < 0x300) {
        shadow_man.enemy_num++;
    }
    shadow_man.change_flag = 1;
    return 1;
}

/**
 * Adds a character's stencil shadow (sh2shd_add_char with its shadow data kg1).
 * Declared int in the DWARF, but no value is returned. kind and id are not used.
 */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 491
int sh2shd_Draw_ShadowChar(struct SubCharacter *scp, void *kg1, unsigned short kind, unsigned short id, short light_kind, float *light_pos, float *light_dir, float *light_param) {
    int check;

    check = sh2shd_add_char(scp, kg1, light_kind, light_pos, light_dir, light_param);
    assert(check);
}

/**
 * Registers background shadow map id as a shadow caster, creating its manager on first use.
 * @return 1
 */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 510
int sh2shd_add_map(unsigned short id, union Q_WORDDATA *raw_data, short light_kind, float *light_pos, float *light_dir, float *light_param) {
    union Q_WORDDATA *cur;
    struct SHADOW_OUTDOOR_HEAD outdoor_head;
    int i;

    if (shadow_man.outdoor_man_num >= 4) {
        return 0;
    }
    cur = raw_data;
    outdoor_head = *(struct SHADOW_OUTDOOR_HEAD *)cur;
    if (outdoor_head.map_id == 7) {
        return;
    }
    for (i = 0; i <= 4; i++) {
        if (shadow_man.outdoor_man[i] == NULL) {
            break;
        }
    }
    if (i == 4) {
        return 0;
    }
    shadow_man.outdoor_man[i] = utilHeapMalloc(shadow_calcheap, sizeof(struct SHADOW_OUTDOOR_MAN));
    if (shadow_man.outdoor_man[i] == NULL) {
        assert_dw(shadow_man.outdoor_man[i] != 0);
    }
    shadow_man.outdoor_man[i]->shape = utilHeapMalloc(shadow_calcheap, outdoor_head.obj_num * sizeof(struct SHADOW_SHAPE_FRAME));

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 545
    assert_dw(shadow_man.outdoor_man[i]->shape != 0);
    shadow_man.outdoor_man_num++;
    sh2shd_init_outdoor_man2(shadow_man.outdoor_man[i], raw_data, id, light_kind, light_pos, light_dir, light_param);
    switch (light_kind) {
    case 0:
        shadow_man.spot_bg_num++;
        break;
    case 4:
        shadow_man.parallel_bg_num++;
        break;
    }
    shadow_man.change_flag = 1;
    return 1;
}

/* Copies the 4x4 matrix s to d through t6/t7: asm_helpers.h's mcopy written as GCC-style asm, with
 * the destination first. Matching: kept local: MWCC optimizes around this form (no other file has it). */
static inline void mcopy_gcc(void *d, void *s) {
    __asm__ __volatile__("
    lq t6, 0x0(%1)
    lq t7, 0x10(%1)
    sq t6, 0x0(%0)
    sq t7, 0x10(%0)
    lq t6, 0x20(%1)
    lq t7, 0x30(%1)
    sq t6, 0x20(%0)
    sq t7, 0x30(%0)
    " : : "r"(d), "r"(s));
}

static void sh2shd_renew_shadow_man(struct sh2gfw_CAMERA *cam, int glb_coord, int map_id);
static void make_chara_reftag_pool_and_kick_packet_for_spot(void);
static void make_bg_reftag_pool_and_kick_packet_for_spot(float spot_cam_angle);
static void make_chara_reftag_pool_and_kick_packet_for_point(void);
static void make_chara_reftag_pool_and_kick_packet_for_self(void);
static void make_chara_reftag_pool_and_kick_packet_for_parallel(void);
static void make_bg_reftag_pool_and_kick_packet_for_parallel(void);
static float get_spot_camera_angle(float *cam_pos, float *light_pos, float *light_dir, float *light_param);

/**
 * Per-frame stencil shadows: rebuilds the reference tag pools and kick packet when the set of
 * casters changed, and updates the managers for the camera and light.
 * @return the kick packet
 */
union Q_WORDDATA *sh2shd_exe_shadow(struct sh2gfw_CAMERA *cam) {
    int i;
    float cam_pos[4];
    float spot_cam_angle_bg;
    int map_id;
    int glb_coord;
    int demo_no;
    int count;

    demo_no = DramaDemoNumber();
    if (!shadow_man.change_flag) {
        return NULL;
    }
    if (!shadow_man.chr_shadow_switch) {
        if (!shadow_man.bg_shadow_switch) {
            return NULL;
        }
        if (!shadow_man.outdoor_man_num) {
            return NULL;
        }
        shadow_man.spot_char_num = 0;
        shadow_man.self_num = 0;
        shadow_man.parallel_char_num = 0;
        shadow_man.point_char_num = 0;
    } else if (!shadow_man.bg_shadow_switch) {
        if (!shadow_man.chr_shadow_switch) {
            return NULL;
        }
        if (!shadow_man.char_man_num) {
            return NULL;
        }
        shadow_man.spot_bg_num = 0;
        shadow_man.parallel_bg_num = 0;
    }
    get_map_id(&glb_coord, &map_id);
    if (demo_no) {
        sh2shd_demo_shadow_off(demo_no);
    } else {
        sh2shd_shadow_off(glb_coord, map_id);
    }
    vwGetViewPosition(cam_pos);
    if (shadow_man.change_flag == 1) {
        sh2shd_reset_packet_buf(&shadow_man.reftag_pool);
        sh2shd_reset_packet_buf(&shadow_man.kick_packet);
        if (shadow_man.spot_char_num > 0) {
            shadow_micro_init(&shadow_micro_frame[0], &shGs_AllEnv);
            shadow_add_micro2kick_packet(&shadow_micro_frame[0], &shadow_man.kick_packet, Shadow_micro_code);
            make_chara_reftag_pool_and_kick_packet_for_spot();
        }
        if (shadow_man.spot_bg_num > 0) {
            for (i = 0; i < 4; i++) {
                if (shadow_man.outdoor_man[i] != NULL) {
                    break;
                }
            }
            spot_cam_angle_bg = get_spot_camera_angle(cam_pos, shadow_man.outdoor_man[i]->light_pos, shadow_man.outdoor_man[i]->light_dir, shadow_man.outdoor_man[i]->light_param);
            shadow_micro_init(&shadow_micro_frame[1], &shGs_AllEnv);
            shadow_add_micro2kick_packet(&shadow_micro_frame[1], &shadow_man.kick_packet, Shadow_micro_code);
            make_bg_reftag_pool_and_kick_packet_for_spot(spot_cam_angle_bg);
        }
        if (shadow_man.self_num > 0) {
            shadow_micro_init(&shadow_micro_frame_parallel[0], &shGs_AllEnv);
            shadow_add_micro2kick_packet(&shadow_micro_frame_parallel[0], &shadow_man.kick_packet, Shadow_micro_code_parallel);
            make_chara_reftag_pool_and_kick_packet_for_self();
        }
        if (shadow_man.point_char_num > 0) {
            shadow_micro_init(&shadow_micro_frame_point, &shGs_AllEnv);
            shadow_add_micro2kick_packet(&shadow_micro_frame_point, &shadow_man.kick_packet, Shadow_micro_code);
            make_chara_reftag_pool_and_kick_packet_for_point();
        }
        if (shadow_man.parallel_char_num > 0) {
            shadow_micro_init(&shadow_micro_frame_parallel[1], &shGs_AllEnv);
            shadow_add_micro2kick_packet(&shadow_micro_frame_parallel[1], &shadow_man.kick_packet, Shadow_micro_code_parallel);
            make_chara_reftag_pool_and_kick_packet_for_parallel();
        }
        if (shadow_man.parallel_bg_num > 0) {
            shadow_micro_init(&shadow_micro_frame_parallel[2], &shGs_AllEnv);
            shadow_add_micro2kick_packet(&shadow_micro_frame_parallel[2], &shadow_man.kick_packet, Shadow_micro_code_parallel);
            make_bg_reftag_pool_and_kick_packet_for_parallel();
        }
        shadow_man.change_flag = 0;
    }
    sh2shd_renew_shadow_man(cam, glb_coord, map_id);
    return shadow_man.kick_packet.head;
}

static void sh2shd_renew_shadow_man(struct sh2gfw_CAMERA *cam, int glb_coord, int map_id) {
    int i;
    float virtual_spot_c_back[4];
    float drop_shadow_matrix[4][4];
    float spot_cam_vector[4];
    float cam_pos[4];
    struct SHADOW_ENV shadow_env;
    int demo_no;

    demo_no = DramaDemoNumber();
    vwGetViewPosition(cam_pos);
    if (shadow_man.spot_char_num > 0) {
        shadow_set_micro_params(&shadow_micro_frame[0], cam, drop_shadow_matrix, 0);
        if (shadow_man.char_man_num > 0) {
            for (i = 0; i < 16; i++) {
                float dis;
                float inverse_vector[4];
                float length_chara; /* @bug never set; passed to sh2shd_renew_char_man_for_spot, which ignores it */
                float spot_cam_angle;
                float virtual_spot_pos[4];
                float chara_pos[4];
                float height;
                float normal[4];

                if (shadow_man.char_man[i] != NULL &&
                    (shadow_man.char_man[i]->light_kind == 0 || shadow_man.char_man[i]->light_kind == 6 ||
                     shadow_man.char_man[i]->light_kind == 7 || shadow_man.char_man[i]->light_kind == 8)) {
                    spot_cam_angle = get_spot_camera_angle(cam_pos, shadow_man.char_man[i]->light_pos, shadow_man.char_man[i]->light_dir, shadow_man.char_man[i]->light_param);
                    _shSubVector(spot_cam_vector, shadow_man.char_man[i]->light_pos, cam_pos);
                    spot_cam_vector[3] = _shVectorLength(spot_cam_vector);
                    vcopy_gcc(virtual_spot_pos, shadow_man.char_man[i]->light_pos);
                    if (spot_cam_angle > 0.0f) {
                        virtual_spot_pos[1] = shadow_man.char_man[i]->light_pos[1] - 400.0f * spot_cam_angle;
                    }
                    if (!demo_no && !(shadow_man.char_man[i]->kind >= 0x100 && shadow_man.char_man[i]->kind <= 0x103) &&
                        !(shadow_man.char_man[i]->kind >= 0x800 && shadow_man.char_man[i]->kind <= 0x808)) {
                        get_distance_from_light_to_chara(shadow_man.char_man[i], inverse_vector);
                        _shAddVector(virtual_spot_pos, virtual_spot_pos, inverse_vector);
                    }
                    if (!demo_no) {
                        sh2shd_get_shadow_env_chr(&shadow_env, shadow_man.char_man[i], glb_coord, map_id);
                        if (spot_cam_angle > 0.5f && shadow_env.leng > 1.5f * spot_cam_vector[3]) {
                            shadow_env.leng = 1.5f * spot_cam_vector[3];
                        }
                    } else {
                        sh2shd_get_demo_shadow_env_for_char(&shadow_env, shadow_man.char_man[i], demo_no);
                    }
                    if (shadow_man.char_man[i]->light_kind >= 6 && shadow_man.char_man[i]->light_kind < 9) {
                        shCharacterGetGroundInfoForShadow(chara_pos, normal, &height, shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id);
                        sh2shd_calc_light_position_for_self_spot(virtual_spot_pos, shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id, &jms_shadow_env, chara_pos, height);
                        if (demo_no == 10) {
                            virtual_spot_pos[0] = -58387.0f;
                            virtual_spot_pos[1] = -1500.0f;
                            virtual_spot_pos[2] = 19326.0f;
                        }
                        shadow_env.clip_kind = 999;
                        shadow_env.leng = 6000.0f;
                    }
                    sh2shd_renew_char_man_for_spot(shadow_man.char_man[i], virtual_spot_pos, length_chara, &shadow_env);
                }
            }
        }
    }
    if (shadow_man.spot_bg_num > 0) {
        int spot_cam_flag;
        float spot_cam_angle;
        int count;

        count = 0;
        for (i = 0; i < 4; i++) {
            if (shadow_man.outdoor_man[i] != NULL) {
                count++;
                if (get_spot_camera_angle(cam_pos, shadow_man.outdoor_man[i]->light_pos, shadow_man.outdoor_man[i]->light_dir, shadow_man.outdoor_man[i]->light_param) > 0.0f) {
                    spot_cam_flag = 1;
                } else {
                    spot_cam_flag = 0;
                }
                vcopy_gcc(virtual_spot_c_back, shadow_man.outdoor_man[i]->light_pos);
                if (!demo_no) {
                    sh2shd_get_shadow_env_bg(&shadow_env, (struct SHADOW_OUTDOOR_HEAD *)shadow_man.outdoor_man[i]->raw_data);
                } else {
                    sh2shd_get_demo_shadow_env_for_bg(&shadow_env, demo_no);
                }
                shadow_set_micro_params(&shadow_micro_frame[1], cam, drop_shadow_matrix, spot_cam_flag);
                if (shadow_man.outdoor_man[i]->light_kind == 0) {
                    sh2shd_renew_outdoor_man(shadow_man.outdoor_man[i], virtual_spot_c_back, &shadow_env);
                }
            }
        }
    }
    if (shadow_man.point_char_num > 0) {
        shadow_set_micro_params(&shadow_micro_frame_point, cam, drop_shadow_matrix, 0);
        if (shadow_man.char_man_num > 0) {
            for (i = 0; i < 16; i++) {
                float light_leng;
                float vec[4];
                float chara_pos[4];

                if (shadow_man.char_man[i] != NULL && shadow_man.char_man[i]->light_kind == 5) {
                    if (demo_no) {
                        sh2shd_get_demo_shadow_env_for_char(&shadow_env, shadow_man.char_man[i], demo_no);
                    } else {
                        shadow_env.clip_kind = 999;
                        sh2shd_get_shadow_env_chr(&shadow_env, shadow_man.char_man[i], glb_coord, map_id);
                        shadow_env.leng = shadow_man.char_man[i]->light_param[2];
                        chara_pos[0] = shadow_man.char_man[i]->scp->pos.x;
                        chara_pos[1] = shadow_man.char_man[i]->scp->pos.y;
                        chara_pos[2] = shadow_man.char_man[i]->scp->pos.z;
                        chara_pos[3] = 1.0f;
                        _shSubVector(vec, chara_pos, shadow_man.char_man[i]->light_pos);
                        light_leng = _shSquareLength(vec);
                        light_leng = 0.3f * sqrtf(light_leng);
                        shadow_man.char_man[i]->light_pos[1] -= light_leng;
                        shadow_env.leng *= 1.2f;
                    }
                    sh2shd_renew_char_man(shadow_man.char_man[i], shadow_man.char_man[i]->light_pos, shadow_env.leng, &shadow_env);
                }
            }
        }
    }
    if (shadow_man.self_num > 0) {
        float chara_pos[4];
        float normal[4];
        float plane[4];
        float virtual_light_position[4];
        float tmp_light_position[4];
        float height;

        for (i = 0; i < 16; i++) {
            if (shadow_man.char_man[i] != NULL && shadow_man.char_man[i]->kind >= 0x100 && shadow_man.char_man[i]->kind < 0x104 &&
                check_self_para(shadow_man.char_man[i]->light_kind)) {
                shCharacterGetGroundInfoForShadow(chara_pos, normal, &height, shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id);
                chara_pos[1] = 100.0f + height;
                normal[0] = 0.0f;
                normal[1] = -1.0f;
                normal[2] = 0.0f;
                sh2shd_make_stencil_drop_shadow_plane(plane, chara_pos, normal);
                sh2shd_calc_light_position_for_self(virtual_light_position, shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id, &jms_shadow_env, chara_pos, height);
                vcopy_gcc(tmp_light_position, virtual_light_position);
                if (tmp_light_position[1] > 0.0f) {
                    tmp_light_position[1] *= -1.0f;
                }
                sceVu0DropShadowMatrix(drop_shadow_matrix, tmp_light_position, plane[0], plane[1], plane[2], 0);
                shadow_set_micro_params(&shadow_micro_frame_parallel[0], cam, drop_shadow_matrix, 0);
                _shAddVector(virtual_light_position, virtual_light_position, chara_pos);
                sh2shd_renew_char_man_parallel(shadow_man.char_man[i], virtual_light_position);
                break;
            }
        }
        for (i = 0; i < 16; i++) {
            if (shadow_man.char_man[i] != NULL && shadow_man.char_man[i]->kind >= 0x800 && shadow_man.char_man[i]->kind < 0x809) {
                if (shadow_man.char_man[i]->light_kind == 1 || shadow_man.char_man[i]->light_kind == 3) {
                    sh2shd_renew_char_man_parallel(shadow_man.char_man[i], virtual_light_position);
                }
                break;
            }
        }
    }
    if (shadow_man.parallel_char_num > 0) {
        int count;
        int weapon_flag;
        float light_pos[4];
        float dir[4];
        float mat[4][4];

        count = 0;
        for (i = 0; i < 16; i++) {
            if (shadow_man.char_man[i] != NULL) {
                if (!count) {
                    int glb_coord;
                    int map_id;

                    get_map_id(&glb_coord, &map_id);
                    count++;
                    vcopy_gcc(dir, shadow_man.char_man[i]->light_dir);
                    _shScaleVector(dir, dir, -1.0f);
                    if (demo_no == 0x4D) {
                        sceVu0DropShadowMatrix(drop_shadow_matrix, dir, 0.0f, 0.1f, 0.0f, 0);
                    } else {
                        sceVu0DropShadowMatrix(drop_shadow_matrix, dir, 0.0f, 100.0f, 0.0f, 0);
                    }
                    shCharacterGetPartsMatrixForShadow(mat, shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id, 0);
                    _shScaleVector(light_pos, dir, 10000.0f);
                    _shAddVector(light_pos, light_pos, mat[3]);
                    shadow_set_micro_params(&shadow_micro_frame_parallel[1], cam, drop_shadow_matrix, 0);
                }
                vcopy_gcc(shadow_man.char_man[i]->light_pos, light_pos);
                if (shadow_man.char_man[i]->light_kind == 4) {
                    sh2shd_renew_char_man_parallel(shadow_man.char_man[i], shadow_man.char_man[i]->light_pos);
                }
            }
        }
    }
    if (shadow_man.parallel_bg_num > 0) {
        int count;
        float dir[4];

        count = 0;
        for (i = 0; i < 4; i++) {
            if (shadow_man.outdoor_man[i] != NULL && shadow_man.outdoor_man[i]->light_kind == 4) {
                if (!count) {
                    count++;
                    vcopy_gcc(dir, shadow_man.outdoor_man[i]->light_dir);
                    _shScaleVector(dir, dir, -1.0f);
                    _shScaleVector(shadow_man.outdoor_man[i]->light_pos, dir, 10000.0f);
                    _shAddVector(shadow_man.outdoor_man[i]->light_pos, shadow_man.outdoor_man[i]->light_pos, shadow_man.outdoor_man[i]->shape->local_world[3]);
                    sceVu0DropShadowMatrix(drop_shadow_matrix, dir, 0.0f, 0.01f, 0.0f, 0);
                    shadow_set_micro_params(&shadow_micro_frame_parallel[2], cam, drop_shadow_matrix, 0);
                }
                sh2shd_get_shadow_env_bg(&shadow_env, (struct SHADOW_OUTDOOR_HEAD *)shadow_man.outdoor_man[i]->raw_data);
                if (shadow_man.outdoor_man[i]->light_kind == 4) {
                    sh2shd_renew_outdoor_man_for_parallel(shadow_man.outdoor_man[i], shadow_man.outdoor_man[i]->light_pos, &shadow_env);
                }
            }
        }
    }
}

static void make_chara_reftag_pool_and_kick_packet_for_spot(void) {
    int i;

    if (shadow_man.char_man_num > 0) {
        for (i = 0; i < 16; i++) {
            if (shadow_man.char_man[i] != NULL) {
                if (shadow_man.char_man[i]->kind >= 0x100 && shadow_man.char_man[i]->kind < 0x104) {
                    if (check_self_spot(shadow_man.char_man[i]->light_kind)) {
                        switch (shadow_man.char_man[i]->light_kind) {
                        case 7:
                            sh2shd_del_jms_upper_body(shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id);
                            break;
                        case 8:
                            sh2shd_del_jms_head(shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id);
                            break;
                        }
                        sh2shd_make_reftag_pool_char(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                        sh2shd_add_char_to_kick_packet(shadow_man.char_man[i], &shadow_man.kick_packet);
                    }
                } else if (shadow_man.char_man[i]->kind >= 0x800 && shadow_man.char_man[i]->kind < 0x809 && jms_added_flag == 1) {
                    if (check_self_spot(shadow_man.char_man[i]->light_kind)) {
                        if (shadow_man.char_man[i]->light_kind == 6 || shadow_man.char_man[i]->light_kind == 8) {
                            sh2shd_make_reftag_pool_char(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                            sh2shd_add_char_to_kick_packet(shadow_man.char_man[i], &shadow_man.kick_packet);
                        }
                    }
                } else if (shadow_man.char_man[i]->light_kind == 0) {
                    if (shadow_man.char_man[i]->kind == 0x105) {
                        sh2shd_make_reftag_pool_char_p_maria(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                    } else {
                        sh2shd_make_reftag_pool_char(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                    }
                    sh2shd_add_char_to_kick_packet(shadow_man.char_man[i], &shadow_man.kick_packet);
                }
            }
        }
    }
}

static void make_bg_reftag_pool_and_kick_packet_for_spot(float spot_cam_angle) {
    int i;

    for (i = 0; i < 4; i++) {
        if (shadow_man.outdoor_man[i] != NULL && shadow_man.outdoor_man[i]->light_kind == 0) {
            sh2shd_make_reftag_pool_outdoor(shadow_man.outdoor_man[i], &shadow_man.reftag_pool, spot_cam_angle);
            sh2shd_add_outdoor_to_kick_packet(shadow_man.outdoor_man[i], &shadow_man.kick_packet);
        }
    }
}

static void make_chara_reftag_pool_and_kick_packet_for_point(void) {
    int i;

    if (shadow_man.char_man_num > 0) {
        for (i = 0; i < 16; i++) {
            if (shadow_man.char_man[i] != NULL && shadow_man.char_man[i]->light_kind == 5) {
                if (shadow_man.char_man[i]->kind >= 0x800 && shadow_man.char_man[i]->kind < 0x809 && !jms_added_flag) {
                    continue;
                }
                sh2shd_make_reftag_pool_char(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                sh2shd_add_char_to_kick_packet(shadow_man.char_man[i], &shadow_man.kick_packet);
            }
        }
    }
}

static void make_chara_reftag_pool_and_kick_packet_for_self(void) {
    int i;

    if (shadow_man.char_man_num > 0) {
        for (i = 0; i < 16; i++) {
            if (shadow_man.char_man[i] != NULL) {
                if (shadow_man.char_man[i]->kind >= 0x100 && shadow_man.char_man[i]->kind < 0x104) {
                    switch (shadow_man.char_man[i]->light_kind) {
                    case 1:
                        break;
                    case 2:
                        sh2shd_del_jms_upper_body(shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id);
                        break;
                    case 3:
                        sh2shd_del_jms_head(shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id);
                        break;
                    }
                    sh2shd_make_reftag_pool_char(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                    sh2shd_add_char_to_kick_packet(shadow_man.char_man[i], &shadow_man.kick_packet);
                } else if (shadow_man.char_man[i]->kind >= 0x800 && shadow_man.char_man[i]->kind < 0x809 && jms_added_flag == 1) {
                    if (shadow_man.char_man[i]->light_kind == 1 || shadow_man.char_man[i]->light_kind == 3) {
                        sh2shd_make_reftag_pool_char(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                        sh2shd_add_char_to_kick_packet(shadow_man.char_man[i], &shadow_man.kick_packet);
                    }
                }
            }
        }
    }
}

static void make_chara_reftag_pool_and_kick_packet_for_parallel(void) {
    int i;

    if (shadow_man.char_man_num > 0) {
        for (i = 0; i < 16; i++) {
            if (shadow_man.char_man[i] != NULL) {
                if (shadow_man.char_man[i]->kind >= 0x800 && shadow_man.char_man[i]->kind < 0x809 && !jms_added_flag) {
                    continue;
                }
                if (shadow_man.char_man[i]->light_kind == 4) {
                    sh2shd_make_reftag_pool_char(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                    sh2shd_add_char_to_kick_packet(shadow_man.char_man[i], &shadow_man.kick_packet);
                }
            }
        }
    }
}

static void make_bg_reftag_pool_and_kick_packet_for_parallel(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (shadow_man.outdoor_man[i] != NULL && shadow_man.outdoor_man[i]->light_kind == 4) {
            sh2shd_make_reftag_pool_outdoor_for_parallel(shadow_man.outdoor_man[i], &shadow_man.reftag_pool);
            sh2shd_add_outdoor_to_kick_packet(shadow_man.outdoor_man[i], &shadow_man.kick_packet);
        }
    }
}

static float get_spot_camera_angle(float *cam_pos, float *light_pos, float *light_dir, float *light_param) {
    float spot_cam_dir[4];
    float inner_product;

    _shSubVector(spot_cam_dir, cam_pos, light_pos);
    spot_cam_dir[1] = spot_cam_dir[3] = 0.0f;
    _shNormalize(spot_cam_dir, spot_cam_dir);
    inner_product = _shInnerProduct(spot_cam_dir, light_dir);
    return inner_product;
}

/**
 * Returns the horizontal distance from a character's ground point to its light, and sets iv
 * to a push-away vector of length 250 - distance when the light is closer than 250.
 */
float get_distance_from_light_to_chara(struct SHADOW_CHAR_MAN *man, float *iv) {
    float distance;
    float chara_pos[4];

    shCharacterGetGroundInfoForShadow(chara_pos, iv, &distance, man->kind, man->id);
    _shSubVector(iv, man->light_pos, chara_pos);
    iv[1] = iv[3] = 0.0f;
    distance = _shSquareLength(iv);
    distance = sqrtf(distance);
    if (distance < 250.0f) {
        _shNormalize(iv, iv);
        _shScaleVector(iv, iv, 250.0f - distance);
    } else {
        iv[0] = iv[1] = iv[2] = 0.0f;
    }
    return distance;
}

/* Matching: 16-byte copy through a2, as a macro (the original's name for it is unknown). Its asm
   makes MWCC compile sh2shd_exe_drop_shadow without global CSE/DCE, as the original was (dead
   alpha_reg stores kept, every address recomputed). As an inline function the destination gets
   a parameter temporary of its own, which swaps v0/v1 in the two copies from test_reg. */
#define qcopy(d, s) \
    __asm__ __volatile__("lq $6, 0x0(%1)\n    sq $6, 0x0(%0)" : : "r"(d), "r"(s))

/** Per-frame drop (blob) shadows: builds their packet. Returns it. */
union Q_WORDDATA *sh2shd_exe_drop_shadow(struct sh2gfw_CAMERA *cam) {
    int i;
    struct SPOT_LIGHT spot;
    union Q_WORDDATA test_reg;
    int demo_no;
    struct DROP_SHADOW_ENV ds_env;
    int glb_coord;
    int map_id;
    union Q_WORDDATA alpha_reg;

    if (!shadow_man.change_flag) {
        return NULL;
    }
    if ((demo_no = DramaDemoNumber()) != 0) {
        sh2shd_get_demo_drop_shadow_env(&ds_env, demo_no);
    } else {
        get_map_id(&glb_coord, &map_id);
        sh2shd_get_drop_shadow_env(&ds_env, glb_coord, map_id, sh2jms.player->pos.x, sh2jms.player->pos.z);
    }
    if (shadow_man.change_flag == 1) {
        alpha_reg.ul64[1] = 0x42;
        alpha_reg.ul64[0] = 0x48;
        sh2shd_reset_packet_buf(&shadow_man.reftag_pool);
        sh2shd_reset_packet_buf(&shadow_man.kick_packet);
        drop_shadow_micro_init(&drop_shadow_micro_frame, &shGs_AllEnv, &ds_env);
        shadow_man.kick_packet.curr->ui32[0] = 0x10000008;
        shadow_man.kick_packet.curr->ui32[1] = 0;
        shadow_man.kick_packet.curr->ui32[2] = 0x11000000;
        shadow_man.kick_packet.curr->ui32[3] = 0x50000008;
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_ZBUF_B[0]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_ZBUF_B[1]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_ALPHA_A[0]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_ALPHA_A[1]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_FBA_B[0]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_FBA_B[1]);
        shadow_man.kick_packet.curr++;
        test_reg.ul64[1] = 0x47;
        test_reg.ul64[0] = 0x5400F;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_TEST_A[0]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &test_reg);
        shadow_man.kick_packet.curr++;
        drop_shadow_add_micro2kick_packet(&drop_shadow_micro_frame, &shadow_man.kick_packet, Drop_Shadow_micro_code);
        if (shadow_man.char_man_num > 0) {
            for (i = 0; i < 16; i++) {
                if (shadow_man.char_man[i] != NULL) {
                    if (shadow_man.char_man[i]->kind == 0x105) {
                        sh2shd_make_reftag_pool_char_for_drop_with_order_p_maria(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                    } else {
                        sh2shd_make_reftag_pool_char_for_drop_with_order(shadow_man.char_man[i], &shadow_man.reftag_pool, shadow_man.char_man[i]->raw_data);
                    }
                }
            }
        }
        if (shadow_man.char_man_num > 0) {
            for (i = 0; i < 16; i++) {
                if (shadow_man.char_man[i] != NULL) {
                    sh2shd_add_char_to_kick_packet(shadow_man.char_man[i], &shadow_man.kick_packet);
                }
            }
        }
        shadow_man.kick_packet.curr->ui32[0] = 0x10000006;
        shadow_man.kick_packet.curr->ui32[1] = 0;
        shadow_man.kick_packet.curr->ui32[2] = 0x11000000;
        shadow_man.kick_packet.curr->ui32[3] = 0x50000006;
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_ZBUF_A[0]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_ZBUF_A[1]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_FBA_A[0]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_FBA_A[1]);
        shadow_man.kick_packet.curr++;
        test_reg.ul64[1] = 0x47;
        test_reg.ul64[0] = 0x50003;
        qcopy(shadow_man.kick_packet.curr, &shGs_AllEnv.GsReg_TEST_A[0]);
        shadow_man.kick_packet.curr++;
        qcopy(shadow_man.kick_packet.curr, &test_reg);
        shadow_man.kick_packet.curr++;
        shadow_man.kick_packet.curr->ui32[0] = 0x70000000;
        shadow_man.kick_packet.curr->ui32[1] = 0;
        shadow_man.kick_packet.curr->ul64[1] = 0;
        shadow_man.kick_packet.curr++;
        shadow_man.change_flag = 0;
    }
    sh2shd_renew_drop_shadow_man(&spot, cam, glb_coord, map_id);
    return shadow_man.kick_packet.head;
}

/** Per-frame update of the drop shadows: each character's ground plane, light and alpha decay. */
void sh2shd_renew_drop_shadow_man(struct SPOT_LIGHT *spot, struct sh2gfw_CAMERA *cam, int glb_coord, int map_id) {
    int i;
    float light_pos[4] = { 1000.0f, -10000.0f, 1000.0f, 1.0f };
    float plane[4];
    float chara_pos[4];
    float normal[4];
    float cam_pos[4];
    float height;
    float distance;
    float alpha_decay;
    int demo_no;
    float jms_light_pos[4];
    float jms_plane[4];
    float jms_drop_shadow_matrix[4][4];
    float jms_alpha_decay;
    float mat[4][4];

    demo_no = DramaDemoNumber();
    drop_shadow_set_micro_params(&drop_shadow_micro_frame, cam);
    if (shadow_man.char_man_num > 0) {
        for (i = 0; i < 16; i++) {
            if (shadow_man.char_man[i] != NULL && (shadow_man.char_man[i]->kind < 0x800 || shadow_man.char_man[i]->kind > 0x808)) {
                shCharacterGetGroundInfoForShadow(chara_pos, normal, &height, shadow_man.char_man[i]->kind, shadow_man.char_man[i]->id);
                chara_pos[1] = height - 10.0f;
                if (demo_no == 3 || demo_no == 4) {
                    if (shadow_man.char_man[i]->kind == 0x107) {
                        chara_pos[1] = 2460.0f;
                    } else if (shadow_man.char_man[i]->kind == 0x103) {
                        chara_pos[1] = 2450.0f;
                    }
                    light_pos[2] = light_pos[0] = 2000.0f;
                } else if (demo_no == 0x15 && shadow_man.char_man[i]->kind == 0x106) {
                    chara_pos[1] = 2990.0f;
                } else if (glb_coord == 1 && (map_id == 10 || map_id == 11 || map_id == 12 || map_id == 13 || map_id == 14)) {
                    if (shadow_man.char_man[i]->kind == 0x107) {
                        shCharacterGetPartsMatrixForShadow(mat, 0x107, 0, 0x24);
                        chara_pos[1] = 40.0f + mat[3][1];
                    }
                }
                sh2shd_make_drop_shadow_plane(plane, chara_pos, normal);
                vwGetViewPosition(cam_pos);
                _shSubVector(normal, cam_pos, chara_pos);
                distance = normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2];
                distance = sqrtf(distance);
                if (distance > 10000.0f) {
                    jms_alpha_decay = 0.1f - (distance - 5000.0f) / 50000.0f;
                    if (jms_alpha_decay < 0.0f) {
                        jms_alpha_decay = 0.0f;
                    }
                } else if (distance > 4000.0f) {
                    jms_alpha_decay = 1.0f - (distance - 4000.0f) / 1000.0f;
                    if (jms_alpha_decay < 0.1f) {
                        jms_alpha_decay = 0.1f;
                    }
                } else {
                    jms_alpha_decay = 1.0f;
                }
                if (jms_alpha_decay < 0.3f) {
                    jms_alpha_decay = 0.3f;
                }
                sceVu0DropShadowMatrix(shadow_man.char_man[i]->drop_shadow_matrix, light_pos, plane[0], plane[1], plane[2], 0);
                sh2shd_renew_char_man_for_drop(shadow_man.char_man[i], (float *)spot, jms_alpha_decay);
                if (shadow_man.char_man[i]->kind >= 0x100 && shadow_man.char_man[i]->kind < 0x104) {
                    mcopy_gcc(jms_drop_shadow_matrix, shadow_man.char_man[i]->drop_shadow_matrix);
                    vcopy_gcc(jms_light_pos, light_pos);
                    vcopy_gcc(jms_plane, plane);
                }
            }
        }
        for (i = 0; i < 16; i++) {
            if (shadow_man.char_man[i] != NULL && shadow_man.char_man[i]->kind >= 0x800 && shadow_man.char_man[i]->kind < 0x809) {
                mcopy_gcc(shadow_man.char_man[i]->drop_shadow_matrix, jms_drop_shadow_matrix);
                sh2shd_renew_char_man_for_drop(shadow_man.char_man[i], (float *)spot, jms_alpha_decay);
            }
        }
    }
}

/** Computes the plane through pos with normal normal, as normal / (normal . pos) (for the stencil drop shadow). */
void sh2shd_make_stencil_drop_shadow_plane(float *plane, float *pos, float *normal) {
    float d;
    float norm[4];

    _shNormalize(norm, normal);
    if (pos[0] == 0.0f && pos[1] == 0.0f && pos[2] == 0.0f) {
        pos[1] = -10.0f;
    }
    if (norm[0] == 0.0f && norm[1] == 0.0f && norm[2] == 0.0f) {
        norm[1] = -1.0f;
    }
    d = norm[0] * pos[0] + norm[1] * pos[1] + norm[2] * pos[2];
    if (d != 0.0) {
        plane[0] = norm[0] / d;
        plane[1] = norm[1] / d;
        plane[2] = norm[2] / d;
    } else {
        plane[0] = 0.0f;
        plane[1] = -1.0f;
        plane[2] = 0.0f;
    }
}

/** Same computation as sh2shd_make_stencil_drop_shadow_plane, for the drop shadow. */
void sh2shd_make_drop_shadow_plane(float *plane, float *pos, float *normal) {
    float d;
    float norm[4];

    _shNormalize(norm, normal);
    if (pos[0] == 0.0f && pos[1] == 0.0f && pos[2] == 0.0f) {
        pos[1] = -10.0f;
    }
    if (norm[0] == 0.0f && norm[1] == 0.0f && norm[2] == 0.0f) {
        norm[1] = -1.0f;
    }
    d = norm[0] * pos[0] + norm[1] * pos[1] + norm[2] * pos[2];
    if (d != 0.0) {
        plane[0] = norm[0] / d;
        plane[1] = norm[1] / d;
        plane[2] = norm[2] / d;
    } else {
        plane[0] = 0.0f;
        plane[1] = -1.0f;
        plane[2] = 0.0f;
    }
}

/** Empties the switched-off shadow lists. */
void sh2shd_reset_shadow_off_work(void) {
    int i;

    for (i = 0; i < 2; i++) {
        shadow_off_work_char[i].kind = 0;
        shadow_off_work_char[i].id = -1;
        shadow_off_work_char[i].obj_id[0] = -1;
    }
    for (i = 0; i < 4; i++) {
        shadow_off_work_bg[i].map_id = -1;
        shadow_off_work_bg[i].obj_id[0] = -1;
    }
}

/** Adds background map map_id to the switched-off list (see sh2shd_off_obj). Returns 1. */
int sh2shd_add_map_to_shadow_off_work(short map_id) {
    int i;

    for (i = 0; i < 4; i++) {
        if (map_id == shadow_off_work_bg[i].map_id) {
            return 1;
        }
        if (shadow_off_work_bg[i].map_id == -1) {
            break;
        }
    }
    if (i > 4) {
        return 0;
    }
    shadow_off_work_bg[i].map_id = map_id;
    shadow_off_work_bg[i].obj_id[0] = -1;
    return 1;
}

/** Switches off the shadow of object obj_id of a background map added with
 * sh2shd_add_map_to_shadow_off_work. Returns 1. */
/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 2334
int sh2shd_off_obj(short map_id, short obj_id) {
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        if (map_id == shadow_off_work_bg[i].map_id) {
            break;
        }
    }
    assert_dw(i < 4);
    for (j = 0; j < 32; j++) {
        if (obj_id == shadow_off_work_bg[i].obj_id[j]) {
            return 1;
        }
        if (shadow_off_work_bg[i].obj_id[j] == -1) {
            break;
        }
    }

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 2357
    assert_dw(j < OFF_OBJ_MAX_NUM_BG);
    shadow_off_work_bg[i].obj_id[j] = obj_id;
    if (j < 31) {
        shadow_off_work_bg[i].obj_id[j + 1] = -1;
    }
    return 1;
}

/** Adds a character to the switched-off list (two slots). Returns 1, or 0 if the list is full. */
int sh2shd_add_char_to_shadow_off_work(unsigned short kind, short id) {
    int i;

    for (i = 0; i < 2; i++) {
        if (shadow_off_work_char[i].kind == 0 || shadow_off_work_char[i].id == -1) {
            break;
        }
    }
    if (i > 1) {
        return 0;
    }
    shadow_off_work_char[i].kind = kind;
    shadow_off_work_char[i].id = id;
    shadow_off_work_char[i].obj_id[0] = -1;
    return 1;
}

/** Switches off the shadow of part obj_id of a listed character. Returns 1, or 0 if it is not
 * listed or its list is full. */
int sh2shd_off_char_obj(unsigned short kind, short id, short obj_id) {
    int i;
    int j;

    for (i = 0; i < 2; i++) {
        if (kind == shadow_off_work_char[i].kind && id == shadow_off_work_char[i].id) {
            break;
        }
    }
    if (i >= 2) {
        return 0;
    }
    for (j = 0; j < 22; j++) {
        if (shadow_off_work_char[i].obj_id[j] == -1) {
            break;
        }
    }
    if (j >= 22) {
        return 0;
    }
    shadow_off_work_char[i].obj_id[j] = obj_id;
    if (j < 21) {
        shadow_off_work_char[i].obj_id[j + 1] = -1;
    }
    return 1;
}

/** Switches off the whole shadow of a listed character. Returns 1, or 0 if it is not listed. */
int sh2shd_off_char_all_parts(unsigned short kind, short id) {
    int i;

    for (i = 0; i < 2; i++) {
        if (kind == shadow_off_work_char[i].kind && id == shadow_off_work_char[i].id) {
            break;
        }
    }
    if (i >= 2) {
        return 0;
    }
    shadow_off_work_char[i].obj_id[0] = 999;
    shadow_off_work_char[i].obj_id[1] = -1;
    return 1;
}

/** Switches the background shadows off. */
void sh2shd_bg_shadow_off(void) {
    shadow_man.bg_shadow_switch = 0;
}

/*
 * check_self_spot and check_self_para are `inline`: the original's symbols have ELF binding 13,
 * which MWCC gives the out-of-line copy of a non-static inline function (docs/headers.md,
 * section 1). Every call is before the definition, so none is inlined.
 */

/** Returns whether shadow light kind kind (6-8) is a self-shadow spot light. */
inline int check_self_spot(unsigned short kind) {
    if (kind >= 6 && kind < 9) {
        return 1;
    }
    return 0;
}

/** Returns whether shadow light kind kind (1-3) is a self-shadow parallel light. */
inline int check_self_para(unsigned short kind) {
    if (kind > 0 && kind < 4) {
        return 1;
    }
    return 0;
}

/** Switches off the shadows of James's upper-body parts. */
void sh2shd_del_jms_upper_body(unsigned short kind, short id) {
    sh2shd_add_char_to_shadow_off_work(kind, id);
    sh2shd_off_char_obj(kind, id, 1);
    sh2shd_off_char_obj(kind, id, 3);
    sh2shd_off_char_obj(kind, id, 0x1C);
    sh2shd_off_char_obj(kind, id, 0x1B);
    sh2shd_off_char_obj(kind, id, 0x22);
    sh2shd_off_char_obj(kind, id, 0x21);
    sh2shd_off_char_obj(kind, id, 0x23);
    sh2shd_off_char_obj(kind, id, 0x28);
    sh2shd_off_char_obj(kind, id, 8);
    sh2shd_off_char_obj(kind, id, 0xF);
}

/** Switches off the shadows of James's head parts. */
void sh2shd_del_jms_head(unsigned short kind, short id) {
    sh2shd_add_char_to_shadow_off_work(kind, id);
    sh2shd_off_char_obj(kind, id, 8);
    sh2shd_off_char_obj(kind, id, 0xF);
}

