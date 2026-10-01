/*
 * Full-screen 2D filters (GFW): the post-process pass on the finished frame. Copies the frame
 * buffer to a work buffer and draws it back through a filter chosen by a filter command
 * (blur, glow, dark blur, soft focus, noise), plus fades in/out to black, white or red, and a
 * paused ("retained") frame. The commands are set by the sh2gfw_Set_* functions and applied
 * by Make_Filter_Packet.
 * The packet builders write qwd[id] and advance id by one per quadword (each has an id local in
 * the DWARF). MWCC folds the increments into the addresses it computes ((id + k) << 4 from the
 * last materialized id), so id changes only at a branch or a loop; that unfolded addressing is
 * what the original's code shows after each function's branch.
 * Matching: statements sharing a line (`...; id++;`, the two halves of a zeroed quadword, the
 * four words of a colour) share it in the original's line table (tools/layout_compare.py).
 */
#include "sh2.h"
#include "libc/math.h"
#include "libc/string.h"

static void sh2gfw_Copy_FrameToWork(union Q_WORDDATA **ppqwd);

struct FilterWork shGsFilterWork;
union Q_WORDDATA Noise_Packet[160];

static void sh2gfw_Copy_FrameToWork(union Q_WORDDATA **ppqwd) {
    int id;
    union Q_WORDDATA *qwd = *ppqwd;
    int idd[3] = { 2, 0, 1 };

    id = 0;
    qwd[id].ui32[0] = 0x10000008;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000008;
    qwd[id + 1].ui32[3] = 0;
    qwd[id + 1].ui32[2] = 0xE;
    qwd[id + 1].ui32[1] = 0x10000000;
    qwd[id + 1].ui32[0] = 0x8007;
    qwd[id + 2].ul64[1] = 0x3F;
    qwd[id + 2].ul64[0] = 0;
    qwd[id + 3].ul64[1] = 0x47;
    qwd[id + 3].ul64[0] = 0x38002;
    qwd[id + 4].ul64[1] = 0x4E;
    qwd[id + 4].ul64[0] = 0x13A0001C0;
    qwd[id + 5].ui32[0] = 0x101A0;
    qwd[id + 5].ui32[1] = 0;
    qwd[id + 5].ul64[1] = 0x4C;
    qwd[id + 6].ul64[1] = 6;
    qwd[id + 6].ul64[0] = (unsigned long)(sh2gfw_GetNowDispFBP(&shGs_AllEnv) << 5) | 0x664020000 | ((unsigned long)Env_ctl.CopyFilterColor.ui32[1] << 35);
    qwd[id + 7].ul64[0] = 0;
    qwd[id + 7].ul64[1] = 0x14;
    qwd[id + 8].ul64[1] = 0x18;
    qwd[id + 8].ul64[0] = 0x7E0000007E00;
    qwd[id + 9].ui32[0] = 0x10000009;
    qwd[id + 9].ui32[1] = 0;
    qwd[id + 9].ui32[2] = 0;
    qwd[id + 9].ui32[3] = 0x50000009;
    qwd[id + 10].ul64[0] = 0x808B400000008001;
    qwd[id + 10].ul64[1] = 0xEE513513;
    qwd[id + 11].ul64[0] = 0x2000000020;
    qwd[id + 11].ul64[1] = 0;
    qwd[id + 12].ui32[0] = Env_ctl.CopyFilterColor.uc8[0];
    qwd[id + 12].ui32[1] = Env_ctl.CopyFilterColor.uc8[1];
    qwd[id + 12].ui32[2] = Env_ctl.CopyFilterColor.uc8[2];
    qwd[id + 12].ui32[3] = Env_ctl.CopyFilterColor.uc8[3];
    qwd[id + 13].ul64[0] = 0x7E0000007E00;
    qwd[id + 13].ul64[1] = 0x10;
    qwd[id + 14].ul64[0] = 0x1FE000001FE0;
    qwd[id + 14].ul64[1] = 0;
    qwd[id + 15].ui32[0] = Env_ctl.CopyFilterColor.uc8[0];
    qwd[id + 15].ui32[1] = Env_ctl.CopyFilterColor.uc8[1];
    qwd[id + 15].ui32[2] = Env_ctl.CopyFilterColor.uc8[2];
    qwd[id + 15].ui32[3] = Env_ctl.CopyFilterColor.uc8[3];
    qwd[id + 16].ul64[0] = 0x820000008200;
    qwd[id + 16].ul64[1] = 0x10;
    qwd[id + 17].ul128 = *(u_long128 *)&shGs_AllEnv.DrawEnv[idd[shGs_AllEnv.loop3]].frame_mskalpha;
    qwd[id + 18].ul64[1] = 0x18;
    qwd[id + 18].ul64[0] = 0x700000007000;
    qwd[id + 19].ul128 = 0;
    qwd[id + 19].ui32[0] = 0x60000000;
    *ppqwd = &qwd[id + 19];
}

/**
 * Writes the packet that draws the work buffer back onto the frame unchanged.
 */
void sh2gfw_Filter_JustCopy2(union Q_WORDDATA **ppqwd) {
    int id;
    int tx;
    int ty;
    Q_WORDDATA *qwd;
    struct FilterParams *pfp;

    id = 0;
    pfp = sh2gfw_Get_FilterCommandParams();
    qwd = *ppqwd;
    qwd[0].ui32[0] = 0x10000007;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0x50000007;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8006;
    qwd[2].ul64[1] = 0x3F;
    qwd[2].ul64[0] = 0;
    qwd[3].ul64[1] = 0x47;
    qwd[3].ul64[0] = 0x38002;
    qwd[4].ul64[1] = 0x4E;
    qwd[4].ul64[0] = 0x13A0001C0;
    qwd[5].ul64[1] = 6;
    qwd[5].ul64[0] = (unsigned long)sh2gfw_GetTexTBP0(&shGs_AllEnv, 7) | 0x664020000 | ((unsigned long)Env_ctl.CopyFilterColor.ui32[1] << 35);
    if (Env_ctl.mode_buf[0] == 0) {
        qwd[6].ul64[0] = 0x60;
        qwd[6].ul64[1] = 0x14;
        id += 7;
        tx = pfp->base_Ix - 4;
        ty = pfp->base_Iy - 4;
    } else {
        qwd[6].ul64[0] = 0x60;
        qwd[6].ul64[1] = 0x14;
        id += 7;
        ty = tx = 0;
    }
    qwd[id].ul64[1] = 8;
    qwd[id].ul64[0] = 5; id++;
    qwd[id].ui32[0] = 0x10000008;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000008; id++;
    qwd[id].ul64[0] = 0x708B400000008001;
    qwd[id].ul64[1] = 0xE513513; id++;
    qwd[id].ul64[0] = pfp->TexTrimSX | ((long)pfp->TexTrimSY << 32);
    qwd[id].ul64[1] = 0; id++;
    qwd[id].ui32[0] = Env_ctl.CopyFilterColor.uc8[0];
    qwd[id].ui32[1] = Env_ctl.CopyFilterColor.uc8[1];
    qwd[id].ui32[2] = Env_ctl.CopyFilterColor.uc8[2];
    qwd[id].ui32[3] = Env_ctl.CopyFilterColor.uc8[3]; id++;
    qwd[id].ul64[0] = (long)(tx + 0x7000) | ((long)(ty + 0x7000) << 32);
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id].ul64[0] = (long)((pfp->TexTrimSX + 0x1FF) << 4) | ((long)((pfp->TexTrimSY + 0x1FF) << 4) << 32);
    qwd[id].ul64[1] = 0; id++;
    qwd[id].ui32[0] = Env_ctl.CopyFilterColor.uc8[0];
    qwd[id].ui32[1] = Env_ctl.CopyFilterColor.uc8[1];
    qwd[id].ui32[2] = Env_ctl.CopyFilterColor.uc8[2];
    qwd[id].ui32[3] = Env_ctl.CopyFilterColor.uc8[3]; id++;
    qwd[id].ul64[0] = (long)(tx + 0x9000) | ((long)(ty + 0x9000) << 32);
    qwd[id].ul64[1] = 0x10; id++;
    if (Env_ctl.mode_buf[0] == 0) {
        qwd[id].ul64[1] = 0xF;
    } else {
        qwd[id].ul64[0] = 0x60;
        qwd[id].ul64[1] = 0x14;
    }
    id++;
    qwd = &qwd[id];
    sh2gfw_setREF_tagchain(&qwd, shGs_AllEnv.DefaultEnv);
    qwd->ul128 = 0;
    qwd->ui32[0] = 0x60000000;
    qwd++;
    *ppqwd = qwd;
}

