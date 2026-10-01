/*
 * Shadow environment tables (sh2shd): per-stage and per-demo shadow settings (clip kind and
 * shadow length for background and characters, James's self-shadow, drop-shadow alpha), and
 * the per-room and per-demo lists of characters and objects whose shadows are switched off.
 */
#include "sh2.h"

static float get_demo_frame(void);

struct DROP_SHADOW_ENV default_drop_env = { 1, 0.0f, 32.0f, 0.0f, 8 };
struct SHADOW_ENV default_env_bg = { 1, 0, 6000.0f };
struct SHADOW_ENV default_env_chr = { 1, 0, 4000.0f };
struct SHADOW_ENV cb_env[2] = { { 0x1E, 888, 8000.0f }, { 0x25, 888, 6000.0f } };
struct SHADOW_ENV cc_env[3] = { { 4, 0, 8000.0f }, { 0x16, 0, 8000.0f }, { 0x47, 999, 6000.0f } };
struct SHADOW_ENV bw_env[3] = { { 1, 0, 3000.0f }, { 5, 0, 6000.0f }, { 9, 0, 6000.0f } };
struct SHADOW_ENV ap_env[3] = { { 1, 0, 6000.0f }, { 0x1E, 0, 3000.0f }, { 0x68, 999, 6000.0f } };
struct SHADOW_ENV hp_env[23] = {
    { 0x05, 999, 6000.0f },
    { 0x09, 999, 6000.0f },
    { 0x0D, 999, 6000.0f },
    { 0x11, 999, 6000.0f },
    { 0x1D, 999, 6000.0f },
    { 0x31, 999, 6000.0f },
    { 0x35, 999, 6000.0f },
    { 0x3D, 999, 6000.0f },
    { 0x41, 999, 6000.0f },
    { 0x69, 999, 6000.0f },
    { 0x71, 0, 8000.0f },
    { 0x79, 999, 6000.0f },
    { 0x7D, 999, 6000.0f },
    { 0x91, 999, 6000.0f },
    { 0x96, 0, 8000.0f },
    { 0x9A, 999, 6000.0f },
    { 0xB2, 0, 8000.0f },
    { 0xBE, 0, 6000.0f },
    { 0xCA, 0, 8000.0f },
    { 0xCE, 0, 8000.0f },
    { 0xD2, 999, 6000.0f },
    { 0xD6, 0, 8000.0f },
    { 0xDE, 999, 6000.0f },
};
struct SHADOW_ENV ps_env[6] = {
    { 0x21, 999, 6000.0f },
    { 0x33, 999, 6000.0f },
    { 0x5D, 0, 8000.0f },
    { 0x71, 999, 6000.0f },
    { 0x91, 0, 2500.0f },
    { 0xBD, 999, 6000.0f },
};
struct SHADOW_ENV rr_env[10] = {
    { 0x05, 999, 6000.0f },
    { 0x09, 999, 6000.0f },
    { 0x15, 666, 6000.0f },
    { 0x19, 999, 8000.0f },
    { 0x1D, 0, 8000.0f },
    { 0x25, 999, 6000.0f },
    { 0x33, 999, 6000.0f },
    { 0x43, 0, 6000.0f },
    { 0x47, 999, 6000.0f },
    { 0x57, 999, 6000.0f },
};
struct SHADOW_ENV ru_env[1] = { { 0x3D, 999, 6000.0f } };

