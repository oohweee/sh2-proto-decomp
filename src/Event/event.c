/*
 * event.c: the map event system. Each stage has an event list (flags, conditions
 * and results packed into Event_List words, decoded by EventListElement) and an
 * item list; EventCheck finds the event the player triggers and EventMainStandard
 * runs its result (flag, message, program, door, item, move, save, map).
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "fi_libvu0_inline.h"

#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)
#define GAME_FLAG_ON(n) (game_flag.flag[(n) >> 5] |= 1 << ((n) & 31))
#define ITEM_FLAG(n) ((item.flag[(n) >> 5] >> ((n) & 31)) & 1)

#define EV_M_STEP(n) \
    ev_m_step = (n); \
    ev_e_step = 0;   \
    ev_p_step = 0;   \
    ev_s_step = 0

#define SH2SYS_STEP2(v)   \
    Sh2sys.step[2] = (v); \
    Sh2sys.step[3] = 0;   \
    Sh2sys.step[4] = 0;   \
    Sh2sys.step[5] = 0;   \
    Sh2sys.step[6] = 0;   \
    Sh2sys.step[7] = 0

static void EventMainStandard(int ev_act_on);
static int EventCheckLook(struct Event_List *el, struct Event_JmsInfo jms);
static int EventCheckLookPoint(float x, float z, struct Event_JmsInfo jms);
static int EventCheckLookLine(float x0, float z0, float x1, float z1, struct Event_JmsInfo jms);
static int EventCheckIn(struct Event_List *el, struct Event_JmsInfo jms);
static int EventListElement(struct Event_List *el, int en);
static int ItemListElement(struct Item_List *il, int en);
static int ItemCheckLookPoint(struct Item_List *il, struct Event_JmsInfo jms);
static int EventExecFlag(void);
static int EventExecMessage(void);
static int EventExecProgram(void);
static int EventExecDoor(void);
static int EventExecItem(void);
static int EventExecMove(void);
static int EventExecSave(void);
static int EventExecChizuFail(void);

struct Event_DoorSound door_se[22] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, { 0x4A38, 0x4A39, 0x4A3B, 0x4A3A, 0x4A3C, 0x0000 },
    { 0x4A38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, { 0x0000, 0x4A39, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x4A4F, 0x4A50, 0x4A51, 0x4A52, 0x4A53, 0x0000 }, { 0x4A4F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x4A50, 0x0000, 0x0000, 0x0000, 0x0000 }, { 0x4A3D, 0x4A3E, 0x4A41, 0x4A40, 0x4A3F, 0x0000 },
    { 0x2B31, 0x2B32, 0x2B34, 0x2B33, 0x2B35, 0x0000 }, { 0x2B36, 0x2B37, 0x2B39, 0x2B38, 0x2B3A, 0x0000 },
    { 0x4DBC, 0x4DB9, 0x0000, 0x4A54, 0x0000, 0x0000 }, { 0x0000, 0x4DB9, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x2AFE, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, { 0x465B, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x4074, 0x4075, 0x4076, 0x2B33, 0x4077, 0x0000 }, { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x2744, 0x2745, 0x0000, 0x0000, 0x0000, 0x0000 }, { 0x4078, 0x4079, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x4079, 0x0000, 0x0000, 0x0000, 0x0000 }, { 0x3A98, 0x3A99, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x4983, 0x4984, 0x0000, 0x0000, 0x0000, 0x0000 }, { 0x0000, 0x0000, 0x0000, 0x3CF3, 0x0000, 0x0000 },
};

struct GAME_FLAG_DATA game_flag;
struct Stage_Data *stage;
struct Radio_Data radio;
int ev_m_step;
int ev_e_step;
int ev_p_step;
int ev_s_step;
int ev_cancel;
int ev_prog_flag_set;
float ev_timer;
float ev_filter;
float ev_cursor_x;
float ev_cursor_y;
static int ev_active;

/** Clears the game flags for a new game and sets the ones decided at the start (riddle level,
 * the ending-dependent choices, some picked at random). */
void FlagInit(void) {
    int shuffle[6];
    int work;
    int i;

    shQzero(&game_flag, sizeof(game_flag));
    if (playing.riddle_level == 2 && (playing.clear_end_kind & 0x20) && (playing.clear_end_kind & 0x40) &&
        (playing.clear_end_kind & 0x80)) {
        playing.riddle_level = 3;
    }
    work = 0;
    for (i = 0; i < 3; i++) {
        if (playing.clear_end_kind & (1 << i)) {
            work++;
        }
    }
    if (work <= 2 && work < playing.clear_end_number) {
        if (work == 1) {
            if (playing.clear_end_kind & 1) {
                if (shRandI() & 1) {
                    GAME_FLAG_ON(8);
                } else {
                    GAME_FLAG_ON(9);
                }
            }
            if (playing.clear_end_kind & 2) {
                if (shRandI() & 1) {
                    GAME_FLAG_ON(9);
                } else {
                    GAME_FLAG_ON(7);
                }
            }
            if (playing.clear_end_kind & 4) {
                if (shRandI() & 1) {
                    GAME_FLAG_ON(7);
                }
                GAME_FLAG_ON(8);
            }
        } else {
            if (!(playing.clear_end_kind & 1)) {
                GAME_FLAG_ON(7);
            }
            if (!(playing.clear_end_kind & 2)) {
                GAME_FLAG_ON(8);
            }
            if (!(playing.clear_end_kind & 4)) {
                GAME_FLAG_ON(9);
            }
        }
    }
    if (work > 0) {
        GAME_FLAG_ON(10);
    }
    if (work == 3 || (playing.clear_end_kind & 8)) {
        GAME_FLAG_ON(11);
    }
    game_flag.clock = shRandI() % 660;
    if (game_flag.clock > 520) {
        game_flag.clock += 60;
    }
    game_flag.clock <<= 6;
    for (i = 0; i < 4; i++) {
        game_flag.safe[i] = shRandI() % 20;
        if (i > 0 && game_flag.safe[i - 1] == game_flag.safe[i]) {
            game_flag.safe[i] = (game_flag.safe[i] + 10) % 20;
        }
    }
    work = (game_flag.safe[0] + shRandI() % 19) % 20;
    for (i = 0; i < 5; i++) {
        if ((1 << i) & work) {
            GAME_FLAG_ON(i + 103);
        }
    }
    for (i = 0; i < 4; i++) {
        game_flag.rotate[i] = -1;
    }
    for (i = 0; i < 4; i++) {
        game_flag.carbon = (game_flag.carbon << 4) + shRandI() % 9 + 1;
        game_flag.guruguru[i] = shRandI() % 9;
        game_flag.cylinder[i] = (game_flag.guruguru[i] + 1 + shRandI() % 8) % 9;
    }
    if (playing.riddle_level < 0) {
        if (game_flag.guruguru[0] == game_flag.guruguru[1]) {
            game_flag.guruguru[1]++;
            if (game_flag.guruguru[1] == 9) {
                game_flag.guruguru[1] = 0;
            }
        }
        game_flag.guruguru[2] = game_flag.guruguru[0];
        game_flag.guruguru[3] = game_flag.guruguru[1];
    }
    game_flag.runaway[0] = shRandI() % 9;
    work = shRandI() % 8;
    game_flag.runaway[1] = work;
    if (work >= game_flag.runaway[0]) {
        game_flag.runaway[1]++;
    }
    work = shRandI() % 7;
    game_flag.runaway[2] = work;
    if (work >= game_flag.runaway[0]) {
        game_flag.runaway[2]++;
    }
    if (work >= game_flag.runaway[1]) {
        game_flag.runaway[2]++;
    }
    game_flag.runaway[3] = 0;
    for (i = 0; i < 6; i++) {
        shuffle[i] = i;
    }
    for (i = 0; i < 6; i++) {
        work = shRandI() % (6 - i);
        game_flag.hanging = game_flag.hanging * 6 + shuffle[i + work];
        shuffle[i + work] = shuffle[i];
    }
    work = shRandI() % 3 + 1;
    if (work & 1) {
        GAME_FLAG_ON(319);
        GAME_FLAG_ON(323);
    }
    if (work & 2) {
        GAME_FLAG_ON(320);
        GAME_FLAG_ON(324);
    }
    work = shRandI() % 3 + 1;
    if (work & 1) {
        GAME_FLAG_ON(317);
        GAME_FLAG_ON(321);
    }
    if (work & 2) {
        GAME_FLAG_ON(318);
        GAME_FLAG_ON(322);
    }
    work = shRandI() % 19;
    for (i = 0; i < 5; i++) {
        if (work & (1 << i)) {
            GAME_FLAG_ON(i + 406);
        }
    }
    if (playing.riddle_level == 0) {
        GAME_FLAG_ON(13);
    }
    if ((shRandI() & 3) == 0) {
        GAME_FLAG_ON(543);
    }
}