/** Writes the blur pass (the work buffer blended over the frame at bl_ratio). */
void sh2gfw_Filter_Blur(union Q_WORDDATA **ppqwd, unsigned int bl_ratio) {
    int id;
    int tx;
    int ty;
    int idd[3] = { 2, 0, 1 };
    union Q_WORDDATA *qwd;

    id = 0;
    /* Matching: the DWARF has tx and ty (the line table leaves room before qwd = *ppqwd) but nothing uses
     * them; value unknown. */
    tx = ty = 0;
    qwd = *ppqwd;
    qwd[id].ui32[0] = 0x10000009;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000009;
    qwd[id + 1].ui32[3] = 0;
    qwd[id + 1].ui32[2] = 0xE;
    qwd[id + 1].ui32[1] = 0x10000000;
    qwd[id + 1].ui32[0] = 0x8008;
    qwd[id + 2].ul64[1] = 0x3F;
    qwd[id + 2].ul64[0] = 0;
    qwd[id + 3].ul64[1] = 0x47;
    qwd[id + 3].ul64[0] = 0x38002;
    qwd[id + 4].ul64[1] = 0x4E;
    qwd[id + 4].ul64[0] = 0x13A0001C0;
    qwd[id + 5].ul64[1] = 6;
    qwd[id + 5].ul64[0] = (unsigned long)(sh2gfw_GetNowDispFBP(&shGs_AllEnv) << 5) | 0x264020000;
    qwd[id + 6].ul64[0] = 0;
    qwd[id + 6].ul64[1] = 0x14;
    qwd[id + 7].ul64[1] = 8;
    qwd[id + 7].ul64[0] = 5;
    qwd[id + 8].ul64[1] = 0x42;
    qwd[id + 8].ul64[0] = 0x44;
    qwd[id + 9].ul128 = *(u_long128 *)&shGs_AllEnv.DrawEnv[idd[shGs_AllEnv.loop3]].frame_mskalpha;
    qwd[id + 10].ui32[0] = 0x10000008;
    qwd[id + 10].ui32[1] = 0;
    qwd[id + 10].ui32[2] = 0;
    qwd[id + 10].ui32[3] = 0x50000008;
    qwd[id + 11].ul64[0] = 0x70AB400000008001;
    qwd[id + 11].ul64[1] = 0xE513513;
    qwd[id + 12].ul64[0] = 0; qwd[id + 12].ul64[1] = 0;
    qwd[id + 13].ui32[0] = 0x80;
    qwd[id + 13].ui32[1] = 0x80;
    qwd[id + 13].ui32[2] = 0x80;
    qwd[id + 13].ui32[3] = bl_ratio;
    qwd[id + 14].ul64[0] = 0x700000007000;
    qwd[id + 14].ul64[1] = 0x10;
    qwd[id + 15].ul64[0] = 0x200000002000;
    qwd[id + 15].ul64[1] = 0;
    qwd[id + 16].ui32[0] = 0x80;
    qwd[id + 16].ui32[1] = 0x80;
    qwd[id + 16].ui32[2] = 0x80;
    qwd[id + 16].ui32[3] = bl_ratio;
    qwd[id + 17].ul64[0] = 0x900000009000;
    qwd[id + 17].ul64[1] = 0x10;
    qwd[id + 18].ul64[0] = 0x60;
    qwd[id + 18].ul64[1] = 0x14;
    qwd[id + 19].ul128 = 0;
    qwd[id + 19].ui32[0] = 0x60000000;
    *ppqwd = &qwd[id + 20];
}

/** Writes the dark blur pass (blur limited by an alpha test against aref). */
void sh2gfw_Filter_Dark_Blur(union Q_WORDDATA **ppqwd, unsigned int bl_ratio, unsigned int aref) {
    int id;
    int tx;
    int ty;
    int idd[3] = { 2, 0, 1 };
    union Q_WORDDATA *qwd;

    id = 0;
    tx = ty = 0; /* Matching: unused, see sh2gfw_Filter_Blur. */
    qwd = *ppqwd;
    qwd[id].ui32[0] = 0x10000009;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000009;
    qwd[id + 1].ui32[3] = 0;
    qwd[id + 1].ui32[2] = 0xE;
    qwd[id + 1].ui32[1] = 0x10000000;
    qwd[id + 1].ui32[0] = 0x8008;
    qwd[id + 2].ul64[1] = 0x3F;
    qwd[id + 2].ul64[0] = 0;
    qwd[id + 3].ul64[1] = 0x47;
    qwd[id + 3].ul64[0] = ((unsigned long)aref << 4) | 0x38007;
    qwd[id + 4].ul64[1] = 0x4E;
    qwd[id + 4].ul64[0] = 0x13A0001C0;
    qwd[id + 5].ul64[1] = 6;
    qwd[id + 5].ul64[0] = (unsigned long)(sh2gfw_GetNowDispFBP(&shGs_AllEnv) << 5) | 0x664020000;
    qwd[id + 6].ul64[0] = 0;
    qwd[id + 6].ul64[1] = 0x14;
    qwd[id + 7].ul64[1] = 8;
    qwd[id + 7].ul64[0] = 5;
    qwd[id + 8].ul64[1] = 0x42;
    qwd[id + 8].ul64[0] = ((unsigned long)bl_ratio << 32) | 0x64;
    qwd[id + 9].ul128 = *(u_long128 *)&shGs_AllEnv.DrawEnv[idd[shGs_AllEnv.loop3]].frame_mskalpha;
    qwd[id + 10].ui32[0] = 0x10000009;
    qwd[id + 10].ui32[1] = 0;
    qwd[id + 10].ui32[2] = 0;
    qwd[id + 10].ui32[3] = 0x50000009;
    qwd[id + 11].ul64[0] = 0x80AB400000008001;
    qwd[id + 11].ul64[1] = 0xEE513513;
    qwd[id + 12].ul64[0] = 0; qwd[id + 12].ul64[1] = 0;
    qwd[id + 13].ui32[0] = 0x80;
    qwd[id + 13].ui32[1] = 0x80;
    qwd[id + 13].ui32[2] = 0x80;
    qwd[id + 13].ui32[3] = 0x80;
    qwd[id + 14].ul64[0] = 0x6FFC00006FFC;
    qwd[id + 14].ul64[1] = 0x10;
    qwd[id + 15].ul64[0] = 0x200000002000;
    qwd[id + 15].ul64[1] = 0;
    qwd[id + 16].ui32[0] = 0x80;
    qwd[id + 16].ui32[1] = 0x80;
    qwd[id + 16].ui32[2] = 0x80;
    qwd[id + 16].ui32[3] = 0x80;
    qwd[id + 17].ul64[0] = 0x8FFC00008FFC;
    qwd[id + 17].ul64[1] = 0x10;
    qwd[id + 18] = shGs_AllEnv.DefaultEnv[2];
    qwd[id + 19].ul64[0] = 0x60;
    qwd[id + 19].ul64[1] = 0x14;
    qwd[id + 20].ul128 = 0;
    qwd[id + 20].ui32[0] = 0x60000000;
    *ppqwd = &qwd[id + 21];
}

/**
 * Writes the glow blur pass: blurs the work buffer and adds the parts above an alpha
 * threshold back onto the frame.
 * @param aref     alpha test reference
 * @param bl_ratio blur ratio
 * @param pam      not used
 */
void sh2gfw_Filter_Glow_Blur(union Q_WORDDATA **ppqwd, unsigned int aref, unsigned int bl_ratio, unsigned int pam) {
    int id = 0;
    int tx;
    int ty;
    int idd[3] = { 2, 0, 1 };
    union Q_WORDDATA *qwd;

    tx = ty = 0; /* Matching: unused, see sh2gfw_Filter_Blur. */
    qwd = *ppqwd;
    qwd[0].ui32[0] = 0x10000009;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0x50000009;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8008;
    qwd[2].ul64[1] = 0x3F;
    qwd[2].ul64[0] = 0;
    qwd[3].ul64[1] = 0x47;
    qwd[3].ul64[0] = ((unsigned long)aref << 4) | 0x3800B;
    qwd[4].ul64[1] = 0x4E;
    qwd[4].ul64[0] = 0x13A0001C0;
    qwd[5].ul64[1] = 6;
    qwd[5].ul64[0] = (unsigned long)(sh2gfw_GetNowDispFBP(&shGs_AllEnv) << 5) | 0x664020000;
    if (pam) {
        qwd[6].ul64[0] = 0x60;
        qwd[6].ul64[1] = 0x14;
        id += 7;
    } else {
        qwd[6].ul64[0] = 0;
        qwd[6].ul64[1] = 0x14;
        id += 7;
    }
    qwd[id].ul64[1] = 8;
    qwd[id].ul64[0] = 5; id++;
    qwd[id].ul64[1] = 0x42;
    qwd[id].ul64[0] = ((unsigned long)bl_ratio << 32) | 0x64; id++;
    qwd[id] = *(union Q_WORDDATA *)&shGs_AllEnv.DrawEnv[idd[shGs_AllEnv.loop3]].frame_mskalpha; id++;
    qwd[id].ui32[0] = 0x10000009;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000009; id++;
    qwd[id].ul64[0] = 0x80AB400000008001;
    qwd[id].ul64[1] = 0xEE513513; id++;
    qwd[id].ul64[0] = 0; qwd[id].ul64[1] = 0; id++;
    qwd[id].ui32[0] = 0x80;
    qwd[id].ui32[1] = 0x80;
    qwd[id].ui32[2] = 0x80;
    qwd[id].ui32[3] = 0x80; id++;
    qwd[id].ul64[0] = 0x6FFC00006FFC;
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id].ul64[0] = 0x200000002000;
    qwd[id].ul64[1] = 0; id++;
    qwd[id].ui32[0] = 0x80;
    qwd[id].ui32[1] = 0x80;
    qwd[id].ui32[2] = 0x80;
    qwd[id].ui32[3] = 0x80; id++;
    qwd[id].ul64[0] = 0x8FFC00008FFC;
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id] = shGs_AllEnv.DefaultEnv[2]; id++;
    qwd[id].ul64[0] = 0x60;
    qwd[id].ul64[1] = 0x14; id++;
    qwd[id].ul128 = 0;
    qwd[id].ui32[0] = 0x60000000; id++;
    *ppqwd = &qwd[id];
}

