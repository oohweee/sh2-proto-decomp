/*
 * Model3 VU1 renderer (Chacter_Draw): the character parts that VU1 microcode transforms,
 * lights and sends to the GS directly. Builds one VIF1 packet per model: light and matrix data
 * for each part, the microprograms it needs (lambert, environment, specular...), split at the
 * model's texture changes so each piece waits for its texture transfer.
 */
#include "sh2.h"
#include "sdk/libdma.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

typedef unsigned int u_int;

extern u_long128 model3_mpg1_view_load[];
extern int model3_mpg1_view_size;
/* Matching: an inline function with a local (not model3_vu0_n's macro): gives MakeData1's code and qwd's
 * DWARF location (v0). */
static inline unsigned int UncachedAddr(void *p) { unsigned int a; a = (unsigned int)p & 0x0FFFFFFF; return a | 0x20000000; }
#define UNCACHED(p) UncachedAddr(p)
static int muga = 0;
static int xitop;
static int prev_xtop;
static struct AllData all_data_db[2] __attribute__((aligned(128)));
static int all_data_page;
static struct AllData *all_data;
static struct SprData spr_data_mem;
static struct SprData *spr_data = (struct SprData *)0x70000100;

/** Loads the VU1 model microprogram (the packet is built once, then resent each call). */
void Model3LoadMpg1(void) {
    static u_long128 packet_buffer[4];
    static int initialized = 0;
    union Q_WORDDATA *qwd;

    if (!initialized) {
        qwd = (union Q_WORDDATA *)UNCACHED(packet_buffer);
        qwd[0].ui32[0] = 0x50000000;
        qwd[0].ui32[1] = (unsigned int)model3_mpg1_view_load;
        qwd[0].ui32[2] = 0x11000000;
        qwd[0].ui32[3] = 0;
        printf("Model3:MPG1: %x\n", model3_mpg1_view_size);
        qwd[1].ui32[0] = 0x70000000;
        qwd[1].ui32[1] = 0;
        qwd[1].ul64[1] = 0;
        initialized = 1;
    }
    d1cSend(packet_buffer);
}

/*
 * FAKEMATCH: bdraw's envtag[0] is stored through a second UNCACHED(&p->bdraw), not data: the extra
 * temporary is numbered below 0xEEEEE's, which then gets s1. Any bdraw store works; this one is out of order.
 */
