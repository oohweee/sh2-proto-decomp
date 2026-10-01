/*
 * item.c: the player's items. item.flag[] has one bit per item kind (have / don't
 * have); item.number[] counts the stackable ones (medicine, weapons and ammo).
 */
#include "sh2.h"

/* x clamped to at most max, with slt/movn through t7 (inline asm in the original; not in
 * asm_helpers.h, only this file uses it). */
inline int imax_limit(int max, int x) {
    asm {
        slt t7, max, x
        movn x, max, t7
    }
    return x;
}

/* Has item k? */
#define ITEM_FLAG(k) ((*(item.flag+((k)>>5))>>((k)&31))&1)
#define ITEM_FLAG_SET(k) (*(item.flag + ((k) >> 5)) |= 1 << ((k) & 31))
#define ITEM_FLAG_CLEAR(k) (*(item.flag + ((k) >> 5)) &= ~(1 << ((k) & 31)))

/** Clears the items for a new game and gives the starting ones (items 0x11 and 0x12), light and
 * radio on. */
void ItemDataInit(void) {
    shQzero(&item, sizeof(struct Item));
    ItemGet(0x11);
    ItemGet(0x12);
    item.light_switch = 1;
    item.radio_switch = 1;
    item.radio_volume = 12;
}

/** Adds item kind: sets its flag, counts it in the statistics (except the starting items) and
 * adds the amount a pick-up of it gives.
 * @param kind item kind */
void ItemGet(int kind) {
    int work;

    ITEM_FLAG_SET(kind);
    if (kind != 0x11 && kind != 0x12) {
        GameItemGetCountUp();
    }
    switch (kind) {
    case 4:
    case 5:
        work = 10;
        break;
    case 6:
    case 7:
        work = 6;
        break;
    case 8:
    case 9:
        work = 4;
        break;
    case 10:
        work = 8;
        break;
    case 1:
    case 2:
    case 3:
        work = 1;
        break;
    default:
        return;
    }
    switch (kind) {
    case 5:
    case 7:
    case 9:
        work *= playing.bullet_adjust;
        break;
    }
    item.number[kind] = imax_limit(999, item.number[kind] + work);
}

/** Uses up one of item kind: decrements its count, and clears its flag once none are left
 * (weapons keep theirs). Kinds 75 and up are combinations and clear their parts.
 * @param kind item kind
 * @return 1 if some are left, else 0 */
int ItemUse(int kind) {
    if (kind <= 10 && item.number[kind] > 0) {
        item.number[kind]--;
        if (item.number[kind] != 0) {
            return 1;
        }
    }
    if (kind < 75) {
        if (kind != 4 && kind != 6 && kind != 8 && kind != 10) {
            ITEM_FLAG_CLEAR(kind);
        }
    } else {
        switch (kind) {
        case 75:
            ITEM_FLAG_CLEAR(32);
            ITEM_FLAG_CLEAR(33);
            break;
        case 76:
            ITEM_FLAG_CLEAR(50);
            ITEM_FLAG_CLEAR(51);
            break;
        case 77:
            ITEM_FLAG_CLEAR(53);
            ITEM_FLAG_CLEAR(54);
            break;
        case 78:
            ITEM_FLAG_CLEAR(56);
            ITEM_FLAG_CLEAR(57);
            break;
        case 79:
            ITEM_FLAG_CLEAR(56);
            ITEM_FLAG_CLEAR(58);
            break;
        case 80:
            ITEM_FLAG_CLEAR(57);
            ITEM_FLAG_CLEAR(58);
            break;
        case 81:
            ITEM_FLAG_CLEAR(56);
            ITEM_FLAG_CLEAR(57);
            ITEM_FLAG_CLEAR(58);
            break;
        case 85:
            ITEM_FLAG_CLEAR(59);
            ITEM_FLAG_CLEAR(60);
            ITEM_FLAG_CLEAR(61);
            break;
        }
    }
    return 0;
}

/** Checks (and optionally spends) a shot of a firearm (4, 6, 8, 10); other kinds always
 * can shoot.
 * @param kind weapon kind, or 0 for the equipped one
 * @param use non-zero to spend one round
 * @return the rounds loaded (1 for weapons without ammunition), 0 if the weapon isn't owned */
int ItemWeaponShoot(int kind, int use) {
    if (kind == 0) {
        kind = item.equip;
    }
    switch (kind) {
    case 4:
    case 6:
    case 8:
    case 10:
        break;
    default:
        return 1;
    }
    if (!ITEM_FLAG(kind)) {
        return 0;
    }
    if (use) {
        ItemUse(kind);
    }
    return item.number[kind];
}

/** Reloads a firearm from its ammunition (kind + 1): the magazine is filled up to its size
 * (10, 6 or 4).
 * @param kind weapon or ammunition kind, or 0 for the equipped weapon
 * @param use non-zero to reload, 0 to only return the ammunition left
 * @return the rounds loaded after reloading (or the ammunition left when use is 0) */