/** Fills CalcTex_buffer with 16 KB of random bytes (made in scratchpad memory, then sent by DMA). */
void sh2gfw_test_MakeNoise(void) {
    unsigned int *noise_data1;
    unsigned int *noise_data2;
    unsigned int ira;
    unsigned int irc;
    unsigned int imsk;
    unsigned int itmp;
    unsigned int isd;
    int i;

    noise_data1 = (unsigned int *)0x70000000;
    isd = Env_ctl.random_seeds.ui32[0];
    for (i = 0xFFF; i >= 0; i--) {
        ira = 0x19660D;
        irc = 0x3C6EF35F;
        isd = isd * ira + irc;
        itmp = isd >> 24;
        isd = isd * ira + irc;
        itmp = (itmp << 8) | (isd >> 24);
        isd = isd * ira + irc;
        itmp = (itmp << 8) | (isd >> 24);
        isd = isd * ira + irc;
        itmp = (itmp << 8) | (isd >> 24);
        noise_data1[i] = itmp;
    }
    Env_ctl.random_seeds.ui32[0] = isd;
    while (*D8_CHCR & 0x100) {
    }
    *D8_QWC = 0x400;
    *D8_MADR = (unsigned int)CalcTex_buffer;
    *D8_SADR = 0;
    *D8_CHCR = 0x100;
    while (*D8_CHCR & 0x100) {
        for (i = 0; i < 3; i++) {
            isd++;
        }
    }
}

/** Builds a full-screen sprite packet in Noise_Packet (with TEST and ZBUF settings) and sends it on DMA channel 1. */
void sh2gfw_Black_Clear(void) {
    union Q_WORDDATA *qwd;
    int id;

    id = 0;
    qwd = Noise_Packet;
    qwd[id].ui32[0] = 0x10000003;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000003;
    qwd[id + 1].ui32[3] = 0;
    qwd[id + 1].ui32[2] = 0xE;
    qwd[id + 1].ui32[1] = 0x10000000;
    qwd[id + 1].ui32[0] = 0x8002;
    qwd[id + 2].ul64[1] = 0x47;
    qwd[id + 2].ul64[0] = 0x3800A;
    qwd[id + 3].ul64[1] = 0x4E;
    qwd[id + 3].ul64[0] = 0x13A0001C0;
    qwd[id + 4].ui32[0] = 0x10000005;
    qwd[id + 4].ui32[1] = 0;
    qwd[id + 4].ui32[2] = 0;
    qwd[id + 4].ui32[3] = 0x50000005;
    qwd[id + 5].ul64[0] = 0x4003400000008001;
    qwd[id + 5].ul64[1] = 0x5151;
    qwd[id + 6].ui32[0] = 0x80;
    qwd[id + 6].ui32[1] = 0x80;
    qwd[id + 6].ui32[2] = 0x80;
    qwd[id + 6].ui32[3] = 0x80;
    qwd[id + 7].ul64[0] = 0x700000007000;
    qwd[id + 7].ul64[1] = 0x10;
    qwd[id + 8].ui32[0] = 0x80;
    qwd[id + 8].ui32[1] = 0x80;
    qwd[id + 8].ui32[2] = 0x80;
    qwd[id + 8].ui32[3] = 0x80;
    qwd[id + 9].ul64[0] = 0x900000009000;
    qwd[id + 9].ul64[1] = 0x10;
    qwd[id + 10].ul128 = 0;
    qwd[id + 10].ui32[0] = 0x70000000;
    d1cSend(Noise_Packet);
}

/**
 * Writes the noise overlay pass (the noise texture blended over the frame).
 * @param ratio blend ratio (the ALPHA register's fixed value)
 * @param pam   0 or 1: the blend equation
 * @param mode  1: the second form of the pass
 * @param loop  1: also restore the frame mask register
 */
void sh2gfw_SendDraw_Noise(union Q_WORDDATA **ppqwd, unsigned int ratio, unsigned int pam, unsigned int mode, int loop) {
    Q_WORDDATA *pqwd;
    Q_WORDDATA *qwd;
    int id;
    int tx;
    int ty;

    pqwd = *ppqwd;
    id = 0;
    {
        int idd[3] = { 2, 0, 1 };

        pqwd->ui32[0] = 0x10000006;
        pqwd->ui32[1] = 0;
        pqwd->ui32[2] = 0;
        pqwd->ui32[3] = 0x50000006;
        pqwd++;
        pqwd->ui32[3] = 0;
        pqwd->ui32[2] = 0xE;
        pqwd->ui32[1] = 0x10000000;
        pqwd->ui32[0] = 0x8004;
        pqwd++;
        pqwd->ul64[1] = 0x50;
        pqwd->ul64[0] = 0x1402350014020000;
        pqwd++;
        pqwd->ul64[1] = 0x51;
        pqwd->ul64[0] = 0;
        pqwd++;
        pqwd->ul64[1] = 0x52;
        pqwd->ui32[0] = 0x80;
        pqwd->ui32[1] = 0x80;
        pqwd++;
        pqwd->ul64[1] = 0x53;
        pqwd->ul64[0] = 0;
        pqwd++;
        pqwd->ui32[3] = 0;
        pqwd->ui32[2] = 0;
        pqwd->ui32[1] = 0x8000000;
        pqwd->ui32[0] = 0x8200;
        pqwd++;
        pqwd->ui32[0] = 0x30000200;
        pqwd->ui32[1] = (unsigned long)(unsigned int)CalcTex_buffer & 0x7FFFFFFF;
        pqwd->ui32[2] = 0;
        pqwd->ui32[3] = 0;
        pqwd->ui32[3] = 0x50000200;
        pqwd++;
        qwd = pqwd;
        qwd[0].ui32[0] = 0x10000009;
        qwd[0].ui32[1] = 0;
        qwd[0].ui32[2] = 0;
        qwd[0].ui32[3] = 0x50000009;
        qwd[1].ui32[3] = 0;
        qwd[1].ui32[2] = 0xE;
        qwd[1].ui32[1] = 0x10000000;
        qwd[1].ui32[0] = 0x8008;
        qwd[2].ul64[1] = 0x3F;
        qwd[2].ul64[0] = 0;
        qwd[3].ul64[1] = 0x47;
        qwd[3].ul64[0] = 0x3001D;
        qwd[4].ul64[1] = 0x4E;
        qwd[4].ul64[0] = 0x13A0001C0;
        qwd[5].ul64[1] = 6;
        qwd[5].ul64[0] = 0x2006F881DD40B500;
        if (Env_ctl.mode_buf[0] == 0) {
            qwd[6].ul64[0] = 0x60;
            qwd[6].ul64[1] = 0x14;
            id += 7;
            ty = tx = -4;
        } else {
            qwd[6].ul64[0] = 0x60;
            qwd[6].ul64[1] = 0x14;
            id += 7;
            ty = tx = 0;
    }
    qwd[id].ul64[1] = 8;
    qwd[id].ul64[0] = 5; id++;
    if (mode == 1) {
        if (pam == 0) {
            qwd[id].ul64[1] = 0x42;
            qwd[id].ul64[0] = ((unsigned long)ratio << 32) | 0x21; id++;
        } else if (pam == 1) {
            qwd[id].ul64[1] = 0x42;
            qwd[id].ul64[0] = ((unsigned long)ratio << 32) | 0x11; id++;
        }
        if (loop == 1) {
            qwd[id].ul64[1] = 0xF; id++;
        } else {
            qwd[id] = *(union Q_WORDDATA *)&shGs_AllEnv.DrawEnv[shGs_AllEnv.loop3].frame_mskalpha; id++;
        }
    } else {
        int idd[3] = { 2, 0, 1 };

        if (pam == 0) {
            qwd[id].ul64[1] = 0x42;
            qwd[id].ul64[0] = ((unsigned long)ratio << 32) | 0x21; id++;
        } else if (pam == 1) {
            qwd[id].ul64[1] = 0x42;
            qwd[id].ul64[0] = ((unsigned long)ratio << 32) | 0x11; id++;
        }
        if (loop == 1) {
            qwd[id].ul64[1] = 0xF; id++;
        } else {
            qwd[id] = *(union Q_WORDDATA *)&shGs_AllEnv.DrawEnv[shGs_AllEnv.loop3].frame_mskalpha; id++;
        }
    }
    {
        int ix;
        int iy;
        int col;

        col = Env_ctl.NoiseCondition.uc8[5];
        for (ix = 0; ix < 4; ix++) {
            for (iy = 0; iy < 4; iy++) {
                qwd[id].ui32[0] = 0x10000007;
                qwd[id].ui32[1] = 0;
                qwd[id].ui32[2] = 0;
                qwd[id].ui32[3] = 0x50000007; id++;
                qwd[id].ul64[0] = 0x60AB400000008001;
                qwd[id].ul64[1] = 0x513513; id++;
                qwd[id].ul64[0] = 0; qwd[id].ul64[1] = 0; id++;
                qwd[id].ui32[0] = col; qwd[id].ui32[1] = col; qwd[id].ui32[2] = col; qwd[id].ui32[3] = 0x80; id++;
                qwd[id].ul64[0] = (long)(tx + 0x7000 + ((ix * 128) << 4)) | ((long)(ty + 0x7000 + ((iy * 128) << 4)) << 32);
                qwd[id].ul64[1] = 0x10; id++;
                qwd[id].ul64[0] = 0x80000000800;
                qwd[id].ul64[1] = 0; id++;
                qwd[id].ui32[0] = col; qwd[id].ui32[1] = col; qwd[id].ui32[2] = col; qwd[id].ui32[3] = 0x80; id++;
                qwd[id].ul64[0] = (long)(tx + 0x6FFF + ((ix * 128 + 128) << 4)) | ((long)(ty + 0x6FFF + ((iy * 128 + 128) << 4)) << 32);
                qwd[id].ul64[1] = 0x10; id++;
            }
        }
    }
    pqwd = &qwd[id];
    if (loop != 1) {
        sh2gfw_setREF_tagchain(&pqwd, shGs_AllEnv.DefaultEnv);
    }
    pqwd->ui32[0] = 0x70000000;
    pqwd->ui32[1] = 0;
    pqwd->ul64[1] = 0;
    pqwd++;
    *ppqwd = pqwd;
    }
}