/** Gets the shadow settings of a background map (by stage kind and map id), or the default. */
void sh2shd_get_shadow_env_bg(struct SHADOW_ENV *env, struct SHADOW_OUTDOOR_HEAD *head) {
    int i;
    int flag;
    float cam_pos[4];

    *env = default_env_bg;
    switch (head->kind) {
    case 0:
    case 1:
        break;
    case 2:
        for (i = 0; i < ARRAY_COUNT(cb_env); i++) {
            if (head->map_id == cb_env[i].block) {
                *env = cb_env[i];
                break;
            }
        }
        break;
    case 3:
        for (i = 0; i < ARRAY_COUNT(cc_env); i++) {
            if (head->map_id == cc_env[i].block) {
                *env = cc_env[i];
                break;
            }
        }
        break;
    case 4:
    case 5:
    case 6:
        break;
    case 7:
        for (i = 0; i < ARRAY_COUNT(bw_env); i++) {
            if (head->map_id == bw_env[i].block) {
                *env = bw_env[i];
                break;
            }
        }
        break;
    case 8:
        if (head->map_id == 1) {
            env->block = 1;
            env->clip_kind = 999;
            env->leng = 6000.0f;
        } else if (head->map_id == 5) {
            env->block = 5;
            env->clip_kind = 0;
            env->leng = 6000.0f;
        }
        break;
    case 9:
        for (i = 0; i < ARRAY_COUNT(ap_env); i++) {
            if (head->map_id == ap_env[i].block) {
                *env = ap_env[i];
                break;
            }
        }
        switch (head->map_id) {
        case 6:
            env->clip_kind = 666;
            break;
        case 0x2A:
            flag = EventProgressCheck();
            if (flag < 5) {
                if (!sh2gfw_Check_JmsSpotOnOff()) {
                    env->clip_kind = 666;
                }
            }
            break;
        }
        break;
    case 10:
        for (i = 0; i < ARRAY_COUNT(hp_env); i++) {
            if (head->map_id == hp_env[i].block) {
                *env = hp_env[i];
                break;
            }
        }
        if (head->map_id == 0x96) {
            if (sh2jms.player->pos.z > -222800.0f) {
                env->clip_kind = 999;
            }
        }
        break;
    case 11:
        for (i = 0; i < ARRAY_COUNT(ps_env); i++) {
            if (head->map_id == ps_env[i].block) {
                *env = ps_env[i];
                break;
            }
        }
        if (head->map_id == 0x91) {
            vwGetViewPosition(cam_pos);
            if (cam_pos[1] < -2000.0f) {
                env->clip_kind = 666;
            }
        }
        break;
    case 12:
        for (i = 0; i < ARRAY_COUNT(rr_env); i++) {
            if (head->map_id == rr_env[i].block) {
                *env = rr_env[i];
                break;
            }
        }
        break;
    case 13:
        for (i = 0; i < ARRAY_COUNT(ru_env); i++) {
            if (head->map_id == ru_env[i].block) {
                *env = ru_env[i];
                break;
            }
        }
        break;
    }
}

/**
 * Gets the shadow settings of a character in a map.
 * @param glb_coord stage number
 * @param map_id    map number within the stage
 */
void sh2shd_get_shadow_env_chr(struct SHADOW_ENV *env, struct SHADOW_CHAR_MAN *chr_man, int glb_coord, int map_id) {
    env->leng = 4000.0f;
    switch (glb_coord) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        break;
    case 9:
        switch (map_id) {
        case 0x16:
            env->clip_kind = 999;
            env->leng = 4000.0f;
            break;
        case 0x1E:
            env->leng = 2000.0f;
            break;
        case 0x2A:
            if (!sh2gfw_Check_JmsSpotOnOff() && chr_man->kind == 0x50E) {
                env->clip_kind = 666;
            }
            break;
        case 0x32:
            if (!sh2gfw_Check_JmsSpotOnOff() && chr_man->scp->pos.z > 10550.0f &&
                ((chr_man->scp->pos.x > -105000.0f && chr_man->scp->pos.x < -103100.0f) ||
                 (chr_man->scp->pos.x > -102600.0f && chr_man->scp->pos.x < -101700.0f))) {
                env->clip_kind = 666;
            }
            break;
        case 0x58:
            env->leng = 3000.0f;
            env->clip_kind = 999;
            break;
        }
        break;
    case 10:
        break;
    case 11:
        switch (map_id) {
        case 0xF:
            env->clip_kind = 999;
            break;
        case 0x6D:
            env->leng = 6000.0f;
            break;
        case 0x91:
            if (chr_man->kind >= 0x100 && chr_man->kind < 0x104) {
                env->clip_kind = 999;
            }
            break;
        }
        break;
    case 12:
    case 13:
        break;
    }
}

