/*
 * pss_vibuf.c: the movie's video input buffer (the viBuf* functions): a
 * ring of 2 KB blocks that DMA channel 4 (toIPU) streams from through a
 * ring of DMA tags, plus the time stamps of the stream data in it.
 */

#include "sh2.h"
#include "sdk/eekernel.h"

#define DMA_SUSPEND_BIT 0x00010000
#define CHCR_STR_BIT 0x00000100

#define DMATAG_REFE 0
#define DMATAG_NEXT 2
#define DMATAG_REF 3

#define TS_UNSET (-1)

#define VIBUF_BLOCK_SIZE 2048

/* Matching: an inline function (name ours), not a macro: as a call argument it is evaluated first. */
static inline void *dmaAddr(void *val) {
    return (void *)((unsigned int)val & 0x0FFFFFFF);
}

#define IN_RING(i, start, len, n) \
    ((0 <= (((i) + (n) - (start)) % (n))) && ((((i) + (n) - (start)) % (n)) < (len)))

static inline int tsInRing(int tgt, int pos, int len, int size) {
    return (tgt + size - pos) % size < len;
}

#define min(a, b) ((a) < (b) ? (a) : (b))

/**
 * Returns the index of the buffer element whose DMA tag is at `addr` (0 for the terminating tag).
 */
int getFIFOindex(ViBuf *f, void *addr) {
    if (addr == dmaAddr(f->tag + (f->n + 1))) {
        return 0;
    } else {
        return ((unsigned int)addr - (unsigned int)f->data) / VIBUF_BLOCK_SIZE;
    }
}

/** Writes `val` to DMA channel 3 (IPU out) CHCR with the DMAC suspended. */
void setD3_CHCR(unsigned int val) {
    DIntr();
    *D_ENABLEW = *D_ENABLER | DMA_SUSPEND_BIT;
    *D3_CHCR = val;
    *D_ENABLEW = *D_ENABLER & ~DMA_SUSPEND_BIT;
    EIntr();
}

/** Writes `val` to DMA channel 4 (IPU in) CHCR with the DMAC suspended. */
void setD4_CHCR(unsigned int val) {
    DIntr();
    *D_ENABLEW = *D_ENABLER | DMA_SUSPEND_BIT;
    *D4_CHCR = val;
    *D_ENABLEW = *D_ENABLER & ~DMA_SUSPEND_BIT;
    EIntr();
}

/** Writes a source-chain DMA tag (`id`, `qwc` qwords at `addr`) into `q`. */
void scTag2(PssQword *q, void *addr, unsigned int id, unsigned int qwc) {
    q->l[0] = (unsigned long)(unsigned int)addr << 32 | (unsigned long)id << 28 | (unsigned long)qwc;
}

/**
 * Sets up video input buffer `f`: `size` elements of data at `data` with the DMA tags `tag`, and
 * `n_ts` time stamp slots at `ts`. Returns 1.
 */
int viBufCreate(ViBuf *f, u_long128 *data, u_long128 *tag, int size, ViBufTs *ts, int n_ts) {
    struct SemaParam param;

    f->data = data;
    f->tag = (u_long128 *)((unsigned int)dmaAddr(tag) | 0x20000000);
    f->n = size;
    f->buffSize = size * VIBUF_BLOCK_SIZE;

    f->ts = ts;
    f->n_ts = n_ts;

    param.initCount = 1;
    param.maxCount = 1;
    f->sema = CreateSema(&param);

    viBufReset(f);
    f->totalBytes = 0;

    return 1;
}

/** Empties `f` and rebuilds its DMA tag ring. Returns 1. */
int viBufReset(ViBuf *f) {
    int i;

    f->dmaStart = 0;
    f->dmaN = 0;
    f->readBytes = 0;
    f->isActive = 1;
    f->count_ts = 0;
    f->wt_ts = 0;

    for (i = 0; i < f->n_ts; i++) {
        f->ts[i].pts = TS_UNSET;
        f->ts[i].dts = TS_UNSET;
        f->ts[i].pos = 0;
        f->ts[i].len = 0;
    }

    for (i = 0; i < f->n; i++) {
        scTag2((PssQword *)(f->tag + i), dmaAddr((char *)f->data + VIBUF_BLOCK_SIZE * i), DMATAG_REF, VIBUF_BLOCK_SIZE / 16);
    }
    scTag2((PssQword *)(f->tag + i), dmaAddr(f->tag), DMATAG_NEXT, 0);

    *D4_QWC = 0;
    *D4_MADR = (unsigned int)dmaAddr(f->data);
    *D4_TADR = (unsigned int)dmaAddr(f->tag);
    setD4_CHCR((1 << 2) | 1);

    return 1;
}