static void InitAllDataOne(struct AllData *p) {
    {
        struct LambertData *lambert_data;

        lambert_data = (struct LambertData *)UNCACHED(&p->lambert);
        sceVu0CopyVector(lambert_data->rgba_max, model3_junk.rgba_max);
    }
    {
        struct DSetupData *data;

        data = (struct DSetupData *)UNCACHED(&p->dsetup);
        sceVu0CopyVector(data->rgba_max.fv, model3_junk.rgba_max);
        data->waittag.u64[0] = 0x8000;
        data->waittag.u64[1] = 0;
    }
    {
        struct EDrawData *data;

        data = (struct EDrawData *)UNCACHED(&p->edraw);
        data->giftag.u64[0] = 0x3000000000008004;
        data->giftag.u64[1] = 0x412;
        data->waittag.u64[0] = 0x8000;
        data->waittag.u64[1] = 0;
        data->envtag.u64[0] = 0x503A400000008001;
        data->envtag.u64[1] = 0xEEEEE;
        data->tex0.u64[0] = 0;
        data->tex0.u64[1] = 6;
        data->tex1.u64[0] = 0x61;
        data->tex1.u64[1] = 0x14;
        data->clamp.u64[0] = 4;
        data->clamp.u64[1] = 8;
        data->alpha.u64[0] = 0x44;
        data->alpha.u64[1] = 0x42;
        data->fogcol.u64[0] = 0;
        data->fogcol.u64[1] = 0x3D;
    }
    {
        struct SDrawData *data;

        data = (struct SDrawData *)UNCACHED(&p->sdraw);
        data->giftag.u64[0] = 0x3000000000008004;
        data->giftag.u64[1] = 0x412;
        data->waittag.u64[0] = 0x8000;
        data->waittag.u64[1] = 0;
        data->envtag.u64[0] = 0x503A400000008001;
        data->envtag.u64[1] = 0xEEEEE;
        data->tex0.u64[0] = 0;
        data->tex0.u64[1] = 6;
        data->tex1.u64[0] = 0x61;
        data->tex1.u64[1] = 0x14;
        data->clamp.u64[0] = 5;
        data->clamp.u64[1] = 8;
        data->alpha.u64[0] = 0x58;
        data->alpha.u64[1] = 0x42;
        data->fogcol.u64[0] = 0;
        data->fogcol.u64[1] = 0x3D;
    }
    {
        struct SDrawData *data;

        data = (struct SDrawData *)UNCACHED(&p->bdraw);
        data->giftag.u64[0] = 0x3000000000008004;
        data->giftag.u64[1] = 0x412;
        ((struct SDrawData *)UNCACHED(&p->bdraw))->envtag.u64[0] = 0x501A400000008001;
        data->waittag.u64[0] = 0x8000;
        data->waittag.u64[1] = 0;
        data->envtag.u64[1] = 0xEEEEE;
        data->tex0.u64[0] = 0;
        data->tex0.u64[1] = 6;
        data->tex1.u64[0] = 0x61;
        data->tex1.u64[1] = 0x14;
        data->clamp.u64[0] = 5;
        data->clamp.u64[1] = 8;
        data->alpha.u64[0] = 0x44;
        data->alpha.u64[1] = 0x42;
        data->fogcol.u64[0] = 0;
        data->fogcol.u64[1] = 0x3D;
    }
}

static void InitSprData(struct SprData *p) {
    int i;

    for (i = 0; i < 2; i++) {
        struct NDrawData *data;

        data = (struct NDrawData *)UNCACHED(&p->ndraw[i]);
        data->giftag.u64[0] = 0x3000000000008004;
        data->giftag.u64[1] = 0x412;
        data->waittag.u64[0] = 0x8000;
        data->waittag.u64[1] = 0;
        data->envtag.u64[0] = 0x501E400000008001;
        data->envtag.u64[1] = 0xEEEEE;
        data->tex0.u64[0] = 0;
        data->tex0.u64[1] = 6;
        data->tex1.u64[0] = 0;
        data->tex1.u64[1] = 0x14;
        data->clamp.u64[0] = 0;
        data->clamp.u64[1] = 8;
        data->alpha.u64[0] = 0x44;
        data->alpha.u64[1] = 0x42;
        data->fogcol.u64[0] = 0;
        data->fogcol.u64[1] = 0x3D;
    }
    for (i = 0; i < 2; i++) {
        struct NDrawData *data;

        data = (struct NDrawData *)UNCACHED(&p->odraw[i]);
        data->giftag.u64[0] = 0x3000000000008004;
        data->giftag.u64[1] = 0x412;
        data->waittag.u64[0] = 0x8000;
        data->waittag.u64[1] = 0;
        data->envtag.u64[0] = 0x503E400000008001;
        data->envtag.u64[1] = 0xEEEEE;
        data->tex0.u64[0] = 0;
        data->tex0.u64[1] = 6;
        data->tex1.u64[0] = 0;
        data->tex1.u64[1] = 0x14;
        data->clamp.u64[0] = 0;
        data->clamp.u64[1] = 8;
        data->alpha.u64[0] = 0x44;
        data->alpha.u64[1] = 0x42;
        data->fogcol.u64[0] = 0;
        data->fogcol.u64[1] = 0x3D;
    }
}