/** Gets the background shadow settings for demo demo_no. */
void sh2shd_get_demo_shadow_env_for_bg(struct SHADOW_ENV *env, int demo_no) {
    *env = default_env_bg;
    switch (demo_no) {
    case 0:
        break;
    case 9:
        env->clip_kind = 999;
        env->leng = 8000.0f;
        break;
    case 0xB:
        env->clip_kind = 999;
        break;
    case 0xD:
        break;
    case 0xE:
        env->clip_kind = 666;
        break;
    case 0xF:
        break;
    case 0x13:
        break;
    case 0x19:
        env->clip_kind = 999;
        break;
    case 0x20:
        break;
    case 0x21:
        break;
    case 0x22:
        break;
    case 0x26:
        env->leng = 2500.0f;
        env->clip_kind = 999;
        break;
    case 0x27:
        break;
    case 0x28:
        env->clip_kind = 999;
        break;
    case 0x2B:
        break;
    case 0x2C:
        break;
    case 0x2E: {
        float env_demo_frame = get_demo_frame();

        if (env_demo_frame >= 563.0f && env_demo_frame <= 576.0f) {
            env->clip_kind = 0;
        } else {
            env->clip_kind = 999;
        }
        break;
    }
    case 0x41:
        break;
    case 0x42: {
        float env_demo_frame = get_demo_frame();

        if (env_demo_frame >= 136.0f && env_demo_frame <= 192.0f) {
            env->clip_kind = 666;
        }
        break;
    }
    case 0x46:
        env->clip_kind = 999;
        break;
    case 0x4B:
        break;
    case 0x50:
        break;
    }
}

/** Gets a character's shadow settings for demo demo_no. */
void sh2shd_get_demo_shadow_env_for_char(struct SHADOW_ENV *env, struct SHADOW_CHAR_MAN *man, int demo_no) {
    *env = default_env_chr;
    switch (demo_no) {
    case 0:
        break;
    case 9:
        env->clip_kind = 999;
        break;
    case 0xD:
        env->clip_kind = 999;
        break;
    case 0xE: {
        float env_demo_frame = get_demo_frame();

        if (env_demo_frame > 394.0f) {
            if (man->kind == 0x208) {
                env->clip_kind = 666;
            }
        }
        env->clip_kind = 999;
        env->leng = 6000.0f;
        break;
    }
    case 0x4B:
        break;
    case 0xF:
        break;
    case 0x13:
        env->leng = 4000.0f;
        break;
    case 0x19:
        env->clip_kind = 999;
        break;
    case 0x1C:
        break;
    case 0x20:
        env->clip_kind = 999;
        break;
    case 0x21:
        env->clip_kind = 999;
        break;
    case 0x22:
        break;
    case 0x26: {
        float cam_pos[4];
        float env_demo_frame;

        vwGetViewPosition(cam_pos);
        env_demo_frame = get_demo_frame();
        if (cam_pos[0] > -141000.0f && cam_pos[2] < -101000.0f) {
            env->clip_kind = 666;
        } else {
            env->clip_kind = 999;
        }
        env->leng = 2000.0f;
        if (env_demo_frame > 2860.0f) {
            env->clip_kind = 666;
        }
        break;
    }
    case 0x27:
        break;
    case 0x28: {
        float env_demo_frame = get_demo_frame();

        if (env_demo_frame > 784.0f && env_demo_frame < 875.0f) {
            env->clip_kind = 666;
        } else {
            env->clip_kind = 999;
        }
        break;
    }
    case 0x2B:
        break;
    case 0x2C:
        break;
    case 0x2E:
        env->clip_kind = 999;
        env->leng = 900.0f;
        break;
    case 0x3E:
        env->clip_kind = 999;
        break;
    case 0x41:
        break;
    case 0x42: {
        float env_demo_frame = get_demo_frame();

        if (man->kind == 0x107) {
            if (env_demo_frame >= 136.0f && env_demo_frame <= 191.0f) {
                env->clip_kind = 666;
            } else if (env_demo_frame >= 191.0f && env_demo_frame < 951.0f) {
                env->clip_kind = 999;
            } else if (env_demo_frame >= 951.0f) {
                env->clip_kind = 999;
            }
        }
        if (man->kind == 0x204) {
            if (env_demo_frame >= 130.0f && env_demo_frame <= 200.0f) {
                env->clip_kind = 666;
            }
            if (env_demo_frame >= 470.0f && env_demo_frame <= 490.0f) {
                env->clip_kind = 666;
            }
        }
        break;
    }
    case 0x46:
        env->clip_kind = 999;
        break;
    case 0x47:
        break;
    case 0x4E:
        break;
    case 0x4F:
        env->clip_kind = 999;
        break;
    case 0x50:
        env->clip_kind = 999;
        break;
    }
}

