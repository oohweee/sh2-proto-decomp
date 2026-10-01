/*
 * Model3 VU0 renderer (Chacter_Draw): the character parts that VU0 transforms and lights
 * (lambert, specular and environment-mapped, with optional clipping). VU0 microprograms build
 * the vertices in scratchpad memory; the CPU turns the triangle strips into GS packets and
 * sorts them by depth into the character ordering table.
 */
#include "sh2.h"
#include "hh_math.h"
#include "model3_helpers.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"
#include "sdk/libvifpk.h"
#include "sdk/libvu0.h"

typedef unsigned int u_int;

extern u_long128 model3_mpg0_skel_load[];
extern u_long128 model3_mpg0_para[];
extern int model3_mpg0_para_size;
extern u_long128 model3_mpg0_point[];
extern int model3_mpg0_point_size;
extern u_long128 model3_mpg0_spot[];
extern int model3_mpg0_spot_size;
extern u_long128 model3_mpg0_lambert[];
extern int model3_mpg0_lambert_size;
extern u_long128 model3_mpg0_clip0v[];
extern int model3_mpg0_clip0v_size;
extern u_long128 model3_mpg0_clip1[];
extern int model3_mpg0_clip1_size;
extern u_long128 model3_mpg0_clipv[];
extern int model3_mpg0_clipv_size;
extern u_long128 model3_mpg0_venvmap[];
extern int model3_mpg0_venvmap_size;
extern u_long128 model3_mpg0_specular[];
extern int model3_mpg0_specular_size;
extern u_long128 model3_mpg0_persfvg[];
extern int model3_mpg0_persfvg_size;

#define UNCACHED(p) (((unsigned int)(p) & 0x0FFFFFFF) | 0x20000000)

static int n_bad_depths;
static int calc_base;
static int draw_base;
static unsigned int xitop;
static unsigned int xmtop;
static struct AllData_Vu0 alldata_Vu0_Dblbuffer[2] __attribute__((aligned(64)));
static struct AllData_Vu0 *pAllData_Vu0;
static struct AllPacket *all_packet = (struct AllPacket *)0x70002000;
static int hogehoge = 0;
static int fugahoge = 0;
static int alldata_Vu0_page = 0;

static void InitTriangleNormal(struct TriangleNormal *p) {
    int qwc;

    qwc = sizeof(struct TriangleNormal) / 16 - 1;
    p->dmatag.u64[0] = qwc | 0x20000000;
    p->dmatag.u32[2] = 0;
    p->dmatag.u32[3] = qwc | 0x50000000;
    p->n_giftag.u64[0] = 0xB03DC00000008001;
    p->n_giftag.u64[1] = 0x412412412EE;
    p->n_tex0.u64[1] = 6;
    p->n_clamp.u64[1] = 8;
}

static void InitTriangleNormalEnviron(struct TriangleNormalEnviron *p) {
    int qwc;

    qwc = sizeof(struct TriangleNormalEnviron) / 16 - 1;
    p->dmatag.u64[0] = qwc | 0x20000000;
    p->dmatag.u32[2] = 0;
    p->dmatag.u32[3] = qwc | 0x50000000;
    p->n_giftag.u64[0] = 0xB03DC00000008001;
    p->n_giftag.u64[1] = 0x412412412EE;
    p->n_tex0.u64[1] = 6;
    p->n_clamp.u64[1] = 8;
    p->e_giftag.u64[0] = 0x9039C00000008001;
    p->e_giftag.u64[1] = 0x424242EEE;
    p->e_tex0.u64[1] = 6;
    p->e_clamp.u64[0] = 4;
    p->e_clamp.u64[1] = 8;
    p->e_rgba.u64[0] = GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, Float_Bits(1.0f));
    p->e_rgba.u64[1] = 1;
    p->e_stq2.fv[2] = p->e_stq1.fv[2] = p->e_stq0.fv[2] = 1.0f;
}

static void InitTriangleNormalSpecular(struct TriangleNormalSpecular *p) {
    int qwc;

    qwc = sizeof(struct TriangleNormalSpecular) / 16 - 1;
    p->dmatag.u64[0] = qwc | 0x20000000;
    p->dmatag.u32[2] = 0;
    p->dmatag.u32[3] = qwc | 0x50000000;
    p->n_giftag.u64[0] = 0xB03DC00000008001;
    p->n_giftag.u64[1] = 0x412412412EE;
    p->n_tex0.u64[1] = 6;
    p->n_clamp.u64[1] = 8;
    p->s_giftag.u64[0] = 0xD039C00000008001;
    p->s_giftag.u64[1] = 0xEE4242421EEEE;
    p->s_tex0.u64[0] = model_common_work->specular_mapping_tex0;
    p->s_tex0.u64[1] = 6;
    p->s_clamp.u64[0] = 5;
    p->s_clamp.u64[1] = 8;
    p->s_alpha.u64[0] = 0x48;
    p->s_alpha.u64[1] = 0x42;
    p->s_fogcol.u64[0] = 0;
    p->s_fogcol.u64[1] = 0x3D;
    p->s_stq2.fv[2] = p->s_stq1.fv[2] = p->s_stq0.fv[2] = 1.0f;
    p->S_alpha.u64[0] = 0x44;
    p->S_alpha.u64[1] = 0x42;
    p->S_fogcol.u64[1] = 0x3D;
}

static void InitTriangleNormalEnvironSpecular(struct TriangleNormalEnvironSpecular *p) {
    int qwc;

    qwc = sizeof(struct TriangleNormalEnvironSpecular) / 16 - 1;
    p->dmatag.u64[0] = qwc | 0x20000000;
    p->dmatag.u32[2] = 0;
    p->dmatag.u32[3] = qwc | 0x50000000;
    p->n_giftag.u64[0] = 0xB03DC00000008001;
    p->n_giftag.u64[1] = 0x412412412EE;
    p->n_tex0.u64[1] = 6;
    p->n_clamp.u64[1] = 8;
    p->e_giftag.u64[0] = 0x9039C00000008001;
    p->e_giftag.u64[1] = 0x424242EEE;
    p->e_tex0.u64[1] = 6;
    p->e_clamp.u64[0] = 4;
    p->e_clamp.u64[1] = 8;
    p->e_rgba.u64[0] = GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, Float_Bits(1.0f));
    p->e_rgba.u64[1] = 1;
    p->e_stq2.fv[2] = p->e_stq1.fv[2] = p->e_stq0.fv[2] = 1.0f;
    p->s_giftag.u64[0] = 0xD039C00000008001;
    p->s_giftag.u64[1] = 0xEE4242421EEEE;
    p->s_tex0.u64[0] = model_common_work->specular_mapping_tex0;
    p->s_tex0.u64[1] = 6;
    p->s_clamp.u64[0] = 5;
    p->s_clamp.u64[1] = 8;
    p->s_alpha.u64[0] = 0x48;
    p->s_alpha.u64[1] = 0x42;
    p->s_fogcol.u64[0] = 0;
    p->s_fogcol.u64[1] = 0x3D;
    p->s_stq2.fv[2] = p->s_stq1.fv[2] = p->s_stq0.fv[2] = 1.0f;
    p->S_alpha.u64[0] = 0x44;
    p->S_alpha.u64[1] = 0x42;
    p->S_fogcol.u64[1] = 0x3D;
}

