/*
 * en_common.c: code shared by all enemies: the enemy table (enLocalWork), creation and
 * deletion, HP and damage, movement and paths, collision checks against the player and the
 * stage, sight and hearing, animation, sleep/regeneration, and the per-kind dispatch to the
 * en_*.c files.
 */
#include "enemy.h"
#include "fi_libvu0_inline.h"


/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)

enum {
    EnKIND_NONE,
    EnKIND_SCU,
    EnKIND_MKN,
    EnKIND_TYU,
    EnKIND_RED,
    EnKIND_ONI,
    EnKIND_NSE,
    EnKIND_IKE,
    EnKIND_PAP,
    EnKIND_EDB,
    EnKIND_ARM,
    EnKIND_BOS,
    EnKIND_NIK,
    EnKIND_TY2,
    EnKIND_TY3,
    EnKIND_INS,
};

int (*EnAnimeSetFunc[12])(struct SubCharacter *, int, int) = {
    NULL,
    shCharacterEnemySCUAnimeSetP,
    shCharacterEnemyMKNAnimeSetP,
    shCharacterEnemyTYUAnimeSetP,
    shCharacterEnemyREDAnimeSetP,
    shCharacterEnemyONIAnimeSetP,
    shCharacterEnemyNSEAnimeSetP,
    shCharacterEnemyIKEAnimeSetP,
    shCharacterEnemyPAPAnimeSetP,
    shCharacterEnemyEDBAnimeSetP,
    shCharacterEnemyARMAnimeSetP,
    shCharacterEnemyBOSAnimeSetP,
};

struct EnSKELETON_DATA EnSkeletonData[16] = {
    { 0, 0, 0.0f },
    { 20, 21, 0.75f },
    { 15, 16, 0.6f },
    { 0, 0, 0.0f },
    { 12, 13, 0.75f },
    { 12, 13, 0.75f },
    { 12, 13, 0.75f },
    { 0, 0, 0.0f },
    { 20, 24, 0.75f },
    { 18, 19, 0.75f },
};

signed char EnSkeletonDataC[16] = { 0, 0, 0, 0, 1, 1, 1, 3, 3 };

float EnBattleCollisionRate[16] = {
    0.0f, 0.8f, 0.8f, 2.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 1.0f, 0.8f, 1.0f, 2.0f, 2.0f, 0.0f,
};

struct EnLOCAL_WORK enLocalWork;

/** Clears the enemy work (enLocalWork) and sets the number of enemies allowed 3D sounds (2). */
void enInitEnemy(void) {
    shQzero(&enLocalWork, sizeof(enLocalWork));
    enLocalWork.Max3DSounds = 2;
}

/** Per-frame enemy update: flags the enemies nearest the player for 3D sound, sets up the
 * room's forbidden areas, runs each enemy's CtrlMain (with its own random seed), then ages the
 * communications and plays the queued sounds whose delay has run out. */
void enExecTask(void) {
    int i;
    struct EnLOCAL_DATA *dp;
    float *p_pos;
    void (*enEnemyControlFunc[15])(struct EnLOCAL_DATA *) = {
        enSCUCtrlMain, enMKNCtrlMain, enTYUCtrlMain, enREDCtrlMain, enONICtrlMain,
        enNSECtrlMain, enIKECtrlMain, enPAPCtrlMain, enEDBCtrlMain, enARMCtrlMain,
        enBOSCtrlMain, enNIKCtrlMain, enTY2CtrlMain, enTY3CtrlMain, enINSCtrlMain,
    };
    float dist[32];
    int num[32];
    int j;
    int max;
    int max2;
    int n;
    float d;
    struct EnFORBIDDENAREA *fa;
    struct EnCOMMUNICATION *cp;
    struct EnSOUND_QUEUE *que;
    int f;

    p_pos = (float *)&sh2jms.player->pos;
    dp = enLocalWork.Data;
    max = 0;
    for (i = 0; i < 32; i++, dp++) {
        if (dp->kind > EnKIND_NONE && dp->kind <= EnKIND_INS) {
            dist[max] = dp->p_dist = enDist((float *)&dp->scp->pos, p_pos);
            num[max] = i;
            dp->flag &= ~0x1000;
            max++;
        }
    }
    max2 = max;
    if (enLocalWork.Max3DSounds < max) {
        max2 = enLocalWork.Max3DSounds;
    }
    for (i = 0; i < max2; i++) {
        d = dist[i];
        n = i;
        for (j = i + 1; j < max; j++) {
            if (dist[j] < d) {
                d = dist[j];
                n = j;
            }
        }
        enLocalWork.Data[num[n]].flag |= 0x1000;
        dist[n] = dist[i];
        num[n] = i;
    }
    enRoomForbiddenArea();
    fa = enLocalWork.ForbiddenArea;
    enLocalWork.Status &= ~1;
    for (i = 0; i < enLocalWork.ForbiddenNum; i++, fa++) {
        if (p_pos[0] >= fa->x0 && p_pos[0] <= fa->x1 && p_pos[2] >= fa->z0 && p_pos[2] <= fa->z1) {
            enLocalWork.Status |= 1;
            break;
        }
    }
    dp = enLocalWork.Data;
    for (i = 0; i < 32; i++, dp++) {
        if (dp->kind > EnKIND_NONE && dp->kind <= EnKIND_INS) {
            enLocalWork.This = dp;
            shPushRandSeed(dp->randseed);
            shBattleGetResult(dp->scp);
            enEnemyControlFunc[dp->kind - 1](dp);
            enSetRadioVolume(dp);
            dp->scp->battle.id = 0;
            dp->randseed = shPopRandSeed();
        }
    }
    enResetForbiddenArea();
    if (enLocalWork.CommunicationNum) {
        cp = enLocalWork.Communication;
        for (i = 0; i < 8; i++, cp++) {
            if (cp->kind && cp->time > 0 && --cp->time <= 0) {
                cp->kind = 0;
                enLocalWork.CommunicationNum--;
            }
        }
    }
    if (enLocalWork.SoundQueueNum) {
        for (i = 0; i < enLocalWork.SoundQueueNum; i++) {
            que = &enLocalWork.SoundQueue[i];
            f = 0;
            if (que->scp->kind == 0) {
                f = 1;
            } else {
                que->time -= shGetDT();
                if (que->time <= 0.0f) {
                    enSoundCall(que->num, que->vol, (float *)&que->scp->pos);
                    f = 1;
                }
            }
            if (f) {
                if (i == --enLocalWork.SoundQueueNum) {
                    break;
                }
                *que = enLocalWork.SoundQueue[enLocalWork.SoundQueueNum];
                i--;
            }
        }
    }
    enLocalWork.This = NULL;
}

/** Takes a free slot in the enemy table.
 * @param kind enemy kind (EnKIND_*)
 * @return the slot, or NULL when all 32 are in use */
struct EnLOCAL_DATA *enEntryEnemy(int kind) {
    struct EnLOCAL_DATA *dp;
    int i;

    i = 0;
    dp = enLocalWork.Data;
    while (dp->kind) {
        if (++i >= 32) {
            printf("enemy task empty.\n");
            return NULL;
        }
        dp++;
    }
    dp->kind = kind;
    dp->mlv = 0;
    return dp;
}

/** Links an enemy slot to its character and runs the kind's InitData.
 * @param dp enemy work
 * @param scp the enemy's character */
void enInitData(struct EnLOCAL_DATA *dp, struct SubCharacter *scp) {
    int kind;
    void (*enInitDataFunc[15])(struct EnLOCAL_DATA *) = {
        enSCUInitData, enMKNInitData, enTYUInitData, enREDInitData, enONIInitData,
        enNSEInitData, enIKEInitData, enPAPInitData, enEDBInitData, enARMInitData,
        enBOSInitData, enNIKInitData, enTY2InitData, enTY3InitData, enINSInitData,
    };

    if (dp == NULL || scp == NULL) {
        return;
    }
    kind = dp->kind;
    /* Matching: the assert bakes its original line number into the object. */
#line 372
    assert(kind > EnKIND_NONE && kind <= EnKIND_INS);
    enLocalWork.This = dp;
    shQzero(dp, sizeof(struct EnLOCAL_DATA));
    dp->kind = kind;
    dp->scp = scp;
    dp->path.markangle = dp->path.angle = dp->scp->rot.y;
    vcopy(&dp->scp->pos, &dp->scp->b_pos);
    dp->scp->battle.status &= ~0x4F;
    enInitDataFunc[kind - 1](dp);
    dp->randseed = shRandI() ^ shRandI();
}

/** Frees an enemy slot and erases its fog object.
 * @param dp enemy work (may be NULL) */
void enDeleteEnemy(struct EnLOCAL_DATA *dp) {
    if (dp) {
        dp->kind = 0;
        fogEraseObj(dp - enLocalWork.Data + 10);
    }
}

/** Control function that does nothing (unused main levels).
 * @param dp enemy work */
void enDummyCtrl(struct EnLOCAL_DATA *dp) {
    void *tmp;

    tmp = dp;
}

/** Maps a character id (0x200...) to an enemy kind (EnKIND_*).
 * @param id character id
 * @return the enemy kind, or 0 for an id that isn't an enemy */
int enTransID(int id) {
    switch (id) {
    case 0x200:
        return 1;
    case 0x201:
        return 2;
    case 0x202:
        return 3;
    case 0x20C:
        return 13;
    case 0x20D:
        return 14;
    case 0x208:
        return 4;
    case 0x209:
        return 5;
    case 0x207:
    case 0x20B:
        return 6;
    case 0x203:
        return 7;
    case 0x204:
        return 8;
    case 0x205:
        return 9;
    case 0x20A:
        return 10;
    case 0x206:
        return 11;
    case 0x421:
        return 12;
    }
    return 0;
}

/** Returns the lighting condition the enemies react to: 1 by day; at night 2 (flashlight on,
 * outdoors), 3 (flashlight on, indoors) or 4 (flashlight off). */
int enGetWorldCondition(void) {
    if (sh2gfw_Get_NightOrDay()) {
        if (item.light_switch) {
            return BgIsOut(stage->glb_crd) ? 2 : 3;
        }
        return 4;
    }
    return 1;
}

/** Returns 1-5 for the stages (stage->glb_crd) 9-13, 0 elsewhere. enSetHP scales HP by it. */
int enGetPlace(void) {
    int place;

    place = 0;
    switch (stage->glb_crd) {
    case 9:
        place = 1;
        break;
    case 10:
        place = 2;
        break;
    case 11:
        place = 3;
        break;
    case 12:
        place = 4;
        break;
    case 13:
        place = 5;
        break;
    }
    return place;
}

