/*
 * Battle: the attack table (sh2_attack_list), the attack queue that turns each attack into
 * collision checks (clBattleAddQue) and applies the results (damage, shock, reactions), and
 * the attack start/end points of James's and the enemies' weapons.
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "fi_libvu0_inline.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"


static const struct shAttackInfo sh2_attack_list[66] = {
    {0x0, 0, 0.0f, 0.0f, 0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0, 0, 0, 0},
    {0x1, 3, 120.0f, 60.0f, -100.0f, 7500.0f, {{0.0f, 0.0f, 0.0f}}, 15, 25, 0, 0},
    {0x2, 3, 120.0f, 60.0f, -100.0f, 7500.0f, {{0.0f, 0.0f, 0.0f}}, 0, 10, 0, 0},
    {0x3, 0, 0.0f, 0.0f, 0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0, 0, 0, 0},
    {0x4, 3, 100.0f, 120.0f, -100.0f, 5000.0f, {{0.0f, 0.0f, 0.0f}}, 0, 10, 0, 0},
    {0x5, 0, 0.0f, 0.0f, 0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0, 0, 0, 0},
    {0x6, 7, 500.0f, 150.0f, -200.0f, 9000.0f, {{0.0f, 0.0f, 0.0f}}, 0, 10, 0, 0},
    {0x7, 0, 0.0f, 0.0f, 0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0, 0, 0, 0},
    {0x8, 7, 1.0f, 0.0f, -125.0f, 1000.0f, {{0.0f, 0.0f, 0.0f}}, 8, 25, 0, 0},
    {0x9, 7, 1.0f, 0.0f, -125.0f, 1000.0f, {{0.0f, 0.0f, 0.0f}}, 2, 19, 0, 0},
    {0xA, 0, 0.0f, 0.0f, 0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0, 30, 0, 0},
    {0xB, 0, 0.0f, 0.0f, 0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0, 24, 0, 0},
    {0xC, 2, 100.0f, 80.0f, 0.0f, 350.0f, {{0.0f, 0.0f, 0.0f}}, 12, 15, 0, 0},
    {0xD, 2, 80.0f, 80.0f, 0.0f, 350.0f, {{0.0f, 0.0f, 0.0f}}, 8, 11, 0, 0},
    {0xE, 2, 140.0f, 80.0f, 0.0f, 350.0f, {{0.0f, 0.0f, 0.0f}}, 8, 10, 0, 0},
    {0xF, 2, 120.0f, 100.0f, 0.0f, 540.0f, {{0.0f, 0.0f, 0.0f}}, 10, 12, 0, 0},
    {0x10, 2, 96.0f, 100.0f, 0.0f, 540.0f, {{0.0f, 0.0f, 0.0f}}, 4, 6, 0, 0},
    {0x11, 2, 160.0f, 100.0f, 0.0f, 540.0f, {{0.0f, 0.0f, 0.0f}}, 15, 17, 0, 0},
    {0x12, 3, 130.0f, 50.0f, 0.0f, 540.0f, {{0.0f, 0.0f, 0.0f}}, 8, 10, 0, 0},
    {0x13, 1, 600.0f, 0.0f, 0.0f, 725.0f, {{0.0f, 0.0f, 0.0f}}, 10, 14, 0, 0},
    {0x14, 1, 600.0f, 0.0f, 0.0f, 725.0f, {{0.0f, 0.0f, 0.0f}}, 13, 17, 0, 0},
    {0x15, 1, 1000.0f, 0.0f, 0.0f, 725.0f, {{0.0f, 0.0f, 0.0f}}, 17, 18, 0, 0},
    {0x16, 1, 1000.0f, 0.0f, 0.0f, 725.0f, {{0.0f, 0.0f, 0.0f}}, 18, 19, 0, 0},
    {0x17, 1, 250.0f, 0.0f, 0.0f, 525.0f, {{0.0f, 0.0f, 0.0f}}, 12, 27, 0, 0},
    {0x18, 3, 300.0f, 0.0f, 0.0f, 525.0f, {{0.0f, 0.0f, 0.0f}}, 6, 13, 0, 0},
    {0x19, 2, 9999.9f, 0.1f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 7, 10, 0, 0},
    {0x1A, 2, 9999.9f, 0.1f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 7, 10, 0, 0},
    {0x1B, 2, 9999.9f, 0.1f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 7, 10, 0, 0},
    {0x1C, 2, 9999.9f, 0.1f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 7, 10, 0, 0},
    {0x1D, 2, 9999.9f, 0.1f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 7, 11, 0, 0},
    {0x1E, 2, 9999.9f, 0.1f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 3, 9, 0, 0},
    {0x1F, 2, 9999.9f, 0.1f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 3, 9, 0, 0},
    {0x20, 2, 9999.9f, 0.1f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 3, 9, 0, 0},
    {0x21, 2, 9999.9f, 0.1f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 3, 9, 0, 0},
    {0x22, 2, 9999.9f, 0.1f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 3, 9, 0, 0},
    {0x0, 0, 0.0f, 0.0f, 0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0, 0, 0, 0},
    {0x24, 4, 7.0f, 5.0f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 9, 22, 0, 0},
    {0x25, 4, 7.0f, 5.0f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 9, 22, 0, 0},
    {0x26, 2, 18.0f, 10.0f, 0.0f, 400.0f, {{0.0f, 0.0f, 0.0f}}, 19, 22, 0, 0},
    {0x27, 2, 18.0f, 10.0f, 0.0f, 400.0f, {{0.0f, 0.0f, 0.0f}}, 19, 22, 0, 0},
    {0x28, 2, 20.0f, 10.0f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 15, 18, 0, 0},
    {0x29, 3, 15.0f, 12.5f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 13, 16, 0, 0},
    {0x2A, 2, 20.0f, 12.5f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 15, 18, 0, 0},
    {0x2B, 3, 15.0f, 15.0f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 13, 16, 0, 0},
    {0x2C, 1, 40.0f, 0.5f, 0.0f, 600.0f, {{0.0f, 0.0f, 0.0f}}, 21, 27, 0, 0},
    {0x2D, 1, 1000.0f, 0.0f, 0.0f, 600.0f, {{0.0f, 0.0f, 0.0f}}, 67, 71, 0, 0},
    {0x2E, 2, 25.0f, 12.5f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 7, 10, 0, 0},
    {0x2F, 2, 1.0f, 0.0f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 8, 10, 0, 0},
    {0x30, 6, 0.1f, 0.0f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 0, 23, 0, 0},
    {0x31, 3, 30.0f, 15.0f, 0.0f, 700.0f, {{0.0f, 0.0f, 0.0f}}, 17, 23, 0, 0},
    {0x32, 2, 50.0f, 17.5f, 0.0f, 700.0f, {{0.0f, 0.0f, 0.0f}}, 0, 28, 0, 0},
    {0x33, 3, 5.0f, 0.0f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 0, 7, 0, 0},
    {0x34, 3, 24.0f, 0.0f, 0.0f, 5000.0f, {{0.0f, 0.0f, 0.0f}}, 0, 1, 0, 0},
    {0x35, 2, 15.0f, 10.0f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 9, 11, 0, 0},
    {0x36, 2, 5.0f, 0.0f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 28, 31, 0, 0},
    {0x37, 6, 0.1f, 0.0f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 0, 23, 0, 0},
    {0x38, 2, 15.0f, 7.5f, 0.0f, 250.0f, {{0.0f, 0.0f, 0.0f}}, 11, 14, 0, 0},
    {0x39, 2, 5.0f, 0.0f, 0.0f, 400.0f, {{0.0f, 0.0f, 0.0f}}, 0, 19, 0, 0},
    {0x3A, 6, 0.1f, 0.0f, 0.0f, 400.0f, {{0.0f, 0.0f, 0.0f}}, 0, 23, 0, 0},
    {0x3B, 2, 0.1f, 0.0f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 11, 14, 0, 0},
    {0x3C, 6, 0.1f, 0.0f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 0, 23, 0, 0},
    {0x3D, 2, 20.0f, 10.0f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 7, 9, 0, 0},
    {0x3E, 2, 1.0f, 0.0f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 0, 99, 0, 0},
    {0x3F, 6, 0.05f, 0.0f, 0.0f, 300.0f, {{0.0f, 0.0f, 0.0f}}, 0, 99, 0, 0},
    {0x40, 3, 10.0f, 0.0f, 0.0f, 450.0f, {{0.0f, 0.0f, 0.0f}}, 0, 16, 0, 0},
    {0x41, 3, 10.0f, 0.0f, 0.0f, 450.0f, {{0.0f, 0.0f, 0.0f}}, 0, 16, 0, 0},
};

static float sh2_battle_wall_hit = 0.0f;
static int sh2_battle_attack_check = 0;
static struct shAttackQueue sh2_attack_queue;

static void shBattleDamageRevise(float *damage, float *shock, struct SubCharacter *scp, struct _CL_BATTLE_RESULT *result) {
    if (scp->battle.status & 0x40) {
        *damage = 0.0f;
    } else {
        switch (result->btlid & 0xFF) {
            case 0x2D:
                scp->battle.hp = -1.0f;
                *damage = sh2_attack_list[result->btlid & 0xFF].ap;
                break;
            default:
                *damage = sh2_attack_list[result->btlid & 0xFF].ap;
                break;
        }
    }
    *shock = sh2_attack_list[result->btlid & 0xFF].sp;
}

static void shBattleSetEffectDamage(struct SubCharacter *scp, float *pos, float *vec, unsigned short atk) {
    float vec_tmp[4];
    int atk_type;

    switch (scp->kind) {
        case 0x208:
        case 0x209:
        case 0x421:
            return;
    }
    if (atk <= 7 || atk == 0x34) {
        atk_type = 0;
    } else {
        atk_type = 1;
    }
    if (atk != 0x35 && atk != 0x3F && atk != 0x3E && atk != 0x3C && atk != 0x3B && atk != 0x3A && atk != 0x39 && atk != 0x37 &&
        atk != 0x36 && atk != 0x30 && atk != 0x2F && atk != 0x25 && atk != 0x24 && atk != 0x9 && atk != 0x8) {
        vcopy_gcc(vec_tmp, vec);
        HH_Effect_Object_Blood_Splash_Impact_Post(pos, vec_tmp, (unsigned int)scp, atk_type);
    }
}

static void shBattleSetSoundDamage(struct SubCharacter *scp, struct _CL_BATTLE_RESULT *result) {
    int se;
    int type;
    float vol;

    type = 0;
    switch (result->btlid & 0xFF) {
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
        case 0x20:
        case 0x21:
        case 0x22:
            se = 0x2B19;
            vol = 0.8f;
            break;
        case 0x17:
        case 0x18:
            se = 0x2B26;
            vol = 0.8f;
            break;
        case 0x8:
        case 0x9:
            return;
        case 0x1:
        case 0x2:
        case 0x4:
        case 0x6:
            type = 1;
        default:
            se = -1;
            break;
    }
    if (se == -1 && !(scp->battle.status & 2)) {
        switch (scp->kind) {
            case 0x200:
                se = 0x2EEF + (shRandI() >> 10) % 4;
                vol = 1.0f;
                break;
            case 0x201:
                se = 0x30D5 + (shRandI() >> 10) % 4;
                vol = 1.0f;
                break;
            case 0x207:
            case 0x20B:
                se = 0x2F51 + (shRandI() >> 10) % 4;
                vol = 1.0f;
                break;
            case 0x205:
                se = 0x471B + (shRandI() >> 10) % 4;
                vol = 1.0f;
                break;
            case 0x203:
                se = 0x4845 + (shRandI() >> 10) % 3;
                vol = 1.0f;
                break;
            case 0x204:
                se = 0x477D + (shRandI() >> 10) % 4;
                vol = 0.8f;
                break;
            case 0x208:
            case 0x209:
                switch (result->btlid & 0xFF) {
                    case 0x2:
                    case 0x1:
                        se = 0x3EEE;
                        vol = 1.0f;
                        break;
                    case 0xC:
                    case 0xD:
                    case 0xE:
                        se = 0x3EF0;
                        vol = 1.0f;
                        break;
                    case 0xF:
                    case 0x10:
                    case 0x11:
                    case 0x12:
                        se = 0x3EEF;
                        vol = 1.0f;
                        break;
                    default:
                        se = 0x3EF2 + (shRandI() >> 10) % 4;
                        vol = 0.8f;
                        break;
                }
                break;
            case 0x202:
                se = 0x2FB0 + (shRandI() >> 10) % 6;
                vol = 0.5f;
                break;
            case 0x20A:
                se = 0x2FDE + (shRandI() >> 10) % 4;
                vol = 1.0f;
                break;
            case 0x206:
                se = 0x49AA + (shRandI() >> 10) % 4;
                vol = 0.8f;
                break;
            default:
                se = -1;
                break;
        }
    }
    if (se > 0) {
        if (type) {
            enSoundSetQueue(scp, se, vol, 0.1f);
        } else {
            SeCallPos(se, vol, result->pos, 0);
        }
    }
}

static void shBattleAddEffectAttack(struct SubCharacter *scp, float *pos, float *vec) {
    EFCTSetGunFire(pos, vec);
    EFCTSetGunSmoke(pos);
    sh2gfw_Set_JmsGunLight();
}

static void shBattleAttackByHumanGunshotTypeA(struct SubCharacter *attacker, unsigned short atk) {
    float gunpos[4];
    float gunvec[4];
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    struct _CL_BATTLE_QUE que;
    int wep;

    if (atk == 3 || atk == 7) {
        return;
    }
    cur_frame = shCharacterAnimeFrameGet_(attacker, 1);
    st = sh2_attack_list[atk].atk_start;
    ed = sh2_attack_list[atk].atk_end;
    if (cur_frame >= st && cur_frame <= ed && !attacker->battle.atk_result) {
        shGetJamesWeaponEndPos(gunpos, gunvec);
        que.svs[0] = gunpos[0] + gunvec[0] * sh2_attack_list[atk].min_range;
        que.svs[1] = gunpos[1] + gunvec[1] * sh2_attack_list[atk].min_range;
        que.svs[2] = gunpos[2] + gunvec[2] * sh2_attack_list[atk].min_range;
        que.svs[3] = 1.0f;
        que.sve[0] = gunpos[0] + gunvec[0] * sh2_attack_list[atk].max_range;
        que.sve[1] = gunpos[1] + gunvec[1] * sh2_attack_list[atk].max_range;
        que.sve[2] = gunpos[2] + gunvec[2] * sh2_attack_list[atk].max_range;
        que.sve[3] = 1.0f;
        que.btlid = atk + 0x100;
        que.kind = sh2_attack_list[atk].kind;
        que.sc = attacker;
        clBattleAddQue(&que);
        attacker->battle.atk_result = 1;
        shBattleAddEffectAttack(attacker, gunpos, gunvec);
        SeCallPos(atk == 6 ? 0x2B28 : 0x2B14, 1.0f, gunpos, 0);
        wep = PlayerNowItemName(sh2jms.weapon);
        ItemWeaponShoot(wep, 1);
        sh2jms.se_on = 1;
        sh2jms.d_shock = 4;
    }
    if (cur_frame > ed) {
        attacker->battle.atk_result = 0;
    }
}

static void shBattleAttackByHumanGunshotTypeB(struct SubCharacter *attacker, unsigned short atk) {
    int i;
    float vec[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float rot[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float gunpos[4];
    float gunvec[4];
    float unit[4][4];
    float mat[4][4];
    float rot_spread;
    float rot_direction;
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    struct _CL_BATTLE_QUE que;
    int wep;

    if (atk == 5) {
        return;
    }
    cur_frame = shCharacterAnimeFrameGet(attacker);
    st = sh2_attack_list[atk].atk_start;
    ed = sh2_attack_list[atk].atk_end;
    if (cur_frame >= st && cur_frame <= ed && !attacker->battle.atk_result) {
        shGetJamesWeaponEndPos(gunpos, gunvec);
        rot[1] = shAtan2(gunvec[2], gunvec[0]);
        rot[0] = -shAtan2(_shLengthXZ(gunvec), gunvec[1]);
        _sceVu0UnitMatrix(unit);
        shRotMatrixZ(mat, unit, rot[2]);
        shRotMatrixX(mat, mat, rot[0]);
        shRotMatrixY(mat, mat, rot[1]);
        for (i = 0; i < 10; i++) {
            rot_spread = 0.5235988f * shRandF();
            rot_direction = 3.1415927f * (2.0f * shRandF() - 1.0f);
            vec[2] = shCosF(rot_spread);
            vec[0] = shSinF(rot_spread) * shSinF(rot_direction);
            vec[1] = -shSinF(rot_spread) * shCosF(rot_direction);
            sceVu0ApplyMatrix(vec, mat, vec);
            que.svs[0] = gunpos[0] + vec[0] * sh2_attack_list[atk].min_range;
            que.svs[1] = gunpos[1] + vec[1] * sh2_attack_list[atk].min_range;
            que.svs[2] = gunpos[2] + vec[2] * sh2_attack_list[atk].min_range;
            que.svs[3] = 1.0f;
            que.sve[0] = gunpos[0] + vec[0] * sh2_attack_list[atk].max_range;
            que.sve[1] = gunpos[1] + vec[1] * sh2_attack_list[atk].max_range;
            que.sve[2] = gunpos[2] + vec[2] * sh2_attack_list[atk].max_range;
            que.sve[3] = 1.0f;
            que.btlid = atk + 0x100;
            que.kind = sh2_attack_list[atk].kind;
            que.sc = attacker;
            clBattleAddQue(&que);
        }
        attacker->battle.atk_result = 1;
        shBattleAddEffectAttack(attacker, gunpos, gunvec);
        SeCallPos(0x2B2B, 1.0f, gunpos, 0);
        wep = PlayerNowItemName(sh2jms.weapon);
        ItemWeaponShoot(wep, 1);
        sh2jms.se_on = 1;
        sh2jms.d_shock = 4;
    }
    if (cur_frame > ed) {
        attacker->battle.atk_result = 0;
    }
}

static void shBattleAttackByHumanFightType(struct SubCharacter *attacker, unsigned short atk) {
    int jouken;
    float s_pos[4];
    float s_vec[4];
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    struct _CL_BATTLE_QUE que;

    cur_frame = shCharacterAnimeFrameGet_(attacker, 1);
    st = sh2_attack_list[atk].atk_start;
    ed = sh2_attack_list[atk].atk_end;
    shGetJamesWeaponStartPos(s_pos, s_vec);
    que.eve[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].max_range;
    que.eve[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].max_range;
    que.eve[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].max_range;
    que.eve[3] = 1.0f;
    switch (atk) {
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:
        case 0x10:
        case 0x11:
        case 0x12:
            if (cur_frame >= st && cur_frame <= ed && !attacker->battle.atk_result) {
                jouken = 1;
            } else {
                jouken = 0;
            }
            break;
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
            if (cur_frame >= st && cur_frame <= ed) {
                jouken = 1;
            } else {
                jouken = 0;
            }
            break;
        default:
            return;
    }
    if (cur_frame == ed) {
        sh2jms.wep_no_hit_floor = 1;
    } else {
        sh2jms.wep_no_hit_floor = 0;
    }
    if (jouken) {
        que.evs[0] = que.svs[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].min_range;
        que.evs[1] = que.svs[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].min_range;
        que.evs[2] = que.svs[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].min_range;
        que.svs[3] = que.evs[3] = 1.0f;
        que.sve[0] = attacker->battle.prev_atk_pos[0];
        que.sve[1] = attacker->battle.prev_atk_pos[1];
        que.sve[2] = attacker->battle.prev_atk_pos[2];
        que.sve[3] = 1.0f;
        que.btlid = atk + 0x100;
        que.kind = sh2_attack_list[atk].kind;
        que.sc = attacker;
        if (attacker->battle.prev_atk_pos[3]) {
            clBattleAddQue(&que);
        }
    }
    sceVu0CopyVector(attacker->battle.prev_atk_pos, que.eve);
    if (cur_frame > ed) {
        attacker->battle.atk_result = 0;
    }
}

/**
 * Gets the direction of spray ray @p i: 0 along @p s_vec, 1-4 turned 0.419 rad (24 degrees)
 * in pitch or yaw either way. @param s_pos spray origin. @param result out: unit direction.
 */
