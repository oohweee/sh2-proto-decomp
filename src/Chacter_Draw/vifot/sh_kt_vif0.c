/*
 * VIF0 DMA channel control (Chacter_Draw/vifot): waits for and kicks VU0 packet transfers.
 */
#include "sh2.h"
#include "libc/string.h"
#include "sdk/libdma.h"

static struct Vif0Work vif0_work;

/** Clears the VIF0 work and gets the DMA channel 0 handle. */
void ktVif0Init(void) {
    memset(&vif0_work, 0, sizeof(vif0_work));
    vif0_work.dma = sceDmaGetChan(0);
}

/** Waits until the VIF0 DMA channel is idle. */
void ktVif0Wait(void) {
    while (sceDmaSync(vif0_work.dma, 0, 0)) {
    }
}

/** Waits for the VIF0 DMA channel, then for VIF0 itself to go idle (VIF0_STAT). */
void ktVif0Wait2(void) {
    ktVif0Wait();
    while (*VIF0_STAT & 0xF000003) {
    }
}

/**
 * Sends a DMA chain to VIF0 once the channel is idle.
 * @param p   the DMA chain
 * @param tte 1 to transfer the DMA tags too (CHCR.TTE)
 */
void ktVif0Send(void *p, int tte) {
    ktVif0Wait();
    vif0_work.dma->chcr.TTE = tte;
    sceDmaSend(vif0_work.dma, p);
}
