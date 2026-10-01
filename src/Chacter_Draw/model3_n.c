/*
 * Model3 character renderer, top level (Chacter_Draw): model/work accessors, the per-frame
 * camera and fog parameters shared by all models, and Model3Draw_n, which updates a model's
 * matrices, textures, lights and cluster (morph) weights and draws its VU1 and VU0 parts.
 */
#include "sh2.h"
#include "fog_param.h"
#include "sdk/libvu0.h"

static void Exec_ModelDraw_OVFunc(void *pp);

/* Two quadwords, copied as one struct (lq/sq pairs). */
typedef struct { float m[2][4]; } M24;

/* Matching: an inline getter (name ours); reading model_common_work in place,
 * sh2_Model_MakeMatrixParams doesn't match. */
static inline struct ModelCommonWork *CurrentModelCommonWork(void) {
    return model_common_work;
}

static unsigned int UIntFogcol(void) {
    unsigned int fogcol;

    /* Matching: the DWARF has fogcol at 0xc(sp), so its address was taken; a plain assignment compiles
     * differently. */
    *(unsigned int *)&fogcol = Env_ctl.fogcolor.ui32[0];
    return fogcol;
}

/** Returns the number of skeleton nodes (bones) of a struct Model. */
int Model3NSkeletons(void *model_) {
    struct Model *model;

    model = model_;
    return model->n_skeletons;
}

/**
 * Returns the skeleton parent table of a struct Model. On the first call the stored parent
 * indices (except the 0xFE/0xFF markers) are halved in place, and the model is flagged.
 */
signed char *Model3SkeletonStructure(void *model_) {
    unsigned int mask;
    struct Model *model;
    signed char *structure;
    int i;
    unsigned char d;

    model = model_;
    structure = (signed char *)((char *)model + model->skeleton_structure_offset);
    mask = 1;
    if (!(model->flag & mask)) {
        for (i = 0; i < model->n_skeletons; i++) {
            d = structure[i];
            if (d < 0xFE) {
                d = (unsigned int)d >> 1;
            }
            structure[i] = d;
        }
        model->flag |= mask;
    }
    return structure;
}

/** Returns the number of clusters (morph targets) of a struct Model. */
int Model3NClusters(void *model_) {
    struct Model *model;

    model = model_;
    return model->n_clusters;
}

/** Per-frame setup before any model is drawn: loads the VU1 microprogram. */
void Model3DrawPre(void) {
    Model3LoadMpg1();
}

/**
 * Per-frame camera parameters for the models: copies the view matrices into model3_junk and
 * builds the view-screen, clip and fog vectors in model_common_work (both pages).
 */
/*
 * Matching: the fog terms and the model_common_work getter are inline functions (no DWARF, names not recovered):
 * written in place, ftmp moves from f3 to f0 and the end copies compute the source address before
 * loading model_common_work.
 */
void sh2_Model_MakeMatrixParams(void) {
    float (*vsm)[4];
    float nearz;
    float farz;
    float scrz;
    float ftmp;
    struct ModelCommonWork *mcw;

    vsm = model3_junk.vsm;
    nearz = Env_ctl.camera_parms[3];
    farz = Env_ctl.camera_parms2[2];
    /*
     * Matching: the DWARF has a local scrz that nothing reads (a dead store, no code); the
     * value is reconstructed: sh2gfw_shcamtest.c sets VbScreenInfo.scr_z from camera_parms[2].
     */
    scrz = Env_ctl.camera_parms[2];
    vwGetViewPosition(model3_junk.camera);
    sceVu0CopyMatrix(model3_junk.wvm, VbWvsMatrix.wvm);
    sceVu0CopyMatrix(model3_junk.vsm, cam0.view_screen);
    sceVu0CopyMatrix(model3_junk.wsm, cam0.world_screen);
    sceVu0InversMatrix(model3_junk.vwm, model3_junk.wvm);
    model3_junk.fogcol = UIntFogcol();
    ftmp = 1.0f / (farz - nearz);
    {
        float (*vsp)[4];

        vsp = model_common_work->vsp;
        vsp[0][0] = vsm[0][0];
        vsp[0][1] = vsm[1][1];
        vsp[0][2] = vsm[3][2];
        vsp[0][3] = FogParamA(Env_ctl.fogparm.fl32);
        vsp[1][0] = vsm[2][0];
        vsp[1][1] = vsm[2][1];
        vsp[1][2] = vsm[2][2];
        vsp[1][3] = FogParamB(Env_ctl.fogparm.fl32);
    }
    {
        float (*p)[4];

        p = model_common_work->vcp;
        p[0][0] = 2.0f * vsm[0][0] / 512.0f;
        p[0][1] = 2.0f * vsm[1][1] / 448.0f;
        p[0][2] = ftmp * (farz + nearz);
        p[0][3] = 1.0f;
        p[1][0] = 0.0f;
        p[1][1] = 0.0f;
        p[1][2] = ftmp * (-2.0f * (farz * nearz));
        p[1][3] = 0.0f;
    }
    {
        float (*p)[4];

        p = model_common_work->vcp_gs;
        p[0][0] = 2.0f * vsm[0][0] / 4080.0f;
        p[0][1] = 2.0f * vsm[1][1] / 4080.0f;
        p[0][2] = ftmp * (farz + nearz);
        p[0][3] = 1.0f;
        p[1][0] = 0.0f;
        p[1][1] = 0.0f;
        p[1][2] = ftmp * (-2.0f * (farz * nearz));
        p[1][3] = 0.0f;
    }
    mcw = model_common_work;
    ModelCommonWorkFlip();
    *(M24 *)CurrentModelCommonWork()->vsp = *(M24 *)mcw->vsp;
    *(M24 *)CurrentModelCommonWork()->vcp = *(M24 *)mcw->vcp;
    *(M24 *)CurrentModelCommonWork()->vcp_gs = *(M24 *)mcw->vcp_gs;
}