static void InitTriangleSpecularNormal(struct TriangleSpecularNormal *p) {
    int qwc;

    qwc = sizeof(struct TriangleSpecularNormal) / 16 - 1;
    p->dmatag.u64[0] = qwc | 0x20000000;
    p->dmatag.u32[2] = 0;
    p->dmatag.u32[3] = qwc | 0x50000000;
    p->s_giftag.u64[0] = 0xD039C00000008001;
    p->s_giftag.u64[1] = 0xEE4242421EEEE;
    p->s_tex0.u64[0] = model_common_work->specular_mapping_tex0;
    p->s_tex0.u64[1] = 6;
    p->s_clamp.u64[0] = 5;
    p->s_clamp.u64[1] = 8;
    p->s_alpha.u64[0] = 0x48;
    p->s_alpha.u64[1] = 0x42;
    p->s_fogcol.u64[0] = 0;
    p->s_fogcol.u64[1] = 0x3D;
    p->s_stq2.fv[2] = p->s_stq1.fv[2] = p->s_stq0.fv[2] = 1.0f;
    p->S_alpha.u64[0] = 0x44;
    p->S_alpha.u64[1] = 0x42;
    p->S_fogcol.u64[1] = 0x3D;
    p->n_giftag.u64[0] = 0xB03DC00000008001;
    p->n_giftag.u64[1] = 0x412412412EE;
    p->n_tex0.u64[1] = 6;
    p->n_clamp.u64[1] = 8;
}

static void InitAllPacket0(struct AllPacket *p) {
    InitTriangleNormal(&p->normal[0]);
    InitTriangleNormal(&p->normal[1]);
    InitTriangleNormalEnviron(&p->normal_environ[0]);
    InitTriangleNormalEnviron(&p->normal_environ[1]);
    InitTriangleNormalSpecular(&p->normal_specular[0]);
    InitTriangleNormalSpecular(&p->normal_specular[1]);
    InitTriangleNormalEnvironSpecular(&p->normal_environ_specular[0]);
    InitTriangleNormalEnvironSpecular(&p->normal_environ_specular[1]);
    InitTriangleSpecularNormal(&p->specular_normal[0]);
    InitTriangleSpecularNormal(&p->specular_normal[1]);
}

static void LoadProgram_Vu0(void) {
    static u_long128 packet_buffer[4];
    static int initialized = 0;
    sceVif0Packet packet;
    sceVif0Packet *pk;

    if (!initialized) {
        pk = &packet;
        sceVif0PkInit(pk, (u_long128 *)UNCACHED(packet_buffer));
        sceVif0PkCall(pk, model3_mpg0_skel_load, 0);
        sceVif0PkEnd(pk, 0);
        sceVif0PkTerminate(pk);
        initialized = 1;
    }
    while (*D0_CHCR & 0x100) {
    }
    *D0_QWC = 0;
    *D0_TADR = (unsigned int)packet_buffer;
    *D0_CHCR = 0x145;
    while (*D0_CHCR & 0x100) {
    }
}

/* Matching: 16 bytes of .bss with no DWARF and no reference in the binary; name unknown. */
static u_long128 model3_vu0_unknown_bss[1];
static int chr_flg;

static void MakeData0(void) {
    static float mag[4] = {0.125f, 0.125f, 0.125f, 0.125f};
    static float offset[4] = {0.0f, 0.5f, 0.25f, 0.75f};

    pAllData_Vu0 = &alldata_Vu0_Dblbuffer[alldata_Vu0_page ^= 1];
    {
        int n_parallels;
        int i;

        n_parallels = LightNValidParallelMatrices();
        for (i = 0; i < n_parallels; i++) {
            struct PLightData *pldata;

            pldata = (struct PLightData *)UNCACHED(&pAllData_Vu0->plight[i]);
            LightGetNthViewNLM(pldata->nlm, i);
            LightGetNthLCM(pldata->lcm, i);
        }
    }
    {
        int n_extras;
        int i;
        struct Light *light;

        n_extras = LightNValidExtras();
        for (i = 0; i < n_extras; i++) {
            struct ELightData *eldata;

            light = LightNthValidExtra(i);
            eldata = (struct ELightData *)UNCACHED(&pAllData_Vu0->elight[i]);
            sceVu0CopyVector(eldata->pos, light->vpos);
            sceVu0CopyVector(eldata->dir, light->vdir);
            sceVu0CopyVector(eldata->col, light->color);
            eldata->param[0] = light->f_ra;
            eldata->param[1] = light->f_rb;
            eldata->param[2] = light->s_a;
            eldata->param[3] = light->s_b;
        }
    }
    {
        struct Lambert0Data *lmdata;

        lmdata = (struct Lambert0Data *)UNCACHED(&pAllData_Vu0->lambert0);
        LightGetNthViewNLM(lmdata->nlm, 0);
        LightGetNthLCM(lmdata->lcm, 0);
    }
    {
        struct Lambert1Data *lm1data;

        lm1data = (struct Lambert1Data *)UNCACHED(&pAllData_Vu0->lambert1);
        sceVu0CopyVector(lm1data->global_ambient, model3_junk.global_ambient);
        lm1data->global_ambient[3] = 128.0f;
    }
    {
        struct EMapData *emdata;

        emdata = (struct EMapData *)UNCACHED(&pAllData_Vu0->emap);
        sceVu0CopyMatrix(emdata->vwm, model3_junk.vwm);
        sceVu0CopyVector(emdata->mag, mag);
        sceVu0CopyVector(emdata->offset, offset);
    }
    {
        struct SMapData *smdata;

        smdata = (struct SMapData *)UNCACHED(&pAllData_Vu0->smap);
        LightGetNthViewNHM(smdata->nhm, 0);
    }
    {
        struct PersData *psdata;

        psdata = (struct PersData *)UNCACHED(&pAllData_Vu0->pers);
        sceVu0CopyVector(psdata->vsp[0], model_common_work->vsp[0]);
        sceVu0CopyVector(psdata->vsp[1], model_common_work->vsp[1]);
        sceVu0CopyVector(psdata->vcp[0], model_common_work->vcp[0]);
        sceVu0CopyVector(psdata->vcp[1], model_common_work->vcp[1]);
        sceVu0CopyVector(psdata->xyz_min, model3_junk.xyz_min_wide);
        sceVu0CopyVector(psdata->xyz_max, model3_junk.xyz_max_wide);
        sceVu0CopyVector(psdata->rgba_max, model3_junk.rgba_max);
    }
}

