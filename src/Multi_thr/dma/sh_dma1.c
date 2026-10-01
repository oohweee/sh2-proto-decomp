/*
 * sh_dma1.c: DMA channel 1 (VIF1) transfer driver.
 *
 * Sends source-chain packets to VIF1 and blocks until the transfer ends.
 *
 * Matching: verbose() messages bake "sh_dma1.c:<line>" into their format
 * strings, so they must stay on lines 87, 163 and 169-173.
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "lib/sh_kernel.h"

/* Matching: #line keeps the original numbering (register defines moved to eeregs.h, declarations to headers). */
#line 36

#define POS __FILE__ ":" SH_STRINGIFY(__LINE__) "> "

/* Semaphore guarding the channel: one transfer at a time. */
static int excl_sid = -1;
/* Semaphore the DMAC interrupt handler signals when a transfer ends. */
static int wait_sid = -1;
/* DMAC handler id from AddDmacHandler(). */
static int dmac_hid = -1;

/* Progress marker of shDmaVif1Send(), for inspecting a hang in the debugger. */
static int AA1_check;

static int shDmaVif1Handler(void) {
    iSignalSemaLimit(wait_sid);
    ExitHandler();
    return 1;
}

/*
 * Stops channel 1: suspend the DMAC through D_ENABLEW, clear STR in D1_CHCR,
 * then restore the previous suspend state.
 */
static void shDmaVif1Halt(void) {
    unsigned int chcr;
    unsigned int enable;

    enable = *D_ENABLER;
    *D_ENABLEW = enable | 0x10000;
    chcr = *D1_CHCR;
    *D1_CHCR = chcr & ~0x100;
    *D_ENABLEW = enable;
}

/*
 * Busy-waits until channel 1 has stopped. Returns how many times timer 3
 * ticked meanwhile (0 when the channel was already idle).
 */
static int shDmaVif1Wait(void) {
    int count;
    unsigned short h0;
    unsigned short h1;

    count = 0;
    h1 = *T3_COUNT;
    while (*D1_CHCR & 0x100) {
        h0 = h1;
        h1 = *T3_COUNT;
        if (h0 != h1) {
            count++;
            if (count % 263 == 0) {
                verbose(3, POS "DMA-1 busy...\n");
            }
        }
    }
    return count;
}

/**
 * Sets up channel 1: creates the semaphores on first use, halts the channel,
 * clears its interrupt status, and installs the DMAC handler.
 *
 * Calling it again later resets the channel; the exclusion semaphore keeps a
 * transfer that is still running from being cut off.
 *
 * Returns 1 when the handler is installed, 0 otherwise.
 */
int shDmaVif1Init(void) {
    unsigned int chcr;
    int ret;

    if (excl_sid == -1) {
        excl_sid = CreateSema2(0, 1, NULL);
    } else {
        WaitSema(excl_sid);
    }
    if (wait_sid == -1) {
        wait_sid = CreateSema2(0, 1, NULL);
    }

    shDmaVif1Halt();
    *D_STAT = 2;
    chcr = 0x40;
    *D1_CHCR = chcr;
    if (excl_sid != -1 && wait_sid != -1 && dmac_hid == -1) {
        dmac_hid = AddDmacHandler(DMAC_VIF1, shDmaVif1Handler, -1);
        EnableDmac(DMAC_VIF1);
    }

    ret = dmac_hid != -1;
    if (ret) {
        SignalSemaLimit(excl_sid);
    }
    return ret;
}

/**
 * Sends the source-chain DMA list at `packet` to VIF1 and waits for it to end.
 *
 * The transfer starts with the tag at `packet` (D1_TADR, QWC 0) in chain mode
 * with TTE and TIE set; the handler signals wait_sid when it ends. packet must
 * be 16-byte aligned and already written back from the data cache.
 *
 * AA1_check records how far a call got:
 *   1  waiting for the channel      2  channel acquired
 *   3  transfer started             4  transfer ended
 *   5  status checked               6  channel released
 *
 * Always returns 1. Before shDmaVif1Init() has installed the handler it does
 * nothing.
 */
int shDmaVif1Send(void *packet) {
    unsigned int chcr;

    if (dmac_hid != -1) {
        AA1_check = 1;
        WaitSema(excl_sid);
        AA1_check = 2;
        *D1_TADR = (unsigned int)packet;
        *D1_QWC = 0;
        chcr = *D1_CHCR;
        PollSemaAll(wait_sid);
        *D1_CHCR = (chcr & ~0xD) | 0x105;
        AA1_check = 3;
        WaitSema(wait_sid);
        AA1_check = 4;
        if (shDmaVif1Wait() > 0) {
            verbose(3, POS "why wait for stop DMA-1 ?\n");
        }

        chcr = *D1_CHCR;
        if ((chcr & 0x70000000) != 0x70000000) {
            verbose(3,
                    POS "DMA-1 don't normally finished.\n"
                    POS "    D1_CHCR: 0x%08x\n"
                    POS "    D1_TADR: 0x%08x\n"
                    POS "    D1_MADR: 0x%08x\n"
                    POS "    D1_QWC : 0x%08x\n",
                    chcr, *D1_TADR, *D1_MADR, *D1_QWC);
        }
        AA1_check = 5;
        SignalSemaLimit(excl_sid);
        AA1_check = 6;
    }
    return 1;
}
