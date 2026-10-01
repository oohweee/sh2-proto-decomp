/*
 * PAP enemy character (model 0x204): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * Abstract Daddy (verified; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static int dpap_anime_adr_list[8] = { 0, 0, 0, 0x82C8, 0x16CE0, 0x26674, 0x32374, 0x3F524 };

static const struct _AnimeInfo pap_anim[29] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1645, 0x0016, 0x0800, 0x0000, 0x0015, 1 },
    { 0x1646, 0x001C, 0x0800, 0x0016, 0x0031, 1 },
    { 0x1647, 0x0014, 0x0800, 0x0032, 0x0045, 0 },
    { 0x1648, 0x0018, 0x0800, 0x0046, 0x005D, 1 },
    { 0x1649, 0x0014, 0x0800, 0x005E, 0x0071, 0 },
    { 0x164A, 0x000D, 0x0800, 0x0072, 0x007E, 0 },
    { 0x164B, 0x001D, 0x0800, 0x007F, 0x009B, 0 },
    { 0x164C, 0x001D, 0x0800, 0x009C, 0x00B8, 0 },
    { 0x164D, 0x0012, 0x0800, 0x00B9, 0x00CA, 0 },
    { 0x164E, 0x0012, 0x0800, 0x00CB, 0x00DC, 0 },
    { 0x164F, 0x000F, 0x0800, 0x00DD, 0x00EB, 0 },
    { 0x1650, 0x001D, 0x0800, 0x00EC, 0x0108, 0 },
    { 0x1651, 0x001D, 0x0800, 0x0109, 0x0125, 0 },
    { 0x1652, 0x0020, 0x0800, 0x0126, 0x0145, 0 },
    { 0x1653, 0x0012, 0x0800, 0x0146, 0x0157, 0 },
    { 0x1654, 0x0015, 0x0800, 0x0158, 0x016C, 0 },
    { 0x1655, 0x0020, 0x0800, 0x016D, 0x018C, 0 },
    { 0x1656, 0x0020, 0x0800, 0x018D, 0x01AC, 0 },
    { 0x1657, 0x0025, 0x0800, 0x01AD, 0x01D1, 0 },
    { 0x1658, 0x001D, 0x0800, 0x01D2, 0x01EE, 0 },
    { 0x1659, 0x001B, 0x0800, 0x01EF, 0x0209, 0 },
    { 0x165A, 0x0021, 0x0800, 0x020A, 0x022A, 0 },
    { 0x165B, 0x0012, 0x0800, 0x022B, 0x023C, 1 },
    { 0x165C, 0x001F, 0x0800, 0x023D, 0x025B, 0 },
    { 0x165D, 0x0012, 0x0800, 0x025C, 0x026D, 0 },
    { 0x165E, 0x0014, 0x0800, 0x026E, 0x0281, 0 },
    { 0x165F, 0x0018, 0x0800, 0x0282, 0x0299, 1 },
    { 0x1660, 0x0014, 0x0800, 0x029A, 0x02AD, 0 },
};

static const struct _AnimeInfo d_pap_anim[8] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x19C9, 0x002A, 0x0800, 0x0000, 0x0029, 1 },
    { 0x19CA, 0x01B6, 0x0800, 0x0000, 0x01B5, 0 },
    { 0x19CB, 0x031F, 0x0800, 0x0000, 0x031E, 0 },
    { 0x19CC, 0x0352, 0x0800, 0x0000, 0x0351, 0 },
    { 0x19CD, 0x0283, 0x0800, 0x0000, 0x0282, 0 },
    { 0x19CE, 0x02C9, 0x0800, 0x0000, 0x02C8, 0 },
    { 0x19CF, 0x0483, 0x0800, 0x0000, 0x0482, 0 },
};

static int EnemyPAPInit(struct SubCharacter *scp) {
    scp->kind = 0x204;
    return 0;
}

static void EnemyPAPFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        if (this->battle.status & 0x400) {
            vcopy(&this->pos, pos);
            vcopy(&this->rot, rot);
            EnemyPAPInit(this);
            if (!PlayerNowDemoEventMode()) {
                SCAnimeTypeSwitch(this, 1);
                shCharacterEnemyPAPAnimeSetP(this, 0x1645, 1);
                if (this->status & 4) {
                    if ((dp = enEntryEnemy(8)) != NULL) {
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
    case 2:
        break;
    }
}

/** Installs the PAP update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyPAPLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyPAPFunction);
}

/**
 * Plays drama (event) animation @p anime_id on a PAP model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a PAP model.
 */
int shCharacterEnemyPAPAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x204) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_pap_anim[anime_id - 0x19C8];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dpap_anime_adr_list[anime_id - 0x19C8] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x19C8));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a PAP model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a PAP model.
 */
int shCharacterEnemyPAPAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x204) {
        aip = (struct _AnimeInfo *)&pap_anim[anime_id - 0x1644];
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
void shGetEnemyPAPAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
    float pos0[4];
    float pos1[4];
    float vec[4];
    struct FMAT lw_mat;
    struct FMAT mat;
    int i;
    int sk_num;
    float wep_range[2][4] = { { 293.0f, 226.0f, 0.0f, 0.0f }, { -293.0f, 226.0f, 0.0f, 0.0f } };
    struct shSkelton *stp;

    lw_mat = scp->mat;
    stp = scp->sk_top;
    sk_num = 14;
    for (i = 0; i < sk_num; i++) {
        stp = stp->next;
    }
    mat = stp->src_m;
    pos0[0] = stp->src_m.d[3][0];
    pos0[1] = stp->src_m.d[3][1];
    pos0[2] = stp->src_m.d[3][2];
    pos0[3] = stp->src_m.d[3][3];
    mat.d[3][0] = mat.d[3][1] = mat.d[3][2] = 0.0f;
    mat.d[3][3] = 1.0f;
    sceVu0ApplyMatrix(pos0, (float (*)[4])&lw_mat, pos0);
    vcopy(pos0, s_pos);
    if (atk == 0x39) {
        sceVu0ApplyMatrix(pos1, (float (*)[4])&mat, wep_range[0]);
    } else {
        sceVu0ApplyMatrix(pos1, (float (*)[4])&mat, wep_range[1]);
    }
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
