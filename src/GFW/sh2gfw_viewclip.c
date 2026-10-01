/*
 * Background view clipping (GFW): each loaded block is divided into tiles; per frame, the tiles
 * inside the camera's view triangle (tested on VU0) are marked so only their packets are drawn.
 * Also the object clip matrix, per-block object display switches, and the character clip flag.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "libc/stdlib.h"
#include "sdk/libvu0.h"

extern u_long128 VU0_MIC_CAL_UTIL_DMAHEAD[];

#define SPR      0x70000000

/* Matching: the original's typedef was 16-byte aligned; a spilled pointer parameter of this type gets a
 * 16-byte stack slot. */
typedef struct sh2gfw_BLOCK_MAN sh2gfw_BLOCK_MAN_a16 __attribute__((aligned(16)));

static void sh2gfw_ClipDraw_BG(struct sh2gfw_BLOCK_MAN *pB_man, int slot, struct sh2gfw_ALLTEXSYNC_MAN *pATSM,
                               float (*bbox)[4], float (*view_triangle)[4], Q_WORDDATA **qwd_data);
static void sh2gfw_get_blockORIGIN(float (*bbox)[4], float *origin);
static int sh2gfw_Get_CamTilePos(float *origin);
static void sh2gfw_get_ViewRecTangle(float *origin, float (*view_triangle)[4], int *view_rect, int view_tile);
static void sh2gfw_get_viewTriangle(float (*view_triangle)[4]);
static void sh2gfw_init_vctagbuf(void *vc);
static void sh2gfw_make_tagclipdata(sceVu0FVECTOR origin, float (*view_triangle)[4], int *view_rect,
                                    sh2gfw_BLOCK_MAN_a16 *pBM);
static int sh2_ClipHitCheckSquare(float (*mat)[4], float *box, float *view_triangle);
static void kari_set_vu0cal(void);
static void sh2gfw_setVCTAG_DrawSys(struct sh2gfw_BLOCK_MAN *pB_man);

float inclip = 1.02f;
void *pGlobalMan;

/**
 * Per frame: marks the visible tiles of every loaded background block and chains its draw
 * packets.
 * @param qwd_tag DMA chain write pointer, advanced
 * @return 0
 */
int sh2gfw_viewclip_block(Q_WORDDATA **qwd_tag) {
    int id;
    int num;
    Q_WORDDATA *qwd;
    float view_triangle[3][4];
    float bbox[4][4];
    int slot;

    sh2gfw_Store_Perf2(*T0_COUNT, 14);
    qwd = *qwd_tag;
    kari_set_vu0cal();
    sh2gfw_get_viewTriangle(view_triangle);
    pGlobalMan = 0;
    sh2gfw_Clear_MicroChange();
    for (slot = 0; slot < 5; slot++) {
        if (b_man[slot].pB_H) {
            sceVu0CopyMatrix(bbox, (float (*)[4])(b_man[slot].p_Matrices + 4));
            sh2gfw_init_vctagbuf(b_man[slot].tileViewClipInfo);
            sh2gfw_ClipDraw_BG(&b_man[slot], slot, &AllTexSync_Man, bbox, view_triangle, &qwd);
        }
    }
    sh2gfw_Change_MicroFLGS();
    *qwd_tag = qwd;
    return 0;
}

