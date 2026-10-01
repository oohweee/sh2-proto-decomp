/* fj_man.c: font/effect setup and the per-frame font draw kick. */
#include "sh2.h"

struct sh2gfw_Effect_Man FjFontTexMan;
u_long128 FjDebugEnvData[4] __attribute__((aligned(64)));

/** Initializes the font, the enemy effects and the font texture, and builds the debug draw
 * environment packet. */
void fjInitAll(void) {
    fontClear();
    enEfctInit();
    enEfctTexInit();
    fjInitFontTexture();
    spkStartPacketS(FjDebugEnvData);
    spkOpenDGiftagS(0x1000000000000000, 0xE);
    *spack.pos++ = 0x44;
    *spack.pos++ = 0x42;
    *spack.pos++ = 0xFFFFFFFF00000061;
    *spack.pos++ = 0x14;
    *spack.pos++ = 0x13A0001C0;
    *spack.pos++ = 0x4E;
    spkCloseGiftagS();
    spkEndPacketS();
    spkSetEnvPacket(FjDebugEnvData, 4);
}

/** Fills in the texture and CLUT headers of the font (512x512, 4-bit) and registers it with the
 * texture transfer manager. */
void fjInitFontTexture(void) {
    struct sh2gfw_TEX_HEAD *pTH;
    struct sh2gfw_CLUTS_HEAD *pCH;

    pTH = (struct sh2gfw_TEX_HEAD *)font.tex_head;
    pCH = (struct sh2gfw_CLUTS_HEAD *)font.clut_head;
    pTH->texture_no = 0x33333334;
    pTH->w = 0x200;
    pTH->h = 0x200;
    pTH->color = 4;
    pTH->padbyte = 0x50;
    pTH->sendpsm = 0x14;
    pTH->drawpsm = 0x14;
    pTH->bitshift = 0;
    pTH->bitw = 9;
    pTH->bith = 9;
    pTH->datasize = 0x20000;
    pCH->clutssize = 0x20;
    pCH->clutamount = 1;
    pCH->clw = 8;
    pCH->clh = 1;
    shQzero(&FjFontTexMan, sizeof(FjFontTexMan));
    FjFontTexMan.pTexHead = pTH;
    FjFontTexMan.pTexMAN = sh2gfw_set_TexToTrasMan(&AllTexSync_Man, pTH, pCH, &FjFontTexMan, 0xDD00);
    FjFontTexMan.valid_id = 0xEF04;
}

/** Runs the effect tasks (task list 5). */
void fjMoveEffect(void) {
    shTSKExecuteTask(5);
}

/** Per-frame draw of the enemy effects and the radar; a button combination (after clearing the
 * game) cycles the radar mode. On debug pad port 6, also the packet debug print. */
void fjDrawExec(void) {
    enEfctDraw();
    if (playing.clear_end_number && shPadPress(0, 0x40000) && shPadTrigger(0, 1)) {
        if (++playing.radar > 4) {
            playing.radar = 0;
        }
    }
    enDrawRadar();
    if (shPadGetPort() == 6) {
        spkDebugPrint();
    }
}

/** Sends the font texture (through the transfer thread) and the font packets for this frame. */
void fjFontDrawExec(void) {
    void *adr1;
    void *adr2;

    sh2gfw_Thr_d2TextureSend(FjFontTexMan.pTexMAN, 1, &FjFontTexMan.thr_cid, &FjFontTexMan.thr_sid);
    font.tex0 = *(unsigned long *)sh2gfw_Get_RegTEX0(FjFontTexMan.pTexMAN, 0, 1);
    adr1 = fontFlush();
    adr2 = mfontFlush();
    d1tscSync(FjFontTexMan.thr_cid);
    d1cSend(adr1);
    d1cSend(adr2);
    d1tscFinishToUseSlot(FjFontTexMan.thr_sid);
    d1cSend(fontAfterEnv());
}

/** Sends the font texture and the font packets directly over VIF1. */
void fjFontDrawExecVif1(void) {
    void *adr1;
    void *adr2;

    d1cSend(fontTexLoad(sh2gfw_Get_BaseTBP0for2D(), 0x3600));
    adr1 = fontFlush();
    adr2 = mfontFlush();
    d1cSend(adr1);
    d1cSend(adr2);
    d1cSend(fontAfterEnv());
}

/** Assertion failure handler: prints the failed expression and where, then breaks.
 * @param file source file
 * @param line source line
 * @param str the expression
 * @return 1 */
int fjAssert_(char *file, int line, char *str) {
    printf("assertion \"%s\" failed: file \"%s\", line %d\n", str, file, line);
    asm { .word 0x0000000D }
    return 1;
}