void shGetHumanAttackSprayPos(int i, float *s_pos, float *s_vec, float *result) {
    float pos1[4];
    float vec[4];
    float rotx;
    float roty;

    roty = shAtan2(s_vec[2], s_vec[0]);
    rotx = shAtan2(_shLengthXZ(s_vec), s_vec[1]);
    switch (i) {
        case 0:
            pos1[0] = s_pos[0] + 900.0f * shSinF(roty);
            pos1[2] = s_pos[2] + 900.0f * shCosF(roty);
            pos1[1] = s_pos[1] + 900.0f * shSinF(rotx);
            break;
        case 1:
            pos1[0] = s_pos[0] + 900.0f * shSinF(roty);
            pos1[2] = s_pos[2] + 900.0f * shCosF(roty);
            pos1[1] = s_pos[1] + 900.0f * shSinF(0.41887903f + rotx);
            break;
        case 2:
            pos1[0] = s_pos[0] + 900.0f * shSinF(0.41887903f + roty);
            pos1[2] = s_pos[2] + 900.0f * shCosF(0.41887903f + roty);
            pos1[1] = s_pos[1] + 900.0f * shSinF(rotx);
            break;
        case 3:
            pos1[0] = s_pos[0] + 900.0f * shSinF(roty);
            pos1[2] = s_pos[2] + 900.0f * shCosF(roty);
            pos1[1] = s_pos[1] + 900.0f * shSinF(rotx - 0.41887903f);
            break;
        case 4:
            pos1[0] = s_pos[0] + 900.0f * shSinF(roty - 0.41887903f);
            pos1[2] = s_pos[2] + 900.0f * shCosF(roty - 0.41887903f);
            pos1[1] = s_pos[1] + 900.0f * shSinF(rotx);
            break;
    }
    vec[0] = pos1[0] - s_pos[0];
    vec[1] = pos1[1] - s_pos[1];
    vec[2] = pos1[2] - s_pos[2];
    vec[3] = 0.0f;
    _shNormalize(result, vec);
}