/**
 * Gets James's self-shadow settings for the current map (spot or parallel light defaults, with
 * per-map exceptions; may switch parts of his shadow off).
 * @param light_kind the shadow light kind
 * @param light_dir  the light direction
 */
void sh2shd_get_jms_shadow_env(struct JMS_SHADOW_ENV *env, int light_kind, float *light_dir) {
    int glb_coord;
    int map_id;
    float cam_pos[4];
    struct JMS_SHADOW_ENV default_env_self_spot = { -1, 600.0f, 900.0f, 10.0f };
    struct JMS_SHADOW_ENV default_env_self_para = { -1, 200.0f, 1500.0f, 1.0f };
    int flag;

    get_map_id(&glb_coord, &map_id);
    *env = default_env_self_para;
    switch (glb_coord) {
    case 9:
        switch (map_id) {
        case 0x22:
            if (light_kind == 1) {
                sh2shd_del_jms_head(0x101, 0);
            }
            break;
        case 0x2A:
            flag = EventProgressCheck();
            if (flag < 5) {
                if (!sh2gfw_Check_JmsSpotOnOff()) {
                    env->light_kind = 9;
                }
            }
            break;
        case 0x48:
            vwGetViewPosition(cam_pos);
            env->light_kind = 4;
            if (cam_pos[0] > 17500.0f) {
                sh2shd_add_char_to_shadow_off_work(0x101, 0);
                sh2shd_off_char_obj(0x101, 0, 0xF);
                sh2shd_off_char_obj(0x101, 0, 8);
                light_dir[0] = 0.0f;
                light_dir[1] = 0.707f;
                light_dir[2] = 0.0f;
                light_dir[3] = 0.0f;
            } else {
                light_dir[0] = 0.2f;
                light_dir[1] = 0.707f;
                light_dir[2] = -0.707f;
                light_dir[3] = 0.0f;
                sh2shd_bg_shadow_off();
            }
            break;
        }
        break;
    case 10:
        switch (map_id) {
        case 0x45:
            env->light_kind = 9;
            break;
        }
        break;
    case 11:
        switch (map_id) {
        case 1:
            vwGetViewPosition(cam_pos);
            if (cam_pos[2] > 20000.0f) {
                env->light_kind = 5;
            }
            break;
        case 0xF:
            if (sh2gfw_Check_JmsSpotOnOff()) {
                env->light_kind = 6;
            }
            break;
        case 0x33:
            if (sh2gfw_Check_JmsSpotOnOff()) {
                env->light_kind = 6;
            }
            break;
        }
        break;
    case 12:
        switch (map_id) {
        case 1:
            env->light_kind = 2;
            break;
        case 5:
            if (sh2gfw_Check_JmsSpotOnOff()) {
                env->light_kind = 2;
            }
            break;
        case 9:
            env->light_kind = 9;
            break;
        case 0x1D:
            if (sh2jms.player->pos.x < -58800.0f && sh2jms.player->pos.x > -61370.0f) {
                if (sh2jms.player->pos.z < 20980.0f && sh2jms.player->pos.z > 18430.0f) {
                    env->light_kind = 5;
                }
            }
            break;
        case 0x21:
            if (light_kind == 2) {
                env->light_kind = 3;
            }
            break;
        case 0x5B:
            env->light_kind = 4;
            break;
        }
        break;
    }
}