static void sh2gfw_ClipDraw_BG(struct sh2gfw_BLOCK_MAN *pB_man, int slot, struct sh2gfw_ALLTEXSYNC_MAN *pATSM,
                               float (*bbox)[4], float (*view_triangle)[4], Q_WORDDATA **qwd_data) {
    Q_WORDDATA *qwd;
    Q_WORDDATA *qbuf;
    int j;
    int flg;
    float origin[4];
    int rect[4];
    int *mp;

    flg = 0;
    qwd = *qwd_data;
    qbuf = qwd;
    if (pB_man->blockid) {
        if (pB_man->pB_H->globaltexnum) {
            sh2gfw_Thr_d2TextureSend(pB_man->pTexMAN[0], 0x10, &pB_man->vif1mark[0], &pB_man->slotid[0]);
            pGlobalMan = pB_man->pTexMAN[0];
        } else if (pGlobalMan) {
            sh2gfw_Thr_d2TextureSend(pGlobalMan, 0, &pB_man->vif1mark[0], &pB_man->slotid[0]);
            flg++;
        }
        for (j = 1; j <= pB_man->texnum; j++) {
            sh2gfw_Thr_d2TextureSend(pB_man->pTexMAN[j], 0, &pB_man->vif1mark[j], &pB_man->slotid[j]);
        }
    } else {
        for (j = 1; j <= pB_man->texnum; j++) {
            sh2gfw_Thr_d2TextureSend(pB_man->pTexMAN[j], 0, &pB_man->vif1mark[j], &pB_man->slotid[j]);
        }
        if (pB_man->pB_H->globaltexnum) {
            sh2gfw_Thr_d2TextureSend(pB_man->pTexMAN[0], 0x10, &pB_man->vif1mark[0], &pB_man->slotid[0]);
        }
    }
    if (pB_man->pB_H->divflg) {
        sh2gfw_get_blockORIGIN(bbox, origin);
        pB_man->view_tile = sh2gfw_Get_CamTilePos(origin);
        sh2gfw_get_ViewRecTangle(origin, view_triangle, rect, pB_man->view_tile);
        sh2gfw_make_tagclipdata(origin, view_triangle, rect, pB_man);
        sh2gfw_setVCTAG_DrawSys(pB_man);
    } else {
        sh2gfw_setVCTAG_DrawSys(pB_man);
    }
    if (!DramaDemoNumber()) {
        mp = Get_NowMapId();
        if (((*mp & 0xF0000) >> 16) == 4) {
            sh2gfw_Rotate_CD11();
        }
    }
    sh2gfw_LightSet_ForBG(pB_man);
    sh2gfw_regist_BLOCKLIGHTS(pB_man, &qwd);
    sh2gfw_regist_BLOCKMATRICES(pB_man, &qwd);
    sh2gfw_setEND_chain(&qwd);
    d1cSend(qbuf);
    if (pB_man->blockid) {
        if (pB_man->pB_H->globaltexnum) {
            qbuf = qwd;
            sh2gfw_registDrawTag(&qwd, (unsigned int)pB_man->pBlockPack[0], pB_man->bp_leng[0] - 1,
                                 pB_man->vif1mark[0]);
            sh2gfw_Thr_d1d2SyncKick(qbuf, pB_man->vif1mark[0], pB_man->slotid[0]);
        } else if (flg) {
            qbuf = qwd;
            qbuf->ul128 = 0;
            qwd->ui32[0] = 0x70000000;
            qwd++;
            sh2gfw_Thr_d1d2SyncKick(qbuf, pB_man->vif1mark[0], pB_man->slotid[0]);
        }
        for (j = 1; j <= pB_man->texnum; j++) {
            qbuf = qwd;
            sh2gfw_registDrawTag(&qwd, (unsigned int)pB_man->pBlockPack[j], pB_man->bp_leng[j] - 1,
                                 pB_man->vif1mark[j]);
            sh2gfw_Thr_d1d2SyncKick(qbuf, pB_man->vif1mark[j], pB_man->slotid[j]);
        }
    } else {
        for (j = 1; j <= pB_man->texnum; j++) {
            qbuf = qwd;
            sh2gfw_registDrawTag(&qwd, (unsigned int)pB_man->pBlockPack[j], pB_man->bp_leng[j] - 1,
                                 pB_man->vif1mark[j]);
            sh2gfw_Thr_d1d2SyncKick(qbuf, pB_man->vif1mark[j], pB_man->slotid[j]);
        }
        if (pB_man->pB_H->globaltexnum) {
            qbuf = qwd;
            sh2gfw_registDrawTag(&qwd, (unsigned int)pB_man->pBlockPack[0], pB_man->bp_leng[0] - 1,
                                 pB_man->vif1mark[0]);
            sh2gfw_Thr_d1d2SyncKick(qbuf, pB_man->vif1mark[0], pB_man->slotid[0]);
        }
    }
    *qwd_data = qwd;
}

