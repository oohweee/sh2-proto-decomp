/*
 * Model3 per-model updates (Chacter_Draw): renderer start-up, texture transfer and TEX0
 * lookup, skeleton and envelope matrices, and the per-model light and ambient setup.
 */
#include "sh2.h"
#include "sdk/libvu0.h"

/** One-time setup of the renderer constants in model3_junk (clip bounds, color limit, GIF tags). */
void Model3Init(void) {
    static float xyz_min_wide[4] = { 16.0f, 16.0f, 0.0f, 0.0f };
    static float xyz_max_wide[4] = { 4080.0f, 4080.0f, 0.0f, 0.0f };
    static float xyz_min[4] = { 1024.0f, 1024.0f, 0.0f, 0.0f };
    static float xyz_max[4] = { 3072.0f, 3072.0f, 0.0f, 0.0f };
    static float rgba_max[4] = { 128.0f, 128.0f, 128.0f, 255.0f };
    static float global_ambient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    static unsigned long giftag_0[2] = { 0x1000000000008000, 0xE };
    static unsigned long giftag_1[2] = { 0x301E400000008000, 0x412 };
    static unsigned long giftag_2[2] = { 0x303E400000008000, 0x412 };
    static int initialized;

    if (!initialized) {
        xyz_min_wide[2] = 2.0f;
        xyz_max_wide[2] = Env_ctl.camera_parms2[3];
        xyz_min[2] = 2.0f;
        xyz_max[2] = Env_ctl.camera_parms2[3];
        sceVu0CopyVector(model3_junk.xyz_min_wide, xyz_min_wide);
        sceVu0CopyVector(model3_junk.xyz_max_wide, xyz_max_wide);
        sceVu0CopyVector(model3_junk.xyz_min, xyz_min);
        sceVu0CopyVector(model3_junk.xyz_max, xyz_max);
        sceVu0CopyVector(model3_junk.rgba_max, rgba_max);
        sceVu0CopyVector(model3_junk.global_ambient, global_ambient);
        sceVu0CopyVector((float *)&model3_junk.giftag_0, (float *)giftag_0);
        sceVu0CopyVector((float *)&model3_junk.giftag_1, (float *)giftag_1);
        sceVu0CopyVector((float *)&model3_junk.giftag_2, (float *)giftag_2);
        model3_junk.vi00 = (void *)0x70000000;
        initialized = 1;
    }
}

/**
 * Sends a model's texture blocks to the GS (recording the DMA ids in UniModelDW_Man) and
 * stores the TEX0 register of each texture position in model_common_work.
 */
void sh2_Model3UpdateTextures(struct sh_Model *model) {
    int n_texture_blocks;
    int *texture_blocks;
    int n_text_poses;
    struct TextPos *text_poses;
    int spemap;
    int envmap;
    int i;

    n_texture_blocks = model->n_texture_blocks;
    texture_blocks = (int *)((char *)model + model->texture_blocks_offset);
    n_text_poses = model->n_text_poses;
    text_poses = (struct TextPos *)((char *)model + model->text_poses_offset);
    spemap = model->revision < 3 || (model->flag & 0x40000000);
    /*
     * Matching: the DWARF has a local envmap, assigned here (lines 127-132 have no code) but
     * never read; the value it was given is not recoverable.
     */
    envmap = 0;
    if (spemap) {
        model_common_work->specular_mapping_tex0 = sh2_SpecularMappingTEX0();
    }
    for (i = 0; i < n_texture_blocks; i++) {
        int id;

        id = texture_blocks[i];
        sh2gfw_Thr_d2TextureSend(model->pTexMAN[id], 1, &UniModelDW_Man.chr_cid[id], &UniModelDW_Man.chr_slotid[id]);
    }
    {
        struct TextPosParam *params;
        int i;

        params = model_common_work->text_pos_params;
        for (i = 0; i < n_text_poses; i++) {
            struct TextPos *text_pos;
            struct TextPosParam *param;
            u_long128 *tmp_tex0;

            text_pos = &text_poses[i];
            param = &params[i];
            tmp_tex0 = sh2gfw_Get_RegTEX0(model->pTexMAN[text_pos->block_index], text_pos->texture_no, 1);
            param->tex0 = *tmp_tex0;
        }
    }
}

