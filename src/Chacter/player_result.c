/*
 * Play statistics for the end-of-game result screen (in `playing`): counters updated during
 * play, and the rank points computed from them.
 */

#include "sh2.h"
#include "sh_vu0.h"

/** Adds this frame's time to the play time. */
void GameTimerCountUp(void) {
    playing.time += shGetDT();
}

/**
 * Adds the distance @p scp moved this frame (position minus previous position; jumps of 200
 * or more are ignored) to the run distance if @p status is 0xC-0x11, else to the walk distance.
 */
void GameMoveDistanceCountUp(struct SubCharacter *scp, int status) {
    float distance;

    distance = _shLength((float *)&scp->pos, (float *)&scp->b_pos);
    if (distance < 200.0f) {
        switch (status) {
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:
        case 0x10:
        case 0x11:
            playing.run_distance += distance;
            break;
        default:
            playing.walk_distance += distance;
            break;
        }
    }
}

/** Counts a kill, by gun if @p atk is 1, 2, 4 or 6, otherwise by melee. */
void GameKillEnemyCountUp(unsigned short atk) {
    switch (atk) {
    case 1:
    case 2:
    case 4:
    case 6:
        playing.kill_by_shot++;
        break;
    default:
        playing.kill_by_fight++;
        break;
    }
}

/** Adds this frame's time to the boat time. */
void GameBoatTimerCountUp(void) {
    playing.boat_clear_time += shGetDT();
}

/** Records @p spd if it is the boat's highest speed so far. */
void GameBoatMaxSpeedCheck(float spd) {
    if (spd > playing.boat_max_speed) {
        playing.boat_max_speed = spd;
    }
}

/** Counts a picked-up item. */
void GameItemGetCountUp(void) {
    playing.item_get++;
}

/** Adds @p damage to James's total damage. */
void GameJamesDamagedCountUp(float damage) {
    playing.jms_damage_total += damage;
}

/** Adds @p damage to Maria's damage, from enemies if @p atk >= 0x24, else from James. */
void GameMariaDamagedCountUp(unsigned short atk, float damage) {
    if (atk >= 0x24) {
        playing.mar_damage_by_enemy += damage;
    } else {
        playing.mar_damage_by_jms += damage;
    }
}

/** Returns 0-5 points for the number of saves (fewer is better). */
unsigned int GameCalcRankSaveCount(void) {
    if (playing.savecount < 3) {
        return 5;
    }
    if (playing.savecount < 6) {
        return 4;
    }
    if (playing.savecount < 11) {
        return 3;
    }
    if (playing.savecount < 21) {
        return 2;
    }
    if (playing.savecount < 31) {
        return 1;
    }
    return 0;
}

/** Returns the points for the endings seen (4 each). */
unsigned int GameCalcRankEndingKind(void) {
    unsigned int result = 0;

    /* @bug `|`, not `&`, so every test is true; as in the original */
    if (playing.clear_end_kind | 1) {
        result += 4;
    }
    if (playing.clear_end_kind | 2) {
        result += 4;
    }
    if (playing.clear_end_kind | 4) {
        result += 4;
    }
    if (playing.clear_end_kind | 8) {
        result += 4;
    }
    if (playing.clear_end_kind | 0x10) {
        result += 4;
    }
    return result;
}

/** Returns the points for the action (battle) difficulty. */
unsigned int GameCalcRankBattleLevel(void) {
    switch (playing.battle_level) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 3;
    case 3:
        return 5;
    default:

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 264
        assert_dw(0); /* Matching: do/while(0) form (its nop) */
    }
}

/** Returns the points for the riddle difficulty. */
unsigned int GameCalcRankRiddleLevel(void) {
    switch (playing.riddle_level) {
    case 0:
        return 1;
    case 1:
        return 3;
    case 2:
        return 5;
    default:

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 292
        assert(0);
    }
}

/** Returns 1 point per 10 items picked up (at most 10). */
unsigned int GameCalcRankItemGet(void) {
    if (playing.item_get > 100) {
        return 10;
    }
    return playing.item_get / 10;
}

/** Returns the points for the hidden items found. */
unsigned int GameCalcRankHiddenItemGet(void) {
    unsigned int result = 0;

    if (playing.hidden_item_get & 1) {
        result += 1;
    }
    if (playing.hidden_item_get & 2) {
        result += 1;
    }
    if (playing.hidden_item_get & 4) {
        result += 3;
    }
    return result;
}

/** Returns 1 point per 10 kills by gun (at most 15). */
unsigned int GameCalcRankKillByShot(void) {
    if (playing.kill_by_shot > 150) {
        return 15;
    }
    return playing.kill_by_shot / 10;
}

/** Returns 1 point per 10 melee kills (at most 15). */
unsigned int GameCalcRankKillByFight(void) {
    if (playing.kill_by_fight > 150) {
        return 15;
    }
    return playing.kill_by_fight / 10;
}

/** Returns 0-10 points for the play time (faster is better). */
unsigned int GameCalcRankClearTime(void) {
    float time;

    time = playing.time / 60.0f;
    if (time <= 120.0f) {
        return 10;
    }
    if (time <= 180.0f) {
        return 5;
    }
    if (time <= 240.0f) {
        return 3;
    }
    if (time <= 360.0f) {
        return 2;
    }
    if (time <= 720.0f) {
        return 1;
    }
    return 0;
}

/** Returns 0-5 points for the boat time (faster is better). */
unsigned int GameCalcRankBoatClearTime(void) {
    if (playing.boat_clear_time <= 60.0f) {
        return 5;
    }
    if (playing.boat_clear_time <= 120.0f) {
        return 4;
    }
    if (playing.boat_clear_time <= 240.0f) {
        return 3;
    }
    if (playing.boat_clear_time <= 480.0f) {
        return 2;
    }
    if (playing.boat_clear_time <= 960.0f) {
        return 1;
    }
    return 0;
}

/** Returns 0-5 points for James's total damage (less is better). */
unsigned int GameCalcRankJamesDamage(void) {
    if (playing.jms_damage_total <= 200.0f) {
        return 5;
    }
    if (playing.jms_damage_total <= 400.0f) {
        return 4;
    }
    if (playing.jms_damage_total <= 800.0f) {
        return 3;
    }
    if (playing.jms_damage_total <= 1600.0f) {
        return 2;
    }
    if (playing.jms_damage_total <= 3200.0f) {
        return 1;
    }
    return 0;
}

/** Returns the sum of all rank points. */
unsigned int GameCalcRankTotal(void) {
    return GameCalcRankSaveCount() + GameCalcRankEndingKind() + GameCalcRankBattleLevel() +
           GameCalcRankRiddleLevel() + GameCalcRankItemGet() + GameCalcRankHiddenItemGet() +
           GameCalcRankKillByShot() + GameCalcRankKillByFight() + GameCalcRankClearTime() +
           GameCalcRankBoatClearTime() + GameCalcRankJamesDamage();
}

/** Stores the total rank in playing.rank. */
void GameSavePreviousTotalRank(void) {
    playing.rank = GameCalcRankTotal();
}

/** Sets playing.spray_pow (-1 to 2) from playing.rank. */
void GameSaveSprayPower(void) {
    if (playing.rank < 6) {
        playing.spray_pow = -1;
    } else if (playing.rank < 80) {
        playing.spray_pow = 0;
    } else if (playing.rank < 100) {
        playing.spray_pow = 1;
    } else {
        playing.spray_pow = 2;
    }
}
