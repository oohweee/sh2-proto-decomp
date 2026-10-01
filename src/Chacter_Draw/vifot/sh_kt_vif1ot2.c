/*
 * Two-level ordering table for VIF1 DMA packets (Chacter_Draw/vifot). Packets are appended to a
 * first-level table indexed by the low depth bits; before the kick they are re-sorted into a
 * second-level table by the high bits (kept in each tag's mark field), which is chained into
 * one DMA list.
 */
#include "sh2.h"
#include "sdk/eekernel.h"

/* Matching: called without a prototype in the original (arguments passed unconverted). */
void *memset();

static void Init2ndOT(struct ktVif1Ot2 *ot);
static void Make2ndOT(struct ktVif1Ot2 *ot);
static void Sort2(struct _sceDmaTag *top_2, struct _sceDmaTag *p);
static void SetNext(struct _sceDmaTag *tag, struct _sceDmaTag *p);
static struct _sceDmaTag *Next(struct _sceDmaTag *tag);
static unsigned int Depth2(struct _sceDmaTag *tag);
static struct _sceDmaTag *Link(struct _sceDmaTag *tag);

struct _sceDmaTag term = { 0, 0, 0x70, NULL, { 0, 0 } };

/**
 * Sets up a two-level ordering table: fills the second level with `cnt` tags, clears the
 * first level and flushes the data cache.
 * @param ot       the table
 * @param n_bits_1 depth bits of the first level (it has 1 << n_bits_1 entries)
 * @param top_1    first-level tag array
 * @param n_bits_2 depth bits of the second level
 * @param top_2    second-level tag array
 */
void ktVif1Ot2Init(struct ktVif1Ot2 *ot, unsigned int n_bits_1, struct _sceDmaTag *top_1, unsigned int n_bits_2,
                   struct _sceDmaTag *top_2) {
    struct _sceDmaTag *p;
    int i;

    ot->top_1 = top_1;
    ot->top_2 = top_2;
    ot->n_bits_1 = n_bits_1;
    ot->n_bits_2 = n_bits_2;
    ot->length_1 = 1 << n_bits_1;
    ot->length_2 = 1 << n_bits_2;
    ot->mask_1 = (1 << n_bits_1) - 1;
    ot->mask_2 = (1 << (n_bits_1 + n_bits_2)) - 1 - ot->mask_1;
    memset(ot->top_2, 0, ot->length_2 * sizeof(struct _sceDmaTag));
    for (i = 0, p = ot->top_2; i < ot->length_2; i++, p++) {
        p->id = 0x20;
    }
    ktVif1Ot2Reset(ot);
    FlushCache(0);
}

/** Clears the first-level table of ot. */
void ktVif1Ot2Reset(struct ktVif1Ot2 *ot) {
    struct _sceDmaTag *top;
    unsigned int length;

    top = ot->top_1;
    length = ot->length_1;
    memset(top, 0, length * sizeof(struct _sceDmaTag));
}

/* Matching: inline tag accessors (no DWARF); the out-of-line SetNext/Next below don't give this codegen. */
static inline void _SetNext(struct _sceDmaTag *tag, struct _sceDmaTag *p) {
    tag->next = p;
}

static inline struct _sceDmaTag *_Next(struct _sceDmaTag *tag) {
    return tag->next;
}

/**
 * Links a packet into the ordering table at a depth.
 * @param ot      the table
 * @param depth   sort key: the low n_bits_1 bits pick the first-level entry, the rest go
 *                into the packet tag's mark for the second-level sort
 * @param packet_ the packet, starting with its DMA tag
 */
void ktVif1Ot2Append(struct ktVif1Ot2 *ot, unsigned int depth, u_long128 *packet_) {
    unsigned int depth_1;
    unsigned int depth_2;
    struct _sceDmaTag *trunk;
    struct _sceDmaTag *packet;

    depth_1 = depth & ot->mask_1;
    depth_2 = depth >> ot->n_bits_1;
    trunk = ot->top_1 + depth_1;
    packet = (struct _sceDmaTag *)packet_;
    packet->mark = depth_2;
    _SetNext(packet, _Next(trunk));
    _SetNext(trunk, packet);
}

/**
 * ktVif1Ot2Append for a packet whose sort tag is not its first tag: the depth and the chain
 * link go into entity_, while the table entry points at packet_.
 * @param ot      the table
 * @param depth   sort key, as in ktVif1Ot2Append
 * @param packet_ start of the packet
 * @param entity_ the tag that carries the mark and the next link
 */
void ktVif1Ot2AppendFake(struct ktVif1Ot2 *ot, unsigned int depth, u_long128 *packet_, void *entity_) {
    unsigned int depth_1;
    unsigned int depth_2;
    struct _sceDmaTag *trunk;
    struct _sceDmaTag *packet;
    struct _sceDmaTag *entity;

    depth_1 = depth & ot->mask_1;
    depth_2 = depth >> ot->n_bits_1;
    trunk = ot->top_1 + depth_1;
    packet = (struct _sceDmaTag *)packet_;
    entity = (struct _sceDmaTag *)entity_;
    entity->mark = depth_2;
    _SetNext(entity, _Next(trunk));
    _SetNext(trunk, packet);
}

static void Init2ndOT(struct ktVif1Ot2 *ot) {
    int i;
    struct _sceDmaTag *pTagNext;
    unsigned int length_2;
    struct _sceDmaTag *top_2;

    length_2 = ot->length_2;
    top_2 = ot->top_2;
    for (i = 0; i < length_2 - 1; i++, top_2++) {
        pTagNext = top_2 + 1;
        top_2->next = pTagNext;
    }
    top_2->next = &term;
}

static void Make2ndOT(struct ktVif1Ot2 *ot) {
    struct _sceDmaTag *top_2;
    struct _sceDmaTag *top_1;
    int length_1;
    int i;
    struct _sceDmaTag *p;
    struct _sceDmaTag *link;

    top_2 = ot->top_2;
    top_1 = ot->top_1;
    length_1 = ot->length_1;
    for (i = length_1 - 1; i >= 0; i--) {
        for (p = Link(&top_1[i]); p != NULL; p = link) {
            link = Link(p);
            Sort2(top_2, p);
        }
    }
    SyncDCache(top_2, top_2 + ot->length_2);
}

static void Sort2(struct _sceDmaTag *top_2, struct _sceDmaTag *p) {
    unsigned int depth_2;
    struct _sceDmaTag *trunk;

    depth_2 = Depth2(p);
    trunk = top_2 + depth_2;
    SetNext(p, Next(trunk));
    SetNext(trunk, p);
}

static void SetNext(struct _sceDmaTag *tag, struct _sceDmaTag *p) {
    tag->next = p;
}

static struct _sceDmaTag *Next(struct _sceDmaTag *tag) {
    return tag->next;
}

static unsigned int Depth2(struct _sceDmaTag *tag) {
    return tag->mark;
}

static struct _sceDmaTag *Link(struct _sceDmaTag *tag) {
    return tag->next;
}

/**
 * Prepares ot for sending: chains the second-level table, sorts every first-level packet into
 * it and clears the first level.
 */
void ktVif1Ot2KickPre(struct ktVif1Ot2 *ot) {
    Init2ndOT(ot);
    Make2ndOT(ot);
    ktVif1Ot2Reset(ot);
}