/** Returns in `ptr0`/`len0` and `ptr1`/`len1` the free space of `f` (two parts when it wraps). */
void viBufBeginPut(ViBuf *f, unsigned char **ptr0, int *len0, unsigned char **ptr1, int *len1) {
    int es;
    int en;
    int fs;
    int fn;

    WaitSema(f->sema);

    fs = (f->dmaStart + f->dmaN) * VIBUF_BLOCK_SIZE;
    fn = (f->n - 2 - f->dmaN) * VIBUF_BLOCK_SIZE;
    es = (fs + f->readBytes) % f->buffSize;
    en = fn - f->readBytes;

    if (f->buffSize - es >= en) {
        *ptr0 = (unsigned char *)f->data + es;
        *len0 = en;
        *ptr1 = NULL;
        *len1 = 0;
    } else {
        *ptr0 = (unsigned char *)f->data + es;
        *len0 = f->buffSize - es;
        *ptr1 = (unsigned char *)f->data;
        *len1 = en - (f->buffSize - es);
    }

    SignalSema(f->sema);
}

/** Commits `size` bytes written after viBufBeginPut(). */
void viBufEndPut(ViBuf *f, int size) {
    WaitSema(f->sema);
    f->readBytes += size;
    f->totalBytes += size;
    SignalSema(f->sema);
}

/**
 * Hands the complete elements written so far to the IPU input DMA (channel 4), starting it if it is
 * idle. Returns whether new data was added.
 */
int viBufAddDMA(ViBuf *f) {
    int i;
    int index;
    int id;
    int last;
    unsigned int d4chcr;
    int isNewData;
    int consume;
    int read_start;
    int read_n;

    isNewData = 0;
    WaitSema(f->sema);

    if (!f->isActive) {
        ErrMessage("DMA ADD not active\n");
        return 0;
    }

    setD4_CHCR((1 << 2) | 1);
    d4chcr = *D4_CHCR;
    index = getFIFOindex(f, (void *)*D4_MADR);

    consume = (index + f->n - f->dmaStart) % f->n;
    f->dmaStart = (f->dmaStart + consume) % f->n;
    f->dmaN -= consume;

    read_start = (f->dmaStart + f->dmaN) % f->n;
    read_n = f->readBytes / VIBUF_BLOCK_SIZE;
    f->readBytes %= VIBUF_BLOCK_SIZE;

    if (read_n > 0) {
        last = (f->n + (f->dmaStart + f->dmaN - 1)) % f->n;
        scTag2((PssQword *)(f->tag + last), (char *)f->data + VIBUF_BLOCK_SIZE * last, DMATAG_REF, VIBUF_BLOCK_SIZE / 16);
        isNewData = 1;
    }

    for (i = 0; i < read_n; i++) {
        id = (i == read_n - 1) ? DMATAG_REFE : DMATAG_REF;
        scTag2((PssQword *)(f->tag + read_start), (char *)f->data + VIBUF_BLOCK_SIZE * read_start, id,
               VIBUF_BLOCK_SIZE / 16);
        read_start = (read_start + 1) % f->n;
    }

    f->dmaN += read_n;

    if (f->dmaN) {
        if (isNewData) {
            d4chcr = (d4chcr & 0x0FFFFFFF) | (DMATAG_REF << 28);
        }
        setD4_CHCR(d4chcr | CHCR_STR_BIT);
    }

    SignalSema(f->sema);
    return 1;
}

/** Stops the IPU input DMA and saves the IPU state so viBufRestartDMA() can resume. Returns 1. */
int viBufStopDMA(ViBuf *f) {
    WaitSema(f->sema);
    f->isActive = 0;

    setD4_CHCR((1 << 2) | 1);
    f->env.d4madr = *D4_MADR;
    f->env.d4tadr = *D4_TADR;
    f->env.d4qwc = *D4_QWC;
    f->env.d4chcr = *D4_CHCR;

    while (*IPU_CTRL & 0xF0) {
    }

    setD3_CHCR(0);
    f->env.d3madr = *D3_MADR;
    f->env.d3qwc = *D3_QWC;
    f->env.d3chcr = *D3_CHCR;
    f->env.ipubp = *IPU_BP;
    f->env.ipuctrl = *IPU_CTRL;

    SignalSema(f->sema);
    return 1;
}