static void InitData1(void) {
    static int initialized = 0;
    sceDmaChan *toSPR;

    if (!initialized) {
        InitAllDataOne(&all_data_db[0]);
        InitAllDataOne(&all_data_db[1]);
        InitSprData(&spr_data_mem);
        initialized = 1;
    }
    while (*D9_CHCR & 0x100) {
    }
    toSPR = sceDmaGetChan(9);
    toSPR->sadr = (void *)((unsigned int)spr_data & 0x3FFF);
    sceDmaSendN(toSPR, &spr_data_mem, 0x20);
    while (*D9_CHCR & 0x100) {
    }
}

/*
 * UNCACHED() is an inline function, not a macro: as a macro, tex0 and sdraw swap s1/s2 in the
 * sdraw/bdraw block.
 */
static void MakeData1(void) {
    static float mag[4] = {0.125f, 0.125f, 0.125f, 0.125f};
    static float offset[4] = {0.0f, 0.5f, 0.25f, 0.75f};
    unsigned int fogcol;

    fogcol = model3_junk.fogcol;
    InitData1();
    all_data = &all_data_db[all_data_page ^= 1];
    {
        int n_parallels;
        int i;

        n_parallels = LightNValidParallelMatrices();
        for (i = 0; i < n_parallels; i++) {
            struct PLightData *data;

            data = (struct PLightData *)UNCACHED(&all_data->plight[i]);
            LightGetNthViewNLM(data->nlm, i);
            LightGetNthLCM(data->lcm, i);
        }
    }
    {
        int n_extras;
        int i;
        struct Light *light;

        n_extras = LightNValidExtras();
        for (i = 0; i < n_extras; i++) {
            struct ELightData *data;

            light = LightNthValidExtra(i);
            data = (struct ELightData *)UNCACHED(&all_data->elight[i]);
            sceVu0CopyVector(data->pos, light->vpos);
            sceVu0CopyVector(data->dir, light->vdir);
            sceVu0CopyVector(data->col, light->color);
            data->param[0] = light->f_ra;
            data->param[1] = light->f_rb;
            data->param[2] = light->s_a;
            data->param[3] = light->s_b;
        }
    }
    {
        struct LambertData *data;

        data = (struct LambertData *)UNCACHED(&all_data->lambert);
        LightGetNthViewNLM(data->nlm, 0);
        LightGetNthLCM(data->lcm, 0);
        sceVu0CopyVector(data->global_ambient, model3_junk.global_ambient);
    }
    {
        struct EMapData *data;

        data = (struct EMapData *)UNCACHED(&all_data->emap);
        sceVu0CopyMatrix(data->vwm, model3_junk.vwm);
        sceVu0CopyVector(data->mag, mag);
        sceVu0CopyVector(data->offset, offset);
    }
    {
        struct SMapData *data;

        data = (struct SMapData *)UNCACHED(&all_data->smap);
        LightGetNthViewNHM(data->nhm, 0);
    }
    {
        struct DSetupData *data;

        data = (struct DSetupData *)UNCACHED(&all_data->dsetup);
        sceVu0CopyVector(data->vsp[0], model_common_work->vsp[0]);
        sceVu0CopyVector(data->vsp[1], model_common_work->vsp[1]);
        sceVu0CopyVector(data->vcp[0], model_common_work->vcp_gs[0]);
        sceVu0CopyVector(data->vcp[1], model_common_work->vcp_gs[1]);
    }
    {
        struct EDrawData *edraw;
        unsigned long tex0;

        edraw = (struct EDrawData *)UNCACHED(&all_data->edraw);
        tex0 = model_common_work->latitude_mapping_tex0;
        edraw->tex0.u64[0] = tex0;
        edraw->fogcol.u64[0] = fogcol;
    }
    {
        struct SDrawData *sdraw;
        struct SDrawData *bdraw;
        unsigned long tex0;
        union Q c;

        sdraw = (struct SDrawData *)UNCACHED(&all_data->sdraw);
        bdraw = (struct SDrawData *)UNCACHED(&all_data->bdraw);
        tex0 = model_common_work->specular_mapping_tex0;
        LightGetReflectionColor(c.fv);
        sdraw->tex0.u64[0] = tex0;
        sdraw->reflection_color.u128 = c.u128;
        bdraw->tex0.u64[0] = tex0;
        bdraw->reflection_color.u128 = c.u128;
        bdraw->fogcol.u64[0] = fogcol;
    }
    spr_data->ndraw[0].fogcol.u64[0] = fogcol;
    spr_data->ndraw[1].fogcol.u64[0] = fogcol;
    spr_data->odraw[0].fogcol.u64[0] = fogcol;
    spr_data->odraw[1].fogcol.u64[0] = fogcol;
}