/** Resets the event program state (steps, active event, subtitles, radio). */
void EventProgInit(void) {
    EV_M_STEP(0);
    ev_active = -1;
    ev_cancel = 0;
    sbt_msg_no = 0;
    radio.se_call = 0;
    radio.event = 0;
}

/** Per-frame event update: ampoule timer; outside events, the menu buttons (item menu, map,
 * flashlight) and the event check (EventMainStandard); the running event otherwise; then the
 * stage's all-time function and the game over after the death animation. */
void EventMain(void) {
    item.ampoule_efficacy = fmaxf(0.0f, item.ampoule_efficacy - shGetDT());
    if (BgIsOut(0)) {
        ConnectCharaWorkAdminOut(1);
    }
    Sh2sys.main_status &= ~0x40;
    Sh2sys.main_status &= ~0x80;
    demo_number = 0;
    if (ev_m_step == 0) {
        if (PlayerEventJamesDeadly() || PlayerEventMariaDeadly()) {
        } else if (PlayerEventButtonCheck(1)) {
            SH2SYS_STEP2(6);
            sh2gfw_Set_CaptureNowFB();
            ScreenEffectFadeStart(1, 1.0f);
        } else if (PlayerEventButtonCheck(3)) {
            if (!sh2gfw_Get_NightOrDay() || LightSpotOnOffCheck() || sh2gfw_Check_CharaDarkOrBright(sh2jms.player)) {
                SH2SYS_STEP2(5);
                Sh2sys.step[3] = 0;
                Sh2sys.step[4] = 0;
                Sh2sys.step[5] = 0;
                Sh2sys.step[6] = 0;
                Sh2sys.step[7] = 0;
                ScreenEffectFadeStart(1, 1.0f);
            } else {
                EV_M_STEP(10);
                EventMainStandard(0);
            }
        } else {
            if (PlayerEventButtonCheck(2)) {
                item.light_switch ^= 1;
                LightSpotOnOffSet();
            }
            EventMainStandard(1);
        }
    } else {
        EventMainStandard(0);
    }
    ev_cancel = 0;
    sh2shd_reset_shadow_off_work();
    if (stage->alltime_func) {
        stage->alltime_func();
    }
    if (PlayerEventDeadAnimeFinish()) {
        SH2SYS_STEP2(12);
        ScreenEffectFadeStart(1, 1.0f);
    }
    if (Sh2sys.step[2] != 4) {
        fontClear();
        shSdCall(0x3F4, 0, 0, 0);
    }
}

static void EventMainStandard(int ev_act_on) {
    struct Event_List *el;
    int st;
    int use_item;
    int ret;

    while (1) {
        if (ev_m_step == 0) {
            use_item = ItemCombinationUseCheck(item.event_use[0], item.event_use[1], item.event_use[2]);
            item.event_use[2] = 0;
            item.event_use[1] = 0;
            item.event_use[0] = 0;
            if (ev_cancel) {
                return;
            }
            ev_active = use_item ? EventCheck(0, use_item, 0) : EventCheck(ev_act_on, 0, 0);
            if (ev_active == -1) {
                return;
            }
            if (ev_active < 0x400) {
                el = &stage->ev_list[ev_active];
                st = EventListElement(el, 13);
                switch (st) {
                case 0:
                    break;
                case 1:
                case 2:
                    EV_M_STEP(1);
                    break;
                case 3:
                    EV_M_STEP(2);
                    break;
                case 4:
                    EV_M_STEP(5);
                    break;
                case 5:
                    EV_M_STEP(3);
                    break;
                case 6:
                    EV_M_STEP(6);
                    break;
                case 7:
                case 8:
                case 9:
                    EV_M_STEP(7);
                    break;
                case 10:
                    EV_M_STEP(9);
                    break;
                }
            } else {
                EV_M_STEP(8);
            }
        }
        switch (ev_m_step) {
        case 1:
            ret = EventExecFlag();
            break;
        case 6:
            ret = EventExecMessage();
            break;
        case 2:
        case 4:
            ret = EventExecProgram();
            break;
        case 7:
            ret = EventExecDoor();
            break;
        case 8:
            ret = EventExecItem();
            break;
        case 3:
        case 5:
            ret = EventExecMove();
            break;
        case 9:
            ret = EventExecSave();
            break;
        case 10:
            ret = EventExecChizuFail();
            break;
        default:
            /* Matching: the assert bakes its original line number into the object. */
#line 528
            assert(0);
        }
        if (ret == 0) {
            return;
        }
        EV_M_STEP(0);
        ev_act_on = 0;
    }
}