/**
 * Writes the soft-focus passes, bouncing the image between the work buffers.
 * @param mode     iteration count
 * @param arg2     passed as 0x80 by the caller
 * @param CompoIt  shift of the blurred copies
 * @param aref     alpha of the composite
 * @param testval  alpha test value
 * @param testmode alpha test mode
 */
void sh2gfw_Swap_Soft(union Q_WORDDATA **ppqwd, int mode, int arg2, int CompoIt, int aref, int testval, int testmode) {
    int id = 0;
    int swapbuff = 1;
    int ix;
    int iy;
    int counter;
    int tx;
    int ty;
    int idd[3] = { 2, 0, 1 };
    union Q_WORDDATA SwapTex0[2];
    union Q_WORDDATA RegFrame[2];
    union Q_WORDDATA *qwd;
    short dx[8];
    short dy[8];
    int i;
    int sz;
    int cent;

    qwd = *ppqwd;
    ty = sh2gfw_GetTexTBP0(&shGs_AllEnv, 7) >> 5;
    RegFrame[0] = *(union Q_WORDDATA *)&shGs_AllEnv.DrawEnv[idd[shGs_AllEnv.loop3]].frame_normal;
    RegFrame[1].ui32[0] = ty | 0x80000;
    RegFrame[1].ui32[1] = 0;
    RegFrame[1].ul64[1] = 0x4C;
    SwapTex0[0].ul64[1] = 6;
    SwapTex0[0].ul64[0] = sh2gfw_GetTexTBP0(&shGs_AllEnv, 7) | 0x664020000;
    SwapTex0[1].ul64[1] = 6;
    SwapTex0[1].ul64[0] = (RegFrame[0].us16[0] << 5) | 0x664020000;
    qwd[0].ui32[0] = 0x10000008;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0x50000008;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8007;
    qwd[2].ul64[1] = 0x3F;
    qwd[2].ul64[0] = 0;
    switch (testmode) {
    case 0:
        qwd[3].ul64[1] = 0x47;
        qwd[3].ul64[0] = ((long)testval << 4) | 0x3800A;
        id += 4;
        break;
    case 1:
        qwd[3].ul64[1] = 0x47;
        qwd[3].ul64[0] = ((long)testval << 4) | 0x3800D;
        id += 4;
        break;
    default:
        qwd[3].ul64[1] = 0x47;
        qwd[3].ul64[0] = ((long)testval << 4) | 0x38007;
        id += 4;
        break;
    }
    qwd[id].ul64[1] = 0x4E;
    qwd[id].ul64[0] = 0x13A0001C0; id++;
    qwd[id] = SwapTex0[0]; id++;
    qwd[id].ul64[0] = 0x60;
    qwd[id].ul64[1] = 0x14; id++;
    qwd[id].ul64[1] = 0x42;
    qwd[id].ul64[0] = ((long)aref << 32) | 0x64; id++;
    qwd[id].ul64[1] = 8;
    qwd[id].ul64[0] = 5; id++;
    if (mode) {
        cent = mode / 2;
        sz = CompoIt * 4;
        if (mode & 1) {
            for (i = 0; i < mode; i++) {
                counter = sz * (i - cent) - 4;
                dx[i] = counter;
                dy[i] = counter;
            }
        } else {
            for (i = 0; i < mode; i++) {
                if (i < cent) {
                    counter = sz * (i - cent) - sz / 2 - 4;
                    dx[i] = counter;
                    dy[i] = counter;
                } else {
                    counter = sz / 2 + sz * (i - cent) - 4;
                    dx[i] = counter;
                    dy[i] = counter;
                }
            }
        }
        cent = mode * (mode * 10);
        qwd[id].ui32[0] = cent | 0x10000000;
        qwd[id].ui32[1] = 0;
        qwd[id].ui32[2] = 0;
        qwd[id].ui32[3] = cent | 0x50000000; id++;
        for (iy = 0; iy < mode; iy++) {
            for (ix = 0; ix < mode; ix++) {
                qwd[id].ul64[0] = 0x90AB400000008001;
                qwd[id].ul64[1] = 0x513513EEE; id++;
                qwd[id] = RegFrame[swapbuff]; id++;
                qwd[id] = SwapTex0[swapbuff];
                swapbuff ^= 1; id++;
                qwd[id].ul64[1] = 0xF; id++;
                qwd[id].ul64[0] = 0; qwd[id].ul64[1] = 0; id++;
                qwd[id].ui32[0] = 0x80;
                qwd[id].ui32[1] = 0x80;
                qwd[id].ui32[2] = 0x80;
                qwd[id].ui32[3] = 0x80; id++;
                qwd[id].ul64[0] = (long)(dx[ix] + 0x7000) | ((long)(dy[iy] + 0x7000) << 32);
                qwd[id].ul64[1] = 0x10; id++;
                qwd[id].ul64[0] = 0x1FF000001FF0;
                qwd[id].ul64[1] = 0; id++;
                qwd[id].ui32[0] = 0x80;
                qwd[id].ui32[1] = 0x80;
                qwd[id].ui32[2] = 0x80;
                qwd[id].ui32[3] = 0x80; id++;
                qwd[id].ul64[0] = (long)(dx[ix] + 0x9000) | ((long)(dy[iy] + 0x9000) << 32);
                qwd[id].ul64[1] = 0x10; id++;
            }
        }
        tx = ty = 0;
        if (!swapbuff) {
            qwd[id].ui32[0] = 0x10000009;
            qwd[id].ui32[1] = 0;
            qwd[id].ui32[2] = 0;
            qwd[id].ui32[3] = 0x50000009; id++;
            qwd[id].ul64[0] = 0x80AB400000008001;
            qwd[id].ul64[1] = 0x513513EE; id++;
            qwd[id] = RegFrame[swapbuff]; id++;
            qwd[id] = SwapTex0[swapbuff]; id++;
            qwd[id].ul64[0] = 0; qwd[id].ul64[1] = 0; id++;
            qwd[id].ui32[0] = 0x80;
            qwd[id].ui32[1] = 0x80;
            qwd[id].ui32[2] = 0x80;
            qwd[id].ui32[3] = 0x80; id++;
            qwd[id].ul64[0] = (long)(tx + 0x7000) | ((long)(ty + 0x7000) << 32);
            qwd[id].ul64[1] = 0x10; id++;
            qwd[id].ul64[0] = 0x1FF000001FF0;
            qwd[id].ul64[1] = 0; id++;
            qwd[id].ui32[0] = 0x80;
            qwd[id].ui32[1] = 0x80;
            qwd[id].ui32[2] = 0x80;
            qwd[id].ui32[3] = 0x80; id++;
            qwd[id].ul64[0] = (long)(tx + 0x9000) | ((long)(ty + 0x9000) << 32);
            qwd[id].ul64[1] = 0x10; id++;
        }
    }
    qwd[id].ul128 = 0;
    qwd[id].ui32[0] = 0x60000000;
    *ppqwd = &qwd[id];
}

/**
 * Writes the glow soft-focus passes (Swap_Soft with an additive composite).
 * Matching: `i` (a DWARF local this function otherwise never uses) holds GetTexTBP0(...) >> 5, as `ty` does
 * in sh2gfw_Swap_Soft; with it the parameters' registers come out as the original's
 * (docs/matching-notes.md#sh2gfw_2d_filters-sh2gfw_swap_glowsoft).
 */