static void MakePartTransferPacket_Vu0(struct Part *part, sceVif0Packet *pk) {
    sceVif0PkRef(pk, (u_long128 *)((char *)part + part->packet_offset), part->packet_qwc, 0, 0, 0);
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
            sceVif0PkRef(pk, (u_long128 *)cluster_nodes[src], n, 0x01000104, dst | (n << 16) | 0x6C000000, 0);
        }
    }
    {
        float (*matrices)[4][4];
        unsigned short *skeletons;
        int dst_top;
        int i;

        matrices = model_common_work->skeleton_matrices;
        skeletons = (unsigned short *)((char *)part + part->skeletons_offset);
        dst_top = part->data_skeletons_offset;
        for (i = 0; i < part->n_skeletons; i++) {
            int skeleton_no;
            float (*src)[4];

            skeleton_no = skeletons[i];
            src = matrices[skeleton_no];
            sceVif0PkRef(pk, (u_long128 *)src, 4, 0x01000101, (dst_top + i * 4) | 0x6C040000, 0);
        }
    }
    {
        unsigned short *pairs;
        int dst_top;
        int i;

        pairs = (unsigned short *)((char *)part + part->skeleton_pairs_offset);
        dst_top = part->data_skeleton_pairs_offset;
        for (i = 0; i < part->n_skeleton_pairs; i++) {
            int pair_no;
            float (*src)[4];

            pair_no = pairs[i];
            src = model_common_work->envelope_matrices[pair_no];
            sceVif0PkRef(pk, (u_long128 *)src, 4, 0x01000101, (dst_top + i * 4) | 0x6C040000, 0);
        }
    }
}

/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 808
static void MakeLambertShadingPacket(struct Part *part, sceVif0Packet *pk) {
    int n_parallels;
    int n_extras;
    int i;
    struct Light *light;
    float brightness;
    struct Data *data;

    n_parallels = LightNValidParallelMatrices();
    n_extras = LightNValidExtras();
    if (n_parallels > 1) {
        xmtop ^= 0x80;
        sceVif0PkRefMpg(pk, xmtop, model3_mpg0_para, model3_mpg0_para_size, 0);
    }
    for (i = 1; i < n_parallels; i++) {
        sceVif0PkRef(pk, (u_long128 *)&pAllData_Vu0->plight[i], 8, 0x01000101, xitop | 0x6C080000, 0);
        sceVif0PkCnt(pk, 0);
        sceVif0PkAddCode(pk, xitop | 0x04000000);
        sceVif0PkAddCode(pk, xmtop | 0x14000000);
        xitop ^= 8;
    }
    for (i = 0; i < n_extras; i++) {
        light = LightNthValidExtra(i);
        xmtop ^= 0x80;
        switch (light->kind) {
            case 2:
                sceVif0PkRefMpg(pk, xmtop, model3_mpg0_point, model3_mpg0_point_size, 0);
                break;
            case 3:
                sceVif0PkRefMpg(pk, xmtop, model3_mpg0_spot, model3_mpg0_spot_size, 0);
                break;
            default:
                assert_dw(0);
        }
        sceVif0PkRef(pk, (u_long128 *)&pAllData_Vu0->elight[i], 4, 0x01000101, xitop | 0x6C040000, 0);
        sceVif0PkCnt(pk, 0);
        sceVif0PkAddCode(pk, xitop | 0x04000000);
        sceVif0PkAddCode(pk, xmtop | 0x14000000);
        xitop ^= 8;
    }
    xmtop ^= 0x80;
    sceVif0PkRefMpg(pk, xmtop, model3_mpg0_lambert, model3_mpg0_lambert_size, 0);
    sceVif0PkRef(pk, (u_long128 *)&pAllData_Vu0->lambert0, 8, 0x01000101, xitop | 0x6C080000, 0);
    sceVif0PkCnt(pk, 0);
    sceVif0PkAddCode(pk, xitop | 0x04000000);
    sceVif0PkAddCode(pk, xmtop | 0x14000000);
    xitop ^= 8;
    brightness = LightReflectionBrightness();
    sceVif0PkRef(pk, (u_long128 *)&pAllData_Vu0->lambert1, 1, 0x01000101, xitop | 0x6C010000, 0);
    sceVif0PkCnt(pk, 0);
    sceVif0PkAddCode(pk, 0x01000101);
    sceVif0PkAddCode(pk, (xitop + 1) | 0x6C030000);
    data = (struct Data *)sceVif0PkReserve(pk, 12);
    /* Matching: the #line keeps the assert string below on the original's line. */
#line 881
    assert_dw(((u_int)data & 0x03) == 0);
    sceVu0CopyVector(data->diffuse, part->diffuse);
    data->diffuse[3] = 1.0f;
    sceVu0CopyVector(data->ambient, part->ambient);
    data->ambient[3] = 1.0f;
    data->param[2] = part->phong_param_a * brightness;
    data->param[3] = part->phong_param_b * brightness;
    sceVif0PkAddCode(pk, xitop | 0x04000000);
    sceVif0PkAddCode(pk, (xmtop + 2) | 0x14000000);
    xitop ^= 8;
}

static void FlipXMTOP(void);

static void MakeClipPacket(struct Part *part, sceVif0Packet *pk) {
    if (part->backclip || model3_junk.view_clip_or) {
        if (!part->backclip) {
            if (model3_junk.view_clip_or) {
                FlipXMTOP();
                sceVif0PkRefMpg(pk, xmtop, model3_mpg0_clip0v, model3_mpg0_clip0v_size, 0);
                sceVif0PkCnt(pk, 0);
                sceVif0PkAddCode(pk, xmtop | 0x14000000);
            }
        } else {
            FlipXMTOP();
            sceVif0PkRefMpg(pk, xmtop, model3_mpg0_clip1, model3_mpg0_clip1_size, 0);
            sceVif0PkCnt(pk, 0);
            sceVif0PkAddCode(pk, xmtop | 0x14000000);
            if (model3_junk.view_clip_or) {
                FlipXMTOP();
                sceVif0PkRefMpg(pk, xmtop, model3_mpg0_clipv, model3_mpg0_clipv_size, 0);
                sceVif0PkCnt(pk, 0);
                sceVif0PkAddCode(pk, xmtop | 0x14000000);
            }
        }
    }
}