/** Finds the event (or item) the player triggers from the event and item lists of the stage.
 * @param act_on non-zero if the action button was pressed
 * @param use_item item being used, if any
 * @param check_only non-zero to only check, without starting the event
 * @return the event index, 0x400 + the item index, or -1 for none */
int EventCheck(int act_on, int use_item, int check_only) {
    static int check[4];
    struct SubCharacter *scp;
    struct Event_JmsInfo jms;
    struct Event_List *el;
    struct Item_List *il;
    int light_on;
    int cond_st;
    int kind;
    int flag;
    int i;

    scp = sh2jms.player;
    jms.pos_x = scp->pos.x;
    jms.pos_y = scp->pos.y;
    jms.pos_z = scp->pos.z;
    jms.rot_y = scp->rot.y;
    jms.view_x = 400.0f * shSinF(jms.rot_y);
    jms.view_z = 400.0f * shCosF(jms.rot_y);
    jms.rear_x = jms.pos_x + 50.0f * shSinF(3.1415927f + jms.rot_y);
    jms.rear_z = jms.pos_z + 50.0f * shCosF(3.1415927f + jms.rot_y);
    act_on = act_on != 0;
    if (act_on) {
        act_on = PlayerEventButtonCheck(0) != 0;
    }
    if (sh2gfw_Get_NightOrDay()) {
        light_on = LightSpotOnOffCheck();
    } else {
        light_on = 1;
    }
    for (i = 0;; i++) {
        el = &stage->ev_list[i];
        if (el->cond == 0 && el->rslt0 == 0) {
            break;
        }
        if (!light_on && EventListElement(el, 12)) {
            continue;
        }
        cond_st = EventListElement(el, 7);
        if (use_item) {
            switch (cond_st) {
            case 5:
            case 6:
            case 10:
                if (use_item == EventListElement(el, 10)) {
                    break;
                }
            default:
                continue;
            }
        } else {
            switch (cond_st) {
            case 2:
                if (!act_on) {
                    continue;
                }
                break;
            case 10:
            case 7:
                if (!act_on) {
                    continue;
                }
            case 8:
            case 9:
                if (!ITEM_FLAG(EventListElement(el, 10))) {
                    continue;
                }
                break;
            case 5:
            case 6:
                continue;
            }
        }
        check[0] = el->flag;
        check[1] = EventListElement(el, 3);
        check[2] = GAME_FLAG(check[1]);
        if (EventListElement(el, 3) && (!GAME_FLAG(EventListElement(el, 3)) ^ !EventListElement(el, 1))) {
            continue;
        }
        if (EventListElement(el, 6) && (!GAME_FLAG(EventListElement(el, 6)) ^ !EventListElement(el, 4))) {
            continue;
        }
        switch (cond_st) {
        case 0:
        case 1:
            break;
        case 2:
        case 7:
        case 10:
            switch (EventListElement(el, 9)) {
            case 11:
            case 12:
            case 13:
                if (!EventCheckIn(el, jms)) {
                    continue;
                }
                break;
            default:
                if (!EventCheckLook(el, jms)) {
                    continue;
                }
                break;
            }
            break;
        case 3:
        case 5:
        case 8:
            switch (EventListElement(el, 9)) {
            case 11:
            case 12:
            case 13:
                /* Matching: the assert bakes its original line number into the object. */
#line 670
                assert(0);
            }
            if (!EventCheckLook(el, jms)) {
                continue;
            }
            break;
        case 4:
        case 6:
        case 9:
            switch (EventListElement(el, 9)) {
            default:

                assert(0);
            case 11:
            case 12:
            case 13:
                break;
            }
            if (!EventCheckIn(el, jms)) {
                continue;
            }
            break;
        }
        switch (cond_st) {
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            if (EventListElement(el, 11) && !check_only) {
                ItemUse(EventListElement(el, 10));
            }
            break;
        }
        return i;
    }
    if (check_only) {
        return -1;
    }
    if (!act_on) {
        return -1;
    }
    if (stage->gi_list == NULL) {
        return -1;
    }
    for (i = 0;; i++) {
        il = &stage->gi_list[i];
        kind = ItemListElement(il, 0);
        if (kind == 7) {
            break;
        }
        flag = ItemListElement(il, 3);
        if (GAME_FLAG(flag) || GAME_FLAG(flag + 1)) {
            continue;
        }
        if (!EventItemConditionCheck(ItemListElement(il, 1), ItemListElement(il, 2))) {
            continue;
        }
        if (!ItemCheckLookPoint(il, jms)) {
            continue;
        }
        if (sh2gfw_Get_NightOrDay() && !LightSpotOnOffCheck()) {
            switch (kind) {
            case 0:
            case 1:
            default:
                kind = 0x703;
                break;
            case 2:
                kind = 0x724;
                break;
            case 3:
                kind = 0x723;
                break;
            case 4:
                kind = 0x700;
                break;
            case 5:
                kind = 0x701;
                break;
            case 6:
                kind = 0x733;
                break;
            }
            scp = shCharacterGetSubCharacter(kind, flag);
            if (scp == NULL) {
                continue;
            }
            if (!sh2gfw_Check_CharaDarkOrBright(scp)) {
                continue;
            }
        }
        return i + 0x400;
    }
    return -1;
}