void sh2gfw_Swap_GlowSoft(union Q_WORDDATA **ppqwd, int mode, int parm, int aref, int Shift) {
    int id = 0;
    int swapbuff = 1;
    int ix;
    int iy;
    int counter;
    int idd[3] = { 2, 0, 1 };
    union Q_WORDDATA SwapTex0[2];
    union Q_WORDDATA RegFrame[2];
    Q_WORDDATA *qwd;
    short dx[8];
    short dy[8];
    int i;
    int sz;
    int cent;

    qwd = *ppqwd;
    i = sh2gfw_GetTexTBP0(&shGs_AllEnv, 7) >> 5;
    RegFrame[0] = *(union Q_WORDDATA *)&shGs_AllEnv.DrawEnv[idd[shGs_AllEnv.loop3]].frame_normal;
    /* Matching: one line in the original */
    RegFrame[1].ui32[0] = i | 0x80000; RegFrame[1].ui32[1] = 0; RegFrame[1].ul64[1] = 0x4C;
    SwapTex0[0].ul64[1] = 6;
    SwapTex0[0].ul64[0] = sh2gfw_GetTexTBP0(&shGs_AllEnv, 7) | 0x664020000;
    SwapTex0[1].ul64[1] = 6;
    SwapTex0[1].ul64[0] = (RegFrame[0].us16[0] << 5) | 0x664020000;
    qwd[0].ui32[0] = 0x10000008;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0x50000008;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8007;
    qwd[2].ul64[1] = 0x3F;
    qwd[2].ul64[0] = 0;
    qwd[3].ul64[1] = 0x47;
    qwd[3].ul64[0] = ((long)aref << 4) | 0x3800B;
    qwd[4].ul64[1] = 0x4E;
    qwd[4].ul64[0] = 0x13A0001C0;
    qwd[5] = SwapTex0[0];
    qwd[6].ul64[0] = 0x60;
    qwd[6].ul64[1] = 0x14;
    qwd[7].ul64[1] = 0x42;
    qwd[7].ul64[0] = ((long)parm << 32) | 0x68;
    qwd[8].ul64[1] = 8;
    qwd[8].ul64[0] = 5;
    id += 9;
    if (mode) {
        cent = mode / 2;
        sz = Shift * 4;
        if (mode & 1) {
            for (ix = 0; ix < mode; ix++) {
                counter = sz * (ix - cent) - 4;
                dx[ix] = counter;
                dy[ix] = counter;
            }
        } else {
            for (ix = 0; ix < mode; ix++) {
                if (ix < cent) {
                    counter = sz * (ix - cent) - sz / 2 - 4;
                    dx[ix] = counter;
                    dy[ix] = counter;
                } else {
                    counter = sz / 2 + sz * (ix - cent) - 4;
                    dx[ix] = counter;
                    dy[ix] = counter;
                }
            }
        }
        counter = mode * (mode * 10);
        qwd[id].ui32[0] = counter | 0x10000000;
        qwd[id].ui32[1] = 0;
        qwd[id].ui32[2] = 0;
        qwd[id].ui32[3] = counter | 0x50000000; id++;
        for (iy = 0; iy < mode; iy++) {
            for (ix = 0; ix < mode; ix++) {
                qwd[id].ul64[0] = 0x90AB400000008001;
                qwd[id].ul64[1] = 0x513513EEE; id++;
                qwd[id] = RegFrame[swapbuff]; id++;
                qwd[id] = SwapTex0[swapbuff];
                swapbuff ^= 1; id++;
                qwd[id].ul64[1] = 0xF;
                id++;
                qwd[id].ul64[0] = 0; qwd[id].ul64[1] = 0; id++;
                qwd[id].ui32[0] = 0x80;
                qwd[id].ui32[1] = 0x80;
                qwd[id].ui32[2] = 0x80;
                qwd[id].ui32[3] = 0x80; id++;
                qwd[id].ul64[0] = (long)(dx[ix] + 0x7000) | ((long)(dy[iy] + 0x7000) << 32);
                qwd[id].ul64[1] = 0x10; id++;
                qwd[id].ul64[0] = 0x1FF000001FF0;
                qwd[id].ul64[1] = 0; id++;
                qwd[id].ui32[0] = 0x80;
                qwd[id].ui32[1] = 0x80;
                qwd[id].ui32[2] = 0x80;
                qwd[id].ui32[3] = 0x80; id++;
                qwd[id].ul64[0] = (long)(dx[ix] + 0x9000) | ((long)(dy[iy] + 0x9000) << 32);
                qwd[id].ul64[1] = 0x10; id++;
            }
        }
        if (!swapbuff) {
            qwd[id].ui32[0] = 0x10000009;
            qwd[id].ui32[1] = 0;
            qwd[id].ui32[2] = 0;
            qwd[id].ui32[3] = 0x50000009; id++;
            qwd[id].ul64[0] = 0x80AB400000008001;
            qwd[id].ul64[1] = 0x513513EE; id++;
            qwd[id] = RegFrame[swapbuff]; id++;
            qwd[id] = SwapTex0[swapbuff]; id++;
            qwd[id].ul64[0] = 0; qwd[id].ul64[1] = 0; id++;
            qwd[id].ui32[0] = 0x80;
            qwd[id].ui32[1] = 0x80;
            qwd[id].ui32[2] = 0x80;
            qwd[id].ui32[3] = 0x80; id++;
            qwd[id].ul64[0] = 0x700000007000;
            qwd[id].ul64[1] = 0x10; id++;
            qwd[id].ul64[0] = 0x1FF000001FF0;
            qwd[id].ul64[1] = 0; id++;
            qwd[id].ui32[0] = 0x80;
            qwd[id].ui32[1] = 0x80;
            qwd[id].ui32[2] = 0x80;
            qwd[id].ui32[3] = 0x80; id++;
            qwd[id].ul64[0] = 0x900000009000;
            qwd[id].ul64[1] = 0x10; id++;
        }
    }
    qwd = &qwd[id];
    sh2gfw_setREF_tagchain(&qwd, shGs_AllEnv.DefaultEnv);
    qwd->ul128 = 0;
    qwd->ui32[0] = 0x60000000;
    *ppqwd = qwd;
}

/* Matching: a fitted stand-in for software-double code (docs/stand-ins.md): it makes sh2gfw_FadeOut_Retain
 * keep pfp->TargetSec in a2; the original's line table has room for a stripped function here
 * (docs/matching-notes.md#sh2gfw_2d_filters-stripped_double_code). */
STRIPPED_DOUBLE_CODE()


/**
 * Writes a fade-out over the retained (frozen) frame.
 * @param BorW 1 black, 0 white, 2 red
 * Matching: TargetSec's a2 comes from the stand-in above; the branch that only assigns the dead faderatio
 * is reconstructed from the line table, not recovered
 * (docs/matching-notes.md#sh2gfw_2d_filters-sh2gfw_fadeout_retain).
 */
void sh2gfw_FadeOut_Retain(union Q_WORDDATA **ppqwd, int BorW) {
    int id;
    int tx;
    int ty;
    int faderatio;
    union Q_WORDDATA *qwd;
    struct FilterParams *pfp;
    int colr;
    int colg;
    int colb;
    int pr;
    int pg;
    int pb;

    id = 0;
    pfp = sh2gfw_Get_FilterCommandParams();
    tx = ty = 0;
    qwd = *ppqwd;
    if (pfp->TargetSec > 0) {
        if (pfp->FO_timer % 2) {
            faderatio = floor(1280.0f / (30.0f * pfp->TargetSec));
        } else {
            faderatio = ceil(1280.0f / (30.0f * pfp->TargetSec));
        }
    } else {
        faderatio = 0x80;
    }
    qwd[0].ui32[0] = 0x10000007;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0x50000007;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8006;
    qwd[2].ul64[1] = 0x3F;
    qwd[2].ul64[0] = 0;
    qwd[3].ul64[1] = 0x47;
    qwd[3].ul64[0] = 0x38002;
    qwd[4].ul64[1] = 0x4E;
    qwd[4].ul64[0] = 0x13A0001C0;
    qwd[5].ul64[0] = 0;
    qwd[5].ul64[1] = 0x14;
    qwd[6].ul64[1] = 6;
    qwd[6].ul64[0] = (unsigned long)(sh2gfw_GetNowDispFBP(&shGs_AllEnv) << 5) | 0x664020000;
    if (BorW == 1) {
        qwd[7].ul64[1] = 0x42;
        qwd[7].ul64[0] = ((long)faderatio << 32) | 0x62;
        id += 8;
        colr = colg = colb = 0x80;
        pr = pg = pb = 0xFF;
    } else if (BorW == 0) {
        qwd[7].ul64[1] = 0x42;
        qwd[7].ul64[0] = ((long)faderatio << 32) | 0x68;
        id += 8;
        colr = colg = colb = 0x80;
        pr = pg = pb = 0xFF;
    } else {
        faderatio <<= 3;
        if (faderatio >= 0x32) {
            faderatio = 0x32;
        }
        colr = faderatio + 0x80;
        colg = colb = 0x80 - faderatio;
        pr = 0xFF;
        pg = pb = 0;
        if (faderatio / 8) { faderatio = 0; } /* Matching: reconstructed, see above */
        qwd[7].ul64[1] = 0x42;
        qwd[7].ul64[0] = 0x100000068;
        id += 8;
    }
    qwd[id].ui32[0] = 0x10000007;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000007; id++;
    qwd[id].ul64[0] = 0x608B400000008001;
    qwd[id].ul64[1] = 0x513513; id++;
    qwd[id].ul64[0] = 0; qwd[id].ul64[1] = 0; id++;
    qwd[id].ui32[0] = colr;
    qwd[id].ui32[1] = colb; /* Matching: colb as in sh2gfw_Filter_Retain (colg == colb) */
    qwd[id].ui32[2] = colb;
    qwd[id].ui32[3] = 0x80; id++;
    qwd[id].ul64[0] = (long)(tx + 0x7000) | ((long)(ty + 0x7000) << 32);
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id].ul64[0] = 0x200000002000;
    qwd[id].ul64[1] = 0; id++;
    qwd[id].ui32[0] = colr;
    qwd[id].ui32[1] = colg;
    qwd[id].ui32[2] = colb;
    qwd[id].ui32[3] = 0x80; id++;
    qwd[id].ul64[0] = (long)(tx + 0x9000) | ((long)(ty + 0x9000) << 32);
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id].ui32[0] = 0x10000007;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000007; id++;
    qwd[id].ul64[0] = 0x6023400000008001;
    qwd[id].ul64[1] = 0xEE5151; id++;
    qwd[id].ui32[0] = pr;
    qwd[id].ui32[1] = pg;
    qwd[id].ui32[2] = pb;
    qwd[id].ui32[3] = 0x80; id++;
    qwd[id].ul64[0] = (long)(tx + 0x7000) | ((long)(ty + 0x7000) << 32);
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id].ui32[0] = pr;
    qwd[id].ui32[1] = pg;
    qwd[id].ui32[2] = pb;
    qwd[id].ui32[3] = 0x80; id++;
    qwd[id].ul64[0] = (long)(tx + 0x9000) | ((long)(ty + 0x9000) << 32);
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id] = shGs_AllEnv.DefaultEnv[2]; id++;
    qwd[id].ul64[0] = 0x60;
    qwd[id].ul64[1] = 0x14; id++;
    qwd[id].ul128 = 0;
    qwd[id].ui32[0] = 0x60000000; id++;
    *ppqwd = &qwd[id];
    pfp->FO_timer++;
}