static void sh2gfw_get_blockORIGIN(float (*bbox)[4], float *origin) {
    int ix;
    int iz;
    int tmp[4];

    sceVu0ScaleVector(origin, origin, 0.0f);
    for (ix = 0; ix < 4; ix++) {
        sceVu0AddVector(origin, origin, bbox[ix]);
    }
    sceVu0ScaleVector(origin, origin, 0.25f);
    sceVu0FTOI0Vector(tmp, origin);
    ix = tmp[0] / 20000;
    iz = tmp[2] / 20000;
    if (tmp[0] < 0) {
        ix--;
    }
    if (tmp[2] < 0) {
        iz--;
    }
    origin[0] = 20000.0f * ix;
    origin[1] = 0.0f;
    origin[2] = 20000.0f * iz;
    origin[3] = 1.0f;
}

static int sh2gfw_Get_CamTilePos(float *origin) {
    int ssx;
    int ssz;
    float svt[4];

    vwGetViewPosition(svt);
    sceVu0SubVector(svt, svt, origin);
    ssz = svt[2] / 2500.0f;
    ssx = svt[0] / 2500.0f;
    return ssx + ssz * 8;
}

/** The tile rectangle covered by the view triangle, extended by one tile on each axis on the side
 *  nearer view_tile. */
static void sh2gfw_get_ViewRecTangle(float *origin, float (*view_triangle)[4], int *view_rect, int view_tile) {
    int i;
    int j;
    int tp;
    int cx;
    int cz;
    float tmpf[3][4];
    int tmpi[3][4];

    for (i = 0; i < 3; i++) {
        sceVu0SubVector(tmpf[i], view_triangle[i], origin);
        sceVu0ScaleVector(tmpf[i], tmpf[i], 0.0004f);
        sceVu0FTOI0Vector(tmpi[i], tmpf[i]);
        if (tmpi[i][0] < 0) {
            tmpi[i][0]--;
        }
        if (tmpi[i][2] < 0) {
            tmpi[i][2]--;
        }
    }
    for (i = 0; i < 2; i++) {
        for (j = i + 1; j < 3; j++) {
            if (tmpi[i][0] < tmpi[j][0]) {
                tp = tmpi[i][0];
                tmpi[i][0] = tmpi[j][0];
                tmpi[j][0] = tp;
            }
        }
    }
    for (i = 0; i < 2; i++) {
        for (j = i + 1; j < 3; j++) {
            if (tmpi[i][2] < tmpi[j][2]) {
                tp = tmpi[i][2];
                tmpi[i][2] = tmpi[j][2];
                tmpi[j][2] = tp;
            }
        }
    }
    cx = view_tile % 8;
    cz = view_tile / 8;
    i = abs(tmpi[0][0] - cx);
    j = abs(tmpi[2][0] - cx);
    if (i > j) {
        tmpi[2][0]--;
    } else {
        tmpi[0][0]++;
    }
    i = abs(tmpi[0][2] - cz);
    j = abs(tmpi[2][2] - cz);
    if (i > j) {
        tmpi[2][2]--;
    } else {
        tmpi[0][2]++;
    }
    view_rect[0] = tmpi[0][0];
    view_rect[1] = tmpi[2][0];
    view_rect[2] = tmpi[0][2];
    view_rect[3] = tmpi[2][2];
}