static int EventCheckLook(struct Event_List *el, struct Event_JmsInfo jms) {
    float p[7][2];
    char *pos_p;
    float f_work;
    int pos_type;
    int work;
    int i;

    pos_p = (char *)stage->ev_pos + EventListElement(el, 8);
    f_work = CharToFloat2(pos_p + 4);
    if ((f_work < 45000.0f && f_work > 100.0f + jms.pos_y) || f_work < -500.0f + jms.pos_y) {
        return 0;
    }
    pos_type = EventListElement(el, 9);
    if (pos_type == 0) {
        return EventCheckLookPoint(CharToFloat4(pos_p), CharToFloat4(pos_p + 6), jms);
    }
    p[0][0] = CharToFloat4(pos_p);
    p[0][1] = CharToFloat4(pos_p + 6);
    switch (pos_type) {
    case 1:
    case 2:
    case 3:
    case 4:
        work = 2;
        p[1][0] = p[0][0];
        p[1][1] = p[0][1];
        switch (pos_type) {
        case 1:
            p[1][0] += CharToFloat2(pos_p + 10);
            break;
        case 2:
            p[1][0] -= CharToFloat2(pos_p + 10);
            break;
        case 3:
            p[1][1] -= CharToFloat2(pos_p + 10);
            break;
        case 4:
            p[1][1] += CharToFloat2(pos_p + 10);
            break;
        }
        break;
    default:
        work = pos_type - 3;
        for (i = 1; i < work; i++) {
            p[i][0] = p[0][0] + CharToFloat2(pos_p + i * 4 + 6);
            p[i][1] = p[0][1] + CharToFloat2(pos_p + i * 4 + 8);
        }
        break;
    }
    for (i = 0; i < work - 1; i++) {
        if (EventCheckLookLine(p[i][0], p[i][1], p[i + 1][0], p[i + 1][1], jms)) {
            return 1;
        }
    }
    return 0;
}

/** Whether James is facing point (x, z) (within his view angle and distance). */
static int EventCheckLookPoint(float x, float z, struct Event_JmsInfo jms) {
    static float pos_x;
    static float pos_z;
    float ang;

    pos_x = x - jms.pos_x;
    pos_z = z - jms.pos_z;
    if (!(fabsf(pos_x) <= 400.0f && fabsf(pos_z) <= 400.0f)) {
        return 0;
    }
    if (!(sqr(pos_x) + sqr(pos_z) <= 160000.0f)) {
        return 0;
    }
    ang = shAngleRegulate(jms.rot_y - shAtan2(z - jms.rear_z, x - jms.rear_x));
    if (!(fabsf(ang) <= 0.5235988f)) {
        return 0;
    }
    return 1;
}

/* does the look line of James cross the segment (x0, z0)-(x1, z1)? */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
static int EventCheckLookLine(float x0, float z0, float x1, float z1, struct Event_JmsInfo jms) {
    int ret;

    asm {
        .set noreorder
        paddub  ret, zero, zero
        lwc1    $f7, jms.view_x
        lwc1    $f8, jms.view_z
        lwc1    $f1, jms.pos_x
        lwc1    $f2, jms.pos_z
        nop
        sub.s   $f0, $f0, $f0
        sub.s   x0, x0, $f1
        sub.s   z0, z0, $f2
        sub.s   x1, x1, $f1
        sub.s   z1, z1, $f2
        mul.s   $f1, x0, $f8
        mul.s   $f2, z0, $f7
        mul.s   $f3, x1, $f8
        mul.s   $f4, z1, $f7
        sub.s   $f1, $f1, $f2
        sub.s   $f5, z1, z0
        sub.s   $f3, $f3, $f4
        sub.s   $f6, x1, x0
        c.lt.s  $f1, $f0
        bc1t    L_end
        c.lt.s  $f0, $f3
        bc1t    L_end
        mul.s   $f1, x0, $f5
        mul.s   $f2, z0, $f6
        sub.s   $f3, $f7, x0
        sub.s   $f4, $f8, z0
        sub.s   $f1, $f1, $f2
        mul.s   $f3, $f3, $f5
        mul.s   $f4, $f4, $f6
        c.lt.s  $f1, $f0
        bc1t    L_end
        sub.s   $f3, $f3, $f4
        c.lt.s  $f3, $f0
        bc1t    L_end
        nop
        addi    ret, ret, 0x1
    L_end:
        .set reorder
    }
    return ret;
}

static int EventCheckIn(struct Event_List *el, struct Event_JmsInfo jms) {
    float p[4][2];
    float w;
    float h;
    float f_work;
    char *pos_p;
    int pos_type;
    int work;
    int i;

    pos_p = (char *)stage->ev_pos + EventListElement(el, 8);
    f_work = CharToFloat2(pos_p + 4);
    if (f_work < 45000.0f && (f_work > 100.0f + jms.pos_y || f_work < -500.0f + jms.pos_y)) {
        return 0;
    }
    pos_type = EventListElement(el, 9);
    p[0][0] = CharToFloat4(pos_p);
    p[0][1] = CharToFloat4(pos_p + 6);
    if (pos_type == 11) {
        w = CharToFloat2(pos_p + 10);
        h = CharToFloat2(pos_p + 12);
        if (jms.pos_x < p[0][0] || jms.pos_x > p[0][0] + w || jms.pos_z < p[0][1] || jms.pos_z > p[0][1] + h) {
            return 0;
        }
        return 1;
    }
    if (pos_type == 12) {
        work = 3;
    } else {
        work = 4;
    }
    for (i = 1; i < work; i++) {
        p[i][0] = p[0][0] + CharToFloat2(pos_p + i * 4 + 6);
        p[i][1] = p[0][1] + CharToFloat2(pos_p + i * 4 + 8);
        if (shOuterXZ(jms.pos_x, jms.pos_z, p[i - 1][0], p[i - 1][1], p[i][0], p[i][1]) < 0.0f) {
            return 0;
        }
    }
    return !(shOuterXZ(jms.pos_x, jms.pos_z, p[i - 1][0], p[i - 1][1], p[0][0], p[0][1]) < 0.0f);
}

static int EventListElement(struct Event_List *el, int en) {
    switch (en) {
    case 1:
        return (el->flag >> 31) & 1;
    case 2:
        return (el->flag >> 30) & 1;
    case 3:
        return (el->flag >> 16) & 0x3FFF;
    case 4:
        return (el->flag >> 15) & 1;
    case 5:
        return (el->flag >> 14) & 1;
    case 6:
        return el->flag & 0x3FFF;
    case 7:
        return (el->cond >> 28) & 0xF;
    case 8:
        return (el->cond >> 16) & 0xFFF;
    case 9:
        return (el->cond >> 12) & 0xF;
    case 10:
        return (el->cond >> 4) & 0xFF;
    case 11:
        return (el->cond >> 3) & 1;
    case 12:
        return (el->cond >> 2) & 1;
    case 13:
        return (el->rslt0 >> 28) & 0xF;
    case 14:
        return (el->rslt0 >> 16) & 0xFFF;
    case 15:
        return (el->rslt0 >> 12) & 0xF;
    case 16:
        return (el->rslt0 >> 5) & 0x7F;
    case 17:
        return (el->rslt0 >> 4) & 1;
    case 18:
        return (el->rslt1 >> 22) & 0x3F;
    case 19:
        return (el->rslt1 >> 14) & 0x7F;
    case 20:
        return (el->rslt1 >> 14) & 0xFF;
    case 21:
        return (el->rslt1 >> 14) & 0xFF;
    case 22:
        return el->rslt1 & 0x3FFF;
    }
    return 0;
}