/** Gets James's self-shadow settings for demo demo_no. */
void sh2shd_get_demo_jms_shadow_env(struct JMS_SHADOW_ENV *env, int demo_no) {
    static float height_test2 = 1300.0f;
    static float scale_test2 = 10.0f;
    static float bias_test2 = 300.0f;

    env->height_revision = 900.0f;
    env->bias = 600.0f;
    env->scale = 10.0f;
    env->light_kind = -1;
    switch (demo_no) {
    case 9:
        break;
    case 0xA:
        env->light_kind = 6;
        break;
    case 0xB:
        env->light_kind = 2;
        break;
    case 0xE:
        env->light_kind = 3;
        env->height_revision = 1800.0f;
        env->bias = 500.0f;
        break;
    case 0x10:
        env->light_kind = 9;
        break;
    case 0x13:
        env->light_kind = 8;
        break;
    case 0x21:
        env->light_kind = 6;
        break;
    case 0x26: {
        float env_demo_frame = get_demo_frame();

        if (env_demo_frame >= 1000.0f && env_demo_frame < 1360.0f) {
            env->light_kind = 3;
        } else if (env_demo_frame >= 1510.0f && env_demo_frame < 2240.0f) {
            env->light_kind = 2;
        } else if (env_demo_frame > 2860.0f) {
            env->light_kind = 9;
        } else {
            env->light_kind = -1;
        }
        break;
    }
    case 0x28: {
        float env_demo_frame = get_demo_frame();

        if (env_demo_frame >= 0.0f && env_demo_frame <= 244.0f) {
            env->light_kind = 3;
        } else if (env_demo_frame > 244.0f && env_demo_frame <= 1100.0f) {
            env->light_kind = 2;
        } else if (env_demo_frame > 1100.0f) {
            env->light_kind = 3;
        }
        break;
    }
    case 0x2E:
        env->light_kind = 8;
        break;
    case 0x33:
        env->light_kind = 8;
        env->height_revision = 3000.0f;
        break;
    case 0x34:
        env->light_kind = 6;
        break;
    case 0x41:
        break;
    case 0x42: {
        float env_demo_frame = get_demo_frame();

        env->light_kind = -1;
        if (env_demo_frame >= 136.0f && env_demo_frame <= 192.0f) {
            env->light_kind = 9;
        } else if (env_demo_frame >= 517.0f && env_demo_frame <= 593.0f) {
            env->light_kind = 9;
        } else if (env_demo_frame >= 979.0f && env_demo_frame <= 1020.0f) {
            env->light_kind = 9;
        } else if (env_demo_frame >= 2290.0f && env_demo_frame <= 2550.0f) {
            env->light_kind = 6;
            env->bias = bias_test2;
            env->height_revision = height_test2;
            env->scale = scale_test2;
        }
        break;
    }
    case 0x45: {
        /* Matching: the original's DWARF has a fourth env_demo_frame; its value is unused. */
        float env_demo_frame = get_demo_frame();

        break;
    }
    case 0x4A: {
        float mat[4][4];

        demo_frame = get_demo_frame();
        shCharacterGetPartsMatrixForShadow(mat, 0x104, 0, 0);
        if (mat[3][2] < 59000.0f) {
            env->light_kind = 9;
        }
        break;
    }
    case 0x4D:
        env->light_kind = 4;
        break;
    case 0x4E:
        env->light_kind = 4;
        break;
    default:
        env->light_kind = -1;
        break;
    }
}

/**
 * Gets the drop-shadow settings for a map (and, in one map, for the position x/z).
 */
void sh2shd_get_drop_shadow_env(struct DROP_SHADOW_ENV *env, int glb_coord, int map_id, float x, float z) {
    *env = default_drop_env;
    switch (glb_coord) {
    case 1:
        if (map_id == 10 || map_id == 11 || map_id == 13 || map_id == 14) {
            env->alpha_max = 64.0f;
            env->color = 0;
        }
        break;
    case 3:
        switch (map_id) {
        case 0x33:
            if (x < -99100.0f) {
                env->alpha_max = 128.0f;
            } else if (x < -98700.0f && z < 39550.0f) {
                env->alpha_max = 128.0f;
            }
            break;
        }
        break;
    case 5:
        env->alpha_max = 128.0f;
        break;
    }
}