/** Writes the packet that redraws the retained (frozen) frame. */
void sh2gfw_Filter_Retain(union Q_WORDDATA **ppqwd) {
    int id;
    int tx;
    int ty;
    union Q_WORDDATA *qwd;
    struct FilterParams *pfp;
    int colr;
    int colg;
    int colb;
    int pr;
    int pg;
    int pb;

    id = 0;
    pfp = sh2gfw_Get_FilterCommandParams();
    tx = ty = 0;
    qwd = *ppqwd;
    qwd[id].ui32[0] = 0x10000007;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000007;
    qwd[id + 1].ui32[3] = 0;
    qwd[id + 1].ui32[2] = 0xE;
    qwd[id + 1].ui32[1] = 0x10000000;
    qwd[id + 1].ui32[0] = 0x8006;
    qwd[id + 2].ul64[1] = 0x3F;
    qwd[id + 2].ul64[0] = 0;
    qwd[id + 3].ul64[1] = 0x47;
    qwd[id + 3].ul64[0] = 0x38002;
    qwd[id + 4].ul64[1] = 0x4E;
    qwd[id + 4].ul64[0] = 0x13A0001C0;
    qwd[id + 5].ul64[0] = 0;
    qwd[id + 5].ul64[1] = 0x14;
    qwd[id + 6].ul64[1] = 6;
    qwd[id + 6].ul64[0] = (unsigned long)(sh2gfw_GetNowDispFBP(&shGs_AllEnv) << 5) | 0x664020000;
    qwd[id + 7].ul64[1] = 0x42;
    qwd[id + 7].ul64[0] = 0x8000000062;
    if (pfp->FO_timer) {
        colr = colg = colb = 0x80;
    } else {
        colr = Env_ctl.CopyFilterColor.uc8[0];
        colg = Env_ctl.CopyFilterColor.uc8[1];
        colb = Env_ctl.CopyFilterColor.uc8[2];
    }
    qwd[id + 8].ui32[0] = 0x10000007;
    qwd[id + 8].ui32[1] = 0;
    qwd[id + 8].ui32[2] = 0;
    qwd[id + 8].ui32[3] = 0x50000007;
    qwd[id + 9].ul64[0] = 0x608B400000008001;
    qwd[id + 9].ul64[1] = 0x513513;
    qwd[id + 10].ul64[0] = 0; qwd[id + 10].ul64[1] = 0;
    qwd[id + 11].ui32[0] = colr;
    qwd[id + 11].ui32[1] = colb;
    qwd[id + 11].ui32[2] = colb;
    qwd[id + 11].ui32[3] = 0x80;
    qwd[id + 12].ul64[0] = (long)(tx + 0x7000) | ((long)(ty + 0x7000) << 32);
    qwd[id + 12].ul64[1] = 0x10;
    qwd[id + 13].ul64[0] = 0x200000002000;
    qwd[id + 13].ul64[1] = 0;
    qwd[id + 14].ui32[0] = colr;
    qwd[id + 14].ui32[1] = colg;
    qwd[id + 14].ui32[2] = colb;
    qwd[id + 14].ui32[3] = 0x80;
    qwd[id + 15].ul64[0] = (long)(tx + 0x9000) | ((long)(ty + 0x9000) << 32);
    qwd[id + 15].ul64[1] = 0x10;
    qwd[id + 16].ul128 = 0;
    qwd[id + 16].ui32[0] = 0x60000000;
    *ppqwd = &qwd[id + 17];
    pfp->FO_timer++;
}

/**
 * Writes one frame of a timed fade and advances its timer.
 * @param mode  0 fade out, 1 fade in
 * @param color 1 black, 0 white
 * @param flg   1 when the fade runs on top of another filter
 * @return 0 once the fade has finished (the filter mode is cleared), else 1
 */
int sh2gfw_Fade2(union Q_WORDDATA **ppqwd, int mode, int color, int flg) {
    int id = 0;
    Q_WORDDATA *qwd = *ppqwd;
    int colordata[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    if (flg == 0) {
        sh2gfw_Filter_JustCopy2(&qwd);
        qwd--;
    }
    if (pfp->Max_Timer >= 0.0f) {
        if (pfp->Max_Timer <= pfp->FO_timer) {
            if (mode == 1 || pfp->Max_Timer == 0) {
                if (flg == 1) {
                    sh2gfw_Filter_Retain(&qwd);
                }
                sh2gfw_Reset_FilterCommand();
                shGsFilterWork.mode = 0;
                return 0;
            }
            pfp->FO_timer = pfp->Max_Timer;
        }
    } else {
        pfp->FO_timer = -pfp->Max_Timer;
    }
    qwd[0].ui32[0] = 0x10000006;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0x50000006;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8005;
    qwd[2].ul64[1] = 0x3F;
    qwd[2].ul64[0] = 0;
    qwd[3].ul64[1] = 0x47;
    qwd[3].ul64[0] = 0x38002;
    qwd[4].ul64[1] = 0x4E;
    qwd[4].ul64[0] = 0x13A0001C0;
    qwd[5].ul64[1] = 8;
    qwd[5].ul64[0] = 5;
    if (color == 0) {
        int ix;

        if (mode == 1) {
            ix = 0x80 - (pfp->FO_timer << 7) / pfp->Max_Timer;
        } else {
            ix = (pfp->FO_timer << 7) / pfp->Max_Timer;
        }
        qwd[6].ul64[1] = 0x42;
        qwd[6].ul64[0] = ((long)ix << 32) | 0x68;
        id += 7;
    } else if (color == 1) {
        int ix;

        if (mode == 1) {
            ix = 0x80 - (pfp->FO_timer << 7) / pfp->Max_Timer;
        } else {
            ix = (pfp->FO_timer << 7) / pfp->Max_Timer;
        }
        qwd[6].ul64[1] = 0x42;
        qwd[6].ul64[0] = ((long)ix << 32) | 0x62;
        id += 7;
    } else {
        int ix;

        if (mode == 1) {
            ix = (pfp->FO_timer << 7) / pfp->Max_Timer;
        } else {
            ix = (pfp->FO_timer << 7) / pfp->Max_Timer;
        }
        qwd[6].ul64[1] = 0x42;
        qwd[6].ul64[0] = ((long)ix << 32) | 0x21;
        id += 7;
    }
    qwd[id].ui32[0] = 0x10000007;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000007; id++;
    qwd[id].ul64[0] = 0x6023400000008001;
    qwd[id].ul64[1] = 0xEE5151; id++;
    qwd[id].ui32[0] = colordata[0];
    qwd[id].ui32[1] = colordata[1];
    qwd[id].ui32[2] = colordata[2];
    qwd[id].ui32[3] = 0x80; id++;
    qwd[id].ul64[0] = 0x700000007000;
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id].ui32[0] = colordata[0];
    qwd[id].ui32[1] = colordata[1];
    qwd[id].ui32[2] = colordata[2];
    qwd[id].ui32[3] = 0x80; id++;
    qwd[id].ul64[0] = 0x900000009000;
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id] = shGs_AllEnv.DefaultEnv[2]; id++;
    qwd[id].ul64[0] = 0x60;
    qwd[id].ul64[1] = 0x14; id++;
    qwd[id].ul128 = 0;
    qwd[id].ui32[0] = 0x60000000; id++;
    *ppqwd = &qwd[id];
    pfp->FO_timer++;
    return 1;
}

/**
 * Writes one frame of a timed fade to an arbitrary color (used for red).
 * @param mode  0 fade out, 1 fade in
 * @param color RGBA
 */