static void FlipXMTOP(void) {
    xmtop ^= 0x80;
}

/* Matching: the #line keeps the assert strings below on the original's lines. */
#line 997
static void MakeCalcPartPacket(struct Part *part) {
    u_long128 *packet_buffer;
    sceVif0Packet packet;
    sceVif0Packet *pk;
    struct Data *data;

    packet_buffer = ktVif0PkBufNext();
    pk = &packet;
    sceVif0PkInit(pk, (u_long128 *)UNCACHED(packet_buffer));
    xitop = 0xF0;
    xmtop = 0x100;
    model3_junk.xtop = (void *)(part->xtop * 16 + 0x70000000);
    MakePartTransferPacket_Vu0(part, pk);
    sceVif0PkCnt(pk, 0);
    sceVif0PkAddCode(pk, part->xtop | 0x04000000);
    sceVif0PkAddCode(pk, xitop | 0x6C010000);
    if (part->shading_type == 1) {
        PkAddFloat(pk, part->diffuse[0]);
        PkAddFloat(pk, part->diffuse[1]);
        PkAddFloat(pk, part->diffuse[2]);
    } else {
        sceVif0PkAddData(pk, 0);
        sceVif0PkAddData(pk, 0);
        sceVif0PkAddData(pk, 0);
    }
    sceVif0PkAddData(pk, 0);
    sceVif0PkAddCode(pk, 0x14000008);
    xitop ^= 8;
    switch (part->shading_type) {
        case 1:
            break;
        case 2:
        case 3:
        case 4:
            MakeLambertShadingPacket(part, pk);
            break;
        default:
            assert(0);
    }
    if (part->envmap_param) {
        xmtop ^= 0x80;
        sceVif0PkRefMpg(pk, xmtop, model3_mpg0_venvmap, model3_mpg0_venvmap_size, 0);
        sceVif0PkRef(pk, (u_long128 *)&pAllData_Vu0->emap, 6, 0x01000101, xitop | 0x6C060000, 0);
        sceVif0PkCnt(pk, 0);
        sceVif0PkAddCode(pk, xitop | 0x04000000);
        sceVif0PkAddCode(pk, xmtop | 0x14000000);
        xitop ^= 8;
    }
    if (part->shading_type == 4) {
        xmtop ^= 0x80;
        sceVif0PkRefMpg(pk, xmtop, model3_mpg0_specular, model3_mpg0_specular_size, 0);
        sceVif0PkRef(pk, (u_long128 *)&pAllData_Vu0->smap, 4, 0x01000101, xitop | 0x6C040000, 0);
        sceVif0PkCnt(pk, 0);
        sceVif0PkAddCode(pk, 0x01000101);
        sceVif0PkAddCode(pk, (xitop + 4) | 0x6C010000);
        data = (struct Data *)sceVif0PkReserve(pk, 4);
    /* Matching: the #line keeps the assert string below on the original's line. */
#line 1070
        assert_dw(((u_int)data & 0x03) == 0);
        data->diffuse[3] = part->blinn_param;
        sceVif0PkAddCode(pk, xitop | 0x04000000);
        sceVif0PkAddCode(pk, xmtop | 0x14000000);
        xitop ^= 8;
    }
    xmtop ^= 0x80;
    sceVif0PkRefMpg(pk, xmtop, model3_mpg0_persfvg, model3_mpg0_persfvg_size, 0);
    sceVif0PkRef(pk, (u_long128 *)&pAllData_Vu0->pers, 7, 0x01000101, xitop | 0x6C070000, 0);
    sceVif0PkCnt(pk, 0);
    sceVif0PkAddCode(pk, xitop | 0x04000000);
    sceVif0PkAddCode(pk, xmtop | 0x14000000);
    sceVif0PkAddCode(pk, 0x10000000);
    xitop ^= 8;
    MakeClipPacket(part, pk);
    sceVif0PkCnt(pk, 0);
    sceVif0PkAddCode(pk, 0x10000000);
    sceVif0PkEnd(pk, 0);
    sceVif0PkTerminate(pk);
}

static void KickCalcPartPacket(void) {
    ktVif0Send(ktVif0PkBufCurrent(), 1);
}

static void TransferToSPR(void) {
    sceDmaChan *toSPR;

    toSPR = sceDmaGetChan(9);
    ktVif0Wait2();
    toSPR->sadr = (void *)calc_base;
    sceDmaSendN(toSPR, (void *)0x11004000, 0xF0);
    calc_base ^= 0x1000;
}

static void PrepareSort(void) {
    InitAllPacket0(all_packet);
}