/** Gets the drop-shadow settings for demo demo_no. */
void sh2shd_get_demo_drop_shadow_env(struct DROP_SHADOW_ENV *env, int demo_no) {
    *env = default_drop_env;
    switch (demo_no) {
    case 0:
        return;
    case 3:
    case 4:
        env->alpha_max = 128.0f;
        env->color = 0;
        break;
    case 0x1C:
    case 0x1D:
    case 0x1E:
        env->alpha_max = env->alpha_min = 32.0f;
        env->color = 0;
        break;
    }
}

/** Switches off the shadows of the characters and objects listed for demo demo_no (some by demo frame). */
void sh2shd_demo_shadow_off(int demo_no) {
    static float tmp_time = 2252.0f;
    float demo_frame;
    float test_time;
    float env_demo_frame;

    switch (demo_no) {
    case 9:
        break;
    case 0xD:
        sh2shd_add_char_to_shadow_off_work(0x103, 0);
        sh2shd_off_char_all_parts(0x103, 0);
        break;
    case 0xE:
        demo_frame = get_demo_frame();
        sh2shd_add_char_to_shadow_off_work(0x103, 0);
        if (demo_frame <= 1699.0f) {
            sh2shd_off_char_all_parts(0x103, 0);
        } else {
            sh2shd_off_char_obj(0x103, 0, 0xF);
            sh2shd_off_char_obj(0x103, 0, 8);
            sh2shd_off_char_obj(0x103, 0, 1);
            sh2shd_off_char_obj(0x103, 0, 3);
            sh2shd_off_char_obj(0x103, 0, 0x1A);
            sh2shd_off_char_obj(0x103, 0, 0x19);
            sh2shd_off_char_obj(0x103, 0, 0xE);
            sh2shd_off_char_obj(0x103, 0, 0xD);
            sh2shd_off_char_obj(0x103, 0, 5);
            sh2shd_off_char_obj(0x103, 0, 4);
        }
        sh2shd_add_char_to_shadow_off_work(0x208, 0);
        if (get_demo_frame() < 190.0f) {
            sh2shd_off_char_obj(0x208, 0, 0x24);
            sh2shd_off_char_obj(0x208, 0, 0x1D);
        } else if (demo_frame >= 360.0f && demo_frame <= 1699.0f) {
            sh2shd_add_char_to_shadow_off_work(0x208, 0);
            sh2shd_off_char_all_parts(0x208, 0);
        }
        break;
    case 0x21:
        test_time = get_demo_frame();
        if (test_time < 902.0f) {
            sh2shd_add_char_to_shadow_off_work(0x103, 0);
            sh2shd_off_char_all_parts(0x103, 0);
        }
        break;
    case 0x26:
        sh2shd_add_char_to_shadow_off_work(0x103, 0);
        sh2shd_off_char_obj(0x103, 0, 0x1A);
        break;
    case 0x2E:
        env_demo_frame = get_demo_frame();
        if (env_demo_frame < 780.0f) {
            sh2shd_add_char_to_shadow_off_work(0x103, 0);
            sh2shd_off_char_obj(0x103, 0, 0x21);
            sh2shd_off_char_obj(0x103, 0, 0x28);
            sh2shd_off_char_obj(0x103, 0, 3);
            sh2shd_off_char_obj(0x103, 0, 1);
            sh2shd_off_char_obj(0x103, 0, 0x1C);
            sh2shd_off_char_obj(0x103, 0, 0x22);
            sh2shd_off_char_obj(0x103, 0, 0x23);
            sh2shd_off_char_obj(0x103, 0, 0x1B);
            if (env_demo_frame > 510.0f) {
                sh2shd_off_char_obj(0x103, 0, 4);
                sh2shd_off_char_obj(0x103, 0, 5);
            }
        }
        break;
    case 0x41:
        break;
    case 0x42: {
        float env_demo_frame;
        float cam_pos[4];

        vwGetViewPosition(cam_pos);
        env_demo_frame = get_demo_frame();
        if (env_demo_frame >= 135.0f && env_demo_frame <= 192.0f) {
            sh2shd_add_char_to_shadow_off_work(0x204, 0);
            sh2shd_off_char_obj(0x204, 0, 0);
            sh2shd_off_char_obj(0x204, 0, 3);
            sh2shd_off_char_obj(0x204, 0, 4);
            sh2shd_off_char_obj(0x204, 0, 5);
            sh2shd_off_char_obj(0x204, 0, 7);
            sh2shd_off_char_obj(0x204, 0, 8);
            sh2shd_off_char_obj(0x204, 0, 9);
            sh2shd_off_char_obj(0x204, 0, 0x11);
            sh2shd_off_char_obj(0x204, 0, 0x12);
            sh2shd_off_char_obj(0x204, 0, 0x13);
            sh2shd_off_char_obj(0x204, 0, 0x17);
            sh2shd_off_char_obj(0x204, 0, 0x1B);
            sh2shd_off_char_obj(0x204, 0, 0x1C);
        } else if (env_demo_frame >= 518.0f && env_demo_frame <= 608.0f) {
            sh2shd_add_char_to_shadow_off_work(0x204, 0);
            sh2shd_off_char_obj(0x204, 0, 0);
            sh2shd_off_char_obj(0x204, 0, 3);
            sh2shd_off_char_obj(0x204, 0, 5);
            sh2shd_off_char_obj(0x204, 0, 7);
            sh2shd_off_char_obj(0x204, 0, 8);
            sh2shd_off_char_obj(0x204, 0, 9);
            sh2shd_off_char_obj(0x204, 0, 0x11);
            sh2shd_off_char_obj(0x204, 0, 0x12);
            sh2shd_off_char_obj(0x204, 0, 0x13);
            sh2shd_off_char_obj(0x204, 0, 0x17);
            sh2shd_off_char_obj(0x204, 0, 0x1B);
            sh2shd_off_char_obj(0x204, 0, 0x1C);
            sh2shd_add_char_to_shadow_off_work(0x107, 0);
            sh2shd_off_char_obj(0x107, 0, 0x1F);
            sh2shd_off_char_obj(0x107, 0, 6);
            sh2shd_off_char_obj(0x107, 0, 2);
            sh2shd_off_char_obj(0x107, 0, 0);
        }
        break;
    }
    case 0x4A: {
        float mat[4][4];

        env_demo_frame = get_demo_frame();
        shCharacterGetPartsMatrixForShadow(mat, 0x104, 0, 0);
        if (mat[3][2] > 59000.0f) {
            if (env_demo_frame > 320.0f && env_demo_frame <= 962.0f) {
                sh2shd_add_char_to_shadow_off_work(0x104, 0);
                sh2shd_off_char_all_parts(0x104, 0);
            }
        } else if (env_demo_frame >= 782.0f && env_demo_frame < 1420.0f) {
            sh2shd_add_char_to_shadow_off_work(0x104, 0);
            sh2shd_off_char_obj(0x104, 0, 0);
            sh2shd_off_char_obj(0x104, 0, 1);
            sh2shd_off_char_obj(0x104, 0, 0xB);
            sh2shd_off_char_obj(0x104, 0, 0xC);
            sh2shd_off_char_obj(0x104, 0, 0xF);
            sh2shd_off_char_obj(0x104, 0, 0x10);
            sh2shd_off_char_obj(0x104, 0, 3);
            sh2shd_off_char_obj(0x104, 0, 7);
            sh2shd_off_char_obj(0x104, 0, 0x12);
            sh2shd_off_char_obj(0x104, 0, 0x16);
            sh2shd_off_char_obj(0x104, 0, 0x1F);
            sh2shd_off_char_obj(0x104, 0, 0x25);
        }
        break;
    }
    case 0x4E:
        demo_frame = get_demo_frame();
        if (demo_frame > 1750.0f) {
            sh2shd_add_char_to_shadow_off_work(0x103, 0);
            if (demo_frame < tmp_time) {
                sh2shd_off_char_all_parts(0x103, 0);
            } else if (demo_frame < 3220.0f) {
                sh2shd_off_char_obj(0x103, 0, 0x1A);
                sh2shd_off_char_obj(0x103, 0, 0x19);
                sh2shd_off_char_obj(0x103, 0, 0xE);
                sh2shd_off_char_obj(0x103, 0, 0xD);
                sh2shd_off_char_obj(0x103, 0, 5);
                sh2shd_off_char_obj(0x103, 0, 4);
                sh2shd_off_char_obj(0x103, 0, 1);
                sh2shd_off_char_obj(0x103, 0, 0x1B);
                sh2shd_off_char_obj(0x103, 0, 0x21);
                sh2shd_add_char_to_shadow_off_work(0x104, 0);
                sh2shd_off_char_obj(0x104, 0, 0);
                sh2shd_off_char_obj(0x104, 0, 1);
                sh2shd_off_char_obj(0x104, 0, 0xB);
                sh2shd_off_char_obj(0x104, 0, 0xC);
                sh2shd_off_char_obj(0x104, 0, 0xF);
                sh2shd_off_char_obj(0x104, 0, 0x10);
                sh2shd_off_char_obj(0x104, 0, 3);
                sh2shd_off_char_obj(0x104, 0, 7);
                sh2shd_off_char_obj(0x104, 0, 0x12);
                sh2shd_off_char_obj(0x104, 0, 0x16);
                sh2shd_off_char_obj(0x104, 0, 0x1F);
                sh2shd_off_char_obj(0x104, 0, 0x25);
            }
        }
        break;
    }
}