/** Reads a 16-bit float (1 sign, 5 exponent, 10 mantissa bits) from a byte string.
 * @param cp the two bytes (little endian) */
/* Matching: compiled without the global optimizer (with it on, sig and coe swap registers); the
   pragma reproduces the function exactly, as it does the same decoder in demoview.c (DdsReadFloat2).
   MWCC also compiles a function containing inline asm this way, but nothing in the code
   shows one. */
#pragma push
#pragma global_optimizer off
float CharToFloat2(char *cp) {
    int coe;
    int exp;
    int sig;
    int work;

    work = 0;
    ((char *)&work)[0] = cp[0];
    ((char *)&work)[1] = cp[1];
    sig = (work >> 15) & 1;
    exp = (work >> 10) & 0x1F;
    coe = work & 0x3FF;
    work = (sig << 31) | ((exp + 112) << 23) | (coe << 13);
    return *(float *)&work;
}
#pragma pop

/** Reads a 32-bit float from a byte string that may be unaligned.
 * @param cp the four bytes */
float CharToFloat4(char *cp) {
    char c_work[4];

    c_work[0] = cp[0];
    c_work[1] = cp[1];
    c_work[2] = cp[2];
    c_work[3] = cp[3];
    return *(float *)c_work;
}

static int ItemListElement(struct Item_List *il, int en) {
    switch (en) {
    case 0:
        return (il->st >> 29) & 7;
    case 1:
        return (il->st >> 26) & 7;
    case 2:
        return (il->st >> 20) & 0x1F;
    case 3:
        return il->st & 0x3FFF;
    }
    return 0;
}

/** Whether James is facing item il (its position within his view angle and distance). */
static int ItemCheckLookPoint(struct Item_List *il, struct Event_JmsInfo jms) {
    float pos_x;
    float pos_y;
    float pos_z;
    float ang;

    pos_y = CharToFloat2((char *)&il->pos_y);
    if (pos_y > 100.0f + jms.pos_y || pos_y < -750.0f + jms.pos_y) {
        return 0;
    }
    pos_x = il->pos_x - jms.pos_x;
    pos_z = il->pos_z - jms.pos_z;
    if (fabsf(pos_x) > 400.0f || fabsf(pos_z) > 400.0f) {
        return 0;
    }
    pos_x = sqr(pos_x);
    pos_z = sqr(pos_z);
    if (!(pos_x + pos_z <= 160000.0f)) {
        return 0;
    }
    ang = shAngleRegulate(jms.rot_y - shAtan2(il->pos_z - jms.rear_z, il->pos_x - jms.rear_x));
    if (!(fabsf(ang) <= 0.5235988f)) {
        return 0;
    }
    return 1;
}

static void EventPositionSet(float *pos_v, char *pos_p, int pos_t) {
    pos_v[0] = CharToFloat4(pos_p);
    pos_v[1] = CharToFloat2(pos_p + 4);
    pos_v[2] = CharToFloat4(pos_p + 6);
    switch (pos_t) {
    case 1:
        pos_v[0] += CharToFloat2(pos_p + 10) / 2.0f;
        break;
    case 2:
        pos_v[0] -= CharToFloat2(pos_p + 10) / 2.0f;
        break;
    case 3:
        pos_v[2] -= CharToFloat2(pos_p + 10) / 2.0f;
        break;
    case 4:
        pos_v[2] += CharToFloat2(pos_p + 10) / 2.0f;
        break;
    case 5:
        pos_v[0] += CharToFloat2(pos_p + 10) / 2.0f;
        pos_v[2] += CharToFloat2(pos_p + 12) / 2.0f;
        break;
    }
}

static void EventResultMovePosition(int ev_no) {
    float mv_pos[4];
    struct Event_List *el;
    char *pos;
    int pos_type;
    float f_work;
    float rot;

    el = &stage->ev_list[ev_no];
    pos = (char *)stage->ev_pos + EventListElement(el, 14);
    pos_type = EventListElement(el, 15);
    _sceVu0UnitVector(mv_pos);
    mv_pos[0] = CharToFloat4(pos);
    mv_pos[1] = CharToFloat2(pos + 4);
    mv_pos[2] = CharToFloat4(pos + 6);
    rot = sh2jms.player->rot.y;
    switch (pos_type) {
    case 1:
        mv_pos[0] += CharToFloat2(pos + 10) / 2.0f;
        mv_pos[2] += 250.0f;
        rot = 0.0f;
        break;
    case 2:
        mv_pos[0] -= CharToFloat2(pos + 10) / 2.0f;
        mv_pos[2] -= 250.0f;
        rot = 3.1415927f;
        break;
    case 3:
        mv_pos[0] += 250.0f;
        mv_pos[2] -= CharToFloat2(pos + 10) / 2.0f;
        rot = 1.5707964f;
        break;
    case 4:
        mv_pos[0] -= 250.0f;
        mv_pos[2] += CharToFloat2(pos + 10) / 2.0f;
        rot = -1.5707964f;
        break;
    case 5:
        mv_pos[0] += CharToFloat2(pos + 10) / 2.0f;
        mv_pos[2] += CharToFloat2(pos + 12) / 2.0f;
        f_work = CharToFloat2(pos + 12);
        rot = -1.5707964f + shAtan2(f_work, CharToFloat2(pos + 10));
        mv_pos[0] += 250.0f * shSinF(rot);
        mv_pos[2] += 250.0f * shCosF(rot);
        break;
    case 0:
        break;
    }
    connect_pos[0] = mv_pos[0];
    connect_pos[1] = mv_pos[1];
    connect_pos[2] = mv_pos[2];
    connect_pos[3] = shAngleRegulate(rot);
}

/** Cancels the running event. */
void EventCancel(void) {
    ev_cancel = 1;
}

static void EventExecSubFlagSet(struct Event_List *el) {
    int flg;

    flg = EventListElement(el, 3);
    if (flg == 0) {
        return;
    }
    if (EventListElement(el, 2)) {
        if (EventListElement(el, 1)) {
            game_flag.flag[flg >> 5] &= ~(1 << (flg & 31));
        } else {
            game_flag.flag[flg >> 5] |= 1 << (flg & 31);
        }
    }
    flg = EventListElement(el, 6);
    if (flg == 0) {
        return;
    }
    if (EventListElement(el, 5)) {
        if (EventListElement(el, 4)) {
            game_flag.flag[flg >> 5] &= ~(1 << (flg & 31));
        } else {
            game_flag.flag[flg >> 5] |= 1 << (flg & 31);
        }
    }
}