static void shBattleAttackByHumanFog(struct SubCharacter *attacker, unsigned short atk) {
    static float max_range;
    static float min_range;
    int i;
    float s_pos[4];
    float s_vec[4];
    float s_vec_result[4];
    float sp_start[4];
    float sp_end[4];
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    struct _CL_BATTLE_QUE que;
    int wep;
    unsigned int pow;

    if (atk == 10 || atk == 11) {
        attacker->battle.se = 0;
        return;
    }
    if (sh2jms.spray_time == 0.0f) {
        wep = PlayerNowItemName(sh2jms.weapon);
        while (ItemWeaponShoot(wep, 1)) {
        }
        return;
    }
    cur_frame = shCharacterAnimeFrameGet_(attacker, 1);
    st = sh2_attack_list[atk].atk_start; /* unused, as in the original (st, ed in its DWARF) */
    ed = sh2_attack_list[atk].atk_end;
    shGetJamesWeaponEndPos(s_pos, s_vec);
    max_range = (sh2jms.spray_time >= 10.0f) ? sh2_attack_list[atk].max_range : 0.1f * (sh2jms.spray_time * sh2_attack_list[atk].max_range);
    min_range = (sh2jms.spray_time >= 10.0f) ? sh2_attack_list[atk].min_range : 0.1f * (sh2jms.spray_time * sh2_attack_list[atk].min_range);
    for (i = 0; i < 5; i++) {
        shGetHumanAttackSprayPos(i, s_pos, s_vec, s_vec_result);
        que.svs[0] = s_pos[0] + s_vec_result[0] * min_range;
        que.svs[1] = s_pos[1] + s_vec_result[1] * min_range;
        que.svs[2] = s_pos[2] + s_vec_result[2] * min_range;
        que.sve[0] = s_pos[0] + s_vec_result[0] * max_range;
        que.sve[1] = s_pos[1] + s_vec_result[1] * max_range;
        que.sve[2] = s_pos[2] + s_vec_result[2] * max_range;
        que.svs[3] = que.sve[3] = 1.0f;
        que.btlid = atk + 0x100;
        que.kind = sh2_attack_list[atk].kind;
        que.sc = attacker;
        clBattleAddQue(&que);
        if (i == 0) {
            max_range += 150.0f;
            que.sve[0] = s_pos[0] + s_vec_result[0] * max_range;
            que.sve[1] = s_pos[1] + s_vec_result[1] * max_range;
            que.sve[2] = s_pos[2] + s_vec_result[2] * max_range;
            vcopy_gcc(sp_start, que.svs);
            _shSubVector(sp_end, que.sve, que.svs);
        }
    }
    /* Matching: #line puts the asserts below on their original source lines. */
#line 939
    switch (playing.spray_pow) {
        case -1:
            pow = 0xFF400040;
            break;
        case 0:
            pow = 0xFFC0C0C0;
            break;
        case 1:
            pow = 0xFF40C0C0;
            break;
        case 2:
            pow = 0xFF40FF40;
            break;
        default:
            assert(0);
    }
    enEfctSetSpray(sp_start, sp_end, pow, 12);
    if (!attacker->battle.se) {
        if (!sh2jms.csaw_se_vol) {
            sh2jms.csaw_se_vol = 0.7f;
            SeCallPos(0x2B27, sh2jms.csaw_se_vol, s_pos, 0);
        }
        attacker->battle.se = 1;
    }
    sh2jms.spray_time -= shGetDT();
    if (sh2jms.spray_time <= 0.0f) {
        wep = PlayerNowItemName(sh2jms.weapon);
        while (ItemWeaponShoot(wep, 1)) {
        }
        sh2jms.spray_time = 0.0f;
    }
}

