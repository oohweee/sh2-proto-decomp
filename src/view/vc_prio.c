/*
 * vc_prio.c: whether an event camera has priority, per stage. Each check
 * looks at where the player is and returns one of the game flags.
 */
#include "sh2.h"

/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)

static int vcRetPrioCbEvnt(void);
static int vcRetPrioApEvnt(void);
static int vcRetPrioCcEvnt(void);

/** Returns whether the event camera has priority in a stage (by the stage's own check).
 * @param glb_crd stage number */
int vcRetPrioStgEvnt(int glb_crd) {
    int prio_f;

    switch (glb_crd) {
    case 2:
        prio_f = vcRetPrioCbEvnt();
        break;
    case 9:
        prio_f = vcRetPrioApEvnt();
        break;
    case 3:
        prio_f = vcRetPrioCcEvnt();
        break;
    }
    return prio_f;
}

static int vcRetPrioCbEvnt(void) {
    int blk_no[4];

    BlockNumber(blk_no, 0, sh2jms.player->pos.x, sh2jms.player->pos.z);
    switch ((unsigned short)blk_no[0]) {
    case 0x36:
        return GAME_FLAG(43);
    case 0x12:
        return GAME_FLAG(47);
    case 0x43:
        return GAME_FLAG(117);
    }
    return 0;
}

static int vcRetPrioApEvnt(void) {
    int room;

    room = RoomNameJms();
    switch (room) {
    case 0x17:
        return GAME_FLAG(62);
    case 0x1B:
        return GAME_FLAG(66);
    case 0x1F:
        return GAME_FLAG(67);
    case 0x18:
        return GAME_FLAG(72);
    case 0x1E:
        return GAME_FLAG(88);
    case 0x15:
        return GAME_FLAG(95);
    case 0x21:
        return GAME_FLAG(146);
    }
    return 0;
}

static int vcRetPrioCcEvnt(void) {
    int blk_no[4];

    BlockNumber(blk_no, 0, sh2jms.player->pos.x, sh2jms.player->pos.z);
    switch ((unsigned short)blk_no[0]) {
    case 0x29:
        return GAME_FLAG(168);
    }
    return 0;
}
