/*
 * chara_admin.c: character management around room and stage changes: creates and deletes the
 * player, weapon, models, items and enemies of the rooms near the player, and the background
 * loading of enemy data.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "fi_libvu0_inline.h"
#include "sh_vu0.h"

/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)

/* Squared distances. Matching: macros, not inline functions: every sqr() is evaluated before the
 * adds (and before a sqr() on the other side of a compare). */
#define CharaAdminLen2(x0, z0, x1, z1) (sqr((x0) - (x1)) + sqr((z0) - (z1)))
#define CharaAdminLen3(a, b) \
    (sqr((a)->pos.z - (b)->pos.z) + (sqr((a)->pos.x - (b)->pos.x) + sqr((a)->pos.y - (b)->pos.y)))

#define CHARA_ADMIN_IS_NEAR(x0, z0, x1, z1, r, t)     (fabsf((x0) - (x1)) < ((t) = 8000.0f + (r)) && fabsf((z0) - (z1)) < (t) &&      CharaAdminLen2(x0, z0, x1, z1) < sqr(t))

static int RoomDistance(short room0, short room1);
static int RoomDistanceSub(struct CharaAdmin_RoomDistance *dist, short room0, short room1, unsigned long flag);
static void DeleteEnemyWork(void);
static void DeleteEnemyWorkIn(void);
static void DeleteEnemyWorkOut(void);
static int CharaAdminEnemyEntryCondition(short cond);

static struct CharaData_DemoList back_load_admin_list[8];
static int back_load_admin;

/** Creates James (or the stage's player model) at the connect position if he doesn't exist
 * yet, and places him there. */
void ConnectCharaWorkJamesSet(void) {
    float pos[4];
    float rot[4];

    _sceVu0UnitVector(pos);
    _sceVu0UnitVector(rot);
    pos[0] = connect_pos[0];
    pos[1] = connect_pos[1];
    pos[2] = connect_pos[2];
    rot[1] = connect_pos[3];
    if (sh2jms.player == NULL) {
        if (stage->pc_model == 1) {
            CharaWorkCreate(0x101, 0, pos, rot, 0);
        } else {
            CharaWorkCreate(0x100, 0, pos, rot, 0);
        }
    } else if (sh2jms.player->kind == 0x100) {
        if (stage->pc_model == 1) {
            shCharacter_Manage_Delete(NULL, 0x100, 0);
            CharaWorkCreate(0x101, 0, pos, rot, 0);
        }
    } else {
        if (stage->pc_model != 1) {
            shCharacter_Manage_Delete(NULL, 0x101, 0);
            CharaWorkCreate(0x100, 0, pos, rot, 0);
        }
    }
}

/** Creates the model of the equipped weapon for the player. */
void ConnectCharaWorkWeapon(void) {
    static short weapon[9][2] = {
        { 0, 0 }, { 4, 1 }, { 6, 2 }, { 8, 3 }, { 10, 4 }, { 11, 5 }, { 12, 6 }, { 13, 8 }, { 14, 7 },
    };
    struct SubCharacter *scp;
    float dummy[4];
    int id;
    int i;

    for (i = 0; i < 9; i++) {
        if (item.equip == weapon[i][0]) {
            break;
        }
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 130
    assert_dw(i < 9);
    id = PlayerGetJamesWeapon();
    shCharacter_Manage_Delete(NULL, id + 0x800, 0);
    shCharacter_Manage_Delete(NULL, id + 0x820, 0);
    if (id != 0 && id != weapon[i][1]) {
        CharaDataDeleteOne(id + 0x820);
    }
    CharaDataLoadWeapon();
    _sceVu0UnitVector(dummy);
    if (weapon[i][1] != 0) {
        scp = CharaWorkCreate(weapon[i][1] + 0x800, 0, dummy, dummy, 0);

        /* Matching: the assert bakes its original line number into the object. */
#line 144
        assert_dw(scp);
        if ((Sh2sys.main_status >> 4) & 1) {
            scp = CharaWorkCreate(weapon[i][1] + 0x820, 0, dummy, dummy, 0);

            /* Matching: the assert bakes its original line number into the object. */
#line 149
            assert_dw(scp);
        } else {
            scp = shCharacterGetSubCharacter(weapon[i][1] + 0x820, 0);
            if (scp != NULL) {
                shCharacter_Manage_Delete(scp, 0, 0);
            }
        }
    }
    JamesWeaponSet(weapon[i][1]);
}

/** Clears the enemies and deletes every character except the players; enemies with no HP
 * left are recorded as dead in game_flag.enemy. */
void ConnectCharaWorkReset(void) {
    struct SubCharacter *scp;
    struct SubCharacter *next;

    enInitEnemy();
    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = next) {
        next = scp->next;
        if (scp->kind == 0x100 || scp->kind == 0x101 || scp->kind == 0x105 || (scp->kind >> 8) == 8) {
            continue;
        }
        if ((scp->kind >> 8) == 2 && scp->battle.hp == 0.0f) {
            game_flag.enemy[scp->id >> 5] |= 1 << (scp->id & 0x1F);
        }
        shCharacter_Manage_Delete(scp, 0, 0);
    }
}