static void SortEnvPrim(void) {
    static struct EnvPacket envpacketdata = {
        {
            0x20000006,
            0x00000000,
            0x50000006,
            0x1000000000008005,
            0xE,
            0x50003,
            0x47,
            0x8000000080,
            0x3B,
            0,
            0x4A,
            0x44,
            0x42,
            0,
            0x3D,
        },
        {
            0x20000002,
            0x00000000,
            0x50000002,
            0x1000000000008001,
            0xE,
            0x5,
            0x8,
        },
    };
    struct EnvPacket *ep;

    ep = (struct EnvPacket *)CharacterOt_RequestPacket(sizeof(struct EnvPacket) / 16);
    *ep = envpacketdata;
    ep->head_ep.fogcol_d = model3_junk.fogcol;
    CharacterOt_Append(0, (u_long128 *)&ep->head_ep);
    CharacterOt_Append(0xFFF, (u_long128 *)&ep->tail_ep);
}

/**
 * Draws one character model.
 * @param scp_d_ the character's struct SubCharacterDisp
 * @param model_ its struct Model
 * @param work_  its struct ModelWork
 * @param mwm    model-to-world matrix
 * @param ws_mat world-to-screen matrix (not used)
 * @param flag   not used
 */
void Model3Draw_n(void *scp_d_, void *model_, void *work_, float (*mwm)[4], float (*ws_mat)[4], int flag) {
    /* Matching: called without a prototype in the original (arguments passed unconverted). */
    void Model3UpdateEnvelopeMatrices();
    void Model3UpdateLightEnv();
    int pef;
    int d1_cid;
    struct Model *model;
    struct ModelWork *mwork;
    struct sh_Model *shM;
    struct SubCharacterDisp *scp_d;
    float *weights;

    model = model_;
    mwork = work_;
    scp_d = scp_d_;
    pef = *T0_COUNT;
    ModelCommonWorkFlip();
    Model3UpdateMatrices(model, mwork, mwm, scp_d->sc.model_type);
    shM = model_;
    sh2_Model3UpdateTextures(shM);
    Model3UpdateEnvelopeMatrices(model, mwork);
    Model3UpdateLightEnv(model, mwork);
    Model3UpdateGlobalAmbient();
    if (model->n_clusters != 0) {
        weights = Model3WorkClusterWeights(mwork);
        ClusterAnimeGetWeights(scp_d->cluster_anime, weights);
    }
    Model3UpdateClusters(model, mwork);
    Exec_ModelDraw_OVFunc(scp_d);
    Model3DrawVu1Parts(model, mwork);
    sh2gfw_Init_CharacterOT();
    Model3DrawVu0Parts(model, mwork);
    if (CharacterOt_IsEmpty()) {
        sh2_Model_DummySyncOT();
        sh2gfw_set_d1cid();
    } else {
        SortEnvPrim();
        CharacterOt_ExecPre();
        CharacterOt_ExecPost();
        sh2gfw_set_d1cid();
    }
    d1_cid = *T0_COUNT;
}

/** Returns the size of the struct ModelWork a model needs, with its matrices and cluster weights. */
int Model3WorkSize(void *model_) {
    struct Model *model;
    int size;

    model = model_;
    size = sizeof(struct ModelWork);
    size += model->n_skeletons * sizeof(float[4][4]);
    size = (size + 15) & ~15;
    size += model->n_clusters * sizeof(float);
    size = (size + 15) & ~15;
    return size;
}

/**
 * Initializes a model work: lays out its matrices (from the model's initial matrices, if any)
 * and zeroed cluster weights after the header, and sets the default equipment flags.
 * @param model_ the struct Model
 * @param work_  the work, Model3WorkSize(model_) bytes
 */
void Model3InitWork(void *model_, void *work_) {
    struct Model *model;
    struct ModelWork *work;
    void *top;
    int size;
    float (*initial)[4][4];
    int i;

    model = model_;
    work = work_;
    top = work;
    work->id = 0xFFFE0003;
    size = sizeof(struct ModelWork);
    work->matrices = (float (*)[4][4])((char *)top + size);
    if (model->initial_matrices_offset != 0) {
        initial = (float (*)[4][4])((char *)model + model->initial_matrices_offset);
        for (i = 0; i < model->n_skeletons; i++) {
            sceVu0CopyMatrix(work->matrices[i], *initial++);
        }
    }
    size += model->n_skeletons * sizeof(float[4][4]);
    size = (size + 15) & ~15;
    work->cluster_weights = (float *)((char *)top + size);
    {
        int i;

        for (i = 0; i < model->n_clusters; i++) {
            work->cluster_weights[i] = 0.0f;
        }
    }
    work->equipment_flag = 0x204041;
    work->draw_hook = NULL;
    work->draw_hook_data = NULL;
}

/** Returns the skeleton matrices of a struct ModelWork. */
float (*Model3WorkMatrices(void *work_))[4][4] {
    struct ModelWork *work;

    work = work_;
    return work->matrices;
}

/** Returns the cluster weights of a struct ModelWork. */
float *Model3WorkClusterWeights(void *work_) {
    struct ModelWork *work;

    work = work_;
    return work->cluster_weights;
}

static void Exec_ModelDraw_OVFunc(void *pp) {
    struct SubCharacterDisp *scp_d;

    scp_d = pp;
    if (stage != NULL && stage->gfw_func != NULL && stage->gfw_func->CharaDraw_Hook != NULL) {
        ((void (*)(struct SubCharacterDisp *, float *))stage->gfw_func->CharaDraw_Hook)(scp_d, model3_junk.global_ambient);
    }
}