/** Returns the enemies' stage number from stage->glb_crd and game flag 251. */
int enGetStage(void) {
    int s;

    s = 0;
    switch (stage->glb_crd) {
    case 1:
        s = 0;
        break;
    case 2:
        if (GAME_FLAG(251)) {
            s = 2;
        } else {
            s = 1;
        }
        break;
    case 3:
        if (GAME_FLAG(251)) {
            s = 4;
        } else {
            s = 3;
        }
        break;
    case 4:
        s = 5;
        break;
    }
    return s;
}

/** Returns the battle difficulty (playing.battle_level), the index of the per-mode tables. */
int enGetMode(void) {
    return playing.battle_level;
}

/** Returns whether the character stands in the dark or the light (sh2gfw_Check_CharaDarkOrBright).
 * @param scp the character */
int enCheckDarkOrBright(struct SubCharacter *scp) {
    return sh2gfw_Check_CharaDarkOrBright(scp);
}

/** enCheckDarkOrBright for the player. */
int enCheckDarkOrBrightPlayer(void) {
    return enCheckDarkOrBright(sh2jms.player);
}

/** Returns non-zero if the enemy is above the water level of its room (rooms without water: always).
 * @param dp enemy work */
int enCheckWater(struct EnLOCAL_DATA *dp) {
    float y;

    y = 3.4028235e38f;
    switch (RoomName(0, dp->scp->pos.x, dp->scp->pos.z)) {
    case 0x63:
    case 0x62:
        y = -200.0f;
        break;
    case 0x80:
    case 0x85:
    case 0x7E:
    case 0x82:
        y = -180.0f;
        break;
    case 0x7A:
    case 0x7C:
        y = -165.0f;
        break;
    case 0xBB:
        y = 160.0f;
        break;
    case 0xB6:
        y = -280.0f;
        break;
    case 0xB7:
        y = -310.0f;
        break;
    case 0xAB:
        y = -320.0f;
        break;
    case 0xB8:
        y = -360.0f;
        break;
    case 0xB9:
        y = -350.0f;
        break;
    }
    return dp->scp->pos.y >= y;
}

/** Picks the enemy's battle target (a human) with shBattleGetTargetHuman.
 * @param dp enemy work
 * @param type target selection type */
void enSetBattleTarget(struct EnLOCAL_DATA *dp, unsigned int type) {
    dp->scp->battle.target = shBattleGetTargetHuman(dp->scp, type);
}

/** Sets HP and endurance, scaled by stage for SCU, MKN and NSE.
 * @param dp enemy work
 * @param hp hit points
 * @param endurance endurance (damage taken before the enemy goes down) */
void enSetHP(struct EnLOCAL_DATA *dp, float hp, float endurance) {
    struct shBattleInfo *bi;

    bi = &dp->scp->battle;
    if (dp->kind == EnKIND_SCU || dp->kind == EnKIND_MKN || dp->kind == EnKIND_NSE) {
        switch (enGetPlace()) {
        case 2:
            hp *= 1.5f;
            break;
        case 3:
            hp *= 1.5f;
            endurance *= 1.2f;
            break;
        case 4:
        case 5:
            hp *= 2.0f;
            endurance *= 1.5f;
            break;
        }
    }
    bi->hp = bi->hp_max = hp;
    bi->hp_rate = 100.0f;
    dp->endurance_max = dp->endurance = endurance;
    bi->damage = 0.0f;
}

/** Subtracts the pending damage from HP and endurance.
 * @param dp enemy work */
float enReduceHP(struct EnLOCAL_DATA *dp) {
    struct shBattleInfo *bi;

    bi = &dp->scp->battle;
    bi->hp -= bi->damage;
    if (bi->hp < 0.0f) {
        bi->hp = 0.0f;
    }
    dp->endurance -= bi->damage;
    if (dp->endurance < 0.0f) {
        dp->endurance = 0.0f;
    }
    bi->damage = 0.0f;
    bi->hp_rate = 100.0f * (bi->hp / bi->hp_max);
    return dp->endurance;
}

/** Adds HP (up to the maximum) and refills endurance (up to HP).
 * @param dp enemy work
 * @param n HP to add */
float enAddHP(struct EnLOCAL_DATA *dp, float n) {
    struct shBattleInfo *bi;

    bi = &dp->scp->battle;
    if ((bi->hp += n) > bi->hp_max) {
        bi->hp = bi->hp_max;
    }
    dp->endurance = dp->endurance_max;
    if (dp->endurance > bi->hp) {
        dp->endurance = bi->hp;
    }
    bi->hp_rate = 100.0f * (bi->hp / bi->hp_max);
    return bi->hp;
}

/** Regenerates endurance at n per second, up to its maximum and to HP.
 * @param dp enemy work
 * @param n endurance per second */
float enAddEnduranceDT(struct EnLOCAL_DATA *dp, float n) {
    struct shBattleInfo *bi;
    float en;

    bi = &dp->scp->battle;
    en = dp->endurance + n * shGetDT();
    if (en > dp->endurance_max) {
        en = dp->endurance_max;
    }
    if (en > bi->hp) {
        en = bi->hp;
    }
    dp->endurance = en;
    return en;
}

/** Checks for damage this frame, dropping hits the enemy is immune to (no-damage flag, some
 * weapons, spray on most kinds). Repeated hits in a row are divided down.
 * @param dp enemy work
 * @return 1 if the enemy took damage */
int enCheckDamage(struct EnLOCAL_DATA *dp) {
    struct shBattleInfo *bi;

    bi = &dp->scp->battle;
    if (bi->damage == 0.0f) {
        dp->d_count = 0;
        return 0;
    }
    if ((bi->status & 0x40) || (bi->id >= 25 && bi->id <= 34 && !(bi->status & 4)) ||
        (dp->kind == EnKIND_SCU && bi->id == 36) || (dp->kind == EnKIND_TYU && bi->id == 51)) {
        bi->damage = 0.0f;
        bi->shock = 0.0f;
        return 0;
    }
    if ((bi->id == 8 || bi->id == 9) && dp->kind != EnKIND_SCU && dp->kind != EnKIND_MKN &&
        dp->kind != EnKIND_NSE && dp->kind != EnKIND_TYU && dp->kind != EnKIND_INS &&
        dp->kind != EnKIND_PAP && enGetSprayPower() >= 0 && enGetSprayPower() <= 1) {
        bi->damage = 0.0f;
        bi->shock = 0.0f;
        return 0;
    }
    if (shPadGetPort() == 6 && shPadPress(0, 0x40000)) {
        dp->endurance = 0.0f;
        bi->hp_rate = 0.0f;
        bi->hp = 0.0f;
    }
    bi->damage /= itof(++dp->d_count);
    dp->last_atk = bi->id;
    return 1;
}

/** Returns non-zero if the last attack was the spray (attacks 8 and 9).
 * @param dp enemy work */
int enCheckSpray(struct EnLOCAL_DATA *dp) {
    if (dp->last_atk == 8 || dp->last_atk == 9) {
        return 1;
    }
    return 0;
}

/** Clears the pending damage, shock and attack id.
 * @param dp enemy work */
void enResetDamage(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.id = 0;
    dp->scp->battle.damage = 0.0f;
    dp->scp->battle.shock = 0.0f;
}

/** Returns non-zero once HP is 0.
 * @param dp enemy work */
int enCheckDeath(struct EnLOCAL_DATA *dp) {
    return dp->scp->battle.hp <= 0.0f;
}

/** Starts a knock-back along the hit direction, from the pending shock.
 * @param dp enemy work */
void enSetHitBack(struct EnLOCAL_DATA *dp) {
    float vec[4];
    struct shBattleInfo *bi;

    bi = &dp->scp->battle;
    _shNormalize(vec, bi->vec);
    dp->hb_x = vec[0];
    dp->hb_z = vec[2];
    dp->hb_s = 2.0f * bi->shock;
    bi->shock = 0.0f;
}

/** Returns non-zero if the last attack kills instantly (attacks 25-34).
 * @param dp enemy work */
int enCheckInstantDeath(struct EnLOCAL_DATA *dp) {
    if (dp->last_atk >= 25 && dp->last_atk <= 34) {
        return 1;
    }
    return 0;
}

/** Sets the collision size now (and as the target of enSetNewSize).
 * @param dp enemy work
 * @param size radius
 * @param tall height
 * @param center centre height
 * @param eye eye height */
void enSetSize(struct EnLOCAL_DATA *dp, float size, float tall, float center, float eye) {
    dp->size = dp->new_size = size;
    dp->tall = dp->new_tall = tall;
    dp->center_y = dp->new_center = center;
    dp->eye_y = dp->new_eye = eye;
}

/** Sets the collision size to change to.
 * @param dp enemy work
 * @param size radius
 * @param tall height
 * @param center centre height
 * @param eye eye height */
void enSetNewSize(struct EnLOCAL_DATA *dp, float size, float tall, float center, float eye) {
    dp->new_size = size;
    dp->new_tall = tall;
    dp->new_center = center;
    dp->new_eye = eye;
}

/** Sets the cone in which the enemy sees the player's light.
 * @param dp enemy work
 * @param center look centre
 * @param radius look radius */
void enSetSeeLightStatus(struct EnLOCAL_DATA *dp, float center, float radius) {
    struct shBattleInfo *bi;

    bi = &dp->scp->battle;
    bi->look.center = center;
    bi->look.radius = radius;
    bi->feel.center = bi->feel.radius = 0.0f;
}

/** Returns non-zero if the enemy sees the player's light.
 * @param dp enemy work */
int enCheckSeeLight(struct EnLOCAL_DATA *dp) {
    return shBattleSeeHumanLight(dp->scp) == 1;
}

/** Returns non-zero if the player is aiming at the enemy.
 * @param dp enemy work */
int enCheckAimedByHuman(struct EnLOCAL_DATA *dp) {
    return shBattleAimedByHuman(dp->scp);
}

/** Returns non-zero if the player has finished the enemy off.
 * @param dp enemy work */
int enCheckFinishedByHuman(struct EnLOCAL_DATA *dp) {
    return shBattleFinishedByHuman(dp->scp);
}

/** Returns non-zero if the enemy's target (James or Maria) can't be damaged right now.
 * @param dp enemy work */
int enCheckNoDamageHuman(struct EnLOCAL_DATA *dp) {
    if (dp->scp->battle.target->kind != 0x105) {
        return shBattleNoDamageHumanJames();
    } else {
        return shBattleNoDamageHumanMaria();
    }
}

/** Starts an attack: resets the hit check and flags the attack as live.
 * @param dp enemy work */
void enAttackStart(struct EnLOCAL_DATA *dp) {
    shBattleAttackHitCheckInit(dp->scp);
    dp->flag |= 4;
}

/** Runs the hit check of a live attack; the attack ends once it hits.
 * @param dp enemy work
 * @param ID attack id
 * @return the hit result (0: no hit) */