/* FAKEMATCH: near_r and near_r2 (not in the DWARF) hold 8000 + work only so that the sum gets its own FPR,
 * as in the original (docs/matching-notes.md#chara_admin-connectcharaworkadminout). */
/** Removes the characters the player has left behind: far enemies are deactivated, far
 * models and items deleted; then (re)creates Maria and the other characters the game flags call for.
 * @param load_check non-zero to only finish a pending background load (and do nothing during an event) */
void ConnectCharaWorkAdminOut(int load_check) {
    struct SubCharacter *scp;
    struct SubCharacter *next;
    struct Model_List *mp;
    struct Item_List *ip;
    struct Enemy_List *ep;
    float pos[4];
    float rot[4];
    int active_enemy;
    int kind;
    int flag;
    float work;
    float near_r;
    float near_r2;

    if (ev_m_step != 0 && load_check != 0) {
        return;
    }
    if (load_check) {
        if (back_load_admin && LoadBgEventIsLoad()) {
            LoadBgEventDispose();
            CharaDataBackInit(back_load_admin_list);
            back_load_admin = 0;
        }
    } else {
        back_load_admin = 0;
    }

    for (scp = shCharacter_Manage_GetCharacterList(), active_enemy = 0; scp != NULL; scp = next) {
        next = scp->next;
        if ((scp->kind >> 8) == 2 && ((scp->battle.status >> 10) & 1)) {
            if (scp->id < 17 ||
                fabsf(scp->pos.x - sh2jms.player->pos.x) + fabsf(scp->pos.z - sh2jms.player->pos.z) < 16000.0f ||
                CharaAdminLen2(scp->pos.x, scp->pos.z, sh2jms.player->pos.x, sh2jms.player->pos.z) <
                    16000.0f * 16000.0f) {
                active_enemy++;
            } else {
                scp->battle.status &= ~0x400;
                scp->status &= ~0x10;
            }
        } else if ((scp->kind >> 8) == 3 || (scp->kind >> 8) == 4 || (scp->kind >> 8) == 5 || (scp->kind >> 8) == 7) {
            work = CharaGetBoundR(scp->kind);
            if (!BgCharaIsId(scp->kind) || BgCharaIsLoad(scp->kind)) {
                if (fabsf(scp->pos.x - sh2jms.player->pos.x) + fabsf(scp->pos.z - sh2jms.player->pos.z) <= (near_r = 8000.0f + work) ||
                    CharaAdminLen2(scp->pos.x, scp->pos.z, sh2jms.player->pos.x, sh2jms.player->pos.z) <=
                        sqr(near_r)) {
                    continue;
                }
            }
            shCharacter_Manage_Delete(scp, 0, 0);
        }
    }

    if (GAME_FLAG(15)) {
        scp = shCharacterGetSubCharacter(0x105, 0);
        if (scp == NULL) {
            scp = CharaWorkCreate(0x105, 0, pos, rot, 0);
        }
        if (scp != NULL && !((Sh2sys.main_status >> 6) & 1)) {
            scp->status |= 0x10;
        }
    }

    for (mp = stage->mdl_list; mp != NULL && mp->kind != 0; mp++) {
        if (mp->flag_off != 0 && GAME_FLAG(mp->flag_off)) {
            continue;
        }
        if (mp->flag_on != 0 && !GAME_FLAG(mp->flag_on)) {
            continue;
        }
        if (BgCharaIsId(mp->kind) && !BgCharaIsLoad(mp->kind)) {
            continue;
        }
        if (shCharacterGetSubCharacter(mp->kind, mp->id) != NULL) {
            continue;
        }
        work = CharaGetBoundR(mp->kind);
        if (CHARA_ADMIN_IS_NEAR(mp->pos[0], mp->pos[2], sh2jms.player->pos.x, sh2jms.player->pos.z, work, near_r2)) {
            scp = CharaWorkCreate(mp->kind, mp->id, mp->pos, mp->rot, 0);
            if (scp != NULL) {
                scp->status |= 0x10;
            }
        }
    }

    _sceVu0UnitVector(pos);
    _sceVu0UnitVector(rot);
    for (ip = stage->gi_list; ip != NULL; ip++) {
        kind = (ip->st >> 29) & 7;
        if (kind == 7) {
            break;
        }
        flag = ip->st & 0x3FFF;
        if (GAME_FLAG(flag)) {
            continue;
        }
        if (GAME_FLAG(flag + 1)) {
            continue;
        }
        if (!EventItemConditionCheck((ip->st >> 26) & 7, (ip->st >> 20) & 0x1F)) {
            continue;
        }
        switch (kind) {
        case 1:
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
        if (shCharacterGetSubCharacter(kind, flag) != NULL) {
            continue;
        }
        work = CharaGetBoundR(kind);
        if (CHARA_ADMIN_IS_NEAR(ip->pos_x, ip->pos_z, sh2jms.player->pos.x, sh2jms.player->pos.z, work, near_r2)) {
            pos[0] = ip->pos_x;
            pos[1] = CharToFloat2((char *)&ip->pos_y);
            pos[2] = ip->pos_z;
            rot[1] = CharToFloat2((char *)&ip->rot_y);
            CharaWorkCreate(kind, flag, pos, rot, 0)->status |= 0x10;
        }
    }

    for (ep = stage->en_list; ep != NULL && ep->kind != 0; ep++) {
        if (active_enemy > 2) {
            break;
        }
        scp = shCharacterGetSubCharacter(ep->kind, ep->id);
        if (scp != NULL && ((scp->battle.status >> 10) & 1)) {
            continue;
        }
        if (!CharaAdminEnemyEntryCheck(ep, 0)) {
            continue;
        }
        if (!sh2gfw_Check_ModelIsOnMemory(ep->kind)) {
            continue;
        }
        work = CharaAdminLen2((float)ep->pos_x, (float)ep->pos_z, sh2jms.player->pos.x, sh2jms.player->pos.z);
        if (work <= 8000.0f * 8000.0f) {
            continue;
        }
        if (!(work < 16000.0f * 16000.0f)) {
            continue;
        }
        if (scp == NULL) {
            pos[0] = ep->pos_x;
            pos[1] = ep->pos_y;
            pos[2] = ep->pos_z;
            rot[1] = itof(ep->rot_y) / 4096.0f;
            scp = CharaWorkCreate(ep->kind, ep->id, pos, rot, ep->status);
        }
        if (scp != NULL) {
            scp->battle.status |= 0x400;
            scp->status |= 0x10;
            active_enemy++;
        }
    }
}

/** Creates the models, items and enemies of the room the player is in. */
void ConnectCharaWorkAdminIn(void) {
    struct SubCharacter *scp;
    struct SubCharacter *next;
    struct Model_List *mp;
    struct Item_List *ip;
    struct Enemy_List *ep;
    float pos[4];
    float rot[4];
    int kind;
    int flag;
    int room;

    room = RoomNameJms();
    _sceVu0UnitVector(pos);
    _sceVu0UnitVector(rot);
    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = next) {
        next = scp->next;
        if ((scp->kind >> 8) == 2) {
            scp->battle.status &= ~0x400;
            scp->status &= ~0x10;
        } else if (scp->kind >= 0x400) {
            shCharacter_Manage_Delete(scp, 0, 0);
        }
    }

    scp = shCharacterGetSubCharacter(0x105, 0);
    if (GAME_FLAG(15)) {
        if (scp == NULL) {
            scp = CharaWorkCreate(0x105, 0, pos, rot, 0);
        }
        if (scp != NULL) {
            scp->status |= 0x10;
        }
    } else {
        CharaDataDeleteOne(0x105);
    }

    for (mp = stage->mdl_list; mp != NULL && mp->kind != 0; mp++) {
        if (mp->flag_off != 0 && GAME_FLAG(mp->flag_off)) {
            continue;
        }
        if (mp->flag_on != 0 && !GAME_FLAG(mp->flag_on)) {
            continue;
        }
        if (room != RoomName(0, mp->pos[0], mp->pos[2])) {
            continue;
        }
        scp = shCharacterGetSubCharacter(mp->kind, mp->id);
        if (scp == NULL) {
            scp = CharaWorkCreate(mp->kind, mp->id, mp->pos, mp->rot, 0);
        }
        if (scp != NULL) {
            scp->status |= 0x10;
        }
    }

    _sceVu0UnitVector(pos);
    _sceVu0UnitVector(rot);
    for (ip = stage->gi_list; ip != NULL; ip++) {
        kind = (ip->st >> 29) & 7;
        if (kind == 7) {
            break;
        }
        flag = ip->st & 0x3FFF;
        if (GAME_FLAG(flag)) {
            continue;
        }
        if (GAME_FLAG(flag + 1)) {
            continue;
        }
        if (room != RoomName(0, ip->pos_x, ip->pos_z)) {
            continue;
        }
        pos[0] = ip->pos_x;
        pos[1] = CharToFloat2((char *)&ip->pos_y);
        pos[2] = ip->pos_z;
        rot[1] = CharToFloat2((char *)&ip->rot_y);
        switch (kind) {
        default:
        case 0:
        case 1:
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
            scp = CharaWorkCreate(kind, flag, pos, rot, 0);
        }
        if (scp != NULL) {
            scp->status |= 0x10;
        }
    }

    for (ep = stage->en_list; ep != NULL && ep->kind != 0; ep++) {
        if (!CharaAdminEnemyEntryCheck(ep, room)) {
            continue;
        }
        scp = shCharacterGetSubCharacter(ep->kind, ep->id);
        if (scp != NULL && CharaAdminLen3(sh2jms.player, scp) < sqr(500.0f)) {
            shCharacter_Manage_Delete(scp, 0, 0);
            scp = NULL;
        }
        if (scp == NULL) {
            pos[0] = ep->pos_x;
            pos[1] = ep->pos_y;
            pos[2] = ep->pos_z;
            rot[1] = itof(ep->rot_y) / 4096.0f;
            scp = CharaWorkCreate(ep->kind, ep->id, pos, rot, ep->status);
        }
        if (scp != NULL) {
            scp->battle.status |= 0x400;
            scp->status |= 0x10;
        }
    }

    shCharacter_Manage_Delete(NULL, 0x121, 0);
    shCharacter_Manage_Delete(NULL, 0x120, 0);
    if ((Sh2sys.main_status >> 4) & 1) {
        _sceVu0UnitVector(pos);
        _sceVu0UnitVector(rot);
        if (stage->pc_model == 1) {
            scp = CharaWorkCreate(0x121, 0, pos, rot, 0);
        } else {
            scp = CharaWorkCreate(0x120, 0, pos, rot, 0);
        }
        if (scp != NULL) {
            scp->status |= 0x10;
        }
    }
}