static void shBattleAttackByHumanFinish(struct SubCharacter *attacker, unsigned short atk) {
    float s_pos[4];
    float s_vec[4];
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    struct _CL_BATTLE_QUE que;

    cur_frame = shCharacterAnimeFrameGet_(attacker, 2);
    st = sh2_attack_list[atk].atk_start;
    ed = sh2_attack_list[atk].atk_end;
    if (atk >= 0x1E) {
        shGetJamesTrampStartPos(s_pos, s_vec);
    } else {
        shGetJamesKickStartPos(s_pos, s_vec);
    }
    que.eve[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].max_range;
    que.eve[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].max_range;
    que.eve[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].max_range;
    que.eve[3] = 1.0f;
    if (cur_frame >= st && cur_frame <= ed) {
        que.evs[0] = que.svs[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].min_range;
        que.evs[1] = que.svs[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].min_range;
        que.evs[2] = que.svs[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].min_range;
        que.sve[0] = attacker->battle.prev_atk_pos[0];
        que.sve[1] = attacker->battle.prev_atk_pos[1];
        que.sve[2] = attacker->battle.prev_atk_pos[2];
        que.svs[3] = que.sve[3] = que.evs[3] = que.eve[3] = 1.0f;
        que.btlid = atk + 0x100;
        que.kind = sh2_attack_list[atk].kind;
        que.sc = attacker;
        if (attacker->battle.prev_atk_pos[3]) {
            clBattleAddQue(&que);
        }
    }
    sceVu0CopyVector(attacker->battle.prev_atk_pos, que.eve);
}