/* Per-word signed minimum of two quadwords (MMI pminw); MWCC C has no 128-bit operators. */
static inline u_long128 pminw(u_long128 a, u_long128 b) {
    u_long128 r;

    __asm__ __volatile__("
    pminw %0, %1, %2
    " : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static void SortTriangleNormal(struct ktVif1Ot2 *ot, struct Part *part) {
    sceDmaChan *fromSPR;
    struct TriangleNormal *pk_base;
    struct TriangleNormal *pSprPacket;
    int pk_page;
    unsigned short *text_pos_indices;
    int text_pos_index;
    struct TextPosParam *text_pos_params;
    struct TextPosParam *text_pos_param;
    struct TextureParam *texture_params;
    struct TextureParam *texture_param;
    struct PartHeader *part_header;
    unsigned int *strip;
    unsigned int *strip_end;
    int strip_length;
    struct Node *n2;
    struct Node *n1;
    int i;
    unsigned int n_;
    unsigned int n;
    struct Node *n0;
    int adc;
    u_long128 *pPacket;
    union Q min;
    int z;

    fromSPR = sceDmaGetChan(8);
    pk_base = all_packet->normal;
    pk_page = 0;
    text_pos_indices = (unsigned short *)((char *)part + part->text_pos_indices_offset);
    text_pos_index = text_pos_indices[0];
    text_pos_params = model_common_work->text_pos_params;
    text_pos_param = &text_pos_params[text_pos_index];
    texture_params = (struct TextureParam *)((char *)part + part->texture_params_offset);
    texture_param = &texture_params[0];
    part_header = (struct PartHeader *)model3_junk.vi00;
    strip = (unsigned int *)((u_long128 *)part_header + part_header->strip_top_offset);
    strip_end = (unsigned int *)((u_long128 *)part_header + part_header->strip_end_offset);
    strip_length = strip_end - strip;
    n2 = NULL;
    n1 = NULL;
    pk_base[0].n_tex0.u64[0] = pk_base[1].n_tex0.u64[0] = text_pos_param->tex0;
    pk_base[0].n_clamp.u64[0] = pk_base[1].n_clamp.u64[0] = texture_param->clamp;
    for (i = 0; i < strip_length; i++) {
        n_ = strip[i];
        n = n_ & 0x3FF;
        n0 = (struct Node *)((u_long128 *)model3_junk.vi00 + n);
        adc = n_ & 0x8000;
        fugahoge++;
        if (!adc) {
            pPacket = CharacterOt_RequestPacket(sizeof(struct TriangleNormal) / 16);
            if (!pPacket) {
                break;
            }
            pk_page ^= 1;
            pSprPacket = &pk_base[pk_page];
            pSprPacket->n_stq2.u128 = n2->d3.u128;
            pSprPacket->n_rgba2.u128 = n2->d2.u128;
            pSprPacket->n_xyzf2.u128 = n2->d0.u128;
            pSprPacket->n_stq1.u128 = n1->d3.u128;
            pSprPacket->n_rgba1.u128 = n1->d2.u128;
            pSprPacket->n_xyzf1.u128 = n1->d0.u128;
            pSprPacket->n_stq0.u128 = n0->d3.u128;
            pSprPacket->n_rgba0.u128 = n0->d2.u128;
            pSprPacket->n_xyzf0.u128 = n0->d0.u128;
            min.u128 = pminw(pSprPacket->n_xyzf2.u128, pSprPacket->n_xyzf1.u128);
            min.u128 = pminw(min.u128, pSprPacket->n_xyzf0.u128);
            z = min.s32[2] >> 8;
            if (z < 0 || z > 0xFFF) {
                n_bad_depths++;
            } else {
                while (sceDmaSync(fromSPR, 0, 0)) {
                }
                ktVif1Ot2AppendFake(ot, z, pPacket, pSprPacket);
                fromSPR->sadr = pSprPacket;
                sceDmaSendN(fromSPR, pPacket, sizeof(struct TriangleNormal) / 16);
                hogehoge++;
            }
        }
        n2 = n1;
        n1 = n0;
    }
    while (sceDmaSync(fromSPR, 0, 0)) {
    }
}

static void SortTriangleNormalEnviron(struct ktVif1Ot2 *ot, struct Part *part) {
    sceDmaChan *fromSPR;
    struct TriangleNormalEnviron *pk_base;
    struct TriangleNormalEnviron *pk;
    int pk_page;
    unsigned short *text_pos_indices;
    int text_pos_index;
    struct TextPosParam *text_pos_params;
    struct TextPosParam *text_pos_param;
    struct TextureParam *texture_params;
    struct TextureParam *texture_param;
    struct PartHeader *part_header;
    unsigned int *strip;
    unsigned int *strip_end;
    int strip_length;
    struct Node *n2;
    struct Node *n1;
    int i;
    unsigned int n_;
    unsigned int n;
    struct Node *n0;
    int adc;
    u_long128 *pp;
    union Q min;
    int z;

    fromSPR = sceDmaGetChan(8);
    pk_base = all_packet->normal_environ;
    pk_page = 0;
    text_pos_indices = (unsigned short *)((char *)part + part->text_pos_indices_offset);
    text_pos_index = text_pos_indices[0];
    text_pos_params = model_common_work->text_pos_params;
    text_pos_param = &text_pos_params[text_pos_index];
    texture_params = (struct TextureParam *)((char *)part + part->texture_params_offset);
    texture_param = &texture_params[0];
    part_header = (struct PartHeader *)model3_junk.vi00;
    strip = (unsigned int *)((u_long128 *)part_header + part_header->strip_top_offset);
    strip_end = (unsigned int *)((u_long128 *)part_header + part_header->strip_end_offset);
    strip_length = strip_end - strip;
    n2 = NULL;
    n1 = NULL;
    pk_base[0].n_tex0.u64[0] = pk_base[1].n_tex0.u64[0] = text_pos_param->tex0;
    pk_base[0].n_clamp.u64[0] = pk_base[1].n_clamp.u64[0] = texture_param->clamp;
    pk_base[0].e_tex0.u64[0] = pk_base[1].e_tex0.u64[0] = model_common_work->latitude_mapping_tex0;
    pk_base[0].e_rgba.u64[0] = pk_base[1].e_rgba.u64[0] = GS_SET_RGBAQ(0x80, 0x80, 0x80, part->envmap_param, Float_Bits(1.0f));
    for (i = 0; i < strip_length; i++) {
        n_ = strip[i];
        n = n_ & 0x3FF;
        n0 = (struct Node *)((u_long128 *)model3_junk.vi00 + n);
        adc = n_ & 0x8000;
        if (!adc) {
            pp = CharacterOt_RequestPacket(sizeof(struct TriangleNormalEnviron) / 16);
            if (!pp) {
                break;
            }
            pk_page ^= 1;
            pk = &pk_base[pk_page];
            pk->n_stq2.u128 = n2->d3.u128;
            pk->n_rgba2.u128 = n2->d2.u128;
            pk->n_xyzf2.u128 = n2->d0.u128;
            pk->e_stq2.fv[0] = n2->d1.fv[3];
            pk->e_stq2.fv[1] = n2->d3.fv[3];
            pk->e_xyzf2.u128 = n2->d0.u128;
            pk->n_stq1.u128 = n1->d3.u128;
            pk->n_rgba1.u128 = n1->d2.u128;
            pk->n_xyzf1.u128 = n1->d0.u128;
            pk->e_stq1.fv[0] = n1->d1.fv[3];
            pk->e_stq1.fv[1] = n1->d3.fv[3];
            pk->e_xyzf1.u128 = n1->d0.u128;
            while (pk->e_stq1.fv[0] < pk->e_stq2.fv[0] - 0.5f) {
                pk->e_stq1.fv[0] += 1.0f;
            }
            while (pk->e_stq1.fv[0] > 0.5f + pk->e_stq2.fv[0]) {
                pk->e_stq1.fv[0] -= 1.0f;
            }
            pk->n_stq0.u128 = n0->d3.u128;
            pk->n_rgba0.u128 = n0->d2.u128;
            pk->n_xyzf0.u128 = n0->d0.u128;
            pk->e_stq0.fv[0] = n0->d1.fv[3];
            pk->e_stq0.fv[1] = n0->d3.fv[3];
            pk->e_xyzf0.u128 = n0->d0.u128;
            while (pk->e_stq0.fv[0] < pk->e_stq1.fv[0] - 0.5f) {
                pk->e_stq0.fv[0] += 1.0f;
            }
            while (pk->e_stq0.fv[0] > 0.5f + pk->e_stq1.fv[0]) {
                pk->e_stq0.fv[0] -= 1.0f;
            }
            min.u128 = pminw(pk->n_xyzf2.u128, pk->n_xyzf1.u128);
            min.u128 = pminw(min.u128, pk->n_xyzf0.u128);
            z = min.s32[2] >> 8;
            if (z < 0 || z > 0xFFF) {
                n_bad_depths++;
            } else {
                while (sceDmaSync(fromSPR, 0, 0)) {
                }
                ktVif1Ot2AppendFake(ot, z, pp, pk);
                fromSPR->sadr = pk;
                sceDmaSendN(fromSPR, pp, sizeof(struct TriangleNormalEnviron) / 16);
            }
        }
        n2 = n1;
        n1 = n0;
    }
    while (sceDmaSync(fromSPR, 0, 0)) {
    }
}

static void SortTriangleNormalSpecular(struct ktVif1Ot2 *ot, struct Part *part) {
    sceDmaChan *fromSPR;
    struct TriangleNormalSpecular *pk_base;
    struct TriangleNormalSpecular *pk;
    int pk_page;
    unsigned short *text_pos_indices;
    int text_pos_index;
    struct TextPosParam *text_pos_params;
    struct TextPosParam *text_pos_param;
    struct TextureParam *texture_params;
    struct TextureParam *texture_param;
    float *reflection_color;
    struct PartHeader *part_header;
    unsigned int *strip;
    unsigned int *strip_end;
    int strip_length;
    struct Node *n2;
    struct Node *n1;
    int i;
    unsigned int n_;
    unsigned int n;
    struct Node *n0;
    int adc;
    u_long128 *pp;
    union Q min;
    int z;

    fromSPR = sceDmaGetChan(8);
    pk_base = all_packet->normal_specular;
    pk_page = 0;
    text_pos_indices = (unsigned short *)((char *)part + part->text_pos_indices_offset);
    text_pos_index = text_pos_indices[0];
    text_pos_params = model_common_work->text_pos_params;
    text_pos_param = &text_pos_params[text_pos_index];
    texture_params = (struct TextureParam *)((char *)part + part->texture_params_offset);
    texture_param = &texture_params[0];
    reflection_color = LightReflectionColor();
    part_header = (struct PartHeader *)model3_junk.vi00;
    strip = (unsigned int *)((u_long128 *)part_header + part_header->strip_top_offset);
    strip_end = (unsigned int *)((u_long128 *)part_header + part_header->strip_end_offset);
    strip_length = strip_end - strip;
    n2 = NULL;
    n1 = NULL;
    pk_base[0].n_tex0.u64[0] = pk_base[1].n_tex0.u64[0] = text_pos_param->tex0;
    pk_base[0].n_clamp.u64[0] = pk_base[1].n_clamp.u64[0] = texture_param->clamp;
    pk_base[0].s_rgba.iv[0] = pk_base[1].s_rgba.iv[0] = part->specular[0] * reflection_color[0];
    pk_base[0].s_rgba.iv[1] = pk_base[1].s_rgba.iv[1] = part->specular[1] * reflection_color[1];
    pk_base[0].s_rgba.iv[2] = pk_base[1].s_rgba.iv[2] = part->specular[2] * reflection_color[2];
    pk_base[0].s_rgba.iv[3] = pk_base[1].s_rgba.iv[3] = 0x80;
    pk_base[0].S_fogcol.u32[0] = pk_base[1].S_fogcol.u32[0] = model3_junk.fogcol;
    for (i = 0; i < strip_length; i++) {
        n_ = strip[i];
        n = n_ & 0x3FF;
        n0 = (struct Node *)((u_long128 *)model3_junk.vi00 + n);
        adc = n_ & 0x8000;
        if (!adc) {
            pp = CharacterOt_RequestPacket(sizeof(struct TriangleNormalSpecular) / 16);
            if (!pp) {
                break;
            }
            pk_page ^= 1;
            pk = &pk_base[pk_page];
            pk->n_stq2.u128 = n2->d3.u128;
            pk->n_rgba2.u128 = n2->d2.u128;
            pk->n_xyzf2.u128 = n2->d0.u128;
            pk->s_stq2.u64[0] = n2->d1.u64[0];
            pk->s_xyzf2.u128 = n2->d0.u128;
            pk->n_stq1.u128 = n1->d3.u128;
            pk->n_rgba1.u128 = n1->d2.u128;
            pk->n_xyzf1.u128 = n1->d0.u128;
            pk->s_stq1.u64[0] = n1->d1.u64[0];
            pk->s_xyzf1.u128 = n1->d0.u128;
            pk->n_stq0.u128 = n0->d3.u128;
            pk->n_rgba0.u128 = n0->d2.u128;
            pk->n_xyzf0.u128 = n0->d0.u128;
            pk->s_stq0.u64[0] = n0->d1.u64[0];
            pk->s_xyzf0.u128 = n0->d0.u128;
            min.u128 = pminw(pk->n_xyzf2.u128, pk->n_xyzf1.u128);
            min.u128 = pminw(min.u128, pk->n_xyzf0.u128);
            z = min.s32[2] >> 8;
            if (z < 0 || z > 0xFFF) {
                n_bad_depths++;
            } else {
                while (sceDmaSync(fromSPR, 0, 0)) {
                }
                ktVif1Ot2AppendFake(ot, z, pp, pk);
                fromSPR->sadr = pk;
                sceDmaSendN(fromSPR, pp, sizeof(struct TriangleNormalSpecular) / 16);
            }
        }
        n2 = n1;
        n1 = n0;
    }
    while (sceDmaSync(fromSPR, 0, 0)) {
    }
}

static void SortTriangleNormalEnvironSpecular(struct ktVif1Ot2 *ot, struct Part *part) {
    sceDmaChan *fromSPR;
    struct TriangleNormalEnvironSpecular *pk_base;
    struct TriangleNormalEnvironSpecular *pk;
    int pk_page;
    unsigned short *text_pos_indices;
    int text_pos_index;
    struct TextPosParam *text_pos_params;
    struct TextPosParam *text_pos_param;
    struct TextureParam *texture_params;
    struct TextureParam *texture_param;
    float *reflection_color;
    struct PartHeader *part_header;
    unsigned int *strip;
    unsigned int *strip_end;
    int strip_length;
    struct Node *n2;
    struct Node *n1;
    int i;
    unsigned int n_;
    unsigned int n;
    struct Node *n0;
    int adc;
    u_long128 *pp;
    union Q min;
    int z;

    fromSPR = sceDmaGetChan(8);
    pk_base = all_packet->normal_environ_specular;
    pk_page = 0;
    text_pos_indices = (unsigned short *)((char *)part + part->text_pos_indices_offset);
    text_pos_index = text_pos_indices[0];
    text_pos_params = model_common_work->text_pos_params;
    text_pos_param = &text_pos_params[text_pos_index];
    texture_params = (struct TextureParam *)((char *)part + part->texture_params_offset);
    texture_param = &texture_params[0];
    reflection_color = LightReflectionColor();
    part_header = (struct PartHeader *)model3_junk.vi00;
    strip = (unsigned int *)((u_long128 *)part_header + part_header->strip_top_offset);
    strip_end = (unsigned int *)((u_long128 *)part_header + part_header->strip_end_offset);
    strip_length = strip_end - strip;
    n2 = NULL;
    n1 = NULL;
    pk_base[0].n_tex0.u64[0] = pk_base[1].n_tex0.u64[0] = text_pos_param->tex0;
    pk_base[0].n_clamp.u64[0] = pk_base[1].n_clamp.u64[0] = texture_param->clamp;
    pk_base[0].e_tex0.u64[0] = pk_base[1].e_tex0.u64[0] = model_common_work->latitude_mapping_tex0;
    pk_base[0].e_rgba.u64[0] = pk_base[1].e_rgba.u64[0] = GS_SET_RGBAQ(0x80, 0x80, 0x80, part->envmap_param, Float_Bits(1.0f));
    pk_base[0].s_rgba.iv[0] = pk_base[1].s_rgba.iv[0] = part->specular[0] * reflection_color[0];
    pk_base[0].s_rgba.iv[1] = pk_base[1].s_rgba.iv[1] = part->specular[1] * reflection_color[1];
    pk_base[0].s_rgba.iv[2] = pk_base[1].s_rgba.iv[2] = part->specular[2] * reflection_color[2];
    pk_base[0].s_rgba.iv[3] = pk_base[1].s_rgba.iv[3] = 0x80;
    pk_base[0].S_fogcol.u32[0] = pk_base[1].S_fogcol.u32[0] = model3_junk.fogcol;
    for (i = 0; i < strip_length; i++) {
        n_ = strip[i];
        n = n_ & 0x3FF;
        n0 = (struct Node *)((u_long128 *)model3_junk.vi00 + n);
        adc = n_ & 0x8000;
        if (!adc) {
            pp = CharacterOt_RequestPacket(sizeof(struct TriangleNormalEnvironSpecular) / 16);
            if (!pp) {
                break;
            }
            pk_page ^= 1;
            pk = &pk_base[pk_page];
            pk->n_stq2.u128 = n2->d3.u128;
            pk->n_rgba2.u128 = n2->d2.u128;
            pk->n_xyzf2.u128 = n2->d0.u128;
            pk->e_stq2.fv[0] = n2->d1.fv[3];
            pk->e_stq2.fv[1] = n2->d3.fv[3];
            pk->e_xyzf2.u128 = n2->d0.u128;
            pk->s_stq2.u64[0] = n2->d1.u64[0];
            pk->s_xyzf2.u128 = n2->d0.u128;
            pk->n_stq1.u128 = n1->d3.u128;
            pk->n_rgba1.u128 = n1->d2.u128;
            pk->n_xyzf1.u128 = n1->d0.u128;
            pk->e_stq1.fv[0] = n1->d1.fv[3];
            pk->e_stq1.fv[1] = n1->d3.fv[3];
            pk->e_xyzf1.u128 = n1->d0.u128;
            pk->s_stq1.u64[0] = n1->d1.u64[0];
            pk->s_xyzf1.u128 = n1->d0.u128;
            while (pk->e_stq1.fv[0] < pk->e_stq2.fv[0] - 0.5f) {
                pk->e_stq1.fv[0] += 1.0f;
            }
            while (pk->e_stq1.fv[0] > 0.5f + pk->e_stq2.fv[0]) {
                pk->e_stq1.fv[0] -= 1.0f;
            }
            pk->n_stq0.u128 = n0->d3.u128;
            pk->n_rgba0.u128 = n0->d2.u128;
            pk->n_xyzf0.u128 = n0->d0.u128;
            pk->e_stq0.fv[0] = n0->d1.fv[3];
            pk->e_stq0.fv[1] = n0->d3.fv[3];
            pk->e_xyzf0.u128 = n0->d0.u128;
            pk->s_stq0.u64[0] = n0->d1.u64[0];
            pk->s_xyzf0.u128 = n0->d0.u128;
            while (pk->e_stq0.fv[0] < pk->e_stq1.fv[0] - 0.5f) {
                pk->e_stq0.fv[0] += 1.0f;
            }
            while (pk->e_stq0.fv[0] > 0.5f + pk->e_stq1.fv[0]) {
                pk->e_stq0.fv[0] -= 1.0f;
            }
            min.u128 = pminw(pk->n_xyzf2.u128, pk->n_xyzf1.u128);
            min.u128 = pminw(min.u128, pk->n_xyzf0.u128);
            z = min.s32[2] >> 8;
            if (z < 0 || z > 0xFFF) {
                n_bad_depths++;
            } else {
                while (sceDmaSync(fromSPR, 0, 0)) {
                }
                ktVif1Ot2AppendFake(ot, z, pp, pk);
                fromSPR->sadr = pk;
                sceDmaSendN(fromSPR, pp, sizeof(struct TriangleNormalEnvironSpecular) / 16);
            }
        }
        n2 = n1;
        n1 = n0;
    }
    while (sceDmaSync(fromSPR, 0, 0)) {
    }
}

static void SortTriangleSpecularNormal(struct ktVif1Ot2 *ot, struct Part *part) {
    sceDmaChan *fromSPR;
    struct TriangleSpecularNormal *pk_base;
    struct TriangleSpecularNormal *pk;
    int pk_page;
    unsigned short *text_pos_indices;
    int text_pos_index;
    struct TextPosParam *text_pos_params;
    struct TextPosParam *text_pos_param;
    struct TextureParam *texture_params;
    struct TextureParam *texture_param;
    float *reflection_color;
    struct PartHeader *part_header;
    unsigned int *strip;
    unsigned int *strip_end;
    int strip_length;
    struct Node *n2;
    struct Node *n1;
    int i;
    unsigned int n_;
    unsigned int n;
    struct Node *n0;
    int adc;
    u_long128 *pp;
    union Q min;
    int z;

    fromSPR = sceDmaGetChan(8);
    pk_base = all_packet->specular_normal;
    pk_page = 0;
    text_pos_indices = (unsigned short *)((char *)part + part->text_pos_indices_offset);
    text_pos_index = text_pos_indices[0];
    text_pos_params = model_common_work->text_pos_params;
    text_pos_param = &text_pos_params[text_pos_index];
    texture_params = (struct TextureParam *)((char *)part + part->texture_params_offset);
    texture_param = &texture_params[0];
    reflection_color = LightReflectionColor();
    part_header = (struct PartHeader *)model3_junk.vi00;
    strip = (unsigned int *)((u_long128 *)part_header + part_header->strip_top_offset);
    strip_end = (unsigned int *)((u_long128 *)part_header + part_header->strip_end_offset);
    strip_length = strip_end - strip;
    n2 = NULL;
    n1 = NULL;
    pk_base[0].s_rgba.iv[0] = pk_base[1].s_rgba.iv[0] = part->specular[0] * reflection_color[0];
    pk_base[0].s_rgba.iv[1] = pk_base[1].s_rgba.iv[1] = part->specular[1] * reflection_color[1];
    pk_base[0].s_rgba.iv[2] = pk_base[1].s_rgba.iv[2] = part->specular[2] * reflection_color[2];
    pk_base[0].s_rgba.iv[3] = pk_base[1].s_rgba.iv[3] = 0x80;
    pk_base[0].S_fogcol.u32[0] = pk_base[1].S_fogcol.u32[0] = model3_junk.fogcol;
    pk_base[0].n_tex0.u64[0] = pk_base[1].n_tex0.u64[0] = text_pos_param->tex0;
    pk_base[0].n_clamp.u64[0] = pk_base[1].n_clamp.u64[0] = texture_param->clamp;
    for (i = 0; i < strip_length; i++) {
        n_ = strip[i];
        n = n_ & 0x3FF;
        n0 = (struct Node *)((u_long128 *)model3_junk.vi00 + n);
        adc = n_ & 0x8000;
        if (!adc) {
            pp = CharacterOt_RequestPacket(sizeof(struct TriangleSpecularNormal) / 16);
            if (!pp) {
                break;
            }
            pk_page ^= 1;
            pk = &pk_base[pk_page];
            pk->s_stq2.u64[0] = n2->d1.u64[0];
            pk->s_xyzf2.u128 = n2->d0.u128;
            pk->n_stq2.u128 = n2->d3.u128;
            pk->n_rgba2.u128 = n2->d2.u128;
            pk->n_xyzf2.u128 = n2->d0.u128;
            pk->s_stq1.u64[0] = n1->d1.u64[0];
            pk->s_xyzf1.u128 = n1->d0.u128;
            pk->n_stq1.u128 = n1->d3.u128;
            pk->n_rgba1.u128 = n1->d2.u128;
            pk->n_xyzf1.u128 = n1->d0.u128;
            pk->s_stq0.u64[0] = n0->d1.u64[0];
            pk->s_xyzf0.u128 = n0->d0.u128;
            pk->n_stq0.u128 = n0->d3.u128;
            pk->n_rgba0.u128 = n0->d2.u128;
            pk->n_xyzf0.u128 = n0->d0.u128;
            min.u128 = pminw(pk->n_xyzf2.u128, pk->n_xyzf1.u128);
            min.u128 = pminw(min.u128, pk->n_xyzf0.u128);
            z = min.s32[2] >> 8;
            if (z < 0 || z > 0xFFF) {
                n_bad_depths++;
            } else {
                while (sceDmaSync(fromSPR, 0, 0)) {
                }
                ktVif1Ot2AppendFake(ot, z, pp, pk);
                fromSPR->sadr = pk;
                sceDmaSendN(fromSPR, pp, sizeof(struct TriangleSpecularNormal) / 16);
            }
        }
        n2 = n1;
        n1 = n0;
    }
    while (sceDmaSync(fromSPR, 0, 0)) {
    }
}

static void (*sort_functions[8])(struct ktVif1Ot2 *, struct Part *) = {
    SortTriangleNormal,
    SortTriangleNormalEnviron,
    SortTriangleNormalSpecular,
    SortTriangleNormalEnvironSpecular,
    NULL,
    NULL,
    SortTriangleSpecularNormal,
    NULL,
};

static void DrawPart0(struct ktVif1Ot2 *ot, struct Part *part, struct ModelWork *work) {
    int func_no;
    sceDmaChan *toSPR;

    func_no = 0;
    if (!Model3WorkEquipmentFlag(work, part->equipment_id)) {
        /* Matching: a dead store; the original's line table skips 1714, between the if (1713) and the
         * return (1715), as a statement with no code does. Removing it changes the code. */
        func_no = 0;
        return;
    }
    model3_junk.vi00 = (void *)(draw_base + 0x70000000);
    if (part->envmap_param) {
        func_no |= 1;
    }
    if (part->shading_type == 4) {
        func_no |= 2;
        if (part->specular_pos) {
            func_no |= 4;
        }
    }
    if (sort_functions[func_no]) {
        toSPR = sceDmaGetChan(9);
        while (sceDmaSync(toSPR, 0, 0)) {
        }
        sort_functions[func_no](ot, part);
    }
    draw_base ^= 0x1000;
}

static void DrawParts0(struct ktVif1Ot2 *ot, struct Model *model, struct ModelWork *work) {
    int n_parts;
    struct Part *parts_top;
    struct Part *part;
    int i;
    struct Part *next;

    n_parts = model->n_vu0_parts;
    part = parts_top = (struct Part *)((char *)model + model->vu0_parts_offset);
    if (n_parts) {
        LoadProgram_Vu0();
        MakeData0();
        MakeCalcPartPacket(part);
        KickCalcPartPacket();
        PrepareSort();
        for (i = 1; i < n_parts; i++) {
            next = (struct Part *)((char *)part + part->size);
            MakeCalcPartPacket(next);
            TransferToSPR();
            KickCalcPartPacket();
            DrawPart0(ot, part, work);
            part = next;
        }
        TransferToSPR();
        DrawPart0(ot, part, work);
    }
}

/**
 * Draws the VU0 parts of a model into the character ordering table (VU0 transforms the next
 * part while the CPU sorts the current one).
 * @param model the struct Model
 * @param work  its work (equipment flags select the parts)
 */
void Model3DrawVu0Parts(struct Model *model, struct ModelWork *work) {
    struct ktVif1Ot2 *ot;

    ot = CharacterOt_KtVif1Ot2();
    n_bad_depths = 0;
    calc_base = 0;
    draw_base = 0;
    hogehoge = 0;
    fugahoge = 0;
    DrawParts0(ot, model, work);
    chr_flg ^= 1;
}