/** Deletes the characters that don't carry over between stages. */
void ConnectCharaWorkAdminClear(void) {
    struct SubCharacter *scp;
    struct SubCharacter *next;

    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = next) {
        next = scp->next;
        switch (scp->kind) {
        case 0x104:
        case 0x106:
        case 0x107:
        case 0x108:
        case 0x109:
        case 0x10A:
        case 0x10B:
        case 0x10C:
        case 0x10D:
        case 0x120:
        case 0x121:
        case 0x821:
        case 0x822:
        case 0x823:
        case 0x824:
        case 0x825:
        case 0x826:
        case 0x827:
        case 0x828:
        case 0x829:
            shCharacter_Manage_Delete(scp, 0, 0);
            break;
        }
    }
}

static struct CharaAdmin_RoomDistance room_dist_hsp_f[31] = {
    { 41, 51 }, { 41, 59 }, { 41, 68 }, { 41, 43 }, { 42, 52 }, { 42, 60 },
    { 42, 69 }, { 44, 45 }, { 44, 59 }, { 46, 51 }, { 46, 47 }, { 47, 51 },
    { 48, 52 }, { 49, 52 }, { 51, 52 }, { 53, 59 }, { 54, 59 }, { 55, 60 },
    { 56, 60 }, { 58, 60 }, { 59, 60 }, { 61, 63 }, { 61, 68 }, { 62, 63 },
    { 62, 68 }, { 64, 69 }, { 65, 69 }, { 66, 69 }, { 67, 69 }, { 68, 69 },
    { 0, 0 },
};

