/*
 * MKN enemy character (model 0x201): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * The Mannequin (verified; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static int dmkn_anime_adr_list[8] = { 0, 0, 0, 0x6974, 0xB348, 0xDD60, 0x14650, 0x17068 };

static const struct _AnimeInfo mkn_anim[35] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x13ED, 0x0018, 0x0800, 0x0000, 0x0017, 1 },
    { 0x13EE, 0x001F, 0x0800, 0x0018, 0x0036, 1 },
    { 0x13EF, 0x0023, 0x0800, 0x0037, 0x0059, 0 },
    { 0x13F0, 0x0023, 0x0800, 0x005A, 0x007C, 0 },
    { 0x13F1, 0x0012, 0x0800, 0x007D, 0x008E, 0 },
    { 0x13F2, 0x0019, 0x0800, 0x008F, 0x00A7, 0 },
    { 0x13F3, 0x0019, 0x0800, 0x00A8, 0x00C0, 0 },
    { 0x13F4, 0x0014, 0x0800, 0x00C1, 0x00D4, 0 },
    { 0x13F5, 0x0014, 0x0800, 0x00D5, 0x00E8, 0 },
    { 0x13F6, 0x001B, 0x0800, 0x00E9, 0x0103, 0 },
    { 0x13F7, 0x0020, 0x0800, 0x0104, 0x0123, 0 },
    { 0x13F8, 0x0020, 0x0800, 0x0124, 0x0143, 0 },
    { 0x13F9, 0x0010, 0x05DC, 0x0144, 0x0153, 0 },
    { 0x13FA, 0x0011, 0x05DC, 0x0154, 0x0164, 0 },
    { 0x13FB, 0x0010, 0x05DC, 0x0165, 0x0174, 0 },
    { 0x13FC, 0x001E, 0x0708, 0x0175, 0x0192, 0 },
    { 0x13FD, 0x001E, 0x0708, 0x0193, 0x01B0, 0 },
    { 0x13FE, 0x000A, 0x0640, 0x01B1, 0x01BA, 0 },
    { 0x13FF, 0x000C, 0x0800, 0x01BB, 0x01C6, 0 },
    { 0x1400, 0x0010, 0x0800, 0x01C7, 0x01D6, 0 },
    { 0x1401, 0x002C, 0x0800, 0x01D7, 0x0202, 0 },
    { 0x1402, 0x0032, 0x0800, 0x0203, 0x0234, 0 },
    { 0x1403, 0x0027, 0x0800, 0x0235, 0x025B, 0 },
    { 0x1404, 0x002A, 0x0800, 0x025C, 0x0285, 0 },
    { 0x1405, 0x0010, 0x0800, 0x0286, 0x0295, 1 },
    { 0x1406, 0x0010, 0x0800, 0x0296, 0x02A5, 1 },
    { 0x1407, 0x0008, 0x0800, 0x02A6, 0x02AD, 1 },
    { 0x1408, 0x0008, 0x0800, 0x02AE, 0x02B5, 1 },
    { 0x1409, 0x000F, 0x0800, 0x02B6, 0x02C4, 0 },
    { 0x140A, 0x0007, 0x0800, 0x02C5, 0x02CB, 0 },
    { 0x140B, 0x0002, 0x0800, 0x02CC, 0x02CD, 0 },
    { 0x140C, 0x0002, 0x0800, 0x02CE, 0x02CF, 0 },
    { 0x140D, 0x0001, 0x0800, 0x0163, 0x0163, 0 },
    { 0x140E, 0x0001, 0x0800, 0x0173, 0x0173, 0 },
};

static const struct _AnimeInfo d_mkn_anim[8] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x17D5, 0x0001, 0x0800, 0x0000, 0x0000, 1 },
    { 0x17D6, 0x0168, 0x0800, 0x0000, 0x0167, 1 },
    { 0x17D7, 0x0538, 0x0800, 0x0000, 0x0537, 1 },
    { 0x17D8, 0x0370, 0x0800, 0x0000, 0x036F, 1 },
    { 0x17D9, 0x0168, 0x0800, 0x0000, 0x0167, 1 },
    { 0x17DA, 0x0538, 0x0800, 0x0000, 0x0537, 1 },
    { 0x17DB, 0x0370, 0x0800, 0x0000, 0x036F, 1 },
};

/*
 * Matching: the DWARF drops this unused parameter. Passing `this` keeps it live in a0 up
 * to the call (MWCC knows this static callee clobbers nothing else), so the switch on
 * this->step is loaded into a1.
 */
