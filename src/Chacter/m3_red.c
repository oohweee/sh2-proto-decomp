/*
 * RED enemy character (model 0x208): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * Pyramid Head, with the Great Knife (verified; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static int dred_anime_adr_list[7] = { 0, 0, 0, 0x10A90, 0x66C74, 0, 0 };

static const struct _AnimeInfo red_anim[12] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x14B5, 0x0001, 0x0800, 0x0000, 0x0000, 1 },
    { 0x14B6, 0x001E, 0x0500, 0x0000, 0x001D, 1 },
    { 0x14B7, 0x0043, 0x0800, 0x001E, 0x0060, 0 },
    { 0x14B8, 0x0078, 0x0800, 0x0061, 0x00D8, 0 },
    { 0x14B9, 0x001C, 0x0800, 0x00D9, 0x00F4, 0 },
    { 0x14BA, 0x000B, 0x0800, 0x00F5, 0x00FF, 0 },
    { 0x14BB, 0x000F, 0x0400, 0x0100, 0x010E, 0 },
    { 0x14BC, 0x0018, 0x0800, 0x010F, 0x0126, 1 },
    { 0x14BD, 0x000B, 0x0800, 0x0127, 0x0131, 0 },
    { 0x14BE, 0x0012, 0x0800, 0x0132, 0x0143, 1 },
    { 0x14BF, 0x0031, 0x0800, 0x0144, 0x0174, 1 },
};

static const struct _AnimeInfo d_red_anim[7] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x189D, 0x0001, 0x0800, 0x0000, 0x0000, 1 },
    { 0x189E, 0x0168, 0x0800, 0x0000, 0x0167, 1 },
    { 0x189F, 0x0538, 0x0800, 0x0000, 0x0538, 1 },
    { 0x18A0, 0x0370, 0x0800, 0x0000, 0x036F, 1 },
    { 0x18A1, 0x0146, 0x0800, 0x0000, 0x0145, 0 },
    { 0x18A2, 0x01D5, 0x0800, 0x0000, 0x01D4, 0 },
};

/*
 * Matching: the DWARF drops this unused parameter. Passing `this` keeps it live in a0 up
 * to the call (MWCC knows this static callee clobbers nothing else), so the switch on
 * this->step is loaded into a1.
 */
static int EnemyREDInit(struct SubCharacter *scp) {
    return 0;
}

static void EnemyREDFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        if (this->battle.status & 0x400) {
            vcopy(&this->pos, pos);
            vcopy(&this->rot, rot);
            EnemyREDInit(this);
            if (!PlayerNowDemoEventMode()) {
                SCAnimeTypeSwitch(this, 1);
                shCharacterEnemyREDAnimeSetP(this, 0x189D, 1);
                if (this->status & 4) {
                    if ((dp = enEntryEnemy(4)) != NULL) {
                        this->enemy_p = dp;
                        enInitData(dp, this);
                    } else {
                        this->enemy_p = NULL;
                    }
                }
                this->battle.target = shCharacterGetSubCharacter(0x100, -1);
            }
            vcopy(pos, &this->pos);
            vcopy(rot, &this->rot);
            this->step++;
        }
    case 1:
        break;
    }
}

/** Installs the RED update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyREDLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyREDFunction);
}

/** Shows or hides the RED's weapon (part 3 in sh2gfw_Set_JMSequip's last slot). @param on_off non-zero to show. */
void shCharacterSetWeaponRED(struct SubCharacter *scp, int on_off) {
    sh2gfw_Set_JMSequip(scp, 0, 0, on_off ? 3 : 0);
}

/**
 * Plays drama (event) animation @p anime_id on a RED model (deletes its enemy AI first).
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a RED model.
 */
int shCharacterEnemyREDAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x208) {
        enDeleteEnemy(scp->enemy_p);
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_red_anim[anime_id - 0x189C];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dred_anime_adr_list[anime_id - 0x189C] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x189C));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a RED model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a RED model.
 */
int shCharacterEnemyREDAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x208) {
        aip = (struct _AnimeInfo *)&red_anim[anime_id - 0x14B4];
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
void shGetEnemyREDAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
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
    case 0x2C:
    case 0x2D:
        sk_num = 36;
        break;
    case 0x2E:
    case 0x2F:
    case 0x30:
        sk_num = 25;
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