/** Resumes the IPU input DMA from the state saved by viBufStopDMA(). Returns 1. */
int viBufRestartDMA(ViBuf *f) {
    int bp;
    int fp;
    int ifc;
    unsigned int d4madr_next;
    unsigned int d4qwc_next;
    unsigned int d4tadr_next;
    unsigned int d4chcr_next;
    int index;
    int index_next;
    int id;

    bp = f->env.ipubp & 0x7F;
    fp = (f->env.ipubp >> 16) & 0x3;
    ifc = (f->env.ipubp >> 8) & 0xF;

    d4madr_next = f->env.d4madr - ((fp + ifc) << 4);
    d4qwc_next = f->env.d4qwc + (fp + ifc);
    d4tadr_next = f->env.d4tadr;
    d4chcr_next = f->env.d4chcr | CHCR_STR_BIT;

    WaitSema(f->sema);

    if (d4madr_next < (unsigned int)f->data) {
        d4qwc_next = ((unsigned int)f->data - d4madr_next) / 16;
        d4madr_next += f->n * VIBUF_BLOCK_SIZE;
        d4tadr_next = (unsigned int)dmaAddr(f->tag);
        id = (f->env.d4madr == (unsigned int)f->data ||
              f->env.d4madr == (unsigned int)f->data + f->n * VIBUF_BLOCK_SIZE)
                 ? DMATAG_REFE
                 : DMATAG_REF;
        d4chcr_next = (f->env.d4chcr & 0x0FFFFFFF) | (id << 28) | CHCR_STR_BIT;

        if (!IN_RING(0, f->dmaStart, f->dmaN, f->n)) {
            f->dmaStart = f->n - 1;
            f->dmaN++;
        }
    } else {
        index = getFIFOindex(f, (void *)f->env.d4madr);
        index_next = getFIFOindex(f, (void *)d4madr_next);

        if (index != index_next) {
            d4tadr_next = (unsigned int)dmaAddr(f->tag + index);
            d4qwc_next = ((unsigned int)f->data + VIBUF_BLOCK_SIZE * index - d4madr_next) / 16;
            id = ((unsigned int)f->data + (f->env.d4madr - (unsigned int)f->data) % (f->n * VIBUF_BLOCK_SIZE) ==
                  (unsigned int)f->data + ((f->dmaStart + f->dmaN) % f->n) * VIBUF_BLOCK_SIZE)
                     ? DMATAG_REFE
                     : DMATAG_REF;
            d4chcr_next = (f->env.d4chcr & 0x0FFFFFFF) | (id << 28) | CHCR_STR_BIT;

            if (!IN_RING(index_next, f->dmaStart, f->dmaN, f->n)) {
                f->dmaStart = index_next;
                f->dmaN++;
            }
        }
    }

    if (f->env.d3madr && f->env.d3qwc) {
        *D3_MADR = f->env.d3madr;
        *D3_QWC = f->env.d3qwc;
        setD3_CHCR(f->env.d3chcr | CHCR_STR_BIT);
    }

    if (f->dmaN) {
        while ((int)*IPU_CTRL < 0) {
        }
        *IPU_CMD = bp;
        while ((int)*IPU_CTRL < 0) {
        }
    }

    *D4_MADR = d4madr_next;
    *D4_TADR = d4tadr_next;
    *D4_QWC = d4qwc_next;
    if (f->dmaN) {
        setD4_CHCR(d4chcr_next);
    }

    *IPU_CTRL = f->env.ipuctrl;
    f->isActive = 1;

    SignalSema(f->sema);
    return 1;
}

/** Stops the IPU input DMA and deletes the semaphore of `f`. Returns 1. */
int viBufDelete(ViBuf *f) {
    setD4_CHCR((1 << 2) | 1);
    *D4_QWC = 0;
    *D4_MADR = 0;
    *D4_TADR = 0;
    DeleteSema(f->sema);
    return 1;
}