static void InitEnv1(sceVif1Packet *pk, int id) {
    static struct Init_Gs_Packet packet4main = {
        0x1000000000008003, 0xE, 0x5000D, 0x47, 0, 0x4A, 0, 0x3F,
    };

    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, 0x11000000);
    sceVif1PkRef(pk, (u_long128 *)&packet4main, 4, 0, 0x50000004, 0);
}

static void TiniEnv(sceVif1Packet *pk) {
    static struct Packet packet = {
        0x1000000000008002, 0xE, 0, 0x3F, 5, 8,
    };

    sceVif1PkRef(pk, (u_long128 *)&packet, 3, 0, 0x50000003, 0);
}

static void MakeVu1PartTransferPacket(struct Part *part, sceVif1Packet *pk) {
    sceVif1PkRef(pk, (u_long128 *)((char *)part + part->packet_offset), part->packet_qwc, 0, 0, 0);
    {
        int n_cluster_data;
        struct ClusterData *cluster_data_top;
        float (*cluster_nodes)[4];
        int i;

        n_cluster_data = part->n_cluster_data;
        cluster_data_top = (struct ClusterData *)((char *)part + part->cluster_data_offset);
        cluster_nodes = model3_junk.cluster_nodes;
        for (i = 0; i < n_cluster_data; i++) {
            struct ClusterData *cluster_data;
            unsigned int src;
            unsigned int dst;
            unsigned int n;

            cluster_data = &cluster_data_top[i];
            src = cluster_data->src;
            dst = cluster_data->dst;
            n = cluster_data->n;
            sceVif1PkRef(pk, (u_long128 *)cluster_nodes[src], n, 0x01000104, dst | (n << 16) | 0x6C000000, 0);
        }
    }
    {
        float (*matrices)[4][4];
        int n_skeletons;
        unsigned short *skeletons;
        int dst_top;
        int i;

        matrices = model_common_work->skeleton_matrices;
        n_skeletons = part->n_skeletons;
        skeletons = (unsigned short *)((char *)part + part->skeletons_offset);
        dst_top = part->data_skeletons_offset;
        for (i = 0; i < n_skeletons; i++) {
            int skeleton_no;
            float (*src)[4];

            skeleton_no = skeletons[i];
            src = matrices[skeleton_no];
            sceVif1PkRef(pk, (u_long128 *)src, 4, 0x01000101, (dst_top + i * 4) | 0x6C040000, 0);
        }
    }
    {
        float (*envelope_matrices)[4][4];
        int n_skeleton_pairs;
        unsigned short *pairs;
        int dst_top;
        int i;

        envelope_matrices = model_common_work->envelope_matrices;
        n_skeleton_pairs = part->n_skeleton_pairs;
        pairs = (unsigned short *)((char *)part + part->skeleton_pairs_offset);
        dst_top = part->data_skeleton_pairs_offset;
        for (i = 0; i < n_skeleton_pairs; i++) {
            int pair_no;
            float (*src)[4];

            pair_no = pairs[i];
            src = envelope_matrices[pair_no];
            sceVif1PkRef(pk, (u_long128 *)src, 4, 0x01000101, (dst_top + i * 4) | 0x6C040000, 0);
        }
    }
}