static int EnemyMKNInit(struct SubCharacter *scp) {
    return 0;
}

static void EnemyMKNFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        if (this->battle.status & 0x400) {
            vcopy(&this->pos, pos);
            vcopy(&this->rot, rot);
            EnemyMKNInit(this);
            if (!PlayerNowDemoEventMode()) {
                SCAnimeTypeSwitch(this, 1);
                shCharacterEnemyMKNAnimeSetP(this, 0x13EE, 1);
                if (this->status & 4) {
                    if ((dp = enEntryEnemy(2)) != NULL) {
                        this->enemy_p = dp;
                        enInitData(dp, this);
                    } else {
                        this->enemy_p = NULL;
                    }
                }
            }
            vcopy(pos, &this->pos);
            vcopy(rot, &this->rot);
            this->step++;
        }
    case 1:
        break;
    }
}

/** Installs the MKN update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyMKNLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyMKNFunction);
}

/**
 * Plays drama (event) animation @p anime_id on a MKN model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a MKN model.
 */
int shCharacterEnemyMKNAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x201) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_mkn_anim[anime_id - 0x17D4];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dmkn_anime_adr_list[anime_id - 0x17D4] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x17D4));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a MKN model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a MKN model.
 */
int shCharacterEnemyMKNAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x201) {
        aip = (struct _AnimeInfo *)&mkn_anim[anime_id - 0x13EC];
        shCharacterAnimeSet(scp, 0, (char)(comp == 1 ? 4 : 2), aip, (int)shCharacterGetAnimeAdrForPlay(scp));
        return 0;
    }
    return -1;
}

/**
 * Gets where attack @p atk starts and which way it points, from the attacking bone.
 * @param scp the enemy. @param s_pos out: world position. @param s_vec out: unit direction.
 * @param atk attack (battle) ID.
 */
void shGetEnemyMKNAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
    float pos0[4];
    float pos1[4];
    float vec[4];
    struct FMAT lw_mat;
    struct FMAT mat;
    int i;
    int sk_num;
    float wep_range[4] = { 500.0f, 0.0f, 0.0f, 0.0f };
    struct shSkelton *stp;

    lw_mat = scp->mat;
    stp = scp->sk_top;
    switch (atk) {
    case 0x26:
        sk_num = 7;
        break;
    case 0x27:
        sk_num = 5;
        break;
    }
    for (i = 0; i < sk_num; i++) {
        stp = stp->next;
    }
    mat = stp->src_m;
    pos0[0] = stp->src_m.d[3][0];
    pos0[1] = stp->src_m.d[3][1];
    pos0[2] = stp->src_m.d[3][2];
    pos0[3] = stp->src_m.d[3][3];
    mat.d[3][0] = 0.0f;
    mat.d[3][3] = 1.0f;
    sceVu0ApplyMatrix(pos0, (float (*)[4])&lw_mat, pos0);
    vcopy(pos0, s_pos);
    sceVu0ApplyMatrix(pos1, (float (*)[4])&mat, wep_range);
    pos1[0] += stp->src_m.d[3][0];
    pos1[1] += stp->src_m.d[3][1];
    pos1[2] += stp->src_m.d[3][2];
    pos1[3] = stp->src_m.d[3][3];
    sceVu0ApplyMatrix(pos1, (float (*)[4])&lw_mat, pos1);
    vec[0] = pos1[0] - pos0[0];
    vec[1] = pos1[1] - pos0[1];
    vec[2] = pos1[2] - pos0[2];
    vec[3] = 0.0f;
    _shNormalize(s_vec, vec);
}

/**
 * Spawns a ground-impact effect at one of the MKN's feet.
 * @param scp the enemy. @param type 0 for one foot bone, non-zero for the other (also passed
 *        on as the effect type).
 */
void shEnemyMKN_EffectFoot(struct SubCharacter *scp, int type) {
    float mat[4][4];
    int i1;
    int foot;
    struct shSkelton *sk;

    foot = type ? 14 : 13;
    sk = scp->sk_top;
    for (i1 = 0; i1 < foot; i1++) {
        sk = sk->next;
    }
    sceVu0MulMatrix(mat, (float (*)[4])&scp->mat, (float (*)[4])&sk->src_m);
    HH_Effect_Object_Ground_Impact_Post_forEnemy(mat[3], type);
}