static void sh2gfw_get_viewTriangle(float (*view_triangle)[4]) {
    float work[4][4];
    int i;
    float wid;

    sceVu0UnitMatrix(work);
    sceVu0RotMatrixY(work, work, Env_ctl.camera_rot[1]);
    if (RoomName(0, 0.0f, 0.0f) == 3 && !DramaDemoNumber()) {
        wid = 512.0f * (Env_ctl.camera_parms2[2] - 2000.0f) / VbScreenInfo.scr_z;
    } else {
        wid = 512.0f * Env_ctl.camera_parms2[2] / VbScreenInfo.scr_z;
    }
    Env_ctl.camera_parms[2] = VbScreenInfo.scr_z;
    for (i = 0; i < 3; i++) {
        sceVu0ScaleVector(view_triangle[i], view_triangle[i], 0.0f);
    }
    view_triangle[1][0] = 0.5f * wid * inclip;
    view_triangle[1][2] = Env_ctl.camera_parms2[2];
    view_triangle[2][0] = -0.5f * wid * inclip;
    view_triangle[2][2] = Env_ctl.camera_parms2[2];
    for (i = 0; i < 3; i++) {
        sceVu0ApplyMatrix(view_triangle[i], work, view_triangle[i]);
        sceVu0AddVector(view_triangle[i], view_triangle[i], Env_ctl.camera_p);
        view_triangle[i][1] = 0.0f;
    }
}

static void sh2gfw_init_vctagbuf(void *vc) {
    Q_WORDDATA *VcBuf = vc;
    union Q_WORDDATA cleardata = {0x10101010, 0x10101010, 0x10101010, 0x10101010};

    VcBuf[0] = cleardata;
    VcBuf[1] = cleardata;
    VcBuf[2] = cleardata;
    VcBuf[3] = cleardata;
}

float y_dirvec[4] = {0.0f, 1.0f, 0.0f, 0.0f};
int ChrClip_FLG = 1;

/* Matching: local declaration order fitted to the original stack slots and registers; the DWARF's order
 * (stx, enx, stz, enz, ix, iz, index, rect, tagbuffer, mp, clip, ssx, ssz, svt) doesn't match
 * (docs/dwarf-fidelity.md). */
static void sh2gfw_make_tagclipdata(sceVu0FVECTOR origin, float (*view_triangle)[4], int *view_rect,
                                    sh2gfw_BLOCK_MAN_a16 *pBM) {
    int ssz;
    float rect[4][4];
    float svt[4];
    int ix;
    unsigned char *tagbuffer;
    int enx;
    int stx;
    int enz;
    int index;
    int ssx;
    int iz;
    int stz;
    int clip;
    int *mp;

    mp = Get_NowMapId();
    enx = view_rect[0];
    stx = view_rect[1];
    enz = view_rect[2];
    stz = view_rect[3];
    if (enx >= 8) {
        enx = 7;
    }
    if (stx < 0) {
        stx = 0;
    }
    if (enz >= 8) {
        enz = 7;
    }
    if (stz < 0) {
        stz = 0;
    }
    clip = stx * 2500;
    rect[0][0] = clip;
    rect[0][1] = 0.0f;
    rect[0][2] = stz * 2500;
    rect[0][3] = 0.0f;
    sceVu0AddVector(rect[0], rect[0], origin);
    sceVu0CopyVector(rect[1], rect[0]);
    rect[1][2] += 2500.0f;
    sceVu0CopyVector(rect[2], rect[0]);
    rect[2][0] += 2500.0f;
    sceVu0CopyVector(rect[3], rect[0]);
    rect[3][0] += 2500.0f;
    rect[3][2] += 2500.0f;
    tagbuffer = pBM->tileViewClipInfo[0];
    for (iz = stz; iz <= enz; iz++) {
        for (ix = stx; ix <= enx; ix++) {
            if (sh2_ClipHitCheckSquare(Env_ctl.objclip_mat, (float *)rect, (float *)view_triangle)) {
                index = ix + iz * 8;
                tagbuffer[index] = 0x20;
            }
            sceVu0CopyVector(rect[0], rect[2]);
            sceVu0CopyVector(rect[1], rect[3]);
            rect[2][0] += 2500.0f;
            rect[3][0] += 2500.0f;
        }
        rect[0][2] = rect[1][2];
        rect[2][2] = rect[3][2];
        rect[1][2] += 2500.0f;
        rect[3][2] += 2500.0f;
        rect[1][0] = rect[0][0] = origin[0] + clip;
        rect[3][0] = rect[2][0] = 2500.0f + rect[0][0];
    }
    vwGetViewPosition(svt);
    sceVu0SubVector(svt, svt, origin);
    ssz = svt[2] / 2500.0f;
    ssx = svt[0] / 2500.0f;
    pBM->view_tile = ssx + ssz * 8;
    for (iz = ssz - 1; iz <= ssz + 1; iz++) {
        if (iz >= 0 && iz < 8) {
            for (ix = ssx - 1; ix <= ssx + 1; ix++) {
                if (ix >= 0 && ix < 8) {
                    tagbuffer[ix + iz * 8] = 0x20;
                }
            }
        }
    }
}