/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 800
static void MakeLambertShadingPacket(struct Part *part, sceVif1Packet *pk) {
    int n_parallels;
    int n_extras;
    int i;
    struct Light *light;
    float brightness;
    struct Data *lf_data;

    n_parallels = LightNValidParallelMatrices();
    n_extras = LightNValidExtras();
    for (i = 1; i < n_parallels; i++) {
        sceVif1PkRef(pk, (u_long128 *)&all_data->plight[i], 8, 0x01000101, xitop | 0x6C080000, 0);
        sceVif1PkCnt(pk, 0);
        sceVif1PkAddCode(pk, xitop | 0x04000000);
        sceVif1PkAddCode(pk, 0x14000008);
        xitop ^= 0x200;
    }
    for (i = 0; i < n_extras; i++) {
        light = LightNthValidExtra(i);
        sceVif1PkRef(pk, (u_long128 *)&all_data->elight[i], 4, 0x01000101, xitop | 0x6C040000, 0);
        sceVif1PkCnt(pk, 0);
        sceVif1PkAddCode(pk, xitop | 0x04000000);
        switch (light->kind) {
            case 2:
                sceVif1PkAddCode(pk, 0x1400000A);
                break;
            case 3:
                sceVif1PkAddCode(pk, 0x1400000C);
                break;
            default:
                assert(0);
        }
        xitop ^= 0x200;
    }
    brightness = LightReflectionBrightness();
    sceVif1PkRef(pk, (u_long128 *)&all_data->lambert, 10, 0x01000101, xitop | 0x6C0A0000, 0);
    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, 0x01000101);
    sceVif1PkAddCode(pk, (xitop + 10) | 0x6C030000);
    lf_data = (struct Data *)sceVif1PkReserve(pk, 12);
    /* Matching: the #line keeps the assert string below on the original's line. */
#line 885
    assert(((u_int)lf_data & 0x03) == 0);
    sceVu0CopyVector(lf_data->diffuse, part->diffuse);
    lf_data->diffuse[3] = 1.0f;
    sceVu0CopyVector(lf_data->ambient, part->ambient);
    lf_data->ambient[3] = 1.0f;
    lf_data->param[2] = part->phong_param_a * brightness;
    lf_data->param[3] = part->phong_param_b * brightness;
    sceVif1PkAddCode(pk, xitop | 0x04000000);
    sceVif1PkAddCode(pk, 0x1400000E);
    xitop ^= 0x200;
}

/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 915
static void MakeNormalPacket(struct Part *part, sceVif1Packet *pk) {
    int n_textures;
    unsigned short *text_pos_indices;
    struct TextPosParam *text_pos_params;
    struct TextureParam *texture_params;
    int mpg;
    sceDmaChan *fromSPR;
    int i;

    n_textures = part->n_textures;
    text_pos_indices = (unsigned short *)((char *)part + part->text_pos_indices_offset);
    text_pos_params = model_common_work->text_pos_params;
    texture_params = (struct TextureParam *)((char *)part + part->texture_params_offset);
    mpg = (unsigned char)(part->backclip == 0 ? 0x16 : 0x18);
    fromSPR = sceDmaGetChan(8);
    for (i = 0; i < n_textures; i++) {
        int text_pos_index;
        struct TextPosParam *text_pos;
        struct TextureParam *texture;
        struct NDrawData *spr;
        struct NDrawData *data;

        text_pos_index = text_pos_indices[i];
        text_pos = &text_pos_params[text_pos_index];
        texture = &texture_params[i];
        spr = &spr_data->ndraw[i % 2];
        sceVif1PkCnt(pk, 0);
        sceVif1PkAddCode(pk, 0x11000000);
        sceVif1PkAddCode(pk, xitop | 0x6C080000);
        data = (struct NDrawData *)sceVif1PkReserve(pk, 0x20);
        assert_dw(((u_int)data & 0x03) == 0);
        spr->tex0.u64[0] = text_pos->tex0;
        spr->tex1.u64[0] = texture->tex1;
        spr->clamp.u64[0] = texture->clamp;
        fromSPR->sadr = (void *)((unsigned int)spr & 0x3FFF);
        sceDmaSendN(fromSPR, (void *)((unsigned int)data & 0x0FFFFFFF), 8);
        sceVif1PkAddCode(pk, xitop | 0x04000000);
        sceVif1PkAddCode(pk, mpg | 0x14000000);
    }
}