static int EventExecFlag(void) {
    struct Event_List *el;
    int st;
    int fl;

    el = &stage->ev_list[ev_active];
    st = EventListElement(el, 13);
    fl = EventListElement(el, 22);
    if (st == 1) {
        game_flag.flag[fl >> 5] |= 1 << (fl & 31);
    } else {
        game_flag.flag[fl >> 5] &= ~(1 << (fl & 31));
    }
    EventExecSubFlagSet(el);
    return 1;
}

#define EV_E_STEP(n) \
    ev_e_step = (n); \
    ev_p_step = 0;   \
    ev_s_step = 0

static int EventExecMessage(void) {
    struct Event_List *el;
    int msg;
    int flg;

    if (ev_e_step == 0) {
        el = &stage->ev_list[ev_active];
        msg = EventListElement(el, 20);
        fontMessageNum(msg_buffer, msg);
        flg = EventListElement(el, 22);
        if (flg) {
            GAME_FLAG_ON(flg);
        }
        EventExecSubFlagSet(el);
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        EV_E_STEP(2);
    }
    if (fontGetStatus() == -2 || ev_cancel) {
        fontClear();
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EventExecProgram(void) {
    struct Event_List *el;
    int flg;
    int prog;

    el = &stage->ev_list[ev_active];
    if (ev_e_step == 0) {
        EventExecSubFlagSet(el);
        ev_prog_flag_set = 1;
        EV_E_STEP(2);
    }
    prog = EventListElement(el, 19);
    if (stage->ev_prog[prog]()) {
        if (ev_prog_flag_set) {
            flg = EventListElement(el, 22);
            if (flg) {
                GAME_FLAG_ON(flg);
            }
        }
        return 1;
    }
    return 0;
}

static int EventExecDoor(void) {
    struct Event_List *el;
    float pos_v[4];
    char *pos_p;
    int pos_t;
    int st;
    int msg;
    int se;
    int fl;

    if (ev_e_step == 0) {
        el = &stage->ev_list[ev_active];
        st = EventListElement(el, 13);
        se = EventListElement(el, 18);
        pos_p = (char *)stage->ev_pos + EventListElement(el, 8);
        EventPositionSet(pos_v, pos_p, EventListElement(el, 9));
        pos_v[1] += -500.0f;
        if (st == 7) {
            msg = EventListElement(el, 20);
            if (msg == 0xFF) {
                fontMessageNum(msg_station, 5);
            } else {
                fontMessageNum(msg_buffer, msg);
            }
            SeCallPos(door_se[se].unlock, 1.0f, pos_v, 0);
        } else if (st == 9) {
            msg = EventListElement(el, 20);
            if (msg == 0xFF) {
                fontMessageNum(msg_station, 6);
            } else if (msg == 0xFE) {
                fontMessageNum(msg_station, 7);
            } else {
                fontMessageNum(msg_buffer, msg);
            }
            SeCallPos(door_se[se].jam, 1.0f, pos_v, 0);
        } else {
            fontMessageNum(msg_station, 4);
            SeCallPos(door_se[se].lock, 1.0f, pos_v, 0);
        }
        fl = EventListElement(el, 22);
        if (fl) {
            GAME_FLAG_ON(fl);
        }
        EventExecSubFlagSet(el);
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        EV_E_STEP(2);
    }
    if (fontGetStatus() == -2 || ev_cancel) {
        fontClear();
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EventExecItem(void) {
    /* Matching: called without a prototype in the original (arguments passed unconverted). */
    int shCharacter_Manage_Delete();
    static struct Event_ExecItemData eei_data[7] = {
        { 0, 0, 0 },         { 5, 9, 0x703 },   { 7, 10, 0x724 }, { 9, 11, 0x723 },
        { 1, 12, 0x700 },    { 2, 13, 0x701 },  { 3, 14, 0x733 },
    };
    struct Item_List *il;
    int kind;

    switch (ev_e_step) {
    case 0:
        il = &stage->gi_list[ev_active - 0x400];
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        if (sh2jms.player->pos.y - CharToFloat2((char *)&il->pos_y) < 100.0f) {
            PlayerEventAnimeSet(0x4E21);
            EV_E_STEP(4);
        } else {
            PlayerEventAnimeSet(0x65);
            EV_E_STEP(1);
        }
        break;
    case 4:
        if (ev_cancel) {
            EV_E_STEP(6);
        } else if (PlayerEventAnimeSuccessFrame()) {
            shCharacterAnimePause(sh2jms.player);
            EV_E_STEP(1);
        }
        break;
    case 1:
        il = &stage->gi_list[ev_active - 0x400];
        kind = ItemListElement(il, 0);
        GAME_FLAG_ON(ItemListElement(il, 3) + 1);
        ItemGet(eei_data[kind].item);
        fontMessageNum(msg_station, eei_data[kind].msg);
        EV_E_STEP(2);
        break;
    case 2:
        il = &stage->gi_list[ev_active - 0x400];
        if (fontGetStatus() == -2 || ev_cancel) {
            fontClear();
            SeCall(0x2B21, 1.0f, 0);
            shCharacter_Manage_Delete(NULL, eei_data[ItemListElement(il, 0)].chara_id, ItemListElement(il, 3));
            EV_E_STEP(6);
            if (sh2jms.player->pos.y - CharToFloat2((char *)&il->pos_y) < 100.0f) {
                shCharacterAnimeRestart(sh2jms.player);
                EV_E_STEP(5);
            }
        }
        break;
    case 5:
        if (shCharacterAnimeIsEnd(sh2jms.player) || ev_cancel) {
            EV_E_STEP(6);
        }
        break;
    case 6:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EventExecMove(void) {
    static short reset_stage_connect[12][2] = {
        { 4, 5 }, { 5, 13 }, { 14, 12 }, { 14, 18 }, { 14, 23 }, { 14, 28 },
        { 30, 31 }, { 33, 34 }, { 36, 37 }, { 37, 38 }, { 42, 46 }, { 0, 0 },
    };
    static float pos_v[4];
    static short close_se;
    struct Event_List *el;
    char *pos_p;
    int pos_t;
    int se;
    int flg;
    int stg;
    int i;

    switch (ev_e_step) {
    case 0:
        el = &stage->ev_list[ev_active];
        EventExecSubFlagSet(el);
        flg = EventListElement(el, 22);
        if (flg && ev_m_step != 3) {
            GAME_FLAG_ON(flg);
        }
        EventResultMovePosition(ev_active);
        pos_p = (char *)&stage->ev_pos[EventListElement(el, 8)];
        pos_t = EventListElement(el, 9);
        EventPositionSet(pos_v, pos_p, pos_t);
        pos_v[1] += -500.0f;
        se = EventListElement(el, 18);
        SeCallPos(door_se[se].open, 1.0f, pos_v, 0);
        close_se = door_se[se].close;
        pos_p = (char *)&stage->ev_pos[EventListElement(el, 14)];
        pos_t = EventListElement(el, 15);
        EventPositionSet(pos_v, pos_p, pos_t);
        pos_v[1] += -500.0f;
        Sh2sys.main_status |= 2;
        SH2SYS_STEP2(1);
        stg = EventListElement(el, 16);
        if (stg) {
            for (i = 0; reset_stage_connect[i][0] != 0; i++) {
                if ((playing.stage == reset_stage_connect[i][0] && stg == reset_stage_connect[i][1]) ||
                    (playing.stage == reset_stage_connect[i][1] && stg == reset_stage_connect[i][0])) {
                    Sh2sys.main_status |= 8;
                }
            }
            playing.stage = EventListElement(el, 16);
            Sh2sys.step[3] = 0;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
        } else {
            Sh2sys.step[3] = 3;
            Sh2sys.step[4] = 0;
            Sh2sys.step[5] = 0;
            Sh2sys.step[6] = 0;
            Sh2sys.step[7] = 0;
        }
        if (EventListElement(el, 17)) {
            Sh2sys.main_status |= 4;
        } else {
            Sh2sys.main_status &= ~4;
        }
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        EV_E_STEP(3);
        ScreenEffectFadeStart(1, 0.0f);
        break;
    case 3:
        SeCallPos(close_se, 1.0f, pos_v, 0);
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        ScreenEffectFadeStart(4, 0.0f);
        if (ev_m_step == 3) {
            EV_M_STEP(4);
            EventExecProgram();
            break;
        }
        return 1;
    }
    return 0;
}

static int EventExecSave(void) {
    struct Event_List *el;

    switch (ev_e_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        if (GAME_FLAG(5)) {
            EV_E_STEP(7);
        } else {
            fontMessageNum(msg_station, 8);
            EV_E_STEP(1);
        }
        break;
    case 1:
        if (fontGetStatus() == -2 || ev_cancel) {
            fontClear();
            if (ev_cancel) {
                EV_E_STEP(6);
            } else {
                GAME_FLAG_ON(5);
                EV_E_STEP(7);
            }
        }
        break;
    case 7:
        ScreenEffectFadeStart(11, 0.0f);
        el = &stage->ev_list[ev_active];
        SetSavePointName(EventListElement(el, 21));
        SeCall(0x2743, 1.0f, 0);
        SH2SYS_STEP2(9);
        EV_E_STEP(6);
        break;
    case 6:
        ScreenEffectFadeStart(4, 0.0f);
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

/** Returns non-zero if the flashlight spot should be on (the player has the light, it is
 * switched on and game flag 272 is off). */
int LightSpotOnOffCheck(void) {
    if (!ITEM_FLAG(15)) {
        return 0;
    }
    if (item.light_switch == 0) {
        return 0;
    }
    return !GAME_FLAG(272);
}

/** Turns the flashlight spot on or off by LightSpotOnOffCheck (at night only). */
void LightSpotOnOffSet(void) {
    if (!sh2gfw_Get_NightOrDay() || !ITEM_FLAG(15)) {
        return;
    }
    if (LightSpotOnOffCheck()) {
        sh2gfw_On_JmsSPOT();
    } else {
        sh2gfw_Off_JmsSPOT();
    }
}

static int EventExecChizuFail(void) {
    if (ev_e_step == 0) {
        fontMessageNum(msg_station, 2);
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        EV_E_STEP(2);
    }
    if (fontGetStatus() == -2 || ev_cancel) {
        fontClear();
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

/** Returns the first unfinished story step (1, 2, 3...) from game flags, the stage and the
 * player position; the stage overlays test it. */
int EventProgressCheck(void) {
    int room;

    /* Matching: the assert bakes its original line number into the object. */
#line 1885
    assert(stage);
    room = RoomNameJms();
    if (stage->glb_crd == 6 && !GAME_FLAG(32)) {
        return 1;
    }
    if (stage->glb_crd == 1 && !GAME_FLAG(36) && sh2jms.player->pos.x > -20000.0f && sh2jms.player->pos.z > -20000.0f) {
        return 2;
    }
    if (stage->glb_crd == 2 && !GAME_FLAG(43)) {
        return 3;
    }
    if (GAME_FLAG(69) && !GAME_FLAG(70)) {
        return 4;
    }
    if (GAME_FLAG(69) && room == 0x18) {
        return 5;
    }
    if (stage->glb_crd == 3 && !GAME_FLAG(150)) {
        return 6;
    }
    if (GAME_FLAG(150) && !GAME_FLAG(152)) {
        return 7;
    }
    if (stage->glb_crd == 3 && GAME_FLAG(152) && !GAME_FLAG(157)) {
        return 8;
    }
    if (stage->glb_crd == 3 && GAME_FLAG(162) && !GAME_FLAG(163)) {
        return 9;
    }
    if (GAME_FLAG(379) && !GAME_FLAG(380)) {
        return 10;
    }
    return 0;
}

/** Returns non-zero if an item with this level condition exists at the current battle level.
 * @param level the item's level condition
 * @param flag extra condition number (e.g. 3: game flag 240 must be set) */
int EventItemConditionCheck(int level, int flag) {
    switch (playing.battle_level) {
    case 1:
        if (level == 2 || level == 3 || level == 6) {
            return 0;
        }
        break;
    case 2:
        if (level == 1 || level == 3 || level == 5) {
            return 0;
        }
        break;
    case 3:
        if (level == 1 || level == 2 || level == 4) {
            return 0;
        }
        break;
    }
    switch (flag) {
    case 1:
        if (!GAME_FLAG(251)) {
            return 0;
        }
        break;
    case 2:
        if (!GAME_FLAG(108)) {
            return 0;
        }
        break;
    case 3:
        if (!GAME_FLAG(240)) {
            return 0;
        }
        break;
    }
    return 1;
}

/** Per-frame radio noise: starts or stops the noise loop and sets its tracks' volume and pan
 * from the nearby enemies (or a radio event's position). */
void RadioNoise(void) {
    static float enemy_track[16][4] = {
        { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f, 0.0f }, { 0.6f, 0.0f, 0.0f, 0.0f },
        { 0.4f, 0.4f, 0.4f, 1.0f }, { 0.4f, 0.4f, 0.4f, 1.0f }, { 1.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.8f, 0.4f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.8f, 0.0f, 0.2f }, { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.6f, 0.0f, 0.0f, 0.0f }, { 0.6f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f },
    };
    float pos[4];
    struct EnLOCAL_DATA *enp;
    struct EnLOCAL_DATA *near[3];
    struct SubCharacter *scp;
    float volume;
    float track[3];
    float enemy[3];
    float work;
    int pan[3];
    int i;
    int j;
    int k;

    if (Sh2sys.step[1] != 13 || Sh2sys.step[2] == 7 || Sh2sys.step[2] == 9 || Sh2sys.step[2] == 10 ||
        Sh2sys.step[2] == 11 || !item.radio_switch) {
        if (radio.se_call) {
            shSdCall(0x406, 0, 0, 0);
            radio.se_call = 0;
        }
        return;
    }
    if (!radio.se_call) {
        shSdCall(0x2B22, 0, 0, 0);
        for (i = 0; i < 4; i++) {
            radio.track[i] = 0.0f;
        }
        radio.volume = 0.0f;
        radio.se_call = 1;
    }
    if (radio.event == 1) {
        track[0] = itof(SeCallPosDistance(0.8f, radio.pos));
        pan[0] = SeCallPosDirection(radio.pos);
        if (pan[0] > 90) {
            pan[0] = 180 - pan[0];
        }
        if (pan[0] < -90) {
            pan[0] = -180 - pan[0];
        }
        for (i = 0; i < 4; i++) {
            volume = (255.0f - track[0]) / 255.0f;
            radio.track[i] = volume * enemy_track[1][i];
            volume = itof(pan[0]) / 90.0f;
            radio.pan[i] = volume * enemy_track[1][i];
        }
        for (i = 0; i < 4; i++) {
            shSdRadio(i + 1, ftoi(127.0f * radio.track[i]), ftoi(64.0f + -60.0f * radio.pan[i]), 0);
        }
    } else if (radio.event == 2) {
        for (i = 0; i < 32; i++) {
            enp = &enLocalWork.Data[i];
            if (enp->kind) {
                break;
            }
        }
        if (i < 32) {
            work = enp->radio;
        } else {
            work = 1.0f;
        }
        for (i = 0; i < 4; i++) {
            volume = enemy_track[1][i] * work;
            if (volume > radio.track[i]) {
                radio.track[i] = fminf(volume, radio.track[i] + shGetDT() / 2.5f);
            } else if (volume < radio.track[i]) {
                radio.track[i] = fmaxf(volume, radio.track[i] - shGetDT() / 2.5f);
            }
        }
        for (i = 0; i < 4; i++) {
            shSdRadio(i + 1, ftoi(127.0f * radio.track[i]), 64, 0);
        }
    } else {
        for (i = 0; i < 3; i++) {
            near[i] = NULL;
        }
        for (i = 0; i < 32; i++) {
            enp = &enLocalWork.Data[i];
            if (enp->kind && enp->p_dist <= 12500.0f && enp->radio != 0.0f) {
                for (j = 0; j < 3; j++) {
                    if (near[j] == NULL || !(near[j]->p_dist <= enp->p_dist)) {
                        for (k = 2; k > j; k--) {
                            near[k] = near[k - 1];
                        }
                        break;
                    }
                }
                if (j < 3) {
                    near[j] = enp;
                }
            }
        }
        for (i = 0; i < 3; i++) {
            if (near[i]) {
                track[i] = shCosF(3.1415927f * (near[i]->p_dist / 12500.0f) / 2.0f);
                scp = near[i]->scp;
                pos[0] = scp->pos.x;
                pos[1] = scp->pos.y;
                pos[2] = scp->pos.z;
                pos[3] = 0.0f;
                pan[i] = SeCallPosDirection(pos);
            } else {
                track[i] = 0.0f;
                pan[i] = 0;
            }
        }
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 3; j++) {
                if (near[j]) {
                    enemy[j] = near[j]->radio * enemy_track[near[j]->kind][i];
                } else {
                    enemy[j] = 0.0f;
                }
            }
            volume = track[0] * enemy[0] + track[1] * enemy[1] / 3.0f + track[2] * enemy[2] / 6.0f;
            if (volume > 1.0f) {
                volume = 1.0f;
            } else if (volume < 0.0f) {
                volume = 0.0f;
            }
            if (volume > radio.track[i]) {
                radio.track[i] = fminf(volume, radio.track[i] + shGetDT() / 2.5f);
            } else if (volume < radio.track[i]) {
                radio.track[i] = fmaxf(volume, radio.track[i] - shGetDT() / 2.5f);
            }
            for (j = 0; j < 3; j++) {
                if (enemy[j] > 0.0f) {
                    enemy[j] = 1.0f;
                }
            }
            volume = enemy[0] * itof(pan[0]) / 90.0f + enemy[1] * itof(pan[1]) / 90.0f / 3.0f + enemy[2] * itof(pan[2]) / 90.0f / 6.0f;
            if (volume > 1.0f) {
                volume = 1.0f;
            } else if (volume < -1.0f) {
                volume = -1.0f;
            }
            if (volume > radio.pan[i]) {
                radio.pan[i] = fminf(volume, radio.pan[i] + shGetDT() / 2.0f);
            } else if (volume < radio.pan[i]) {
                radio.pan[i] = fmaxf(volume, radio.pan[i] - shGetDT() / 2.0f);
            }
        }
        if (Sh2sys.step[2] == 6 || Sh2sys.step[2] == 5) {
            work = 0.5f;
        } else if (Sh2sys.step[2] == 8) {
            work = 0.25f;
        } else {
            work = 1.0f;
        }
        if (work > radio.volume) {
            radio.volume = fminf(work, radio.volume + shGetDT());
        } else if (work < radio.volume) {
            radio.volume = fmaxf(work, radio.volume - shGetDT());
        }
        for (i = 0; i < 4; i++) {
            shSdRadio(i + 1, ftoi(127.0f * radio.track[i] * radio.volume * itof(item.radio_volume) / 15.0f),
                      ftoi(64.0f + 32.0f * radio.pan[i]), 0);
        }
    }
}