/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
static int sh2_ClipHitCheckSquare(float (*mat)[4], float *box, float *view_triangle) {
    int ret;

    asm {
        .set noreorder
        lqc2         vf13, 0x0(mat)
        lqc2         vf14, 0x10(mat)
        lqc2         vf15, 0x20(mat)
        lqc2         vf16, 0x30(mat)
        lqc2         vf1, 0x0(box)
        lqc2         vf2, 0x10(box)
        lqc2         vf3, 0x20(box)
        lqc2         vf4, 0x30(box)
        lqc2         vf5, 0x0(view_triangle)
        lqc2         vf6, 0x10(view_triangle)
        lqc2         vf7, 0x20(view_triangle)
        vmulax.xyzw  ACC, vf13, vf1x
        vmadday.xyzw ACC, vf14, vf1y
        vmaddaz.xyzw ACC, vf15, vf1z
        vmaddw.xyzw  vf9, vf16, vf0w
        vmulax.xyzw  ACC, vf13, vf2x
        vmadday.xyzw ACC, vf14, vf2y
        vmaddaz.xyzw ACC, vf15, vf2z
        vmaddw.xyzw  vf10, vf16, vf0w
        .word        0x4BC949FF
        vmulax.xyzw  ACC, vf13, vf3x
        vmadday.xyzw ACC, vf14, vf3y
        vmaddaz.xyzw ACC, vf15, vf3z
        vmaddw.xyzw  vf11, vf16, vf0w
        cfc2.ni      t0, vi18
        andi         t0, t0, 0x3F
        beqz         t0, @1
        nop
        .word        0x4BCA51FF
        vmulax.xyzw  ACC, vf13, vf4x
        vmadday.xyzw ACC, vf14, vf4y
        vmaddaz.xyzw ACC, vf15, vf4z
        vmaddw.xyzw  vf12, vf16, vf0w
        cfc2.ni      t0, vi18
        andi         t0, t0, 0x3F
        beqz         t0, @1
        nop
        .word        0x4BCB59FF
        vmove.xyzw   vf20, vf5
        vmove.xyzw   vf22, vf1
        vmove.xyzw   vf21, vf6
        cfc2.ni      t0, vi18
        .word        0x4BCC61FF
        andi         t0, t0, 0x3F
        beqz         t0, @1
        nop
        vmove.xyzw   vf23, vf2
        cfc2.ni      t0, vi18
        andi         t0, t0, 0x3F
        beqz         t0, @1
        nop
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf21, vf7
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf20, vf6
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf20, vf5
        vmove.xyzw   vf22, vf1
        vmove.xyzw   vf21, vf6
        vmove.xyzw   vf23, vf3
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf21, vf7
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf20, vf6
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf20, vf5
        vmove.xyzw   vf22, vf4
        vmove.xyzw   vf21, vf6
        vmove.xyzw   vf23, vf2
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf21, vf7
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf20, vf6
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf20, vf5
        vmove.xyzw   vf22, vf4
        vmove.xyzw   vf21, vf6
        vmove.xyzw   vf23, vf3
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf21, vf7
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vmove.xyzw   vf20, vf6
        vcallms      0x0
        cfc2.i       t1, vi9
        bnez         t1, @2
        vnop
        and          t1, t1, zero
        b            @2
        vnop
    @1:
        ori          t1, zero, 0xF
    @2:
        or           ret, t1, t1
        .set reorder
    }
    return ret;
}

