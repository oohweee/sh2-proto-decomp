/*
 * Graphics debug helpers (GFW test code, "kari": provisional): uploads the debug font and noise
 * CLUTs to the GS, builds a normal-light matrix, and prints the lighting/fog/filter debug
 * overlay selected by Env_ctl.stat_ctl_2.
 */
#include "sh2.h"
#include "libc/stdio.h"
#include "sdk/eekernel.h"
#include "sdk/libgraph.h"
#include "sdk/libvu0.h"

union Q_WORDDATA noise_clut[4] = {
    {0x10181818, 0x10000000, 0x10181818, 0x10000000},
    {0x10000000, 0x10181818, 0x10181818, 0x10000000},
    {0x10000000, 0x10181818, 0x10000000, 0x10181818},
    {0x10181818, 0x10000000, 0x10000000, 0x10181818},
};

/** Uploads the noise CLUTs and the 8x8 debug font texture and CLUT to GS memory over DMA channel 1. */
void DBG_data_loadGS(void) {
    int id;
    union Q_WORDDATA qwd[48];

    id = 0;
    qwd[id].ui32[0] = 0x10000006;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000006;
    qwd[id + 1].ui32[3] = 0;
    qwd[id + 1].ui32[2] = 0xE;
    qwd[id + 1].ui32[1] = 0x10000000;
    qwd[id + 1].ui32[0] = 0x8004;
    qwd[id + 2].ul64[1] = 0x50;
    qwd[id + 2].ul64[0] = 0x137C000010000;
    qwd[id + 3].ul64[1] = 0x51;
    qwd[id + 3].ul64[0] = 0;
    qwd[id + 4].ul64[1] = 0x52;
    qwd[id + 4].ui32[0] = 16;
    qwd[id + 4].ui32[1] = 16;
    qwd[id + 5].ul64[1] = 0x53;
    qwd[id + 5].ul64[0] = 0;
    qwd[id + 6].ui32[3] = 0;
    qwd[id + 6].ui32[2] = 0;
    qwd[id + 6].ui32[1] = 0x8000000;
    qwd[id + 6].ui32[0] = 0x8040;
    qwd[id + 7].ui32[0] = 0x30000040;
    qwd[id + 7].ui32[1] = (unsigned int)noise_clut & 0x7FFFFFFF;
    qwd[id + 7].ui32[2] = 0;
    qwd[id + 7].ui32[3] = 0;
    qwd[id + 7].ui32[3] = 0x50000040;

    qwd[id + 8].ui32[0] = 0x10000006;
    qwd[id + 8].ui32[1] = 0;
    qwd[id + 8].ui32[2] = 0;
    qwd[id + 8].ui32[3] = 0x50000006;
    qwd[id + 9].ui32[3] = 0;
    qwd[id + 9].ui32[2] = 0xE;
    qwd[id + 9].ui32[1] = 0x10000000;
    qwd[id + 9].ui32[0] = 0x8004;
    qwd[id + 10].ul64[1] = 0x50;
    qwd[id + 10].ul64[0] = 0x137C400010000;
    qwd[id + 11].ul64[1] = 0x51;
    qwd[id + 11].ul64[0] = 0;
    qwd[id + 12].ul64[1] = 0x52;
    qwd[id + 12].ui32[0] = 8;
    qwd[id + 12].ui32[1] = 2;
    qwd[id + 13].ul64[1] = 0x53;
    qwd[id + 13].ul64[0] = 0;
    qwd[id + 14].ui32[3] = 0;
    qwd[id + 14].ui32[2] = 0;
    qwd[id + 14].ui32[1] = 0x8000000;
    qwd[id + 14].ui32[0] = 0x8004;
    qwd[id + 15].ui32[0] = 0x30000004;
    qwd[id + 15].ui32[1] = (unsigned int)noise_clut & 0x7FFFFFFF;
    qwd[id + 15].ui32[2] = 0;
    qwd[id + 15].ui32[3] = 0;
    qwd[id + 15].ui32[3] = 0x50000004;

    qwd[id + 16].ui32[0] = 0x10000006;
    qwd[id + 16].ui32[1] = 0;
    qwd[id + 16].ui32[2] = 0;
    qwd[id + 16].ui32[3] = 0x50000006;
    qwd[id + 17].ui32[3] = 0;
    qwd[id + 17].ui32[2] = 0xE;
    qwd[id + 17].ui32[1] = 0x10000000;
    qwd[id + 17].ui32[0] = 0x8004;
    qwd[id + 18].ul64[1] = 0x50;
    qwd[id + 18].ul64[0] = 0x137CC00010000;
    qwd[id + 19].ul64[1] = 0x51;
    qwd[id + 19].ul64[0] = 0;
    qwd[id + 20].ul64[1] = 0x52;
    qwd[id + 20].ui32[0] = 8;
    qwd[id + 20].ui32[1] = 2;
    qwd[id + 21].ul64[1] = 0x53;
    qwd[id + 21].ul64[0] = 0;
    qwd[id + 22].ui32[3] = 0;
    qwd[id + 22].ui32[2] = 0;
    qwd[id + 22].ui32[1] = 0x8000000;
    qwd[id + 22].ui32[0] = 0x8004;
    qwd[id + 23].ui32[0] = 0x30000004;
    qwd[id + 23].ui32[1] = (unsigned int)shDBG_font8_clut & 0x7FFFFFFF;
    qwd[id + 23].ui32[2] = 0;
    qwd[id + 23].ui32[3] = 0;
    qwd[id + 23].ui32[3] = 0x50000004;

    qwd[id + 24].ui32[0] = 0x10000006;
    qwd[id + 24].ui32[1] = 0;
    qwd[id + 24].ui32[2] = 0;
    qwd[id + 24].ui32[3] = 0x50000006;
    qwd[id + 25].ui32[3] = 0;
    qwd[id + 25].ui32[2] = 0xE;
    qwd[id + 25].ui32[1] = 0x10000000;
    qwd[id + 25].ui32[0] = 0x8004;
    qwd[id + 26].ul64[1] = 0x50;
    qwd[id + 26].ul64[0] = 0x140237D014020000;
    qwd[id + 27].ul64[1] = 0x51;
    qwd[id + 27].ul64[0] = 0;
    qwd[id + 28].ul64[1] = 0x52;
    qwd[id + 28].ui32[0] = 0x80;
    qwd[id + 28].ui32[1] = 0x40;
    qwd[id + 29].ul64[1] = 0x53;
    qwd[id + 29].ul64[0] = 0;
    qwd[id + 30].ui32[3] = 0;
    qwd[id + 30].ui32[2] = 0;
    qwd[id + 30].ui32[1] = 0x8000000;
    qwd[id + 30].ui32[0] = 0x8100;
    qwd[id + 31].ui32[0] = 0x30000100;
    qwd[id + 31].ui32[1] = (unsigned int)shDBG_font8_tex & 0x7FFFFFFF;
    qwd[id + 31].ui32[2] = 0;
    qwd[id + 31].ui32[3] = 0;
    qwd[id + 31].ui32[3] = 0x50000100;
    qwd[id + 32].ul128 = 0;
    qwd[id + 32].ui32[0] = 0x70000000;

    DisableDmac(1);
    FlushCache(0);
    sceGsSyncPath(0, 0);
    *D1_QWC = 0;
    *D1_TADR = (unsigned int)qwd;
    *D1_CHCR = 0x145;
    sceGsSyncPath(0, 0);
    *D_STAT = 2;
    EnableDmac(1);
}