int sh2gfw_Fade3(union Q_WORDDATA **ppqwd, int mode, int *color) {
    int id;
    int ix;
    int tx;
    int ty;
    union Q_WORDDATA *qwd;
    struct FilterParams *pfp;

    id = 0;
    qwd = *ppqwd;
    pfp = sh2gfw_Get_FilterCommandParams();
    qwd[0].ui32[0] = 0x10000005;
    qwd[0].ui32[1] = 0;
    qwd[0].ui32[2] = 0;
    qwd[0].ui32[3] = 0x50000005;
    qwd[1].ui32[3] = 0;
    qwd[1].ui32[2] = 0xE;
    qwd[1].ui32[1] = 0x10000000;
    qwd[1].ui32[0] = 0x8004;
    qwd[2].ul64[1] = 0x3F;
    qwd[2].ul64[0] = 0;
    qwd[3].ul64[1] = 0x47;
    qwd[3].ul64[0] = 0x38002;
    qwd[4].ul64[1] = 0x4E;
    qwd[4].ul64[0] = 0x13A0001C0;
    qwd[5].ul64[1] = 8;
    qwd[5].ul64[0] = 5;
    qwd[6].ui32[0] = 0x10000006;
    qwd[6].ui32[1] = 0;
    qwd[6].ui32[2] = 0;
    qwd[6].ui32[3] = 0x50000006;
    qwd[7].ul64[0] = 0x5003400000008001;
    qwd[7].ul64[1] = 0xE5151;
    qwd[8].ui32[0] = color[0];
    qwd[8].ui32[1] = color[1];
    qwd[8].ui32[2] = color[2];
    qwd[8].ui32[3] = 0x80;
    qwd[9].ul64[0] = 0x700000007000;
    qwd[9].ul64[1] = 0x10;
    qwd[10].ui32[0] = color[0];
    qwd[10].ui32[1] = color[1];
    qwd[10].ui32[2] = color[2];
    qwd[10].ui32[3] = 0x80;
    qwd[11].ul64[0] = 0x900000009000;
    qwd[11].ul64[1] = 0x10;
    qwd[12].ul64[0] = 0x60;
    qwd[12].ul64[1] = 0x14;
    if (pfp->Max_Timer >= 0.0f) {
        if (pfp->Max_Timer <= pfp->FO_timer) {
            if (pfp->Max_Timer == 0) {
                qwd[13].ul128 = 0;
                qwd[13].ui32[0] = 0x60000000;
                return 0;
            }
            pfp->FO_timer = pfp->Max_Timer;
        }
    } else {
        pfp->FO_timer = -pfp->Max_Timer;
    }
    qwd[13].ui32[0] = 0x10000007;
    qwd[13].ui32[1] = 0;
    qwd[13].ui32[2] = 0;
    qwd[13].ui32[3] = 0x50000007;
    qwd[14].ui32[3] = 0;
    qwd[14].ui32[2] = 0xE;
    qwd[14].ui32[1] = 0x10000000;
    qwd[14].ui32[0] = 0x8006;
    qwd[15].ul64[1] = 0x3F;
    qwd[15].ul64[0] = 0;
    qwd[16].ul64[1] = 0x47;
    qwd[16].ul64[0] = 0x38002;
    ix = (pfp->FO_timer << 7) / pfp->Max_Timer;
    if (ix < 0) {
        ix = 0;
    }
    qwd[17].ul64[1] = 0x42;
    qwd[17].ul64[0] = ((long)ix << 32) | 0x64;
    qwd[18].ul64[1] = 6;
    qwd[18].ul64[0] = (unsigned long)sh2gfw_GetTexTBP0(&shGs_AllEnv, 7) | 0x664020000 | ((unsigned long)Env_ctl.CopyFilterColor.ui32[1] << 35);
    if (Env_ctl.mode_buf[0] == 0) {
        qwd[19].ul64[0] = 0x60;
        qwd[19].ul64[1] = 0x14;
        id += 20;
        tx = pfp->base_Ix - 4;
        ty = pfp->base_Iy - 4;
    } else {
        qwd[19].ul64[0] = 0x60;
        qwd[19].ul64[1] = 0x14;
        id += 20;
        ty = tx = 0;
    }
    qwd[id].ul64[1] = 8;
    qwd[id].ul64[0] = 5; id++;
    qwd[id].ui32[0] = 0x10000008;
    qwd[id].ui32[1] = 0;
    qwd[id].ui32[2] = 0;
    qwd[id].ui32[3] = 0x50000008; id++;
    qwd[id].ul64[0] = 0x70AB400000008001;
    qwd[id].ul64[1] = 0xE513513; id++;
    qwd[id].ul64[0] = 0; qwd[id].ul64[1] = 0; id++;
    qwd[id].ui32[0] = Env_ctl.CopyFilterColor.uc8[0];
    qwd[id].ui32[1] = Env_ctl.CopyFilterColor.uc8[1];
    qwd[id].ui32[2] = Env_ctl.CopyFilterColor.uc8[2];
    qwd[id].ui32[3] = Env_ctl.CopyFilterColor.uc8[3]; id++;
    qwd[id].ul64[0] = (long)(tx + 0x7000) | ((long)(ty + 0x7000) << 32);
    qwd[id].ul64[1] = 0x10; id++;
    qwd[id].ul64[0] = 0x200000002000;
    qwd[id].ul64[1] = 0; id++;
    qwd[id].ui32[0] = Env_ctl.CopyFilterColor.uc8[0];
    qwd[id].ui32[1] = Env_ctl.CopyFilterColor.uc8[1];
    qwd[id].ui32[2] = Env_ctl.CopyFilterColor.uc8[2];
    qwd[id].ui32[3] = Env_ctl.CopyFilterColor.uc8[3]; id++;
    qwd[id].ul64[0] = (long)(tx + 0x9000) | ((long)(ty + 0x9000) << 32);
    qwd[id].ul64[1] = 0x10; id++;
    if (Env_ctl.mode_buf[0] == 0) {
        qwd[id].ul64[1] = 0xF;
    } else {
        qwd[id].ul64[0] = 0x60;
        qwd[id].ul64[1] = 0x14;
    }
    id++;
    qwd[id].ul128 = 0;
    qwd[id].ui32[0] = 0x60000000;
    *ppqwd = qwd;
    pfp->FO_timer++;
    return 1;
}

/** Starts a fade to black over the retained frame; ra sets the speed. */
void sh2gfw_Set_FadeOutRetain_Black(float ra) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.GsFilterKind = 0x11;
    shGsFilterWork.mode = shGsFilterWork.GsFilterKind;
    pfp->FIO_ratio = fabs(ra) * 4.27f;
    pfp->TargetSec = 10.0f * ra;
    pfp->FO_timer = 0;
}

/** Starts a fade out to black lasting ra seconds. */
void sh2gfw_Set_FadeOut_Black(float ra) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.GsFilterKind = 0xE;
    shGsFilterWork.mode = shGsFilterWork.GsFilterKind;
    pfp->FIO_ratio = fabs(ra) * 4.27f;
    pfp->TargetSec = ra;
    pfp->FO_timer = 0;
    pfp->Max_Timer = ra * shGetFPS();
}

/** Starts a fade to white over the retained frame; ra sets the speed. */
void sh2gfw_Set_FadeOutRetain_White(float ra) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.GsFilterKind = 0x12;
    shGsFilterWork.mode = shGsFilterWork.GsFilterKind;
    pfp->FIO_ratio = fabs(ra) * 4.27f;
    pfp->TargetSec = 10.0f * ra;
    pfp->FO_timer = 0;
}

/** Starts a fade to red over the retained frame; ra sets the speed. */
void sh2gfw_Set_FadeOutRetain_Red(float ra) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.GsFilterKind = 0x10;
    shGsFilterWork.mode = shGsFilterWork.GsFilterKind;
    pfp->FIO_ratio = fabs(ra) * 4.27f;
    pfp->TargetSec = 10.0f * ra;
    pfp->FO_timer = 0;
}

/** Starts a fade out to white lasting ra seconds. */
void sh2gfw_Set_FadeOut_White(float ra) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.GsFilterKind = 0xF;
    shGsFilterWork.mode = shGsFilterWork.GsFilterKind;
    pfp->FIO_ratio = fabs(ra) * 4.27f;
    pfp->TargetSec = ra;
    pfp->FO_timer = 0;
    pfp->Max_Timer = ra * shGetFPS();
}

/** Starts a fade in from black lasting ra seconds. */
void sh2gfw_Set_FadeIn_Black(float ra) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.mode = shGsFilterWork.GsFilterKind = 0x13;
    pfp->FIO_ratio = fabs(ra) * 4.27f;
    pfp->TargetSec = ra;
    pfp->FO_timer = 0;
    pfp->Max_Timer = ra * shGetFPS();
}

/** Starts a fade in from white lasting ra seconds. */
void sh2gfw_Set_FadeIn_White(float ra) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.mode = shGsFilterWork.GsFilterKind = 0x14;
    pfp->FIO_ratio = fabs(ra) * 4.27f;
    pfp->TargetSec = ra;
    pfp->FO_timer = 0;
    pfp->Max_Timer = ra * shGetFPS();
}

/** Starts a fade in from red lasting ra seconds. */
void sh2gfw_Set_FadeIn_Red(float ra) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.mode = shGsFilterWork.GsFilterKind = 0x15;
    pfp->FIO_ratio = fabs(ra) * 4.27f;
    pfp->TargetSec = ra;
    pfp->FO_timer = 0;
    pfp->Max_Timer = ra * shGetFPS();
}

/** Selects the blur filter with ratio rt. */
void sh2gfw_Set_FilterBlur(int rt) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.GsFilterKind = 2;
    pfp->blurRatio = rt;
}

/** Freezes the displayed frame (filter 0x17), remembering the current filter. */
void sh2gfw_Set_PauseRetain(void) {
    struct FilterParams *pfp;

    if (shGsFilterWork.GsFilterKind != 0x17) {
        pfp = sh2gfw_Get_FilterCommandParams();
        shGsFilterWork.Kind_History = shGsFilterWork.GsFilterKind;
        shGsFilterWork.GsFilterKind = 0x17;
    }
}