static void kari_set_vu0cal(void) {
    int i;

    i = 0;
    while (*D0_CHCR & 0x100) {
        i++;
    }
    *D0_TADR = (unsigned int)VU0_MIC_CAL_UTIL_DMAHEAD;
    *D0_QWC = 0;
    *D0_CHCR = 0x145;
    while (*D0_CHCR & 0x100) {
        i++;
    }
}

static void sh2gfw_setVCTAG_DrawSys(struct sh2gfw_BLOCK_MAN *pB_man) {
    Q_WORDDATA *qwd;
    unsigned char *tagbuffer;
    unsigned int igeomax;
    unsigned int i;
    unsigned int j;
    unsigned int k;
    unsigned int l;
    unsigned int igg;
    unsigned int ivumax;
    int mapdiv;

    mapdiv = pB_man->pB_H->divflg;
    tagbuffer = pB_man->tileViewClipInfo[0];
    for (i = (pB_man->pTexMAN[0] == 0) ? 1 : 0; i <= pB_man->texnum; i++) {
        igg = pB_man->gifnum[i];
        for (j = 0; j < igg; j++) {
            ivumax = pB_man->vunum[i][j];
            for (k = 0; k < ivumax; k++) {
                igeomax = pB_man->geom_amount[i][j][k];
                qwd = pB_man->pBlockPack[i] + pB_man->idVU_tag[i][j][k];
                if (qwd[igeomax + 1].uc8[3] & 1) {
                    for (l = igeomax - 1; qwd[l + 2].uc8[3] & 1; l--) {
                        if (pB_man->ObjCondition & (1 << qwd[l + 2].uc8[2])) {
                            qwd[l + 2].uc8[3] = 0x21;
                        } else {
                            qwd[l + 2].uc8[3] = 0x11;
                        }
                    }
                    igeomax = l + 1;
                }
                if (mapdiv) {
                    for (l = 0; l < igeomax; l++) {
                        qwd[l + 2].uc8[3] = tagbuffer[qwd[l + 2].uc8[2]];
                    }
                }
            }
        }
    }
}

/**
 * Builds Env_ctl.objclip_mat: a clip matrix for a level camera at the viewer's position
 * (height -1125) looking along the horizontal eye direction.
 */
void sh2gfw_set_objclip_matrix(void) {
    float gsx;
    float gsy;
    float NearZ;
    float FarZ;
    float tmp[4];
    float wvm[4][4];

    NearZ = VbScreenInfo.nearz;
    FarZ = VbScreenInfo.farz;
    /* Matching: dead; the DWARF has gsx (at 0x70(sp), as this store gives). clip_volume[0] is a guess. */
    gsx = clip_volume[0];
    gsy = clip_volume[2];
    sceVu0UnitMatrix(Env_ctl.objclip_mat);
    sceVu0UnitMatrix(wvm);
    sh2gde_Get_EyeDir(tmp);
    tmp[1] = 0.0f;
    _shNormalize(wvm[2], tmp);
    _shOuterProduct(wvm[0], wvm[2], y_dirvec);
    vcopy_gcc(wvm[1], y_dirvec);
    vwGetViewPosition(tmp);
    tmp[1] = -1125.0f;
    sceVu0TransMatrix(wvm, wvm, tmp);
    sceVu0InversMatrix(wvm, wvm);
    Env_ctl.objclip_mat[0][0] = 2.0f * NearZ / (gsy + gsy);
    Env_ctl.objclip_mat[1][1] = Env_ctl.objclip_mat[0][0];
    Env_ctl.objclip_mat[2][2] = (FarZ + NearZ) / (FarZ - NearZ);
    Env_ctl.objclip_mat[3][2] = -2.0f * (FarZ * NearZ) / (FarZ - NearZ);
    Env_ctl.objclip_mat[2][3] = 1.0f;
    Env_ctl.objclip_mat[3][3] = 0.0f;
    sceVu0MulMatrix(Env_ctl.objclip_mat, Env_ctl.objclip_mat, wvm);
}