static void shGetEnemyAttackStartPos(struct SubCharacter *attacker, unsigned short atk, float *s_pos, float *s_vec) {
    switch (attacker->kind) {
        case 0x200:
            shGetEnemySCUAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x201:
            shGetEnemyMKNAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x207:
        case 0x20B:
            shGetEnemyNSEAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x208:
            shGetEnemyREDAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x209:
            shGetEnemyONIAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x205:
            shGetEnemyEDBAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x204:
            shGetEnemyPAPAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x202:
            shGetEnemyTYUAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x203:
            shGetEnemyIKEAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x206:
            shGetEnemyBOSAttackPos(attacker, s_pos, s_vec, atk);
            break;
        case 0x20A:
            shGetEnemyARMAttackPos(attacker, s_pos, s_vec, 0);
            shGetEnemyARMAttackPos(attacker, s_pos, s_vec, 1);
            break;
    }
}

static void shBattleAttackByEnemySlash(struct SubCharacter *attacker, unsigned short atk) {
    float s_pos[4];
    float s_vec[4];
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    struct _CL_BATTLE_QUE que;

    cur_frame = shCharacterAnimeFrameGet(attacker);
    st = sh2_attack_list[atk].atk_start;
    ed = sh2_attack_list[atk].atk_end;
    shGetEnemyAttackStartPos(attacker, atk, s_pos, s_vec);
    que.eve[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].max_range;
    que.eve[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].max_range;
    que.eve[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].max_range;
    que.eve[3] = 1.0f;
    if (cur_frame >= st && cur_frame <= ed) {
        que.evs[0] = que.svs[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].min_range;
        que.evs[1] = que.svs[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].min_range;
        que.evs[2] = que.svs[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].min_range;
        que.svs[3] = que.evs[3] = 1.0f;
        que.sve[0] = attacker->battle.prev_atk_pos[0];
        que.sve[1] = attacker->battle.prev_atk_pos[1];
        que.sve[2] = attacker->battle.prev_atk_pos[2];
        que.sve[3] = 1.0f;
        que.btlid = atk + 0x100;
        que.kind = sh2_attack_list[atk].kind;
        que.sc = attacker;
        clBattleAddQue(&que);
        if (!attacker->battle.se) {
            switch (atk) {
                case 0x26:
                case 0x27:
                    break;
                case 0x2C:
                case 0x2D:
                    SeCallPos(0x3EEA, 0.7f, s_pos, 0);
                    break;
            }
            attacker->battle.se = 1;
        }
    }
    sceVu0CopyVector(attacker->battle.prev_atk_pos, que.eve);
}