static struct CharaAdmin_RoomDistance room_dist_hsp_b[18] = {
    { 70, 78 }, { 70, 80 }, { 70, 87 }, { 71, 88 }, { 71, 89 }, { 72, 87 },
    { 73, 79 }, { 74, 77 }, { 76, 78 }, { 78, 79 }, { 80, 82 }, { 80, 83 },
    { 84, 88 }, { 85, 87 }, { 86, 87 }, { 87, 88 }, { 89, 90 }, { 0, 0 },
};

static struct CharaAdmin_RoomDistance room_dist_apt[30] = {
    { 7, 16 }, { 7, 18 }, { 15, 18 }, { 15, 21 }, { 15, 31 }, { 16, 21 },
    { 16, 31 }, { 17, 31 }, { 18, 20 }, { 18, 29 }, { 19, 31 }, { 21, 22 },
    { 21, 23 }, { 21, 24 }, { 21, 25 }, { 21, 26 }, { 21, 39 }, { 24, 25 },
    { 27, 31 }, { 28, 31 }, { 29, 30 }, { 32, 37 }, { 32, 38 }, { 33, 38 },
    { 34, 37 }, { 35, 36 }, { 35, 37 }, { 38, 39 }, { 38, 40 }, { 0, 0 },
};

static int RoomDistance(short room0, short room1) {
    int len;

    switch (playing.stage) {
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        len = RoomDistanceSub(room_dist_apt, room0, room1, 0);
        break;
    case 18:
    case 19:
    case 20:
    case 21:
        len = RoomDistanceSub(room_dist_hsp_f, room0, room1, 0);
        break;
    case 22:
    case 24:
    case 25:
    case 26:
        len = RoomDistanceSub(room_dist_hsp_b, room0, room1, 0);
        break;
    default:
        return 0xFFFF;
    }
    if (len == -1) {
        return 0xFFFF;
    }
    return len;
}

