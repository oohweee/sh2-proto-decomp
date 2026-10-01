/*
 * Character ordering table (Chacter_Draw): a double-buffered two-level VIF1 ordering table
 * plus a packet buffer, into which the VU0 renderer sorts each character's GS packets by depth.
 */
#include "sh2.h"

/* Matching: .bss statics come out in reverse declaration order. */
static struct sh2gfw_CharacterOT_Man ChrOT_Man_dblbuf[2];
struct sh2gfw_CharacterOT_Man *pChrOT_Man;
/* Matching: the original buffer starts on a 128-byte boundary (0x438600 after a 4-byte int at 0x438588). */
static u_long128 CharacterOT_Data[2][10240] __attribute__((aligned(128)));
static int otbuffer_page;

/** Sets up both pages of the character ordering table and packet buffer. */
void sh2gfw_AllInit_CharacterOT(void) {
    int i;
    struct ktVif1Ot2 *ot;

    for (i = 0; i < 2; i++) {
        ot = &ChrOT_Man_dblbuf[i].vif1ot2;
        ktVif1Ot2Init(ot, 6, ChrOT_Man_dblbuf[i].vif1ot2_top_1, 6, ChrOT_Man_dblbuf[i].vif1ot2_top_2);
        ChrOT_Man_dblbuf[i].pp_top = ChrOT_Man_dblbuf[i].pp_end = CharacterOT_Data[i];
        ChrOT_Man_dblbuf[i].pp_max = ChrOT_Man_dblbuf[i].pp_top + sizeof(CharacterOT_Data[0]) / sizeof(u_long128);
        ChrOT_Man_dblbuf[i].buf_step = 0;
        ChrOT_Man_dblbuf[i].used_qwc = 0;
        ChrOT_Man_dblbuf[i].tmp_qwc = 0;
    }
}

/** Starts a character: resets the current page's table and packet buffer, then flips pages for the next one. */
void sh2gfw_Init_CharacterOT(void) {
    struct ktVif1Ot2 *ot;

    pChrOT_Man = &ChrOT_Man_dblbuf[otbuffer_page];
    ot = &pChrOT_Man->vif1ot2;
    ktVif1Ot2Init(ot, 6, pChrOT_Man->vif1ot2_top_1, 6, pChrOT_Man->vif1ot2_top_2);
    pChrOT_Man->pp_top = pChrOT_Man->pp_end = CharacterOT_Data[otbuffer_page];
    pChrOT_Man->pp_max = pChrOT_Man->pp_top + sizeof(CharacterOT_Data[0]) / sizeof(u_long128);
    pChrOT_Man->buf_step = 0;
    pChrOT_Man->used_qwc = 0;
    pChrOT_Man->tmp_qwc = 0;
    otbuffer_page ^= 1;
}

/** Returns the current ordering table. */
struct ktVif1Ot2 *CharacterOt_KtVif1Ot2(void) {
    return &pChrOT_Man->vif1ot2;
}

/**
 * Allocates qwc quadwords from the current packet buffer.
 * @return the packet, or NULL (with a message) if the buffer is full
 */
u_long128 *CharacterOt_RequestPacket(int qwc) {
    u_long128 *pp;

    pp = pChrOT_Man->pp_end;
    if (pp + qwc > pChrOT_Man->pp_max) {
        printf("CharacterOT: over\n");
        return NULL;
    }
    pChrOT_Man->pp_end += qwc;
    pChrOT_Man->used_qwc += qwc;
    return pp;
}

/** Links packet into the current ordering table at depth (0-0xFFF). */
void CharacterOt_Append(unsigned int depth, u_long128 *packet) {
    struct ktVif1Ot2 *ot;

    ot = &pChrOT_Man->vif1ot2;
    ktVif1Ot2Append(ot, depth, packet);
}

/** Returns whether nothing was allocated since the table was reset. */
int CharacterOt_IsEmpty(void) {
    return pChrOT_Man->used_qwc == pChrOT_Man->tmp_qwc;
}

/** Sorts the current ordering table into its sendable second level. */
void CharacterOt_ExecPre(void) {
    struct ktVif1Ot2 *ot;

    ot = &pChrOT_Man->vif1ot2;
    ktVif1Ot2KickPre(ot);
}

/** Sends the sorted ordering table on DMA channel 1 (sh2_Model_SyncOT, which returns no value). */
int CharacterOt_ExecPost(void) {
    struct ktVif1Ot2 *ot;

    ot = &pChrOT_Man->vif1ot2;
    return sh2_Model_SyncOT(ot->top_2);
}