static void shBattleAttackByEnemyStrike(struct SubCharacter *attacker, unsigned short atk) {
    float s_pos[4];
    float s_vec[4];
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    struct _CL_BATTLE_QUE que;

    cur_frame = shCharacterAnimeFrameGet(attacker);
    st = sh2_attack_list[atk].atk_start;
    ed = sh2_attack_list[atk].atk_end;
    shGetEnemyAttackStartPos(attacker, atk, s_pos, s_vec);
    que.eve[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].max_range;
    que.eve[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].max_range;
    que.eve[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].max_range;
    que.eve[3] = 1.0f;
    if (cur_frame >= st && cur_frame <= ed && !attacker->battle.atk_result) {
        que.evs[0] = que.svs[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].min_range;
        que.evs[1] = que.svs[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].min_range;
        que.evs[2] = que.svs[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].min_range;
        que.evs[3] = que.svs[3] = 1.0f;
        que.sve[0] = attacker->battle.prev_atk_pos[0];
        que.sve[1] = attacker->battle.prev_atk_pos[1];
        que.sve[2] = attacker->battle.prev_atk_pos[2];
        que.sve[3] = 1.0f;
        que.btlid = atk + 0x100;
        que.kind = sh2_attack_list[atk].kind;
        que.sc = attacker;
        if (attacker->battle.prev_atk_pos[3]) {
            clBattleAddQue(&que);
        }
        if (!attacker->battle.se) {
            switch (atk) {
                case 0x26:
                case 0x27:
                    break;
                case 0x28:
                case 0x2A:
                    SeCallPos(0x2F46, 0.7f, s_pos, 0);
                    break;
                case 0x29:
                case 0x2B:
                    SeCallPos(0x2F45, 0.7f, s_pos, 0);
                    break;
                case 0x31:
                    SeCallPos(0x3EEC, 0.7f, s_pos, 0);
                    break;
                case 0x32:
                    SeCallPos(0x3EED, 0.7f, s_pos, 0);
                    break;
                case 0x38:
                    SeCallPos(0x4844, 1.0f, s_pos, 0);
                    break;
                case 0x36:
                    SeCallPos(0x4848 + ((shRandI() >> 10) & 1), 1.0f, s_pos, 0);
                    break;
                case 0x3B:
                    SeCallPos(0x49A4 + ((shRandI() >> 10) & 3), 0.7f, s_pos, 0);
                    break;
                case 0x3D:
                    SeCallPos(0x49A2 + ((shRandI() >> 10) & 1), 0.7f, s_pos, 0);
                    break;
            }
            attacker->battle.se = 1;
        }
    }
    sceVu0CopyVector(attacker->battle.prev_atk_pos, que.eve);
}

static void shBattleAttackByEnemyFog(struct SubCharacter *attacker, unsigned short atk) {
    int i;
    float s_pos[4];
    float s_vec[4];
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    float max_range;
    struct _CL_BATTLE_QUE que;

    cur_frame = shCharacterAnimeFrameGet(attacker);
    st = sh2_attack_list[atk].atk_start;
    ed = sh2_attack_list[atk].atk_end;
    if (cur_frame >= st && cur_frame <= ed && !attacker->battle.atk_result) {
        max_range = (cur_frame - st) * (500.0f * (BgIsOut(0) ? 3.6f : 1.8f)) / (ed - st);
        for (i = 0; i < 5; i++) {
            shGetEnemyAttackStartPos(attacker, i, s_pos, s_vec);
            que.svs[0] = s_pos[0];
            que.svs[1] = s_pos[1];
            que.svs[2] = s_pos[2];
            que.sve[0] = s_pos[0] + s_vec[0] * max_range;
            que.sve[1] = s_pos[1] + s_vec[1] * max_range;
            que.sve[2] = s_pos[2] + s_vec[2] * max_range;
            que.svs[3] = que.sve[3] = 1.0f;
            que.btlid = atk + 0x100;
            que.kind = sh2_attack_list[atk].kind;
            que.sc = attacker;
            clBattleAddQue(&que);
        }
        if (!attacker->battle.se) {
            SeCallPos(0x2EE0, 0.7f, s_pos, 0);
            attacker->battle.se = 1;
        }
    }
}

/* Matching: K&R definition; called with arguments it ignores (the DWARF shows none). */
static void shBattleAttackByEnemyBite() {
}

static void shBattleAttackByEnemyHug(struct SubCharacter *attacker, unsigned short atk) {
    float s_pos[4];
    float s_vec[4];
    struct _CL_BATTLE_QUE que;

    shGetEnemyAttackStartPos(attacker, atk, s_pos, s_vec);
    que.eve[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].max_range;
    que.eve[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].max_range;
    que.eve[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].max_range;
    que.eve[3] = 1.0f;
    que.evs[0] = que.svs[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].min_range;
    que.evs[1] = que.svs[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].min_range;
    que.evs[2] = que.svs[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].min_range;
    que.evs[3] = que.svs[3] = 1.0f;
    que.sve[0] = attacker->battle.prev_atk_pos[0];
    que.sve[1] = attacker->battle.prev_atk_pos[1];
    que.sve[2] = attacker->battle.prev_atk_pos[2];
    que.sve[3] = 1.0f;
    que.btlid = atk + 0x100;
    que.kind = sh2_attack_list[atk].kind;
    que.sc = attacker;
    if (attacker->battle.prev_atk_pos[3]) {
        clBattleAddQue(&que);
    }
    sceVu0CopyVector(attacker->battle.prev_atk_pos, que.eve);
}