/* Fewest connections between two rooms (flag: connections already used). */
static int RoomDistanceSub(struct CharaAdmin_RoomDistance *dist, short room0, short room1, unsigned long flag) {
    int len;
    int work;
    int i;

    if (room0 > room1) {
        work = room1;
        room1 = room0;
        room0 = work;
    }
    for (i = 0; dist[i].rm0; i++) {
        if (room0 == dist[i].rm0 && room1 == dist[i].rm1) {
            return 1;
        }
    }
    len = -1;
    for (i = 0; dist[i].rm0; i++) {
        if (flag & (1 << i)) {
            continue;
        }
        if (room0 == dist[i].rm0) {
            work = RoomDistanceSub(dist, dist[i].rm1, room1, flag + (1 << i));
        } else if (room0 == dist[i].rm1) {
            work = RoomDistanceSub(dist, dist[i].rm0, room1, flag + (1 << i));
        } else {
            work = -1;
        }
        if (work != -1) {
            if (len == -1 || work + 1 < len) {
                len = work + 1;
            }
        }
    }
    if (flag == 0 && len == -1) {
        return 0;
    }
    return len;
}

/** Creates a character and shows it; when the character or skeleton pool is full, deletes
 * enemies and retries.
 * @param kind character kind
 * @param id character id
 * @param pos position
 * @param rot rotation
 * @param status initial status
 * @return the character, or NULL */
