/*
 * ARM enemy character (model 0x20A): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * The Mandarin (suspected; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static const struct _AnimeInfo arm_anim[14] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1483, 0x0031, 0x0800, 0x0000, 0x0030, 1 },
    { 0x1484, 0x0011, 0x0800, 0x0031, 0x0041, 0 },
    { 0x1485, 0x0011, 0x0800, 0x0042, 0x0052, 0 },
    { 0x1486, 0x0011, 0x0800, 0x0053, 0x0063, 0 },
    { 0x1487, 0x0011, 0x0800, 0x0064, 0x0074, 0 },
    { 0x1488, 0x0010, 0x0800, 0x0075, 0x0084, 0 },
    { 0x1489, 0x0010, 0x0800, 0x0085, 0x0094, 0 },
    { 0x148A, 0x001F, 0x0800, 0x0095, 0x00B3, 0 },
    { 0x148B, 0x001F, 0x0800, 0x00B4, 0x00D2, 0 },
    { 0x148C, 0x001F, 0x0800, 0x00D3, 0x00F1, 0 },
    { 0x148D, 0x001F, 0x0800, 0x00F2, 0x0110, 0 },
    { 0x148E, 0x0042, 0x0800, 0x0111, 0x0152, 0 },
    { 0x148F, 0x0042, 0x0800, 0x0153, 0x0194, 0 },
};

/*
 * Matching: the DWARF drops this unused parameter. Passing `this` keeps it live in a0 up
 * to the call (MWCC knows this static callee clobbers nothing else), so the switch on
 * this->step is loaded into a1.
 */
static int EnemyARMInit(struct SubCharacter *scp) {
    return 0;
}

static void EnemyARMFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        if (this->battle.status & 0x400) {
            vcopy(&this->pos, pos);
            vcopy(&this->rot, rot);
            EnemyARMInit(this);
            if (!PlayerNowDemoEventMode()) {
                SCAnimeTypeSwitch(this, 1);
                shCharacterEnemyARMAnimeSetP(this, 0x1483, 1);
                if (this->status & 4) {
                    if ((dp = enEntryEnemy(10)) != NULL) {
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

/** Installs the ARM update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyARMLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyARMFunction);
}

/**
 * Plays gameplay animation @p anime_id on a ARM model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a ARM model.
 */
int shCharacterEnemyARMAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x20A) {
        aip = (struct _AnimeInfo *)&arm_anim[anime_id - 0x1482];
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
void shGetEnemyARMAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
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
    case 0:
        sk_num = 13;
        break;
    case 1:
        sk_num = 14;
        break;
    }
    i = 0;
    while (i < sk_num) {
        stp = stp->next;
        i++;
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