static void shBattleAttackByEnemyNeedle(struct SubCharacter *attacker, unsigned short atk) {
    int i;
    float s_pos[4];
    float s_vec[4];
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    struct _CL_BATTLE_QUE que;

    cur_frame = shCharacterAnimeFrameGet(attacker);
    st = sh2_attack_list[atk].atk_start;
    ed = sh2_attack_list[atk].atk_end;
    for (i = 0; i < 2; i++) {
        shGetEnemyAttackStartPos(attacker, i, s_pos, s_vec);
        que.sve[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].max_range;
        que.sve[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].max_range;
        que.sve[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].max_range;
        que.sve[3] = 1.0f;
        que.svs[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].min_range;
        que.svs[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].min_range;
        que.svs[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].min_range;
        que.svs[3] = 1.0f;
        que.btlid = atk + 0x100;
        que.kind = sh2_attack_list[atk].kind;
        que.sc = attacker;
        clBattleAddQue(&que);
        if (!attacker->battle.se) {
            SeCallPos(0x2FDC + ((shRandI() >> 10) & 1), 0.8f, s_pos, 0);
            attacker->battle.se = 1;
        }
    }
}

static void shBattleAttackByEnemyShot(struct SubCharacter *attacker, unsigned short atk) {
    float s_pos[4];
    float s_vec[4];
    unsigned short cur_frame;
    unsigned short st;
    unsigned short ed;
    struct _CL_BATTLE_QUE que;

    cur_frame = shCharacterAnimeFrameGet(attacker);
    st = sh2_attack_list[atk].atk_start;
    ed = sh2_attack_list[atk].atk_end;
    if (cur_frame >= st && cur_frame <= ed && !attacker->battle.atk_result) {
        shGetEnemyAttackStartPos(attacker, atk, s_pos, s_vec);
        que.svs[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].min_range;
        que.svs[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].min_range;
        que.svs[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].min_range;
        que.svs[3] = 1.0f;
        que.sve[0] = s_pos[0] + s_vec[0] * sh2_attack_list[atk].max_range;
        que.sve[1] = s_pos[1] + s_vec[1] * sh2_attack_list[atk].max_range;
        que.sve[2] = s_pos[2] + s_vec[2] * sh2_attack_list[atk].max_range;
        que.sve[3] = 1.0f;
        que.btlid = atk + 0x100;
        que.kind = sh2_attack_list[atk].kind;
        que.sc = attacker;
        clBattleAddQue(&que);
        attacker->battle.atk_result = 1;
        if (attacker->kind == 0x205) {
            enEDBSetGunFire(attacker->enemy_p);
            SeCallPos(0x4719, 1.0f, s_pos, 1);
        }
    }
    if (cur_frame > ed) {
        attacker->battle.atk_result = 0;
    }
}

/* Matching: #line puts the asserts below on their original source lines. */
#line 1686
static void shBattleAddAttackQueue(struct SubCharacter *scp, unsigned char wep_no, unsigned short atk_no) {
    int no;

    if (!sh2_attack_queue.rest) {
        assert(0);
    }
    no = 20 - sh2_attack_queue.rest;
    sh2_attack_queue.queue[no].scp = scp;
    sh2_attack_queue.queue[no].wep_no = wep_no;
    sh2_attack_queue.queue[no].atk_no = atk_no;
    sh2_attack_queue.rest--;
}

/** Clears @p scp's attack state before a new attack. */
void shBattleAttackHitCheckInit(struct SubCharacter *scp) {
    scp->battle.se = 0;
    scp->battle.prev_atk_pos[3] = 0.0f;
    scp->battle.atk_result = 0;
}

/** Queues attack @p atk_no with weapon @p wep_no by a human against the enemies. */
void shBattleAttackHitCheckToEnemy(struct SubCharacter *scp, unsigned char wep_no, unsigned short atk_no) {
    shBattleAddAttackQueue(scp, wep_no, atk_no);
}

/** Queues attack @p atk_no by an enemy against the humans. */
void shBattleAttackHitCheckToHuman(struct SubCharacter *scp, unsigned short atk_no) {
    shBattleAddAttackQueue(scp, 0, atk_no);
}

/** Returns 1 if animation frame @p frame of attack @p atk is inside the window for chaining the next attack. */
int shBattleRequestNextAttackIsOk(unsigned short atk, unsigned short frame) {
    switch (atk) {
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            return 0;
        case 1:
        case 2:
            if (frame >= sh2_attack_list[atk].atk_start + 9 && frame <= sh2_attack_list[atk].atk_end + 9) {
                return 1;
            }
            return 0;
        case 8:
        case 9:
        case 10:
        case 11:
            if (frame >= sh2_attack_list[atk].atk_end - 2 && frame <= sh2_attack_list[atk].atk_end) {
                return 1;
            }
            return 0;
        case 12:
        case 13:
        case 15:
        case 16:
            if ((frame >= sh2_attack_list[atk].atk_end + 2 && frame <= sh2_attack_list[atk].atk_end + 4) || shCharacterAnimeSpeedGet_(sh2jms.player, 1) < 0) {
                return 1;
            }
            return 0;
        case 14:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
            return 0;
        default:
            if (frame >= sh2_attack_list[atk].atk_start && frame <= sh2_attack_list[atk].atk_end) {
                return 1;
            }
            return 0;
    }
}

