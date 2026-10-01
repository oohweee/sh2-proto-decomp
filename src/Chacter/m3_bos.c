/*
 * BOS enemy character (model 0x206): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * Mary, final boss (verified; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static int dbos_anime_adr_list[21] = { 0, 0x3B154, 0x3D910, 0x4186C, 0x44AA8, 0x46CDC, 0x1FBA8, 0x3B154, 0x3D9E8, 0x42B2C, 0x46CB0, 0x4BD04, 0x3B154, 0x3E1B0, 0x41D1C, 0x443A0, 0x4730C, 0x3B154, 0x3D2C8, 0x4171C, 0x46020 };

static const struct _AnimeInfo bos_anim[10] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x16A9, 0x003B, 0x0800, 0x0000, 0x003A, 1 },
    { 0x16AA, 0x000F, 0x0800, 0x003B, 0x0049, 0 },
    { 0x16AB, 0x000F, 0x0400, 0x004A, 0x0058, 0 },
    { 0x16AC, 0x0018, 0x0800, 0x0059, 0x0070, 1 },
    { 0x16AD, 0x0018, 0x0800, 0x0071, 0x0088, 0 },
    { 0x16AE, 0x0009, 0x0800, 0x0089, 0x0091, 0 },
    { 0x16AF, 0x001B, 0x0800, 0x0092, 0x00AC, 0 },
    { 0x16B0, 0x0018, 0x0800, 0x00AD, 0x00C4, 0 },
    { 0x16B1, 0x001F, 0x0800, 0x00C5, 0x00E3, 1 },
};

static const struct _AnimeInfo d_bos_anim[21] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1A2D, 0x0191, 0x0800, 0x0000, 0x0190, 0 },
    { 0x1A2E, 0x028D, 0x0800, 0x0000, 0x028C, 0 },
    { 0x1A2F, 0x0201, 0x0800, 0x0000, 0x0200, 0 },
    { 0x1A30, 0x0156, 0x0800, 0x0000, 0x0155, 0 },
    { 0x1A31, 0x00BC, 0x0800, 0x0000, 0x00BB, 0 },
    { 0x1A32, 0x018F, 0x0800, 0x0000, 0x018E, 0 },
    { 0x1A33, 0x019A, 0x0800, 0x0000, 0x0199, 0 },
    { 0x1A34, 0x034C, 0x0800, 0x0000, 0x034B, 0 },
    { 0x1A35, 0x02A4, 0x0800, 0x0000, 0x02A3, 0 },
    { 0x1A36, 0x0342, 0x0800, 0x0000, 0x0341, 0 },
    { 0x1A37, 0x00BC, 0x0800, 0x0000, 0x00BB, 0 },
    { 0x1A38, 0x01ED, 0x0800, 0x0000, 0x01EC, 0 },
    { 0x1A39, 0x0263, 0x0800, 0x0000, 0x0262, 0 },
    { 0x1A3A, 0x0184, 0x0800, 0x0000, 0x0183, 0 },
    { 0x1A3B, 0x01E3, 0x0800, 0x0000, 0x01E2, 0 },
    { 0x1A3C, 0x00BC, 0x0800, 0x0000, 0x00BB, 0 },
    { 0x1A3D, 0x014E, 0x0800, 0x0000, 0x014D, 0 },
    { 0x1A3E, 0x02C2, 0x0800, 0x0000, 0x02C1, 0 },
    { 0x1A3F, 0x02F4, 0x0800, 0x0000, 0x02F3, 0 },
    { 0x1A40, 0x00BC, 0x0800, 0x0000, 0x00BC, 0 },
};

/*
 * Matching: the DWARF drops this unused parameter. Passing `this` keeps it live in a0 up
 * to the call (MWCC knows this static callee clobbers nothing else), so the switch on
 * this->step is loaded into a1.
 */
static int EnemyBOSInit(struct SubCharacter *scp) {
    return 0;
}

static void EnemyBOSFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        vcopy(&this->pos, pos);
        vcopy(&this->rot, rot);
        EnemyBOSInit(this);
        if (!PlayerNowDemoEventMode()) {
            SCAnimeTypeSwitch(this, 1);
            shCharacterEnemyBOSAnimeSetP(this, 0x16A9, 1);
            if (this->status & 4) {
                if ((dp = enEntryEnemy(11)) != NULL) {
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

/** Installs the BOS update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyBOSLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyBOSFunction);
}

/**
 * Plays drama (event) animation @p anime_id on a BOS model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a BOS model.
 */
int shCharacterEnemyBOSAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x206) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_bos_anim[anime_id - 0x1A2C];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dbos_anime_adr_list[anime_id - 0x1A2C] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x1A2C));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a BOS model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a BOS model.
 */
int shCharacterEnemyBOSAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x206) {
        aip = (struct _AnimeInfo *)&bos_anim[anime_id - 0x16A8];
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
void shGetEnemyBOSAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
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
    if (atk == 0x3E || atk == 0x3F) {
        s_pos[0] = sh2jms.player->pos.x;
        s_pos[1] = sh2jms.player->pos.y - 1000.0f;
        s_pos[2] = sh2jms.player->pos.z;
        s_pos[3] = 1.0f;
        s_vec[0] = 0.0f;
        s_vec[1] = 1.0f;
        s_vec[2] = 0.0f;
        s_vec[3] = 1.0f;
        return;
    }
    sk_num = 44;
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