/**
 * Builds a normal-light matrix for three lights plus the eye direction: the four normalized
 * vectors as rows, then transposed.
 * @param m      result
 * @param l0     light 0 direction
 * @param l1     light 1 direction
 * @param l2     light 2 direction
 * @param eyedir eye direction
 */
void sh2gfw_Vu0NormalLightPhongMatrix(float (*m)[4], float *l0, float *l1, float *l2, float *eyedir) {
    sceVu0Normalize(m[0], l0);
    sceVu0Normalize(m[1], l1);
    sceVu0Normalize(m[2], l2);
    sceVu0Normalize(m[3], eyedir);
    sceVu0TransposeMatrix(m, m);
}

/* Matching: fitted stand-in for double-precision code (docs/stand-ins.md; STRIPPED_DOUBLE_CODE in common.h). */
STRIPPED_DOUBLE_CODE()

/**
 * Prints the debug overlay (view clip tiles, spot/fog/filter/clear/noise/light parameters),
 * picked by Env_ctl.stat_ctl_1/stat_ctl_2, when a debug pad port (2 or 3) is in use.
 * @param res  not used
 * @param res2 not used
 */
void kari_DBG_print_junbi(unsigned long *res, unsigned long *res2) {
    char string_buf[128];
    float svt[4];
    unsigned int iy;
    int padport;
    int kx;
    int ky;
    int ix[8];
    int iyy;
    int iz;
    int ih;

    iy = 0x38;
    padport = shPadGetPort();
    if (Env_ctl.stat_ctl_2.uc8[0] - 4 == 0) {
        return;
    }
    if (padport != 2 && padport != 3) {
        sprintf(string_buf, "pad_x=%d lines=%d ", padport, *res);
        shDBG_print_string(string_buf, 8, 0x28);
        return;
    }
    sh2gde_Get_EyeDir(svt);
    sprintf(string_buf, "padx=%d %d %f %f %f", padport, *res, svt[0], svt[1], svt[2]);
    shDBG_print_string(string_buf, 8, 0x28);
    sprintf(string_buf, "X=%f Y=%f Z=%f", Env_ctl.camera_p[0], Env_ctl.camera_p[1], Env_ctl.camera_p[2]);
    shDBG_print_string(string_buf, 8, 0x30);
    vwGetViewPosition(svt);

    switch (Env_ctl.stat_ctl_1.uc8[0]) {
    case 1:
        sprintf(string_buf, "X=%f Y=%f Z=%f", Env_ctl.camera_parms[0], Env_ctl.camera_parms[3], Env_ctl.camera_parms[2]);
        shDBG_print_string(string_buf, 8, 0x38);
        iy += 8;
        for (ky = 0; ky < 2; ky++) {
            for (kx = 0; kx < 2; kx++) {
                unsigned char *sbb[3] = {(unsigned char *)"@", (unsigned char *)"-", (unsigned char *)"O"};
                iyy = iy + 0x48;
                for (iz = 0; iz < 8; iz++) {
                    for (ih = 0; ih < 8; ih++) {
                        ix[ih] = b_man[ky * 2 + kx].tileViewClipInfo[iz][ih] >> 4;
                        if (ih + iz * 8 == b_man[ky * 2 + kx].view_tile) {
                            ix[ih] = 0;
                        }
                    }
                    sprintf(string_buf, "%1s%1s%1s%1s%1s%1s%1s%1s", sbb[ix[0]], sbb[ix[1]], sbb[ix[2]], sbb[ix[3]],
                            sbb[ix[4]], sbb[ix[5]], sbb[ix[6]], sbb[ix[7]]);
                    shDBG_print_string(string_buf, kx * 80 + 8, iyy + 80 - ky * 160);
                    iyy -= 8;
                }
            }
            iy += 0x50;
        }
        break;
    case 5:
        break;
    case 6:
        sprintf(string_buf, "%d %f %f %f", Env_ctl.stat_ctl_1.uc8[0], Env_ctl.camera_parms2[2], Env_ctl.camera_parms2[0], Env_ctl.camera_parms2[1]);
        shDBG_print_string(string_buf, 8, 0x38);
        iy += 8;
        break;
    }

    {
        int k1;
        struct LP *lp;

        k1 = Env_ctl.stat_ctl_2.uc8[0];
        switch (k1) {
        case 0:
            sprintf(string_buf, "%d %7.2f %7.2f %f %7.3f %7.3f", k1, Env_ctl.SpotL0.decayparm.fl32[0],
                    Env_ctl.SpotL0.decayparm.fl32[2], Env_ctl.SpotL0.decayparm.fl32[3], Env_ctl.SpotL0.color.fl32[2],
                    Env_ctl.SpotL0.color.fl32[3]);
            shDBG_print_string(string_buf, 8, iy);
            break;
        case 1:
            sprintf(string_buf, "%d %f %f %f %f", k1, Env_ctl.fogparm.fl32[0], Env_ctl.fogparm.fl32[1],
                    Env_ctl.fogparm.fl32[2], Env_ctl.fogparm.fl32[3]);
            shDBG_print_string(string_buf, 8, iy);
            break;
        case 2:
            sprintf(string_buf, "CPF m%d %02X %02X %02X %02X %2d %2d", Env_ctl.CopyFilterColor.si32[1],
                    Env_ctl.CopyFilterColor.uc8[0], Env_ctl.CopyFilterColor.uc8[1], Env_ctl.CopyFilterColor.uc8[2],
                    Env_ctl.CopyFilterColor.uc8[3], Env_ctl.CopyFilterColor.sc8[6], Env_ctl.CopyFilterColor.sc8[7]);
            shDBG_print_string(string_buf, 8, iy);
            break;
        case 3:
            sprintf(string_buf, "%d %f %f %d %d", k1, Env_ctl.ambient[0], Env_ctl.ambient[3], Env_ctl.fogcolor.uc8[0],
                    Env_ctl.clearcolor.uc8[0]);
            shDBG_print_string(string_buf, 8, iy);
            break;
        case 5:
            sprintf(string_buf, "NS %d %d %d %d %d", Env_ctl.NoiseCondition.uc8[0], Env_ctl.NoiseCondition.uc8[1],
                    Env_ctl.NoiseCondition.uc8[2], Env_ctl.NoiseCondition.uc8[3], Env_ctl.NoiseCondition.uc8[5]);
            shDBG_print_string(string_buf, 8, iy);
            break;
        case 6:
            k1 = Env_ctl.stat_ctl_2.uc8[6];
            lp = (struct LP *)((float (*)[4])Env_ctl.p_light0 + k1);
            sprintf(string_buf, "P%d %f %f %f", k1, lp->pldir[0], lp->pldir[1], lp->pldir[2], lp->col[0]);
            shDBG_print_string(string_buf, 8, iy);
            sprintf(string_buf, "C%d %f %f %f", k1, lp->col[0], lp->col[1], lp->col[2]);
            shDBG_print_string(string_buf, 8, iy + 8);
            break;
        case 7: {
            struct DrawEnvData *ded;
            struct PointLightData *pld;
            struct SpotLightData *sld;
            int realsnum;
            int snum;

            ded = Get_NowDrawEnvData();
            snum = ded->FakeSpotNum;
            realsnum = ded->spotLNum;
            if (realsnum > 0) {
                snum += realsnum;
            } else {
                realsnum = 0;
            }
            if (snum > 0) {
                if (Env_ctl.stat_ctl_2.uc8[6] > snum) {
                    Env_ctl.stat_ctl_2.uc8[6] = 0;
                }
                sld = (struct SpotLightData *)ded->Ld_0;
                pld = (struct PointLightData *)sld + ded->pointLNum;
                sld = (struct SpotLightData *)pld;
                if (Env_ctl.stat_ctl_2.uc8[6] >= ded->spotLNum) {
                    sld += realsnum;
                    sld = (struct SpotLightData *)((struct PointLightData *)sld + ded->FakePointNum);
                    sld += Env_ctl.stat_ctl_2.uc8[6] - realsnum;
                } else {
                    sld += Env_ctl.stat_ctl_2.uc8[6];
                }
                sprintf(string_buf, "SL%d %f %f %f", Env_ctl.stat_ctl_2.uc8[6], sld->Col[0], sld->Col[1], sld->Col[2]);
                shDBG_print_string(string_buf, 8, iy);
                sprintf(string_buf, "SL%d FallStart = %f %f ", Env_ctl.stat_ctl_2.uc8[6], sld->DecayParm[2],
                        0.02f * sld->DecayParm[2]);
                shDBG_print_string(string_buf, 8, iy + 8);
                sprintf(string_buf, "SL%d FallEnd = %f %f ", Env_ctl.stat_ctl_2.uc8[6], sld->DecayParm[3],
                        0.02f * sld->DecayParm[3]);
                shDBG_print_string(string_buf, 8, iy + 0x10);
                sprintf(string_buf, "SL%d SpreadEnd = (cos)%f ", Env_ctl.stat_ctl_2.uc8[6], sld->DecayParm[0]);
                shDBG_print_string(string_buf, 8, iy + 0x18);
                sprintf(string_buf, "Pos = %f %f %f", sld->Pos[0], sld->Pos[1], sld->Pos[2]);
                shDBG_print_string(string_buf, 8, iy + 0x20);
                sprintf(string_buf, "Dir = %f %f %f", sld->Dir[0], sld->Dir[1], sld->Dir[2]);
                shDBG_print_string(string_buf, 8, iy + 0x28);
            }
            break;
        }
        case 8:
            sprintf(string_buf, "BC %f %f %f", Env_ctl.BaseVertexColor[0], Env_ctl.BaseVertexColor[1],
                    Env_ctl.BaseVertexColor[2]);
            shDBG_print_string(string_buf, 8, iy + 2);
            sprintf(string_buf, "BA %f %f %f", Env_ctl.BaseAmbient[0], Env_ctl.BaseAmbient[1], Env_ctl.BaseAmbient[2]);
            shDBG_print_string(string_buf, 8, iy + 0xC);
            sprintf(string_buf, "VA %f %f %f", Env_ctl.VertexAmbient[0], Env_ctl.VertexAmbient[1],
                    Env_ctl.VertexAmbient[2]);
            shDBG_print_string(string_buf, 8, iy + 0x16);
            sprintf(string_buf, "AM %f %f %f", Env_ctl.ambient[0], Env_ctl.ambient[1], Env_ctl.ambient[2]);
            shDBG_print_string(string_buf, 8, iy + 0x20);
            break;
        case 9:
            sprintf(string_buf, "FOG %d %d %d %d", Env_ctl.MoveFogColor.si32[0], Env_ctl.MoveFogColor.si32[1],
                    Env_ctl.MoveFogColor.si32[2], Env_ctl.MoveFogColor.si32[3]);
            shDBG_print_string(string_buf, 8, iy);
            break;
        case 10: {
            struct FilterParams *pfp;

            sprintf(string_buf, "Filter %d", sh2gfw_Get_FilterCommand());
            shDBG_print_string(string_buf, 8, iy);
            pfp = sh2gfw_Get_FilterCommandParams();
            switch (sh2gfw_Get_FilterCommand()) {
            case 5:
                sprintf(string_buf, "SF I=%d A=%d C=%d Shift=%d", pfp->SoftIter, pfp->SoftAref, pfp->SoftCit,
                        pfp->SoftShift);
                shDBG_print_string(string_buf, 0x58, iy);
                break;
            case 8:
            case 9:
            case 10:
                sprintf(string_buf, "DF Z = %d  KeyA=%d  TAA=%d", pfp->DOF_ZDepth, pfp->KeyAlpha, pfp->TrimAlpha);
                shDBG_print_string(string_buf, 0x58, iy);
                sprintf(string_buf, "S1IT=%d S1A=%d S1sh=%d S1ba=%d", pfp->S1_iter, pfp->S1_alpha, pfp->S1_shift,
                        pfp->S1_baseIX);
                shDBG_print_string(string_buf, 0x58, iy + 0xA);
                sprintf(string_buf, "S2IT=%d S2A=%d S2sh=%d S2ba=%d", pfp->S2_iter, pfp->S2_alpha, pfp->S2_shift,
                        pfp->S2_baseIX);
                shDBG_print_string(string_buf, 0x58, iy + 0x14);
                break;
            case 6:
            case 7:
                sprintf(string_buf, "SGD I=%d A=%d C=%d", pfp->SoftIter, pfp->SoftAref, pfp->SoftCit);
                shDBG_print_string(string_buf, 0x58, iy);
                sprintf(string_buf, "SGD Ix=%d Iy=%d Shift=%d", pfp->base_Ix, pfp->base_Ix, pfp->SoftShift);
                shDBG_print_string(string_buf, 0x58, iy + 8);
                break;
            case 0:
            case 1:
                sprintf(string_buf, "%d %d", pfp->base_Ix - 4, pfp->base_Iy - 4);
                shDBG_print_string(string_buf, 0x58, iy);
                sprintf(string_buf, "%d %d %d %d", pfp->TexTrimSX, pfp->TexTrimSY, pfp->TexTrimEX, pfp->TexTrimEY);
                shDBG_print_string(string_buf, 0x58, iy + 8);
                break;
            case 2:
                sprintf(string_buf, "%d %d BR=%d", pfp->base_Ix - 4, pfp->base_Iy - 4, pfp->blurRatio);
                shDBG_print_string(string_buf, 0x58, iy);
                break;
            default:
                sprintf(string_buf, "%d %d BR=%d LA=%d GA=%d T=%d", pfp->base_Ix - 4, pfp->base_Iy - 4, pfp->blurRatio,
                        pfp->LesserA + 0x7F, pfp->GreaterA + 0xE0, pfp->FO_timer);
                shDBG_print_string(string_buf, 0x58, iy);
                break;
            }
            break;
        }
        case 11: {
            struct DrawEnvData *ded;
            struct PointLightData *pld;
            struct SpotLightData *sld;

            ded = Get_NowDrawEnvData();
            if (ded->pointLNum) {
                sld = (struct SpotLightData *)ded->Ld_0;
                if (ded->spotLNum > 0) {
                    sld += ded->spotLNum;
                }
                pld = (struct PointLightData *)sld + Env_ctl.stat_ctl_2.uc8[5];
                sld = (struct SpotLightData *)pld;
                sprintf(string_buf, "PL%d %f %f %f", Env_ctl.stat_ctl_2.uc8[5], sld->Col[0], sld->Col[1], sld->Col[2]);
                shDBG_print_string(string_buf, 8, iy);
                sprintf(string_buf, "PL%d FallStart = %f %f ", Env_ctl.stat_ctl_2.uc8[5], sld->DecayParm[0],
                        0.02f * sld->DecayParm[0]);
                shDBG_print_string(string_buf, 8, iy + 8);
                sprintf(string_buf, "PL%d FallEnd = %f %f ", Env_ctl.stat_ctl_2.uc8[5], sld->DecayParm[1],
                        0.02f * sld->DecayParm[1]);
                shDBG_print_string(string_buf, 8, iy + 0x10);
                sprintf(string_buf, "Pos = %f %f %f", sld->Pos[0], sld->Pos[1], sld->Pos[2]);
                shDBG_print_string(string_buf, 8, iy + 0x18);
            }
            break;
        }
        }
    }
}