int enAttackCheck(struct EnLOCAL_DATA *dp, int ID) {
    int result;

    if (!(dp->flag & 4)) {
        return 0;
    }
    result = dp->scp->battle.atk_result;
    if (result) {
        dp->flag &= ~4;
        return result;
    }
    shBattleAttackHitCheckToHuman(dp->scp, ID);
    return result;
}

/** Runs the hit check of a grab attack.
 * @param dp enemy work
 * @param ID attack id
 * @return the hit result (0: no hit) */
int enAttackCheckHug(struct EnLOCAL_DATA *dp, int ID) {
    shBattleAttackHitCheckToHuman(dp->scp, ID);
    return dp->scp->battle.atk_result;
}

/** Returns non-zero if the player is being held by an enemy. */
int enCheckHuggedPlayer(void) {
    return shBattleHuggedHuman();
}

/** Returns non-zero if the enemy should go to sleep (inactive, or far from the player outdoors).
 * @param dp enemy work */
int enCheckSleepIn(struct EnLOCAL_DATA *dp) {
    if ((!(dp->scp->battle.status & 0x400) || (BgIsOut(0) && dp->p_dist > 15000.0f)) &&
        !(dp->scp->battle.status & 2)) {
        return 1;
    }
    return 0;
}

/** Returns non-zero if a sleeping enemy should wake up.
 * @param dp enemy work */
int enCheckSleepOut(struct EnLOCAL_DATA *dp) {
    if (dp->scp->battle.status & 0x400) {
        if (!BgIsOut(0) || dp->p_dist < 7500.0f) {
            return 1;
        }
    }
    return 0;
}

/** Puts the enemy to sleep (main level 2).
 * @param dp enemy work */
void enSleepIn(struct EnLOCAL_DATA *dp) {
    dp->mlv = 2;
    dp->scp->battle.status &= ~1;
}

/** Wakes the enemy (main level 1, automatic).
 * @param dp enemy work */
void enSleepOut(struct EnLOCAL_DATA *dp) {
    dp->mlv = 1;
    dp->scp->battle.status &= ~0x4C;
    dp->scp->battle.status |= 1;
}

/** Counts the kill in the game statistics, by the last attack.
 * @param dp enemy work */
void enKillCountUp(struct EnLOCAL_DATA *dp) {
    GameKillEnemyCountUp(dp->last_atk);
}

/** Returns the position of the enemy's target (the player's movement column for James).
 * @param dp enemy work */
float *enGetPlayerPos(struct EnLOCAL_DATA *dp) {
    if (dp->scp->battle.target == sh2jms.player) {
        return sh2jms.column_mov.p[0];
    } else {
        return (float *)&dp->scp->battle.target->pos;
    }
}

/** Returns the XZ distance to the enemy's target.
 * @param dp enemy work */
float enGetPlayerDistance(struct EnLOCAL_DATA *dp) {
    return enDistXZ(enGetPlayerPos(dp), (float *)&dp->scp->pos);
}

/** Returns the angle from the enemy to its target.
 * @param dp enemy work */
float enGetPlayerDirection(struct EnLOCAL_DATA *dp) {
    float vec[4];

    _shSubVector(vec, enGetPlayerPos(dp), (float *)&dp->scp->pos);
    return shAtanV(vec);
}

/** Returns the player's current weapon. */
int enGetPlayerWeapon(void) {
    return sh2jms.weapon;
}

/** Returns non-zero if the player holds weapon 1, 2 or 3. */
int enCheckPlayerWeapon(void) {
    switch (sh2jms.weapon) {
    case 1:
    case 2:
    case 3:
        return 1;
    }
    return 0;
}

/** Returns the y rotation of the enemy's target.
 * @param dp enemy work */
float enGetPlayerAngle(struct EnLOCAL_DATA *dp) {
    return dp->scp->battle.target->rot.y;
}

/** Returns the radius of the player's movement column. */
float enGetPlayerSize(void) {
    return sh2jms.column_mov.p[1][3];
}

/** Returns non-zero if the enemy hears its target.
 * @param dp enemy work */
int enCheckPlayerSound(struct EnLOCAL_DATA *dp) {
    return shBattleListenHumanSound(dp->scp, dp->scp->battle.target);
}

/** Returns flags for the player's state as the enemy sees it: bit 0 locked on, bit 1 holding
 * weapon 1-3, bit 2 not facing the enemy (more than 90 degrees off).
 * @param dp enemy work */
int enCheckPlayerCondition(struct EnLOCAL_DATA *dp) {
    int r;
    int w;
    float a;

    r = 0;
    if (sh2jms.lock_on) {
        r |= 1;
    }
    w = enGetPlayerWeapon();
    if (w == 1 || w == 2 || w == 3) {
        r |= 2;
    }
    a = enCalcAngleDifference(PI + enGetPlayerAngle(dp), dp->scp->rot.y);
    if (a >= PI / 2) {
        r |= 4;
    }
    return r;
}

/** Returns non-zero if the flashlight is on. */
int enCheckPlayerLight(void) {
    return item.light_switch;
}

/** Returns non-zero while the player is spraying (weapon 4, attacks 8 and 9). */
int enCheckPlayerSprayNow(void) {
    if (sh2jms.weapon == 4 && (sh2jms.upper_st_flg & 0x10000000) &&
        (sh2jms.attack_no == 8 || sh2jms.attack_no == 9)) {
        return 1;
    }
    return 0;
}

/** Returns the spray power (playing.spray_pow, with 1 counted as 0). */
int enGetSprayPower(void) {
    int n;

    n = playing.spray_pow;
    if (n == 1) {
        n = 0;
    }
    return n;
}

/** Returns non-zero if the player has no ammunition left (items 4-9). */
int enCheckPlayerBulletEmpty(void) {
    if (item.number[4] || item.number[5] || item.number[6] || item.number[7] || item.number[8] ||
        item.number[9]) {
        return 0;
    }
    return 1;
}

/** Returns the player's dead state. */
int enCheckDeadPlayer(void) {
    return sh2jms.dead;
}

/** Marks the player as killed (dead = 2). */
void enSetGameOver(void) {
    sh2jms.dead = 2;
}

/** Sets battle status bit 0 (the enemy moves).
 * @param dp enemy work */
void enFlagSetMoved(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status |= 1;
}

/** Clears battle status bit 0 (the enemy moves).
 * @param dp enemy work */
void enFlagResetMoved(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status &= ~1;
}

/** Sets battle status bit 2 (lying down).
 * @param dp enemy work */
void enFlagSetLieDown(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status |= 4;
}

/** Clears battle status bit 2 (lying down).
 * @param dp enemy work */
void enFlagResetLieDown(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status &= ~4;
}

/** Sets battle status bit 3 (critical).
 * @param dp enemy work */
void enFlagSetCritical(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status |= 8;
}

/** Clears battle status bit 3 (critical).
 * @param dp enemy work */
void enFlagResetCritical(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status &= ~8;
}

/** Returns non-zero if battle status bit 3 (critical) is set.
 * @param dp enemy work */
int enCheckCritical(struct EnLOCAL_DATA *dp) {
    return (dp->scp->battle.status & 8) != 0;
}

/** Sets battle status bit 6 (takes no damage).
 * @param dp enemy work */
void enFlagSetNoDamage(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status |= 0x40;
}

/** Clears battle status bit 6 (takes no damage).
 * @param dp enemy work */
void enFlagResetNoDamage(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status &= ~0x40;
}

/** Sets battle status bit 1 (dead).
 * @param dp enemy work */
void enFlagSetDead(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status |= 2;
}

/** Clears battle status bit 1 (dead).
 * @param dp enemy work */
void enFlagResetDead(struct EnLOCAL_DATA *dp) {
    dp->scp->battle.status &= ~2;
}

/** Makes the enemy tilt to follow the floor.
 * @param dp enemy work */
void enFlagSetRotFloor(struct EnLOCAL_DATA *dp) {
    dp->flag |= 0x800;
    dp->scp->status |= 0x80;
}

/** Stops the floor tilt; the enemy turns back upright over time.
 * @param dp enemy work */
void enFlagResetRotFloor(struct EnLOCAL_DATA *dp) {
    dp->flag &= ~0x800;
    dp->trx = dp->trz = 0.0f;
}

/** Stops the floor tilt and sets the enemy upright at once.
 * @param dp enemy work */
void enFlagResetRotFloorJust(struct EnLOCAL_DATA *dp) {
    dp->flag &= ~0x800;
    dp->trx = dp->trz = 0.0f;
    dp->scp->rot.z = 0.0f;
    dp->scp->rot.x = 0.0f;
    dp->scp->status &= ~0x80;
}

/** Shows the enemy's character (status bits 0x410).
 * @param dp enemy work */
void enFlagSetDisplay(struct EnLOCAL_DATA *dp) {
    dp->scp->status |= 0x410;
}

/** Hides the enemy's character (status bits 0x410).
 * @param dp enemy work */
void enFlagResetDisplay(struct EnLOCAL_DATA *dp) {
    dp->scp->status &= ~0x410;
}

/** Returns the angle of the vector from pb to pa.
 * @param pa target point
 * @param pb start point */
float enCalcDirection(float *pa, float *pb) {
    float vec[4];

    _shSubVector(vec, pa, pb);
    return shAtanV(vec);
}

/** Returns the absolute difference of two angles, in [0, PI].
 * @param angle1 first angle
 * @param angle2 second angle */
float enCalcAngleDifference(float angle1, float angle2) {
    return fabsf(shAngleRegulate(angle1 - angle2));
}

/** Returns a speed factor from 1 (facing the target) down to 0.5 (facing away).
 * @param angle current heading
 * @param mpos own position
 * @param tpos target position */
float enCalcSpeedRate(float angle, float *mpos, float *tpos) {
    float d;

    d = fabsf(shAngleRegulate(enCalcDirection(tpos, mpos) - angle));
    return 0.5f + 0.5f * ((PI - d) / PI);
}