/** Applies this frame's battle results for @p scp: hits it landed, wall/floor hits, and damage and shock it took. */
void shBattleGetResult(struct SubCharacter *scp) {
    struct _CL_BATTLE_RESULT *result;
    float damage_revise;
    float shock_revise;

    result = NULL;
    while ((result = clBattleGetResult((unsigned int)scp, result))->atr) {
        switch (result->atr) {
            case 1:
                if (result->btlid & 0xFF00) {
                    scp->battle.atk_result = result->atr;
                }
                scp->battle.target = result->obj.en;
                if (scp->kind <= 0x101 && sh2jms.attack_no <= 0x18) {
                    switch (sh2jms.weapon) {
                        case 5:
                        case 6:
                        case 8:
                        case 7:
                            sh2jms.d_shock = 1;
                            break;
                    }
                }
                break;
            case 2:
            case 3:
                if (!sh2jms.wep_no_hit_floor) {
                    if (scp->kind <= 0x101 && sh2jms.attack_no <= 0x18) {
                        switch (sh2jms.weapon) {
                            case 5:
                            case 6:
                            case 8:
                            case 7:
                                sh2jms.d_shock = 4;
                                break;
                        }
                    }
                    scp->battle.atk_result = result->atr;
                }
                if (scp->kind <= 0x101) {
                    if (result->obj.pl->pad) {
                        sh2_battle_wall_hit = sh2_attack_list[result->btlid & 0xFF].ap;
                    } else {
                        sh2_battle_wall_hit = 0.0f;
                    }
                }
                break;
            case 4:
                if (result->btlid & 0xFF00) {
                    if (scp->kind <= 0x12D && (result->btlid & 0xFF) >= 0x19 && (result->btlid & 0xFF) < 0x23) {
                        break;
                    }
                    shBattleDamageRevise(&damage_revise, &shock_revise, scp, result);
                    if (!(damage_revise < 0.0f)) {
                        scp->battle.damage += damage_revise;
                    }
                    if (scp->battle.shock <= shock_revise && !scp->battle.id) {
                        scp->battle.shock = shock_revise;
                        sceVu0CopyVector(scp->battle.pos, result->pos);
                        sceVu0CopyVector(scp->battle.vec, result->vec);
                        scp->battle.id = result->btlid & 0xFF;
                        scp->battle.kind = result->kind;
                        scp->battle.target = result->obj.en;
                    }
                    if (scp->kind > 0x101 || (!PlayerChectGuardSuccess() && !shBattleNoDamageHuman())) {
                        if (damage_revise > 0.0f) {
                            shBattleSetEffectDamage(scp, result->pos, result->vec, result->btlid & 0xFF);
                        }
                        shBattleSetSoundDamage(scp, result);
                    }
                }
                break;
            default:
                scp->battle.atk_result = result->atr;
                break;
        }
    }
}

/** Clears the attack queue and the wall-hit value. */
void shBattleInitAttackQueue(void) {
    shQzero(&sh2_attack_queue, sizeof(sh2_attack_queue));
    sh2_attack_queue.rest = 20;
    sh2_battle_wall_hit = 0.0f;
}

/** Runs the queued attacks through the collision checks. */
/* Matching: #line puts the asserts below on their original source lines. */
#line 2031
void shBattleExecAttackQueue(void) {
    int i;

    i = 0;
    if (sh2_attack_queue.rest == 20) {
        sh2_battle_attack_check = 0;
    } else {
        sh2_battle_attack_check = 1;
    }
    for (; sh2_attack_queue.queue[i].scp; i++) {
        if (sh2_attack_queue.queue[i].atk_no >= 0x24) {
            switch (sh2_attack_list[sh2_attack_queue.queue[i].atk_no].kind) {
                case 1:
                    shBattleAttackByEnemySlash(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                    break;
                case 2:
                    shBattleAttackByEnemyStrike(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                    break;
                case 4:
                    shBattleAttackByEnemyFog(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                    break;
                case 5:
                    shBattleAttackByEnemyBite(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                    break;
                case 6:
                    shBattleAttackByEnemyHug(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                    break;
                case 3:
                    switch (sh2_attack_queue.queue[i].atk_no) {
                        case 0x34:
                        case 0x33:
                            shBattleAttackByEnemyShot(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                            break;
                        case 0x29:
                        case 0x2B:
                        case 0x31:
                            shBattleAttackByEnemyStrike(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                            break;
                        case 0x40:
                        case 0x41:
                            shBattleAttackByEnemyNeedle(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                            break;
                    }
                    break;
            }
        } else if (sh2_attack_queue.queue[i].atk_no >= 0x19) {
            shBattleAttackByHumanFinish(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
        } else {
            switch (sh2_attack_queue.queue[i].wep_no) {
                case 0:
                    assert(0);
                case 1:
                case 3:
                    shBattleAttackByHumanGunshotTypeA(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                    break;
                case 2:
                    shBattleAttackByHumanGunshotTypeB(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                    break;
                case 5:
                case 6:
                case 7:
                case 8:
                    shBattleAttackByHumanFightType(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                    break;
                case 4:
                    shBattleAttackByHumanFog(sh2_attack_queue.queue[i].scp, sh2_attack_queue.queue[i].atk_no);
                    break;
            }
        }
    }
}

/** Returns James's HP. */
float shBattleGetJamesHP(void) {
    return sh2jms.player->battle.hp;
}

/** Returns James's HP rate. */
float shBattleGetJamesHP_Rate(void) {
    return sh2jms.player->battle.hp_rate;
}

/** Deals @p damage of attack @p id to James from direction @p vec. */
void shBattleSetJamesDamage(unsigned short id, float damage, float *vec) {
    sh2jms.player->battle.damage = damage;
    sh2jms.player->battle.id = id;
    vcopy_gcc(sh2jms.player->battle.vec, vec);
}

/** Returns the attack power of James's last hit on a flagged wall (0 if none). */
float shBattleEventWallHitCheck(void) {
    return sh2_battle_wall_hit;
}

/** Returns 1 if the last run of the attack queue had any attacks in it. */
int shBattleCheckAttackByEnemy(void) {
    return sh2_battle_attack_check;
}