struct SubCharacter *CharaWorkCreate(short kind, short id, float *pos, float *rot, unsigned int status) {
    struct SubCharacter *scp;
    int sk_last;

    sk_last = sh2skelton.last - shCharacterGetSkeltonNum(kind);
    if (sk_last <= 0 || sh2chara.total == 32) {
        DeleteEnemyWork();
        return CharaWorkCreate(kind, id, pos, rot, status);
    }
    shCharacter_Manage_Create(kind, id, pos, rot, status);
    scp = shCharacterGetSubCharacter(kind, id);
    if (scp != NULL) {
        scp->status |= 0x10;
    }
    return scp;
}

static void DeleteEnemyWork(void) {
    switch (stage->glb_crd) {
    default:
        DeleteEnemyWorkIn();
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        DeleteEnemyWorkOut();
        break;
    }
}

static void DeleteEnemyWorkIn(void) {
    struct SubCharacter *scp;
    struct SubCharacter *del;
    int j_room;
    int e_room;
    int del_point;
    int work;

    j_room = RoomName(0, sh2jms.player->pos.x, sh2jms.player->pos.z);
    del_point = 0;
    del = NULL;
    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = scp->next) {
        if ((scp->kind >> 8) == 2) {
            e_room = RoomName(0, scp->pos.x, scp->pos.z);
            if (j_room != e_room) {
                work = RoomDistance(j_room, e_room);
                if (scp->battle.hp <= 0.0f) {
                    work += 4;
                }
                if (del_point < work) {
                    del = scp;
                    del_point = work;
                }
            }
        }
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 739
    assert(del);
    if (del->battle.hp <= 0.0f) {
        game_flag.enemy[del->id >> 5] |= 1 << (del->id & 0x1F);
    }
    shCharacter_Manage_Delete(del, 0, 0);
}

static void DeleteEnemyWorkOut(void) {
    struct SubCharacter *scp;
    struct SubCharacter *del;
    float vec[4];
    float len;
    float far;

    _sceVu0UnitVector(vec);
    del = NULL;
    far = 16000.0f;
    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = scp->next) {
        if ((scp->kind >> 8) == 2) {
            vec[0] = scp->pos.x - sh2jms.player->pos.x;
            vec[2] = scp->pos.z - sh2jms.player->pos.z;
            len = _shLengthXZ(vec);
            if (scp->battle.hp <= 0.0f) {
                len += 32000.0f;
            }
            if (len >= far) {
                far = len;
                del = scp;
            }
        }
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 775
    assert(del);
    if (del->battle.hp <= 0.0f) {
        game_flag.enemy[del->id >> 5] |= 1 << (del->id & 0x1F);
    }
    shCharacter_Manage_Delete(del, 0, 0);
}

/* Whether an enemy of the stage list should be created in the given room. */
/** Returns non-zero if an enemy of the stage's list may appear: not killed, in room (if given)
 * and its entry condition met.
 * @param ep the enemy's list entry
 * @param room room number, or 0 for any */
int CharaAdminEnemyEntryCheck(struct Enemy_List *ep, int room) {
    if ((game_flag.enemy[ep->id >> 5] >> (ep->id & 0x1F)) & 1) {
        return 0;
    }
    if (room != 0 && room != RoomName(0, itof(ep->pos_x), itof(ep->pos_z))) {
        return 0;
    }
    if (!CharaAdminEnemyEntryCondition(ep->condition)) {
        return 0;
    }
    return 1;
}