/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 974
static void MakeEnvironPacket(struct Part *part, sceVif1Packet *pk) {
    struct Data *data;
    int mpg;

    mpg = (unsigned char)(part->backclip == 0 ? 0x1A : 0x1C);
    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, 0x11000000);
    sceVif1PkRef(pk, (u_long128 *)&all_data->edraw, 8, 0x01000101, xitop | 0x6C080000, 0);
    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, 0x01000101);
    sceVif1PkAddCode(pk, (xitop + 8) | 0x6C010000);
    data = (struct Data *)sceVif1PkReserve(pk, 4);
    assert(((u_int)data & 0x03) == 0);
    ((unsigned int *)data)[0] = 0x80;
    ((unsigned int *)data)[1] = 0x80;
    ((unsigned int *)data)[2] = 0x80;
    ((unsigned int *)data)[3] = part->envmap_param;
    sceVif1PkAddCode(pk, xitop | 0x04000000);
    sceVif1PkAddCode(pk, mpg | 0x14000000);
}

/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 1005
static void MakeSpecularPacket(struct Part *part, sceVif1Packet *pk) {
    struct Data *data;
    int mpg;

    mpg = (unsigned char)(part->backclip == 0 ? 0x1E : 0x20);
    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, 0x11000000);
    sceVif1PkRef(pk, (u_long128 *)&all_data->sdraw, 9, 0x01000101, xitop | 0x6C090000, 0);
    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, 0x01000101);
    sceVif1PkAddCode(pk, (xitop + 9) | 0x6C010000);
    data = (struct Data *)sceVif1PkReserve(pk, 4);
    assert(((u_int)data & 0x03) == 0);
    sceVu0CopyVector(data->diffuse, part->specular);
    data->diffuse[3] = 128.0f;
    sceVif1PkAddCode(pk, xitop | 0x04000000);
    sceVif1PkAddCode(pk, mpg | 0x14000000);
}

/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 1039
static void MakeBaseSpecularPacket(struct Part *part, sceVif1Packet *pk) {
    struct Data *data;
    int mpg;

    mpg = (unsigned char)(part->backclip == 0 ? 0x1E : 0x20);
    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, 0x11000000);
    sceVif1PkRef(pk, (u_long128 *)&all_data->bdraw, 9, 0x01000101, xitop | 0x6C090000, 0);
    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, 0x01000101);
    sceVif1PkAddCode(pk, (xitop + 9) | 0x6C010000);
    data = (struct Data *)sceVif1PkReserve(pk, 4);
    assert(((u_int)data & 0x03) == 0);
    sceVu0CopyVector(data->diffuse, part->specular);
    data->diffuse[3] = 128.0f;
    sceVif1PkAddCode(pk, xitop | 0x04000000);
    sceVif1PkAddCode(pk, mpg | 0x14000000);
}

