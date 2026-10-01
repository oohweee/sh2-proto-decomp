/*
 * IKE enemy character (model 0x203): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * The Flesh Lip (verified; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static int dike_anime_adr_list[12] = { 0, 0, 0, 0x1918, 0x2850, 0xB3BC, 0xC060, 0xFCD4, 0x115EC, 0x1199C, 0x12E6C, 0x13044 };

static const struct _AnimeInfo ike_anim[21] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x15E1, 0x0017, 0x0800, 0x0000, 0x0016, 1 },
    { 0x15E2, 0x0020, 0x0800, 0x0017, 0x0036, 0 },
    { 0x15E3, 0x0010, 0x0400, 0x0037, 0x0046, 0 },
    { 0x15E4, 0x0010, 0x0800, 0x0047, 0x0056, 0 },
    { 0x15E5, 0x0018, 0x0800, 0x0057, 0x006E, 1 },
    { 0x15E6, 0x000F, 0x0800, 0x006F, 0x007D, 0 },
    { 0x15E7, 0x0013, 0x0800, 0x007E, 0x0090, 0 },
    { 0x15E8, 0x0013, 0x0800, 0x0091, 0x00A3, 0 },
    { 0x15E9, 0x0013, 0x0800, 0x00A4, 0x00B6, 0 },
    { 0x15EA, 0x0013, 0x0800, 0x00B7, 0x00C9, 0 },
    { 0x15EB, 0x0021, 0x0800, 0x00CA, 0x00EA, 0 },
    { 0x15EC, 0x0021, 0x0800, 0x00EB, 0x010B, 0 },
    { 0x15ED, 0x0021, 0x0800, 0x010C, 0x012C, 0 },
    { 0x15EE, 0x0021, 0x0800, 0x012D, 0x014D, 0 },
    { 0x15EF, 0x0021, 0x0800, 0x014E, 0x016E, 0 },
    { 0x15F0, 0x0021, 0x0800, 0x016F, 0x018F, 0 },
    { 0x15F1, 0x0021, 0x0800, 0x0190, 0x01B0, 0 },
    { 0x15F2, 0x0021, 0x0800, 0x01B1, 0x01D1, 0 },
    { 0x15F3, 0x001C, 0x0800, 0x01D2, 0x01ED, 0 },
    { 0x15F4, 0x0017, 0x0800, 0x01EE, 0x0204, 0 },
};

static const struct _AnimeInfo d_ike_anim[12] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1965, 0x002A, 0x0800, 0x0000, 0x0029, 1 },
    { 0x1966, 0x030A, 0x0800, 0x0000, 0x0309, 0 },
    { 0x1967, 0x005D, 0x0800, 0x0000, 0x005C, 0 },
    { 0x1968, 0x0281, 0x0800, 0x0000, 0x0280, 0 },
    { 0x1969, 0x0022, 0x0800, 0x0000, 0x0021, 0 },
    { 0x196A, 0x0295, 0x0800, 0x0000, 0x0294, 0 },
    { 0x196B, 0x030A, 0x0800, 0x0000, 0x0309, 0 },
    { 0x196C, 0x005D, 0x0800, 0x0000, 0x005C, 0 },
    { 0x196D, 0x0281, 0x0800, 0x0000, 0x0280, 0 },
    { 0x196E, 0x0022, 0x0800, 0x0000, 0x0022, 0 },
    { 0x196F, 0x0295, 0x0800, 0x0000, 0x0294, 0 },
};

static int EnemyIKEInit(struct SubCharacter *scp) {
    scp->kind = 0x203;
    return 0;
}

static void EnemyIKEFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        vcopy(&this->pos, pos);
        vcopy(&this->rot, rot);
        EnemyIKEInit(this);
        if (!PlayerNowDemoEventMode()) {
            SCAnimeTypeSwitch(this, 1);
            shCharacterEnemyIKEAnimeSetP(this, 0x15E1, 1);
            if (this->status & 4) {
                if ((dp = enEntryEnemy(7)) != NULL) {
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
    case 1:
        break;
    }
}

/** Installs the IKE update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyIKELow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyIKEFunction);
}

/**
 * Plays drama (event) animation @p anime_id on a IKE model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a IKE model.
 */
int shCharacterEnemyIKEAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x203) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_ike_anim[anime_id - 0x1964];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dike_anime_adr_list[anime_id - 0x1964] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x1964));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a IKE model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a IKE model.
 */
int shCharacterEnemyIKEAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if ((scp->status & 4) && shCharacterGetModelID(scp) == 0x203) {
        aip = (struct _AnimeInfo *)&ike_anim[anime_id - 0x15E0];
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
void shGetEnemyIKEAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
    float pos0[4];
    float pos1[4];
    float vec[4];
    struct FMAT lw_mat;
    struct FMAT mat;
    int i;
    int sk_num;
    float wep_range[4] = { 93.0f, 237.0f, 0.0f, 0.0f };
    struct shSkelton *stp;

    lw_mat = scp->mat;
    stp = scp->sk_top;
    sk_num = 5;
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