static int CharaAdminEnemyEntryCondition(short cond) {
    if (playing.battle_level == 1) {
        if (cond & 1) {
            return 0;
        }
    } else if (playing.battle_level == 2) {
        if (cond & 2) {
            return 0;
        }
    } else if (playing.battle_level == 3) {
        if (cond & 4) {
            return 0;
        }
    }
    switch (cond & 0xFFC0) {
    case 0x40:
        if (!GAME_FLAG(43) || GAME_FLAG(251)) {
            return 0;
        }
        break;
    case 0x80:
        if (!GAME_FLAG(67)) {
            return 0;
        }
        break;
    case 0x100:
        if (!GAME_FLAG(91)) {
            return 0;
        }
        break;
    case 0x440:
        if (GAME_FLAG(101) || GAME_FLAG(99)) {
            return 0;
        }
        break;
    case 0x140:
        if (!GAME_FLAG(146)) {
            return 0;
        }
        break;
    case 0xC0:
        if (!GAME_FLAG(483)) {
            return 0;
        }
        break;
    case 0x180:
        if (!GAME_FLAG(68) || GAME_FLAG(70)) {
            return 0;
        }
        break;
    case 0x1C0:
        if (GAME_FLAG(68) && !GAME_FLAG(70)) {
            return 0;
        }
        break;
    case 0x2C0:
        if (!GAME_FLAG(154) || GAME_FLAG(251)) {
            return 0;
        }
        break;
    case 0x400:
        if (!GAME_FLAG(154) || GAME_FLAG(162)) {
            return 0;
        }
        break;
    case 0x3C0:
        if (!GAME_FLAG(154) || (GAME_FLAG(156) && !GAME_FLAG(157)) || GAME_FLAG(162)) {
            return 0;
        }
        break;
    case 0x300:
        if ((item.flag[1] >> 20) & 1) {
            return 0;
        }
        break;
    case 0x200:
        if (!GAME_FLAG(335)) {
            return 0;
        }
        break;
    case 0x240:
        if (!GAME_FLAG(365)) {
            return 0;
        }
        break;
    case 0x280:
        if (GAME_FLAG(329) && !GAME_FLAG(330)) {
            return 0;
        }
        break;
    case 0x340:
        if (!GAME_FLAG(251)) {
            return 0;
        }
        break;
    case 0x380:
        if (GAME_FLAG(482)) {
            return 0;
        }
        break;
    }
    return 1;
}

/** Shows or hides the player characters (James, Maria and the other playable kinds).
 * @param xxx non-zero to show them */
void CharaAdminPlayableDisplay(int xxx) {
    struct SubCharacter *scp;

    for (scp = shCharacter_Manage_GetCharacterList(); scp != NULL; scp = scp->next) {
        switch (scp->kind) {
        case 0x100:
        case 0x120:
        case 0x101:
        case 0x121:
        case 0x105:
        case 0x125:
        case 0x801:
        case 0x821:
        case 0x802:
        case 0x822:
        case 0x803:
        case 0x823:
        case 0x804:
        case 0x824:
        case 0x805:
        case 0x825:
        case 0x806:
        case 0x826:
        case 0x807:
        case 0x827:
        case 0x808:
        case 0x828:
            if (xxx) {
                scp->status |= 0x10;
            } else {
                scp->status &= ~0x10;
            }
            break;
        }
    }
}

/** Deletes character (kind, old_id) and creates (kind, new_id) in its place.
 * @param kind character kind
 * @param new_id id of the new character
 * @param old_id id of the one to delete
 * @param pos position
 * @param rot rotation
 * @param status initial status */
struct SubCharacter *CharaAdminReCreate(int kind, int new_id, int old_id, float *pos, float *rot, int status) {
    shCharacter_Manage_Delete(NULL, kind, old_id);
    return CharaWorkCreate(kind, new_id, pos, rot, status);
}

/** Starts loading the enemy data of a demo list in the background; ConnectCharaWorkAdminOut
 * finishes it.
 * @param list the demo's character list, ended by kind 0 */
void CharaAdminBackLoadEnemy(struct CharaData_DemoList *list) {
    int i;

    CharaDataBackLoadInit();
    CharaDataLoadDemo(list, 4);
    back_load_admin = 1;
    i = 0;
    while (list->kind != 0) {
        /* Matching: the assert bakes its original line number into the object. */
#line 962
        assert_dw(i < ( 8 - 1 ));
        back_load_admin_list[i].kind = list->kind;
        back_load_admin_list[i].model = list->model;
        back_load_admin_list[i].animation = list->animation;
        back_load_admin_list[i].shadow = list->shadow;
        back_load_admin_list[i].cluster = list->cluster;
        list++;
        i++;
    }
    back_load_admin_list[i].kind = 0;
}