/* Matching: the #line keeps the assert string below on the original's line. */
#line 1057
static void MakeOverPacket(struct Part *part, sceVif1Packet *pk) {
    int n_textures;
    unsigned short *text_pos_indices;
    struct TextPosParam *text_pos_params;
    struct TextureParam *texture_params;
    int mpg;
    sceDmaChan *fromSPR;
    int i;

    n_textures = part->n_textures;
    text_pos_indices = (unsigned short *)((char *)part + part->text_pos_indices_offset);
    text_pos_params = model_common_work->text_pos_params;
    texture_params = (struct TextureParam *)((char *)part + part->texture_params_offset);
    mpg = (unsigned char)(part->backclip == 0 ? 0x16 : 0x18);
    fromSPR = sceDmaGetChan(8);
    for (i = 0; i < n_textures; i++) {
        int text_pos_index; struct TextPosParam *text_pos; struct TextureParam *texture; struct NDrawData *spr; struct NDrawData *data;
        text_pos_index = text_pos_indices[i];
        text_pos = &text_pos_params[text_pos_index];
        texture = &texture_params[i];
        spr = &spr_data->odraw[i % 2];
        sceVif1PkCnt(pk, 0);
        sceVif1PkAddCode(pk, 0x11000000);
        sceVif1PkAddCode(pk, xitop | 0x6C080000);
        data = (struct NDrawData *)sceVif1PkReserve(pk, 0x20);
        assert(((u_int)data & 0x03) == 0);
        spr->tex0.u64[0] = text_pos->tex0;
        spr->tex1.u64[0] = texture->tex1;
        spr->clamp.u64[0] = texture->clamp;
        fromSPR->sadr = (void *)((unsigned int)spr & 0x3FFF);
        sceDmaSendN(fromSPR, (void *)((unsigned int)data & 0x0FFFFFFF), 8);
        sceVif1PkAddCode(pk, xitop | 0x04000000);
        sceVif1PkAddCode(pk, mpg | 0x14000000);
    }
}

static void MakeDrawPacket(struct Part *part, sceVif1Packet *pk) {
    u_long128 *gifad;

    sceVif1PkRef(pk, (u_long128 *)&all_data->dsetup, 8, 0x01000101, xitop | 0x6C080000, 0);
    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, xitop | 0x04000000);
    sceVif1PkAddCode(pk, 0x14000014);
    xitop ^= 0x200;
    if (!part->specular_pos) {
        MakeNormalPacket(part, pk);
        if (part->envmap_param) {
            MakeEnvironPacket(part, pk);
        }
        if (part->shading_type == 4) {
            gifad = (u_long128 *)&shGs_AllEnv.Now_DrawEnv.frame_mskalpha - 1;
            sceVif1PkRef(pk, gifad, 2, 0x11000000, 0x50000002, 0);
            MakeSpecularPacket(part, pk);
            gifad = (u_long128 *)&shGs_AllEnv.Now_DrawEnv.frame_normal - 1;
            sceVif1PkRef(pk, gifad, 2, 0x11000000, 0x50000002, 0);
        }
    } else {
        if (part->shading_type == 4) {
            MakeBaseSpecularPacket(part, pk);
        }
        MakeOverPacket(part, pk);
    }
}

/* Adds f's bit pattern as a data word. An inline helper in the original (d has no DWARF). */
static inline void PkAddFloat(sceVif1Packet *pk, float f) {
    unsigned int d;

    d = *(unsigned int *)&f;
    sceVif1PkAddData(pk, d);
}