/**
 * Sets the object display flag of the loaded block mapid.
 * @return its slot, or -1 if it isn't loaded
 */
int sh2gfw_Set_DispOnOffObj(int mapid, int dispflg) {
    struct sh2gfw_BLOCK_MAN *pB_man;
    int slot;

    for (slot = 0; slot < 5; slot++) {
        if (b_man[slot].pB_H) {
            pB_man = &b_man[slot];
            if (pB_man->pB_H->block_id == mapid) {
                pB_man->ObjCondition = dispflg;
                return slot;
            }
        }
    }
    return -1;
}

/** Builds the block-id-to-slot lookup (in scratchpad memory) used by sh2gfw_FastSet_DispOnOffObj. */
void sh2gfw_Init_DispOnOffObj(void) {
    int i;
    int slot;
    int bid;
    u_long128 *ul;
    char *ch;
    struct sh2gfw_BLOCK_MAN *pB_man;

    ul = (u_long128 *)SPR;
    for (i = 0; i < 16; i++) {
        ul[i] = 0;
    }
    for (slot = 0; slot < 5; slot++) {
        if (b_man[slot].pB_H) {
            pB_man = &b_man[slot];
            bid = (unsigned char)pB_man->pB_H->block_id;
            ch = (char *)SPR;
            ch[bid] = slot + 1;
        }
    }
}

/**
 * sh2gfw_Set_DispOnOffObj through the scratchpad lookup (low byte of mapid).
 * @return slot + 1, or -1 if the block isn't loaded
 */
int sh2gfw_FastSet_DispOnOffObj(int mapid, int dispflg) {
    int index;
    struct sh2gfw_BLOCK_MAN *pB_man;
    char *ch;

    pB_man = b_man;
    ch = (char *)SPR;
    index = ch[mapid & 0xFF];
    if (index) {
        mapid = index - 1;
        pB_man[mapid].ObjCondition = dispflg;
        return index;
    }
    return -1;
}

/** Returns whether characters are view-clipped in the current map. */
int sh2gfw_Get_ChrClip_FLG(void) {
    return ChrClip_FLG;
}

/** Turns character clipping off in a few maps and back on elsewhere (maps 0x10013-0x10014 keep the old setting). */
void sh2gfw_Check_ChrClip_FLG(int mp) {
    int bid;

    /* Matching: dead; the DWARF has bid (unused, register untracked). Its value (the block id, as in
     * sh2gfw_FastSet_DispOnOffObj) is a guess. */
    bid = mp & 0xFF;
    if (mp >= 0xC0029 && mp < 0xC002D) {
        ChrClip_FLG = 0;
    } else if (mp >= 0xA00BE && mp < 0xA00C2) {
        ChrClip_FLG = 0;
    } else if (mp >= 0xB00BD && mp < 0xB00C1) {
        ChrClip_FLG = 0;
    } else if (mp < 0x10013 || mp > 0x10014) {
        ChrClip_FLG = 1;
    }
}

/** Returns whether a character may be view-clipped (not for some character kinds, nor in maps 0x10011-0x10016). */
int sh2gfw_Check_ClipOKChar(void *sp) {
    struct SubCharacter *scp;
    int *mp;

    scp = sp;
    switch (scp->kind) {
    case 0x10B:
    case 0x312:
    case 0x310:
    case 0x311:
    case 0x30F:
    case 0x313:
    case 0x30C:
    case 0x301:
    case 0x30B:
    case 0x517:
        return 0;
    }
    mp = Get_NowMapId();
    if (*mp >= 0x10011 && *mp < 0x10017) {
        return 0;
    }
    return 1;
}