/**
 * Computes the model-to-view matrix of every skeleton node into model_common_work.
 * @param model      the struct Model
 * @param work       its work (the node matrices, relative to the model)
 * @param mwm        model-to-world matrix
 * @param model_type non-zero to mirror each node in x first
 */
void Model3UpdateMatrices(struct Model *model, struct ModelWork *work, float (*mwm)[4], int model_type) {
    int n_skeletons = model->n_skeletons;
    float (*matrices)[4][4] = work->matrices;
    /* Matching: 16-byte aligned, as the original's aligned matrix typedef made them. */
    float wvm[4][4] __attribute__((aligned(16)));
    float mvm[4][4] __attribute__((aligned(16)));
    float xsin[4][4] __attribute__((aligned(16))) = {
        { -1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
    };
    int i;

    sceVu0CopyMatrix(wvm, model3_junk.wvm);
    sceVu0MulMatrix(mvm, wvm, mwm);
    sceVu0MulMatrix(model_common_work->top_skeleton_matrix, mvm, *matrices);
    mvm[0][3] = mvm[0][2];
    mvm[1][3] = mvm[1][2];
    mvm[2][3] = mvm[2][2];
    mvm[3][3] = mvm[3][2];
    if (model_type) {
        for (i = 0; i < n_skeletons; i++) {
            float (*lvm)[4];
            float (*lmm)[4];

            lvm = model_common_work->skeleton_matrices[i];
            lmm = matrices[i];
            shMulMatrix(lmm, xsin, lmm);
            shMulMatrix(lvm, mvm, lmm);
        }
    } else {
        for (i = 0; i < n_skeletons; i++) {
            float (*lvm)[4];
            float (*lmm)[4];

            lvm = model_common_work->skeleton_matrices[i];
            lmm = matrices[i];
            shMulMatrix(lvm, mvm, lmm);
        }
    }
}

/** Computes the envelope (skinning) matrices: each skeleton pair's child matrix times its default matrix. */
void Model3UpdateEnvelopeMatrices(struct Model *model) {
    int n_pairs;
    float (*default_pcms)[4][4];
    float (*skeleton_matrices)[4][4];
    struct SkeletonPair *pairs;
    float (*envelope_matrices)[4][4];
    int i;

    n_pairs = model->n_skeleton_pairs;
    default_pcms = (float (*)[4][4])((char *)model + model->default_pcms_offset);
    skeleton_matrices = model_common_work->skeleton_matrices;
    pairs = (struct SkeletonPair *)((char *)model + model->skeleton_pairs_offset);
    envelope_matrices = model_common_work->envelope_matrices;
    for (i = 0; i < n_pairs; i++) {
        struct SkeletonPair *pair;
        float (*em)[4];
        int child_no;
        float (*pcm)[4];
        float (*cvm)[4];

        pair = &pairs[i];
        em = envelope_matrices[i];
        child_no = pair->child_no;
        pcm = default_pcms[i];
        cvm = skeleton_matrices[child_no];
        shMulMatrix(em, cvm, pcm);
    }
}

/** Picks the lights for the model being drawn, from its root position and a radius of 800. */
void Model3UpdateLightEnv(void) {
    static float base[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    static float radius = 800.0f;
    float wpos[4] __attribute__((aligned(16))); /* Matching: aligned as above. */

    sceVu0ApplyMatrix(wpos, model_common_work->top_skeleton_matrix, base);
    sceVu0ApplyMatrix(wpos, model3_junk.vwm, wpos);
    LightUpdateInfoByPos(wpos, radius);
}

/* FAKEMATCH: the (float *)& cast is there only for codegen: with amb_scale it puts the 2.0f between a0 and
 * a1, as in the original. amb_scale (Matching:) is a propagated static const, which MWCC leaves no DWARF
 * for, so it can't be verified. */
static const float amb_scale = 2.0f;

/** Sets the models' global ambient: the scene ambient times 2 times the character light factor (alpha doubled). */
void Model3UpdateGlobalAmbient(void) {
    sceVu0ScaleVector((float *)&model3_junk.global_ambient, Env_ctl.ambient, amb_scale);
    sceVu0ScaleVector(model3_junk.global_ambient, model3_junk.global_ambient, Env_ctl.CharacterLightFactor[0]);
    model3_junk.global_ambient[3] *= 2.0f;
}
