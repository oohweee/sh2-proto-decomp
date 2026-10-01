/*
 * EDB enemy character (model 0x205): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * Eddie, boss fight (verified; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static int dedb_anime_adr_list[2] = { 0, 0x30394 };

static const struct _AnimeInfo edb_anim[22] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x170D, 0x000A, 0x0180, 0x0000, 0x0009, 1 },
    { 0x170E, 0x0018, 0x0800, 0x000A, 0x0021, 1 },
    { 0x170F, 0x0018, 0x0800, 0x0022, 0x0039, 1 },
    { 0x1710, 0x0014, 0x0800, 0x003A, 0x004D, 0 },
    { 0x1711, 0x0014, 0x0800, 0x004E, 0x0061, 0 },
    { 0x1712, 0x0018, 0x0800, 0x0062, 0x0079, 0 },
    { 0x1713, 0x002A, 0x0800, 0x007A, 0x00A3, 0 },
    { 0x1714, 0x0021, 0x0800, 0x00A4, 0x00C4, 0 },
    { 0x1715, 0x003C, 0x0800, 0x00C5, 0x0100, 0 },
    { 0x1716, 0x000F, 0x0800, 0x0101, 0x010F, 0 },
    { 0x1717, 0x000F, 0x0800, 0x0110, 0x011E, 0 },
    { 0x1718, 0x000F, 0x0800, 0x011F, 0x012D, 0 },
    { 0x1719, 0x000F, 0x0800, 0x012E, 0x013C, 0 },
    { 0x171A, 0x000F, 0x0800, 0x013D, 0x014B, 0 },
    { 0x171B, 0x000F, 0x0800, 0x014C, 0x015A, 0 },
    { 0x171C, 0x000F, 0x0800, 0x015B, 0x0169, 0 },
    { 0x171D, 0x000F, 0x0800, 0x016A, 0x0178, 0 },
    { 0x171E, 0x000F, 0x0800, 0x0179, 0x0187, 0 },
    { 0x171F, 0x000F, 0x0800, 0x0188, 0x0196, 0 },
    { 0x1720, 0x000F, 0x0800, 0x0197, 0x01A5, 0 },
    { 0x1721, 0x000F, 0x0800, 0x01A6, 0x01B4, 0 },
};

static const struct _AnimeInfo d_edb_anim[2] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1A91, 0x004C, 0x0800, 0x0000, 0x004B, 0 },
};

/* Matching: the DWARF drops this unused parameter (see m3_arm); it keeps `this` live in a0. */
static int EnemyEDBInit(struct SubCharacter *scp) {
    return 0;
}

static void EnemyEDBFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        vcopy(&this->pos, pos);
        vcopy(&this->rot, rot);
        if (!PlayerNowDemoEventMode()) {
            EnemyEDBInit(this);
            SCAnimeTypeSwitch(this, 1);
            shCharacterEnemyEDBAnimeSetP(this, 0x170D, 1);
            if (this->status & 4) {
                if ((dp = enEntryEnemy(9)) != NULL) {
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

/** Installs the EDB update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyEDBLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyEDBFunction);
}

/**
 * Plays drama (event) animation @p anime_id on a EDB model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a EDB model.
 */
int shCharacterEnemyEDBAnimeSet(struct SubCharacter *scp, int anime_id) {
    short id;
    struct _AnimeInfo *aip;

    id = shCharacterGetModelID(scp);
    if (id == 0x205) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_edb_anim[anime_id - 0x1A90];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dedb_anime_adr_list[anime_id - 0x1A90] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x1A90));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a EDB model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a EDB model.
 */
int shCharacterEnemyEDBAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x205) {
        aip = (struct _AnimeInfo *)&edb_anim[anime_id - 0x170C];
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
/*
 * Matching: MWCC evaluates converted arguments first. In case 0x34 the
 * original passes mat.d unconverted (a0 set before a1), but the lw_mat pointer and the
 * pos0/pos1 source vectors converted (a1, a2, then a0).
 */
void shGetEnemyEDBAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
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
    case 0x34:
        sk_num = 31;
        i = 0;
        while (i < sk_num) {
            stp = stp->next;
            i++;
        }
        mat = stp->src_m;
        mat.d[3][0] = 0.0f;
        mat.d[3][3] = 1.0f;
        pos0[0] = 208.0f;
        pos0[1] = -26.0f;
        pos0[2] = -68.0f;
        pos0[3] = 0.0f;
        sceVu0ApplyMatrix(pos0, mat.d, pos0);
        pos0[0] += stp->src_m.d[3][0];
        pos0[1] += stp->src_m.d[3][1];
        pos0[2] += stp->src_m.d[3][2];
        pos0[3] = stp->src_m.d[3][3];
        sceVu0ApplyMatrix(s_pos, (float (*)[4])&lw_mat, (float *)&pos0);
        pos1[0] = 511.0f;
        pos1[1] = -64.0f;
        pos1[2] = -88.0f;
        pos1[3] = 0.0f;
        sceVu0ApplyMatrix(pos1, mat.d, pos1);
        pos1[0] += stp->src_m.d[3][0];
        pos1[1] += stp->src_m.d[3][1];
        pos1[2] += stp->src_m.d[3][2];
        pos1[3] = 1.0f;
        sceVu0ApplyMatrix(s_vec, (float (*)[4])&lw_mat, (float *)&pos1);
        s_vec[0] = s_vec[0] - s_pos[0];
        s_vec[1] -= s_pos[1];
        s_vec[2] -= s_pos[2];
        _shNormalize(s_vec, s_vec);
        break;
    case 0x35:
        sk_num = 27;
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
        break;
    }
}