/** Switches off the shadows of the characters and objects listed for a map (some depending on the camera or James). */
void sh2shd_shadow_off(int glb_coord, int map_id) {
    switch (glb_coord) {
    case 9:
        if (map_id == 0x58) {
            sh2shd_add_char_to_shadow_off_work(0x200, 9);
            sh2shd_off_char_all_parts(0x200, 9);
        }
        break;
    case 10:
        if (map_id == 0x2D) {
            sh2shd_add_char_to_shadow_off_work(0x104, 0);
            sh2shd_off_char_all_parts(0x104, 0);
        } else if (map_id == 0x96) {
            if (sh2jms.player->pos.z > -222800.0f) {
                sh2shd_add_map_to_shadow_off_work(0x96);
                sh2shd_off_obj(0x96, 8);
            }
        }
        break;
    case 11:
        if (map_id == 0x71) {
            float cam_pos[4];

            vwGetViewPosition(cam_pos);
            if (cam_pos[1] > -1000.0f) {
                sh2shd_add_map_to_shadow_off_work(0x71);
                sh2shd_off_obj(0x71, 2);
                sh2shd_off_obj(0x71, 0x15);
                sh2shd_off_obj(0x71, 0x17);
                sh2shd_off_obj(0x71, 0x16);
                sh2shd_off_obj(0x71, 0x18);
            }
        }
        break;
    case 12:
        if (map_id == 9) {
            float cam_pos[4];

            vwGetViewPosition(cam_pos);
            if (cam_pos[1] > -2000.0f) {
                sh2shd_add_map_to_shadow_off_work(9);
                sh2shd_off_obj(9, 0x52);
            }
        }
        break;
    }
}

static float get_demo_frame(void) {
    return demo_frame;
}

/** Gets the stage number and map number of the block James is in. */
void get_map_id(int *glb_coord, int *map_id) {
    int block[4];

    BlockNumber(block, 0, sh2jms.player->pos.x, sh2jms.player->pos.z);
    *glb_coord = (block[0] >> 16) & 0xFFFF;
    *map_id = block[0] & 0xFFFF;
}

/** Returns the background shadow light kind of the current map: 0 and 4 for two maps, else 9. */
int check_bg_light_exist(void) {
    int glb_coord;
    int map_id;

    get_map_id(&glb_coord, &map_id);
    switch (glb_coord) {
    case 9:
        switch (map_id) {
        case 0x2A:
            return 0;
        }
        break;
    case 12:
        if (map_id == 0x5B) {
            return 4;
        }
        break;
    }
    return 9;
}