int ItemWeaponReload(int kind, int use) {
    int weapon;
    int bullet;
    int work;

    if (kind == 0) {
        kind = item.equip;
    }
    switch (kind) {
    case 4:
    case 6:
    case 8:
        weapon = kind;
        kind++;
        break;
    case 5:
    case 7:
    case 9:
        weapon = kind - 1;
        break;
    case 10:
        if (use == 0) {
            return 1;
        } else {
            item.number[10] = 8;
            return 8;
        }
    default:
        return 1;
    }
    if (!use) {
        return item.number[kind];
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 230
    assert(((*(item.flag+((weapon)>>5))>>((weapon)&31))&1));
    item.number[kind] += item.number[weapon];
    item.number[weapon] = 0;
    switch (weapon) {
    case 4:
    default:
        work = 10;
        break;
    case 6:
        work = 6;
        break;
    case 8:
        work = 4;
        break;
    }
    if (work >= item.number[kind]) {
        work = item.number[kind];
        ITEM_FLAG_CLEAR(kind);
    }
    item.number[weapon] += (unsigned short)work;
    item.number[kind] -= (unsigned short)work;
    bullet = item.number[weapon];
    return bullet;
}

/** Uses a medicine (1, 2, 3), healing James by a quarter, a half or all of his maximum HP;
 * the ampoule (3) also starts its 600-unit effect.
 * @param kind item kind
 * @return 1 if used, 0 if not a medicine or none left */
int ItemMedicineUse(int kind) {
    if (kind != 1 && kind != 2 && kind != 3) {
        return 0;
    }
    if (item.number[kind] == 0) {
        return 0;
    }
    ItemUse(kind);
    if (kind == 1) {
        sh2jms.player->battle.hp += 0.25f * sh2jms.player->battle.hp_max;
    } else if (kind == 2) {
        sh2jms.player->battle.hp += 0.5f * sh2jms.player->battle.hp_max;
    } else {
        sh2jms.player->battle.hp += sh2jms.player->battle.hp_max;
        item.ampoule_efficacy = 600.0f;
    }
    if (sh2jms.player->battle.hp > sh2jms.player->battle.hp_max) {
        sh2jms.player->battle.hp = sh2jms.player->battle.hp_max;
    }
    return 1;
}

/** Returns the ampoule's remaining effect, 0 to 1 (full above 300). */
float ItemAmpolueEfficacy(void) {
    float work;

    work = item.ampoule_efficacy;
    if (work == 0.0f) {
        return 0.0f;
    }
    if (work > 300.0f) {
        return 1.0f;
    }
    return work / 300.0f;
}

/** Checks whether the item (or combination of up to three items) the player uses triggers an
 * event.
 * @param kind_0 first item
 * @param kind_1 second item, or 0
 * @param kind_2 third item, or 0
 * @return the event (EventCheck), or -2 if the items don't combine */
int ItemEventCheck(int kind_0, int kind_1, int kind_2) {
    int use_item;

    use_item = ItemCombinationUseCheck(kind_0, kind_1, kind_2);
    if (use_item == 0) {
        return -2;
    } else {
        return EventCheck(0, use_item, 1);
    }
}

/** Returns the item being used: the item itself when only one is given, else the
 * combination item made from them (0 if they don't combine).
 * @param kind_0 first item
 * @param kind_1 second item, or 0
 * @param kind_2 third item, or 0 */
int ItemCombinationUseCheck(int kind_0, int kind_1, int kind_2) {
    static int cmb_check[11][4] = {
        { 0x00, 0x20, 0x21, 0x4B },
        { 0x00, 0x32, 0x33, 0x4C },
        { 0x00, 0x35, 0x36, 0x4D },
        { 0x00, 0x38, 0x39, 0x4E },
        { 0x00, 0x38, 0x3A, 0x4F },
        { 0x00, 0x39, 0x3A, 0x50 },
        { 0x38, 0x39, 0x3A, 0x51 },
        { 0x00, 0x3B, 0x3C, 0x52 },
        { 0x00, 0x3B, 0x3D, 0x53 },
        { 0x00, 0x3C, 0x3D, 0x54 },
        { 0x3B, 0x3C, 0x3D, 0x55 },
    };
    int kind_x;
    int i;

    if (kind_0 > kind_1) {
        kind_x = kind_0;
        kind_0 = kind_1;
        kind_1 = kind_x;
    }
    if (kind_0 > kind_2) {
        kind_x = kind_0;
        kind_0 = kind_2;
        kind_2 = kind_x;
    }
    if (kind_1 > kind_2) {
        kind_x = kind_1;
        kind_1 = kind_2;
        kind_2 = kind_x;
    }
    if (kind_1 == 0) {
        return kind_2;
    }
    kind_x = 0;
    for (i = 0; i < 11; i++) {
        if (kind_0 == cmb_check[i][0] && kind_1 == cmb_check[i][1] && kind_2 == cmb_check[i][2]) {
            kind_x = cmb_check[i][3];
            break;
        }
    }
    return kind_x;
}

/** Runs the stage's event program 1 (putting an item on a shelf). */
void ItemPutForShelf(void) {
    stage->ev_prog[1]();
}