/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 1174
static void DrawPart1(struct Part *part, sceVif1Packet *pk) {
    xitop = 0x1F0;
    if (part->xtop == prev_xtop) {
        sceVif1PkCnt(pk, 0);
        sceVif1PkAddCode(pk, 0x11000000);
    }
    prev_xtop = part->xtop;
    MakeVu1PartTransferPacket(part, pk);
    sceVif1PkCnt(pk, 0);
    sceVif1PkAddCode(pk, part->xtop | 0x04000000);
    sceVif1PkAddCode(pk, 0x11000000);
    sceVif1PkAddCode(pk, xitop | 0x6C010000);
    if (part->shading_type == 1) {
        PkAddFloat(pk, part->diffuse[0]);
        PkAddFloat(pk, part->diffuse[1]);
        PkAddFloat(pk, part->diffuse[2]);
        PkAddFloat(pk, 128.0f);
    } else {
        sceVif1PkAddData(pk, 0);
        sceVif1PkAddData(pk, 0);
        sceVif1PkAddData(pk, 0);
        sceVif1PkAddData(pk, 0);
    }
    sceVif1PkAddCode(pk, 0x14000006);
    xitop ^= 0x200;
    switch (part->shading_type) {
        case 1:
            break;
        case 2:
        case 3:
        case 4:
            MakeLambertShadingPacket(part, pk);
            break;
        default:
            assert_dw(0);
    }
    if (part->envmap_param) {
        sceVif1PkRef(pk, (u_long128 *)&all_data->emap, 6, 0x01000101, xitop | 0x6C060000, 0);
        sceVif1PkCnt(pk, 0);
        sceVif1PkAddCode(pk, xitop | 0x04000000);
        sceVif1PkAddCode(pk, 0x14000010);
        xitop ^= 0x200;
    }
    if (part->shading_type == 4) {
        struct Data *data;

        sceVif1PkRef(pk, (u_long128 *)&all_data->smap, 4, 0x01000101, xitop | 0x6C040000, 0);
        sceVif1PkCnt(pk, 0);
        sceVif1PkAddCode(pk, 0x01000101);
        sceVif1PkAddCode(pk, (xitop + 4) | 0x6C010000);
        data = (struct Data *)sceVif1PkReserve(pk, 4);
        /* Matching: the #line keeps the assert string below on the original's line. */
#line 1238
        assert(((u_int)data & 0x03) == 0);
        data->diffuse[3] = part->blinn_param;
        sceVif1PkAddCode(pk, xitop | 0x04000000);
        sceVif1PkAddCode(pk, 0x14000012);
        xitop ^= 0x200;
    }
    MakeDrawPacket(part, pk);
}

static void DrawParts1(struct sh_Model *model, struct ModelWork *work) {
    u_long128 *packet_buffer;
    sceVif1Packet packet;
    sceVif1Packet *pk;
    int n_parts;
    struct Part *parts_top;
    struct Part *part;
    int i;
    int itex;
    void *pktop;
    int st;
    int en;

    packet_buffer = ktVif1PkBufNext();
    pk = &packet;
    n_parts = model->n_vu1_parts;
    part = parts_top = (struct Part *)((char *)model + model->vu1_parts_offset);
    prev_xtop = 1;
    xitop = 0x1F0;
    MakeData1();
    sceVif1PkInit(pk, (u_long128 *)UNCACHED(packet_buffer));
    pktop = packet.pBase;
    InitEnv1(pk, sh2gfw_get_Charaid());
    for (itex = 0; itex < sh2gfw_get_TBChangeVU1num(); itex++) {
        st = sh2gfw_get_ModelChangeTB(itex);
        en = sh2gfw_get_ModelChangeTB(itex + 1);
        for (; st < en; st++, part = (struct Part *)((char *)part + part->size)) {
            if (Model3WorkEquipmentFlag(work, part->equipment_id)) {
                DrawPart1(part, pk);
            }
        }
        sceVif1PkEnd(pk, 0);
        sceVif1PkAddCode(pk, 0x11000000);
        sceVif1PkAddCode(pk, 0);
        sh2gfw_Thr_Chracter_d1d2SyncKick(pktop, sh2gfw_get_ModelIndexTB(itex));
        pktop = pk->pCurrent;
    }
    TiniEnv(pk);
    sceVif1PkEnd(pk, 0);
    sceVif1PkAddCode(pk, 0x11000000);
    sceVif1PkAddCode(pk, 0);
    sceVif1PkTerminate(pk);
    sh2gfw_set_CharaD1CID(d1cSend((void *)((unsigned int)pktop & 0x0FFFFFFF)));
}

/**
 * Draws the VU1 parts of a model (those its equipment flags enable) and sends the packet on
 * DMA channel 1.
 * @param model the struct Model
 * @param work  its work
 */
void Model3DrawVu1Parts(struct Model *model, struct ModelWork *work) {
    muga ^= 1;
    DrawParts1((struct sh_Model *)model, work);
}
