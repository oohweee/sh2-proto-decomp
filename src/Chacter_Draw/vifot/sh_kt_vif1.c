/*
 * VIF1 DMA channel setup (Chacter_Draw/vifot).
 */
#include "sh2.h"
#include "libc/string.h"
#include "sdk/libdma.h"

static struct Vif1Work vif1_work;

/** Clears the VIF1 work and gets the DMA channel 1 handle. */
void ktVif1Init(void) {
    memset(&vif1_work, 0, sizeof(vif1_work));
    vif1_work.dma = sceDmaGetChan(1);
}