/* like _sceVu0ApplyMatrix, with w taken as 1 */
static inline void _shApplyMatrixW1(float *v0, float (*m0)[4], float *v1) {
    __asm__ __volatile__("
    lqc2         vf4, 0x0(%2)
    lqc2         vf5, 0x0(%1)
    lqc2         vf6, 0x10(%1)
    vmulax.xyzw  ACC, vf5, vf4x
    lqc2         vf5, 0x20(%1)
    vmadday.xyzw ACC, vf6, vf4y
    lqc2         vf6, 0x30(%1)
    vmaddaz.xyzw ACC, vf5, vf4z
    vmaddw.xyzw  vf4, vf6, vf0w
    sqc2         vf4, 0x0(%0)
    " : : "r"(v0), "r"(m0), "r"(v1));
}

/** Makes a vector of length range along +z, rotated by rot (z, y, x).
 * @param vec result
 * @param rot rotation
 * @param range length */
void enMakeRotVector(float *vec, float *rot, float range) {
    float rmat[4][4];

    _sceVu0UnitMatrix(rmat);
    shRotMatrixZ(rmat, rmat, rot[2]);
    shRotMatrixY(rmat, rmat, rot[1]);
    shRotMatrixX(rmat, rmat, rot[0]);
    vzero(vec);
    vec[2] = range;
    _sceVu0ApplyMatrix(vec, rmat, vec);
}

/** Returns non-zero if the enemy's eye point is on screen.
 * @param dp enemy work */
int enCheckIntoScreen(struct EnLOCAL_DATA *dp) {
    float vec[4];

    _sceVu0CopyVectorXYZ(vec, (float *)&dp->scp->pos);
    vec[1] -= dp->eye_y;
    _shRotTransPersQ(vec, VbWvsMatrix.wsm, vec);
    if (shScreenClipF(vec)) {
        return 0;
    }
    return 1;
}

/** Returns the nearest of the enemy's human target and the other living enemies.
 * @param dp enemy work */
struct SubCharacter *enGetNearCharacter(struct EnLOCAL_DATA *dp) {
    float dist;
    float d;
    int i;
    struct EnLOCAL_DATA *tp;
    struct SubCharacter *scp;

    tp = enLocalWork.Data;
    scp = shBattleGetTargetHuman(dp->scp, 0);
    dist = enDistXZ((float *)&scp->pos, (float *)&dp->scp->pos);
    for (i = 0; i < 32; i++, tp++) {
        if (tp->kind && tp != dp && tp->scp->battle.hp > 0.0f) {
            d = enDistXZ((float *)&tp->scp->pos, (float *)&dp->scp->pos);
            if (d < dist) {
                dist = d;
                scp = tp->scp;
            }
        }
    }
    return scp;
}

/** Converts a time in frames at 60 fps to timer units (currently the same).
 * @param t frames */
int enCalcTimer(int t) {
    return t * 60 / 60;
}

/** Starts the enemy's timer.
 * @param dp enemy work
 * @param t frames */
void enSetTimer(struct EnLOCAL_DATA *dp, int t) {
    if ((dp->timer = enCalcTimer(t) - 1) < 0) {
        dp->timer = 0;
    }
    dp->flag &= ~0x2000;
}

/** Counts the enemy's timer down by this frame's length; flag 0x2000 marks each whole second passed.
 * @param dp enemy work
 * @return the time left */
int enReduceTimer(struct EnLOCAL_DATA *dp) {
    int t;
    int n;

    t = dp->timer;
    n = t / 60;
    t -= shGetDF();
    if (t < 0) {
        t = 0;
    }
    if (t / 60 < n) {
        dp->flag |= 0x2000;
    } else {
        dp->flag &= ~0x2000;
    }
    dp->timer = t;
    return t;
}

/** Returns the nearest other living enemy, or NULL.
 * @param dp enemy work */
struct EnLOCAL_DATA *enGetNearOtherEnemy(struct EnLOCAL_DATA *dp) {
    int i;
    struct EnLOCAL_DATA *tp;
    struct EnLOCAL_DATA *rp;
    float d;
    float m;

    tp = enLocalWork.Data;
    rp = NULL;
    m = 3.4028235e38f;
    for (i = 0; i < 32; i++, tp++) {
        if (tp->kind && tp != dp && !(tp->scp->battle.status & 2)) {
            d = enDist((float *)&dp->scp->pos, (float *)&tp->scp->pos);
            if (d < m) {
                rp = tp;
                m = d;
            }
        }
    }
    return rp;
}

/** While the player aims at the enemy from within limit, keeps it in front of him at the closest
 * distance reached, for up to 60 frames (then a 10-frame pause).
 * @param dp enemy work
 * @param count frame counter (reset when the player is out of range or not aiming)
 * @param dist the distance kept
 * @param limit the range */
void enCheckNearPlayer(struct EnLOCAL_DATA *dp, int *count, float *dist, float limit) {
    float d;
    float vec[4];

    if (!enCheckAimedByHuman(dp) || (d = enDistXZ((float *)&dp->scp->pos, enGetPlayerPos(dp))) > limit) {
        *count = 0;
        return;
    }
    if (*count <= 0) {
        *count += shGetDF();
        if (*count > 0) {
            *dist = d;
        }
        return;
    }
    *count += shGetDF();
    if (*count > 60) {
        *count = -10;
        return;
    }
    if (d < *dist) {
        *dist = d;
    } else {
        d = *dist;
    }
    shSinCosV_Scale(vec, enGetPlayerAngle(dp), d);
    _shAddVector(vec, enGetPlayerPos(dp), vec);
    dp->scp->pos.x = vec[0];
    dp->scp->pos.z = vec[2];
    vzero(dp->vec);
}

/** Sets how much the enemy makes the radio noise (dp->radio), by kind and state: 0 for the kinds
 * the radio ignores and for hidden or inactive enemies.
 * @param dp enemy work */
void enSetRadioVolume(struct EnLOCAL_DATA *dp) {
    if (dp->kind == EnKIND_RED && dp->type == 0) {
        dp->radio = 1.0f;
    } else if (dp->kind == EnKIND_IKE || dp->kind == EnKIND_EDB || dp->kind == EnKIND_RED ||
               dp->kind == EnKIND_ONI || dp->kind == EnKIND_BOS || !(dp->scp->status & 0x10)) {
        dp->radio = 0.0f;
    } else if (dp->scp->battle.status & 1) {
        if (dp->kind == EnKIND_TYU) {
            dp->radio = 0.5f;
        } else if (dp->scp->battle.status & 4) {
            dp->radio = 0.8f;
        } else {
            dp->radio = 1.0f;
        }
    } else {
        dp->radio = 0.0f;
    }
}

/* turn *angle toward target by at most speed */
static inline void enTurnAngle(float *angle, float target, float speed) {
    float a;

    a = shAngleRegulate(target - *angle);
    if (a < -speed) {
        target = *angle - speed;
    } else if (a > speed) {
        target = *angle + speed;
    }
    *angle = shAngleRegulate(target);
}

/** Turns a path's heading toward its mark angle.
 * @param p the path
 * @param delta turn speed (per frame at 60 fps) */
void enMoveAngle(struct EnPATH_DATA *p, float delta) {
    enTurnAngle(&p->angle, p->markangle, 60.0f * delta * shGetDT());
}

/** Turns the enemy toward its target.
 * @param dp enemy work
 * @param delta turn speed (per frame at 60 fps) */
void enMoveAngleToPlayer(struct EnLOCAL_DATA *dp, float delta) {
    dp->path.markangle = enGetPlayerDirection(dp);
    enMoveAngle(&dp->path, delta);
    dp->scp->rot.y = dp->path.angle;
}

/* move *v toward target by at most speed */
static inline void enApproach(float *v, float target, float speed) {
    float d;

    d = target - *v;
    if (d < -speed) {
        *v -= speed;
    } else if (d > speed) {
        *v += speed;
    } else {
        *v = target;
    }
}

/** Moves the enemy for this frame: floor tilt, animation translation, knock-back and the
 * size change toward the new size, then updates its collision column.
 * @param dp enemy work */
void enMoveExec(struct EnLOCAL_DATA *dp) {
    float vec[4];
    float vec2[4];
    float s;
    float t;
    float c;
    float e;
    float dt;
    float hb;

    dt = 30.0f * shGetDT();
    if (dp->flag & 0x800) {
        enSetRotFloor(dp);
    }
    enTurnAngle(&dp->scp->rot.x, dp->trx, 0.08726646f * dt);
    enTurnAngle(&((float *)&dp->scp->rot)[2], dp->trz, 0.08726646f * dt);
    s = dp->size;
    if (dp->new_size != s) {
        enApproach(&s, dp->new_size, 25.0f);
        dp->size = s;
    }
    t = dp->tall;
    if (dp->new_tall != t) {
        enApproach(&t, dp->new_tall, 20.0f * dt);
        dp->tall = t;
    }
    c = dp->center_y;
    if (dp->new_center != c) {
        enApproach(&c, dp->new_center, 20.0f * dt);
        dp->center_y = c;
    }
    e = dp->eye_y;
    if (dp->new_eye != e) {
        enApproach(&e, dp->new_eye, 20.0f * dt);
        dp->eye_y = e;
    }
    if ((dp->scp->battle.status & 2) && dp->kind != EnKIND_IKE && dp->kind != EnKIND_NIK &&
        dp->kind != EnKIND_ONI && (dp->kind != EnKIND_PAP || dp->type != 1)) {
        return;
    }
    if (dp->kind != EnKIND_IKE && dp->kind != EnKIND_ARM && dp->kind != EnKIND_INS) {
        vcopy(&dp->scp->pos, &dp->scp->b_pos);
    }
    _shScaleVector(vec, dp->vec, shGetDT());
    if (dp->tx != dp->tx2 || dp->tz != dp->tz2) {
        vzero(vec2);
        vec2[0] = dp->tx2 - dp->tx;
        vec2[2] = dp->tz2 - dp->tz;
        dp->tx2 = dp->tx;
        dp->tz2 = dp->tz;
        shRotVectorY(vec2, vec2, dp->scp->rot.y);
        _shAddVectorXYZ(vec, vec, vec2);
    }
    if (dp->hb_s) {
        hb = dp->hb_s *= 0.5f;
        if (hb > dp->size - 50.0f) {
            hb = dp->size - 50.0f;
        }
        vzero(vec2);
        vec2[0] = dp->hb_x;
        vec2[2] = dp->hb_z;
        _shScaleVector(vec2, vec2, hb);
        _shAddVectorXYZ(vec, vec, vec2);
        if (dp->hb_s < 1.0f) {
            dp->hb_s = 0.0f;
        }
    }
    _shAddVectorXYZ(vec, (float *)&dp->scp->pos, vec);
    enCheckForbiddenArea((float *)&dp->scp->pos, vec, s);
    _sceVu0CopyVectorXYZ((float *)&dp->scp->pos, vec);
    vzero(dp->vec);
    enSetHitColumn(dp);
}

/** Sets the tilt targets (trx, trz) from the floor normal under the enemy.
 * @param dp enemy work */
void enSetRotFloor(struct EnLOCAL_DATA *dp) {
    float vec[4];

    shRotVectorY(vec, dp->scp->grnd_normal, -dp->scp->rot.y);
    dp->trx = -shAtan2(-vec[1], vec[2]);
    dp->trz = shAtan2(-vec[1], vec[0]);
}

/** Registers the enemy's collision columns and its fog object.
 * @param dp enemy work */
void enSetHitColumn(struct EnLOCAL_DATA *dp) {
    struct _CL_HITPOLY_COLUMN column;
    struct _CL_HITPOLY_COLUMN column2;
    float vec[4];
    float s;
    float t;
    float c;
    float e;
    float *pos;
    int ID;
    struct FOG_OBJ_DATA *od;

    if ((dp->flag & 8) || (dp->kind != EnKIND_NIK && !(dp->scp->status & 0x10))) {
        fogEraseObj(dp - enLocalWork.Data + 10);
        return;
    }
    pos = (float *)&dp->scp->pos;
    s = dp->size;
    t = dp->tall;
    c = dp->center_y;
    e = dp->eye_y;
    column.kind = 1;
    column.shape = 3;
    column.weight = dp->weight;
    column.material = 12;
    _sceVu0CopyVectorXYZ(column.p[0], pos);
    vzero(column.p[1]);
    if (dp->flag & 0x20) {
        column.p[0][1] += s;
        column.p[1][1] = pos[1] - s;
        column.p[1][3] = t;
        dp->scp->center_y = pos[1];
        dp->scp->eye_y = pos[1];
    } else if (dp->flag & 0x40) {
        if (!(dp->flag & 0x400)) {
            if (dp->kind == EnKIND_NIK) {
                _shScaleVector(vec, dp->nik.swing, 0.8f);
            } else {
                enGetSkeletonVector(vec, dp, EnSkeletonDataC[dp->kind]);
                shRotVectorY(vec, vec, dp->scp->rot.y);
            }
            column.p[0][0] += vec[0];
            column.p[0][2] += vec[2];
        }
        if (dp->kind == EnKIND_ARM) {
            column.p[1][1] = 500.0f + pos[1];
            column.p[0][1] += 500.0f + t;
        } else {
            column.p[1][1] = pos[1];
            column.p[0][1] += t;
        }
        column.p[1][3] = s;
        dp->scp->center_y = pos[1] + c;
        dp->scp->eye_y = pos[1] + e;
    } else {
        if (!(dp->flag & 0x400)) {
            enGetSkeletonVector(vec, dp, EnSkeletonDataC[dp->kind]);
            shRotVectorY(vec, vec, dp->scp->rot.y);
            column.p[0][0] += vec[0];
            column.p[0][2] += vec[2];
        }
        if (dp->kind == EnKIND_BOS && dp->slv <= 4) {
            column.p[0][1] = 400.0f + pos[1];
            column.p[1][1] = 400.0f + (pos[1] - t);
        } else {
            column.p[1][1] = pos[1] - t;
        }
        column.p[1][3] = s;
        dp->scp->center_y = pos[1] - c;
        dp->scp->eye_y = pos[1] - e;
    }
    column2 = column;
    column2.p[1][3] *= EnBattleCollisionRate[dp->kind];
    if (dp->scp->battle.status & 4) {
        column2.p[1][3] *= 1.5f;
    }
    if (dp->flag & 0x10) {
        column.p[1][3] = 0.0f;
    }
    if (dp->flag & 2) {
        clSetCharaHitColumn(&column, &column2, dp->scp, enFlyingFunc);
        return;
    }
    clSetCharaHitColumn(&column, &column2, dp->scp, NULL);
    ID = dp - enLocalWork.Data + 10;
    od = fogGetObj(ID);
    column.p[0][1] = column.p[1][1];
    if (od) {
        fogMoveObj(ID, column.p[0]);
        fogSetObjSize(ID, 0.8f * column.p[1][3]);
    } else {
        fogSetObj2(ID, column.p[0], 0.8f * column.p[1][3]);
    }
}

/* rotate by z, then x, then y */
static inline void shRotMatrixZXY(float (*m0)[4], float (*m1)[4], float *r) {
    shRotMatrixZ(m0, m1, r[2]);
    shRotMatrixX(m0, m0, r[0]);
    shRotMatrixY(m0, m0, r[1]);
}

/** Character callback of IKE: moves the held hand position with the skeleton.
 * @param dp enemy work (may be NULL) */
void enIKETrans(struct EnLOCAL_DATA *dp) {
    float vec[4];
    float rot[4][4];

    if (dp == NULL) {
        return;
    }
    vcopy(&dp->scp->pos, &dp->scp->b_pos);
    if (dp->slv == 0 || dp->slv == 1) {
        enGetSkeletonVector(dp->ike.handpos, dp, 12);
    } else {
        vcopy(dp->ike.handpos, vec);
        enGetSkeletonVector(dp->ike.handpos, dp, 12);
        _shSubVector(vec, dp->ike.handpos, vec);
        _sceVu0UnitMatrix(rot);
        shRotMatrixZXY(rot, rot, (float *)&dp->scp->rot);
        _shApplyMatrixW1(vec, rot, vec);
        _shSubVectorXYZ((float *)&dp->scp->pos, (float *)&dp->scp->pos, vec);
    }
}

/** Character callback of ARM: moves the hand position with the body.
 * @param dp enemy work (may be NULL) */
void enARMTrans(struct EnLOCAL_DATA *dp) {
    float vec[4];

    if (dp == NULL || (dp->scp->battle.status & 2)) {
        return;
    }
    vcopy(&dp->scp->pos, &dp->scp->b_pos);
    _shSubVector(vec, (float *)&dp->scp->pos, dp->arm.old_pos);
    _shAddVectorXYZ(dp->arm.hand_pos, dp->arm.hand_pos, vec);
    if (dp->arm.arm >= 0) {
        enARMGetHandPos(vec, dp, dp->arm.arm);
        _shSubVectorXYZ(vec, dp->arm.hand_pos, vec);
        dp->scp->pos.x = vec[0];
        dp->scp->pos.z = vec[2];
    }
}

/** First step of dying: blood pool, dead flags; SCU, MKN, NSE and TYU may be picked to
 * regenerate later (main level 6), with a chance by difficulty mode.
 * @param dp enemy work */
void enDyingExec(struct EnLOCAL_DATA *dp) {
    if (dp->sslv == 0) {
        if (dp->scp->en_first_status != 13 && dp->scp->en_first_status != 14) {
            enEfctBloodPool(dp);
        }
        enFlagSetDead(dp);
        enFlagResetLieDown(dp);
        enFlagResetMoved(dp);
        fogEraseObj(dp - enLocalWork.Data + 10);
        if (dp->kind == EnKIND_SCU || dp->kind == EnKIND_MKN || dp->kind == EnKIND_NSE || dp->kind == EnKIND_TYU) {
            float regenerate_rate[5] = { 0.0f, 0.05f, 0.2f, 0.6f, 0.8f };

            if (shRandF() < regenerate_rate[enGetMode()]) {
                dp->mlv = 6;
                return;
            }
        }
        dp->sslv++;
    }
}

/** Regeneration wait: once the enemy was out of play and comes back into play, it is set
 * up again (enInitData).
 * @param dp enemy work */
void enWaitRegenerate(struct EnLOCAL_DATA *dp) {
    switch (dp->sslv) {
    case 0:
        if (!(dp->scp->battle.status & 0x400) || (BgIsOut(0) && dp->p_dist > 30000.0f)) {
            dp->sslv++;
        }
        break;
    case 1:
        if ((dp->scp->battle.status & 0x400) && (!BgIsOut(0) || dp->p_dist < 15000.0f)) {
            enInitData(dp, dp->scp);
        }
        break;
    }
}

/** Height callback for flying enemies' collision columns (does nothing).
 * @param scp the character */
void enFlyingFunc(struct SubCharacter *scp) {
    void *tmp;

    tmp = scp;
}

/** Deletes the enemy's character.
 * @param dp enemy work */
void enDeleteCharacter(struct EnLOCAL_DATA *dp) {
    shCharacter_Manage_Delete(dp->scp, dp->scp->kind, dp->scp->id);
}

/** Resets a path to a heading.
 * @param p the path
 * @param angle heading */
void enInitPath(struct EnPATH_DATA *p, float angle) {
    p->markangle = p->angle = shAngleRegulate(angle);
    p->step = 0;
    p->timer = 0;
}

/** Steers the enemy's path toward target, trying detours when the way is blocked.
 * @param dp enemy work
 * @param target target position
 * @param pos own position */
int enSetPath(struct EnLOCAL_DATA *dp, float *target, float *pos) {
    struct EnPATH_DATA *p = &dp->path;
    float vec[4];
    float dist;
    float ma;
    float a;
    float a1;
    float a2;
    float d1;
    float d2;
    int k;
    signed char timer[12] = { 0, 30, 20, 0, 20, 20, 15, 12, 30, 15, 12, 12 };

    dist = enCheckPath(dp, target, pos);
    _shSubVector(vec, target, pos);
    ma = shAtanV(vec);
    if (dist < 0.0f) {
        p->markangle = ma;
        p->dist = -dist;
        p->step = 0;
        p->timer = 0;
        p->deadend = 0;
        return 0;
    }
    if (p->timer > 0) {
        p->timer -= (short)shGetDF();
        return 1;
    }
    p->dist = dist;
    dist += 50.0f;
    k = p->step;
    switch (k) {
    case 0:
        p->step = 1;
        a1 = ma + 0.2617994f;
        shSinCosV_Scale(vec, a1, dist);
        _shAddVector(vec, pos, vec);
        d1 = enCheckPath(dp, vec, pos);
        a2 = ma - 0.2617994f;
        shSinCosV_Scale(vec, a2, dist);
        _shAddVector(vec, pos, vec);
        d2 = enCheckPath(dp, vec, pos);
        if (d1 < 0.0f && d2 >= 0.0f) {
            ma = a1;
        } else if (d1 >= 0.0f && d2 < 0.0f) {
            ma = a2;
        } else if (d1 >= 0.0f && d2 >= 0.0f && ++p->deadend >= 3) {
            ma = PI + ma;
            p->step = -1;
        } else {
            ma -= 0.0017453292f;
            p->step = 2;
        }
        break;
    case 1:
        a = shAngleRegulate(ma - p->angle);
        if (a == 0.0f) {
            a = 0.017453292f;
        }
        if (fabsf(a) < 0.2617994f) {
            a1 = ma;
        } else {
            a1 = p->angle + 0.2617994f * _shSign(a);
        }
        shSinCosV_Scale(vec, a1, dist);
        _shAddVector(vec, pos, vec);
        d1 = enCheckPath(dp, vec, pos);
        if (d1 < 0.0f) {
            ma = a1;
            p->timer = timer[dp->kind] * 60 / 60.0f * (1.0f + shSway1f(-0.5f, 0.5f));
            break;
        }
        shSinCosV_Scale(vec, ma, dist);
        _shAddVector(vec, pos, vec);
        d2 = enCheckPath(dp, vec, pos);
        if (d2 < 0.0f) {
            p->timer = timer[dp->kind] * 60 / 60.0f;
            break;
        }
        if (d1 < d2) {
            a1 = ma;
            d1 = d2;
        }
        a = _shSignIP(a);
        a2 = p->angle - 0.2617994f * a;
        shSinCosV_Scale(vec, a2, dist);
        _shAddVector(vec, pos, vec);
        d2 = enCheckPath(dp, vec, pos);
        if (d2 < 0.0f) {
            ma = a2;
            p->timer = timer[dp->kind] * 60 / 60.0f;
            break;
        }
        if (d1 < 0.0f) {
            ma = a1;
            p->timer = timer[dp->kind] * 60 / 60.0f;
            break;
        }
        ma = a1;
        p->step = 2;
        break;
    case -1:
        ma = p->markangle;
        if (enCalcAngleDifference(ma, p->angle) < 0.2617994f) {
            p->deadend = 0;
            p->step = 1;
        }
        break;
    default:
        a1 = ma + 0.2617994f * k;
        shSinCosV_Scale(vec, a1, dist);
        _shAddVector(vec, pos, vec);
        d1 = enCheckPath(dp, vec, pos);
        a2 = ma - 0.2617994f * k;
        shSinCosV_Scale(vec, a2, dist);
        _shAddVector(vec, pos, vec);
        d2 = enCheckPath(dp, vec, pos);
        if (d1 < 0.0f) {
            if (d2 < 0.0f) {
                if (++k >= 6) {
                    ma = a1;
                    k = 1;
                }
            } else {
                ma = a1;
                k = 1;
            }
        } else if (d2 < 0.0f) {
            ma = a2;
            k = 1;
        } else {
            if (d1 > d2) {
                ma = a1;
            } else {
                ma = a2;
            }
            if (++k >= 6) {
                ma = PI + ma;
                k = -1;
            }
        }
        p->step = k;
        break;
    }
    p->markangle = shAngleRegulate(ma);
    return 1;
}

/** Checks the line of movement from mpos to tpos, 50 (or, with flag 0x100, the eye height) above
 * or below the enemy's position by its orientation flag (0x40).
 * @param dp enemy work
 * @param tpos end point
 * @param mpos start point
 * @return as enCheckHitEyes */
float enCheckPath(struct EnLOCAL_DATA *dp, float *tpos, float *mpos) {
    float sp[4];
    float ep[4];

    vcopy(mpos, sp);
    vcopy(tpos, ep);
    if (dp->flag & 0x40) {
        if (dp->flag & 0x100) {
            sp[1] = ep[1] = dp->scp->pos.y + dp->eye_y;
        } else {
            sp[1] = ep[1] = 50.0f + dp->scp->pos.y;
        }
    } else {
        if (dp->flag & 0x100) {
            sp[1] = ep[1] = dp->scp->pos.y - dp->eye_y;
        } else {
            sp[1] = ep[1] = dp->scp->pos.y - 50.0f;
        }
    }
    return enCheckHitEyes(dp, sp, ep);
}

/** enCheckPath at eye height (above or below by the enemy's orientation flag).
 * @param dp enemy work
 * @param tpos end point
 * @param mpos start point */
float enCheckPath2(struct EnLOCAL_DATA *dp, float *tpos, float *mpos) {
    float sp[4];
    float ep[4];

    vcopy(mpos, sp);
    vcopy(tpos, ep);
    if (dp->flag & 0x40) {
        sp[1] = ep[1] = dp->scp->pos.y + dp->eye_y;
    } else {
        sp[1] = ep[1] = dp->scp->pos.y - dp->eye_y;
    }
    return enCheckHitEyes2(dp, sp, ep);
}

/** Checks the line of sight from pos along rot for range.
 * @param dp enemy work
 * @param pos start point
 * @param rot direction
 * @param range length */
float enCheckForward(struct EnLOCAL_DATA *dp, float *pos, float *rot, float range) {
    float tp[4];

    enMakeRotVector(tp, rot, range);
    _shAddVector(tp, pos, tp);
    return enCheckHitEyes(dp, pos, tp);
}

/** Checks the line from sp to ep against the stage, characters and forbidden areas.
 * @param dp enemy work (its own character is ignored)
 * @param sp start point
 * @param ep end point
 * @return the distance to the hit; negative (minus the distance) when the way is clear or only a
 *         low character is in it (unless flag 0x200) */
float enCheckHitEyes(struct EnLOCAL_DATA *dp, float *sp, float *ep) {
    struct _CL_VHIT_RESULT *res;
    int f;
    float cp[4];

    res = &enLocalWork.HitResult;
    f = 0;
    vcopy(ep, cp);
    if (enCheckForbiddenArea(sp, cp, dp->size)) {
        f = 1;
    }
    clCheckHitEyes(res, (unsigned int)dp->scp, sp, cp, 2);
    if (res->kind == 0) {
        if (f) {
            return enDist(sp, cp);
        }
        return -enDist(sp, cp);
    }
    if (res->kind == 3) {
        if (res->hobj.chara.sc->kind <= 0x12D && !(dp->flag & 0x200)) {
            return -enDist(sp, (float *)&res->hobj.chara.sc->pos);
        }
        return enDist(sp, res->hobj.chara.cp);
    }
    return enDist(sp, res->hobj.wall.cp);
}

/** enCheckHitEyes without the forbidden areas; low characters don't block unless flag 0x200 is set.
 * @param dp enemy work
 * @param sp start point
 * @param ep end point */
float enCheckHitEyes2(struct EnLOCAL_DATA *dp, float *sp, float *ep) {
    struct _CL_VHIT_RESULT *res;

    res = &enLocalWork.HitResult;
    clCheckHitEyes(res, (unsigned int)dp->scp, sp, ep, 0);
    if (res->kind == 0) {
        return -enDist(sp, ep);
    }
    if (res->kind == 3) {
        if (res->hobj.chara.sc->kind <= 0x12D && !(dp->flag & 0x200)) {
            return -enDist(sp, (float *)&res->hobj.chara.sc->pos);
        }
        return enDist(sp, res->hobj.chara.cp);
    }
    return enDist(sp, res->hobj.wall.cp);
}

/** Checks the line of sight from the target's eye to ep.
 * @param dp enemy work
 * @param ep end point */
float enCheckPlayerHitEyes(struct EnLOCAL_DATA *dp, float *ep) {
    struct _CL_VHIT_RESULT *res;
    float p1[4];
    float p2[4];

    res = &enLocalWork.HitResult;
    _shCopyVector(p1, (float *)&dp->scp->battle.target->pos);
    vcopy(ep, p2);
    p2[1] = p1[1] = dp->scp->battle.target->eye_y;
    clCheckHitEyes(res, (unsigned int)dp->scp->battle.target, p1, p2, 0);
    if (res->kind == 0) {
        return -distXYZ(p2, p1);
    }
    if (res->kind == 3) {
        return enDist(p1, res->hobj.chara.cp);
    }
    return enDist(p1, res->hobj.wall.cp);
}

/** Returns non-zero if there is floor within 100 above or below pos.
 * @param pos the point */
int enCheckFloor(float *pos) {
    struct _CL_VHIT_RESULT *res;
    float vec1[4];
    float vec2[4];
    int m;

    res = &enLocalWork.HitResult;
    vcopy(pos, vec1);
    vcopy(pos, vec2);
    vec1[1] -= 100.0f;
    vec2[1] += 100.0f;
    clCheckHitEyesOnlyFloor(res, NULL, vec1, vec2);
    if (res->kind == 0) {
        return 0;
    }
    m = res->hobj.wall.pd->material;
    if (m == 9 || m == 11) {
        return 2;
    }
    return 1;
}

/** Gets the world position of skeleton node n (zero while the character isn't displayed).
 * @param vec result
 * @param dp enemy work
 * @param n node index */
void enGetSkeletonVector(float *vec, struct EnLOCAL_DATA *dp, int n) {
    struct shSkelton *sp;
    int i;

    sp = dp->scp->sk_top;
    if (!(dp->scp->status & 0x10)) {
        vzero(vec);
        return;
    }
    i = 0;
    while (i < n && sp->next) {
        sp = sp->next;
        i++;
    }
    _sceVu0CopyVectorXYZ(vec, ((float (*)[4])&sp->src_m)[3]);
}

inline void mzero(void *d) {
    asm {
        sq zero, 0x0(d)
        sq zero, 0x10(d)
        sq zero, 0x20(d)
        sq zero, 0x30(d)
    }
}

/** Gets the matrix of skeleton node n (zero while the character isn't displayed).
 * @param mat result
 * @param dp enemy work
 * @param n node index */
void enGetSkeletonMatrix(float (*mat)[4], struct EnLOCAL_DATA *dp, int n) {
    struct shSkelton *sp;
    int i;

    sp = dp->scp->sk_top;
    if (!(dp->scp->status & 0x10)) {
        mzero(mat);
        return;
    }
    for (i = 0; i < n && sp->next; i++) {
        sp = sp->next;
    }
    mcopy(&sp->src_m, mat);
}

/** Picks the damage motion from where the hit came from and how hard it was.
 * @param dp enemy work */
int enGetDamageMotion(struct EnLOCAL_DATA *dp) {
    struct shBattleInfo *bi;
    int m;
    int id;
    int dd;
    float a;

    bi = &dp->scp->battle;
    a = shAngleRegulate(shAtanV(bi->vec) - dp->scp->rot.y);
    if (fabsf(a) > PI / 2) {
        dd = 0;
    } else {
        dd = 1;
    }
    id = dp->last_atk;
    switch (id) {
    case 2:
    case 1:
    case 0x24:
        m = 5;
        break;
    case 4:
    case 6:
        m = dd + 6;
        break;
    case 0x17:
        bi->shock = 0.0f;
        dp->hb_s = 0.0f;
    case 0xC:
    case 0xD:
    case 0xF:
    case 0x10:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x2C:
    case 0x2E:
    case 0x32:
        if (a < 0.0f) {
            m = 8;
        } else {
            m = 9;
        }
        break;
    case 0xE:
    case 0x11:
        m = 10;
        break;
    case 0x12:
    case 0x29:
    case 0x31:
        m = dd + 11;
        bi->shock = 0.0f;
        dp->hb_s = 0.0f;
        break;
    case 0x13:
    case 0x14:
        if (a < 0.0f) {
            m = 17;
        } else {
            m = 18;
        }
        break;
    case 0x15:
    case 0x16:
    case 0x2D:
        m = 19;
        break;
    case 0x18:
        m = 13;
        bi->shock = 0.0f;
        dp->hb_s = 0.0f;
        break;
    default:
        m = 5;
        printf("Illegal damage type!(%d)\n", id);
        break;
    }
    return m;
}

/** Picks the knock-down motion from where the hit came from.
 * @param dp enemy work */
int enGetDownMotion(struct EnLOCAL_DATA *dp) {
    struct shBattleInfo *bi;
    int m;
    int id;
    int dd;
    float a;

    bi = &dp->scp->battle;
    a = shAngleRegulate(shAtanV(bi->vec) - dp->scp->rot.y);
    if (fabsf(a) > PI / 2) {
        dd = 0;
    } else {
        dd = 1;
    }
    id = dp->last_atk;
    switch (id) {
    case 2:
    case 1:
    case 0x24:
        m = 14;
        break;
    case 4:
    case 6:
        m = dd + 15;
        break;
    case 0xC:
    case 0xD:
    case 0xF:
    case 0x10:
    case 0x13:
    case 0x14:
    case 0x17:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x2C:
    case 0x2E:
    case 0x32:
        if (a < 0.0f) {
            m = 17;
        } else {
            m = 18;
        }
        break;
    case 0xE:
    case 0x11:
    case 0x15:
    case 0x16:
    case 0x2D:
        m = 19;
        break;
    case 0x12:
    case 0x18:
    case 0x29:
    case 0x31:
        m = dd + 20;
        bi->shock = 0.0f;
        dp->hb_s = 0.0f;
        break;
    default:
        m = 14;
        printf("Illegal down type!(%d)\n", id);
        break;
    }
    return m;
}

/** Returns 1 if a down motion ends face up (15), else 0.
 * @param dm down motion */
int enGetLieDirection(int dm) {
    int m;

    switch (dm) {
    case 14:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
        m = 0;
        break;
    case 15:
        m = 1;
        break;
    default:
        m = 0;
        printf("Illegal down motion!(%d)\n", dm);
        break;
    }
    return m;
}

/** Starts animation anim (motion id) from its first frame.
 * @param dp enemy work
 * @param anim index in the kind's animation table
 * @param id motion id */
void enAnimeSet(struct EnLOCAL_DATA *dp, int anim, int id) {
    enAnimeRestart(dp);
    dp->anim = anim;
    dp->anim_s = 0x1000;
    dp->anim_n = 0;
    dp->anim_loop = 0;
    dp->anim_step = 0;
    dp->flag &= ~0x4080;
    if (EnAnimeSetFunc[dp->kind]) {
        EnAnimeSetFunc[dp->kind](dp->scp, id, (dp->flag & 0x8000) ? 0 : 1);
    }
    enSetTrans(dp);
    dp->tx2 = dp->tx;
    dp->tz2 = dp->tz;
}

/** Starts animation anim at a given frame.
 * @param dp enemy work
 * @param anim index in the kind's animation table
 * @param id motion id
 * @param frame start frame */
void enAnimeSetDirectFrame(struct EnLOCAL_DATA *dp, int anim, int id, int frame) {
    struct _AnimeInfo *ai;

    enAnimeRestart(dp);
    dp->anim = anim;
    dp->anim_s = 0x1000;
    dp->anim_loop = 0;
    dp->anim_step = 0;
    dp->flag &= ~0x4080;
    EnAnimeSetFunc[dp->kind](dp->scp, id, 0);
    ai = shCharacterAnimeGetInfo(dp->scp);
    if (frame < 0) {
        frame = ai->frame + frame;
        if (frame < 0) {
            frame = 0;
        }
    } else if (frame >= ai->frame) {
        frame = ai->frame - 1;
    }
    shCharacterAnimeFrameSet(dp->scp, frame);
    dp->anim_n = frame * 2400;
    enSetTrans(dp);
    dp->tx2 = dp->tx;
    dp->tz2 = dp->tz;
}

/** Advances the enemy's animation: looping and step counting.
 * @param dp enemy work
 * @param pa the kind's animation table
 * @param anm_none motion id meaning "no animation" */
void enAnimeExec(struct EnLOCAL_DATA *dp, struct EnANIME_DATA *pa, short anm_none) {
    int n;
    int limit;
    int speed;
    int loop;
    int f;
    struct _AnimeInfo *ai;

    if ((dp->flag & 1) || dp->anim_n < 0) {
        return;
    }
    pa += dp->anim;
    ai = shCharacterAnimeGetInfo(dp->scp);
    limit = ai->frame;
    speed = ai->speed;
    if (pa->Anime != anm_none) {
        loop = 1;
    } else {
        loop = 0;
    }
    f = shCharacterAnimeFrameGet(dp->scp) * 2400;
    if (dp->flag & 0x4000) {
        n = f;
        dp->flag &= ~0x4000;
    } else {
        n = dp->anim_n;
    }
    n += (dp->anim_s * ((ftoi(144000.0f * shGetDT()) * iabs(speed)) >> 12)) >> 12;
    if (dp->flag & 0x80) {
        if (dp->anim_s > 0) {
            if (n >= dp->anim_step) {
                n = -1;
            }
        } else {
            if (n <= -dp->anim_step) {
                n = -1;
            }
        }
    } else if (!pa->Loop && pa->Anime != anm_none) {
        if (f == 0) {
            dp->flag |= 0x4000;
        } else if (n >= 24000 && shCharacterAnimeIsEnd(dp->scp)) {
            n = -1;
        }
    } else if (dp->anim_s > 0) {
        if (n >= limit * 2400) {
            if (pa->Loop) {
                n = 0;
                dp->flag |= 0x4000;
                dp->anim_loop++;
            } else {
                n = -1;
            }
        } else if (f == 0) {
            dp->flag |= 0x4000;
        }
    } else {
        if (n <= 0) {
            if (pa->Loop) {
                if (f) {
                    n = limit * 2400;
                    dp->anim_loop++;
                } else {
                    n = 0;
                }
                dp->flag |= 0x4000;
            } else {
                n = -1;
            }
        } else if (f == 0) {
            dp->flag |= 0x4000;
        }
    }
    dp->anim_n = n;
    shCharacterAnimeSpeedAdd(dp->scp, speed * dp->anim_s / 4096 - speed);
}

/** Sets the animation translation (tx, tz) from the kind's two reference skeleton nodes.
 * @param dp enemy work */
void enSetTrans(struct EnLOCAL_DATA *dp) {
    struct EnSKELETON_DATA *sd;
    float vec1[4];
    float vec2[4];

    sd = &EnSkeletonData[dp->kind];
    if (sd->rate != 0.0f) {
        enGetSkeletonVector(vec1, dp, sd->num1);
        enGetSkeletonVector(vec2, dp, sd->num2);
        _shAddVector(vec1, vec1, vec2);
        _shScaleVector(vec1, vec1, sd->rate);
        dp->tx = vec1[0];
        dp->tz = vec1[2];
    }
}

/** Sets the animation translation (tx, tz) from skeleton node n.
 * @param dp enemy work
 * @param n node index */
void enSetTransN(struct EnLOCAL_DATA *dp, int n) {
    float vec[4];

    enGetSkeletonVector(vec, dp, n);
    dp->tx = vec[0];
    dp->tz = vec[2];
}

/** Sets the animation translation of a walk from the kind's two reference nodes.
 * @param dp enemy work */
void enSetTransWalk(struct EnLOCAL_DATA *dp) {
    float vec[2][4];
    int sn[2];
    int d;
    float od;

    od = dp->tz - dp->tz2;
    sn[0] = EnSkeletonData[dp->kind].num1;
    sn[1] = EnSkeletonData[dp->kind].num2;
    enGetSkeletonVector(vec[0], dp, sn[0]);
    enGetSkeletonVector(vec[1], dp, sn[1]);
    d = dp->anim_step - 1;
    if (d < 0) {
        d = vec[0][1] > vec[1][1];
    }
    if ((vec[d][1] < vec[d ^ 1][1]) != 0) {
        d ^= 1;
    }
    enSetTransN(dp, sn[d]);
    dp->tx = dp->tx2 = 0.0f;
    if (dp->anim_step != d + 1) {
        dp->anim_step = d + 1;
        dp->tz2 = dp->tz - od;
    }
}

/** Sets a straight forward translation of speed s.
 * @param dp enemy work
 * @param s speed */
void enSetTransForward(struct EnLOCAL_DATA *dp, float s) {
    dp->tx = dp->tx2 = dp->tz2 = 0.0f;
    dp->tz = -s * dp->anim_s / 4096.0f * shGetDT();
}

/** Pauses the enemy's animation.
 * @param dp enemy work */
void enAnimePause(struct EnLOCAL_DATA *dp) {
    dp->flag |= 1;
    shCharacterAnimePause(dp->scp);
}

/** Resumes the enemy's animation.
 * @param dp enemy work */
void enAnimeRestart(struct EnLOCAL_DATA *dp) {
    dp->flag &= ~1;
    shCharacterAnimeRestart(dp->scp);
}

/** Sets the current animation frame.
 * @param dp enemy work
 * @param frame the frame */
void enAnimeFrameSet(struct EnLOCAL_DATA *dp, unsigned short frame) {
    dp->anim_n = frame * 2400;
    shCharacterAnimeFrameSet(dp->scp, frame);
}

/** Plays the current animation backwards.
 * @param dp enemy work */
void enAnimeReverse(struct EnLOCAL_DATA *dp) {
    struct _AnimeInfo *ai;

    ai = shCharacterAnimeGetInfo(dp->scp);
    if (dp->anim_s > 0) {
        dp->anim_s = -dp->anim_s;
        if (dp->anim_n == 0) {
            dp->anim_n = ai->frame * 2400;
        }
    }
}

/** Sets how far into the animation the step counter runs (count / 4096 of its length).
 * @param dp enemy work
 * @param count fraction in 1/4096 */
void enSetAnimeCount(struct EnLOCAL_DATA *dp, int count) {
    int limit;

    limit = shCharacterAnimeGetInfo(dp->scp)->frame;
    dp->flag |= 0x80;
    dp->anim_step = (count * (limit * 2400)) >> 12;
}

/** Starts a blood pool under the enemy.
 * @param dp enemy work */
void enEfctBloodPool(struct EnLOCAL_DATA *dp) {
    float pos[4];

    _sceVu0CopyVectorXYZ(pos, (float *)&dp->scp->pos);
    pos[1] -= 3.0f;
    HH_Effect_Object_Blood_Pool_Impact_Post(pos, 0);
}

/** Starts a poison-fog puff (enEfctSetPoisonFog).
 * @param pos position
 * @param vec direction and size */
void enEfctPoisonFog(float *pos, float *vec) {
    enEfctSetPoisonFog(pos, vec);
}

/** Turns on the screen blur filter. */
void enSetBlur(void) {
    sh2gfw_Set_FilterBlur(0x60);
}

/** Starts a 2-second fade to black. */
void enSetFadeOut(void) {
    sh2gfw_Set_FadeOut_Black(2.0f);
}

/** Clears the screen filters. */
void enResetFilter(void) {
    sh2gfw_Reset_FilterCommand();
}

/** Plays sound effect num at pos.
 * @param num sound id
 * @param vol volume
 * @param pos position */
void enSoundCall(int num, float vol, float *pos) {
    SeCallPos(num, vol, pos, 0);
}

/** Plays sound effect num at pos, positioned in 3D.
 * @param num sound id
 * @param vol volume
 * @param pos position */
void enSoundCall3D(int num, float vol, float *pos) {
    SeCallPos(num, vol, pos, 1);
}

/** Stops sound effect num.
 * @param num sound id */
void enSoundStop(int num) {
    shSdSeStop(num);
}

/** Queues sound effect num to play at the character after time seconds (up to 8 queued).
 * @param scp the character
 * @param num sound id
 * @param vol volume
 * @param time delay in seconds */
void enSoundSetQueue(struct SubCharacter *scp, int num, float vol, float time) {
    struct EnSOUND_QUEUE *que;

    if (enLocalWork.SoundQueueNum < 8) {
        que = &enLocalWork.SoundQueue[enLocalWork.SoundQueueNum++];
        que->scp = scp;
        que->num = num;
        que->vol = vol;
        que->time = time;
    }
}

/** Posts a message for other enemies of a kind at pos.
 * @param kind enemy kind it is for
 * @param type message type
 * @param pos position
 * @param dist range
 * @param time lifetime in frames (negative: kept)
 * @return the message, or NULL when all 8 are in use */
struct EnCOMMUNICATION *enSetCommunication(int kind, int type, float *pos, float dist, int time) {
    int i;
    struct EnCOMMUNICATION *p;

    i = 0;
    p = enLocalWork.Communication;
    while (p->kind) {
        if (++i >= 8) {
            return NULL;
        }
        p++;
    }
    vcopy(pos, p->pos);
    p->kind = kind;
    p->type = type;
    p->dist = dist;
    if (p->time >= 0) {
        p->time = enCalcTimer(time);
    } else {
        p->time = time;
    }
    enLocalWork.CommunicationNum++;
    return p;
}

/** Returns the nearest message for kind whose range covers pos, or NULL.
 * @param kind enemy kind
 * @param pos position */
struct EnCOMMUNICATION *enCommunicateTribe(int kind, float *pos) {
    struct EnCOMMUNICATION *mp;
    struct EnCOMMUNICATION *p;
    int i;
    float d;
    float dist;

    mp = NULL;
    p = enLocalWork.Communication;
    if (enLocalWork.CommunicationNum == 0) {
        return mp;
    }
    dist = 3.4028235e38f;
    for (i = 0; i < 8; i++, p++) {
        if (p->kind == kind) {
            d = enDist(pos, p->pos);
            if (d < p->dist && d < dist) {
                dist = d;
                mp = p;
            }
        }
    }
    return mp;
}

/** Clears the forbidden areas. */
void enResetForbiddenArea(void) {
    enLocalWork.ForbiddenNum = 0;
}

#define EN_FORBIDDENAREA_MAX 2

/** Adds a rectangle (in XZ) the enemies must not enter.
 * @param x0 min x
 * @param z0 min z
 * @param x1 max x
 * @param z1 max z */
void enSetForbiddenArea(float x0, float z0, float x1, float z1) {
    int n = enLocalWork.ForbiddenNum; /* one line, as in the original's line table (2968) */

    /* Matching: the assert bakes its original line number into the object. */
#line 2969
    fjAssert(n < EN_FORBIDDENAREA_MAX);
    enLocalWork.ForbiddenArea[n].x0 = x0;
    enLocalWork.ForbiddenArea[n].z0 = z0;
    enLocalWork.ForbiddenArea[n].x1 = x1;
    enLocalWork.ForbiddenArea[n].z1 = z1;
    enLocalWork.ForbiddenNum = n + 1;
}

/* Matching: enRoomForbiddenArea: the stand-in before it sets its float-constant argument order (see its FAKEMATCH
 * note; fitted, not recovered: the original's line table leaves no room for a function here, docs/stand-ins.md). */
static float __stripped_float_code_1(float x0, float x1) { x0 += x0 * (x1 * x0); x1 += x1; x0 += -x0 * (x1 * x0 * x0 * x0 * x0 * x0 * x0 * x0 * x0 * x0 * x0 * x0 * x0 * x0) * (x1 * x0 * x0); if (x0 > 100.0f) { x1 = x0; } x1 += x0; x0 += 1000.0f; 1000.0f; x1 += x1 * (x0 * x1 * x1); { float t = x0; x0 = t * t; } if (x0 > x1) { x0 = x1; } return x0; } /* fitted, not recovered: 3 constants */
/** Sets up the forbidden areas of the player's stage and room (a fixed table per room). */
/* FAKEMATCH: the enSetForbiddenArea calls' float-constant argument order comes from the stand-in above, fitted to
 * what the compiler leaves over; the original had no code there (docs/stand-ins.md)
 * (docs/matching-notes.md#en_common-enroomforbiddenarea). */
void enRoomForbiddenArea(void) {
    int f;

    f = 0;
    switch (stage->glb_crd) {
    case 3:
        enSetForbiddenArea(-20000.0f, 40000.0f, -4000.0f, 57000.0f);
        enSetForbiddenArea(-100000.0f, 60000.0f, -80000.0f, 100000.0f);
        break;
    default:
        switch (RoomNameJms()) {
        case 0x30:
        case 0x65:
        case 0xBB:
            f = 1;
            break;
        case 0x7:
            enSetForbiddenArea(83500.0f, -71300.0f, 85000.0f, -69600.0f);
            break;
        case 0x92:
            enSetForbiddenArea(-20800.0f, 18400.0f, -19200.0f, 22800.0f);
            break;
        case 0x99:
            enSetForbiddenArea(-64000.0f, 20800.0f, -63200.0f, 23500.0f);
            enSetForbiddenArea(-60750.0f, 20750.0f, -59250.0f, 25600.0f);
            break;
        case 0xA0:
        case 0xB2:
            enSetForbiddenArea(60000.0f, -19400.0f, 60750.0f, -16800.0f);
            enSetForbiddenArea(61200.0f, -19400.0f, 62000.0f, -16800.0f);
            break;
        case 0xA3:
        case 0xB5:
            enSetForbiddenArea(16000.0f, 20000.0f, 16800.0f, 22600.0f);
            break;
        case 0xA8:
            enSetForbiddenArea(-20800.0f, -60000.0f, -19200.0f, -55600.0f);
            break;
        }
        break;
    }
    if (f) {
        enLocalWork.Status |= 2;
    } else {
        enLocalWork.Status &= ~2;
        enLocalWork.ActiveEnemy = 0;
    }
}

/** Returns non-zero if the segment sp-ep, widened by size, crosses a forbidden area.
 * @param sp start point
 * @param ep end point
 * @param size width */
int enCheckForbiddenArea(float *sp, float *ep, float size) {
    int i;
    int f;

    f = 0;
    for (i = 0; i < enLocalWork.ForbiddenNum; i++) {
        f |= enCheckForbiddenAreaSub(&enLocalWork.ForbiddenArea[i], sp, ep, size);
    }
    return f;
}

/** Returns non-zero if the segment sp-ep, widened by size, crosses forbidden area fa.
 * @param fa the area
 * @param sp start point
 * @param ep end point
 * @param size width */
int enCheckForbiddenAreaSub(struct EnFORBIDDENAREA *fa, float *sp, float *ep, float size) {
    float x0;
    float z0;
    float x1;
    float z1;
    float sx;
    float sz;
    float vx;
    float vz;
    float c;
    int f;

    x0 = fa->x0 - size;
    z0 = fa->z0 - size;
    x1 = fa->x1 + size;
    z1 = fa->z1 + size;
    sx = sp[0];
    sz = sp[2];
    if ((sx <= x0 && ep[0] <= x0) || (sx >= x1 && ep[0] >= x1) || (sz <= z0 && ep[2] <= z0) ||
        (sz >= z1 && ep[2] >= z1)) {
        return 0;
    }
    f = 0;
    vx = ep[0] - sx;
    vz = ep[2] - sz;
    if (sx < x0 + 2.0f * size) {
        c = sz + vz * (x0 - sx) / vx;
        if (c >= z0 && c <= z1) {
            f = 1;
            ep[0] = x0;
            ep[2] = c;
            vx = x0 - sx;
            vz = c - sz;
        }
    }
    if (sx > x1 - 2.0f * size) {
        c = sz + vz * (x1 - sx) / vx;
        if (c >= z0 && c <= z1) {
            f = 1;
            ep[0] = x1;
            ep[2] = c;
            vx = x1 - sx;
            vz = c - sz;
        }
    }
    if (sz < z0 + 2.0f * size) {
        c = sx + vx * (z0 - sz) / vz;
        if (c >= x0 && c <= x1) {
            f = 1;
            ep[0] = c;
            ep[2] = z0;
            vx = c - sx;
            vz = z0 - sz;
        }
    }
    if (sz > z1 - 2.0f * size) {
        c = sx + vx * (z1 - sz) / vz;
        if (c >= x0 && c <= x1) {
            f = 1;
            ep[0] = c;
            ep[2] = z1;
        }
    }
    return f;
}

/** Applies an event to the enemies (0: all to main level 0, 1: all but the automatic ones to
 * level 3, then kind-specific events for MKN, RED, ONI...).
 * @param event event number
 * @param id enemy id, for the events that need one */
void enEventDriven(int event, int id) {
    int i;
    struct EnLOCAL_DATA *dp;

    dp = enLocalWork.Data;
    switch (event) {
    case 0:
        for (i = 0; i < 32; i++, dp++) {
            if (dp->kind && dp->mlv) {
                dp->mlv = 0;
            }
        }
        break;
    case 1:
        for (i = 0; i < 32; i++, dp++) {
            if (dp->kind && dp->mlv != 1) {
                dp->mlv = 3;
            }
        }
        break;
    case 2:
        for (i = 0; i < 32; i++, dp++) {
            if (dp->kind == EnKIND_MKN && dp->type == 6) {
                dp->type = 7;
            }
        }
        break;
    case 3:
        for (i = 0; i < 32; i++, dp++) {
            if (dp->kind == EnKIND_RED) {
                dp->mlv = 4;
                dp->slv = 3;
            }
        }
        break;
    case 4:
        for (i = 0; i < 32; i++, dp++) {
            if (dp->kind == EnKIND_ONI && dp->mlv == 4) {
                dp->slv = event;
                dp->sslv = id;
            }
        }
        break;
    }
}