/** Ends sh2gfw_Set_PauseRetain, restoring the remembered filter. */
void sh2gfw_Reset_PauseRetain(void) {
    struct FilterParams *pfp;

    if (shGsFilterWork.GsFilterKind == 0x17) {
        pfp = sh2gfw_Get_FilterCommandParams();
        shGsFilterWork.GsFilterKind = shGsFilterWork.Kind_History;
        shGsFilterWork.Kind_History = 0x17;
    }
}

/** Clears the filter command and parameters, keeping the fade mode and the capture flag. */
void sh2gfw_Reset_FilterCommand(void) {
    int mm;
    int kk;
    int ss;
    struct FilterParams *pfp;

    mm = shGsFilterWork.mode;
    kk = shGsFilterWork.GsFilterKind;
    pfp = sh2gfw_Get_FilterCommandParams();
    ss = pfp->sw_flg;
    memset(&shGsFilterWork, 0, sizeof(shGsFilterWork));
    shGsFilterWork.mode = mm;
    shGsFilterWork.Kind_History = kk;
    pfp->sw_flg = ss;
}

/** Returns the current filter command. */
int sh2gfw_Get_FilterCommand(void) {
    return shGsFilterWork.GsFilterKind;
}

/** Returns the filter parameters (struct FilterParams). */
void *sh2gfw_Get_FilterCommandParams(void) {
    return shGsFilterWork.FilterData;
}

/** Requests a capture of the current frame buffer into the work buffer (filter 0x16). Returns 0x3400. */
int sh2gfw_Set_CaptureNowFB(void) {
    struct FilterParams *pfp;

    pfp = sh2gfw_Get_FilterCommandParams();
    shGsFilterWork.GsFilterKind = 0x16;
    pfp->sw_flg = 1;
    return 0x3400;
}

/** Calls the current stage's post-draw hook, if it has one. */
void Exec_PostDraw_OVfunc(void) {
    if (stage && stage->gfw_func && stage->gfw_func->PostDraw) {
        stage->gfw_func->PostDraw();
    }
}

/** Returns whether a soft-focus filter (commands 5-10) is active with no fade running. */
int Check_Filter_Soft(void) {
    if (shGsFilterWork.GsFilterKind >= 5 && shGsFilterWork.GsFilterKind < 0xB) {
        return !shGsFilterWork.mode;
    }
    return 0;
}

/**
 * Draws two horizontal bands at fixed screen positions (KARI_OBI: "provisional bands").
 * The arguments are not used.
 */
void DemoFadeDraw2(int a, int b, int c, int d) {
    static union Q_WORDDATA KARI_OBI[16];
    int id;

    id = 0;
    KARI_OBI[id].ui32[0] = 0x1000000A;
    KARI_OBI[id].ui32[1] = 0;
    KARI_OBI[id].ui32[2] = 0;
    KARI_OBI[id].ui32[3] = 0x5000000A;
    KARI_OBI[id + 1].ui32[3] = 0;
    KARI_OBI[id + 1].ui32[2] = 0xE;
    KARI_OBI[id + 1].ui32[1] = 0x10000000;
    KARI_OBI[id + 1].ui32[0] = 0x8001;
    KARI_OBI[id + 2].ul64[1] = 0x47;
    KARI_OBI[id + 2].ul64[0] = 0x3001D;
    KARI_OBI[id + 3].ul64[0] = 0x3003400000008001;
    KARI_OBI[id + 3].ul64[1] = 0x551;
    KARI_OBI[id + 4].ui32[0] = 0;
    KARI_OBI[id + 4].ui32[1] = 0;
    KARI_OBI[id + 4].ui32[2] = 0;
    KARI_OBI[id + 4].ui32[3] = 2;
    KARI_OBI[id + 5].ul64[0] = 0x700000007000;
    KARI_OBI[id + 5].ul64[1] = 0x10;
    KARI_OBI[id + 6].ul64[0] = 0x740000009000;
    KARI_OBI[id + 6].ul64[1] = 0x10;
    KARI_OBI[id + 7].ul64[0] = 0x3003400000008001;
    KARI_OBI[id + 7].ul64[1] = 0x551;
    KARI_OBI[id + 8].ui32[0] = 0;
    KARI_OBI[id + 8].ui32[1] = 0;
    KARI_OBI[id + 8].ui32[2] = 0;
    KARI_OBI[id + 8].ui32[3] = 2;
    KARI_OBI[id + 9].ul64[0] = 0x900000007000;
    KARI_OBI[id + 9].ul64[1] = 0x10;
    KARI_OBI[id + 10].ul64[0] = 0x8C0000009000;
    KARI_OBI[id + 10].ul64[1] = 0x10;
    KARI_OBI[id + 11].ui32[0] = 0x70000000;
    KARI_OBI[id + 11].ui32[1] = 0;
    KARI_OBI[id + 11].ui32[2] = 0;
    KARI_OBI[id + 11].ui32[3] = 0;
    d1cSend(KARI_OBI);
}

/**
 * Writes the frame's filter packets for the current filter command (capturing the frame
 * first if requested), then a fade in on top if one is running.
 * @param pb   packet buffer
 * @param mode not used
 */
void Make_Filter_Packet(void *pb, int mode) {
    Q_WORDDATA *qwd;
    struct FilterParams *pfp;

    qwd = pb;
    pfp = sh2gfw_Get_FilterCommandParams();
    if (pfp->sw_flg) {
        d2sSync(0, -1);
        d1sSync(0, -1);
        sh2gfw_Copy_FrameToWork(&qwd);
        pfp->sw_flg = 0;
    }
    switch (shGsFilterWork.GsFilterKind) {
    case 5:
        sh2gfw_Filter_JustCopy2(&qwd);
        qwd--;
        sh2gfw_Swap_Soft(&qwd, pfp->SoftIter, 0x80, pfp->SoftShift, pfp->SoftAref, pfp->SoftCit, 0);
        break;
    case 6:
    case 7:
        sh2gfw_Filter_JustCopy2(&qwd);
        qwd--;
        sh2gfw_Swap_GlowSoft(&qwd, pfp->SoftIter, pfp->SoftCit, pfp->SoftAref, pfp->SoftShift);
        break;
    default:
    case 0:
    case 1:
    case 11:
    case 12:
    case 13:
        sh2gfw_Filter_JustCopy2(&qwd);
        break;
    case 2:
        sh2gfw_Filter_JustCopy2(&qwd);
        qwd--;
        sh2gfw_Filter_Blur(&qwd, (unsigned char)pfp->blurRatio);
        break;
    case 3:
        sh2gfw_Filter_JustCopy2(&qwd);
        qwd--;
        sh2gfw_Filter_Glow_Blur(&qwd, (unsigned char)(pfp->GreaterA + 0xE0), (unsigned char)pfp->blurRatio, 0);
        break;
    case 4:
        sh2gfw_Filter_JustCopy2(&qwd);
        qwd--;
        sh2gfw_Filter_Dark_Blur(&qwd, (unsigned char)pfp->blurRatio, (unsigned char)(pfp->LesserA + 0x7F));
        break;
    case 14:
        sh2gfw_Fade2(&qwd, 0, 1, 0);
        break;
    case 17:
        sh2gfw_FadeOut_Retain(&qwd, 1);
        break;
    case 15:
        sh2gfw_Fade2(&qwd, 0, 0, 0);
        break;
    case 18:
        sh2gfw_FadeOut_Retain(&qwd, 0);
        break;
    case 16:
        sh2gfw_FadeOut_Retain(&qwd, 2);
        break;
    case 19:
        pfp->base_Ix = 0;
        pfp->base_Iy = 0;
        sh2gfw_Fade2(&qwd, 1, 1, 0);
        break;
    case 20:
        pfp->base_Ix = 0;
        pfp->base_Iy = 0;
        sh2gfw_Fade2(&qwd, 1, 0, 0);
        break;
    case 21:
        pfp->base_Ix = 0;
        pfp->base_Iy = 0;
        {
            int col[4] = { 0xFF, 0, 0, 0 };

            sh2gfw_Fade3(&qwd, 1, col);
        }
        break;
    case 23:
        sh2gfw_Filter_Retain(&qwd);
        break;
    case 8:
    case 9:
    case 10:
        sh2gfw_Filter_JustCopy2(&qwd);
        qwd--;
        sh2gfw_Swap_Soft(&qwd, pfp->S1_iter, 0x80, pfp->S1_shift, pfp->S1_alpha, pfp->KeyAlpha + pfp->TrimAlpha, 1);
        sh2gfw_Swap_Soft(&qwd, pfp->S2_iter, 0x80, pfp->S2_shift, pfp->S2_alpha, pfp->KeyAlpha + pfp->TrimAlpha, 2);
        break;
    case 22:
        sh2gfw_Filter_JustCopy2(&qwd);
        qwd--;
        sh2gfw_Reset_FilterCommand();
        break;
    }
    if (shGsFilterWork.mode >= 0x13 && shGsFilterWork.mode < 0x16 && shGsFilterWork.GsFilterKind < 0xE && shGsFilterWork.GsFilterKind != 0) {
        switch (shGsFilterWork.mode) {
        case 0x13:
            sh2gfw_Fade2(&qwd, 1, 1, 1);
            break;
        case 0x14:
            sh2gfw_Fade2(&qwd, 1, 0, 1);
            break;
        case 0x15: {
            int col[4] = { 0xFF, 0, 0, 0 };

            sh2gfw_Fade3(&qwd, 1, col);
            break;
        }
        }
    }
}