/** Pads the data written to `f` to a whole element, so the last element can be sent. */
void viBufFlush(ViBuf *f) {
    WaitSema(f->sema);
    f->readBytes = (f->readBytes + VIBUF_BLOCK_SIZE - 1) / VIBUF_BLOCK_SIZE * VIBUF_BLOCK_SIZE;
    SignalSema(f->sema);
}

/**
 * Drops the queued time stamps (or the parts of them) whose data lies in the region `new_ts`
 * covers. Returns 0.
 */
int viBufModifyPts(ViBuf *f, ViBufTs *new_ts) {
    ViBufTs *ts;
    int rd;
    int datasize;
    int loop;
    int len;

    rd = (f->n_ts + (f->wt_ts - f->count_ts)) % f->n_ts;
    datasize = VIBUF_BLOCK_SIZE * f->n;
    loop = 1;

    if (f->count_ts > 0) {
        while (loop) {
            ts = f->ts + rd;
            if (ts->len == 0 || new_ts->len == 0) {
                break;
            }
            if (tsInRing(ts->pos, new_ts->pos, new_ts->len, datasize)) {
                len = min(ts->len, new_ts->pos + new_ts->len - ts->pos);
                ts->pos = (ts->pos + len) % datasize;
                ts->len -= len;
                if (ts->len == 0) {
                    if (ts->pts >= 0) {
                        ts->pts = TS_UNSET;
                        ts->dts = TS_UNSET;
                        ts->pos = 0;
                        ts->len = 0;
                    }
                    f->count_ts = (f->count_ts - 1 > 0) ? f->count_ts - 1 : 0;
                }
            } else {
                loop = 0;
            }
            rd = (rd + 1) % f->n_ts;
        }
    }
    return 0;
}

/**
 * Queues time stamp `ts` for the data being written. Returns 0 when the time stamp queue is full.
 */
int viBufPutTs(ViBuf *f, ViBufTs *ts) {
    int ret;

    ret = 0;
    WaitSema(f->sema);

    if (f->count_ts < f->n_ts) {
        viBufModifyPts(f, ts);
        if (ts->pts >= 0 || ts->dts >= 0) {
            f->ts[f->wt_ts].pts = ts->pts;
            f->ts[f->wt_ts].dts = ts->dts;
            f->ts[f->wt_ts].pos = ts->pos;
            f->ts[f->wt_ts].len = ts->len;
            f->count_ts++;
            f->wt_ts = (f->wt_ts + 1) % f->n_ts;
        }
        ret = 1;
    }

    SignalSema(f->sema);
    return ret;
}

/** Stores in `ts` the time stamp of the data the IPU is decoding. Returns 0 when there is none. */
int viBufGetTs(ViBuf *f, ViBufTs *ts) {
    unsigned int d4madr;
    unsigned int ipubp;
    int bp;
    int fp;
    int ifc;
    unsigned int d4madr_next;
    unsigned int stop;
    int datasize;
    int isEnd;
    int tscount;
    int wt;
    int i;
    int rd;

    d4madr = *D4_MADR;
    ipubp = *IPU_BP;
    bp = f->env.ipubp & 0x7F;
    fp = (ipubp >> 16) & 0x3;
    ifc = (ipubp >> 8) & 0xF;
    d4madr_next = d4madr - ((fp + ifc) << 4);
    datasize = VIBUF_BLOCK_SIZE * f->n;
    isEnd = 0;

    WaitSema(f->sema);

    ts->pts = TS_UNSET;
    ts->dts = TS_UNSET;

    stop = (datasize + (d4madr_next + (bp >> 3)) - (unsigned int)f->data) % datasize;

    tscount = f->count_ts;
    wt = f->wt_ts;
    for (i = 0; i < tscount && !isEnd; i++) {
        rd = (i + (f->n_ts + (wt - tscount))) % f->n_ts;
        if (tsInRing(stop, f->ts[rd].pos, f->ts[rd].len, datasize)) {
            ts->pts = f->ts[rd].pts;
            ts->dts = f->ts[rd].dts;
            f->ts[rd].pts = TS_UNSET;
            f->ts[rd].dts = TS_UNSET;
            isEnd = 1;
            f->count_ts -= min(f->count_ts, 1);
        }
    }

    SignalSema(f->sema);
    return 1;
}
