/*
 * sh_dma2.c: DMA channel 2 (GIF) transfer driver.
 *
 * Matching: verbose() messages bake "sh_dma2.c:<line>" into their format
 * strings, so they must stay on lines 83 and 165-169.
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "lib/sh_kernel.h"

/* Matching: #line keeps the original numbering (register defines moved to eeregs.h, declarations to headers). */
#line 35

#define POS __FILE__ ":" SH_STRINGIFY(__LINE__) "> "

static int excl_sid = -1; /* one transfer at a time */
static int wait_sid = -1; /* signalled by the handler when a transfer ends */
static int dmac_hid = -1; /* from AddDmacHandler() */

/* Progress marker of shDmaGifSend(), for inspecting a hang in the debugger. */
static int AA2_check;

static int shDmaGifHandler(void) {
    iSignalSemaLimit(wait_sid);
    ExitHandler();
    return 1;
}

/*
 * Stops channel 2: suspend the DMAC through D_ENABLEW, clear STR in D2_CHCR,
 * then restore the previous suspend state.
 */
static void shDmaGifHalt(void) {
    unsigned int chcr;
    unsigned int enable;

    enable = *D_ENABLER;
    *D_ENABLEW = enable | 0x10000;
    chcr = *D2_CHCR;
    *D2_CHCR = chcr & ~0x100;
    *D_ENABLEW = enable;
}

/*
 * Busy-waits until channel 2 has stopped. Returns how many times timer 3
 * ticked meanwhile (0 when the channel was already idle).
 */
static int shDmaGifWait(void) {
    int count;
    unsigned short h0;
    unsigned short h1;

    count = 0;
    h1 = *T3_COUNT;
    while (*D2_CHCR & 0x100) {
        h0 = h1;
        h1 = *T3_COUNT;
        if (h0 != h1) {
            count++;
            if (count % 263 == 0) {
                verbose(3, POS "DMA-2 busy...\n");
            }
        }
    }
    return count;
}

/**
 * Sets up channel 2: creates the semaphores on first use, halts the channel,
 * clears its interrupt status, and installs the DMAC handler.
 *
 * Calling it again later resets the channel; the exclusion semaphore keeps a
 * transfer that is still running from being cut off.
 *
 * Returns 1 when the handler is installed, 0 otherwise.
 */
int shDmaGifInit(void) {
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

    shDmaGifHalt();
    *D_STAT = 4;
    chcr = 0x40;
    *D2_CHCR = chcr;
    if (excl_sid != -1 && wait_sid != -1 && dmac_hid == -1) {
        dmac_hid = AddDmacHandler(DMAC_GIF, shDmaGifHandler, -1);
        EnableDmac(DMAC_GIF);
    }

    ret = dmac_hid != -1;
    if (ret) {
        SignalSemaLimit(excl_sid);
    }
    return ret;
}

/**
 * Sends the source-chain DMA list at `packet` to the GIF and waits for it to end.
 *
 * The transfer starts with the tag at `packet` (D2_TADR, QWC 0) in chain mode
 * with TTE and TIE set; the handler signals wait_sid when it ends. packet must
 * be 16-byte aligned and already written back from the data cache.
 *
 * AA2_check records how far a call got:
 *   1  waiting for the channel      2  channel acquired
 *   3  transfer started             4  transfer ended
 *   5  status checked               6  channel released
 *
 * Always returns 1; does nothing before shDmaGifInit().
 */
int shDmaGifSend(void *packet) {
    unsigned int chcr;

    if (dmac_hid != -1) {
        AA2_check = 1;
        WaitSema(excl_sid);
        AA2_check = 2;
        *D2_TADR = (unsigned int)packet;
        *D2_QWC = 0;
        chcr = *D2_CHCR;
        FlushCache(0);
        PollSemaAll(wait_sid);
        *D2_CHCR = (chcr & ~0xD) | 0x105;
        AA2_check = 3;
        WaitSema(wait_sid);
        AA2_check = 4;
        if (shDmaGifWait() > 0) {
            verbose(3, "why wait for stop DMA-2 ?\n");
        }

        chcr = *D2_CHCR;
        if ((chcr & 0x70000000) != 0x70000000) {
            verbose(3,
                    POS "DMA-2 don't normally finished.\n"
                    POS "    D2_CHCR: 0x%08x\n"
                    POS "    D2_TADR: 0x%08x\n"
                    POS "    D2_MADR: 0x%08x\n"
                    POS "    D2_QWC : 0x%08x\n",
                    chcr, *D2_TADR, *D2_MADR, *D2_QWC);
        }
        AA2_check = 5;
        SignalSemaLimit(excl_sid);
        AA2_check = 6;
    }
    return 1;
}
